// PrayerTimesManager.cpp
#include "PrayerTimesManager.h"

// المُنشئ (Constructor) لفئة PrayerTimesManagementClass
#ifdef USE_EXTERNAL_EEPROM
PrayerTimesManagementClass::PrayerTimesManagementClass(WebServer& serverRef, int relayPin)
    : MainControlClass(serverRef, relayPin), _rtcManager() { // تهيئة RTCManager
    // بدء تشغيل RTC
    if (!_rtcManager.beginRTC()) {
        Serial.println("فشل تهيئة RTC في PrayerTimesManager. يرجى التحقق من التوصيلات والبطارية.");
    } else {
        Serial.println("تم تهيئة RTC في PrayerTimesManager بنجاح.");
    }
    // قراءة الإعدادات من EEPROM أولاً
    readPrayerConfig(_latitude, _longitude, _timezone);
    _autoRelayConfig = readAutoRelayConfig();

    // تهيئة كائن PrayerTimes بالقيم المقروءة
    _prayerTimes.setCoordinates(_latitude, _longitude, _timezone);
    _prayerTimes.setCalcMethod(MWL); // طريقة حساب افتراضية (يمكن تغييرها عبر API)
    _prayerTimes.setHanafi(false); // إعداد افتراضي (يمكن تغييرها عبر API)

    Serial.print("إعدادات الصلاة الأولية: خط العرض="); Serial.print(_latitude, 4);
    Serial.print(", خط الطول="); Serial.print(_longitude, 4);
    Serial.print(", المنطقة الزمنية="); Serial.println(_timezone);
    Serial.print("المرحل التلقائي مفعل: "); Serial.println(_autoRelayConfig.enabled ? "صحيح" : "خطأ");
}
#else
PrayerTimesManagementClass::PrayerTimesManagementClass(WebServer& serverRef, int relayPin, EEPROMClass& eepromRef)
    : MainControlClass(serverRef, relayPin, eepromRef), _rtcManager() { // تهيئة RTCManager
    // بدء تشغيل RTC
    if (!_rtcManager.beginRTC()) {
        Serial.println("فشل تهيئة RTC في PrayerTimesManager. يرجى التحقق من التوصيلات والبطارية.");
    } else {
        Serial.println("تم تهيئة RTC في PrayerTimesManager بنجاح.");
    }
    // قراءة الإعدادات من EEPROM أولاً
    readPrayerConfig(_latitude, _longitude, _timezone);
    _autoRelayConfig = readAutoRelayConfig();

    // تهيئة كائن PrayerTimes بالقيم المقروءة
    _prayerTimes.setCoordinates(_latitude, _longitude, _timezone);
    _prayerTimes.setCalcMethod(MWL); // طريقة حساب افتراضية (يمكن تغييرها عبر API)
    _prayerTimes.setHanafi(false); // إعداد افتراضي (يمكن تغييرها عبر API)

    Serial.print("إعدادات الصلاة الأولية: خط العرض="); Serial.print(_latitude, 4);
    Serial.print(", خط الطول="); Serial.print(_longitude, 4);
    Serial.print(", المنطقة الزمنية="); Serial.println(_timezone);
    Serial.print("المرحل التلقائي مفعل: "); Serial.println(_autoRelayConfig.enabled ? "صحيح" : "خطأ");
}
#endif

// إعداد نقاط نهاية API المتعلقة بأوقات الصلاة
void PrayerTimesManagementClass::setupPrayerEndpoints() {
    _server.on("/api/prayer/set_config", HTTP_POST, [this]() { handleSetPrayerConfig(); });
    _server.on("/api/prayer/get_config", HTTP_GET, [this]() { handleGetPrayerConfig(); });
    _server.on("/api/prayer/get_times", HTTP_GET, [this]() { handleGetPrayerTimes(); });
    _server.on("/api/prayer/set_auto_relay_config", HTTP_POST, [this]() { handleSetAutoRelayConfig(); });
    _server.on("/api/prayer/get_auto_relay_config", HTTP_GET, [this]() { handleGetAutoRelayConfig(); });

    // نقاط نهاية RTC خاصة بـ PrayerTimesManager
    _server.on("/api/prayer/time/get", HTTP_GET, [this]() { handleGetTime(); });
    _server.on("/api/prayer/time/set", HTTP_POST, [this]() { handleSetTime(); });
}

// وظيفة يتم استدعاؤها في دالة loop() الرئيسية لفحص المرحل التلقائي
void PrayerTimesManagementClass::loopTasks() {
    // فحص أوقات الصلاة كل 30 ثانية على الأقل لتجنب التحميل الزائد
    if (millis() - _lastPrayerCheck >= 30000) { 
        if (_autoRelayConfig.enabled) {
            handleAutoRelayByPrayerTimes();
        }
        _lastPrayerCheck = millis();
    }
}

