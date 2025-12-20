<#
.SYNOPSIS
    Installs Classroom Control Client as a user application.
.DESCRIPTION
    Installs the client to run automatically on user login using Task Scheduler.
    The client runs in the user session with full desktop access for screen capture.
.NOTES
    Requires Administrator privileges.
#>

$ErrorActionPreference = "Stop"

# Verify Admin Privileges
if (!([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Error "This script must be run as Administrator."
    exit 1
}

# Configuration
$TargetDir = "C:\Program Files\ClassroomControl"
$TaskName = "ClassroomControlClient"
$DisplayName = "Classroom Control Client"
$BinPath = Join-Path $TargetDir "cms_client.exe"
$ConfigPath = Join-Path $TargetDir "config.json"
$StartScript = Join-Path $TargetDir "start_client.bat"
$SourceExe = "cms_client.exe"
$SourceConfig = "config.json"
$SourceBat = "start_client.bat"

try {
    Write-Host "Installing Classroom Control Client (Application Mode)..." -ForegroundColor Green
    Write-Host "This will configure the client to run on user login." -ForegroundColor Cyan
    
    # Check if old service exists
    $Service = Get-Service -Name $TaskName -ErrorAction SilentlyContinue
    if ($Service) {
        Write-Warning "Old service installation detected!"
        Write-Host "Please run 'uninstall_client.ps1' first to remove the service." -ForegroundColor Yellow
        $continue = Read-Host "Continue anyway? (Y/N)"
        if ($continue -ne "Y" -and $continue -ne "y") {
            exit 0
        }
    }
    
    # Create installation directory
    if (!(Test-Path $TargetDir)) {
        New-Item -ItemType Directory -Force -Path $TargetDir | Out-Null
        Write-Host "Created directory: $TargetDir" -ForegroundColor Cyan
    }
    
    # Copy files
    if (Test-Path $SourceExe) {
        Copy-Item -Path $SourceExe -Destination $BinPath -Force
        Write-Host "Copied executable." -ForegroundColor Cyan
    }
    else {
        Write-Warning "Source executable '$SourceExe' not found. Please build the client first."
        exit 1
    }
    
    if (Test-Path $SourceBat) {
        Copy-Item -Path $SourceBat -Destination $StartScript -Force
        Write-Host "Copied startup script." -ForegroundColor Cyan
    }
    
    # Create or update config
    if (Test-Path $SourceConfig) {
        Copy-Item -Path $SourceConfig -Destination $ConfigPath -Force
        Write-Host "Copied config." -ForegroundColor Cyan
    }
    elseif (!(Test-Path $ConfigPath)) {
        # Create default config
        @{
            master_address     = "127.0.0.1"
            master_port        = 5000
            machine_id         = [Guid]::NewGuid().ToString()
            encryption_enabled = $false
            log_level          = "INFO"
        } | ConvertTo-Json | Set-Content $ConfigPath
        Write-Host "Created default config." -ForegroundColor Cyan
    }
    
    # Configure firewall
    Write-Host "Configuring firewall..." -ForegroundColor Cyan
    Remove-NetFirewallRule -DisplayName $DisplayName -ErrorAction SilentlyContinue
    New-NetFirewallRule -DisplayName $DisplayName `
        -Direction Inbound `
        -Program $BinPath `
        -Action Allow `
        -Profile Any | Out-Null
    Write-Host "Firewall rule created." -ForegroundColor Cyan
    
    # Create Task Scheduler task
    Write-Host "Creating Task Scheduler task..." -ForegroundColor Cyan
    
    # Remove old task if exists
    Unregister-ScheduledTask -TaskName $TaskName -Confirm:$false -ErrorAction SilentlyContinue
    
    # Create new task
    $action = New-ScheduledTaskAction -Execute $StartScript
    $trigger = New-ScheduledTaskTrigger -AtLogOn
    $principal = New-ScheduledTaskPrincipal -UserId $env:USERNAME `
        -LogonType Interactive `
        -RunLevel Highest
    $settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries `
        -DontStopIfGoingOnBatteries `
        -StartWhenAvailable `
        -RestartCount 3 `
        -RestartInterval (New-TimeSpan -Minutes 1)
    
    Register-ScheduledTask -TaskName $TaskName `
        -Action $action `
        -Trigger $trigger `
        -Principal $principal `
        -Settings $settings `
        -Description "Classroom Control Client - Runs on user login" | Out-Null
    
    Write-Host "Task Scheduler task created successfully." -ForegroundColor Green
    
    # Ask to start now
    Write-Host "`nInstallation completed!" -ForegroundColor Green
    Write-Host "Installation Path: $TargetDir" -ForegroundColor Cyan
    Write-Host "Config File: $ConfigPath" -ForegroundColor Cyan
    Write-Host "Task Name: $TaskName" -ForegroundColor Cyan
    
    $startNow = Read-Host "`nStart the client now? (Y/N)"
    if ($startNow -eq "Y" -or $startNow -eq "y") {
        Write-Host "Starting client..." -ForegroundColor Cyan
        Start-ScheduledTask -TaskName $TaskName
        Start-Sleep -Seconds 2
        
        # Check if running
        $process = Get-Process -Name "cms_client" -ErrorAction SilentlyContinue
        if ($process) {
            Write-Host "Client started successfully! (PID: $($process.Id))" -ForegroundColor Green
        }
        else {
            Write-Warning "Client may not have started. Check Task Scheduler and logs."
        }
    }
    
    Write-Host "`nThe client will start automatically on next login." -ForegroundColor Cyan
    Write-Host "To manage: Open Task Scheduler and look for '$TaskName'" -ForegroundColor Gray
    
}
catch {
    Write-Error "Installation failed: $_"
    exit 1
}
