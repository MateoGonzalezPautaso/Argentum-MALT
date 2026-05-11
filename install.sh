#!/bin/bash
set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log()  { echo -e "${BLUE}[INFO]${NC}  $1"; }
ok()   { echo -e "${GREEN}[OK]${NC}    $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC}  $1"; }
fail() { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }

# ----------------------------------------------------------------
# Argument parsing
# ----------------------------------------------------------------
BUILD_TYPE="Release"
SKIP_DEPS=0
SKIP_BUILD=0
SKIP_TESTS=0
SKIP_INSTALL=0

usage() {
    echo "Uso: $0 [opciones]"
    echo ""
    echo "  --debug          Compilar en modo Debug (default: Release)"
    echo "  --skip-deps      Saltear instalación de dependencias"
    echo "  --skip-build     Saltear compilación"
    echo "  --skip-tests     Saltear ejecución de tests"
    echo "  --skip-install   Saltear instalación de binarios/assets"
    echo "  -h, --help       Mostrar esta ayuda"
    echo ""
    exit 0
}

for arg in "$@"; do
    case $arg in
        --debug)        BUILD_TYPE="Debug" ;;
        --skip-deps)    SKIP_DEPS=1 ;;
        --skip-build)   SKIP_BUILD=1 ;;
        --skip-tests)   SKIP_TESTS=1 ;;
        --skip-install) SKIP_INSTALL=1 ;;
        -h|--help)      usage ;;
        *) warn "Argumento desconocido: $arg (ignorado)" ;;
    esac
done

# ----------------------------------------------------------------
# Header
# ----------------------------------------------------------------
echo ""
echo -e "${BLUE}================================================${NC}"
echo -e "${BLUE}        Argentum Online — Instalador            ${NC}"
echo -e "${BLUE}================================================${NC}"
echo ""
log "Modo de compilación: ${BUILD_TYPE}"
echo ""

# ----------------------------------------------------------------
# Checks previos
# ----------------------------------------------------------------
if ! command -v apt-get &> /dev/null; then
    fail "Este script requiere apt-get (Ubuntu/Debian)."
fi

# Detectar Ubuntu 24.04 (requerido por la cátedra)
if command -v lsb_release &> /dev/null; then
    DISTRO=$(lsb_release -si)
    VERSION=$(lsb_release -sr)
    log "Sistema detectado: ${DISTRO} ${VERSION}"
    if [[ "$DISTRO" != "Ubuntu" ]]; then
        warn "Este proyecto está testeado en Ubuntu 24.04. Continuando de todas formas..."
    fi
fi

# ----------------------------------------------------------------
# PASO 1 — Dependencias del sistema
# ----------------------------------------------------------------
if [ "$SKIP_DEPS" -eq 1 ]; then
    warn "Saltando instalación de dependencias (--skip-deps)."
else
    echo ""
    echo -e "${BLUE}--- PASO 1: Dependencias del sistema ---${NC}"
    echo ""

    log "Actualizando lista de paquetes..."
    sudo apt-get update -qq

    # Build tools
    log "Instalando herramientas de compilación..."
    sudo apt-get install -y \
        build-essential \
        cmake \
        git \
        pkg-config \
        ninja-build
    ok "Herramientas de compilación instaladas."

    # Testing y análisis estático
    log "Instalando herramientas de testing y análisis..."
    sudo apt-get install -y \
        libgtest-dev \
        valgrind \
        cppcheck \
        clang-format
    ok "Herramientas de testing instaladas."

    # X11 (requerido por SDL2)
    log "Instalando dependencias de X11..."
    sudo apt-get install -y \
        libx11-dev \
        libxext-dev \
        libxrandr-dev \
        libxi-dev \
        libxcursor-dev \
        libxinerama-dev \
        libxss-dev \
        libxxf86vm-dev
    ok "Dependencias X11 instaladas."

    # OpenGL (requerido por SDL2)
    log "Instalando dependencias de OpenGL..."
    sudo apt-get install -y \
        libgl1-mesa-dev \
        libgles2-mesa-dev \
        libegl1-mesa-dev
    ok "OpenGL instalado."

    # Audio backends (requerido por SDL2_mixer)
    log "Instalando backends de audio..."
    sudo apt-get install -y \
        libasound2-dev \
        libpulse-dev
    # Jack y PipeWire son opcionales; en algunas configs pueden conflictuar
    sudo apt-get install -y libpipewire-0.3-dev || warn "PipeWire opcional no disponible, continuando."
    sudo apt-get install -y libjack-jackd2-dev  || warn "Jack opcional no disponible, continuando."
    ok "Backends de audio instalados."

    # Codecs de audio para SDL2_mixer
    log "Instalando codecs de audio..."
    sudo apt-get install -y \
        libopus-dev \
        libopusfile-dev \
        libvorbis-dev \
        libogg-dev \
        libflac-dev \
        libmpg123-dev \
        libmodplug-dev \
        libxmp-dev \
        libfluidsynth-dev \
        fluidsynth \
        libwavpack-dev
    ok "Codecs de audio instalados."

    # SDL2_ttf (fuentes)
    log "Instalando dependencias de SDL2_ttf..."
    sudo apt-get install -y \
        libfreetype-dev \
        libharfbuzz-dev
    ok "Dependencias de SDL2_ttf instaladas."

    # SDL2_image
    log "Instalando dependencias de SDL2_image..."
    sudo apt-get install -y \
        libpng-dev \
        libjpeg-dev \
        libtiff-dev \
        libwebp-dev
    ok "Dependencias de SDL2_image instaladas."

    # SDL2 — las librerías en sí
    log "Instalando SDL2 y extensiones..."
    sudo apt-get install -y \
        libsdl2-dev \
        libsdl2-image-dev \
        libsdl2-ttf-dev \
        libsdl2-mixer-dev
    ok "SDL2 instalado."

    # Qt6 — para el editor gráfico
    log "Instalando Qt6..."
    sudo apt-get install -y \
        qt6-base-dev \
        qt6-tools-dev-tools \
        libqt6widgets6t64 \
        libqt6opengl6-dev
    ok "Qt6 instalado."

    # Utilidades adicionales
    log "Instalando utilidades adicionales..."
    sudo apt-get install -y \
        libssl-dev \
        zlib1g-dev
    ok "Utilidades adicionales instaladas."

    # Verificar versión de CMake
    CMAKE_MIN="3.24"
    CMAKE_ACTUAL=$(cmake --version | head -n1 | awk '{print $3}')
    log "Versión de CMake activa: ${CMAKE_ACTUAL}"
    if dpkg --compare-versions "$CMAKE_ACTUAL" ge "$CMAKE_MIN"; then
        ok "CMake >= ${CMAKE_MIN} ✓"
    else
        warn "CMake ${CMAKE_ACTUAL} es menor a ${CMAKE_MIN}. El proyecto puede no configurarse bien."
        warn "Instalá una versión más nueva desde: https://cmake.org/download/"
    fi
