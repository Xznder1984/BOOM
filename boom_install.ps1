# BOOM - Windows PowerShell Installer
# Usage: irm https://raw.githubusercontent.com/Xznder1984/BOOM/main/boom_install.ps1 | iex
# Or:    powershell -ExecutionPolicy Bypass -Command "irm https://raw.githubusercontent.com/Xznder1984/BOOM/main/boom_install.ps1 | iex"

$ErrorActionPreference = "Stop"

$BOOM_REPO = "https://github.com/Xznder1984/BOOM.git"
$BOOM_DIR = "$env:USERPROFILE\.boom"
$BOOM_BIN = "$env:USERPROFILE\.boom\bin"

Write-Host ""
Write-Host "  ███████╗  ██████╗ ███████╗" -ForegroundColor Red
Write-Host "  ██╔════╝ ██╔═══██╗██╔════╝" -ForegroundColor Red
Write-Host "  ███████╗ ██║   ██║███████╗" -ForegroundColor Red
Write-Host "  ╚════██║ ██║   ██║╚════██║" -ForegroundColor Red
Write-Host "  ███████║ ╚██████╔╝███████║" -ForegroundColor Red
Write-Host "  ╚══════╝  ╚═════╝ ╚══════╝" -ForegroundColor Red
Write-Host "  Terminal DOOM Clone v1.0.0" -ForegroundColor Cyan
Write-Host ""
Write-Host "  https://github.com/Xznder1984/BOOM" -ForegroundColor DarkCyan
Write-Host ""

# --- Detect System ---
Write-Host "System Detection:" -ForegroundColor Yellow
$OS = if ($IsMacOS) { "macos" } elseif ($IsLinux) { "linux" } else { "windows" }
$Arch = [System.Environment]::Is64BitOperatingSystem ? "x64" : "x86"
$PSVer = $PSVersionTable.PSVersion.ToString()
Write-Host "  OS:       $OS"
Write-Host "  Arch:     $Arch"
Write-Host "  PS:       $PSVer"
Write-Host ""

# --- Check Python ---
Write-Host "Checking dependencies..." -ForegroundColor Yellow
$Python = $null
foreach ($py in @("python", "python3", "py")) {
    try {
        $ver = & $py -c "import sys; print(f'{sys.version_info.major}.{sys.version_info.minor}')" 2>$null
        $major = & $py -c "import sys; print(sys.version_info.major)" 2>$null
        $minor = & $py -c "import sys; print(sys.version_info.minor)" 2>$null
        if ([int]$major -ge 3 -and [int]$minor -ge 6) {
            $Python = $py
            Write-Host "  Python $ver found: $Python" -ForegroundColor Green
            break
        }
    } catch {}
}

if (-not $Python) {
    Write-Host "  Python 3.6+ NOT FOUND!" -ForegroundColor Red
    Write-Host ""
    Write-Host "  Please install Python from: https://www.python.org/downloads/" -ForegroundColor Yellow
    Write-Host "  Make sure to check 'Add Python to PATH' during installation!"
    exit 1
}

# --- Check pip ---
Write-Host "  Checking pip..." -ForegroundColor Gray
& $Python -m pip --version 2>$null | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "  Installing pip..." -ForegroundColor Yellow
    & $Python -m ensurepip --upgrade 2>$null
}

# --- Check git ---
$HasGit = $false
try {
    $null = Get-Command git -ErrorAction Stop
    $HasGit = $true
    Write-Host "  git found" -ForegroundColor Green
} catch {
    Write-Host "  git not found, will download ZIP" -ForegroundColor Yellow
}

# --- Install curses ---
Write-Host "  Checking curses support..." -ForegroundColor Gray
& $Python -c "import curses" 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Host "  Installing windows-curses..." -ForegroundColor Yellow
    & $Python -m pip install windows-curses 2>$null
}

Write-Host ""

# --- Download BOOM ---
Write-Host "Downloading BOOM..." -ForegroundColor Yellow

if (Test-Path $BOOM_DIR) {
    Remove-Item -Recurse -Force $BOOM_DIR -ErrorAction SilentlyContinue
}

