#ifndef H_MACHINEACTIONS
#define H_MACHINEACTIONS

void cmdResetAndHome()
{
  uint8_t message[8]   = {0xEE, 0xBB, 0x05, 0x00, 0x00, 0x02, 0x00, 0x55};
  g_expected_response[0] = 0xAA;
  g_expected_response[1] = 0xFF;
  //g_expected_response[2] = 0x05;
  //g_expected_response[5] = 0x02;
  //g_expected_response[6] = 0x11;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdResetAndHome");
#endif
  sendMessageToKayo(message);

  g_position_message[1] = 0x00;
  g_position_message[2] = 0x00;
  g_position_message[3] = 0x00;
  g_position_message[4] = 0x00;
}


/* ******************* BASIC MOVEMENT ***************** */
/*
   Move the gantry to an X/Y location.
   WARNING: The Kayo WILL NOT MOVE X OR Y while any nozzle is extended AT ALL!
   We have to explicitly retract all nozzles before attempting an X/Y movement.
*/

void decodePositionMessage(uint8_t message[])
{
  uint16_t x_byte_0 = message[2];
  uint16_t x_byte_1 = message[3];
  uint16_t x_raw_pos = (x_byte_1 << 8) + x_byte_0;
  float current_x_position = -1 * (x_raw_pos / KAYO_SCALE_FACTOR);

  uint16_t y_byte_0 = message[4];
  uint16_t y_byte_1 = message[5];
  uint16_t y_raw_pos = (y_byte_1 << 8) + y_byte_0;
  float current_y_position = -1 * (y_raw_pos / KAYO_SCALE_FACTOR);
#if COMMS_DEBUGGING
  Serial.print("Decoded position: ");
  Serial.print(current_x_position);
  Serial.print(",");
  Serial.println(current_y_position);
#endif
  g_x_reported_position = current_x_position;
  g_y_reported_position = current_y_position;
}

void cmdMoveXY(float x, float y, bool suppress_ok_response)
{
#if COMMS_DEBUGGING
  Serial.print(millis()); Serial.print("    ");
  Serial.println("f: cmdMoveXY()");
  Serial.print("Going to X: ");
  Serial.println(x);
  Serial.print("Going to Y: ");
  Serial.println(y);
#endif
  if (0.0 != g_nozzle_z_position[0])
  {
    cmdMoveZ(1, 0, true);
    //delay(200);
  }
  if (0.0 != g_nozzle_z_position[1])
  {
    cmdMoveZ(2, 0, true);
    //delay(100);
  }
  if (0.0 != g_nozzle_z_position[2])
  {
    cmdMoveZ(3, 0, true);
    //delay(100);
  }
  if (0.0 != g_nozzle_z_position[3])
  {
    cmdMoveZ(4, 0, true);
    //delay(100);
  }

#if COMMS_DEBUGGING
  Serial.print(millis()); Serial.print("    ");
  Serial.print("Pos message1: ");
#endif
  printKayoMessage(g_position_message);
  if (NULL != x)
  {
    g_x_position = x;
    float kayo_x_requested = -1 * x * KAYO_SCALE_FACTOR;
    uint16_t kayo_x_dest = (int)kayo_x_requested;
    g_position_message[1] = kayo_x_dest >> 8;
    g_position_message[2] = kayo_x_dest & 0xFF;
  }
  if (NULL != y)
  {
    g_y_position = y;
    float kayo_y_requested = -1 * y * KAYO_SCALE_FACTOR;
    uint16_t kayo_y_dest = (int)kayo_y_requested;
    g_position_message[3] = kayo_y_dest >> 8;
    g_position_message[4] = kayo_y_dest & 0xFF;
  }

  //g_position_message[6] = 0x01;
  g_position_message[6] = 0x04;
  //g_position_message[6] = 0x06;
  //g_position_message[6] = 0x07;  // Crazy warp speed. Can make parts move on the nozzle tip.
  // Do we need to set byte 6 for a speed value? We're currently
  // leaving it as 0x00, but Glen's spreadsheet shows different
  // values being sent. Glen's code does this:
  // def Throw(self,XYpos : tuple, nozzleName : str):
  //   self.DoToolToXY( C4d(XYpos), 1 , movetype = 'NORMAL' )
  // def GotoXYAbsPos(self,xy,speed,movetype  ):# take lists of things to move.
  //   b=self.km.makeXYabsMsg(xy,speed,movetype)
  // def makeXYabsMsg(self, xy: tuple , speed: int, movetype='NORMAL'):
  //   b[6] = (self.ABSMOVETYPES[movetype.upper()] << 4) | speed
  // So Glen's code sets speed to 1 (byte 6 = 0x01) for "throw" operations.
  // His spreadsheet also shows speed of 4 for most movements, and 6 for
  // the first movement after loading a board.

  g_expected_response[0] = 0xAA;
  g_expected_response[1] = 0xA1; // OR g_expected_response[1] = 0xEE; Which?
  g_expected_response[2] = 0x00;
  g_expected_response[3] = 0x00;
  g_expected_response[4] = 0x00;
  g_expected_response[5] = 0x00;
  //g_expected_response[6] = 0x00;
  //g_expected_response[2] = g_position_message[2];
  //g_expected_response[3] = g_position_message[1];
  //g_expected_response[4] = g_position_message[4];
  //g_expected_response[5] = g_position_message[3];
  g_expected_response[6] = 0x00;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis()); Serial.print("    ");
  Serial.print("Pos message2: ");
  printKayoMessage(g_position_message);
