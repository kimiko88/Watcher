<#
.SYNOPSIS
Installs the Classroom Control Client Service on Windows.

.DESCRIPTION
This script installs the cms_client executable as a Windows Service, configures the firewall,
and sets up the necessary configuration files.
Requires Administrator privileges.

.PARAMETER SourceDir
Directory containing the source files (cms_client.exe, config.json). Defaults to current directory.

.PARAMETER InstallDir
Target installation directory. Defaults to "C:\Program Files\ClassroomControl".

.EXAMPLE
.\install_client.ps1 -SourceDir ".\build\bin"
#>

param (
    [string]$SourceDir = $PSScriptRoot,
    [string]$InstallDir = "C:\Program Files\ClassroomControl"
)

$ServiceName = "ClassroomControlClient"
$DisplayName = "Classroom Control Client"
$ExeName = "cms_client.exe"

# 1. Check Admin Privileges
$currentPrincipal = [Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
if (-not $currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Error "This script requires Administrator privileges. Please run as Administrator."
    exit 1
}

try {
    Write-Host "Starting installation..." -ForegroundColor Cyan

    # 2. Setup Directories
    if (-not (Test-Path -Path $InstallDir)) {
        New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
        Write-Host "Created installation directory: $InstallDir" -ForegroundColor Green
    }

    $SourceExe = Join-Path -Path $SourceDir -ChildPath $ExeName
    $TargetExe = Join-Path -Path $InstallDir -ChildPath $ExeName
    $SourceConfig = Join-Path -Path $SourceDir -ChildPath "config.json"
    $TargetConfig = Join-Path -Path $InstallDir -ChildPath "config.json"

    if (-not (Test-Path -Path $SourceExe)) {
        throw "Executable not found at: $SourceExe"
    }

    # 3. Copy Files
    Stop-Service -Name $ServiceName -ErrorAction SilentlyContinue
    
    # Copy ALL files from source directory (EXE + DLLs + Plugins)
    # Exclude config.json from this bulk copy to handle it separately/safely below
    Get-ChildItem -Path $SourceDir -Exclude "config.json" | Copy-Item -Destination $InstallDir -Recurse -Force
    Write-Host "Copied all files from $SourceDir to $InstallDir" -ForegroundColor Green

    if (Test-Path -Path $SourceConfig) {
        Copy-Item -Path $SourceConfig -Destination $TargetConfig -Force
        Write-Host "Copied config to $TargetConfig" -ForegroundColor Green
    }
    else {
        Write-Warning "config.json not found in source. Using default/existing config."
        if (-not (Test-Path $TargetConfig)) {
            # Create default config if missing
            $defaultConfig = @{
                server_ip   = "127.0.0.1"
                server_port = 5000
                client_name = $env:COMPUTERNAME
            } | ConvertTo-Json
            $defaultConfig | Set-Content -Path $TargetConfig
            Write-Host "Created default config.json" -ForegroundColor Yellow
        }
    }

    # 4. Service Management (Recreate to ensure correct config)
    $service = Get-Service -Name $ServiceName -ErrorAction SilentlyContinue
    if ($service) {
        Write-Host "Service exists. Removing old instance..."
        Stop-Service -Name $ServiceName -ErrorAction SilentlyContinue
        # Use Start-Process to avoid PowerShell quote parsing issues with sc.exe
        $proc = Start-Process -FilePath "sc.exe" -ArgumentList "delete $ServiceName" -PassThru -Wait
        if ($proc.ExitCode -ne 0) {
            Write-Warning "Failed to delete service. It might be marked for deletion. Retrying creation anyway..."
        }
        Start-Sleep -Seconds 2
    }

    Write-Host "Creating Windows Service..."
    $binPath = "`"$TargetExe`" --service"
    Write-Host "Binary Path: $binPath" -ForegroundColor Gray
    
    # Use New-Service (PowerShell native)
    New-Service -Name $ServiceName `
        -DisplayName $DisplayName `
        -BinaryPathName $binPath `
        -StartupType Automatic `
        -DependsOn "Tcpip", "EventLog" `
        -Description "Client service for Classroom Management System." `
        -ErrorAction Stop
    
    Write-Host "Service created successfully." -ForegroundColor Green

    # 5. Firewall Rules
    cmd /c "netsh advfirewall firewall delete rule name=""$DisplayName""" | Out-Null
    New-NetFirewallRule -DisplayName "$DisplayName" -Direction Inbound -Program "$TargetExe" -Action Allow -Profile Any | Out-Null
    Write-Host "Added Firewall exception" -ForegroundColor Green

    # 6. Start Service
    Start-Service -Name $ServiceName
    Write-Host "Service started successfully." -ForegroundColor Green

    # 7. Verification
    $status = Get-Service -Name $ServiceName
    if ($status.Status -eq 'Running') {
        Write-Host "Installation Complete! Service is RUNNING." -ForegroundColor Cyan
    }
    else {
        Write-Error "Service failed to start. Check Event Viewer."
    }

}
catch {
    Write-Error "Installation Failed: $_"
    exit 1
}
