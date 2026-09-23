#include "GS32BIOS.h"

GS32BIOS::GS32BIOS() 
  : headerTitle("GS-32"), 
    productName("GS-32 Engine"), 
    productVersion("v1.0.0"), 
    currentPageIdx(0), 
    activeItemIndex(0), 
    localActiveIndex(0), 
    isEditing(false), 
    isSubMenuOpen(false), 
    subMenuSelectionIndex(0), 
    saveCallback(nullptr), 
    resetCallback(nullptr) {
  
  pages.push_back("Info");
  pages.push_back("Exit");
}

void GS32BIOS::setHeaderTitle(const String &title) {
  headerTitle = title;
}

void GS32BIOS::setProductInfo(const String &name, const String &version) {
  productName = name;
  productVersion = version;
}

void GS32BIOS::addPage(const String &pageName) {
  if (pageName.equals("Info") || pageName.equals("Exit")) return;
  for (const auto &p : pages) {
    if (p.equals(pageName)) return; // Защита от дубликатов
  }
  pages.insert(pages.end() - 1, pageName);
}

void GS32BIOS::addInfo(const String &label, const String &value) {
  MenuItem item;
  item.page = "Info";
  item.label = label;
  item.type = TYPE_INFO;
  
  char* valStr = new char[64];
  strncpy(valStr, value.c_str(), 63);
  valStr[63] = '\0';
  
  item.valPtr = (void*)valStr;
  item.maxOptions = 0; item.minVal = 0; item.maxVal = 0;
  item.action = nullptr; item.options = nullptr;
  item.maxLen = 64; item.allowEmpty = true;
  
  menuItems.push_back(item);
}

void GS32BIOS::addText(const String &pageName, const String &label, char* valPtr, size_t maxLen, bool allowEmpty) {
  MenuItem item;
  item.page = pageName;
  item.label = label;
  item.type = TYPE_TEXT;
  item.valPtr = (void*)valPtr;
  item.maxLen = maxLen;
  item.allowEmpty = allowEmpty;
  item.maxOptions = 0; item.minVal = 0; item.maxVal = 0;
  item.action = nullptr; item.options = nullptr;
  
  menuItems.push_back(item);
}

void GS32BIOS::addInt(const String &pageName, const String &label, int* valPtr, int minVal, int maxVal) {
  MenuItem item;
  item.page = pageName;
  item.label = label;
  item.type = TYPE_INT;
  item.valPtr = (void*)valPtr;
  item.minVal = minVal;
  item.maxVal = maxVal;
  item.maxOptions = 0; item.action = nullptr; item.options = nullptr;
  item.maxLen = 0; item.allowEmpty = true;
  
  menuItems.push_back(item);
}

void GS32BIOS::addBool(const String &pageName, const String &label, bool* valPtr) {
  MenuItem item;
  item.page = pageName;
  item.label = label;
  item.type = TYPE_BOOL;
  item.valPtr = (void*)valPtr;
  item.maxOptions = 0; item.minVal = 0; item.maxVal = 0;
  item.action = nullptr; item.options = nullptr;
  item.maxLen = 0; item.allowEmpty = true;
  
  menuItems.push_back(item);
}

void GS32BIOS::addSelect(const String &pageName, const String &label, int* valPtr, int optionsCount, const char** options) {
  MenuItem item;
  item.page = pageName;
  item.label = label;
  item.type = TYPE_SELECT;
  item.valPtr = (void*)valPtr;
  item.maxOptions = optionsCount;
  item.options = options;
  item.minVal = 0; item.maxVal = 0; item.action = nullptr;
  item.maxLen = 0; item.allowEmpty = true;
  
  menuItems.push_back(item);
}

void GS32BIOS::addAction(const String &pageName, const String &label, std::function<void()> action) {
  MenuItem item;
  item.page = pageName;
  item.label = label;
  item.type = TYPE_ACTION;
  item.action = action;
  item.valPtr = nullptr; item.maxOptions = 0; item.minVal = 0; item.maxVal = 0; item.options = nullptr;
  item.maxLen = 0; item.allowEmpty = true;
  
  menuItems.push_back(item);
}

void GS32BIOS::onSave(std::function<void()> callback) {
  saveCallback = callback;
}

void GS32BIOS::onFactoryReset(std::function<void()> callback) {
  resetCallback = callback;
}

