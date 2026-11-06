# WFS-43 — Arbres : finitions (cœur du houppier, ombres des arbres)
Épic : E2 (Monde et décor) · Priorité : 17.5 · Dépend de : WFS-34 (✅), WFS-07 (✅)

## Besoin (qui, quoi, pourquoi)
Restes non bloquants de la QA de WFS-34 (journal du 2026-11-03, ANO-01 et ANO-02) : le cœur du houppier se voit un peu au
bord d'un arbre (`trafic-cam0`) et les ombres des arbres se voient à peine (rues à l'ombre des immeubles). Finition visuelle,
petite taille.

## Règles de gestion
- RG-01 : le cœur du houppier ne dépasse jamais du feuillage découpé vu de la route : le réduire (rayon ≤ 85 % du houppier)
  ou le décaler vers le tronc, comme le cœur de `js/world/life.js` (même proportions que le jeu web).
- RG-02 : les arbres projettent leur ombre dans la passe d'ombre de WFS-07 (feuillage découpé testé en alpha, seuil 0,5) ;
  vérifier sur une capture où un arbre est au soleil (rue ensoleillée ou carte sans immeubles proches).
- RG-03 : les branches « en baleines de parapluie » restent telles quelles (identiques au jeu web) : hors périmètre.
- RG-04 : ≤ 300 appels de dessin, banc `0 écart(s)`, aucune autre capture dégradée.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Rien de nouveau à porter : `js/world/life.js` est déjà porté dans `src/world/vegetation.*` (WFS-34) ; n'y reprendre que les
  proportions du cœur. Atlas `assets/world/feuillage.png` inchangé.

## Critères d'acceptation (vérifiables sur les captures du banc PC)
- [ ] `trafic-cam0` : plus de cœur visible au bord des houppiers
- [ ] une capture `course-*` (ou nouvelle `ville-arbre-soleil`) : ombre d'arbre découpée visible sur le trottoir ou la chaussée
- [ ] ≤ 300 appels de dessin ; banc `0 écart(s)`

## Hors périmètre
- Forme des branches (conforme au jeu web), nouvelles essences d'arbres (WFS-14).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| — | WFS-43 | Arbres : finitions (cœur du houppier, ombres des arbres) | E2 | ↪ Fusionné dans WFS-53 |  | — | `feature/waforspeed-43` |  |  |

Critères et notes de suivi :

**WFS-43 — Arbres : finitions (cœur du houppier, ombres des arbres)** · E2 · priorité P4 Basse · ✅ Fusionné avec WFS-53 (PR #61) (règles : `regles-de-gestion/WFS-43.md`) — ↪ fusionné dans WFS-53
- [ ] `trafic-cam0` : plus de cœur visible au bord des houppiers ; ombre d'arbre découpée visible sur une capture au soleil
- [ ] ≤ 300 appels de dessin ; banc `0 écart(s)`
