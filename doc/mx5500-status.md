# Logitech Cordless Desktop MX 5500 Revolution unter Linux

**Projektstand:** 17. August 2026  
**Schwerpunkt:** Linux-/Debian-/Proxmox-Nutzung der MX5500-Tastatur, LCD-Funktionen, Sondertasten und Einbindung in eine virtualisierte Desktop-Umgebung

> Dieses Dokument trennt bewusst zwischen **praktisch verifizierten Funktionen**, **vom Quellcode unterstützten Funktionen** und **noch offenen/ungeprüften Punkten**.

---

## 1. Kurzfassung

Die Logitech **MX5500** ist trotz ihres Alters unter aktuellem Linux erstaunlich gut nutzbar:

- normale Tastaturfunktion über Bluetooth funktioniert;
- die Tastatur kann direkt mit einem Linux-Bluetooth-Adapter gekoppelt werden;
- die Linux-Community-Software **`mx5000tools`** erkennt die MX5500 ausdrücklich per Bluetooth und USB;
- Uhrzeit/Datum lassen sich mit `mx5000-tool --time` synchronisieren;
- das Tastatur-Piepen lässt sich mit `--keybdopts` abschalten;
- das LCD kann mit eigenen Texten, Lauftext, Fortschrittsbalken und kleinen PBM-Grafiken beschrieben werden;
- das E-Mail- und das Mute-Icon lassen sich auf der MX5500 setzen;
- die beiden anderen von `mx5000tools` angebotenen Legacy-Icons werden von der MX5500 ignoriert;
- `--name` wird von der MX5500 offenbar ebenfalls ignoriert;
- die eingebauten Media- und Posteingangsseiten können mit `mx5000tools` **nicht** wie früher über Logitech SetPoint mit nativen Daten versorgt werden;
- Sondertasten lassen sich mit normalen Linux-Input-Werkzeugen auswerten;
- für die Sleep-Taste eignet sich `triggerhappy`, wenn Suspend auf Host und ggf. Gast deaktiviert wird.

Die Tastatur wird aktuell an einer Entwickler-Workstation genutzt. Sie ist dafür mit dem Proxmox VE Host gekoppelt, steuert aber meistens via SPICE eine Entwickler-VM.

---

## 2. Hardware und Identifikation

### Modell

- Produktfamilie: **Logitech Cordless Desktop MX 5500 Revolution**
- Tastaturmodell laut Typenschild: **Y-RBF91** (P/N:820-000818)
- Bluetooth-HID Vendor/Product ID der MX5500 in `mx5000tools`:
  - Vendor: `046d`
  - Product: `b30b`
- Linux-Bus:
  - USB: `BUS_USB`
  - direkte Bluetooth-Verbindung: `BUS_BLUETOOTH`

Der `mx5000tools`-Quellcode enthält ausdrücklich beide Varianten:

```c
/* MX5500 */
{ { BUS_USB,       0x046d, 0xb30b } },
{ { BUS_BLUETOOTH, 0x046d, 0xb30b } },
```

Quelle:  
https://github.com/jwrdegoede/mx5000tools/blob/master/libmx5000/mx5000.c

### Aktueller Hardwarezustand

- Der originale Logitech-USB-Empfänger ist derzeit **nicht vorhanden**.
- Die Tastatur ist deshalb direkt mit dem eingebauten Bluetooth-Adapter des Rechners gekoppelt.
- Für UEFI/Pre-Boot wird derzeit eine separate USB-Tastatur benötigt.
- Direktes Bluetooth steht erst zur Verfügung, nachdem der Linux-Bluetooth-Stack gestartet wurde.

Ein originaler MX5500-Empfänger kann prinzipiell als HID-Proxy arbeiten und damit Pre-Boot-Tastatureingaben ermöglichen. Das ist im aktuellen Senza-Setup mangels Empfänger jedoch **nicht praktisch getestet**.

---

## 3. Bluetooth-Einrichtung unter Debian/Proxmox

