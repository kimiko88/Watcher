# Guida Installazione Master - Windows

Questa guida fornisce istruzioni per installare e configurare l'applicazione Master di Classroom Control su un PC Windows.

## Indice

1. [Requisiti di Sistema](#requisiti-di-sistema)
2. [Preparazione del Pacchetto](#preparazione-del-pacchetto)
3. [Installazione](#installazione)
4. [Configurazione](#configurazione)
5. [Avvio del Master](#avvio-del-master)
6. [Configurazione Firewall](#configurazione-firewall)
7. [Troubleshooting](#troubleshooting)

---

## Requisiti di Sistema

- **Sistema Operativo**: Windows 10 o superiore (64-bit)
- **RAM**: Minimo 4 GB
- **Spazio Disco**: 500 MB liberi
- **Rete**: Connessione di rete attiva
- **Altri**: Non richiede installazione di Qt (le DLL sono incluse)

---

## Preparazione del Pacchetto

### Sul PC di Sviluppo (dove hai compilato il progetto)

1. **Compilare il Master in modalità Release**:

```powershell
cd C:\Users\chimi\Desktop\Programmazione\Watcher

# Compilare in Release per migliori performance
cmake --build build --config Release
```

2. **Eseguire lo script di packaging**:

```powershell
# Eseguire lo script per creare il pacchetto di deployment
.\scripts\windows\package_master.ps1 -BuildConfig Release

# Output: C:\Temp\CMSMaster_Deploy (e opzionalmente un file ZIP)
```

Lo script raccoglierà automaticamente:
- ✅ `cms_master.exe`
- ✅ Tutte le DLL Qt6 necessarie
- ✅ Plugin Qt (platforms, styles, ecc.)
- ✅ File di configurazione predefinito
- ✅ README con istruzioni

3. **Trasferire il pacchetto al PC di destinazione**:

Hai due opzioni:
- Copiare l'intera cartella `C:\Temp\CMSMaster_Deploy` su una chiavetta USB
- Usare il file ZIP creato (più comodo) e trasferirlo via rete/USB

> [!TIP]
> Il pacchetto ZIP è più facile da distribuire e occupa meno spazio.

---

## Installazione

### Sul PC di Destinazione (dove vuoi installare il Master)

1. **Estrarre il pacchetto**:

```powershell
# Se hai il ZIP
Expand-Archive -Path "CMSMaster_Deploy.zip" -DestinationPath "C:\Program Files\ClassroomControl\Master"

# Oppure copiare direttamente la cartella
Copy-Item -Path "D:\CMSMaster_Deploy\*" -Destination "C:\Program Files\ClassroomControl\Master" -Recurse
```

2. **Verificare i file installati**:

```powershell
Get-ChildItem "C:\Program Files\ClassroomControl\Master"

# Dovresti vedere:
# - cms_master.exe
# - config.json
# - varie DLL (Qt6Core.dll, Qt6Widgets.dll, ecc.)
# - cartelle: platforms/, styles/
# - README.txt
```

---

## Configurazione

### File di Configurazione: config.json

Il file `config.json` contiene i parametri di configurazione del Master:

```json
{
    "port": 5555,
    "max_clients": 50,
    "log_level": "INFO",
    "screenshots_dir": "screenshots",
    "enable_encryption": false
}
```

| Parametro | Descrizione | Valori Consigliati |
|-----------|-------------|-------------------|
| `port` | Porta TCP per connessioni client | `5555` (default) |
| `max_clients` | Numero massimo di client simultanei | `50` (laboratorio standard) |
| `log_level` | Livello di logging | `INFO` (produzione), `DEBUG` (troubleshooting) |
| `screenshots_dir` | Directory per salvare screenshot | `screenshots` o percorso assoluto |
| `enable_encryption` | Abilita crittografia (futuro) | `false` |

### Modificare la Configurazione

```powershell
# Aprire il file per modificarlo
notepad "C:\Program Files\ClassroomControl\Master\config.json"
```

> [!WARNING]
> Assicurati che il file JSON sia sintatticamente corretto dopo le modifiche.

---

## Avvio del Master

### Metodo 1: Esecuzione Diretta (Test/Debug)

```powershell
cd "C:\Program Files\ClassroomControl\Master"
.\cms_master.exe
```

La GUI del Master si aprirà mostrando:
- Elenco client connessi
- Pannello di controllo
- Invio comandi
- Visualizzazione screenshot

### Metodo 2: Creazione Collegamento Desktop

```powershell
# Creare collegamento sul desktop
$TargetPath = "C:\Program Files\ClassroomControl\Master\cms_master.exe"
$ShortcutPath = [Environment]::GetFolderPath("Desktop") + "\Classroom Control Master.lnk"
$WScriptShell = New-Object -ComObject WScript.Shell
$Shortcut = $WScriptShell.CreateShortcut($ShortcutPath)
$Shortcut.TargetPath = $TargetPath
$Shortcut.WorkingDirectory = "C:\Program Files\ClassroomControl\Master"
$Shortcut.Save()

Write-Host "Collegamento creato sul desktop!" -ForegroundColor Green
```

### Metodo 3: Avvio Automatico (Opzionale)

Per avviare automaticamente all'accesso di Windows:

```powershell
# Aggiungere alla cartella di Avvio
$StartupPath = [Environment]::GetFolderPath("Startup")
$ShortcutPath = Join-Path $StartupPath "Classroom Control Master.lnk"
$WScriptShell = New-Object -ComObject WScript.Shell
$Shortcut = $WScriptShell.CreateShortcut($ShortcutPath)
$Shortcut.TargetPath = "C:\Program Files\ClassroomControl\Master\cms_master.exe"
$Shortcut.WorkingDirectory = "C:\Program Files\ClassroomControl\Master"
$Shortcut.Save()
```

---

## Configurazione Firewall

Il Master deve accettare connessioni in entrata dai client. Configurare il firewall:

```powershell
# Creare regola firewall in entrata
New-NetFirewallRule `
    -DisplayName "Classroom Control Master" `
    -Direction Inbound `
    -Program "C:\Program Files\ClassroomControl\Master\cms_master.exe" `
    -Action Allow `
    -Profile Any `
    -Protocol TCP `
    -LocalPort 5555

Write-Host "Regola firewall creata!" -ForegroundColor Green
```

### Verificare la Regola

```powershell
Get-NetFirewallRule -DisplayName "Classroom Control Master" | Format-List *
```

### Test di Connettività

Da un altro PC sulla stessa rete:

```powershell
# Sostituire 192.168.1.100 con l'IP del PC Master
Test-NetConnection -ComputerName 192.168.1.100 -Port 5555

# Output atteso: TcpTestSucceeded : True
```

---

## Troubleshooting

### Problema: DLL Mancanti

**Sintomo**: Errore "Qt6Widgets.dll non trovato" o simili

**Causa**: Il pacchetto non è stato creato correttamente o è incompleto.

**Soluzione**:
1. Assicurati di aver usato `package_master.ps1` per creare il pacchetto
2. Verifica che tutte le DLL siano presenti nella cartella di installazione
3. Ri-esegui lo script di packaging sul PC di sviluppo

```powershell
# Verificare le DLL Qt presenti
Get-ChildItem "C:\Program Files\ClassroomControl\Master" -Filter "Qt6*.dll"

# Dovresti vedere almeno:
# Qt6Core.dll, Qt6Gui.dll, Qt6Widgets.dll, Qt6Network.dll
```

### Problema: Porta Già in Uso

**Sintomo**: "Address already in use" o porta 5555 non disponibile

**Soluzione**:

```powershell
# Trovare il processo che usa la porta 5555
netstat -ano | findstr :5555

# Terminare il processo (sostituire PID)
taskkill /F /PID <PID>

# Oppure cambiare porta in config.json
```

### Problema: Client Non Si Connettono

**Cause possibili**:
1. Firewall blocca la connessione
2. Master non è in ascolto
3. Client ha IP/porta errati

**Diagnostica**:

```powershell
# 1. Verificare che il Master sia in esecuzione
Get-Process cms_master -ErrorAction SilentlyContinue

# 2. Verificare la porta in ascolto
netstat -an | findstr :5555

# 3. Verificare IP del PC Master
ipconfig

# 4. Testare connessione dal client
Test-NetConnection -ComputerName <IP_MASTER> -Port 5555
```

### Problema: Visual C++ Runtime Mancante

**Sintomo**: Errore "VCRUNTIME140.dll non trovato"

**Soluzione**: Installare Visual C++ Redistributable:

1. Scaricare da: https://aka.ms/vs/17/release/vc_redist.x64.exe
2. Eseguire l'installer
3. Riavviare l'applicazione

> [!NOTE]
> Lo script `package_master.ps1` dovrebbe includere già queste DLL, ma in alcuni casi potrebbero servire i redistributable system-wide.

### Problema: Applicazione Non Si Avvia

**Diagnostica**:

```powershell
# Eseguire da riga di comando per vedere gli errori
cd "C:\Program Files\ClassroomControl\Master"
.\cms_master.exe --help

# Controllare Event Viewer
eventvwr.msc
# Navigare a: Windows Logs > Application
```

---

## Opzioni da Riga di Comando

```powershell
# Mostrare aiuto
.\cms_master.exe --help

# Avviare con porta personalizzata
.\cms_master.exe --port 6000

# Avviare con log debug
.\cms_master.exe --log-level DEBUG
```

---

## Best Practices

> [!TIP]
> **Consigli per l'Installazione in Ambiente Scolastico**

1. **PC Dedicato**: Usa un PC dedicato per il Master (non il PC del docente principale)
2. **IP Statico**: Assegna un IP statico al PC Master per evitare cambiamenti di indirizzo
3. **Performance**: Usa un PC con almeno 8 GB RAM se gestisci più di 30 client
4. **Backup**: Esegui backup periodici della cartella `screenshots` e della configurazione
5. **Aggiornamenti**: Testa nuove versioni su un PC di test prima del deployment in produzione

---

## Disinstallazione

```powershell
# 1. Chiudere l'applicazione
Stop-Process -Name "cms_master" -Force -ErrorAction SilentlyContinue

# 2. Rimuovere regola firewall
Remove-NetFirewallRule -DisplayName "Classroom Control Master" -ErrorAction SilentlyContinue

# 3. Eliminare i file
Remove-Item -Path "C:\Program Files\ClassroomControl\Master" -Recurse -Force

# 4. Rimuovere collegamenti (se creati)
Remove-Item -Path "$env:USERPROFILE\Desktop\Classroom Control Master.lnk" -ErrorAction SilentlyContinue
Remove-Item -Path "$env:APPDATA\Microsoft\Windows\Start Menu\Programs\Startup\Classroom Control Master.lnk" -ErrorAction SilentlyContinue

Write-Host "Disinstallazione completata!" -ForegroundColor Green
```

---

## Aggiornamento

Per aggiornare a una nuova versione:

1. **Backup della configurazione**:
   ```powershell
   Copy-Item "C:\Program Files\ClassroomControl\Master\config.json" -Destination "C:\Temp\config_backup.json"
   ```

2. **Chiudere l'applicazione corrente**

3. **Sostituire i file** (mantenere `config.json` e `screenshots/`)

4. **Ripristinare la configurazione personalizzata**

---

## Supporto

Per ulteriori informazioni, consultare:
- [README.md](../../README.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)
- [GUIDA_INSTALLAZIONE_CLIENT_WINDOWS.md](GUIDA_INSTALLAZIONE_CLIENT_WINDOWS.md)

---

**Fine della Guida**
