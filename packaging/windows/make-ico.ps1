# The Windows icon is generated on Linux from packaging\nothingx.svg by
# bin/make-ico and committed, so both platforms ship the same artwork.
param([string]$QtDir)
if (-not (Test-Path "$PSScriptRoot\nothingx.ico")) {
    throw "packaging\windows\nothingx.ico is missing; regenerate it with bin/make-ico"
}
