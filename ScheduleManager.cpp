// ScheduleManager.cpp
#include "ScheduleManager.h"

// المُنشئ (Constructor) لفئة ScheduleManagerClass
#ifdef USE_EXTERNAL_EEPROM
ScheduleManagerClass::ScheduleManagerClass(WebServer& serverRef, int relayPin)
    : MainControlClass(serverRef, relayPin), _rtcManager() { // تهيئة RTCManager
    // بدء تشغيل RTC
    if (!_rtcManager.beginRTC()) {
        Serial.println("فشل تهيئة RTC في ScheduleManager. يرجى التحقق من التوصيلات والبطارية.");
    } else {
        Serial.println("تم تهيئة RTC في ScheduleManager بنجاح.");
    }
    // تهيئة _lastCheckedTime بالوقت الحالي عند بدء التشغيل
    _lastCheckedTime = _rtcManager.now(); 
}
#else
ScheduleManagerClass::ScheduleManagerClass(WebServer& serverRef, int relayPin, EEPROMClass& eepromRef)
    : MainControlClass(serverRef, relayPin, eepromRef), _rtcManager() { // تهيئة RTCManager
    // بدء تشغيل RTC
    if (!_rtcManager.beginRTC()) {
        Serial.println("فشل تهيئة RTC في ScheduleManager. يرجى التحقق من التوصيلات والبطارية.");
    } else {
        Serial.println("تم تهيئة RTC في ScheduleManager بنجاح.");
    }
    // تهيئة _lastCheckedTime بالوقت الحالي عند بدء التشغيل
    _lastCheckedTime = _rtcManager.now(); 
}
#endif

// إعداد نقاط نهاية API المتعلقة بالجداول الزمنية
void ScheduleManagerClass::setupScheduleEndpoints() {
    _server.on("/api/schedules/add", HTTP_POST, [this]() { handleAddSchedule(); });
    _server.on("/api/schedules/get_all", HTTP_GET, [this]() { handleGetSchedules(); });
    _server.on("/api/schedules/update", HTTP_POST, [this]() { handleUpdateSchedule(); });
    _server.on("/api/schedules/delete", HTTP_POST, [this]() { handleDeleteSchedule(); });

    // نقاط نهاية RTC خاصة بـ ScheduleManager
    _server.on("/api/schedules/time/get", HTTP_GET, [this]() { handleGetTime(); });
    _server.on("/api/schedules/time/set", HTTP_POST, [this]() { handleSetTime(); });
}

// وظيفة يتم استدعاؤها في دالة loop() الرئيسية لفحص الجداول الزمنية
void ScheduleManagerClass::loopTasks() {
    // فحص الجداول الزمنية كل 5 ثوانٍ على الأقل لتجنب التحميل الزائد على المعالج
    if (millis() - _lastScheduleCheck >= 5000) { 
        checkSchedules();
        _lastScheduleCheck = millis();
    }
}

// حفظ جدول زمني في EEPROM في فهرس محدد
void ScheduleManagerClass::saveScheduleToEEPROM(int index, const Schedule& s) {
    int base = SCHEDULE_START_ADDR + index * SCHEDULE_SIZE;
    EEPROMHelper::writeByte(base, s.id);
    EEPROMHelper::writeByte(base + 1, s.hour);
    EEPROMHelper::writeByte(base + 2, s.minute);
    EEPROMHelper::writeByte(base + 3, s.turnOn);
    EEPROMHelper::writeByte(base + 4, s.repeatEveryDay);
    for (int i = 0; i < 7; i++) {
        EEPROMHelper::writeByte(base + 5 + i, s.days[i]);
    }
    EEPROMHelper::writeByte(base + 12, s.active);
}

// قراءة جدول زمني من EEPROM من فهرس محدد
Schedule ScheduleManagerClass::readScheduleFromEEPROM(int index) {
    Schedule s;
    int base = SCHEDULE_START_ADDR + index * SCHEDULE_SIZE;
    s.id = EEPROMHelper::readByte(base);
    s.hour = EEPROMHelper::readByte(base + 1);
    s.minute = EEPROMHelper::readByte(base + 2);
    s.turnOn = EEPROMHelper::readByte(base + 3);
    s.repeatEveryDay = EEPROMHelper::readByte(base + 4);
    for (int i = 0; i < 7; i++) {
        s.days[i] = EEPROMHelper::readByte(base + 5 + i);
    }
    s.active = EEPROMHelper::readByte(base + 12);
    return s;
}

