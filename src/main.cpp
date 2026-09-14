#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <LittleFS.h>
#include <WebServer.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WebServer server(80);
WiFiManager wifiManager;

const int pinLeft = 4;
const int pinRight = 5;
const int pinDown = 6;
const int pinRotate = 7;
const int pinPause = 9;
const int pinBUZZER = 8;
const int pinLED = 2;
const char* googleSheetsUrl = "https://script.google.com/macros/s/AKfycbwffRV2scPw2zQlvZ_5IsjHOVbul0d1vmrOtT-WFDQibqawrBlPSHblQreHbFbD3W9wtQ/exec";

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
int nastepnyKlocek;  // <--- Zmienna na kolejny klocek
int punkty = 0;
int highScore = 0;
int linie = 0;   // <--- zlicza usunięte linie
int poziom = 1;  // <--- poziom gry (wzrost prędkości)
unsigned long czasOpadania = 0;
unsigned long interwal = 500; 

unsigned long ostatniRuch = 0;
bool przyciskPuszczony = true;
bool czyPauza = false;
bool poprzedniStanPauzy = HIGH;
bool czyKoniecGry = false;
bool wynikWyslany = false;
// Bufor polecenia wysyłanego z interfejsu web (0 = brak, 1=left,2=right,3=rotate,4=drop)
volatile uint8_t webAction = 0;

