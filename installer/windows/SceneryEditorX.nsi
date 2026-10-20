; -------------------------------------------------------
; Scenery Editor X - Windows installer
; -------------------------------------------------------
; Packages a staged Release build for testers (alpha builds by default):
;   - installs for all users (Program Files, needs admin) or just the current user (no admin)
;   - makes Scenery Editor X the handler of .edX (JSON) and .sex (binary) project files
;   - adds a Start menu entry, an optional desktop shortcut and an Add/Remove Programs entry
;   - uninstalls exactly the files it installed; user settings and caches only when asked to
;
; Build it with installer\windows\Build-Installer.ps1, which stages the files and writes the uninstall list.
; By hand: makensis /DSTAGE_DIR=<stage> /DFILE_LIST=<uninstall_files.nsh> installer\windows\SceneryEditorX.nsi
;
; Command line of the produced setup:
;   /S                silent install          /D=<folder>   install folder (must come last)
;   /AllUsers         install for all users   /CurrentUser  install for the current user only
; -------------------------------------------------------

Unicode true
ManifestDPIAware true
SetCompressor /SOLID lzma
SetCompressorDictSize 64

; makensis compiles from this script's folder (unless run with /NOCD), so the repository is two levels up.
; Not ${__FILEDIR__}: it keeps the script path as typed, which is relative to the folder makensis left.
!define ROOT "..\.."

; Inputs (Build-Installer.ps1 passes all of them) ----------

!define /ifndef STAGE_DIR "${ROOT}\build\installer\stage"
!define /ifndef FILE_LIST "${ROOT}\build\installer\uninstall_files.nsh"
!define /ifndef CHANNEL "Alpha"

!if ! /FileExists "${STAGE_DIR}\SceneryEditorX.exe"
  !error "No staged build in ${STAGE_DIR}. Run installer\windows\Build-Installer.ps1 first."
!endif
!if ! /FileExists "${FILE_LIST}"
  !error "No uninstall list at ${FILE_LIST}. Run installer\windows\Build-Installer.ps1 first."
!endif

; The version comes from the same header the editor's version resource uses
!ifndef VERSION
  !searchparse /file "${ROOT}\source\resource.h" '#define SEDX_VERSION_STRING' VERSION_SPACING '"' VERSION '"'
!endif
!searchparse "${VERSION}" "" VERSION_MAJOR "." VERSION_MINOR "." VERSION_PATCH "." VERSION_BUILD

!define /ifndef OUT_FILE "${ROOT}\build\installer\SceneryEditorX-${VERSION}-${CHANNEL}-Setup.exe"

; Product ---------------------------------------------------

!define APP_NAME "Scenery Editor X"
!define APP_EXE "SceneryEditorX.exe"
!define APP_ID "SceneryEditorX"
!define APP_DESCRIPTION "A 3D scenery editor for X-Plane 12 airports."
!define PUBLISHER "Coalition of Freeware Developers"
!define HELP_URL "https://github.com/Coalition-of-Freeware-Developers/Scenery-Editor-X"
!define ISSUES_URL "${HELP_URL}/issues"

!define APP_KEY "Software\${APP_ID}"
!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_ID}"

; Project file types: extension, ProgID, type name, icon
!define EDX_PROGID "${APP_ID}.edX"
!define SEX_PROGID "${APP_ID}.sex"
!define EDX_TYPE_NAME "Scenery Editor X Project"
!define SEX_TYPE_NAME "Scenery Editor X Binary Project"
!define EDX_ICON "$INSTDIR\resources\edX.ico"
!define SEX_ICON "$INSTDIR\${APP_EXE},0"

Name "${APP_NAME} ${CHANNEL}"
OutFile "${OUT_FILE}"
BrandingText "${APP_NAME} ${VERSION} (${CHANNEL})"
ShowInstDetails show
ShowUninstDetails show