// قراءة آخر معرف جدول زمني تم استخدامه
uint8_t ScheduleManagerClass::readLastScheduleId() {
    return EEPROMHelper::readByte(LAST_SCHEDULE_ID_ADDR);
}

// كتابة آخر معرف جدول زمني تم استخدامه
void ScheduleManagerClass::writeLastScheduleId(uint8_t id) {
    EEPROMHelper::writeByte(LAST_SCHEDULE_ID_ADDR, id);
}

// معالج لإضافة جدول زمني جديد
void ScheduleManagerClass::handleAddSchedule() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<300> doc; // Adjust size as needed
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON غير صالح\"}");
            return;
        }

        uint8_t lastId = readLastScheduleId();
        if(lastId >= MAX_SCHEDULES){
            _server.send(400, "application/json", "{\"status\":\"schedule_id_full\",\"message\":\"الحد الأقصى للجداول الزمنية (10) قد اكتمل\"}");
            return;
        }
        
        // استخدام معرف جديد (lastId + 1) كمعرف للجدول الزمني
        uint8_t newId = lastId + 1;

        Schedule s;
        s.id = newId; // تعيين المعرف الجديد
        s.hour = doc["hour"];
        s.minute = doc["minute"];
        s.turnOn = doc["turnOn"];
        s.repeatEveryDay = doc["repeatEveryDay"];
        JsonArray daysArray = doc["days"];
        for (int i = 0; i < 7; i++) {
            s.days[i] = daysArray[i];
        }
        s.active = true; // تفعيل الجدول الزمني عند الإضافة

        saveScheduleToEEPROM(newId, s); // حفظ الجدول الزمني في EEPROM
        writeLastScheduleId(newId); // تحديث آخر معرف جدول زمني

        _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"تمت إضافة الجدول الزمني بنجاح\",\"id\":" + String(newId) + "}");
    } else {
        _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"جسم الطلب مفقود\"}");
    }
}

// معالج للحصول على جميع الجداول الزمنية
void ScheduleManagerClass::handleGetSchedules() {
    StaticJsonDocument<2048> doc; // حجم كبير بما يكفي لعدة جداول
    JsonArray arr = doc.to<JsonArray>();
    uint8_t count = readLastScheduleId(); // الحصول على عدد الجداول الزمنية النشطة

    for (int i = 1; i <= count; i++) { // التكرار من 1 إلى العدد الحالي للجداول
        Schedule s = readScheduleFromEEPROM(i);
        // إضافة الجداول النشطة فقط
        if (s.active) {
            JsonObject o = arr.createNestedObject();
            o["id"] = s.id;
            o["hour"] = s.hour;
            o["minute"] = s.minute;
            o["turnOn"] = s.turnOn;
            o["repeatEveryDay"] = s.repeatEveryDay;
            JsonArray d = o.createNestedArray("days");
            for (int j = 0; j < 7; j++) d.add(s.days[j]);
            o["active"] = s.active;
        }
    }
    String output;
    serializeJsonPretty(doc, output); // تنسيق JSON بشكل جميل
    _server.send(200, "application/json", output);
}

