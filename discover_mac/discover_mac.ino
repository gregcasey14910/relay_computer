#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n========================================");
  Serial.println("ESP32 MAC Address & I2C Inventory");
  Serial.println("========================================");
  
  // Initialize I2C with proper delays (like Master2)
  Wire.begin(0, 1);  // SDA = GPIO0, SCL = GPIO1
  delay(100);
  
  // Second initialization for stability
  Wire.begin(0, 1);
  delay(100);
  
  // ===== MAC ADDRESS =====
  WiFi.mode(WIFI_STA);
  delay(100);
  
  String macStr = WiFi.macAddress();
  Serial.print("MAC Address: ");
  Serial.println(macStr);
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.print("Array format: {");
  for (int i = 0; i < 6; i++) {
    Serial.printf("0x%02X", mac[i]);
    if (i < 5) Serial.print(", ");
  }
  Serial.println("}");
  Serial.println();
  
  // ===== I2C SCANNER =====
  Serial.println("Scanning I2C bus (SDA=GPIO0, SCL=GPIO1)...");
  Serial.println("----------------------------------------");
  
  int deviceCount = 0;
  String i2cDevices = "";
  
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.printf("I2C device found at 0x%02X", addr);
      
      // Identify common devices
      if (addr == 0x20 || addr == 0x21 || addr == 0x27) {
        Serial.print(" (MCP23017/PCF8574)");
      } else if (addr == 0x3C || addr == 0x3D) {
        Serial.print(" (SSD1306 OLED)");
      }
      Serial.println();
      
      i2cDevices += String("0x") + String(addr, HEX) + " ";
      deviceCount++;
    }
    delay(5);
  }
  
  if (deviceCount == 0) {
    Serial.println("No I2C devices found!");
    i2cDevices = "None";
  } else {
    Serial.printf("\nTotal devices found: %d\n", deviceCount);
  }
  
  Serial.println("========================================\n");
  
  // ===== DISPLAY OUTPUT =====
  Serial.println("Attempting SSD1306 initialization...");
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("✗ SSD1306 allocation failed"));
  } else {
    Serial.println("✓ SSD1306 initialized");
    
    // Set maximum contrast
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(0xFF);  // Max contrast
    
    // Clear and test with simple pattern first
    display.clearDisplay();
    display.fillRect(0, 0, 128, 64, SSD1306_WHITE);  // Fill entire screen white
    display.display();
    delay(1000);
    Serial.println("✓ Screen should be WHITE now");
    
    // Now show the actual info
    display.clearDisplay();
    display.setTextSize(1);      
    display.setFont();
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    
    display.println("ESP32 Info:");
    display.println();
    
    display.print("MAC: ");
    display.println(macStr.substring(0, 8));
    display.print("     ");
    display.println(macStr.substring(9));
    display.println();
    
    display.print("I2C: ");
    if (deviceCount == 0) {
      display.println("None");
    } else {
      display.println(i2cDevices);
    }
    
    display.display();
    delay(10);
    Serial.println("✓ Info displayed on SSD1306");
  }
}

void loop() {
  // Nothing needed
}