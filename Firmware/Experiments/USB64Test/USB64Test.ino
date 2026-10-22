/*
  USB CDC packet-boundary test for the OpenKayo board (ESP32-S3).

  Send a number N followed by a newline. The board replies with exactly
  N bytes in a single Serial.write() and then stays silent. The host
  script usb64-test.py checks whether all N bytes arrive without any
  further host writes.

  If replies of exactly 64 (or 128...) bytes stall while 63 and 65 don't,
  the USB stack isn't ending transfers that finish on a full packet.

  Build with the same board settings as OpenKayo.ino, then repeat with
  Tools -> USB Mode -> "USB-OTG (TinyUSB)" to compare.
*/
#define MAX_REPLY 300

String g_line = "";
uint8_t g_reply[MAX_REPLY];

void setup()
{
  Serial.begin(115200);
  while (!Serial)
  {
    // nothing
  }
}

void loop()
{
  while (Serial.available())
  {
    char c = (char)Serial.read();
    if (c == '\n')
    {
      int n = g_line.toInt();
      g_line = "";
      if (n < 1 || n > MAX_REPLY)
      {
        continue;
      }
      // Printable filler, ending in a newline so the host can frame it
      for (int i = 0; i < n - 1; i++)
      {
        g_reply[i] = 'a' + (i % 26);
      }
      g_reply[n - 1] = '\n';
      Serial.write(g_reply, n);
    } else {
      g_line += c;
    }
  }
}
