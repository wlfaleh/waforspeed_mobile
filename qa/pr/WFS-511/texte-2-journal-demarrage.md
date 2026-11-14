# WFS-511 — capture 2 : journal du démarrage (connexion, produits demandés, rattrapage, aucun jeton complet)

**Limite (dite franchement)** : ce n'est pas un `logcat` de l'APK. Dans ce conteneur il n'y a ni NDK, ni émulateur, et le dépôt Maven de Google est bloqué par le proxy de la session (`regles-reseau.md` : pas de contournement) ; la CI de branche n'est pas permise pour ce ticket. Le vrai `logcat` viendra de l'APK de `main` (émulateur des bancs du matin) et de l'essai avec la Play Console (WFS-515).

## 2a. Lignes que `Achats.java` écrit au démarrage (texte exact du code, balise `WaForSpeed`, préfixe `ACHATS`)

Ordre au lancement : `démarrage` → `connecté au service Google Play` → `produits demandés à Google` → `produits : N trouvé(s)` → `rattrapage : N achat(s) renvoyé(s) au jeu`. Aucun de ces messages ne contient de jeton ni d'identifiant de compte.

```
Achats.java:87  JOURNAL.ecrire("service de facturation indisponible : " + e.getClass().getSimpleName());
Achats.java:91  JOURNAL.ecrire("démarrage : " + produitsDemandes.size() + " produit(s) : " + produitsDemandes);
Achats.java:118  JOURNAL.ecrire("achat demandé : " + produit + " (code " + r.getResponseCode() + ")");
Achats.java:126  PRINCIPAL.post(new Runnable() { @Override public void run() { JOURNAL.ecrire("restauration demandée"); if (pret()) rattraper(); else connecter(); } });
Achats.java:161  JOURNAL.ecrire("connecté au service Google Play");
Achats.java:165  JOURNAL.ecrire("connexion refusée (code " + r.getResponseCode() + ") : nouvel essai dans " + attenteMs / 1000 + " s");
Achats.java:175  JOURNAL.ecrire("service Google Play coupé : reconnexion dans " + attenteMs / 1000 + " s");
Achats.java:183  JOURNAL.ecrire("connexion impossible : " + e.getClass().getSimpleName());
Achats.java:198  JOURNAL.ecrire("produits demandés à Google : " + produitsDemandes);
Achats.java:209  } else JOURNAL.ecrire("produits non reçus (code " + r.getResponseCode() + ")");
```

## 2b. Même séquence rejouée sur la JVM (partie règles, `LogiqueAchats`) : produits de Google, rattrapage d'un achat interrompu, hors ligne

```
▶ prixDeGoogle
ACHATS produit introuvable chez Google : etoiles_or (pack masqué)
ACHATS produits : 2 trouvé(s) sur 3 demandé(s)
  OK

▶ rattrapageAuLancement
ACHATS etoiles_bronze abcdefgh… : acheté, envoyé au jeu
ACHATS pack_debutant zyxwvuts… : acheté, envoyé au jeu
ACHATS rattrapage : 2 achat(s) renvoyé(s) au jeu
ACHATS etoiles_bronze abcdefgh… : crédité par le jeu, consommation demandée
ACHATS pack_debutant zyxwvuts… : crédité par le jeu, confirmation demandée
  OK

▶ horsLigne
ACHATS rattrapage impossible (code 3) : nouvel essai au prochain retour au jeu
ACHATS produit introuvable chez Google : etoiles_bronze (pack masqué)
ACHATS produit introuvable chez Google : sans_pub (pack masqué)
ACHATS produits : 0 trouvé(s) sur 2 demandé(s)
  OK

▶ journalSansJetonComplet
ACHATS etoiles_or abcdefgh… : acheté, envoyé au jeu
ACHATS etoiles_or abcdefgh… : crédité par le jeu, consommation demandée
ACHATS abcdefgh… : consommé / confirmé chez Google
ACHATS rattrapage : 0 achat(s) renvoyé(s) au jeu
  OK

```

Contrôle RG-08 (test `journalSansJetonComplet`) : aucune ligne du journal ne contient le jeton complet ni plus de ses 8 premiers caractères (`abcdefgh…`).
