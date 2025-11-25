#ifndef H_CAN_COMMS
#define H_CAN_COMMS

void sendConveyorWidthToCAN(uint16_t width_requested)
{
#if ENABLE_CAN
#if DEBUG_CAN
  Serial.print  ("Sending CAN for conveyor width: ");
  Serial.println(width_requested);
#endif

  g_last_can_tx_time = millis();

  twai_message_t message;


  /* Set up the 29-bit extended CAN ID header, similar to: 0x18FFF080 */
  uint32_t extended_can_id = 0x18FF0000;                               // Create the basic header

  uint32_t can_destination_address = 0xF0;
  extended_can_id = extended_can_id | (can_destination_address << 8);  // Insert the destination address into the header
  extended_can_id = extended_can_id | g_can_source_address;            // Insert the source address into the header

#if DEBUG_CAN
  Serial.println(extended_can_id, HEX);
#endif

  /* Create the payload */
  char arg_buffer[5];
  itoa(width_requested, arg_buffer, 10);
  for (uint8_t i = 0; i < 5; i++)
  {
    Serial.print(arg_buffer[i]);
    Serial.print(" ");
  }
  Serial.println();

  //uint8_t can_payload[8] = { 'G', '0', ' ', 'Y', '1', '2', '3', 0 };
  uint8_t can_payload[8] = { 'G', '0', ' ', 'Y', arg_buffer[0], arg_buffer[1], arg_buffer[2], arg_buffer[3] };

  /* Create the packet and add the payload */
  message.identifier = extended_can_id;
  message.extd = 1;
  message.rtr = 0;
  message.data_length_code = 8;
  message.data[0] = can_payload[0];
  message.data[1] = can_payload[1];
  message.data[2] = can_payload[2];
  message.data[3] = can_payload[3];
  message.data[4] = can_payload[4];
  message.data[5] = can_payload[5];
  message.data[6] = can_payload[6];
  message.data[7] = can_payload[7];
  twai_transmit(&message, 0);
  //Serial.printf("I'm in the CAN loop sending %s\n", message);
  //printKayoMessage(can_payload);
#endif ENABLE_CAN
}

void sendHomeToCAN()
{
#if ENABLE_CAN
#if DEBUG_CAN
  Serial.println("Sending home command to upstream conveyor.");
#endif DEBUG_CAN

  g_last_can_tx_time = millis();

  twai_message_t message;

  /* Set up the 29-bit extended CAN ID header, similar to: 0x18FFF080 */
  uint32_t extended_can_id = 0x18FF0000;                               // Create the basic header

  uint32_t can_destination_address = J1939_UPSTREAM_ADDRESS;
  extended_can_id = extended_can_id | (can_destination_address << 8);  // Insert the destination address into the header
  extended_can_id = extended_can_id | g_can_source_address;            // Insert the source address into the header

#if DEBUG_CAN
  Serial.println(extended_can_id, HEX);
#endif DEBUG_CAN

  /* Create the payload */
  uint8_t can_payload[8] = { 'G', '2', '8', 0, 0, 0, 0, 0 };

  /* Create the packet and add the payload */
  message.identifier = extended_can_id;
  message.extd = 1;
  message.rtr = 0;
  message.data_length_code = 8;
  message.data[0] = can_payload[0];
  message.data[1] = can_payload[1];
  message.data[2] = can_payload[2];
  message.data[3] = can_payload[3];
  message.data[4] = can_payload[4];
  message.data[5] = can_payload[5];
  message.data[6] = can_payload[6];
  message.data[7] = can_payload[7];
  twai_transmit(&message, 0);
  //printKayoMessage(can_payload);
#endif ENABLE_CAN
}

/*
  Send message to upstream conveyor to send us a board to load
*/
void sendLoadToCAN()
{
#if ENABLE_CAN
#if DEBUG_CAN
  Serial.println("Sending load command to upstream conveyor.");
#endif DEBUG_CAN

  g_last_can_tx_time = millis();

  twai_message_t message;

  /* Set up the 29-bit extended CAN ID header, similar to: 0x18FFF080 */
  uint32_t extended_can_id = 0x18FF0000;                               // Create the basic header

  uint32_t can_destination_address = J1939_UPSTREAM_ADDRESS;
  extended_can_id = extended_can_id | (can_destination_address << 8);  // Insert the destination address into the header
  extended_can_id = extended_can_id | g_can_source_address;            // Insert the source address into the header

#if DEBUG_CAN
  Serial.println(extended_can_id, HEX);
#endif DEBUG_CAN

  /* Create the payload */
  // TODO: Set speed from UPSTREAM_LOAD_SPEED
  // TODO: Use compressed G-Code when target parser can handle it on conveyor
  // M55 S800
  uint8_t can_payload[8] = { 'M', '5', '5', ' ', 'S', '8', '0', '0' };

  /* Create the packet and add the payload */
  message.identifier = extended_can_id;
  message.extd = 1;
  message.rtr = 0;
  message.data_length_code = 8;
  message.data[0] = can_payload[0];
  message.data[1] = can_payload[1];
  message.data[2] = can_payload[2];
  message.data[3] = can_payload[3];
  message.data[4] = can_payload[4];
  message.data[5] = can_payload[5];
  message.data[6] = can_payload[6];
  message.data[7] = can_payload[7];
  twai_transmit(&message, 0);
  //printKayoMessage(can_payload);
#endif ENABLE_CAN
}


#endif H_CAN_COMMS
