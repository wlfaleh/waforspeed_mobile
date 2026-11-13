// WaForSpeed — suivi des courses pour le profil du joueur (WFS-480, RG-07, RG-10) : voir profil_system.cpp.
#pragma once
#include "profil.h"

namespace wfs {

// résultat de la course en cours, donné par le système du mode à son arrivée (course_system, course5_system,
// poursuite_system, duel_system) : mode, victoire, place, tempsMs et score seulement ; le reste (voiture, distance,
// vitesse maximale, frôlements, date) est relevé par le ProfilSystem. Sans appel, la course est notée sans résultat.
void profilNoterResultat(const CourseJouee& resultat);

} // namespace wfs
