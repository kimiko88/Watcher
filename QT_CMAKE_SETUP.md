# Come Impostare Qt Path in CMake

## Metodo 1: CMAKE_PREFIX_PATH (Raccomandato)

```batch
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_PREFIX_PATH="C:\Qt\6.10.1\msvc2022_64"
```

Sostituisci il path con la tua installazione Qt. Path comuni:
- `C:\Qt\6.10.1\msvc2022_64`
- `C:\Qt\6.6.0\msvc2019_64`  
- `C:\Qt6\6.5.3\msvc2019_64`

## Metodo 2: Variabile Ambiente

```batch
set CMAKE_PREFIX_PATH=C:\Qt\6.10.1\msvc2022_64
cmake -B build -G "Visual Studio 17 2022" -A x64
```

## Metodo 3: Qt6_DIR Diretto

```batch
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
  -DQt6_DIR="C:\Qt\6.10.1\msvc2022_64\lib\cmake\Qt6"
```

## Trova il tuo Qt Path

```batch
# Cerca Qt installation
dir C:\Qt /s /b | findstr "msvc"

# O
dir C:\Qt6 /s /b | findstr "msvc"
```

## Nota

**Qt NON è necessario per i servizi client e master!** Solo per `cms_master` GUI.

I servizi (`cms_client_service`, `cms_client_worker`, `cms_master_service`) compilano senza Qt.
