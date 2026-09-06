# MX5000 / MX5500 references

**Recherche-Stand:** 6. September 2026

Diese Datei sammelt öffentlich auffindbare Quellen zur Logitech-Cordless-Desktop-Serie **MX5000** und **MX5500**, mit Schwerpunkt auf:

- den von Logitech/SetPoint vorgesehenen Tastatur- und LCD-Funktionen;
- bekannten USB-/Bluetooth-/HID-IDs;
- Host→Tastatur- und Tastatur→Host-Protokollnachrichten;
- freier LCD-Ausgabe;
- eingebautem Menüsystem;
- Batterie-, Zeit-, Datums-, Temperatur-, Icon- und Keyboard-Optionen;
- SetPoint-Ressourcen, die für weiteres Reverse Engineering relevant sind.

Die Liste ist eine **Best-Effort-Inventur der heute noch öffentlich auffindbaren Quellen**. Bei fast 20 Jahre alter Software sind einige Originalseiten verschwunden, und manche Informationen existieren nur noch in Quellcode-Repositories, Mirrors, Foren oder privaten Reverse-Engineering-Notizen. Sprachlich duplizierte Logitech-Supportseiten werden nicht vollständig wiederholt; bevorzugt wird jeweils eine stabile, inhaltlich identische Fassung.

## Quellenbewertung

Die Quellen werden in dieser Datei grob so eingeordnet:

- **A – Primärquelle:** Logitech, USB-IF, Bluetooth SIG, Linux-Kernel-Dokumentation oder unmittelbar ausführbarer Quellcode einer Implementierung.
- **B – Reverse Engineering / unabhängige Implementierung:** eigener Treiber, Sniffing, historische Testprogramme oder technische Analyse mit konkreten Bytes/Strukturen.
- **C – Sekundärquelle:** Handbuch-Mirror, Forum, Blog oder Installationsinventar; nützlich als Hinweis, aber nicht alleinige Grundlage für ein Protokollfeld.

---

# 1. Wichtigste Protokollquellen

## 1.1 `mx5000tools` – zentrale Linux-Implementierung

**Bewertung: A/B – wichtigste Referenz für bereits implementierte Host→Tastatur-Kommandos und freie LCD-Ausgabe.**

Projekt:

https://github.com/jwrdegoede/mx5000tools

README:

https://github.com/jwrdegoede/mx5000tools/blob/master/README

Das README dokumentiert die Aufteilung in `libmx5000` und `mx5000-tool`, den Zugriff über Linux HIDRAW und nennt ausdrücklich als noch offene Reverse-Engineering-Aufgabe:

- Kontrolle der Inhalte des eingebauten Menüsystems;
- weitere von SetPoint verwendete, aber noch unbekannte Funktionen.

### Geräteerkennung und Low-Level-Kommandos

https://github.com/jwrdegoede/mx5000tools/blob/master/libmx5000/mx5000.c

Diese Datei ist die wichtigste kompakte Übersicht über die bekannten kurzen Output-Reports. Aktueller Stand:

- MX5000 Wireless Product ID: `046d:b305`;
- MX5500 Wireless Product ID: `046d:b30b`;
- beide werden sowohl über direkte Bluetooth-Verbindung als auch im USB/HID-Proxy-Modus erkannt;
- HID-Report-ID `0x10`: 6 Byte Nutzdaten, also 7 Byte inklusive Report-ID;
- HID-Report-ID `0x11`: 19 Byte Nutzdaten, also 20 Byte inklusive Report-ID;
- HID-Report-ID `0x12`: 45 Byte Nutzdaten, also 46 Byte inklusive Report-ID.

Dort implementierte Kommandofamilien:

| Funktion | Report | wesentlicher Selector / Command |
|---|---:|---:|
| Keyboard-Optionen | `0x10` | `0x01` |
| Legacy-Icons | `0x11` | `0x11` |
| Temperatur-Einheit / Zeitformat-Feld | `0x10` | `0x30` |
| Uhrzeit | `0x10` | `0x31` |
| Tag/Monat/Wochentag | `0x10` | `0x32` |
| Jahr | `0x10` | `0x33` |
| Name | `0x11` | `0x34` |
| Beep | `0x10` | `0x50` |

### Freie LCD-Ausgabe und historischer Menücode

https://github.com/jwrdegoede/mx5000tools/blob/master/libmx5000/mx5000screencontent.c

Öffentlicher Header:

https://github.com/jwrdegoede/mx5000tools/blob/master/libmx5000/mx5000screencontent.h

Diese Implementierung dokumentiert die bislang beste öffentlich verfügbare Beschreibung des frei beschreibbaren LCD-Protokolls:

- statischer Text;
- `big` / `huge` Textmodi;
- rollender Text;
- dreizeiliges Scrolling;
- horizontale Linien;
- Fortschrittsbalken;
- eingebaute Glyphen/Icons;
- monochrome Bilder;
- referenzierte Textbereiche für spätere Updates;
- Reset zurück zum Firmware-Menü.

Für die Übertragung von Screen-Content wird eine längere Sequenz verwendet:

1. Initialisierung über `0x10`, Command `0xA1`;
2. 13 Datenreports mit Report-ID `0x12`;
3. erster Block mit `0x90`;
4. Zwischenblöcke mit `0x91`;
5. letzter Block mit `0x93`;
6. Abschluss über `0x11`, Command `0xA0`.