### Benötigte Software

Unter Debian 13 / Proxmox:

```bash
apt install bluez
systemctl enable --now bluetooth
```

Debian-Paket:  
https://packages.debian.org/trixie/bluez

BlueZ-Projekt:  
https://www.bluez.org/

### Pairing mit `bluetoothctl`

```bash
bluetoothctl
```

Dann interaktiv:

```text
power on
agent on
default-agent
scan on
```

Sobald die MX5500 sichtbar ist:

```text
pair AA:BB:CC:DD:EE:FF
trust AA:BB:CC:DD:EE:FF
connect AA:BB:CC:DD:EE:FF
quit
```

`AA:BB:CC:DD:EE:FF` durch die tatsächliche Bluetooth-Adresse ersetzen.

### Prüfen, ob Linux die Tastatur sieht

Beispielsweise:

```bash
grep -H . /sys/class/hidraw/hidraw*/device/uevent 2>/dev/null
```

oder:

```bash
udevadm info /dev/hidrawX
```

Für die Input-Seite:

```bash
cat /proc/bus/input/devices
```

Im Senza-System taucht die Tastatur als:

```text
Logitech MX5500 Keyboard
```

auf.

---

## 4. Linux-Input und Sondertasten

Die normalen Tasten funktionieren als Standard-HID-Tastatur.

Für Sondertasten empfiehlt sich zum Testen:

```bash
apt install evtest
evtest
```

Debian-Paket:  
https://packages.debian.org/trixie/evtest

`evtest` zeigt beispielsweise Ereignisse wie:

```text
KEY_SLEEP
KEY_VOLUMEUP
KEY_VOLUMEDOWN
KEY_MUTE
...
```

Für neue Projekte weist Debian darauf hin, dass `evtest` im Wartungsmodus ist und `evemu-record` für neuere Input-Funktionen moderner ist. Für die MX5500-Diagnose reicht `evtest` normalerweise aus.

---

## 5. `mx5000tools`

### Projekt

`mx5000tools` ist eine Linux-Userspace-Software zur Ansteuerung der Zusatzfunktionen der MX5000-Serie.

Repository:

https://github.com/jwrdegoede/mx5000tools

README:

https://github.com/jwrdegoede/mx5000tools/blob/master/README

Wesentliche Komponenten:

- `libmx5000`: direkte Kommunikation mit der Tastatur;
- `mx5000-tool`: Kommandozeilenfrontend;
- Zugriff über Linux **`hidraw`**;
- Abhängigkeit von Netpbm für Grafik-/PBM-Funktionen.

Das Projekt wurde ursprünglich für die MX5000 entwickelt, enthält inzwischen aber explizite IDs für die MX5500.

---

## 6. `mx5000tools` unter Debian 13 bauen

Für Debian 13 gibt es im Standardarchiv kein fertiges `mx5000tools`-Paket. Der Quellcode lässt sich lokal bauen.

### Build-Abhängigkeiten

```bash
apt install \
    git \
    build-essential \
    autoconf \
    automake \
    libtool \
    libnetpbm-dev
```

Netpbm-Development-Paket:  
https://packages.debian.org/trixie/libnetpbm-dev

Die Abhängigkeit von Netpbm ist im Upstream-`configure.ac` hinterlegt:

https://github.com/jwrdegoede/mx5000tools/blob/master/configure.ac

### Quellcode holen und bauen

```bash
cd /usr/local/src

git clone https://github.com/jwrdegoede/mx5000tools.git
cd mx5000tools

autoreconf -fi
./configure
make
make install
ldconfig
```

Danach sollte das Programm typischerweise hier liegen:

```text
/usr/local/bin/mx5000-tool
```

Prüfen:

```bash
/usr/local/bin/mx5000-tool --help
```

---

## 7. Praktisch bestätigte `mx5000-tool`-Funktionen auf der MX5500

### 7.1 Uhrzeit und Datum

Bestätigt funktionierend:

