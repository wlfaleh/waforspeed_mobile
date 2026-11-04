# WaForSpeed — cible iOS (WFS-22) : application UIKit, rendu Vulkan par MoltenVK (Metal), mêmes sources que Android.
# Incluse par CMakeLists.txt quand CMAKE_SYSTEM_NAME = iOS. Construction (CI : .github/workflows/ios.yml) :
#   cmake -S . -B build-ios -G Ninja -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator \
#         -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 -DMOLTENVK_LIB=<libMoltenVK.a> \
#         -DVULKAN_INCLUDE=<dossier contenant vulkan/vulkan.h>
#   cmake --build build-ios   → build-ios/WaForSpeed.app (assets, licences et shaders inclus)
# Signature : aucune ici (CODE_SIGNING_ALLOWED=NO) ; signature manuelle et TestFlight : store/signature.md.
enable_language(OBJCXX)
set(MOLTENVK_LIB "" CACHE FILEPATH "bibliothèque statique MoltenVK (libMoltenVK.a de l'xcframework, simulateur ou appareil)")
set(VULKAN_INCLUDE "" CACHE PATH "dossier des en-têtes Vulkan (vulkan/vulkan.h)")
if(NOT MOLTENVK_LIB)
  message(FATAL_ERROR "iOS : indiquer -DMOLTENVK_LIB=<libMoltenVK.a>")
endif()

if(NOT WFS_BUILD)
  set(WFS_BUILD 1)   # numéro de build (CI : GITHUB_RUN_NUMBER, comme versionCode Android)
endif()
add_executable(WaForSpeed MACOSX_BUNDLE ${ENGINE_SOURCES} src/platform/ios_main.mm)
add_dependencies(WaForSpeed shaders)
target_include_directories(WaForSpeed PRIVATE ${SHADER_OUT} ${VULKAN_INCLUDE})
target_compile_options(WaForSpeed PRIVATE ${WARN})
set_source_files_properties(src/platform/ios_main.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
target_link_libraries(WaForSpeed ${MOLTENVK_LIB} "-framework UIKit" "-framework Foundation" "-framework QuartzCore"
  "-framework Metal" "-framework IOSurface" "-framework CoreGraphics")
target_link_libraries(WaForSpeed "-framework AudioToolbox" "-framework AVFAudio")   # WFS-79 : son (RemoteIO, session ambiante)
set_target_properties(WaForSpeed PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/ios/Info.plist.in
  MACOSX_BUNDLE_GUI_IDENTIFIER com.waforspeed.game
  MACOSX_BUNDLE_BUNDLE_NAME WaForSpeed
  MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
  MACOSX_BUNDLE_BUNDLE_VERSION "${WFS_BUILD}"
  XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO)
# assets partagés (.wfm, .wfp, sons, police, licences) copiés dans l'application (RG-06)
add_custom_command(TARGET WaForSpeed POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_BUNDLE_CONTENT_DIR:WaForSpeed>/assets
  COMMENT "Assets copiés dans WaForSpeed.app")
