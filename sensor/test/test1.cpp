
// Perform measurements or read nameplate values on your tub to define the power [kW]
// for each device in order to calculate tub power usage
const float POWER_HEATER = 2.8;
const float POWER_PUMP_CIRCULATION = 0.3;
const float POWER_PUMP1_LOW = 0.31;
const float POWER_PUMP1_HIGH = 1.3;
const float POWER_PUMP2_LOW = 0.3;
const float POWER_PUMP2_HIGH = 0.6;

const int MINUTES_PER_DEGC = 1;

#define tub Serial1

#define RTS_PIN 1
#define PIN_5_PIN 1

#include "balboaGL.h"

void telnetSend(String message) {}

#include <gtest/gtest.h>
// uncomment line below if you plan to use GMock
// #include <gmock/gmock.h>

// TEST(...)
// TEST_F(...)

int test_send_command() {
  sendCommand("test", 2);
  int count = sendBuffer.itemCount();
  sendBuffer.dequeue();
  sendBuffer.dequeue();
  return count;
}

BalboaStatus parse_status(String msg) {
  result = msg;
  handleMessage();
  return status;
}


TEST(TestSuiteName, test_send_command) {
  EXPECT_EQ(2, test_send_command());
}

TEST(TestSuiteName, test_heater_on) {
  EXPECT_EQ(true, parse_status("fa142d2d2d2d0000040000008002ffffff000000000068").heater);
}

TEST(TestSuiteName, test_light_on) {
  EXPECT_EQ(true, parse_status("fa142d2d2d2d0000040000008002ffffff000000000068").light);
}


#if defined(ARDUINO)
#include <Arduino.h>

void setup()
{
    // should be the same value as for the `test_speed` option in "platformio.ini"
    // default value is test_speed=115200
    Serial.begin(115200);

    ::testing::InitGoogleTest();
    // if you plan to use GMock, replace the line above with
    // ::testing::InitGoogleMock();
}

void loop()
{
  // Run tests
  if (RUN_ALL_TESTS())
  ;

  // sleep for 1 sec
  delay(1000);
}

#else
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    // if you plan to use GMock, replace the line above with
    // ::testing::InitGoogleMock(&argc, argv);

    if (RUN_ALL_TESTS())
    ;

    // Always return zero-code and allow PlatformIO to parse results
    return 0;
}
#endif