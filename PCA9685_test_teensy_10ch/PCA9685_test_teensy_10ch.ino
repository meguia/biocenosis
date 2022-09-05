#include "PCA9685.h"

TwoWire *wi0;
TwoWire *wi1;

// First Channel
PCA9685 ledArray0(0x40);
PCA9685 ledArray1(0x41);
PCA9685 ledArray2(0x42);
PCA9685 ledArray3(0x43);
PCA9685 ledArray4(0x44);

// Second Channel
PCA9685 ledArray5(0x40);
PCA9685 ledArray6(0x41);
PCA9685 ledArray7(0x42);
PCA9685 ledArray8(0x43);
PCA9685 ledArray9(0x44);



long speed[1] = {100};
int speeds=1;

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting Wire0");
  Wire.begin();
  wi1 = &Wire1;
  Serial.println("Starting Wire1");
  Wire1.begin();
  wi0 = &Wire;
  delay(500);
  bool resp0 = ledArray0.set_wire(wi0);
  Serial.print("C0 > ");
  Serial.println(resp0);
  resp0 = ledArray1.set_wire(wi0);
  Serial.print("C1 > ");
  Serial.println(resp0);
  resp0 = ledArray2.set_wire(wi0);
  Serial.print("C2 > ");
  Serial.println(resp0);
  resp0 = ledArray3.set_wire(wi0);
  Serial.print("C3 > ");
  Serial.println(resp0);
  resp0 = ledArray4.set_wire(wi0);
  Serial.print("C4 > ");
  Serial.println(resp0);
  resp0 = ledArray5.set_wire(wi1);
  Serial.print("C5 > ");
  Serial.println(resp0);
  resp0 = ledArray6.set_wire(wi1);
  Serial.print("C6 > ");
  Serial.println(resp0);
  resp0 = ledArray7.set_wire(wi1);
  Serial.print("C7 > ");
  Serial.println(resp0);
  resp0 = ledArray8.set_wire(wi1);
  Serial.print("C8 > ");
  Serial.println(resp0);
  resp0 = ledArray9.set_wire(wi1);
  Serial.print("C9 > ");
  Serial.println(resp0);

  is_connected(ledArray0,0);
  is_connected(ledArray1,1);
  is_connected(ledArray2,2);
  is_connected(ledArray3,3);
  is_connected(ledArray4,4);
  is_connected(ledArray5,5);
  is_connected(ledArray6,6);
  is_connected(ledArray7,7);
  is_connected(ledArray8,8);
  is_connected(ledArray9,9);
  delay(5000);
}


void loop()
{
  test_speed(ledArray0,0);
  test_speed(ledArray1,1);
  test_speed(ledArray2,2);
  test_speed(ledArray3,3);
  test_speed(ledArray4,4);
  test_speed(ledArray5,5);
  test_speed(ledArray6,6);
  test_speed(ledArray7,7);
  test_speed(ledArray8,8);
  test_speed(ledArray9,9);
}


void is_connected(PCA9685 ledArray, int n) {
  Serial.print("Starting Ledarray ");
  Serial.print(n);
  bool resp1 = ledArray.begin();
  Serial.print(" | Returned ");
  Serial.print(resp1);
  Serial.print(" | Is Connected ? ");
  bool resp2 = ledArray.isConnected();
  Serial.println(resp2);
}

void test_speed(PCA9685 ledArray, int n) {
  Serial.print("\nTesting Speeds Ledarray ");
  Serial.print(n);
  Serial.print(":");
  for (uint8_t s = 0; s < speeds ; s++)
  {
    Serial.print(speed[s]);
    Serial.print(" kHz:");
    ledArray.busSpeed(speed[s] * 1000UL);
    
    bool resp2 = ledArray.isConnected();
    Serial.print(resp2);
    Serial.print("\t");
    delay(500);
  }  
}
