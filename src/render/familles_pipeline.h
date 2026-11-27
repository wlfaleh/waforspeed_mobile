// WaForSpeed — moteur 40 img/s : familles de pipelines de la scène (constante MASQUE_FAMILLE de shaders/common.glsl).
//
// Pourquoi : scene.frag dessine tout (route, trottoirs, sols, décor, eau, peinture, vitres…) dans un seul shader de
// ≈ 20 000 instructions SPIR-V. Même si un objet n'emprunte qu'une branche, le GPU réserve les registres du pire cas pour
// chaque pixel : sur un GPU mobile (Adreno 660 du S21 FE, Mali), beaucoup moins de pixels sont calculés en même temps.
// Chaque famille ci-dessous est le même shader, spécialisé au chargement avec le masque des drapeaux qu'elle sait dessiner :
// le pilote retire tout le reste (mesuré avec spirv-opt, SANS_DISCARD vrai : complet 19 959, route 7 403, trottoir et sol
// 9 536, décor 4 024, voiture 5 325 instructions).
//
// Règle : un objet va dans la plus petite famille dont le masque contient TOUS les drapeaux de son matériau. Le shader
// spécialisé fait alors exactement les mêmes calculs que le shader complet : même image au pixel près (test_familles_pipeline).
// Fichier pur (aucun Vulkan) : testé sur le processeur.
#pragma once
#include <cstdint>

namespace famille_pipeline {

// drapeaux de matériau (mêmes valeurs que MatFlags de renderer.h et F_* des shaders)
constexpr uint32_t PEINTURE = 1, MELANGE = 2, FEU_ARRIERE = 4, PHARE = 8, DOUBLE = 16, DECOUPE = 32, VITRE = 64, ROUE = 128,
                   ROUTE = 256, TROTTOIR = 512, SOL = 1024, OMBRE_TACHE = 2048, NORMALES = 4096, METAL_RUGOSITE = 8192,
                   VEGETATION = 16384, TACHE_PHARES = 32768, ADDITIF = 65536, FANTOME = 131072, EMISSION_TEXTURE = 262144,
                   TANGENTE = 524288, INTERIEUR = 1048576, FACADE = 2097152, OCCLUSION = 4194304, EAU = 8388608;

// drapeaux que toutes les familles savent lire (textures, découpe, deux faces, occlusion précalculée)
constexpr uint32_t COMMUNS = DOUBLE | DECOUPE | NORMALES | METAL_RUGOSITE | TANGENTE | OCCLUSION;

enum Famille : int { COMPLET = 0, FAM_ROUTE = 1, FAM_SOL = 2, FAM_DECOR = 3, FAM_VOITURE = 4, NOMBRE = 5 };

// masque de chaque famille (indice = Famille) ; COMPLET : tous les drapeaux (shader d'avant, inchangé)
constexpr uint32_t MASQUES[NOMBRE] = {
    0xFFFFFFFFu,
    COMMUNS | ROUTE,
    COMMUNS | TROTTOIR | SOL,
    COMMUNS | VEGETATION | EMISSION_TEXTURE | INTERIEUR | FACADE,
    COMMUNS | PEINTURE | FEU_ARRIERE | PHARE | ROUE | EMISSION_TEXTURE | FANTOME,
};

// vrai si la famille f sait dessiner tous les drapeaux du matériau
constexpr bool contient(int f, uint32_t drapeaux) { return f >= 0 && f < NOMBRE && (drapeaux & ~MASQUES[f]) == 0u; }

// famille d'un objet opaque : décor d'abord (le plus léger), puis route, voiture, trottoir et sol, sinon le shader complet.
// voiture = objet dessiné comme une voiture (peinture 2 du toit possible même sans le drapeau PEINTURE : jamais en décor)
constexpr int choisir(uint32_t drapeaux, bool voiture) {
    if (!voiture && contient(FAM_DECOR, drapeaux)) return FAM_DECOR;
    if (!voiture && contient(FAM_ROUTE, drapeaux)) return FAM_ROUTE;
    if (contient(FAM_VOITURE, drapeaux)) return FAM_VOITURE;
    if (!voiture && contient(FAM_SOL, drapeaux)) return FAM_SOL;
    return COMPLET;
}

}  // namespace famille_pipeline
