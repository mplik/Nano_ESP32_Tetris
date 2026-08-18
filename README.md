# Nano ESP32 

## 🎮 Retro Tetris Handheld - DFRobot + Nano ESP32

Zbudowałem wraz z grupą przyjacół małą konsolkę na płytce prototypowej z joystickiem i OLED 0.96" SSD1306.

<p align="center">
  <img src="docs/assets/images/retro_tetris_handheld.gif" width="450" />
</p>

- **MCU:** Arduino Nano ESP32
- **Wyświetlacz:** OLED SSD1306 128x64 (PTS:0 HI:110)
- **Sterowanie:** Analog joystick
- **Styl:** 16-bit pixel art, CRT effect


Projekt jest realizowany w ramach eksperymentu z mikrokontrolerem Arduino Nano ESP32 oraz środowiskiem PlatformIO. Celem jest stworzenie klasycznej gry Tetris uruchamianej na małym wyświetlaczu OLED, z prostym sterowaniem i dodatkowymi funkcjami rozszerzającymi doświadczenie użytkownika.

## Opis projektu
Nano ESP32 Tetris to gra logiczna oparta na mechanice klasycznego Tetrisa, uruchomiona na płytce Arduino Nano ESP32. Projekt jest rozwijany jako przykład połączenia sprzętu embedded z prostym interfejsem WWW oraz obsługą lokalnego Wi‑Fi.

## Aktualny stan rozwoju
Projekt jest w trakcie rozwoju. Obecnie wspierane są:
- logika gry Tetris na wyświetlaczu OLED,
- sterowanie joystickiem,
- zapis najlepszego wyniku do pamięci EEPROM,
- podstawowe efekty dźwiękowe,
- eksperymentalne wsparcie dla Wi‑Fi i prostego serwera WWW.

## Sprzęt
- Arduino Nano ESP32
- Wyświetlacz OLED SSD1306 128x64
- Joystick analogowy 2-axis
- Buzzer
- Płytka prototypowa / przewody

## Sterowanie
- Lewo / Prawo: ruch klocka w poziomie
- Dół: przyspieszenie spadania
- Przycisk: obrót klocka

## Funkcje
- mechanika gry Tetris,
- punktacja i usuwanie pełnych linii,
- zapis najwyższego wyniku,
- obsługa dźwięków,
- lokalne Wi‑Fi z prostą stroną WWW (w trakcie rozwijania).

## Środowisko i zależności
Projekt jest przygotowany pod PlatformIO i wykorzystuje:
- framework Arduino dla ESP32,
- biblioteki Adafruit GFX i Adafruit SSD1306,
- LittleFS do obsługi plików strony WWW,
- WiFi oraz WebServer.

## Jak uruchomić
1. Otwórz projekt w PlatformIO.
2. Wgraj firmware do płyty:
   ```bash
   pio run --target upload --environment arduino_nano_esp32
   ```
3. Jeśli chcesz użyć obsługi plików strony WWW, wgraj również system plików (LittleFS).
   Najpierw zbuduj obraz systemu plików, następnie go wgraj:
   ```bash
   pio run --target buildfs --environment arduino_nano_esp32
   pio run --target uploadfs --environment arduino_nano_esp32
   ```

   W PowerShell (jeśli używasz wbudowanego środowiska PlatformIO):
   ```powershell
   .\.platformio\penv\Scripts\platformio.exe run --target buildfs --environment arduino_nano_esp32
   .\.platformio\penv\Scripts\platformio.exe run --target uploadfs --environment arduino_nano_esp32
   ```
4. Po uruchomieniu płyty połącz się z siecią Wi‑Fi utworzoną przez urządzenie (jeśli funkcja jest aktywna) i otwórz adres IP podany w monitorze szeregowym.

## Struktura projektu
- `src/main.cpp` – główna logika gry, sterowanie oraz obsługa Wi‑Fi/WWW
- `data/` – pliki strony internetowej (`index.html`, `style.css`)
- `platformio.ini` – konfiguracja projektu PlatformIO

## Uwagi
Projekt jest nadal rozwijany. Niektóre elementy, zwłaszcza związane z Wi‑Fi i interfejsem WWW, mogą ulegać zmianom w zależności od aktualnego etapu prac.
