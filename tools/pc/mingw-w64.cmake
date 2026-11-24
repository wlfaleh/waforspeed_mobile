# WaForSpeed — version PC Windows (WFS-602) : compilation croisée MinGW-w64 depuis Linux.
#   cmake -S . -B build-windows -G Ninja -DCMAKE_TOOLCHAIN_FILE=tools/pc/mingw-w64.cmake
# (ou simplement : bash tools/pc/construire_installateur.sh)
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc-posix)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++-posix)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)   # glslc de Linux pour les shaders
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
