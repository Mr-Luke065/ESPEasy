#include "../ESPEasyCore/ESPEasy_Console.h"


#include "../Commands/InternalCommands.h"

#include "../DataStructs/TimingStats.h"

#include "../DataTypes/ESPEasy_plugin_functions.h"

#include "../Globals/Cache.h"
#include "../Globals/Logging.h"
#include "../Globals/Plugins.h"
#include "../Globals/Settings.h"

#include "../Helpers/Memory.h"

#include <ESPEasySerialPort.h>

/*
 #if FEATURE_DEFINE_SERIAL_CONSOLE_PORT
 # include "../Helpers/_Plugin_Helper_serial.h"
 #endif // if FEATURE_DEFINE_SERIAL_CONSOLE_PORT
 */


EspEasy_Console_t::EspEasy_Console_t()
{
#if FEATURE_DEFINE_SERIAL_CONSOLE_PORT
  const ESPEasySerialPort port = static_cast<ESPEasySerialPort>(_console_serial_port);

# if USES_USBCDC

  /*
     if (port == ESPEasySerialPort::usb_cdc_0 ||
        port == ESPEasySerialPort::usb_cdc_1)
     {
      USB.manufacturerName(F("ESPEasy"));
      USB.productName()
     }
   */
# endif // if USES_USBCDC

# ifdef ESP8266
  constexpr size_t buffSize = 256;
# endif // ifdef ESP8266
# ifdef ESP32
  constexpr size_t buffSize = 4096;
# endif // ifdef ESP32

  ESPEasySerialConfig config;
  config.port          = port;
  config.baud          = DEFAULT_SERIAL_BAUD;
  config.receivePin    = _console_serial_rxpin;
  config.transmitPin   = _console_serial_txpin;
  config.inverse_logic = false;
  config.rxBuffSize    = 256;
  config.txBuffSize    = buffSize;

  _serial = new (std::nothrow) ESPeasySerial(config);

# if USES_ESPEASY_CONSOLE_FALLBACK_PORT

  if (Settings.console_serial0_fallback && (port != ESPEasySerialPort::serial0)) {
    config.port        = ESPEasySerialPort::serial0;
    config.receivePin  = SOC_RX0;
    config.transmitPin = SOC_TX0;

    _serial_fallback = new (std::nothrow) ESPeasySerial(config);
  }
# endif // if USES_ESPEASY_CONSOLE_FALLBACK_PORT


#endif  // if FEATURE_DEFINE_SERIAL_CONSOLE_PORT
}

EspEasy_Console_t::~EspEasy_Console_t() {
#if FEATURE_DEFINE_SERIAL_CONSOLE_PORT

  if (_serial != nullptr) {
    delete _serial;
    _serial = nullptr;
  }
# if USES_ESPEASY_CONSOLE_FALLBACK_PORT

  if (_serial_fallback != nullptr) {
    delete _serial_fallback;
    _serial_fallback = nullptr;
  }
# endif // if USES_ESPEASY_CONSOLE_FALLBACK_PORT
#endif  // if FEATURE_DEFINE_SERIAL_CONSOLE_PORT
}

