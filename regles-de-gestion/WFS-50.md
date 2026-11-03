# WFS-50 — [MVP] Vérification simulateur du MVP (parcours complet sur Android)
Épic : E7 · Priorité : 0.44 (MVP du 2026-11-03, dernier) · Dépend de : WFS-35, WFS-27, WFS-37, WFS-38, WFS-29, WFS-47 · Taille : ≈ 1 h

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