#endif
  sendMessageToKayo(g_position_message, suppress_ok_response, false);
}


/*
   Move the specified nozzle (1-4) to the requested height
   Example:
     CC 04 1D 00 00 00 14 55
     04 1D is position.
     14 is nozzle 1
   Do we need to keep track of Z position? Currently unknown. Moving
   one nozzle causes the other nozzle in the pair to fully retract, so
   we lose track of where they are. This may cause OpenPnP to think a
   nozzle is extended when it's not. This may or may not be a problem.
   Also, we forcibly retract all nozzles before X or Y moves, because
   the Kayo will refuse to move otherwise.
*/
void cmdMoveZ(uint8_t nozzle, float z, bool suppress_ok_response)
{
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdMoveZ()");
#endif

  if (-9999.00 == z)
  {
    return;
  }

  uint16_t z_microsteps = KAYO_N1_Z_STEP_BASE + (abs(z) * KAYO_Z_STEPS_PER_MM);

  uint8_t message[8]  = {0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x55};  // Byte 6 sets speed to 4
  message[1] = z_microsteps >> 8;
  message[2] = z_microsteps & 0xFF;
  if (0 == z)
  {
#if COMMS_DEBUGGING
    Serial.println("Special-casing zero position");
#endif
    message[1] = 0x00;
    message[2] = 0x00;
  }

  switch (nozzle)
  {
    case 1:
      g_nozzle_z_position[0] = z;
      g_nozzle_z_position[1] = 0.0;  // Nozzles 1 and 2 are ganged
      message[6] = 0x14;
      break;
    case 2:
      g_nozzle_z_position[0] = 0.0;  // Nozzles 1 and 2 are ganged
      g_nozzle_z_position[1] = z;
      message[6] = 0x24;
      break;
    case 3:
      g_nozzle_z_position[2] = z;
      g_nozzle_z_position[3] = 0.0;  // Nozzles 3 and 4 are ganged
      message[6] = 0x34;
      break;
    case 4:
      g_nozzle_z_position[2] = 0.0;  // Nozzles 3 and 4 are ganged
      g_nozzle_z_position[3] = z;
      message[6] = 0x44;
      break;
  }

  g_expected_response[0] = 0xAA;
  g_expected_response[1] = 0xA2;
  g_expected_response[2] = message[2];
  g_expected_response[3] = message[1];
  g_expected_response[6] = nozzle;
  g_expected_response[7] = 0x55;

  sendMessageToKayo(message, suppress_ok_response);
#if COMMS_DEBUGGING
  Serial.println("z sent");
#endif
}


