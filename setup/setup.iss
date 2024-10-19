; Script generated by the Inno Setup Script Wizard. (Modified)
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "Spplice"
#define MyAppVersion "0.5.1"
#define MyAppPublisher "PortalRunner"
#define MyAppURL "https://p2r3.com/spplice"
#define MyAppExeName "SppliceCPP.exe"
#define MyAppAssocName "Spplice Package File"
#define MyAppAssocExt ".sppkg"
#define MyAppAssocKey StringChange(MyAppAssocName, " ", "") + MyAppAssocExt

[Setup]
; NOTE: The value of AppId uniquely identifies this application. Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{2E9867AC-76D0-4713-B398-37B65A26FB4E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\SppliceCPP
; "ArchitecturesAllowed=x64compatible" specifies that Setup cannot run
; on anything but x64 and Windows 11 on Arm.
ArchitecturesAllowed=x64compatible
; "ArchitecturesInstallIn64BitMode=x64compatible" requests that the
; install be done in "64-bit mode" on x64 or Windows 11 on Arm,
; meaning it should use the native 64-bit Program Files directory and
; the 64-bit view of the registry.
ArchitecturesInstallIn64BitMode=x64compatible
ChangesAssociations=yes
DisableProgramGroupPage=yes
InfoBeforeFile="..\resources\tos.md"
; Remove the following line to run in administrative install mode (install for all users.)
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
OutputBaseFilename=Setup
SetupIconFile="..\resources\icon.ico"
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\dist\win32\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\win32\archive.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\win32\libcrypto-1_1-x64.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\win32\libcurl-4.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\win32\libgcc_s_seh-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\win32\liblzma.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\win32\libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\win32\libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\win32\Qt5Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\win32\Qt5Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\win32\Qt5Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\dist\win32\platforms\qwindows.dll"; DestDir: "{app}\platforms"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Registry]
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocExt}\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppAssocKey}"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppAssocName}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".myp"; ValueData: ""

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

