---
name: qa-lead
description: Validation des tickets de WaForSpeed Mobile par le QA lead — regarder les captures d'écran de l'APK (émulateur Android et banc d'essai) et rendre le verdict VALIDÉ / REFUSÉ avant toute fusion. À utiliser quand on teste, valide ou fusionne un ticket.
---

# QA — regarder les captures

- Après chaque APK d'un ticket, l'agent `qa-lead` regarde les captures publiées par la CI dans la Release
  `apercu-waforspeed-XX` et rend un verdict **VALIDÉ** ou **REFUSÉ**.
- **QA de chaque ticket = banc d'essai PC** (rapide, ~3 min) : `vitrine-<voiture>-N`, `course-camC-N` et les captures
  propres au ticket, dans la Release `apercu-waforspeed-XX`.
- **QA de fin d'épic = simulateur Android** : quand tous les tickets d'une épic sont terminés, le PO crée le ticket
  « Vérification simulateur EX » ; on lance le workflow `simulateur.yml` (entrée `epic`), puis le QA lead regarde les
  captures de la Release `simulateur-EX` (`qa-01-demarrage` … `qa-07-fin`, `qa-logcat.txt`) et écrit
  `qa/rapports/simulateur-EX.md` (orientation, tactile, démarrage, plantages sur un vrai APK).
- Ce qu'on regarde : la fonctionnalité du ticket se voit et est correcte, l'image n'est pas déformée, la voiture est
  visible, rien n'est cassé.
- Rapport court dans `qa/rapports/WFS-XX.md` : verdict + problèmes (capture concernée, ce qui ne va pas).
- **Pas de fusion sans VALIDÉ.**
