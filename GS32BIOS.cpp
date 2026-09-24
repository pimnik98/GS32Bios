#include "GS32BIOS.h"

GS32BIOS::GS32BIOS()
    : headerTitle("GS-32"), productName("GS-32 Engine"), productVersion("v1.0.0"),
      currentPageIdx(0), activeItemIndex(0), localActiveIndex(0),
      isEditing(false), isSubMenuOpen(false), subMenuSelectionIndex(0),
      saveCallback(nullptr), resetCallback(nullptr), keyPressCallback(nullptr),
      originalIntValue(0), currentNavDepth(0), _isActive(false)
{
  theme = {"30;47", "37;44", "44;37", "30;47", "37;44", "30;47"};

  pages.push_back({"Info", "", false});
  pages.push_back({"Exit", "", false});
}

GS32BIOS::~GS32BIOS()
{
  for (auto &item : menuItems)
  {
    if (item.type == TYPE_INFO && item.valPtr && item.maxLen == 64)
    {
      delete[] static_cast<char *>(item.valPtr);
    }
  }
}

void GS32BIOS::enable()
{
  _isActive = true;
  Serial.print(F("\e[2J\e[H"));
  updateSystemStats();
  renderMenu(Serial);
}

void GS32BIOS::disable()
{
  _isActive = false;
  Serial.print(F("\e[?25h"));
  Serial.println(F("\n[GS-32 BIOS] Disabled. Serial console released."));
}

void GS32BIOS::setHeaderTitle(const String &title) { headerTitle = title; }

void GS32BIOS::setProductInfo(const String &name, const String &version)
{
  productName = name;
  productVersion = version;
}

void GS32BIOS::setTheme(const char *bgWork, const char *bgHeader,
                        const char *highlight, const char *tabActive,
                        const char *popupBg, const char *popupHighlight)
{
  theme = {bgWork, bgHeader, highlight, tabActive, popupBg, popupHighlight};
}

void GS32BIOS::addPage(const String &pageName, const String &parentPage)
{
  if (pageName == "Info" || pageName == "Exit")
    return;
  for (const auto &p : pages)
  {
    if (p.name == pageName)
      return;
  }
  pages.insert(pages.end() - 1, {pageName, parentPage, false});
}

void GS32BIOS::addSubMenuAction(const String &pageName, const String &label, const String &targetPageName)
{
  MenuItem item;
  item.page = pageName;
  item.label = label;
  item.type = TYPE_SUBMENU_LINK;
  item.targetPage = targetPageName;
  item.valPtr = nullptr;
  item.minVal = 0;
  item.maxVal = 0;
  item.maxOptions = 0;
  item.options = nullptr;
  item.dynamicOptionsFunc = nullptr;
  item.action = nullptr;
  item.maxLen = 0;
  item.allowEmpty = true;
  menuItems.push_back(item);
}

void GS32BIOS::addInfo(const String &label, const String &value)
{
  char *valStr = new char[64];
  strlcpy(valStr, value.c_str(), 64);
  MenuItem item;
  item.page = "Info";
  item.label = label;
  item.type = TYPE_INFO;
  item.valPtr = valStr;
  item.maxLen = 64;
  item.allowEmpty = true;
  menuItems.push_back(item);
}

void GS32BIOS::addText(const String &pageName, const String &label, char *valPtr, size_t maxLen, bool allowEmpty)
{
  MenuItem item;
  item.page = pageName;
  item.label = label;
  item.type = TYPE_TEXT;
  item.valPtr = valPtr;
  item.maxLen = maxLen;
  item.allowEmpty = allowEmpty;
  menuItems.push_back(item);
}

void GS32BIOS::addInt(const String &pageName, const String &label, int *valPtr, int minVal, int maxVal)
{
  MenuItem item;
  item.page = pageName;
  item.label = label;
  item.type = TYPE_INT;
  item.valPtr = valPtr;
  item.minVal = minVal;
  item.maxVal = maxVal;
  menuItems.push_back(item);
}

void GS32BIOS::addBool(const String &pageName, const String &label, bool *valPtr)
{
  MenuItem item;
  item.page = pageName;
  item.label = label;
  item.type = TYPE_BOOL;
  item.valPtr = valPtr;
  menuItems.push_back(item);
}

