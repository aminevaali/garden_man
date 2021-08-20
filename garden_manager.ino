  #include <limits.h>
  #define SECOND 1000UL
  #define MINUTE 60UL*SECOND
  #define HOUR 60UL*MINUTE

const int valve1 = 11;
const int valve2 = 12;
const int dirtHumidityPin = A1;

const int indicatorSize = 7;
const int indicatorDigits[indicatorSize] = {2, 3, 4, 5, 6, 7, 8};

const unsigned long wateringTime = HOUR; // 1 hour by milliseconds
const unsigned long wateringGapTime = 24UL * HOUR - wateringTime;


bool watering;
unsigned long t0;


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

  openValve1();
  openValve2();
  t0 = millis();

  Serial.begin(9600);
}

void loop() {

  if(watering){
    if(millis() - t0 >= wateringTime){
      closeValve1();
      closeValve2();
      t0 = millis();
    }
  }else{
    if(millis() - t0 >= wateringGapTime){
      openValve1();
      openValve2();
      t0 = millis();
    }
  }

  int humidity = analogRead(dirtHumidityPin);
  humidity /= 8;
  Serial.println(humidity);
  showHumidityValue(humidity);
  delay(30000);
}

void openValve1(){
  watering = true;
  digitalWrite(valve1, LOW);
}

void closeValve1(){
  watering = false;
  digitalWrite(valve1, HIGH);
}

void openValve2(){
  watering = true;
  digitalWrite(valve2, LOW);
}

void closeValve2(){
  watering = false;
  digitalWrite(valve2, HIGH);
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
