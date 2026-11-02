# WFS-25 — Préparation des stores
Épic : E7 · Priorité : 25 · Dépend de : WFS-22

## Besoin (qui, quoi, pourquoi)
Rassembler ce qu'exigent Google Play et l'App Store : icônes, signature, textes, captures, mentions de licence.

## Règles de gestion
- RG-01 : icônes aux tailles exigées (Android adaptative 512 px Play, iOS 1024 px) au style du jeu, sans logo tiers.
- RG-02 : signature : clé de publication Android (AAB signé) et profil iOS fournis par l'utilisateur en secrets du CI ; jamais dans le dépôt.
- RG-03 : fiches en français : nom « WaForSpeed », description courte et longue, mots-clés ; captures prises sur le banc PC et le simulateur (paysage).
- RG-04 : crédits et licences des modèles 3D dans l'application et dans la fiche : licences CC BY, CC BY-SA, CC BY-NC (donc jeu gratuit) ; polices OFL ; sons du moteur (`assets/licences/sons-moteur.txt`).
- RG-05 : politique de confidentialité : pas de collecte de données (hors duel : pseudo et présence, à indiquer) ; classification d'âge renseignée.
- RG-06 : numéro de version et de build cohérents avec les livraisons du Journal.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Fichiers : `models/README.md` (tableau auteurs / licences), `models/*/license.txt`, `audio/engines/license.txt`, `js/cars/catalog.js` (champ `credit` de `CAR_MODELS`), `README.md` du jeu (description).
- Retour connu : les licences NC imposent un jeu gratuit ; SA (Porsche, Mustang, T-Cross, X5) imposent de partager les versions modifiées sous la même licence.

## Critères d'acceptation
- [ ] dossier `store/` : icônes, textes FR, captures, liste des crédits complète pour les 20 voitures
- [ ] vérification croisée : chaque modèle livré a sa licence dans `assets/licences/`
- [ ] AAB signé produit par le CI (clé factice en test) ; checklist des deux stores cochée

## Hors périmètre
Soumission effective aux stores, achats intégrés, publicité.