fi

# ----------------------------------------------------------------
# PASO 2 — Compilación
# ----------------------------------------------------------------
if [ "$SKIP_BUILD" -eq 1 ]; then
    warn "Saltando compilación (--skip-build)."
else
    echo ""
    echo -e "${BLUE}--- PASO 2: Compilación ---${NC}"
    echo ""

    BUILD_DIR="build"

    log "Configurando CMake (modo ${BUILD_TYPE})..."
    cmake -B "$BUILD_DIR" \
          -G Ninja \
          -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    ok "CMake configurado."

    log "Compilando el proyecto..."
    cmake --build "$BUILD_DIR" --parallel "$(nproc)"
    ok "Compilación exitosa."
fi

# ----------------------------------------------------------------
# PASO 3 — Tests
# ----------------------------------------------------------------
if [ "$SKIP_TESTS" -eq 1 ]; then
    warn "Saltando tests (--skip-tests)."
else
    echo ""
    echo -e "${BLUE}--- PASO 3: Tests unitarios ---${NC}"
    echo ""

    BUILD_DIR="build"

    if [ ! -d "$BUILD_DIR" ]; then
        fail "Directorio de build no encontrado. Corré sin --skip-build primero."
    fi

    log "Ejecutando tests con CTest..."
    ctest --test-dir "$BUILD_DIR" \
          --output-on-failure \
          --parallel "$(nproc)"
    ok "Todos los tests pasaron."
fi

# ----------------------------------------------------------------
# PASO 4 — Instalación de binarios y assets
# ----------------------------------------------------------------
if [ "$SKIP_INSTALL" -eq 1 ]; then
    warn "Saltando instalación (--skip-install)."
else
    echo ""
    echo -e "${BLUE}--- PASO 4: Instalación ---${NC}"
    echo ""

    BUILD_DIR="build"
    APP_NAME="argentum"

    if [ ! -d "$BUILD_DIR" ]; then
        fail "Directorio de build no encontrado. Corré sin --skip-build primero."
    fi

    # Binarios → ~/.local/bin
    log "Instalando binarios en ~/.local/bin..."
    cmake --install "$BUILD_DIR" --prefix "$HOME/.local"
    ok "Binarios instalados en ~/.local/bin"

    # Assets → ~/.local/share/argentum/
    ASSETS_DEST="$HOME/.local/share/${APP_NAME}"
    if [ -d "assets" ]; then
        log "Copiando assets a ${ASSETS_DEST}..."
        mkdir -p "$ASSETS_DEST"
        cp -r assets/. "$ASSETS_DEST/"
        ok "Assets instalados en ${ASSETS_DEST}"
    else
        warn "Directorio 'assets/' no encontrado. Saltando copia de assets."
    fi

    # Config → ~/.config/argentum/
    CONFIG_DEST="$HOME/.config/${APP_NAME}"
    if [ -d "config" ]; then
        log "Copiando configuración a ${CONFIG_DEST}..."
        mkdir -p "$CONFIG_DEST"
        # No pisamos config existente del usuario (--no-clobber)
        cp -rn config/. "$CONFIG_DEST/" || true
        ok "Configuración instalada en ${CONFIG_DEST}"
    else
        warn "Directorio 'config/' no encontrado. Saltando copia de configuración."
    fi

    # Agregar ~/.local/bin al PATH si no está
    if [[ ":$PATH:" != *":$HOME/.local/bin:"* ]]; then
        warn "~/.local/bin no está en tu PATH."
        warn "Agregá esta línea a tu ~/.bashrc o ~/.zshrc:"
        warn "    export PATH=\"\$HOME/.local/bin:\$PATH\""
    fi
fi

# ----------------------------------------------------------------
# Listo
# ----------------------------------------------------------------
echo ""
echo -e "${GREEN}================================================${NC}"
echo -e "${GREEN}        Instalación completada con éxito        ${NC}"
echo -e "${GREEN}================================================${NC}"
echo ""
