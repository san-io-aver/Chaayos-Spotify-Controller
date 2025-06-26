#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include  "secrets.h"
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

const int trig = D2;
const int echo = D1;

#define OLED_SCL D3 
#define OLED_SDA D4   

const int BLUE = D5;
const int GREEN = D6;
const int RED = D7;

int   distance;
long  duration;

int scrollX=0;
String song;
int song_progress=0;
int song_duration=1;
unsigned long lastGestureTime=0;
unsigned long lastSongUpdateTime=0;
unsigned long lastSongScrollTime=0;

bool wasFar   = true;
bool isMute   = false;
bool isPaused = false;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
const unsigned char unmute_bmp [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0xc0, 0x01, 0x40, 0x1e, 0x44, 0x24, 0x40, 0x24, 0x40, 
	0x24, 0x40, 0x24, 0x40, 0x1c, 0x40, 0x00, 0x40, 0x00, 0xc0, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00
};
// 'mute', 16x16px
const unsigned char mute_bmp [] PROGMEM = {
	0x80, 0x08, 0x40, 0x18, 0x20, 0x48, 0x10, 0x88, 0x3b, 0x08, 0x2d, 0x08, 0x22, 0x08, 0x21, 0x08, 
	0x20, 0x88, 0x21, 0x48, 0x21, 0x28, 0x3f, 0x10, 0x00, 0x88, 0x00, 0x44, 0x00, 0x3a, 0x00, 0x09
};


void setup() {

  pinMode(trig,OUTPUT);
  pinMode(echo,INPUT);

  pinMode(RED,OUTPUT);
  pinMode(GREEN,OUTPUT);
  pinMode(BLUE,OUTPUT);

  Serial.begin(115200);
  Wire.begin(OLED_SDA, OLED_SCL);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // 0x3C is common address
    Serial.println(F("OLED not found"));
    while (1);  // Stop if not found
  }

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  blink("GREEN",3);
  Serial.println("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
  song = getSong();
}
int getDistance() {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  duration = pulseIn(echo, HIGH, 20000);
  if (duration == 0) {
    delay(50);  // wait briefly and retry
    duration = pulseIn(echo, HIGH, 20000);
    if (duration == 0) return -1;
  }

  distance = duration * 0.034 / 2;
  return distance;
}
int smoothDistance() {
  int readings = 0;
  int total = 0;
  for (int i = 0; i < 3; i++) {
    int d = getDistance();
    if (d > 0) {
      total += d;
      readings++;
    }
    delay(20);
  }
  return (readings > 0) ? total / readings : -1;
}

void blink(String color,int count) {
  int pin;

  if (color == "RED") {
    pin = RED;
  } else if (color == "GREEN") {
    pin = GREEN;
  } else if (color == "BLUE") {
    pin = BLUE;
  } else {
    Serial.println("Invalid color!");
    return;
  }
  for(int i=0;i<count;i++){
    digitalWrite(pin, HIGH);
    delay(100);
    digitalWrite(pin, LOW);
    delay(100);
  }

}

