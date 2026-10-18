# Build-Prozess

Dieses Projekt baut die Modulbibliothek **lib3d** zu einer statischen
Bibliothek `lib/lib3d.a` und linkt daraus die Anwendungen in `apps/`.

> Für die Benutzung der Bibliothek (Objekte mit/ohne Body, Rendermethoden)
> siehe [`docs/library.md`](library.md).

## Verzeichnisstruktur

```
.
├── Makefile          Build-Regeln (Bibliothek, Apps, Tests, Start)
├── lib/              Ziel der statischen Bibliothek
│   ├── lib3d.a       erzeugte statische Bibliothek (ignoriert von git)
│   ├── src/          alle Quellen (*.cpp) der Bibliothek
│   └── header/       alle Header (*.hpp) der Bibliothek
├── apps/             Anwendungen, je App ein Unterordner
│   ├── boids/        main.cpp + vehicle.hpp/vehicle.cpp (mehrere Quellen)
│   ├── rocket/       main.cpp + vehicle.hpp/vehicle.cpp + rocket.obj (Mesh-Asset)
│   ├── ray/main.cpp
│   ├── vorlage/main.cpp
│   ├── beispiel/main.cpp
│   └── instanzen/main.cpp
├── tests/            Unit-Test der Bibliothek
│   └── lib3d_test.cpp
├── build/            Objektdateien (*.o) – Zwischenerzeugnisse
└── bin/              fertig gelinkte Programme
```

Die Bibliothek wird aus **allen** `lib/src/*.cpp` gebaut. Eine neue
Quelldatei muss also nur in `lib/src/` abgelegt werden – das `Makefile`
findet sie automatisch über eine Wildcard. Header liegen dagegen in
`lib/header/` und werden über den Include-Pfad gefunden. Dasselbe gilt für
Apps: **alle** `apps/<name>/*.cpp` werden kompiliert und gemeinsam gelinkt.
Ein App-Ordner darf also neben `main.cpp` weitere Quellen (und Header)
enthalten; vorausgesetzt ist nur, dass der App-Name in `APP_NAMES` im
Makefile steht (siehe unten).

## Voraussetzungen

- `g++` mit C++20
- `make`
- `ar` (binutils)
- GLFW3 + OpenGL (`libglfw3-dev`, `libgl-dev`) – vorhanden, wenn
  `pkg-config --cflags glfw3` etwas ausgibt

## Targets

| Befehl | Wirkung |
| --- | --- |
| `make lib` | Baut **nur** die statische Bibliothek `lib/lib3d.a`. |
| `make` / `make all` | Baut Bibliothek **und** alle Apps (Standardziel). |
| `make apps` | Baut und linkt alle Apps (Bibliothek wird bei Bedarf mitgebaut). |
| `make bin/<name>` | Baut nur eine einzelne App, z. B. `make bin/ray`. |
| `make run APP=<name>` | Baut die App `<name>` und **startet** sie. |
| `make run-<name>` | Kurzform zum Starten, z. B. `make run-ray`. |
| `make test` | Baut den Unit-Test und führt ihn aus. |
| `make clean` | Löscht `build/`, `bin/` und `lib/lib3d.a`. |

Verfügbare App-Namen (aus `APP_NAMES` im Makefile):
`beispiel`, `vorlage`, `instanzen`, `ray`, `boids`, `rocket`.

Die App `rocket` (basiert auf `boids`) lädt ihr Vehicle-Mesh zur Laufzeit aus
`rocket.obj` (Wavefront-OBJ, siehe `docs/library.md`, Abschnitt 4.3). Das OBJ
liegt im App-Verzeichnis `apps/rocket/` und wird relativ zum Arbeitsverzeichnis
gesucht. Der Build kopiert jede `apps/<name>/*.obj` mit nach `bin/`, damit
`bin/rocket` die Datei neben dem Binary findet (z. B. `cd bin && ./rocket`).
Das geladene Mesh wird um `rocketScale` vergrößert (`ScaleMesh`, Skalierung
der Mesh-Daten – damit bleiben Geometrie und `faces` konsistent).

### Beispiele

```bash
make lib              # nur die Bibliothek bereitstellen
make                  # alles bauen
make apps             # alle Apps bauen und linken
make run APP=ray      # App "ray" bauen und starten
make run-ray          # dasselbe, kurz
make bin/instanzen    # nur "instanzen" bauen
make test             # Unit-Test ausführen
make clean            # aufräumen
```

Wird `make run` ohne `APP` aufgerufen, gibt das Makefile eine
Fehlermeldung mit der Liste der verfügbaren Apps aus.

## Wichtiger Hinweis zum Starten

Die Anwendungen starten im **Fenstermodus** (zentriert, 1280×800) und
benötigen eine laufende grafische Sitzung (X11/Wayland mit GL). Mit **F11**
wird zur Laufzeit auf **Vollbild** umgeschaltet und wieder zurück – das
erledigt die Bibliothek, alle Apps bekommen es automatisch. Programmatisch geht
es über `ToggleFullscreen()` / `SetFullscreen(on)` / `IsFullscreen()`; der
**OpenGL-Kontext bleibt beim Umschalten erhalten** (kein Fenster-Neuaufbau).
Für echtes Vollbild wird ein Monitor benötigt; ohne Monitor bleibt das Fenster
im Fenstermodus (der Start schlägt nicht fehl). In einer reinen SSH-/Headless-
Sitzung lässt sich das Fenster weiterhin nicht erzeugen.