void GS32BIOS::begin() {
  snprintf(sys_chip_model, 32, "%s Rev.%d", ESP.getChipModel(), ESP.getChipRevision());
  snprintf(sys_flash_size, 16, "%d MB", ESP.getFlashChipSize() / (1024 * 1024));

  // 1. Автоматическое добавление стандартных действий на страницу Exit
  addAction("Exit", "Save Changes and Reset  ", [this]() {
    if (saveCallback) saveCallback();
    ESP.restart();
  });
  
  addAction("Exit", "Discard Changes and Exit", []() {
    ESP.restart();
  });
  
  addAction("Exit", "Factory Reset (Default) ", [this]() {
    if (resetCallback) resetCallback();
    ESP.restart();
  });

  // 2. Добавление базовых системных пунктов на страницу Info (без Wi-Fi)
  std::vector<MenuItem> baseInfo;
  
  auto createSysInfo = [this](const char* label, char* valPtr) {
    MenuItem it;
    it.page = "Info"; it.label = label; it.type = TYPE_INFO;
    it.valPtr = (void*)valPtr; it.maxOptions = 0; it.minVal = 0; it.maxVal = 0;
    it.action = nullptr; it.options = nullptr; it.maxLen = 64; it.allowEmpty = true;
    return it;
  };

  baseInfo.push_back(createSysInfo("Product Name       ", (char*)productName.c_str()));
  baseInfo.push_back(createSysInfo("Version            ", (char*)productVersion.c_str()));
  baseInfo.push_back(createSysInfo("Core Architecture  ", sys_chip_model));
  baseInfo.push_back(createSysInfo("Flash Memory Size  ", sys_flash_size));
  baseInfo.push_back(createSysInfo("System Free RAM    ", sys_free_ram));
  baseInfo.push_back(createSysInfo("System Uptime      ", sys_uptime));

  // Вставляем базовую Info в САМОЕ НАЧАЛО вектора меню
  menuItems.insert(menuItems.begin(), baseInfo.begin(), baseInfo.end());

  updateActiveIndex();
  Serial.print("\e[2J");
  renderMenu(Serial);
}

void GS32BIOS::handle() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 1000) {
    lastUpdate = millis();
    updateSystemStats();
  }

  if (Serial.available()) handleInput(Serial);
}

void GS32BIOS::updateSystemStats() {
  snprintf(sys_free_ram, 16, "%d KB", ESP.getFreeHeap() / 1024);
  unsigned long sec = millis() / 1000;
  snprintf(sys_uptime, 16, "%02d:%02d:%02d", (int)(sec/3600), (int)((sec%3600)/60), (int)(sec%60));
}

void GS32BIOS::updateActiveIndex() {
  String curPage = pages[currentPageIdx];
  for (size_t i = 0; i < menuItems.size(); i++) {
    if (menuItems[i].page.equals(curPage)) { 
      activeItemIndex = i; 
      localActiveIndex = 0; 
      return; 
    }
  }
}

void GS32BIOS::moveActiveItem(int dir) {
  String curPage = pages[currentPageIdx];
  int count = 0;
  for (size_t i = 0; i < menuItems.size(); i++) if (menuItems[i].page.equals(curPage)) count++;
  if (count == 0) return;
  
  localActiveIndex = (localActiveIndex + dir + count) % count;
  int curr = 0;
  for (size_t i = 0; i < menuItems.size(); i++) {
    if (menuItems[i].page.equals(curPage)) {
      if (curr == localActiveIndex) { activeItemIndex = i; return; }
      curr++;
    }
  }
}

void GS32BIOS::setCursor(Stream &c, int r, int col) { c.print("\e["); c.print(r); c.print(";"); c.print(col); c.print("H"); }
void GS32BIOS::setColors(Stream &c, const char* code) { c.print("\e["); c.print(code); c.print("m"); }

void GS32BIOS::drawRect(Stream &c, int sr, int sc, int h, int w, const char* clr) {
  setColors(c, clr);
  for (int r = 0; r < h; r++) {
    setCursor(c, sr + r, sc);
    for (int i = 0; i < w; i++) c.print(" ");
  }
}

