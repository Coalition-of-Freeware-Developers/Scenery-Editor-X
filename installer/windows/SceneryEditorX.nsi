; -------------------------------------------------------
; Scenery Editor X - Windows installer
; -------------------------------------------------------
; Packages a staged Release build for testers (alpha builds by default):
;   - installs for all users (Program Files, needs admin) or just the current user (no admin)
;   - makes Scenery Editor X the handler of .edX (JSON) and .sex (binary) project files
;   - adds a Start menu entry, an optional desktop shortcut and an Add/Remove Programs entry
;   - over an installed copy, offers to update, repair or uninstall it (same folder, same install mode)
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
!include WordFunc.nsh
!include Sections.nsh
!include nsDialogs.nsh
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
${APP_NAME} needs a graphics driver with Vulkan 1.3 and an X-Plane 11 or 12 installation. Setup checks for Vulkan and can install the Vulkan runtime when it is missing.$\r$\n$\r$\n$_CLICK"
; An installed copy gets the maintenance page instead of the first-install pages (see DetectInstalled)
Var InstalledVersion	; The installed copy's version, empty when there is none
Var InstalledCompare	; ${VersionCompare} of it against this setup: 0 same, 1 installed is newer, 2 this setup is newer
Var Maintenance			; What this setup does to it: "update", "repair" or "downgrade" (empty: a first install)
Var MaintenanceKeep		; The page's radio buttons
Var MaintenanceUninstall
Var FinishText			; The finish page's first line: installed, updated, repaired or replaced

!define MUI_PAGE_CUSTOMFUNCTION_PRE SkipWhenInstalled
!insertmacro MUI_PAGE_WELCOME
Page custom MaintenancePageCreate MaintenancePageLeave
!define MUI_PAGE_CUSTOMFUNCTION_PRE SkipWhenInstalled
!insertmacro MUI_PAGE_LICENSE "${ROOT}\LICENSE.txt"
!define MULTIUSER_PAGE_CUSTOMFUNCTION_PRE SkipWhenInstalled
!insertmacro MULTIUSER_PAGE_INSTALLMODE
!define MUI_PAGE_CUSTOMFUNCTION_PRE SkipWhenInstalled
!insertmacro MUI_PAGE_COMPONENTS
!define MUI_PAGE_CUSTOMFUNCTION_PRE SkipWhenInstalled
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_TEXT "$FinishText$\r$\n$\r$\nClick Finish to close Setup."
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
      ; Only a running editor keeps it locked: a just-closed one, or a virus scan of a just-written file, lets go
      ; within moments, so wait a little before asking (a silent setup would give up straight away)
      StrCpy $1 0
      ${Do}
        ClearErrors
        FileOpen $0 "$INSTDIR\${APP_EXE}" a
        ${IfNot} ${Errors}
          FileClose $0
          Return
        ${EndIf}
        Sleep 500
        IntOp $1 $1 + 1
      ${LoopWhile} $1 < 10
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
  !insertmacro MULTIUSER_INIT		; Picks the installed copy's mode and folder when there is one
  Call DetectInstalled

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
  ${If} $Maintenance == "update"
    DetailPrint "Updating ${APP_NAME} $InstalledVersion to ${VERSION} in $INSTDIR"
  ${ElseIf} $Maintenance == "repair"
    DetailPrint "Repairing ${APP_NAME} ${VERSION} in $INSTDIR"
  ${ElseIf} $Maintenance == "downgrade"
    DetailPrint "Replacing ${APP_NAME} $InstalledVersion with ${VERSION} in $INSTDIR"
  ${EndIf}
  Call CheckEditorClosed

  ; Updating or repairing: the installed version's uninstaller removes the files it installed (and nothing else)
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

; Prerequisites -------------------------------------------------
; Everything else the editor loads is installed beside it (the Visual C++ runtime included). What can be missing is
; system-wide: the Vulkan runtime (vulkan-1.dll), which setup can install, and a graphics driver with Vulkan 1.3, which
; only the card's maker provides. The editor checks both the way its renderer will (--check-vulkan, exit codes below).

!define VULKAN_OK 0
!define VULKAN_NO_RUNTIME 2
!define VULKAN_OLD_RUNTIME 3
!define VULKAN_NO_DRIVER 4
!define VULKAN_OLD_DRIVER 5
!define DRIVER_HELP "Install the latest driver for your graphics card:$\r$\n\
  NVIDIA: https://www.nvidia.com/drivers$\r$\n\
  AMD: https://www.amd.com/support$\r$\n\
  Intel: https://www.intel.com/content/www/us/en/support/detect.html"

