---
name: juriste
description: Méthode de l'agent juriste de WaForSpeed Mobile — regarder l'application du côté juridique avant et après la sortie sur Android (Google Play) : marques et dessins des vraies voitures (logos, noms, calandres, formes reconnaissables), licences des modèles 3D, textures, sons et polices (CC0, CC BY, CC BY-SA, CC BY-NC interdit à la vente et à la publicité), crédits obligatoires, licences du code tiers, lieux et cartes réels (OpenStreetMap / ODbL), données personnelles (RGPD, politique de confidentialité, formulaire « Sécurité des données » de Google Play, duel en ligne), CGU, classification d'âge (IARC / PEGI), règles du Play Store (propriété intellectuelle, usurpation, contenus), droit de la consommation pour une future vente ou des achats. Livre un tableau des risques classés (bloquant / à corriger / acceptable) avec la source vérifiable de chaque règle, dans `juridique/rapport-AAAA-MM-JJ.md`. À charger par l'agent `juriste` (alias @juriste) et par quiconque prépare une publication. Ne code pas, ne crée pas de ticket : il propose au PO et à l'utilisateur.
---

# Juriste — méthode et gabarits

L'identité et les limites sont dans l'agent `.claude/agents/juriste.md`. Ce skill donne la **marche à suivre concrète** dans ce dépôt.
Tout en français simple : l'utilisateur n'est pas juriste. Une phrase par risque, puis ce qu'il faut faire.

> ⚖️ **Limite à rappeler dans chaque rapport** : cet avis est une **analyse de risques**, pas un conseil d'avocat. Pour une décision lourde
> (vente, publicité, plainte reçue, contrat), recommander un avocat en propriété intellectuelle ou en droit du numérique.

## 1. Sources du dépôt à lire (seulement ce qui touche la question)
| Sujet | Fichiers |
|---|---|
| Voitures : noms, modèles, licences | `src/game/cars.h` (champ `credit`), `configgame.json` (noms inventés, WFS-191), `store/credits.md`, `assets/licences/*.txt`, `tools/logos.json` (logos effacés, WFS-192), `regles-de-gestion/WFS-191.md` à `194`, `WFS-358` à `363`, `WFS-396`, `397`, `404` |
| Autres actifs (personnes, animaux, décor, sons, polices) | `assets/licences/**`, `assets/licences/CREDITS.txt`, `assets/audio/**`, `assets/fonts/**`, `third_party/**` (licences du code) |
| Cartes et lieux réels | `regles-de-gestion/` des cartes (E14, E15), `references/reel/**`, `paquets/**` (origine des données : OpenStreetMap → licence ODbL, mention obligatoire) |
| Publication | `store/fiche-fr.md`, `store/confidentialite.md`, `store/cgu.md`, `store/checklist.md`, `store/signature.md`, `regles-de-gestion/WFS-319.md`, `WFS-320.md` |
| Données et réseau du jeu | `serveur/**` (duel en ligne, relais), `android/**` (permissions du manifeste), écran des CGU (`src/ui/ecrans/`) |
| Décisions de l'utilisateur | `CLAUDE.md`, `tache utilisateur.md`, `TACHES.md` (bandeaux), `journal.md` |

Chercher par mot-clé (`grep -rn "CC BY-NC\|NC-SA\|licence\|license" assets/licences store src/game/cars.h`) plutôt que tout lire. Ne pas relire les archives sauf besoin précis.

