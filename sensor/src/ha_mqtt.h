#include <ArduinoHA.h>

HADevice device(mac, sizeof(mac));
HAMqtt mqtt(clients[0], device, 30);
HASensorNumber temp("temp", HANumber::PrecisionP1);
HASensorNumber targetTemp("targetTemp", HANumber::PrecisionP1);
HASensorNumber timeToTemp("timeToTemp");
HASensor currentState("status");
HASensor haTime("time");
HASensor rawData("raw");
HASensor rawData2("raw2");
HASensor rawData3("raw3");
HASensor rawData7("raw7");
HASelect tubMode("mode");
HASensorNumber uptime("uptime");
HASelect pump1("pump1");
HASelect pump2("pump2");
HABinarySensor heater("heater");
HASwitch light("light");
HASensorNumber tubpower("tubpower", HANumber::PrecisionP1);

HAButton btnUp("up");
HAButton btnDown("down");
HAButton btnMode("mode");

HAHVAC hvac( // Not really HVAC device, but only way to get controls to set
  "temp",
  HAHVAC::TargetTemperatureFeature
);



void onSwitchStateChanged(bool state, HASwitch* sender)
{
    Serial.printf("Switch %s changed - ", sender->getName());
    if(state != lightState) {
      Serial.println("Toggle");
      sendBuffer.enqueue(COMMAND_LIGHT);
    }
    else {
      Serial.println("No change needed");
    }
}

void onPumpSwitchStateChanged(int8_t index, HASelect* sender)
{
  Serial.printf("onPumpSwitchStateChanged %s %u\n", sender->getName(), index);
  int currentIndex = sender->getCurrentState();
  String command = COMMAND_JET1;
  int options = PUMP1_STATE_HIGH + 1;
  if(sender->getName() == "Pump2") {
    command = COMMAND_JET2;
    options = PUMP2_STATE_HIGH + 1;
  }
  setOption(currentIndex, index, options, command);
}


void onModeSwitchStateChanged(int8_t index, HASelect* sender) {
  Serial.printf("Mode Switch changed - %u\n", index);
  int currentIndex = sender->getCurrentState();
  int options = 3;
  sendBuffer.enqueue(COMMAND_CHANGE_MODE);
  setOption(currentIndex, index, options);
  sendBuffer.enqueue(COMMAND_CHANGE_MODE);
}

void onButtonPress(HAButton * sender) {
  String name = sender->getName();
  Serial.printf("Button press - %s\n", name);
  if(name == "Up") {
    sendBuffer.enqueue(COMMAND_UP);
  }
  else if(name == "Down") {
    sendBuffer.enqueue(COMMAND_DOWN);
  }
  else if(name == "Mode") {
    sendBuffer.enqueue(COMMAND_CHANGE_MODE);
  }
  else {
    Serial.printf("Unknown button %s\n", name);
  }

}

void onTargetTemperatureCommand(HANumeric temperature, HAHVAC* sender) {
  float temperatureFloat = temperature.toFloat();

  Serial.print("Target temperature: ");
  Serial.println(temperatureFloat);

  if(tubTargetTemp < 0) {
    Serial.print("ERROR: can't adjust target as current value not known");
    sendBuffer.enqueue(COMMAND_UP); // Enter set temp mode - won't change, but should allow us to capture the set target value
    return;
  }

  int target = temperatureFloat * 2; // 0.5 inc so double
  int current = tubTargetTemp * 2;
  sendBuffer.enqueue(COMMAND_UP); // Enter set temp mode
  sendBuffer.enqueue(COMMAND_EMPTY);

  if(temperatureFloat > tubTargetTemp) {
    for(int i = 0; i < (target - current); i++) {
      Serial.println("Raise the temp");
      sendBuffer.enqueue(COMMAND_UP);
      // sendBuffer.enqueue(COMMAND_EMPTY);
    }
  }
  else {
    for(int i = 0; i < (current - target); i++) {
     Serial.println("Lower the temp");
      sendBuffer.enqueue(COMMAND_DOWN);
      // sendBuffer.enqueue(COMMAND_EMPTY);
    }
  }

    // sender->setTargetTemperature(temperature); // report target temperature back to the HA panel - better to see what the control unit reports that assume our commands worked
}

void setupHaMqtt(){
  // Home Assistant
  device.setName("Hottub");
  device.setSoftwareVersion("0.2.1");
  device.setManufacturer("Balboa");
  device.setModel("GL2000");

  // configure sensor (optional)
  temp.setUnitOfMeasurement("°C");
  temp.setDeviceClass("temperature");
  temp.setName("Tub temperature");

  targetTemp.setUnitOfMeasurement("°C");
  targetTemp.setDeviceClass("temperature");
  targetTemp.setName("Target Tub temp");


  timeToTemp.setName("Time to temp");
  timeToTemp.setUnitOfMeasurement("minutes");
  timeToTemp.setDeviceClass("duration");
  timeToTemp.setIcon("mdi:clock");

  currentState.setName("Status");

  pump1.setName("Pump1");
  #ifdef PUMP1_DUAL_SPEED
  pump1.setOptions("Off;Medium;High");
  #else
  pump1.setOptions("Off;High");
  #endif 
  pump1.setIcon("mdi:chart-bubble");
  pump1.onCommand(onPumpSwitchStateChanged);

  pump2.setName("Pump2");
  #ifdef PUMP2_DUAL_SPEED
  pump2.setOptions("Off;Medium;High");
  #else
  pump2.setOptions("Off;High");
  #endif 
  pump2.setIcon("mdi:chart-bubble");
  pump2.onCommand(onPumpSwitchStateChanged);

  heater.setName("Heater");
  heater.setIcon("mdi:radiator");
  light.setName("Light");
  light.onCommand(onSwitchStateChanged);

  haTime.setName("Time");

  rawData.setName("Raw data");
  rawData2.setName("CMD");
  rawData3.setName("post temp: ");
  rawData7.setName("FB");

  
  tubMode.setName("Mode");
  tubMode.setOptions("Standard;Economy;Sleep");
  tubMode.onCommand(onModeSwitchStateChanged);

  
  uptime.setName("Uptime");
  uptime.setUnitOfMeasurement("seconds");
  uptime.setDeviceClass("duration");

  tubpower.setName("Tub Power");
  tubpower.setUnitOfMeasurement("kW");
  tubpower.setDeviceClass("power");

  btnUp.setName("Up");
  btnDown.setName("Down");
  btnMode.setName("Mode");

  btnUp.onCommand(onButtonPress);
  btnDown.onCommand(onButtonPress);
  btnMode.onCommand(onButtonPress);

  hvac.onTargetTemperatureCommand(onTargetTemperatureCommand);
  hvac.setModes(HAHVAC::AutoMode);
  hvac.setMode(HAHVAC::AutoMode);
  hvac.setName("Target Temp");
  hvac.setMinTemp(26);
  hvac.setMaxTemp(40);
  hvac.setTempStep(0.5);
}