```bash
mx5000-tool --time
```

Ohne Zeitargument nimmt das Tool die aktuelle lokale Systemzeit.

Im Quellcode wird `time(NULL)` verwendet und anschließend lokale Zeit, Datum, Wochentag und Jahr an die Tastatur übertragen.

Quelle:

https://github.com/jwrdegoede/mx5000tools/blob/master/mx5000-tool/mx5000-tool.c

### 7.2 Tastatur-Piepen / Media-Keys

Bestätigt funktionierend:

```bash
mx5000-tool --keybdopts 0
mx5000-tool --keybdopts 1
mx5000-tool --keybdopts 2
mx5000-tool --keybdopts 3
```

Bedeutung laut Upstream:

| Wert | Wirkung |
|---:|---|
| `0` | Piepen und Media-Keys aktiv |
| `1` | Piepen bei Sondertasten deaktivieren |
| `2` | Media-Keys deaktivieren |
| `3` | Piepen und Media-Keys deaktivieren |

Für den Alltag besonders nützlich:

```bash
mx5000-tool --keybdopts 1
```

Damit bleiben die Media-Tasten aktiv, aber störendes Piepen kann abgeschaltet werden.

### 7.3 E-Mail- und Mute-Icon

Syntax laut Tool:

```bash
mx5000-tool --icons ABCD
```

Upstream interpretiert die vier Positionen als:

1. E-Mail
2. IM/Messenger
3. Mute
4. Phone/Walkie

Jede Position:

- `0` = aus
- `1` = an
- `2` = blinken

Beispiele:

```bash
# E-Mail an
mx5000-tool --icons 1000

# E-Mail blinkt
mx5000-tool --icons 2000

# Mute an
mx5000-tool --icons 0010

# E-Mail und Mute an
mx5000-tool --icons 1010
```

**Praktischer MX5500-Befund:**

- E-Mail-Icon: funktioniert;
- Mute-Icon: funktioniert;
- IM/Messenger: wird ignoriert;
- Phone/Walkie: wird ignoriert.

Das ist plausibel, weil `mx5000tools` hier ein älteres MX5000-Protokoll verwendet, die MX5500-Firmware aber andere feste Symbole besitzt.

Quellcode:

https://github.com/jwrdegoede/mx5000tools/blob/master/libmx5000/mx5000.c

### 7.4 `--name`

Upstream bietet:

```bash
mx5000-tool --name "spve"
```

Der Name ist auf 11 Zeichen begrenzt.

**Praktischer MX5500-Befund:**

Der Befehl wird ohne Fehler übertragen, aber der Name erscheint **auf keiner internen Displayseite**.

Für die MX5500 ist `--name` daher praktisch als **Legacy-No-op** zu betrachten.

### 7.5 Temperatur-Einheit

Vom Tool unterstützt:

```bash
mx5000-tool --celcius
mx5000-tool --farenheight
```

Die Schreibweise der Optionen entspricht dem historischen Upstream-Code.

Diese Funktion wird vom Quellcode unterstützt; sie wurde im Senza-Projekt nicht so intensiv getestet wie `--time` und `--keybdopts`.

### 7.6 Beep-Test

```bash
mx5000-tool --beep
```

Erzeugt einen Tastatur-Beep und ist nützlich, um die Kommunikation zu testen.

---

## 8. Freie LCD-Ausgabe

`mx5000tools` kann das LCD außerhalb des eingebauten Logitech-Menüsystems frei beschreiben.

### Statischer Text
Text mit normaler Größe funktioniert:
```bash
mx5000-tool --static --add-text "Proxmox VE" reg 20 25
```

## Größerer Text
Extra großer Text funktioniert **nicht**:
```bash
mx5000-tool --static --add-text "SPVE" huge 10 15
```

### Lauftext

```bash
mx5000-tool \
    --rolling "Dies ist ein langer Text fuer das Display..." 16 0 20
```

### Fortschrittsbalken

