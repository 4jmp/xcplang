# network and ai

xcplang has small network helpers. They use the system `curl` command, so install curl before using them.

## shell commands

`run_bash(command)` runs a shell command in a child process, waits for it, and returns its output. It does not open a terminal or leave a process running in the background. Destructive commands targeting `/` or `/boot` are blocked unless the file is run with `--allow-destructive` or `-d`.

```xcp
print(run_bash("uname -s"))
```

This function can change files, start programs, and remove data. Only run trusted code.

## http and rest

`fetch(url)`, `curl(url)`, `http_get(url)`, and `rest_get(url)` make a GET request and return the response text.

`http_post(url, body)` and `rest_post(url, body)` send JSON or text as a POST body.

`http_serve_files(port, directory)` serves `index.html` and static HTML, CSS,
and JavaScript files from a local directory.

```xcp
var page = fetch("https://example.com")
print(len(page))
var reply = rest_post("https://api.example.com/items", "{\"name\":\"xcp\"}")
print(reply)
```

## websocket

`ws_open(url)` returns a URL handle. `ws_send(url, message)` and `ws_request(url, message)` use the optional `websocat` program for a real WebSocket request.

```xcp
var socket = ws_open("wss://echo.example.com")
print(ws_send(socket, "hello"))
```

Install `websocat` separately. A WebSocket connection needs a process that stays open, so use `websocat` for long sessions.

## google genai

`google_genai(prompt, token, model)` calls the Google Gemini REST endpoint and returns its JSON response. The model argument is optional. Its default is `gemini-3.6-flash`.

```xcp
var answer = google_genai("write one short hello", run_bash("printf $GEMINI_API_KEY"))
print(answer)
```

You can select a model:

```xcp
var answer = google_genai("make a short plan", token, "gemini-3.6-flash")
```

Do not put API keys in source files. Read them from a protected environment or secret manager.