void sendAction(String action){
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    http.begin(client,serverUrl);
    http.addHeader("Content-Type", "application/JSON");
    String data = "{ \"action\": \""+action+ "\"}";
    Serial.println("Sending JSON: " + data);  // ADD THIS

    int response = http.POST(data);
    Serial.println("response: " + String(response));
    Serial.print("Error: ");
    Serial.println(http.errorToString(response));
    if(response == 200) blink("GREEN",2);
    else{blink("RED",2);}
    http.end();
  }
}
void drawProgressBar(int songProgress, int songDuration, int y = 30, int height = 7) {
  if (songDuration == 0) return;

  float progress = (float)songProgress / songDuration;
  int barWidth = (int)(SCREEN_WIDTH * progress);
  int radius = height / 2;

  display.drawRoundRect(0, y, SCREEN_WIDTH, height, radius, WHITE);
  if (barWidth > radius * 2) {
    display.fillRect(radius, y, barWidth - radius * 2, height, WHITE);
    display.fillCircle(radius, y + radius, radius, WHITE);
    display.fillCircle(barWidth - radius, y + radius, radius, WHITE);
  } else {
    display.fillCircle(barWidth / 2, y + radius, barWidth / 2, WHITE);
  }
}
void drawPlayPauseIcon(bool isPaused, bool isMuted, int y = 56) {
  int iconSpacing = 30;
  int centerX = SCREEN_WIDTH / 2;
  int x = centerX - iconSpacing;

  if (isMuted) {
    display.drawBitmap(centerX - iconSpacing, y-4, mute_bmp, 16, 16, WHITE);
  }else {
    display.drawBitmap(centerX - iconSpacing, y-4, unmute_bmp, 16, 16, WHITE);
  }

  if (isPaused) {
    display.fillTriangle(centerX, y, centerX, y + 10, centerX + 8, y + 5, WHITE);
  } else {
    display.fillRect(centerX, y, 3, 10, WHITE);
    display.fillRect(centerX + 5, y, 3, 10, WHITE);
  }
//will fix later
  display.drawCircle(centerX + iconSpacing, y + 5, 3, WHITE);
  display.drawLine(centerX + iconSpacing + 4, y + 2, centerX + iconSpacing + 4, y + 8, WHITE);
}


String getSong(){
  if(WiFi.status()==WL_CONNECTED){
    WiFiClient client;
    HTTPClient http;
    http.begin(client, song_serverUrl);
    http.addHeader("Content-Type", "application/JSON");
    int httpCode = http.GET();

    if(httpCode == 200){
      String data = http.getString();
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, data);
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
        return "error retrieving music";
      }

      String newSong = doc["song"];
      String artist = doc["artist"];
      song_progress = doc["progress_ms"];
      song_duration = doc["duration_ms"];
      
      http.end();
      newSong.trim();
      if (newSong != song) {
        song = newSong;
        scrollX = -SCREEN_WIDTH;  
      }
    }
  }
  return song;
}

void scrollSong(String song,int songProgress,int songDuration) {
  int16_t x1, y1;
  uint16_t w, h;
  display.setTextSize(2);
  String dsong = song + "  " ;
  display.getTextBounds(dsong, 0, 0, &x1, &y1, &w, &h);

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextWrap(false);

  int y = 10;
  display.setCursor(-scrollX, y);
  display.print(dsong);

  if (w > SCREEN_WIDTH) {
    display.setCursor(w - scrollX + 10, y);  // loop offset
    display.print(dsong);
  }
  drawProgressBar(songProgress,songDuration);
  drawPlayPauseIcon(isPaused, isMute, 45);

  display.display();

  scrollX++;
  if (scrollX > w + 10) {
    scrollX = 0;
  }
}

void loop() {
  String action;
  unsigned long currentTime = millis();
  if (currentTime - lastGestureTime < 100){
    return;
  }

  distance = smoothDistance();if(distance < 0) return; 
  // Serial.println(distance);
  if(distance>9){wasFar = true;}

  if(distance<=9 && (currentTime-lastGestureTime>2000) && wasFar){
    lastGestureTime = currentTime;
    blink("BLUE",1);
    sendAction(isMute? "unmute":"mute");
    isMute=!isMute;
    wasFar = false;
  }
  // pause and resume
  if ((distance > 15 && distance <30) && (currentTime - lastGestureTime > 2000)) {  
    //Serial.println("in pause/resume section");
    blink("BLUE",2);
    sendAction(isPaused ? "resume" : "pause");
    isPaused = !isPaused;
    lastGestureTime = currentTime;
    Serial.println("pause/resume triggered");
  }
  if(currentTime-lastSongUpdateTime > 5000){
    song = getSong();
    lastSongUpdateTime = currentTime;
  }
  if(currentTime-lastSongScrollTime > 70){
    scrollSong(song,song_progress,song_duration);
    lastSongScrollTime = currentTime;
  }
  if (!isPaused && song_progress < song_duration) {
    song_progress += (millis() - lastSongScrollTime);
  }
  if (song_progress > song_duration) song_progress = song_duration;

  //volume soon
}
