# Appeler un agent avec « @ » — liste des agents

> **Règle de l'utilisateur (2026-11-09 12:12 UTC)** : pour donner son rôle à une session, l'utilisateur écrit **`@<agent>`** au début de son
> message (ex. `@po`, `@leaddev`, `@smartbrain`, `@cicd`). La session lit ce tableau, **prend le rôle** demandé et le suit tout de suite,
> sans demander de confirmation. Les alias ne tiennent pas compte des majuscules, des tirets ni des espaces (`@lead-dev` = `@leaddev` = `@Lead Dev`).

## Comment la session réagit

1. **`@<agent>` au début du 1er message de la session** : la session **devient** cet agent pour toute la session.
   Elle lit le fichier de l'agent (colonne « Fichier à lire ») et charge ses skills, puis fait ce que dit la suite du message.
   - Agent **développeur** (`@dev`, `@conduite`, `@cartes`, `@devartiste`, `@artiste`) : il suit la règle « un seul sujet par session » : il ajoute sa ligne dans
     `track-agent.md`, demande **un** ticket au PO (agent `po-scrum-master`), le mène jusqu'à la fusion par le lead dev, puis s'arrête.
     Le texte après l'alias est transmis au PO comme souhait (ex. `@conduite un sujet sur les freins`) ; le PO reste seul à attribuer.
2. **`@<agent>` plus tard dans la conversation** : la session **appelle** cet agent avec l'outil Agent (`subagent_type` du tableau) et lui
   passe le texte qui suit l'alias, puis rend compte de sa réponse. Exception : `@dev`, `@conduite`, `@cartes`, `@devartiste` et `@artiste` en cours de session
   ne lancent **pas** un 2e sujet (règle du 2026-11-09) : la session répond qu'il faut ouvrir une nouvelle session avec cet alias.
