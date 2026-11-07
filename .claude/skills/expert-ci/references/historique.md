# Rouges de la CI déjà rencontrés (à compléter à chaque nouveau cas)
| Date | Run(s) | Job / étape | Cause | Correction |
|---|---|---|---|---|
| 2026-11-05 → 06 | #680 → #772 | `endurance` | Poursuite bloquée à 9,87 km, écarts de parcours, mémoire +10,7 % ; fuite de jeux de descripteurs | WFS-145 (banc réparé, verdict sur la tendance), WFS-187 (`Renderer::destroy(Material&)`) |
| 2026-11-06 | #761 → #795 | `banc-essai (realisme)` | « passes (WFS-150) : 1 écart », M-01 nuit face +X −1,5 % : géométrie fine du feuillage à 1 échantillon par pixel | #170 (capture suréchantillonnée 2 × 2) ; WFS-200 (vêtements en passe miroir) |
| 2026-11-06 | #800 | (installation) | miroir apt sans réponse (infrastructure) | rien : run suivant vert |
| 2026-11-07 | #816 (nuit) | `endurance` / Chauffe | banc tué à 240 s (machine CI plus lente) + régression de sauvegarde de l'écran terrain (WFS-208) | WFS-220 (`CHAUFFE_LIMITE` 600 s, accroche `terrain.cpp`) |
| 2026-11-06 | file d'attente | tous | lanceurs saturés par les CI de branche et les runs de main empilés | WFS-185 (plus de CI de branche), annulation des runs de main remplacés |
