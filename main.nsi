;--------------------------------
;Including Header Files
; - Unicode
; - LZMA compression (solid)
; - Includes resources/, plugins/, projects/, bin/, and slang_runtime/ (expect next to script)
; - Per-user file associations for .edx .sex .edX
; - Creates %USERPROFILE%\SceneryEditorX with required subfolders
; - Updates uninstall registry keys and fixes typos
!define UNICODE
!define _UNICODE
!include "MUI2.nsh"
!include "LogicLib.nsh"

;--------------------------------
;Settings

  !define APPNAME "Scenery Editor X"
  !define APP_NAME_IN_INSTALLED_DIR "SceneryEditorX"
  !define COMPANYNAME "Coalition of Free Developers"
  !define DESCRIPTION "A 3d scenery editor for X-Plane 12 airports."
  !define DEVELOPER "Thomas Ray & Coalition of Free Developers"
  # Files Directory
  !define FILE_DIR "G:\Repositorys\Coalition-of-Freeware-Developers\Scenery-Editor-X" #Replace with the full path of install folder
  !define LOGO_ICON_FILE "${FILE_DIR}\resources\icon.ico"
  !define LICENSE_TEXT_FILE "${FILE_DIR}\LICENSE.txt"
  !define SPLASH_IMG_FILE "${FILE_DIR}\resources\splash_screen.bmp"
  !define HEADER_IMG_FILE "${FILE_DIR}\header.bmp"
  # These three must be integers
  !define VERSIONMAJOR 0	#Major release Number
  !define VERSIONMINOR 2	#Minor release Number
  !define VERSIONBUILD 27	#Maintenance release Number (bugfixes only)
  !define BUILDNUMBER 528		#Source control revision number
  # These will be displayed by the "Click here for support information" link in "Add/Remove Programs"
  # It is possible to use "mailto:" links in here to open email client
  !define HELPURL "https://github.com/Coalition-of-Freeware-Developers/Scenery-Editor-X"
  !define UPDATEURL "https://"
  !define ABOUTURL "https://www.youtube.com/c/codewar"
  # This is the size (in kB) of all the files copied into "Program Files"
  !define INSTALLSIZE 79672
;--------------------------------
;General

  ;Name and file
  Name "${APPNAME}"
  Icon "${LOGO_ICON_FILE}"
  OutFile "SceneryEditorX_Setup.exe"

  ; Default installation folder and registry integration for previous install
  InstallDir "$PROGRAMFILES64\${APPNAME}"
  InstallDirRegKey HKCU "Software\${APPNAME}" "InstallDir"

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin ;Require admin rights on NT6+ (When UAC is turned on)

  ; Compression
  SetCompressor /SOLID lzma
  ; Use a valid value for SetCompress (off|auto|force). 'auto' lets NSIS choose best behavior.
  SetCompress auto

;--------------------------------
; Variables

  Var StartMenuFolder
  Var USER_DATA_DIR ; will hold $PROFILE\SceneryEditorX
  Var USER_TEMP_DIR ; will hold $PROFILE\AppData\Local\Temp\SceneryEditorX

;--------------------------------
; Splash (kept compatible)
XPStyle on

Function .onInit
	# the plugins dir is automatically deleted when the installer exits
  InitPluginsDir
  File /oname=$PLUGINSDIR\splash.bmp "${SPLASH_IMG_FILE}"
  splash::show 3000 $PLUGINSDIR\splash
	Pop $0
	; $0 has '1' if the user closed the splash screen early,
	; '0' if everything closed normally, and '-1' if some error occurred.
FunctionEnd

;--------------------------------
;Interface Settings

  !define MUI_HEADERIMAGE
  !define MUI_HEADERIMAGE_BITMAP "${HEADER_IMG_FILE}" ; optional
  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "${LICENSE_TEXT_FILE}"
  !insertmacro MUI_PAGE_DIRECTORY

  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU"
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\${APPNAME}"
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"

  !insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder

  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_WELCOME
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Verify User is Admin or not

  !macro VerifyUserIsAdmin
  UserInfo::GetAccountType
  pop $0
  ${If} $0 != "admin" ;Require admin rights on NT4+
	messageBox mb_iconstop "Administrator rights required!"
	setErrorLevel 740 ;ERROR_ELEVATION_REQUIRED
	quit
  ${EndIf}
  !macroend

