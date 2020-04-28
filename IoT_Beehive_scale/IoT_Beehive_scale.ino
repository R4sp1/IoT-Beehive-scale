#include <SoftwareSerial.h>
#include <DHT.h>
#include <HX711.h>
#include "LowPower.h"

#define TX 10
#define RX 11
#define pinDHTA 4
#define typDHTA DHT21
#define pinDHTB 5
#define typDHTB DHT22
#define calibration_factor -24000.0
#define DT 8
#define SCK 7


int En_volreg = 12;    //enable pin voltage regulatoru
int spinac = 9;       //vstup spínače na hlídání úlu
int state;            //stav pinu 9

SoftwareSerial Sigfox(RX, TX);
DHT DHTA(pinDHTA, typDHTA);
DHT DHTB(pinDHTB, typDHTB);
HX711 scale(DT, SCK);

void setup() {
  Serial.begin(9600);
  Sigfox.begin(9600);
  DHTA.begin();
  DHTB.begin();
  scale.set_scale(calibration_factor);
  pinMode(spinac, INPUT);
  pinMode(En_volreg, OUTPUT);
  delay(1000);
}

void loop() {
  if (Sigfox.available()) {
    Serial.write(Sigfox.read());
  }
  if (Serial.available()) {
    Sigfox.write(Serial.read());
  }

  SigProbudit();
  delay(1000);
  odesliData();
  delay(10000);
  SigUsnout();
  ArSpanek();
  delay(100000);
}


void odesliData() {
  char zprava[12];

  float teplotaA = DHTA.readTemperature();
  unsigned int tepA = (teplotaA + 10) * 10;
  float vlhkostA = DHTA.readHumidity();
  unsigned int vlhA = (vlhkostA);

  float teplotaB = DHTB.readTemperature();
  unsigned int tepB_cela = (teplotaB + 10) * 10;
  float vlhkostB = DHTB.readHumidity();
  unsigned int vlhB = (vlhkostB);

  unsigned int tepB = tepB_cela - tepA + 20;  //zjišťování rozdílu teplot

  int k = 359.64; //číslo musím zjitit samotným scale.get_units potom jen přičítat aby byla bez váhy 0
  float vaha = scale.get_units() + k;
  unsigned int vah = (vaha) * 10;

  delay(500);

  sprintf(zprava, "%03X%02X%02X%02X%03X", tepA, vlhA, tepB, vlhB, vah);
  Sigfox.print("AT$SF=");
  Sigfox.println(zprava);
}


void odeslipoplach() {  //odeslání zprávy o manipulaci
  Sigfox.println("AT$SF=ffffffffffff");
}

void SigUsnout() {  //spánek Sigfox modulu
  Sigfox.println("AT$P=1");
  delay(100);
  digitalWrite(En_volreg, LOW);
}

void SigProbudit() { //probuzení sigfox modulu
  Sigfox.println("\n");
  delay(100);
  digitalWrite(En_volreg, HIGH);
}

void ArSpanek() {  //spánek Arduina
  for (int i = 0; i < 900; i++) {               //i<113=15min, <225=30min, <450=60min, <900=2h, <1350=3h, <1800=4h, <2250=5h, <4500=10h
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    state = digitalRead(spinac);
    if (state == HIGH) {
      SigProbudit();
      delay(1000);
      odeslipoplach();
      delay(5000);
      SigUsnout();
    }
  }
}
