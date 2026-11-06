# WFS-26 — Rendu de texte (police, nombres, libellés)
Épic : E5 · Priorité : 4 · Dépend de : WFS-30 (✅) · Ticket 🔧 en cours (session moteur-natif) : règles écrites pour la revue

## Besoin (qui, quoi, pourquoi)
Le moteur n'affiche aucun texte. Menu, HUD, décompte, notifications et fin de course en ont besoin, avec accents français.

## Règles de gestion
- RG-01 : trois familles comme le web : affichage `Orbitron` (700-900, titres, chiffres de vitesse), HUD `Rajdhani` (600-700, libellés), corps `Inter` (400-600) ; licence OFL copiée dans `assets/licences/`. Si une seule police est livrée, elle couvre l'affichage et le HUD.
- RG-02 : police en atlas (SDF ou bitmap net), jeu de caractères complet français : é è ê à ç ù î ô « » ’ œ, chiffres, `× ·  : / % +`.
- RG-03 : taille en pixels logiques (paysage), indépendante de la résolution ; lisible sur 2400x1080 et sur petit écran (corps ≥ 11 px logiques, comme `font-size: 11px` des libellés du web).
- RG-04 : libellés en majuscules espacées (`letter-spacing .16em`) ; nombres au format français (`toLocaleString('fr-FR')` : espace insécable des milliers, virgule décimale, `1,2 km`).
- RG-05 : alignement gauche / centre / droite, ombre ou contour pour rester lisible sur la route ; couleurs du skill `design-street-racing` (cyan `#00E5FF`, orange `#FF6A00`, rouge `#FF1F4B`).
- RG-06 : texte dessiné via `System::drawUi`, sans allocation par image pour les chaînes inchangées.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `css/style.css` (variables `--font-display`, `--font-hud`, `--font-body`, tailles, `letter-spacing`) ; `index.html` ligne des polices Google ; `js/ui/hud.js` → `fmtTime` (`m:ss.d`), formats `fr-FR`.
- Rien à convertir en 3D : seulement l'identité typographique. Retour connu : accents français obligatoires partout.

## Critères d'acceptation (captures du banc PC)
- [ ] capture « texte d'essai » : phrase avec tous les accents, chiffres `0123456789`, `1:23.4`, `12 345`, `1,2 km`
- [ ] net à 2400x1080 et à 1280x720 (pas de flou, pas de bords crénelés)
- [ ] trois tailles et deux alignements visibles sur la même capture

## Hors périmètre
Mise en page du HUD (WFS-27), écrans de menu (WFS-11), langues autres que le français.

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| 4 | WFS-26 | Rendu de texte (police, nombres, libellés) | E5 | ✅ Terminé | Claude (session moteur-natif), 2026-11-02 | WFS-30 | `feature/waforspeed-26` | PR #9 | ✅ VALIDÉ par qa-lead |

Critères et notes de suivi :

**WFS-26 — Rendu de texte (police, nombres, libellés)** · E5 · priorité 4 · ✅ Terminé (PR #9)
- [ ] Police (licence libre) en atlas, texte en pixels logiques, accents français
- [ ] Lisible sur 2400x1080 et petits écrans
- [ ] Capture : texte d'essai net
