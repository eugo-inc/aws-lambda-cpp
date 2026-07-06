# CLAUDE.md

Eugo fork (`eugo-inc/aws-lambda-cpp`, default branch `eugo-main`) of
awslabs/aws-lambda-cpp, the C++ Lambda runtime (`libaws-lambda-runtime`).
Forked so the eugo `awslambdaric` fork can dynamically link it as a shared
system library; carries API patches from awslambdaric (tenant_id,
xray/runtime_response, `get_content_type`) and comments out
`-fvisibility=hidden`. protomolecule pins this repo by commit sha.

Durable knowledge lives in `.claude/skills/`. Route by what is happening:

- Building, smoke-testing, or diagnosing a build failure ->
  `eugo-build-and-test` (protomolecule setup flow, local recipe,
  healthy-install artifact list, testspec smoke programs).
- Wondering whether a diff needs a rebuild or a pin bump -> `eugo-rebuild`.
- About to commit a `CMakeLists.txt` or patched-source change ->
  `eugo-cmake-review`. NEVER reformat/delete the protomolecule patch-anchor
  lines -> keep upstream's form; each of these greps must return 1:
  `grep -cE '^\s*"-Werror"\s*$' CMakeLists.txt` (same for `"-fno-exceptions"`,
  `"-fno-rtti"`, `^set\(CMAKE_CXX_STANDARD 11\)`).
- About to merge upstream -> `eugo-upstream-merge`. Land merge-commit only;
  NEVER squash or force-push -> both invalidate shas pinned in protomolecule.

Traps the skills do not cover:

- `origin/awslabs-master` is the upstream mirror, not pristine -- do not treat
  it as pure upstream (see `eugo-upstream-merge`).
- Only the tenant_id patch carries `@EUGO_CHANGE` markers (`src/runtime.cpp`,
  `include/aws/lambda-runtime/runtime.h`). The older awslambdaric patches
  (runtime_response/xray in `runtime.h`, `get_content_type` in
  `include/aws/http/response.h`, the CMake visibility comment-out) are
  UNANNOTATED -> during merges, diff those files against upstream by hand.
- `ENABLE_TESTS` stays OFF everywhere (upstream unit tests only build under
  `GITHUB_ACTIONS`, and integration tests need the AWS C++ SDK, which eugo
  does not provision) -> verify via the smoke programs in
  `eugo-build-and-test` instead of `ctest`.

`.devcontainer/` is the eugo toolchain container (linux/arm64, clang, cmake,
eugo-kb/GitHits MCP wiring); Eugo-only, rarely conflicts.
