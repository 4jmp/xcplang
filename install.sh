#!/usr/bin/env bash
set -e

case "$(uname -s)" in
  Linux|FreeBSD|OpenBSD|NetBSD) ;;
  *) echo "xcplang installer supports Linux and BSD only" >&2; exit 1 ;;
esac

pick() {
  local title="$1"; local state=0; local key
  while true; do
    if [ "$state" -eq 1 ]; then printf '\r\033[2K[X] %s' "$title"; else printf '\r\033[2K[ ] %s' "$title"; fi
    IFS= read -r -s -n 1 key
    case "$key" in
      ' ') state=$((1-state)) ;;
      '') printf '\n'; PICK_RESULT="$state"; return 0 ;;
      q|Q) printf '\n'; PICK_RESULT=0; return 0 ;;
    esac
  done
}

pick 'Install xcplang (Space toggles, Enter confirms, q cancels)'
if [ "$PICK_RESULT" -ne 1 ]; then echo 'Installation cancelled by user.'; exit 0; fi

bin_dir="$HOME/.xcplang/bin"
mkdir -p "$bin_dir"
if ! command -v cargo >/dev/null 2>&1; then
  echo 'xcplang error: cargo is required for the rust discord gateway' >&2
  exit 1
fi
echo 'building rust discord gateway...'
if ! cargo build --release --bin xcpgateway; then
  echo 'xcplang error: rust gateway build failed; check cargo and network access' >&2
  exit 1
fi
if [ -x "./xcp" ]; then
  cp ./xcp "$bin_dir/xcp"
elif command -v meson >/dev/null 2>&1 && command -v ninja >/dev/null 2>&1; then
  meson setup build >/dev/null 2>&1 || true
  meson compile -C build >/dev/null
  cp build/xcp "$bin_dir/xcp"
else
  make >/dev/null
  cp xcp "$bin_dir/xcp"
fi
cp target/release/xcpgateway "$bin_dir/xcpgateway"

fix_path() {
  local profile="$1"; local temp
  [ -f "$profile" ] || : > "$profile"
  temp="$profile.xcp.$$"
  awk '$0 != "export PATH=\"$HOME/.xcplang/bin:$PATH\"" { print }' "$profile" > "$temp"
  printf 'export PATH="$HOME/.xcplang/bin:$PATH"\n' >> "$temp"
  mv "$temp" "$profile"
}

pick 'Add xcplang to PATH (Space toggles, Enter confirms, q skips)'
if [ "$PICK_RESULT" -eq 1 ]; then
  profile="$HOME/.profile"
  [ -n "${BASH_VERSION:-}" ] && profile="$HOME/.bashrc"
  [ -n "${ZSH_VERSION:-}" ] && profile="$HOME/.zshrc"
  fix_path "$profile"
fi
echo "xcplang installed: $bin_dir/xcp"
