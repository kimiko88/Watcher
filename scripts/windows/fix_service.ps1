<#
.SYNOPSIS
    Fixes the ClassroomControlClient service by updating its binary path to include the config parameter.
#>

$ErrorActionPreference = "Stop"

# Configuration
$ServiceName = "ClassroomControlClient"
$DisplayName = "Classroom Control Client"
$BinPath = "C:\Program Files\ClassroomControl\cms_client.exe"
$ConfigPath = "C:\Program Files\ClassroomControl\config.json"

try {
    Write-Host "Fixing ClassroomControlClient service..." -ForegroundColor Green
    
    # Stop service if running
    $Service = Get-Service -Name $ServiceName -ErrorAction SilentlyContinue
    if ($Service) {
        if ($Service.Status -eq "Running") {
            Write-Host "Stopping service..."
            Stop-Service -Name $ServiceName -Force
            Start-Sleep -Seconds 2
        }
        
        Write-Host "Deleting existing service..."
        sc.exe delete $ServiceName
        Start-Sleep -Seconds 2
    }
    
    # Create service with correct parameters
    Write-Host "Creating service with config parameter..."
    $binPathValue = "`"$BinPath`" --service --config `"$ConfigPath`""
    
    New-Service -Name $ServiceName `
        -BinaryPathName $binPathValue `
        -DisplayName $DisplayName `
        -StartupType Automatic `
        -DependsOn @("Tcpip", "EventLog")
    
    Write-Host "Service created successfully." -ForegroundColor Green
    
    # Start service
    Write-Host "Starting service..."
    Start-Service -Name $ServiceName
    
    Start-Sleep -Seconds 2
    
    # Verify
    $ServiceStatus = Get-Service -Name $ServiceName
    if ($ServiceStatus.Status -eq "Running") {
        Write-Host "SUCCESS! Service is running." -ForegroundColor Green
    }
    else {
        Write-Warning "Service status: $($ServiceStatus.Status)"
        Write-Host "Check Event Viewer or C:\Users\Public\cms_debug.txt for details." -ForegroundColor Yellow
    }
    
}
catch {
    Write-Error "Failed: $_"
    exit 1
}
