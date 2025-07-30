// PrayerTimesManager.h
#ifndef PRAYER_TIMES_MANAGER_H
#define PRAYER_TIMES_MANAGER_H

#include "Config.h"
#include "MainControl.h" // الوراثة من MainControlClass
#include "PrayerTimes.h" // تضمين مكتبة PrayerTimes
#include "RTCManager.h"  // تضمين RTCManager كفئة مساعدة

// فئة PrayerTimesManagementClass لإدارة أوقات الصلاة والمرحل التلقائي
class PrayerTimesManagementClass : public MainControlClass { 
private:
    RTCManager _rtcManager; // كائن RTCManager لإدارة الوقت
    PrayerTimes _prayerTimes; // كائن أوقات الصلاة
    double _latitude;         // خط العرض لموقع الصلاة
    double _longitude;        // خط الطول لموقع الصلاة
    int _timezone;            // المنطقة الزمنية لموقع الصلاة
    AutoRelayPrayerConfig _autoRelayConfig; // إعدادات المرحل التلقائي لأوقات الصلاة
    unsigned long _lastPrayerCheck = 0; // لتتبع آخر فحص لأوقات الصلاة

public:
    // المُنشئ (Constructor) لفئة PrayerTimesManagementClass
#ifdef USE_EXTERNAL_EEPROM
    PrayerTimesManagementClass(WebServer& serverRef, int relayPin);
#else
    PrayerTimesManagementClass(WebServer& serverRef, int relayPin, EEPROMClass& eepromRef);
#endif

    // إعداد نقاط نهاية API المتعلقة بأوقات الصلاة
    void setupPrayerEndpoints();
    // وظيفة يتم استدعاؤها في دالة loop() الرئيسية لفحص المرحل التلقائي
    void loopTasks(); 

private: 
    // حفظ إعدادات أوقات الصلاة في EEPROM
    void savePrayerConfig(double lat, double lon, int tz);
    // قراءة إعدادات أوقات الصلاة من EEPROM
    void readPrayerConfig(double& lat, double& lon, int& tz);
    // حفظ إعدادات المرحل التلقائي لأوقات الصلاة في EEPROM
    void saveAutoRelayConfig(const AutoRelayPrayerConfig& config);
    // قراءة إعدادات المرحل التلقائي لأوقات الصلاة من EEPROM
    AutoRelayPrayerConfig readAutoRelayConfig();

    // --- معالجات API لإدارة أوقات الصلاة ---
    void handleSetPrayerConfig();      // تعيين إعدادات أوقات الصلاة
    void handleGetPrayerConfig();      // الحصول على إعدادات أوقات الصلاة
    void handleGetPrayerTimes();       // الحصول على أوقات الصلاة لليوم الحالي
    void handleSetAutoRelayConfig();   // تعيين إعدادات المرحل التلقائي لأوقات الصلاة
    void handleGetAutoRelayConfig();   // الحصول على إعدادات المرحل التلقائي لأوقات الصلاة
    void handleAutoRelayByPrayerTimes(); // وظيفة داخلية لتشغيل المرحل تلقائياً بناءً على أوقات الصلاة

    // --- معالجات API لـ RTC (خاصة بـ PrayerTimesManager) ---
    void handleGetTime(); // الحصول على الوقت
    void handleSetTime(); // تعيين الوقت
};

#endif // PRAYER_TIMES_MANAGER_H
