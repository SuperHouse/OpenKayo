/* ----------------- General config -------------------------------- */
#define  ENABLE_KAYO       true
#define  REQUIRE_HOMING   false
#define  COMMS_DEBUGGING   true

/* CAN bus */
#define  CAN_BUS_SPEED            500E3  // 500kbps

/* Serial */
#define  SERIAL_BAUD_RATE        115200  // Speed for USB serial console
#define  KAYO_BAUD_RATE          115200  // Speed for Kayo serial comms

/* Machine */
#define  KAYO_SCALE_FACTOR      (100.0)  // Multiply mm by this to give Kayo movement units
#define  KAYO_CONVEYOR_CAL        (2.0)  // Calibration djustment for conveyor width
#define  KAYO_CONVEYOR_CLEARANCE   0.5   // Add this to conveyor width to prevent binding. Set to 0 for calibration.
#define  KAYO_CONVEYOR_OFFSET    565     // The Kayo seems to add this to the value sent
//#define  KAYO_TRACK_MIN_WIDTH    (57.5)
//#define  KAYO_TRACK_MAX_WIDTH   (300.0)
#define  KAYO_ROTATION_SCALE    (3200.0 / 360.0)
//#define  KAYO_X_MAX              (800.0)
//#define  KAYO_Y_MAX              (600.0)
//#define  RESPONSE_NOZZLEROTATION    9
#define  KAYO_N1_Z_STEP_BASE        850
// #define  KAYO_Z_STEPS_PER_MM        220  // Machine 3
#define  KAYO_Z_STEPS_PER_MM        110   // Machine 1
#define  COMMAND_DELAY                0   // ms delay after each command

/*
      self.ROTATION_SCALE_FACTOR = 3200.0 / 360.0    # 3200 steps / 360 degrees
      SMSG_NOZZLEROTATION_RESPONSE = 9
*/
/* ----------------- Hardware-specific config ---------------------- */
/* Serial */
#define  KAYO_RX_PIN            16
#define  KAYO_TX_PIN            17

/* I2C */
#define  I2C_SDA_PIN            18
#define  I2C_SCL_PIN            19

/* CAN bus */
#define  CAN_RX_PIN              4
#define  CAN_TX_PIN              5