if ($HasGit) {
    Write-Host "  Cloning repository..."
    git clone --depth 1 $BOOM_REPO $BOOM_DIR 2>$null
} else {
    Write-Host "  Downloading ZIP..."
    $ZipUrl = "https://github.com/Xznder1984/BOOM/archive/refs/heads/main.zip"
    $TmpZip = "$env:TEMP\boom_install.zip"
    $TmpExtract = "$env:TEMP\boom_extract"

    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -Uri $ZipUrl -OutFile $TmpZip -UseBasicParsing

    if (Test-Path $TmpExtract) { Remove-Item -Recurse -Force $TmpExtract }
    Expand-Archive -Path $TmpZip -DestinationPath $TmpExtract -Force

    $ExtractedDir = Get-ChildItem -Path $TmpExtract -Directory | Select-Object -First 1
    if ($ExtractedDir) {
        Move-Item -Path $ExtractedDir.FullName -Destination $BOOM_DIR -Force
    }

    Remove-Item -Recurse -Force $TmpExtract -ErrorAction SilentlyContinue
    Remove-Item -Force $TmpZip -ErrorAction SilentlyContinue
}

if (-not (Test-Path "$BOOM_DIR\boom.py")) {
    Write-Host "  Download failed! boom.py not found." -ForegroundColor Red
    exit 1
}
Write-Host "  BOOM downloaded to $BOOM_DIR" -ForegroundColor Green

# --- Install dependencies ---
Write-Host "Installing dependencies..." -ForegroundColor Yellow
& $Python -m pip install -q --upgrade pip 2>$null
if (Test-Path "$BOOM_DIR\requirements.txt") {
    & $Python -m pip install -q -r "$BOOM_DIR\requirements.txt" 2>$null
}
Write-Host "  Dependencies installed" -ForegroundColor Green

# --- Create launcher ---
Write-Host "Creating launcher..." -ForegroundColor Yellow
New-Item -ItemType Directory -Path $BOOM_BIN -Force | Out-Null

$LauncherContent = @"
@echo off
"$Python" "%~dp0..\boom.py" %*
"@
Set-Content -Path "$BOOM_BIN\boom.bat" -Value $LauncherContent

# Also create a PowerShell launcher
$PSLauncher = @"
`$ScriptDir = Split-Path -Parent `$MyInvocation.MyCommand.Path
& "$Python" "`$ScriptDir\..\boom.py" @args
"@
Set-Content -Path "$BOOM_BIN\boom.ps1" -Value $PSLauncher

Write-Host "  Launcher created at $BOOM_BIN\boom.bat" -ForegroundColor Green

# --- Add to PATH ---
$CurrentPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($CurrentPath -notlike "*$BOOM_BIN*") {
    [Environment]::SetEnvironmentVariable("Path", "$BOOM_BIN;$CurrentPath", "User")
    $env:Path = "$BOOM_BIN;$env:Path"
    Write-Host "  Added $BOOM_BIN to user PATH" -ForegroundColor Green
}

# --- Quick test ---
Write-Host ""
Write-Host "Running quick test..." -ForegroundColor Yellow
& $Python -c @"
import sys
sys.path.insert(0, r'$BOOM_DIR')
from boom_wad import WADParser
from boom_engine import Raycaster
from boom_game import create_player
from boom_maps import get_builtin_map
print('All modules loaded successfully!')
print(f'Python: {sys.version}')
"@ 2>&1

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  BOOM installed successfully!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "How to play:" -ForegroundColor Yellow
Write-Host ""
Write-Host "  # From anywhere (restart terminal first):"
Write-Host "  boom"
Write-Host ""
Write-Host "  # Or directly:"
Write-Host "  $Python $BOOM_DIR\boom.py"
Write-Host ""
Write-Host "  # With a DOOM WAD file:"
Write-Host "  $Python $BOOM_DIR\boom.py --wad C:\path\to\doom1.wad"
Write-Host ""
Write-Host "Controls:" -ForegroundColor Yellow
Write-Host "  W/A/S/D    - Move / Strafe"
Write-Host "  Left/Right - Turn"
Write-Host "  Space      - Use door / switch"
Write-Host "  F          - Fire weapon"
Write-Host "  1-7        - Switch weapon"
Write-Host "  Tab        - Toggle minimap"
Write-Host "  +/-        - Adjust FPS"
Write-Host "  Q          - Quit"
Write-Host ""
Write-Host "Music (optional):" -ForegroundColor Yellow
Write-Host "  Place MP3/FLAC files in $BOOM_DIR\music\"
Write-Host "  Or run: $Python $BOOM_DIR\boom_audio_setup.ps1"
Write-Host ""
Write-Host "Source: https://github.com/Xznder1984/BOOM" -ForegroundColor DarkCyan
Write-Host ""
