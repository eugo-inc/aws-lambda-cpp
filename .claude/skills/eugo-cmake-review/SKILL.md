---
name: eugo-cmake-review
description: Pre-commit review checklist for CMakeLists.txt (and source-divergence) changes in the Eugo aws-lambda-cpp fork - the protomolecule patch-anchor invariant, @EUGO_CHANGE annotation discipline, the shared-lib/visibility rules, and the export-set names the awslambdaric consumer depends on. Activates on "review cmake change", "check aws-lambda-cpp CMakeLists", "patch anchor", "EUGO_CHANGE aws-lambda-cpp", "pre-commit cmake audit".
---

# Eugo CMake review (aws-lambda-cpp, pre-commit)

There is no sync_audit.sh in this repo - review is by eye against this list.
The stakes are concentrated in one file (`CMakeLists.txt`, ~148 lines) plus the
three patched source/header files.

## 1. Patch-anchor invariant (the #1 breaker)

`protomolecule/dependencies/native/aws/aws_lambda_cpp/setup` regex-patches
CMakeLists.txt at build time with `*_or_die` helpers. These EXACT lines are
anchors and must each remain on their own line (the regexes tolerate leading /
trailing whitespace, nothing else):

```
    "-fno-exceptions"
    "-fno-rtti"
    "-Werror"
set(CMAKE_CXX_STANDARD 11)
```

Do NOT delete, reformat, merge onto one line, or conditionally wrap them - the
repo deliberately keeps upstream's form and lets protomolecule strip them at
build time (keeps the diff-vs-upstream minimal). If a change genuinely must
alter an anchor, update the `setup` regexes in the same change and say so in
the commit message.

## 2. Divergences that must not regress

- `#"-fvisibility=hidden"` stays commented out (line ~77). Re-enabling it hides
  the symbols awslambdaric's dynamically-linked `runtime_client.so` needs.
- `add_library(${PROJECT_NAME} ...)` stays keyword-less (no STATIC/SHARED) so
  `-DBUILD_SHARED_LIBS=ON` from the eugo build controls the type.
- Keep `SOVERSION 0` / `VERSION` properties - consumers link the `.so.0` chain.
- Export set + namespace are load-bearing: `aws-lambda-runtime-targets`,
  namespace `AWS::`, config package installed to
  `${CMAKE_INSTALL_LIBDIR}/aws-lambda-runtime/cmake/`. awslambdaric's meson
  resolves `dependency('aws-lambda-runtime', method: 'cmake')` against exactly
  this. Renaming the project/target/export path breaks the consumer silently
  until its next build.
- Install destinations keep upstream's `GNUInstallDirs` form; `lib64` comes
  from `CMAKE_INSTALL_LIBDIR` in `EUGO_CMAKE_COMMON_OPTIONS`. This repo has no
  pytorch-style `@CIDTATWB` / `EUGO_*_INSTALL_*` rewrites - do not invent them.
- `ENABLE_LTO` / `ENABLE_SANITIZERS` / `ENABLE_TESTS` option declarations stay:
  the eugo build passes explicit `OFF` for all three (LTO comes from eugo's own
  C/CXX/LDFLAGS, tests need the unprovisioned AWS C++ SDK).

## 3. New dependencies

A new `find_package(X)` only works in the eugo build if X exists as a
protomolecule native package AND is declared in
`dependencies/native/aws/aws_lambda_cpp/meta.json` (`dependencies.runtime`,
like `native/curl`). Undeclared deps mean the build DAG never provisions the
package and configure dies in-container. The optional Backtrace/libdw/libbfd
block is intentionally best-effort: libdw comes via `native/elfutils`, libbfd
is deliberately NOT provisioned - do not promote either to REQUIRED.

## 4. @EUGO_CHANGE annotation discipline

Every NEW divergence from upstream awslabs/aws-lambda-cpp gets an annotation
with a WHY (inline `// @EUGO_CHANGE: <reason>` or a
`// @EUGO_CHANGE: @begin <reason>` ... `// @EUGO_CHANGE: @end` block).
Current annotated population: `include/aws/lambda-runtime/runtime.h` (tenant_id
field) and `src/runtime.cpp` (TENANT_ID_HEADER + one block in `get_next()`).

KNOWN GAP: the older awslambdaric patches (runtime_response base class, xray
plumbing, `get_content_type()` in `include/aws/http/response.h`) are
UNANNOTATED. When a diff touches those regions, do not mistake them for
upstream code and do not lose them; annotating them while you are there is
encouraged. Provenance: eugo-inc/aws-lambda-python-runtime-interface-client
`deps/patches`.

## 5. Before approving

- Configure check passes with the eugo flag set (see `eugo-rebuild`).
- `grep -n '"-Werror"\|"-fno-exceptions"\|"-fno-rtti"\|CMAKE_CXX_STANDARD 11' CMakeLists.txt`
  still shows all four anchors, unmodified.
- If the diff touches `include/aws/**`: flag the awslambdaric ABI impact in
  review (see `eugo-rebuild`).
- If the divergence set changed: update the inventory in the
  `eugo-upstream-merge` skill so the next sync preserves it.

## Related

- `eugo-build-and-test`, `eugo-rebuild`, `eugo-upstream-merge`
