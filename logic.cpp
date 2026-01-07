#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// -------------------- RFID PINS --------------------
#define RST_PIN 22
#define SS_PIN 5

MFRC522 mfrc522(SS_PIN, RST_PIN);

// -------------------- LCD PINS --------------------
LiquidCrystal lcd(21, 27, 16, 17, 25, 26);

// -------------------- STORED UIDS --------------------
byte samUID[]   = {0x63, 0x26, 0xB7, 0x14}; // Sam
byte guestUID[] = {0xF3, 0x2E, 0x2A, 0x0E}; // Guest
byte staffUID[] = {0xCC, 0xCA, 0x2E, 0x40}; // Staff
byte thobUID[]  = {0x0C, 0x9F, 0x3B, 0x49}; // Thob

bool samIsHere   = false;
bool guestIsHere = false;
bool staffIsHere = false;

// -------------------- WIFI --------------------
const char* ssid = "VODAFONE-C4B0";
const char* password = "Thng4he2c79pr7nM";

// -------------------- THINGSPEAK --------------------
const char* apiKey = "0KQBGJYZ6CG2EOCY";

// -------------------- DATA VARIABLES --------------------
String lastScan = "";
String uidString = "";
int statusCode = 0;
String nameString = "";
String eventType = "";
String dateTimeString = "";

// -------------------- COMPARE UIDS --------------------
bool compareUID(byte *uidA, byte *uidB, byte size) {
  for (byte i = 0; i < size; i++) {
    if (uidA[i] != uidB[i]) return false;
  }
  return true;
}

// -------------------- GET CURRENT TIME --------------------
String getTimeNow() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "TimeError";
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

// -------------------- UPLOAD TO THINGSPEAK --------------------
void uploadToThingSpeak() {

  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;

  auto urlEncode = [](String str) {
    String encoded = "";
    for (int i = 0; i < str.length(); i++) {
      char c = str.charAt(i);
      if (isalnum(c)) encoded += c;
      else if (c == ' ') encoded += "%20";
      else {
        char buf[4];
        sprintf(buf, "%%%02X", c);
        encoded += buf;
      }
    }
    return encoded;
  };

  String url =
    "https://api.thingspeak.com/update?api_key=" + String(apiKey) +
    "&field1=" + urlEncode(lastScan) +
    "&field2=" + urlEncode(uidString) +
    "&field3=" + String(statusCode) +
    "&field4=" + urlEncode(nameString) +
    "&field5=" + urlEncode(eventType) +
    "&field6=" + urlEncode(dateTimeString);

  http.begin(url);
  http.GET();
  http.end();
}

// -------------------- SETUP --------------------
void setup() {
  Serial.begin(9600);

  SPI.begin();
  mfrc522.PCD_Init();

  lcd.begin(16, 2);
  lcd.print("Starting...");
  delay(800);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(300);

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  delay(1000);

  lcd.clear();
  lcd.print("Scan your card...");
}

// -------------------- LOOP --------------------
void loop() {

  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  // ---------- UID STRING ----------
  uidString = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) uidString += "0";
    uidString += String(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1) uidString += ":";
  }
  uidString.toUpperCase();

  Serial.println("Scanned UID: " + uidString);
  dateTimeString = getTimeNow();

  // ---------- SAM ----------
  if (compareUID(mfrc522.uid.uidByte, samUID, mfrc522.uid.size)) {
    lcd.clear();
    if (!samIsHere) {
      lcd.print("Welcome, Sam");
      lastScan = "Sam entered";
      statusCode += 1;
      eventType = "enter";
      samIsHere = true;
    } else {
      lcd.print("Goodbye, Sam");
      lastScan = "Sam left";
      statusCode -= 1;
      eventType = "exit";
      samIsHere = false;
    }
    nameString = "Sam";
  }

  // ---------- GUEST ----------
  else if (compareUID(mfrc522.uid.uidByte, guestUID, mfrc522.uid.size)) {
    lcd.clear();
    if (!guestIsHere) {
      lcd.print("Welcome, Guest");
      lastScan = "Guest entered";
      statusCode += 1;
      eventType = "enter";
      guestIsHere = true;
    } else {
      lcd.print("Goodbye, Guest");
      lastScan = "Guest left";
      statusCode -= 1;
      eventType = "exit";
      guestIsHere = false;
    }
    nameString = "Guest";
  }

  // ---------- STAFF ----------
  else if (compareUID(mfrc522.uid.uidByte, staffUID, mfrc522.uid.size)) {
    lcd.clear();
    if (!staffIsHere) {
      lcd.print("Welcome, Staff");
      lastScan = "Staff entered";
      statusCode += 1;
      eventType = "enter";
      staffIsHere = true;
    } else {
      lcd.print("Goodbye, Staff");
      lastScan = "Staff left";
      statusCode -= 1;
      eventType = "exit";
      staffIsHere = false;
    }
    nameString = "Staff";
  }

  // ---------- DENIED ----------
  else if (compareUID(mfrc522.uid.uidByte, thobUID, mfrc522.uid.size)) {
    lcd.clear();
    lcd.print("Access Denied");
    lastScan = "Thob denied";
    //statusCode = 0;
    nameString = "Thob";
    eventType = "denied";
  }

  // ---------- UNKNOWN ----------
  else {
    lcd.clear();
    lcd.print("Unknown Card");
    Serial.println("⚠️ UNKNOWN UID: " + uidString);
    lastScan = "Unknown card";
    nameString = "Unknown";
    eventType = "unknown";
  }

  uploadToThingSpeak();

  delay(16000);
  lcd.clear();
  lcd.print("Scan your card...");

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
