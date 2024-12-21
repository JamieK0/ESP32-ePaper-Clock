#include <WiFiManager.h>
#include <WebServer.h>
#include <WiFi.h>
#include <time.h>
#include "images.h"
#include <ESPmDNS.h>


//Heltec E290
#include <heltec-eink-modules.h>
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeSans18pt7b.h"
#include "Fonts/FreeSans24pt7b.h"
#include "Custom_Fonts/FreeSans40pt7b.h"

// Wiring (SPI Displays only)
         #define PIN_DC   2
         #define PIN_CS   4
         #define PIN_BUSY 5
//DEPG0290BNS800 display( PIN_DC, PIN_CS, PIN_BUSY );      // 2.9"  - Mono 
EInkDisplay_VisionMasterE290 display;

//DEPG0290BxS800FxX_BW display(5, 4, 3, 6, 2, 1, -1, 6000000);  //rst,dc,cs,busy,sck,mosi,miso,frequency
typedef void (*Demo)(void);

int demoMode = 0;
int width, height;

//Clock Config
#define TRIGGER_PIN 0
const char* ntpServer = "pool.ntp.org";
const int daylightOffset_sec = 3600;

// Variable for screen update based on current time
String lastDisplayedTime = "";  // Stores the last displayed time
String lastDisplayedHr = "";  // Stores the last displayed hour
String lastDisplayedDate = ""; //Stores the last displayed date

// Create an instance of the Webserver
WebServer server(80);

// Global variable for timezone offset
long gmtOffset_sec = 39600;                                               // Initial GMT offset, can be changed by user selection
String alrm = "07:00";                                                    // Default alarm time
bool alarmDays[7] = { false, false, false, false, false, false, false };  // For each day of the week
bool alarmTriggered = false;

String alrm2 = "07:00";                                                    // 2nd alarm time
bool alarmDays2[7] = { false, false, false, false, false, false, false };  // For each day of the week
bool alarmTriggered2 = false;

//Buzzer pin
int buzzer = 8; //GPIO 8
bool alarmOn = false;

// Button pin
const int buttonPin = 17;
int buttonState = 0;  // variable for reading the pushbutton status
unsigned long button_time = 0;  // used for button debounce
unsigned long last_button_time = 0; // used for button debounce
bool buttonPressed = false;

//White LED
const int ledPin = 45;
bool ledActivated = 0;
unsigned long led_time = 0;  // used for led timer
unsigned long last_led_time = 0; // used for led timer

void IRAM_ATTR isr() {
  button_time = millis();
if (button_time - last_button_time > 250) {
    buttonPressed = true;
    last_button_time = button_time;
  }
}