```bash
mx5000-tool \
    --static --add-text "Fortschritt" 6 0 20 \
    --add-progressbar 20 10 reg 0 35
```

### Aktualisierbare Textfelder

```bash
mx5000-tool --static --add-text "Status     " reg 20 25 --ref
```

Das Tool gibt eine Referenznummer aus, zum Beispiel:

```text
0
```

Später:

```bash
mx5000-tool --update 0 "VM RUNNING"
```
(FIXME: Das Update funktioniert nicht immer...)

### Kleine PBM-Grafiken
TODO: testen

```bash
mx5000-tool --image bild.pbm 0 0
```

Maximalgröße laut Upstream-Hilfe:

```text
29x40
```

### Eingebaute Symbole für freie LCD-Seiten

`--add-icon` unterstützt u. a.:

```text

person
circle
right_triangle
left_triangle
arrow_up
arrow_down
square
pause
next_track
undo
a
b
c
d
```

Liste anzeigen:

```bash
mx5000-tool --help-icons
```

### Zurück zum eingebauten Display

```bash
mx5000-tool --reset
```

---

## 9. Eingebaute LCD-Seiten der MX5500

Die Tastatur besitzt eigene Firmware-Seiten. Vermutlich folgende sechs Hauptseiten:

1. Datum/Uhrzeit
2. Media
3. Temperatur
4. Favoriten
5. Tastenzähler
6. E-Mail/Posteingang

## Taschenrechner
Der Taschenrechner ist ein separater, von der Tastatur selbst implementierter Modus, der sich mit der Taschenrechner-Taste ein- und ausschalten lässt und keine siebte Seite.

Im Taschenrechner-Modus wird die numerische Tastatur fest dem Taschenrechner zugeordnet; dem PC steht sie solange nicht zur Verfügung.

Der Taschenrechner rechnet stupide in der Reihenfolge der Eingabe, **ohne** Punkt-vor-Strich!

### Verhalten ohne Logitech SetPoint

| Seite | Quelle der Daten | Verhalten unter Linux ohne SetPoint |
|---|---|---|
| Uhr/Datum | Tastatur + Host-Synchronisation | funktioniert; mit `--time` synchronisierbar |
| Media | PC/SetPoint | zeigt sinngemäß „Daten fehlen“ |
| Temperatur | Tastatur intern | funktioniert |
| Favoriten | PC-Konfiguration | nur eingeschränkt sinnvoll |
| Tastenzähler | Tastatur intern | funktioniert |
| Posteingang | PC/SetPoint | zeigt „Eingang fehlt“ |

### Wichtige Einschränkung

`mx5000tools` kann **nicht** einfach die originalen Media- und Posteingangsseiten mit SetPoint-kompatiblen Daten versorgen.

Das Upstream-README nennt ausdrücklich als noch offene Reverse-Engineering-Aufgabe die Kontrolle des Inhalts des eingebauten Menüsystems.

Das bedeutet:

- Media-Informationen lassen sich auf einer **eigenen freien LCD-Seite** nachbauen;
- Mail-Status lässt sich auf einer **eigenen freien LCD-Seite** darstellen;
- das E-Mail-Symbol kann gesetzt oder zum Blinken gebracht werden;
- die originalen internen Media-/Inbox-Seiten bleiben trotzdem unversorgt.

Quelle:

https://github.com/jwrdegoede/mx5000tools/blob/master/README

---

## 10. Sinnvolle eigene LCD-Nutzung

Für einen Linux-/Proxmox-Rechner sind eigene Statusseiten interessanter als die alten SetPoint-Seiten.

Beispiele:

```text
spve
VM100   RUN
AI      2 CT
```

oder:

```text
RAM   21/32G
CPU      18%
TEMP     54C
```

oder:

```text
BACKUP FAILED
CT 203
08:17
```

Mögliche Datenquellen:

