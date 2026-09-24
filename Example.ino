#include <Arduino.h>
#include "GS32BIOS.h"

GS32BIOS bios;

// Переменные для примера
char ssidBuffer[32] = "MyHomeWiFi";
int channelNum = 6;
bool dhcpEnabled = true;
int selectedAuthMode = 0;
const char* authModes[] = {"Open", "WPA2-PSK", "WPA3-SAE", "WEE"};

int selectedWifiNetwork = 0;
bool customShellMode = false; // Флаг для демонстрации переключения на кастомный Shell

// Демо-функция для динамического списка (сканирование сетей "на лету")
std::vector<String> scanAvailableNetworks() {
  std::vector<String> networks;
  networks.push_back("Home_Net_5G (-45dBm)");
  networks.push_back("Guest_WiFi (-62dBm)");
  networks.push_back("IoT_Smart_House (-70dBm)");
  networks.push_back("Neighbor_Network (-85dBm)");
  return networks;
}

void setup() {
  Serial.begin(115200);
  while(!Serial); // Ожидание подключения Serial в PuTTY

  // 1. Настройка заголовков бренда
  bios.setHeaderTitle("GS-32 PRO");
  bios.setProductInfo("IoT Controller", "v2.5.1");

  // 2. Настройка темы оформления (6 параметров: bgWork, bgHeader, highlight, tabActive, popupBg, popupHighlight)
  //bios.setTheme("37;44", "30;47", "30;47", "30;46", "37;40", "30;47");

  // 3. Создание страниц и иерархии (родительские страницы)
  bios.addPage("Network");                  // Страница верхнего уровня
  bios.addPage("Settings");                 // Страница верхнего уровня
  bios.addPage("Tools");                    // Страница с действиями и переключением на Shell

  // Страницы подменю (указываем родителя вторым параметром)
  bios.addPage("Wi-Fi", "Network");         // Network -> Wi-Fi
  bios.addPage("Ethernet", "Network");      // Network -> Ethernet

  // 4. Добавление пунктов-ссылок для перехода в подменю по Enter
  bios.addSubMenuAction("Network", "Configure Wi-Fi...     ", "Wi-Fi");
  bios.addSubMenuAction("Network", "Configure Ethernet...  ", "Ethernet");

  // 5. Наполнение пунктов меню по страницам
  // Страница: Settings
  bios.addBool("Settings", "Enable MQTT Logs  ", &dhcpEnabled);
  bios.addInt("Settings", "Heartbeat Interval", &channelNum, 1, 60);
  bios.addText("Settings", "Device Hostname   ", ssidBuffer, 32, false);

  // Страница: Wi-Fi (внутри Network)
  bios.addText("Wi-Fi", "SSID Name         ", ssidBuffer, 32, false);
  bios.addSelect("Wi-Fi", "Security Mode     ", &selectedAuthMode, 4, authModes);
  bios.addDynamicSelect("Wi-Fi", "Select AP (Scan)  ", &selectedWifiNetwork, scanAvailableNetworks);

  // Страница: Ethernet (внутри Network)
  bios.addBool("Ethernet", "Use DHCP Client   ", &dhcpEnabled);
  bios.addInt("Ethernet", "Static IP Octet   ", &channelNum, 1, 255);

  // Страница: Tools (Демонстрация disable / enable для Shell)
  bios.addAction("Tools", "Switch to Custom Shell  ", []() {
    customShellMode = true;
    bios.disable(); // Отключаем BIOS, освобождаем Serial и возвращаем видимый курсор
    Serial.println("\n--- GS-32 Custom Shell Mode ---");
    Serial.println("Type 'help' for commands, or type 'bios' to return.");
    Serial.print("shell> ");
  });

  // 6. Обработчики
  bios.onSave([]() {
    Serial.println("\n[CALLBACK] User saved settings! Writing to EEPROM/SPIFFS...");
  });

  bios.onFactoryReset([]() {
    Serial.println("\n[CALLBACK] Factory reset triggered!");
  });

  bios.onKeyPress([](char key, const char* activePage) {
    if (key == 'h' || key == 'H') {
      Serial.printf("\n[HELP] Pressed 'H' on page: %s\n", activePage);
    }
  });

  // Запуск системы
  bios.begin();
}

void loop() {
  // Если активирован режим кастомного Shell, обрабатываем команды вручную
  if (customShellMode) {
    if (Serial.available()) {
      String cmd = Serial.readStringUntil('\n');
      cmd.trim();
      
      if (cmd.equalsIgnoreCase("bios")) {
        customShellMode = false;
        bios.enable(); // Возвращаем управление BIOS, очищаем экран и скрываем курсор
      } else if (cmd.equalsIgnoreCase("help")) {
        Serial.println("Available commands: help, uptime, bios");
      } else if (cmd.equalsIgnoreCase("uptime")) {
        Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
      } else if (cmd.length() > 0) {
        Serial.printf("Unknown command: '%s'. Type 'bios' to return to BIOS.\n", cmd.c_str());
      }
      
      if (customShellMode) {
        Serial.print("shell> ");
      }
    }
    return;
  }

  // Обязательный вызов обработчика BIOS в штатном режиме
  bios.handle();
}
