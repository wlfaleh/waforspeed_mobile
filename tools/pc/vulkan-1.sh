#!/usr/bin/env bash
# WaForSpeed — version PC Windows (WFS-602) : bibliothèque d'importation de vulkan-1.dll (le chargeur Vulkan installé
# par le pilote de la carte graphique), fabriquée depuis les en-têtes Vulkan, sans le SDK de LunarG.
#   bash tools/pc/vulkan-1.sh <dossier des en-têtes contenant vulkan/> <sortie libvulkan-1.a>
set -euo pipefail
INC="$1"; SORTIE="$2"
DEF="$(mktemp)"
{ echo "LIBRARY vulkan-1.dll"; echo "EXPORTS";
  grep -ho 'VKAPI_CALL vk[A-Za-z0-9]*(' "$INC"/vulkan/vulkan_core.h "$INC"/vulkan/vulkan_win32.h | sed 's/VKAPI_CALL //; s/($//' | sort -u; } > "$DEF"
x86_64-w64-mingw32-dlltool -d "$DEF" -l "$SORTIE"
rm -f "$DEF"
