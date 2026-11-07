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
  target_link_libraries(test_trafic_vivant Threads::Threads)
  add_executable(test_qualite tests/test_qualite.cpp src/render/qualite_auto.cpp)   # WFS-15
  add_executable(test_chauffe tests/test_chauffe.cpp src/platform/chauffe.cpp src/render/qualite_auto.cpp)   # WFS-123 : chauffe
  add_executable(test_appui tests/test_appui.cpp)   # WFS-113 : un doigt = un appui
  add_executable(test_petit_ecran tests/test_petit_ecran.cpp)   # WFS-147 : téléphone couché en dp
  add_executable(test_minicarte tests/test_minicarte.cpp)   # WFS-173 : place et projection de la mini-carte
  add_executable(test_course5 tests/test_course5.cpp src/game/course5.cpp tests/test_poursuite.cpp src/game/poursuite.cpp src/game/sauvegarde.cpp src/game/track.cpp src/game/player.cpp)   # WFS-110 : course à 5 voitures ; WFS-111 : poursuite ; WFS-119 : sauvegarde (mode gardé) ; WFS-189 : player.cpp (courses simulées)
  add_executable(test_retour_systeme tests/test_retour_systeme.cpp)   # WFS-116 : touche Retour du téléphone
  add_executable(test_configgame tests/test_configgame.cpp src/core/configgame.cpp)   # WFS-128 : configgame.json
  add_executable(test_manette tests/test_manette.cpp src/platform/manette.cpp)   # WFS-22 : manettes
  add_executable(test_masquage tests/test_masquage.cpp src/render/masquage.cpp)   # WFS-82
  add_executable(test_ktx2 tests/test_ktx2.cpp src/render/ktx2.cpp)   # WFS-149 (RG-01) : textures KTX2
  add_executable(test_cockpit tests/test_cockpit.cpp src/render/vehicule_cockpit_defs.cpp)   # WFS-197 : intérieurs par modèle de la vue cockpit
  add_executable(test_tangentes tests/test_tangentes.cpp src/render/mesh.cpp)   # WFS-149 (RG-04, M-02) : tangentes, .wfm versionnés
  add_executable(test_passes tests/test_passes.cpp src/render/qualite_auto.cpp)   # WFS-150 : faces de la cubemap, niveau Ultra
  add_executable(test_feux_modele tests/test_feux_modele.cpp)   # WFS-109 : découpe des feux arrière, transition du stop
  add_executable(test_conduite_libre tests/test_conduite_libre.cpp src/game/track.cpp src/game/player.cpp)   # WFS-90
  add_executable(test_commandes tests/test_commandes.cpp src/game/track.cpp src/game/player.cpp)   # WFS-108 : commandes tactiles
  target_compile_definitions(test_commandes PRIVATE TEST_COMMANDES_SEUL)
  add_executable(test_audio tests/test_audio.cpp src/audio/engine_synth.cpp src/audio/trafic_sons.cpp src/game/player.cpp src/game/track.cpp)   # WFS-121 : sons du trafic
  add_executable(test_sauvegarde tests/test_sauvegarde.cpp src/game/sauvegarde.cpp src/game/track.cpp)   # WFS-24 (std::thread : glibc ≥ 2.34)
  add_executable(test_records tests/test_records.cpp src/game/sauvegarde.cpp src/game/fantome.cpp src/game/binarypack.cpp src/game/track.cpp)   # WFS-125 : records par mode, fantôme
  add_executable(test_plantage tests/test_plantage.cpp src/platform/plantage.cpp)   # WFS-126 : anneau du journal, rapport de plantage
  add_executable(test_fidelite_soleil tests/fidelite_soleil.cpp)   # WFS-148 (RG-02) : soleil (NOAA) contre 10 valeurs de contrôle
  add_executable(test_fidelite tests/fidelite_referentiel.cpp src/game/roues.cpp)   # WFS-148 (RG-01, RG-05) : référentiel du réel, mesures géométriques
  add_executable(test_fidelite_humains tests/fidelite_humains.cpp)   # WFS-209 : humains réalistes (M-01, M-02, squelette du jeu)
  target_compile_definitions(test_fidelite_humains PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_executable(test_fidelite_voitures tests/fidelite_voitures.cpp src/game/roues.cpp)   # WFS-158 : audit des 20 voitures (mesure « fiche »)
  target_compile_definitions(test_fidelite_voitures PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_executable(test_conducteurs tests/conducteurs_test.cpp)   # WFS-73 : conducteurs (pavillon mesuré, placement)
  add_executable(test_fidelite_routes tests/fidelite_routes.cpp src/render/ktx2.cpp src/render/route_revetements_defs.cpp)   # WFS-205 : revêtements de route (M-01, M-02)
  target_compile_definitions(test_fidelite_routes PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_executable(corriger_voitures tools/corriger_voitures.cpp src/game/roues.cpp)   # WFS-158 (ST-2) : échelle et roues d'après le référentiel
  target_compile_definitions(corriger_voitures PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  target_compile_definitions(test_fidelite_soleil PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  target_compile_definitions(test_fidelite PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_executable(test_fidelite_lumiere tests/fidelite_lumiere.cpp)   # WFS-151 (ST-1) : photométrie, exposition, ciel, IBL
  target_compile_definitions(test_fidelite_lumiere PRIVATE WFS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
  add_executable(test_ville_lieux tests/test_ville_lieux.cpp src/world/ville_quartiers.cpp src/world/zones.cpp src/game/track.cpp)   # WFS-137 : lieux de la Ville au tracé 2
  add_executable(test_trace tests/test_trace.cpp src/game/trace_cartes.cpp src/game/duel.cpp src/game/traffic.cpp src/game/track.cpp src/game/player.cpp src/game/sauvegarde.cpp src/core/jobs.cpp)   # WFS-136
  add_executable(test_rien_sur_chaussee tests/test_rien_sur_chaussee.cpp src/game/trace_cartes.cpp src/game/duel.cpp src/game/traffic.cpp src/game/track.cpp src/game/player.cpp src/game/sauvegarde.cpp src/core/jobs.cpp)   # WFS-212 : rien sur la chaussée (rond-point, épingles)
  # WFS-136 (accroche) : tout test qui lie track.cpp lie aussi le tracé 2 (trace::construire, sans dépendance au moteur)
  get_property(cibles_wfs DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
  foreach(cible IN LISTS cibles_wfs)
    get_target_property(sources_cible ${cible} SOURCES)
    if(sources_cible MATCHES "src/game/track.cpp" AND NOT sources_cible MATCHES "trace_cartes")
      target_sources(${cible} PRIVATE src/game/trace_cartes.cpp)
    endif()
  endforeach()
endif()
