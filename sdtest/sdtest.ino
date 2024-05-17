#include <SPI.h>
#include <SD.h>

const int chipSelect = 5;

int dat = -10;
int pid = 20;
String datstring = "";

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200L);
  while (!Serial);

  Serial.print("Initializing SD card...");
  SPI.begin();
  delay(3000);
  // see if the card is present and can be initialized:
  if (!SD.begin(SS, SPI)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  Serial.println("card initialized.");
}

void loop() {
    File dataFile = SD.open("/datalog2.csv", FILE_APPEND, true);
    if (dataFile) {
    char buf[26];
    byte len = sprintf(buf, "%X,%d", pid, dat);
    dataFile.write((uint8_t *) buf, len);
    dataFile.write('\n');
    Serial.write((uint8_t *) buf, len);
    dataFile.close();
    // print to the serial port too:
    dat++;
  }else{Serial.println("ERRO-FILE");}
  delay(100);
}
