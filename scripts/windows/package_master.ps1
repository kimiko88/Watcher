<#
.SYNOPSIS
    Packages the Classroom Control Master for deployment.
.DESCRIPTION
    This script creates a deployment package containing:
    - cms_master.exe
    - All required Qt6 DLLs and plugins
    - Visual C++ runtime dependencies
    - Default configuration file
.NOTES
    Run this script on the build machine where Qt is installed.
#>

param(
    [string]$BuildConfig = "Release",
    [string]$OutputDir = "C:\Temp\CMSMaster_Deploy"
)

$ErrorActionPreference = "Stop"

# Configuration
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$BuildDir = Join-Path $ProjectRoot "build"
$MasterExePath = Join-Path $BuildDir "src\master\$BuildConfig\cms_master.exe"
$QtBinPath = "C:\Qt\6.10.1\msvc2022_64\bin"
$WinDeployQt = Join-Path $QtBinPath "windeployqt.exe"

Write-Host "=== Classroom Control Master - Deployment Packager ===" -ForegroundColor Cyan
Write-Host ""

# 1. Check if master executable exists
if (!(Test-Path $MasterExePath)) {
    Write-Error "Master executable not found at: $MasterExePath"
    Write-Host "Please build the project first:" -ForegroundColor Yellow
    Write-Host "  cmake --build build --config $BuildConfig" -ForegroundColor Yellow
    exit 1
}

Write-Host "[1/6] Found master executable: $MasterExePath" -ForegroundColor Green

# 2. Check windeployqt
if (!(Test-Path $WinDeployQt)) {
    Write-Error "windeployqt not found at: $WinDeployQt"
    Write-Host "Please set the correct Qt path in the script." -ForegroundColor Yellow
    exit 1
}

Write-Host "[2/6] Found windeployqt tool" -ForegroundColor Green

# 3. Create output directory
if (Test-Path $OutputDir) {
    Write-Host "Cleaning existing deployment directory..." -ForegroundColor Yellow
    Remove-Item $OutputDir -Recurse -Force
}
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null

Write-Host "[3/6] Created deployment directory: $OutputDir" -ForegroundColor Green

# 4. Copy master executable
Copy-Item $MasterExePath -Destination $OutputDir -Force
Write-Host "[4/6] Copied master executable" -ForegroundColor Green

# 5. Run windeployqt to collect Qt dependencies
Write-Host "[5/6] Collecting Qt dependencies (this may take a moment)..." -ForegroundColor Yellow

$deployExe = Join-Path $OutputDir "cms_master.exe"
$deployArgs = @(
    $deployExe,
    "--release",
    "--no-translations",
    "--no-system-d3d-compiler",
    "--no-opengl-sw"
)

& $WinDeployQt $deployArgs | Out-Null

if ($LASTEXITCODE -ne 0) {
    Write-Error "windeployqt failed with exit code $LASTEXITCODE"
    exit 1
}

Write-Host "[5/6] Qt dependencies collected successfully" -ForegroundColor Green

# 6. Create default configuration file
$defaultConfig = @{
    port              = 5000
    max_clients       = 50
    log_level         = "INFO"
    screenshots_dir   = "screenshots"
    enable_encryption = $false
} | ConvertTo-Json

$configPath = Join-Path $OutputDir "config.json"
$defaultConfig | Set-Content $configPath

Write-Host "[6/6] Created default configuration file" -ForegroundColor Green

# 7. Create README for deployment
$readmeContent = @"
# Classroom Control Master - Deployment Package

## Contents
- cms_master.exe - Main application
- config.json - Configuration file
- *.dll - Required Qt and runtime libraries
- platforms/, styles/ - Qt plugins

## Installation Instructions

1. Copy this entire folder to the target PC (e.g., C:\Program Files\ClassroomControl\Master)

2. Edit config.json if needed:
   - port: Network port (default: 5000)
   - max_clients: Maximum number of clients
   - log_level: DEBUG, INFO, WARNING, or ERROR

3. Run cms_master.exe

4. (Optional) Create a desktop shortcut or Windows service

## Firewall Configuration

Allow incoming connections on port 5000:
```powershell
New-NetFirewallRule -DisplayName "Classroom Control Master" -Direction Inbound -Program "<path>\cms_master.exe" -Action Allow -Profile Any
```

## System Requirements
- Windows 10 or later (64-bit)
- 4 GB RAM minimum
- Network connectivity

## Troubleshooting

If the application doesn't start:
1. Check Event Viewer for errors
2. Verify config.json is valid JSON
3. Ensure no other application is using port 5000
4. Run with --help to see command-line options

For more information, see the full documentation.
"@

$readmePath = Join-Path $OutputDir "README.txt"
$readmeContent | Set-Content $readmePath

Write-Host ""
Write-Host "=== Packaging Complete! ===" -ForegroundColor Green
Write-Host ""
Write-Host "Deployment package created at:" -ForegroundColor Cyan
Write-Host "  $OutputDir" -ForegroundColor White
Write-Host ""
Write-Host "Package contents:" -ForegroundColor Cyan
$items = Get-ChildItem $OutputDir
$totalSize = ($items | Measure-Object -Property Length -Sum).Sum / 1MB
Write-Host "  Total files: $($items.Count)" -ForegroundColor White
Write-Host "  Total size: $([math]::Round($totalSize, 2)) MB" -ForegroundColor White
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Compress this folder to a ZIP file for easy distribution" -ForegroundColor White
Write-Host "  2. Copy to target PC and extract" -ForegroundColor White
Write-Host "  3. Run cms_master.exe" -ForegroundColor White
Write-Host ""

# Option to create ZIP
$createZip = Read-Host "Create a ZIP archive for distribution? (y/n)"
if ($createZip -eq "y" -or $createZip -eq "Y") {
    $zipPath = "$OutputDir.zip"
    
    if (Test-Path $zipPath) {
        Remove-Item $zipPath -Force
    }
    
    Compress-Archive -Path "$OutputDir\*" -DestinationPath $zipPath -CompressionLevel Optimal
    
    Write-Host ""
    Write-Host "ZIP archive created:" -ForegroundColor Green
    Write-Host "  $zipPath" -ForegroundColor White
    
    $zipSize = (Get-Item $zipPath).Length / 1MB
    Write-Host "  Size: $([math]::Round($zipSize, 2)) MB" -ForegroundColor White
}

Write-Host ""
Write-Host "Done!" -ForegroundColor Green
