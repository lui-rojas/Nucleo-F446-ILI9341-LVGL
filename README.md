# Setup-Anleitung: STM32 RTOS + LVGL + ILI9341

Diese Dokumentation enthält alle Informationen zur Installation der Toolchain unter Ubuntu, der Verzeichnisstruktur sowie der notwendigen Makefile- und Systemkonfigurationen.

---

## 1. System-Installation (Ubuntu)

Um das Projekt zu kompilieren und auf das Board zu laden, müssen die folgenden Pakete über das Terminal installiert werden:

*   **Toolchain:** `build-essential`, `gcc-arm-none-eabi`, `binutils-arm-none-eabi`
*   **Bibliotheken:** `libnewlib-arm-none-eabi`
*   **Debugging/Flashing:** `gdb-arm-none-eabi`, `openocd`

### Empfohlene VS Code Erweiterungen
*   **C/C++** (IntelliSense und Error-Checking)
*   **Cortex-Debug** (Für das Debuggen auf der Hardware)
*   **Makefile Tools** (Zur Integration der Build-Befehle)

---

## 2. Verzeichnisstruktur und Bibliotheken

### LVGL (Version 8.3)
Die LVGL-Bibliothek muss im Verzeichnis `Core/` abgelegt werden. Der empfohlene Pfad für die Einbindung ist `Core/lvgl-release-v8.3`. 

### ILI9341 Display-Treiber
Der Treiber und die zugehörigen Schriftarten befinden sich im Verzeichnis `Drivers/ILI9341/`. Hier müssen die Header- und Quelldateien für den ILI9341-Chip sowie die `fonts`-Dateien abgelegt werden.

---

## 3. Makefile-Konfiguration

Damit das Projekt erfolgreich linkt und keine Laufzeitfehler (HardFaults) auftreten, sind zwei Anpassungen im Makefile zwingend erforderlich:

### Einbindung der Quelldateien (C_SOURCES)
Alle Unterverzeichnisse von LVGL (core, draw, extra, font, hal, misc, widgets) sowie der Pfad zum Display-Treiber müssen über `wildcard`-Befehle in die `C_SOURCES` aufgenommen werden. Dies stellt sicher, dass der Compiler alle notwendigen Module findet.

### FPU-Einstellung (Floating Point Unit)
Für die Kompatibilität mit dem Assembler-Code des RTOS-Kontextwechsels muss die Floating-Point-Einstellung auf den Software-Modus gesetzt werden:
*   **Einstellung:** `FLOAT-ABI = -mfloat-abi=soft`

---

## 4. System-Stabilität und Initialisierungs-Folge

Um Abstürze (HardFaults) während des Boot-Vorgangs zu vermeiden, muss eine strikte Reihenfolge bei der Initialisierung eingehalten werden:

1.  **Initialisierungs-Guard:** Es wird eine globale Status-Variable (z. B. `lvgl_is_initialized`) benötigt, die standardmäßig auf `0` steht.
2.  **Hauptprogramm (main.c):** Die Hardware-Inits (SPI, GPIO, Display) und `lv_init()` müssen vollständig abgeschlossen sein, bevor die Status-Variable auf `1` gesetzt wird. Erst danach darf die Funktion zum Starten des Betriebssystems (`Start_OS`) aufgerufen werden.
3.  **Interrupt-Steuerung (stm32f4xx_it.c):** Im `SysTick_Handler` müssen alle Operationen, die das RTOS oder LVGL betreffen (wie das Inkrementieren der Ticks oder das Auslösen des PendSV-Interrupts), durch eine Abfrage der Status-Variable abgesichert werden. Sie dürfen nur ausgeführt werden, wenn die Initialisierung abgeschlossen ist.

---

## 5. Build-Prozess

Das Projekt wird über das Terminal mit den Standard-Make-Befehlen verwaltet:
*   `make clean`: Löscht alte Build-Dateien.
*   `make -j`: Kompiliert das Projekt unter Nutzung aller verfügbaren CPU-Kerne.
