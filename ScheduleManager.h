// ScheduleManager.h
#ifndef SCHEDULE_MANAGER_H
#define SCHEDULE_MANAGER_H

#include "Config.h"
#include "MainControl.h" // الوراثة من MainControlClass
#include "RTCManager.h"  // تضمين RTCManager كفئة مساعدة

// فئة ScheduleManagerClass لإدارة الجداول الزمنية لتشغيل/إيقاف المرحل
class ScheduleManagerClass : public MainControlClass {
private:
    RTCManager _rtcManager; // كائن RTCManager لإدارة الوقت
    unsigned long _lastScheduleCheck = 0; // لتتبع آخر فحص للجدول الزمني (لمنع التكرار)
    DateTime _lastCheckedTime; // لتخزين آخر وقت تم فحص الجداول فيه

public:
    // المُنشئ (Constructor) لفئة ScheduleManagerClass
#ifdef USE_EXTERNAL_EEPROM
    ScheduleManagerClass(WebServer& serverRef, int relayPin);
#else
    ScheduleManagerClass(WebServer& serverRef, int relayPin, EEPROMClass& eepromRef);
#endif

    // إعداد نقاط نهاية API المتعلقة بالجداول الزمنية
    void setupScheduleEndpoints();
    // وظيفة يتم استدعاؤها في دالة loop() الرئيسية لفحص الجداول الزمنية
    void loopTasks(); 

private:
    // --- وظائف مساعدة لـ EEPROM (تستخدم EEPROMHelper) ---
    // حفظ جدول زمني في EEPROM في فهرس محدد
    void saveScheduleToEEPROM(int index, const Schedule& s);
    // قراءة جدول زمني من EEPROM من فهرس محدد
    Schedule readScheduleFromEEPROM(int index);    
    // قراءة آخر معرف جدول زمني تم استخدامه
    uint8_t readLastScheduleId();
    // كتابة آخر معرف جدول زمني تم استخدامه
    void writeLastScheduleId(uint8_t id);
    
    // --- معالجات API لإدارة الجداول الزمنية ---
    void handleAddSchedule();    // إضافة جدول زمني جديد
    void handleGetSchedules();   // الحصول على جميع الجداول الزمنية
    void handleUpdateSchedule(); // تحديث جدول زمني موجود
    void handleDeleteSchedule(); // حذف جدول زمني
    void checkSchedules();       // وظيفة داخلية لفحص وتفعيل الجداول الزمنية

    // --- معالجات API لـ RTC (خاصة بـ ScheduleManager) ---
    void handleGetTime(); // الحصول على الوقت
    void handleSetTime(); // تعيين الوقت
};

#endif // SCHEDULE_MANAGER_H