void GS32BIOS::renderMenu(Stream &client) {
  client.print("\e[?25l"); 
  drawRect(client, 2, 1, 21, 80, "37;44"); // Синий фон рабочей области

  // Верхняя панель бренда
  drawRect(client, 1, 1, 1, 80, "30;47");
  setCursor(client, 1, 3); client.print(headerTitle);

  // Отрисовка вкладок
  int currentCol = 14;
  for (size_t i = 0; i < pages.size(); i++) {
    setCursor(client, 1, currentCol);
    if ((int)i == currentPageIdx) { 
      setColors(client, "37;44"); client.print(" "); client.print(pages[i]); client.print(" "); 
    } else { 
      setColors(client, "30;47"); client.print(" "); client.print(pages[i]); client.print(" "); 
    }
    currentCol += pages[i].length() + 3;
  }

  // Отрисовка пунктов меню активной страницы
  int row = 3;
  String curPage = pages[currentPageIdx];
  for (size_t i = 0; i < menuItems.size(); i++) {
    if (!menuItems[i].page.equals(curPage)) continue;
    
    setCursor(client, row, 2);
    if ((int)i == activeItemIndex) setColors(client, "30;47"); else setColors(client, "37;44");
    
    char line[81];
    char val[44] = "";
    if (menuItems[i].type == TYPE_INFO) snprintf(val, 44, "%s", (char*)menuItems[i].valPtr);
    else if (menuItems[i].type == TYPE_TEXT) snprintf(val, 44, "%s", (char*)menuItems[i].valPtr);
    else if (menuItems[i].type == TYPE_INT) snprintf(val, 44, "%d", *(int*)menuItems[i].valPtr);
    else if (menuItems[i].type == TYPE_BOOL) snprintf(val, 44, "%s", *(bool*)menuItems[i].valPtr ? "[Enabled]" : "[Disabled]");
    else if (menuItems[i].type == TYPE_SELECT) snprintf(val, 44, "[ %s ]", menuItems[i].options[*(int*)menuItems[i].valPtr]);

    if (menuItems[i].type == TYPE_ACTION) snprintf(line, 81, "   %-30s %-45s", menuItems[i].label.c_str(), "");
    else snprintf(line, 81, "   %-28s : %-43s", menuItems[i].label.c_str(), val);
    
    client.print(line);
    row++;
  }

  // Выпадающее окно подменю выбора (TYPE_SELECT)
  if (isSubMenuOpen) {
    MenuItem *it = &menuItems[activeItemIndex];
    int bh = it->maxOptions + 2, br = 5, bc = 22, bw = 36;
    drawRect(client, br, bc, bh, bw, "30;40");
    setColors(client, "37;40");
    setCursor(client, br, bc); client.print("+"); for(int i=0;i<bw-2;i++) client.print("-"); client.print("+");
    for(int i=0; i<it->maxOptions; i++) {
      setCursor(client, br+1+i, bc); client.print("|");
      if (i == subMenuSelectionIndex) setColors(client, "30;47"); else setColors(client, "37;40");
      client.print(" "); client.print(it->options[i]);
      for(size_t p=strlen(it->options[i]); p<(size_t)(bw-4); p++) client.print(" ");
      setColors(client, "37;40"); client.print("|");
    }
    setCursor(client, br+bh-1, bc); client.print("+"); for(int i=0;i<bw-2;i++) client.print("-"); client.print("+");
  }

  // Модальное окно редактирования текста (TYPE_TEXT) в центре экрана
  if (isEditing) {
    MenuItem *it = &menuItems[activeItemIndex];
    int br = 8, bc = 15, bw = 50, bh = 6;
    drawRect(client, br, bc, bh, bw, "37;40");
    
    // Границы
    setColors(client, "37;40");
    setCursor(client, br, bc); client.print("+"); for(int i=0;i<bw-2;i++) client.print("-"); client.print("+");
    for(int r=1; r<bh-1; r++) {
      setCursor(client, br+r, bc); client.print("|");
      for(int c=0; c<bw-2; c++) client.print(" ");
      client.print("|");
    }
    setCursor(client, br+bh-1, bc); client.print("+"); for(int i=0;i<bw-2;i++) client.print("-"); client.print("+");

    // Заголовок и текущий ввод
    setCursor(client, br+1, bc+2); setColors(client, "33;40"); client.print("EDIT: "); client.print(it->label);
    setCursor(client, br+3, bc+2); setColors(client, "30;47");
    client.print(" "); client.print(inputBuffer); client.print("_");
    for(size_t p = inputBuffer.length() + 1; p < (size_t)(bw-6); p++) client.print(" ");
    
    setColors(client, "37;40");
    setCursor(client, br+4, bc+2);
    if (it->allowEmpty) client.print("[Enter] Confirm  [Esc] Cancel  (Can be empty)");
    else client.print("[Enter] Confirm  [Esc] Cancel  (Cannot be empty)");
  }

  // Нижняя информационная панель
  drawRect(client, 23, 1, 2, 80, "30;47");
  setCursor(client, 23, 3);
  if (isEditing) {
    client.print("EDITING TEXT: Type characters, press [Enter] to save or [Esc] to cancel.");
  } else if (isSubMenuOpen) {
    client.print("SUBMENU: [Up/Down] Select item, [Enter] Confirm choice, [Esc] Cancel.");
  } else {
    client.print("NAVIGATION: [Left/Right] Tabs, [Up/Down] Move, [Enter] Select/Edit.");
  }
  
  setCursor(client, 24, 3);
  client.print("STATUS: BIOS System Operational.                               ");
}