String contentTypeFromPath(const String& path) {
  if (path.endsWith(".html")) return "text/html";
  if (path.endsWith(".css")) return "text/css";
  if (path.endsWith(".js")) return "application/javascript";
  if (path.endsWith(".json") || path.endsWith(".webmanifest")) return "application/manifest+json";
  if (path.endsWith(".png")) return "image/png";
  if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
  if (path.endsWith(".webp")) return "image/webp";
  if (path.endsWith(".svg")) return "image/svg+xml";
  if (path.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

void zapiszHighScore() {
  if (punkty > highScore) {
    highScore = punkty;
    EEPROM.put(EEPROM_ADDR, highScore);
    EEPROM.commit();
  }
}

void odczytajHighScore() {
  EEPROM.get(EEPROM_ADDR, highScore);
  if (highScore < 0 || highScore > 30000) {
    highScore = 0;
  }
}

void grajDzwiek(int czestotliwosc, int czasMs) {
  digitalWrite(pinBUZZER, HIGH);
  delay(czasMs);
  digitalWrite(pinBUZZER, LOW);
}

void wyslijWynikDoArkusza() {
  if (wynikWyslany) {
    return;
  }
  wynikWyslany = true;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Brak WiFi - wynik nie zostal wyslany.");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  String url = String(googleSheetsUrl) + "?punkty=" + String(punkty) + "&player=Gracz";

  Serial.println("Wysylanie wyniku do Google Sheets...");
  if (!http.begin(client, url)) {
    Serial.println("Nie mozna polaczyc z Google Sheets.");
    return;
  }
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  int kodOdpowiedzi = http.GET();
  if (kodOdpowiedzi > 0) {
    Serial.print("Odpowiedz Google Sheets: ");
    Serial.println(kodOdpowiedzi);
    Serial.println(http.getString());
  } else {
    Serial.print("Blad wysylania wyniku: ");
    Serial.println(http.errorToString(kodOdpowiedzi));
  }
  http.end();
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
  aktualnyKlocek = nastepnyKlocek;
  nastepnyKlocek = random(0, 7);   // <--- Losowanie kolejnego klocka
  aktualnyX = BOARD_WIDTH / 2 - 2;
  aktualnyY = 0;
  rotacja = 0;
  if (kolizja(aktualnyX, aktualnyY, rotacja)) {
    zapiszHighScore();
    wyslijWynikDoArkusza();
    czyKoniecGry = true;
    grajDzwiek(150, 300);
  }
}

void resetujGre() {
  punkty = 0;
  linie = 0;
  poziom = 1;
  interwal = 500;
  memset(plansza, 0, sizeof(plansza));
  czyKoniecGry = false;
  wynikWyslany = false;
  czyPauza = false;
  przyciskPuszczony = false;
  aktualnyKlocek = random(0, 7);
  nastepnyKlocek = random(0, 7);
  aktualnyX = BOARD_WIDTH / 2 - 2;
  aktualnyY = 0;
  rotacja = 0;
  czasOpadania = millis();
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
      linie++;
      if (linie % 10 == 0) { // <--- Co 10 linii zwiększ poziom
        poziom++;
        interwal = max(100UL, interwal - 40); // <--- Zwiększ prędkość
      }
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
    digitalWrite(pinLED, HIGH);
    delay(50);
    digitalWrite(pinLED, LOW);
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
  Serial.begin(9600);
  randomSeed(analogRead(A3));
  pinMode(pinLeft, INPUT_PULLUP);
  pinMode(pinRight, INPUT_PULLUP);
  pinMode(pinDown, INPUT_PULLUP);
  pinMode(pinRotate, INPUT_PULLUP);
  pinMode(pinPause, INPUT_PULLUP);
  pinMode(pinBUZZER, OUTPUT);
  pinMode(pinLED, OUTPUT);
  digitalWrite(pinBUZZER, LOW);
  digitalWrite(pinLED, LOW);
  
  EEPROM.begin(EEPROM_SIZE);
  odczytajHighScore();
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    while(1); 
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Laczenie z WiFi...");
  display.println("Otworz Tetris_Setup");
  display.display();
  nowyKlocek();

  // --- Uruchomienie systemu plików i serwera WWW ---
  if(!LittleFS.begin(true)){
    Serial.println("Błąd montowania LittleFS!");
  } else {
    Serial.println("LittleFS zamontowany.");
  }

  wifiManager.autoConnect("Tetris_Setup");
  Serial.print("Adres IP serwera: ");
  Serial.println(WiFi.localIP());

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

  // Endpoint do przyjmowania poleceń z interfejsu web
  server.on("/action", HTTP_GET, []() {
    if (server.hasArg("go")) {
      String a = server.arg("go");
      uint8_t code = 0;
      if (a == "left") code = 1;
      else if (a == "right") code = 2;
      else if (a == "rotate") code = 3;
      else if (a == "drop") code = 4;
      else if (a == "start") code = 5;
      if (code) {
        webAction = code;
        Serial.print("Web action: ");
        Serial.println(a);
      }
    }
    server.send(200, "text/plain", "OK");
  });

  server.onNotFound([]() {
    String path = server.uri();
    if (path == "/") {
      path = "/index.html";
    }

    if (!LittleFS.exists(path)) {
      server.send(404, "text/plain", "Not found");
      return;
    }

    File file = LittleFS.open(path, "r");
    if (!file) {
      server.send(500, "text/plain", "Błąd otwarcia pliku");
      return;
    }

    server.streamFile(file, contentTypeFromPath(path));
    file.close();
  });

  server.begin();
  Serial.println("Serwer WWW wystartował.");
}

void loop() {
  server.handleClient();

  if (czyKoniecGry) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(35, 15);
    display.print("GAME OVER");
    display.setCursor(20, 30);
    display.print("Wynik: ");
    display.print(punkty);

    if ((millis() / 500) % 2 == 0) {
      display.setCursor(18, 48);
      display.print("PRESS TO START");
    }

    display.display();

    uint8_t gameOverAction = webAction;
    webAction = 0;
    if (digitalRead(pinRotate) == LOW || gameOverAction == 5) {
      resetujGre();
      delay(300);
    }
    return;
  }

  bool aktualnyStanPauzy = digitalRead(pinPause);
  if (aktualnyStanPauzy == LOW && poprzedniStanPauzy == HIGH) {
    czyPauza = !czyPauza;
    delay(200);
  }
  poprzedniStanPauzy = aktualnyStanPauzy;

  if (!czyPauza) {

  // Obsługa poleceń z interfejsu web (ustawiana przez handler /action)
  if (webAction != 0) {
    uint8_t action = webAction;
    webAction = 0;
    if (action == 1) { // left
      if (!kolizja(aktualnyX - 1, aktualnyY, rotacja)) aktualnyX--;
    } else if (action == 2) { // right
      if (!kolizja(aktualnyX + 1, aktualnyY, rotacja)) aktualnyX++;
    } else if (action == 3) { // rotate
      int nastepnaRotacja = (rotacja + 1) % 4;
      if (!kolizja(aktualnyX, aktualnyY, nastepnaRotacja)) {
        rotacja = nastepnaRotacja;
        grajDzwiek(600, 40);
      }
    } else if (action == 4) { // drop
      if (!kolizja(aktualnyX, aktualnyY + 1, rotacja)) {
        aktualnyY++;
      } else {
        zamrozKlocek();
      }
    }
    ostatniRuch = millis();
  }

  bool stanLeft = digitalRead(pinLeft);
  bool stanRight = digitalRead(pinRight);
  bool stanDown = digitalRead(pinDown);
  bool stanRotate = digitalRead(pinRotate);
  
  // Sterowanie poziome przyciskami
  if (millis() - ostatniRuch > 150) {
    if (stanLeft == LOW) {
      if (!kolizja(aktualnyX - 1, aktualnyY, rotacja)) aktualnyX--;
      ostatniRuch = millis();
    }
    else if (stanRight == LOW) {
      if (!kolizja(aktualnyX + 1, aktualnyY, rotacja)) aktualnyX++;
      ostatniRuch = millis();
    }
  }

  // Przyspieszenie w dół
  unsigned long aktualnyInterwal = interwal;
  if (stanDown == LOW) {
    aktualnyInterwal = 50; 
  }

  // Obrót przyciskiem
  if (stanRotate == LOW) {
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

  // Wyświetlanie punktów i najlepszego wyniku
  display.setCursor(0, 0);
  display.print("PTS:");
  display.setCursor(0, 10);
  display.print(punkty);


  // Wyświetlanie najlepszego wyniku
  display.setCursor(0, 20);
  display.print("HI:");
  display.setCursor(0, 30);
  display.print(highScore);

  // Wyświetlanie napisu NEXT i podglądu klocka
  display.setCursor(0, 40);
  display.print("NEXT:");

  // Wyświetlanie poziomu trudności
  display.setCursor(0, 50);
  display.print("L:");
  display.print(poziom);

  for (int x = 0; x < 4; x++) {
    for (int y = 0; y < 4; y++) {
      if (klocki[nastepnyKlocek][y][x]) {
        display.fillRect(20 + x * 2, 50 + y * 2, 2, 2, SSD1306_WHITE);
      }
    }
  }

  if (czyPauza) {
    display.setCursor(50, 28);
    display.print("PAUSE");
  }

  display.display();
  delay(20);
}