Besonders relevant für weiteres Reverse Engineering sind die im C-File vorhandenen, aber nicht im öffentlichen Header und nicht in `mx5000-tool` exponierten Funktionen:

- `mx5000_sc_add_menuline()`;
- `mx5000_sc_nextmenu()`;
- `mx5000_sc_send_menus()`.

Sie zeigen, dass bereits früher versucht wurde, Daten für das eingebaute Menüsystem zu erzeugen.

### Bildkonvertierung

https://github.com/jwrdegoede/mx5000tools/blob/master/libmx5000/mx5000image.c

https://github.com/jwrdegoede/mx5000tools/blob/master/libmx5000/mx5000image.h

Relevant für PBM→MX5000-Bitmap-Konvertierung und das interne Bildformat der freien LCD-Seiten.

### HIDRAW-Transport

https://github.com/jwrdegoede/mx5000tools/blob/master/libmx5000/hidraw_lib.c

https://github.com/jwrdegoede/mx5000tools/blob/master/libmx5000/hidraw_lib.h

Relevant für Geräteerkennung, Output-Reports und Wiederöffnen des Geräts nach Verbindungsverlust.

### Kommandozeilenfrontend

https://github.com/jwrdegoede/mx5000tools/blob/master/mx5000-tool/mx5000-tool.c

Zeigt exakt, welche Library-Funktionen vom normalen Tool erreichbar sind und welche nur in `libmx5000` existieren.

---

## 1.2 Historisches `mx5000tools`-Testprogramm – besonders wichtig für Menü-Reverse-Engineering

**Bewertung: B – experimenteller Upstream-Code; die dortigen Werte sind nicht als vollständig verstandenes Protokoll zu behandeln.**

Vor der Entfernung der alten Tests enthielt das Repository ein experimentelles Testprogramm:

https://github.com/jwrdegoede/mx5000tools/blob/5f67a71edafe57d4140c10c7ba966b16909de16b/tests/mx5000-test.c

Es enthält deaktivierte Experimente mit:

- `mx5000_sc_add_menuline()`;
- `mx5000_sc_nextmenu()`;
- `mx5000_sc_send_menus()`;
- mehreren 16-Zeichen-Menüzeilen;
- jeweils vier zusätzlichen, nur teilweise verstandenen Bytes;
- Beispiel-`pageid`-Werten `5` und `6`;
- Texten wie `BACK`, `ENTER`, `PLAY`;
- einer experimentellen `PLEASE WAIT`-Grafik.

Beispiele der historischen 4-Byte-Werte sind u. a.:

- `03 00 00 00`;
- `00 05 64 00`;
- `01 06 00 00`;
- `02 FF FF 00`;
- `2E FF FF 00`;
- `2F FF FF 00`;
- `30 FF FF 00`;
- `31 FF FF 00`.

Diese Werte sind **Testmaterial**, keine bestätigte Felddefinition.

Commit, mit dem Hans de Goede die alten Tests 2019 entfernte:

https://github.com/jwrdegoede/mx5000tools/commit/397d3f845c629921a84a7124f4f72a5543a6267f

Die Commit-Nachricht bezeichnet sie ausdrücklich als überwiegend experimentelle Anwendungen. Für unser Projekt sind sie dennoch wertvoll, weil sie den einzigen erhaltenen Upstream-Testpfad für `mx5000_sc_send_menus()` darstellen.

---

## 1.3 Wichtige `mx5000tools`-Commits zur MX5500-Unterstützung

**Bewertung: A – Quellcodehistorie.**

MX5500 USB-/Bluetooth-IDs hinzugefügt:

https://github.com/jwrdegoede/mx5000tools/commit/67a878731f1957e66102c82bae589c4814df8360

Verbesserte Übertragung von Sekunden, Wochentag und Jahr:

https://github.com/jwrdegoede/mx5000tools/commit/0ef8887ee62cd07127f327074b5b1b847a26789b

Umstellung/Erweiterung des HIDRAW-Zugriffs und automatisches Wiederöffnen nach Verbindungsverlust:

https://github.com/jwrdegoede/mx5000tools/commit/e23fa7e30fd34d0e745f48be0e4720742caa4d7d

Verwendung der Wireless Product IDs auch für den HID-Proxy-Modus:

https://github.com/jwrdegoede/mx5000tools/commit/c575ea33f92495b4b0ccdb1ce09099f9c011e43f

Der letzte Punkt ist wichtig für die Interpretation von USB-IDs: `b305` bzw. `b30b` repräsentieren im HID-Proxy-Modus das Gerät **hinter** dem Empfänger; sie sind nicht die physische USB-PID des Receivers selbst.

Fedora-Paketierung des aktuellen Upstream-Stands:

https://packages.fedoraproject.org/pkgs/mx5000tools/mx5000tools

---

# 2. Ursprüngliche `mx5000lib`-Quelle

## 2.1 MX5000 Library .NET

**Bewertung: A/B – historischer Ursprung eines Teils des Wissens in `mx5000tools`.**

Projektseite:

https://sourceforge.net/projects/mx5000lib/

Dateien:

https://sourceforge.net/projects/mx5000lib/files/

Quellcode-Archiv:

https://sourceforge.net/projects/mx5000lib/files/mx5000lib-src/

SourceForge beschreibt die Bibliothek als C#/.NET-Library, die:

