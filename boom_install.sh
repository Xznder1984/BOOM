#!/usr/bin/env bash
# BOOM - Universal Installer for macOS, Linux, and WSL
# Detects OS, checks dependencies, installs BOOM
# Usage: curl -sSL https://raw.githubusercontent.com/Xznder1984/BOOM/main/boom_install.sh | bash
set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

BOOM_REPO="https://github.com/Xznder1984/BOOM.git"
BOOM_DIR="$HOME/.boom"
BOOM_BIN="$HOME/.local/bin"

echo ""
echo -e "${RED}██████╗  ██████╗ ███████╗${NC}"
echo -e "${RED}██╔══██╗██╔═══██╗██╔════╝${NC}"
echo -e "${RED}██████╔╝██║   ██║███████╗${NC}"
echo -e "${RED}██╔══██╗██║   ██║╚════██║${NC}"
echo -e "${RED}██████╔╝╚██████╔╝███████║${NC}"
echo -e "${RED}╚═════╝  ╚═════╝ ╚══════╝${NC}"
echo -e "${BOLD}Terminal DOOM Clone v1.0.0${NC}"
echo ""
echo -e "${CYAN}https://github.com/Xznder1984/BOOM${NC}"
echo ""

# --- Detect OS ---
OS="unknown"
ARCH=$(uname -m 2>/dev/null || echo "unknown")

detect_os() {
    case "$(uname -s)" in
        Linux*)
            if grep -qi microsoft /proc/version 2>/dev/null; then
                OS="wsl"
            else
                OS="linux"
            fi
            ;;
        Darwin*)
            OS="macos"
            ;;
        CYGWIN*|MINGW*|MSYS*)
            OS="windows_unix"
            ;;
        *)
            OS="unknown"
            ;;
    esac

    if [ "$OS" = "unknown" ]; then
        if [ -n "$WINDIR" ] || [ -n "$windir" ]; then
            OS="windows"
        fi
    fi
}

detect_os

echo -e "${BOLD}System Detection:${NC}"
echo -e "  OS:       ${CYAN}${OS}${NC}"
echo -e "  Arch:     ${CYAN}${ARCH}${NC}"
echo -e "  Kernel:   ${CYAN}$(uname -r 2>/dev/null || echo 'N/A')${NC}"
echo ""

# --- Check Python ---
echo -e "${BOLD}Checking dependencies...${NC}"
PYTHON=""
for py in python3 python python3.14 python3.13 python3.12 python3.11 python3.10 python3.9; do
    if command -v "$py" >/dev/null 2>&1; then
        PY_VER=$("$py" -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')" 2>/dev/null || echo "0.0")
        PY_MAJOR=$("$py" -c "import sys; print(sys.version_info.major)" 2>/dev/null || echo "0")
        PY_MINOR=$("$py" -c "import sys; print(sys.version_info.minor)" 2>/dev/null || echo "0")
        if [ "$PY_MAJOR" -ge 3 ] && [ "$PY_MINOR" -ge 6 ]; then
            PYTHON="$py"
            echo -e "  ${GREEN}Python ${PY_VER} found: ${PYTHON}${NC}"
            break
        fi
    fi
done

if [ -z "$PYTHON" ]; then
    echo -e "  ${RED}Python 3.6+ not found!${NC}"
    echo ""
    echo -e "${YELLOW}Please install Python 3.6+ first:${NC}"
    case "$OS" in
        macos)
            echo "  brew install python3"
            echo "  # or download from https://www.python.org/downloads/"
            ;;
        linux|wsl)
            echo "  sudo apt install python3 python3-pip python3-venv"
            echo "  # or: sudo dnf install python3"
            echo "  # or: sudo pacman -S python"
            ;;
        *)
            echo "  https://www.python.org/downloads/"
            ;;
    esac
    exit 1
fi

# --- Check pip ---
PIP=""
for pip_cmd in "$PYTHON -m pip" "pip3" "pip"; do
    if $pip_cmd --version >/dev/null 2>&1; then
        PIP="$pip_cmd"
        echo -e "  ${GREEN}pip found: ${PIP}${NC}"
        break
    fi
