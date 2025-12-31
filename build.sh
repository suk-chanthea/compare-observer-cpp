#!/bin/bash

# MSYS2 Build Script for Compare Observer
# Run this in MSYS2 MINGW64 terminal

set -e  # Exit on error

echo "=== Compare Observer MSYS2 Build Script ==="

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_DIR="build"
DIST_DIR="dist"

echo -e "${BLUE}Build Type: ${BUILD_TYPE}${NC}"

# Check for required tools
echo -e "${BLUE}Checking dependencies...${NC}"

MISSING_DEPS=()

if ! command -v cmake &> /dev/null; then
    echo -e "${RED}✗ CMake not found${NC}"
    MISSING_DEPS+=("mingw-w64-x86_64-cmake")
else
    echo -e "${GREEN}✓ CMake found: $(cmake --version | head -n1)${NC}"
fi

if ! command -v ninja &> /dev/null; then
    echo -e "${RED}✗ Ninja not found${NC}"
    MISSING_DEPS+=("mingw-w64-x86_64-ninja")
else
    echo -e "${GREEN}✓ Ninja found${NC}"
fi

if ! command -v qmake6 &> /dev/null; then
    echo -e "${RED}✗ Qt6 not found${NC}"
    MISSING_DEPS+=("mingw-w64-x86_64-qt6-base" "mingw-w64-x86_64-qt6-tools")
else
    echo -e "${GREEN}✓ Qt6 found${NC}"
fi

# If missing dependencies, show install command
if [ ${#MISSING_DEPS[@]} -gt 0 ]; then
    echo -e "${RED}Missing dependencies detected!${NC}"
    echo -e "${BLUE}Install them with:${NC}"
    echo -e "  ${GREEN}pacman -S ${MISSING_DEPS[*]}${NC}"
    echo ""
    read -p "Install now? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        pacman -S --needed ${MISSING_DEPS[*]}
    else
        exit 1
    fi
fi

echo ""

# Clean previous build (optional)
if [ "$1" = "clean" ]; then
    echo -e "${BLUE}Cleaning previous build...${NC}"
    rm -rf "$BUILD_DIR" "$DIST_DIR"
fi

# Create build directory
echo -e "${BLUE}Creating build directory...${NC}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo -e "${BLUE}Configuring with CMake...${NC}"
cmake .. \
    -G "Ninja" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_PREFIX_PATH="/mingw64" \
    -DCMAKE_INSTALL_PREFIX="../install"

# Build
echo -e "${BLUE}Building project...${NC}"
ninja

# Check if build succeeded
if [ ! -f "bin/CompareObserver.exe" ]; then
    echo -e "${RED}Build failed: CompareObserver.exe not found${NC}"
    exit 1
fi

echo -e "${GREEN}Build successful!${NC}"

# Create distribution package
echo -e "${BLUE}Creating distribution package...${NC}"
cd ..
mkdir -p "$DIST_DIR"
cp "$BUILD_DIR/bin/CompareObserver.exe" "$DIST_DIR/"

cd "$DIST_DIR"

# Deploy Qt dependencies
echo -e "${BLUE}Deploying Qt dependencies...${NC}"
if command -v windeployqt6 &> /dev/null; then
    windeployqt6 CompareObserver.exe --release --no-translations
else
    echo -e "${YELLOW}Warning: windeployqt6 not found, skipping Qt deployment${NC}"
fi

# Copy ALL DLLs using ldd (most reliable method)
echo -e "${BLUE}Copying all required DLLs...${NC}"

if command -v ldd &> /dev/null; then
    # Get all DLL dependencies from ldd
    ALL_DLLS=$(ldd CompareObserver.exe | grep -i '/mingw64/bin' | awk '{print $3}' | sort -u)

    copied=0
    for dll_path in $ALL_DLLS; do
        if [ -f "$dll_path" ]; then
            dll_name=$(basename "$dll_path")
            if [ ! -f "$dll_name" ]; then
                cp "$dll_path" . 2>/dev/null && ((copied++)) || true
            fi
        fi
    done

    echo -e "  ${GREEN}✓${NC} Copied $copied DLLs"

    # Recursively check copied DLLs for more dependencies (2 passes)
    for pass in 1 2; do
        new_count=0
        for existing_dll in *.dll; do
            if [ -f "$existing_dll" ]; then
                new_deps=$(ldd "$existing_dll" 2>/dev/null | grep -i '/mingw64/bin' | awk '{print $3}' | sort -u)
                for dep_path in $new_deps; do
                    dep_name=$(basename "$dep_path")
                    if [ -f "$dep_path" ] && [ ! -f "$dep_name" ]; then
                        cp "$dep_path" . 2>/dev/null && ((new_count++)) || true
                    fi
                done
            fi
        done
        if [ $new_count -gt 0 ]; then
            echo -e "  ${GREEN}✓${NC} Pass $pass: Copied $new_count additional DLLs"
        fi
    done
else
    echo -e "  ${YELLOW}⚠${NC} ldd not available, copying common DLLs..."

    # Fallback: copy common DLLs manually
    COMMON_DLLS=(
        "libgcc_s_seh-1.dll"
        "libstdc++-6.dll"
        "libwinpthread-1.dll"
        "libb2-1.dll"
        "libicuin*.dll"
        "libicuuc*.dll"
        "libicudt*.dll"
        "libpcre2-16-0.dll"
        "libpcre2-8-0.dll"
        "zlib1.dll"
        "libbz2-1.dll"
        "libharfbuzz-0.dll"
        "libpng16-16.dll"
        "libfreetype-6.dll"
        "libglib-2.0-0.dll"
        "libintl-8.dll"
        "libiconv-2.dll"
        "libgraphite2.dll"
        "libbrotlidec.dll"
        "libbrotlicommon.dll"
        "libdouble-conversion.dll"
        "libzstd.dll"
        "libmd4c.dll"
    )

    for pattern in "${COMMON_DLLS[@]}"; do
        for dll in /mingw64/bin/$pattern; do
            [ -f "$dll" ] && cp "$dll" . 2>/dev/null || true
        done
    done
fi

cd ..

# Check for missing dependencies
echo ""
echo -e "${BLUE}Checking for missing dependencies...${NC}"
if command -v ldd &> /dev/null; then
    cd "$DIST_DIR"
    MISSING=$(ldd CompareObserver.exe | grep "not found" | wc -l)
    if [ "$MISSING" -eq 0 ]; then
        echo -e "  ${GREEN}✓ All dependencies satisfied!${NC}"
    else
        echo -e "  ${YELLOW}⚠ Warning: $MISSING missing dependencies${NC}"
        ldd CompareObserver.exe | grep "not found" || true
    fi
    cd ..
fi

# Print results
echo ""
echo -e "${GREEN}======================================${NC}"
echo -e "${GREEN}Build Complete!${NC}"
echo -e "${GREEN}======================================${NC}"
echo ""
echo -e "Executable: ${BLUE}$BUILD_DIR/bin/CompareObserver.exe${NC}"
echo -e "Distribution: ${BLUE}$DIST_DIR/${NC}"
echo ""
echo -e "To run from MSYS2:"
echo -e "  ${BLUE}./$DIST_DIR/CompareObserver.exe${NC}"
echo ""
echo -e "To run from Windows:"
echo -e "  ${BLUE}Add C:\\msys64\\mingw64\\bin to PATH${NC}"
echo -e "  ${BLUE}Or double-click $DIST_DIR/CompareObserver.exe${NC}"
echo ""
echo -e "To create portable ZIP:"
echo -e "  ${BLUE}cd $DIST_DIR && zip -r CompareObserver-portable.zip *${NC}"
echo ""