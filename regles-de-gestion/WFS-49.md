# WFS-49 — Banc d'essai : captures du chargement en 2400x1080 sans planche de texte (correction ANO-10)
Épic : E7 · Priorité : 0.47 (hors MVP, petit) · Dépend de : WFS-35 · Taille : ≈ 30 min · Origine : ANO-10 (QA WFS-35)

## Besoin
Les captures `chargement-*-2400` sont prises dans le lancement du banc qui affiche la planche de test du texte
(`WFS_TEXTE_ESSAI=1`) : le panneau « DÉMO DU TEXTE » chevauche l'écran de chargement. Le joueur ne le voit pas, mais le QA
juge sur des captures fausses.

## Règles de gestion
- RG-01 : dans `.github/workflows/build.yml`, prendre les `chargement-*-2400` dans un **lancement séparé** du banc en
  `WFS_TAILLE=2400x1080` **sans** `WFS_TEXTE_ESSAI` (ou ne copier depuis le lancement « texte » que `texte-essai-*`).
- RG-02 : aucun changement du code du jeu (`src/**`) ; les captures 1280x720 et `texte-essai-*` restent identiques.
- RG-03 : durée de la CI : + 1 lancement du banc au plus.

## À réutiliser
Rien du jeu web (outil de test natif). Natif : étape « captures » de `.github/workflows/build.yml` (sur `main` au 2026-11-03 : l. 40-41, la boucle `for f in texte/chargement-*.png` copie les captures du lancement « texte »).

## Critères d'acceptation
- [ ] `chargement-0/1/2/fondu/erreur-2400` : plus aucun panneau « DÉMO DU TEXTE » ; `texte-essai-2400` inchangée

## Hors périmètre
Tout changement de l'écran de chargement (E8).
