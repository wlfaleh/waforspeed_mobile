# QA WFS-31 : CI simulateur et aperçus

**Verdict : VALIDÉ**

- Release `apercu-waforspeed-31` présente (« Aperçu — construction 66 »), avec `WaForSpeed.apk` (24,5 Mo) et 18 captures `course-cam*` / `vitrine-*` non vides (1,4 à 1,6 Mo chacune). Aucun fichier `qa-*`.
- Captures contrôlées visuellement (`course-cam0-2`, `vitrine-golf-0`) : ville, voiture bleue et HUD rendus correctement, rien de cassé.
- Workflow `.github/workflows/simulateur.yml` présent sur `origin/feature/waforspeed-31`, déclenchement à la demande (`workflow_dispatch`, entrée `epic`).
