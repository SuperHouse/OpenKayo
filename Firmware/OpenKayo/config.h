/* ----------------- General config -------------------------------- */
#define  ENABLE_KAYO       true   // Send commands to the target (turn off for experiments / debugging)
#define  REQUIRE_HOMING    true   // Refuse to move until a homing operation has been completed
#define  ENABLE_CAN        false

/* CAN bus */
//#define  CAN_BUS_SPEED         250E3
#define  J1939_UPSTREAM_ADDRESS   0x90   // Range for conveyors: 0x90-9F
#define  J1939_SOURCE_ADDRESS     0x80   // Range for PnP: 0x80-8F, 0xF0-F1 (0xF0-F1 are reserved)
#define  J1939_DOWNSTREAM_ADDRESS 0x91   // Range for conveyors: 0x90-9F
#define  UPSTREAM_LOAD_SPEED       800   // mm/min to run upstream conveyor
#define  DOWNSTREAM_LOAD_SPEED     800   // mm/min to run downstream conveyor


/* Serial */
#define  SERIAL_BAUD_RATE        115200  // Speed for USB serial console
#define  KAYO_BAUD_RATE          115200  // Speed for Kayo serial comms

/* Machine */
#define  KAYO_SCALE_FACTOR      (100.0)  // Multiply mm by this to give Kayo movement units
#define  KAYO_CONVEYOR_CAL        (2.0)  // Calibration adjustment for conveyor width
#define  KAYO_CONVEYOR_CLEARANCE   0.5   // Add this to conveyor width to prevent binding. Set to 0 for calibration.
#define  KAYO_CONVEYOR_OFFSET    565     // The Kayo seems to add this to the value sent
//#define  KAYO_TRACK_MIN_WIDTH    (57.5)
//#define  KAYO_TRACK_MAX_WIDTH   (300.0)
#define  KAYO_ROTATION_SCALE    (3200.0 / 360.0)
//#define  KAYO_X_MAX              (800.0)
//#define  KAYO_Y_MAX              (600.0)
//#define  RESPONSE_NOZZLEROTATION    9
#define  KAYO_N1_Z_STEP_BASE        850
#define  KAYO_Z_STEPS_PER_MM        110   // Machine 1
//#define  KAYO_Z_STEPS_PER_MM        220  // Machine 2
#define  COMMAND_DELAY                0   // ms delay after each command

/* Debugging */
#define  DEBUG_CAN               false
#define  DEBUG_KAYO              true   // Report debug messages relating to comms with the Kayo

/*
      self.ROTATION_SCALE_FACTOR = 3200.0 / 360.0    # 3200 steps / 360 degrees
      SMSG_NOZZLEROTATION_RESPONSE = 9
*/
/* ----------------- Hardware-specific config ---------------------- */
/* Kayo Serial */
#define  KAYO_RX_PIN               2 // 43
#define  KAYO_TX_PIN               1 // 44
//
/* I2C */
#define  I2C_SDA_PIN               5
#define  I2C_SCL_PIN               4

/* CAN bus */
#define  CAN_RX_PIN               17
#define  CAN_TX_PIN               16