/*
  Diagnostic function to send a specific number of microsteps to a
  nozzle Z axis stepper.
*/
void cmdSendZSteps(uint8_t nozzle, uint16_t z_microsteps)
{
  if (-9999 == z_microsteps)
  {
    z_microsteps = 0;
  }
#if COMMS_DEBUGGING
  Serial.print("Microsteps:");
  Serial.println(z_microsteps);
#endif

  uint8_t message[8]  = {0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x55};
  message[1] = z_microsteps >> 8;
  message[2] = z_microsteps & 0xFF;

  switch (nozzle)
  {
    case 1:
      message[6] = 0x14;
      break;
    case 2:
      message[6] = 0x24;
      break;
    case 3:
      message[6] = 0x34;
      break;
    case 4:
      message[6] = 0x44;
      break;
  }
  g_expected_response[0] = 0xAA;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdSendZSteps");
#endif
  sendMessageToKayo(message);
}


/*
   Rotate the specified nozzle (1-4) to the requested angle.

   The Kayo doesn't have a sense of absolute position. It takes a
   direction and a distance to move, in microsteps. We have to keep
   track of position ourselves, and calculate the delta to send to
   the machine.

   Example:
     DD BB 00 00 00 01 11 55
     Bytes 2/3 00 00 is microsteps.
     Byte 5 01 is clockwise, 00 is counterclockwise.
     Byte 6 11 is nozzle 1.

   self.ROTATION_SCALE_FACTOR = 3200.0 / 360.0   # 3200 steps / 360 degrees
   SMSG_NOZZLEROTATION_RESPONSE = 9

  def RotateNozzle(self,angle : float, nozzleName : str , direction : str , speed=1):
    b=self.km.makeNozzleRotationMsg(angle,nozzleName.lower(),direction,int(speed))  #type1 0 = CW,  (above), 1 = CCW (above)
    self.SendBA(b,1.0)
    self.SwallowDecodeUntilMsg([CkayoMessageDecoder.SMSG_NOZZLEROTATION_RESPONSE])

  def makeNozzleRotationMsg(self, rotation : float , nozzlename : str  , direction : str,speed : int ):
    b = self.NozzRotation.copy()
    zaxisKayo = g.md.headZNameDict[nozzlename][0]

    # rotation appears to be 8 x microsteps, and 200 steps per revolution

    rb = int((rotation * g.md.ROTATION_SCALE_FACTOR)+0.5 ).to_bytes(length=2, byteorder='big')
    b[2:4] = rb

    if direction == 'CW' :  b[5] = 1
    else : b[5] = 0

    b[6] = (speed << 4 ) | (int(zaxisKayo) & 0xf)
    return b
*/
void cmdRotateNozzle(uint8_t nozzle_number, float nozzle_angle)
{
  uint8_t message[8] = {0xDD, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55};

  float nozzle_delta = nozzle_angle - g_nozzle_angle[nozzle_number - 1];
  g_nozzle_angle[nozzle_number - 1] = nozzle_angle;

  uint16_t kayo_a_steps = int((abs(nozzle_delta) * KAYO_ROTATION_SCALE) + 0.5);  // Convert angular degrees to microsteps
  message[2] = kayo_a_steps >> 8;
  message[3] = kayo_a_steps & 0xFF;

  uint8_t nozzle_direction = 0x00; // Clockwise (from above)
  if (nozzle_delta < 0)
  {
    nozzle_direction = 0x01; // Counterclockwise (from above)
  }
  message[5] = nozzle_direction;

  switch (nozzle_number) // Nozzle number
  {
    case 1:
      message[6] = 0x11;
      break;
    case 2:
      message[6] = 0x12;
      break;
    case 3:
      message[6] = 0x13;
      break;
    case 4:
      message[6] = 0x14;
      break;
  }

  g_expected_response[0] = 0xAA;
  //g_expected_response[1] = 0xBB;
  //g_expected_response[6] = message[6];
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdRotateNozzle");
#endif
  sendMessageToKayo(message);
}


