#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <WebServer.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WebServer server(80);

const int pinVERT = A0;     
const int pinHORZ = A1;     
const int pinSEL = A2;      
const int pinBUZZER = 8;    

#define MARGIN_LEFT 40
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define BLOCK_SIZE 3
#define EEPROM_ADDR 0
#define EEPROM_SIZE 512

bool plansza[BOARD_WIDTH][BOARD_HEIGHT] = {false};

const byte klocki[7][4][4] = {
  {{1,1,1,1}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}, // I
  {{1,1,1,0}, {1,0,0,0}, {0,0,0,0}, {0,0,0,0}}, // L
  {{1,1,1,0}, {0,0,1,0}, {0,0,0,0}, {0,0,0,0}}, // J
  {{1,1,0,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}}, // O
  {{0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}}, // S
  {{1,1,1,0}, {0,1,0,0}, {0,0,0,0}, {0,0,0,0}}, // T
  {{1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}}  // Z
};

int aktualnyKlocek, aktualnyX, aktualnyY, rotacja;
int punkty = 0;
int highScore = 0;
unsigned long czasOpadania = 0;
unsigned long interwal = 500; 

unsigned long ostatniRuch = 0;
bool przyciskPuszczony = true;

void zapiszHighScore() {
  if (punkty > highScore) {
    highScore = punkty;
    EEPROM.put(EEPROM_ADDR, highScore);
    EEPROM.commit();
  }
}

void odczytajHighScore() {
  EEPROM.get(EEPROM_ADDR, highScore);
  if (highScore == 0xFFFFFFFF) {
    highScore = 0;
  }
}

void grajDzwiek(int czestotliwosc, int czasMs) {
  digitalWrite(pinBUZZER, HIGH);
  delay(czasMs);
  digitalWrite(pinBUZZER, LOW);
}

bool pobierzKlocek(int k, int r, int x, int y) {
  int nx = x, ny = y;
  if (r == 1) { nx = y; ny = 3 - x; }
  else if (r == 2) { nx = 3 - x; ny = 3 - y; }
  else if (r == 3) { nx = 3 - y; ny = x; }
  return klocki[k][ny][nx];
}

bool kolizja(int nx, int ny, int nr) {
  for (int x = 0; x < 4; x++) {
    for (int y = 0; y < 4; y++) {
      if (pobierzKlocek(aktualnyKlocek, nr, x, y)) {
        int px = nx + x;
        int py = ny + y;
        if (px < 0 || px >= BOARD_WIDTH || py >= BOARD_HEIGHT) return true;
        if (py >= 0 && plansza[px][py]) return true;
      }
    }
  }
  return false;
}

void nowyKlocek() {
  aktualnyKlocek = random(0, 7);
  aktualnyX = BOARD_WIDTH / 2 - 2;
  aktualnyY = 0;
  rotacja = 0;
  if (kolizja(aktualnyX, aktualnyY, rotacja)) {
    zapiszHighScore();
    punkty = 0;
    memset(plansza, 0, sizeof(plansza));
    grajDzwiek(150, 300);
  }
}

void sprawdzLinie() {
  bool zrobionoPunkt = false;
  for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
    bool pelna = true;
    for (int x = 0; x < BOARD_WIDTH; x++) {
      if (!plansza[x][y]) { pelna = false; break; }
    }
    if (pelna) {
      punkty += 10;
      zrobionoPunkt = true;
      for (int ty = y; ty > 0; ty--) {
        for (int tx = 0; tx < BOARD_WIDTH; tx++) {
          plansza[tx][ty] = plansza[tx][ty-1];
        }
      }
      for (int tx = 0; tx < BOARD_WIDTH; tx++) plansza[tx][0] = false;
      y++; 
    }
  }
  if (zrobionoPunkt) {
    grajDzwiek(800, 100);
  }
}

void zamrozKlocek() {
  for (int x = 0; x < 4; x++) {
    for (int y = 0; y < 4; y++) {
      if (pobierzKlocek(aktualnyKlocek, rotacja, x, y)) {
        if (aktualnyY + y >= 0) {
          plansza[aktualnyX + x][aktualnyY + y] = true;
        }
      }
    }
  }
  sprawdzLinie();
  nowyKlocek();
}

