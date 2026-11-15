; Inno Setup script for Nothing X (built by build.ps1).
#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif

[Setup]
AppId={{6B0E9B58-3F7C-4C1B-9E44-8C2E5D0A7F11}
AppName=Nothing X
AppVersion={#AppVersion}
AppPublisher=Nothing X Desktop contributors
DefaultDirName={autopf}\Nothing X
DefaultGroupName=Nothing X
PrivilegesRequired=lowest
OutputBaseFilename=NothingX-Setup-{#AppVersion}
SetupIconFile=nothingx.ico
UninstallDisplayIcon={app}\nothingx.exe
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
; Updates run silently from inside the app: close it, then start it again.
CloseApplications=yes
RestartApplications=no
ArchitecturesInstallIn64BitMode=x64compatible

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: recursesubdirs ignoreversion

[Icons]
Name: "{group}\Nothing X"; Filename: "{app}\nothingx.exe"
Name: "{autodesktop}\Nothing X"; Filename: "{app}\nothingx.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked
Name: "autostart"; Description: "Start Nothing X when I sign in"

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "Nothing X"; ValueData: """{app}\nothingx.exe"" --hidden"; Flags: uninsdeletevalue; Tasks: autostart

[Run]
Filename: "{app}\nothingx.exe"; Description: "Launch Nothing X"; Flags: nowait postinstall skipifsilent
Filename: "{app}\nothingx.exe"; Parameters: "--hidden"; Flags: nowait; Check: WizardSilent

[UninstallRun]
Filename: "{app}\nothingx.exe"; Parameters: "--quit"; Flags: runhidden; RunOnceId: "QuitNothingX"