3. **Plusieurs alias** (ex. `@po @leaddev`) : le 1er donne le rôle de la session, les suivants sont appelés comme au point 2.
4. **Alias inconnu** : la session donne cette liste et demande lequel prendre (seul cas où elle pose une question).
5. Les règles de `CLAUDE.md` restent valables pour tous les rôles (gel iOS, lead dev seul à fusionner, réseau, nom de l'agent dans les commits…).

6. **Nom de la session (règle de l'utilisateur, 2026-11-09 13:55 UTC)** : la session **se renomme elle-même**, sans demander.
   Elle charge l'outil avec `ToolSearch` (`select:mcp__claude-code-remote__set_session_title`), puis appelle `set_session_title`.
   Si elle ne connaît pas son identifiant, elle appelle d'abord `get_session` sans argument.
   - **Agent hors développeur** (`@po`, `@leaddev`, `@qa`, `@cicd`, `@smartbrain`, `@3d`) : dès le 1er message, le nom est
     **`Agent <alias> (<id complet de la session>)`**, par exemple `Agent po (session_01UY1Vv5B99mCrTFBmEatAjE)`.
   - **Développeur** (`@dev`, `@conduite`, `@cartes`, `@devartiste`, `@artiste`) : sur place, dès qu'il reçoit le numéro de son ticket du PO et avant toute autre action (avant même la branche), le nom est
     **`WFS-XXX — <nom de l'agent dans track-agent.md> (<sujet en 3 à 5 mots>)`**, par exemple `WFS-343 — Claude, agent 3 (carte de Noël 1/3)`.
     Après la fusion, quand la session s'arrête : **`⏹️ Terminée — <nom de l'agent> (WFS-XXX fusionné)`**.
   - **C'est l'agent lui-même qui renomme sa discussion, jamais le PO** (précision de l'utilisateur, 2026-11-09).
   - Aucune tâche planifiée pour ce renommage (décision de l'utilisateur, 2026-11-09).

**Skills chargés automatiquement** : chaque fichier `.claude/agents/<nom>.md` liste ses skills dans le champ `skills:`. Quand l'agent est appelé (outil Agent), Claude Code charge ces skills tout seul au démarrage ; quand une session prend le rôle avec `@<agent>`, elle les charge elle-même avec l'outil Skill (règle de `CLAUDE.md`).

Remarque : dans Claude Code, « @ » ouvre aussi la suggestion de fichiers ; il suffit de continuer à taper l'alias puis un espace.

## Nos agents (ceux de `.claude/agents/`, seulement)

| Alias (au choix) | Agent | Rôle en une phrase | Fichier à lire / skills | `subagent_type` |
|---|---|---|---|---|
| `@dev`, `@developpeur` | Développeur | prend **un** ticket du PO (hors conduite, cartes, CI), code, captures, PR, appelle le lead dev, s'arrête après la fusion | `.claude/agents/dev.md` (skills `agent-developer`, `waforspeed-natif`) | `dev` |
| `@conduite`, `@expertconduite` | Expert conduite | développeur de la physique de la voiture, des aides et de la conduite des adversaires | `.claude/agents/expert-conduite.md` (skill `expert-conduite`) | `expert-conduite` |
| `@cartes`, `@expertcartes` | Expert cartes | développeur des cartes, des paquets `.wfcarte` et de leur téléchargement (E14) | `.claude/agents/expert-cartes.md` (skill `expert-cartes`) | `expert-cartes` |
| `@devartiste`, `@voitures`, `@devartiste3d` | Développeur artiste 3D | développeur **et** artiste 3D : modèles 3D à choisir, retoucher, juger **et** code (outils, moteur) ; voitures sans marque, WFS-358 à 361 | `.claude/agents/dev-artiste-3d.md` (skills `agent-developer`, `waforspeed-natif`, `artiste-3d`) | `dev-artiste-3d` |
| `@artiste`, `@artiste3d`, `@artist3d` | Artiste 3D | choisit, retouche et intègre les modèles et actifs 3D avec les outils du dépôt, sans code du moteur | `.claude/agents/artiste-3d.md` (skills `artiste-3d`, `waforspeed-natif`) | `artiste-3d` |
| `@po`, `@scrum`, `@scrummaster` | PO / Scrum Master | étudie les besoins, crée et priorise les tickets, attribue, tient `TACHES.md`, `track-agent.md`, le journal | `.claude/agents/po-scrum-master.md` (skill `po-scrum-master`) | `po-scrum-master` |
| `@leaddev`, `@lead` | Lead dev | relit une PR (captures + diff), approuve et fusionne en squash, ou refuse avec la liste des défauts | `.claude/agents/lead-dev.md` (skill `lead-dev`) | `lead-dev` |
| `@qa`, `@qalead` | QA lead | recette des tickets de `qa/Check.md`, verdicts, anomalies ; campagne VABF sur l'émulateur | `.claude/agents/qa-lead.md` (skill `qa-lead`) | `qa-lead` |
| `@cicd`, `@ci`, `@expertcicd` | Expert CI / CD | pipeline GitHub Actions, rouges de `main`, US techniques, versions, AAB, publication Google Play | `.claude/agents/expert-ci-cd.md` (skill `expert-ci-cd` ; + `agent-developer`, `waforspeed-natif` pour coder) | `expert-ci-cd` |
| `@3d`, `@realisme`, `@expert3d` | Expert réalisme 3D | conseille sur l'aspect du monde 3D (matériaux, lumière, animation, LOD) ; consultant, ne tient pas de ticket de code | `.claude/agents/expert-realisme-3d.md` (skill `expert-realisme-3d`) | `expert-realisme-3d` |
| `@smartbrain`, `@brainstorm`, `@idees` | Smart Brainstorming | diagnostic et idées classées P1–P3 dans `recommandations-smart-brain.md` ; propose, ne décide rien | `.claude/agents/smart-brainstorming.md` (skill `smart-brainstorming`) | `smart-brainstorming` |

## Exemples

- `@cartes` → la session devient expert cartes, ajoute sa ligne à `track-agent.md` et demande un ticket au PO.
- `@po j'ai une nouvelle idée : un mode drift` → la session devient PO et étudie la demande.
- `@leaddev relis la PR 305` → la session devient lead dev et relit la PR #305.
- `@smartbrain mode Challenge sur le mode carrière` → la session devient Smart Brainstorming.
- `@cicd pourquoi le run de main est rouge ?` → la session devient expert CI / CD et vérifie sur GitHub.
- En cours de session PO : `@qa` → le PO appelle l'agent `qa-lead` et rend compte.

**Ajouter un agent** : créer `.claude/agents/<nom>.md` (et son skill si besoin), puis ajouter sa ligne ici et dans `track-agent.md` (PO).
