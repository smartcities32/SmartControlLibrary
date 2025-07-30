// Config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Wire.h> 
#include <EEPROM.h>
#include "RTClib.h" 
#include <ArduinoJson.h> 
#include "PrayerTimes.h" 

#ifdef ESP32
#include <WiFi.h>
#include <WebServer.h>
#elif ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#define WebServer ESP8266WebServer 
#endif

// تضمين مكتبة Wire للتعامل مع EEPROM الخارجية
#include <Wire.h> 
// عنوان I2C الافتراضي لـ EEPROM 24C256
#define EXTERNAL_EEPROM_ADDR 0x50 
// تعريف هذا لاستخدام EEPROM خارجية بشكل مشروط
#define USE_EXTERNAL_EEPROM 

// تعريف دبابيس I2C (SDA, SCL) لـ EEPROM
#define EEPROM_SDA_PIN 0
#define EEPROM_SCL_PIN 2

// --- تعريفات الأجهزة ---
// دبوس المرحل (Relay)
#define RELAY_PIN 16
// حجم EEPROM الداخلية (إذا لم يتم استخدام الخارجية)
#define EEPROM_SIZE 1024 
// حجم EEPROM الخارجية (لضمان مساحة كافية)
#define EX_EEPROM_SIZE 32000 

// الحد الأقصى لطول SSID وكلمة المرور
#define SSID_MAX_LEN 16
#define PASSWORD_MAX_LEN 16
// طول علامة المستخدم (البطاقة)
#define USER_TAG_LEN 11 
// الحد الأقصى لعدد المستخدمين المدعومين (هذا هو الحد الأقصى الفعلي لقدرة النظام)
#define MAX_USER_TAGS 300

// --- تعيينات عناوين EEPROM (تم تعديلها لتجنب التداخل) ---
// عنوان حالة المرحل (Relay) - 1 بايت (true/false)
#define RELAY_STATE_ADDR 0 
// عنوان طريقة التشغيل (0: يدوي, 1: تلقائي) - 1 بايت
#define OP_METHOD_ADDR 1 // تم الإبقاء عليه لعدم كسر تسلسل العناوين القديم، لكنه غير مستخدم الآن
// عنوان SSID لشبكة AP - 16 بايت
#define SSID_ADDR (OP_METHOD_ADDR + 1) 
// عنوان كلمة المرور لشبكة AP - 16 بايت
#define PASSWORD_ADDR (SSID_ADDR + SSID_MAX_LEN) 
// عنوان بطاقة الإضافة - 11 بايت
#define ADD_CARD_ADDR (PASSWORD_ADDR + PASSWORD_MAX_LEN) 
// عنوان بطاقة الإزالة - 11 بايت
#define REMOVE_CARD_ADDR (ADD_CARD_ADDR + USER_TAG_LEN) 
// عنوان الحد الأقصى لعدد المستخدمين المسموح بإضافتهم (قيمة قابلة للتكوين) - 4 بايت (int)
#define MAX_NUM_OF_USERS_ADD (REMOVE_CARD_ADDR + USER_TAG_LEN) 

// عنوان آخر معرف جدول زمني تم استخدامه - 1 بايت
#define LAST_SCHEDULE_ID_ADDR (MAX_NUM_OF_USERS_ADD + sizeof(int)) 
// عنوان إعدادات أوقات الصلاة (خط العرض، خط الطول، المنطقة الزمنية) - 28 بايت (2 * sizeof(double) + sizeof(int))
#define PRAYER_CONFIG_ADDR (LAST_SCHEDULE_ID_ADDR + sizeof(uint8_t)) 
// عنوان إعدادات المرحل التلقائي لأوقات الصلاة - 25 بايت (حجم هيكل AutoRelayPrayerConfig)
#define AUTO_RELAY_CONFIG_ADDR (PRAYER_CONFIG_ADDR + (2 * sizeof(double) + sizeof(int))) 

// -------------------------------------------------------------------
// #define ENABLE_USER_STATISTICS // قم بإزالة التعليق لتفعيل ميزة الإحصائيات للمستخدمين
// -------------------------------------------------------------------

#ifdef ENABLE_USER_STATISTICS
// عنوان حالة تفعيل/إلغاء تفعيل الإحصائيات - 1 بايت (true/false)
#define STATISTICS_ENABLED_ADDR (AUTO_RELAY_CONFIG_ADDR + sizeof(AutoRelayPrayerConfig))
// عنوان عدد علامات المستخدمين المخزنة - 4 بايت (int)
#define USER_TAG_COUNT_ADDR (STATISTICS_ENABLED_ADDR + sizeof(uint8_t)) 
// عنوان بداية تخزين علامات المستخدمين - (MAX_USER_TAGS * USER_TAG_LEN) بايت
#define USER_TAGS_START_ADDR (USER_TAG_COUNT_ADDR + sizeof(int)) 
// عنوان بداية تخزين إحصائيات المستخدمين - (MAX_USER_TAGS * sizeof(int)) بايت
#define STATISTICS_START_ADDR (USER_TAGS_START_ADDR + (MAX_USER_TAGS * USER_TAG_LEN)) 
#else
// إذا لم يتم تفعيل الإحصائيات، فستكون عناوين EEPROM مختلفة
// عنوان عدد علامات المستخدمين المخزنة - 4 بايت (int)
#define USER_TAG_COUNT_ADDR (AUTO_RELAY_CONFIG_ADDR + sizeof(AutoRelayPrayerConfig)) 
// عنوان بداية تخزين علامات المستخدمين - (MAX_USER_TAGS * USER_TAG_LEN) بايت
#define USER_TAGS_START_ADDR (USER_TAG_COUNT_ADDR + sizeof(int)) 
// لا يوجد STATISTICS_START_ADDR إذا لم يتم تفعيل الإحصائيات
#endif


// الحد الأقصى لعدد الجداول الزمنية
#define MAX_SCHEDULES 10
// حجم هيكل الجدول الزمني بالبايت
#define SCHEDULE_SIZE 13 
// عنوان بداية تخزين الجداول الزمنية - (MAX_SCHEDULES * SCHEDULE_SIZE) بايت
#ifdef ENABLE_USER_STATISTICS
#define SCHEDULE_START_ADDR (STATISTICS_START_ADDR + (MAX_USER_TAGS * sizeof(int))) 
#else
#define SCHEDULE_START_ADDR (USER_TAGS_START_ADDR + (MAX_USER_TAGS * USER_TAG_LEN)) 
#endif

// هيكل إعدادات المرحل التلقائي لأوقات الصلاة
struct AutoRelayPrayerConfig {
    bool enabled;           // هل المرحل التلقائي مفعل؟
    int minutesBefore[3];   // دقائق قبل (للفجر، المغرب، العشاء)
    int minutesAfter[3];    // دقائق بعد (للفجر، المغرب، العشاء)
};

// هيكل الجدول الزمني
struct Schedule {
    uint8_t id;             // معرف فريد للجدول الزمني
    uint8_t hour;           // الساعة (0-23)
    uint8_t minute;         // الدقيقة (0-59)
    bool turnOn;            // True لتشغيل المرحل، False لإيقافه
    bool repeatEveryDay;    // True إذا كان الجدول يتكرر يومياً
    bool days[7];           // مصفوفة لأيام الأسبوع المحددة (الأحد=0، الإثنين=1، ...، السبت=6)
    bool active;            // True إذا كان الجدول نشطاً، False إذا تم حذفه/إلغاء تنشيطه
};

// تعريف كائن الخادم الويب كخارجي ليتم الوصول إليه من جميع الفئات
extern WebServer server; 

#endif // CONFIG_H
