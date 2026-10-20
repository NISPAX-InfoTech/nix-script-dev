# NixScript per-user library and package paths.
export NIXSCRIPT_HOME="${NIXSCRIPT_HOME:-$HOME/.nixscript}"
export NIXSCRIPT_LIB="${NIXSCRIPT_LIB:-$NIXSCRIPT_HOME/lib}"
export NIXSCRIPT_INCLUDE="${NIXSCRIPT_INCLUDE:-$NIXSCRIPT_HOME/include}"
export NIXSCRIPT_PACKAGES="${NIXSCRIPT_PACKAGES:-$NIXSCRIPT_HOME/packages}"
if [ -n "${HOME:-}" ]; then mkdir -p "$NIXSCRIPT_LIB" "$NIXSCRIPT_INCLUDE" "$NIXSCRIPT_PACKAGES" 2>/dev/null || true; fi