- gedrückte Tasten lesen kann;
- eigene Inhalte auf das Display schreiben kann;
- einige Tastaturoptionen ändern kann.

`mx5000tools` nennt diese Arbeit von **E. Heidstra / boesj** ausdrücklich als Grundlage.

Die letzten öffentlich angebotenen Dateien umfassen `mx5000lib-0.1.4.zip`; die Quellcode-Verzeichnisse reichen bis in das Jahr 2006 zurück.

SourceForge-Diskussion:

https://sourceforge.net/p/mx5000lib/discussion/

Feature Requests:

https://sourceforge.net/p/mx5000lib/feature-requests/

---

# 3. MX5500-spezifische Userspace-Implementierung

## 3.1 `mx5500-set`

**Bewertung: B – eigenständige, MX5500-spezifische HIDRAW-Implementierung; bestätigt mehrere Kommandos unabhängig von `mx5000tools`.**

Repository:

https://github.com/jsparber/mx5500-set

README:

https://github.com/jsparber/mx5500-set/blob/master/README.md

Quellcode:

https://github.com/jsparber/mx5500-set/blob/master/mx5500.c

Header:

https://github.com/jsparber/mx5500-set/blob/master/mx5500.h

Das Projekt nennt als Grundlage sowohl `mx5000lib` als auch `mx5000tools`, richtet sich aber ausdrücklich an die MX5500 über HIDRAW.

Besonders nützlich sind direkte 7-Byte-Reports im Quellcode, z. B.:

- Beep über Report `0x10`, Selector `0x50`;
- Temperatur-/Zeitformat über Selector `0x30`;
- `0x00` Celsius / `0x01` Fahrenheit;
- Kommentar: `0xDC` für 24-Stunden- und `0xDB` für 12-Stunden-Anzeige;
- Uhrzeit über `0x31`;
- Datum über `0x32`;
- Jahr über `0x33`.

Das Projekt verwendet außerdem die HIDRAW-ioctls `HIDIOCGRAWNAME`, `HIDIOCGRAWPHYS` und `HIDIOCGRAWINFO`.

Im README verbleiben ausdrücklich als TODO:

- weitere LCD-Seiten/Funktionen;
- Lesen des Taschenrechners.

---

# 4. MX5500-Kernel-Reverse-Engineering: `linux-hid-lg-extended`

## 4.1 Projektübersicht

**Bewertung: B – eine der wertvollsten Quellen für Tastatur→Host-Antworten und READ-Kommandos.**

Repository:

https://github.com/RobertMe/linux-hid-lg-extended

README:

https://github.com/RobertMe/linux-hid-lg-extended/blob/master/README

Der Treiber dokumentiert für die MX5500 folgende Sysfs-Funktionen:

- Batteriestand in Prozent lesen;
- aktuell gewählte LCD-Seitennummer lesen;
- Uhrzeit lesen und schreiben;
- Datum lesen und schreiben;
- Gerätename lesen.

Damit ergänzt er `mx5000tools`, das fast ausschließlich Host→Tastatur-Kommandos verwendet, um Query-/Response- und Event-Pfade.

## 4.2 Keyboard-Protokoll

https://github.com/RobertMe/linux-hid-lg-extended/blob/master/src/hid-lg-mx5500-keyboard.c

Besonders relevante Protokollinformationen:

| Funktion | Report / Action | Selector | Auswertung |
|---|---|---:|---|
| Batterie abfragen | `0x10`, GET | `0x0d` | Batteriewert aus Antwort |
| Zeit abfragen | `0x10`, GET | `0x31` | Stunde/Minute/Sekunde |
| Zeit setzen | `0x10`, SET | `0x31` | Sekunde/Minute/Stunde |
| Datum abfragen | `0x10`, GET | `0x32` | Tag/Monat |
| Jahr abfragen | `0x10`, GET | `0x33` | Jahr |
| Datum setzen | `0x10`, SET | `0x32`, `0x33` | Tag/Monat/Jahr |
| LCD-Seitenwechsel | Event-Action `0x0b` | `0x00` | neue Seite aus Payload |

Die Datei ist außerdem wichtig, weil sie zeigt, dass sich Seitenwechsel der eingebauten Firmware als eingehendes Ereignis beobachten lassen. Das ist ein sinnvoller Ansatzpunkt für das Reverse Engineering von Media-, Inbox- und Favorites-Seiten.

## 4.3 Geräte-IDs

https://github.com/RobertMe/linux-hid-lg-extended/blob/master/src/hid-lg-mx5500.h

Dort stehen:

- MX5500 Receiver: `046d:c71c`;
- MX5500 Keyboard: `046d:b30b`;
- MX Revolution: `046d:b007`.

Bind-Hilfsskript:

https://github.com/RobertMe/linux-hid-lg-extended/blob/master/tools/lg-bind

Es unterscheidet ausdrücklich USB-Bus (`0003`) und Bluetooth-Bus (`0005`).

Receiver-Code:

https://github.com/RobertMe/linux-hid-lg-extended/blob/master/src/hid-lg-mx5500-receiver.c

MX-Revolution-Code aus demselben Desktop-Set:

https://github.com/RobertMe/linux-hid-lg-extended/blob/master/src/hid-lg-mx-revolution.c

---

# 5. Aktueller Linux-Kernel

## 5.1 `hid-logitech-hidpp`

