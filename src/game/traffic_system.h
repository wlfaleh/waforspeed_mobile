// WaForSpeed — module « trafic » (WFS-08) branché sur le moteur par les systèmes enfichables (WFS-30).
// Simulation : traffic.h. Ce module la fait avancer, applique les chocs à la voiture du joueur, dessine les voitures
// et aide le pilote automatique à éviter le trafic. Accès pour le banc d'essai : trafficSystem().
#pragma once
#include <string>
#include "../core/system.h"
#include "traffic.h"

namespace wfs {

class Game;

struct TrafficStats { int nearMisses = 0, accidents = 0, scrapes = 0, bumps = 0; };

// état du module actif (nullptr si aucune partie) : lecture pour le banc d'essai et, plus tard, le HUD (WFS-12)
const Traffic* activeTraffic();
const TrafficStats* activeTrafficStats();
Traffic* activeTrafficMut();                 // course (WFS-12) : +6 voitures à chaque checkpoint
void trafficRestart(double playerPos);       // course (WFS-12) : « Rejouer », même graine de trafic
void trafficRestartSeed(uint32_t seed, double playerPos);   // duel (WFS-21) : graine commune de la manche
// banc d'essai : au prochain pas, place le joueur `metres` derrière la voiture la plus entourée, décalé de `lat`
// (demi-largeurs de route), à sa vitesse + `extraKmh` ; `holdLane` : garde sa file sans freiner (scène de choc)
void trafficBenchPlace(double metres, double lat, double extraKmh, bool holdLane);
int trafficBenchCar();
// WFS-281 (RG-02) : médianes, sur les `images` dernières images dessinées (128 au plus), des appels de dessin, triangles et
// véhicules dessinés par le trafic ; faux sans partie
bool trafficMedianes(int images, double& appels, double& triangles, double& dessines);   // indice de la voiture visée par le dernier placement
// scènes du banc d'essai du trafic (captures trafic-*.png), appelées par src/platform/desktop_main.cpp
void benchTrafficScenes(Game& g, Renderer& r, const std::string& outDir);
// WFS-67 : trafic vivant (captures trafic-clignotant.png, trafic-freinage.png, trafic-accident.png)
void benchTraficVivant(Game& g, Renderer& r, const std::string& outDir);
// WFS-232 : deux-roues du trafic seuls (trafic-motos-*, Ville et Corniche, jour et nuit ; aussi à la fin de benchTraficVivant)
void benchDeuxRoues(Game& g, Renderer& r, const std::string& outDir);

} // namespace wfs
