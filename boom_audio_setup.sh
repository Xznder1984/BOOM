#!/usr/bin/env bash
# BOOM Audio Setup - Download DOOM music from Internet Archive
# This is optional - BOOM works without music using terminal beeps
# Usage: bash boom_audio_setup.sh
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MUSIC_DIR="$SCRIPT_DIR/music"
TMP_DIR="/tmp/boom_music_dl"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo ""
echo -e "${CYAN}BOOM Audio Setup - Download DOOM Music${NC}"
echo ""

# Check for downloader
CURL=""
if command -v curl >/dev/null 2>&1; then
    CURL="curl -sSL"
elif command -v wget >/dev/null 2>&1; then
    CURL="wget -q -O -"
else
    echo -e "${RED}Error: Neither curl nor wget found!${NC}"
    exit 1
fi

mkdir -p "$MUSIC_DIR"
mkdir -p "$TMP_DIR"

# Archive.org URLs for DOOM OST (MP3 format)
# Bobby Prince DOOM OST from archive.org
BASE_URL="https://archive.org/download/bobby-prince-doom-ost"

TRACKS=(
    "01. Introduction.mp3|01. Introduction"
    "02. At Doom's Gate.mp3|02. At Doom's Gate"
    "03. The Imp's Song.mp3|03. The Imp's Song"
    "04. Dark Halls.mp3|04. Dark Halls"
    "05. Kitchen Ace (and Taking Names).mp3|05. Kitchen Ace (and Taking Names)"
    "06. Suspense.mp3|06. Suspense"
    "07. On the Hunt.mp3|07. On the Hunt"
    "08. Demons on the Prey.mp3|08. Demons on the Prey"
    "09. Sign of Evil.mp3|09. Sign of Evil"
    "10. Hiding the Secrets.mp3|10. Hiding the Secrets"
    "11. I Sawed the Demons.mp3|11. I Sawed the Demons"
    "12. The Demons from Adrian's Pen.mp3|12. The Demons from Adrian's Pen"
    "13. Intermission from DOOM.mp3|13. Intermission from DOOM"
    "14. They're Going To Get You.mp3|14. They're Going to Get You"
    "15. Sinister.mp3|15. Sinister"
    "16. Waltz of the Demons.mp3|16. Waltz of the Demons"
    "17. Nobody Told Me About id.mp3|17. Nobody Told Me About id"
    "18. Hell Keep.mp3|18. Hell Keep"
    "19. Donna to the Rescue.mp3|19. Donna to the Rescue"
    "20. Deep Into the Code.mp3|20. Deep Into the Code"
    "21. Facing the Spider.mp3|21. Facing the Spider"
    "22. Victory.mp3|22. Victory"
    "23. Sweet Little Dead Bunny.mp3|23. Sweet Little Dead Bunny"
)

echo -e "${YELLOW}Note: DOOM music is copyright id Software / Bobby Prince.${NC}"
echo -e "${YELLOW}These files are for personal use only.${NC}"
echo ""
echo "Downloading ${#TRACKS[@]} tracks to: $MUSIC_DIR"
echo ""

downloaded=0
failed=0

for entry in "${TRACKS[@]}"; do
    IFS='|' read -r filename name <<< "$entry"
    target="$MUSIC_DIR/$name.mp3"

    if [ -f "$target" ]; then
        echo -e "  ${GREEN}[SKIP]${NC} $name (already exists)"
        ((downloaded++))
        continue
    fi

    encoded=$(python3 -c "import urllib.parse; print(urllib.parse.quote('$filename'))" 2>/dev/null || echo "$filename")
    url="$BASE_URL/$encoded"

    echo -n "  Downloading: $name ... "

    if command -v curl >/dev/null 2>&1; then
        if curl -sSL -o "$target" --connect-timeout 10 --max-time 30 "$url" 2>/dev/null; then
            if [ -s "$target" ]; then
                echo -e "${GREEN}OK${NC}"
                ((downloaded++))
            else
                rm -f "$target"
                echo -e "${RED}EMPTY${NC}"
                ((failed++))
            fi
        else
            rm -f "$target"
            echo -e "${RED}FAILED${NC}"
            ((failed++))
        fi
    elif command -v wget >/dev/null 2>&1; then
        if wget -q -O "$target" --timeout=10 "$url" 2>/dev/null; then
            if [ -s "$target" ]; then
                echo -e "${GREEN}OK${NC}"
                ((downloaded++))
            else
                rm -f "$target"
                echo -e "${RED}EMPTY${NC}"
                ((failed++))
            fi
        else
            rm -f "$target"
            echo -e "${RED}FAILED${NC}"
            ((failed++))
        fi
    fi

    sleep 0.5
done

rm -rf "$TMP_DIR" 2>/dev/null

echo ""
echo -e "${CYAN}Results:${NC}"
echo -e "  Downloaded: ${GREEN}${downloaded}${NC}/${#TRACKS[@]}"
if [ "$failed" -gt 0 ]; then
    echo -e "  Failed: ${RED}${failed}${NC}"
    echo ""
    echo -e "${YELLOW}Some tracks failed to download. This may be due to:${NC}"
    echo "  - Network issues"
    echo "  - Archive.org rate limiting"
    echo "  - File naming differences"
    echo ""
    echo "You can manually download from:"
    echo "  https://archive.org/details/bobby-prince-doom-ost"
    echo "  https://archive.org/details/03-dark-halls"
    echo "  https://archive.org/download/bobby-prince-doom-ost/"
    echo ""
    echo "Place MP3 files in: $MUSIC_DIR/"
fi

echo ""
echo -e "${GREEN}Audio setup complete!${NC}"
echo -e "Music directory: $MUSIC_DIR"
echo ""
