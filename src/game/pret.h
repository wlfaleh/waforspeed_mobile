// WaForSpeed — voiture prêtée par le cadeau du jour (WFS-502, épic E19, zone E3) : état du prêt, activation sous 24 h, prêt de 3 h,
// heure du téléphone qui recule, fin du prêt et message « Garde-la pour toujours ». Logique seulement, sans écran (le garage, le menu
// et l'écran du cadeau l'affichent : garage_achat.cpp, pret_bandeau.cpp, cadeau_jour.cpp).
//
// Règles (regles-de-gestion/WFS-502.md, règles communes RC-E19-01 à 07 de regles-de-gestion/E19-cadeau-du-jour.md) :
//   - RG-01 : la voiture tirée est gardée (pret-voiture, pret-recu) ; « Activer » sous cadeau.activation_heures (24 h), sinon le prêt
//     est perdu (message « Prêt expiré » une fois) ; un seul prêt à la fois (le tirage ne donne pas de voiture pendant un prêt) ;
//   - RG-02 : activation : pret-fin = maintenant + cadeau.pret_heures (3 h) ; la voiture est utilisable dans tous les modes, comme
//     une voiture possédée (garage_achat.cpp : voitureBloquee) ; les étoiles se gagnent normalement (rien ici) ;
//   - RG-04 : le temps restant ne peut jamais augmenter : on garde le dernier temps restant vu (pret-restant-vu) ; si l'heure du
//     téléphone recule, la fin est recalée sur maintenant + ce temps (il reprend ensuite sa course, jamais plus long) ;
//   - RG-05 : fin du prêt seulement HORS course (une course commencée avant la fin se termine normalement, gains comptés) : la voiture
//     redevient non possédée ; si elle était la voiture choisie, le choix revient à la voiture d'avant (pret-avant) ; message
//     « Garde-la pour toujours : <prix> ★ » une fois (pret-message) ;
//   - RG-06 : tout est dans la sauvegarde (pret-voiture, pret-recu, pret-fin, pret-restant-vu, pret-avant, pret-message) : relancer
//     le jeu ne remet pas de temps. L'appelant écrit la sauvegarde (enregistrer) quand une fonction rend « changé ».
// Durées lues dans config().cadeau à chaque appel (RC-E19-02). Thread principal (comme la sauvegarde).
#pragma once
#include <string>
#include "sauvegarde.h"

namespace wfs::pret {

enum class Etat { Aucun, EnAttente, Actif };   // aucun prêt ; voiture gagnée pas encore activée ; prêt en cours
Etat etat(const EtatCadeau& c);
// la voiture est prêtée et le prêt est actif (pas encore rendu) : utilisable comme une voiture possédée
bool utilisable(const Sauvegarde& s, const std::string& carId);

// secondes qu'il reste pour appuyer sur « Activer » (EnAttente), 0 sinon ; heure reculée : jamais plus que le délai entier
long long resteActivation(const EtatCadeau& c, long long maintenant);
// secondes de prêt qu'il reste (Actif), 0 sinon : min(pret-fin − maintenant, pret-restant-vu), jamais négatif (RG-04)
long long restant(const EtatCadeau& c, long long maintenant);

// RG-01 : voiture gagnée par le cadeau (appelée par cadeau::ouvrir avant l'écriture de la sauvegarde)
void recevoir(EtatCadeau& c, const std::string& voiture, long long maintenant);
// RG-02 : « Activer » ; vrai si le prêt vient de commencer (sinon rien de changé : pas de prêt en attente, ou délai dépassé)
bool activer(Sauvegarde& s, long long maintenant);

// RG-01, RG-04, RG-05 : à appeler hors course (menu, garage, terrain : système pret_system.cpp ; au lancement).
//   enCourse = vrai : rien n'est rendu ni perdu (une course commencée se termine normalement) ; seul RG-04 est appliqué.
enum class Evenement { Rien, Change, Expire, Fini };   // Change : seulement pret-restant-vu ou pret-avant mis à jour
Evenement mettreAJour(Sauvegarde& s, long long maintenant, bool enCourse = false);

// message à montrer une fois au menu (RG-01, RG-05) : vide si aucun
enum class Message { Aucun, Expire, Fin };
Message message(const EtatCadeau& c, std::string* voiture = nullptr);
void oublierMessage(EtatCadeau& c);   // le joueur l'a vu

// heure du jeu : celle du téléphone (cadeau::maintenant), ou celle forcée par un banc d'essai (0 : aucune)
long long maintenant();
void forcerHeure(long long epoch);

} // namespace wfs::pret
