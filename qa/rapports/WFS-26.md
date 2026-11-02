# Rapport QA — WFS-26 Rendu de texte (police, nombres, libellés)

**Verdict : VALIDÉ** (avec une remarque mineure, non bloquante)

- Construction : `feature/waforspeed-26`, commit 19c62c6 (release `apercu-waforspeed-26`, `target_commitish` 19c62c6).
- Captures examinées (banc d'essai PC) :
  - `texte-essai-1280.png` (1280x720)
  - `texte-essai-2400.png` (2400x1080)
  - `course-cam0-2.png` (1280x720, course sans planche de test)

## Critères d'acceptation

| Critère | Résultat | Constat |
|---|---|---|
| Texte d'essai : accents, chiffres, `1:23.4`, `12 345`, `1,2 km` | OK | Accents minuscules et majuscules lisibles (é è ê à ç ù î ô « » œ Œ, ÀÂÉÈÊËÎÔÙÛÇ), `0123456789`, `CHRONO 1:23.4`, `SCORE 12 345`, `1,2 km`, `× 3 · 100 %` présents sur les deux résolutions. |
| Net à 2400x1080 et à 1280x720 | OK | Contours nets, pas de flou, pas de crénelage, aucun caractère manquant ni carré visible, en zoom x4 sur les deux tailles. |
| Trois tailles sur la même capture | OK | Titre « DÉMO DU TEXTE » (grand, espacé), corps (phrases d'essai), libellés HUD (petits, majuscules espacées). Le « 182 » orange de la vitesse est une quatrième taille, hors planche. |
| Deux alignements sur la même capture | OK | Gauche (titre, corps, HUD) et centré (« Centré : WaForSpeed »). Le droit n'est pas démontré sur la planche, seul le « 182 / KM/H » de la vitesse est calé à droite. |
| `course-cam0-2` sans texte parasite | OK | Seuls la jauge en haut à gauche, les joysticks tactiles et la route sont visibles. Aucun texte parasite, aucune voiture déformée. |

## Constats

1. **Mineur (non bloquant) : apostrophe droite.** Dans « d'un » (phrase d'essai) et « déjà », l'apostrophe est droite (`'`), pas la typographique `’` demandée en RG-02. Le glyphe `’` n'est pas testé sur la planche, donc il faut le vérifier au prochain ticket qui écrit du texte français. Gravité : mineure.
2. **Mineur (non bloquant) : alignement droit non démontré.** La planche montre gauche et centre. Le texte aligné à droite n'apparaît que par le HUD de vitesse. Il faudra une ligne alignée à droite sur la planche pour WFS-27 (HUD). Gravité : mineure.

Aucun défaut bloquant.

## Décision

Les critères du ticket sont remplis sur les deux résolutions. Le ticket est VALIDÉ. Les points 1 et 2 sont à traiter au ticket suivant (WFS-27) ou dans une correction légère si le développeur le juge utile.