/*
   Respond with the most recent X, Y, and Z positions. This is
   a basic implementation that uses the values stored internally.
   It does not query the machine. In future, this should query
   the machine.
*/
void cmdReportPosition()
{
  Serial.print("MPos:X:");
  Serial.print(g_x_reported_position);
  Serial.print(" Y:");
  Serial.print(g_y_reported_position);
  Serial.print(" I:");
  Serial.print(g_nozzle_z_position[0]);
  Serial.print(" J:");
  Serial.print(g_nozzle_z_position[1]);
  Serial.print(" K:");
  Serial.print(g_nozzle_z_position[2]);
  Serial.print(" L:");
  Serial.print(g_nozzle_z_position[3]);
  Serial.print(" A:");
  Serial.print(g_nozzle_angle[0]);
  Serial.print(" B:");
  Serial.print(g_nozzle_angle[1]);
  Serial.print(" C:");
  Serial.print(g_nozzle_angle[2]);
  Serial.print(" D:");
  Serial.print(g_nozzle_angle[3]);
  Serial.println();
  //Serial.println("ok");
}

/* ******************* CONVEYOR ***************** */
/*
  EE BB 06 00 00 02 00 55
*/
void cmdResetConveyor()
{
  uint8_t message[8] = {0xEE, 0xBB, 0x06, 0x00, 0x00, 0x02, 0x00, 0x55};
  g_expected_response[0] = 0xAA;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdResetConveyor");
#endif
  sendMessageToKayo(message);
  //memcpy(g_expected_response, message, sizeof(g_expected_response));
  //swallowDecodeUntilMessage(g_expected_response);
}


/*
  EE DD 00 00 00 00 00 55
*/
void cmdAdjustConveyorWidth(float width_requested)
{
  if (NULL != width_requested)
  {
    uint8_t message[8] = {0xEE, 0xDD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55};
    float width_in_kayo_units = (width_requested + KAYO_CONVEYOR_CAL + KAYO_CONVEYOR_CLEARANCE) * 10;  // Kayo uses 0.1mm units for conveyor width
    uint16_t width_dest = (int)width_in_kayo_units;
    width_dest -= KAYO_CONVEYOR_OFFSET;
    message[2] = width_dest >> 8;
    message[3] = width_dest & 0xFF;

    g_expected_response[0] = 0xAA;
    g_expected_response[7] = 0x55;

#if COMMS_DEBUGGING
    Serial.print(millis());
    Serial.println("  f: cmdAdjustConveyorWidth");
#endif
    sendMessageToKayo(message);

    //g_expected_response[1] = 0xDD;
    //memcpy(g_expected_response, message, sizeof(g_expected_response));
    //swallowDecodeUntilMessage(g_expected_response);
  } else {
    Serial.println("ERR: No conveyor width specified");
  }
}


/*
  EE CC 01 00 00 00 00 55
*/
void cmdRunConveyor()
{
  uint8_t message[8] = {0xEE, 0xCC, 0x01, 0x00, 0x00, 0x00, 0x00, 0x55};
  g_expected_response[0] = 0xAA;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdRunConveyor");
#endif
  sendMessageToKayo(message);
  //memcpy(g_expected_response, message, sizeof(g_expected_response));
  //swallowDecodeUntilMessage(g_expected_response);
}


/*
  EE CC 00 07 00 00 00 55
*/
void cmdStopConveyor()
{
  uint8_t message[8] = {0xEE, 0xCC, 0x00, 0x07, 0x00, 0x00, 0x00, 0x55};
  g_expected_response[0] = 0xAA;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdStopConveyor");
#endif
  sendMessageToKayo(message);
  //memcpy(g_expected_response, message, sizeof(g_expected_response));
  //swallowDecodeUntilMessage(g_expected_response);
}


/*
  EE CC 00 06 00 00 00 55
*/
void cmdStopPinEngageConveyor()
{
  uint8_t message[8] = {0xEE, 0xCC, 0x00, 0x06, 0x00, 0x00, 0x00, 0x55};
  g_expected_response[0] = 0xAA;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdStopPinEngageConveyor");
#endif
  sendMessageToKayo(message);
  //memcpy(g_expected_response, message, sizeof(g_expected_response));
  //swallowDecodeUntilMessage(g_expected_response);
}


/*
  EE CC 00 01 00 00 00 55
*/
void cmdClipPcbConveyor()
{
  uint8_t message[8] = {0xEE, 0xCC, 0x00, 0x01, 0x00, 0x00, 0x00, 0x55};
  g_expected_response[0] = 0xAA;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdClipPcbConveyor");
#endif
  sendMessageToKayo(message);
  //memcpy(g_expected_response, message, sizeof(g_expected_response));
  //swallowDecodeUntilMessage(g_expected_response);
}


