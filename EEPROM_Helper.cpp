// EEPROM_Helper.cpp
#include "EEPROM_Helper.h"

// قسم الكود الخاص بـ EEPROM الخارجية (باستخدام Wire)
#ifdef USE_EXTERNAL_EEPROM

// قراءة بايت واحد من EEPROM الخارجية
uint8_t EEPROMHelper::readByte(unsigned int address) {
    Wire.beginTransmission(EXTERNAL_EEPROM_ADDR);
    Wire.write((int)(address >> 8));   // الجزء العلوي من العنوان (MSB)
    Wire.write((int)(address & 0xFF)); // الجزء السفلي من العنوان (LSB)
    Wire.endTransmission();
    Wire.requestFrom(EXTERNAL_EEPROM_ADDR, 1); // طلب بايت واحد
    return Wire.read(); // قراءة البايت
}

// كتابة بايت واحد في EEPROM الخارجية
void EEPROMHelper::writeByte(unsigned int address, uint8_t data) {
    Wire.beginTransmission(EXTERNAL_EEPROM_ADDR);
    Wire.write((int)(address >> 8));   // الجزء العلوي من العنوان (MSB)
    Wire.write((int)(address & 0xFF)); // الجزء السفلي من العنوان (LSB)
    Wire.write(data); // كتابة البيانات
    Wire.endTransmission();
    delay(5); // تأخير قصير للسماح لـ EEPROM بإكمال دورة الكتابة
}

// قراءة بايتات متعددة من EEPROM الخارجية
void EEPROMHelper::readBytes(unsigned int address, byte* buffer, int length) {
    Wire.beginTransmission(EXTERNAL_EEPROM_ADDR);
    Wire.write((int)(address >> 8));   // الجزء العلوي من العنوان (MSB)
    Wire.write((int)(address & 0xFF)); // الجزء السفلي من العنوان (LSB)
    Wire.endTransmission();
    Wire.requestFrom(EXTERNAL_EEPROM_ADDR, length); // طلب عدد معين من البايتات
    for (int i = 0; i < length; i++) {
        if (Wire.available()) {
            buffer[i] = Wire.read(); // قراءة البايتات في المخزن المؤقت
        }
    }
}

// كتابة بايتات متعددة في EEPROM الخارجية
void EEPROMHelper::writeBytes(unsigned int address, const byte* buffer, int length) {
    Wire.beginTransmission(EXTERNAL_EEPROM_ADDR);
    Wire.write((int)(address >> 8));   // الجزء العلوي من العنوان (MSB)
    Wire.write((int)(address & 0xFF)); // الجزء السفلي من العنوان (LSB)
    for (int i = 0; i < length; i++) {
        Wire.write(buffer[i]); // كتابة البايتات من المخزن المؤقت
    }
    Wire.endTransmission();
    delay(5); // تأخير قصير للسماح لـ EEPROM بإكمال دورة الكتابة
}

// قراءة قيمة عدد صحيح (int) من EEPROM الخارجية
int EEPROMHelper::readInt(unsigned int address) {
    int value;
    readBytes(address, (byte*)&value, sizeof(int)); // قراءة البايتات وتحويلها إلى int
    return value;
}

// كتابة قيمة عدد صحيح (int) في EEPROM الخارجية
void EEPROMHelper::writeInt(unsigned int address, int value) {
    writeBytes(address, (const byte*)&value, sizeof(int)); // كتابة البايتات من int
}

// قراءة سلسلة نصية (String) من EEPROM الخارجية
String EEPROMHelper::readString(uint16_t address, uint16_t length) {
    String result = "";
    Wire.beginTransmission(EXTERNAL_EEPROM_ADDR);
    Wire.write((address >> 8) & 0xFF); // الجزء العلوي من العنوان (MSB)
    Wire.write(address & 0xFF);        // الجزء السفلي من العنوان (LSB)
    Wire.endTransmission();

    Wire.requestFrom(EXTERNAL_EEPROM_ADDR, length); // طلب السلسلة بطولها المحدد

    while (Wire.available()) {
        char c = Wire.read();
        result += c; // إضافة الحرف إلى السلسلة
    }
    return result;
}

// كتابة سلسلة نصية (String) في EEPROM الخارجية
void EEPROMHelper::writeString(uint16_t address, String data) {
    Wire.beginTransmission(EXTERNAL_EEPROM_ADDR);
    Wire.write((address >> 8) & 0xFF); // الجزء العلوي من العنوان (MSB)
    Wire.write(address & 0xFF);        // الجزء السفلي من العنوان (LSB)

    for (uint16_t i = 0; i < data.length(); i++) {
        Wire.write(data[i]); // كتابة كل حرف من السلسلة
    }
    Wire.endTransmission();
    delay(5);  // تأخير قصير لدورة كتابة EEPROM
}

// قسم الكود الخاص بـ EEPROM الداخلية (باستخدام مكتبة EEPROM)
#else 

// قراءة بايت واحد من EEPROM الداخلية
uint8_t EEPROMHelper::readByte(unsigned int address) {
    return EEPROM.read(address);
}

// كتابة بايت واحد في EEPROM الداخلية
void EEPROMHelper::writeByte(unsigned int address, uint8_t data) {
    EEPROM.write(address, data);
    EEPROM.commit(); // حفظ التغييرات
}

// قراءة بايتات متعددة من EEPROM الداخلية
void EEPROMHelper::readBytes(unsigned int address, byte* buffer, int length) {
    EEPROM.readBytes(address, buffer, length);
}

// كتابة بايتات متعددة في EEPROM الداخلية
void EEPROMHelper::writeBytes(unsigned int address, const byte* buffer, int length) {
    EEPROM.writeBytes(address, buffer, length);
    EEPROM.commit(); // حفظ التغييرات
}

// قراءة قيمة عدد صحيح (int) من EEPROM الداخلية
int EEPROMHelper::readInt(unsigned int address) {
    return EEPROM.readInt(address);
}

// كتابة قيمة عدد صحيح (int) في EEPROM الداخلية
void EEPROMHelper::writeInt(unsigned int address, int value) {
    EEPROM.writeInt(address, value);
    EEPROM.commit(); // حفظ التغييرات
}

// قراءة سلسلة نصية (String) من EEPROM الداخلية
String EEPROMHelper::readString(uint16_t address, uint16_t length) {
    String data = "";
    for (int i = 0; i < length; ++i) {
        char c = EEPROM.read(address + i);
        if (c == 0) { // عند الوصول إلى حرف النهاية (Null terminator)
            break;
        }
        data += c;
    }
    return data;
}

// كتابة سلسلة نصية (String) في EEPROM الداخلية
void EEPROMHelper::writeString(uint16_t address, String data) {
    int len = data.length();
    for (int i = 0; i < len; ++i) {
        EEPROM.write(address + i, data.charAt(i));
    }
    EEPROM.write(address + len, 0); // إضافة حرف النهاية (Null terminator)
    EEPROM.commit(); // حفظ التغييرات
}

#endif // USE_EXTERNAL_EEPROM