void setup() {
  Serial.begin(115200);
  pinMode(buzzer, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(buttonPin, isr, FALLING); // Button ISR interrupt
  pinMode(ledPin, OUTPUT);
  // Display setup
  //if (DIRECTION == ANGLE_0_DEGREE || DIRECTION == ANGLE_180_DEGREE) {}
  VextON();
  delay(100);
  display.clear();
  display.landscape();
  display.setFont( &FreeSans9pt7b );
  //display.setFont(ArialMT_Plain_10);
  display.setCursor(0, 0);
  display.println("1. Connect to the E-Paper Clock ");
  display.setCursor(0, 10);
  display.println("  Wifi using the QR Code or");
  display.setCursor(0, 20);
  display.println("  choosing it in your device's wifi");
  display.drawXBitmap(168, 0, Wifi_QR_bits, 128, 128, BLACK);
  display.update();

  //display.drawString(0, 0, "1. Connect to the E-Paper Clock ");
  Serial.print("Connecting to ");
  //display.drawString(0, 10, "  Wifi using the QR Code or");
  //display.drawString(0, 20, "  choosing it in your device's wifi");
  //display.drawString(0, 30, "  settings.");
  //display.drawString(0, 50, "2. Wait for the pop-up to appear");
  //display.drawString(0, 60, "   on your device. This may take");
  //display.drawString(0, 70, "   a few seconds.");
  //display.drawString(0, 100, "3. Choose 'Configure WiFi' and");
  //display.drawString(0, 110, "   enter wifi details.");
  //display.drawXbm(168, 0, 128, 128, Wifi_QR_bits);
  //display.display();

  WiFi.mode(WIFI_STA);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  Serial.println("Turn on");

  WiFiManager wm;
  bool res = wm.autoConnect("E-paper Clock");

  // reset settings - wipe stored credentials for testing
    // these are stored by the esp library
    //wm.resetSettings(); //COMMENT OUT FOR FINAL RELEASE


  if (!res) {
    Serial.println("Failed to connect");
    //display.clear();
    //display.drawString(0, 60, "Failed to connect");
    //display.display();
  } else {
    Serial.println("Connected to Wi-Fi");
    //display.clear();
    //display.drawString(0, 60, "Connected to Wifi");
    //display.display();
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    String lastDisplayedTime = ""; //clears String so that printLocalTime displays current time
    display.clear();
    delay(100);
    display.setFont( &FreeSans24pt7b );
    display.fastmodeOn();
    printLocalTime();
    
  }

  // mDNS setup
  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp32.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin("clock")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);


  // Web server setup
  server.on("/", HTTP_GET, []() {
    String html = "<html><body>";
    html += "<h2>Select Your Time Zone</h2>";
    html += "<form action=\"/setTimezone\" method=\"POST\">";
    html += "<label for=\"timezone\">Timezone:</label>";
    html += "<select name=\"timezone\">";

    // Insert timezone options here, for example:
    html += R"html(
      <option value="GMT0">Africa/Abidjan</option>
<option value="GMT0">Africa/Accra</option>
<option value="EAT-3">Africa/Addis_Ababa</option>
<option value="CET-1">Africa/Algiers</option>
<option value="EAT-3">Africa/Asmara</option>
<option value="GMT0">Africa/Bamako</option>
<option value="WAT-1">Africa/Bangui</option>
<option value="GMT0">Africa/Banjul</option>
<option value="GMT0">Africa/Bissau</option>
<option value="CAT-2">Africa/Blantyre</option>
<option value="WAT-1">Africa/Brazzaville</option>
<option value="CAT-2">Africa/Bujumbura</option>
<option value="EET-2EEST,M4.5.5/0,M10.5.4/24">Africa/Cairo</option>
<option value="<+01>-1">Africa/Casablanca</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Africa/Ceuta</option>
<option value="GMT0">Africa/Conakry</option>
<option value="GMT0">Africa/Dakar</option>
<option value="EAT-3">Africa/Dar_es_Salaam</option>
<option value="EAT-3">Africa/Djibouti</option>
<option value="WAT-1">Africa/Douala</option>
<option value="<+01>-1">Africa/El_Aaiun</option>
<option value="GMT0">Africa/Freetown</option>
<option value="CAT-2">Africa/Gaborone</option>
<option value="CAT-2">Africa/Harare</option>
<option value="SAST-2">Africa/Johannesburg</option>
<option value="CAT-2">Africa/Juba</option>
<option value="EAT-3">Africa/Kampala</option>
<option value="CAT-2">Africa/Khartoum</option>
<option value="CAT-2">Africa/Kigali</option>
<option value="WAT-1">Africa/Kinshasa</option>
<option value="WAT-1">Africa/Lagos</option>
<option value="WAT-1">Africa/Libreville</option>
<option value="GMT0">Africa/Lome</option>
<option value="WAT-1">Africa/Luanda</option>
<option value="CAT-2">Africa/Lubumbashi</option>
<option value="CAT-2">Africa/Lusaka</option>
<option value="WAT-1">Africa/Malabo</option>
<option value="CAT-2">Africa/Maputo</option>
<option value="SAST-2">Africa/Maseru</option>
<option value="SAST-2">Africa/Mbabane</option>
<option value="EAT-3">Africa/Mogadishu</option>
<option value="GMT0">Africa/Monrovia</option>
<option value="EAT-3">Africa/Nairobi</option>
<option value="WAT-1">Africa/Ndjamena</option>
<option value="WAT-1">Africa/Niamey</option>
<option value="GMT0">Africa/Nouakchott</option>
<option value="GMT0">Africa/Ouagadougou</option>
<option value="WAT-1">Africa/Porto-Novo</option>
<option value="GMT0">Africa/Sao_Tome</option>
<option value="EET-2">Africa/Tripoli</option>
<option value="CET-1">Africa/Tunis</option>
<option value="CAT-2">Africa/Windhoek</option>
<option value="HST10HDT,M3.2.0,M11.1.0">America/Adak</option>
<option value="AKST9AKDT,M3.2.0,M11.1.0">America/Anchorage</option>
<option value="AST4">America/Anguilla</option>
<option value="AST4">America/Antigua</option>
<option value="<-03>3">America/Araguaina</option>
<option value="<-03>3">America/Argentina/Buenos_Aires</option>
<option value="<-03>3">America/Argentina/Catamarca</option>
<option value="<-03>3">America/Argentina/Cordoba</option>
<option value="<-03>3">America/Argentina/Jujuy</option>
<option value="<-03>3">America/Argentina/La_Rioja</option>
<option value="<-03>3">America/Argentina/Mendoza</option>
<option value="<-03>3">America/Argentina/Rio_Gallegos</option>
<option value="<-03>3">America/Argentina/Salta</option>
<option value="<-03>3">America/Argentina/San_Juan</option>
<option value="<-03>3">America/Argentina/San_Luis</option>
<option value="<-03>3">America/Argentina/Tucuman</option>
<option value="<-03>3">America/Argentina/Ushuaia</option>
<option value="AST4">America/Aruba</option>
<option value="<-04>4<-03>,M10.1.0/0,M3.4.0/0">America/Asuncion</option>
<option value="EST5">America/Atikokan</option>
<option value="<-03>3">America/Bahia</option>
<option value="CST6">America/Bahia_Banderas</option>
<option value="AST4">America/Barbados</option>
<option value="<-03>3">America/Belem</option>
<option value="CST6">America/Belize</option>
<option value="AST4">America/Blanc-Sablon</option>
<option value="<-04>4">America/Boa_Vista</option>
<option value="<-05>5">America/Bogota</option>
<option value="MST7MDT,M3.2.0,M11.1.0">America/Boise</option>
<option value="MST7MDT,M3.2.0,M11.1.0">America/Cambridge_Bay</option>
<option value="<-04>4">America/Campo_Grande</option>
<option value="EST5">America/Cancun</option>
<option value="<-04>4">America/Caracas</option>
<option value="<-03>3">America/Cayenne</option>
<option value="EST5">America/Cayman</option>
<option value="CST6CDT,M3.2.0,M11.1.0">America/Chicago</option>
<option value="CST6">America/Chihuahua</option>
<option value="CST6">America/Costa_Rica</option>
<option value="MST7">America/Creston</option>
<option value="<-04>4">America/Cuiaba</option>
<option value="AST4">America/Curacao</option>
<option value="GMT0">America/Danmarkshavn</option>
<option value="MST7">America/Dawson</option>
<option value="MST7">America/Dawson_Creek</option>
<option value="MST7MDT,M3.2.0,M11.1.0">America/Denver</option>
<option value="EST5EDT,M3.2.0,M11.1.0">America/Detroit</option>
<option value="AST4">America/Dominica</option>
<option value="MST7MDT,M3.2.0,M11.1.0">America/Edmonton</option>
<option value="<-05>5">America/Eirunepe</option>
<option value="CST6">America/El_Salvador</option>
<option value="<-03>3">America/Fortaleza</option>
<option value="MST7">America/Fort_Nelson</option>
<option value="AST4ADT,M3.2.0,M11.1.0">America/Glace_Bay</option>
<option value="<-02>2<-01>,M3.5.0/-1,M10.5.0/0">America/Godthab</option>
<option value="AST4ADT,M3.2.0,M11.1.0">America/Goose_Bay</option>
<option value="EST5EDT,M3.2.0,M11.1.0">America/Grand_Turk</option>
<option value="AST4">America/Grenada</option>
<option value="AST4">America/Guadeloupe</option>
<option value="CST6">America/Guatemala</option>
<option value="<-05>5">America/Guayaquil</option>
<option value="<-04>4">America/Guyana</option>
<option value="AST4ADT,M3.2.0,M11.1.0">America/Halifax</option>
<option value="CST5CDT,M3.2.0/0,M11.1.0/1">America/Havana</option>
<option value="MST7">America/Hermosillo</option>
<option value="EST5EDT,M3.2.0,M11.1.0">America/Indiana/Indianapolis</option>
<option value="CST6CDT,M3.2.0,M11.1.0">America/Indiana/Knox</option>
<option value="EST5EDT,M3.2.0,M11.1.0">America/Indiana/Marengo</option>
<option value="EST5EDT,M3.2.0,M11.1.0">America/Indiana/Petersburg</option>
<option value="CST6CDT,M3.2.0,M11.1.0">America/Indiana/Tell_City</option>
<option value="EST5EDT,M3.2.0,M11.1.0">America/Indiana/Vevay</option>
<option value="EST5EDT,M3.2.0,M11.1.0">America/Indiana/Vincennes</option>
<option value="EST5EDT,M3.2.0,M11.1.0">America/Indiana/Winamac</option>
<option value="MST7MDT,M3.2.0,M11.1.0">America/Inuvik</option>
<option value="EST5EDT,M3.2.0,M11.1.0">America/Iqaluit</option>
<option value="EST5">America/Jamaica</option>
<option value="AKST9AKDT,M3.2.0,M11.1.0">America/Juneau</option>
<option value="EST5EDT,M3.2.0,M11.1.0">America/Kentucky/Louisville</option>
<option value="EST5EDT,M3.2.0,M11.1.0">America/Kentucky/Monticello</option>
<option value="AST4">America/Kralendijk</option>
<option value="<-04>4">America/La_Paz</option>
<option value="<-05>5">America/Lima</option>
<option value="PST8PDT,M3.2.0,M11.1.0">America/Los_Angeles</option>
<option value="AST4">America/Lower_Princes</option>
<option value="<-03>3">America/Maceio</option>
<option value="CST6">America/Managua</option>
<option value="<-04>4">America/Manaus</option>
<option value="AST4">America/Marigot</option>
<option value="AST4">America/Martinique</option>
<option value="CST6CDT,M3.2.0,M11.1.0">America/Matamoros</option>
<option value="MST7">America/Mazatlan</option>
<option value="CST6CDT,M3.2.0,M11.1.0">America/Menominee</option>
<option value="CST6">America/Merida</option>
<option value="AKST9AKDT,M3.2.0,M11.1.0">America/Metlakatla</option>
<option value="CST6">America/Mexico_City</option>
<option value="<-03>3<-02>,M3.2.0,M11.1.0">America/Miquelon</option>
<option value="AST4ADT,M3.2.0,M11.1.0">America/Moncton</option>
<option value="CST6">America/Monterrey</option>
<option value="<-03>3">America/Montevideo</option>
<option value="EST5EDT,M3.2.0,M11.1.0">America/Montreal</option>
<option value="AST4">America/Montserrat</option>
<option value="EST5EDT,M3.2.0,M11.1.0">America/Nassau</option>
<option value="EST5EDT,M3.2.0,M11.1.0">America/New_York</option>
<option value="EST5EDT,M3.2.0,M11.1.0">America/Nipigon</option>
<option value="AKST9AKDT,M3.2.0,M11.1.0">America/Nome</option>
<option value="<-02>2">America/Noronha</option>
<option value="CST6CDT,M3.2.0,M11.1.0">America/North_Dakota/Beulah</option>
<option value="CST6CDT,M3.2.0,M11.1.0">America/North_Dakota/Center</option>
<option value="CST6CDT,M3.2.0,M11.1.0">America/North_Dakota/New_Salem</option>
<option value="<-02>2<-01>,M3.5.0/-1,M10.5.0/0">America/Nuuk</option>
<option value="CST6CDT,M3.2.0,M11.1.0">America/Ojinaga</option>
<option value="EST5">America/Panama</option>
<option value="EST5EDT,M3.2.0,M11.1.0">America/Pangnirtung</option>
<option value="<-03>3">America/Paramaribo</option>
<option value="MST7">America/Phoenix</option>
<option value="EST5EDT,M3.2.0,M11.1.0">America/Port-au-Prince</option>
<option value="AST4">America/Port_of_Spain</option>
<option value="<-04>4">America/Porto_Velho</option>
<option value="AST4">America/Puerto_Rico</option>
<option value="<-03>3">America/Punta_Arenas</option>
<option value="CST6CDT,M3.2.0,M11.1.0">America/Rainy_River</option>
<option value="CST6CDT,M3.2.0,M11.1.0">America/Rankin_Inlet</option>
<option value="<-03>3">America/Recife</option>
<option value="CST6">America/Regina</option>
<option value="CST6CDT,M3.2.0,M11.1.0">America/Resolute</option>
<option value="<-05>5">America/Rio_Branco</option>
<option value="<-03>3">America/Santarem</option>
<option value="<-04>4<-03>,M9.1.6/24,M4.1.6/24">America/Santiago</option>
<option value="AST4">America/Santo_Domingo</option>
<option value="<-03>3">America/Sao_Paulo</option>
<option value="<-02>2<-01>,M3.5.0/-1,M10.5.0/0">America/Scoresbysund</option>
<option value="AKST9AKDT,M3.2.0,M11.1.0">America/Sitka</option>
<option value="AST4">America/St_Barthelemy</option>
<option value="NST3:30NDT,M3.2.0,M11.1.0">America/St_Johns</option>
<option value="AST4">America/St_Kitts</option>
<option value="AST4">America/St_Lucia</option>
<option value="AST4">America/St_Thomas</option>
<option value="AST4">America/St_Vincent</option>
<option value="CST6">America/Swift_Current</option>
<option value="CST6">America/Tegucigalpa</option>
<option value="AST4ADT,M3.2.0,M11.1.0">America/Thule</option>
<option value="EST5EDT,M3.2.0,M11.1.0">America/Thunder_Bay</option>
<option value="PST8PDT,M3.2.0,M11.1.0">America/Tijuana</option>
<option value="EST5EDT,M3.2.0,M11.1.0">America/Toronto</option>
<option value="AST4">America/Tortola</option>
<option value="PST8PDT,M3.2.0,M11.1.0">America/Vancouver</option>
<option value="MST7">America/Whitehorse</option>
<option value="CST6CDT,M3.2.0,M11.1.0">America/Winnipeg</option>
<option value="AKST9AKDT,M3.2.0,M11.1.0">America/Yakutat</option>
<option value="MST7MDT,M3.2.0,M11.1.0">America/Yellowknife</option>
<option value="<+08>-8">Antarctica/Casey</option>
<option value="<+07>-7">Antarctica/Davis</option>
<option value="<+10>-10">Antarctica/DumontDUrville</option>
<option value="AEST-10AEDT,M10.1.0,M4.1.0/3">Antarctica/Macquarie</option>
<option value="<+05>-5">Antarctica/Mawson</option>
<option value="NZST-12NZDT,M9.5.0,M4.1.0/3">Antarctica/McMurdo</option>
<option value="<-03>3">Antarctica/Palmer</option>
<option value="<-03>3">Antarctica/Rothera</option>
<option value="<+03>-3">Antarctica/Syowa</option>
<option value="<+00>0<+02>-2,M3.5.0/1,M10.5.0/3">Antarctica/Troll</option>
<option value="<+05>-5">Antarctica/Vostok</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Arctic/Longyearbyen</option>
<option value="<+03>-3">Asia/Aden</option>
<option value="<+05>-5">Asia/Almaty</option>
<option value="<+03>-3">Asia/Amman</option>
<option value="<+12>-12">Asia/Anadyr</option>
<option value="<+05>-5">Asia/Aqtau</option>
<option value="<+05>-5">Asia/Aqtobe</option>
<option value="<+05>-5">Asia/Ashgabat</option>
<option value="<+05>-5">Asia/Atyrau</option>
<option value="<+03>-3">Asia/Baghdad</option>
<option value="<+03>-3">Asia/Bahrain</option>
<option value="<+04>-4">Asia/Baku</option>
<option value="<+07>-7">Asia/Bangkok</option>
<option value="<+07>-7">Asia/Barnaul</option>
<option value="EET-2EEST,M3.5.0/0,M10.5.0/0">Asia/Beirut</option>
<option value="<+06>-6">Asia/Bishkek</option>
<option value="<+08>-8">Asia/Brunei</option>
<option value="<+09>-9">Asia/Chita</option>
<option value="<+08>-8">Asia/Choibalsan</option>
<option value="<+0530>-5:30">Asia/Colombo</option>
<option value="<+03>-3">Asia/Damascus</option>
<option value="<+06>-6">Asia/Dhaka</option>
<option value="<+09>-9">Asia/Dili</option>
<option value="<+04>-4">Asia/Dubai</option>
<option value="<+05>-5">Asia/Dushanbe</option>
<option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Asia/Famagusta</option>
<option value="EET-2EEST,M3.4.4/50,M10.4.4/50">Asia/Gaza</option>
<option value="EET-2EEST,M3.4.4/50,M10.4.4/50">Asia/Hebron</option>
<option value="<+07>-7">Asia/Ho_Chi_Minh</option>
<option value="HKT-8">Asia/Hong_Kong</option>
<option value="<+07>-7">Asia/Hovd</option>
<option value="<+08>-8">Asia/Irkutsk</option>
<option value="WIB-7">Asia/Jakarta</option>
<option value="WIT-9">Asia/Jayapura</option>
<option value="IST-2IDT,M3.4.4/26,M10.5.0">Asia/Jerusalem</option>
<option value="<+0430>-4:30">Asia/Kabul</option>
<option value="<+12>-12">Asia/Kamchatka</option>
<option value="PKT-5">Asia/Karachi</option>
<option value="<+0545>-5:45">Asia/Kathmandu</option>
<option value="<+09>-9">Asia/Khandyga</option>
<option value="IST-5:30">Asia/Kolkata</option>
<option value="<+07>-7">Asia/Krasnoyarsk</option>
<option value="<+08>-8">Asia/Kuala_Lumpur</option>
<option value="<+08>-8">Asia/Kuching</option>
<option value="<+03>-3">Asia/Kuwait</option>
<option value="CST-8">Asia/Macau</option>
<option value="<+11>-11">Asia/Magadan</option>
<option value="WITA-8">Asia/Makassar</option>
<option value="PST-8">Asia/Manila</option>
<option value="<+04>-4">Asia/Muscat</option>
<option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Asia/Nicosia</option>
<option value="<+07>-7">Asia/Novokuznetsk</option>
<option value="<+07>-7">Asia/Novosibirsk</option>
<option value="<+06>-6">Asia/Omsk</option>
<option value="<+05>-5">Asia/Oral</option>
<option value="<+07>-7">Asia/Phnom_Penh</option>
<option value="WIB-7">Asia/Pontianak</option>
<option value="KST-9">Asia/Pyongyang</option>
<option value="<+03>-3">Asia/Qatar</option>
<option value="<+05>-5">Asia/Qyzylorda</option>
<option value="<+03>-3">Asia/Riyadh</option>
<option value="<+11>-11">Asia/Sakhalin</option>
<option value="<+05>-5">Asia/Samarkand</option>
<option value="KST-9">Asia/Seoul</option>
<option value="CST-8">Asia/Shanghai</option>
<option value="<+08>-8">Asia/Singapore</option>
<option value="<+11>-11">Asia/Srednekolymsk</option>
<option value="CST-8">Asia/Taipei</option>
<option value="<+05>-5">Asia/Tashkent</option>
<option value="<+04>-4">Asia/Tbilisi</option>
<option value="<+0330>-3:30">Asia/Tehran</option>
<option value="<+06>-6">Asia/Thimphu</option>
<option value="JST-9">Asia/Tokyo</option>
<option value="<+07>-7">Asia/Tomsk</option>
<option value="<+08>-8">Asia/Ulaanbaatar</option>
<option value="<+06>-6">Asia/Urumqi</option>
<option value="<+10>-10">Asia/Ust-Nera</option>
<option value="<+07>-7">Asia/Vientiane</option>
<option value="<+10>-10">Asia/Vladivostok</option>
<option value="<+09>-9">Asia/Yakutsk</option>
<option value="<+0630>-6:30">Asia/Yangon</option>
<option value="<+05>-5">Asia/Yekaterinburg</option>
<option value="<+04>-4">Asia/Yerevan</option>
<option value="<-01>1<+00>,M3.5.0/0,M10.5.0/1">Atlantic/Azores</option>
<option value="AST4ADT,M3.2.0,M11.1.0">Atlantic/Bermuda</option>
<option value="WET0WEST,M3.5.0/1,M10.5.0">Atlantic/Canary</option>
<option value="<-01>1">Atlantic/Cape_Verde</option>
<option value="WET0WEST,M3.5.0/1,M10.5.0">Atlantic/Faroe</option>
<option value="WET0WEST,M3.5.0/1,M10.5.0">Atlantic/Madeira</option>
<option value="GMT0">Atlantic/Reykjavik</option>
<option value="<-02>2">Atlantic/South_Georgia</option>
<option value="<-03>3">Atlantic/Stanley</option>
<option value="GMT0">Atlantic/St_Helena</option>
<option value="ACST-9:30ACDT,M10.1.0,M4.1.0/3">Australia/Adelaide</option>
<option value="AEST-10">Australia/Brisbane</option>
<option value="ACST-9:30ACDT,M10.1.0,M4.1.0/3">Australia/Broken_Hill</option>
<option value="AEST-10AEDT,M10.1.0,M4.1.0/3">Australia/Currie</option>
<option value="ACST-9:30">Australia/Darwin</option>
<option value="<+0845>-8:45">Australia/Eucla</option>
<option value="AEST-10AEDT,M10.1.0,M4.1.0/3">Australia/Hobart</option>
<option value="AEST-10">Australia/Lindeman</option>
<option value="<+1030>-10:30<+11>-11,M10.1.0,M4.1.0">Australia/Lord_Howe</option>
<option value="AEST-10AEDT,M10.1.0,M4.1.0/3">Australia/Melbourne</option>
<option value="AWST-8">Australia/Perth</option>
<option value="AEST-10AEDT,M10.1.0,M4.1.0/3">Australia/Sydney</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Amsterdam</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Andorra</option>
<option value="<+04>-4">Europe/Astrakhan</option>
<option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Europe/Athens</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Belgrade</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Berlin</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Bratislava</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Brussels</option>
<option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Europe/Bucharest</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Budapest</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Busingen</option>
<option value="EET-2EEST,M3.5.0,M10.5.0/3">Europe/Chisinau</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Copenhagen</option>
<option value="IST-1GMT0,M10.5.0,M3.5.0/1">Europe/Dublin</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Gibraltar</option>
<option value="GMT0BST,M3.5.0/1,M10.5.0">Europe/Guernsey</option>
<option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Europe/Helsinki</option>
<option value="GMT0BST,M3.5.0/1,M10.5.0">Europe/Isle_of_Man</option>
<option value="<+03>-3">Europe/Istanbul</option>
<option value="GMT0BST,M3.5.0/1,M10.5.0">Europe/Jersey</option>
<option value="EET-2">Europe/Kaliningrad</option>
<option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Europe/Kiev</option>
<option value="MSK-3">Europe/Kirov</option>
<option value="WET0WEST,M3.5.0/1,M10.5.0">Europe/Lisbon</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Ljubljana</option>
<option value="GMT0BST,M3.5.0/1,M10.5.0">Europe/London</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Luxembourg</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Madrid</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Malta</option>
<option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Europe/Mariehamn</option>
<option value="<+03>-3">Europe/Minsk</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Monaco</option>
<option value="MSK-3">Europe/Moscow</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Oslo</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Paris</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Podgorica</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Prague</option>
<option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Europe/Riga</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Rome</option>
<option value="<+04>-4">Europe/Samara</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/San_Marino</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Sarajevo</option>
<option value="<+04>-4">Europe/Saratov</option>
<option value="MSK-3">Europe/Simferopol</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Skopje</option>
<option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Europe/Sofia</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Stockholm</option>
<option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Europe/Tallinn</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Tirane</option>
<option value="<+04>-4">Europe/Ulyanovsk</option>
<option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Europe/Uzhgorod</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Vaduz</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Vatican</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Vienna</option>
<option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Europe/Vilnius</option>
<option value="MSK-3">Europe/Volgograd</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Warsaw</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Zagreb</option>
<option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Europe/Zaporozhye</option>
<option value="CET-1CEST,M3.5.0,M10.5.0/3">Europe/Zurich</option>
<option value="EAT-3">Indian/Antananarivo</option>
<option value="<+06>-6">Indian/Chagos</option>
<option value="<+07>-7">Indian/Christmas</option>
<option value="<+0630>-6:30">Indian/Cocos</option>
<option value="EAT-3">Indian/Comoro</option>
<option value="<+05>-5">Indian/Kerguelen</option>
<option value="<+04>-4">Indian/Mahe</option>
<option value="<+05>-5">Indian/Maldives</option>
<option value="<+04>-4">Indian/Mauritius</option>
<option value="EAT-3">Indian/Mayotte</option>
<option value="<+04>-4">Indian/Reunion</option>
<option value="<+13>-13">Pacific/Apia</option>
<option value="NZST-12NZDT,M9.5.0,M4.1.0/3">Pacific/Auckland</option>
<option value="<+11>-11">Pacific/Bougainville</option>
<option value="<+1245>-12:45<+1345>,M9.5.0/2:45,M4.1.0/3:45">Pacific/Chatham</option>
<option value="<+10>-10">Pacific/Chuuk</option>
<option value="<-06>6<-05>,M9.1.6/22,M4.1.6/22">Pacific/Easter</option>
<option value="<+11>-11">Pacific/Efate</option>
<option value="<+13>-13">Pacific/Enderbury</option>
<option value="<+13>-13">Pacific/Fakaofo</option>
<option value="<+12>-12">Pacific/Fiji</option>
<option value="<+12>-12">Pacific/Funafuti</option>
<option value="<-06>6">Pacific/Galapagos</option>
<option value="<-09>9">Pacific/Gambier</option>
<option value="<+11>-11">Pacific/Guadalcanal</option>
<option value="ChST-10">Pacific/Guam</option>
<option value="HST10">Pacific/Honolulu</option>
<option value="<+14>-14">Pacific/Kiritimati</option>
<option value="<+11>-11">Pacific/Kosrae</option>
<option value="<+12>-12">Pacific/Kwajalein</option>
<option value="<+12>-12">Pacific/Majuro</option>
<option value="<-0930>9:30">Pacific/Marquesas</option>
<option value="SST11">Pacific/Midway</option>
<option value="<+12>-12">Pacific/Nauru</option>
<option value="<-11>11">Pacific/Niue</option>
<option value="<+11>-11<+12>,M10.1.0,M4.1.0/3">Pacific/Norfolk</option>
<option value="<+11>-11">Pacific/Noumea</option>
<option value="SST11">Pacific/Pago_Pago</option>
<option value="<+09>-9">Pacific/Palau</option>
<option value="<-08>8">Pacific/Pitcairn</option>
<option value="<+11>-11">Pacific/Pohnpei</option>
<option value="<+10>-10">Pacific/Port_Moresby</option>
<option value="<-10>10">Pacific/Rarotonga</option>
<option value="ChST-10">Pacific/Saipan</option>
<option value="<-10>10">Pacific/Tahiti</option>
<option value="<+12>-12">Pacific/Tarawa</option>
<option value="<+13>-13">Pacific/Tongatapu</option>
<option value="<+12>-12">Pacific/Wake</option>
<option value="<+12>-12">Pacific/Wallis</option>
<option value="GMT0">Etc/GMT</option>
<option value="GMT0">Etc/GMT-0</option>
<option value="<+01>-1">Etc/GMT-1</option>
<option value="<+02>-2">Etc/GMT-2</option>
<option value="<+03>-3">Etc/GMT-3</option>
<option value="<+04>-4">Etc/GMT-4</option>
<option value="<+05>-5">Etc/GMT-5</option>
<option value="<+06>-6">Etc/GMT-6</option>
<option value="<+07>-7">Etc/GMT-7</option>
<option value="<+08>-8">Etc/GMT-8</option>
<option value="<+09>-9">Etc/GMT-9</option>
<option value="<+10>-10">Etc/GMT-10</option>
<option value="<+11>-11">Etc/GMT-11</option>
<option value="<+12>-12">Etc/GMT-12</option>
<option value="<+13>-13">Etc/GMT-13</option>
<option value="<+14>-14">Etc/GMT-14</option>
<option value="GMT0">Etc/GMT0</option>
<option value="GMT0">Etc/GMT+0</option>
<option value="<-01>1">Etc/GMT+1</option>
<option value="<-02>2">Etc/GMT+2</option>
<option value="<-03>3">Etc/GMT+3</option>
<option value="<-04>4">Etc/GMT+4</option>
<option value="<-05>5">Etc/GMT+5</option>
<option value="<-06>6">Etc/GMT+6</option>
<option value="<-07>7">Etc/GMT+7</option>
<option value="<-08>8">Etc/GMT+8</option>
<option value="<-09>9">Etc/GMT+9</option>
<option value="<-10>10">Etc/GMT+10</option>
<option value="<-11>11">Etc/GMT+11</option>
<option value="<-12>12">Etc/GMT+12</option>
<option value="UTC0">Etc/UCT</option>
<option value="UTC0">Etc/UTC</option>
<option value="GMT0">Etc/Greenwich</option>
<option value="UTC0">Etc/Universal</option>
<option value="UTC0">Etc/Zulu</option>
    )html";

    html += "</select><br><br>";
    html += "<input type=\"submit\" value=\"Set Time Zone\">";
    html += "</form>";

    // Alarm configuration form
    html += "<h2>Set Alarm</h2>";
    html += "<form action=\"/setAlarm\" method=\"POST\">";
    html += "<label for=\"alrm\">Alarm Time:</label>";
    html += "<input type=\"time\" id=\"alrm\" name=\"alrm\" value=\"" + alrm + "\"><br><br>";

    // Days of the week checkboxes
    html += "<label>Days:</label><br>";
    const char* daysOfWeek[] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };
    for (int i = 0; i < 7; i++) {
      html += "<input type=\"checkbox\" name=\"alarm" + String(daysOfWeek[i]) + "\"";
      if (alarmDays[i]) html += " checked";  // Preserve checkbox state
      html += "> " + String(daysOfWeek[i]) + "<br>";
    }

    html += "<br><input type=\"submit\" value=\"Set Alarm\">";
    html += "</form></body></html>";


    // 2nd Alarm configuration form
    html += "<h2>Set 2nd Alarm</h2>";
    html += "<form action=\"/setAlarm2\" method=\"POST\">";
    html += "<label for=\"alrm2\">Alarm Time:</label>";
    html += "<input type=\"time\" id=\"alrm2\" name=\"alrm2\" value=\"" + alrm2 + "\"><br><br>";

    // 2nd alarm days of the week checkboxes
    html += "<label>Days:</label><br>";
    const char* daysOfWeek2[] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };
    for (int i = 0; i < 7; i++) {
      html += "<input type=\"checkbox\" name=\"alarm2" + String(daysOfWeek2[i]) + "\"";
      if (alarmDays2[i]) html += " checked";  // Preserve checkbox state
      html += "> " + String(daysOfWeek2[i]) + "<br>";
    }

    html += "<br><input type=\"submit\" value=\"Set Alarm\">";
    html += "</form></body></html>";

    server.send(200, "text/html", html);
  });

  // Handle timezone change
  server.on("/setTimezone", HTTP_POST, []() {
    if (server.hasArg("timezone")) {
      String timezone = server.arg("timezone");
      newTimeZone(timezone);
    }
    server.send(200, "text/html", "<html><body><h2>Timezone Updated</h2><a href=\"/\">Go Back</a></body></html>");
  });

  // Handle alarm settings update
  server.on("/setAlarm", HTTP_POST, []() {
    if (server.hasArg("alrm")) {
      alrm = server.arg("alrm");
      Serial.printf("Alarm Time set to: %s\n", alrm.c_str());
    }

    const char* daysOfWeek[] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };
    for (int i = 0; i < 7; i++) {
      // Check if each day is selected
      alarmDays[i] = server.hasArg("alarm" + String(daysOfWeek[i]));
      Serial.printf("Alarm for %s: %s\n", daysOfWeek[i], alarmDays[i] ? "On" : "Off");
    }

    server.send(200, "text/html", "<html><body><h2>Alarm Settings Updated</h2><a href=\"/\">Go Back</a></body></html>");
  });

  // Handle 2nd alarm settings update
  server.on("/setAlarm2", HTTP_POST, []() {
    if (server.hasArg("alrm2")) {
      alrm2 = server.arg("alrm2");
      Serial.printf("Alarm Time set to: %s\n", alrm2.c_str());
    }

    const char* daysOfWeek2[] = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };
    for (int i = 0; i < 7; i++) {
      // Check if each day is selected
      alarmDays2[i] = server.hasArg("alarm2" + String(daysOfWeek2[i]));
      Serial.printf("Alarm for %s: %s\n", daysOfWeek2[i], alarmDays2[i] ? "On" : "Off");
    }

    server.send(200, "text/html", "<html><body><h2>Alarm Settings Updated</h2><a href=\"/\">Go Back</a></body></html>");
  });

  server.begin();
}