/*
  Send: EE CC 02 00 00 02 00 55
  Recv: AA CC 02 00 00 02 00 55
*/
void cmdExitPcbConveyor()
{
  uint8_t message[8] = {0xEE, 0xCC, 0x02, 0x00, 0x00, 0x02, 0x00, 0x55};
  g_expected_response[0] = 0xAA;
  g_expected_response[1] = 0xCC;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdexitPcbConveyor");
#endif
  sendMessageToKayo(message);
}

/*
  // self.ConveyorClear()

    self.SendBA(self.km.Oddball,1.0) # move camera over PCB.
    self.SwallowDecodeUntilMsg([CkayoMessageDecoder.SMSG_ANY])

  //   self.SendBA(self.km.ConveyorStep1,1.0) // Activates some solenoid
  //  self.SwallowDecodeUntilMsg([CkayoMessageDecoder.SMSG_ANY])

    #WAIT FOR pcb
  //  if self.WaitForInputPCB(30.0) == -1 :
  //    logger.error('PCB not presented within time allocated')
  //    self.ConveyorClear()
  //    return -1


    logger.info('got PCB')

   // self.SendBA(self.km.ConveyorStep2,1.0)
   // self.SwallowDecodeUntilMsg([CkayoMessageDecoder.SMSG_ANY])

   // self.ConveyorEngageStopPin()

   // logger.info('ConveyorRun1')
   // self.SendBA(self.km.ConveyorRun,1.0)
   // self.SwallowDecodeUntilMsg([CkayoMessageDecoder.SMSG_ANY])

    logger.info('Waiting  conveyor')
  //  time.sleep(2.5)
    #when conveyor gets RUN for second time, it slows down
    logger.info('ConveyorRun2')
  //  self.SendBA(self.km.ConveyorRun,1.0)
    self.SwallowDecodeUntilMsg([CkayoMessageDecoder.SMSG_ANY])

    time.sleep(0.5)

    logger.info('stopping conveyor')
    self.ConveyorClear()

    self.SendBA(self.km.ConveyorStop,1.0)
    self.SwallowDecodeUntilMsg([CkayoMessageDecoder.SMSG_ANY])

    self.ConveyorClipPCB()
*/
void cmdIngestPcbConveyor()
{
  // ConveyorClear()
  // Send: EE CC 00 00 00 00 00 55
  // Recv: AA CC 00 00 00 00 00 55
  uint8_t message0[] = {0xEE, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55};
  g_expected_response[0] = 0xAA;
  g_expected_response[1] = 0xCC;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdIngestPcbConveyor 1");
#endif
  sendMessageToKayo(message0);
  delay(100);

  // Conveyor Step 1
  // Send: EE CC 00 02 00 00 00 55
  // Recv: AA CC 00 02 00 00 00 55
  uint8_t message1[8] = {0xEE, 0xCC, 0x00, 0x02, 0x00, 0x00, 0x00, 0x55};
  g_expected_response[0] = 0xAA;
  g_expected_response[1] = message1[1];
  g_expected_response[3] = message1[3];
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdIngestPcbConveyor 2");
#endif
  sendMessageToKayo(message1);
  delay(100);

  // WaitForInputPCB
  // Send: FF 00 00 00 00 00 00 55 <- check PCB on conveyor status
  // Recv: AA A7 00 01 00 00 00 55 <- 01 in byte 4 means PCB found
  uint8_t message9[8] = {0xEE, 0xCC, 0x00, 0x02, 0x00, 0x00, 0x00, 0x55};
  g_expected_response[0] = 0xAA;
  //g_expected_response[1] = 0xA7;
  //g_expected_response[3] = 0x01;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdIngestPcbConveyor 3");
#endif
  sendMessageToKayo(message9);
  // After we get the 01 (PCB present) response, wait 3 seconds:
  delay(3000); // Why this? It's in Glen's code

  // Conveyor Step 2
  // Send: EE CC 00 03 00 00 00 55
  // Recv: AA CC 00 03 00 00 00 55
  uint8_t message2[] = {0xEE, 0xCC, 0x00, 0x03, 0x00, 0x00, 0x00, 0x55};
  g_expected_response[0] = 0xAA;
  g_expected_response[1] = message2[1];
  g_expected_response[3] = message2[3];
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdIngestPcbConveyor 4");
#endif
  sendMessageToKayo(message2);
  delay(100);

  // ConveyorEngageStopPin()
  // Send: EE CC 00 06 00 00 00 55
  // Recv: AA CC 00 06 00 00 00 55
  uint8_t message3[] = {0xEE, 0xCC, 0x00, 0x06, 0x00, 0x00, 0x00, 0x55};
  g_expected_response[0] = 0xAA;
  g_expected_response[1] = message3[1];
  g_expected_response[3] = message3[3];
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdIngestPcbConveyor 5");
#endif
  sendMessageToKayo(message3);
  delay(100);

  // ConveyorRun()
  // Send: EE CC 01 00 00 00 00 55
  // Recv: AA 00 00 06 00 00 00 55
  uint8_t message4[] = {0xEE, 0xCC, 0x01, 0x00, 0x00, 0x00, 0x00, 0x55};
  g_expected_response[0] = 0xAA;
  g_expected_response[3] = 0x06;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdIngestPcbConveyor 6");
#endif
  sendMessageToKayo(message4);

  // sleep(2.5)
  delay(2500);

  // when conveyor gets RUN for second time, it slows down
  // ConveyorRun()
  // Send: EE CC 01 00 00 00 00 55
  // Recv: AA 00 00 06 00 00 00 55
  //uint8_t message4[] = {0xEE, 0xCC, 0x01, 0x00, 0x00, 0x00, 0x00, 0x55};
  g_expected_response[0] = 0xAA;
  g_expected_response[3] = 0x06;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdIngestPcbConveyor 7");
#endif
  sendMessageToKayo(message4);

  // sleep(0.5)
  delay(500);

  // ConveyorClear()
  // Send: EE CC 00 00 00 00 00 55
  // Recv: AA CC 00 00 00 00 00 55
  uint8_t message6[] = {0xEE, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55};
  g_expected_response[0] = 0xAA;
  g_expected_response[1] = 0xCC;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdIngestPcbConveyor 8");
#endif
  sendMessageToKayo(message6);
  delay(100);

  // ConveyorStop()
  // Send: EE CC 00 07 00 00 00 55
  // Recv: AA CC 00 07 00 00 00 55
  uint8_t message7[] = {0xEE, 0xCC, 0x00, 0x07, 0x00, 0x00, 0x00, 0x55};
  g_expected_response[0] = 0xAA;
  g_expected_response[1] = 0xCC;
  g_expected_response[3] = 0x07;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdIngestPcbConveyor 9");
#endif
  sendMessageToKayo(message7);
  delay(100);

  // ConveyorClipPCB()
  // Send: EE CC 00 01 00 00 00 55
  // Recv: AA CC 00 01 00 00 00 55
  uint8_t message8[] = {0xEE, 0xCC, 0x00, 0x01, 0x00, 0x00, 0x00, 0x55};
  g_expected_response[0] = 0xAA;
  g_expected_response[1] = 0xCC;
  g_expected_response[3] = 0x01;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdIngestPcbConveyor 10");
#endif
  sendMessageToKayo(message8);
  delay(100);
}