;--------------------------------
; Helper macro: write file associations (per-user)
!macro WriteFileAssociation EXT ProgID FriendlyName IconPath ExePath
  ; HKCU per-user classes avoids touching HKCR directly in some environments
  WriteRegStr HKCU "Software\Classes\${EXT}" "" "${ProgID}"
  WriteRegStr HKCU "Software\Classes\${ProgID}" "" "${FriendlyName}"
  WriteRegStr HKCU "Software\Classes\${ProgID}\DefaultIcon" "" "${IconPath}"
  WriteRegStr HKCU "Software\Classes\${ProgID}\Shell\Open\Command" "" '"${ExePath}" "%1"'
  ; Optional: set perceived type
  WriteRegStr HKCU "Software\Classes\${EXT}" "PerceivedType" "document"
!macroend

;--------------------------------
;Installer section

Section "install" SEC01
  SetOutPath "$INSTDIR"

  ; Create install directory
  IfFileExists "$INSTDIR\${APP_NAME_IN_INSTALLED_DIR}" 0 +2
  ; if file exists, continue to overwrite

  ; Ensure installation dir exists
  CreateDirectory "$INSTDIR"

  ; Copy main exe and icon
  ; The build system should place the EXE and logo next to the script or in a known path
  File "${FILE_DIR}\bin\${APP_NAME_IN_INSTALLED_DIR}.exe"
  File "${FILE_DIR}\resources\icon.ico"
  File "${FILE_DIR}\resources\icon_edx.ico"

  ; Include bin/ recursively into installer
  ; Expect a 'bin' folder beside this script
  SetOutPath "$INSTDIR\bin"
  File /r "${FILE_DIR}\\bin\\*.*"

  ; Include resources/, plugins/, projects/``
  SetOutPath "$INSTDIR\resources"
  File /r "${FILE_DIR}\\resources\\*.*"

  SetOutPath "$INSTDIR\plugins"
  File /r "${FILE_DIR}\\plugins\\*.*"

  SetOutPath "$INSTDIR\projects"
  File /r "${FILE_DIR}\\projects\\*.*"

  ; Include Slang compiler runtime redistributables
  ; Place the Slang runtime files in a folder named 'slang_runtime' next to this script before building the installer
  ;SetOutPath "$INSTDIR\thirdparty\slang_runtime"
  ;File /r "${FILE_DIR}\\slang_runtime\\*.*"

  ; Create Start Menu shortcuts
  CreateDirectory "$SMPROGRAMS\${APPNAME}"
  CreateShortCut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\${APP_NAME_IN_INSTALLED_DIR}" "" "$INSTDIR\logo.ico"
  CreateShortCut "$SMPROGRAMS\${APPNAME}\uninstall.lnk" "$INSTDIR\uninstall.exe" "" ""

  ; Desktop shortcut
  CreateShortCut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\${APP_NAME_IN_INSTALLED_DIR}" "" "$INSTDIR\logo.ico"

  ; Write per-user InstallDir to HKCU for future updates/uninstall
  WriteRegStr HKCU "Software\${APPNAME}" "InstallDir" "$INSTDIR"

  ; Write Uninstall information under HKLM (standard) - keep consistent names and fix typos
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayName" "${APPNAME} - ${DESCRIPTION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString" "$\"$INSTDIR\\uninstall.exe$\""
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayIcon" "$\"$INSTDIR\\logo.ico$\""
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "Publisher" "${COMPANYNAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "HelpLink" "${HELPURL}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "URLUpdateInfo" "${UPDATEURL}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "URLInfoAbout" "${ABOUTURL}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayVersion" "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}.${BUILDNUMBER}"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "VersionMajor" ${VERSIONMAJOR}
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "VersionMinor" ${VERSIONMINOR}
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoRepair" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "EstimatedSize" ${INSTALLSIZE}

  ; Write file associations (per-user) for .edx, .sex
  StrCpy $0 "$INSTDIR\${APP_NAME_IN_INSTALLED_DIR}"

  ; Default icon path
  StrCpy $1 "$INSTDIR\resources\icon.ico"
  !insertmacro WriteFileAssociation ".edx" "SceneryEditorX.Document" "Scenery Editor X Compressed Binary" "$1" "$0"
  !insertmacro WriteFileAssociation ".sex" "SceneryEditorX.Project" "Scenery Editor X Project" "$1" "$0"

  ; Create per-user application data directory in %USERPROFILE%\Scenery Editor X
  StrCpy $USER_DATA_DIR "$PROFILE\Scenery Editor X"
  CreateDirectory "$USER_DATA_DIR"
  CreateDirectory "$USER_DATA_DIR\cache"
  CreateDirectory "$USER_DATA_DIR\autoback"

  ; %TEMP% (or %USERPROFILE%\AppData\Local\Temp)
  StrCpy $USER_TEMP_DIR "$PROFILE\AppData\Local\Temp\SceneryEditorX"
  CreateDirectory "$USER_TEMP_DIR"


  ; Optionally deploy default config files into user profile if present in resources
  ; If you have default templates in the resources folder (e.g. default_settings.json), copy them to user folder
  ; File /oname="$USER_DATA_DIR\user_settings\default_settings.json" "../resources/default_settings.json"

  ; Create uninstaller
  WriteUninstaller "$INSTDIR\uninstall.exe"

