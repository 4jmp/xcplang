#include "xcp/runtime.hpp"
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <netinet/in.h>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace xcp {
struct Token {
  std::string text;
  int line;
};
static std::vector<Token> scan(const std::string &source) {
  std::vector<Token> out;
  int line = 1;
  for (size_t i = 0; i < source.size();) {
    char c = source[i];
    if (c == '\n') {
      ++line;
      ++i;
      continue;
    }
    if (std::isspace((unsigned char)c)) {
      ++i;
      continue;
    }
    if (c == '#' ||
        (c == '/' && i + 1 < source.size() && source[i + 1] == '/')) {
      while (i < source.size() && source[i] != '\n')
        ++i;
      continue;
    }
    if (c == '/' && i + 1 < source.size() && source[i + 1] == '*') {
      i += 2;
      while (i < source.size()) {
        if (source[i] == '\n')
          ++line;
        if (i + 1 < source.size() && source[i] == '*' && source[i + 1] == '/') {
          i += 2;
          break;
        }
        ++i;
      }
      continue;
    }
    if (c == '"' || c == '\'') {
      char q = c;
      ++i;
      std::string s;
      while (i < source.size() && source[i] != q) {
        if (source[i] == '\\' && i + 1 < source.size()) {
          ++i;
          char esc = source[i++];
          s += esc == 'n' ? '\n' : esc == 't' ? '\t' : esc;
        } else
          s += source[i++];
      }
      if (i < source.size())
        ++i;
      out.push_back({"\"" + s + "\"", line});
      continue;
    }
    if (std::isdigit((unsigned char)c) ||
        (c == '.' && i + 1 < source.size() &&
         std::isdigit((unsigned char)source[i + 1]))) {
      size_t b = i++;
      while (i < source.size() &&
             (std::isdigit((unsigned char)source[i]) || source[i] == '.'))
        ++i;
      out.push_back({source.substr(b, i - b), line});
      continue;
    }
    if (std::isalpha((unsigned char)c) || c == '_') {
      size_t b = i++;
      while (i < source.size() &&
             (std::isalnum((unsigned char)source[i]) || source[i] == '_'))
        ++i;
      out.push_back({source.substr(b, i - b), line});
      continue;
    }
    if (i + 1 < source.size()) {
      std::string two = source.substr(i, 2);
      if (two == "==" || two == "!=" || two == "<=" || two == ">=" ||
          two == "&&" || two == "||") {
        out.push_back({two, line});
        i += 2;
        continue;
      }
    }
    out.push_back({std::string(1, c), line});
    ++i;
  }
  out.push_back({"<eof>", line});
  return out;
}

struct Value;
struct Env;
struct Stmt;
struct Callable;
using Array = std::vector<Value>;
using StmtPtr = std::shared_ptr<Stmt>;
struct Value {
  using Data = std::variant<std::monostate, double, bool, std::string,
                            std::shared_ptr<Array>, std::shared_ptr<Callable>>;
  Data data;
  Value() = default;
  template <class T> Value(T v) : data(std::move(v)) {}
};
static std::string text(const Value &v);
static bool yes(const Value &v) {
  if (auto p = std::get_if<bool>(&v.data))
    return *p;
  if (auto p = std::get_if<double>(&v.data))
    return *p != 0;
  if (auto p = std::get_if<std::string>(&v.data))
    return !p->empty();
  if (auto p = std::get_if<std::shared_ptr<Array>>(&v.data))
    return !(*p)->empty();
  return !std::holds_alternative<std::monostate>(v.data);
}
static double num(const Value &v) {
  if (auto p = std::get_if<double>(&v.data))
    return *p;
  throw std::runtime_error("number required");
}
static std::string text(const Value &v) {
  if (std::holds_alternative<std::monostate>(v.data))
    return "null";
  if (auto p = std::get_if<double>(&v.data)) {
    std::ostringstream o;
    o << *p;
    return o.str();
  }
  if (auto p = std::get_if<bool>(&v.data))
    return *p ? "true" : "false";
  if (auto p = std::get_if<std::string>(&v.data))
    return *p;
  if (auto p = std::get_if<std::shared_ptr<Array>>(&v.data)) {
    std::string s = "[";
    for (size_t i = 0; i < (*p)->size(); ++i) {
      if (i)
        s += ", ";
      s += text((*p)->at(i));
    }
    return s + "]";
  }
  return "<fn>";
}
static bool same(const Value &a, const Value &b) {
  if (a.data.index() != b.data.index())
    return false;
  if (std::holds_alternative<std::monostate>(a.data))
    return true;
  if (auto p = std::get_if<double>(&a.data))
    return *p == std::get<double>(b.data);
  if (auto p = std::get_if<bool>(&a.data))
    return *p == std::get<bool>(b.data);
  if (auto p = std::get_if<std::string>(&a.data))
    return *p == std::get<std::string>(b.data);
  return text(a) == text(b);
}

struct Env : std::enable_shared_from_this<Env> {
  std::shared_ptr<Env> parent;
  std::unordered_map<std::string, Value> values;
  explicit Env(std::shared_ptr<Env> p = {}) : parent(std::move(p)) {}
  void put(const std::string &n, Value v) { values[n] = std::move(v); }
  Value get(const std::string &n) {
    auto i = values.find(n);
    if (i != values.end())
      return i->second;
    if (parent)
      return parent->get(n);
    throw std::runtime_error("unknown name: " + n);
  }
  void set(const std::string &n, Value v) {
    if (values.count(n)) {
      values[n] = std::move(v);
      return;
    }
    if (parent) {
      parent->set(n, std::move(v));
      return;
    }
    values[n] = std::move(v);
  }
};
struct ReturnSignal {
  Value value;
};
struct BreakSignal {};
struct Callable {
  virtual ~Callable() = default;
  virtual Value call(const std::vector<Value> &) = 0;
};
struct Expr {
  virtual ~Expr() = default;
  virtual Value eval(std::shared_ptr<Env>) = 0;
};
using ExprPtr = std::shared_ptr<Expr>;
struct Stmt {
  virtual ~Stmt() = default;
  virtual void run(std::shared_ptr<Env>) = 0;
};
struct Literal : Expr {
  Value value;
  explicit Literal(Value v) : value(std::move(v)) {}
  Value eval(std::shared_ptr<Env>) override { return value; }
};
struct Name : Expr {
  std::string name;
  explicit Name(std::string n) : name(std::move(n)) {}
  Value eval(std::shared_ptr<Env> e) override { return e->get(name); }
};
struct ArrayExpr : Expr {
  std::vector<ExprPtr> items;
  Value eval(std::shared_ptr<Env> e) override {
    auto a = std::make_shared<Array>();
    for (auto &x : items)
      a->push_back(x->eval(e));
    return a;
  }
};
struct Unary : Expr {
  std::string op;
  ExprPtr right;
  Unary(std::string o, ExprPtr r) : op(std::move(o)), right(std::move(r)) {}
  Value eval(std::shared_ptr<Env> e) override {
    auto v = right->eval(e);
    if (op == "!")
      return !yes(v);
    return -num(v);
  }
};
struct Binary : Expr {
  ExprPtr left, right;
  std::string op;
  Binary(ExprPtr l, std::string o, ExprPtr r)
      : left(std::move(l)), right(std::move(r)), op(std::move(o)) {}
  Value eval(std::shared_ptr<Env> e) override {
    auto a = left->eval(e);
    if (op == "&&")
      return yes(a) && yes(right->eval(e));
    if (op == "||")
      return yes(a) || yes(right->eval(e));
    auto b = right->eval(e);
    if (op == "+") {
      if (std::holds_alternative<std::string>(a.data) ||
          std::holds_alternative<std::string>(b.data))
        return text(a) + text(b);
      return num(a) + num(b);
    }
    if (op == "-")
      return num(a) - num(b);
    if (op == "*")
      return num(a) * num(b);
    if (op == "/")
      return num(a) / num(b);
    if (op == "==")
      return same(a, b);
    if (op == "!=")
      return !same(a, b);
    if (op == "<")
      return num(a) < num(b);
    if (op == ">")
      return num(a) > num(b);
    if (op == "<=")
      return num(a) <= num(b);
    return num(a) >= num(b);
  }
};
struct Call : Expr {
  ExprPtr callee;
  std::vector<ExprPtr> args;
  Value eval(std::shared_ptr<Env> e) override {
    auto c = callee->eval(e);
    auto f = std::get_if<std::shared_ptr<Callable>>(&c.data);
    if (!f || !*f)
      throw std::runtime_error("value is not a function");
    std::vector<Value> a;
    for (auto &x : args)
      a.push_back(x->eval(e));
    return (*f)->call(a);
  }
};
struct Index : Expr {
  ExprPtr target, index;
  Index(ExprPtr t, ExprPtr i) : target(std::move(t)), index(std::move(i)) {}
  Value eval(std::shared_ptr<Env> e) override {
    auto a = std::get<std::shared_ptr<Array>>(target->eval(e).data);
    return a->at((size_t)num(index->eval(e)));
  }
};
struct ExprStmt : Stmt {
  ExprPtr expr;
  explicit ExprStmt(ExprPtr x) : expr(std::move(x)) {}
  void run(std::shared_ptr<Env> e) override { expr->eval(e); }
};
struct VarStmt : Stmt {
  std::string name;
  ExprPtr value;
  VarStmt(std::string n, ExprPtr v) : name(std::move(n)), value(std::move(v)) {}
  void run(std::shared_ptr<Env> e) override {
    e->put(name, value ? value->eval(e) : Value{});
  }
};
struct SetStmt : Stmt {
  std::string name;
  ExprPtr value;
  SetStmt(std::string n, ExprPtr v) : name(std::move(n)), value(std::move(v)) {}
  void run(std::shared_ptr<Env> e) override { e->set(name, value->eval(e)); }
};
struct Block : Stmt {
  std::vector<StmtPtr> body;
  void run(std::shared_ptr<Env> e) override {
    auto local = std::make_shared<Env>(e);
    for (auto &s : body)
      s->run(local);
  }
};
struct IfStmt : Stmt {
  ExprPtr test;
  StmtPtr yes_body, no_body;
  IfStmt(ExprPtr t, StmtPtr y, StmtPtr n)
      : test(std::move(t)), yes_body(std::move(y)), no_body(std::move(n)) {}
  void run(std::shared_ptr<Env> e) override {
    if (yes(test->eval(e)))
      yes_body->run(e);
    else if (no_body)
      no_body->run(e);
  }
};
struct WhileStmt : Stmt {
  ExprPtr test;
  StmtPtr body;
  WhileStmt(ExprPtr t, StmtPtr b) : test(std::move(t)), body(std::move(b)) {}
  void run(std::shared_ptr<Env> e) override {
    int guard = 0;
    while (yes(test->eval(e))) {
      try {
        body->run(e);
      } catch (BreakSignal &) {
        break;
      }
      if (++guard > 1000000)
        throw std::runtime_error("loop limit reached");
    }
  }
};
struct ReturnStmt : Stmt {
  ExprPtr value;
  explicit ReturnStmt(ExprPtr v) : value(std::move(v)) {}
  void run(std::shared_ptr<Env> e) override {
    throw ReturnSignal{value ? value->eval(e) : Value{}};
  }
};
struct BreakStmt : Stmt {
  void run(std::shared_ptr<Env>) override { throw BreakSignal{}; }
};
struct UserFn : Callable {
  std::vector<std::string> names;
  std::shared_ptr<Block> body;
  std::shared_ptr<Env> closure;
  Value call(const std::vector<Value> &args) override {
    auto e = std::make_shared<Env>(closure);
    for (size_t i = 0; i < names.size(); ++i)
      e->put(names[i], i < args.size() ? args[i] : Value{});
    try {
      body->run(e);
    } catch (ReturnSignal &r) {
      return r.value;
    }
    return {};
  }
};
struct FnStmt : Stmt {
  std::string name;
  std::vector<std::string> args;
  std::shared_ptr<Block> body;
  FnStmt(std::string n, std::vector<std::string> a, std::shared_ptr<Block> b)
      : name(std::move(n)), args(std::move(a)), body(std::move(b)) {}
  void run(std::shared_ptr<Env> e) override {
    auto f = std::make_shared<UserFn>();
    f->names = args;
    f->body = body;
    f->closure = e;
    e->put(name, f);
  }
};

class Parser {
  std::vector<Token> t;
  size_t i = 0;
  Token p() { return t[i]; }
  bool eat(const std::string &s) {
    if (p().text == s) {
      ++i;
      return true;
    }
    return false;
  }
  void need(const std::string &s) {
    if (!eat(s))
      throw std::runtime_error("line " + std::to_string(p().line) +
                               ": expected " + s);
  }
  ExprPtr expr() { return logic(); }
  ExprPtr logic() {
    auto a = equality();
    while (p().text == "&&" || p().text == "||") {
      auto o = p().text;
      ++i;
      a = std::make_shared<Binary>(a, o, equality());
    }
    return a;
  }
  ExprPtr equality() {
    auto a = compare();
    while (p().text == "==" || p().text == "!=") {
      auto o = p().text;
      ++i;
      a = std::make_shared<Binary>(a, o, compare());
    }
    return a;
  }
  ExprPtr compare() {
    auto a = term();
    while (p().text == "<" || p().text == ">" || p().text == "<=" ||
           p().text == ">=") {
      auto o = p().text;
      ++i;
      a = std::make_shared<Binary>(a, o, term());
    }
    return a;
  }
  ExprPtr term() {
    auto a = factor();
    while (p().text == "+" || p().text == "-") {
      auto o = p().text;
      ++i;
      a = std::make_shared<Binary>(a, o, factor());
    }
    return a;
  }
  ExprPtr factor() {
    auto a = unary();
    while (p().text == "*" || p().text == "/") {
      auto o = p().text;
      ++i;
      a = std::make_shared<Binary>(a, o, unary());
    }
    return a;
  }
  ExprPtr unary() {
    if (p().text == "!" || p().text == "-") {
      auto o = p().text;
      ++i;
      return std::make_shared<Unary>(o, unary());
    }
    return postfix();
  }
  ExprPtr postfix() {
    auto a = primary();
    while (true) {
      if (eat("(")) {
        auto c = std::make_shared<Call>();
        c->callee = a;
        if (p().text != ")")
          do {
            c->args.push_back(expr());
          } while (eat(","));
        need(")");
        a = c;
      } else if (eat("[")) {
        a = std::make_shared<Index>(a, expr());
        need("]");
      } else
        break;
    }
    return a;
  }
  ExprPtr primary() {
    auto x = p();
    if (eat("(")) {
      auto a = expr();
      need(")");
      return a;
    }
    if (eat("true"))
      return std::make_shared<Literal>(true);
    if (eat("false"))
      return std::make_shared<Literal>(false);
    if (eat("null"))
      return std::make_shared<Literal>(Value{});
    if (x.text.size() > 1 && x.text[0] == '"') {
      ++i;
      return std::make_shared<Literal>(x.text.substr(1, x.text.size() - 2));
    }
    if (std::isdigit((unsigned char)x.text[0]) || x.text[0] == '.') {
      ++i;
      return std::make_shared<Literal>(std::stod(x.text));
    }
    if (x.text == "[") {
      ++i;
      auto a = std::make_shared<ArrayExpr>();
      if (p().text != "]")
        do {
          a->items.push_back(expr());
        } while (eat(","));
      need("]");
      return a;
    }
    if (std::isalpha((unsigned char)x.text[0]) || x.text[0] == '_') {
      ++i;
      return std::make_shared<Name>(x.text);
    }
    throw std::runtime_error("line " + std::to_string(x.line) +
                             ": bad expression");
  }
  StmtPtr statement() {
    if (eat(";"))
      return std::make_shared<ExprStmt>(std::make_shared<Literal>(Value{}));
    if (eat("var") || eat("let")) {
      auto n = p().text;
      ++i;
      ExprPtr v;
      if (eat("="))
        v = expr();
      eat(";");
      return std::make_shared<VarStmt>(n, v);
    }
    if (eat("return")) {
      ExprPtr v = p().text == ";" || p().text == "}" ? nullptr : expr();
      eat(";");
      return std::make_shared<ReturnStmt>(v);
    }
    if (eat("break")) {
      eat(";");
      return std::make_shared<BreakStmt>();
    }
    if (eat("if")) {
      need("(");
      auto c = expr();
      need(")");
      auto y = statement();
      StmtPtr n;
      if (eat("else"))
        n = statement();
      return std::make_shared<IfStmt>(c, y, n);
    }
    if (eat("while")) {
      need("(");
      auto c = expr();
      need(")");
      return std::make_shared<WhileStmt>(c, statement());
    }
    if (eat("fn")) {
      auto n = p().text;
      ++i;
      need("(");
      std::vector<std::string> a;
      if (p().text != ")")
        do {
          a.push_back(p().text);
          ++i;
        } while (eat(","));
      need(")");
      auto b = std::dynamic_pointer_cast<Block>(statement());
      return std::make_shared<FnStmt>(n, a, b);
    }
    if (eat("{")) {
      auto b = std::make_shared<Block>();
      while (p().text != "}" && p().text != "<eof>")
        b->body.push_back(statement());
      need("}");
      return b;
    }
    if (std::isalpha((unsigned char)p().text[0]) && i + 1 < t.size() &&
        t[i + 1].text == "=") {
      auto n = p().text;
      i += 2;
      auto v = expr();
      eat(";");
      return std::make_shared<SetStmt>(n, v);
    }
    auto v = expr();
    eat(";");
    return std::make_shared<ExprStmt>(v);
  }

public:
  explicit Parser(std::vector<Token> x) : t(std::move(x)) {}
  std::vector<StmtPtr> program() {
    std::vector<StmtPtr> out;
    while (p().text != "<eof>")
      out.push_back(statement());
    return out;
  }
};
struct Native : Callable {
  std::function<Value(const std::vector<Value> &)> fn;
  explicit Native(std::function<Value(const std::vector<Value> &)> f)
      : fn(std::move(f)) {}
  Value call(const std::vector<Value> &a) override { return fn(a); }
};
static void builtins(const std::shared_ptr<Env> &e) {
  e->put("print", std::make_shared<Native>([](const auto &a) {
           for (auto &v : a)
             std::cout << text(v) << ' ';
           std::cout << '\n';
           return Value{};
         }));
  e->put("len", std::make_shared<Native>([](const auto &a) {
           if (a.empty())
             return Value(0.0);
           if (auto p = std::get_if<std::string>(&a[0].data))
             return Value((double)p->size());
           return Value(
               (double)std::get<std::shared_ptr<Array>>(a[0].data)->size());
         }));
  e->put("input", std::make_shared<Native>([](const auto &) {
           std::string s;
           std::getline(std::cin, s);
           return Value(s);
         }));
  e->put("push", std::make_shared<Native>([](const auto &a) {
           auto p = std::get<std::shared_ptr<Array>>(a.at(0).data);
           p->push_back(a.at(1));
           return Value{};
         }));
  e->put("pop", std::make_shared<Native>([](const auto &a) {
           auto p = std::get<std::shared_ptr<Array>>(a.at(0).data);
           if (p->empty())
             return Value{};
           auto v = p->back();
           p->pop_back();
           return v;
         }));
  e->put("abs", std::make_shared<Native>([](const auto &a) {
           return Value(std::abs(num(a.at(0))));
         }));
}
static std::string quote_shell(const std::string &s) {
  std::string out = "'";
  for (char c : s) {
    if (c == '\'')
      out += "'\\''";
    else
      out += c;
  }
  return out + "'";
}
static std::string json_quote(const std::string &s) {
  std::string out = "\"";
  for (char c : s) {
    if (c == '\"' || c == '\\')
      out += '\\';
    if (c == '\n')
      out += "\\n";
    else
      out += c;
  }
  return out + "\"";
}
static std::string shell_text(const std::string &command) {
  FILE *pipe = popen(command.c_str(), "r");
  if (!pipe)
    throw std::runtime_error("cannot start command");
  std::string output;
  char buffer[4096];
  while (fgets(buffer, sizeof(buffer), pipe))
    output += buffer;
  if (pclose(pipe) != 0)
    throw std::runtime_error("command failed");
  return output;
}

static bool destructive_command(const std::string &command) {
  static const std::regex rm_command(
      R"((^|[;&|])\s*(sudo\s+)?([^\s;&|]+/)?rm(\s|$))");
  std::string lower = command;
  for (char &c : lower)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  if (!std::regex_search(lower, rm_command))
    return false;

  bool recursive = lower.find("-r") != std::string::npos ||
                   lower.find("--recursive") != std::string::npos;
  if (!recursive)
    return false;
  if (lower.find("--no-preserve-root") != std::string::npos)
    return true;
  static const std::regex root_target(R"(\s/(?:\s|$|\*))");
  static const std::regex boot_target(R"(\s/boot(?:\s|$|\*))");
  return std::regex_search(lower, root_target) ||
         std::regex_search(lower, boot_target);
}

static std::string run_bash_command(const std::string &command,
                                    bool allow_destructive) {
  if (!allow_destructive && destructive_command(command)) {
    throw std::runtime_error(
        "destructive command blocked; use --allow-destructive or -d");
  }

  int output_pipe[2];
  if (pipe(output_pipe) != 0)
    throw std::runtime_error("cannot create command pipe");
  pid_t child = fork();
  if (child < 0) {
    close(output_pipe[0]);
    close(output_pipe[1]);
    throw std::runtime_error("cannot start command");
  }
  if (child == 0) {
    dup2(output_pipe[1], STDOUT_FILENO);
    dup2(output_pipe[1], STDERR_FILENO);
    close(output_pipe[0]);
    close(output_pipe[1]);
    execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<char *>(nullptr));
    _exit(127);
  }

  close(output_pipe[1]);
  std::string output;
  char buffer[4096];
  ssize_t count;
  while ((count = read(output_pipe[0], buffer, sizeof(buffer))) > 0) {
    output.append(buffer, static_cast<std::size_t>(count));
  }
  close(output_pipe[0]);

  int status = 0;
  waitpid(child, &status, 0);
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    throw std::runtime_error(output.empty() ? "command failed" : output);
  }
  return output;
}
static std::string secret_input() {
  std::string value;
  termios old_state{};
  bool hidden = tcgetattr(STDIN_FILENO, &old_state) == 0;
  if (hidden) {
    auto state = old_state;
    state.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &state);
  }
  std::getline(std::cin, value);
  if (hidden) {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_state);
    std::cerr << '\n';
  }
  return value;
}
static std::string curl_text(const std::string &method, const std::string &url,
                             const std::string &body) {
  std::string cmd = "curl -fsSL --max-time 30 -X " + quote_shell(method) + " " +
                    quote_shell(url);
  if (!body.empty())
    cmd += " -H 'Content-Type: application/json' --data " + quote_shell(body);
  return shell_text(cmd);
}
static int open_http_server(int port) {
  int server = socket(AF_INET, SOCK_STREAM, 0);
  if (server < 0)
    throw std::runtime_error("cannot create http server");
  int reuse = 1;
  setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(static_cast<uint16_t>(port));
  if (bind(server, reinterpret_cast<sockaddr *>(&address), sizeof(address)) <
      0) {
    close(server);
    throw std::runtime_error("cannot bind http server");
  }
  if (listen(server, 16) < 0) {
    close(server);
    throw std::runtime_error("cannot listen on http server");
  }
  return server;
}
static std::string request_path(int client) {
  char buffer[4096]{};
  ssize_t size = recv(client, buffer, sizeof(buffer) - 1, 0);
  std::istringstream line(std::string(buffer, size > 0 ? size : 0));
  std::string method, path, version;
  line >> method >> path >> version;
  auto query = path.find('?');
  if (query != std::string::npos)
    path.resize(query);
  return path;
}
static void send_http(int client, const std::string &body,
                      const std::string &type = "text/plain; charset=utf-8",
                      const std::string &status = "200 OK") {
  std::string response = "HTTP/1.1 " + status + "\r\nContent-Type: " + type +
                         "\r\nContent-Length: " + std::to_string(body.size()) +
                         "\r\nConnection: close\r\n\r\n" + body;
  send(client, response.data(), response.size(), 0);
  close(client);
}
static void serve_http(int port, const std::shared_ptr<Callable> &handler) {
  int server = open_http_server(port);
  for (;;) {
    int client = accept(server, nullptr, nullptr);
    if (client < 0)
      continue;
    auto body = text(handler->call({Value(request_path(client))}));
    send_http(client, body);
  }
}
static std::pair<std::string, std::string>
static_file(const std::string &directory, const std::string &path) {
  if (path.find("..") != std::string::npos)
    return {"not found", "text/plain; charset=utf-8"};
  auto relative =
      path == "/" ? "index.html" : path.substr(path[0] == '/' ? 1 : 0);
  auto file = std::filesystem::path(directory) / relative;
  std::ifstream in(file, std::ios::binary);
  if (!in)
    return {"not found", "text/plain; charset=utf-8"};
  std::ostringstream body;
  body << in.rdbuf();
  auto ext = file.extension().string();
  auto type = ext == ".html" || ext == ".htm" ? "text/html; charset=utf-8"
              : ext == ".css"                 ? "text/css; charset=utf-8"
              : ext == ".js"                  ? "text/javascript; charset=utf-8"
                                              : "application/octet-stream";
  return {body.str(), type};
}
static void serve_files(int port, const std::string &directory) {
  int server = open_http_server(port);
  for (;;) {
    int client = accept(server, nullptr, nullptr);
    if (client < 0)
      continue;
    auto path = request_path(client);
    auto file = static_file(directory, path);
    send_http(client, file.first, file.second,
              file.first == "not found" ? "404 Not Found" : "200 OK");
  }
}
static void network_builtins(const std::shared_ptr<Env> &e,
                             bool allow_destructive) {
  e->put("run_bash",
         std::make_shared<Native>([allow_destructive](const auto &a) {
           return Value(run_bash_command(text(a.at(0)), allow_destructive));
         }));
  e->put("curl", std::make_shared<Native>([](const auto &a) {
           return Value(curl_text("GET", text(a.at(0)), ""));
         }));
  e->put("fetch", std::make_shared<Native>([](const auto &a) {
           return Value(curl_text("GET", text(a.at(0)), ""));
         }));
  e->put("rest_get", std::make_shared<Native>([](const auto &a) {
           return Value(curl_text("GET", text(a.at(0)), ""));
         }));
  e->put("rest_post", std::make_shared<Native>([](const auto &a) {
           return Value(curl_text("POST", text(a.at(0)), text(a.at(1))));
         }));
  e->put("http_get", std::make_shared<Native>([](const auto &a) {
           return Value(curl_text("GET", text(a.at(0)), ""));
         }));
  e->put("http_post", std::make_shared<Native>([](const auto &a) {
           return Value(curl_text("POST", text(a.at(0)), text(a.at(1))));
         }));
  e->put("http_serve", std::make_shared<Native>([](const auto &a) {
           serve_http(static_cast<int>(num(a.at(0))),
                      std::get<std::shared_ptr<Callable>>(a.at(1).data));
           return Value{};
         }));
  e->put("http_serve_files", std::make_shared<Native>([](const auto &a) {
           serve_files(static_cast<int>(num(a.at(0))), text(a.at(1)));
           return Value{};
         }));
  e->put("ws_send", std::make_shared<Native>([](const auto &a) {
           std::string cmd = "printf " + quote_shell(text(a.at(1))) +
                             " | websocat " + quote_shell(text(a.at(0)));
           return Value(shell_text(cmd));
         }));
  e->put("ws_open", std::make_shared<Native>(
                        [](const auto &a) { return Value(text(a.at(0))); }));
  e->put("ws_request", std::make_shared<Native>([](const auto &a) {
           std::string cmd = "printf " + quote_shell(text(a.at(1))) +
                             " | websocat " + quote_shell(text(a.at(0)));
           return Value(shell_text(cmd));
         }));
  e->put("genai_google", std::make_shared<Native>([](const auto &a) {
           std::string key = text(a.at(1)), prompt = text(a.at(0));
           std::string url = "https://generativelanguage.googleapis.com/v1beta/"
                             "models/gemini-2.0-flash:generateContent?key=" +
                             key;
           std::string body =
               "{\"contents\":[{\"parts\":[{\"text\":" + json_quote(prompt) +
               "}]}]}";
           return Value(curl_text("POST", url, body));
         }));
  e->put("google_gemini", std::make_shared<Native>([](const auto &a) {
           std::string prompt = text(a.at(0)),
                       key = a.size() > 1 ? text(a.at(1)) : "";
           return Value(curl_text(
               "POST",
               "https://generativelanguage.googleapis.com/v1beta/models/"
               "gemini-2.0-flash:generateContent?key=" +
                   key,
               "{\"contents\":[{\"parts\":[{\"text\":" + json_quote(prompt) +
                   "}]}]}"));
         }));
  e->put("google_genai", std::make_shared<Native>([](const auto &a) {
           if (a.size() < 2)
             throw std::runtime_error("google_genai needs prompt and token");
           std::string prompt = text(a.at(0)), token = text(a.at(1)),
                       model =
                           a.size() > 2 ? text(a.at(2)) : "gemini-3.6-flash";
           std::string url =
               "https://generativelanguage.googleapis.com/v1beta/models/" +
               model + ":generateContent?key=" + token;
           std::string body =
               "{\"contents\":[{\"parts\":[{\"text\":" + json_quote(prompt) +
               "}]}]}";
           return Value(curl_text("POST", url, body));
         }));
}
static void discord_builtins(const std::shared_ptr<Env> &e) {
  e->put("discord_bot_on", std::make_shared<Native>([](const auto &a) {
           auto token = text(a.at(0));
           if (token.empty()) {
             std::cerr << "discord token (hidden): ";
             token = secret_input();
           }
           if (token.empty())
             throw std::runtime_error("discord token is empty");
           auto handler =
               a.size() > 1 ? text(a.at(1)) : "discord/discord_gateway.xcp";
           std::string cmd = "if [ -x \"$HOME/.xcplang/bin/xcpgateway\" ]; "
                             "then \"$HOME/.xcplang/bin/xcpgateway\" " +
                             quote_shell(token) + " " + quote_shell(handler) +
                             "; else ./target/release/xcpgateway " +
                             quote_shell(token) + " " + quote_shell(handler) +
                             "; fi";
           return Value(shell_text(cmd));
         }));
  e->put("discord_send", std::make_shared<Native>([](const auto &a) {
           std::string token = text(a.at(0)), channel = text(a.at(1)),
                       message = text(a.at(2));
           std::string body = "{\"content\":" + json_quote(message) + "}";
           std::string cmd =
               "curl -fsSL --max-time 30 -X POST " +
               quote_shell("https://discord.com/api/v10/channels/" + channel +
                           "/messages") +
               " -H " + quote_shell("Authorization: Bot " + token) +
               " -H 'Content-Type: application/json' --data " +
               quote_shell(body);
           return Value(shell_text(cmd));
         }));
  e->put("discord_message_reply", std::make_shared<Native>([](const auto &a) {
           std::string token = text(a.at(0)), channel = text(a.at(1)),
                       message_id = text(a.at(2)), message = text(a.at(3));
           std::string body = "{\"content\":" + json_quote(message) +
                              ",\"message_reference\":{\"message_id\":" +
                              json_quote(message_id) + "}}";
           std::string cmd =
               "curl -fsSL --max-time 30 -X POST " +
               quote_shell("https://discord.com/api/v10/channels/" + channel +
                           "/messages") +
               " -H " + quote_shell("Authorization: Bot " + token) +
               " -H 'Content-Type: application/json' --data " +
               quote_shell(body);
           return Value(shell_text(cmd));
         }));
  e->put(
      "discord_interaction_reply", std::make_shared<Native>([](const auto &a) {
        std::string token = text(a.at(0)), id = text(a.at(1)),
                    interaction = text(a.at(2)), message = text(a.at(3));
        std::string body =
            "{\"type\":4,\"data\":{\"content\":" + json_quote(message) + "}}";
        std::string cmd =
            "curl -fsSL --max-time 30 -X POST " +
            quote_shell("https://discord.com/api/v10/interactions/" + id + "/" +
                        interaction + "/callback") +
            " -H 'Content-Type: application/json' --data " + quote_shell(body);
        return Value(shell_text(cmd));
      }));
  auto register_slash = [](const auto &a) {
    std::string token = text(a.at(0)), application = text(a.at(1)),
                guild = text(a.at(2)), name = text(a.at(3)),
                description = text(a.at(4));
    std::string body = "{\"name\":" + json_quote(name) +
                       ",\"description\":" + json_quote(description) +
                       ",\"type\":1}";
    std::string cmd =
        "curl -fsSL --max-time 30 -X POST " +
        quote_shell("https://discord.com/api/v10/applications/" + application +
                    "/guilds/" + guild + "/commands") +
        " -H " + quote_shell("Authorization: Bot " + token) +
        " -H 'Content-Type: application/json' --data " + quote_shell(body);
    return Value(shell_text(cmd));
  };
  e->put("discord_register_slashcommand",
         std::make_shared<Native>(register_slash));
}
static void text_builtins(const std::shared_ptr<Env> &e) {
  e->put("starts_with", std::make_shared<Native>([](const auto &a) {
           auto value = text(a.at(0)), prefix = text(a.at(1));
           return value.compare(0, prefix.size(), prefix) == 0;
         }));
}
static std::string imports(const std::string &source) {
  std::istringstream in(source);
  std::string line, out;
  while (std::getline(in, line)) {
    std::string s = line;
    size_t b = s.find_first_not_of(" \t");
    if (b != std::string::npos && s.compare(b, 6, "import") == 0) {
      auto q = s.find_first_of("\"'", b + 6);
      if (q != std::string::npos) {
        auto e = s.find(s[q], q + 1);
        std::ifstream f(s.substr(q + 1, e - q - 1));
        if (!f)
          throw std::runtime_error("cannot import " +
                                   s.substr(q + 1, e - q - 1));
        std::stringstream buf;
        buf << f.rdbuf();
        out += imports(buf.str());
        out += '\n';
        continue;
      }
    }
    out += line + '\n';
  }
  return out;
}
int run_source(const std::string &source, const std::string &filename) {
  return run_source(source, filename, false);
}