// حفظ إعدادات أوقات الصلاة في EEPROM
void PrayerTimesManagementClass::savePrayerConfig(double lat, double lon, int tz) {
    EEPROMHelper::put(PRAYER_CONFIG_ADDR, lat);
    EEPROMHelper::put(PRAYER_CONFIG_ADDR + sizeof(double), lon);
    EEPROMHelper::put(PRAYER_CONFIG_ADDR + 2 * sizeof(double), tz);
}

// قراءة إعدادات أوقات الصلاة من EEPROM
void PrayerTimesManagementClass::readPrayerConfig(double& lat, double& lon, int& tz) {
    EEPROMHelper::get(PRAYER_CONFIG_ADDR, lat);
    EEPROMHelper::get(PRAYER_CONFIG_ADDR + sizeof(double), lon);
    EEPROMHelper::get(PRAYER_CONFIG_ADDR + 2 * sizeof(double), tz);

    // التحقق من القيم غير المهيأة (عادة 0.0 للأعداد العشرية، 0 للعدد الصحيح)
    // إذا كانت جميعها افتراضية (على الأرجح غير مكتوبة)، قم بتعيينها إلى افتراضيات القاهرة وحفظها.
    if (abs(lat) < 0.0001 && abs(lon) < 0.0001 && tz == 0) { 
        lat = 30.0444; // خط عرض القاهرة
        lon = 31.2357; // خط طول القاهرة
        tz = 2;        // المنطقة الزمنية للقاهرة (UTC+2)
        savePrayerConfig(lat, lon, tz); // حفظ هذه الافتراضيات
        Serial.println("تم تعيين إعدادات الصلاة الافتراضية (القاهرة).");
    }
}

// حفظ إعدادات المرحل التلقائي لأوقات الصلاة في EEPROM
void PrayerTimesManagementClass::saveAutoRelayConfig(const AutoRelayPrayerConfig& config) {
    EEPROMHelper::put(AUTO_RELAY_CONFIG_ADDR, config);
    _autoRelayConfig = config; // تحديث النسخة المحلية
    Serial.println("تم حفظ إعدادات المرحل التلقائي لأوقات الصلاة.");
}

// قراءة إعدادات المرحل التلقائي لأوقات الصلاة من EEPROM
AutoRelayPrayerConfig PrayerTimesManagementClass::readAutoRelayConfig() {
    AutoRelayPrayerConfig config;
    EEPROMHelper::get(AUTO_RELAY_CONFIG_ADDR, config);
   
    // تحقق بسيط للقيم غير الصالحة (مثل 0xFF من EEPROM غير المكتوبة)
    bool isValid = true;
    for (int i = 0; i < 3; ++i) {
        if (config.minutesBefore[i] < 0 || config.minutesAfter[i] < 0 ||
            config.minutesBefore[i] > 1440 || config.minutesAfter[i] > 1440) { 
            isValid = false;
            break;
        }
    }

    if (!isValid) {
        // إعادة تعيين إلى القيم الافتراضية إذا كانت غير صالحة
        config.enabled = false;
        for (int i = 0; i < 3; i++) {
            config.minutesBefore[i] = 0;
            config.minutesAfter[i] = 0;
        }
        Serial.println("تم إعادة تعيين إعدادات المرحل التلقائي إلى الافتراضيات بسبب قيم غير صالحة.");
        saveAutoRelayConfig(config); // حفظ الافتراضيات
    }
    return config;
}

// معالج لتعيين إعدادات أوقات الصلاة
void PrayerTimesManagementClass::handleSetPrayerConfig() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON غير صالح\"}");
            return;
        }
        _latitude = doc["latitude"].as<double>();
        _longitude = doc["longitude"].as<double>();
        _timezone = doc["timezone"].as<int>();
        savePrayerConfig(_latitude, _longitude, _timezone);
        // تحديث كائن PrayerTimes بالإعدادات الجديدة
        _prayerTimes.setCoordinates(_latitude, _longitude, _timezone); 
        _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"تم حفظ إعدادات الصلاة\"}");
        Serial.print("تم تعيين إعدادات الصلاة إلى: خط العرض="); Serial.print(_latitude, 4);
        Serial.print(", خط الطول="); Serial.print(_longitude, 4);
        Serial.print(", المنطقة الزمنية="); Serial.println(_timezone);
    } else {
        _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"جسم الطلب مفقود\"}");
    }
}

