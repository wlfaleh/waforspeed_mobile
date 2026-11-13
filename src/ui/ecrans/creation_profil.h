// WaForSpeed — écran « Crée ton pilote » au 1er lancement (WFS-481, épic E17, zone E8).
// RG-01 : montré au menu principal, après le chargement et l'acceptation des CGU (WFS-308), tant que le profil n'est pas créé
//         (clé profil=0 de la sauvegarde, WFS-480 RG-06) ; jamais ensuite. Joueur déjà installé : son pseudo est proposé.
// RG-02 : un seul écran : titre, champ Pseudo, champ Plaque (proposée d'après le pseudo), aperçu 2D de la plaque, « C'est parti ».
// RG-03 : clavier du jeu (creation_profil_saisie.h), touches de 9 mm au moins (57 dp).
// RG-06 : textes par ui::tr (assets/textes/fr.txt et en.txt, clés creation.*), langue du téléphone ; banc : WFS_LANGUE.
// RG-07 : « C'est parti » écrit le profil (Profil::creer, pseudo dans la clé nick, aussi pour le duel), puis le menu.
// RG-08 : touche Retour d'Android : comme au menu (le jeu passe en arrière-plan) ; l'écran revient à la reprise.
// Deux systèmes : CreationProfilToucher (ordre 401, prend tous les touchers avant le menu et ses bandeaux) et CreationProfil
// (ordre 459, dessin par-dessus le menu, ses bandeaux et le panneau des modes, sous les Réglages 460).
#pragma once
#include <string>

namespace wfs {
class Game;
class Renderer;
namespace ecrans {

// RG-01 : écran ouvert (le menu ne dessine rien dessous, le lien d'invitation du duel attend la création du profil)
bool creationProfilOuverte();

// banc d'essai (WFS_BANC=creation, WFS_LANGUE=fr|en) : 1er lancement (sauvegarde du banc) → CGU acceptées → création → menu →
// course → fin → menu ; 2e lancement sans écran ; erreurs RG-04 ; chrono du scénario RG-05. Rend le nombre d'écarts.
int benchCreationProfil(Game& g, Renderer& r, const std::string& out);

} // namespace ecrans
} // namespace wfs
