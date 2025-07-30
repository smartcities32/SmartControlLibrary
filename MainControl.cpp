// MainControl.cpp
#include "MainControl.h"

#ifdef USE_EXTERNAL_EEPROM
MainControlClass::MainControlClass(WebServer& serverRef, int relayPin)
    : _server(serverRef), _relayPin(relayPin) {
}
#else
MainControlClass::MainControlClass(WebServer& serverRef, int relayPin, EEPROMClass& eepromRef)
    : _server(serverRef), _relayPin(relayPin), _eeprom(eepromRef) {
}
#endif

void MainControlClass::beginAPAndWebServer(const char* ap_ssid, const char* ap_password) {
    Wire.begin(); 

#ifndef USE_EXTERNAL_EEPROM
    if (!_eeprom.begin(EEPROM_SIZE)) {
        Serial.println("فشل تهيئة EEPROM الداخلية!");
        Serial.println("إعادة التشغيل...");
        delay(1000);
        ESP.restart();
    }
    Serial.println("تم تهيئة EEPROM الداخلية بنجاح.");
#else
    Serial.println("EEPROM الخارجية (24C256) مفترضة مهيأة عبر Wire.begin().");
#endif

    pinMode(_relayPin, OUTPUT);
    bool savedState = getRelayStateFromEEPROM();
    digitalWrite(_relayPin, savedState ? HIGH : LOW);
    Serial.print("الحالة الأولية للمرحل من EEPROM: ");
    Serial.println(savedState ? "تشغيل" : "إيقاف");

    String ssid = readStringFromEEPROM(SSID_ADDR, SSID_MAX_LEN);
    String password = readStringFromEEPROM(PASSWORD_ADDR, PASSWORD_MAX_LEN);

    if (ssid.isEmpty() || password.isEmpty() || ssid == "\0" || password == "\0") { 
        Serial.println("SSID أو كلمة المرور غير مضبوطة في EEPROM. استخدام بيانات AP الافتراضية.");
        ssid = ap_ssid;
        password = ap_password;
        saveStringToEEPROM(SSID_ADDR, ap_ssid, SSID_MAX_LEN);
        saveStringToEEPROM(PASSWORD_ADDR, ap_password, PASSWORD_MAX_LEN);
    } 
    Serial.print("إعداد نقطة الوصول (AP): ");
    Serial.println(ssid);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("عنوان IP لنقطة الوصول (AP): ");
    Serial.println(IP);

    _server.on("/api/wifi/set_ssid", HTTP_POST, [this]() { handleSetSSID(); });
    _server.on("/api/wifi/get_ssid", HTTP_GET, [this]() { handleGetSSID(); });
    _server.on("/api/wifi/set_password", HTTP_POST, [this]() { handleSetPassword(); });
    _server.on("/api/wifi/get_password", HTTP_GET, [this]() { handleGetPassword(); });
    _server.on("/api/wifi/get_network_info", HTTP_GET, [this]() { handleGetnetworkinfo(); });
    _server.on("/api/wifi/set_network_info", HTTP_POST, [this]() { handleSetnetworkinfo(); });

    _server.on("/api/relay/set_state", HTTP_POST, [this]() { handleSetRelayState(); });
    _server.on("/api/relay/get_state", HTTP_GET, [this]() { handleGetRelayState(); });
    _server.on("/api/relay/toggle", HTTP_POST, [this]() { handleToggleRelay(); });

    _server.on("/api/reset", HTTP_POST, [this]() { resetConfigurations(); }); 

    _server.onNotFound([this]() { handleNotFound(); });

    _server.begin();
    Serial.println("تم بدء تشغيل خادم HTTP");
}

void MainControlClass::handleClient() {
    _server.handleClient();
}

void MainControlClass::resetConfigurations() {
    Serial.println("إعادة تعيين الإعدادات...");
    EEPROMHelper::writeInt(USER_TAG_COUNT_ADDR, 0); // إعادة تعيين عدد المستخدمين
    saveRelayStateToEEPROM(false); // إيقاف المرحل
    EEPROMHelper::writeByte(LAST_SCHEDULE_ID_ADDR, 0); // إعادة تعيين آخر معرف جدول زمني مباشرة باستخدام EEPROMHelper
    saveStringToEEPROM(SSID_ADDR, "Smart Timer", SSID_MAX_LEN); // إعادة تعيين SSID الافتراضي
    saveStringToEEPROM(PASSWORD_ADDR, "sM@rt123", PASSWORD_MAX_LEN); // إعادة تعيين كلمة المرور الافتراضية
    // تم حذف استدعاء writeOperationMethod(0);

    // إعادة تعيين إعدادات المرحل التلقائي لأوقات الصلاة
    AutoRelayPrayerConfig defaultConfig;
    defaultConfig.enabled = false;
    for (int i = 0; i < 3; i++) {
        defaultConfig.minutesAfter[i] = 0;
        defaultConfig.minutesBefore[i] = 0;
    }
    EEPROMHelper::put(AUTO_RELAY_CONFIG_ADDR, defaultConfig);

    Serial.println("تم إعادة تعيين الإعدادات. إعادة تشغيل ESP...");
    _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"تمت إعادة التعيين\"}");

    delay(1000);
    ESP.restart();
}

void MainControlClass::saveStringToEEPROM(int address, const String& data, int max_len) {
    int len = data.length();
    if (len > max_len) {
        len = max_len; 
    }
    for (int i = 0; i < len; ++i) {
        EEPROMHelper::writeByte(address + i, data.charAt(i));
    }
    EEPROMHelper::writeByte(address + len, 0);
}

String MainControlClass::readStringFromEEPROM(int address, int max_len) {
    String data = "";
    for (int i = 0; i < max_len; ++i) {
        char c = EEPROMHelper::readByte(address + i);
        if (c == 0) { 
            break;
        }
        data += c;
    }
    return data;
}