void GS32BIOS::addSelect(const String &pageName, const String &label, int *valPtr, int optionsCount, const char **options)
{
  MenuItem item;
  item.page = pageName;
  item.label = label;
  item.type = TYPE_SELECT;
  item.valPtr = valPtr;
  item.maxOptions = optionsCount;
  item.options = options;
  menuItems.push_back(item);
}

void GS32BIOS::addDynamicSelect(const String &pageName, const String &label, int *valPtr, std::function<std::vector<String>()> fetchOptionsFunc)
{
  MenuItem item;
  item.page = pageName;
  item.label = label;
  item.type = TYPE_DYNAMIC_SELECT;
  item.valPtr = valPtr;
  item.dynamicOptionsFunc = fetchOptionsFunc;
  menuItems.push_back(item);
}

void GS32BIOS::addAction(const String &pageName, const String &label, std::function<void()> action)
{
  MenuItem item;
  item.page = pageName;
  item.label = label;
  item.type = TYPE_ACTION;
  item.action = action;
  menuItems.push_back(item);
}

void GS32BIOS::onSave(std::function<void()> callback) { saveCallback = callback; }
void GS32BIOS::onFactoryReset(std::function<void()> callback) { resetCallback = callback; }
void GS32BIOS::onKeyPress(std::function<void(char, const char *)> callback) { keyPressCallback = callback; }

void GS32BIOS::begin()
{
  Serial.println(F("\n=================================================================="));
  Serial.println(F(" [!] WARNING: use PuTTY/TeraTerm or another ANSI/VT100 terminal."));
  Serial.println(F("==================================================================\n"));
  delay(1000);

  snprintf(sys_chip_model, sizeof(sys_chip_model), "%s Rev.%d", ESP.getChipModel(), ESP.getChipRevision());
  snprintf(sys_flash_size, sizeof(sys_flash_size), "%d MB", (int)(ESP.getFlashChipSize() / (1024 * 1024)));

  addAction("Exit", "Save Changes and Reset", [this]()
            { if (saveCallback) saveCallback(); ESP.restart(); });
  addAction("Exit", "Discard Changes and Exit", []()
            { ESP.restart(); });
  addAction("Exit", "Factory Reset", [this]()
            { if (resetCallback) resetCallback(); ESP.restart(); });

  std::vector<MenuItem> baseInfo;
  auto createSysInfo = [](const char *label, char *value)
  {
    MenuItem item;
    item.page = "Info";
    item.label = label;
    item.type = TYPE_INFO;
    item.valPtr = value;
    return item;
  };
  baseInfo.push_back(createSysInfo("Product Name", (char *)productName.c_str()));
  baseInfo.push_back(createSysInfo("Version", (char *)productVersion.c_str()));
  baseInfo.push_back(createSysInfo("Core Architecture", sys_chip_model));
  baseInfo.push_back(createSysInfo("Flash Memory Size", sys_flash_size));
  baseInfo.push_back(createSysInfo("System Free RAM", sys_free_ram));
  baseInfo.push_back(createSysInfo("System Uptime", sys_uptime));
  menuItems.insert(menuItems.begin(), baseInfo.begin(), baseInfo.end());

  enable();
}

void GS32BIOS::handle()
{
  if (_isActive && Serial.available())
  {
    handleInput(Serial);
  }
}

void GS32BIOS::updateSystemStats()
{
  snprintf(sys_free_ram, sizeof(sys_free_ram), "%d KB", (int)(ESP.getFreeHeap() / 1024));
  unsigned long sec = millis() / 1000;
  snprintf(sys_uptime, sizeof(sys_uptime), "%02d:%02d:%02d", (int)(sec / 3600), (int)((sec % 3600) / 60), (int)(sec % 60));
}

std::vector<GS32BIOS::PageNode> GS32BIOS::getCurrentLevelPages()
{
  std::vector<PageNode> result;
  String parent = (currentNavDepth == 0) ? "" : activeSubmenuPath[currentNavDepth - 1];
  for (const auto &p : pages)
  {
    if (p.parent == parent)
      result.push_back(p);
  }
  return result;
}

