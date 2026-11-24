; WaForSpeed — version PC Windows (WFS-602, RG-04) : installateur NSIS (WaForSpeed-Setup.exe).
; Installe dans Program Files\WaForSpeed, crée le raccourci du bureau et l'entrée du menu Démarrer, inscrit la
; désinstallation (Paramètres → Applications). La désinstallation retire tout le programme ; la sauvegarde du joueur
; (%APPDATA%\WaForSpeed) est gardée. Construit sous Linux par tools/pc/construire_installateur.sh (makensis).
; Défines attendus : SOURCE (dossier préparé : WaForSpeed.exe + assets\), SORTIE (fichier .exe produit), VERSION.
Unicode true
ManifestDPIAware true
SetCompressor /SOLID lzma
!include "MUI2.nsh"

!ifndef VERSION
  !define VERSION "1.0.0"
!endif
!define NOM "WaForSpeed"
!define CLE_DESINSTALLATION "Software\Microsoft\Windows\CurrentVersion\Uninstall\WaForSpeed"

Name "${NOM}"
OutFile "${SORTIE}"
InstallDir "$PROGRAMFILES64\${NOM}"
InstallDirRegKey HKLM "Software\${NOM}" "Dossier"
RequestExecutionLevel admin
BrandingText "${NOM} ${VERSION}"

!define MUI_ICON "waforspeed.ico"
!define MUI_UNICON "waforspeed.ico"
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\WaForSpeed.exe"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "English"

VIProductVersion "${VERSION}.0"
VIAddVersionKey /LANG=${LANG_FRENCH} "ProductName" "${NOM}"
VIAddVersionKey /LANG=${LANG_FRENCH} "FileDescription" "Installation de ${NOM}"
VIAddVersionKey /LANG=${LANG_FRENCH} "FileVersion" "${VERSION}"
VIAddVersionKey /LANG=${LANG_FRENCH} "ProductVersion" "${VERSION}"

Section "WaForSpeed" SecJeu
  SectionIn RO
  SetOutPath "$INSTDIR"
  RMDir /r "$INSTDIR\assets"   ; mise à jour : pas d'anciens fichiers mélangés aux nouveaux
  File "${SOURCE}\WaForSpeed.exe"
  File /r "${SOURCE}\assets"
  WriteUninstaller "$INSTDIR\Desinstaller.exe"
  ; raccourcis : bureau et menu Démarrer (pour tous les utilisateurs)
  SetShellVarContext all
  CreateShortcut "$DESKTOP\${NOM}.lnk" "$INSTDIR\WaForSpeed.exe" "" "$INSTDIR\WaForSpeed.exe" 0
  CreateDirectory "$SMPROGRAMS\${NOM}"
  CreateShortcut "$SMPROGRAMS\${NOM}\${NOM}.lnk" "$INSTDIR\WaForSpeed.exe" "" "$INSTDIR\WaForSpeed.exe" 0
  CreateShortcut "$SMPROGRAMS\${NOM}\Désinstaller ${NOM}.lnk" "$INSTDIR\Desinstaller.exe"
  ; inscription dans Paramètres → Applications
  WriteRegStr HKLM "Software\${NOM}" "Dossier" "$INSTDIR"
  WriteRegStr HKLM "${CLE_DESINSTALLATION}" "DisplayName" "${NOM}"
  WriteRegStr HKLM "${CLE_DESINSTALLATION}" "DisplayVersion" "${VERSION}"
  WriteRegStr HKLM "${CLE_DESINSTALLATION}" "Publisher" "${NOM}"
  WriteRegStr HKLM "${CLE_DESINSTALLATION}" "DisplayIcon" "$INSTDIR\WaForSpeed.exe"
  WriteRegStr HKLM "${CLE_DESINSTALLATION}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "${CLE_DESINSTALLATION}" "UninstallString" '"$INSTDIR\Desinstaller.exe"'
  WriteRegStr HKLM "${CLE_DESINSTALLATION}" "QuietUninstallString" '"$INSTDIR\Desinstaller.exe" /S'
  WriteRegDWORD HKLM "${CLE_DESINSTALLATION}" "NoModify" 1
  WriteRegDWORD HKLM "${CLE_DESINSTALLATION}" "NoRepair" 1
SectionEnd

Section "Uninstall"
  SetShellVarContext all
  Delete "$DESKTOP\${NOM}.lnk"
  RMDir /r "$SMPROGRAMS\${NOM}"
  RMDir /r "$INSTDIR\assets"
  Delete "$INSTDIR\WaForSpeed.exe"
  Delete "$INSTDIR\Desinstaller.exe"
  RMDir "$INSTDIR"
  DeleteRegKey HKLM "${CLE_DESINSTALLATION}"
  DeleteRegKey HKLM "Software\${NOM}"
  ; la sauvegarde du joueur (%APPDATA%\WaForSpeed) est gardée (RG-04)
SectionEnd
