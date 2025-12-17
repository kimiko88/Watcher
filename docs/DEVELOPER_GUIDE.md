# Guida per Sviluppatori (Developer Guide)

Benvenuto nella documentazione tecnica di **Classroom Control**. Questa guida è destinata agli sviluppatori che desiderano contribuire al progetto, estenderne le funzionalità o comprendere l'architettura interna.

## 1. Panoramica dell'Architettura (Architecture Overview)

Classroom Control segue un'architettura **Client-Server** classica, ottimizzata per reti locali (LAN).

### Diagramma di Sistema

```mermaid
graph TD
    M[Master Application (Qt GUI)] <-->|TCP/5555| C1[Client Service 1]
    M <-->|TCP/5555| C2[Client Service 2]
    M <-->|TCP/5555| C3[Client Service 3]
    
    subgraph "Client Node"
        C1 -->|IInputControl| OS[OS API (Win32/X11/Cocoa)]
        C1 -->|ScreenCapture| GDI[Graphics API]
    end
```

### Componenti Principali
1.  **Core (`src/core`)**: Libreria condivisa contenente la logica di business comune.
    *   **Protocol**: Serializzazione/Deserializzazione messaggi JSON.
    *   **Logger**: Sistema di logging thread-safe.
    *   **Socket**: Wrapper cross-platform per socket TCP.
2.  **Client (`src/client`)**: Servizio background che gira sui computer degli studenti.
    *   Gestisce la connessione persistente col Master.
    *   Esegue comandi privilegiati (blocco input, spegnimento).
3.  **Master (`src/master`)**: Applicazione GUI (Qt6) per l'insegnante.
    *   Gestisce la lista client e le connessioni socket multiple.
    *   Visualizza le miniature in tempo reale.
4.  **Platform (`src/platform`)**: Abstraction Layer per le chiamate specifiche del sistema operativo.

### Thread Model
*   **Client**:
    *   `Main Thread`: Loop di elaborazione comandi e keep-alive.
    *   `Network Thread` (impl): Gestione I/O socket bloccante.
*   **Master**:
    *   `UI Thread (Main)`: Event loop Qt, rendering interfaccia.
    *   `Server Thread`: `QTcpServer` gestisce le connessioni in arrivo. Ogni client ha un buffer di lettura non bloccante.

## 2. Compilazione e Sviluppo (Build & Development)

### Prerequisiti
*   **CMake**: 3.14 o superiore.
*   **Compilatore C++17**: MSVC 2019+, GCC 9+, Clang 10+.
*   **Qt6**: Richiesto per il modulo Master (Componenti: Core, Gui, Widgets, Network).
*   **Git**: Per il versionamento.

### Compilazione da Sorgente

**Windows (PowerShell):**
```powershell
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2019_64"
cmake --build . --config Debug
```

**Linux/macOS (Bash):**
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Debug vs Release
*   **Debug**: Simboli attivi, ottimizzazioni disabilitate. Macro `DEBUG` definita.
*   **Release**: Ottimizzazioni O2/O3, simboli rimossi. Macro `NDEBUG` definita.
    *   Flag CMake: `-DCMAKE_BUILD_TYPE=Release`

### Platform-Specific Flags
Il sistema rileva automaticamente la piattaforma e definisce le macro:
*   `CMS_PLATFORM_WINDOWS`
*   `CMS_PLATFORM_LINUX`
*   `CMS_PLATFORM_MACOS`

## 3. Struttura del Codice (Code Structure)

```text
ClassroomControl/
├── docs/               # Documentazione
├── include/            # Header files pubblici
│   └── cms/            # Namespace cms
├── scripts/            # Script di installazione/deploy
├── src/
│   ├── client/         # Sorgenti Client Service
│   ├── core/           # Sorgenti Libreria Core
│   ├── master/         # Sorgenti GUI Master
│   └── platform/       # Implementazioni specifiche OS
└── tests/              # Unit e Integration tests
```

