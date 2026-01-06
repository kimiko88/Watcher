# Guida alla Build e al Deploy di Watcher

Questa guida descrive i passaggi necessari per compilare (build) e installare (deploy) i componenti **Master** e **Client** del sistema Watcher.

## 📋 Prerequisiti

Prima di iniziare, assicurati di avere installato:

1.  **Visual Studio 2022** (con workload "Sviluppo di applicazioni desktop con C++").
2.  **CMake 3.15+** (incluso in Visual Studio o installabile a parte).
3.  **Qt 6.5 o superiore** (Richiesto per la GUI del Master).
    -   Installare tramite Qt Online Installer.
    -   Selezionare il kit **MSVC 2019 64-bit** (o compatibile es. MSVC 2022).
    -   Selezionare "Qt 5 Compatibility Module".

> **Nota**: Il Client può essere compilato anche senza Qt, ma il Master richiede le librerie Qt per l'interfaccia grafica.

---

## 🚀 Build Automatica (Consigliata)

Il progetto include uno script che automatizza l'intero processo di compilazione.

1.  Apri un prompt dei comandi (o PowerShell).
2.  Esegui lo script `build_all.bat` dalla root del progetto:

```cmd
build_all.bat
```

Lo script eseguirà le seguenti operazioni:
-   Pulisce la cartella `build/` (se esistente).
-   Cerca automaticamente l'installazione di Qt.
-   Configura il progetto con CMake per Visual Studio 2022 (x64).
-   Compila tutti i componenti:
    -   `cms_core` (Libreria base)
    -   `cms_platform` (Libreria specifica OS)
    -   `cms_client_service` (Servizio Windows Client)
    -   `cms_client_worker` (Processo utente Client)
    -   `cms_master_service` (Servizio backend Master)
    -   `cms_master` (Interfaccia GUI Master - solo se Qt è trovato)

Al termine, troverai gli eseguibili nella cartella `build/Release/`.

---

## 🛠️ Build Manuale

Se preferisci controllare ogni passaggio o se lo script automatico fallisce, procedi come segue:

### 1. Configurazione

```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.10.1\msvc2022_64"
```

*Sostituisci `C:\Qt\6.10.1\msvc2022_64` con il percorso reale della tua installazione Qt.*

### 2. Compilazione

Compila i singoli target in modalità Release:

**Per il Client:**
```cmd
cmake --build . --target cms_client_service --config Release
cmake --build . --target cms_client_worker --config Release
```

**Per il Master:**
```cmd
cmake --build . --target cms_master_service --config Release
cmake --build . --target cms_master --config Release
```

---

## 📦 Deploy e Installazione

Il sistema utilizza degli script `.bat` situati nella cartella `scripts/` per installare i servizi su Windows.

### Installazione Client (Sui PC Studenti)

1.  Copia i seguenti file dalla cartella `build/Release/` del tuo PC di sviluppo alla cartella `scripts/` (o assicurati che gli script li trovino):
    > **Nota**: Lo script di installazione `install_client_service.bat` si aspetta di trovare gli eseguibili nella **stessa cartella** da cui viene lanciato.
    
    Copia in una cartella di destinazione sul PC studente:
    -   `install_client_service.bat`
    -   `cms_client_service.exe`
    -   `cms_client_worker.exe`
    -   `client_config.json` (Da creare/configurare prima, vedi sotto)
    -   `cms_core.dll` (se necessario)
    
2.  Apri un prompt dei comandi **come Amministratore** sul PC studente.
3.  Posizionati nella cartella.
4.  Esegui l'installazione:

```cmd
install_client_service.bat
```

> **Importante**: Assicurati che il file `client_config.json` sia configurato correttamente (vedi sezione Configurazione) e posizionato dove il servizio se lo aspetta (solitamente nella cartella dell'eseguibile o in `../config`).

### Installazione Master (Sul PC Docente)

1.  Assicurati che gli eseguibili `cms_master.exe` e `cms_master_service.exe` siano presenti.
2.  Per il deploy della GUI Qt (`cms_master.exe`), è necessario includere le DLL di Qt. Puoi usare il tool `windeployqt` per copiare le dipendenze nella stessa cartella dell'eseguibile:
    ```cmd
    C:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe --release build\Release\cms_master.exe
    ```
3.  Ora puoi avviare `cms_master.exe`.

---

## ⚙️ Configurazione Rapida

### Client (`config/client_config.json`)
Modifica questo file e posizionalo insieme agli eseguibili o nella cartella `config` relativa.

```json
{
  "master_address": "192.168.1.X",  <-- Inserisci IP del Master
  "master_port": 5555,
  "machine_id": "PC-AULA-01",
  "log_level": "INFO"
}
```

### Master
Il Master ascolta di default sulla porta 5555. Assicurati che il Firewall di Windows permetta il traffico in entrata su questa porta TCP/5555.
