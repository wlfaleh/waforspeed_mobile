# QA WFS-89 : salon par code, Wi-Fi local (décision B)

## QA après fusion, recette de nuit (2026-11-04 22:36:48 UTC, qa-lead, accent qualité et design)

**Verdict : PASSABLE AVEC ANOMALIES MINEURES**

- **Captures regardées** : Release de la branche `apercu-waforspeed-89` (commit `ce35ded`, publiée à 18:22 UTC). Le run de `main` du commit de fusion `fb50d09` (run 41924029885) était encore en cours à 22:36 UTC et la Release `apercu` en était restée à `f542093`. La revue sur `main` sera à refaire quand la Release `apercu` sera à jour.
- **Périmètre recetté** : décision B du PO (journal du 2026-11-04 à 18:00:55), c'est-à-dire la saisie du code, le duel en Wi-Fi local, la note « Sans Internet » et le relais simulé. Le vrai réseau PeerJS (critère APK entre deux réseaux, et avec le jeu web) est reporté à WFS-92 ⏸ et n'est pas recetté ici.
- **Gravité** : les anomalies nouvelles sont mineures. Les captures exigées sont toutes conformes, et l'aide sous le code (RG-07) n'empêche pas le duel. Elles vont au tableau `anno.md` (ticket déjà fusionné).

### Critères d'acceptation

| Critère | Capture | Constat |
|---|---|---|
| Bloc « Rejoindre avec un code », 4 cases, clavier lisible dans la zone sûre, rien ne déborde | `salon-code`, `salon-code-2400` | OK. 4 cases, la 1re soulignée en cyan, clavier de 31 caractères (sans I, L, O, 0, 1) plus effacer, à 16 px du bord droit, « REJOINDRE » grisé |
| Code tapé, « Rejoindre » actif | `salon-code-saisi`, `-2400` | OK. « K 7 Q X » en grand italique, cases soulignées en orange, « REJOINDRE » orange |
| Salon rejoint par le relais simulé | `salon-internet` | OK. « Alex (toi) · hôte » et « Sam », « Adversaire trouvé par Internet », voiture et terrain de l'hôte |
| Note orange, pas de plantage | `salon-hors-ligne` | OK. « Sans Internet : le duel marche seulement sur le même Wi-Fi » en orange (texte adapté par la décision B) |
| `test_binarypack`, tests CPU verts | CI de la branche (run 41883025840) | OK (vert d'après la revue WFS-103) |
| Wi-Fi local inchangé | `salon-accueil`, `salon-hote` | OK. « Adversaire trouvé sur le Wi-Fi local » |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-43 | Mineure | `salon-code`, `salon-code-saisi`, `-2400` | La touche « effacer » du clavier est un trait cyan « ⊢— » qu'on ne reconnaît pas. Elle a la couleur de la sélection, alors que c'est une action. **Correction** (`salon.cpp`) : icône retour arrière de 22 × 16 px (pentagone pointé vers la gauche, trait 2 px #F2F4F8, croix de 6 px au centre) en segments `uiQuad`, ou libellé « EFF. » de 13 px #F2F4F8, sur une touche deux fois plus large (≥ 44 dp après WFS-102) |
| ANO-44 | Mineure | `salon-hote`, `salon-hors-ligne`, `salon-internet` | L'aide sous le code (RG-07) manque : rien ne dit au joueur quoi faire du code « 8 4 T U ». **Correction** : sous le code, en 12 px #8A90A6 (14 px en 2400), « Donne ce code à ton adversaire : il le tape dans « Rejoindre avec un code ». » ; ajouter « , sur téléphone ou sur le jeu web » seulement quand WFS-92 apportera le vrai réseau |
| (ANO-19) | Bloquante, déjà planifiée | `salon-code-2400` | Touches du clavier de 69 px en 2400x1080, soit ≈ 26 dp : sous les 44 dp, et 32 touches serrées avec 6 px d'écart. Risque de faute de frappe au pouce → WFS-102 (ajouter le clavier du salon à sa liste) |
| (ANO-17 / ANO-38) | Déjà planifiées | `salon-*` | Note orange de 9 px, écran aux deux tiers vide → WFS-95 / WFS-102 |
