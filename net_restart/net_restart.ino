#include <WiFi.h>
#include <ESP32httpUpdate.h>

#include "secrets.h"

#define USB_OUTPUT_LED 17

#define US_TO_S_MULTIPLIER 1000000
#define SEC_TO_SLEEP 300

RTC_DATA_ATTR int bootCount = 0;

void setup_wpa2() {  
  char* ssid = WPA2_SSID;
  char* password = WPA2_PASS;

  Serial.println();
  Serial.print("Connecting to WPA2: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
}

bool check_wifi(int secondsTimeout) {
  int retryCount = 0;
  
  while (WiFi.status() != WL_CONNECTED && retryCount < secondsTimeout) {
      delay(1000);
      Serial.print(".");
      retryCount++;
  }
  Serial.println("");

  bool wifiOk = WiFi.status() == WL_CONNECTED;

  if (wifiOk) {
    print_wifi_connected();
  } else {
    Serial.println("WiFi cannot connect");
  }

  return wifiOk;
}

void print_wifi_connected() {
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  
  Serial.println();
  Serial.print("Firmware version: ");
  Serial.println(VERSION);
  
  pinMode(USB_OUTPUT_LED, OUTPUT);
  Serial.println("Boot number: " + String(++bootCount));
}

void restart_usb_output() {
  Serial.println("Turning OFF output USB");  
  digitalWrite(USB_OUTPUT_LED, HIGH);
  delay(60000);
  
  Serial.println("Turning ON output USB");
  digitalWrite(USB_OUTPUT_LED, LOW);

  // also restart
  // because wifi may not connect because esp bugs, in which case a restart is recommended
  reset_with_dellay();
}

void rest() {  
  Serial.print("Setting up a timed wakeup after number of seconds: ");
  Serial.println(SEC_TO_SLEEP);
  esp_sleep_enable_timer_wakeup(SEC_TO_SLEEP * US_TO_S_MULTIPLIER);
  
  Serial.print("Entering a deep and dreamless slumber ...");
  Serial.flush();
  delay(1000);
  
  esp_deep_sleep_start();
}

void reset_with_dellay() {
  Serial.println("Pending restart ...");
  delay(SEC_TO_SLEEP * 1000);
  ESP.restart();
}

void start_update() {
  Serial.println();
  Serial.println("Starting update ...");
  Serial.println(UPDATE_SERVER);

  ESPhttpUpdate.rebootOnUpdate(false);
  t_httpUpdate_return ret = ESPhttpUpdate.update(UPDATE_SERVER, VERSION);

  switch(ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILED - Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;

  case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;

  case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK - restarting ...");
      delay(5000);
      ESP.restart();
      break;
  }
  
  Serial.println();
}

bool resolve_google_dot_com() {
  IPAddress resolvedIP;
  Serial.println("Resolving google.com ");

  bool resolved = WiFi.hostByName("google.com", resolvedIP);

  if (resolved) {
    Serial.println("Successfully resolved google.com");
  } else {
    Serial.println("Could not resolve google.com");
  }
  
  return resolved;
}

void loop() {
  if (bootCount >= 300) {
    // as a fallback, always restart once a day or so
    restart_usb_output();
  }
  
  setup_wpa2();

  bool wifiOk = check_wifi(30);

  if (!wifiOk) {
    restart_usb_output();
  } else {
    start_update();
    
    if (!resolve_google_dot_com()) {
      restart_usb_output();
    }

    Serial.println();
    Serial.println("All tests passed! Yay!");
    Serial.println();

    rest();
  }  
}
