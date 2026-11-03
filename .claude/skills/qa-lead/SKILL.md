---
name: qa-lead
description: Validation des tickets de WaForSpeed Mobile par le QA lead — regarder les captures d'écran de l'APK (émulateur Android et banc d'essai) classer les anomalies (bloquante / mineure) et rendre le verdict VALIDÉ, VALIDÉ AVEC RÉSERVES ou REFUSÉ avant toute fusion. À utiliser quand on teste, valide ou fusionne un ticket.
---

# QA — regarder les captures

- Après chaque APK d'un ticket, l'agent `qa-lead` regarde les captures publiées par la CI dans la Release
  `apercu-waforspeed-XX` et rend un verdict **VALIDÉ**, **VALIDÉ AVEC RÉSERVES** ou **REFUSÉ**.
- **QA de chaque ticket = banc d'essai PC** (rapide, ~3 min) : `vitrine-<voiture>-N`, `course-camC-N` et les captures
  propres au ticket, dans la Release `apercu-waforspeed-XX`.
- **QA de fin d'épic = simulateur Android** : quand tous les tickets d'une épic sont terminés, le PO crée le ticket
  « Vérification simulateur EX » ; on lance le workflow `simulateur.yml` (entrée `epic`), puis le QA lead regarde les
  captures de la Release `simulateur-EX` (`qa-01-demarrage` … `qa-07-fin`, `qa-logcat.txt`) et écrit
  `qa/rapports/simulateur-EX.md` (orientation, tactile, démarrage, plantages sur un vrai APK).
- Ce qu'on regarde : la fonctionnalité du ticket se voit et est correcte, l'image n'est pas déformée, la voiture est
  visible, rien n'est cassé.
- **Le QA décide de la gravité de chaque anomalie** :
  - **bloquante** (critère d'acceptation ou règle de gestion non respecté, plantage, écran noir, image cassée, régression) → **REFUSÉ** :
    le développeur corrige jusqu'à ce qu'il n'y ait plus d'anomalie bloquante ;
  - **mineure** (détail de finition qui ne gêne ni le joueur ni les autres tickets) → **VALIDÉ AVEC RÉSERVES** : fusion autorisée,
    l'anomalie va au **tableau des anomalies** de `TACHES.md` (ANO-XX, statut `🆕 Nouvelle`) ;
  - aucune anomalie → **VALIDÉ**.
- Rapport court dans `qa/rapports/WFS-XX.md` : verdict + tableau `| ANO-XX | Gravité | Capture | Ce qui ne va pas |`.
- Les anomalies mineures sont corrigées plus tard par des **US de correction créées par le PO**, jamais en plus dans le ticket.
- **Fusion avec VALIDÉ ou VALIDÉ AVEC RÉSERVES, jamais avec REFUSÉ.**
