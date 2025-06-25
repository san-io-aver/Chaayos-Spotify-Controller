#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include  "secrets.h"
const int trig = D2;
const int echo = D1;

const int BLUE = D5;
const int GREEN = D6;
const int RED = D7;

int   distance;
long  duration;

unsigned long lastGestureTime=0;


bool wasFar   = true;
bool isMute   = false;
bool isPaused = false;


void setup() {

  pinMode(trig,OUTPUT);
  pinMode(echo,INPUT);

  pinMode(RED,OUTPUT);
  pinMode(GREEN,OUTPUT);
  pinMode(BLUE,OUTPUT);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  blink("GREEN",3);
  Serial.println("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
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


void loop() {
  String action;
  unsigned long currentTime = millis();
  if (currentTime - lastGestureTime < 100){return;}

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
    Serial.println("in pause/resume section");
    blink("BLUE",2);
    sendAction(isPaused ? "resume" : "pause");
    isPaused = !isPaused;
    lastGestureTime = currentTime;
    Serial.println("pause/resume triggered");
  }
  //volume soon
}
