# Watcher - Sistema di Gestione Classe

[![Licenza](https://img.shields.io/badge/licenza-MIT-blue.svg)](LICENSE)
[![Piattaforma](https://img.shields.io/badge/piattaforma-Windows-lightgrey.svg)]()
[![C++](https://img.shields.io/badge/C++-17-blue.svg)]()
[![Qt](https://img.shields.io/badge/Qt-6-green.svg)]()

**[🇬🇧 English Version](README.md)**

Un sistema completo di gestione classe (CMS) per ambienti educativi, che consente agli insegnanti di monitorare e controllare i computer degli studenti da un'applicazione master centrale.

## 🎯 Funzionalità

### 📸 Monitoraggio in Tempo Reale
- **Cattura Screenshot Live**: Cattura screenshot a risoluzione completa degli schermi degli studenti con rendering DPI-aware
- **Aggiornamento Automatico Thumbnail**: Visualizza anteprime thumbnail in tempo reale di tutti i client connessi
- **Supporto Multi-Monitor**: Cattura correttamente l'intero schermo su display multipli
- **Esportazione Alta Qualità**: Salva screenshot in `Pictures/Screenshots` in formato PNG con timestamp

### 🔒 Controllo Schermo
- **Blocco Input**: Blocca sia tastiera che mouse sui computer degli studenti
- **Sblocco Selettivo**: Controlla indipendentemente gli stati di blocco tastiera e mouse
- **Controllo Livello Admin**: Richiede privilegi appropriati per massima sicurezza

### 🌐 Filtraggio Rete
- **Blocco Domini**: Blocca l'accesso a siti web specifici tramite modifica del file hosts
- **Lista Permessi Domini**: Modalità solo whitelist per controllo internet rigoroso
- **Gestione Regole Dinamica**: Aggiungi/rimuovi domini senza riavvio del sistema
- **Flush DNS Automatico**: Le modifiche hanno effetto immediato

### 🚫 Controllo Applicazioni
- **Blocco Processi**: Previeni l'esecuzione di applicazioni specifiche
- **Persistenza Regole**: Le regole applicazione vengono salvate tra le sessioni
- **Monitoraggio Tempo Reale**: Scansiona continuamente e blocca applicazioni proibite
- **Filtraggio Flessibile**: Supporto per modalità blacklist e whitelist

### ⚡ Gestione Alimentazione
- **Spegnimento Remoto**: Spegni i computer degli studenti da remoto
- **Riavvio Remoto**: Riavvia i computer client
- **Supporto Ibernazione**: Metti le macchine in stato a basso consumo
- **Stato Batteria**: Monitora livello batteria e stato di ricarica (laptop)

### 🖥️ Informazioni Client
- **Dettagli Sistema**: Visualizza CPU, RAM e risoluzione schermo
- **Informazioni Rete**: Mostra indirizzo IP (IPv4/IPv6)
- **Visualizzazione Hostname**: Mostra nome computer e username
- **Stato Connessione**: Monitoraggio in tempo reale della connettività client

## 🏗️ Architettura

### Applicazione Master
L'applicazione GUI per l'insegnante costruita con Qt6:
- **Dashboard**: Vista griglia di tutti i client connessi
- **Visualizzatore Screenshot**: Visualizzatore immagini a schermo intero con capacità zoom
- **Gestione Policy**: Configura filtraggio domini e applicazioni
- **Comandi Broadcast**: Invia comandi a tutti o ai client selezionati

### Servizio Client
Servizio in background eseguito sui computer degli studenti:
- **Leggero**: Uso minimo delle risorse
- **Riconnessione Automatica**: Si riconnette automaticamente al master se la connessione viene persa
- **Comunicazione Sicura**: Protocollo basato su JSON su TCP
- **Specifico Piattaforma**: Integrazione ottimizzata delle API Windows

### Protocollo Comunicazione
- **Basato su TCP**: Consegna messaggi affidabile
- **Messaggi JSON**: Formato comandi leggibile
- **Delimitato da Newline**: Parsing messaggi efficiente
- **Codifica Base64**: Trasmissione dati screenshot

## 📋 Requisiti

### Applicazione Master
- Windows 10/11
- **Qt 6.5 o superiore** (richiesto per la GUI)
- CMake 3.15+
- MSVC 2019 o più recente
- Visual Studio 2019/2022 con workload Sviluppo Desktop C++

### Servizio Client
- Windows 10/11
- CMake 3.15+
- MSVC 2019 o più recente
- Privilegi amministratore (per blocco input e filtraggio domini)
- Connettività di rete al master

## 🔧 Installazione Qt

### Installare Qt 6

1. **Scarica Qt Online Installer**
   - Visita il [Sito Ufficiale Qt](https://www.qt.io/download-qt-installer)
   - Scarica il Qt Online Installer per Windows

2. **Installa Qt 6.5+ con i Componenti Richiesti**
   ```
   Durante l'installazione, seleziona:
   ✓ Qt 6.5 (o più recente)
   ✓ MSVC 2019 64-bit (o MSVC 2022 64-bit)
   ✓ Qt 5 Compatibility Module
   ✓ Librerie Aggiuntive (se richiesto)
   ```

3. **Annota il Percorso di Installazione**
   - Predefinito: `C:\Qt\6.10.1\msvc2022_64`
   - Avrai bisogno di questo percorso per la configurazione CMake

### Impostare il Percorso Qt

Aggiungi Qt al PATH di sistema o usa il prefisso CMake:

**Opzione 1: Imposta CMAKE_PREFIX_PATH**
```bash
set CMAKE_PREFIX_PATH=C:\Qt\6.10.1\msvc2022_64
```

**Opzione 2: Usa il Percorso CMake di Qt**
```bash
set Qt6_DIR=C:\Qt\6.10.1\msvc2022_64\lib\cmake\Qt6
```

## 🚀 Avvio Rapido

### Compilazione da Sorgente

```bash
# Clona il repository
git clone https://github.com/yourusername/Watcher.git
cd Watcher

# Crea directory build
mkdir build
cd build

# Configura con CMake (specifica il percorso Qt)
cmake .. -DCMAKE_PREFIX_PATH=C:\Qt\6.10.1\msvc2022_64

# Alternativa: specifica Qt6_DIR
cmake .. -DQt6_DIR=C:\Qt\6.10.1\msvc2022_64\lib\cmake\Qt6

# Compila applicazione master (con GUI Qt)
cmake --build . --target cms_master --config Release

# Compila servizio client (nessuna dipendenza Qt)
cmake --build . --target cms_client --config Release
```

### Compilazione con Visual Studio

```bash
# Genera soluzione Visual Studio
cmake -B build -G "Visual Studio 16 2019" -A x64 ^
  -DCMAKE_PREFIX_PATH=C:\Qt\6.10.1\msvc2022_64

# Apri in Visual Studio
start build\Watcher.sln

# Oppure compila da riga di comando
cmake --build build --config Release
```

### Configurazione

#### Setup Master
1. Avvia `cms_master.exe`
2. Il server si avvierà automaticamente sulla porta `5555`
3. I client si connetteranno automaticamente

#### Setup Client
1. Modifica `client_config.json`:
```json
{
  "master_address": "192.168.1.100",
  "master_port": 5555,
  "machine_id": "student-pc-01",
  "encryption_enabled": false,
  "log_level": "INFO"
}
```

2. Esegui `cms_client.exe` (richiede privilegi amministratore)

## 🔧 Utilizzo

### Cattura Screenshot
1. Seleziona un client dalla griglia (o usa selezione automatica se c'è solo un client)
2. Clicca il pulsante **📸 Screenshot** nella toolbar
3. Visualizza lo screenshot nel dialog popup
4. Usa i controlli zoom o salva su disco

### Blocco/Sblocco Schermi
- **Blocca Tutti**: Pulsante toolbar per bloccare tutti i client connessi
- **Sblocca Tutti**: Pulsante toolbar per sbloccare tutti i client
- **Blocco Individuale**: Click destro sul thumbnail client per controllo per-client

### Filtraggio Domini
1. Clicca **Policy** → **Filtro Domini**
2. Scegli modalità: **Blacklist** o **Whitelist**
3. Aggiungi domini da bloccare/permettere
4. Clicca **Applica** per inviare regole a tutti i client

### Controllo Applicazioni
1. Clicca **Policy** → **Filtro Applicazioni**
2. Seleziona modalità filtro
3. Aggiungi percorsi applicazione o nomi processi
4. Le regole vengono applicate immediatamente

## 🛠️ Dettagli Tecnici

### Cattura Screenshot
- Usa `GetDeviceCaps(DESKTOPHORZRES/DESKTOPVERTRES)` per risoluzione DPI-aware
- Cattura dati pixel RGBA grezzi (4 byte per pixel)
- Rilevamento automatico risoluzione dalla dimensione dati
- Supporto per risoluzioni comuni: 1920x1080, 1600x900, 1366x768, 1280x720

### Protocollo di Rete
I messaggi sono oggetti JSON con delimitatori newline:

```json
{
  "type": "SCREENSHOT_REQUEST",
  "source": "master",
  "destination": "client-01",
  "timestamp": 1234567890,
  "payload": {}
}
```

### Tipi di Comandi
- `HELLO` - Handshake client
- `PING` - Heartbeat
- `SCREENSHOT_REQUEST` - Richiesta screenshot
- `SCREENSHOT_DATA` - Risposta screenshot
- `SCREEN_LOCK` / `SCREEN_UNLOCK` - Controllo input
- `DOMAIN_BLOCK` / `DOMAIN_ALLOW` - Filtraggio rete
- `APP_BLOCK` / `APP_ALLOW` - Controllo applicazioni
- `POWER_CONTROL` - Spegnimento/riavvio/ibernazione

## 🐛 Problemi Noti

- Il blocco input richiede privilegi amministratore
- Il filtraggio domini modifica il file hosts di sistema (richiede admin)
- Il supporto multi-monitor cattura solo lo schermo primario in alcuni casi

## 🤝 Contribuire

I contributi sono benvenuti! Sentiti libero di inviare pull request o aprire issue per bug e richieste di funzionalità.

## 📄 Licenza

Questo progetto è sotto licenza MIT - vedi il file [LICENSE](LICENSE) per i dettagli.

## 🙏 Ringraziamenti

- Costruito con Qt 6 Framework
- Usa nlohmann/json per parsing JSON
- Windows API per funzionalità specifiche della piattaforma

## 📞 Supporto

Per problemi e domande:
- Crea un issue su GitHub
- Controlla gli issue esistenti per soluzioni
- Rivedi la documentazione

---

**Nota**: Questo software è destinato ad ambienti educativi. Assicurati di avere l'autorizzazione appropriata prima di distribuire su qualsiasi rete.
