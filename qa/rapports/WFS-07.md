# QA — WFS-07 : ombres dynamiques du soleil
Date : 2026-11-03 · Relecteur : qa-lead · Branche : feature/waforspeed-07 (commit 3ac3d0f) · CI run 41547683867
Captures : Release `apercu-waforspeed-07`, comparées à `apercu-waforspeed-32` (main avant le ticket)

## Verdict : REFUSÉ

Les ombres du soleil n'apparaissent **que sur les façades verticales** des immeubles. Aucune ombre portée n'est visible
**sur la chaussée ni sur les trottoirs** : ni sous les voitures, ni au pied des arbres et des lampadaires, ni au pied des
immeubles. C'est le cœur du premier critère d'acceptation (« ombres nettes des 4 voitures et des immeubles/lampadaires
sur la chaussée et les trottoirs »).

## Critères d'acceptation
- [ ] **`course-cam0-*` de jour : ombres des voitures, immeubles et lampadaires sur chaussée et trottoirs** : NON.
  Seule nouveauté visible : une grande ombre diagonale sur la façade de droite (`course-cam0-0`, `course-cam0-1`).
  La différence pixel à pixel avec `apercu-waforspeed-32/course-cam0-0` est quasi nulle sur toute la route et sur le
  trottoir de gauche : seuls changent les façades (nouvelle direction du soleil + ombre) et une auréole autour de la
  voiture (ombre de contact allégée à 0,55, RG-05).
- [ ] **Deux captures à 1 m d'écart, bords stables** : NON DÉMONTRÉ. Le journal confirme 1,07 m d'écart à 234 km/h,
  mais `ombres-a` / `ombres-b` ne montrent aucun bord d'ombre au sol à comparer ; les façades de droite y sont
  entièrement dans l'ombre. Pas de crénelage visible, faute de bord à observer.
- [~] **Nuit : aucune ombre solaire, ombre de contact conservée** (`ombres-coupees`) : pas d'ombre solaire ; une légère
  ombre de contact est visible sous l'arrière de la Golf. En revanche la route a la même luminosité que dans `ombres-a`
  (moyenne RVB 51/59/74 contre 53/61/76) : couper les ombres ne change rien au sol, ce qui confirme le défaut ci-dessus.
- [ ] **Coût ≤ 3 ms ; pas de régression de fidélité** : fidélité OK (« mode exact : 39 voitures comparées au jeu web,
  0 écart(s) »). Coût : de 16,4 à 25,2 ms GPU par passe d'ombre, carte 2048, sur lavapipe (GPU logiciel). Ce n'est pas
  représentatif d'un téléphone et ne permet pas de conclure. La carte 1024 « mobile » de RG-02 n'est pas mesurée.

## Constats par capture
- `course-cam0-0`, `course-cam0-1` : voiture visible, image non déformée. Ombre nette sur la façade de droite.
  **Aucune ombre de la Golf ni des lampadaires et arbres sur la route ou le trottoir.**
- `course-cam0-2` : même constat. Les arbres au premier plan à droite ne projettent rien sur le trottoir.
- `course-cam1-0` … `course-cam1-2` (vue haute) : trottoirs et arbres bien visibles, **aucune ombre au sol**.
  Même avec un contraste poussé (égalisation), le trottoir reste uniforme au pied des arbres et des lampadaires.
- `vitrine-m4-1` : l'arbre et le lampadaire de gauche projettent leur ombre **sur la façade** (preuve que la carte
  d'ombre fonctionne). Rien au sol sous la M4.
- `vitrine-gallardo-0`, `vitrine-golf-0`, `vitrine-p911-2` : voitures complètes, couleurs correctes, **aucune ombre
  portée sur la chaussée**.
- `ombres-a`, `ombres-b` : rendu correct, rien de cassé. Pas d'ombre au sol (voir critère 2).
- `ombres-coupees` : correct pour la nuit simulée (voir critère 3).
- Acné d'ombre (rayures), peter-panning, coupure au bord de la boîte : **rien vu**. Ce n'est pas probant tant que le sol ne
  reçoit pas d'ombre.
- Pas d'écran noir, pas de couleurs aberrantes, pas d'objet flottant.

## Ce qu'il faut corriger
Les surfaces horizontales (route, trottoirs, sol) doivent recevoir l'ombre du soleil. À vérifier côté développeur :
le terme `sunShadow` est-il appliqué au matériau route/trottoir ? La profondeur de ces surfaces dans la boîte d'ombre
(test `c.z >= 1` qui renvoie « éclairé ») et le sens du biais conviennent-ils ? Les arbres, lampadaires et voitures
sont-ils bien dessinés dans la passe d'ombre ? Puis republier les captures : on doit voir au minimum l'ombre de la
voiture du joueur et celles des arbres et lampadaires sur le trottoir dans `course-cam1-*`, et un bord d'ombre au sol dans
`ombres-a` / `ombres-b` pour juger la stabilité.

## Limites
- Coût GPU mesuré sur lavapipe : à remesurer sur un vrai GPU mobile (carte 1024) pour le seuil de 3 ms.
- Pas encore de mode nuit dans le jeu : `ombres-coupees` le simule (ombres forcées à zéro, soleil conservé).
