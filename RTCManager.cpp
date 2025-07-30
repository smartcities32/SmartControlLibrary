// RTCManager.cpp
#include "RTCManager.h"

RTCManager::RTCManager() : _rtc() {
}

// بدء تشغيل وحدة RTC
bool RTCManager::beginRTC() {
    Wire.begin(); // بدء اتصال I2C
    if (!_rtc.begin()) {
        Serial.println("لم يتم العثور على RTC! يرجى التحقق من التوصيلات.");
        return false;
    }
    if (_rtc.lostPower()) {
        Serial.println("فقدت RTC الطاقة، يتم ضبط الوقت!");
        // ضبط الوقت والتاريخ الحاليين (وقت التجميع)
        _rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    return true;
}

// الحصول على الوقت والتاريخ الحالي من RTC
DateTime RTCManager::now() {
    return _rtc.now();
}

// ضبط الوقت والتاريخ في RTC
void RTCManager::adjustRTC(const DateTime& dateTime) {
    _rtc.adjust(dateTime);
}
// لا توجد معالجات API هنا
