# GS32BIOS

**GS32BIOS** — это библиотека для ESP32 (Arduino IDE), которая позволяет создать профессиональный текстовый интерфейс (BIOS) прямо в терминале. Она идеально подходит для настройки устройств через Serial-порт без необходимости создания веб-интерфейса или использования дисплеев.

![Интерфейс](https://dev.piminoff.ru/img/github/UI-Terminal-blue.svg) ![Платформа](https://dev.piminoff.ru/img/github/Platform-ESP32-orange.svg)

## Скриншоты
![ГлавноеОкно](https://dev.piminoff.ru/img/github/GS32Bios/1.png)
![Редактирование](https://dev.piminoff.ru/img/github/GS32Bios/2.png)
![Выбор](https://dev.piminoff.ru/img/github/GS32Bios/3.png)

## ✨ Особенности

*   **Полноценный UI в терминале:** Использование ANSI-последовательностей для отрисовки рамок, цветов и навигации.
*   **Система вкладок:** Разделение настроек по страницам (например, Info, Wireless, Advanced).
*   **Типы данных:**
    *   `TYPE_INFO` — вывод статической информации.
    *   `TYPE_TEXT` — редактирование строк с модальным окном ввода.
    *   `TYPE_INT` — редактирование целых чисел.
    *   `TYPE_BOOL` — переключатели [Enabled/Disabled].
    *   `TYPE_SELECT` — выпадающие списки выбора.
    *   `TYPE_ACTION` — кнопки для выполнения функций (Save, Reset, etc).
*   **Автоматическая страница Info:** Отображение модели чипа, объема Flash, свободной ОЗУ и времени работы (Uptime).
*   **Гибкий брендинг:** Настройка заголовка шапки, названия продукта и версии.
*   **Система колбэков:** Легкая интеграция с `Preferences.h` для сохранения настроек.
*   **Поддержка Esc:** Отмена редактирования или закрытие подменю.

## 🚀 Установка

1.  Скачайте файлы `GS32BIOS.h` и `GS32BIOS.cpp`.
2.  Создайте папку `GS32BIOS` в вашей директории библиотек Arduino (обычно `Documents/Arduino/libraries/`).
3.  Поместите скачанные файлы в созданную папку.
4.  Перезапустите Arduino IDE.

## ⌨️ Навигация

| Клавиша | Действие |
| :--- | :--- |
| **Стрелки влево/вправо** | Переключение между вкладками (страницами) |
| **Стрелки вверх/вниз** | Перемещение по пунктам меню |
| **Enter** | Редактирование параметра или выполнение действия |
| **Esc** | Отмена ввода текста или закрытие списка выбора |
| **Backspace** | Удаление символа при вводе текста |

## 📖 Быстрый старт

```cpp
#include <GS32BIOS.h>

GS32BIOS bios;

// Ваши переменные
char device_name[32] = "Smart-Node-01";
bool led_enabled = true;
int operation_mode = 0;
const char* modes[] = {"Normal", "Turbo", "Eco"};

void setup() {
  Serial.begin(115200);

  // 1. Настройка внешнего вида
  bios.setHeaderTitle("GS32");
  bios.setProductInfo("IoT Controller", "v1.0.2");

  // 2. Инициализация
  bios.begin();

  // 3. Создание страниц
  bios.addPage("General");

  // 4. Добавление настроек
  bios.addText("General", "Device Name", device_name, sizeof(device_name), false);
  bios.addBool("General", "Status LED", &led_enabled);
  bios.addSelect("General", "Power Mode", &operation_mode, 3, modes);

  // 5. Настройка действий (например, сохранение)
  bios.onSave([]() {
    Serial.println("Сохранение настроек в память...");
    // Тут ваш код для Preferences.h
  });
}

void loop() {
  bios.handle(); // Обработка интерфейса
}
```

## 🛠 Методы API

### Инициализация и метаданные
*   `void begin()` — запуск движка BIOS.
*   `void setHeaderTitle(String title)` — текст в левом верхнем углу (шапка).
*   `void setProductInfo(String name, String version)` — данные для вкладки Info.

### Создание контента
*   `void addPage(String name)` — создание новой вкладки.
*   `void addInfo(String label, String value)` — статичная строка на вкладке Info.
*   `void addText(String page, String label, char* ptr, size_t len, bool allowEmpty)` — текстовое поле.
*   `void addBool(String page, String label, bool* ptr)` — логический переключатель.
*   `void addSelect(String page, String label, int* ptr, int count, const char** opts)` — список выбора.
*   `void addAction(String page, String label, function action)` — кнопка-действие.

### Колбэки
*   `void onSave(function cb)` — выполняется при нажатии "Save Changes and Reset".
*   `void onFactoryReset(function cb)` — выполняется при нажатии "Factory Reset".

## 📝 Требования
*   Плата: **ESP32** (любая версия).
*   Терминал: Любой с поддержкой **ANSI/VT100** (Putty, TeraTerm, встроенный монитор порта VS Code/PlatformIO). 
    *   *Примечание: Стандартный монитор порта Arduino IDE до версии 2.0 плохо поддерживает ANSI-цвета.*

## Лицензия
MIT Лицензия. Используйте в любых проектах!
