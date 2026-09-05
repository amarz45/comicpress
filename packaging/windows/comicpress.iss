#ifndef DistDir
  #error DistDir is not defined. Pass e.g. //DDistDir=C:\Users\you\comicpress-dist
#endif

#ifndef OutputDir
  #define OutputDir "."
#endif

#define AppName "Comicpress"
#define AppExe "comicpress.exe"
#define AppPublisher "Amar Al-Zubaidi"
#define AppUrl "https://github.com/amarz45/comicpress"

#define AppVersion GetStringFileInfo(DistDir + "\" + AppExe, "ProductVersion")

[Setup]
AppId={{7B6643FA-3974-416C-924B-83FDE509EB5F}
AppName={#AppName}
AppVersion={#AppVersion}
VersionInfoVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppUrl}
AppSupportURL={#AppUrl}/issues
AppUpdatesURL={#AppUrl}/releases

PrivilegesRequired=lowest
DefaultDirName={autopf}\{#AppName}
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\{#AppExe}

ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

SetupIconFile=comicpress.ico
WizardStyle=modern
Compression=lzma2/max
SolidCompression=yes

OutputDir={#OutputDir}
OutputBaseFilename=comicpress-{#AppVersion}-windows-x64

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; \
    GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; ignoreversion so updates always replace the DLLs, regardless of their own
; version resources.
Source: "{#DistDir}\*"; DestDir: "{app}"; \
    Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,{#AppName}}"; \
    Flags: nowait postinstall skipifsilent

[UninstallDelete]
Type: filesandordirs; Name: "{localappdata}\Temp\comicpress_*"
