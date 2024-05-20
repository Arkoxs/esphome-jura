#include "esphome.h"

class JuraCoffee : public PollingComponent, public UARTDevice {
 Sensor *xsensor0 {nullptr};
 Sensor *xsensor1 {nullptr};
 Sensor *xsensor2 {nullptr};
 Sensor *xsensor3 {nullptr};
 Sensor *xsensor4 {nullptr};
 TextSensor *xsensor5 {nullptr};
 TextSensor *xsensor6 {nullptr};
 TextSensor *xsensor7 {nullptr};
 TextSensor *xsensor8 {nullptr};
 TextSensor *xsensor9 {nullptr};

 public:
  JuraCoffee(UARTComponent *parent, Sensor *sensor0, Sensor *sensor1, Sensor *sensor2, Sensor *sensor3, Sensor *sensor4, TextSensor *sensor5, TextSensor *sensor6, TextSensor *sensor7, TextSensor *sensor8, TextSensor *sensor9) : UARTDevice(parent) , xsensor0(sensor0) , xsensor1(sensor1) , xsensor2(sensor2) , xsensor3(sensor3) , xsensor4(sensor4) , xsensor5(sensor5) , xsensor6(sensor6), xsensor7(sensor7), xsensor8(sensor8), xsensor9(sensor9) {} 

  long num_single_espresso, num_double_espresso, num_coffee, num_double_coffee, num_clean;
  std::string tray_status, tank_status, heat_status, beans_status, rinse_status; 

  // Jura communication function taken in entirety from cmd2jura.ino, found at https://github.com/hn/jura-coffee-machine
  String cmd2jura(String outbytes) {
    String inbytes;
    int w = 0;

    while (available()) {
      read();
    }

    outbytes += "\r\n";
    for (int i = 0; i < outbytes.length(); i++) {
      for (int s = 0; s < 8; s += 2) {
        char rawbyte = 255;
        bitWrite(rawbyte, 2, bitRead(outbytes.charAt(i), s + 0));
        bitWrite(rawbyte, 5, bitRead(outbytes.charAt(i), s + 1));
        write(rawbyte);
      }
      delay(8);
    }

    int s = 0;
    char inbyte;
    while (!inbytes.endsWith("\r\n")) {
      if (available()) {
        byte rawbyte = read();
        bitWrite(inbyte, s + 0, bitRead(rawbyte, 2));
        bitWrite(inbyte, s + 1, bitRead(rawbyte, 5));
        if ((s += 2) >= 8) {
          s = 0;
          inbytes += inbyte;
        }
      } else {
        delay(10);
      }
      if (w++ > 500) {
        return "";
      }
    }

    return inbytes.substring(0, inbytes.length() - 2);
  }

  void setup() override {
    this->set_update_interval(10000); // 600000 = 10 minutes // Now 10 seconds
  }

  void loop() override {
  }

