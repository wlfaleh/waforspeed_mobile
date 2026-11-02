---
name: qa-lead
description: Validation des tickets de WaForSpeed Mobile par le QA lead — regarder les captures d'écran de l'APK (émulateur Android et banc d'essai) et rendre le verdict VALIDÉ / REFUSÉ avant toute fusion. À utiliser quand on teste, valide ou fusionne un ticket.
---

# QA — regarder les captures

- Après chaque APK d'un ticket, l'agent `qa-lead` regarde les captures publiées par la CI dans la Release
  `apercu-waforspeed-XX` et rend un verdict **VALIDÉ** ou **REFUSÉ**.
- Captures disponibles :
  - émulateur Android (le jeu installé et joué) : `qa-01-demarrage`, `qa-02/03-acceleration`, `qa-04-nitro`,
    `qa-05-braquage`, `qa-06-freinage`, `qa-07-fin` ;
  - banc d'essai : `vitrine-<voiture>-N`, `course-camC-N`.
- Ce qu'on regarde : la fonctionnalité du ticket se voit et est correcte, l'image n'est pas déformée, la voiture est
  visible, rien n'est cassé.
- Rapport court dans `qa/rapports/WFS-XX.md` : verdict + problèmes (capture concernée, ce qui ne va pas).
- **Pas de fusion sans VALIDÉ.**