void loop() {
  // Always check if the trigger pin is pressed for Wi-Fi config portal
  if (digitalRead(TRIGGER_PIN) == LOW) {
    WiFiManager wm;
    wm.setConfigPortalTimeout(120);  // 120 seconds timeout for the portal

    if (!wm.startConfigPortal("E-Paper Clock")) {
      Serial.println("Failed to connect, restarting...");
      ESP.restart();
    }
    Serial.println("Connected to Wi-Fi");
  }
  server.handleClient();
  checkAlarm();
  checkAlarm2();
  if (buttonPressed == true && ledActivated == 0) {
    digitalWrite(ledPin, HIGH);
    ledActivated = 1;
    led_time = millis();
  }

  else if (buttonPressed == true && ledActivated == 1) {
    buttonPressed = false;
    display.clear();
    display.fastmodeOff();
    display.setFont( &FreeSans9pt7b );
    //display.setFont(ArialMT_Plain_10);
    display.setCursor(0, 15);
    display.println("Change clock configuration by going to the following URL in your browser ");
    display.print("clock.local,");
    display.print(" or alternatively go to ");
    display.println(WiFi.localIP());
    display.print(" instead.");
    display.update();
    display.fastmodeOn();
    buttonPressed = false;
  }

  // check led timer
  if (led_time - last_led_time > 30000) { // 30 000 millis = 30 seconds
    last_led_time = led_time;
    ledActivated = 0;
    digitalWrite(ledPin, LOW);
  }

  delay(1000);
  printLocalTime();
}

