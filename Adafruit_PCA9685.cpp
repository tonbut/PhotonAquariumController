/*************************************************** 
  This is a library for our Adafruit 16-channel PWM & Servo driver

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/815

  These displays use I2C to communicate, 2 pins are required to  
  interface. For Arduino UNOs, thats SCL -> Analog 5, SDA -> Analog 4

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
  
  Adapted for Spark Core by Paul Kourany, Sept. 15, 2014
  Adapted for Grove Labs by Chad Bean, Nov. 13, 2014
  Adapted for Grove Labs by Louis DeScioli, Dec 12, 2014
 ****************************************************/

#include "Adafruit_PCA9685.h"
#include <math.h>

/**
 * Initializes the driver with an I2C address. This address must match
 * the address that is set by the physical jumpers on the driver. If the
 * address is not being set by hardware, give no parameters and it will
 * use the default as specified in the header file (0x40)
 */
Adafruit_PCA9685::Adafruit_PCA9685(uint8_t addr, bool debug) {
  _i2caddr = addr;
  _debug = debug;
}

/**
 * Join the I2C bus as a master and setup the driver's mode
 */
void Adafruit_PCA9685::begin(void) {
 Wire.begin();
 reset();
}

/**
 * Setup the driver's modes
 */
void Adafruit_PCA9685::reset(void) {
 write8(MODE1, 0x0);  // See page 13 of datasheet
}

/**
 * Set the output frequency of the board
 * @param freq  The frequency
 */
void Adafruit_PCA9685::setPWMFreq(float freq) {
  if ( _debug ) {
    //Serial.print("Attempting to set freq "); Serial.println(freq);
  }
  freq *= 0.9;  // Correct for overshoot in the frequency setting (see issue #11).
  float prescaleval = 25000000;
  prescaleval /= 4096;
  prescaleval /= freq;
  prescaleval -= 1;
  if ( _debug ) {
    //Serial.print("Estimated pre-scale: "); Serial.println(prescaleval);
  }
  uint8_t prescale = floor(prescaleval + 0.5);
  if ( _debug ) {
    //Serial.print("Final pre-scale: "); Serial.println(prescale);
  }
  
  uint8_t oldmode = read8(MODE1);
  uint8_t newmode = (oldmode & 0x7F) | 0x10; // sleep
  write8(MODE1, newmode); // go to sleep
  write8(PRESCALE, prescale); // set the prescaler
  write8(MODE1, oldmode);
  delay(5);
  write8(MODE1, oldmode | 0xa1);  // Turns on auto increment in MODE1 register
}

/**
 * Sets the value of an LED without having to deal with on/off tick placement.
 * Also properly handles a zero value as completely off.  Optional invert 
 * parameter supports inverting the pulse for sinking to ground.
 * @param ledNum  The LED number on the driver board (0 -> 15)
 * @param val     The duty cycle value. Should be from 0 to 4095 inclusive, 
 *                will be clamped if not within range
 * @param invert  Whether or not to invert the pulse for sinking to ground
 */
void Adafruit_PCA9685::setVal(uint8_t ledNum, uint16_t val, bool invert)
{
  // Clamp value between 0 and 4095 inclusive.
  val = min(val, 4095);
  val = max(0, val);
  if (invert) {
    if (val == 0) {
      // Special value for signal fully on.
      setPWM(ledNum, 4096, 0);
    }
    else if (val == 4095) {
      // Special value for signal fully off.
      setPWM(ledNum, 0, 4096);
    }
    else {
      setPWM(ledNum, 0, 4095-val);
    }
  }
  else {
    if (val == 4095) {
      // Special value for signal fully on.
      setPWM(ledNum, 4096, 0);
    }
    else if (val == 0) {
      // Special value for signal fully off.
      setPWM(ledNum, 0, 4096);
    }
    else {
      setPWM(ledNum, 0, val);
    }
  }
}

/**
 * Read the set PWM-off value for a given LED
 * @param  ledNum  The LED number on the driver
 * @return         The 12-bit PWM-off value, given in 2 bytes
 */
uint16_t Adafruit_PCA9685::readPWMOff(uint8_t ledNum) {
  int toReturn =  (read8(LED0_OFF_H + 4*ledNum) << 8);  // Read first byte and shift it
  toReturn += read8(LED0_OFF_L + 4*ledNum);             // Read the second byte
  return toReturn;
}

/**
 * Read the set PWM-on value for a given LED
 * @param  ledNum  The LED number on the driver
 * @return         The 12-bit PWM-on value, given in 2 bytes
 */
uint16_t Adafruit_PCA9685::readPWMOn(uint8_t ledNum) {
  int result = (read8(LED0_ON_H + 4*ledNum) << 8);
  result += read8(LED0_ON_L + 4*ledNum);
  return result;
}

/**
 * Explicitly set the duty cycle for an LED. setVal() provides some abstractions atop this method 
 * and is recommended over this function
 * @param ledNum  The LED number on the driver board (0 -> 15)
 * @param on      12-bit PWM-on value
 * @param off     12-bit PWM-off value
 */
void Adafruit_PCA9685::setPWM(uint8_t ledNum, uint16_t on, uint16_t off) {
  if (_debug) {
   //Serial.print("Setting PWM for LED "); Serial.print(ledNum); Serial.print(" to ");
   //Serial.print(on); Serial.print(" -> "); Serial.println(off);
  }

  Wire.beginTransmission(_i2caddr);
  Wire.write(LED0_ON_L + 4*ledNum);  // Offset the address of the LED
  Wire.write(on);                    // Write the first byte for On
  Wire.write(on >> 8);               // Write the second byte
  Wire.write(off);                   // First byte for Off
  Wire.write(off >> 8);              // Second byte for Off
  Wire.endTransmission();
}

/**
 * Read a byte from a given address on the driver
 * @param  addr  The address
 * @return       The value at the given address
 */
uint8_t Adafruit_PCA9685::read8(uint8_t addr) {
  Wire.beginTransmission(_i2caddr);
  Wire.write(addr);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)_i2caddr, (uint8_t)1);
  return Wire.read();
}

/**
 * Write a byte to a given address on the driver
 * @param addr  The address
 * @param val   The byte to be written
 */
void Adafruit_PCA9685::write8(uint8_t addr, uint8_t val) {
  Wire.beginTransmission(_i2caddr);
  Wire.write(addr);
  Wire.write(val);
  Wire.endTransmission();
}