- `qm status <VMID>`
- `pct status <CTID>`
- `/proc/loadavg`
- `/proc/meminfo`
- `sensors`
- PVE-API
- Backup-/Job-Status
- AI-Containerstatus

Eine kleine Shell- oder Python-Anwendung kann daraus LCD-Seiten generieren und vorhandene Textfelder über `--update` aktualisieren.

---

## 11. Automatische Zeitsynchronisation

### Aktueller Projektstatus

`mx5000-tool --time` funktioniert zuverlässig.

Ein automatischer Trigger bei Bluetooth-Reconnect ist sinnvoll, aber noch **nicht abschließend als final getestet dokumentiert**.
TODO: finalisieren und testen!

### Einfache systemd-Unit

```ini
# /etc/systemd/system/mx5500-sync.service

[Unit]
Description=Synchronize Logitech MX5500 clock
After=bluetooth.service

[Service]
Type=oneshot
ExecStart=/usr/local/bin/mx5000-tool --time
```

Kein `[Install]` ist nötig, wenn der Dienst ausschließlich von einem Device-Event gestartet wird.

### Möglicher udev-Trigger

Die MX5500 erscheint über Bluetooth als HID-Gerät mit:

```text
046d:b30b
```

Ein möglicher Ansatz ist:

```udev
# /etc/udev/rules.d/90-mx5500.rules

ACTION=="add", SUBSYSTEM=="hidraw", \
KERNELS=="0005:046D:B30B.*", \
TAG+="systemd", \
ENV{SYSTEMD_WANTS}+="mx5500-sync.service"
```

Danach:

```bash
udevadm control --reload-rules
systemctl daemon-reload
```

**Vor produktiver Übernahme unbedingt den tatsächlichen Sysfs-Pfad prüfen:**

```bash
udevadm info -a /dev/hidrawX
```

und Reconnect beobachten:

```bash
udevadm monitor --udev --property --subsystem-match=hidraw
```

Dieser Teil ist als **vorgesehene Lösung**, nicht als bereits vollständig verifizierter Projektstand zu behandeln.

---

## 12. Sleep-Taste: Suspend verhindern und Desktop sperren

Die Sleep-Taste Ist mit PVE problematisch:

- Ohne Spezialbehandlung suspendiert sie den PVE-Host;
- der Host lässt sich nicht per Bluetooth wieder aufwecken;
- nach Suspend/Resume sind ggf. VMs beschädigt;
- Selbst nachdem Host-Suspend unterbunden ist, wird `KEY_SLEEP` weiterhin über SPICE an die Desktop-VM übertragen und suspendiert dort die VM;
- auch dieser Suspend funktioniert je nach Setup nicht zuverlässig.

Eine sinnvolle Entscheidung lautet daher:

> **Suspend wird weder auf dem PVE-Host noch in der Desktop-VM benötigt und wird auf beiden Systemen deaktiviert. Die MX5500-Sleep-Taste sperrt stattdessen die GNOME-Sitzung der Desktop-VM.**

---

## 13. Suspend auf dem PVE-Host deaktivieren

### systemd-sleep

```bash
mkdir -p /etc/systemd/sleep.conf.d

cat >/etc/systemd/sleep.conf.d/60-senza-no-suspend.conf <<'CONF'
[Sleep]
AllowSuspend=no
AllowHibernation=no
AllowHybridSleep=no
AllowSuspendThenHibernate=no
CONF
```

Upstream-Dokumentation:

https://www.freedesktop.org/software/systemd/man/systemd-sleep.conf.html

Test:

```bash
systemctl suspend
```

Der Suspend-Versuch muss fehlschlagen.

### Sleep-Taste in `systemd-logind` ignorieren

```bash
mkdir -p /etc/systemd/logind.conf.d

cat >/etc/systemd/logind.conf.d/99-MX5500.conf <<'CONF'
[Login]
HandleSuspendKey=ignore
CONF
```

Upstream-Dokumentation:

https://www.freedesktop.org/software/systemd/man/logind.conf.html

Nach Neustart prüfen:

