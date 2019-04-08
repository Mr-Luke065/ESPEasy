/*********************************************************************************************\
   Get value count from sensor type
  \*********************************************************************************************/

byte getValueCountFromSensorType(byte sensorType)
{
  switch (sensorType)
  {
    case SENSOR_TYPE_NONE:
      return 0;
    case SENSOR_TYPE_SINGLE: // single value sensor, used for Dallas, BH1750, etc
    case SENSOR_TYPE_SWITCH:
    case SENSOR_TYPE_DIMMER:
      return 1;
    case SENSOR_TYPE_LONG:   // single LONG value, stored in two floats (rfid tags)
      return 1;
    case SENSOR_TYPE_TEMP_HUM:
    case SENSOR_TYPE_TEMP_BARO:
    case SENSOR_TYPE_DUAL:
      return 2;
    case SENSOR_TYPE_TEMP_HUM_BARO:
    case SENSOR_TYPE_TEMP_EMPTY_BARO: // Values 1 and 3 will contain data.
    case SENSOR_TYPE_TRIPLE:
    case SENSOR_TYPE_WIND:
      return 3;
    case SENSOR_TYPE_QUAD:
      return 4;
  }
  addLog(LOG_LEVEL_ERROR, F("getValueCountFromSensorType: Unknown sensortype"));
  return 0;
}

String getSensorTypeLabel(byte sensorType) {
  switch (sensorType) {
    case SENSOR_TYPE_SINGLE:           return  F("Single");
    case SENSOR_TYPE_TEMP_HUM:         return  F("Temp / Hum");
    case SENSOR_TYPE_TEMP_BARO:        return  F("Temp / Baro");
    case SENSOR_TYPE_TEMP_HUM_BARO:    return  F("Temp / Hum / Baro");
    case SENSOR_TYPE_DUAL:             return  F("Dual");
    case SENSOR_TYPE_TRIPLE:           return  F("Triple");
    case SENSOR_TYPE_QUAD:             return  F("Quad");
    case SENSOR_TYPE_SWITCH:           return  F("Switch");
    case SENSOR_TYPE_DIMMER:           return  F("Dimmer");
    case SENSOR_TYPE_LONG:             return  F("Long");
    case SENSOR_TYPE_WIND:             return  F("Wind");
  }
  return "";
}

void sensorTypeHelper_webformLoad_allTypes(struct EventStruct *event, byte pconfigIndex)
{
  byte optionValues[11];
  optionValues[0] = SENSOR_TYPE_SINGLE;
  optionValues[1] = SENSOR_TYPE_TEMP_HUM;
  optionValues[2] = SENSOR_TYPE_TEMP_BARO;
  optionValues[3] = SENSOR_TYPE_TEMP_HUM_BARO;
  optionValues[4] = SENSOR_TYPE_DUAL;
  optionValues[5] = SENSOR_TYPE_TRIPLE;
  optionValues[6] = SENSOR_TYPE_QUAD;
  optionValues[7] = SENSOR_TYPE_SWITCH;
  optionValues[8] = SENSOR_TYPE_DIMMER;
  optionValues[9] = SENSOR_TYPE_LONG;
  optionValues[10] = SENSOR_TYPE_WIND;
  sensorTypeHelper_webformLoad(event, pconfigIndex, 11, optionValues);
}

void sensorTypeHelper_webformLoad_simple(struct EventStruct *event, byte pconfigIndex)
{
  byte optionValues[4];
  optionValues[0] = SENSOR_TYPE_SINGLE;
  optionValues[1] = SENSOR_TYPE_DUAL;
  optionValues[2] = SENSOR_TYPE_TRIPLE;
  optionValues[3] = SENSOR_TYPE_QUAD;
  sensorTypeHelper_webformLoad(event, pconfigIndex, 4, optionValues);
}

void sensorTypeHelper_webformLoad(struct EventStruct *event, byte pconfigIndex, int optionCount, const byte options[]) //, bool simple)
{

  byte choice = PCONFIG(pconfigIndex);
  byte DeviceIndex = getDeviceIndex(Settings.TaskDeviceNumber[event->TaskIndex]);
  if (getValueCountFromSensorType(choice) != Device[DeviceIndex].ValueCount) {
    // Invalid value
    choice = Device[DeviceIndex].VType;
  }
  addRowLabel(F("Output Data Type"));
  addSelector_Head(PCONFIG_LABEL(pconfigIndex), false);
  for (byte x = 0; x < optionCount; x++)
  {
    String name = getSensorTypeLabel(options[x]);
    bool disabled = false;
    addSelector_Item(name,
                     options[x],
                     choice == options[x],
                     disabled,
                     "");
  }
  addSelector_Foot();
  addFormNote(F("Changing 'Output Data Type' may affect behavior of some controllers (e.g. Domoticz)"));

  //addFormSelector(F("Output Data Type"), PCONFIG_LABEL(pconfigIndex), 11, options, optionValues, choice);
}

void sensorTypeHelper_saveSensorType(struct EventStruct *event, byte pconfigIndex)
{
  pconfig_webformSave(event, pconfigIndex);
  sensorTypeHelper_setSensorType(event, pconfigIndex);
}

void sensorTypeHelper_setSensorType(struct EventStruct *event, byte pconfigIndex)
{
  byte sensorType = PCONFIG(pconfigIndex);
  byte DeviceIndex = getDeviceIndex(Settings.TaskDeviceNumber[event->TaskIndex]);
  Device[DeviceIndex].VType = sensorType;
  Device[DeviceIndex].ValueCount = getValueCountFromSensorType(sensorType);
}


void pconfig_webformSave(struct EventStruct *event, byte pconfigIndex)
{
  PCONFIG(pconfigIndex) = getFormItemInt(PCONFIG_LABEL(pconfigIndex), 0);
}
