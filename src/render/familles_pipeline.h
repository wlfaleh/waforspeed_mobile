// WaForSpeed — moteur 40 img/s : familles de pipelines de la scène (constante MASQUE_FAMILLE de shaders/common.glsl).
//
// Pourquoi : scene.frag dessine tout (route, trottoirs, sols, décor, eau, peinture, vitres…) dans un seul shader de
// ≈ 20 000 instructions SPIR-V. Même si un objet n'emprunte qu'une branche, le GPU réserve les registres du pire cas pour
// chaque pixel : sur un GPU mobile (Adreno 660 du S21 FE, Mali), beaucoup moins de pixels sont calculés en même temps.
// Chaque famille ci-dessous est le même shader, spécialisé au chargement avec le masque des drapeaux qu'elle sait dessiner :
// le pilote retire tout le reste (mesures sous MASQUES).
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

// drapeaux que toutes les familles savent lire (textures, découpe, deux faces, occlusion précalculée, mélange alpha : le même
// masque sert aux opaques et aux transparents de la famille)
constexpr uint32_t COMMUNS = MELANGE | DOUBLE | DECOUPE | NORMALES | METAL_RUGOSITE | TANGENTE | OCCLUSION;

enum Famille : int { COMPLET = 0, FAM_ROUTE = 1, FAM_TROTTOIR = 2, FAM_SOL = 3, FAM_DECOR = 4, FAM_VOITURE = 5, FAM_VITRE = 6, NOMBRE = 7 };

// masque de chaque famille (indice = Famille) ; COMPLET : tous les drapeaux (shader d'avant, inchangé). Instructions SPIR-V après
// spécialisation (spirv-opt -O, opaques sans découpe) : complet 19 959, route 7 409, trottoir 4 769, sol 8 049, décor 4 030,
// voiture 5 330, vitre 5 391
constexpr uint32_t MASQUES[NOMBRE] = {
    0xFFFFFFFFu,
    COMMUNS | ROUTE,
    COMMUNS | TROTTOIR,
    COMMUNS | SOL,
    COMMUNS | VEGETATION | EMISSION_TEXTURE | INTERIEUR | FACADE,
    COMMUNS | PEINTURE | FEU_ARRIERE | PHARE | ROUE | EMISSION_TEXTURE | FANTOME,
    COMMUNS | VITRE | PEINTURE | EMISSION_TEXTURE,   // peinture gardée : une vitre de voiture peut porter la couleur 2 du toit
};

// vrai si la famille f sait dessiner tous les drapeaux du matériau
constexpr bool contient(int f, uint32_t drapeaux) { return f >= 0 && f < NOMBRE && (drapeaux & ~MASQUES[f]) == 0u; }

// famille d'un objet : la plus légère qui sait tout dessiner, sinon le shader complet. voiture = objet dessiné comme une voiture
// (peinture 2 du toit possible même sans le drapeau PEINTURE : jamais en décor, route, trottoir ni sol)
constexpr int choisir(uint32_t drapeaux, bool voiture) {
    if (!voiture) {
        if (contient(FAM_DECOR, drapeaux)) return FAM_DECOR;
        if (contient(FAM_TROTTOIR, drapeaux)) return FAM_TROTTOIR;
        if (contient(FAM_ROUTE, drapeaux)) return FAM_ROUTE;
        if (contient(FAM_SOL, drapeaux)) return FAM_SOL;
    }
    if (contient(FAM_VOITURE, drapeaux)) return FAM_VOITURE;
    if (contient(FAM_VITRE, drapeaux)) return FAM_VITRE;
    return COMPLET;
}

}  // namespace famille_pipeline