VIProductVersion "${VERSION}"
VIFileVersion "${VERSION}"
VIAddVersionKey /LANG=1033 "ProductName" "${APP_NAME}"
VIAddVersionKey /LANG=1033 "ProductVersion" "${VERSION} ${CHANNEL}"
VIAddVersionKey /LANG=1033 "FileVersion" "${VERSION}"
VIAddVersionKey /LANG=1033 "FileDescription" "${APP_NAME} ${CHANNEL} Setup"
VIAddVersionKey /LANG=1033 "CompanyName" "${PUBLISHER}"
VIAddVersionKey /LANG=1033 "LegalCopyright" "Copyright (c) 2026 Thomas Ray, ${PUBLISHER}"
VIAddVersionKey /LANG=1033 "Comments" "${APP_DESCRIPTION}"

; Install mode: all users when the user can elevate, otherwise just them --------

!define MULTIUSER_EXECUTIONLEVEL Highest
!define MULTIUSER_MUI
!define MULTIUSER_INSTALLMODE_COMMANDLINE
!define MULTIUSER_USE_PROGRAMFILES64
!define MULTIUSER_INSTALLMODE_INSTDIR "${APP_NAME}"
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_KEY "${APP_KEY}"
!define MULTIUSER_INSTALLMODE_INSTDIR_REGISTRY_VALUENAME "InstallDir"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_KEY "${APP_KEY}"
!define MULTIUSER_INSTALLMODE_DEFAULT_REGISTRY_VALUENAME "InstallMode"

!include MultiUser.nsh
!include MUI2.nsh
!include LogicLib.nsh
!include FileFunc.nsh
!include WinVer.nsh
!include x64.nsh
!include "FileAssociations.nsh"
!include "${FILE_LIST}"

; Interface ----------------------------------------------------

!define MUI_ICON "${ROOT}\resources\icon.ico"
!define MUI_UNICON "${ROOT}\resources\icon.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${ROOT}\header.bmp"
!define MUI_HEADERIMAGE_UNBITMAP "${ROOT}\header.bmp"
!define MUI_ABORTWARNING
!define MUI_COMPONENTSPAGE_SMALLDESC

!define MUI_WELCOMEPAGE_TITLE "${APP_NAME} ${VERSION} (${CHANNEL})"
!define MUI_WELCOMEPAGE_TEXT "This installs an $\"${CHANNEL}$\" test build of ${APP_NAME}.$\r$\n$\r$\n\
Test builds are unfinished. Keep backups of your projects and scenery, and report anything that goes wrong at:$\r$\n\
${ISSUES_URL}$\r$\n$\r$\n\
${APP_NAME} needs a Vulkan graphics driver and an X-Plane 12 installation.$\r$\n$\r$\n$_CLICK"
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${ROOT}\LICENSE.txt"
!insertmacro MULTIUSER_PAGE_INSTALLMODE
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Start ${APP_NAME}"
!define MUI_FINISHPAGE_RUN_FUNCTION LaunchEditor
!define MUI_FINISHPAGE_LINK "Report a problem"
!define MUI_FINISHPAGE_LINK_LOCATION "${ISSUES_URL}"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_COMPONENTS
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; Shared helpers -------------------------------------------

; Files of a running editor can't be replaced: ask the user to close it (silent setups give up).
!macro SEDX_DEFINE_CHECK_NOT_RUNNING PREFIX
  Function ${PREFIX}CheckEditorClosed
    ${IfNot} ${FileExists} "$INSTDIR\${APP_EXE}"
      Return
    ${EndIf}
    retry:
      ClearErrors
      FileOpen $0 "$INSTDIR\${APP_EXE}" a
      ${IfNot} ${Errors}
        FileClose $0
        Return
      ${EndIf}
      MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "${APP_NAME} is running.$\r$\n$\r$\nSave your work and close it, then click Retry." /SD IDCANCEL IDRETRY retry
      Abort "${APP_NAME} is still running."
  FunctionEnd
!macroend
!insertmacro SEDX_DEFINE_CHECK_NOT_RUNNING ""
!insertmacro SEDX_DEFINE_CHECK_NOT_RUNNING "un."

; Installer --------------------------------------------------

