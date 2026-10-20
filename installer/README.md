# Windows MSI

The release workflow builds the MSI with WiX Toolset v4 when the Windows package job succeeds. The MSI is a convenience installer for the CLI and documentation; it is not Authenticode-signed unless a code-signing certificate is configured in the release workflow.
