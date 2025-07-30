// MainControl.h
#ifndef MAIN_CONTROL_H
#define MAIN_CONTROL_H

#include "Config.h"
#include "EEPROM_Helper.h" // تضمين الفئة المساعدة لـ EEPROM

// فئة التحكم الرئيسية (MainControlClass)
// توفر الوظائف الأساسية للتحكم في الجهاز وإدارة الخادم الويب
class MainControlClass {
protected: // الأعضاء المحمية يمكن الوصول إليها من الفئات المشتقة
    WebServer& _server; // مرجع لكائن خادم الويب الرئيسي
    int _relayPin;      // دبوس المرحل (Relay)

#ifndef USE_EXTERNAL_EEPROM
    EEPROMClass& _eeprom; // مرجع لكائن EEPROM الداخلية (فقط إذا لم يتم استخدام الخارجية)
#endif

public:
    // المُنشئ (Constructor) لفئة MainControlClass
#ifdef USE_EXTERNAL_EEPROM
    MainControlClass(WebServer& serverRef, int relayPin); // لا يوجد مرجع لـ EEPROMClass
#else
    MainControlClass(WebServer& serverRef, int relayPin, EEPROMClass& eepromRef);
#endif

    // بدء تشغيل نقطة الوصول (AP) وخادم الويب
    void beginAPAndWebServer(const char* ap_ssid, const char* ap_password);
    // معالجة طلبات العملاء (يجب استدعاؤها بشكل متكرر في دالة loop())
    void handleClient();
    // إعادة تعيين جميع الإعدادات إلى القيم الافتراضية
    void resetConfigurations();
    // تعيين الحالة الفيزيائية للمرحل وحفظها في EEPROM
    void setRelayPhysicalState(bool state);
    
    // وظائف متعلقة بـ EEPROM (تستخدم EEPROMHelper)
    // قراءة سلسلة نصية من EEPROM
    String readStringFromEEPROM(int address, int max_len);
    // حفظ سلسلة نصية في EEPROM
    void saveStringToEEPROM(int address, const String& data, int max_len);
    // حفظ حالة المرحل في EEPROM
    void saveRelayStateToEEPROM(bool state);
    // الحصول على حالة المرحل من EEPROM
    bool getRelayStateFromEEPROM();

protected: // المعالجات الخاصة (الآن محمية للوصول من الفئات المشتقة)
    // --- معالجات إدارة Wi-Fi ---
    void handleSetSSID();
    void handleGetSSID();
    void handleSetPassword();
    void handleGetPassword();
    void handleGetnetworkinfo();
    void handleSetnetworkinfo();

    // --- معالجات إدارة المرحل (Relay) ---
    void handleSetRelayState();
    void handleGetRelayState();
    void handleToggleRelay();

    // --- معالجات عامة ---
    void handleNotFound(); // معالج الطلبات غير الموجودة
};

#endif // MAIN_CONTROL_H
