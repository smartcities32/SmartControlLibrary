// EEPROM_Helper.h
#ifndef EEPROM_HELPER_H
#define EEPROM_HELPER_H

#include "Config.h" // لتضمين USE_EXTERNAL_EEPROM, EXTERNAL_EEPROM_ADDR

// فئة مساعدة للتعامل مع عمليات قراءة وكتابة EEPROM
class EEPROMHelper {
public:
    // قراءة بايت واحد من عنوان محدد
    static uint8_t readByte(unsigned int address);
    // كتابة بايت واحد في عنوان محدد
    static void writeByte(unsigned int address, uint8_t data);
    // قراءة عدد معين من البايتات في مخزن مؤقت
    static void readBytes(unsigned int address, byte* buffer, int length);
    // كتابة عدد معين من البايتات من مخزن مؤقت
    static void writeBytes(unsigned int address, const byte* buffer, int length);
    // قراءة قيمة عدد صحيح (int) من عنوان محدد
    static int readInt(unsigned int address);
    // كتابة قيمة عدد صحيح (int) في عنوان محدد
    static void writeInt(unsigned int address, int value);
    // قراءة سلسلة نصية (String) من عنوان محدد بطول معين
    static String readString(uint16_t address, uint16_t length);
    // كتابة سلسلة نصية (String) في عنوان محدد
    static void writeString(uint16_t address, String data);

    // دالة قالبية (Template) لحفظ أي هيكل (struct) أو نوع بيانات في EEPROM
    template <typename T>
    static void put(int address, const T& value) {
        writeBytes(address, (const byte*)&value, sizeof(T));
    }

    // دالة قالبية (Template) لقراءة أي هيكل (struct) أو نوع بيانات من EEPROM
    template <typename T>
    static void get(int address, T& value) {
        readBytes(address, (byte*)&value, sizeof(T));
    }
};

#endif // EEPROM_HELPER_H
