// UserManager.cpp
#include "UserManager.h"

// المُنشئ (Constructor) لفئة UserManager
#ifdef USE_EXTERNAL_EEPROM
UserManager::UserManager(WebServer& serverRef, int relayPin)
    : MainControlClass(serverRef, relayPin) {
    // قراءة الحد الأقصى لعدد المستخدمين القابل للتكوين عند بدء التشغيل
    _maxConfigurableUsers = EEPROMHelper::readInt(MAX_NUM_OF_USERS_ADD);
    // إذا كانت القيمة غير صالحة (مثل 0xFFFFFFFF من EEPROM غير المهيأة)، قم بتعيين قيمة افتراضية
    if (_maxConfigurableUsers <= 0 || _maxConfigurableUsers > MAX_USER_TAGS) {
        _maxConfigurableUsers = MAX_USER_TAGS; // تعيين الحد الأقصى الفعلي كقيمة افتراضية
        EEPROMHelper::writeInt(MAX_NUM_OF_USERS_ADD, _maxConfigurableUsers);
    }

#ifdef ENABLE_USER_STATISTICS
    _statisticsEnabled = readStatisticsEnabledState(); // قراءة حالة الإحصائيات عند بدء التشغيل
#endif
}
#else
UserManager::UserManager(WebServer& serverRef, int relayPin, EEPROMClass& eepromRef)
    : MainControlClass(serverRef, relayPin, eepromRef) {
    // قراءة الحد الأقصى لعدد المستخدمين القابل للتكوين عند بدء التشغيل
    _maxConfigurableUsers = EEPROMHelper::readInt(MAX_NUM_OF_USERS_ADD);
    // إذا كانت القيمة غير صالحة (مثل 0xFFFFFFFF من EEPROM غير المهيأة)، قم بتعيين قيمة افتراضية
    if (_maxConfigurableUsers <= 0 || _maxConfigurableUsers > MAX_USER_TAGS) {
        _maxConfigurableUsers = MAX_USER_TAGS; // تعيين الحد الأقصى الفعلي كقيمة افتراضية
        EEPROMHelper::writeInt(MAX_NUM_OF_USERS_ADD, _maxConfigurableUsers);
    }

#ifdef ENABLE_USER_STATISTICS
    _statisticsEnabled = readStatisticsEnabledState(); // قراءة حالة الإحصائيات عند بدء التشغيل
#endif
}
#endif

// --- وظائف مساعدة لتوليد كلمات المرور ---

// توليد كلمة مرور عشوائية
String UserManager::generatePassword() {
    String password = "";

    // التأكد من تضمين حرف واحد على الأقل من كل نوع
    password += getRandomUpperCase();
    password += getRandomLowerCase();
    password += getRandomSymbol();

    // ملء الطول المتبقي بأرقام عشوائية
    for (int i = 0; i < 6; i++) {
        password += getRandomNumber();
    }
    // لا حاجة لخلط السلسلة إذا لم يتم استخدامها في الأصل
    return password;
}

// الحصول على حرف كبير عشوائي
char UserManager::getRandomUpperCase() {
    const char upperCaseLetters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    return upperCaseLetters[random(0, sizeof(upperCaseLetters) - 1)];
}

// الحصول على حرف صغير عشوائي
char UserManager::getRandomLowerCase() {
    const char lowerCaseLetters[] = "abcdefghijklmnopqrstuvwxyz";
    return lowerCaseLetters[random(0, sizeof(lowerCaseLetters) - 1)];
}

// الحصول على رقم عشوائي
char UserManager::getRandomNumber() {
    const char numbers[] = "0123456789";
    return numbers[random(0, 9)];
}

// الحصول على رمز عشوائي
char UserManager::getRandomSymbol() {
    const char symbols[] = "!@$&*";
    return symbols[random(0, 4)];
}

// الحصول على حرف عشوائي من الأنواع المتاحة
char UserManager::getRandomCharacter() {
    int charType = random(0, 4);  // 0 = حرف كبير, 1 = حرف صغير, 2 = أرقام, 3 = رموز

    switch (charType) {
        case 0: return getRandomUpperCase();
        case 1: return getRandomLowerCase();
        case 2: return getRandomNumber();
        case 3: return getRandomSymbol();
    }
    return '!';  // حرف احتياطي
}

