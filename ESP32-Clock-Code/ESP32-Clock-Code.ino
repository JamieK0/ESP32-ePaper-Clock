/**
 * OnDemandConfigPortal.ino
 * example of running the configPortal AP manually, independantly from the captiveportal
 * trigger pin will start a configPortal AP for 120 seconds then turn it off.
 * 
 */
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

// select which pin will trigger the configuration portal when set to LOW
#define TRIGGER_PIN 0

int timeout = 120; // seconds to run for

void setup() {
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP  
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("\n Starting");
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  WiFiManager wm;
  wm.resetSettings(); //REMOVE IN PROD
  bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
    res = wm.autoConnect("E-Paper Clock"); // password protected ap

    if(!res) {
        Serial.println("Failed to connect");
        // ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected");
    }
}

void loop() {
  // is configuration portal requested?
  if ( digitalRead(TRIGGER_PIN) == LOW) {
    WiFiManager wm;    

    //reset settings - for testing
    wm.resetSettings();

  
    // set configportal timeout
    wm.setConfigPortalTimeout(timeout);

    if (!wm.startConfigPortal("E-Paper Clock")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected");

  }

  // put your main code here, to run repeatedly:
}