### Convenzioni di Naming
*   **Classi**: PascalCase (es. `ClientService`, `InputLockManager`).
*   **Metodi/Variabili**: camelCase (es. `startService`, `clientCount`).
*   **Membri privati**: camelCase con underscore finale (es. `socket_`, `isRunning_`).
*   **Costanti/Macro**: UPPER_SNAKE_CASE (es. `MAX_BUFFER_SIZE`).

## 4. Aggiungere Nuove Funzionalità (Adding New Features)

Per aggiungere un nuovo comando (es. "Invia File"):

1.  **Estendere il Protocollo**:
    *   Aggiungere `SEND_FILE` in `include/cms/Protocol.h` (`CommandType`).
    *   Documentare il payload JSON atteso.

2.  **Implementazione Client**:
    *   Aggiornare `ClientService::processCommands` per gestire il nuovo case.
    *   Implementare la logica (es. scrittura file su disco).

3.  **Implementazione Master**:
    *   Aggiungere il pulsante nella GUI.
    *   Creare il messaggio JSON e inviarlo tramite `MasterServer::broadcast` o `sendToClient`.

4.  **Testing**:
    *   Aggiungere un test unitario per la serializzazione.
    *   Aggiungere un test di integrazione in `tests/integration/EndToEndTest.cpp`.

## 5. Debugging

### Logging
Il sistema usa una classe `Logger` thread-safe.
*   Abilitare log verbosi nel file `config.json` (`log_level`: "DEBUG").
*   I log vengono scritti su stdout e (opzionalmente) su file in `/var/log` o `ProgramData`.

### Packet Inspection
Poiché il protocollo è JSON su TCP, è possibile ispezionare il traffico usando **Wireshark**:
*   Filtro: `tcp.port == 5555`
*   Analizzare il payload ASCII per verificare la correttezza del JSON.

### Profiling
*   **Windows**: Usare Visual Studio Performance Profiler per analizzare l'uso CPU durante lo streaming schermo.
*   **Linux**: Usare `valgrind` per rilevare memory leaks.

## 6. Contributing

Siamo aperti a contributi!

### Code Style Guide
Adottiamo lo stile **Google C++ Style Guide** con lievi modifiche (indentazione 4 spazi).
*   Usare `clang-format` prima di committare.

### Commit Message Format
Usare il formato [Conventional Commits](https://www.conventionalcommits.org/):
*   `feat: aggiunta gestione mouse remoto`
*   `fix(net): risolto crash su disconnessione`
*   `docs: aggiornato README`

### Pull Request Process
1.  Forkare il repository.
2.  Creare un branch feature (`git checkout -b feature/nuova-funzione`).
3.  Committare e pushare.
4.  Aprire una PR verso `main`.
5.  **Checklist**:
    - [ ] Il codice compila su tutte le piattaforme target?
    - [ ] I test unitari passano?
    - [ ] Nessun warning del compilatore aggiunto?

## 7. Limitazioni Note (Known Limitations)

1.  **Larghezza di Banda**: Lo streaming desktop ("Show Screen" o "Remote View") consuma ~2-5 Mbps per client. Su reti 100Mbps, limitare a max 10-15 client contemporanei in visualizzazione remota.
2.  **Latenza**: Non adatto per applicazioni real-time critiche (gaming, editing video remoto). Latenza tipica: 100-300ms.
3.  **Client Limit**: Testato stabilmente fino a 30 client. Oltre, l'interfaccia Master potrebbe rallentare (necessaria ottimizzazione del rendering delle miniature).
4.  **Wayland (Linux)**: La cattura schermo su Wayland è sperimentale; si consiglia X11.

## 8. Roadmap Futura

*   [ ] **Compressione ZSTD**: Sostituire l'attuale codifica base per ridurre l'uso di banda.
*   [ ] **Audio Streaming**: Inviare l'audio del Master ai Client.
*   [ ] **Web Console**: Interfaccia Master basata su browser (React/Vue) per controllo da tablet.
*   [ ] **Plugin System**: API Lua/Python per script di automazione personalizzati.