**Bewertung: A – aktueller Linux-Kernel.**

https://github.com/torvalds/linux/blob/master/drivers/hid/hid-logitech-hidpp.c

Der aktuelle Kernel enthält explizite Geräte-Einträge für:

- MX5000 im HID-Proxy-Modus: Wireless PID `0xb305`;
- MX5000 direkt über Bluetooth: `0xb305`;
- MX5500 im HID-Proxy-Modus: Wireless PID `0xb30b`;
- MX5500 direkt über Bluetooth: `0xb30b`.

Für beide Tastaturen wird der Quirk `HIDPP_QUIRK_HIDPP_CONSUMER_VENDOR_KEYS` verwendet.

Das ist vor allem für Geräteerkennung und Sondertasten relevant. Es ist **keine** vollständige Implementierung der LCD-/SetPoint-Funktionen.

Alternative aktuelle Kernel-Ansicht:

https://kernel.googlesource.com/pub/scm/linux/kernel/git/torvalds/linux/+/master/drivers/hid/hid-logitech-hidpp.c

---

# 6. LCDproc als reale Anwendung von `libmx5000`

## 6.1 LCDproc MX5000-Treiber

**Bewertung: A/B – unabhängige reale Nutzung des freien LCD-Protokolls.**

Projekt:

https://github.com/lcdproc/lcdproc

Treiber:

https://github.com/lcdproc/lcdproc/blob/master/server/drivers/mx5000.c

Build-Erkennung von `libmx5000`:

https://github.com/lcdproc/lcdproc/blob/master/acinclude.m4

Hardware-Liste:

https://lcdproc.org/hardware.php3

LCDproc nennt die MX5000 ausdrücklich als unterstütztes Display und benötigt dafür `mx5000tools`.

Der Treiber zeigt praktische Annahmen/Erfahrungen:

- Behandlung als 16×4-Zeichenanzeige für LCDproc;
- Textausgabe über `mx5000_sc_add_text()`;
- Fortschrittsbalken;
- mehrere eingebaute Icons;
- große Ziffern über `STATICHUGE`;
- Backlight wird nicht unterstützt;
- bei Refresh wird der komplette Screen neu gesendet;
- Kommentar, dass das Display auf schnelle aufeinanderfolgende Updates empfindlich/langsam reagiert;
- `mx5000_reset()` beim Schließen.

LCDproc 0.5.3 nennt den MX5000-Treiber als damals neu hinzugefügten Treiber:

https://lcdproc.org/download.php3

---

# 7. Neuere unabhängige Bluetooth-HID-Sniffs der MX5000

## 7.1 RandomInsano, 2025: “Hacking a Logitech MX 5000 keyboard via Bluetooth HID”

**Bewertung: B – sehr wertvoller moderner Mitschnitt, aber ausschließlich MX5000 und keine offizielle Spezifikation.**

https://gist.github.com/RandomInsano/55e6af2092f7976bc83ff96491e64753

Die Analyse entstand aus Bluetooth/Wireshark-Sniffs und bestätigt bzw. erweitert mehrere ältere Erkenntnisse.

Genannte Report-/Protocol-Codes:

- `0x01`: normale Tastatur-HID-Daten;
- `0x10`: Sondertasten/Settings und weitere kurze Kommandos;
- `0x11`: Username/Name;
- `0x12`: Display-/Textdaten.

Beobachtete Display-Sequenz:

- Tastatur meldet `10 00 0b 02 ...`;
- Host antwortet mit Init-Command `... 80 a1 ...`;
- anschließend `0x12`-Datenblöcke mit `0x90`, `0x91`, `0x93`.

Das deckt sich strukturell sehr gut mit `mx5000screencontent.c`.

Weitere besonders interessante Beobachtungen:

- Menüauswahl erzeugt `0x0b`-Ereignisse;
- unterschiedliche Werte wurden für Bluetooth, Home, Control Panel, SetPoint, Playlist und Radio beobachtet;
- Username wird über `0x11 / 0x82 / 0x34` übertragen;
- Keyboard-/Touch-/Beep-Optionen verwenden Selector `0x01`;
- Temperatur-Einstellung verwendet Selector `0x30`;
- Beep verwendet Selector `0x50`;
- diverse Sondertasten wurden mit HID-Usage-Werten korreliert.

Wichtig: Die MX5000- und MX5500-Firmware ist verwandt, aber nicht identisch. Ein auf der MX5000 beobachtetes Feld ist daher ein **Testkandidat**, kein Beweis für identisches MX5500-Verhalten.

---

# 8. Offizielle Logitech-Quellen – MX5000

## 8.1 Produktankündigung 2005

**Bewertung: A – beste offizielle Funktionsübersicht des eingebauten MX5000-LCDs.**

https://ir.logitech.com/press-releases/press-release-details/2005/Keyboard-LCD-a-Smart-Addition-to-Logitech-Cordless-Desktop-MX-5000-Laser/default.aspx

Logitech beschreibt dort unter anderem:

- Default/Dashboard-Seite mit Benutzername, Raumtemperatur, Caps Lock, F-Lock, Batterie, Uhrzeit und Datum;
- automatische Zeit-/Datumssynchronisation mit dem PC;
- Main Menu mit Smart-Key-Zuweisungen, Playlists und Internet-Radio;
- vier programmierbare Smart Keys F9–F12;
- Navigation über vier Tasten unterhalb des LCDs;
- Medieninformationen und Mediensteuerung;
- E-Mail- und Instant-Messaging-Benachrichtigungen;
- Anzeige von Lautstärke/Mute;
- Bluetooth-Verbindung zwischen Keyboard und PC.

