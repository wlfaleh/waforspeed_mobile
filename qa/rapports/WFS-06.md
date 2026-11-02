# QA — WFS-06 : Ville : bâtiments, trottoirs, lampadaires, arbres
Date : 2026-11-02 · Branche : feature/waforspeed-06 · Release : apercu-waforspeed-06

## Verdict : VALIDÉ (avec limites)

## Captures regardées (1 montage de 6 images, PNG téléchargés depuis la Release)
- `course-cam0-0` (banc d'essai, caméra arrière) : immeubles des deux côtés de la rue, trottoirs bordés de lampadaires
  (bras au-dessus de la route), rangée d'arbres sur le trottoir au loin, voiture entière et bien posée. Rien ne flotte
  ni ne traverse la route. ✅
- `course-cam0-2` (même caméra, plus loin) : immeubles, arbre sur le trottoir de droite, lampadaire et arbre de gauche
  à l'échelle plausible. ✅
- `course-cam1-1` (caméra latérale/éloignée) : route qui s'incurve, arbres et immeubles le long des trottoirs, voiture
  visible. ✅
- `vitrine-m4-0` : rue bordée d'immeubles, lampadaires et arbres sur les trottoirs, image en paysage non déformée. ✅
- `qa-01-demarrage` (émulateur, 2400x1080) : paysage correct, immeubles des deux côtés, arbres sur les trottoirs, Golf
  entière. ✅
- `qa-07-fin` : rien de cassé, scène toujours rendue, appli en marche. ✅
- `qa-03-acceleration-8s` : **fichier identique octet pour octet à `qa-01-demarrage`** (même empreinte MD5). Le log
  annonce pourtant 0 → 11 km/h et 1 m parcourus. La capture ne prouve donc pas le déplacement. Gravité : **majeur**
  (preuve de QA défaillante, pas un défaut visible du décor). Non bloquant pour le verdict visuel.

## Journal (qa-logcat.txt)
- « Ville : 899 immeubles, 222 lampadaires, 439 arbres » et « QA pret : moteur initialise » : décor généré au démarrage.
- « Systeme : Ville (ordre 100) » : système enfichable actif.
- Aucune erreur, aucun plantage, pas d'avertissement de rendu.

## Limites (non vérifiables avec ces captures)
- Nombre d'appels de dessin (≤ 300, ≈ 150 annoncés) : absent des captures et du log. À mesurer par le développeur.
- Graine de la carte : le décor est visiblement généré, mais la reproductibilité par graine n'est pas visible ici.
- L'émulateur tourne à ~0,2 image/s (SwiftShader). Performances réelles à mesurer sur téléphone.
- Le sujet « lampadaires et arbres » est jugé à l'échelle des captures : ils sont petits à l'écran, mais bien présents.
