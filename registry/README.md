# Official NixScript library registry

This directory is the versioned, team-maintained source for packages supported by `nix install` and `nix update`.

- `index.txt` lists one package per line as `name MAJOR.MINOR.PATCH`.
- Every index entry must point to `libraries/<name>/<version>/<name>.ns`.
- Package names are lowercase NixScript identifiers.
- Released version directories should be immutable. Publish a new version directory and update `index.txt` to release an update.
- Only code reviewed and maintained by the NixScript team belongs here. The CLI derives package URLs from the fixed official registry and does not accept arbitrary repositories or download URLs.

Keep each package a single `.ns` source file for the first registry format. Package dependencies and checksum signatures may be added in a future registry format.
