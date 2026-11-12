# Épic E17 — Identité et historique du joueur

Créée le 2026-11-12 par le PO, sur demande de l'utilisateur (1re étape de l'ordre **identité → score → voitures et couleurs → cadeau → market**).

## Demande de l'utilisateur (résumé fidèle)
1. **Profil** créé au 1er lancement en moins de 20 s : pseudo obligatoire (3 à 16 caractères, mots interdits, modifiable) ;
   immatriculation souhaitée (1 à 8 caractères : lettres, chiffres, tiret ; mots interdits ; proposée d'après le pseudo, modifiable) ;
   e-mail **facultatif**, demandé seulement après la 3e course (« Reçois 100 ★ et les nouveautés »), case d'accord séparée pour les messages.
2. **Plaque** avant et arrière avec le texte du joueur, sur toutes les voitures, style inventé pour le jeu, vrai modèle 3D aux bonnes mesures,
   lisible au garage et en vue arrière ; trafic et adversaires : plaques au hasard, même style.
3. **Écran « Profil »** : pseudo, plaque, date d'arrivée ; courses, km, vitesse maximale, frôlements, victoires ; records par mode, voiture préférée ;
   20 dernières courses (mode, voiture, résultat, date).
4. **Rien de perdu** : sauvegarde automatique Android (seulement le fichier de sauvegarde) ; vérifié par record → désinstallation → réinstallation.
5. **E-mail** : si gardé, le juriste vérifie avant la sortie (confidentialité, « Sécurité des données », suppression à la demande).

Critères de l'utilisateur : captures du profil, de la plaque sur 3 voitures (jour et nuit), de l'écran de création en français et en anglais.

## Découpage (un ticket ≤ 1 jour, une session par ticket)
| Ticket | Sujet | Épic de code | Agent | Dépend de |
|---|---|---|---|---|
| WFS-480 | Profil : données, contrôles (mots interdits), statistiques, 20 dernières courses | E3 | `@dev` | — |
| WFS-485 | Sauvegarde automatique Android (seulement le fichier de sauvegarde) | E7 | `@dev` | — |
| WFS-481 | Écran de création du profil au 1er lancement (FR / EN) | E8 | `@dev` | WFS-480 |
| WFS-482 | Écran « Profil » : historique, modifier pseudo et plaque | E8 | `@dev` | WFS-481 |
| WFS-483 | Plaque 3D avant / arrière de la voiture du joueur (toutes les voitures) | E10 | `@dev` (artiste 3D) | WFS-480 |
| WFS-484 | Plaques au hasard : trafic, adversaires, police, duel, fantôme | E10 (+ accroche E3) | `@dev` (artiste 3D) | WFS-483 |
| WFS-486 | E-mail facultatif après la 3e course (+ 100 ★) | E8 + E3 (+ serveur) | `@dev` | décision de l'utilisateur, étoiles (étape « score »), avis du `juriste` |

## Règles communes à toute l'épic
- RC-E17-01 : **aucun nom, prénom, mot de passe ni compte maison**. Pseudo et plaque restent sur le téléphone (fichier de sauvegarde).
- RC-E17-02 : l'interface 2D suit `design-street-racing` ; la plaque 3D suit la règle « pas de forme nue » et `DA-realisme.md`.
- RC-E17-03 : réutiliser la sauvegarde existante (`src/game/sauvegarde.*`, clé `nick`, `nettoyerPseudo`) : aucune donnée d'avant n'est perdue
  (records, voiture, peintures, réglages, adversaires du duel).
- RC-E17-04 : jeu en français ; seuls les écrans de création du profil et de l'e-mail ont aussi l'anglais (critère de l'utilisateur).
  Traduire tout le jeu est **hors périmètre** (idée R14 du Smart Brain, à demander par l'utilisateur).
- RC-E17-05 : parcours complet testé (chargement → CGU → création → menu → course → fin → Profil), noté au journal avant la PR.

## Langue (épic E20, ajout du PO, 2026-11-12 22:30 UTC, demande de l'utilisateur)
Tous les écrans de cette épic passent par le système de langue de **WFS-504** (`ui::tr`, tables `assets/textes/fr.txt` et `en.txt`) : ils **dépendent de WFS-504** et livrent leurs textes **en français et en anglais dans la même PR**, avec les captures dans les deux langues (`regles-de-gestion/E20-langue-anglaise.md`, RC-E20-01 à 07). Les tickets de données sans texte ne changent pas.
