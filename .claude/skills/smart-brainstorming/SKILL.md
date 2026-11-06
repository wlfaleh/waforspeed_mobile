---
name: smart-brainstorming
description: Méthode de l'agent Smart Brainstorming Jeux Vidéo appliquée à WaForSpeed Mobile — cadrage, diagnostic, brainstorming, priorisation et synthèse, avec le tableau d'axes d'amélioration obligatoire (ID, domaine, constat, proposition, bénéfice joueur, impact, complexité, priorité P1–P3, risques), les 10 modes (Diagnostic, Créatif, Gameplay, UX/UI, Rétention, Level Design, Narratif, Priorisation, Challenge, Prototype) et les sources du dépôt à lire avant de proposer. À utiliser dès qu'on demande des idées, un diagnostic ou une priorisation d'améliorations du jeu, d'un mode de jeu, d'un écran ou d'une fonctionnalité. Ne s'utilise pas pour coder ni pour créer des tickets (rôle du PO).
---

# Smart Brainstorming — méthode et gabarits

L'identité, les principes et le format complet sont dans l'agent `.claude/agents/smart-brainstorming.md`.
Ce skill donne la **marche à suivre concrète** dans ce dépôt.

## 1. Sources à lire avant de proposer (cadrage, étape A)
Lire ce qui touche au sujet, pas tout le dépôt :

| Besoin | Source |
|---|---|
| Demandes et retours de l'utilisateur (seuls retours de joueur réels) | `tache utilisateur.md`, `qa/retours/` |
| Ce qui existe, ce qui est prévu, épics et zones | `TACHES.md` (rôles, épics, MVP, axe Réalisme, tableau) |
| Règles d'une fonctionnalité existante | `regles-de-gestion/WFS-XX.md`, `regles-de-gestion/README.md` |
| Défauts connus | `anno.md` (ANO-XX), `qa/rapports/WFS-XX.md`, `qa/Check.md` |
| Historique des décisions | `journal.md` (chercher par mot-clé, ne pas tout lire) |
| Aspect du monde 3D | `regles-de-gestion/DA-realisme.md` |
| Interface (menus, HUD) | skill `design-street-racing` |
| Réglages du jeu, contenus | `configgame.json`, `configgame.README.md` |
| Contraintes techniques | skill `waforspeed-natif` (téléphone Android, Vulkan, performance) |

Contexte de départ (à revérifier dans les sources, il évolue) : jeu de course « street racing » natif sur Android,
moteur maison C++20 / Vulkan, en développement actif (tickets WFS-XX), testé par l'utilisateur sur son téléphone ;
iOS gelé, pas de version Web, monde 3D réaliste mesuré.

## 2. Déroulé
1. **Cadrage** : 3 questions au plus, seulement si l'objectif ou le périmètre manque. Sinon, écrire les hypothèses et avancer.
2. **Diagnostic** : 4 listes courtes — fonctionne / frictions (marquées *observée* avec source, ou *supposée*) /
   opportunités / informations manquantes.
3. **Brainstorming** : 6 à 12 propositions, variées (quick wins, structurantes, expérimentales). Chaque idée vérifiée
   contre `TACHES.md` et `anno.md` : si elle existe déjà, citer WFS-XX / ANO-XX.
4. **Priorisation** : tableau obligatoire (§ 3), puis regroupement Quick wins / Structurantes / Expérimentales.
5. **Synthèse** : Top 3 avec la raison, risques et validations, prochaine étape (prototype, playtest sur le téléphone
   de l'utilisateur, capture du banc PC, audit UX, étude technique).

## 3. Gabarit du tableau (à copier)
```markdown
| ID | Domaine | Constat / opportunité | Proposition d'amélioration | Bénéfice joueur | Impact | Complexité | Priorité | Risques / points à vérifier |
|---|---|---|---|---|---|---|---|---|
| A01 | | | | | Faible / Moyen / Élevé | Faible / Moyen / Élevé | P1 / P2 / P3 | |
```
- Une ligne par proposition, ID `A01`, `A02`… dans l'ordre de priorité.
- Constat : préciser *observé* (avec source) ou *supposé*.
- Impact et complexité : qualitatifs ; jamais de chiffre inventé (« estimation qualitative » si besoin).
- Complexité : tenir compte de la zone de code (épic) et du noyau partagé — un changement du noyau est au moins « Moyen ».
- Risques : équilibrage, performance du téléphone, cohérence avec la DA, dépendance à un autre ticket.

## 4. Hypothèse testable (mode Prototype ou idée P1)
> Si nous mettons en place [changement], alors [comportement ou ressenti attendu] devrait s'améliorer, ce que nous vérifierons avec [test ou indicateur].

Indicateurs réalistes dans ce projet : capture du banc d'essai PC, test de l'utilisateur sur l'APK de `main`, mesure du
banc de fidélité (WFS-148), temps d'image, test CPU. Pas d'analytics joueurs : il n'en existe pas.

## 5. Modes
Diagnostic · Créatif · Gameplay · UX/UI · Rétention (sans mécanique manipulatrice : pas de FOMO, pas de pression
d'achat, pas de récompense qui punit l'absence) · Level Design · Narratif · Priorisation · Challenge (remet en cause
même les décisions prises, en le disant) · Prototype. Sans mode : standard (diagnostic + brainstorming + priorisation).

## 6. Après la session
- Idée retenue par l'utilisateur → transmise au **PO** (`po-scrum-master`) avec la ligne du tableau et l'hypothèse
  testable ; c'est lui qui crée l'US. L'agent ne touche pas `TACHES.md`.
- Session à garder → `brainstorming/AAAA-MM-JJ-sujet.md` + entrée au `journal.md`
  (`### AAAA-MM-JJ hh:mm:ss — titre (smart-brainstorming)`, heure par `date -u "+%Y-%m-%d %H:%M:%S"`).

## 7. Contrôle avant de rendre
- [ ] Chaque idée est reliée à WaForSpeed (pas de conseil générique).
- [ ] Faits sourcés, hypothèses marquées, aucune donnée inventée.
- [ ] Aucune idée contraire aux décisions de l'utilisateur sans le signaler (mode Challenge seulement).
- [ ] Doublons avec `TACHES.md` / `anno.md` vérifiés.
- [ ] Tableau complet, Top 3, prochaine étape.
