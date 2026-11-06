# WFS-50 — [MVP] Vérification simulateur du MVP (parcours complet sur Android)
Épic : E7 · Priorité : P4 Basse (sans urgence, décision utilisateur 2026-11-03) · Dépend de : WFS-35, WFS-27, WFS-37, WFS-38, WFS-29, WFS-47, WFS-56, WFS-57, WFS-58, WFS-59 (tous ✅) · Taille : ≈ 1 h

> **Mise à jour PO 2026-11-03 21:40:54** : le run 41719456821 (`feature/waforspeed-58`, cc78b4d = code de la PR #32, Release `simulateur-WFS-58`) passe tout le parcours (« RÉSULTAT TECHNIQUE : OK », 2 fins à 124 s et 140 s, score 2 260). Seule marque FIGÉE : `03-appui-course-solo` = `02-menu`, **attendue** (capture prise juste après l’appui, correction WFS58-3) : ne pas la compter comme anomalie. WFS-64 n’est plus une dépendance. Relancer sur `main` (`epic=MVP`, `scenario=mvp`) : un gel juste après le chargement, s’il réapparaît, est noté au rapport et réactive WFS-64, sans bloquer le verdict si une relance passe.

## Besoin
Avant de dire « MVP livré », vérifier le parcours entier sur l'émulateur Android, comme un joueur : c'est le seul moment où
l'APK est jouée de bout en bout.

## Règles de gestion
- RG-01 : workflow `simulateur.yml` lancé sur `main` (paramètre `epic=MVP`), APK de `main` après fusion de tous les tickets du MVP.
- RG-02 : parcours à capturer : démarrage → chargement (barre) → menu principal → appui « Course solo » → décompte 3-2-1-GO →
  course avec trafic et HUD lisible → fin de temps → écran de fin → « Rejouer » (nouveau décompte) puis « Menu » (retour au menu).
- RG-02 bis : si `tools/qa/emulateur.sh` ne sait pas encore appuyer sur l'écran, l'ajouter (`adb shell input tap x y` aux
  coordonnées des boutons, attente entre les étapes) : c'est la zone E7, aucun changement du jeu.
- RG-03 : paysage, plein écran (sans barres système), aucun plantage ni ANR, journal `logcat` sans erreur Vulkan.
- RG-04 : rapport `qa/rapports/simulateur-MVP.md` écrit par `qa-lead` ; chaque défaut = un ticket de correction créé par le PO.
- RG-05 : verdict VALIDÉ → le PO note « Livraison MVP » au Journal avec le lien de l'APK (`apercu`).

## À réutiliser
Workflow `.github/workflows/simulateur.yml` (WFS-31), scripts `tools/qa/**`. Rien du jeu web.

## Critères d'acceptation
- [ ] captures du simulateur pour chaque étape de RG-02 ; rapport `simulateur-MVP.md` VALIDÉ

## Hors périmètre
Garage, terrain, pause, duel, sauvegarde.

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P4 Basse | WFS-50 | [MVP] Vérification simulateur du MVP (parcours complet sur Android) | E7 | 🔎 Fusionné (PR #47, correction du scénario : appuis sur la taille logique du jeu) ; parcours OK au run 41747781094 et sur `main` 14241f4 (Release `simulateur-MVP`, `RÉSULTAT TECHNIQUE : OK`, 10 étapes) ; recette `qa-lead` de nuit | Claude (session trafic), 2026-11-04 | WFS-35 ✅, WFS-27 ✅, WFS-37 ✅, WFS-38 ✅, WFS-29 ✅, WFS-47 ✅, WFS-56 ✅, WFS-57 ✅, WFS-58 ✅, WFS-59 ✅ | `feature/waforspeed-50` |  | ⛔ REFUSÉ (SIM-MVP-1 à 4) — [rapport](qa/rapports/simulateur-MVP.md) |

Critères et notes de suivi :

**WFS-50 — [MVP] Vérification simulateur du MVP** · E7 · priorité P4 Basse · 🔎 Fusionné, PR #47 (REFUSÉ au 1er passage, [rapport](qa/rapports/simulateur-MVP.md) ; échec du 2e passage sur `main` b01ae36 : barre de navigation de l'émulateur affichée, gaz appuyé sous la fenêtre du jeu, corrigé dans `tools/qa/emulateur.sh` ; parcours complet OK au run 41747781094 ; relancé sur `main`, recette `qa-lead` de nuit) (règles : `regles-de-gestion/WFS-50.md`)
- [ ] `simulateur.yml` (`epic=MVP`) : chargement → menu → Course solo → décompte → course + HUD → fin → Rejouer puis Menu ; paysage plein écran, aucun plantage
- [ ] rapport `qa/rapports/simulateur-MVP.md` VALIDÉ par `qa-lead`