## 8.2 Technische Daten

https://support.logi.com/hc/en-us/articles/360023466293-MX-5000-Technical-Specifications

Relevant:

- Bluetooth 2.0;
- Verbindung über USB oder Bluetooth;
- SetPoint als Software;
- LCD mit Batterie-/Caps-Anzeige;
- Receiver unterstützt HCI und HID;
- physische Receiver-PIDs `046d:c70a` und `046d:c70e`.

Die physischen Receiver-PIDs dürfen nicht mit der Wireless PID `b305` verwechselt werden, die Linux im HID-Proxy-Modus für das Keyboard sehen kann.

## 8.3 Uhrzeit und Datum

https://support.logi.com/hc/en-001/articles/360023213214-Setting-the-time-and-date-on-my-MX-5000-keyboard-display

Offizielle Bestätigung, dass SetPoint Zeit und Datum des Keyboards mit dem Host synchronisiert.

## 8.4 Event-/Beep-Signale

https://support.logi.com/hc/en-ca/articles/360023213574-Using-MX-5000-event-audio-signals

Relevant für die Interpretation der in `mx5000tools` vorhandenen Keyboard-/Beep-Optionen.

## 8.5 SetPoint 3.30 BT / Firmware

https://support.logi.com/hc/en-gb/articles/360025279253-SetPoint

Logitech nennt dort als Änderungen:

- Firmware-Upgrade für MX5000-/diNovo-Media-Desktop-Laser-Receiver;
- 24-Stunden-Uhr-Fix für europäische MX5000-Sets.

## 8.6 Offizieller Getting-Started-Guide

https://support.logi.com/hc/en-my/articles/360024843114-Getting-Started-Guide-PDF-Cordless-Desktop-MX-5000-Laser

---

# 9. Offizielle Logitech-Quellen – MX5500

## 9.1 Produktankündigung 2008

**Bewertung: A – beste offizielle Übersicht über die Firmware-Seiten der MX5500.**

https://ir.logitech.com/press-releases/press-release-details/2008/New-Logitech-Premium-Bluetooth-Keyboard-and-Mouse-Combination-Features-Flagship-MX-Revolution-Mouse/default.aspx

Logitech nennt für das dynamische LCD ausdrücklich:

- Uhrzeit und Datum;
- Taschenrechner;
- Temperatur;
- Website-Favoriten;
- Tastenzähler;
- E-Mail-Updates;
- Media-Informationen wie Band-/Artist- und Songtitel.

Außerdem: Bluetooth 2.0 EDR.

## 9.2 Produkt-Supportseite

https://support.logi.com/hc/de/articles/360024642314-Cordless-Desktop-MX-5500-Revolution

Nützlich als offizielle Produkt-/Modellreferenz.

## 9.3 Offizieller Getting-Started-Guide

https://support.logi.com/hc/en-150/articles/360024842674-Getting-Started-Guide-PDF-Cordless-Desktop-MX-5500-Revolution

Der Quick-Start-Guide zeigt die LCD-Funktionen, Sondertasten und SetPoint-Abhängigkeiten.

Sekundärer HTML-Mirror des Handbuchs:

https://www.manualslib.com/manual/405661/Logitech-5500-Cordless-Desktop-Mx-Revolution-Wireless-Keyboard.html

Dort sind u. a. direkt lesbar:

- Media-Seite: Artist, Songtitel und Zeit/Timing;
- Temperatur: Fahrenheit/Celsius;
- Favorites-Zuweisungen;
- E-Mail-Anzeige;
- Taschenrechner.

## 9.4 Uhrzeit und Datum

https://support.logi.com/hc/en-001/articles/360023397533-Changing-the-time-and-date-on-the-LCD-display-of-the-MX5500-keyboard

Offizielle Aussage: Nach Installation von SetPoint wird die LCD-Uhr automatisch mit der Rechnerzeit synchronisiert.

## 9.5 Temperatur-Einheit

https://support.logi.com/hc/en-my/articles/360023398153-Changing-the-temperature-on-the-LCD-display-of-the-MX5500-keyboard

Offizielle SetPoint-Funktion zum Umschalten Celsius/Fahrenheit.

## 9.6 Favorites F9–F12

https://support.logi.com/hc/en-gb/articles/360023234414-Programming-the-Favorites-keys-F9-F12-on-the-MX5500-keyboard

Besonders relevant für das Menüprotokoll: Beim Programmieren zeigt das LCD ausdrücklich `Please wait`. Die Tasten F9–F12 sind als Favorites A–D vorgesehen.

## 9.7 FN-Funktionen

https://support.logi.com/hc/en-in/articles/360023233534-Using-the-FN-key-on-my-MX5500-keyboard

Dokumentiert die SetPoint-konfigurierbaren Enhanced-Funktionen der F-Tasten.

## 9.8 Caps Lock / Num Lock

https://support.logi.com/hc/en-150/articles/360023397333-MX-5500-Num-Lock-and-Caps-Lock-indicators

Offizielle Aussage:

- Caps-Lock-Status wird im LCD angezeigt;
- kein klassischer Num-Lock-Modus, Nummernblock ist grundsätzlich aktiv.

