#include <Wire.h>

// minimal Arduino TCS3472(5) I2C code
// put together with help from the VERY COMPLETE TSL2591 library by Austria Microsystems
// "TSL2591MI.h" Yet Another Arduino ams TSL2591MI Lux Sensor
// https://bitbucket.org/christandlg/tsl2591mi/src/master/
// https://www.arduino.cc/reference/en/libraries/tsl2591mi/
// SJ Remington 3/2021, 7/2021

// some useful device-specific constants
#define TCS3472_ADDR 0x29
#define TCS3472_CMD 0b10000000
#define TCS3472_TRANSACTION_AUTOINC 0b00100000
#define TCS3472_MASK_AEN 0b00000010
#define TCS3472_MASK_PON 0b00000001
#define TCS3472_MASK_AVALID 0b00000001

// device-specific register numbers
#define TCS3472_REG_ENABLE 0x00
#define TCS3472_REG_ATIME  0x01
#define TCS3472_REG_WTIME  0x03
#define TCS3472_REG_CONFIG 0x0D
#define TCS3472_REG_CONTROL 0x0F
#define TCS3472_REG_ID 0x12  //value 0x44 for TCS34725
#define TCS3472_REG_STATUS 0x13
#define TCS3472_REG_CDATAL 0x14
#define TCS3472_REG_CDATAH 0x15
#define TCS3472_REG_RDATAL 0x16
#define TCS3472_REG_RDATAH 0x17
#define TCS3472_REG_GDATAL 0x18
#define TCS3472_REG_GDATAH 0x19
#define TCS3472_REG_BDATAL 0x1A
#define TCS3472_REG_BDATAH 0x1B

// read a register in the TCS3472
int readRegister(uint8_t reg) {  //int return value, because -1 is possible
  Wire.beginTransmission(TCS3472_ADDR);
  Wire.write(TCS3472_CMD | TCS3472_TRANSACTION_AUTOINC | reg);
  Wire.endTransmission(false);
  Wire.requestFrom(TCS3472_ADDR, 1);
  return Wire.read();
}

//write value to TSL2591 register. Special functions not implemented
void writeRegister(uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(TCS3472_ADDR);
  Wire.write(TCS3472_CMD | TCS3472_TRANSACTION_AUTOINC | reg);
  Wire.write(value);
  Wire.endTransmission();
}

// reset to power on state
void TCS3472_reset(void) {
  writeRegister(TCS3472_REG_ENABLE, TCS3472_MASK_AEN | TCS3472_MASK_PON);
  writeRegister(TCS3472_REG_CONFIG, 0);  //WLONG off, may need to clear other registers too
  delay(3); //must wait > 2.4 ms before RGBC can be initiated
}
// reset to sleep state
void TCS3472_sleep(void) {
  writeRegister(TCS3472_REG_ENABLE, 0);
}

// power on and enable the ambient light sensor
void TCS3472_enable() {
  writeRegister(TCS3472_REG_ENABLE, TCS3472_MASK_AEN | TCS3472_MASK_PON);
}

// Configure: set gain 0 to 3 (low, med, high, max), 
// set integration time to 1 to 255 x (2.4 ms) cycles
void TCS3472_config(uint8_t gain, uint8_t time) {
  if (time == 0) time = 1;
  writeRegister(TCS3472_REG_ATIME, 256 - (int)time);
  writeRegister(TCS3472_REG_CONTROL, gain & 3); //1x, 4x, 16x, 60x
}

// get RGBC readings
void TCS3472_getData(uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c ) {
  uint8_t t;
  t = readRegister(TCS3472_REG_RDATAH);
  *r = (t << 8) |  readRegister(TCS3472_REG_RDATAL);
  t = readRegister(TCS3472_REG_GDATAH);
  *g = (t << 8) |  readRegister(TCS3472_REG_GDATAL);
  t = readRegister(TCS3472_REG_BDATAH);
  *b = (t << 8) |  readRegister(TCS3472_REG_BDATAL);
  t = readRegister(TCS3472_REG_CDATAH);
  *c = (t << 8) |  readRegister(TCS3472_REG_CDATAL);
}

// register dump for debugging
void TCS3472_regDump() {
  for (int i = 0; i < 0x1C; i++) {
    Serial.print("reg "); Serial.print(i);  Serial.print(" "); Serial.println(readRegister(i), HEX);
  }
}

void setup() {
  Serial.begin(9600);

  //wait for serial connection to open (only necessary on some boards)
  while (!Serial);

  Wire.begin();
  uint8_t id = readRegister(TCS3472_REG_ID); // read ID register
  if (id != 0x44) {
    Serial.print("ID: ");
    Serial.print(id, HEX);
    Serial.println("? TCS3472 not found");
    while (1);
  }
  Serial.println("TCS3472 OK");
  TCS3472_reset();
  TCS3472_enable();  //use TCS3472_sleep() to disable and power down
  TCS3472_config(1, 255); //med gain, 255*2.4 ms integration time
  TCS3472_sleep();
  delay(100);
}

void loop() {
  uint16_t r, g, b, c;
  unsigned int wait_count=0;
  
  TCS3472_enable();

  //no need to use blocking wait, but here we check when AVALID becomes high and quantify the delay.
  while ((readRegister(TCS3472_REG_STATUS) & TCS3472_MASK_AVALID) == 0) {wait_count++;}; //wait for data available

  TCS3472_getData(&r, &g, &b, &c);
  TCS3472_sleep();
  
  Serial.print("rgbc(w): ");
  Serial.print(r);
  Serial.print(", ");
  Serial.print(g);
  Serial.print(", ");
  Serial.print(b);
  Serial.print(", ");
  Serial.print(c);
  Serial.print(",");
  Serial.println(wait_count);  //quantify wait time
}
