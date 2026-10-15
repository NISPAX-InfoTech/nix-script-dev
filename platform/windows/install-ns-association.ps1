# Installs the NixScript .ns file association and official wolf icon for the current Windows user.
$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$python = Get-Command py -ErrorAction SilentlyContinue
if ($python) {
    & $python.Source -m pip show Pillow *> $null
    if ($LASTEXITCODE -ne 0) { & $python.Source -m pip install --user Pillow }
    & $python.Source (Join-Path $repoRoot "tools\prepare_brand_assets.py")
} else {
    $python = Get-Command python -ErrorAction SilentlyContinue
    if (-not $python) { throw "Python 3 is required. Install Python and Pillow, then rerun this script." }
    & $python.Source -m pip show Pillow *> $null
    if ($LASTEXITCODE -ne 0) { & $python.Source -m pip install --user Pillow }
    & $python.Source (Join-Path $repoRoot "tools\prepare_brand_assets.py")
}
$iconDir = Join-Path $env:LOCALAPPDATA "NixScript"
New-Item -ItemType Directory -Force -Path $iconDir | Out-Null
$iconPath = Join-Path $iconDir "nixscript-ns.ico"
Copy-Item (Join-Path $repoRoot "platform\windows\nixscript-ns.ico") $iconPath -Force

$extensionKey = "HKCU:\Software\Classes\.ns"
$progIdKey = "HKCU:\Software\Classes\NixScript.Source"
$iconKey = Join-Path $progIdKey "DefaultIcon"
New-Item -Path $extensionKey -Force | Out-Null
Set-Item -Path $extensionKey -Value "NixScript.Source"
Set-ItemProperty -Path $extensionKey -Name "Content Type" -Value "text/plain"
New-Item -Path $progIdKey -Force | Out-Null
Set-Item -Path $progIdKey -Value "NixScript source file"
New-Item -Path $iconKey -Force | Out-Null
Set-Item -Path $iconKey -Value $iconPath
Write-Host "NixScript .ns files now use the official wolf icon. Configure an open action separately if you want them to launch an editor."