void EspEasy_Console_t::begin(uint32_t baudrate)
{
  updateActiveTaskUseSerial0();
#if FEATURE_DEFINE_SERIAL_CONSOLE_PORT_bla

  if ((_console_serial_port != Settings.console_serial_port) ||
      (_console_serial_rxpin != Settings.console_serial_rxpin) ||
      (_console_serial_txpin != Settings.console_serial_txpin)) {
    if (_serial != nullptr) {
      delete _serial;
      _serial = nullptr;
    }
# if USES_ESPEASY_CONSOLE_FALLBACK_PORT

    if (_serial_fallback != nullptr) {
      delete _serial_fallback;
      _serial_fallback = nullptr;
    }
# endif // if USES_ESPEASY_CONSOLE_FALLBACK_PORT

    _console_serial_port  = Settings.console_serial_port;
    _console_serial_rxpin = Settings.console_serial_rxpin;
    _console_serial_txpin = Settings.console_serial_txpin;

    const ESPEasySerialPort port = static_cast<ESPEasySerialPort>(_console_serial_port);

    if (!(activeTaskUseSerial0() || log_to_serial_disabled) ||
        !(
# ifdef ESP8266
          (port == ESPEasySerialPort::serial0_swap) ||
# endif // ifdef ESP8266
          port == ESPEasySerialPort::serial0)
        ) {
      _serial = new (std::nothrow) ESPeasySerial(
        static_cast<ESPEasySerialPort>(_console_serial_port),
        _console_serial_rxpin,
        _console_serial_txpin);
    }
# if USES_ESPEASY_CONSOLE_FALLBACK_PORT

    if (Settings.console_serial0_fallback && (port != ESPEasySerialPort::serial0)) {
      if (!(activeTaskUseSerial0() || log_to_serial_disabled)) {
        _serial_fallback = new (std::nothrow) ESPeasySerial(
          ESPEasySerialPort::serial0,
          SOC_RX0,
          SOC_TX0);
      }
    }
# endif // if USES_ESPEASY_CONSOLE_FALLBACK_PORT
  }
#endif  // if FEATURE_DEFINE_SERIAL_CONSOLE_PORT

  _baudrate = baudrate;

  if ((_serial != nullptr) /*&& _serial->operator bool()*/) {
#if FEATURE_DEFINE_SERIAL_CONSOLE_PORT
    _serial->begin(baudrate);
    addLog(LOG_LEVEL_INFO, F("ESPEasy console using ESPEasySerial"));
#else // if FEATURE_DEFINE_SERIAL_CONSOLE_PORT
# ifdef ESP8266
    _serial->begin(baudrate);
    addLog(LOG_LEVEL_INFO, F("ESPEasy console using HW Serial"));
# endif // ifdef ESP8266
# ifdef ESP32

    // Allow to flush data from the serial buffers
    // When not opening the USB serial port, the ESP may hang at boot.
    delay(10);
    _serial->end();
    delay(10);
    _serial->begin(baudrate);
    _serial->flush();
# endif // ifdef ESP32
#endif  // if FEATURE_DEFINE_SERIAL_CONSOLE_PORT
  }
#if USES_ESPEASY_CONSOLE_FALLBACK_PORT

  if ((_serial_fallback != nullptr) /* && _serial_fallback->connected()*/) {
    _serial_fallback->begin(baudrate);
    addLog(LOG_LEVEL_INFO, F("ESPEasy console fallback enabled"));
  }
#endif // if USES_ESPEASY_CONSOLE_FALLBACK_PORT
}

void EspEasy_Console_t::init() {
  updateActiveTaskUseSerial0();

  if (!Settings.UseSerial) {
    return;
  }

#if !FEATURE_DEFINE_SERIAL_CONSOLE_PORT

  if (activeTaskUseSerial0() || log_to_serial_disabled) {
    return;
  }
#endif // if !FEATURE_DEFINE_SERIAL_CONSOLE_PORT

  begin(Settings.BaudRate);
}

void EspEasy_Console_t::loop()
{
  START_TIMER;
  auto port = getPort();

  if (port == nullptr) {
    return;
  }

  if (port->available())
  {
#if FEATURE_DEFINE_SERIAL_CONSOLE_PORT

    if (static_cast<ESPEasySerialPort>(_console_serial_port) == ESPEasySerialPort::serial0
# ifdef ESP8266
        || static_cast<ESPEasySerialPort>(_console_serial_port) == ESPEasySerialPort::serial0_swap
# endif // ifdef ESP8266
        )
#endif  // if FEATURE_DEFINE_SERIAL_CONSOLE_PORT
    {
      String dummy;

      if (PluginCall(PLUGIN_SERIAL_IN, 0, dummy)) {
        return;
      }
    }
  }
#if USES_ESPEASY_CONSOLE_FALLBACK_PORT

  if ((_serial_fallback != nullptr) &&
      _serial_fallback->connected() &&
      _serial_fallback->available())
  {
    String dummy;

    if (PluginCall(PLUGIN_SERIAL_IN, 0, dummy)) {
      return;
    }
  }
#endif // if USES_ESPEASY_CONSOLE_FALLBACK_PORT

#if FEATURE_DEFINE_SERIAL_CONSOLE_PORT

  // FIXME TD-er: Must add check whether SW serial may be using the same pins as Serial0
  if (!Settings.UseSerial) { return; }
#else // if FEATURE_DEFINE_SERIAL_CONSOLE_PORT

  if (!Settings.UseSerial || activeTaskUseSerial0()) { return; }
#endif // if FEATURE_DEFINE_SERIAL_CONSOLE_PORT

  while (port->available())
  {
    delay(0);
    const uint8_t SerialInByte = port->read();

    if (SerialInByte == 255) // binary data...
    {
      port->flush();
      return;
    }

    if (isprint(SerialInByte))
    {
      if (SerialInByteCounter < CONSOLE_INPUT_BUFFER_SIZE) { // add char to string if it still fits
        InputBuffer_Serial[SerialInByteCounter++] = SerialInByte;
      }
    }

    if ((SerialInByte == '\r') || (SerialInByte == '\n'))
    {
      if (SerialInByteCounter == 0) {              // empty command?
        break;
      }
      InputBuffer_Serial[SerialInByteCounter] = 0; // serial data completed
      addToSerialBuffer('>');
      addToSerialBuffer(String(InputBuffer_Serial));
      ExecuteCommand_all(EventValueSource::Enum::VALUE_SOURCE_SERIAL, InputBuffer_Serial);
      SerialInByteCounter   = 0;
      InputBuffer_Serial[0] = 0; // serial data processed, clear buffer
    }
  }
  STOP_TIMER(CONSOLE_LOOP);
}