done

if [ -z "$PIP" ]; then
    echo -e "  ${YELLOW}pip not found, installing...${NC}"
    $PYTHON -m ensurepip --upgrade 2>/dev/null || true
    PIP="$PYTHON -m pip"
fi

# --- Check git ---
HAS_GIT=false
if command -v git >/dev/null 2>&1; then
    HAS_GIT=true
    echo -e "  ${GREEN}git found${NC}"
else
    echo -e "  ${YELLOW}git not found, will download ZIP instead${NC}"
fi

# --- Check terminal capabilities ---
TERM_OK=true
TERM_NAME="${TERM:-dumb}"
echo -e "  Terminal:  ${CYAN}${TERM_NAME}${NC}"

# --- Install windows-curses on Windows/WSL if needed ---
case "$OS" in
    windows|wsl|windows_unix)
        echo -e "  ${YELLOW}Windows/WSL detected - checking curses support...${NC}"
        if ! $PYTHON -c "import curses" 2>/dev/null; then
            echo -e "  Installing windows-curses..."
            $PYTHON -m pip install windows-curses 2>/dev/null || echo -e "  ${YELLOW}Could not install windows-curses (may need manual install)${NC}"
        else
            echo -e "  ${GREEN}curses module available${NC}"
        fi
        ;;
    *)
        if $PYTHON -c "import curses" 2>/dev/null; then
            echo -e "  ${GREEN}curses module available${NC}"
        else
            echo -e "  ${YELLOW}curses not found, attempting install...${NC}"
            case "$OS" in
                macos)
                    echo "  On macOS, curses is usually built-in."
                    echo "  If needed: xcode-select --install"
                    ;;
                linux)
                    $PYTHON -m pip install windows-curses 2>/dev/null || true
                    ;;
            esac
        fi
        ;;
esac

echo ""

# --- Download BOOM ---
echo -e "${BOLD}Downloading BOOM...${NC}"

mkdir -p "$BOOM_DIR" 2>/dev/null || true
rm -rf "$BOOM_DIR" 2>/dev/null || true

if [ "$HAS_GIT" = true ]; then
    echo -e "  Cloning repository..."
    git clone --depth 1 "$BOOM_REPO" "$BOOM_DIR" 2>/dev/null
else
    echo -e "  Downloading ZIP..."
    ZIP_URL="https://github.com/Xznder1984/BOOM/archive/refs/heads/main.zip"
    TMP_ZIP="/tmp/boom_install_$$.zip"
    if command -v curl >/dev/null 2>&1; then
        curl -sSL -o "$TMP_ZIP" "$ZIP_URL"
    elif command -v wget >/dev/null 2>&1; then
        wget -q -O "$TMP_ZIP" "$ZIP_URL"
    else
        echo -e "  ${RED}Neither curl nor wget found!${NC}"
        exit 1
    fi
    $PYTHON -c "
import zipfile, os, shutil
zf = zipfile.ZipFile('$TMP_ZIP')
zf.extractall('/tmp/boom_extract_$$_dir')
src = '/tmp/boom_extract_$$_dir/BOOM-main'
dst = '$BOOM_DIR'
if os.path.exists(dst):
    shutil.rmtree(dst)
shutil.move(src, dst)
shutil.rmtree('/tmp/boom_extract_$$_dir')
"
    rm -f "$TMP_ZIP"
fi

if [ ! -f "$BOOM_DIR/boom.py" ]; then
    echo -e "  ${RED}Download failed! boom.py not found in $BOOM_DIR${NC}"
    exit 1
fi
echo -e "  ${GREEN}BOOM downloaded to $BOOM_DIR${NC}"

# --- Install dependencies ---
echo -e "${BOLD}Installing dependencies...${NC}"
$PYTHON -m pip install -q --upgrade pip 2>/dev/null || true
if [ -f "$BOOM_DIR/requirements.txt" ]; then
    $PYTHON -m pip install -q -r "$BOOM_DIR/requirements.txt" 2>/dev/null || true
fi
echo -e "  ${GREEN}Dependencies installed${NC}"