void GS32BIOS::updateActiveIndex()
{
  auto level = getCurrentLevelPages();
  if (level.empty())
    return;
  if (currentPageIdx < 0 || currentPageIdx >= (int)level.size())
    currentPageIdx = 0;

  String pageName = level[currentPageIdx].name;
  for (size_t i = 0; i < menuItems.size(); ++i)
  {
    if (menuItems[i].page == pageName)
    {
      activeItemIndex = (int)i;
      break;
    }
  }
}

void GS32BIOS::moveActiveItem(int dir)
{
  auto level = getCurrentLevelPages();
  if (level.empty() || currentPageIdx < 0 || currentPageIdx >= (int)level.size())
    return;

  String pageName = level[currentPageIdx].name;
  std::vector<size_t> indices;
  for (size_t i = 0; i < menuItems.size(); ++i)
  {
    if (menuItems[i].page == pageName)
      indices.push_back(i);
  }
  if (indices.empty())
    return;

  localActiveIndex = (localActiveIndex + dir + (int)indices.size()) % (int)indices.size();
  activeItemIndex = (int)indices[localActiveIndex];
}

void GS32BIOS::setCursor(Stream &c, int r, int col)
{
  c.print(F("\e["));
  c.print(r);
  c.print(';');
  c.print(col);
  c.print('H');
}

void GS32BIOS::setColors(Stream &c, const char *code)
{
  c.print(F("\e["));
  c.print(code);
  c.print('m');
}

void GS32BIOS::drawRect(Stream &c, int sr, int sc, int h, int w, const char *clr)
{
  setColors(c, clr);
  for (int r = 0; r < h; ++r)
  {
    setCursor(c, sr + r, sc);
    for (int i = 0; i < w; ++i)
      c.print(' ');
  }
}

void GS32BIOS::drawBoxBorder(Stream &client, int br, int bc, int bw, int bh)
{
  setCursor(client, br, bc);
  client.print('+');
  for (int i = 0; i < bw - 2; i++)
    client.print('-');
  client.print('+');

  for (int i = 1; i < bh - 1; i++)
  {
    setCursor(client, br + i, bc);
    client.print('|');
    setCursor(client, br + i, bc + bw - 1);
    client.print('|');
  }

  setCursor(client, br + bh - 1, bc);
  client.print('+');
  for (int i = 0; i < bw - 2; i++)
    client.print('-');
  client.print('+');
}

