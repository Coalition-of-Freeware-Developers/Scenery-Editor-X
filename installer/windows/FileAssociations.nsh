; -------------------------------------------------------
; Scenery Editor X - file associations for the installer
; -------------------------------------------------------
; Registers the editor as the handler of its project files. Everything goes under SHCTX, which MultiUser.nsh
; points at HKLM for an all-users install and at HKCU for a per-user one:
;
;   Software\Classes\<.ext>                  (Default) = ProgID, OpenWithProgids\<ProgID>
;   Software\Classes\<ProgID>                type name, icon, shell\open\command
;   Software\Classes\Applications\<exe>      the editor's "Open with" entry and the types it supports
;   <AppKey>\Capabilities                    the editor's page in Settings > Apps > Default apps
;   Software\RegisteredApplications          points Windows at those capabilities
;
; Windows looks extensions up case-insensitively, so ".edX" also covers ".edx" and ".EDX".
; -------------------------------------------------------

!ifndef SEDX_FILE_ASSOCIATIONS_NSH
!define SEDX_FILE_ASSOCIATIONS_NSH

!include LogicLib.nsh

!define /ifndef SHCNE_ASSOCCHANGED 0x08000000
!define /ifndef SHCNF_IDLIST 0x0000

; The editor itself: its "Open with" entry and its Default apps page.
; APP_KEY: the editor's registry key (Software\...); EXE: file name of the executable in $INSTDIR.
!macro SEDX_REGISTER_APPLICATION APP_KEY APP_NAME DESCRIPTION EXE
  WriteRegStr SHCTX "Software\Classes\Applications\${EXE}" "FriendlyAppName" "${APP_NAME}"
  WriteRegStr SHCTX "Software\Classes\Applications\${EXE}\DefaultIcon" "" "$INSTDIR\${EXE},0"
  WriteRegStr SHCTX "Software\Classes\Applications\${EXE}\shell\open\command" "" '"$INSTDIR\${EXE}" --project "%1"'

  WriteRegStr SHCTX "${APP_KEY}\Capabilities" "ApplicationName" "${APP_NAME}"
  WriteRegStr SHCTX "${APP_KEY}\Capabilities" "ApplicationDescription" "${DESCRIPTION}"
  WriteRegStr SHCTX "${APP_KEY}\Capabilities" "ApplicationIcon" "$INSTDIR\${EXE},0"
  WriteRegStr SHCTX "Software\RegisteredApplications" "${APP_NAME}" "${APP_KEY}\Capabilities"
!macroend

; One file type: EXT (".edX"), its ProgID, the type name Explorer shows, its icon ("path" or "path,index").
!macro SEDX_REGISTER_EXTENSION APP_KEY EXE EXT PROGID TYPE_NAME ICON
  WriteRegStr SHCTX "Software\Classes\${PROGID}" "" "${TYPE_NAME}"
  WriteRegStr SHCTX "Software\Classes\${PROGID}" "FriendlyTypeName" "${TYPE_NAME}"
  WriteRegStr SHCTX "Software\Classes\${PROGID}\DefaultIcon" "" "${ICON}"
  WriteRegStr SHCTX "Software\Classes\${PROGID}\shell" "" "open"
  WriteRegStr SHCTX "Software\Classes\${PROGID}\shell\open\command" "" '"$INSTDIR\${EXE}" --project "%1"'

  ; The editor becomes the default for the extension and stays in its "Open with" list if the user picks another app
  WriteRegStr SHCTX "Software\Classes\${EXT}" "" "${PROGID}"
  WriteRegNone SHCTX "Software\Classes\${EXT}\OpenWithProgids" "${PROGID}"
  WriteRegStr SHCTX "Software\Classes\Applications\${EXE}\SupportedTypes" "${EXT}" ""
  WriteRegStr SHCTX "${APP_KEY}\Capabilities\FileAssociations" "${EXT}" "${PROGID}"
!macroend

; Removes what SEDX_REGISTER_EXTENSION wrote. The extension's default is only cleared while it still names this
; ProgID, so an app the user switched to keeps the files.
!macro SEDX_UNREGISTER_EXTENSION EXT PROGID
  Push $0
  ReadRegStr $0 SHCTX "Software\Classes\${EXT}" ""
  ${If} $0 == "${PROGID}"
    DeleteRegValue SHCTX "Software\Classes\${EXT}" ""
  ${EndIf}
  DeleteRegValue SHCTX "Software\Classes\${EXT}\OpenWithProgids" "${PROGID}"
  DeleteRegKey /IfNoSubkeys /IfNoValues SHCTX "Software\Classes\${EXT}\OpenWithProgids"
  DeleteRegKey /IfNoSubkeys /IfNoValues SHCTX "Software\Classes\${EXT}"
  DeleteRegKey SHCTX "Software\Classes\${PROGID}"
  Pop $0
!macroend

!macro SEDX_UNREGISTER_APPLICATION APP_KEY APP_NAME EXE
  DeleteRegKey SHCTX "Software\Classes\Applications\${EXE}"
  DeleteRegKey SHCTX "${APP_KEY}\Capabilities"
  DeleteRegValue SHCTX "Software\RegisteredApplications" "${APP_NAME}"
!macroend

; Per-user keys override all-users ones, so a stale per-user entry from an older test installer would keep the
; new association from showing. Clears EXT's per-user default while it names OLD_PROGID, and that ProgID.
!macro SEDX_REMOVE_LEGACY_ASSOCIATION EXT OLD_PROGID
  Push $0
  ReadRegStr $0 HKCU "Software\Classes\${EXT}" ""
  ${If} $0 == "${OLD_PROGID}"
    DeleteRegValue HKCU "Software\Classes\${EXT}" ""
    DeleteRegKey /IfNoSubkeys /IfNoValues HKCU "Software\Classes\${EXT}"
  ${EndIf}
  DeleteRegKey HKCU "Software\Classes\${OLD_PROGID}"
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\${EXT}\OpenWithProgids" "${OLD_PROGID}"
  Pop $0
!macroend

; Tells Explorer the associations changed, so icons and "Open with" update without signing out.
!macro SEDX_NOTIFY_ASSOCIATIONS_CHANGED
  System::Call 'shell32::SHChangeNotify(i ${SHCNE_ASSOCCHANGED}, i ${SHCNF_IDLIST}, p 0, p 0)'
!macroend

!endif ; SEDX_FILE_ASSOCIATIONS_NSH
