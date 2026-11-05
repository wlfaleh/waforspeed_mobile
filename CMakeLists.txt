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
foreach(S scene.vert scene.frag sky.vert sky.frag post.vert post.frag ui.vert ui.frag shadow.vert shadow.frag scene_skin.vert shadow_skin.vert scene_skin.frag)
  add_custom_command(
    OUTPUT ${SHADER_OUT}/${S}.inc
    COMMAND ${GLSLC} -O --target-env=vulkan1.0 -mfmt=num -I ${SHADER_DIR} -o ${SHADER_OUT}/${S}.inc ${SHADER_DIR}/${S}
    DEPENDS ${SHADER_DIR}/${S} ${SHADER_DIR}/common.glsl
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
  add_executable(test_course5 tests/test_course5.cpp src/game/course5.cpp tests/test_poursuite.cpp src/game/poursuite.cpp src/game/sauvegarde.cpp src/game/track.cpp)   # WFS-110 : course à 5 voitures ; WFS-111 : poursuite ; WFS-119 : sauvegarde (mode gardé)
  add_executable(test_retour_systeme tests/test_retour_systeme.cpp)   # WFS-116 : touche Retour du téléphone
  add_executable(test_manette tests/test_manette.cpp src/platform/manette.cpp)   # WFS-22 : manettes
  add_executable(test_masquage tests/test_masquage.cpp src/render/masquage.cpp)   # WFS-82
  add_executable(test_conduite_libre tests/test_conduite_libre.cpp src/game/track.cpp src/game/player.cpp)   # WFS-90
  add_executable(test_commandes tests/test_commandes.cpp src/game/track.cpp src/game/player.cpp)   # WFS-108 : commandes tactiles
  target_compile_definitions(test_commandes PRIVATE TEST_COMMANDES_SEUL)
  add_executable(test_audio tests/test_audio.cpp src/audio/engine_synth.cpp src/audio/trafic_sons.cpp src/game/player.cpp src/game/track.cpp)   # WFS-121 : sons du trafic
  add_executable(test_sauvegarde tests/test_sauvegarde.cpp src/game/sauvegarde.cpp src/game/track.cpp)   # WFS-24 (std::thread : glibc ≥ 2.34)
endif()