## 9.9 Direkte Bluetooth-Kopplung

https://support.logi.com/hc/de/articles/360023234434-Verbinden-des-Cordless-Desktop-MX-5500-mit-einem-Bluetooth-f%C3%A4higen-PC-oder-Notebook

Bestätigt, dass Keyboard und MX-Revolution-Maus auch mit einem integrierten Bluetooth-Funkgerät gekoppelt werden können und beschreibt den Discoverable-/Pairing-Modus.

## 9.10 Ersatzempfänger

https://support.logi.com/hc/en-sg/articles/360023398673-Purchasing-a-replacement-USB-receiver-for-my-MX5500

Logitech nennt den MX5500-Mini-Receiver mit P/N `830-000021`.

---

# 10. SetPoint-Ressourcen und Windows-Seite

Für das noch fehlende native Media-/Inbox-/Favorites-Protokoll sind SetPoint-Ressourcen besonders interessant. Es wurde keine öffentliche offizielle Logitech-Paketspezifikation für das LCD-Protokoll gefunden; deshalb sind die folgenden Quellen Indizien für die Softwarearchitektur.

## 10.1 MX5500-LCD-Customization / `MX5500.xml`

**Bewertung: C/B – detaillierte, reproduzierbare SetPoint-Dateipfade und Konfigurationsnamen; kein vollständiges Protokoll.**

https://tnm2.wordpress.com/english-section/logitech-mx-5500/

Die Seite dokumentiert u. a.:

- `%ProgramData%\LogiShrd\SetPointP\Devices\Display\400000A\MX5500.xml`;
- ältere Logitech-Pfade derselben Datei;
- `ADHIL_PRIMARYLANG_ID`;
- `ADHIL_TIME_FORMAT`;
- `MX5500-flash.bin` im Benutzerprofil;
- HBMP-Ressourcen unter `...\400000A\hbmps`;
- dass `PLEASE WAIT` aus der XML-/Display-Konfiguration beeinflusst wird;
- Hinweise darauf, dass SetPoint Teile der Display-Konfiguration/Strings in eine Flash-/Cache-Datei überführt.

Für Reverse Engineering interessant: Der interne Name `ADHIL` taucht auch in Windows-Komponenten rund um die MX5500 auf.

## 10.2 Unabhängige 24-Stunden-Uhr-Analyse

https://dereenigne.org/windows/logitech-mx5500-24-hour-lcd-clock/

Bestätigt unabhängig den Pfad zu `MX5500.xml` und Konfigurationsfelder wie:

- `ADHIL_PRIMARYLANG_ID`;
- `ADHIL_LEAD_0_TIME`;
- `ADHIL_TIME_FORMAT`.

## 10.3 MX5000-/MX5500-XML-Pfade und Zeitformat

**Bewertung: C.**

https://ru-board.club/computers/hardware/drivers-bios/292939-39.html

Enthält historische Hinweise auf:

- MX5000: `...\Devices\Display\4000006`;
- `MX5000_PRIMARYLANG_ID`;
- `MX5000_LEAD_0_TIME`;
- `MX5000_TIME_FORMAT`;
- `MX5000_START_POS`;
- MX5500: `...\Devices\Display\400000A\MX5500.xml`.

## 10.4 SetPoint-Dateiinventar aus einer Sandbox-Analyse

**Bewertung: C – kein Logitech-Dokument, aber nützlich zur Identifikation historischer SetPoint-Ressourcen.**

https://www.hybrid-analysis.com/sample/a0b9e2477f0bdc32f69e8695a87115c5b5d58789947f89c0a34d28bba624cda9/5d5c55420388386e7e4f7184

Für SetPoint 6.69 werden dort unter anderem tatsächlich installierte Dateien aufgelistet:

- `...\Devices\Display\400000A\MX5500.xml`;
- Dateigröße 10839 Byte;
- SHA-256 `cf1f0b900d420a16321bc7bcdaf6325afe6c813ed969a4e32367d956f4d41365`;
- `chartable_CP12501.bin`;
- weitere Ressourcen unter dem Display-Verzeichnis.

Das ist nützlich, um historische SetPoint-Pakete oder archivierte Dateien eindeutig zu identifizieren.

## 10.5 SetPoint-DLL-Inventar

**Bewertung: C – Sekundärinventar.**

https://www.shouldiremoveit.com/setpoint-15352-program.aspx

Genannte Komponenten umfassen unter anderem:

- `mx5000.dll` – Logitech MX5000 Support;
- `MX5500.dll` – Logitech Adhil Support;
- `SetPointCOMWMP9.dll` – Windows Media Player Support Server;
- `SetPointCOMMM9.dll` – MusicMatch Support Server;
- `MessengerHook.dll`;
- `IMHook.dll`;
- `KEMMAPI.dll` für Mail-/Outlook-Integration.

Diese Dateinamen passen zur bekannten SetPoint-Funktionalität: Media-Metadaten und Mail/IM-Status werden von PC-seitigen Komponenten eingesammelt und anschließend an die Tastatur übertragen.

## 10.6 Beobachtung zur Media-Seite unter Windows

**Bewertung: C – Anwenderbericht, aber nützlich zur Eingrenzung der Datenquelle.**

https://mediamonkey.com/forum/posting.php?mode=quote&p=124465