!ifdef VULKAN_RUNTIME
  !getdllversion "${VULKAN_RUNTIME}" VULKAN_RUNTIME_
  !define VULKAN_RUNTIME_VERSION "${VULKAN_RUNTIME_1}.${VULKAN_RUNTIME_2}.${VULKAN_RUNTIME_3}"
!endif

; $0 = the check's exit code ("error" when the editor couldn't be run)
!macro SEDX_CHECK_VULKAN
  nsExec::Exec '"$INSTDIR\${APP_EXE}" --check-vulkan'
  Pop $0
!macroend

Section "-Vulkan" SecVulkan
  DetailPrint "Checking for Vulkan 1.3..."
  !insertmacro SEDX_CHECK_VULKAN

!ifdef VULKAN_RUNTIME
  ; The runtime needs System32, so only an all-users setup can install it; LunarG's installer asks for its license itself
  ${If} $0 == ${VULKAN_NO_RUNTIME}
  ${OrIf} $0 == ${VULKAN_OLD_RUNTIME}
    ${If} $MultiUser.InstallMode == "AllUsers"
    ${AndIfNot} ${Silent}
      MessageBox MB_YESNO|MB_ICONQUESTION "${APP_NAME} needs the Vulkan runtime (vulkan-1.dll), which is missing or older than 1.3 on this PC.$\r$\n$\r$\n\
        Install the Vulkan Runtime ${VULKAN_RUNTIME_VERSION} from LunarG now? Its installer asks you to accept its license." IDNO vulkan_runtime_done
      DetailPrint "Installing the Vulkan Runtime ${VULKAN_RUNTIME_VERSION}..."
      File "/oname=$PLUGINSDIR\VulkanRT.exe" "${VULKAN_RUNTIME}"
      ExecWait '"$PLUGINSDIR\VulkanRT.exe"' $1
      DetailPrint "Vulkan Runtime installer finished (exit code $1)"
      !insertmacro SEDX_CHECK_VULKAN
    ${EndIf}
  ${EndIf}
  vulkan_runtime_done:
!endif

  ${If} $0 == ${VULKAN_OK}
    DetailPrint "Vulkan 1.3 is available"
  ${ElseIf} $0 == ${VULKAN_NO_RUNTIME}
  ${OrIf} $0 == ${VULKAN_OLD_RUNTIME}
    DetailPrint "The Vulkan runtime is missing or older than 1.3"
!ifdef VULKAN_RUNTIME
    StrCpy $1 "$\r$\n$\r$\nRunning this setup for anyone who uses this computer (as an administrator) can also install the Vulkan runtime."
!else
    StrCpy $1 ""
!endif
    MessageBox MB_OK|MB_ICONEXCLAMATION "${APP_NAME} needs the Vulkan runtime (vulkan-1.dll), which is missing or older than 1.3 on this PC. \
      Graphics drivers install it.$\r$\n$\r$\n${DRIVER_HELP}$1" /SD IDOK
  ${ElseIf} $0 == ${VULKAN_NO_DRIVER}
    DetailPrint "No graphics driver offers Vulkan"
    MessageBox MB_OK|MB_ICONEXCLAMATION "No graphics driver on this PC offers Vulkan, and ${APP_NAME} needs Vulkan 1.3 to start.$\r$\n$\r$\n${DRIVER_HELP}" /SD IDOK
  ${ElseIf} $0 == ${VULKAN_OLD_DRIVER}
    DetailPrint "The graphics driver offers a Vulkan older than 1.3"
    MessageBox MB_OK|MB_ICONEXCLAMATION "This PC's graphics driver offers an older Vulkan than the 1.3 ${APP_NAME} needs.$\r$\n$\r$\n${DRIVER_HELP}$\r$\n$\r$\n\
      If the newest driver doesn't add Vulkan 1.3, the graphics card is too old for ${APP_NAME}." /SD IDOK
  ${Else}
    DetailPrint "Could not check for Vulkan ($0)"
  ${EndIf}
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecEditor} "The editor, its shaders, fonts and textures, and the runtime libraries it needs."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecAssociations} "Double-clicking a .edX or .sex project opens it in ${APP_NAME}."
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDesktop} "A shortcut to ${APP_NAME} on the desktop."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; Update / repair -------------------------------------------------
; MULTIUSER_INIT has already switched to the installed copy's mode and folder (InstallMode and InstallDir in the
; registry). Its setup also wrote the version, so this setup knows whether it updates, repairs or downgrades it.