void GS32BIOS::renderMenu(Stream &client)
{
  client.print(F("\e[?25l"));

  drawRect(client, 2, 1, 22, 80, theme.bgWork);
  drawRect(client, 1, 1, 1, 80, theme.bgHeader);
  setCursor(client, 1, 3);
  client.print(headerTitle);

  auto level = getCurrentLevelPages();
  if (currentNavDepth > 0)
  {
    setColors(client, theme.bgHeader);
    setCursor(client, 1, 14);
    client.print(F("> "));
    if (!level.empty() && currentPageIdx >= 0 && currentPageIdx < (int)level.size())
    {
      client.print(level[currentPageIdx].name);
    }
  }
  else
  {
    int col = 14;
    for (size_t i = 0; i < level.size(); ++i)
    {
      setCursor(client, 1, col);
      setColors(client, ((int)i == currentPageIdx) ? theme.tabActive : theme.bgHeader);
      client.print(' ');
      client.print(level[i].name);
      client.print(' ');
      col += level[i].name.length() + 3;
    }
  }

  int row = 3;
  if (!level.empty() && currentPageIdx >= 0 && currentPageIdx < (int)level.size())
  {
    String pageName = level[currentPageIdx].name;
    for (size_t i = 0; i < menuItems.size(); ++i)
    {
      if (menuItems[i].page != pageName)
        continue;
      setCursor(client, row++, 2);
      setColors(client, ((int)i == activeItemIndex) ? theme.highlight : theme.bgWork);

      char value[44] = "";
      const auto &item = menuItems[i];
      if (item.type == TYPE_INFO || item.type == TYPE_TEXT)
      {
        snprintf(value, sizeof(value), "%s", (char *)item.valPtr);
      }
      else if (item.type == TYPE_INT)
      {
        snprintf(value, sizeof(value), "%d", *(int *)item.valPtr);
      }
      else if (item.type == TYPE_BOOL)
      {
        snprintf(value, sizeof(value), "%s", *(bool *)item.valPtr ? "[Enabled]" : "[Disabled]");
      }
      else if (item.type == TYPE_SELECT || item.type == TYPE_DYNAMIC_SELECT)
      {
        int sel = *(int *)item.valPtr;
        String optStr = (item.type == TYPE_SELECT) ? item.options[sel] : item.dynamicOptionsFunc()[sel];
        snprintf(value, sizeof(value), "[ %s ]", optStr.c_str());
      }
      else if (item.type == TYPE_SUBMENU_LINK)
      {
        snprintf(value, sizeof(value), ">>");
      }

      char line[81];
      if (item.type == TYPE_ACTION || item.type == TYPE_SUBMENU_LINK)
      {
        snprintf(line, sizeof(line), "   %-30s %-45s", item.label.c_str(), value);
      }
      else
      {
        snprintf(line, sizeof(line), "   %-28s : %-43s", item.label.c_str(), value);
      }
      client.print(line);
    }
  }

  if (isSubMenuOpen && activeItemIndex >= 0 && activeItemIndex < (int)menuItems.size())
  {
    const auto &item = menuItems[activeItemIndex];
    std::vector<String> dynOpts;
    int count = item.maxOptions;
    if (item.type == TYPE_DYNAMIC_SELECT)
    {
      dynOpts = item.dynamicOptionsFunc();
      count = (int)dynOpts.size();
    }
    if (count <= 0)
      count = 1;

    int bh = count + 2, br = 5, bc = 22, bw = 36;
    drawRect(client, br, bc, bh, bw, theme.popupBg);
    drawBoxBorder(client, br, bc, bw, bh);

    for (int i = 0; i < count; i++)
    {
      setCursor(client, br + 1 + i, bc + 1);
      setColors(client, (i == subMenuSelectionIndex) ? theme.popupHighlight : theme.popupBg);
      String n = (item.type == TYPE_SELECT && item.options) ? item.options[i] : (!dynOpts.empty() ? dynOpts[i] : "No items");

      client.print(' ');
      client.print(n);
      for (size_t p = n.length(); p < (size_t)(bw - 4); p++)
        client.print(' ');
    }
  }

  if (isEditing && activeItemIndex >= 0 && activeItemIndex < (int)menuItems.size())
  {
    const auto &item = menuItems[activeItemIndex];
    int br = 8, bc = 15, bw = 50, bh = 6;
    drawRect(client, br, bc, bh, bw, theme.popupBg);
    drawBoxBorder(client, br, bc, bw, bh);

    setCursor(client, br + 1, bc + 2);
    client.print(F("EDIT: "));
    client.print(item.label);
    setCursor(client, br + 3, bc + 2);
    setColors(client, theme.popupHighlight);
    client.print(' ');
    client.print(inputBuffer);
    client.print('_');
    for (size_t p = inputBuffer.length() + 1; p < (size_t)(bw - 6); p++)
      client.print(' ');

    if (item.type == TYPE_INT)
    {
      setCursor(client, br + 4, bc + 2);
      setColors(client, theme.popupBg);
      client.print(F("(range: "));
      client.print(item.minVal);
      client.print('-');
      client.print(item.maxVal);
      client.print(')');
    }
  }

  drawRect(client, 24, 1, 1, 80, theme.bgHeader);
  setCursor(client, 24, 3);
  client.print(F("NAV: [Arrows] Move, [Enter] Select, [Esc] Back."));
}