SectionEnd

;--------------------------------
;Version Information

  VIProductVersion "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}.${BUILDNUMBER}"
  VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "${APPNAME}"
  VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments" "${DESCRIPTION}"
  VIAddVersionKey /LANG=${LANG_ENGLISH} "CompanyName" "${COMPANYNAME}"
  VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalTrademarks" "${APPNAME} is a trademark of ${COMPANYNAME}"
  VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" "${DEVELOPER} | ${COMPANYNAME}"
  VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "${APPNAME}"
  VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}"
  VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductVersion" "${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}.${BUILDNUMBER}"

;--------------------------------
;Uninstaller Section

;--------------------------------
; Uninstaller
Function un.onInit
  MessageBox MB_OKCANCEL "Permanently remove ${APPNAME}?" IDOK next
    Abort
  next:
    ; Ensure admin when removing registry keys
    !insertmacro VerifyUserIsAdmin
FunctionEnd

Section "uninstall"
  ; Remove shortcuts
  Delete "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk"
  Delete "$SMPROGRAMS\${APPNAME}\uninstall.lnk"
  Delete "$DESKTOP\${APPNAME}.lnk"
  RMDir "$SMPROGRAMS\${APPNAME}" ; only removes if empty

  ; Remove installed files
  Delete "$INSTDIR\${APP_NAME_IN_INSTALLED_DIR}"
  Delete "$INSTDIR\logo.ico"
  Delete "$INSTDIR\icon.ico"

  ; Remove directories (bin, resources, plugins, projects, thirdparty)
  RMDir /r "$INSTDIR\bin"
  RMDir /r "$INSTDIR\resources"
  RMDir /r "$INSTDIR\plugins"
  RMDir /r "$INSTDIR\projects"

  ; Remove uninstaller
  Delete "$INSTDIR\uninstall.exe"

  ; Remove install directory if empty
  RMDir "$INSTDIR"

  ; Remove per-user install registry entry
  DeleteRegKey /ifempty HKCU "Software\${APPNAME}"

  ; Remove uninstall information from HKLM
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"

  ; Remove per-user file associations (HKCU)
  DeleteRegKey HKCU "Software\Classes\.edx"
  DeleteRegKey HKCU "Software\Classes\.sex"
  DeleteRegKey HKCU "Software\Classes\.edX"
  DeleteRegKey HKCU "Software\Classes\SceneryEditorX.Document"
  DeleteRegKey HKCU "Software\Classes\SceneryEditorX.Project"

  ; Optionally: remove the user data folder in %USERPROFILE%\SceneryEditorX
  ; Only remove if it exists and user agrees — uncomment to remove recursively
  RMDir /r "$PROFILE\SceneryEditorX"
  RMDir /r "$USER_TEMP_DIR\SceneryEditorX"

SectionEnd


;--------------------------------