Function .onInit
  ${IfNot} ${RunningX64}
  ${OrIfNot} ${AtLeastWin10}
    MessageBox MB_OK|MB_ICONSTOP "${APP_NAME} needs 64-bit Windows 10 or later." /SD IDOK
    Abort
  ${EndIf}

  SetRegView 64
  !insertmacro MULTIUSER_INIT

  ; Vulkan comes with the graphics driver: without it the editor can't start
  ${DisableX64FSRedirection}
  ${If} ${FileExists} "$SYSDIR\vulkan-1.dll"
    StrCpy $0 1
  ${Else}
    StrCpy $0 0
  ${EndIf}
  ${EnableX64FSRedirection}
  ${If} $0 == 0
  ${AndIf} ${Cmd} `MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION "No Vulkan driver was found on this PC. ${APP_NAME} needs one to start: install the latest driver for your graphics card.$\r$\n$\r$\nInstall anyway?" /SD IDOK IDCANCEL`
    Abort
  ${EndIf}

  ${IfNot} ${Silent}
    InitPluginsDir
    File "/oname=$PLUGINSDIR\splash.bmp" "${ROOT}\resources\splash_screen.bmp"
    splash::show 1500 "$PLUGINSDIR\splash"
    Pop $0
  ${EndIf}
FunctionEnd

; Starts the editor the way Explorer would (not elevated, even when this setup is).
Function LaunchEditor
  Exec '"$WINDIR\explorer.exe" "$INSTDIR\${APP_EXE}"'
FunctionEnd

Section "!${APP_NAME}" SecEditor
  SectionIn RO
  Call CheckEditorClosed

  ; Upgrading: the previous version's uninstaller removes the files it installed (and nothing else)
  ReadRegStr $0 SHCTX "${APP_KEY}" "InstallDir"
  ${If} $0 == $INSTDIR
  ${AndIf} ${FileExists} "$INSTDIR\uninstall.exe"
    DetailPrint "Removing the previous version's files..."
    ExecWait '"$INSTDIR\uninstall.exe" /S /UPGRADE /$MultiUser.InstallMode _?=$INSTDIR'
  ${EndIf}

  SetOutPath "$INSTDIR"
  File /r "${STAGE_DIR}\*.*"
  WriteUninstaller "$INSTDIR\uninstall.exe"

  ; Start menu entry (SetOutPath above makes the install folder its working folder)
  CreateShortCut "$SMPROGRAMS\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}" 0 SW_SHOWNORMAL "" "${APP_DESCRIPTION}"

  WriteRegStr SHCTX "${APP_KEY}" "InstallDir" "$INSTDIR"
  WriteRegStr SHCTX "${APP_KEY}" "InstallMode" "$MultiUser.InstallMode"
  WriteRegStr SHCTX "${APP_KEY}" "Version" "${VERSION}"
  WriteRegStr SHCTX "${APP_KEY}" "Channel" "${CHANNEL}"

  ; Add/Remove Programs
  WriteRegStr SHCTX "${UNINSTALL_KEY}" "DisplayName" "${APP_NAME} (${CHANNEL})"
  WriteRegStr SHCTX "${UNINSTALL_KEY}" "DisplayVersion" "${VERSION}"
  WriteRegStr SHCTX "${UNINSTALL_KEY}" "DisplayIcon" "$INSTDIR\${APP_EXE},0"
  WriteRegStr SHCTX "${UNINSTALL_KEY}" "Publisher" "${PUBLISHER}"
  WriteRegStr SHCTX "${UNINSTALL_KEY}" "Comments" "${APP_DESCRIPTION}"
  WriteRegStr SHCTX "${UNINSTALL_KEY}" "HelpLink" "${ISSUES_URL}"
  WriteRegStr SHCTX "${UNINSTALL_KEY}" "URLInfoAbout" "${HELP_URL}"
  WriteRegStr SHCTX "${UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr SHCTX "${UNINSTALL_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe" /$MultiUser.InstallMode'
  WriteRegStr SHCTX "${UNINSTALL_KEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /$MultiUser.InstallMode /S'
  WriteRegDWORD SHCTX "${UNINSTALL_KEY}" "VersionMajor" ${VERSION_MAJOR}
  WriteRegDWORD SHCTX "${UNINSTALL_KEY}" "VersionMinor" ${VERSION_MINOR}
  WriteRegDWORD SHCTX "${UNINSTALL_KEY}" "NoModify" 1
  WriteRegDWORD SHCTX "${UNINSTALL_KEY}" "NoRepair" 1
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  WriteRegDWORD SHCTX "${UNINSTALL_KEY}" "EstimatedSize" "$0"
SectionEnd

Section "Open .edX and .sex project files" SecAssociations
  SectionIn RO
  !insertmacro SEDX_REMOVE_LEGACY_ASSOCIATION ".edX" "SceneryEditorX.Document"
  !insertmacro SEDX_REMOVE_LEGACY_ASSOCIATION ".sex" "SceneryEditorX.Project"

  !insertmacro SEDX_REGISTER_APPLICATION "${APP_KEY}" "${APP_NAME}" "${APP_DESCRIPTION}" "${APP_EXE}"
  !insertmacro SEDX_REGISTER_EXTENSION "${APP_KEY}" "${APP_EXE}" ".edX" "${EDX_PROGID}" "${EDX_TYPE_NAME}" "${EDX_ICON}"
  !insertmacro SEDX_REGISTER_EXTENSION "${APP_KEY}" "${APP_EXE}" ".sex" "${SEX_PROGID}" "${SEX_TYPE_NAME}" "${SEX_ICON}"
  !insertmacro SEDX_NOTIFY_ASSOCIATIONS_CHANGED
SectionEnd

Section "Desktop shortcut" SecDesktop
  SetOutPath "$INSTDIR"
  CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}" "" "$INSTDIR\${APP_EXE}" 0 SW_SHOWNORMAL "" "${APP_DESCRIPTION}"
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecEditor} "The editor, its shaders, fonts and textures, and the runtime libraries it needs."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecAssociations} "Double-clicking a .edX or .sex project opens it in ${APP_NAME}."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop} "A shortcut to ${APP_NAME} on the desktop."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; Uninstaller ------------------------------------------------

