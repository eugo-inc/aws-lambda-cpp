---
name: eugo-build-and-test
description: Use when building, installing, or smoke-testing the Eugo aws-lambda-cpp fork (libaws-lambda-runtime) - the protomolecule setup flow (sha tarball fetch, build-time CMakeLists patches, cmake flags), the local in-container build recipe, the healthy-install artifact list, and the testspec smoke programs. Activates on "build aws-lambda-cpp", "build the lambda runtime", "smoke test aws_lambda_cpp", "libaws-lambda-runtime missing", "verify lambda runtime build", "diagnose aws-lambda-cpp build failure".
---

# Build & test the Eugo aws-lambda-cpp fork

C++ AWS Lambda runtime library, 4 TUs (`logging.cpp`, `runtime.cpp`, `backward.cpp`,
generated `version.cpp`), one required dep (libcurl). Eugo builds it as a SHARED
system library so the awslambdaric fork's `runtime_client` extension can link it
dynamically. A full build is seconds - never optimize for incrementality here.

## How eugo actually builds it (protomolecule flow)

Canonical recipe: `protomolecule/dependencies/native/aws/aws_lambda_cpp/setup`
(helpers `eugo_log` / `eugo_patch_or_die` / `eugo_delete_lines_or_die` come from
`protomolecule/scripts/eugo_shared.inc`). Three steps:

1. **Fetch by pinned sha** - downloads
   `https://github.com/eugo-inc/aws-lambda-cpp/archive/<commit>.tar.gz` where
   `<commit>` is `meta.json`'s `version.commit`. Push to GitHub BEFORE bumping
   the pin or the fetch 404s.
2. **Build-time CMakeLists.txt patches** (they live in protomolecule, NOT in this
   repo - the repo keeps upstream's lines verbatim so the regexes keep matching):
   - delete the exact lines `"-Werror"`, `"-fno-exceptions"`, `"-fno-rtti"`
     (Eugo forbids all three; clang defaults - exceptions + RTTI ON - apply);
   - rewrite `set(CMAKE_CXX_STANDARD 11)` -> `${EUGO_CXX_STANDARD}` (gnu++23,
     `CMAKE_CXX_EXTENSIONS` default ON).
   Each patch is `*_or_die`: if upstream reformats/moves any anchor line, the
   protomolecule build fails at patch time (see eugo-cmake-review).
3. **Configure + build + install**:

```bash
cmake .. ${EUGO_CMAKE_COMMON_OPTIONS} \
  -DENABLE_LTO=OFF `# eugo applies its own LTO via C/CXX/LDFLAGS` \
  -DCMAKE_INSTALL_PREFIX="${EUGO_INSTALL_PREFIX_PATH}" `# /usr/local` \
  -DBUILD_SHARED_LIBS=ON \
  -DENABLE_SANITIZERS=OFF \
  -DENABLE_TESTS=OFF
ninja && ninja install
```

`EUGO_CMAKE_COMMON_OPTIONS` (set by the harness / `ben_life_easy.sh`) injects
`CMAKE_CXX_STANDARD`, `CMAKE_INSTALL_LIBDIR=lib64`, `CMAKE_BUILD_TYPE=Release`,
Ninja, PIC, and the clang++ toolchain prefix paths.

## Local build in the eugo container

Same commands from a fresh `build/` dir in the repo checkout. CRITICAL parity
caveat: a raw checkout still carries `-Werror`, `-fno-exceptions`, `-fno-rtti`,
and `CXX_STANDARD 11` (step 2 happens only in the protomolecule flow), so a
local build is NOT bit-identical to what ships - apply the same line deletions /
standard rewrite locally when you need to reproduce a shipped-build failure or
validate exceptions/RTTI-sensitive code. `ENABLE_TESTS` stays OFF everywhere:
upstream's gtest suite needs the AWS C++ SDK, which eugo does not provision.

## What healthy looks like (under /usr/local)

- `lib64/libaws-lambda-runtime.so` + its SOVERSION-0 symlink chain (shared, not `.a`)
- headers: `include/aws/lambda-runtime/{runtime,version,outcome}.h`,
  `include/aws/http/response.h`, `include/aws/logging/logging.h`
- CMake package: `lib64/aws-lambda-runtime/cmake/aws-lambda-runtime-{config,config-version}.cmake`
  + targets file exporting `AWS::aws-lambda-runtime`, plus the `packager` script

A missing config-package or a static `.a` instead of `.so` breaks the downstream
awslambdaric meson resolution (`dependency('aws-lambda-runtime', method: 'cmake')`).

## Smoke tests (the eugo verification, replacing upstream gtest)

`protomolecule/dependencies/tests/native/aws_lambda_cpp/` holds 4 compile+link+run
programs driven by `eugo_run_tests` via `testspec.json`: `main.cpp` (success
factory), `main_failure.cpp` (failure path incl. the fork's 3-arg `failure()`
with xray_response), `main_http_response.cpp` (header-only http surface),
`main_outcome.cpp`. Quick manual equivalent:

```bash
clang++ -std=gnu++23 main.cpp -laws-lambda-runtime -o /tmp/t && /tmp/t
# expect: "aws_lambda_cpp test OK"
```

The end-to-end consumer check is rebuilding `python/wave_4/awslambdaric` and
confirming `runtime_client.so` resolves `libaws-lambda-runtime` at import.

## Related

- `eugo-rebuild` - what level of rebuild a given diff needs
- `eugo-cmake-review` - pre-commit checklist incl. the patch-anchor invariant
- `eugo-upstream-merge` - merge recipe, divergence inventory, pin-bump procedure
