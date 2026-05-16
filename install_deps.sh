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

echo ""
echo -e "${BLUE}================================================${NC}"
echo -e "${BLUE}   Instalador de dependencias - taller_tp       ${NC}"
echo -e "${BLUE}================================================${NC}"
echo ""

# Check we're on a Debian/Ubuntu system
if ! command -v apt-get &> /dev/null; then
    fail "Este script requiere apt-get (Ubuntu/Debian)."
fi

# Check cmake version
CMAKE_MIN=3.24
if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -n1 | awk '{print $3}')
    log "CMake encontrado: v${CMAKE_VERSION}"
else
    warn "CMake no encontrado. Se instalará."
fi

log "Actualizando lista de paquetes..."
sudo apt-get update -qq

# ----------------------------------------------------------------
# Build tools
# ----------------------------------------------------------------
log "Instalando herramientas de compilación..."
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    ninja-build

ok "Herramientas de compilación instaladas."

# ----------------------------------------------------------------
# X11 / display (requerido por SDL2)
# ----------------------------------------------------------------
log "Instalando dependencias de X11 (requeridas por SDL2)..."
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

# ----------------------------------------------------------------
# OpenGL (requerido por SDL2)
# ----------------------------------------------------------------
log "Instalando dependencias de OpenGL..."
sudo apt-get install -y \
    libgl1-mesa-dev \
    libgles2-mesa-dev \
    libegl1-mesa-dev

ok "OpenGL instalado."

# ----------------------------------------------------------------
# Audio (requerido por SDL2_mixer)
# ----------------------------------------------------------------
log "Instalando backends de audio..."
sudo apt-get install -y \
    libasound2-dev \
    libpulse-dev \
    libjack-jackd2-dev \
    libpipewire-0.3-dev

ok "Backends de audio instalados."

# ----------------------------------------------------------------
# SDL2_mixer codecs (mencionados en el CMakeLists.txt)
# ----------------------------------------------------------------
log "Instalando codecs de audio para SDL2_mixer..."
sudo apt-get install -y \
    libopus-dev \
    libopusfile-dev \
    libxmp-dev \
    libfluidsynth-dev \
    fluidsynth \
    libwavpack1 \
    libwavpack-dev \
    wavpack \
    libvorbis-dev \
    libogg-dev \
    libflac-dev \
    libmpg123-dev \
    libmodplug-dev

ok "Codecs de audio instalados."

# ----------------------------------------------------------------
# SDL2_ttf (fuentes)
# ----------------------------------------------------------------
log "Instalando dependencias de SDL2_ttf..."
sudo apt-get install -y \
    libfreetype-dev \
    libharfbuzz-dev

ok "Dependencias de SDL2_ttf instaladas."

# ----------------------------------------------------------------
# SDL2_image (imágenes)
# ----------------------------------------------------------------
log "Instalando dependencias de SDL2_image..."
sudo apt-get install -y \
    libpng-dev \
    libjpeg-dev \
    libtiff-dev \
    libwebp-dev

ok "Dependencias de SDL2_image instaladas."

# ----------------------------------------------------------------
# Otras utilidades útiles
# ----------------------------------------------------------------
log "Instalando otras utilidades..."
sudo apt-get install -y \
    libssl-dev \
    zlib1g-dev

ok "Utilidades adicionales instaladas."

# ----------------------------------------------------------------
# Verificar versión de CMake
# ----------------------------------------------------------------
echo ""
CMAKE_ACTUAL=$(cmake --version | head -n1 | awk '{print $3}')
log "Versión de CMake activa: ${CMAKE_ACTUAL}"

# Compare versions (simple check for major.minor)
CMAKE_MAJOR=$(echo "$CMAKE_ACTUAL" | cut -d. -f1)
CMAKE_MINOR=$(echo "$CMAKE_ACTUAL" | cut -d. -f2)

if [ "$CMAKE_MAJOR" -gt 3 ] || { [ "$CMAKE_MAJOR" -eq 3 ] && [ "$CMAKE_MINOR" -ge 24 ]; }; then
    ok "CMake >= ${CMAKE_MIN} ✓"
else
    warn "CMake ${CMAKE_ACTUAL} es menor a ${CMAKE_MIN}. El proyecto puede no configurarse bien."
    warn "Podés instalar una versión más nueva desde: https://cmake.org/download/"
fi

# ----------------------------------------------------------------
# Listo
# ----------------------------------------------------------------
echo ""
echo -e "${GREEN}================================================${NC}"
echo -e "${GREEN}   Todas las dependencias instaladas con éxito  ${NC}"
echo -e "${GREEN}================================================${NC}"
echo ""
echo "Ahora podés compilar el proyecto con:"
echo ""
echo -e "  ${YELLOW}make compile-debug${NC}"
echo ""
echo "O manualmente:"
echo ""
echo -e "  ${YELLOW}mkdir -p build && cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Debug${NC}"
echo -e "  ${YELLOW}cmake --build build/${NC}"
echo ""
