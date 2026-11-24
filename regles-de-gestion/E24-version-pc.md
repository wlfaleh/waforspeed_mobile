# Épic E24 — Version PC Windows

**Demande de l'utilisateur (2026-11-24)** : une version PC Windows du jeu, avec les mêmes fonctions, livrée par un **installateur Windows** (pas de Steam), **sans achats sur PC** au début, et **cross-play** : un joueur Android / iOS joue en duel contre un joueur PC (le duel passe déjà par le relais Cloudflare `waforspeed-relais.wlfaleh.workers.dev`, indépendant de la plateforme). Sujet confié en urgence au lead dev en renfort. La priorité Android reste : aucun ticket de E24 ne change le jeu Android.

| Ordre | Ticket | Sujet | Dépend de |
|---|---|---|---|
| 1 | WFS-602 (P1) | Fenêtre Windows, boucle, clavier / souris / manette, installateur `.exe` (MinGW + NSIS) | — |
| 2 | WFS-603 | Son PC (WASAPI) | 602 |
| 3 | WFS-604 | Réseau TLS Windows + duel cross-play PC ↔ Android | 602 |
| 4 | WFS-605 | Téléchargement des paquets de cartes et sauvegarde sur PC (`@cartes`) | 602 |
| 5 | WFS-606 | Réglages graphiques PC | 602 |
| 6 | WFS-607 | [TECHNIQUE] CI Windows : construction de l'installateur (`@cicd`) | 602 |
| 7 | WFS-608 | Équité du duel PC / téléphone : **décision de l'utilisateur à prendre** (même physique pour tous ; question : aide de direction imposée ? classement séparé ?) | 604 |

Interdits : Steam, achats sur PC (`achats_*` non porté), iOS (gel), toute modification du comportement Android.
