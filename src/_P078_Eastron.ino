#include "_Plugin_Helper.h"

#ifdef USES_P078

// #######################################################################################################
// ############## Plugin 078: SDM120/SDM120CT/220/230/630/72D/DDM18SD Eastron Energy Meter ###############
// #######################################################################################################

/*
   Plugin written by: Sergio Faustino sjfaustino__AT__gmail.com

   This plugin reads available values of an Eastron SDM120C SDM120/SDM120CT/220/230/630/72D & also DDM18SD.
 */

# define PLUGIN_078
# define PLUGIN_ID_078         78
# define PLUGIN_NAME_078       "Energy (AC) - Eastron SDM120/SDM120CT/220/230/630/72D/DDM18SD"

# define P078_NR_OUTPUT_VALUES                            4
# define P078_NR_OUTPUT_OPTIONS_SDM220_SDM120CT_SDM120    14
# define P078_NR_OUTPUT_OPTIONS_SDM230                    24
# define P078_NR_OUTPUT_OPTIONS_SDM630                    86
# define P078_NR_OUTPUT_OPTIONS_SDM72D                    9
# define P078_NR_OUTPUT_OPTIONS_DDM18SD                   7


# include "src/PluginStructs/P078_data_struct.h"

// These pointers may be used among multiple instances of the same plugin,
// as long as the same serial settings are used.
ESPeasySerial *Plugin_078_ESPEasySerial = nullptr;
SDM *Plugin_078_SDM                  = nullptr;
boolean Plugin_078_init              = false;