Ein Anwender berichtet 2008, dass Windows Media Player über SetPoint die Songinformationen korrekt an das MX5500-LCD liefert, während MediaMonkey trotz funktionierender Multimedia-Tasten keine LCD-Daten liefert. Das unterstützt die Annahme, dass die native Media-Seite über einen SetPoint-/Player-Plugin-Pfad versorgt wird und nicht allein aus Standard-Media-Key-Events entsteht.

---

# 11. HID-/Bluetooth-Spezifikationen zum Einordnen der Reports

Diese Quellen beschreiben **nicht** Logitechs proprietäre LCD-Payloads, aber den Transport und Standard-HID-Usages, auf denen die Tastatur aufbaut.

## 11.1 Linux HIDRAW

**Bewertung: A.**

https://docs.kernel.org/hid/hidraw.html

Erklärt:

- Rohzugriff auf USB- und Bluetooth-HID-Geräte;
- `read()` / `write()` von HID-Reports;
- dass bei nummerierten Reports das erste Byte die Report-ID ist;
- `HIDIOCGRAWINFO`, `HIDIOCGRAWNAME`, Descriptor-Zugriff usw.;
- warum HIDRAW für Userspace-Treiber proprietärer HID-Funktionen geeignet ist.

Das ist die Transportgrundlage von aktuellem `mx5000tools` und `mx5500-set`.

## 11.2 USB HID Device Class

USB-IF HID-Dokumentbibliothek:

https://www.usb.org/documents?search=hid

Enthält u. a. die HID Device Class Definition 1.11.

HID-Übersicht und aktuelle Usage-Tabellen:

https://www.usb.org/hid

## 11.3 HID Usage Tables

Aktuelle USB-IF-HID-Seite:

https://www.usb.org/hid

Die 2025er MX5000-Sniffing-Analyse verwendete außerdem die HID Usage Tables 1.22 zur Zuordnung von Sondertasten:

https://usb.org/sites/default/files/hut1_22.pdf

Für neue Zuordnungen sollte die jeweils aktuelle USB-IF-Version verwendet werden.

## 11.4 Bluetooth HID Profile

Bluetooth HID Profile 1.0:

https://www.bluetooth.com/specifications/specs/human-interface-device-profile-1-0/

Bluetooth HID Profile 1.1.1:

https://www.bluetooth.com/specifications/specs/human-interface-device-profile-1-1-1/

Aktuelle Bluetooth-HID-Spezifikationsübersicht:

https://www.bluetooth.com/specifications/specs/

Die Profile beschreiben HID über klassisches Bluetooth/L2CAP. Das ist von HID over GATT/Bluetooth Low Energy zu unterscheiden; MX5000/MX5500 stammen aus der klassischen Bluetooth-HID-Generation.

---

# 12. Protokoll-Kurzindex mit Quellen

Dieser Abschnitt ist **keine neue Spezifikation**, sondern ein Index zu Feldern, die in mehreren Quellen wiederkehren.

| Wert | bekannte Bedeutung | wichtigste Quellen |
|---:|---|---|
| Report `0x10` | kurze Settings/Status-/Control-Reports | `mx5000.c`, `mx5500-set`, `linux-hid-lg-extended`, 2025 Gist |
| Report `0x11` | längerer Control-/Name-/Icon-Report | `mx5000.c`, 2025 Gist |
| Report `0x12` | LCD-Datenblöcke | `mx5000screencontent.c`, 2025 Gist |
| `0x01` | Keyboard-/Beep-/Media-Key-Options | `mx5000.c`, 2025 Gist |
| `0x0b` | LCD-/Menü-Seitenereignis | `linux-hid-lg-extended`, 2025 Gist |
| `0x0d` | Batterie-Query | `linux-hid-lg-extended` |
| `0x11` | Legacy-Icon-Block | `mx5000.c` |
| `0x30` | Temperatur-Einheit / Zeitformat-Umfeld | `mx5000.c`, `mx5500-set`, 2025 Gist |
| `0x31` | Uhrzeit | `mx5000.c`, `mx5500-set`, `linux-hid-lg-extended` |
| `0x32` | Tag/Monat/Wochentag | dieselben Quellen |
| `0x33` | Jahr | dieselben Quellen |
| `0x34` | Username/Name | `mx5000.c`, 2025 Gist |
| `0x50` | Beep | `mx5000.c`, `mx5500-set`, 2025 Gist |
| `0x90` | erster LCD-Datenblock | `mx5000screencontent.c`, 2025 Gist |
| `0x91` | mittlerer LCD-Datenblock | `mx5000screencontent.c`, 2025 Gist |
| `0x93` | letzter LCD-Datenblock | `mx5000screencontent.c`, 2025 Gist |
| `0xA0` | LCD-Transferabschluss | `mx5000screencontent.c` |
| `0xA1` | LCD-Transferinitialisierung | `mx5000screencontent.c`, 2025 Gist |

Bei `0x80`, `0x81`, `0x82` handelt es sich in mehreren Implementierungen offenbar um Action-/Request-Richtungs- bzw. Command-Varianten. Ihre genaue Semantik sollte pro Nachricht aus den konkreten Implementierungen abgeleitet und nicht pauschal verallgemeinert werden.

---

# 13. Geräte-/Receiver-IDs

## Tastatur-Wireless-PIDs