void printLocalTime() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  char timeBuffer[50];
  char timeBufferHr[50];
  char dateBuffer[50];
  char min[10];
  char hr[10];
  strftime(timeBuffer, sizeof(timeBuffer), "%H %M", &timeinfo); //Creating timeBuffer string from NTP timeinfo String
  strftime(timeBufferHr, sizeof(timeBufferHr), "%H", &timeinfo); //Creating timeBufferHr string from NTP timeinfo String
  strftime(dateBuffer, sizeof(dateBuffer), "%A, %B %d %Y", &timeinfo); //Creating dateBuffer string from NTP timeinfo String
  // Time in min and hr used that will be displayed
  strftime(min, sizeof(min), "%M", &timeinfo); //Creating dateBuffer string from NTP timeinfo String
  strftime(hr, sizeof(hr), "%H", &timeinfo); //Creating dateBuffer string from NTP timeinfo String


  if (timeBufferHr != lastDisplayedHr) { //The minute = 0 (New hour), then do full refresh of e-paper
      display.fastmodeOff();
      display.clear();
      delay(2000);
      display.fastmodeOn();
      DRAW (display) {
      printTime(min, hr, dateBuffer);
      }
    }
    else if (timeBuffer != lastDisplayedTime) {
    Serial.println("Hour buffer");
    Serial.println(timeBufferHr);
    Serial.println("Time Buffer");
    Serial.println(timeBuffer);
    display.setWindow( display.left(), display.top(), display.width(), display.height() /* - 35 */ ); // Don't overwrite the bottom 35px
    DRAW (display) {
    printTime(min, hr, dateBuffer);
    }
      
  }

  lastDisplayedTime = timeBuffer;  // Update the stored time
  lastDisplayedHr = timeBufferHr;  // Update the stored hour
}

