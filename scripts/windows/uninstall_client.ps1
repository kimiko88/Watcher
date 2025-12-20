<#
.SYNOPSIS
    Uninstalls the Classroom Control Client Service.
.DESCRIPTION
    Removes the Windows Service, files, and firewall rules.
.NOTES
    Requires Administrator privileges.
#>

$ErrorActionPreference = "Stop"

# Verify Admin Privileges
if (!([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Error "This script must be run as Administrator."
    exit 1
}

$ServiceName = "ClassroomControlClient"
$DisplayName = "Classroom Control Client"
$InstallDir = "C:\Program Files\ClassroomControl"

try {
    Write-Host "Uninstalling Classroom Control Client..." -ForegroundColor Yellow
    
    # Stop and remove service
    $Service = Get-Service -Name $ServiceName -ErrorAction SilentlyContinue
    if ($Service) {
        Write-Host "Stopping service..." -ForegroundColor Cyan
        Stop-Service -Name $ServiceName -Force -ErrorAction SilentlyContinue
        Start-Sleep -Seconds 2
        
        Write-Host "Removing service..." -ForegroundColor Cyan
        sc.exe delete $ServiceName
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Service removed successfully." -ForegroundColor Green
        }
        else {
            Write-Warning "Failed to remove service (may not exist)."
        }
    }
    else {
        Write-Host "Service not found (already removed or never installed)." -ForegroundColor Gray
    }
    
    # Remove firewall rule
    Write-Host "Removing firewall rule..." -ForegroundColor Cyan
    Remove-NetFirewallRule -DisplayName $DisplayName -ErrorAction SilentlyContinue
    Write-Host "Firewall rule removed." -ForegroundColor Green
    
    # Ask about removing files
    $removeFiles = Read-Host "Do you want to remove installed files from '$InstallDir'? (Y/N)"
    if ($removeFiles -eq "Y" -or $removeFiles -eq "y") {
        if (Test-Path $InstallDir) {
            Write-Host "Removing installation directory..." -ForegroundColor Cyan
            Remove-Item $InstallDir -Recurse -Force
            Write-Host "Files removed." -ForegroundColor Green
        }
        else {
            Write-Host "Installation directory not found." -ForegroundColor Gray
        }
    }
    
    Write-Host "`nUninstallation completed successfully!" -ForegroundColor Green
    Write-Host "The client has been removed from this system." -ForegroundColor Cyan
    
}
catch {
    Write-Error "Uninstallation failed: $_"
    exit 1
}