void EspEasy_Console_t::addToSerialBuffer(const __FlashStringHelper *line)
{
  addToSerialBuffer(String(line));
}

void EspEasy_Console_t::addToSerialBuffer(char c)
{
  _serialWriteBuffer.add(c);
#if USES_ESPEASY_CONSOLE_FALLBACK_PORT

  if (_serial_fallback != nullptr) {
    _serial_fallback_WriteBuffer.add(c);
  }
#endif // if USES_ESPEASY_CONSOLE_FALLBACK_PORT
  process_serialWriteBuffer();
}

void EspEasy_Console_t::addToSerialBuffer(const String& line) {
  process_serialWriteBuffer();
  _serialWriteBuffer.add(line);
#if USES_ESPEASY_CONSOLE_FALLBACK_PORT

  if (_serial_fallback != nullptr) {
    _serial_fallback_WriteBuffer.add(line);
  }
#endif // if USES_ESPEASY_CONSOLE_FALLBACK_PORT
  process_serialWriteBuffer();
}

void EspEasy_Console_t::addNewlineToSerialBuffer() {
  _serialWriteBuffer.addNewline();
#if USES_ESPEASY_CONSOLE_FALLBACK_PORT

  if (_serial_fallback != nullptr) {
    _serial_fallback_WriteBuffer.addNewline();
  }
#endif // if USES_ESPEASY_CONSOLE_FALLBACK_PORT
  process_serialWriteBuffer();
}

bool EspEasy_Console_t::process_serialWriteBuffer() {
  START_TIMER;
  bool res = false;

  if ((_serial != nullptr)) {
    const int snip = _serial->availableForWrite();

    if  (snip > 0) {
      res = _serialWriteBuffer.write(*_serial, snip) != 0;
    }
  }


#if USES_ESPEASY_CONSOLE_FALLBACK_PORT

  if ((_serial_fallback != nullptr) && (_serial_fallback != _serial)) {
    const int snip = _serial_fallback->availableForWrite();

    if  (snip > 0) {
      if (_serial_fallback_WriteBuffer.write(*_serial_fallback, snip)) {
        res = true;
      }
    }
  }
#endif // if USES_ESPEASY_CONSOLE_FALLBACK_PORT

  STOP_TIMER(CONSOLE_WRITE_SERIAL);
  return res;
}

void EspEasy_Console_t::setDebugOutput(bool enable)
{
  auto port = getPort();

  if (port == nullptr) {
    port->setDebugOutput(enable);
  }
}

#if FEATURE_DEFINE_SERIAL_CONSOLE_PORT
ESPeasySerial * EspEasy_Console_t::getPort()
{
  if ((_serial != nullptr) && _serial->connected()) {
    return _serial;
  }
# if USES_ESPEASY_CONSOLE_FALLBACK_PORT

  if ((_serial_fallback != nullptr) && _serial_fallback->connected()) {
    return _serial_fallback;
  }
# endif // if USES_ESPEASY_CONSOLE_FALLBACK_PORT
  return nullptr;
}

#else // if FEATURE_DEFINE_SERIAL_CONSOLE_PORT
HardwareSerial * EspEasy_Console_t::getPort()
{
  if (_serial != nullptr) {
    return _serial;
  }
  return nullptr;
}

#endif // if FEATURE_DEFINE_SERIAL_CONSOLE_PORT


void EspEasy_Console_t::endPort()
{
  if (_serial != nullptr) {
    //    if (_serial->operator bool()) {
    _serial->end();

    //    }
  }
#if USES_ESPEASY_CONSOLE_FALLBACK_PORT

  if ((_serial_fallback != nullptr)) {
    _serial_fallback->end();
  }
#endif // if USES_ESPEASY_CONSOLE_FALLBACK_PORT
  delay(10);
}

int EspEasy_Console_t::availableForWrite()
{
  auto serial = getPort();

  if ((serial != nullptr) && serial->operator bool()) {
    return serial->availableForWrite();
  }
  return 0;
}
