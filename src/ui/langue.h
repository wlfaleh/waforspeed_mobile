// WaForSpeed — système de langue (WFS-504, épic E20) : le jeu parle français sur un téléphone en français, anglais sur tous
// les autres ; le joueur peut forcer la langue (réglage « Langue » : Automatique / Français / English).
// Utilisation (tout texte visible, RC-E20-01) :
//   ui::text(r, x, y, px, ui::tr("reglages.titre"), ...);
//   ui::tr("hud.tour", ui::nombre(t), ui::nombre(n))   // « Tour {1}/{2} » / « Lap {1}/{2} »
// Tables : assets/textes/fr.txt et assets/textes/en.txt (« cle=texte », règles : langue_regles.h), lues une fois au chargement.
#pragma once
#include <string>
#include <vector>
#include "langue_regles.h"

namespace wfs {
namespace ui {

// RG-05 : texte de la clé dans la langue active ; clé absente : texte français + trace au journal (une fois) ; absente des
// deux tables : texte vide (jamais la clé brute à l'écran)
const std::string& tr(const char* cle);
std::string tr(const char* cle, const std::string& v1, const std::string& v2 = "");
// texte de la clé dans une langue donnée (mesure de la taille fixe d'un bouton : RC-E20-04)
const std::string& trLangue(const char* cle, langue::Langue l);

namespace langue {
bool charger();                                 // lit les deux tables (readAsset), une seule fois ; faux si une table manque
void definirLangueTelephone(const std::string& code);   // RG-01 : Android (JNI, avant le thread du jeu) ; banc : WFS_LANGUE
std::string langueTelephone();
void choisir(Choix c);                          // RG-02, RG-03 : appliqué tout de suite (la prochaine image est dans la langue)
Choix choix();
Langue active();
const char* code();                             // « fr » ou « en » (journal, banc)

// RG-07, RG-08 : journal des textes affichés, en mode banc seulement (aucune allocation sinon)
struct TexteVu { std::string ecran, texte; bool technique; };
struct Debord { std::string ecran, texte, cle; float largeur, place; };
void journalActif(bool oui, const char* ecran = "");   // ecran : nom donné aux textes notés ensuite (« reglages-affichage »)
bool journalOuvert();
// lignes techniques de mesure (rendu, mémoire, images/s : textes d'autres zones, à traduire par un autre ticket) : notées
// à part et listées par verifierJournal, sans échec
void journalTechnique(bool oui);
void noter(const std::string& texte);           // appelé par ui::text
void noterDebord(const std::string& texte, float largeur, float place);   // appelé par ui::textBoite
const std::vector<TexteVu>& textesVus();
const std::vector<Debord>& debords();
void viderJournal();
std::string cleDuTexte(const std::string& texte);   // clé de la table active dont le texte (ou ses capitales) est celui-ci
// RG-07, RG-08 : vérifie le journal (textes notés depuis viderJournal) ; écrit chaque échec au journal, rend leur nombre.
// Rien n'est vérifié en français (les accents y sont attendus).
int verifierJournal();
} // namespace langue

} // namespace ui
} // namespace wfs
