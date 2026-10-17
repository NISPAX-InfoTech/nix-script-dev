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

Pushing a `vX.Y.Z` tag starts `.github/workflows/release.yml`. It builds and smoke-tests the CLI on Linux and Windows, packages the executable with the README, language reference, and examples, and publishes the archives with SHA-256 checksums.

- Linux x64: `nixscript-linux-x64.tar.gz`
- Windows x64: `nixscript-windows-x64.zip`
- Checksums: `SHA256SUMS.txt`

The workflow dispatch option can package an existing tag. It does not create or move tags.

## Before calling a release stable

- [ ] Define the language's compatibility and deprecation policy.
- [ ] Decide and document a stable versioning policy for the interpreter and library registry.
- [ ] Add package dependency resolution and integrity verification before promising those capabilities.
- [ ] Decide whether a compiler/runtime installation experience is required beyond downloadable CLI archives.
- [ ] Publish a security/reporting policy and a supported-platform matrix.
