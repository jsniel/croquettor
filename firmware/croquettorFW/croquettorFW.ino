/*
 * Croquettor Firmware
 * 
 * Author: Jean-Sebastien Niel
 * GitHub: https://github.com/jsniel
 * License: MIT
 */
 
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>

#define BUTTON_PIN 10
#define LED_PIN 2
#define ENDSTOP_PIN 3
#define MOTOR_PIN 4
#define RTC_INT_PIN 5
#define SDAPIN 8
#define SCLPIN 9
#define EEPROM_SIZE 64

RTC_DS3231 rtc;

const char* ssid = "croquettor";
const char* password = "12345678";

WebServer server(80);
bool webServerEnabled = false;
unsigned long webServerStartTime = 0;
const unsigned long WEB_SERVER_DURATION = 1 * 600 * 1000UL;
DNSServer dnsServer;

bool serving = false;
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

bool lastButtonState = HIGH;
bool buttonStateStable = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
unsigned long lastCheckTime = 0;

bool rtcOk = false;
unsigned long lastBlinkTime = 0;
bool ledState = false;
unsigned long bootTime = 0;

struct AutoFeedConfig {
  bool enabled;
  uint8_t hour;
  uint8_t minute;
  uint8_t doses;
};

AutoFeedConfig autoFeed1, autoFeed2;
bool hasDistributed1 = false;
bool hasDistributed2 = false;

void saveConfig() {
  EEPROM.put(0, autoFeed1);
  EEPROM.put(sizeof(AutoFeedConfig), autoFeed2);
  EEPROM.commit();
}

void loadConfig() {
  EEPROM.get(0, autoFeed1);
  EEPROM.get(sizeof(AutoFeedConfig), autoFeed2);
  if (autoFeed1.doses == 0 || autoFeed1.doses > 5) autoFeed1 = {false, 8, 0, 1};
  if (autoFeed2.doses == 0 || autoFeed2.doses > 5) autoFeed2 = {false, 20, 0, 1};
}

String getFormattedTime() {
  DateTime now = rtc.now();
  char buffer[6];
  sprintf(buffer, "%02d:%02d", now.hour(), now.minute());
  return String(buffer);
}

void distributeCroquettes(uint8_t doses = 1) {
  for (uint8_t i = 0; i < doses; i++) {
    Serial.printf("Distribution %d/%d...\n", i + 1, doses);
    serving = true;
    digitalWrite(MOTOR_PIN, HIGH);
    while (digitalRead(ENDSTOP_PIN) == LOW) delay(50);
    while (digitalRead(ENDSTOP_PIN) == HIGH) delay(50);
    digitalWrite(MOTOR_PIN, LOW);
    serving = false;
    delay(500);
  }
}

String generateOptions(int max, int selected) {
  String options;
  for (int i = 0; i <= max; i++) {
    options += "<option value='" + String(i) + "'" + (i == selected ? " selected" : "") + ">" + String(i) + "</option>";
  }
  return options;
}

