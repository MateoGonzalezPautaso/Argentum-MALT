cmake_minimum_required(VERSION 3.24)

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# ----------------------------------------------------------------
# Estándar de C++
# ----------------------------------------------------------------
if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)   # permite gnu++20 que usa la cátedra

# ----------------------------------------------------------------
# Proyecto
# ----------------------------------------------------------------
project(taller_tp
    VERSION 1.0
    HOMEPAGE_URL "https://github.com/URL-HERE"
    LANGUAGES CXX
)

message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# ----------------------------------------------------------------
# Opciones
# ----------------------------------------------------------------
option(TALLER_TESTS "Enable / disable tests." ON)
option(TALLER_CLIENT "Enable / disable client program." ON)
option(TALLER_SERVER "Enable / disable server program." ON)
option(TALLER_EDITOR "Enable / disable editor program." ON)
option(TALLER_MAKE_WARNINGS_AS_ERRORS "Enable / disable warnings as errors." ON)

# ----------------------------------------------------------------
# Dependencias externas via FetchContent
# Todas las declaraciones van acá arriba
# FetchContent_MakeAvailable se llama más abajo según lo que se necesite
# ----------------------------------------------------------------
include(FetchContent)

# toml++ (header-only, siempre disponible para server también)
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(tomlplusplus)

# GoogleTest (solo si hay tests)
if(TALLER_TESTS)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    include(GoogleTest)
endif()

# SDL2 y extensiones: se usan las del sistema (instaladas via apt).
# NO usar FetchContent para SDL2: ya está en el sistema
if(TALLER_CLIENT OR TALLER_EDITOR)
    find_package(SDL2       REQUIRED)
    find_package(SDL2_image REQUIRED)
    find_package(SDL2_ttf   REQUIRED)
    find_package(SDL2_mixer REQUIRED)

    # libSDL2pp: wrapper C++ de SDL2. Este sí se baja via FetchContent
    # porque no está empaquetado en apt con la versión que necesitamos.
    FetchContent_Declare(
        libSDL2pp
        URL https://github.com/libSDL2pp/libSDL2pp/archive/cc198c9a5657048bee67ece82de620b2d5661084.zip
    )
    FetchContent_MakeAvailable(libSDL2pp)
endif()

# ----------------------------------------------------------------
# Warnings
# ----------------------------------------------------------------
include(cmake/CompilerWarnings.cmake)

# ----------------------------------------------------------------
# taller_common - librería estática compartida por todos
# ----------------------------------------------------------------
add_library(taller_common STATIC)

add_subdirectory(common/)

set_project_warnings(taller_common ${TALLER_MAKE_WARNINGS_AS_ERRORS} FALSE)

# PUBLIC: cualquiera que linkee taller_common hereda este include path.
# Así los includes del estilo #include "common/socket.h" funcionan
# desde cualquier subdirectorio.
target_include_directories(taller_common PUBLIC ${CMAKE_SOURCE_DIR})

# toml++ disponible para todo el proyecto (server lo usa para Config)
target_link_libraries(taller_common PUBLIC tomlplusplus::tomlplusplus)

# ----------------------------------------------------------------
# taller_server
# ----------------------------------------------------------------
if(TALLER_SERVER)
    add_executable(taller_server)

    add_subdirectory(server/)

    set_project_warnings(taller_server ${TALLER_MAKE_WARNINGS_AS_ERRORS} FALSE)

    target_link_libraries(taller_server PRIVATE taller_common)
endif()

# ----------------------------------------------------------------
# taller_client
# ----------------------------------------------------------------
if(TALLER_CLIENT)
    add_executable(taller_client)

    add_subdirectory(client/)

    set_project_warnings(taller_client ${TALLER_MAKE_WARNINGS_AS_ERRORS} FALSE)

    # SDL2 del sistema expone targets modernos SDL2::SDL2, etc.
    # Si find_package no los define (versión vieja de Ubuntu), usamos
    # las variables clásicas SDL2_LIBRARIES / SDL2_INCLUDE_DIRS.
    if(TARGET SDL2::SDL2)
        target_link_libraries(taller_client PRIVATE
            taller_common
            SDL2::SDL2
            SDL2_image::SDL2_image
            SDL2_ttf::SDL2_ttf
            SDL2_mixer::SDL2_mixer
            SDL2pp::SDL2pp
        )
    else()
        target_include_directories(taller_client PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(taller_client PRIVATE
            taller_common
            ${SDL2_LIBRARIES}
            SDL2_image
            SDL2_ttf
            SDL2_mixer
            SDL2pp::SDL2pp
        )
    endif()

    target_include_directories(taller_client PRIVATE ${libSDL2pp_SOURCE_DIR})
endif()

# ----------------------------------------------------------------
# taller_editor
# ----------------------------------------------------------------
if(TALLER_EDITOR)
    # Qt6 - del sistema (instalado via apt)
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL)
    qt6_standard_project_setup()

    add_executable(taller_editor)

    add_subdirectory(editor/)

    set_project_warnings(taller_editor ${TALLER_MAKE_WARNINGS_AS_ERRORS} FALSE)

    if(TARGET SDL2::SDL2)
        target_link_libraries(taller_editor PRIVATE
            taller_common
            SDL2::SDL2
            SDL2pp::SDL2pp
            Qt6::Core
            Qt6::Widgets
            Qt6::OpenGL
        )
    else()
        target_include_directories(taller_editor PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(taller_editor PRIVATE
            taller_common
            ${SDL2_LIBRARIES}
            SDL2pp::SDL2pp
            Qt6::Core
            Qt6::Widgets
            Qt6::OpenGL
        )
    endif()

    target_include_directories(taller_editor PRIVATE ${libSDL2pp_SOURCE_DIR})
endif()

enable_testing()

# ----------------------------------------------------------------
# taller_tests
# ----------------------------------------------------------------
if(TALLER_TESTS)
    add_executable(taller_tests)

    add_subdirectory(tests/)

    set_project_warnings(taller_tests ${TALLER_MAKE_WARNINGS_AS_ERRORS} TRUE)

    target_include_directories(taller_tests PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${gtest_SOURCE_DIR}/include
        ${gmock_SOURCE_DIR}/include
    )

    target_link_libraries(taller_tests PRIVATE
        taller_common
        GTest::gtest_main
    )

    # Registra los tests para que `ctest` los descubra automáticamente
    gtest_discover_tests(taller_tests DISCOVERY_MODE PRE_TEST)
endif()

message(STATUS "TALLER_TESTS = ${TALLER_TESTS}")
message(STATUS "taller_tests target exists: $<TARGET_EXISTS:taller_tests>")