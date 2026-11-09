# WaForSpeed Mobile — moteur natif C++20 / Vulkan
cmake_minimum_required(VERSION 3.22)
project(WaForSpeed LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---------- shaders GLSL -> SPIR-V (inclus dans le binaire) ----------
if(ANDROID)
  # glslc est fourni par le NDK (shader-tools/<hôte>/glslc)
  file(GLOB GLSLC_FOUND "${ANDROID_NDK}/shader-tools/*/glslc")
  list(LENGTH GLSLC_FOUND GLSLC_N)
  if(GLSLC_N EQUAL 0)
    message(FATAL_ERROR "glslc introuvable dans ${ANDROID_NDK}/shader-tools")
  endif()
  list(GET GLSLC_FOUND 0 GLSLC)
else()
  find_program(GLSLC glslc REQUIRED)
endif()
message(STATUS "Compilateur de shaders : ${GLSLC}")
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_OUT ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT})
set(SHADER_INCS)
file(GLOB SHADER_GLSL CONFIGURE_DEPENDS ${SHADER_DIR}/*.glsl)   # WFS-149 (RG-05) : route, vehicule, decor, eclairage
foreach(S scene.vert scene.frag sky.vert sky.frag post.vert post.frag ui.vert ui.frag shadow.vert shadow.frag scene_skin.vert shadow_skin.vert scene_skin.frag prepasse.frag plein_ecran.vert passe_essai.frag)   # WFS-150 : pré-passe, passe d'essai
  add_custom_command(
    OUTPUT ${SHADER_OUT}/${S}.inc
    COMMAND ${GLSLC} -O --target-env=vulkan1.0 -mfmt=num -I ${SHADER_DIR} -o ${SHADER_OUT}/${S}.inc ${SHADER_DIR}/${S}
    DEPENDS ${SHADER_DIR}/${S} ${SHADER_DIR}/common.glsl ${SHADER_GLSL}   # WFS-149 : fichiers inclus par domaine
    COMMENT "Shader ${S}")
  list(APPEND SHADER_INCS ${SHADER_OUT}/${S}.inc)
endforeach()
add_custom_target(shaders DEPENDS ${SHADER_INCS})

# sources du moteur : tous les .cpp des dossiers du jeu sont pris automatiquement (WFS-30) —
# un nouveau module = un nouveau fichier, sans modifier ce fichier
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/render/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/game/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/world/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/audio/*.cpp)
list(APPEND ENGINE_SOURCES src/platform/manette.cpp src/platform/manette_jeu.cpp)   # WFS-22 : manettes (Android, banc)
list(APPEND ENGINE_SOURCES src/platform/retour_systeme.cpp)   # WFS-116 : touche Retour du téléphone (Android ; compilé aussi au banc)
list(APPEND ENGINE_SOURCES src/platform/chauffe.cpp src/platform/chauffe_systeme.cpp)   # WFS-123 : chauffe, 30 / 60 images/s
list(APPEND ENGINE_SOURCES src/platform/plantage.cpp)   # WFS-126 : rapport de plantage (Android, banc)
# WFS-279 : version de l'écran de chargement : versionName de android/app/build.gradle (seule source, -DWFS_VERSION) ;
# PC, bancs et tests : « dev » (captures identiques d'un run à l'autre) ; seuls chargement.cpp et son test la reçoivent
set(WFS_VERSION "dev" CACHE STRING "Version de l'APK affichée à l'écran de chargement (WFS-279)")
set_source_files_properties(src/ui/ecrans/chargement.cpp tests/test_version_chargement.cpp PROPERTIES COMPILE_DEFINITIONS "WFS_VERSION=\"${WFS_VERSION}\"")

if(MSVC)
  set(WARN /W3)
else()
  set(WARN -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers)
endif()

if(ANDROID)
  add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
  target_include_directories(native_app_glue PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)
  add_library(waforspeed SHARED ${ENGINE_SOURCES} src/platform/android_main.cpp src/platform/manette_android.cpp)
  add_dependencies(waforspeed shaders)
  target_include_directories(waforspeed PRIVATE ${SHADER_OUT})
  target_compile_options(waforspeed PRIVATE ${WARN})
  set_target_properties(waforspeed PROPERTIES LINK_FLAGS "-u ANativeActivity_onCreate")
  target_link_libraries(waforspeed native_app_glue android log vulkan aaudio)   # aaudio : son (WFS-10)
elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
  include(ios/ios.cmake)   # WFS-22 : application iOS (MoltenVK), voir ios/ios.cmake et .github/workflows/ios.yml
else()
  find_package(Vulkan REQUIRED)
  find_package(Threads REQUIRED)
  add_executable(wfs_bench ${ENGINE_SOURCES} src/platform/desktop_main.cpp src/platform/endurance_banc.cpp)   # WFS-91 : banc d'endurance
  add_dependencies(wfs_bench shaders)
  target_include_directories(wfs_bench PRIVATE ${SHADER_OUT})
  target_compile_options(wfs_bench PRIVATE ${WARN})
  target_link_libraries(wfs_bench Vulkan::Vulkan Threads::Threads)
  add_executable(test_track tests/test_track.cpp src/game/track.cpp src/game/player.cpp)
  add_executable(test_systems tests/test_systems.cpp)
  add_executable(test_traffic tests/test_traffic.cpp src/game/traffic.cpp src/game/track.cpp src/game/player.cpp src/core/jobs.cpp)
  add_executable(test_cartes tests/test_cartes.cpp src/game/track.cpp)   # WFS-14 : tracé des cartes = jeu web
  target_link_libraries(test_traffic Threads::Threads)
  add_executable(test_course tests/test_course.cpp tests/test_commandes.cpp src/game/course.cpp src/game/traffic.cpp src/game/track.cpp src/game/player.cpp src/core/jobs.cpp)
  target_link_libraries(test_course Threads::Threads)
  add_executable(test_duel tests/test_duel.cpp src/game/duel.cpp src/game/reseau_udp.cpp src/game/reseau_peerjs.cpp src/game/binarypack.cpp src/game/traffic.cpp src/game/track.cpp src/game/player.cpp src/core/jobs.cpp)   # WFS-21
  add_executable(test_binarypack tests/test_binarypack.cpp src/game/binarypack.cpp)   # WFS-89 : messages de PeerJS
  target_link_libraries(test_duel Threads::Threads)
  add_executable(test_relais tests/test_relais.cpp src/game/reseau_relais.cpp src/game/duel.cpp src/game/player.cpp src/game/track.cpp)   # WFS-112 : relais du duel entre amis
  target_link_libraries(test_relais Threads::Threads)
  target_compile_definitions(test_relais PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_executable(test_effets tests/test_effets.cpp src/render/effets_particules.cpp)   # WFS-65
  add_executable(test_roues tests/test_roues.cpp src/game/roues.cpp)   # WFS-60
  add_executable(test_trafic_vivant tests/test_trafic_vivant.cpp src/game/traffic.cpp src/game/track.cpp src/game/player.cpp src/core/jobs.cpp)   # WFS-67
  add_executable(test_trafic_duree tests/test_trafic_duree.cpp src/game/trace_cartes.cpp src/game/duel.cpp src/game/traffic.cpp src/game/track.cpp src/game/player.cpp src/game/sauvegarde.cpp src/core/jobs.cpp)   # WFS-281 : trafic stable sur 10 min
  target_link_libraries(test_trafic_duree Threads::Threads)
  target_link_libraries(test_trafic_vivant Threads::Threads)
  add_executable(test_qualite tests/test_qualite.cpp src/render/qualite_auto.cpp)   # WFS-15
  add_executable(test_cache_pipelines tests/test_cache_pipelines.cpp src/render/cache_pipelines.cpp)   # WFS-262 (RG-01)
  add_executable(test_shader_leger tests/test_shader_leger.cpp)   # WFS-264 (RG-06)
  add_executable(test_lod_distance tests/test_lod_distance.cpp)   # WFS-265 (RG-01)
  add_executable(test_chauffe tests/test_chauffe.cpp src/platform/chauffe.cpp src/render/qualite_auto.cpp)   # WFS-123 : chauffe
  add_executable(test_appui tests/test_appui.cpp)   # WFS-113 : un doigt = un appui
  add_executable(test_petit_ecran tests/test_petit_ecran.cpp)   # WFS-147 : téléphone couché en dp
  add_executable(test_version_chargement tests/test_version_chargement.cpp)   # WFS-279 : version de l'écran de chargement
  add_executable(test_minicarte tests/test_minicarte.cpp)   # WFS-173 : place et projection de la mini-carte
  add_executable(test_course5 tests/test_course5.cpp src/game/course5.cpp tests/test_poursuite.cpp src/game/poursuite.cpp src/game/sauvegarde.cpp src/game/track.cpp src/game/player.cpp)   # WFS-110 : course à 5 voitures ; WFS-111 : poursuite ; WFS-119 : sauvegarde (mode gardé) ; WFS-189 : player.cpp (courses simulées)
  add_executable(test_course5_ordinateurs tests/test_course5_ordinateurs.cpp src/game/course5.cpp src/game/duel.cpp src/game/traffic.cpp src/game/reseau_udp.cpp src/game/reseau_peerjs.cpp src/game/binarypack.cpp src/core/jobs.cpp src/game/sauvegarde.cpp src/game/trace_cartes.cpp src/game/track.cpp src/game/player.cpp)   # WFS-327 : adversaires sur une vitesse de virage possible, course disputée
  target_link_libraries(test_course5_ordinateurs Threads::Threads)
  add_executable(test_retour_systeme tests/test_retour_systeme.cpp)   # WFS-116 : touche Retour du téléphone
  add_executable(test_reglages_appui tests/test_reglages_appui.cpp)   # WFS-290 : cible de 48 dp, anti-rebond des réglages
  add_executable(test_cgu_appui tests/test_cgu_appui.cpp)   # WFS-355 : appuis des CGU (destination, cible de 48 dp, pose ou lever perdu)
  add_executable(test_reglages_onglets tests/test_reglages_onglets.cpp)   # WFS-291, WFS-318 : lignes et onglets des réglages
  add_executable(test_fantome_etiquette tests/test_fantome_etiquette.cpp src/game/fantome.cpp src/game/binarypack.cpp)   # WFS-318 : réglage et étiquette du fantôme
  add_executable(test_configgame tests/test_configgame.cpp src/core/configgame.cpp)   # WFS-128 : configgame.json
  add_executable(test_manette tests/test_manette.cpp src/platform/manette.cpp)   # WFS-22 : manettes
  add_executable(test_masquage tests/test_masquage.cpp src/render/masquage.cpp)   # WFS-82
  add_executable(test_ktx2 tests/test_ktx2.cpp src/render/ktx2.cpp)   # WFS-149 (RG-01) : textures KTX2
  add_executable(test_cockpit tests/test_cockpit.cpp src/render/vehicule_cockpit_defs.cpp)   # WFS-197 : intérieurs par modèle de la vue cockpit
  add_executable(test_regard_cockpit tests/test_regard_cockpit.cpp src/core/configgame.cpp src/render/vehicule_cockpit_defs.cpp)   # WFS-235 : regard du conducteur en vue Cockpit
  add_executable(test_tangentes tests/test_tangentes.cpp src/render/mesh.cpp)   # WFS-149 (RG-04, M-02) : tangentes, .wfm versionnés
  add_executable(test_passes tests/test_passes.cpp src/render/qualite_auto.cpp)   # WFS-150 : faces de la cubemap, niveau Ultra
  add_executable(test_feux_modele tests/test_feux_modele.cpp)   # WFS-109 : découpe des feux arrière, transition du stop
  add_executable(test_phares tests/test_phares.cpp)   # WFS-218 : feux de croisement du joueur (flux, coupure, portée)
  add_executable(test_conduite_libre tests/test_conduite_libre.cpp src/game/track.cpp src/game/player.cpp)   # WFS-90
  add_executable(test_commandes tests/test_commandes.cpp src/game/track.cpp src/game/player.cpp)   # WFS-108 : commandes tactiles
  target_compile_definitions(test_commandes PRIVATE TEST_COMMANDES_SEUL)
  add_executable(test_audio tests/test_audio.cpp src/audio/engine_synth.cpp src/audio/trafic_sons.cpp src/game/player.cpp src/game/track.cpp)   # WFS-121 : sons du trafic
  add_executable(test_sauvegarde tests/test_sauvegarde.cpp src/game/sauvegarde.cpp src/game/track.cpp)   # WFS-24 (std::thread : glibc ≥ 2.34)
  add_executable(test_paquets tests/test_paquets.cpp src/ui/ecrans/paquets_etape.cpp src/core/carte_cuite.cpp)   # WFS-306 : étape « Mise à jour et installation des paquets » (machine d'états)
  add_executable(test_carte_cuite tests/test_carte_cuite.cpp src/core/carte_cuite.cpp)   # WFS-301 : paquets de cartes (format, vérification, 2e source)
  add_executable(test_cartes_v2_donnees tests/test_cartes_v2_donnees.cpp src/world/cartes_v2/carte_donnees.cpp src/core/carte_cuite.cpp src/game/track.cpp src/game/trace_cartes.cpp)   # WFS-343 : cartes 100 % dynamiques
  target_compile_definitions(test_cartes_v2_donnees PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_executable(test_cartes_v2_saison tests/test_cartes_v2_saison.cpp src/world/cartes_v2/carte_donnees.cpp src/core/carte_cuite.cpp src/game/track.cpp src/game/trace_cartes.cpp src/game/player.cpp src/render/route_revetements_defs.cpp)   # WFS-346 : saison (neige, flocons, adhérence)
  target_compile_definitions(test_cartes_v2_saison PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_executable(test_cgu tests/test_cgu.cpp src/ui/ecrans/cgu_texte.cpp src/game/sauvegarde.cpp src/game/track.cpp)   # WFS-308 : CGU (texte, version, clé cgu=)
  add_executable(test_records tests/test_records.cpp src/game/sauvegarde.cpp src/game/fantome.cpp src/game/binarypack.cpp src/game/track.cpp)   # WFS-125 : records par mode, fantôme
  add_executable(test_plantage tests/test_plantage.cpp src/platform/plantage.cpp)   # WFS-126 : anneau du journal, rapport de plantage
  add_executable(test_fidelite_soleil tests/fidelite_soleil.cpp)   # WFS-148 (RG-02) : soleil (NOAA) contre 10 valeurs de contrôle
  add_executable(test_fidelite tests/fidelite_referentiel.cpp src/game/roues.cpp)   # WFS-148 (RG-01, RG-05) : référentiel du réel, mesures géométriques
  add_executable(test_fidelite_humains tests/fidelite_humains.cpp)   # WFS-209 : humains réalistes (M-01, M-02, squelette du jeu)
  target_compile_definitions(test_fidelite_humains PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_executable(test_fidelite_voitures tests/fidelite_voitures.cpp src/game/roues.cpp)   # WFS-158 : audit des 20 voitures (mesure « fiche »)
  target_compile_definitions(test_fidelite_voitures PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_executable(test_vehicule_lod tests/test_vehicule_lod.cpp)   # WFS-158 (RG-04) : 4 niveaux de détail des voitures
  target_compile_definitions(test_vehicule_lod PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_executable(test_conducteurs tests/conducteurs_test.cpp)   # WFS-73 : conducteurs (pavillon mesuré, placement)
  add_executable(test_fond_fige tests/test_fond_fige.cpp)   # WFS-329 : fond figé des écrans hors course (ANO-178)
  add_executable(test_apparition_pietons tests/apparition_pietons_test.cpp)   # WFS-257 : apparition naturelle des personnes (ANO-130)
  add_executable(test_fidelite_routes tests/fidelite_routes.cpp src/render/ktx2.cpp src/render/route_revetements_defs.cpp src/render/route_mesures.cpp)   # WFS-205 : revêtements de route (M-01, M-02)
  target_compile_definitions(test_fidelite_routes PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_executable(corriger_voitures tools/corriger_voitures.cpp src/game/roues.cpp)   # WFS-158 (ST-2) : échelle et roues d'après le référentiel
  target_compile_definitions(corriger_voitures PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  target_compile_definitions(test_fidelite_soleil PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  target_compile_definitions(test_fidelite PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_executable(test_archi_kit tests/archi_kit.cpp src/world/archi_kit.cpp)   # WFS-161 (ST-1) : kit d'architecture, M-01 et règles des ouvertures
  target_compile_definitions(test_archi_kit PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_executable(test_marina_bateaux tests/test_marina_bateaux.cpp src/world/marina_bateaux.cpp)   # WFS-376 : bateaux de la marina (mesures RG-04 / RG-05, budget RG-10, aucune voile hissée)
  add_executable(test_archi_geo tests/archi_geo.cpp src/world/archi_kit.cpp src/world/archi_geo.cpp)   # WFS-161 (ST-1) : géométrie du kit (trous, sens des triangles, M-01, LOD)
  target_compile_definitions(test_archi_geo PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_executable(test_fluidite_reference tests/test_fluidite_reference.cpp)   # WFS-293 (RG-10) : l'indicateur à 10-20 img/s voit les sauts
  add_executable(test_fidelite_lumiere_precalcule tests/fidelite_lumiere_precalcule.cpp src/render/eclairage_precalcule.cpp src/core/jobs.cpp)   # WFS-269 : occlusion du décor fixe précalculée
  target_link_libraries(test_fidelite_lumiere_precalcule Threads::Threads)
  add_executable(test_fidelite_lumiere tests/fidelite_lumiere.cpp)   # WFS-151 (ST-1) : photométrie, exposition, ciel, IBL
  target_compile_definitions(test_fidelite_lumiere PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_executable(test_fidelite_lumiere_ombres tests/fidelite_lumiere_ombres.cpp)   # WFS-227 : cascades d'ombre, M-01, M-02
  target_compile_definitions(test_fidelite_lumiere_ombres PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_executable(test_pluie tests/test_pluie.cpp src/render/vehicule_cockpit_defs.cpp)   # WFS-334 : pluie sous un toit, vue Cockpit, gouttes inclinées par la vitesse
  add_executable(test_ville_lieux tests/test_ville_lieux.cpp src/world/ville_quartiers.cpp src/world/zones.cpp src/game/track.cpp)   # WFS-137 : lieux de la Ville au tracé 2
  add_executable(test_ville_sol tests/test_ville_sol.cpp src/world/ville_quartiers.cpp src/world/zones.cpp src/game/track.cpp)   # WFS-305 : sol fermé côté intérieur des courbes serrées (ANO-171)
  add_executable(test_trace tests/test_trace.cpp src/game/trace_cartes.cpp src/game/duel.cpp src/game/traffic.cpp src/game/track.cpp src/game/player.cpp src/game/sauvegarde.cpp src/core/jobs.cpp)   # WFS-136
  add_executable(test_aide_direction tests/test_aide_direction.cpp src/game/trace_cartes.cpp src/game/duel.cpp src/game/traffic.cpp src/game/track.cpp src/game/player.cpp src/game/sauvegarde.cpp src/core/jobs.cpp)   # WFS-217 : aide à la direction
  add_executable(test_conduite tests/test_conduite.cpp src/game/trace_cartes.cpp src/game/duel.cpp src/game/traffic.cpp src/game/track.cpp src/game/player.cpp src/game/sauvegarde.cpp src/core/jobs.cpp)   # WFS-241 : banc de conduite sans GPU (fiches, 6 virages, RG-07, déterminisme)
  add_executable(test_sons_conduite tests/test_sons_conduite.cpp src/audio/engine_synth.cpp)   # WFS-255 : sons et vibrations liés aux forces
  add_executable(test_audio_lieux tests/test_audio_lieux.cpp src/audio/ambiance_lieux.cpp src/game/track.cpp src/game/player.cpp)   # WFS-261 : son des lieux
  add_executable(test_audio_voitures tests/test_audio_voitures.cpp src/audio/voitures_sons.cpp src/audio/ambiance_lieux.cpp src/audio/engine_synth.cpp src/game/track.cpp src/game/player.cpp)   # WFS-272 : son des autres voitures
  add_executable(test_audio_parois tests/test_audio_parois.cpp src/audio/parois_sons.cpp src/audio/ambiance_lieux.cpp src/game/track.cpp src/game/player.cpp)   # WFS-275 : écho des murs, sons masqués
  add_executable(test_audio_pluie tests/test_audio_pluie.cpp src/audio/pluie_sons.cpp src/audio/engine_synth.cpp src/game/track.cpp src/game/player.cpp)   # WFS-288 : son de la pluie
  add_executable(test_audio_chocs tests/test_audio_chocs.cpp src/audio/chocs_sons.cpp src/audio/voitures_sons.cpp src/audio/ambiance_lieux.cpp src/audio/engine_synth.cpp src/game/track.cpp src/game/player.cpp)   # WFS-298 : son des chocs
  add_executable(test_chocs tests/test_chocs.cpp src/game/trace_cartes.cpp src/game/track.cpp src/game/player.cpp)   # WFS-256 : chocs avec le décor (poteau, mur, test continu)
  add_executable(test_virages_fleches tests/test_virages_fleches.cpp src/game/trace_cartes.cpp src/game/track.cpp src/game/player.cpp)   # WFS-295 : rayon juste partout, épingles aux flèches, réponse après un choc
  add_executable(test_direction_anneau tests/test_direction_anneau.cpp src/game/trace_cartes.cpp src/game/track.cpp src/game/player.cpp)   # WFS-316 : volant tenu, anneau et virages serrés à 20 et 60 img/s
  add_executable(test_direction_vitesse tests/test_direction_vitesse.cpp src/game/trace_cartes.cpp src/game/track.cpp src/game/player.cpp)   # WFS-258 (point 3) : direction moins vive au-dessus de 100 km/h
  add_executable(test_aide_ralentit tests/test_aide_ralentit.cpp src/game/course5.cpp src/game/duel.cpp src/game/traffic.cpp src/game/reseau_udp.cpp src/game/reseau_peerjs.cpp src/game/binarypack.cpp src/core/jobs.cpp src/game/trace_cartes.cpp src/game/track.cpp src/game/player.cpp)
  target_link_libraries(test_aide_ralentit Threads::Threads)   # WFS-258 : aide activée, la voiture ralentit à temps (1er virage du GP à 228 km/h, tours des 4 cartes)
  add_executable(test_chocs_cartes tests/test_chocs_cartes.cpp src/game/chocs_declaration.cpp src/world/ville_quartiers.cpp src/world/zones.cpp src/world/giratoire.cpp src/game/trace_cartes.cpp src/game/track.cpp src/game/player.cpp)   # WFS-256 : obstacles de la Ville et des îlots, aucun mur invisible
  add_executable(test_pilote_file tests/test_pilote_file.cpp src/game/trace_cartes.cpp src/game/track.cpp src/game/player.cpp)   # WFS-339 : pilote des bancs, changement de file sans sortir de la chaussée
  add_executable(test_chocs_voitures tests/test_chocs_voitures.cpp src/game/duel.cpp src/game/reseau_udp.cpp src/game/reseau_peerjs.cpp src/game/binarypack.cpp src/game/traffic.cpp src/game/trace_cartes.cpp src/game/track.cpp src/game/player.cpp src/core/jobs.cpp)   # WFS-256 : jamais deux voitures l'une dans l'autre
  target_link_libraries(test_chocs_voitures Threads::Threads)
  add_executable(test_penetration_voitures tests/test_penetration_voitures.cpp src/game/duel.cpp src/game/reseau_udp.cpp src/game/reseau_peerjs.cpp src/game/binarypack.cpp src/game/traffic.cpp src/game/trace_cartes.cpp src/game/track.cpp src/game/player.cpp src/core/jobs.cpp)   # WFS-333 : aucune voiture dans celle du joueur
  target_link_libraries(test_penetration_voitures Threads::Threads)
  add_executable(test_rien_sur_chaussee tests/test_rien_sur_chaussee.cpp src/game/trace_cartes.cpp src/game/duel.cpp src/game/traffic.cpp src/game/track.cpp src/game/player.cpp src/game/sauvegarde.cpp src/core/jobs.cpp)   # WFS-212 : rien sur la chaussée (rond-point, épingles)
  add_executable(test_hud_lisible tests/test_hud_lisible.cpp)   # WFS-245 : textes du HUD lisibles (tailles, surfaces, contrastes)
  add_executable(test_bandeau_gps tests/test_bandeau_gps.cpp)   # WFS-245 (RG-09) : textes du bandeau de la carte GPS (tailles, place, toutes les consignes et rues)
  add_executable(test_lots_decor tests/lots_decor_test.cpp src/game/trace_cartes.cpp src/game/duel.cpp src/game/traffic.cpp src/game/track.cpp src/game/player.cpp src/game/sauvegarde.cpp src/core/jobs.cpp)   # WFS-268 : passage d'un tour au suivant (décor préparé au chargement)
  add_executable(test_navigation tests/test_navigation.cpp src/game/trace_cartes.cpp src/game/duel.cpp src/game/traffic.cpp src/game/track.cpp src/game/player.cpp src/game/sauvegarde.cpp src/core/jobs.cpp)   # WFS-234 : carte de navigation (prochain virage, distance)
  add_executable(test_memoire tests/test_memoire.cpp)   # WFS-297 : compteurs de mémoire et de chargement
  # WFS-136 (accroche) : tout test qui lie track.cpp lie aussi le tracé 2 (trace::construire, sans dépendance au moteur)
  get_property(cibles_wfs DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
  foreach(cible IN LISTS cibles_wfs)
    get_target_property(sources_cible ${cible} SOURCES)
    if(sources_cible MATCHES "src/game/track.cpp" AND NOT sources_cible MATCHES "trace_cartes")
      target_sources(${cible} PRIVATE src/game/trace_cartes.cpp)
    endif()
  endforeach()
endif()
