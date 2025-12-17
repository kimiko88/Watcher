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
        # We don't delete to preserve config, but we updating bin path/config requires sc config normally
        # For clean install, one might sc delete. Let's assume update.
    }
    else {
        Write-Host "Creating Windows Service..."
        # Note: binPath must be quoted if contains spaces
        $scArgs = @("create", $ServiceName, "binPath=", "`"$BinPath`" --service", "start=", "auto", "DisplayName=", $DisplayName, "depend=", "Tcpip/EventLog")
        Start-Process -FilePath "sc.exe" -ArgumentList $scArgs -Wait -NoNewWindow
    }

    # 6. Firewall Rule
    # Remove old rule if exists
    Remove-NetFirewallRule -DisplayName $DisplayName -ErrorAction SilentlyContinue
    New-NetFirewallRule -DisplayName $DisplayName -Direction Inbound -Program $BinPath -Action Allow -Profile Any | Out-Null
    Write-Host "Firewall rule created."

    # 7. Start Service
    Write-Host "Starting service..."
    Start-Service -Name $ServiceName
    
    # 8. Verify
    Start-Sleep -Seconds 2
    $Status = (Get-Service -Name $ServiceName).Status
    if ($Status -eq "Running") {
        Write-Host "Installation SUCCESS! Service is running." -ForegroundColor Green
        
        # Event log entry check (simulated or real if service supports it)
        # Write-EventLog -LogName Application -Source "ClassroomControl" ...
    }
    else {
        Write-Error "Service failed to start. Current status: $Status"
        exit 1
    }

}
catch {
    Write-Error "Installation failed: $_"
    exit 1
}