boolean Plugin_078(uint8_t function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
    {
      Device[++deviceCount].Number           = PLUGIN_ID_078;
      Device[deviceCount].Type               = DEVICE_TYPE_SERIAL_PLUS1; // connected through 3 datapins
      Device[deviceCount].VType              = Sensor_VType::SENSOR_TYPE_QUAD;
      Device[deviceCount].Ports              = 0;
      Device[deviceCount].PullUpOption       = false;
      Device[deviceCount].InverseLogicOption = false;
      Device[deviceCount].FormulaOption      = true;
      Device[deviceCount].ValueCount         = P078_NR_OUTPUT_VALUES;
      Device[deviceCount].OutputDataType     = Output_Data_type_t::Simple;
      Device[deviceCount].SendDataOption     = true;
      Device[deviceCount].TimerOption        = true;
      Device[deviceCount].GlobalSyncOption   = true;
      Device[deviceCount].PluginStats        = true;
      break;
    }

    case PLUGIN_GET_DEVICENAME:
    {
      string = F(PLUGIN_NAME_078);
      break;
    }

    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      for (uint8_t i = 0; i < VARS_PER_TASK; ++i) {
        if (i < P078_NR_OUTPUT_VALUES) {
          const SDM_MODEL model  = static_cast<SDM_MODEL>(P078_MODEL);
          const uint8_t   choice = PCONFIG(i + P078_QUERY1_CONFIG_POS);
          safe_strncpy(
            ExtraTaskSettings.TaskDeviceValueNames[i],
            SDM_getValueNameForModel(model, choice),
            sizeof(ExtraTaskSettings.TaskDeviceValueNames[i]));
        } else {
          ZERO_FILL(ExtraTaskSettings.TaskDeviceValueNames[i]);
        }
      }
      break;
    }

    case PLUGIN_GET_DEVICEGPIONAMES:
    {
      serialHelper_getGpioNames(event);
      event->String3 = formatGpioName_output_optional(F("DE"));
      break;
    }

    case PLUGIN_WEBFORM_SHOW_CONFIG:
    {
      string += serialHelper_getSerialTypeLabel(event);
      success = true;
      break;
    }

    case PLUGIN_SET_DEFAULTS:
    {
      P078_DEV_ID   = P078_DEV_ID_DFLT;
      P078_MODEL    = P078_MODEL_DFLT;
      P078_BAUDRATE = P078_BAUDRATE_DFLT;
      P078_QUERY1   = P078_QUERY1_DFLT;
      P078_QUERY2   = P078_QUERY2_DFLT;
      P078_QUERY3   = P078_QUERY3_DFLT;
      P078_QUERY4   = P078_QUERY4_DFLT;

      success = true;
      break;
    }

    case PLUGIN_WEBFORM_SHOW_SERIAL_PARAMS:
    {
      if ((P078_DEV_ID == 0) || (P078_DEV_ID > 247) || (P078_BAUDRATE >= 6)) {
        // Load some defaults
        P078_DEV_ID   = P078_DEV_ID_DFLT;
        P078_MODEL    = P078_MODEL_DFLT;
        P078_BAUDRATE = P078_BAUDRATE_DFLT;
        P078_QUERY1   = P078_QUERY1_DFLT;
        P078_QUERY2   = P078_QUERY2_DFLT;
        P078_QUERY3   = P078_QUERY3_DFLT;
        P078_QUERY4   = P078_QUERY4_DFLT;
      }
      {
        String options_baudrate[6];

        for (int i = 0; i < 6; ++i) {
          options_baudrate[i] = String(p078_storageValueToBaudrate(i));
        }
        addFormSelector(F("Baud Rate"), P078_BAUDRATE_LABEL, 6, options_baudrate, nullptr, P078_BAUDRATE);
        addUnit(F("baud"));
      }

      if ((P078_MODEL == 0) && (P078_BAUDRATE > 3)) {
        addFormNote(F("<span style=\"color:red\"> SDM120 only allows up to 9600 baud with default 2400!</span>"));
      }

      if ((P078_MODEL == 3) && (P078_BAUDRATE == 0)) {
        addFormNote(F("<span style=\"color:red\"> SDM630 only allows 2400 to 38400 baud with default 9600!</span>"));
      }

      addFormNumericBox(F("Modbus Address"), P078_DEV_ID_LABEL, P078_DEV_ID, 1, 247);

      if (Plugin_078_SDM != nullptr) {
        addRowLabel(F("Checksum (pass/fail)"));
        String chksumStats;
        chksumStats  = Plugin_078_SDM->getSuccCount();
        chksumStats += '/';
        chksumStats += Plugin_078_SDM->getErrCount();
        addHtml(chksumStats);
      }

      break;
    }

    case PLUGIN_WEBFORM_LOAD_OUTPUT_SELECTOR:
    {
      // In a separate scope to free memory of String array as soon as possible
      for (uint8_t i = 0; i < P078_NR_OUTPUT_VALUES; ++i) {
        const uint8_t pconfigIndex = i + P078_QUERY1_CONFIG_POS;
        SDM_loadOutputSelector(event, pconfigIndex, i);
      }

      break;
    }

    case PLUGIN_WEBFORM_LOAD:
    {
      {
        const __FlashStringHelper *options_model[] = {
          F("SDM220 & SDM120CT & SDM120"),
          F("SDM230"),
          F("SDM72D"),
          F("DDM18SD"),
          F("SDM630"),
          F("SDM72_V2"),
          F("SDM320C")
        };
        constexpr size_t nrOptions = sizeof(options_model) / sizeof(options_model[0]);
        addFormSelector(F("Model Type"), P078_MODEL_LABEL, nrOptions, options_model, nullptr, P078_MODEL);
        addFormNote(F("Submit after changing the modell to update Output Configuration."));
      }
      success = true;
      break;
    }


    case PLUGIN_WEBFORM_SAVE:
    {
      // Save output selector parameters.
      const SDM_MODEL model = static_cast<SDM_MODEL>(P078_MODEL);

      for (uint8_t i = 0; i < P078_NR_OUTPUT_VALUES; ++i) {
        const uint8_t pconfigIndex = i + P078_QUERY1_CONFIG_POS;
        const uint8_t choice       = PCONFIG(pconfigIndex);
        sensorTypeHelper_saveOutputSelector(
          event,
          pconfigIndex,
          i,
          SDM_getValueNameForModel(model, choice));
      }

      P078_DEV_ID   = getFormItemInt(P078_DEV_ID_LABEL);
      P078_MODEL    = getFormItemInt(P078_MODEL_LABEL);
      P078_BAUDRATE = getFormItemInt(P078_BAUDRATE_LABEL);

      Plugin_078_init = false; // Force device setup next time
      success         = true;
      break;
    }

    case PLUGIN_INIT:
    {
      Plugin_078_init = true;

      if (Plugin_078_ESPEasySerial != nullptr) {
        delete Plugin_078_ESPEasySerial;
        Plugin_078_ESPEasySerial = nullptr;
      }
      Plugin_078_ESPEasySerial = new (std::nothrow) ESPeasySerial(static_cast<ESPEasySerialPort>(CONFIG_PORT), CONFIG_PIN1, CONFIG_PIN2);

      if (Plugin_078_ESPEasySerial == nullptr) {
        break;
      }
      unsigned int baudrate = p078_storageValueToBaudrate(P078_BAUDRATE);
      Plugin_078_ESPEasySerial->begin(baudrate);

      if (Plugin_078_SDM != nullptr) {
        delete Plugin_078_SDM;
        Plugin_078_SDM = nullptr;
      }
      if (Plugin_078_ESPEasySerial->setRS485Mode(P078_DEPIN)) {
        Plugin_078_SDM = new SDM(*Plugin_078_ESPEasySerial, baudrate);
      } else {
        Plugin_078_SDM = new SDM(*Plugin_078_ESPEasySerial, baudrate, P078_DEPIN);
      }

      if (Plugin_078_SDM != nullptr) {
        success = true;
        Plugin_078_SDM->begin();

        // Set timeout to time needed to receive 1 byte
        Plugin_078_SDM->setMsTimeout((10000 / baudrate) + 1);

        SDM_MODEL model  = static_cast<SDM_MODEL>(P078_MODEL);
        uint8_t   dev_id = P078_DEV_ID;
/*
        if (loglevelActiveFor(LOG_LEVEL_INFO)) {
          String log;
          log = F("Eastron: SN: ");
          log += Plugin_078_SDM->getSerialNumber(dev_id);
          log += ',';
          log += Plugin_078_SDM->getErrCode(true);
          log += F(" SW-ver: ");
          log += Plugin_078_SDM->readHoldingRegister(SDM_HOLDING_SOFTWARE_VERSION, dev_id);
          log += ',';
          log += Plugin_078_SDM->getErrCode(true);
          log += F(" ID: ");
          log += Plugin_078_SDM->readHoldingRegister(SDM_HOLDING_METER_ID, dev_id);
          log += ',';
          log += Plugin_078_SDM->getErrCode(true);
          log += F(" baudrate: ");
          log += Plugin_078_SDM->readHoldingRegister(SDM_HOLDING_BAUD_RATE, dev_id);
          log += ',';
          log += Plugin_078_SDM->getErrCode(true);
          addLogMove(LOG_LEVEL_INFO, log);
        }
*/
        for (taskVarIndex_t i = 0; i < VARS_PER_TASK; ++i) {
          const uint16_t reg = SDM_getRegisterForModel(model, PCONFIG((P078_QUERY1_CONFIG_POS) + i));
          SDM_addRegisterReadQueueElement(event->TaskIndex, i, reg, dev_id);
        }
      }
      break;
    }

    case PLUGIN_EXIT:
    {
      for (taskVarIndex_t i = 0; i < VARS_PER_TASK; ++i) {
        SDM_removeRegisterReadQueueElement(event->TaskIndex, i);
      }

      Plugin_078_init = false;

      if (Plugin_078_ESPEasySerial != nullptr) {
        delete Plugin_078_ESPEasySerial;
        Plugin_078_ESPEasySerial = nullptr;
      }

      if (Plugin_078_SDM != nullptr) {
        delete Plugin_078_SDM;
        Plugin_078_SDM = nullptr;
      }
      break;
    }

    case PLUGIN_TEN_PER_SECOND:
    {
      if (Plugin_078_init)
      {
        SDM_loopRegisterReadQueue(Plugin_078_SDM);
      }
      break;
    }

    case PLUGIN_READ:
    {
      if (Plugin_078_init)
      {
        success = true;
        break;
      }
      break;
    }
  }
  return success;
}

int p078_storageValueToBaudrate(uint8_t baudrate_setting) {
  int baudrate = 9600;

  if (baudrate_setting < 6) {
    baudrate = 1200 << baudrate_setting;
  }
  return baudrate;
}

#endif // USES_P078