```bash
busctl get-property \
    org.freedesktop.login1 \
    /org/freedesktop/login1 \
    org.freedesktop.login1.Manager \
    HandleSuspendKey
```

Erwartet:

```text
s "ignore"
```

---

## 14. Suspend in der Desktop-VM deaktivieren

In der Debian-/GNOME-Desktop-VM ebenfalls:

```bash
sudo mkdir -p /etc/systemd/sleep.conf.d

sudo tee /etc/systemd/sleep.conf.d/99-no-suspend.conf >/dev/null <<'CONF'
[Sleep]
AllowSuspend=no
AllowHibernation=no
AllowHybridSleep=no
AllowSuspendThenHibernate=no
CONF
```

Optional zusätzlich:

```bash
sudo mkdir -p /etc/systemd/logind.conf.d

sudo tee /etc/systemd/logind.conf.d/99-MX5500.conf >/dev/null <<'CONF'
[Login]
HandleSuspendKey=ignore
CONF
```

Danach VM neu starten.

Damit bleibt ein eventuell über SPICE weitergereichtes `KEY_SLEEP` wirkungslos.

---

## 15. `triggerhappy` für die Sleep-Taste

### Installation

```bash
apt install triggerhappy
```

Debian-Paket:

https://packages.debian.org/trixie/triggerhappy

Upstream:

https://github.com/wertarbyte/triggerhappy

`triggerhappy` ist ein kleiner systemweiter Hotkey-Daemon, der Linux-Input-Events beobachtet und Kommandos startet.

### Event prüfen

```bash
thd --dump /dev/input/event17
```

Die Event-Nummer kann sich ändern; vorher den richtigen MX5500-Event-Knoten bestimmen.

Beim Drücken der Sleep-Taste wird im Senza-System:

```text
KEY_SLEEP
```

erzeugt.

### Trigger-Konfiguration

```text
# /etc/triggerhappy/triggers.d/mx5500.conf

KEY_SLEEP    1    /usr/bin/sudo -n /usr/local/sbin/senza-lock-vdesktop
```

Bei `triggerhappy` bedeutet:

- Wert `1`: Taste gedrückt;
- Wert `2`: gehalten/repeat;
- Wert `0`: losgelassen.

Quelle:

https://github.com/wertarbyte/triggerhappy

---

## 16. Sperrskript für die Desktop-VM

Im konkreten Senza-Projekt hat die Desktop-VM die VM-ID `100`.

```bash
# /usr/local/sbin/lock-vdesktop
#!/bin/sh

exec /usr/sbin/qm guest exec 100 -- /usr/bin/loginctl lock-sessions
```

Rechte:

```bash
chown root:root /usr/local/sbin/lock-vdesktop
chmod 755 /usr/local/sbin/lock-vdesktop
```

Voraussetzung:

- QEMU Guest Agent in der VM installiert und aktiv;
- PVE-Guest-Agent-Kommunikation funktioniert.

Manueller Test:

```bash
/usr/local/sbin/senza-lock-vdesktop
```

Die GNOME-Sitzung in VM 100 sollte sofort gesperrt werden.

---

## 17. Rechte für `triggerhappy`

Im Senza-System läuft `triggerhappy` unprivilegiert, konkret als Benutzer `nobody`.

Prüfen:

```bash
ps -o user=,args= -C thd
```

`qm guest exec` benötigt Host-Rechte. Deshalb wird **nicht** der gesamte Hotkey-Daemon als root betrieben; stattdessen bekommt nur das eine root-eigene Skript eine gezielte `sudo`-Freigabe.

```bash
visudo -f /etc/sudoers.d/senza-triggerhappy
```

Inhalt:

```sudoers
nobody ALL=(root) NOPASSWD: /usr/local/sbin/senza-lock-vdesktop
```

Sicherheit:

```bash
chown root:root /usr/local/sbin/senza-lock-vdesktop
chmod 755 /usr/local/sbin/senza-lock-vdesktop
```

