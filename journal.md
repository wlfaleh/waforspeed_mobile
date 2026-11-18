# WaForSpeed Mobile — journal

Journal du projet (tickets WFS-XX de `TACHES.md`) : une entrée par étape (début, construction, PR, verdict QA, fin…) avec
fait / fichiers / vérification / limites. **Nouvelles entrées à la fin du fichier.** Toujours partir de la dernière version de
`main` (`git pull`), ajouter seulement ses entrées, puis envoyer tout de suite ; ne jamais écraser le fichier.
**Chaque entrée commence par la date et l’heure UTC complètes, jusqu’à la seconde** : `### AAAA-MM-JJ hh:mm:ss — titre (auteur)`, heure obtenue avec `date -u "+%Y-%m-%d %H:%M:%S"` au moment d’écrire, jamais devinée ni recopiée (une date seule, sans heure, est refusée).
Exemple : `### 2026-11-03 19:43:10 — WFS-58 construit, essai simulateur lancé (Claude, session trafic)`.
> **Archive (règle de l'utilisateur, 2026-11-06)** : ce fichier ne garde que les **2 derniers jours**. Les entrées plus anciennes sont dans `journal/archive/AAAA-MM-JJ.md` (un fichier par jour) ; ne les relire qu'en cas de besoin précis. Le PO déplace chaque jour les entrées de plus de 2 jours.

### 2026-11-17 11:06:13 — Statuts automatiques, bancs du matin #46 (robot statuts)
- Run rouge (schedule, commit 3388570), aucun statut changé. Job(s) en échec : endurance et chauffe, banc (realisme-2), banc (complements-c).
- Tickets 🟣 contenus dans ce run : WFS-555, WFS-564, WFS-496, WFS-503.

### 2026-11-17 11:41:34 — Archives automatiques (robot archives)
- Journal : 96 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-15).
- TACHES.md : 0 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md`.

### 2026-11-17 19:35:26 — Campagne VABF de la dernière version de main : NON APTE (Agent qa (session_01UY1Vv3tQoXdU3HwAjakb9t))

- Demande de l’utilisateur (« @qa lance une campagne de test recette la dernière version ») ; version : bancs du matin #46, commit `33885704` (avec WFS-557), Release `captures-matin` du jour et `simulateur-matin`.
- ~1 300 images contrôlées, ~520 ouvertes (5 agents QA en parallèle, lots A à D). Rapport `qa/rapports/VABF-2026-11-17.md` (+ lots détaillés `qa/rapports/VABF-2026-11-17/`).
- 38 anomalies ajoutées à `anno.md` : **ANO-276 à ANO-313** (11 bloquantes, 15 majeures, 12 mineures).
- Points urgents pour le PO : ANO-276 (scénario de l’émulateur bloqué aux nouvelles CGU de WFS-557 : aucun essai sur vrai APK possible) et ANO-277 (bancs `complements-c` et chauffe rouges : « Menu » de l’écran de fin déplacé, banc pas mis à jour ; `realisme-2` rouge) → US technique P1 « correction pipeline CI ». Puis ANO-288 (commandes absentes en vue arrière), ANO-289 (gaucher non miroir), ANO-287 (score de fin incohérent), formes nues ANO-278 à 286.

### 2026-11-17 20:55:42 — VABF 2026-11-17 : preuves ajoutées (Agent qa (session_01UY1Vv3tQoXdU3HwAjakb9t))

- Sur demande de l’utilisateur (« ta des preuves de tes anno ? ») : 28 images de preuve dans `qa/rapports/VABF-2026-11-17/preuves/` (ANO-276 à 301), toutes ouvertes et confirmées ; ANO-294 corrigée (pose « assis » = vitrine du banc, retirée du constat).

### 2026-11-17 20:58:14 — ANO-298 abandonnée (Agent qa (session_01UY1Vv3tQoXdU3HwAjakb9t))

- Décision de l’utilisateur : la pluie de jour invisible (ANO-298, VABF 2026-11-17) « c’est pas grave » → 🚫 Abandonnée dans `anno.md`, pas d’US de correction.

### 2026-11-17 21:04:15 — VABF 2026-11-17 : anomalies de design à confirmer sur téléphone (Agent qa (session_01UY1Vv3tQoXdU3HwAjakb9t))

- Retour de l’utilisateur : toutes les anomalies de design ne se reproduisent pas sur son téléphone (le banc PC ne rend pas comme le téléphone).
- 25 anomalies de design (ANO-278 à 286, 290, 292 à 297, 299 à 303, 307 à 309, 313) passent en « ⏸ À confirmer sur téléphone » dans `anno.md` : hors verdict, pas d’US sans preuve du téléphone ou de l’émulateur.
- Restent ouvertes : ANO-276, 277 (bloquantes, CI et émulateur), 287, 288, 289, 291, 304, 305, 306, 310, 311, 312. Verdict maintenu NON APTE (ANO-276, 277).
- Skill `qa-lead` : règle « design : le téléphone fait foi » ajoutée. Point pour le PO et l’expert CI / CD : écart de rendu banc PC / téléphone.

### 2026-11-17 21:05:45 — VABF 2026-11-17 : défauts d’interface responsive (Agent qa (session_01UY1Vv3tQoXdU3HwAjakb9t))

- Piste de l’utilisateur (« c’est pas responsive ») vérifiée : ANO-302 (CHOISIR sur les stats) et ANO-303 (« Étape ' sur 3 ») n’existent qu’en 1280x720 (16:9) et sont propres en 2400x1080 (20:9, format du téléphone de l’utilisateur). Elles reviennent en 🆕 Nouvelle avec la mention responsive (preuves en vis-à-vis dans `qa/rapports/VABF-2026-11-17/preuves/`). ANO-290, 304, 307 : responsive probable, à vérifier en 20:9.
- Proposition au PO : US « interface responsive » (16:9, 20:9, 4:3). Skill `qa-lead` : comparer les deux tailles de capture avant de conclure sur l’interface.

### 2026-11-17 21:07:16 — VABF 2026-11-17 : anomalies du monde 3D confirmées (Agent qa (session_01UY1Vv3tQoXdU3HwAjakb9t))

- L’utilisateur confirme les anomalies du monde 3D : ANO-278 à 286, 292 à 297, 299 à 301, 308, 309, 313 reviennent en 🆕 Nouvelle. Verdict final NON APTE (11 bloquantes : ANO-276, 277, 278 à 286). Skill `qa-lead` : monde 3D au banc = valable ; interface = comparer 16:9 et 20:9 (responsive).

### 2026-11-18 10:37:34 — Statuts automatiques, bancs du matin #50 (robot statuts)
- Run rouge (schedule, commit 0d13d2e), aucun statut changé. Job(s) en échec : endurance et chauffe, banc (realisme-2), banc (complements-c).
- Tickets 🟣 contenus dans ce run : WFS-555, WFS-564, WFS-496, WFS-503.

### 2026-11-18 11:29:49 — Archives automatiques (robot archives)
- Journal : 74 entrée(s) de plus de 2 jours déplacée(s) vers `journal/archive/` (2026-11-16).
- TACHES.md : 0 ligne(s) de ticket terminé, annulé ou abandonné déplacée(s) vers `archive/taches-terminees.md`.
