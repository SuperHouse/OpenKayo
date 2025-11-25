#ifndef H_GCODE
#define H_GCODE

String g_usb_input_buffer  = "";
String g_kayo_input_buffer = "";

#define GCODE_HOME                28
#define GCODE_MOVE                 0

// The MCode values for vacuum control are consecutive, so that we can
// do a range test later in the case statement:
#define MCODE_CLOSE_VAC_OPEN_BLOW  2   // "M2 N2"     Close vac and open blow on nozzle 2
#define MCODE_CLOSE_VAC_BLOW_ALL   3   // "M3"        Close vac and blow on all nozzles
#define MCODE_OPEN_BLOW            4   // "M4 N2"     Open blow on nozzle 2
#define MCODE_CLOSE_BLOW           5   // "M5 N1"     Close blow on nozzle 1
#define MCODE_OPEN_VAC             6   // "M6 N3"     Open vac on nozzle 3
#define MCODE_CLOSE_VAC            7   // "M7 N3"     Close vac on nozzle 3

#define MCODE_RESET_CONV          20   // "M20"       Reset conveyor
#define MCODE_ADJUST_CONV         21   // "M21 N93.4" Adjust conveyor to 93.4mm width + margin
#define MCODE_INGEST_PCB_MANUAL   22   // "M22"       Load a PCB manually placed and clip it in position
#define MCODE_EXIT_PCB_CONV       23   // "M23"       Unclip PCB and move it to the exit location
#define MCODE_RUN_CONV            24   // "M24"       Start conveyor
#define MCODE_STOP_CONV           25   // "M25"       Stop conveyor and drop pin
#define MCODE_PIN_ENGAGE_CONV     26   // "M26"       Engages the stop pin
#define MCODE_CLIP_PCB_CONV       27   // "M27"       Clip a PCB on the conveyor
#define MCODE_INGEST_PCB_CONV     28   // "M28"       Load a PCB from upstream conveyor and clip it in position

#define MCODE_LIGHTS_OFF          30   // "M30"       Turn off camera lights
#define MCODE_FAST_LIGHT_ON       31   // "M31 S15"   Turn on precision camera light at 15% brightness
#define MCODE_PREC_LIGHT_ON       32   // "M32 S46"   Turn on precision camera light at 46% brightness

#define MCODE_SPECIAL_Z           99   // "M99 N1 S1234" Send 1234 microsteps to N1
#define MCODE_REPORT_POSITION    114   // "M114"      Reports current position
#define MCODE_DISCOVER_FIRMWARE  115   // "M115"      Responds with firmware name and version

#define MCODE_OPEN_FEEDER        600   // "M600 N12 S4" Open feeder #12 4mm (1 increment) or "M600 N12 S8" for 8mm (2 increments)
#define MCODE_CLOSE_FEEDER       601   // "M601"      Close feeder (no need to specify)

/*
  Look for character "code" in the inputBuffer and read the value that follows it.
  Return the value found.  If nothing is found, "default_value" is returned.
*/
float parseGCodeParameter(char code, float default_value) {
  int code_position = g_usb_input_buffer.indexOf(code);
  if (code_position != -1)  // The code has been found in the buffer.
  {
    // Find the end of the number (separated by " " (space))
    int delimiter_position = g_usb_input_buffer.indexOf(" ", code_position + 1);
    float parsed_value = g_usb_input_buffer.substring(code_position + 1, delimiter_position).toFloat();
    return parsed_value;
  } else {
    return default_value;
  }
}