// معالج لحذف جدول زمني
void ScheduleManagerClass::handleDeleteSchedule() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<100> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON غير صالح\"}");
            return;
        }
        int idToDelete = doc["id"];
        uint8_t count = readLastScheduleId();
        
        bool found = false;
        for (int i = 1; i <= count; i++) {
            Schedule s = readScheduleFromEEPROM(i);
            if (s.id == idToDelete && s.active) { // البحث عن الجدول الزمني النشط بالمعرف
                found = true;
                // إزاحة الجداول الزمنية اللاحقة لملء الفجوة
                for (int j = i; j < count; j++) {
                    Schedule next_s = readScheduleFromEEPROM(j + 1);
                    next_s.id = j; // تحديث المعرف للحفاظ على الترتيب التسلسلي
                    saveScheduleToEEPROM(j, next_s);
                }
                // مسح آخر خانة في EEPROM (اختياري، ولكن يفضل لتنظيف البيانات)
                for (int k = 0; k < SCHEDULE_SIZE; ++k) {
                    EEPROMHelper::writeByte(SCHEDULE_START_ADDR + (count * SCHEDULE_SIZE) + k, 0xFF);
                }
                writeLastScheduleId(count - 1); // تحديث آخر معرف جدول زمني
                _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"تم حذف الجدول الزمني بنجاح\"}");
                Serial.print("تم حذف الجدول الزمني بالمعرف: "); Serial.println(idToDelete);
                return;
            }
        }
        if (!found) {
            _server.send(404, "application/json", "{\"status\":\"error\",\"message\":\"لم يتم العثور على الجدول الزمني\"}");
        }
    } else {
        _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"جسم الطلب مفقود\"}");
    }
}

// معالج لتحديث جدول زمني موجود
void ScheduleManagerClass::handleUpdateSchedule() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<300> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON غير صالح\"}");
            return;
        }
        int idToUpdate = doc["id"];
        uint8_t count = readLastScheduleId();
        for (int i = 1; i <= count; i++) { 
            Schedule s = readScheduleFromEEPROM(i);
            if (s.id == idToUpdate && s.active) { // البحث عن الجدول الزمني النشط بالمعرف
                s.hour = doc["hour"];
                s.minute = doc["minute"];
                s.turnOn = doc["turnOn"];
                s.repeatEveryDay = doc["repeatEveryDay"];
                JsonArray daysArray = doc["days"];
                for (int j = 0; j < 7; j++) s.days[j] = daysArray[j];
                saveScheduleToEEPROM(i, s); // حفظ التغييرات في نفس الفهرس
                _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"تم تحديث الجدول الزمني بنجاح\"}");
                Serial.print("تم تحديث الجدول الزمني بالمعرف: "); Serial.println(idToUpdate);
                return;
            }
        }
        _server.send(404, "application/json", "{\"status\":\"error\",\"message\":\"لم يتم العثور على الجدول الزمني\"}");
    } else {
        _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"جسم الطلب مفقود\"}");
    }
}

// وظيفة داخلية لفحص وتفعيل الجداول الزمنية
void ScheduleManagerClass::checkSchedules() {
    DateTime currentTime = _rtcManager.now(); // الحصول على الوقت والتاريخ الحاليين من RTCManager

    // التحقق فقط إذا تغيرت الدقيقة لتجنب التكرار الزائد
    if (currentTime.minute() == _lastCheckedTime.minute() &&
        currentTime.hour() == _lastCheckedTime.hour() &&
        currentTime.day() == _lastCheckedTime.day() &&
        currentTime.month() == _lastCheckedTime.month() &&
        currentTime.year() == _lastCheckedTime.year()) {
        return; // لم تتغير الدقيقة، لا تفعل شيئاً
    }

    uint8_t count = readLastScheduleId(); 

    for (int i = 1; i <= count; i++) { 
        Schedule s = readScheduleFromEEPROM(i);

        // التحقق مما إذا كان الجدول الزمني نشطاً والوقت الحالي يطابق وقت الجدول
        if (s.active && s.hour == currentTime.hour() && s.minute == currentTime.minute()) {
            bool today = s.repeatEveryDay || s.days[currentTime.dayOfTheWeek()];
            if (today) {
                setRelayPhysicalState(s.turnOn); // تعيين حالة المرحل
                Serial.print("تم تفعيل الجدول الزمني ID: ");
                Serial.print(s.id);
                Serial.print("، تعيين المرحل إلى: ");
                Serial.println(s.turnOn ? "تشغيل" : "إيقاف");
            }
        }
    }
    _lastCheckedTime = currentTime; // تحديث آخر وقت تم فحصه
}

// معالج للحصول على الوقت الحالي عبر API (خاص بـ ScheduleManager)
void ScheduleManagerClass::handleGetTime() {
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

// معالج لتعيين الوقت عبر API (خاص بـ ScheduleManager)
void ScheduleManagerClass::handleSetTime() {
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