/* ******************* VAC / BLOW ***************** */
/*
   Values in third byte:
  1: DO NOT USE, DANGER! (Don't know why: this was in Glen's notes)
  2: Close vac, open blow on N          'DD CC 02 00 00 00 00 55'
  3: Close vac and blow on all nozzles  'DD CC 03 00 00 00 00 55'
  4: Open blow on N                     'DD CC 04 00 00 00 00 55'
  5: Close blow on N                    'DD CC 05 00 00 00 00 55'
  6: Open vac on N                      'DD CC 06 00 00 00 00 55'
  7: Close vac on N                     'DD CC 07 00 00 00 00 55'
   Expected response messages:
  SMSG_OPENBLOWN = 14
  SMSG_CLOSEVACN = 12
  SMSG_CLOSEVACN = 13
*/
void cmdSetVacBlow(uint16_t command_code, uint8_t nozzle_id)
{
  uint8_t message[8] = {0xDD, 0xCC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55};
  // command_code is a uint16_t, but we need to ensure a single byte is used
  if (command_code >= 2 && command_code <= 7)
  {
    message[2] = command_code;
  }

  if (3 != command_code) // Exclude the "all" command, which doesn't need a nozzle ID
  {
    if (nozzle_id >= 1 && nozzle_id <= 4) // Apply the nozzle ID to the message
    {
      message[6] = nozzle_id;
    } else {
      Serial.println("ERR: No valid nozzle ID specified");
      return;
    }
  }

  // Open suppliment valve if we're going to blow off a part. Do this before
  // opening the valve for the specific nozzle.
  if (2 == command_code || 4 == command_code) // We want to blow off a part
  {
    openSupplimentValve();
    delay(25); // Give the valve time to actuate.
  }

  g_expected_response[0] = 0xAA;
  g_expected_response[1] = message[1];
  g_expected_response[2] = message[2];
  g_expected_response[3] = 0x00;
  g_expected_response[4] = 0x00;
  g_expected_response[5] = 0x00;
  g_expected_response[6] = message[6];
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdSetVacBlow");
#endif
  sendMessageToKayo(message);

  // Close suppliment valve if we no longer want to blow off a part. Do this before
  // closing the valve for a specific nozzle.
  if (3 == command_code || 5 == command_code) // We no longer want to blow off a part
  {
    closeSupplimentValve();
  }
}