// خلط سلسلة نصية (عادة ما يتم ذلك على مصفوفات char)
String UserManager::shuffleString(String str) {
    for (int i = 0; i < str.length(); i++) {
        int randomIndex = random(0, str.length());
        char temp = str[i];
        str[i] = str[randomIndex];
        str[randomIndex] = temp;
    }
    return str;
}

// --- معالجات API لإدارة المستخدمين ---

// معالج لتوليد SSID وكلمة مرور عشوائية
void UserManager::handleGenerateSSIDAndPASS() {
#ifdef ESP32
    uint64_t mac = ESP.getEfuseMac(); // الحصول على عنوان MAC 64 بت لـ ESP32
#elif ESP8266
    uint32_t mac = ESP.getChipId();   // الحصول على معرف الشريحة 32 بت لـ ESP8266
#endif

    char macStr[13]; // مخزن مؤقت لسلسلة hex المكونة من 12 حرفاً + حرف النهاية
    sprintf(macStr, "%012llX", (long long unsigned int)mac); // تنسيق إلى 12 حرفاً hex كبيراً، مع حشو بالأصفار

    String ssid = String(macStr); // تحويل مصفوفة char إلى String لاستخدامها كـ SSID
    String password = generatePassword(); // توليد كلمة مرور عشوائية

    String jsonResponse = "{\"SSID\":\"" + ssid + "\",\"PASS\":\"" + password + "\"}";
    Serial.println(jsonResponse);
    _server.send(200, "application/json", jsonResponse);
}

// إعداد نقاط نهاية API لإدارة المستخدمين والإحصائيات
void UserManager::setupUserEndpoints() {
    _server.on("/api/users/add_tag", HTTP_POST, [this]() { handleAddUserTag(); });
    _server.on("/api/users/delete_tag", HTTP_POST, [this]() { handleDeleteUserTag(); });
    _server.on("/api/users/delete_all_tags", HTTP_POST, [this]() { handleDeleteAllUserTags(); });
    _server.on("/api/users/check_tag", HTTP_POST, [this]() { handleCheckUserTag(); });
    _server.on("/api/users/get_count", HTTP_GET, [this]() { handleGetUserTagCount(); });
    _server.on("/api/users/use_tag", HTTP_POST, [this]() { handleUseUserTag(); });
    _server.on("/api/users/remove_card", HTTP_POST, [this]() { handleRemoveCard(); });
    _server.on("/api/users/add_card", HTTP_POST, [this]() { handleAddCard(); });
    _server.on("/api/users/generate_ssid_pass", HTTP_GET, [this]() { handleGenerateSSIDAndPASS(); });
    _server.on("/api/users/get_tags", HTTP_GET, [this]() { handleGetTags(); });
    _server.on("/api/users/set_users_max_number", HTTP_POST, [this]() { handleSetUsersMaxNumber(); });
    _server.on("/api/users/get_users_max_number", HTTP_GET, [this]() { handleGetUsersMaxNumber(); }); // نقطة نهاية جديدة
    
#ifdef ENABLE_USER_STATISTICS
    _server.on("/api/users/get_statistics", HTTP_GET, [this]() { handleGetStatistics(); });
    _server.on("/api/users/set_statistics_enabled", HTTP_POST, [this]() { handleSetStatisticsEnabled(); }); // نقطة نهاية جديدة
#endif
}

// معالج لحذف جميع علامات المستخدمين
void UserManager::handleDeleteAllUserTags() {
    EEPROMHelper::writeInt(USER_TAG_COUNT_ADDR, 0); // إعادة تعيين عدد المستخدمين إلى 0
#ifdef ENABLE_USER_STATISTICS
    // مسح جميع الإحصائيات إذا كانت الميزة مفعلة
    for (int i = 0; i < MAX_USER_TAGS; ++i) {
        ClearStatisticsAtIndex(i);
    }
#endif
    _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"تم حذف جميع علامات المستخدمين\"}");
}

// حفظ عدد علامات المستخدمين في EEPROM
void UserManager::saveUserTagCountToEEPROM(int count) {
    EEPROMHelper::writeInt(USER_TAG_COUNT_ADDR, count);
}

// الحصول على عدد علامات المستخدمين من EEPROM
int UserManager::getUserTagCountFromEEPROM() {
    return EEPROMHelper::readInt(USER_TAG_COUNT_ADDR);
}

