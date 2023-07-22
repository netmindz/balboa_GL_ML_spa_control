#include "unity.h"

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


void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void test_function_should_doBlahAndBlah(void) {
  // test stuff
}

void test_function_should_doAlsoDoBlah(void) {
  // more test stuff
}

int runUnityTests(void) {
  UNITY_BEGIN();
  RUN_TEST(test_function_should_doBlahAndBlah);
  RUN_TEST(test_function_should_doAlsoDoBlah);
  return UNITY_END();
}

// WARNING!!! PLEASE REMOVE UNNECESSARY MAIN IMPLEMENTATIONS //

/**
  * For native dev-platform or for some embedded frameworks
  */
int main(void) {
  return runUnityTests();
}

/**
  * For Arduino framework
  */
void setup() {
  // Wait ~2 seconds before the Unity test runner
  // establishes connection with a board Serial interface
  delay(2000);

  runUnityTests();
}
void loop() {}

/**
  * For ESP-IDF framework
  */
void app_main() {
  runUnityTests();
}
