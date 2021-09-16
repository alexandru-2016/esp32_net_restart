#include <WiFi.h>
#include <ESP32httpUpdate.h>

#include "secrets.h"

#define USB_OUTPUT_LED 17

#define US_TO_S_MULTIPLIER 1000000
#define SEC_TO_SLEEP 300
#define WIFI_CONNECT_WAIT_SEC 90
#define INTERNET_ACCESS_WAIT_SEC 900
#define INTERNET_ACCESS_TEST_HOST "google.com"

RTC_DATA_ATTR int bootCount = 0;

void setup_wifi() {  
  char* ssid = WIFI_SSID;
  char* password = WIFI_PASS;

  Serial.println();
  Serial.print("Connecting to wifi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
}

bool check_wifi(int secondsTimeout) {
  int retryCount = 0;
  
  while (WiFi.status() != WL_CONNECTED && retryCount < secondsTimeout) {
      delay(1000);
      Serial.print(".");
      retryCount++;

      if (retryCount % 5 == 0) {
        Serial.println("Wifi reconnect");
        WiFi.reconnect(); // try to workaround WiFi not connecting after deep sleep
      }
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
  // hacked ESP32httpUpdate.cpp handleUpdate
  // to increase http.setTimeout to 5 minutes
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
  Serial.print("Resolving ");
  Serial.println(INTERNET_ACCESS_TEST_HOST);

  bool resolved = WiFi.hostByName(INTERNET_ACCESS_TEST_HOST, resolvedIP);

  if (resolved) {
    Serial.print("Successfully resolved ");
    Serial.println(INTERNET_ACCESS_TEST_HOST);
  } else {
    Serial.print("Could not resolve ");
    Serial.println(INTERNET_ACCESS_TEST_HOST);
  }
  
  return resolved;
}

bool check_internet(int secondsTimeout) {
  int retryTime = 0;
  bool resolved = false;
  
  Serial.println("Check internet access");
  
  while (retryTime < secondsTimeout) {
    resolved = resolve_google_dot_com();
    if (resolved) {
      break;
    }
    
    delay(5000);
    retryTime = retryTime + 5;
  }
  
  Serial.println("");

  return resolved;
}

void loop() {
  if (bootCount >= 300) {
    // as a fallback, always restart once a day or so
    restart_usb_output();
  }
  
  setup_wifi();

  bool wifiOk = check_wifi(WIFI_CONNECT_WAIT_SEC);

  if (!wifiOk) {
    restart_usb_output();
  } else {
    start_update();
    
    if (!check_internet(INTERNET_ACCESS_WAIT_SEC)) {
      restart_usb_output();
    }

    Serial.println();
    Serial.println("All tests passed! Yay!");
    Serial.println();

    rest();
  }  
}
