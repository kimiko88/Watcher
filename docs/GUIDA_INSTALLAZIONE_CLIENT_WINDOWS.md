# Guida Installazione Client - Windows

Questa guida fornisce istruzioni dettagliate per installare e configurare il servizio client di Classroom Control su un PC Windows.

## Indice

1. [Requisiti di Sistema](#requisiti-di-sistema)
2. [Prerequisiti](#prerequisiti)
3. [Metodo 1: Installazione Automatica (Consigliato)](#metodo-1-installazione-automatica-consigliato)
4. [Metodo 2: Installazione Manuale](#metodo-2-installazione-manuale)
5. [Configurazione](#configurazione)
6. [Verifica dell'Installazione](#verifica-dellinstallazione)
7. [Gestione del Servizio](#gestione-del-servizio)
8. [Troubleshooting](#troubleshooting)
9. [Disinstallazione](#disinstallazione)

---

## Requisiti di Sistema

- **Sistema Operativo**: Windows 10 o superiore (anche Windows Server 2016+)
- **Architettura**: x64 (64-bit)
- **RAM**: Minimo 2 GB
- **Spazio Disco**: 100 MB liberi
- **Rete**: Connessione di rete attiva
- **Privilegi**: Accesso amministratore

## Prerequisiti

### 1. Compilare il Client

Prima di installare, è necessario compilare il progetto:

```powershell
# Dalla directory principale del progetto
cd C:\Users\chimi\Desktop\Programmazione\Watcher

# Configurare il progetto con CMake
cmake -B build -G "Visual Studio 16 2019"

# Compilare in modalità Release
cmake --build build --config Release
```

L'eseguibile verrà creato in: `build\src\client\Release\cms_client.exe`

### 2. Verificare i Prerequisiti di Rete

- Assicurarsi che il firewall Windows consenta connessioni in uscita
- Conoscere l'indirizzo IP del server master
- Verificare che la porta di comunicazione (default: 5555) non sia bloccata

---

## Metodo 1: Installazione Automatica (Consigliato)

### Passo 1: Preparare i File di Deployment

Creare una cartella temporanea per il deployment:

```powershell
# Creare cartella di deployment
New-Item -ItemType Directory -Path C:\Temp\CMSClient -Force

# Copiare l'eseguibile compilato
Copy-Item "build\src\client\Release\cms_client.exe" -Destination "C:\Temp\CMSClient\"

# Copiare lo script di installazione
Copy-Item "scripts\windows\install_client.ps1" -Destination "C:\Temp\CMSClient\"
```

### Passo 2: Creare il File di Configurazione

Creare un file `config.json` nella cartella `C:\Temp\CMSClient\`:

```json
{
    "master_address": "192.168.1.100",
    "master_port": 5555,
    "machine_id": "client-lab01-pc15",
    "encryption_enabled": false,
    "log_level": "INFO"
}
```

> [!IMPORTANT]
> Sostituire `192.168.1.100` con l'indirizzo IP effettivo del server master.
> Assegnare un `machine_id` univoco per ogni client (es: `client-lab01-pc01`, `client-lab01-pc02`, ecc.)

### Passo 3: Eseguire lo Script di Installazione

Aprire PowerShell come **Amministratore**:

```powershell
# Navigare nella cartella di deployment
cd C:\Temp\CMSClient

# Abilitare l'esecuzione di script (se necessario)
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope Process

# Eseguire lo script di installazione
.\install_client.ps1
```

Lo script eseguirà automaticamente:
1. ✅ Creazione della directory `C:\Program Files\ClassroomControl`
2. ✅ Copia dell'eseguibile e del file di configurazione
3. ✅ Registrazione del servizio Windows
4. ✅ Configurazione delle regole firewall
5. ✅ Avvio del servizio

### Passo 4: Verificare l'Installazione

```powershell
# Verificare lo stato del servizio
Get-Service -Name "ClassroomControlClient"

# Output atteso:
# Status   Name                           DisplayName
# ------   ----                           -----------
# Running  ClassroomControlClient         Classroom Control Client
```

---

## Metodo 2: Installazione Manuale

Se preferisci installare manualmente o lo script automatico non funziona, segui questi passaggi:

### Passo 1: Creare la Directory di Installazione

```powershell
# Creare la cartella del programma
New-Item -ItemType Directory -Path "C:\Program Files\ClassroomControl" -Force
```

### Passo 2: Copiare i File

```powershell
# Copiare l'eseguibile
Copy-Item "build\src\client\Release\cms_client.exe" -Destination "C:\Program Files\ClassroomControl\"

# Creare il file di configurazione
$config = @{
    master_address = "192.168.1.100"
    master_port = 5555
    machine_id = "client-" + $env:COMPUTERNAME
    encryption_enabled = $false
    log_level = "INFO"
} | ConvertTo-Json

$config | Set-Content "C:\Program Files\ClassroomControl\config.json"
```

### Passo 3: Registrare il Servizio Windows

```powershell
# Creare il servizio
sc.exe create ClassroomControlClient `
    binPath= "\"C:\Program Files\ClassroomControl\cms_client.exe\" --service" `
    start= auto `
    DisplayName= "Classroom Control Client" `
    depend= Tcpip/EventLog

# Output atteso:
# [SC] CreateService SUCCESS
```

> [!NOTE]
> L'opzione `--service` indica all'eseguibile di funzionare come servizio Windows.

### Passo 4: Configurare il Firewall

```powershell
# Aggiungere regola firewall in entrata
New-NetFirewallRule `
    -DisplayName "Classroom Control Client" `
    -Direction Inbound `
    -Program "C:\Program Files\ClassroomControl\cms_client.exe" `
    -Action Allow `
    -Profile Any
```

### Passo 5: Avviare il Servizio

```powershell
# Avviare il servizio
Start-Service -Name "ClassroomControlClient"

# Verificare lo stato
Get-Service -Name "ClassroomControlClient"
```

---

## Configurazione

### File di Configurazione: config.json

Il file `C:\Program Files\ClassroomControl\config.json` contiene i parametri di configurazione:

| Parametro | Descrizione | Valori | Esempio |
|-----------|-------------|--------|---------|
| `master_address` | Indirizzo IP del server master | Indirizzo IPv4 valido | `"192.168.1.100"` |
| `master_port` | Porta del server master | 1-65535 | `5555` |
| `machine_id` | Identificatore univoco del client | Stringa alfanumerica | `"client-lab01-pc15"` |
| `encryption_enabled` | Abilita crittografia (futuro) | `true` / `false` | `false` |
| `log_level` | Livello di logging | `DEBUG`, `INFO`, `WARNING`, `ERROR` | `"INFO"` |

### Esempio di Configurazione per Lab Scolastico

```json
{
    "master_address": "10.0.1.50",
    "master_port": 5555,
    "machine_id": "lab-informatica-pc12",
    "encryption_enabled": false,
    "log_level": "INFO"
}
```

### Modificare la Configurazione

> [!WARNING]
> Ogni modifica alla configurazione richiede il riavvio del servizio.

```powershell
# 1. Modificare il file config.json
notepad "C:\Program Files\ClassroomControl\config.json"

# 2. Riavviare il servizio
Restart-Service -Name "ClassroomControlClient"

# 3. Verificare che sia ripartito
Get-Service -Name "ClassroomControlClient"
```

---

## Verifica dell'Installazione

### 1. Verificare lo Stato del Servizio

```powershell
Get-Service -Name "ClassroomControlClient" | Format-List *

# Output atteso:
# Name                : ClassroomControlClient
# DisplayName         : Classroom Control Client
# Status              : Running
# StartType           : Automatic
```

### 2. Verificare i Log

I log vengono scritti nel **Visualizzatore Eventi** di Windows:

```powershell
# Aprire il Visualizzatore Eventi
eventvwr.msc

# Navigare a: Registri di Windows > Applicazione
# Cercare eventi con origine "ClassroomControl"
```

In alternativa, visualizzare gli eventi recenti tramite PowerShell:

```powershell
Get-EventLog -LogName Application -Source "ClassroomControl*" -Newest 10
```

### 3. Testare la Connessione al Master

Verificare che il client possa raggiungere il server master:

```powershell
# Test di connettività di rete
Test-NetConnection -ComputerName 192.168.1.100 -Port 5555

# Output atteso:
# TcpTestSucceeded : True
```

### 4. Verificare i File Installati

```powershell
# Elencare i file nella directory di installazione
Get-ChildItem "C:\Program Files\ClassroomControl"

# Output atteso:
# cms_client.exe
# config.json
```

---

## Gestione del Servizio

### Avviare il Servizio

```powershell
Start-Service -Name "ClassroomControlClient"
```

### Fermare il Servizio

```powershell
Stop-Service -Name "ClassroomControlClient"
```

### Riavviare il Servizio

```powershell
Restart-Service -Name "ClassroomControlClient"
```

### Cambiare Tipo di Avvio

```powershell
# Automatico (default)
Set-Service -Name "ClassroomControlClient" -StartupType Automatic

# Manuale
Set-Service -Name "ClassroomControlClient" -StartupType Manual

# Disabilitato
Set-Service -Name "ClassroomControlClient" -StartupType Disabled
```

### Visualizzare le Proprietà del Servizio

```powershell
Get-Service -Name "ClassroomControlClient" | Select-Object *
```

---

## Troubleshooting

### Il Servizio Non Si Avvia

**Sintomi:**
```
Il servizio ClassroomControlClient non può essere avviato
```

**Soluzioni:**

1. **Verificare il file di configurazione:**
   ```powershell
   # Controllare che il file esista
   Test-Path "C:\Program Files\ClassroomControl\config.json"
   
   # Verificare la sintassi JSON
   Get-Content "C:\Program Files\ClassroomControl\config.json" | ConvertFrom-Json
   ```

2. **Verificare i permessi:**
   ```powershell
   # Controllare i permessi della cartella
   icacls "C:\Program Files\ClassroomControl"
   ```

3. **Controllare i log degli errori:**
   ```powershell
   Get-EventLog -LogName Application -EntryType Error -Newest 5
   ```

### Errore "Connessione Rifiutata"

**Causa:** Il server master non è raggiungibile o non è in esecuzione.

**Soluzioni:**

1. Verificare che il server master sia attivo
2. Testare la connettività di rete:
   ```powershell
   ping 192.168.1.100
   Test-NetConnection -ComputerName 192.168.1.100 -Port 5555
   ```
3. Verificare le regole del firewall su entrambi client e server

### Il Servizio Si Ferma Improvvisamente

**Soluzioni:**

1. **Verificare i log per crash:**
   ```powershell
   Get-EventLog -LogName Application -Source "ClassroomControl*" -EntryType Error
   ```

2. **Ricompilare in Debug per diagnostica:**
   ```powershell
   cmake --build build --config Debug
   ```

3. **Eseguire manualmente per debug:**
   ```powershell
   # Fermare il servizio
   Stop-Service -Name "ClassroomControlClient"
   
   # Eseguire manualmente
   cd "C:\Program Files\ClassroomControl"
   .\cms_client.exe
   ```

### Firewall Blocca la Connessione

**Soluzioni:**

1. **Verificare le regole firewall esistenti:**
   ```powershell
   Get-NetFirewallRule -DisplayName "Classroom Control Client"
   ```

2. **Ricreare la regola:**
   ```powershell
   # Rimuovere regola esistente
   Remove-NetFirewallRule -DisplayName "Classroom Control Client"
   
   # Creare nuova regola
   New-NetFirewallRule `
       -DisplayName "Classroom Control Client" `
       -Direction Inbound `
       -Program "C:\Program Files\ClassroomControl\cms_client.exe" `
       -Action Allow `
       -Profile Any
   ```

3. **Verificare Windows Defender:**
   - Aprire Windows Security → Protezione da virus e minacce → Gestisci impostazioni
   - Aggiungere un'esclusione per `NNew`

### Errore "Port Already in Use"

**Causa:** Un altro processo sta usando la stessa porta.

**Soluzione:**
```powershell
# Trovare il processo che usa la porta
netstat -ano | findstr :5555

# Terminare il processo (sostituire PID)
taskkill /F /PID <PID>
```

---

## Disinstallazione

### Metodo Automatico (PowerShell)

```powershell
# Eseguire come Amministratore

# 1. Fermare il servizio
Stop-Service -Name "ClassroomControlClient" -Force -ErrorAction SilentlyContinue

# 2. Rimuovere il servizio
sc.exe delete ClassroomControlClient

# 3. Rimuovere la regola firewall
Remove-NetFirewallRule -DisplayName "Classroom Control Client" -ErrorAction SilentlyContinue

# 4. Eliminare i file
Remove-Item -Path "C:\Program Files\ClassroomControl" -Recurse -Force

Write-Host "Disinstallazione completata." -ForegroundColor Green
```

### Metodo Manuale

1. **Fermare il servizio:**
   - Aprire `services.msc`
   - Trovare "Classroom Control Client"
   - Click destro → Stop

2. **Rimuovere il servizio:**
   - Aprire PowerShell come Amministratore
   - Eseguire: `sc.exe delete ClassroomControlClient`

3. **Rimuovere regola firewall:**
   - Aprire `wf.msc` (Windows Firewall with Advanced Security)
   - Trovare "Classroom Control Client" nelle regole in entrata
   - Click destro → Elimina

4. **Eliminare i file:**
   - Cancellare la cartella `C:\Program Files\ClassroomControl`

---

## Deployment su Più Computer

### Usando Group Policy (Active Directory)

Per installare su più PC in un dominio:

1. **Creare uno script di startup:**
   - Salvare `install_client.ps1` in una condivisione di rete
   - Configurare Group Policy → Computer Configuration → Scripts → Startup
   - Aggiungere lo script

2. **Distribuzione via SCCM/Intune:**
   - Creare un package con `cms_client.exe` e `config.json`
   - Distribuire usando il client SCCM

### Script di Deployment Batch

Creare `deploy.bat` per installazione rapida:

```batch
@echo off
echo Installing Classroom Control Client...

:: Copiare file
xcopy /Y /I cms_client.exe "C:\Program Files\ClassroomControl\"
xcopy /Y /I config.json "C:\Program Files\ClassroomControl\"

:: Installare servizio
powershell -ExecutionPolicy Bypass -File install_client.ps1

echo Installation complete!
pause
```

---

## Best Practices

> [!TIP]
> **Consigli per l'Installazione in Ambienti Scolastici**

1. **Identificatori Univoci:**
   - Usare naming convention coerente: `lab-[nome]-pc[numero]`
   - Esempio: `lab-informatica-pc01`, `lab-informatica-pc02`

2. **Configurazione Centralizzata:**
   - Mantenere un template `config.json` centrale
   - Modificare solo `machine_id` per ogni client

3. **Monitoraggio:**
   - Configurare log aggregation per monitorare tutti i client
   - Usare `log_level: "INFO"` in produzione, `"DEBUG"` solo per troubleshooting

4. **Sicurezza:**
   - In futuro abilitare `encryption_enabled: true` quando disponibile
   - Isolare la rete dell'aula dai client non autorizzati

5. **Manutenzione:**
   - Pianificare riavvii periodici dei servizi
   - Testare aggiornamenti su un client prima del deployment globale

---

## Supporto e Risorse

### Documentazione Aggiuntiva

- [README.md](../README.md) - Panoramica del progetto
- [CLIENT_SERVICE.md](CLIENT_SERVICE.md) - Documentazione tecnica del servizio client
- [ARCHITECTURE.md](ARCHITECTURE.md) - Architettura del sistema

### Comandi Utili di Riferimento Rapido

```powershell
# Stato servizio
Get-Service -Name "ClassroomControlClient"

# Riavvio servizio
Restart-Service -Name "ClassroomControlClient"

# Log eventi
Get-EventLog -LogName Application -Source "ClassroomControl*" -Newest 10

# Test connettività
Test-NetConnection -ComputerName <MASTER_IP> -Port 5555

# Configurazione firewall
Get-NetFirewallRule -DisplayName "Classroom Control Client"
```

---

## Changelog

| Versione | Data | Modifiche |
|----------|------|-----------|
| 1.0.0 | 2025-12-19 | Prima versione della guida di installazione |

---

**Fine della Guida**

Per ulteriori informazioni o problemi non risolti, consultare la documentazione tecnica o contattare il supporto.
