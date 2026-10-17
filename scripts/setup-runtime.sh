#!/usr/bin/env bash
# Populate ./runtime with OpenClaw for Local Gateway mode.
# Uses China npm mirror by default (official registry often unreachable in CN).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RUNTIME="${ROOT}/runtime"
MIRROR="${NPM_MIRROR:-https://registry.npmmirror.com}"

mkdir -p "${RUNTIME}/bin"
cd "${RUNTIME}"

if ! command -v npm >/dev/null 2>&1; then
  echo "npm not found. Install Node.js first, then re-run." >&2
  exit 1
fi

echo "Using npm registry: ${MIRROR}"
npm install openclaw@latest --registry="${MIRROR}" --prefix "${RUNTIME}"

OPENCLAW_BIN=""
if [[ -x "${RUNTIME}/node_modules/.bin/openclaw" ]]; then
  OPENCLAW_BIN="${RUNTIME}/node_modules/.bin/openclaw"
elif [[ -f "${RUNTIME}/node_modules/openclaw/dist/entry.js" ]]; then
  cat > "${RUNTIME}/bin/openclaw" << WRAP
#!/usr/bin/env bash
set -euo pipefail
NODE="\$(command -v node)"
exec "\${NODE}" "${RUNTIME}/node_modules/openclaw/dist/entry.js" "\$@"
WRAP
  chmod +x "${RUNTIME}/bin/openclaw"
  OPENCLAW_BIN="${RUNTIME}/bin/openclaw"
fi

if [[ -n "${OPENCLAW_BIN}" && ! -e "${RUNTIME}/bin/openclaw" ]]; then
  ln -sf "${OPENCLAW_BIN}" "${RUNTIME}/bin/openclaw"
fi

echo "Runtime ready under ${RUNTIME}"
echo "CLI: ${RUNTIME}/bin/openclaw (or node_modules/.bin/openclaw)"
"${RUNTIME}/bin/openclaw" --version 2>/dev/null || true