void setup() {
  randomSeed(analogRead(A3));
  pinMode(pinSEL, INPUT_PULLUP);
  pinMode(pinBUZZER, OUTPUT);
  digitalWrite(pinBUZZER, LOW);
  
  EEPROM.begin(EEPROM_SIZE);
  odczytajHighScore();
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    while(1); 
  }
  display.clearDisplay();
  nowyKlocek();

  // --- Uruchomienie systemu plików i serwera WWW ---
  if(!LittleFS.begin(true)){
    Serial.println("Błąd montowania LittleFS!");
  } else {
    Serial.println("LittleFS zamontowany.");
  }

  WiFi.softAP("Tetris_ESP32", "12345678");
  Serial.print("Adres IP serwera: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, []() {
    File file = LittleFS.open("/index.html", "r");
    if (!file) {
      server.send(404, "text/plain", "Brak pliku index.html!");
      return;
    }
    server.streamFile(file, "text/html");
    file.close();
  });

  server.on("/style.css", HTTP_GET, []() {
    File file = LittleFS.open("/style.css", "r");
    if (!file) {
      server.send(404, "text/plain", "Brak pliku style.css!");
      return;
    }
    server.streamFile(file, "text/css");
    file.close();
  });

  server.begin();
  Serial.println("Serwer WWW wystartował.");
}

void loop() {
  server.handleClient();
  int odczytX = analogRead(pinVERT);      
  int odczytY = analogRead(pinHORZ);      
  int stanPrzycisku = digitalRead(pinSEL); 
  
  // Sterowanie poziome dopasowane do odczytów 0 i 4095
  if (millis() - ostatniRuch > 150) {
    if (odczytX > 3800) { // Lewo
      if (!kolizja(aktualnyX - 1, aktualnyY, rotacja)) aktualnyX--;
      ostatniRuch = millis();
    }
    else if (odczytX < 500) { // Prawo
      if (!kolizja(aktualnyX + 1, aktualnyY, rotacja)) aktualnyX++;
      ostatniRuch = millis();
    }
  }

  // Przyspieszenie w dół (dół daje odczyt bliski 0)
  unsigned long aktualnyInterwal = interwal;
  if (odczytY > 3500) { 
    aktualnyInterwal = 50; 
  }

  // Obrót przyciskiem
  if (stanPrzycisku == LOW) { 
    if (przyciskPuszczony) {
      int nastepnaRotacja = (rotacja + 1) % 4;
      if (!kolizja(aktualnyX, aktualnyY, nastepnaRotacja)) {
        rotacja = nastepnaRotacja;
        grajDzwiek(600, 40);
      }
      przyciskPuszczony = false; 
    }
  } else {
    przyciskPuszczony = true;
  }

  if (millis() - czasOpadania > aktualnyInterwal) {
    if (!kolizja(aktualnyX, aktualnyY + 1, rotacja)) {
      aktualnyY++;
    } else {
      zamrozKlocek();
    }
    czasOpadania = millis();
  }

  display.clearDisplay();
  display.drawRect(MARGIN_LEFT - 1, 0, BOARD_WIDTH * BLOCK_SIZE + 2, BOARD_HEIGHT * BLOCK_SIZE + 2, SSD1306_WHITE);
  
  for (int x = 0; x < BOARD_WIDTH; x++) {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
      if (plansza[x][y]) {
        display.fillRect(MARGIN_LEFT + x * BLOCK_SIZE, y * BLOCK_SIZE + 1, BLOCK_SIZE - 1, BLOCK_SIZE - 1, SSD1306_WHITE);
      }
    }
  }

  for (int x = 0; x < 4; x++) {
    for (int y = 0; y < 4; y++) {
      if (pobierzKlocek(aktualnyKlocek, rotacja, x, y)) {
        display.fillRect(MARGIN_LEFT + (aktualnyX + x) * BLOCK_SIZE, (aktualnyY + y) * BLOCK_SIZE + 1, BLOCK_SIZE - 1, BLOCK_SIZE - 1, SSD1306_WHITE);
      }
    }
  }

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("PTS:");
  display.print(punkty);
  display.setCursor(0, 10);
  display.print("HI:");
  display.print(highScore);

  display.display();
  delay(20);
}