## Wie gebaut wird

1. **Bibliothek:** Für jede `lib/src/*.cpp` entsteht die Objektdatei
   `build/lib/<name>.o` (kompiliert mit `-Ilib/header`). Anschließend werden
   alle Objekte mit `ar rcs lib/lib3d.a …` zur statischen Bibliothek
   archiviert.
2. **Apps:** Für **jede** `apps/<name>/*.cpp` entsteht eine Objektdatei,
   z. B. `build/apps/boids/main.o` und `build/apps/boids/vehicle.o`.
   Gelinkt werden **alle** Objekte der App **zuerst** und die Bibliothek
   danach, damit der Linker die benötigten Symbole findet:

   ```bash
   g++ build/apps/<name>/main.o build/apps/<name>/<weitere>.o \
       lib/lib3d.a -o bin/<name> -lglfw -lGL
   ```

   Links von statischen Bibliotheken müssen nach den Objekten stehen –
   daher die Reihenfolge App-Objekte → `lib3d.a` → Systembibliotheken.
    Externe Mesh-Assets der App (`apps/<name>/*.obj`) werden nach dem Linken
    mit nach `bin/` kopiert, damit das Programm sie relativ zum Binary findet
    (z. B. `rocket.obj` für die App `rocket`).
3. **Header-Abhängigkeiten:** Beim Übersetzen erzeugt der Compiler mit
   `-MMD -MP` zu jeder Objektdatei eine `.d`-Datei (z. B.
   `build/apps/boids/vehicle.d`), die die eingebundenen Header auflistet.
   Das Makefile bindet diese `.d`-Dateien am Ende ein (`-include $(DEPS)`),
   sodass eine Änderung an einem Header (z. B. `vehicle.hpp`) automatisch
   das Neuübersetzen aller davon abhängigen Objekte auslöst. Nach einem
   `make clean` werden die `.d`-Dateien beim ersten Build neu erzeugt.
4. **Header:** Quellen liegen in `lib/src/`, Header in `lib/header/`. Der
   Include-Pfad wird über `-Ilib/header` gesetzt, daher lautet das Einbinden
   in Apps und Tests:

   ```cpp
   #include "lib3d.hpp"   // Gesamt-API der Bibliothek
   #include "bodies.hpp"  // Zusatzmodul (Faces/Raycasting)
   ```

   Innerhalb der Bibliothek werden sich gegenseitig einbindende Header mit
   ihrem reinen Dateinamen eingebunden (z. B. `#include "vec.hpp"`).

## Erweitern

### Neue Bibliotheks-Quelldatei

1. `.cpp` in `lib/src/` ablegen; einen zugehörigen Header (`.hpp`) in
   `lib/header/`.
2. Fertig – die Wildcard `lib/src/*.cpp` nimmt die Quelle automatisch auf.
3. Optional in `lib/header/lib3d.hpp` einbinden, wenn sie Teil der
   Gesamt-API sein soll.

### Neue App

1. Ordner `apps/<name>/` mit `main.cpp` anlegen.
2. Im Makefile in der Zeile `APP_NAMES := …` den Namen ergänzen.
3. Bauen mit `make apps`, starten mit `make run APP=<name>` bzw.
   `make run-<name>`.

Optional können im App-Ordner weitere `.cpp`-Dateien (z. B. `vehicle.cpp`)
und passende Header (`.hpp`) liegen. Sie werden automatisch mitkompiliert
und gelinkt – es muss nur der App-Ordner selbst in `APP_NAMES` stehen. Für
Header innerhalb der App genügt das Einbinden mit dem reinen Dateinamen
(`#include "vehicle.hpp"`).

### Externe Meshes (`apps/<name>/*.obj`)

Meshes werden zur Laufzeit über `LoadOBJ(path)` geladen (siehe
`docs/library.md`, Abschnitt 4.3). Beispiel: `apps/rocket/rocket.obj`
(Wavefront-OBJ, Y-up, Basis bei y=0, Spitze bei y=10, mit Drahtgitter-Linien).

Das OBJ liegt neben `main.cpp` im App-Verzeichnis und wird **relativ zum
Arbeitsverzeichnis** gesucht. Damit es gefunden wird, kopiert der Build jede
`apps/<name>/*.obj` mit nach `bin/`, sodass das Programm die Datei neben dem
Binary findet (z. B. `cd bin && ./rocket`).

## Hinweise

- `build/`, `bin/` und `*.a` sind über `.gitignore` von der
  Versionsverwaltung ausgenommen.
- Überschreiben der Werkzeuge ist möglich, z. B.
  `make CXX=clang++ lib`.
- `CXXFLAGS`/`CPPFLAGS` können beim Aufruf ergänzt werden.
