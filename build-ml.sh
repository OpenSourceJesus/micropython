#!/usr/bin/env bash
# Build and test the ml unix variant (ulab + scipy + torch_c).
# Safe to run from any working directory.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NUMERIC="${ROOT}/lib/numeric-modules"
UNIX="${ROOT}/ports/unix"
MPY="${UNIX}/build-ml/micropython"
TEST="${ROOT}/tests/ml/integration.py"
MPL_TEST="${ROOT}/tests/ml/matplotlib_integration.py"

mkdir -p "${NUMERIC}"
ln -sfn ../../micropython-ulab/code "${NUMERIC}/ulab"
ln -sfn ../cml-micropython/cml "${NUMERIC}/cml"

if [[ ! -d "${UNIX}" ]]; then
    echo "error: expected unix port at ${UNIX}" >&2
    exit 1
fi

make -C "${UNIX}" VARIANT=ml submodules
make -C "${UNIX}" VARIANT=ml "$@"

if [[ "${1:-}" != "clean" ]]; then
    cd "${ROOT}"
    "${MPY}" "${TEST}"
    "${MPY}" "${MPL_TEST}"
fi