// البحث عن فهرس علامة مستخدم معينة
int UserManager::findUserTagIndex(const String& tag) {
    int userCount = getUserTagCountFromEEPROM();
    for (int i = 0; i < userCount; ++i) { 
        int currentTagAddr = USER_TAGS_START_ADDR + (i * USER_TAG_LEN);
        String storedTag = readStringFromEEPROM(currentTagAddr, USER_TAG_LEN);
        storedTag.trim(); // إزالة المسافات البيضاء
        char firstChar = EEPROMHelper::readByte(currentTagAddr); // التحقق من أول بايت

        if (storedTag == tag && !storedTag.isEmpty() && firstChar != 0xFF) {
            return i; // إرجاع الفهرس إذا تم العثور على العلامة
        }
    }
    return -1; // إرجاع -1 إذا لم يتم العثور على العلامة
}

// حفظ علامة مستخدم جديدة في EEPROM
bool UserManager::storeTag(String tag) {
    // حشو بالأصفار البادئة إذا كانت العلامة أقصر من الطول المحدد
    String paddedTag = "";
    for(int i = tag.length() ; i < USER_TAG_LEN ; i++){
        paddedTag += "0";
    }
    paddedTag += tag;
    Serial.print("علامة المستخدم المحشوة: ");
    Serial.println(paddedTag);

    if (findUserTagIndex(paddedTag) != -1){
        Serial.println("العلامة موجودة بالفعل");
        return false;
    }

    int userCount = getUserTagCountFromEEPROM();
    // التحقق من الحد الأقصى القابل للتكوين بدلاً من MAX_USER_TAGS الثابت
    if (userCount < _maxConfigurableUsers) { 
        saveStringToEEPROM(USER_TAGS_START_ADDR + (userCount * USER_TAG_LEN), paddedTag, USER_TAG_LEN);
#ifdef ENABLE_USER_STATISTICS
        ClearStatisticsAtIndex(userCount); // مسح الإحصائيات للمستخدم الجديد
#endif
        userCount++;
        saveUserTagCountToEEPROM(userCount);
        return true;
    }
    Serial.println("تم الوصول للحد الأقصى لعدد المستخدمين المسموح به.");
    return false;
}

// معالج لإضافة بطاقة (علامة مستخدم)
void UserManager::handleAddCard(){
    if (_server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON غير صالح\"}");
            return;
        }
        String card = doc["card"].as<String>();
        if (card.length() > USER_TAG_LEN) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"يجب ألا يتجاوز طول العلامة 11 رقماً\"}");
            return;
        } else {
            String temp = "";
            for(int i = card.length() ; i < USER_TAG_LEN ; i++){
                temp += "0";
            }
            card = temp + card;
            Serial.print("بطاقة الإضافة: ");
            Serial.println(card);
        }
        saveStringToEEPROM(ADD_CARD_ADDR, card, USER_TAG_LEN);
        _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"تمت إضافة بطاقة الإضافة\"}");
        Serial.println("تمت إضافة بطاقة الإضافة بنجاح");
        return;
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"جسم الطلب غير صالح. المتوقع {\"card\":\"11_digits\"}\"}");
}

// معالج لإزالة بطاقة (علامة مستخدم)
void UserManager::handleRemoveCard(){
    if (_server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON غير صالح\"}");
            return;
        }
        String card = doc["card"].as<String>();
        if (card.length() > USER_TAG_LEN) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"يجب ألا يتجاوز طول العلامة 11 رقماً\"}");
            return;
        } else {
            String temp = "";
            for(int i = card.length() ; i < USER_TAG_LEN ; i++){
                temp += "0";
            }
            card = temp + card;
            Serial.print("بطاقة الإزالة: ");
            Serial.println(card);
        }
        saveStringToEEPROM(REMOVE_CARD_ADDR, card, USER_TAG_LEN);
        _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"تمت إضافة بطاقة الإزالة\"}");
        Serial.println("تمت إضافة بطاقة الإزالة بنجاح");
        return;
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"جسم الطلب غير صالح. المتوقع {\"card\":\"11_digits\"}\"}");
}