/* ******************* LIGHTS ***************** */
/*
   BB 00 00 19 00 00 01 55
   4th byte is brightness: 25% = 0x19
   7th byte has # of led.
   1 = high-speed light, 2 = precision light
*/
void cmdLightOn(uint8_t light, uint8_t brightness)
{
  if (light != 1 && light != 2)
  {
    Serial.println("ERR: No valid light specified (1=high-speed, 2=precision)");
    return;
  }
  if (brightness < 0 || brightness > 100)
  {
    Serial.println("ERR: No valid brightness specified (0-100)");
    return;
  }

  uint8_t message[8] = {0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55};
  message[3] = brightness;
  message[6] = light;

  g_expected_response[0] = 0xAA;
  //g_expected_response[3] = message[3];
  //g_expected_response[6] = message[6];
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdLightOn");
#endif
  sendMessageToKayo(message);
}

/*
  Send: BB 00 00 01 00 00 08 55
  Recv: AA 00 00 01 00 00 08 55
*/
void cmdLightsOff()
{
  uint8_t message[8] = {0xBB, 0x00, 0x00, 0x01, 0x00, 0x00, 0x08, 0x55};

  g_expected_response[0] = 0xAA;
  //g_expected_response[3] = 0x01;
  //g_expected_response[6] = 0x08;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdLightsOff");
#endif
  sendMessageToKayo(message);
}


/* ******************* FEEDERS ***************** */
/*

*/
void cmdOpenFeeder(uint8_t feeder_id)
{
  if (NULL != feeder_id)
  {
    // Should we call cmdCloseFeeders() here, so that repeated calls
    // to open the feeder will cause it to advance?
    uint8_t message[8] = {0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55};
    message[2] = feeder_id;

    g_expected_response[0] = 0xAA;
    g_expected_response[7] = 0x55;
    //g_expected_response[2] = message[2];
#if COMMS_DEBUGGING
    Serial.print(millis());
    Serial.println("  f: cmdOpenFeeder");
#endif
    sendMessageToKayo(message);
#if COMMS_DEBUGGING
    Serial.println("Delay to allow feeder to open");
#endif
    //delay(200);
  } else {
    Serial.println("ERR: No feeder ID specified");
  }
}


/*
  The "close feeder" command causes any currently open feeder to be closed.
  Only one feeder can be open at a time, and opening any other feeder
  automatically closes the previous feeder. This command ensures there are
  no feeders currently open.
*/
void cmdCloseFeeders()
{
  uint8_t message[8] = {0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55};

  g_expected_response[0] = 0xAA;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: cmdCloseFeeders");
#endif
  sendMessageToKayo(message);
#if COMMS_DEBUGGING
  Serial.println("Delaying to allow feeder to close");
#endif
  //delay(200);
}