Test aus Sicht von `triggerhappy`:

```bash
sudo -u nobody sudo -n /usr/local/sbin/senza-lock-vdesktop
```

Wenn das funktioniert:

```bash
systemctl restart triggerhappy
```

Danach sperrt die MX5500-Sleep-Taste den virtuellen Desktop.

---

## 18. Warum kein `keyd`?

`keyd` wurde als mögliche Lösung erwogen, um `KEY_SLEEP` in einen harmlosen Ersatz-Key umzuschreiben.

Im aktuellen Setup ist das unnötig:

- Host-Suspend ist grundsätzlich deaktiviert;
- `systemd-logind` ignoriert `KEY_SLEEP`;
- Gast-Suspend ist ebenfalls deaktiviert;
- `triggerhappy` kann direkt auf `KEY_SLEEP` reagieren.

Dadurch bleibt die Konfiguration kleiner und hat keinen zusätzlichen Remapping-Daemon.

---

## 19. Zusammenspiel mit einer virtuellen Desktop-VM

Empfohlene Architektur:

```text
                         Bluetooth
MX5500  ------------------------------------------> Linux/PVE-Host
  |                                                    |
  |                                                    +--> BlueZ
  |                                                    +--> hidraw
  |                                                    +--> mx5000tools
  |                                                    |      |
  |                                                    |      +--> Uhr/LCD/Icons
  |                                                    |
  |                                                    +--> triggerhappy
  |                                                           |
  |                                                           +--> Lock VM
  |
  +--> normale Tastatureingaben --> remote-viewer/SPICE --> Desktop-VM
```

Vorteile:

- Bluetooth-Gerät bleibt am Host;
- LCD-/HIDRAW-Sonderfunktionen bleiben verfügbar;
- keine exklusive Bluetooth- oder USB-Durchreichung an die VM;
- normale Tasten sind in der VM nutzbar;
- hostseitige Sondertasten können unabhängig von der aktiven SPICE-Session ausgewertet werden.

---

## 20. Bekannte Einschränkungen

### UEFI / Pre-Boot

Bei direkter Kopplung mit dem eingebauten Bluetooth-Adapter steht die MX5500 erst nach Start des Linux-Bluetooth-Stacks zur Verfügung.

Aktuell:

- UEFI: separate USB-Tastatur;
- Linux/PVE: MX5500 per Bluetooth.

### `--name`

Wird auf der MX5500 ignoriert.

### Vier Legacy-Icons

Von den vier `mx5000tools`-Feldern reagieren auf der MX5500 praktisch nur:

- E-Mail;
- Mute.

### Originale Media-Seite

Kann mit `mx5000tools` nicht nativ mit Player-Metadaten befüllt werden.

### Originale Posteingangsseite

Kann mit `mx5000tools` nicht nativ mit einem Unread-Counter befüllt werden.

### Menüsystem

Das `mx5000tools`-README nennt das Reverse Engineering der internen Menüinhalte ausdrücklich als unvollständig.

### Bluetooth-Reconnect-Zeitsync

Der udev/systemd-Trigger ist vorgesehen, aber noch als zu verifizierende Projektkomponente zu behandeln.

---

## 21. Sinnvolle nächste Erweiterungen

### Eigener LCD-Statusdienst

Ein kleines Skript/Daemon könnte zyklisch anzeigen:

- Hostname;
- VM-Status;
- Containerstatus;
- CPU/RAM;
- Temperatur;
- Backupstatus;
- AI-GPU/NPU-Status;
- Warnungen.

### Media-Infos als eigene LCD-Seite

Statt die originale Media-Seite zu rekonstruieren:

- MPRIS/Player-Daten aus Linux lesen;
- Titel/Interpret über `--rolling` anzeigen;
- Fortschritt über `--add-progressbar` oder `--update`.

### Mailstatus

Statt der originalen Inbox-Seite:

