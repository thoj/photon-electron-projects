/*
Notes
BLUE LCD = 0x3F
Yellow LCD = 0x20 (12V?)
*/

#include "OneWire/OneWire.h"
#include "LiquidCrystal_I2C_Spark/LiquidCrystal_I2C_Spark.h"
#include "chars.h"
#include "RunningAverage/RunningAverage.h"

LiquidCrystal_I2C *lcd; //set the LCD address to 0x27 for a 16 chars and 2 line display

OneWire ds = OneWire(D4);
Timer update_clock(1000, print_time_string);
//Timer update_ow_bus(10000, scan_ow);

bool update_2stage = false;
Timer update_temps_t(1000, update_temps); // two step!

int nr_ds;
byte ds_addrs[8][8];

bool status;

void setup(void)
{
  pinMode(D6, OUTPUT); //Status
  
  lcd = new LiquidCrystal_I2C(0x3F,20,4);
  lcd->init();
  lcd->backlight();
  lcd->clear();
  for (int i = 0; i < 8; i++)  lcd->createChar(i, NEWCHARS[i]);

  Time.zone(+1);

  update_clock.start();
 // update_ow_bus.start();
  update_temps_t.start();
  scan_ow();
}

void loop(void)
{
    status = !status;
    digitalWrite(D6, status);
    delay(250);
}

void print_time_string() {
    lcd->setCursor(0,0);
    lcd->print(Time.format(Time.now(), "%Y-%m-%d %H:%M:%S"));
}

void update_temps() {
    ds.reset();
    if (! update_2stage) {
        ds.skip();
        ds.write(0x44,0);
        update_2stage = true;
    } else {
        for(int i = 0; i < nr_ds; i++) {
            byte data[12];
            ds.select(ds_addrs[i]);
            ds.write(0xB8, 0);
            ds.write(0x00, 0);
            ds.reset();
            ds.select(ds_addrs[i]);
            ds.write(0xBE, 0);
            if (ds_addrs[i][0] == 0x26 ) ds.write(0x00,0); //pagenr for ds2438
            for ( int x = 0; x < 9; x++) {
                data[x] = ds.read();
            }

            int16_t raw = (data[1] << 8) | data[0];
            float celsius = 0;
            byte cfg = (data[4] & 0x60);
            switch (ds_addrs[i][0]) {
                case 0x10:
                    raw = raw << 3;
                    if (data[7] == 0x10) {
                        raw = (raw & 0xFFF0) + 12 - data[6]; 
                    }
                    break;
                case 0x28:
                case 0x22:
                    if (cfg == 0x00) raw = raw & ~7;
                    if (cfg == 0x20) raw = raw & ~3;
                    if (cfg == 0x40) raw = raw & ~1;
                    celsius = (float)raw * 0.0625;
                    break;
                case 0x26:
                    data[1] = (data[1] >> 3) & 0x1f;
                    if (data[2] > 127) {
                        celsius = (float)data[2] - ((float)data[1] * .03125);
                    } else {
                        celsius = (float)data[2] + ((float)data[1] * .03125);
                    }
                break;
            }
            if (OneWire::crc8(data, 8) == data[8]) { // Only update on good data!
                lcd->setCursor(0, i+1);
                lcd->print(String::format("%d:", i));
                lcd->print(String::format("%5.1f", celsius));
                lcd->write(0);
            }
        }
        update_2stage = false;
    }
}

void scan_ow() {
    int i = 0;
    while(ds.search(ds_addrs[i])) {
        i++;
    }
    nr_ds = i;
    ds.reset_search();
}

String fmt_hexstr(byte *a, uint8_t cnt) {
    String s = String(a[0], HEX);
    for (int i = 1; i < cnt; i++) {
        s.concat(String(a[i], HEX));
    }
    return s;
}

/*
void scan_bus() {
    
  Serial.println ();
  Serial.println ("I2C scanner. Scanning ...");
  byte count = 0;
  
  Wire.begin();
  for (byte i = 8; i < 120; i++)
  {
    Wire.beginTransmission (i);
    if (Wire.endTransmission () == 0)
      {
      Serial.print ("Found address: ");
      Serial.print (i, DEC);
      Serial.print (" (0x");
      Serial.print (i, HEX);
      Serial.println (")");
      count++;
      delay (1);  // maybe unneeded?
      } // end of good response
  } // end of for loop
  Serial.println ("Done.");
  Serial.print ("Found ");
  Serial.print (count, DEC);
  Serial.println (" device(s).");
    
}*/