// معالج لإضافة علامة مستخدم جديدة
void UserManager::handleAddUserTag() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON غير صالح\"}");
            return;
        }
        String tag = doc["tag"].as<String>();
        if (tag.length() > USER_TAG_LEN) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"يجب ألا يتجاوز طول العلامة 11 رقماً\"}");
            return;
        } 
        
        // حشو بالأصفار البادئة
        String paddedTag = "";
        for(int i = tag.length() ; i < USER_TAG_LEN ; i++){
            paddedTag += "0";
        }
        paddedTag += tag;
        Serial.print("علامة المستخدم للإضافة: ");
        Serial.println(paddedTag);

        if (findUserTagIndex(paddedTag) != -1) {
            _server.send(409, "application/json", "{\"status\":\"error\",\"message\":\"العلامة موجودة بالفعل\"}");
            return;
        }

        if(storeTag(paddedTag)){
            _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"تمت إضافة علامة المستخدم بنجاح\"}");
            return;
        } else {
            _server.send(500, "application/json", "{\"status\":\"error\",\"message\":\"فشل إضافة علامة المستخدم\"}");
            return;
        }
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"جسم الطلب غير صالح. المتوقع {\\\"tag\\\":\\\"11_digits\\\"}\"}");
}

// وظيفة مساعدة لحذف علامة مستخدم وإزاحة العلامات اللاحقة
void UserManager::shiftTagsAndDelete(int indexToDelete) {
    int userCount = getUserTagCountFromEEPROM();
    // إزاحة العلامات اللاحقة لملء الفجوة
    for (int i = indexToDelete; i < userCount - 1; ++i) {
        int nextTagAddr = USER_TAGS_START_ADDR + ((i + 1) * USER_TAG_LEN);
        String nextTag = readStringFromEEPROM(nextTagAddr, USER_TAG_LEN);
        saveStringToEEPROM(USER_TAGS_START_ADDR + (i * USER_TAG_LEN), nextTag, USER_TAG_LEN);
#ifdef ENABLE_USER_STATISTICS
        UpdateStatistics(i, GetStatistics(i + 1)); // إزاحة الإحصائيات أيضاً
#endif
    }
    // مسح مساحة العلامة الأخيرة وإحصائياتها
    saveStringToEEPROM(USER_TAGS_START_ADDR + ((userCount - 1) * USER_TAG_LEN), "", USER_TAG_LEN);
#ifdef ENABLE_USER_STATISTICS
    ClearStatisticsAtIndex(userCount - 1);
#endif
    saveUserTagCountToEEPROM(userCount - 1); // تحديث العدد الإجمالي للمستخدمين
}

// معالج لحذف علامة مستخدم
void UserManager::handleDeleteUserTag() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON غير صالح\"}");
            return;
        }
        String tag = doc["tag"].as<String>();
        
        // حشو بالأصفار البادئة
        String paddedTag = "";
        for(int i = tag.length() ; i < USER_TAG_LEN ; i++){
            paddedTag += "0";
        }
        paddedTag += tag;
        Serial.print("علامة المستخدم للحذف: ");
        Serial.println(paddedTag);

        int index = findUserTagIndex(paddedTag);
        if (index != -1) {
            shiftTagsAndDelete(index); // استخدام وظيفة المساعدة للحذف والإزاحة
            _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"تم حذف علامة المستخدم بنجاح\"}");
            Serial.print("تم حذف علامة المستخدم: ");
            Serial.println(paddedTag);
            return;
        } else {
            _server.send(200, "application/json", "{\"status\":\"success\",\"found\":false,\"message\":\"لم يتم العثور على علامة المستخدم\"}");
            Serial.print("لم يتم العثور على علامة المستخدم: ");
            Serial.println(paddedTag);
            return;
        }
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"جسم الطلب غير صالح. المتوقع {\\\"tag\\\":\\\"11_digits\\\"}\"}");
}

// معالج للتحقق من وجود علامة مستخدم
void UserManager::handleCheckUserTag() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON غير صالح\"}");
            return;
        }
        String tag = doc["tag"].as<String>();
        // حشو بالأصفار البادئة
        String paddedTag = "";
        for(int i = tag.length() ; i < USER_TAG_LEN ; i++){
            paddedTag += "0";
        }
        paddedTag += tag;
        Serial.print("علامة المستخدم للتحقق: ");
        Serial.println(paddedTag);

        int index = findUserTagIndex(paddedTag);
        if (index != -1) {
            _server.send(200, "application/json", "{\"status\":\"success\",\"found\":true,\"message\":\"تم العثور على علامة المستخدم\"}");
            Serial.print("تم العثور على علامة المستخدم: ");
            Serial.println(paddedTag);
            return;
        } else {
            _server.send(200, "application/json", "{\"status\":\"success\",\"found\":false,\"message\":\"لم يتم العثور على علامة المستخدم\"}");
            Serial.print("لم يتم العثور على علامة المستخدم: ");
            Serial.println(paddedTag);
            return;
        }
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"جسم الطلب غير صالح. المتوقع {\\\"tag\\\":\\\"11_digits\\\"}\"}");
}

