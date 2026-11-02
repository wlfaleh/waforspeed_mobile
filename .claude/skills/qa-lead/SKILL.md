---
name: qa-lead
description: Protocole de QA de WaForSpeed Mobile — tester chaque ticket WFS-XX après la construction de son APK (CI, émulateur Android, captures, logcat, performances), comparer aux critères d'acceptation, écrire le rapport qa/rapports/WFS-XX.md et rendre le verdict VALIDÉ / REFUSÉ avant toute fusion. À utiliser dès qu'on teste, valide, vérifie ou fusionne un ticket, ou quand on modifie les tests automatiques.
---

# QA de WaForSpeed Mobile

## Principe
- Chaque ticket est testé **après chaque APK** construit sur sa branche, et **avant chaque fusion**. Le test est fait par l'agent
  `qa-lead` (`.claude/agents/qa-lead.md`), pas par l'utilisateur.
- **Aucune fusion sans verdict `VALIDÉ`** dans `qa/rapports/WFS-XX.md`.
- Une preuve pour chaque critère : nom de la capture ou ligne du journal.

## Outils automatiques (CI `.github/workflows/build.yml`)
| Job | Ce qu'il produit |
|---|---|
| `banc-essai` | `test_track` (piste identique au jeu web) ; captures Vulkan logiciel : `vitrine-<voiture>-N.png`, `course-camC-N.png` |
| `android` | `WaForSpeed.apk` (arm64 + x86_64) |
| `qa-emulateur` | `tools/qa/emulateur.sh` sur émulateur Android 14 (Pixel 6, GPU SwiftShader) : installe, lance en paysage, accélère 8 s, nitro, braque, freine ; captures `qa-01…07`, `qa-logcat.txt`, `qa-deroulement.txt` ; échoue si plantage ou moteur non initialisé |
| `publication` | Release `apercu-waforspeed-XX` (branche) ou `apercu` (main) avec tout ce qui précède |

Télécharger : `https://github.com/wlfaleh/waforspeed_mobile/releases/download/apercu-waforspeed-XX/<fichier>`.
Lire les échecs : outils GitHub `actions_list` / `get_job_logs`.

## Grille de contrôle (à chaque ticket)
1. **Construction** : 3 jobs verts ; branche à jour avec `main`.
2. **Démarrage** : « QA pret » dans logcat, capture `qa-01` non noire, en paysage.
3. **Affichage** : proportions correctes (les commandes rondes sont rondes), voiture entière et posée au sol (ombre), horizon
   droit, pas de bande noire anormale, HUD visible.
4. **Conduite** : `QA ips=… vitesse=…` : vitesse > 100 km/h après 8 s d'accélération, plus haute avec la nitro, en baisse après
   le freinage ; distance croissante. Captures `qa-02/03` : la route défile.
5. **Stabilité** : aucune ligne `Fatal signal`, `SIGSEGV`, `ERREUR`, `VK_ERROR` ; l'appli tourne encore à la fin.
6. **Performances** (indicatives sur émulateur) : noter les `ips` ; sur le banc d'essai noter le temps de rendu.
7. **Critères propres au ticket** (dans `TACHES.md`) : chacun prouvé ou marqué non vérifiable avec la raison.
8. **Non-régression** : comparer aux captures de la dernière livraison (`apercu`) — rien de ce qui marchait ne doit casser.

## Modèle de rapport `qa/rapports/WFS-XX.md`
```
# QA — WFS-XX : <titre>
Date : … · Branche : feature/waforspeed-XX · Commit : … · Construction n° … · PR : #…
## Verdict : VALIDÉ | REFUSÉ
## Critères d'acceptation
- [x] <critère> — preuve : qa-03-acceleration-8s.png / « QA ips=… vitesse=… »
- [ ] <critère> — NON VÉRIFIÉ / ÉCHEC : <raison>
## Défauts
| Gravité | Description | Preuve | Correction attendue |
## Mesures
ips émulateur : … · vitesse après 8 s : … · plantages : 0
## Limites du test
Émulateur à GPU logiciel : performances et défauts spécifiques aux GPU mobiles non représentatifs.
```

## Gravité
- **Bloquant** : plantage, écran noir, image déformée, commande inutilisable, régression → fusion interdite.
- **Majeur** : critère d'acceptation non rempli → fusion interdite.
- **Mineur** : défaut visuel léger → fusion possible, ticket de correction créé dans `TACHES.md`.
