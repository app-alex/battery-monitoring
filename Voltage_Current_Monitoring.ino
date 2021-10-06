//#include <OneWire.h>
//#include <DallasTemperature.h>


//#define ONE_WIRE_BUS A2
//OneWire oneWire(ONE_WIRE_BUS);
//DallasTemperature sensors(&oneWire);

//CONSTANTE
float batteryMaxVoltage = 4.0;
float batteryMinVoltage = 3.0;
float batteryBOLCapacity = 1900; //[Ampers*s]
//float batteryBOLCapacity = (8800 / 1000.0) * 60.0 * 60.0; //capacitatea specificata de producator
float currentSensorSensitivity = 0.100;
float currentSensorMarginError = 0.02;
int maxChargingCicles = 1000;
int timerCounterOffset = 3036;

 
//VARIABILE GLOBALE
float batteryMaxCapacity = batteryBOLCapacity;
float batteryUsedCapacity = 0;
float batteryRemainingCapacity;
float chargingCicles = 0;
bool isCharging = false;
int batteryRestTime = 0;


void setup() {
  Serial.begin(9600);
  //sensors.begin();
  
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = timerCounterOffset;
  TCCR1B |= (1 << CS12);
  TIMSK1 |= (1 << TOIE1);

  //initiere capacitate ramasa
  batteryRemainingCapacity = batteryMaxCapacity * getBatterySOCVoltageMethod(getBatteryVoltage())/100;
  
  Serial.println("start");
}


//Functie interval 1s
ISR(TIMER1_OVF_vect) {
  TCNT1 = timerCounterOffset;

  //Masurare voltaj+curent
  float batteryVoltage = getBatteryVoltage();
  float batteryCurrent = getBatteryCurrent();

  updateChargingCicles(batteryCurrent, batteryMaxCapacity);

  //Recalibrare SOC cu metoda OCV
  if (batteryRestTime> 60*60){//Dupa o ora de inactivitate a bateriei
    batteryMaxCapacity = batteryMaxCapacity * getBatterySOCVoltageMethod(getBatteryVoltage())/getBatterySOCCoulombMethod();
    batteryRemainingCapacity = batteryMaxCapacity * getBatterySOCVoltageMethod(getBatteryVoltage())/100;
    batteryUsedCapacity = 0;
  }

  //Verificare curent
  if (batteryCurrent > currentSensorMarginError) {//exista consum
    batteryUsedCapacity += batteryCurrent;
    batteryRestTime = 0;
  } else if (batteryCurrent < -currentSensorMarginError){//incarcare (curent negativ)
    isCharging = true;
    //Recalibrare SOC cu metoda OCV pentru incarcare 100%
    //batteryMaxCapacity = batteryMaxCapacity * getBatterySOCVoltageMethod(getBatteryVoltage())/100;
    batteryRemainingCapacity = batteryMaxCapacity * getBatterySOCVoltageMethod(getBatteryVoltage())/100;
    batteryUsedCapacity = 0;
    batteryRestTime = 0;
  } else {//baterie inactiva
    batteryRestTime += 1;
  }

  
  //Afisare date
  //Serial.print(round(millis()));
  //Serial.print(",");
  Serial.print(batteryVoltage);
  Serial.print(",");
  Serial.print(batteryCurrent);
  Serial.print(",");
  Serial.print(getBatterySOCVoltageMethod(batteryVoltage));
  Serial.print(",");
  Serial.print(getBatterySOCCoulombMethod());
  Serial.print(",");
  Serial.print(batteryUsedCapacity);
  Serial.print(",");
  Serial.print(batteryRemainingCapacity);
  Serial.print(",");
  Serial.print(getSOH());
  Serial.print(",");
  Serial.print(chargingCicles);
  Serial.print(",");
  Serial.println(batteryRestTime);
}


void loop() {

}


//VOLTAGE MEASURE FUNCTIONS
int readVoltageSensor() {
  float value = analogRead(A0);
  return value;
}


float getBatteryVoltage() {
  float voltageSamples = 0;
  for (int i = 0; i < 1000; i++) {
    voltageSamples += readVoltageSensor() * 5.0 / 1024.0 / 0.2;
  }
  return voltageSamples / 1000;
}


//CURRENT MEASURE FUNCTIONS
int readCurrentSensor() {
  float value = analogRead(A1);
  return value;
}


float getBatteryCurrent() {
  float currentSamples = 0;
  for (int i = 0; i < 1000; i++) {
    currentSamples += ((readCurrentSensor()*(5.0/1024.0)) - 2.5)/currentSensorSensitivity;
  }
  return currentSamples / 1000;
}


//FUNCTII DE CALCUL CAPACITATE
float getBatterySOCVoltageMethod(float voltage) {
  if (voltage > batteryMaxVoltage) {
    return 100.0;
  } else if (voltage < batteryMinVoltage) {
    return 0.0;
  }
  return (voltage - batteryMinVoltage) * 100.0 / (batteryMaxVoltage - batteryMinVoltage);
}


float getBatterySOCCoulombMethod() {
//formula derivata - estimare in functie de capacitatea ramasa, aceasta estimata din tensiunea initiala
  if (batteryRemainingCapacity - batteryUsedCapacity > 0) {
    return ((batteryRemainingCapacity - batteryUsedCapacity) * 100.0)/batteryMaxCapacity;
  } else {
    return 0;
    batteryMaxCapacity = batteryMaxCapacity * getBatterySOCVoltageMethod(getBatteryVoltage())/100;
  }

  //formula conventionala
  //if ((current > currentSensorMarginError)) {   
  //  batterySOC -= ((current * 100.0) / batteryMaxCapacity);
  //}
  //return batterySOC;
}

void updateChargingCicles(float current, float maxCapacity){
  if (current > currentSensorMarginError){
    chargingCicles += current/maxCapacity;
  }
}


float getSOH(){ // State of Health
  float SOH = (batteryMaxCapacity / batteryBOLCapacity)*100;
  return SOH;
}

//float getBatteryTemp() {
//  sensors.requestTemperatures();  
//  return sensors.getTempCByIndex(0)
//}