void newTimeZone(const String& timezone) {
  Serial.println(timezone);
  setenv("TZ", timezone.c_str(), 1);
  tzset();
}

void checkAlarm() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;
  }

  // Check if the current day is selected for the alarm
  int dayIndex = (timeinfo.tm_wday + 6) % 7;  // Adjusting for Sunday as 0 in `tm_wday`
  if (!alarmDays[dayIndex]) {
    alarmTriggered = false;  // Reset alarm if it's a new day
    return;
  }

  // Format current time and check if it matches the alarm time
  char currentTime[6];
  strftime(currentTime, sizeof(currentTime), "%H:%M", &timeinfo);
  if (alrm == currentTime && !alarmTriggered) {
    Serial.println("Alarm! It's time to wake up!");
    // Add actions here, e.g., turn on LED or buzzer
    Alarm();
    alarmTriggered = true;  // Set to true to prevent retriggering within the same minute
  } else if (alrm != currentTime) {
    alarmTriggered = false;  // Reset when current time is different
  }
}

void checkAlarm2() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;
  }

  // Check if the current day is selected for the alarm
  int dayIndex2 = (timeinfo.tm_wday + 6) % 7;  // Adjusting for Sunday as 0 in `tm_wday`
  if (!alarmDays2[dayIndex2]) {
    alarmTriggered2 = false;  // Reset alarm if it's a new day
    return;
  }

  // Format current time and check if it matches the alarm time
  char currentTime[6];
  strftime(currentTime, sizeof(currentTime), "%H:M", &timeinfo);
  if (alrm2 == currentTime && !alarmTriggered2) {
    Serial.println("Alarm! It's time to wake up!");
    // Add actions here, e.g., turn on LED or buzzer
    alarmTriggered2 = true;  // Set to true to prevent retriggering within the same minute
    Alarm();
  } else if (alrm2 != currentTime) {
    alarmTriggered2 = false;  // Reset when current time is different
  }
}

