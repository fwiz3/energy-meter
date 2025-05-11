#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <PZEM004Tv30.h>

// SoftwareSerial RX, TX
SoftwareSerial pzemSerial(D6, D7); // D6 = RX (GPIO12), D7 = TX (GPIO13)
PZEM004Tv30 pzem(pzemSerial);

void setup() {
  Serial.begin(9600);
  pzemSerial.begin(9600);

  Serial.println("PZEM-004T v3.0 with ESP8266");
}

void loop() {
  float voltage = pzem.voltage();
  float current = pzem.current();
  float power   = pzem.power();
  float energy  = pzem.energy();
  float frequency = pzem.frequency();
  float pf      = pzem.pf();

  Serial.print("Voltage: "); Serial.print(voltage); Serial.println(" V");
  Serial.print("Current: "); Serial.print(current); Serial.println(" A");
  Serial.print("Power: "); Serial.print(power); Serial.println(" W");
  Serial.print("Energy: "); Serial.print(energy); Serial.println(" Wh");
  Serial.print("Frequency: "); Serial.print(frequency); Serial.println(" Hz");
  Serial.print("Power Factor: "); Serial.println(pf);

  Serial.println("-----------------------------");
  delay(2000); // Read every 2 seconds
}