# --- Create launcher script ---
echo -e "${BOLD}Creating launcher...${NC}"
mkdir -p "$BOOM_BIN" 2>/dev/null || true

cat > "$BOOM_BIN/boom" << LAUNCHER
#!/usr/bin/env bash
# BOOM launcher - auto-generated
exec $PYTHON "$BOOM_DIR/boom.py" "\$@"
LAUNCHER
chmod +x "$BOOM_BIN/boom" 2>/dev/null || true

# Also create a direct alias script
cat > "$BOOM_DIR/boom.sh" << 'ALIAS'
#!/usr/bin/env bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec python3 "$SCRIPT_DIR/boom.py" "$@"
ALIAS
chmod +x "$BOOM_DIR/boom.sh" 2>/dev/null || true

echo -e "  ${GREEN}Launcher created at $BOOM_BIN/boom${NC}"

# --- Add to PATH if needed ---
SHELL_RC=""
if [ -f "$HOME/.bashrc" ]; then
    SHELL_RC="$HOME/.bashrc"
elif [ -f "$HOME/.zshrc" ]; then
    SHELL_RC="$HOME/.zshrc"
elif [ -f "$HOME/.profile" ]; then
    SHELL_RC="$HOME/.profile"
fi

if [ -n "$SHELL_RC" ]; then
    if ! grep -q "$BOOM_BIN" "$SHELL_RC" 2>/dev/null; then
        echo "" >> "$SHELL_RC"
        echo "# BOOM terminal DOOM clone" >> "$SHELL_RC"
        echo "export PATH=\"$BOOM_BIN:\$PATH\"" >> "$SHELL_RC"
        echo -e "  ${GREEN}Added $BOOM_BIN to PATH in $SHELL_RC${NC}"
    fi
fi

# --- Quick test ---
echo ""
echo -e "${BOLD}Running quick test...${NC}"
if $PYTHON -c "
import sys
sys.path.insert(0, '$BOOM_DIR')
from boom_wad import WADParser
from boom_engine import Raycaster
from boom_game import create_player
from boom_maps import get_builtin_map
print('All modules loaded successfully!')
print(f'Python: {sys.version}')
" 2>&1; then
    echo -e "  ${GREEN}All systems go!${NC}"
else
    echo -e "  ${YELLOW}Test had issues but BOOM may still work.${NC}"
fi

# --- Done! ---
echo ""
echo -e "${GREEN}${BOLD}========================================${NC}"
echo -e "${GREEN}${BOLD}  BOOM installed successfully!${NC}"
echo -e "${GREEN}${BOLD}========================================${NC}"
echo ""
echo -e "${BOLD}How to play:${NC}"
echo ""
echo -e "  ${CYAN}# From anywhere (if PATH was updated):${NC}"
echo -e "  boom"
echo ""
echo -e "  ${CYAN}# Or directly:${NC}"
echo -e "  $PYTHON $BOOM_DIR/boom.py"
echo ""
echo -e "  ${CYAN}# With a DOOM WAD file:${NC}"
echo -e "  $PYTHON $BOOM_DIR/boom.py --wad /path/to/doom1.wad"
echo ""
echo -e "  ${CYAN}# List available maps:${NC}"
echo -e "  $PYTHON $BOOM_DIR/boom.py --wad /path/to/doom1.wad --list-maps"
echo ""
echo -e "${BOLD}Controls:${NC}"
echo "  W/A/S/D    - Move / Strafe"
echo "  Left/Right - Turn"
echo "  Space      - Use door / switch"
echo "  F          - Fire weapon"
echo "  1-7        - Switch weapon"
echo "  Tab        - Toggle minimap"
echo "  +/-        - Adjust FPS"
echo "  Q          - Quit"
echo ""
echo -e "${BOLD}Music (optional):${NC}"
echo "  Place MP3/FLAC files in $BOOM_DIR/music/"
echo "  Named like: '02. At Doom's Gate.mp3'"
echo "  Or run: bash $BOOM_DIR/boom_audio_setup.sh"
echo ""
echo -e "Source: ${CYAN}https://github.com/Xznder1984/BOOM${NC}"
echo ""