// معالج للحصول على عدد علامات المستخدمين
void UserManager::handleGetUserTagCount() {
    int userCount = getUserTagCountFromEEPROM();
    String response = "{\"status\":\"success\",\"count\":" + String(userCount) + "}";
    _server.send(200, "application/json", response);
}

// معالج للحصول على قائمة بجميع علامات المستخدمين
void UserManager::handleGetTags() {
    int usercount = getUserTagCountFromEEPROM();
    String jsonTagsArray = "["; 
    for (int i = 0; i < usercount; i++) {
        int currentTagAddr = USER_TAGS_START_ADDR + (i * USER_TAG_LEN);
        String storedTag = readStringFromEEPROM(currentTagAddr, USER_TAG_LEN);
        storedTag.trim(); 
        jsonTagsArray += "\"" + storedTag + "\""; // إضافة العلامة كسلسلة نصية
        if (i < usercount - 1) {
            jsonTagsArray += ",";
        }
    }
    jsonTagsArray += "]"; 
    String response = "{\"status\":\"success\",\"tags\":" + jsonTagsArray + "}";
    _server.send(200, "application/json", response);
}

// معالج لاستخدام علامة مستخدم (تشغيل المرحل وتحديث الإحصائيات)
void UserManager::handleUseUserTag() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON غير صالح\"}");
            return;
        }
        String tag = doc["tag"].as<String>();
        // حشو بالأصفار البادئة
        String paddedTag = "";
        for(int i = tag.length() ; i < USER_TAG_LEN ; i++){
            paddedTag += "0";
        }
        paddedTag += tag;
        Serial.print("علامة المستخدم للاستخدام: ");
        Serial.println(paddedTag);

        int index = findUserTagIndex(paddedTag);
        if (index != -1) {
            setRelayPhysicalState(true); // تشغيل المرحل
            _server.send(200, "application/json", "{\"status\":\"success\",\"found\":true,\"message\":\"تم العثور على علامة المستخدم\"}");
            Serial.print("تم العثور على علامة المستخدم: ");
            Serial.println(paddedTag);
            delay(5000); // تأخير 5 ثواني
            setRelayPhysicalState(false); // إيقاف المرحل
#ifdef ENABLE_USER_STATISTICS
            if (_statisticsEnabled) { // تحديث الإحصائيات فقط إذا كانت الميزة مفعلة
                IncrementStatistics(index); // تحديث الإحصائيات لهذه العلامة
            }
#endif
            return;
        } else {
            _server.send(200, "application/json", "{\"status\":\"success\",\"found\":false,\"message\":\"لم يتم العثور على علامة المستخدم\"}");
            Serial.print("لم يتم العثور على علامة المستخدم: ");
            Serial.println(paddedTag);
            return;
        }
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"جسم الطلب غير صالح. المتوقع {\\\"tag\\\":\\\"11_digits\\\"}\"}");
}

// --- وظائف إدارة إحصائيات المستخدمين ---

#ifdef ENABLE_USER_STATISTICS
// تحديث إحصائية مستخدم في فهرس معين بعدد معين
void UserManager::UpdateStatistics(int index, int count){
    EEPROMHelper::writeInt(index * sizeof(int) + STATISTICS_START_ADDR, count);
}