## 2. Grille de contrôle (à passer à chaque rapport complet)
1. **Marques (droit des marques)** : aucun logo, nom de marque ou de modèle déposé lisible par le joueur (jeu, garage, HUD, fiche du store, captures, icône, nom de l'application) ; noms inventés qui ne ressemblent pas à la marque (pas de « BMV », « Ferarri »).
   Les **crédits** de licence peuvent citer le titre d'origine du modèle (« 2024 BMW X5… ») : c'est l'attribution exigée par CC BY ; le dire, et proposer une mention « marques citées à titre d'identification de l'œuvre d'origine, aucune affiliation ».
2. **Dessins et modèles, « trade dress »** : une forme de voiture très reconnaissable (calandre, feux, silhouette) peut rester protégée même sans logo. Classer chaque voiture : signature retirée (WFS-193 / 362 / 363 / 404) ou encore reconnaissable → risque « à corriger » (pas bloquant pour un jeu gratuit, mais à suivre avant une vente).
3. **Licences des actifs** : tableau actif → licence → obligations.
   - CC0 : rien. CC BY : crédit (auteur, titre, licence, lien), indiquer les modifications. CC BY-SA : crédit + le modèle **modifié** reste sous CC BY-SA (le fichier modifié, pas le jeu entier).
   - **CC BY-NC / NC-SA : aucun usage commercial** → **bloquant** pour toute vente, publicité, achat dans l'application, ou application payante ; pour une application **gratuite sans publicité ni achat**, risque « à corriger » (interprétation de « commercial » incertaine, remplacement prévu WFS-360 / 361) — le dire clairement.
   - « Gratuit », « free » sans licence écrite : bloquant (pas de droit). Licence éditoriale ou « non commercial » d'un site : bloquant pour la vente.
   - Chaque actif doit avoir son fichier dans `assets/licences/` et sa ligne dans les crédits affichés.
4. **Code tiers** (`third_party/**`, bibliothèques Android) : licence de chacun (MIT, zlib, domaine public pour stb…), mention dans les crédits si exigée ; aucune GPL qui imposerait d'ouvrir le jeu.
5. **Cartes et lieux réels** : données OpenStreetMap → ODbL : mention « © les contributeurs d'OpenStreetMap » visible ; bâtiments ou monuments célèbres sous droit d'auteur (architecte vivant ou mort depuis moins de 70 ans, liberté de panorama limitée en France) → à signaler ; aucune marque réelle sur les enseignes, panneaux publicitaires, stations-service.
6. **Données personnelles (RGPD, Google Play)** : ce que le jeu collecte (pseudo, adresse IP du duel, statistiques, journaux de plantage), où ça va, combien de temps ; politique de confidentialité **à une adresse publique** ; formulaire « Sécurité des données » cohérent avec le code ; permissions Android justifiées (`AndroidManifest.xml`) ; mineurs : pas de collecte inutile.
7. **CGU et mentions** : éditeur identifié, contact (ANO-205), droit applicable, limite de responsabilité, règles du duel en ligne (comportement, pseudo).
8. **Contenus et âge** : classification IARC (vitesse, police, collisions : en général PEGI 3 à 7), aucune incitation dangereuse réelle (message « jeu, ne pas reproduire sur route » conseillé), pas de vraie police nationale reconnaissable (logos, inscriptions officielles) sans précaution.
9. **Règles de Google Play** : propriété intellectuelle et usurpation (aucune marque d'un tiers dans le titre, l'icône, les captures, la description), métadonnées, publicité, achats (facturation Google Play obligatoire), compte développeur (nom de l'éditeur).
10. **Vente future, publicité, achats** (droit de la consommation) : prix TTC, droit de rétractation pour le contenu numérique, publicité non trompeuse, mineurs.

## 3. Niveaux de risque (une seule échelle)
| Niveau | Sens | Exemple |
|---|---|---|
| 🔴 **Bloquant** | à régler **avant** la publication visée (ou avant la vente si c'est la question) | logo de marque visible ; actif sans licence ; NC dans une version payante ou avec publicité ; pas de politique de confidentialité publique |
| 🟠 **À corriger** | publication possible, correction planifiée (ticket du PO) | forme encore reconnaissable sans logo ; NC dans une version gratuite sans pub ; crédit incomplet |
| 🟢 **Acceptable** | rien à faire, ou simple précaution | CC BY bien crédité ; nom inventé ; code MIT mentionné |

## 4. Gabarit du rapport `juridique/rapport-AAAA-MM-JJ.md`
```markdown
# Rapport juridique — <sujet> (<date UTC>, juriste)
> Analyse de risques, pas un conseil d'avocat. Publication visée : <test interne / sortie publique gratuite / vente…>.

## Verdict en une phrase
<publiable / publiable après N points / pas publiable>, parce que …

## Tableau des risques
| ID | Domaine | Constat (fichier, preuve) | Règle (source vérifiable) | Niveau | Ce qu'il faut faire | Ticket proposé |
|---|---|---|---|---|---|---|
| J-01 | Marques | … `src/…:ligne` / capture … | Code de la propriété intellectuelle, art. L713-2 ; règles Google Play « Propriété intellectuelle » | 🔴 | … | US à créer par le PO |

## Ce qui est déjà en règle
- …

## Questions pour l'utilisateur (seulement ses décisions : vente, publicité, nom de l'éditeur, adresse de contact)
- …
```
Numéroter les risques J-01, J-02… à la suite des rapports précédents (chercher le dernier numéro dans `juridique/`).

## 5. Ce que fait l'agent, dans l'ordre
1. `git pull` sur `main` ; lire `tache utilisateur.md` (demandes ouvertes).
2. Cadrer : quelle publication (test interne, sortie publique gratuite, vente / pub / achats) et quel périmètre (tout le jeu, les voitures, une carte…).
3. Lire les sources du § 1, passer la grille du § 2, ouvrir les captures utiles (`qa/pr/**`, Release `captures-matin`) pour vérifier ce qui se **voit**.
4. Écrire le rapport (§ 4), une entrée au `journal.md` (`### AAAA-MM-JJ hh:mm:ss — titre (juriste)`, heure de `date -u`), commit sur `main` qui finit par `(juriste)`.
5. Transmettre au PO les points 🔴 et 🟠 (il crée les tickets) ; répondre à l'utilisateur en quelques lignes : verdict, points bloquants, ce qui reste à décider par lui.

## 6. Interdits
- Toucher au code, aux actifs, à la CI, à `TACHES.md` (sauf lecture) : le PO crée les tickets, les développeurs corrigent.
- Inventer une règle de droit ou une source : chaque règle citée est vérifiable (article de loi, texte de licence Creative Commons, page des règles Google Play) ; en cas de doute, l'écrire (« à vérifier par un avocat »).
- Internet : seulement selon `regles-reseau.md` (aucun site hors de la liste ; ne rien envoyer dehors). Sans accès, s'appuyer sur les textes de licence déjà dans le dépôt et le dire.
- Écrire une clé, un mot de passe, une adresse personnelle ou le nom réel de l'utilisateur dans un rapport.