/* Open the valve that allows blowoff */
void openSupplimentValve()
{
  uint8_t message[8] = {0xEE, 0xCC, 0x00, 0x08, 0x00, 0x00, 0x00, 0x55};
  g_expected_response[0] = 0xAA;
  g_expected_response[1] = 0xCC;
  g_expected_response[2] = 0x00;
  g_expected_response[3] = 0x08;
  g_expected_response[4] = 0x00;
  g_expected_response[5] = 0x00;
  g_expected_response[6] = 0x00;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: openSupplimentValve");
#endif
  sendMessageToKayo(message);
}

/* Close the valve that allows blowoff */
void closeSupplimentValve()
{
  uint8_t message[8] = {0xEE, 0xCC, 0x00, 0x09, 0x00, 0x00, 0x00, 0x55};
  g_expected_response[0] = 0xAA;
  g_expected_response[1] = 0xCC;
  g_expected_response[2] = 0x00;
  g_expected_response[3] = 0x09;
  g_expected_response[4] = 0x00;
  g_expected_response[5] = 0x00;
  g_expected_response[6] = 0x00;
  g_expected_response[7] = 0x55;
#if COMMS_DEBUGGING
  Serial.print(millis());
  Serial.println("  f: closeSupplimentValve");
#endif
  sendMessageToKayo(message);
}

/*  **************** OBSOLETE ************* */
/*
   def ZaxisConvertZdepth(self, toolname: str, Z: float):
   # parameter Z is the height above the deck of the nozzle holder
    # for KayoA4, we need to convert that to a top is zero, fully lowered is some position number down...

    zaxisKayo = g.md.headZNameDict[toolname][0]
    zaxisIndex = g.md.headZNameDict[toolname][1]
    Zzero = g.md.headZMaxList[zaxisIndex]
    Zmin = g.md.headZMinList[zaxisIndex]
    if Z > Zzero:
      logger.fatal(' attempt to go Z {:3f} > Zmax {:3f} ! '.format(Z,Zzero))
      Z = Zzero
    elif Z < Zmin:
      logger.fatal(' attempt to go Z {:3f}< Zmin {:3f}! '.format(Z,Zmin))
      Z = Zmin
    Znew = Zzero - Z  # subtract desired height from homed height
    return (Znew, zaxisKayo)
*/
float zAxisConvertDepth(float z)
{
  // We want the z axis coordinate to be zero at the homed height, and a negative
  // number going down from there. We accept a negative number and turn it into
  // a positive number to tell the Kayo how far to move down from home.
  return -z;
}


/*
   Accepts a distance in mm, and returns the number of microsteps
   required to move the nozzle that distance from the home position.
*/
uint16_t makeZDepthConversion(float z_distance)
{
  // Maybe move all these values into the config file:
  float    motor_degrees_per_step      = 1.8;  // 1.8 degrees/step, 200 steps/revolution
  uint8_t  z_axis_microsteps           = 16;   // Perhaps should be 32. Don't know!
  uint16_t max_z_axis_microsteps_value = 2492; // Taken from Glen's code, I don't know the basis of it

  // Cam wheel is 16mm diameter. Motor shaft center to cam wheel center is 12mm
  float depth_deg90  = 10.54;
  float depth_deg135 = 18.6;
  float cam_angle    =  0.0;

  if (z_distance <= depth_deg90)
  {
    cam_angle = asin(z_distance / depth_deg90);
  } else {
    z_distance -= depth_deg90;
    cam_angle   = 3.14159 * 0.5;
    cam_angle  += asin(z_distance / ((depth_deg135 - depth_deg90) * 1.414));
  }

  // deg = rad * 57296 / 1000  <-- Convert radians to degrees using only integer math
  uint16_t full_steps = (cam_angle * 57296 / 1000) / motor_degrees_per_step;
  uint16_t micro_steps = int((full_steps * z_axis_microsteps) + 0.5);

  if (micro_steps > max_z_axis_microsteps_value)
  {
    micro_steps = max_z_axis_microsteps_value;
    Serial.println("ERR: micro_steps > max_z_axis_microsteps_value!");
  } else if (micro_steps < 0) {
    micro_steps = 0;
    Serial.println("ERR: micro_steps < 0!");
  }
  return micro_steps;
}



#endif H_MACHINEACTIONS