// معالج للحصول على إعدادات أوقات الصلاة
void PrayerTimesManagementClass::handleGetPrayerConfig() {
    readPrayerConfig(_latitude, _longitude, _timezone); // التأكد من تحميل القيم الحالية
    String json = "{ \"latitude\": " + String(_latitude, 6) +
                  ", \"longitude\": " + String(_longitude, 6) +
                  ", \"timezone\": " + String(_timezone) + " }";
    _server.send(200, "application/json", json);
}

// معالج للحصول على أوقات الصلاة لليوم الحالي
void PrayerTimesManagementClass::handleGetPrayerTimes() {
    DateTime nowDt = _rtcManager.now(); // الحصول على التاريخ الحالي من كائن RTCManager
    
    // التأكد من أن كائن PrayerTimes مهيأ بالإعدادات الصحيحة
    _prayerTimes.setCoordinates(_latitude, _longitude, _timezone);
    _prayerTimes.setCalcMethod(Egyptian); // تعيين طريقة الحساب
    _prayerTimes.setAdjustments(0, 0, 0, 0, 0, 0); // تعديلات اختيارية

    int fajrH, fajrM, sunH, sunM, dhuhrH, dhuhrM;
    int asrH, asrM, maghribH, maghribM, ishaH, ishaM;

    _prayerTimes.calculate(
        nowDt.day(), nowDt.month(), nowDt.dayOfTheWeek(), nowDt.year(),
        fajrH, fajrM,
        sunH, sunM,
        dhuhrH, dhuhrM,
        asrH, asrM,
        maghribH, maghribM,
        ishaH, ishaM
    );

    StaticJsonDocument<512> doc;
    char buf[6]; // لتنسيق الوقت HH:MM

    sprintf(buf, "%02d:%02d", fajrH, fajrM); doc["Fajr"] = buf;
    sprintf(buf, "%02d:%02d", sunH, sunM); doc["Sunrise"] = buf;
    sprintf(buf, "%02d:%02d", dhuhrH, dhuhrM); doc["Dhuhr"] = buf;
    sprintf(buf, "%02d:%02d", asrH, asrM); doc["Asr"] = buf;
    sprintf(buf, "%02d:%02d", maghribH, maghribM); doc["Maghrib"] = buf;
    sprintf(buf, "%02d:%02d", ishaH, ishaM); doc["Isha"] = buf;

    String output;
    serializeJsonPretty(doc, output);
    _server.send(200, "application/json", output);
}

// معالج لتعيين إعدادات المرحل التلقائي لأوقات الصلاة
void PrayerTimesManagementClass::handleSetAutoRelayConfig() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON غير صالح\"}");
            return;
        }

        AutoRelayPrayerConfig newConfig;
        newConfig.enabled = doc["enabled"].as<bool>();
        JsonArray minutesBeforeArray = doc["minutesBefore"];
        JsonArray minutesAfterArray = doc["minutesAfter"];

        if (minutesBeforeArray.size() == 3 && minutesAfterArray.size() == 3) {
            for (int i = 0; i < 3; i++) {
                newConfig.minutesBefore[i] = minutesBeforeArray[i].as<int>();
                newConfig.minutesAfter[i] = minutesAfterArray[i].as<int>();
            }
            saveAutoRelayConfig(newConfig); // حفظ الإعدادات الجديدة
            _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"تم حفظ إعدادات المرحل التلقائي\"}");
            Serial.println("تم تحديث إعدادات المرحل التلقائي.");
            return;
        }
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"جسم الطلب غير صالح. المتوقع {\"enabled\":true/false, \"minutesBefore\":[b0,b1,b2], \"minutesAfter\":[a0,a1,a2]}\"}");
}

// معالج للحصول على إعدادات المرحل التلقائي لأوقات الصلاة
void PrayerTimesManagementClass::handleGetAutoRelayConfig() {
    _autoRelayConfig = readAutoRelayConfig(); // التأكد من تحميل القيم الحالية
    StaticJsonDocument<256> doc;
    doc["enabled"] = _autoRelayConfig.enabled;
    JsonArray beforeArr = doc.createNestedArray("minutesBefore");
    JsonArray afterArr = doc.createNestedArray("minutesAfter");
    for (int i = 0; i < 3; i++) {
        beforeArr.add(_autoRelayConfig.minutesBefore[i]);
        afterArr.add(_autoRelayConfig.minutesAfter[i]);
    }
    String output;
    serializeJsonPretty(doc, output);
    _server.send(200, "application/json", output);
}

