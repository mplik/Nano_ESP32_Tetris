# Nano ESP32 

## 🎮 Retro Tetris Handheld - DFRobot + Nano ESP32

Zbudowałem wraz z grupą przyjacół małą konsolkę na płytce prototypowej z joystickiem i OLED 0.96" SSD1306.

<p align="center">
  <img src="docs/assets/images/retro_tetris_handheld.gif" width="450" />
</p>

- **MCU:** Arduino Nano ESP32
- **Wyświetlacz:** OLED SSD1306 128x64 (PTS:0 HI:110)
- **Sterowanie:** Analog joystick lub przyciski
- **Styl:** 16-bit pixel art, CRT effect


Projekt jest realizowany w ramach eksperymentu z mikrokontrolerem Arduino Nano ESP32 oraz środowiskiem PlatformIO. Celem jest stworzenie klasycznej gry Tetris uruchamianej na małym wyświetlaczu OLED, z prostym sterowaniem i dodatkowymi funkcjami rozszerzającymi doświadczenie użytkownika.

## Opis projektu
Nano ESP32 Tetris to gra logiczna oparta na mechanice klasycznego Tetrisa, uruchomiona na płytce Arduino Nano ESP32. Projekt jest rozwijany jako przykład połączenia sprzętu embedded z prostym interfejsem WWW oraz obsługą lokalnego Wi‑Fi.

## Aktualny stan rozwoju
Projekt jest w trakcie rozwoju. Obecnie wspierane są:
- logika gry Tetris na wyświetlaczu OLED,
- sterowanie joystickiem lub alternatywnie zestawem przycisków,
- możliwość wstrzymania i wznowienia gry,
- zapis najlepszego wyniku do pamięci EEPROM,
- podstawowe efekty dźwiękowe,
- sygnalizacja usunięcia pełnej linii za pomocą diody LED,
- eksperymentalne wsparcie dla Wi‑Fi i prostego serwera WWW.

## Sprzęt
- Arduino Nano ESP32
- Wyświetlacz OLED SSD1306 128x64
- Joystick analogowy 2-axis
- Alternatywnie: cztery przyciski sterujące oraz przycisk pauzy
- Buzzer
- Płytka prototypowa / przewody

## Sterowanie

### Wariant joystickowy
- Lewo / Prawo: ruch klocka w poziomie
- Dół: przyspieszenie spadania
- Przycisk joysticka: obrót klocka

### Wariant z przyciskami
- Lewo: przesunięcie klocka w lewo
- Prawo: przesunięcie klocka w prawo
- Dół: przyspieszenie spadania
- Obrót: obrót klocka
- Pauza: wstrzymanie lub wznowienie gry

## Funkcje
- mechanika gry Tetris,
- punktacja i usuwanie pełnych linii,
- zapis najwyższego wyniku,
- podgląd następnego klocka,
- obsługa dźwięków,
- ustawienia poziomów trudności,
- pauza i wznowienie rozgrywki,
- lokalne Wi‑Fi z prostą stroną WWW (w trakcie rozwijania).

## Szczegóły rozgrywki
- plansza gry ma wymiary `10 × 20` pól,
- dostępnych jest 7 rodzajów klocków,
- za każdą usuniętą linię przyznawane jest 10 punktów,
- poziom zwiększa się automatycznie po usunięciu każdych 10 linii,
- wraz ze wzrostem poziomu skraca się czas opadania klocków,
- po zakończeniu gry plansza i bieżący wynik są resetowane.

Na ekranie OLED wyświetlane są aktualny wynik (`PTS`), najlepszy wynik (`HI`),
poziom (`L`) oraz podgląd kolejnego klocka (`NEXT`).

## Połączenia sprzętowe
| Element | Pin / interfejs |
| --- | --- |
| Joystick, oś pionowa | `A0` |
| Joystick, oś pozioma | `A1` |
| Przycisk joysticka | `A2` |
| Buzzer | `D8` |
| Dioda LED | `D2` |
| OLED SSD1306 | I2C, adres `0x3C` |

### Wariant z przyciskami

| Funkcja | Pin |
| --- | --- |
| Lewo | `D4` |
| Prawo | `D5` |
| Dół | `D6` |
| Obrót | `D7` |
| Pauza | `D9` |

## Wi-Fi i panel WWW
Do konfiguracji połączenia używany jest WiFiManager. Przy braku zapisanej
konfiguracji urządzenie uruchamia sieć konfiguracyjną `Tetris_Setup`.
Po połączeniu z domową siecią adres urządzenia można odczytać w monitorze
szeregowym. Panel WWW jest serwowany z systemu plików LittleFS na porcie `80`.

Panel umożliwia sterowanie grą z przeglądarki: ruchem w lewo i w prawo,
obrotem oraz przyspieszeniem opadania. Polecenia są wysyłane przez endpoint:

```text
/action?go=left
/action?go=right
/action?go=rotate
/action?go=drop
```

## Efekty i pamięć
- obrót klocka, usunięcie linii i zakończenie gry sygnalizowane są dźwiękiem,
- dioda LED pozostaje zgaszona podczas normalnej rozgrywki,
- po uzupełnieniu i usunięciu pełnej linii dioda LED zapala się na krótką chwilę,
- najlepszy wynik jest zapisywany w EEPROM i zachowywany po restarcie urządzenia.

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

W obecnej wersji przycisk `Start Game` w panelu WWW jest elementem interfejsu,
ale nie uruchamia osobnej procedury startowej. Panel WWW nie wyświetla jeszcze
wyniku ani bieżącego stanu gry.
