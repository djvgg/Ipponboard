# Ipponboard - API Dokumentation

## Inhaltsverzeichnis

**Linux**
- [CI/CD Pipeline - automatisches Kompilieren](#cicd-pipeline---automatisches-kompilieren)
- [Auf Linux kompilieren](#auf-linux-kompilieren)
  - [Voraussetzungen](#voraussetzungen)
  - [Umgebung konfigurieren](#umgebung-konfigurieren)
  - [Build starten](#build-starten)
  - [CI-Build (ohne Menü)](#ci-build-ohne-menü)
  - [Starten der Anwendung](#starten-der-anwendung)

**Windows**
- [Auf Windows kompilieren](#auf-windows-kompilieren)

**API**
- [API-Server](#api-server)
  - [Konfiguration](#konfiguration)
  - [Webhook (Score-Rückmeldung)](#webhook-score-rückmeldung)

---

## CI/CD Pipeline - automatisches Kompilieren

Die GitLab-CI-Pipeline (`.gitlab-ci.yml`) baut Ipponboard automatisch auf **Ubuntu 24.04**:

**Was passiert:**
1. Ubuntu 24.04 Docker-Image wird gestartet
2. Alle Abhängigkeiten werden per `apt-get` installiert
3. `env_cfg.bat` wird mit System-Qt/Boost-Pfaden konfiguriert
4. `./build.sh --ci` führt Clean-Build + Tests + Archiv-Erstellung durch
5. Das `.7z`-Archiv wird als CI-Artefakt gespeichert (1 Woche)

Am Ende kann man das Archiv herunterladen und entpacken und das Programm starten.

---

## Auf Linux kompilieren

### Voraussetzungen

Folgende Pakete müssen installiert sein:

| Paket                     | Beschreibung            |
| ------------------------- | ----------------------- |
| `build-essential`         | GCC, Make & Co.         |
| `cmake`                   | Build-System            |
| `qtbase5-dev`             | Qt5 Basis               |
| `qtmultimedia5-dev`       | Qt5 Multimedia          |
| `libqt5xmlpatterns5-dev`  | Qt5 XML Patterns        |
| `qttools5-dev-tools`      | Qt5 Tools (lrelease)    |
| `libboost-all-dev`        | Boost-Bibliotheken      |
| `p7zip-full`              | 7-Zip (für Archiv)      |
| `pandoc`                  | Doku-Erzeugung (HTML)   |

### Umgebung konfigurieren

Beim ersten Start von `build.sh` wird eine Datei `env_cfg.bat` erzeugt.
Dort müssen die Pfade zu Qt und Boost angegeben werden.

> **Tipp:** Wenn Qt und Boost über `apt` installiert wurden, reicht:
> ```
> set "LINUX_QTDIR=/usr"
> set "LINUX_BOOST_DIR=/usr"
> ```

### Build starten

```bash
./build.sh
```

Es erscheint ein interaktives Menü:

| Option | Aktion                           |
| ------ | -------------------------------- |
| `1`    | Alles bereinigen (clean)         |
| `2`    | CMake Makefiles erstellen        |
| `3`    | Nur Tests bauen & ausführen      |
| `4`    | Alles bauen (inkl. Tests & Doku) |
| `5`    | Ipponboard starten               |
| `6`    | Dokumentation erzeugen           |
| `7`    | Übersetzungen kompilieren        |
| `8`    | Archiv (.7z) erstellen           |
| `9`    | Komplett-Build + Archiv          |
| `s`    | Debug/Release umschalten         |
| `q`    | Beenden                          |

**Typischer Erstbuild:** `1` → `2` → `4`

### CI-Build (ohne Menü)

```bash
./build.sh --ci
```

Führt automatisch einen Clean-Build im Release-Modus durch und erzeugt ein `.7z`-Archiv unter `_output/`. Ist
genau das gleiche wie die Option 9 im interaktiven Menü.

Die Datei kann entpackt werden und das Programm kann gestartet werden ./Ipponboard.

### Starten der Anwendung

```bash
./Ipponboard
```

Im gleichen Ordner liegt eine Datei `categories.config` und beim erstenmal Starten der Anwendung wird eine `Ipponboard.ini` und eine `Ipponboard.log` erzeugt.

---

## Auf Windows kompilieren

Auf Windows gibt es keine automatische CI/CD-Pipeline. Der Build muss manuell auf dem Rechner durchgeführt werden.


### Wichtige Anmerkung

- Auf dem grauen PC, wo noch Windows drauf ist, sind alle Biblitoheken und Programme schon installiert, auch ist eine exe schon drauf, bitte mit dem admin anmelden. Heißt auf diesem PC kann einfach mit Visual Studio das Ipponboard Projekt geöffnet werden und darin immer beliebig eine neue exe kompiliert werden. 

- Auf dem Windows Rechnern kann die Firewall Probleme verursachen diese für das jeweilige Netzwerk in dem sich das Gerät befindet anpassen/ausschalten. Das heißt es muss sichergestellt werden dass der API-Server auf dem jeweiligen Netzwerk erreichbar ist.

- Bitte immer den Explorer von Windows nutzen um Dateien vom Ipponboard zu konfigurieren, da im Projektordner von Visual Studio nicht alle Dateien angezeigt werden oder genauer gesagt es wird nicht die vollkommene Ordnerstruktur angezeigt werden.

- Des Weiteren können unter Windows die Namen der Dateien verschiedene heißen wie die `Ipponboard.ini` oder `Ipponboard.log`. Sie heißen dann einfach `Ipponboard` heißt der Suffix wird abgeschnitten bitte schaut immer in den Ordner wo die exe liegt.

### Voraussetzungen

Folgende Software muss installiert sein:

| Software                       | Beschreibung                  |
| ------------------------------ | ----------------------------- |
| Visual Studio 2022 oder höher  | Compiler (MSVC, C++ Workload) |
| CMake                          | Build-System (in VS enthalten)|
| Qt 5.15.x (x86, MSVC)         | UI-Framework                  |
| Boost 1.81.0 oder höher        | C++ Bibliotheken              |
| Inno Setup 6                   | Installer-Erstellung          |
| Pandoc                         | Doku-Erzeugung (HTML)         |

### Umgebung konfigurieren

Beim ersten Start eines Build-Skripts wird `env_cfg.bat` im Projektordner erzeugt. Dort müssen die Pfade angepasst werden (Wichtige Info: das unten sind nur Platzhalter-Beispiele, je nach System müssen die Pfade konkret angegeben werden):

```bat
set "IPPONBOARD_ROOT_DIR=c:\dev\_cpp\Ipponboard"
set "QTDIR=c:\devtools\qt5\Qt5.15.13-x86-msvc2022"
set "BOOST_DIR=c:\devtools\boost_1_81_0"
set "INNO_DIR=c:\Program Files (x86)\Inno Setup 6"
```

### Build starten

In der **PowerShell** ausführen:

```powershell
.\build.ps1
```

Es erscheint ein interaktives Menü:

| Option | Aktion                              |
| ------ | ----------------------------------- |
| `1`    | Alles bereinigen (clean)            |
| `2`    | CMake Makefiles erstellen           |
| `3`    | Nur Tests bauen & ausführen         |
| `4`    | Alles bauen (inkl. Tests & Doku)    |
| `5`    | Ipponboard starten                  |
| `6`    | Dokumentation erzeugen              |
| `7`    | Übersetzungen kompilieren           |
| `8`    | Setup (Installer) erstellen         |
| `9`    | Komplett-Build + Setup (Release)    |
| `s`    | Debug/Release umschalten            |
| `q`    | Beenden                             |

**Typischer Erstbuild:** `1` → `2` → `4`

Die fertige `.exe` liegt unter `_bin\Ipponboard-release\`.

---

## API-Server

### Konfiguration

Die Ports werden in der `.ini`-Datei von Ipponboard gespeichert und können dort angepasst werden sie ist unter dem namen `Ipponboard.ini` zu finden:

| Einstellung      | Standard | Beschreibung                           |
| ---------------- | -------- | -------------------------------------- |
| Ipponboard-Port  | `8080`   | Port auf dem der API-Server lauscht    |
| Website-Port     | `5001`   | Port der Website (für Webhook-Callback)|

In der `categories.config` können die Kategorien und Rundenzeiten für den Wettkampf konfiguriert werden. 
In der `Ipponboard.log` werden die Logs der Anwendung gespeichert und können dort eingesehen werden. Als Beispiel kann dort entnommen werden auf welchem Port der API-Server also das Ipponboard gestartet wurde und auf welchem Port das Ipponboard denkt dass die Website gestartet wurde. Außerdem ist zu sehen ob der API-Server erfolgreich gestartet wurde.


### Webhook (Score-Rückmeldung)

Nach einem erfolgreichen `POST /fighters` setzt Ipponboard automatisch eine Callback-URL basierend auf der **IP-Adresse des Senders** und dem konfigurierten **Website-Port**:

```
http://<sender-ip>:<website-port>/api/ippon-score
```

Nachdem der Button Senden gedrückt wird, werden die Daten an die Website gesendet und dort angezeigt.

> **Wichtig:** Beide Geräte müssen im selben Netzwerk sein. WLAN ist nicht zwingend erforderlich – eine direkte LAN-Verbindung funktioniert ebenfalls.