void MainControlClass::saveRelayStateToEEPROM(bool state) {
    EEPROMHelper::writeByte(RELAY_STATE_ADDR, state ? 1 : 0);
}

bool MainControlClass::getRelayStateFromEEPROM() {
    return EEPROMHelper::readByte(RELAY_STATE_ADDR) == 1;
}

void MainControlClass::setRelayPhysicalState(bool state) {
    digitalWrite(_relayPin, state ? HIGH : LOW);
    saveRelayStateToEEPROM(state);
}

// --- Private Handlers Implementations for MainControlClass ---
void MainControlClass::handleSetSSID() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON غير صالح\"}");
            return;
        }
        String ssid = doc["ssid"].as<String>();
        if (!ssid.isEmpty()) {
            saveStringToEEPROM(SSID_ADDR, ssid, SSID_MAX_LEN);
            _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"تم حفظ SSID\"}");
            Serial.print("تم تعيين SSID إلى: ");
            Serial.println(ssid);
            return;
        }
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"جسم الطلب غير صالح. المتوقع {\"ssid\":\"your_ssid\"}\"}");
}

void MainControlClass::handleGetSSID() {
    String ssid = readStringFromEEPROM(SSID_ADDR, SSID_MAX_LEN);
    String response = "{\"status\":\"success\",\"ssid\":\"" + ssid + "\"}";
    _server.send(200, "application/json", response);
}

void MainControlClass::handleSetPassword() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON غير صالح\"}");
            return;
        }
        String password = doc["password"].as<String>();
        if (!password.isEmpty()) {
            saveStringToEEPROM(PASSWORD_ADDR, password, PASSWORD_MAX_LEN);
            _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"تم حفظ كلمة المرور\"}");
            Serial.print("كلمة المرور تعيين إلى: ");
            Serial.println(password);
            return;
        }
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"جسم الطلب غير صالح. المتوقع {\"password\":\"your_password\"}\"}");
}

void MainControlClass::handleGetPassword() {
    String password = readStringFromEEPROM(PASSWORD_ADDR, PASSWORD_MAX_LEN);
    String response = "{\"status\":\"success\",\"password\":\"" + password + "\"}";
    _server.send(200, "application/json", response);
}


void MainControlClass::handleGetnetworkinfo() {
    String ssid = readStringFromEEPROM(SSID_ADDR, SSID_MAX_LEN);
    String password = readStringFromEEPROM(PASSWORD_ADDR, PASSWORD_MAX_LEN);
    String response = "{\"status\":\"success\",\"ssid\": \"" + ssid + "\", \"password\":\"" + password + "\"}";
    _server.send(200, "application/json", response);
}
void MainControlClass::handleSetnetworkinfo() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        deserializeJson(doc, _server.arg("plain"));
        String ssid = doc["ssid"];
        String password = doc["password"];
        Serial.println(ssid);
        Serial.println(password);
        saveStringToEEPROM(SSID_ADDR, ssid, SSID_MAX_LEN);
        saveStringToEEPROM(PASSWORD_ADDR, password, PASSWORD_MAX_LEN);
        _server.send(200, "application/json", "{\"status\":\"تم تحديث الشبكة\"}");
        delay(1000);
        ESP.restart();
    } else {
        _server.send(400, "application/json", "{\"error\":\"جسم الطلب مفقود\"}");
    }
}

void MainControlClass::handleSetRelayState() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<100> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON غير صالح\"}");
            return;
        }
        String stateStr = doc["state"].as<String>();
        if (stateStr.equalsIgnoreCase("on")) {
            setRelayPhysicalState(true);
            _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"تم تعيين المرحل إلى تشغيل\"}");
            Serial.println("تم تعيين المرحل إلى تشغيل");
            return;
        } else if (stateStr.equalsIgnoreCase("off")) {
            setRelayPhysicalState(false);
            _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"تم تعيين المرحل إلى إيقاف\"}");
            Serial.println("تم تعيين المرحل إلى إيقاف");
            return;
        }
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"جسم الطلب غير صالح. المتوقع {\"state\":\"on\"} أو {\"state\":\"off\"}\"}");
}

void MainControlClass::handleGetRelayState() {
    bool state = digitalRead(_relayPin) == HIGH; 
    String stateStr = state ? "on" : "off";
    String response = "{\"status\":\"success\",\"state\":\"" + stateStr + "\"}";
    _server.send(200, "application/json", response);
}

void MainControlClass::handleToggleRelay() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<100> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON غير صالح\"}");
            return;
        }
        int duration = doc["duration"].as<int>();

        if (duration > 0) {
            setRelayPhysicalState(true); 
            _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"تم تبديل المرحل إلى تشغيل لمدة " + String(duration) + " ثانية\"}");
            Serial.print("تم تبديل المرحل إلى تشغيل لمدة ");
            Serial.print(duration);
            Serial.println(" ثانية");
            delay(duration * 1000);
            setRelayPhysicalState(false); // Turn off
            Serial.println("تم إيقاف المرحل بعد التبديل");
            return;
        }
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"جسم الطلب غير صالح. المتوقع {\"duration\":1} أو {\"duration\":5}\"}");
}

void MainControlClass::handleNotFound() {
    String message = "الملف غير موجود\n\n";
    message += "URI: ";
    message += _server.uri();
    message += "\nالطريقة: ";
    message += (_server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nالوسائط: ";
    message += _server.args();
    message += "\n";
    for (uint8_t i = 0; i < _server.args(); i++) {
        message += " " + _server.argName(i) + ": " + _server.arg(i) + "\n";
    }
    _server.send(404, "text/plain", message);
}
