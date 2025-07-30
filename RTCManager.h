// RTCManager.h
#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include "Config.h" // For basic types, Wire.h
#include "RTClib.h" // For RTC_DS3231

// فئة RTCManager لإدارة وظائف ساعة الوقت الحقيقي (RTC)
// هذه الفئة لا ترث من MainControlClass وتعمل كأداة مساعدة مستقلة.
class RTCManager { 
private:
    RTC_DS3231 _rtc; // كائن RTC

public:
    RTCManager(); // مُنشئ بسيط

    // بدء تشغيل وحدة RTC
    bool beginRTC();
    // الحصول على الوقت والتاريخ الحالي من RTC
    DateTime now();
    // ضبط الوقت والتاريخ في RTC
    void adjustRTC(const DateTime& dateTime);
    // لا توجد نقاط نهاية API هنا، حيث أنها فئة مساعدة
};

#endif // RTC_MANAGER_H