- ungelesene Anzahl auf eigener LCD-Seite;
- E-Mail-Icon bei neuen Nachrichten einschalten oder blinken lassen.

### Weitere Sondertasten

Mit `evtest`/`evemu-record` systematisch erfassen und anschließend entscheiden:

- direkt an Desktop-VM durchlassen;
- hostseitig mit `triggerhappy` verwenden;
- unbelegt lassen.

---

## 22. Software- und Quellenlinks

### `mx5000tools`

Projekt:

https://github.com/jwrdegoede/mx5000tools

README:

https://github.com/jwrdegoede/mx5000tools/blob/master/README

MX5500-Geräte-IDs und Low-Level-Kommandos:

https://github.com/jwrdegoede/mx5000tools/blob/master/libmx5000/mx5000.c

CLI-Optionen:

https://github.com/jwrdegoede/mx5000tools/blob/master/mx5000-tool/mx5000-tool.c

Build-Abhängigkeiten:

https://github.com/jwrdegoede/mx5000tools/blob/master/configure.ac

### Debian 13

BlueZ:

https://packages.debian.org/trixie/bluez

Netpbm Development:

https://packages.debian.org/trixie/libnetpbm-dev

Triggerhappy:

https://packages.debian.org/trixie/triggerhappy

Evtest:

https://packages.debian.org/trixie/evtest

### Triggerhappy

https://github.com/wertarbyte/triggerhappy

### systemd

`logind.conf`:

https://www.freedesktop.org/software/systemd/man/logind.conf.html

`systemd-sleep.conf`:

https://www.freedesktop.org/software/systemd/man/systemd-sleep.conf.html

### Logitech

Aktuelle Logitech-Supportseite zu SetPoint; SetPoint wird nicht mehr gepflegt:

https://support.logi.com/hc/en-gb/articles/360025141274-SetPoint

Logitech-Kompatibilitätsübersicht, in der die MX5500 weiterhin als älteres SetPoint-Gerät geführt wird:

https://support.logi.com/hc/de/articles/360023353713-Kompatibilit%C3%A4t-von-Logitech-M%C3%A4usen-und-Tastaturen-mit-Windows-8-und-Windows-10

---

## 23. Verifizierungsstatus

### Praktisch im Senza-Projekt bestätigt

- direkte Bluetooth-Nutzung der MX5500 unter Linux;
- normale Tastaturfunktion;
- `mx5000-tool --time`;
- `mx5000-tool --keybdopts`;
- E-Mail-Icon;
- Mute-Icon;
- die beiden anderen Legacy-Icons werden ignoriert;
- `--name` wird ignoriert;
- freie LCD-Ansteuerung grundsätzlich nutzbar;
- Sleep-Taste erzeugt `KEY_SLEEP`;
- Host-Suspend über systemd erfolgreich deaktiviert;
- Gast-Suspend erfolgreich deaktiviert;
- `triggerhappy`-Reaktion auf `KEY_SLEEP`;
- gezielte `sudoers`-Freigabe für das Lock-Skript;
- Sleep-Taste sperrt damit die GNOME-Session der Desktop-VM.

### Vom Upstream-Quellcode unterstützt

- MX5500 USB-ID `046d:b30b`;
- MX5500 Bluetooth-ID `046d:b30b`;
- Uhrzeit/Datum;
- Temperatur-Einheit;
- vier Legacy-Icon-Felder;
- Keyboard-Optionen;
- Beep;
- Name;
- freie statische/rollende/scrollende LCD-Ausgabe;
- Fortschrittsbalken;
- PBM-Bilder;
- aktualisierbare Textfelder.

### Noch offen / nicht final verifiziert

- automatischer Bluetooth-Reconnect-Trigger für `--time`;
- originales Media-/Inbox-Menüprotokoll;
- sinnvolle komplette Belegung aller Sondertasten;
- eigener PVE-/AI-LCD-Statusdienst;
- Verhalten mit einem wiederbeschafften originalen USB-Empfänger im UEFI-Menü.
