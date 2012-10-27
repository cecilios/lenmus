; File 'fr.nsh'
;========================================================================================
; French language file for lenmus installer/uninstaller
;---------------------------------------------------------------------------------------------------------

LicenseLangString license ${LANG_French} ".\locale\license_fr.txt"

;strings to customize MUI pages
LangString MSG_OptionalComponents   ${LANG_French} "S'il vous pla�t, choisissez les composants optionels que vous voulez installer."
LangString MSG_CONTINUE             ${LANG_French} "Voulez-vous poursuivre l'installation ?"
LangString MSG_ABORT                ${LANG_French} "Installation annul�e"
LangString MSG_Running              ${LANG_French} "Another copy of this installer is already running."


;section titles, what user sees to select components for installation
LangString TITLE_CreateIcon     ${LANG_French} "Raccourci sur le bureau"
LangString TITLE_Scores         ${LANG_French} "Exemples de partitions"

;descriptions for the sections. Displayed to user when mouse hovers over a section
LangString DESC_CreateIcon      ${LANG_French} "Cr�er un raccourci sur le bureau"
LangString DESC_Scores          ${LANG_French} "Cr�er un dossier contenant quelques exemples de partitions"


;error messages and other texts
LangString ERROR_CopyFiles          ${LANG_French} "Erreur durant la copie des fichiers du logiciel"
LangString ERROR_CreateIcon         ${LANG_French} "Erreur durant la cr�ation du raccourci sur le bureau"
LangString ERROR_CopyScores         ${LANG_French} "Erreur durant la copie des exemples de partitions"


;start menu & desktop descriptions
LangString SM_PRODUCT_GROUP             ${LANG_French} "${APP_NAME}"
LangString SHORTCUT_NAME_EXEC           ${LANG_French} "Ex�cuter ${APP_NAME}"
LangString SHORTCUT_NAME_UNINSTALL      ${LANG_French} "D�sinstaller ${APP_NAME}"

;other strings
LangString OTHER_LangName             ${LANG_French} "fr"

;End French language file
;=========================================================================================
