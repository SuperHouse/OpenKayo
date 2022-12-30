/*
  Kayo-A4 Pick-and-Place GCode Interface Firmware

  Written by Jonathan Oxer <jon@oxer.com.au>

  Inspired by a Python-based Kayo control system written by Glen English.

  Controlled using GCODE. Understands these codes:

  === Basic movement ===
  "G28"             Reset and home
  "G0 X-23.4 Y-18"  Move X & Y to the defined absolute position
  "G0 I-8.9"        Move nozzle 1 vertically      (I=1, J=2, K=3, L=4)
  "G0 A48"          Rotate nozzle 1 to 48 degrees (A=1, B=2, C=3, D=4)

  === Vac / Blow ===
  "M2 N2"           Close vac and open blow on nozzle 2
  "M3"              Close vac and blow on all nozzles
  "M4 N2"           Open blow on nozzle 2
  "M5 N1"           Close blow on nozzle 1
  "M6 N3"           Open vac on nozzle 3
  "M7 N3"           Close vac on nozzle 3

  === Conveyor Belt ===
  "M20"             Reset conveyor
  "M21 N93.4"       Adjust conveyor to 93.4mm width + margin
  "M22"             Ingest a PCB and clip it in position
  "M23"             Unclip PCB and move it to the exit location
  "M24"             Start conveyor
  "M25"             Stop conveyor and drop pin
  "M26"             Engages the stop pin
  "M27"             Clip a PCB on the conveyor

  === Lights ===
  "M30"             Turn off camera lights
  "M31 S15"         Turn on fast camera light at 15% brightness
  "M32 S46"         Turn on precision camera light at 46% brightness

  === Status ===
  "M114"            Report position
  "M115"            Report firmware type and version

  === Feeders ===
  "M600 N12"        Open feeder 12
  "M601"            Close feeders (no need to specify)

  === Diagnostic ===
  "M99 N1 S1234"    Send 1234 microsteps to N1


  X axis is horizontal left/right.
  Y axis is horizontal forward/back.
  Kayo home is top right corner, so the working area is negative coordinates
  relative to that.

  What to do about Z axis? Just 1 Z, or a separate Z for each nozzle? I/J/K/L?
  A-D are the axes used for rotation of nozzles 1-4.
  Ideally this firmware should support 8-nozzle machines, so allowing for
  8 x Z axes and 8 x rotation axes is best.

  No movement is allowed until the home sequence has been completed.

  Arduino IDE ESP32 board profile:
  Go into Preferences -> Additional Board Manager URLs
  Add:
   https://dl.espressif.com/dl/package_esp32_index.json

  Then go to Tools -> Board -> Boards Manager..., update, and add "esp32 by Espressif Systems"

  Arduino IDE settings under "Tools":
    - Board:            ESP32 Dev Module
    - Upload speed:     115200bps
    - CPU Speed:        240MHz
    - Flash Frequency:  40MHz
    - Flash Mode:       DOUT
    - Flash Size:       4MB
    - Partition Scheme: Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)
    - Core Debug Level: None
    - PSRAM:            Disabled

  To do:
    - Fix interaction between XY and Z axis movements.
    - Query machine for position report instead of using stored position.
    - Make received message parser resilient to missed characters.
    - Figure out why machines have different microstep counts on Z axis.
*/
#define VERSION "0.2-20221229"


/*--------------------------- Configuration ---------------------------------*/
// Configuration should be done in the included file:
#include "config.h"

/*--------------------------- Libraries -------------------------------------*/


/*--------------------------- Global Variables ------------------------------*/
uint8_t  g_homed                 = false;
uint8_t  g_kayo_send_buffer[8]   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t  g_kayo_recv_buffer[8]   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t  g_expected_response[8]  = {0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55};
//uint8_t  g_recv_buffer_position  = 0;
float    g_x_position            = 0.0;
float    g_y_position            = 0.0;
float    g_nozzle_angle[4]       = {0.0, 0.0, 0.0, 0.0};
float    g_nozzle_z_position[4]  = {0.0, 0.0, 0.0, 0.0};
uint8_t  g_matching_response     = false;
float    g_x_reported_position   = 0.0;
float    g_y_reported_position   = 0.0;

// g_position_message is maintained as a global, populated with the latest requested
// position values. This allows persistence of values that haven't been requested in
// a specific move. For example, "G0 X123" will move to 123,0. Then a subsequent
// "G0 Y321" would move to 123,321 because the previous X value is persisted.
uint8_t  g_position_message[8]   = {0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55};

#define  MAX_USB_INPUT             80 // Input buffer size

// General
uint64_t g_device_id;                 // Unique ID from ESP chip ID

/*--------------------------- Function Signatures ---------------------------*/
void sendMessageToKayo(uint8_t message[8], bool suppress_ok_response = false, bool exact_response_required = false);
void decodePositionMessage(uint8_t message[]);
void cmdResetAndHome();
void cmdMoveXY(float x, float y, bool suppress_ok_response = false);
void cmdMoveZ(uint8_t n, float z, bool suppress_ok_response = false);
void cmdRotateNozzle(uint8_t n, float a);

void cmdSetVacBlow(uint16_t command_code, uint8_t nozzle_id);

void cmdResetConveyor();
void cmdAdjustConveyorWidth(float width_requested);
void cmdRunConveyor();
void cmdStopConveyor();
void cmdStopPinEngageConveyor();
void cmdClipPcbConveyor();
void cmdExitPcbConveyor();
void cmdIngestPcbConveyor();

void cmdLightOn(uint8_t light, uint8_t brightness);
void cmdLightsOff();

void cmdReportPosition();

void cmdOpenFeeder(uint8_t feeder_id);
void cmdCloseFeeders();

void printKayoMessage(uint8_t message[]);
void listenToKayoSerialStream(bool suppress_ok_response = false, bool exact_match_required = false);
void clearExpectedResponse();
//void addMessageToQueue(uint8_t message[], uint8_t expected_response[]);

void cmdSendZSteps(uint8_t n, uint16_t z_microsteps);

/*--------------------------- Instantiate Global Objects --------------------*/


/*--------------------------- Program ---------------------------------------*/
/* Resources */
#include "gcode.h"
#include "serial_comms.h"
#include "machine_actions.h"

/*
  Setup
*/
void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.print("OpenKayo GCode adapter v");
  Serial.println(VERSION);

  // We need a unique device ID
  g_device_id = ESP.getEfuseMac();
  Serial.print("Device ID: ");
  Serial.println(g_device_id, HEX);

  if (ENABLE_KAYO) {
    Serial2.begin(KAYO_BAUD_RATE, SERIAL_8N1, KAYO_RX_PIN, KAYO_TX_PIN);
  }

  g_usb_input_buffer.reserve(MAX_USB_INPUT);
}

/*
   Main loop
*/
void loop()
{
  listenToUsbSerialStream();
  listenToKayoSerialStream();
}