String generateHTML() {
  String page = R"rawliteral(
  <!DOCTYPE html>
  <html lang='fr'>
  <head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Croquettor</title>
    <style>
      body { font-family: Arial, sans-serif; text-align: center; background-color: #f8f3e3; padding: 20px; }
      h1 { color: #6a4c93; }
      button {
        background-color: #ffcb77;
        border: none;
        padding: 15px 30px;
        font-size: 24px;
        color: white;
        cursor: pointer;
        border-radius: 10px;
        margin-top: 20px;
        box-shadow: 2px 2px 10px rgba(0,0,0,0.2);
      }
      button:active { background-color: #ff9f1c; }
      .form-container {
        margin-top: 40px;
        background-color: #fff;
        padding: 20px;
        border-radius: 12px;
        display: inline-block;
        box-shadow: 2px 2px 15px rgba(0,0,0,0.1);
      }
      select { padding: 10px; font-size: 16px; margin: 5px; }
    </style>
  </head>
  <body>
    <div style='position: absolute; top: 10px; left: 10px; font-size: 18px; color: #333;'>Temps restant : <span id='countdown'>--</span>s</div>
    <div id='clock'>--:--</div>
    <h1>Distributeur de Croquettes</h1>
    <button onclick="fetch('/feed')">Croquettes !</button>

    <div class='form-container'>
      <h2>Distribution automatique 1</h2>
      <form method='GET' action='/setauto'>
        <input type='hidden' name='id' value='1'>
        <label><input type='checkbox' name='en' value='1' %AUTO1_CHECKED%> Activer</label><br>
        Heure: <select name='h'>%HOUR1%</select>
        Minute: <select name='m'>%MIN1%</select><br>        Dose: <select name='d'>%DOSE1%</select><br>
        <button type='submit'>Configurer</button>
      </form>
      <p>Heure enregistrée : %AUTO1%</p>
    </div>

    <div class='form-container'>
      <h2>Distribution automatique 2</h2>
      <form method='GET' action='/setauto'>
        <input type='hidden' name='id' value='2'>
        <label><input type='checkbox' name='en' value='1' %AUTO2_CHECKED%> Activer</label><br>
        Heure: <select name='h'>%HOUR2%</select>
        Minute: <select name='m'>%MIN2%</select><br>        Dose: <select name='d'>%DOSE2%</select><br>
        <button type='submit'>Configurer</button>
      </form>
      <p>Heure enregistrée : %AUTO2%</p>
    </div>

    <div class='form-container'>
      <h2>Mettre à jour l'heure</h2>
      <form method='GET' action='/settime'>
        Heure: <select name='h'>%HOURS%</select>
        Minute: <select name='m'>%MINUTES%</select>
        <button type='submit'>Définir l'heure</button>
      </form>
    </div>

    <script>
      async function updateClock() {
        try {
          const res = await fetch('/time');
          const text = await res.text();
          document.getElementById('clock').textContent = text;
        } catch (e) {
          document.getElementById('clock').textContent = '--:--';
        }
      }

      let remaining = parseInt('%SERVER_REMAINING%');
      if (isNaN(remaining)) remaining = 0;

      function updateCountdown() {
        const countdownEl = document.getElementById('countdown');
        if (remaining > 0) {
          countdownEl.textContent = remaining;
          countdownEl.style.color = (remaining <= 10) ? 'red' : '#333';
          remaining--;
        } else {
          countdownEl.textContent = '--';
          countdownEl.style.color = '#333';
        }
      }

      setInterval(updateClock, 10000);
      setInterval(updateCountdown, 1000);
      updateClock();
      updateCountdown();
    </script>
  </body>
  </html>
  )rawliteral";

  DateTime now = rtc.now();
  page.replace("%HOURS%", generateOptions(23, now.hour()));
  page.replace("%MINUTES%", generateOptions(59, now.minute()));
  page.replace("%HOUR1%", generateOptions(23, autoFeed1.hour));
  page.replace("%MIN1%", generateOptions(59, autoFeed1.minute));
  page.replace("%DOSE1%", generateOptions(5, autoFeed1.doses));
  page.replace("%HOUR2%", generateOptions(23, autoFeed2.hour));
  page.replace("%MIN2%", generateOptions(59, autoFeed2.minute));
  page.replace("%DOSE2%", generateOptions(5, autoFeed2.doses));
  page.replace("%AUTO1%", String(autoFeed1.hour) + ":" + (autoFeed1.minute < 10 ? "0" : "") + String(autoFeed1.minute) + " (" + String(autoFeed1.doses) + " dose(s))");
  page.replace("%AUTO2%", String(autoFeed2.hour) + ":" + (autoFeed2.minute < 10 ? "0" : "") + String(autoFeed2.minute) + " (" + String(autoFeed2.doses) + " dose(s))");
  page.replace("%AUTO1_CHECKED%", autoFeed1.enabled ? "checked" : "");
  page.replace("%AUTO2_CHECKED%", autoFeed2.enabled ? "checked" : "");

  unsigned long remainingTime = WEB_SERVER_DURATION;
  if (webServerEnabled && millis() - webServerStartTime < WEB_SERVER_DURATION) {
    remainingTime = WEB_SERVER_DURATION - (millis() - webServerStartTime);
  }
  page.replace("%SERVER_REMAINING%", String(remainingTime / 1000));

  return page;
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  EEPROM.begin(EEPROM_SIZE);
  loadConfig();

  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(ENDSTOP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

  Wire.begin(SDAPIN, SCLPIN);
  rtcOk = rtc.begin();
  if (!rtcOk) {
    Serial.println("Erreur : RTC non détectée !");
  } else {
    if (rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
  dnsServer.start(53, "", WiFi.softAPIP());

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", generateHTML());
  });

  server.on("/feed", HTTP_GET, []() {
    distributeCroquettes();
    server.send(200, "text/plain", "Croquettes distribuées");
  });

  server.on("/time", HTTP_GET, []() {
    if (rtcOk) {
      server.send(200, "text/plain", getFormattedTime());
    } else {
      server.send(503, "text/plain", "RTC non disponible");
    }
  });

  server.on("/settime", HTTP_GET, []() {
    if (server.hasArg("h") && server.hasArg("m")) {
      int h = server.arg("h").toInt();
      int m = server.arg("m").toInt();
      DateTime now = rtc.now();
      rtc.adjust(DateTime(now.year(), now.month(), now.day(), h, m, 0));
      server.sendHeader("Location", "/", true);
      server.send(302, "Heure mise à jour", "");
    } else {
      server.send(400, "text/plain", "Paramètres manquants");
    }
  });

  server.on("/setauto", HTTP_GET, []() {
    if (server.hasArg("id") && server.hasArg("h") && server.hasArg("m") && server.hasArg("d")) {
      int id = server.arg("id").toInt();
      bool en = server.hasArg("en");
      int h = server.arg("h").toInt();
      int m = server.arg("m").toInt();
      int d = server.arg("d").toInt();

      if (id == 1) { autoFeed1 = {en, (uint8_t)h, (uint8_t)m, (uint8_t)d}; hasDistributed1 = false; }
      if (id == 2) { autoFeed2 = {en, (uint8_t)h, (uint8_t)m, (uint8_t)d}; hasDistributed2 = false; }

      saveConfig();
      server.sendHeader("Location", "/", true);
      server.send(302, "Configuration enregistrée", "");
    } else {
      server.send(400, "text/plain", "Paramètres manquants");
    }
  });

  server.onNotFound([]() {
    server.sendHeader("Location", "http://" + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "Redirecting...");
  });

  bootTime = millis();
  // server.begin();  // Lancement à la demande
}

void loop() {
  dnsServer.processNextRequest();
  if (webServerEnabled) server.handleClient();

  handleButton();

  unsigned long nowMillis = millis();
  if (nowMillis - lastCheckTime >= 10000) {
    if (webServerEnabled && (millis() - webServerStartTime > WEB_SERVER_DURATION)) {
      Serial.println("Extinction automatique du serveur web");
      server.stop();
      WiFi.softAPdisconnect(true);
      webServerEnabled = false;
    }

    lastCheckTime = nowMillis;
    DateTime now = rtc.now();
    rtcOk = now.isValid() && now.hour() < 24 && now.minute() < 60;

    if (rtcOk && millis() - bootTime > 5000) {
      if (autoFeed1.enabled && now.hour() == autoFeed1.hour && now.minute() == autoFeed1.minute && !hasDistributed1) {
        distributeCroquettes(autoFeed1.doses);
        hasDistributed1 = true;
      }
      if (autoFeed2.enabled && now.hour() == autoFeed2.hour && now.minute() == autoFeed2.minute && !hasDistributed2) {
        distributeCroquettes(autoFeed2.doses);
        hasDistributed2 = true;
      }
      if (now.minute() != autoFeed1.minute) hasDistributed1 = false;
      if (now.minute() != autoFeed2.minute) hasDistributed2 = false;
    }
  }

  if (!rtcOk || !rtc.begin()) {
    if (millis() - lastBlinkTime >= 500) {
      lastBlinkTime = millis();
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
    }
  } else {
    if (webServerEnabled) {
      unsigned long patternTime = millis() % 1200;
      if (patternTime < 200 || (patternTime >= 400 && patternTime < 600)) {
        digitalWrite(LED_PIN, HIGH);
      } else {
        digitalWrite(LED_PIN, LOW);
      }
    } else {
      digitalWrite(LED_PIN, (autoFeed1.enabled || autoFeed2.enabled) ? HIGH : LOW);
    }
  }
}

void handleButton() {
  if (millis() - bootTime < 3000) return; // Protection au démarrage

  static bool lastButtonState = HIGH;
  static unsigned long lastDebounceTime = 0;
  static bool buttonStateStable = HIGH;
  static unsigned long buttonPressStart = 0;
  static bool longPressHandled = false;

  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonStateStable) {
      buttonStateStable = reading;

      if (buttonStateStable == LOW) {
        buttonPressStart = millis();
        longPressHandled = false;
      } else {
        unsigned long pressDuration = millis() - buttonPressStart;
        if (pressDuration < 3000 && !longPressHandled) {
          Serial.println("Appui bouton court détecté - distribution manuelle");
          distributeCroquettes();
        }
        buttonPressStart = 0;
        longPressHandled = false;
      }
    } else if (buttonStateStable == LOW && !longPressHandled) {
      if (millis() - buttonPressStart >= 3000) {
        Serial.println("Appui long détecté - activation du serveur web");
        WiFi.softAPConfig(local_IP, gateway, subnet);
        WiFi.softAP(ssid, password);
        dnsServer.start(53, "", WiFi.softAPIP());
        server.begin();
        webServerEnabled = true;
        webServerStartTime = millis();
        longPressHandled = true;
      }
    }
  }

  lastButtonState = reading;
}