Function DetectInstalled
  StrCpy $Maintenance ""
  StrCpy $FinishText "${APP_NAME} ${VERSION} has been installed on your computer."
  ReadRegStr $InstalledVersion SHCTX "${APP_KEY}" "Version"
  ${If} $InstalledVersion != ""
  ${AndIf} ${FileExists} "$INSTDIR\${APP_EXE}"
    ${VersionCompare} $InstalledVersion "${VERSION}" $InstalledCompare
    ${If} $InstalledCompare == 2
      StrCpy $Maintenance "update"
      StrCpy $FinishText "${APP_NAME} has been updated from $InstalledVersion to ${VERSION}."
    ${ElseIf} $InstalledCompare == 0
      StrCpy $Maintenance "repair"
      StrCpy $FinishText "${APP_NAME} ${VERSION} has been repaired."
    ${Else}
      StrCpy $Maintenance "downgrade"
      StrCpy $FinishText "${APP_NAME} $InstalledVersion has been replaced with ${VERSION}."
    ${EndIf}
    ; The components page is skipped: keep the desktop shortcut as the installed copy has it
    ${IfNot} ${FileExists} "$DESKTOP\${APP_NAME}.lnk"
      !insertmacro UnselectSection ${SecDesktop}
    ${EndIf}
    Return
  ${EndIf}
  StrCpy $InstalledVersion ""

  ; Installed for all users while this user can't elevate: only an administrator can update that copy
  ${If} $MultiUser.InstallMode == "CurrentUser"
    ReadRegStr $0 HKLM "${APP_KEY}" "InstallDir"
    ${If} $0 != ""
    ${AndIf} ${Cmd} `MessageBox MB_YESNO|MB_ICONEXCLAMATION "${APP_NAME} is installed for all users in $0. Updating that copy needs an administrator: run this setup from an administrator account.$\r$\n$\r$\nInstall a separate copy just for you instead?" /SD IDNO IDNO`
      Quit
    ${EndIf}
  ${EndIf}
FunctionEnd

; The first-install pages (welcome, license, install mode, components, folder) don't apply to an installed copy
Function SkipWhenInstalled
  ${If} $Maintenance != ""
    Abort
  ${EndIf}
FunctionEnd

Function MaintenancePageCreate
  ${If} $Maintenance == ""
    Abort
  ${EndIf}

  !insertmacro MUI_HEADER_TEXT "${APP_NAME} $InstalledVersion is already installed" "$INSTDIR"
  nsDialogs::Create 1018
  Pop $0
  GetDlgItem $0 $HWNDPARENT 3		; Back: every page before this one is skipped
  EnableWindow $0 0

  ${If} $Maintenance == "update"
    StrCpy $1 "&Update to ${VERSION}"
    StrCpy $2 "Replaces the program files with version ${VERSION}. Shortcuts and the .edX / .sex file associations stay as they are."
  ${ElseIf} $Maintenance == "repair"
    StrCpy $1 "&Repair ${VERSION}"
    StrCpy $2 "Reinstalls every program file, the .edX / .sex file associations, the shortcuts and the Add/Remove Programs entry."
  ${Else}
    StrCpy $1 "&Replace it with the older ${VERSION}"
    StrCpy $2 "The installed $InstalledVersion is newer than this setup. Projects saved with it may not open in ${VERSION}."
  ${EndIf}

  ${NSD_CreateLabel} 0 0 100% 20u "Your settings, projects and unsaved-project recovery data are kept whatever you choose."
  Pop $0
  ${NSD_CreateRadioButton} 10u 28u -10u 12u $1
  Pop $MaintenanceKeep
  ${NSD_CreateLabel} 24u 42u -24u 24u $2
  Pop $0
  ${NSD_CreateRadioButton} 10u 72u -10u 12u "Unin&stall ${APP_NAME}"
  Pop $MaintenanceUninstall
  ${NSD_CreateLabel} 24u 86u -24u 24u "Removes the program, its shortcuts and file associations. The uninstaller asks whether your settings and caches go too."
  Pop $0
  ${NSD_Check} $MaintenanceKeep
  nsDialogs::Show
FunctionEnd

Function MaintenancePageLeave
  ${NSD_GetState} $MaintenanceUninstall $0
  ${If} $0 == ${BST_CHECKED}
    Exec '"$INSTDIR\uninstall.exe" /$MultiUser.InstallMode'
    Quit
  ${EndIf}
FunctionEnd

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