void Alarm() {
  alarmOn = true;
  buttonState = digitalRead(buttonPin);
  Serial.println("Alarm Buzzer activated");
  for (int i = 0; i < 60; i++) {
  tone(buzzer, 1000); // Send 1KHz sound signal...
  delay(1000);        // ...for 1 sec
  noTone(buzzer);     // Stop sound...
  delay(1000);        // ...for 1sec
  digitalWrite(ledPin, HIGH);
  ledActivated = 1;
  Serial.println(buttonState);
  Serial.println(i);
  if (buttonPressed == true) {
    buttonPressed = false;
    alarmOn = false;
    digitalWrite(ledPin, LOW);
    ledActivated = 0;
    noTone(buzzer);     // Stop sound...
    break; // exit for loop
  }
  }
  }


void VextON(void) {
  pinMode(18, OUTPUT);
  digitalWrite(18, HIGH);
}

void printTime(const String& min, const String& hr, const String& db) { //tb = timebuffer
  display.setFont( &FreeSans40pt7b );
  display.setCursor(((196 - 126) / 2) + 15 /* + C, where C is an adjustment to get time centered */, 80); //aprox length of time is 126pixels used measurements of screen
  display.print(hr);
  display.print(":");
  display.print(min);
  printDate(db);
}

void printDate(const String& db) { // db = datebuffer
  Serial.println("Print Date");
  display.setFont( &FreeSans9pt7b );
  int textWidth = strlen(db.c_str()) * 9;
  display.setCursor((296 - textWidth) / 2, 114);
  //display.setWindow( display.left(), display.top() + 100, display.width(), display.height() ); // Don't overwrite the bottom 35px
  display.print(db);
}