  void update() override {
    String result, hexString, substring;
    byte hex_to_byte;
    int trayBit, tankBit, heatBit, rinseBit, beansBit; 

    // For Testing
    int read_bit0,read_bit1,read_bit2,read_bit3,read_bit4,read_bit5,read_bit6,read_bit7;

    // For Jura Impressa C5

    // Fetch our line of EEPROM
    result = cmd2jura("RE:15");
    // Coffees made
    substring = result.substring(3,7);
    num_coffee = strtol(substring.c_str(),NULL,16);
    //ESP_LOGD("main", "Single Coffee result: %s, %s, %d", result.c_str(), substring.c_str(), num_coffee);

    // Fetch our line of EEPROM
    result = cmd2jura("RT:E000");
    // Get Single Espressos made
    substring = result.substring(3,7);
    num_single_espresso = strtol(substring.c_str(),NULL,16);
    //ESP_LOGD("main", "Single Espresso result: %s, %s, %d", result.c_str(), substring.c_str(), num_single_espresso);
    // Double Espressos made
    substring = result.substring(7,11);
    num_double_espresso = strtol(substring.c_str(),NULL,16);
    //ESP_LOGD("main", "Double Espresso result: %s, %s, %d", result.c_str(), substring.c_str(), num_double_espresso);
    // Double Coffees made
    result = cmd2jura("RE:36");
    substring = result.substring(3,7);
    num_double_coffee = strtol(substring.c_str(),NULL,16);
    //ESP_LOGD("main", "Double coffee result: %s, %s, %d", result.c_str(), substring.c_str(), num_double_coffee);

    // Fetch our line of EEPROM
    result = cmd2jura("RT:0000");
    // Cleanings done
    substring = result.substring(59,63);
    num_clean = 16 - strtol(substring.c_str(),NULL,16);
    //ESP_LOGD("main", "Count until cleaning result: %s, 16 - %s, %d", result.c_str(), substring.c_str(), num_clean);

    // Tray & water tank status
    // Fetch our line of EEPROM
    // Much gratitude to https://www.instructables.com/id/IoT-Enabled-Coffee-Machine/ for figuring out these bits are stored
    result = cmd2jura("IC:");
    hexString = result.substring(3,5);
    hex_to_byte = strtol(hexString.c_str(),NULL,16);

    // For Testing
    read_bit0 = bitRead(hex_to_byte, 0);  // Out of Beans
    read_bit1 = bitRead(hex_to_byte, 1);  // Boiler heated
    read_bit2 = bitRead(hex_to_byte, 2);  //
    read_bit3 = bitRead(hex_to_byte, 3);  // Water Tank
    read_bit4 = bitRead(hex_to_byte, 4);  // Waste Tray
    read_bit5 = bitRead(hex_to_byte, 5);  // 
    read_bit6 = bitRead(hex_to_byte, 6);  //
    read_bit7 = bitRead(hex_to_byte, 7);  // Press rotary to rinse
    ESP_LOGD("main", "Raw IC result: IC:hh 7654 3210");
    ESP_LOGD("main", "Raw IC result: %s %d%d%d%d %d%d%d%d", result.c_str(),read_bit7,read_bit6,read_bit5,read_bit4,read_bit3,read_bit2,read_bit1,read_bit0);
    // End Testing

    trayBit   = bitRead(hex_to_byte, 4);
    tankBit   = bitRead(hex_to_byte, 3);
    heatBit   = bitRead(hex_to_byte, 1);
    beansBit  = bitRead(hex_to_byte, 0);
    rinseBit  = bitRead(hex_to_byte, 7);

    //ESP_LOGD("main", "Status bits: Tray (%d) Tank (%d) Heat (%d) Beans (%d) Rinse (%d)", trayBit, tankBit, heatBit, beansBit, rinseBit);

    if (trayBit == 1)  { tray_status  = "Present"; } else { tray_status  = "Missing"; }
    if (tankBit == 1)  { tank_status  = "Fill"; }    else { tank_status  = "OK"; }
    if (heatBit == 1)  { heat_status  = "OK"; }      else { heat_status  = "Heating"; }
    if (beansBit == 1) { beans_status = "OK"; }    else { beans_status = "Fill"; }
    if (rinseBit == 1) { rinse_status = "OK"; }      else { rinse_status = "Press rotary"; }


    if (xsensor0 != nullptr)   xsensor0->publish_state(num_single_espresso);
    if (xsensor1 != nullptr)   xsensor1->publish_state(num_double_espresso);
    if (xsensor2 != nullptr)   xsensor2->publish_state(num_coffee);
    if (xsensor3 != nullptr)   xsensor3->publish_state(num_double_coffee);
    if (xsensor4 != nullptr)   xsensor4->publish_state(num_clean);
    if (xsensor5 != nullptr)   xsensor5->publish_state(tray_status);
    if (xsensor6 != nullptr)   xsensor6->publish_state(tank_status);
    if (xsensor7 != nullptr)   xsensor7->publish_state(heat_status);
    if (xsensor8 != nullptr)   xsensor8->publish_state(beans_status);
    if (xsensor9 != nullptr)   xsensor9->publish_state(rinse_status);

    // Debug code read EEPROM
    //result = cmd2jura("RT:0000");
    //ESP_LOGD("main", "Raw RT: 0000 result: %s", result.c_str());
    //result = cmd2jura("RT:1000");
    //ESP_LOGD("main", "Raw RT: 1000 result: %s", result.c_str());
    //result = cmd2jura("RT:2000");
    //ESP_LOGD("main", "Raw RT: 2000 result: %s", result.c_str());
    //result = cmd2jura("RT:3000");
    //ESP_LOGD("main", "Raw RT: 3000 result: %s", result.c_str());
    //result = cmd2jura("RT:4000");
    //ESP_LOGD("main", "Raw RT: 4000 result: %s", result.c_str());
    //result = cmd2jura("RT:5000");
    //ESP_LOGD("main", "Raw RT: 5000 result: %s", result.c_str());
    //result = cmd2jura("RT:6000");
    //ESP_LOGD("main", "Raw RT: 6000 result: %s", result.c_str());
    //result = cmd2jura("RT:7000");
    //ESP_LOGD("main", "Raw RT: 7000 result: %s", result.c_str());
    //result = cmd2jura("RT:8000");
    //ESP_LOGD("main", "Raw RT: 8000 result: %s", result.c_str());
    //result = cmd2jura("RT:9000");
    //ESP_LOGD("main", "Raw RT: 9000 result: %s", result.c_str());
    //result = cmd2jura("RT:A000");
    //ESP_LOGD("main", "Raw RT: A000 result: %s", result.c_str());
    //result = cmd2jura("RT:B000");
    //ESP_LOGD("main", "Raw RT: B000 result: %s", result.c_str());
    //result = cmd2jura("RT:C000");
    //ESP_LOGD("main", "Raw RT: C000 result: %s", result.c_str());
    //result = cmd2jura("RT:D000");
    //ESP_LOGD("main", "Raw RT: D000 result: %s", result.c_str());
    //result = cmd2jura("RT:E000");
    //ESP_LOGD("main", "Raw RT: E000 result: %s", result.c_str());
    //result = cmd2jura("RT:F000");
    //ESP_LOGD("main", "Raw RT: F000 result: %s", result.c_str());


    //result = cmd2jura("RE:0E");
    //ESP_LOGD("main", "Raw RE: 0E result: %s", result.c_str());
    //result = cmd2jura("RE:0F");
    //ESP_LOGD("main", "Raw RE: 0F result: %s", result.c_str());
    //result = cmd2jura("RE:10");
    //ESP_LOGD("main", "Raw RE: 10 result: %s", result.c_str());
    //result = cmd2jura("RE:14");
    //ESP_LOGD("main", "Raw RE: 14 result: %s", result.c_str());
    //result = cmd2jura("RE:15");
    //ESP_LOGD("main", "Raw RE: 15 result: %s", result.c_str());
    //result = cmd2jura("RE:34");
    //ESP_LOGD("main", "Raw RE: 34 result: %s", result.c_str());
    //result = cmd2jura("RE:36");
    //ESP_LOGD("main", "Raw RE: 36 result: %s", result.c_str());
    //result = cmd2jura("RE:7C");
    //ESP_LOGD("main", "Raw RE: 7C result: %s", result.c_str());
    //result = cmd2jura("RE:CE");
    //ESP_LOGD("main", "Raw RE: CE result: %s", result.c_str());
    //result = cmd2jura("RE:CD");
    //ESP_LOGD("main", "Raw RE: CD result: %s", result.c_str());
    //result = cmd2jura("RE:E0");
    //ESP_LOGD("main", "Raw RE: E0 result: %s", result.c_str());
    //result = cmd2jura("RE:E1");
    //ESP_LOGD("main", "Raw RE: E1 result: %s", result.c_str());
    //result = cmd2jura("RE:E2");
    //ESP_LOGD("main", "Raw RE: E2 result: %s", result.c_str());
  }
};
