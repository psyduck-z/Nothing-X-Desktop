# Builds Nothing X for Windows from the same sources as the Omarchy build.
#
# Needs Qt 6.8+ for MSVC 2022 (modules: Qt Bluetooth, Qt SVG; Qt Quick is
# part of the base install) and Visual Studio 2022's C++ tools. Run it from
# a "x64 Native Tools Command Prompt for VS 2022" PowerShell:
#
#   .\packaging\windows\build.ps1 -QtDir C:\Qt\6.8.3\msvc2022_64
#
# Output: dist\windows\nothingx\ (portable folder) and, when Inno Setup 6 is
# installed, dist\windows\NothingX-Setup-<version>.exe.
param(
    [Parameter(Mandatory = $true)][string]$QtDir,
    [string]$InnoSetup = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
)
$ErrorActionPreference = "Stop"

$Root = (Resolve-Path "$PSScriptRoot\..\..").Path
$Build = "$Root\build\windows"
$Dist = "$Root\dist\windows\nothingx"
New-Item -ItemType Directory -Force -Path $Build, $Dist | Out-Null

# The .ico is generated from the same SVG as the Linux icon.
& "$PSScriptRoot\make-ico.ps1" -QtDir $QtDir

Push-Location $Build
& "$QtDir\bin\qmake.exe" "$Root\nothingx.pro"
if ($LASTEXITCODE) { throw "qmake failed" }
nmake
if ($LASTEXITCODE) { throw "build failed" }
Pop-Location

Copy-Item "$Build\release\nothingx.exe" $Dist -Force
& "$QtDir\bin\windeployqt.exe" --release --qmldir "$Root\src\qml" --no-translations "$Dist\nothingx.exe"
if ($LASTEXITCODE) { throw "windeployqt failed" }

if (Test-Path $InnoSetup) {
    $Version = (Select-String -Path "$Root\nothingx.pro" -Pattern '^VERSION = (.+)$').Matches[0].Groups[1].Value
    & $InnoSetup "/DAppVersion=$Version" "/DSourceDir=$Dist" "/O$Root\dist\windows" "$PSScriptRoot\nothingx.iss"
} else {
    Write-Host "Inno Setup not found; the portable build is in $Dist"
}