void GS32BIOS::handleInput(Stream &client) {
  char c = client.read();
  
  // Обработка клавиши ESC (код '\e' / 27)
  if (c == '\e') {
    delay(15);
    if (client.available() && client.read() == '[') {
      char a = client.read();
      if (isSubMenuOpen) {
        if (a == 'A') subMenuSelectionIndex = (subMenuSelectionIndex - 1 + menuItems[activeItemIndex].maxOptions) % menuItems[activeItemIndex].maxOptions;
        if (a == 'B') subMenuSelectionIndex = (subMenuSelectionIndex + 1) % menuItems[activeItemIndex].maxOptions;
        if (a == 'D') isSubMenuOpen = false;
        renderMenu(client);
      } else if (!isEditing) {
        if (a == 'A') moveActiveItem(-1);
        else if (a == 'B') moveActiveItem(1);
        else if (a == 'C') { currentPageIdx = (currentPageIdx + 1) % pages.size(); updateActiveIndex(); }
        else if (a == 'D') { currentPageIdx = (currentPageIdx - 1 + pages.size()) % pages.size(); updateActiveIndex(); }
        renderMenu(client);
      }
    } else { 
      // Одиночное нажатие ESC -> Отмена редактирования/подменю
      if (isEditing) {
        // Восстанавливаем оригинальное значение при отмене
        strncpy((char*)menuItems[activeItemIndex].valPtr, originalTextValue, menuItems[activeItemIndex].maxLen);
        isEditing = false;
        inputBuffer = "";
      }
      if (isSubMenuOpen) {
        isSubMenuOpen = false;
      }
      renderMenu(client); 
    }
    return;
  }

  // Логика ввода текста в модальном окне редактирования
  if (isEditing) {
    MenuItem *it = &menuItems[activeItemIndex];
    if (c == '\r' || c == '\n') { 
      if (inputBuffer.length() > 0 || it->allowEmpty) {
        strncpy((char*)it->valPtr, inputBuffer.c_str(), it->maxLen - 1);
        ((char*)it->valPtr)[it->maxLen - 1] = '\0';
      }
      isEditing = false; 
      inputBuffer = ""; 
      renderMenu(client);
    } else if (c == 8 || c == 127) { // Backspace
      if (inputBuffer.length() > 0) { 
        inputBuffer.remove(inputBuffer.length() - 1); 
        renderMenu(client); 
      }
    } else if (c >= 32 && c <= 126) {
      if (inputBuffer.length() < (it->maxLen - 1)) {
        inputBuffer += c; 
        renderMenu(client);
      }
    }
    return;
  }

  // Подтверждение выбора в выпадающем подменю
  if (isSubMenuOpen && (c == '\r' || c == '\n')) {
    *(int*)menuItems[activeItemIndex].valPtr = subMenuSelectionIndex;
    isSubMenuOpen = false;
    renderMenu(client); 
    return;
  }

  // Обработка нажатия Enter на стандартном пункте
  if (c == '\r' || c == '\n') {
    MenuItem *it = &menuItems[activeItemIndex];
    if (it->type == TYPE_BOOL) { 
      *(bool*)it->valPtr = !(*(bool*)it->valPtr); 
      renderMenu(client); 
    }
    else if (it->type == TYPE_SELECT) { 
      isSubMenuOpen = true; 
      subMenuSelectionIndex = *(int*)it->valPtr; 
      renderMenu(client); 
    }
    else if (it->type == TYPE_TEXT) { 
      isEditing = true; 
      // Заполняем поле изначальным текстом
      inputBuffer = String((char*)it->valPtr);
      // Сохраняем резервную копию на случай нажатия ESC
      strncpy(originalTextValue, (char*)it->valPtr, sizeof(originalTextValue));
      renderMenu(client); 
    }
    else if (it->type == TYPE_ACTION && it->action) {
      it->action();
    }
  }
}
