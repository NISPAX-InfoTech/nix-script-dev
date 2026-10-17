# CLI release checklist

NixScript is an evolving C++17 interpreter. Treat releases as **preview releases** until the language/runtime compatibility promises are stable. A passing build is necessary, but it is not a claim that every planned language feature is complete.

## Before tagging

- [ ] Review the diff and confirm the intended source commit is on `main`.
- [ ] Confirm the Ubuntu build/smoke tests pass.
- [ ] Confirm the Windows MSVC build and language regression suites pass.
- [ ] Confirm the AddressSanitizer/UndefinedBehaviorSanitizer job passes.
- [ ] Run the Android APK workflow if the Android IDE is part of the release.
- [ ] Review open bugs and explicitly list known limitations in the release notes.
- [ ] Check that README.md and LANGUAGE.md describe the same supported syntax and limitations.
- [ ] Verify the official library registry packages referenced in the release still install and run.
- [ ] Choose a semantic-version tag such as `v0.1.0`; use a prerelease suffix while compatibility is still changing.

## Build artifacts

Pushing a `vX.Y.Z` tag to the private `NISPAX-InfoTech/nix-script-dev` repository starts `.github/workflows/release.yml`. It builds and smoke-tests the CLI on Linux and Windows, packages the executable with the README, language reference, and examples, and publishes the archives with SHA-256 checksums to the public `NISPAX-InfoTech/nix-script` releases repository. The source tag must point to a commit reachable from `nix-script-dev/main`; the published release tag is created on the release repository's `main` branch.

- Linux x64: `nixscript-linux-x64.tar.gz`
- Windows x64: `nixscript-windows-x64.zip`
- Checksums: `SHA256SUMS.txt`

The workflow dispatch option can package an existing source tag. It does not create or move source tags. It publishes the release to the public release repository and will fail if that version already has a release there.

## One-time GitHub setup

- [ ] Create `NISPAX-InfoTech/nix-script` as a **public** repository for releases only, with a `main` branch (a README-only initial commit is fine). Do not rename `nix-script-dev` back to `nix-script`.
- [ ] Create a fine-grained personal access token (or equivalent GitHub App token) scoped only to `NISPAX-InfoTech/nix-script`, with **Contents: Read and write** permission.
- [ ] In `nix-script-dev`, add that token as an Actions repository secret named `NIXSCRIPT_RELEASE_TOKEN`.
- [ ] Ensure Actions are allowed to run for tags and workflow dispatch in the development repository.
- [ ] Verify the target repository is public and the secret can create releases before tagging a version.

The release workflow deliberately uses the dedicated secret instead of the source repository's `GITHUB_TOKEN`, because that token normally cannot publish releases into a different repository.

## Before calling a release stable

- [ ] Define the language's compatibility and deprecation policy.
- [ ] Decide and document a stable versioning policy for the interpreter and library registry.
- [ ] Add package dependency resolution and integrity verification before promising those capabilities.
- [ ] Decide whether a compiler/runtime installation experience is required beyond downloadable CLI archives.
- [ ] Publish a security/reporting policy and a supported-platform matrix.
