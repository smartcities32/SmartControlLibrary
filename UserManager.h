// UserManager.h
#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include "Config.h"
#include "MainControl.h" // الوراثة من MainControlClass

// فئة UserManager لإدارة المستخدمين وإحصائياتهم
// تجمع وظائف UserManagementClass و UserStatistics السابقة
class UserManager : public MainControlClass {
public:
    // المُنشئ (Constructor) لفئة UserManager
#ifdef USE_EXTERNAL_EEPROM
    UserManager(WebServer& serverRef, int relayPin);
#else
    UserManager(WebServer& serverRef, int relayPin, EEPROMClass& eepromRef);
#endif

    // إعداد نقاط نهاية API المتعلقة بإدارة المستخدمين والإحصائيات
    void setupUserEndpoints();

private:
    int _maxConfigurableUsers; // متغير لتخزين الحد الأقصى لعدد المستخدمين القابل للتكوين

#ifdef ENABLE_USER_STATISTICS
    bool _statisticsEnabled; // متغير لتخزين حالة تفعيل/إلغاء تفعيل الإحصائيات
#endif

    // --- وظائف مساعدة لتوليد كلمات المرور ---
    String generatePassword();
    char getRandomUpperCase();
    char getRandomLowerCase();
    char getRandomNumber();
    char getRandomSymbol();
    char getRandomCharacter();
    String shuffleString(String str); // وظيفة لخلط السلسلة (إذا لزم الأمر)

    // --- معالجات API لإدارة المستخدمين ---
    void handleAddUserTag();
    void handleDeleteUserTag();
    void handleDeleteAllUserTags();
    void handleCheckUserTag();
    void handleGetUserTagCount();
    void handleUseUserTag(); // تم تغيير الاسم ليكون أكثر وضوحاً
    void handleGetTags();
    void handleAddCard();
    void handleRemoveCard();
    void handleGenerateSSIDAndPASS(); // تم تغيير الاسم ليكون أكثر وضوحاً
    void handleSetUsersMaxNumber(); // معالج لتعيين الحد الأقصى لعدد المستخدمين
    void handleGetUsersMaxNumber(); // معالج للحصول على الحد الأقصى لعدد المستخدمين

#ifdef ENABLE_USER_STATISTICS
    void handleSetStatisticsEnabled(); // معالج لتعيين حالة تفعيل الإحصائيات
#endif

    // --- وظائف إدارة علامات المستخدمين (البطاقات) في EEPROM ---
    void saveUserTagCountToEEPROM(int count);
    int getUserTagCountFromEEPROM();
    int findUserTagIndex(const String& tag); // تم تغيير الاسم ليعكس إرجاع الفهرس
    bool storeTag(String tag); // حفظ علامة مستخدم جديدة
    void shiftTagsAndDelete(int indexToDelete); // وظيفة مساعدة لحذف العلامات وإزاحتها

    // --- وظائف إدارة إحصائيات المستخدمين في EEPROM ---
#ifdef ENABLE_USER_STATISTICS
    void UpdateStatistics(int index, int count);
    void ClearStatisticsAtIndex(int index);
    void IncrementStatistics(int index); // تم تغيير الاسم ليكون أكثر وضوحاً
    int GetStatistics(int index);
    void handleGetStatistics();

    // --- وظائف خاصة بحالة تفعيل الإحصائيات ---
    bool readStatisticsEnabledState();
    void saveStatisticsEnabledState(bool enabled);
#endif
};

#endif // USER_MANAGER_H
