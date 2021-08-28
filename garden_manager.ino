  #include <limits.h>
  #define SECOND 1000UL
  #define MINUTE 60UL*SECOND
  #define HOUR 60UL*MINUTE

const int valve1 = 11;
const int valve2 = 12;
const int dirtHumidityPin = A1;

const int indicatorSize = 7;
const int indicatorDigits[indicatorSize] = {2, 3, 4, 5, 6, 7, 8};

const unsigned long wateringTime = 45UL * MINUTE; // 1 hour by milliseconds

bool watering1 = false;
bool watering2 = false;
unsigned long t0;
const int HUMIDITY_TO_WATERING = 952;
int dryCounter = 0;
const int DRYCOUNTERLIMIT = 7;

void openValve1();
void closeValve1();
void openValve2();
void closeValve2();
void showHumidityValue(unsigned long);


void setup() {
  pinMode(valve1, OUTPUT);
  pinMode(valve2, OUTPUT);
  pinMode(dirtHumidityPin, INPUT);

  for(int i = 0; i < indicatorSize; i++){
    pinMode(indicatorDigits[i], OUTPUT);
  }

  closeValves();

  Serial.begin(9600);
}

void loop() {
  int humidity = analogRead(dirtHumidityPin);
  if(humidity >= HUMIDITY_TO_WATERING){
    if(dryCounter < DRYCOUNTERLIMIT){
      dryCounter++;
    }
  }else{
    dryCounter = 0;
  }

  

  if(watering1){
    if(millis() - t0 >= wateringTime){
     closeValve1();
     openValve2();
     t0 = millis();
    }
  }else if(watering2){
    if(millis() - t0 >= wateringTime){
      closeValve2();
    }
  }else{
    if(dryCounter == DRYCOUNTERLIMIT){
      openValve1();
      t0 = millis();
    }
  }

  Serial.println(humidity);
  humidity /= 8;
  showHumidityValue(humidity);
  delay(1000);
}

//void openValves(){
//  openValve1();
//  openValve2();
//}

void closeValves(){
  closeValve1();
  closeValve2();
}

void openValve1(){
  digitalWrite(valve1, LOW);
  watering1 = true;
}

void closeValve1(){
  digitalWrite(valve1, HIGH);
  watering1 = false;
}

void openValve2(){
  digitalWrite(valve2, LOW);
  watering2 = true;
}

void closeValve2(){
  digitalWrite(valve2, HIGH);
  watering2 = false;
}

void showHumidityValue(int x){
  int p = 0;
  while(x != 0){
    int d = x % 2;
    digitalWrite(indicatorDigits[p], d);
    p++;
    x /= 2;
  }

  if(p < indicatorSize){
    for(; p < indicatorSize; p++){
      digitalWrite(indicatorDigits[p], LOW);
    }
  }
}