void GS32BIOS::handleInput(Stream &client)
{
  char c = client.read();

  if (keyPressCallback)
  {
    auto l = getCurrentLevelPages();
    const char *pageName = (!l.empty() && currentPageIdx >= 0 && currentPageIdx < (int)l.size()) ? l[currentPageIdx].name.c_str() : "";
    keyPressCallback(c, pageName);
  }

  if (c == '\e')
  {
    delay(15);
    if (client.available() && client.read() == '[')
    {
      char code = client.read();
      if (isSubMenuOpen)
      {
        const auto &it = menuItems[activeItemIndex];
        int count = (it.type == TYPE_DYNAMIC_SELECT) ? (int)it.dynamicOptionsFunc().size() : it.maxOptions;
        if (count <= 0)
          count = 1;

        if (code == 'A')
          subMenuSelectionIndex = (subMenuSelectionIndex - 1 + count) % count;
        else if (code == 'B')
          subMenuSelectionIndex = (subMenuSelectionIndex + 1) % count;
        else if (code == 'D')
          isSubMenuOpen = false;
        renderMenu(client);
      }
      else if (!isEditing)
      {
        auto level = getCurrentLevelPages();
        if (level.empty())
          return;

        if (code == 'A' || code == 'B')
        {
          moveActiveItem(code == 'A' ? -1 : 1);
          renderMenu(client);
        }
        else if (currentNavDepth == 0 && (code == 'C' || code == 'D'))
        {
          currentPageIdx = (currentPageIdx + (code == 'C' ? 1 : -1) + level.size()) % level.size();
          updateActiveIndex();
          renderMenu(client);
        }
      }
    }
    else
    {
      while (client.available() > 0)
        client.read();

      if (isEditing)
      {
        auto &it = menuItems[activeItemIndex];
        if (it.type == TYPE_TEXT)
          strncpy((char *)it.valPtr, originalTextValue, it.maxLen);
        else
          *(int *)it.valPtr = originalIntValue;
        isEditing = false;
      }
      else if (isSubMenuOpen)
      {
        isSubMenuOpen = false;
      }
      else if (currentNavDepth > 0)
      {
        activeSubmenuPath.pop_back();
        currentNavDepth--;
        currentPageIdx = 0;
        updateActiveIndex();
      }
      renderMenu(client);
    }
    return;
  }

  if (isEditing)
  {
    auto &it = menuItems[activeItemIndex];
    if (c == '\r' || c == '\n')
    {
      if (it.type == TYPE_INT)
      {
        int v = inputBuffer.toInt();
        if (v >= it.minVal && v <= it.maxVal)
          *(int *)it.valPtr = v;
      }
      else if (inputLevelCheckEmpty(&it, inputBuffer))
      {
        strlcpy((char *)it.valPtr, inputBuffer.c_str(), it.maxLen);
      }
      isEditing = false;
      renderMenu(client);
    }
    else if (c == 8 || c == 127)
    {
      if (inputBuffer.length() > 0)
      {
        inputBuffer.remove(inputBuffer.length() - 1);
        renderMenu(client);
      }
    }
    else if (c >= 32 && c <= 126)
    {
      inputBuffer += c;
      renderMenu(client);
    }
    return;
  }

  if (isSubMenuOpen && (c == '\r' || c == '\n'))
  {
    *(int *)menuItems[activeItemIndex].valPtr = subMenuSelectionIndex;
    isSubMenuOpen = false;
    renderMenu(client);
    return;
  }

  if (c == '\r' || c == '\n')
  {
    if (activeItemIndex < 0 || activeItemIndex >= (int)menuItems.size())
      return;
    auto &it = menuItems[activeItemIndex];

    if (it.type == TYPE_SUBMENU_LINK)
    {
      activeSubmenuPath.push_back(getCurrentLevelPages()[currentPageIdx].name);
      currentNavDepth++;
      auto ch = getCurrentLevelPages();
      currentPageIdx = 0;
      for (size_t i = 0; i < ch.size(); i++)
      {
        if (ch[i].name == it.targetPage)
        {
          currentPageIdx = (int)i;
          break;
        }
      }
      updateActiveIndex();
      renderMenu(client);
    }
    else if (it.type == TYPE_BOOL)
    {
      *(bool *)it.valPtr = !(*(bool *)it.valPtr);
      renderMenu(client);
    }
    else if (it.type == TYPE_SELECT || it.type == TYPE_DYNAMIC_SELECT)
    {
      isSubMenuOpen = true;
      subMenuSelectionIndex = *(int *)it.valPtr;
      renderMenu(client);
    }
    else if (it.type == TYPE_TEXT || it.type == TYPE_INT)
    {
      isEditing = true;
      if (it.type == TYPE_TEXT)
      {
        inputBuffer = (char *)it.valPtr;
        strlcpy(originalTextValue, (char *)it.valPtr, sizeof(originalTextValue));
      }
      else
      {
        inputBuffer = String(*(int *)it.valPtr);
        originalIntValue = *(int *)it.valPtr;
      }
      renderMenu(client);
    }
    else if (it.type == TYPE_ACTION)
    {
      it.action();
    }
  }
}

bool GS32BIOS::inputLevelCheckEmpty(MenuItem *it, const String &buf)
{
  return buf.length() > 0 || it->allowEmpty;
}
