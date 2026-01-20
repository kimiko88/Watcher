# Guida Completa: Build e Installazione Watcher

Questa guida illustra come compilare il progetto e installarlo su macchine Client (Studenti) e Master (Docente).

## 1. Build (Compilazione)

### Metodo Automatico (Consigliato)
Esegui lo script `build_all.bat` dalla cartella principale del progetto.
Questo script:
1. Pulisce vecchie build.
2. Trova automaticamente Qt.
3. Compila tutti i servizi e l'interfaccia grafica.

```cmd
build_all.bat
```

Gli eseguibili verranno generati in:
- Client: `build\src\client\Release\`
- Master: `build\src\master\Release\`

---

## 2. Installazione Client (PC Studente)

Il Client include due componenti: un **Servizio** (che gira come SYSTEM) e un **Worker** (che gira come Utente).

### Passaggio 1: Preparazione Cartella
Sul PC studente, crea una cartella (es. `C:\WatcherClient`) e copia al suo interno TUTTI i seguenti file (presi dal tuo PC di sviluppo):

1.  `cms_client_service.exe` (da `build\src\client\Release\`)
2.  `cms_client_worker.exe` (da `build\src\client\Release\`)
3.  `install_client_service.bat` (da `scripts\`)
4.  `client_config.json` (vedi sezione Configurazione)

**IMPORTANTE**: Tutti i file devono trovarsi nella *stessa cartella* per garantire che il servizio trovi l'eseguibile worker corretto.

### Passaggio 2: Installazione Servizio
1.  Apri un Prompt dei Comandi **come Amministratore**.
2.  Spostati nella cartella creata: `cd C:\WatcherClient`
3.  Esegui lo script:
    ```cmd
    install_client_service.bat
    ```

Se tutto è corretto, vedrai: *"Service created successfully"* e *"Installation completed successfully"*.

---

## 3. Installazione Master (PC Docente)

Il Master include il **Servizio di Backend** e l'**Interfaccia Grafica (GUI)**.

### Passaggio 1: Deploy delle Librerie Qt
Poiché la GUI usa Qt, non puoi semplicemente copiare l'eseguibile. Devi includere le DLL necessarie.
1.  Sul PC di sviluppo, vai nella cartella di build del master: `cd build\src\master\Release`
2.  Esegui il tool di deploy di Qt (aggiusta il percorso di Qt se necessario):
    ```cmd
    C:\Qt\6.10.1\msvc2022_64\bin\windeployqt.exe cms_master.exe
    ```
    Questo copierà tutte le DLL Qt necessarie accanto all'eseguibile.

### Passaggio 2: Preparazione Cartella
Sul PC Server/Docente, crea una cartella (es. `C:\WatcherMaster`) e copia:
1.  Tutto il contenuto di `build\src\master\Release\` (inclusi `cms_master.exe` e le DLL create sopra).
2.  `cms_master_service.exe` (da `build\src\master\Release\`)
3.  `install_master_service.bat` (da `scripts\`)

### Passaggio 3: Installazione
1.  Apri Prompt dei Comandi **come Amministratore**.
2.  Vai nella cartella: `cd C:\WatcherMaster`
3.  Esegui:
    ```cmd
    install_master_service.bat
    ```

---

## 4. Configurazione (`client_config.json`)

Crea questo file nella stessa cartella degli eseguibili Client:

```json
{
  "master_address": "192.168.1.100",  <-- IP del PC DOCENTE
  "master_port": 5555,
  "machine_id": "AULA-01-PC01",       <-- Nome univoco per questo PC
  "log_level": "INFO"
}
```

## 5. Troubleshooting (Risoluzione Problemi)

- **Errore 5: Accesso Negato**: Assicurati di eseguire gli script `.bat` come Amministratore.
- **Servizio non parte**: Controlla i log in `C:\Users\Public\cms_service_log.txt` (Client) o `cms_master_service_log.txt` (Server).
- **GUI non parte (Master)**: Probabilmente mancano le DLL di Qt. Assicurati di aver eseguito `windeployqt` o di aver copiato manualmente le DLL `Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll` etc.
- **Errore "File non trovato" durante installazione**: Assicurati che lo script `.bat` e gli `.exe` siano nella **stessa cartella**.
