---
name: eugo-upstream-merge
description: Use when merging upstream awslabs/aws-lambda-cpp into the Eugo fork (eugo-inc/aws-lambda-cpp, branch eugo-main). Covers the fork's divergences (awslambdaric patches, tenant_id, -fvisibility=hidden removal), the merge recipe, and the protomolecule native/aws/aws_lambda_cpp commit-pin bump. Activates on "merge upstream", "upstream sync", "aws-lambda-cpp sync", "catch up to awslabs".
---

# Merging upstream awslabs/aws-lambda-cpp into the Eugo fork

## What this fork is / who consumes it

C++ AWS Lambda runtime library (`libaws-lambda-runtime`). Eugo forks it so the
`awslambdaric` fork (eugo-inc/aws-lambda-python-runtime-interface-client) can link
its `runtime_client` extension against a shared system library instead of
vendoring aws-lambda-cpp + patch files. Consumers in protomolecule:

- `dependencies/native/aws/aws_lambda_cpp` -- meta.json pins this repo by
  `git_commit` (branch `eugo-main`, `should_auto_update: true`); its `setup`
  downloads `https://github.com/eugo-inc/aws-lambda-cpp/archive/<commit>.tar.gz`
  and builds with `-DBUILD_SHARED_LIBS=ON`.
- `dependencies/python/wave_4/awslambdaric` -- `runtime_client.so` carries a
  runtime DT_NEEDED on `libaws-lambda-runtime`.

Because the pin is a commit sha, merging upstream here changes nothing in
protomolecule until the meta.json pin is bumped (merge and adoption are decoupled).

## Divergence inventory (vs pure upstream master)

- `include/aws/lambda-runtime/runtime.h`: new `runtime_response` base class with
  xray_response support; `invocation_response` now derives from it; new
  `invocation_request.tenant_id` field (`@EUGO_CHANGE`-annotated).
- `src/runtime.cpp`: xray plumbing plus `TENANT_ID_HEADER`
  ("lambda-runtime-aws-tenant-id") extraction in `get_next()` (`@EUGO_CHANGE`).
- `include/aws/http/response.h`: added `get_content_type()` accessor.
- `CMakeLists.txt`: `-fvisibility=hidden` commented out (breaks dynamic-library
  consumers; the fork ships a shared lib).
- README.md "Eugo Fork Differences" section; `.devcontainer/`, `.mcp.json`,
  `.vscode/settings.json` are Eugo-only dev tooling (rarely conflict).

WARNING: only the tenant_id patch carries `@EUGO_CHANGE` markers. The older
awslambdaric API patches (commit a87bd6b: runtime_response/xray/get_content_type)
are UNANNOTATED -- do not lose them when resolving conflicts. Patch provenance:
eugo-inc/aws-lambda-python-runtime-interface-client `deps/patches`.

## Merge recipe

Canonical branch is `eugo-main`. Upstream is
https://github.com/awslabs/aws-lambda-cpp (branch `master`).

The repo has a mirror branch `awslabs-master`, but its tip (903c423) is itself a
merge containing Eugo content -- do NOT treat it as pristine upstream. Merge
`upstream/master` directly:

```
git remote add upstream https://github.com/awslabs/aws-lambda-cpp.git
git fetch upstream
git checkout -b <user>/feat/MM-DD-YY-merge-upstream eugo-main
git merge upstream/master
# resolve conflicts, preserving every divergence listed above
```

Open a PR to `eugo-main` and land it with the MERGE-COMMIT method only -- never
squash; squashing destroys upstream history and breaks every future sync.
Optionally fast-forward `awslabs-master` to `upstream/master` afterwards for
bookkeeping, but keep it a pure mirror from now on.

## Post-merge adoption in protomolecule

1. Bump `commit` in `dependencies/native/aws/aws_lambda_cpp/meta.json` to the
   new eugo-main head.
2. Re-verify the `setup` regex patches still match: it runs
   `eugo_delete_lines_or_die` on the exact lines `"-Werror"`, `"-fno-exceptions"`,
   `"-fno-rtti"` and `eugo_patch_or_die` on `set(CMAKE_CXX_STANDARD 11)` in
   CMakeLists.txt. If upstream reformats, moves, or removes any of these lines the
   protomolecule build dies at patch time -- update `setup` in the same change.
3. `runtime.h` is part of the ABI surface of `runtime_client.so`; rebuild and
   retest `python/wave_4/awslambdaric` after adopting a new pin.

## Push before pinning

The protomolecule `setup` fetches the tarball by sha from GitHub. Push the merge
commit to `eugo-inc/aws-lambda-cpp` BEFORE bumping the meta.json pin, or the
protomolecule fetch 404s. Never force-push `eugo-main`: rewritten history invalidates
any sha already pinned in protomolecule.