| Gerät | Vendor | Product | Quellen |
|---|---:|---:|---|
| MX5000 Keyboard | `046d` | `b305` | `mx5000tools`, aktueller Linux-Kernel |
| MX5500 Keyboard | `046d` | `b30b` | `mx5000tools`, `linux-hid-lg-extended`, aktueller Linux-Kernel |
| MX Revolution Mouse | `046d` | `b007` | `linux-hid-lg-extended` |

## Physische Receiver

| Set | PID / Kennung | Quelle |
|---|---|---|
| MX5000 | `046d:c70a`, `046d:c70e` | Logitech Technical Specifications |
| MX5500 | `046d:c71c` | `linux-hid-lg-extended`, frühere `mx5000tools`-Geräteerkennung |
| MX5500 Ersatzteil | P/N `830-000021` | Logitech Support |

Wichtig: Im HID-Proxy-Modus kann Linux die **Wireless Product ID des Keyboards** (`b305`/`b30b`) sehen, obwohl physisch ein Receiver mit einer anderen USB-PID steckt. Der Upstream-Commit `c575ea3...` stellt `mx5000tools` explizit auf dieses Modell um.

---

# 14. Was öffentlich weiterhin nicht vollständig dokumentiert ist

Trotz der oben genannten Quellen wurde **keine vollständige öffentliche Logitech-Spezifikation** des MX5000-/MX5500-LCD-Protokolls gefunden.

Insbesondere fehlen weiterhin belastbare Feldbeschreibungen für:

- den vollständigen Aufbau des eingebauten Firmware-Menüs;
- die native MX5500-Media-Seite einschließlich Artist/Song/Timing;
- die native MX5500-Inbox-Seite einschließlich Unread-Counter bzw. Maildaten;
- die native Favorites-Seite;
- die Semantik der vier Zusatzbytes pro Zeile im alten `mx5000_sc_add_menuline()`-Testcode;
- die genaue Bedeutung von `pageid` und `first` in `mx5000_sc_send_menus()`;
- alle MX5500-spezifischen festen LCD-Icons;
- das Format der SetPoint-`*.hbmp`-Ressourcen;
- den genauen Inhalt/ Aufbau von `MX5500-flash.bin`;
- eine offizielle Zuordnung der SetPoint-DLL-Aufrufe zu den HID-Reports.

Das Upstream-README von `mx5000tools` bestätigt explizit, dass die Kontrolle des Menüinhalts noch reverse-engineert werden müsste.

---

# 15. Priorität der Quellen für weiteres Reverse Engineering

Für die Nachimplementierung sollten die Quellen in dieser Reihenfolge verwendet werden:

1. **`libmx5000/mx5000.c`** – bestätigte kurze Output-Kommandos.
2. **`libmx5000/mx5000screencontent.c`** – LCD-Transport und alter Menüpfad.
3. **historisches `tests/mx5000-test.c`** – konkrete experimentelle Menüwerte.
4. **`linux-hid-lg-extended`** – GET-Kommandos, Antworten und LCD-Seitenwechsel-Events der MX5500.
5. **`mx5500-set`** – unabhängige Bestätigung der MX5500-Settings und 12/24h-Feldhinweise.
6. **2025er Bluetooth-Sniff der MX5000** – moderne Paketmitschnitte zur Korrelation mit dem alten Code.
7. **Logitech-Dokumentation** – Soll-Funktion und sichtbares Verhalten der Firmware-Seiten.
8. **SetPoint XML-/DLL-/Flash-Ressourcen** – Ansatzpunkt zur Rekonstruktion der noch fehlenden Media-/Inbox-/Favorites-Daten.
9. **Foren/Blogs** – nur zur Hypothesenbildung, nicht als alleinige Protokollquelle.

---

# 16. Historische Mirrors und ergänzende Quellen

Historischer Git-Mirror von `mx5000tools`:

https://www.repo.or.cz/mx5000tools.mirror.git

Russischer historischer Linux-Artikel zur Nutzung der MX5500 mit `mx5000tools`:

https://stoplinux.org.ru/linux/kak_v_kamennom_veke.html

LCDproc-Hardwareübersicht:

https://lcdproc.org/hardware.php3

ManualsLib MX5500 Quick Start Mirror:

https://www.manualslib.com/manual/405661/Logitech-5500-Cordless-Desktop-Mx-Revolution-Wireless-Keyboard.html

Manualzz MX5500 User-Manual-Mirror:

https://manualzz.com/doc/63039513/logitech-mx-5500-revolution--mx-5500-revolution-keyboard-...

Diese Mirrors sind nützlich, falls alte Herstellerdownloads verschwinden; für technische Aussagen sind die Original-Logitech-Seiten oder Quellcodequellen vorzuziehen.

---

# 17. Projektinterne Dokumente

Aktueller praktisch verifizierter MX5500-Linux-Status:

[`mx5500-status.md`](mx5500-status.md)

Geplante Reverse-Engineering- und Implementierungsschritte:

[`TODO.md`](TODO.md)

Experimentelles Testprogramm für Library-Funktionen, die in `mx5000-tool` fehlen:

[`../tests/mx5000-lib-test.c`](../tests/mx5000-lib-test.c)

Bei neuen Erkenntnissen sollte diese Datei um die **konkrete Ursprungsquelle** ergänzt werden, bevor ein bislang unbekanntes Feld als bestätigte Protokolleigenschaft dokumentiert wird.