// معالج لحفظ الحد الأقصى لعدد المستخدمين
void UserManager::handleSetUsersMaxNumber(){
    if (_server.hasArg("plain")) {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON غير صالح\"}");
            return;
        }
        int userNum = doc["userNum"].as<int>(); // قراءة العدد كـ int مباشرة
        
        // التحقق من أن القيمة لا تتجاوز الحد الأقصى الفعلي للنظام
        if (userNum > MAX_USER_TAGS) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"الحد الأقصى لعدد المستخدمين لا يمكن أن يتجاوز " + String(MAX_USER_TAGS) + "\"}");
            return;
        }
        
        EEPROMHelper::writeInt(MAX_NUM_OF_USERS_ADD, userNum); 
        _maxConfigurableUsers = userNum; // تحديث المتغير المحلي
        
        _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"تم تحديث الحد الأقصى لعدد المستخدمين\"}");
        Serial.print("تم تعيين الحد الأقصى لعدد المستخدمين إلى: ");
        Serial.println(userNum);
        return;
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"جسم الطلب غير صالح. المتوقع {\"userNum\":number}\"}");
}

// مسح إحصائية مستخدم في فهرس معين (تعيينها إلى 0)
void UserManager::ClearStatisticsAtIndex(int index){
    EEPROMHelper::writeInt(index * sizeof(int) + STATISTICS_START_ADDR, 0); 
}

// زيادة إحصائية مستخدم في فهرس معين بمقدار 1
void UserManager::IncrementStatistics(int index){
    int count = EEPROMHelper::readInt(index * sizeof(int) + STATISTICS_START_ADDR);
    count++;
    EEPROMHelper::writeInt(index * sizeof(int) + STATISTICS_START_ADDR, count); 
}

// الحصول على إحصائية مستخدم في فهرس معين
int UserManager::GetStatistics(int index){
    return EEPROMHelper::readInt(index * sizeof(int) + STATISTICS_START_ADDR);
}

// معالج للحصول على إحصائيات جميع المستخدمين
void UserManager::handleGetStatistics() {
    int usercount = getUserTagCountFromEEPROM(); // الحصول على عدد المستخدمين الحالي
    Serial.print("عدد المستخدمين للإحصائيات: ");
    Serial.println(usercount);
    String jsonUsersArray = "["; 
    for (int i = 0; i < usercount; i++) {
        int currentTagAddr = USER_TAGS_START_ADDR + (i * USER_TAG_LEN);
        String storedTag = readStringFromEEPROM(currentTagAddr, USER_TAG_LEN); 
        storedTag.trim(); // إزالة المسافات البيضاء
        int count = GetStatistics(i); // الحصول على الإحصائية للمستخدم الحالي

        jsonUsersArray += "{\"tag\":\"" + storedTag + "\",\"count\":" + String(count) + "}";
        if (i < usercount - 1) {
            jsonUsersArray += ",";
        }
    }
    jsonUsersArray += "]"; 
    String response = "{\"status\":\"success\",\"users\":" + jsonUsersArray + "}";
    Serial.println(response);
    _server.send(200, "application/json", response);
}

// --- وظائف خاصة بحالة تفعيل الإحصائيات ---

// قراءة حالة تفعيل الإحصائيات من EEPROM
bool UserManager::readStatisticsEnabledState() {
    uint8_t state = EEPROMHelper::readByte(STATISTICS_ENABLED_ADDR);
    // إذا كانت القيمة غير 0 أو 1، افترض أنها غير مفعلة (افتراضي)
    if (state != 0 && state != 1) {
        return false; 
    }
    return state == 1;
}

// حفظ حالة تفعيل الإحصائيات في EEPROM
void UserManager::saveStatisticsEnabledState(bool enabled) {
    EEPROMHelper::writeByte(STATISTICS_ENABLED_ADDR, enabled ? 1 : 0);
    _statisticsEnabled = enabled; // تحديث المتغير المحلي أيضاً
    Serial.print("تم تعيين حالة الإحصائيات إلى: ");
    Serial.println(enabled ? "مفعلة" : "معطلة");
}

// معالج لتعيين حالة تفعيل الإحصائيات عبر API
void UserManager::handleSetStatisticsEnabled() {
    if (_server.hasArg("plain")) {
        StaticJsonDocument<100> doc;
        DeserializationError error = deserializeJson(doc, _server.arg("plain"));
        if (error) {
            _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"JSON غير صالح\"}");
            return;
        }
        bool enabled = doc["enabled"].as<bool>();
        saveStatisticsEnabledState(enabled);
        _server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"تم تحديث حالة الإحصائيات\"}");
        return;
    }
    _server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"جسم الطلب غير صالح. المتوقع {\"enabled\":true/false}\"}");
}
#endif // ENABLE_USER_STATISTICS
