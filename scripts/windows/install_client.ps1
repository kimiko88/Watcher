<#
.SYNOPSIS
    Installs the Classroom Control Client Service on Windows.
.DESCRIPTION
    This script installs the client executable, registers it as a Windows Service,
    configures firewall rules, and starts the service.
.NOTES
    Requires Administrator privileges.
#>

$ErrorActionPreference = "Stop"

# 1. Verify Admin Privileges
if (!([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Error "This script must be run as Administrator."
    exit 1
}

# Configuration
$TargetDir = "C:\Program Files\ClassroomControl"
$ServiceName = "ClassroomControlClient"
$DisplayName = "Classroom Control Client"
$BinPath = Join-Path $TargetDir "cms_client.exe"
$ConfigPath = Join-Path $TargetDir "config.json"
$SourceExe = "cms_client.exe" # Assumes script runs from a deployment folder containing the exe
$SourceConfig = "config.json"

try {
    Write-Host "Starting installation..." -ForegroundColor Green

    # 2. Check Antivirus/Firewall (logging warning)
    # Simple check for Windows Defender status or just log a warning
    Write-Warning "Ensure Antivirus exclusions are set for $TargetDir if connection is blocked."

    # 3. Create folder
    if (!(Test-Path $TargetDir)) {
        New-Item -ItemType Directory -Force -Path $TargetDir | Out-Null
        Write-Host "Created directory: $TargetDir"
    }

    # 4. Copy executable and config
    if (Test-Path $SourceExe) {
        Copy-Item -Path $SourceExe -Destination $BinPath -Force
        Write-Host "Copied executable."
    }
    else {
        Write-Warning "Source executable '$SourceExe' not found in current directory. Skipping file copy (assuming dev/test mode)."
    }

    if (Test-Path $SourceConfig) {
        Copy-Item -Path $SourceConfig -Destination $ConfigPath -Force
        Write-Host "Copied config."
    }
    elseif (!(Test-Path $ConfigPath)) {
        # Create default config if missing
        @{
            master_address = "127.0.0.1"
            master_port    = 5000
            machine_id     = [Guid]::NewGuid().ToString()
            log_level      = "INFO"
        } | ConvertTo-Json | Set-Content $ConfigPath
        Write-Host "Created default config."
    }

    # 5. Create Service
    # Check if service exists
    $Service = Get-Service -Name $ServiceName -ErrorAction SilentlyContinue
    if ($Service) {
        Write-Host "Service already exists. Stopping..."
        Stop-Service -Name $ServiceName -Force -ErrorAction SilentlyContinue
        Write-Host "Deleting existing service to recreate..."
        sc.exe delete $ServiceName
        Start-Sleep -Seconds 2
    }
    
    Write-Host "Creating Windows Service..."
    # Use PowerShell's native New-Service cmdlet for reliability
    $binPathValue = "`"$BinPath`" --service"
    
    try {
        New-Service -Name $ServiceName `
            -BinaryPathName $binPathValue `
            -DisplayName $DisplayName `
            -StartupType Automatic `
            -DependsOn @("Tcpip", "EventLog") `
            -ErrorAction Stop
        
        Write-Host "Service created successfully."
    }
    catch {
        Write-Error "Failed to create service: $_"
        exit 1
    }

    # 6. Firewall Rule
    # Remove old rule if exists
    Remove-NetFirewallRule -DisplayName $DisplayName -ErrorAction SilentlyContinue
    New-NetFirewallRule -DisplayName $DisplayName -Direction Inbound -Program $BinPath -Action Allow -Profile Any | Out-Null
    Write-Host "Firewall rule created."

    # 7. Start Service
    Write-Host "Starting service..."
    try {
        Start-Service -Name $ServiceName -ErrorAction Stop
        
        # 8. Verify
        Start-Sleep -Seconds 2
        $ServiceStatus = Get-Service -Name $ServiceName
        
        if ($ServiceStatus.Status -eq "Running") {
            Write-Host "Installation SUCCESS! Service is running." -ForegroundColor Green
            Write-Host "Service Name: $ServiceName" -ForegroundColor Cyan
            Write-Host "Installation Path: $TargetDir" -ForegroundColor Cyan
            Write-Host "Config File: $ConfigPath" -ForegroundColor Cyan
        }
        else {
            Write-Error "Service failed to start. Current status: $($ServiceStatus.Status)"
            Write-Host "Check Event Viewer (eventvwr.msc) for error details." -ForegroundColor Yellow
            exit 1
        }
    }
    catch {
        Write-Error "Failed to start service: $_"
        Write-Host "Troubleshooting tips:" -ForegroundColor Yellow
        Write-Host "  1. Check if cms_client.exe exists at: $BinPath" -ForegroundColor Yellow
        Write-Host "  2. Verify config.json syntax at: $ConfigPath" -ForegroundColor Yellow
        Write-Host "  3. Check Event Viewer for service startup errors" -ForegroundColor Yellow
        exit 1
    }

}
catch {
    Write-Error "Installation failed: $_"
    exit 1
}
