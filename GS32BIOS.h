#ifndef GS32_BIOS_H
#define GS32_BIOS_H

#include <Arduino.h>
#include <esp_system.h>
#include <vector>
#include <functional>

enum ItemType { 
  TYPE_INFO, 
  TYPE_TEXT, 
  TYPE_INT, 
  TYPE_BOOL, 
  TYPE_SELECT, 
  TYPE_ACTION 
};

struct MenuItem {
  String page;       
  String label;   
  ItemType type;       
  void* valPtr;        
  int maxOptions;      
  int minVal;          
  int maxVal;          
  std::function<void()> action;    
  const char** options;
  size_t maxLen;       // Максимальная длина строки для TYPE_TEXT
  bool allowEmpty;     // Разрешено ли сохранять пустую строку
};

class GS32BIOS {
public:
  GS32BIOS();
  
  // Инициализация (чисто локальная, без Wi-Fi и серверов)
  void begin();
  void handle();

  // Брендинг и метаданные
  void setHeaderTitle(const String &title);
  void setProductInfo(const String &name, const String &version);

  // Управление страницами и пунктами меню
  void addPage(const String &pageName);
  void addInfo(const String &label, const String &value);
  void addText(const String &pageName, const String &label, char* valPtr, size_t maxLen, bool allowEmpty = false);
  void addInt(const String &pageName, const String &label, int* valPtr, int minVal, int maxVal);
  void addBool(const String &pageName, const String &label, bool* valPtr);
  void addSelect(const String &pageName, const String &label, int* valPtr, int optionsCount, const char** options);
  void addAction(const String &pageName, const String &label, std::function<void()> action);

  // События (Callback)
  void onSave(std::function<void()> callback);
  void onFactoryReset(std::function<void()> callback);

private:
  String headerTitle;
  String productName;
  String productVersion;

  std::vector<String> pages;
  std::vector<MenuItem> menuItems;

  int currentPageIdx;
  int activeItemIndex;
  int localActiveIndex;
  
  bool isEditing;
  bool isSubMenuOpen;
  int subMenuSelectionIndex;
  String inputBuffer;
  char originalTextValue[64]; // Буфер для восстановления при Esc

  // Системные строки (только память и аптайм, без Wi-Fi)
  char sys_chip_model[32];
  char sys_flash_size[16];
  char sys_free_ram[16];
  char sys_uptime[16];

  std::function<void()> saveCallback;
  std::function<void()> resetCallback;

  void updateSystemStats();
  void updateActiveIndex();
  void moveActiveItem(int dir);
  
  void renderMenu(Stream &client);
  void handleInput(Stream &client);
  
  void setCursor(Stream &c, int r, int col);
  void setColors(Stream &c, const char* code);
  void drawRect(Stream &c, int sr, int sc, int h, int w, const char* clr);
};

#endif
