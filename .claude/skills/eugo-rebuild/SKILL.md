---
name: eugo-rebuild
description: Decide what level of rebuild a change to the Eugo aws-lambda-cpp fork actually requires - docs-only vs configure check vs full build - and when the protomolecule package pin must be bumped and consumers rebuilt. Activates on "rebuild aws-lambda-cpp", "do I need to rebuild the lambda runtime", "bump the aws_lambda_cpp pin", "is this a docs-only change", "rebuild awslambdaric".
---

# eugo-rebuild (aws-lambda-cpp)

The library is 4 TUs; a full clean build is seconds. The expensive part is never
the compile - it is the protomolecule pin bump and the downstream awslambdaric
rebuild. Walk the diff and pick the first matching row.

## Decision table

| Files changed | Action |
|---------------|--------|
| Docs only (`README.md`, `.claude/`, `.devcontainer/`, `.mcp.json`, `.vscode/`, `ci/`) | Nothing. No build, no pin bump. |
| `CMakeLists.txt`, `cmake/`, `tests/CMakeLists.txt` | Configure check first (below), then full build. ALSO verify the protomolecule patch anchors still match (see eugo-cmake-review) - a reformatted `"-Werror"` / `"-fno-exceptions"` / `"-fno-rtti"` / `set(CMAKE_CXX_STANDARD 11)` line kills the protomolecule build at patch time. |
| `src/*.cpp`, `src/backward.h`, `src/version.cpp.in` | Full build (seconds). Implementation-only; consumers pick it up at the next pin bump. |
| `include/aws/**` headers | Full build + ABI alarm: `runtime.h` is part of the ABI/API surface of awslambdaric's `runtime_client.so`. After adopting the pin, rebuild and retest `python/wave_4/awslambdaric`. |
| `packaging/packager` | No compile needed, but it installs into the CMake package dir - sanity-run `ninja install` and check the file lands. |

## Configure check (cheap gate for CMake changes)

```bash
mkdir -p build && cd build
cmake .. ${EUGO_CMAKE_COMMON_OPTIONS} -DENABLE_LTO=OFF \
  -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_SHARED_LIBS=ON \
  -DENABLE_SANITIZERS=OFF -DENABLE_TESTS=OFF
```

Catches broken `option()` declarations, `find_package(CURL)` failures, and
dangling install rules before you touch protomolecule.

## Full build

`ninja && ninja install` after the configure above - see `eugo-build-and-test`
for the parity caveat (the raw checkout lacks the protomolecule build-time
patches) and the healthy-artifact list.

## When protomolecule must react

Nothing in protomolecule changes until `dependencies/native/aws/aws_lambda_cpp/meta.json`
adopts a new `version.commit` (merge/land and adoption are decoupled; the pin has
`should_auto_update: true` but treat the bump as an explicit, reviewed step).
Order of operations:

1. Push the commit to `eugo-inc/aws-lambda-cpp` master FIRST (the setup fetches
   the tarball by sha; unpushed sha = 404). Never force-push master.
2. Bump `version.commit` in meta.json.
3. Rebuild `native/aws/aws_lambda_cpp`, run its testspec programs, then rebuild
   and retest `python/wave_4/awslambdaric` if headers changed.

## Related

- `eugo-build-and-test`, `eugo-cmake-review`, `eugo-upstream-merge`
