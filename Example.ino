#include <GS32BIOS.h>
#include <Preferences.h>

GS32BIOS bios;
Preferences prefs;

// Переменные конфигурации
int  cfg_wifi_mode_idx     = 0; 
char cfg_wifi_ssid[32]     = "My_Home_WiFi";
char cfg_wifi_pass[32]     = "secretpassword";
bool cfg_debug_mode        = true;

const char* wifi_mode_options[] = { "WiFi Client (STA)", "Access Point (AP)" };

void setup() {
  Serial.begin(115200);

  // 1. Загрузка настроек из Preferences
  prefs.begin("gs32_config", true);
  cfg_wifi_mode_idx = prefs.getInt("wf_mode", 0);
  prefs.getString("wf_ssid", "My_Home_WiFi").toCharArray(cfg_wifi_ssid, sizeof(cfg_wifi_ssid));
  prefs.getString("wf_pass", "secretpassword").toCharArray(cfg_wifi_pass, sizeof(cfg_wifi_pass));
  cfg_debug_mode = prefs.getBool("dbg", true);
  prefs.end();

  // 2. Брендинг
  bios.setHeaderTitle("MY-DEVICE");
  bios.setProductInfo("Smart Controller", "v1.2.0");

  // 3. Инициализация UI
  bios.begin();

  // 4. Добавление страниц и пунктов
  bios.addPage("Wireless");
  bios.addPage("Advanced");

  bios.addSelect("Wireless", "Wireless Mode", &cfg_wifi_mode_idx, 2, wifi_mode_options);
  bios.addText("Wireless", "Client SSID", cfg_wifi_ssid, sizeof(cfg_wifi_ssid), true);
  bios.addText("Wireless", "Client Password", cfg_wifi_pass, sizeof(cfg_wifi_pass), false);

  bios.addBool("Advanced", "Hardware Debug Log", &cfg_debug_mode);

  // 5. Обработчики
  bios.onSave([]() {
    prefs.begin("gs32_config", false);
    prefs.putInt("wf_mode", cfg_wifi_mode_idx);
    prefs.putString("wf_ssid", cfg_wifi_ssid);
    prefs.putString("wf_pass", cfg_wifi_pass);
    prefs.putBool("dbg", cfg_debug_mode);
    prefs.end();
    Serial.println("Saved!");
  });

  bios.onFactoryReset([]() {
    prefs.begin("gs32_config", false);
    prefs.clear();
    prefs.end();
    Serial.println("Factory reset done!");
  });
}

void loop() {
  bios.handle(); // Обработка UI и терминала
}