// وظيفة داخلية لتشغيل المرحل تلقائياً بناءً على أوقات الصلاة
void PrayerTimesManagementClass::handleAutoRelayByPrayerTimes() {
    if (!_autoRelayConfig.enabled) {
        return; // لا تفعل شيئاً إذا لم يكن المرحل التلقائي مفعلاً
    }

    DateTime nowDt = _rtcManager.now(); // الحصول على الوقت والتاريخ الحاليين من RTCManager
    
    // التأكد من أن كائن PrayerTimes مهيأ بالإعدادات الصحيحة
    _prayerTimes.setCoordinates(_latitude, _longitude, _timezone);
    _prayerTimes.setCalcMethod(Egyptian); 
    _prayerTimes.setAdjustments(0, 0, 0, 0, 0, 0);

    int fajrH, fajrM, sunH, sunM, dhuhrH, dhuhrM;
    int asrH, asrM, maghribH, maghribM, ishaH, ishaM;

    _prayerTimes.calculate(
        nowDt.day(), nowDt.month(), nowDt.dayOfTheWeek(), nowDt.year(),
        fajrH, fajrM,
        sunH, sunM,
        dhuhrH, dhuhrM,
        asrH, asrM,
        maghribH, maghribM,
        ishaH, ishaM
    );

    // تعريف أوقات الصلاة ذات الصلة للمرحل التلقائي (الفجر، المغرب، العشاء)
    struct PrayerTimeInfo {
        int hour;
        int minute;
        int beforeMinutes;
        int afterMinutes;
    };

    PrayerTimeInfo prayers[3] = {
        { fajrH, fajrM, _autoRelayConfig.minutesBefore[0], _autoRelayConfig.minutesAfter[0] },   // الفجر
        { maghribH, maghribM, _autoRelayConfig.minutesBefore[1], _autoRelayConfig.minutesAfter[1] }, // المغرب
        { ishaH, ishaM, _autoRelayConfig.minutesBefore[2], _autoRelayConfig.minutesAfter[2] }     // العشاء
    };

    bool shouldBeOn = false;
    int nowMinutes = nowDt.hour() * 60 + nowDt.minute(); // الوقت الحالي بالدقائق من منتصف الليل

    for (int i = 0; i < 3; i++) {
        int totalPrayerMinutes = prayers[i].hour * 60 + prayers[i].minute;
        int start = totalPrayerMinutes - prayers[i].beforeMinutes;
        int end = totalPrayerMinutes + prayers[i].afterMinutes;

        // معالجة الالتفاف حول منتصف الليل
        if (start < 0) start += 24 * 60;
        if (end < 0) end += 24 * 60; 

        if (start <= end) { 
            // الحالة الطبيعية (مثال: 10:00 إلى 11:00)
            if (nowMinutes >= start && nowMinutes <= end) {
                shouldBeOn = true;
                break;
            }
        } else { 
            // تلتف حول منتصف الليل (مثال: 23:00 إلى 01:00)
            if (nowMinutes >= start || nowMinutes <= end) {
                shouldBeOn = true;
                break;
            }
        }
    }

    bool currentRelayState = digitalRead(_relayPin) == HIGH;
    if (shouldBeOn != currentRelayState) {
        setRelayPhysicalState(shouldBeOn); // تعيين حالة المرحل
        Serial.print("المرحل التلقائي: تعيين المرحل إلى ");
        Serial.println(shouldBeOn ? "تشغيل" : "إيقاف");
    }
}

// معالج للحصول على الوقت الحالي عبر API (خاص بـ PrayerTimesManager)
void PrayerTimesManagementClass::handleGetTime() {
    DateTime currentTime = _rtcManager.now(); // الحصول على الوقت الحالي
    String response = "{ \"year\": " + String(currentTime.year()) +
                      ", \"month\": " + String(currentTime.month()) +
                      ", \"day\": " + String(currentTime.day()) +
                      ", \"hour\": " + String(currentTime.hour()) +
                      ", \"minute\": " + String(currentTime.minute()) +
                      ", \"second\": " + String(currentTime.second()) + " }";
    Serial.println(response);
    _server.send(200, "application/json", response);
}

// معالج لتعيين الوقت عبر API (خاص بـ PrayerTimesManager)
void PrayerTimesManagementClass::handleSetTime() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        deserializeJson(doc, _server.arg("plain"));
        // قراءة قيم الوقت والتاريخ من JSON
        int year = doc["year"];
        int month = doc["month"];
        int day = doc["day"];
        int hour = doc["hour"];
        int minute = doc["minute"];
        int second = doc["second"];
        // ضبط RTC بالقيم الجديدة
        _rtcManager.adjustRTC(DateTime(year, month, day, hour, minute, second));
        _server.send(200, "application/json", "{\"status\":\"تم تحديث الوقت\"}");
    } else {
        _server.send(400, "application/json", "{\"error\":\"جسم الطلب مفقود\"}");
    }
}