int run_source(const std::string &source, const std::string &filename,
               bool allow_destructive) {
  try {
    auto e = std::make_shared<Env>();
    builtins(e);
    network_builtins(e, allow_destructive);
    discord_builtins(e);
    text_builtins(e);
    Parser parser(scan(imports(source)));
    for (auto &s : parser.program())
      s->run(e);
    auto event = std::getenv("XCP_DISCORD_EVENT");
    if (event && std::string(event) == "message" &&
        e->values.count("on_message")) {
      auto f = std::get<std::shared_ptr<Callable>>(e->get("on_message").data);
      f->call({Value(std::getenv("XCP_DISCORD_COMMAND")
                         ? std::getenv("XCP_DISCORD_COMMAND")
                         : ""),
               Value(std::getenv("XCP_DISCORD_CHANNEL")
                         ? std::getenv("XCP_DISCORD_CHANNEL")
                         : ""),
               Value(std::getenv("XCP_DISCORD_PING")
                         ? std::getenv("XCP_DISCORD_PING")
                         : "0")});
    }
    if (event && std::string(event) == "slash" &&
        e->values.count("on_slash_command")) {
      auto f =
          std::get<std::shared_ptr<Callable>>(e->get("on_slash_command").data);
      f->call({Value(std::getenv("XCP_DISCORD_COMMAND")
                         ? std::getenv("XCP_DISCORD_COMMAND")
                         : ""),
               Value(std::getenv("XCP_DISCORD_CHANNEL")
                         ? std::getenv("XCP_DISCORD_CHANNEL")
                         : ""),
               Value(std::getenv("XCP_DISCORD_INTERACTION_ID")
                         ? std::getenv("XCP_DISCORD_INTERACTION_ID")
                         : ""),
               Value(std::getenv("XCP_DISCORD_INTERACTION_TOKEN")
                         ? std::getenv("XCP_DISCORD_INTERACTION_TOKEN")
                         : ""),
               Value(std::getenv("XCP_DISCORD_PING")
                         ? std::getenv("XCP_DISCORD_PING")
                         : "0")});
    }
    if ((!event || !*event) && e->values.count("register_commands")) {
      auto f =
          std::get<std::shared_ptr<Callable>>(e->get("register_commands").data);
      f->call({});
    }
    if ((!event || !*event) &&
        (e->values.count("on_message") ||
         e->values.count("on_slash_command")) &&
        e->values.count("token")) {
      auto bot = e->get("discord_bot_on");
      std::get<std::shared_ptr<Callable>>(bot.data)->call(
          {e->get("token"), Value(filename)});
    }
    return 0;
  } catch (const std::exception &x) {
    std::cerr << "xcp error: " << x.what() << '\n';
    return 1;
  }
}
}
