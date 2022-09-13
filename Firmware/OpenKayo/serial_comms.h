#ifndef H_SERIAL_COMMS
#define H_SERIAL_COMMS

/*
  Read messages from OpenPnP arriving via the USB connection
*/
void listenToUsbSerialStream()
{
  while (Serial.available())
  {
    // Get the received byte, convert to char for adding to buffer
    char receivedChar = (char)Serial.read();

    // Add character to buffer
    g_usb_input_buffer += receivedChar;

    // If the received character is a newline, process the message and clear the buffer
    if (receivedChar == '\n')
    {
      processGCodeMessage();
      g_usb_input_buffer = "";
    }
  }
}


/*

*/
void sendMessageToKayo(uint8_t message[8], bool suppress_response)
{
  //Serial.print("Sending:   ");
  //printKayoMessage(message);
  //Serial.print("Expecting: ");
  //printKayoMessage(g_expected_response);

  if (ENABLE_KAYO)
  {
    Serial2.write(message[0]);
    Serial2.write(message[1]);
    Serial2.write(message[2]);
    Serial2.write(message[3]);
    Serial2.write(message[4]);
    Serial2.write(message[5]);
    Serial2.write(message[6]);
    Serial2.write(message[7]);

    g_matching_response = false;
    while (!g_matching_response)
    {
      listenToKayoSerialStream(suppress_response);
    }

    /*
        Serial2.setTimeout(25000);
        uint8_t response_buffer[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        if (Serial2.readBytes(response_buffer, 8))
        {
          // Got a message
          //Serial.print("Got:       ");
          //printKayoMessage(response_buffer);
          uint8_t response_matches = true;
          for (uint8_t i = 0; i <= 6; i++)
          {
            if (g_expected_response[i] != 0x00 && response_buffer[i] != g_expected_response[i])
            {
              response_matches = false;
            }
          }

          if (response_matches)
          {
            clearExpectedResponse();
            if (!suppress_response)
            {
              Serial.println("ok");
            }
          } else {
            Serial.println("ERR: Response from Kayo does not match expected value");
          }
        } else {
          Serial.println("ERR: No response received from Kayo");
        }
    */
  }
}


/*
  Read messages from Kayo arriving via the serial connection.

  WARNING: this is not robust! If the buffer position gets out
  of sync, there could be comms problems because the 0x55 terminator
  won't appear in the final position. Can 0x55 ever appear elsewhere
  in the message? Probably, so we can't rely on only that. We may
  be able to check for 0xAA in byte 0 and 0x55 in byte 7.
*/
void listenToKayoSerialStream(bool suppress_response)
{
  while (Serial2.available())
  {
    uint8_t received_byte = Serial2.read();

    // Move the buffer sideways and add this character to the end
    for (uint8_t i = 0; i < 7; i++)
    {
      g_kayo_recv_buffer[i] = g_kayo_recv_buffer[i + 1];
    }
    g_kayo_recv_buffer[7] = received_byte;

    // Check if we have a complete message
    if (0xAA == g_kayo_recv_buffer[0] && 0x55 == g_kayo_recv_buffer[7])
    {
      // We do, so process it
      uint8_t response_matches = true;
      for (uint8_t i = 0; i <= 6; i++)
      {
        if (g_expected_response[i] != 0x00 && g_kayo_recv_buffer[i] != g_expected_response[i])
        {
          response_matches = false;
        }
      }

      if (response_matches)
      {
        g_matching_response = true;
        clearExpectedResponse();
        if (!suppress_response)
        {
          Serial.println("ok");
        }
      } else {
        g_matching_response = false;
        Serial.println("ERR: Response from Kayo does not match expected value.");
        Serial.print("Expected: ");
        printKayoMessage(g_expected_response);
        Serial.print("Received: ");
        printKayoMessage(g_kayo_recv_buffer);
      }
    }
  }
}


/*

*/
void clearExpectedResponse()
{
  g_expected_response[0] = 0x00;
  g_expected_response[1] = 0x00;
  g_expected_response[2] = 0x00;
  g_expected_response[3] = 0x00;
  g_expected_response[4] = 0x00;
  g_expected_response[5] = 0x00;
  g_expected_response[6] = 0x00;
  g_expected_response[7] = 0x55;
}


/*
   Print an 8-byte Kayo message in hex
*/
void printKayoMessage(uint8_t message[])
{
  char dataString[50] = {0};
  sprintf(dataString, "%02X %02X %02X %02X %02X %02X %02X %02X", message[0], message[1], message[2], message[3], message[4], message[5], message[6], message[7]);
  Serial.println(dataString);
}

#endif H_SERIAL_COMMS
