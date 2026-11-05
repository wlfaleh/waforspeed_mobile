# Relais du « Duel entre amis » (WFS-112)

Petit serveur WebSocket gratuit (Cloudflare Workers) qui relaie les positions des joueurs d'un même code de salon.
Il marche en 4G et derrière toutes les box. Il ne garde aucune donnée personnelle et n'a pas de chat.

## Mise en place (5 lignes)

1. Crée un compte gratuit sur https://dash.cloudflare.com/sign-up (adresse e-mail et mot de passe suffisent).
2. Sur un ordinateur avec Node.js 18 ou plus : `cd serveur && npx wrangler login` (une page s'ouvre, clique « Allow »).
3. Déploie : `npx wrangler deploy`. Wrangler affiche l'adresse du relais, par exemple `https://waforspeed-relais.<ton-sous-domaine>.workers.dev`.
4. Ouvre cette adresse dans un navigateur : tu dois lire « relais WaForSpeed v1 ».
5. Donne cette adresse à l'agent (ou mets-la dans `RELAIS_HOTE`, `src/game/reseau_relais.h`) : le jeu suivant l'utilise.

Sans ordinateur : crée plutôt un jeton d'API (Mon profil → Jetons d'API → modèle « Modifier Cloudflare Workers ») et
donne-le à l'agent avec ton identifiant de compte : il déploiera avec `CLOUDFLARE_API_TOKEN=… npx wrangler deploy`.

## Essai en local

- `node serveur/local.js` : même relais sur `ws://127.0.0.1:8787` (aucune bibliothèque à installer).
- `node serveur/test/bout_en_bout.js` : deux clients simulés, messages hostiles, salon plein, reconnexion, code expiré.
- `build/test_relais` (tests CPU du jeu) lance le relais local et y fait jouer deux clients du jeu, de bout en bout.

## Fichiers

- `src/salon.js` : règles d'un salon (protocole, 4 joueurs, code valable 15 min, place gardée 30 s après une coupure,
  messages bornés) ; utilisé tel quel par Cloudflare et par le relais local.
- `src/index.js` : Worker Cloudflare (une Durable Object par code de salon) et page du lien d'invitation `/d/<code>`.
- `wrangler.toml` : configuration du déploiement (plan gratuit).
- `local.js`, `src/ws_mini.js` : relais local pour les tests (WebSocket minimale avec les modules de Node).