Var UpgradeOnly	; 1: run by a newer setup, which only needs the old program files gone

Function un.onInit
  SetRegView 64
  !insertmacro MULTIUSER_UNINIT

  StrCpy $UpgradeOnly 0
  ${un.GetParameters} $0
  ClearErrors
  ${un.GetOptions} $0 "/UPGRADE" $1
  ${IfNot} ${Errors}
    StrCpy $UpgradeOnly 1
  ${EndIf}
FunctionEnd

Section "-un.${APP_NAME}"
  Call un.CheckEditorClosed

  ${If} $UpgradeOnly == 0
    !insertmacro SEDX_UNREGISTER_EXTENSION ".edX" "${EDX_PROGID}"
    !insertmacro SEDX_UNREGISTER_EXTENSION ".sex" "${SEX_PROGID}"
    !insertmacro SEDX_UNREGISTER_APPLICATION "${APP_KEY}" "${APP_NAME}" "${APP_EXE}"
    !insertmacro SEDX_NOTIFY_ASSOCIATIONS_CHANGED

    Delete "$SMPROGRAMS\${APP_NAME}.lnk"
    Delete "$DESKTOP\${APP_NAME}.lnk"
    DeleteRegKey SHCTX "${UNINSTALL_KEY}"
    DeleteRegKey SHCTX "${APP_KEY}"
  ${EndIf}

  ; Only the files this version installed: anything else in the folder stays
  !insertmacro SEDX_UNINSTALL_FILES

  ${If} $UpgradeOnly == 0
    Delete "$INSTDIR\uninstall.exe"
    RMDir "$INSTDIR"
  ${EndIf}
SectionEnd

Section /o "un.Settings, logs and caches" SecUserData
  ${If} $UpgradeOnly == 0
    ; They belong to whoever runs the uninstaller, also after an all-users install
    SetShellVarContext current
    RMDir /r "$APPDATA\${APP_ID}"
    RMDir /r "$LOCALAPPDATA\${APP_ID}"
    RMDir /r "$TEMP\${APP_ID}"
    ${If} $MultiUser.InstallMode == "AllUsers"
      SetShellVarContext all
    ${EndIf}
  ${EndIf}
SectionEnd

!insertmacro MUI_UNFUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecUserData} "Your settings and logs (%APPDATA%\${APP_ID}), caches and the recovery copies of unsaved projects (%LOCALAPPDATA%\${APP_ID}). Your project files are never removed."
!insertmacro MUI_UNFUNCTION_DESCRIPTION_END
