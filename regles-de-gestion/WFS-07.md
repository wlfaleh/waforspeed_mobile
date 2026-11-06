# WFS-07 — Ombres dynamiques du soleil
Épic : E4 · Priorité : 6 · Dépend de : WFS-30 (✅)

## Besoin (qui, quoi, pourquoi)
Le joueur doit voir les voitures, la route et le décor projeter des ombres du soleil de jour, sans scintillement,
pour donner du relief. Le jeu web le fait seulement sur ordinateur ; en natif, le mobile doit l'avoir à coût maîtrisé.

## Règles de gestion
- RG-01 : un seul soleil directionnel ; direction de jour `sunDir = [-0.45, 0.75, -0.5]`, couleur `#fff0d4`, intensité 1,2 (`envFor`, mode `jour`). Nuit : pas d'ombre solaire (lune `sunI` 0,35, ombre simulée sous les voitures).
- RG-02 : la boîte d'ombre est serrée autour du joueur : demi-largeur 40 m, `near` 1, `far` 260, carte 2048 (1024 sur mobile), décalée de `SHADOW_AHEAD = 20` m vers l'avant dans le sens de la caméra.
- RG-03 : anti-scintillement : le centre de la boîte est calé sur la grille des texels (`texel = largeur / taille de carte`) à chaque image (`stableShadow`).
- RG-04 : réglages : `bias -0.0003`, `normalBias 0.035`, rayon de flou 3 (ombres douces).
- RG-05 : ombre de contact sous chaque voiture quand les ombres temps réel sont coupées (opacité 0,55 avec ombres, 0,62 jour / 0,7 nuit sans).
- RG-06 : coût mesuré dans le journal du banc : ≤ 3 ms GPU sur mobile ; la taille de la carte ne change qu'au changement de qualité, jamais par image.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/world/renderer.js` → `stableShadow`, `SHADOW_AHEAD`, bloc « ombres » de `applyEnv` ; `js/world/maps.js` → `envFor` (jour).
- Retour déjà connu (skill `waforspeed-3d`) : pas d'ombre temps réel sur mobile dans le web faute de budget ; caméra d'ombre serrée qui suit la voiture ; plans d'ombre plats sous la voiture supprimés des modèles.
- Modèles : voitures `models/<voiture>/` déjà chargées par WFS-04 (aucun nouveau modèle).

## Critères d'acceptation (captures du banc PC)
- [ ] `course-cam0-*` en jour : ombres nettes des 4 voitures et des immeubles/lampadaires sur la chaussée et les trottoirs
- [ ] deux captures successives à 1 m d'écart : bords d'ombre stables (pas de crénelage mobile)
- [ ] nuit : aucune ombre solaire, ombre de contact conservée
- [ ] journal du banc : coût de la passe d'ombre ≤ 3 ms ; aucune régression du test de fidélité

## Hors périmètre
Phares et éclairage nocturne (WFS-09), niveaux de qualité (WFS-15), ombres des piétons.

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| 6 | WFS-07 | Ombres dynamiques du soleil | E4 | ✅ Terminé | Claude (session moteur-natif), 2026-11-02 | WFS-30 | `feature/waforspeed-07` | PR #11 | ✅ VALIDÉ par qa-lead |

Critères et notes de suivi :

**WFS-07 — Ombres dynamiques du soleil** · E4 · priorité 6 · ✅ Terminé (PR #11) (règles : `regles-de-gestion/WFS-07.md`)
- [ ] `course-cam0-*` en jour : ombres nettes des voitures, immeubles et lampadaires sur chaussée et trottoirs
- [ ] Deux captures à 1 m d'écart : bords d'ombre stables (calage sur la grille des texels)
- [ ] Nuit : aucune ombre solaire, ombre de contact conservée
- [ ] Journal du banc : passe d'ombre ≤ 3 ms ; test de fidélité inchangé