/*

*/
void processGCodeMessage()
{
  uint8_t valid_command_found = false;
  int16_t  command_code = -1;

  // Strip comments
  g_usb_input_buffer.remove(g_usb_input_buffer.indexOf(";"));
  g_usb_input_buffer.trim();
#if DEBUG_KAYO
  Serial.print("GCODE: ");
  Serial.println(g_usb_input_buffer);
#endif

  /*-- Check for G-code messages --*/
  // Extract the command, default -1 if not found
  command_code = parseGCodeParameter('G', -1);

  switch (command_code)
  {
    case GCODE_HOME:
      {
        valid_command_found = true;
        //Serial.println("Homing start");
        //Serial.println("ok");
        sendHomeToCAN();
        cmdResetAndHome();
        g_homed = true;
        break;
      }

    case GCODE_MOVE:
      {
        valid_command_found = true;
        if (REQUIRE_HOMING) {
          // Don't move unless we've already been homed.
          if (false == g_homed)
          {
            Serial.println("ERR: home the machine first using command 'G28'");
            break;
          }
        }

        float requested_x = parseGCodeParameter('X', NULL);
        float requested_y = parseGCodeParameter('Y', NULL);
        float requested_i = parseGCodeParameter('I', -9999);  // Nozzle 1 Z axis
        float requested_j = parseGCodeParameter('J', -9999);  // Nozzle 2 Z axis
        float requested_k = parseGCodeParameter('K', -9999);  // Nozzle 3 Z axis
        float requested_l = parseGCodeParameter('L', -9999);  // Nozzle 4 Z axis

        if (requested_x || requested_y) {
          // Check if Z is also being set:
          if (-9999 != requested_i || -9999 != requested_j || -9999 != requested_k || -9999 != requested_l)
          {
            cmdMoveXY(requested_x, requested_y, true);  // Suppress "ok" response
          } else {
            cmdMoveXY(requested_x, requested_y);
          }
        }

        // Move any Z axis that's part of the request
        if (-9999 != requested_i) {
          cmdMoveZ(1, requested_i);
        }
        if (-9999 != requested_j) {
          cmdMoveZ(2, requested_j);
        }
        if (-9999 != requested_k) {
          cmdMoveZ(3, requested_k);
        }
        if (-9999 != requested_l) {
          cmdMoveZ(4, requested_l);
        }

        // Move any rotation axis that's part of the request
        float requested_a = parseGCodeParameter('A', -9999);
        if (-9999 != requested_a) {
          cmdRotateNozzle(1, requested_a);
        }
        float requested_b = parseGCodeParameter('B', -9999);
        if (-9999 != requested_b) {
          cmdRotateNozzle(2, requested_b);
        }
        float requested_c = parseGCodeParameter('C', -9999);
        if (-9999 != requested_c) {
          cmdRotateNozzle(3, requested_c);
        }
        float requested_d = parseGCodeParameter('D', -9999);
        if (-9999 != requested_d) {
          cmdRotateNozzle(4, requested_d);
        }
        break;
      }
  }

  /*-- Check for M-code messages --*/
  // Extract the command, default -1 if not found
  command_code = parseGCodeParameter('M', -1);

  switch (command_code)
  {
    case MCODE_SPECIAL_Z:
      {
        valid_command_found = true;
        int32_t requested_steps = parseGCodeParameter('S', -9999);
        uint8_t requested_nozzle = parseGCodeParameter('N', 1);
        if (requested_steps) {
          cmdSendZSteps(requested_nozzle, requested_steps);
          break;
        }
        break;
      }

    // Report the firmware type and version to assist OpenPnP with autodiscovery
    // FIRMWARE_NAME:Marlin 1.1.0 (Github) SOURCE_CODE_URL:https://github.com/MarlinFirmware/Marlin PROTOCOL_VERSION:1.0 MACHINE_TYPE:RepRap EXTRUDER_COUNT:1 UUID:cede2a2f-41a2-4748-9b12-c55c62f367ff
    case MCODE_DISCOVER_FIRMWARE:
      {
        valid_command_found = true;
        Serial.print("FIRMWARE_NAME:OpenKayo ");
        Serial.print(VERSION);
        Serial.print(" SOURCE_CODE_URL:https://github.com/SuperHouse/OpenKayo");
        Serial.print(" PROTOCOL_VERSION:1.0");
        Serial.print(" MACHINE_TYPE:Kayo-A4");
        Serial.print(" UUID:");
        Serial.println(g_device_id, HEX);
        Serial.println("ok");
        break;
      }

    case MCODE_REPORT_POSITION:
      {
        valid_command_found = true;
        cmdReportPosition();
        break;
      }

    case MCODE_RESET_CONV:
      {
        valid_command_found = true;
        cmdResetConveyor();
        break;
      }

    case MCODE_ADJUST_CONV:
      {
        valid_command_found = true;
        float requested_width = parseGCodeParameter('N', NULL);
        sendConveyorWidthToCAN(requested_width);
        cmdAdjustConveyorWidth(requested_width);
        break;
      }

    case MCODE_RUN_CONV:
      {
        valid_command_found = true;
        cmdRunConveyor();
        break;
      }

    case MCODE_STOP_CONV:
      {
        valid_command_found = true;
        cmdStopConveyor();
        break;
      }

    case MCODE_PIN_ENGAGE_CONV:
      {
        valid_command_found = true;
        cmdStopPinEngageConveyor();
        break;
      }

    case MCODE_CLIP_PCB_CONV:
      {
        valid_command_found = true;
        cmdClipPcbConveyor();
        break;
      }

    case MCODE_EXIT_PCB_CONV:
      {
        valid_command_found = true;
        cmdExitPcbConveyor();
        break;
      }

    case MCODE_INGEST_PCB_MANUAL:
      {
        valid_command_found = true;
        cmdIngestPcbManual();
        break;
      }
      
    case MCODE_INGEST_PCB_CONV:
      {
        valid_command_found = true;
        cmdIngestPcbConveyor();
        break;
      }

    case MCODE_FAST_LIGHT_ON:
      {
        valid_command_found = true;
        uint8_t brightness = parseGCodeParameter('S', -1);
        cmdLightOn(1, brightness);
        break;
      }

    case MCODE_PREC_LIGHT_ON:
      {
        valid_command_found = true;
        uint8_t brightness = parseGCodeParameter('S', -1);
        cmdLightOn(2, brightness);
        break;
      }

    case MCODE_LIGHTS_OFF:
      {
        valid_command_found = true;
        cmdLightsOff();
        break;
      }


    // These MCodes have been strategically chosen to match the actual
    // byte value sent to the Kayo, so we can use the requested code
    // to form the message:
    case MCODE_CLOSE_VAC_OPEN_BLOW ... MCODE_CLOSE_VAC:
      {
        valid_command_found = true;
        uint8_t nozzle_id = parseGCodeParameter('N', NULL);
        cmdSetVacBlow(command_code, nozzle_id);
        break;
      }

    case MCODE_OPEN_FEEDER:
      {
        valid_command_found = true;
        uint8_t feeder_id = parseGCodeParameter('N', NULL);
        uint8_t feeder_distance = parseGCodeParameter('S', 4);
        cmdOpenFeeder(feeder_id);
        break;
      }

    case MCODE_CLOSE_FEEDER:
      {
        valid_command_found = true;
        cmdCloseFeeders();
        break;
      }
  }

  if (!valid_command_found)
  {
    Serial.println("ERR: Unknown or empty command ignored");
  }
}

#endif H_GCODE
