# Require the audited static window/input library profile.
if(NOT TARGET SDL2::SDL2-static)
    find_package(SDL2 CONFIG REQUIRED)
endif()
if(NOT TARGET SDL2::SDL2-static)
    message(FATAL_ERROR "A static SDL2 build is required")
endif()
set(SDL2_LIBRARIES SDL2::SDL2-static)
