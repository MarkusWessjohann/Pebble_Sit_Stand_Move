# Sit Stand Move – Kurzbeschreibung

## Projektbeschreibung

**Sit Stand Move** ist eine native Pebble-Watch-App, die Nutzerinnen und Nutzer durch wiederkehrende Sitz-, Steh- und Bewegungsphasen begleitet und sie direkt auf der Uhr steuern lässt. Die App verwaltet Sitzungen auch ohne Telefonverbindung, bietet Pausen, Phasensteuerung und einstellbare Dauer- und Vibrationsoptionen; optional kann sie den aktuellen Phasenabschluss über eine Telefon-Bridge an Pebble Timeline melden.

## Eingesetzte Technologien

- **Watch-App:** C mit Pebble SDK, Pebble-Wakeup-API und nativer Pebble-Benutzeroberfläche.
- **Optionale Telefon-Bridge:** PebbleKit JS; veröffentlicht bei entsprechender Konfiguration einen informativen Pebble-Timeline-Pin.
- **Build- und Entwicklungswerkzeuge:** Pebble Tool 5.0.40, Python 3.13.5 und Pebble C SDK 4.33.1; Abhängigkeiten werden mit `uv` festgeschrieben.
- **Tests:** C-Tests des Sitzungskoordinators mit Pebble-API-Stubs sowie Python-Tests für den Build-Wrapper.
- **Zielplattformen:** Pebble Time 2 (`emery`), Pebble Round 2 (`gabbro`), Pebble 2 Duo (`flint`) und ältere Pebble-Plattformen.

## Epics und User Stories

| Epic | Ziel | User Stories |
|---|---|---|
| **1 – Sitzung auf der Uhr ausführen und steuern** *(abgeschlossen)* | Einen zuverlässigen Sit–Stand–Move-Zyklus direkt auf der Uhr starten, steuern und nach Unterbrechungen fortsetzen. | **1.1** Pebble-Toolchain und Emulator vorbereiten; **1.2** Sitzung auf der Uhr starten und beenden; **1.3** Zyklus ausführen und verpasste Fristen aufholen; **1.4** Sitzung manuell pausieren und fortsetzen; **1.5** zeitgesteuerte Pause nehmen und beenden; **1.6** aktive Phase überspringen; **1.7** aktive Phase verlängern; **1.8** fertige App auf Pebble Time 2 vor dem Release prüfen. |
| **2 – Sitzungsansicht auf einen Blick lesbar machen** *(abgeschlossen)* | Uhrzeit, Tageszähler und Tastenfunktionen besser lesbar darstellen. | **2.1** Uhrzeit und Zähler vergrößern und Tastenbeschriftungen neben den Tasten anzeigen. |
| **3 – Benachrichtigungsmuster wählen** *(abgeschlossen)* | Die Vibrationsart über die Uhreinstellungen auswählen und bei Bedarf bis zur Bestätigung wiederholen lassen. | **3.1** Vibrationsmuster in den Einstellungen auswählen; **3.2** Alarm bis zum Drücken von Select wiederholen. |
| **4 – Dauer der Phasen wählen** *(abgeschlossen)* | Länge der Sitz-, Steh- und Bewegungsphasen konfigurieren und auf Standardwerte zurücksetzen. | **4.1** Dauer von Sit, Stand und Move in den Einstellungen festlegen; **4.2** Phasendauern auf 45/10/5 Minuten zurücksetzen. |

## Zeitleiste der Projektaktivitäten

| Zeitraum | Aktivitäten und Ergebnisse |
|---|---|
| **23.–25.10.2026** | Projektplanung mit PRD, Architektur und UX begonnen; Pebble-Projektgerüst und Build-Werkzeuge ergänzt. |
| **31.10.–01.11.2026** | Emulator- und Wakeup-Diagnose vorbereitet, Build- und SDK-Versionen festgelegt und den Sitzungsablauf implementiert: Sitzungen starten/beenden, Phasenübergänge und Aufholen verpasster Fristen. |
| **02.11.2026** | Manuelles Pausieren/Fortsetzen und zeitgesteuerte Pausen ergänzt; Emulator- und Toolchain-Nachweise dokumentiert. |
| **03.11.2026** | Überspringen und Verlängern von Phasen sowie verbesserte Anzeige umgesetzt; Vibrationsauswahl und wiederholbarer Alarm ergänzt; Einstellungen für Phasendauern samt Rücksetzen auf 45/10/5 erstellt. Geräteabnahme-Protokoll und Logging vorbereitet. |
| **04.11.2026** | Sprintstatus aktualisiert und Release für Pebble Time 2 durch den Projektverantwortlichen bestätigt; MIT-Lizenz, öffentliche Paketmetadaten und App-Name „Sit Stand Move“ ergänzt. Das Abnahmeprotokoll hält fest, dass detaillierte Einzelprüfungen nicht dokumentiert wurden. |
| **06.11.2026** | Optionale PebbleKit-JS-Bridge für einen informativen Pebble-Timeline-Pin implementiert; Screenshots und Paketangaben ergänzt. |
| **07.11.2026** | App-Version auf **1.0.6** angehoben. |
