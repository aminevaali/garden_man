#include <limits.h>
#include <SoftwareSerial.h>

//------------------------watering codes-------------------------
#define SECOND 1000UL
#define MINUTE 60UL*SECOND
#define HOUR 60UL*MINUTE

const int valve1 = 5;
const int valve2 = 6;
const int dirtHumidityPin = A1;

const int indicatorSize = 7;
const int indicatorDigits[indicatorSize] = {7, 8, 9, 10, 11, 12, 13};

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



//----------------------sms codes---------------------------
SoftwareSerial mySerial(3,2);
const int gsmResetPin = 4;


const int ALLOWED_NUM_SIZE = 1;
String allowedNumbers[ALLOWED_NUM_SIZE] = {"9027732097"};


String numcheck = "\"+98";
int numPos = 0;
String lastNumber = "";
bool numflag = false;

void updateNumber(char ch){
  if(numflag == false){
    if(ch == numcheck[numPos]){
      numPos++;
    }else{
      numPos = 0;
    }
  
    if(numPos == numcheck.length()){
      numflag = true;
      numPos = 0;
      lastNumber = "";
    }
  }else{ // number started
    if(ch != '"'){
      lastNumber.concat(ch);
    }else{
      numflag = false;
    }
  }
  
}

bool checkLastNumber(){
  for(int i = 0; i < ALLOWED_NUM_SIZE; i++){
    if(lastNumber.equals(allowedNumbers[i])){
      return true;
    }
  }
  return false;
}

class Command{
public:
  String cmd;
  int pos = 0;
};

Command commands[5]; 
void initialCommands(){
  commands[0] = Command();
  commands[0].cmd = "gman_status";
  
  commands[1] = Command();
  commands[1].cmd = "gman_help";
  
  commands[2] = Command();
  commands[2].cmd = "gman_callme";

  commands[3] = Command();
  commands[3].cmd = "gman_ov1";

  commands[4] = Command();
  commands[4].cmd = "gman_ov2";

  commands[5] = Command();
  commands[5].cmd = "gman_cv1";

  commands[6] = Command();
  commands[6].cmd = "gman_cv2";

  commands[7] = Command();
  commands[7].cmd = "gman_water";
}

void runCmd(String s){
  if(!checkLastNumber()){
    sendSMS("A disallowed number tried to command me!", "+989027732097");
    return;
  }
  Serial.print("\n\nYour command is : ");
  Serial.println(s);
  Serial.println("--------------------------");

//todo manage all commands in this if-else statements
  if(s.equals("gman_status")){
    String stat = "status : ";
    stat += "valve1: ";
    stat += (watering1? "watering" : "not watering");
    stat += "valve2: ";
    stat += (watering2? "watering" : "not watering");
    stat += "\n";
    stat += "humidity : ";
    int humidity = digitalRead(dirtHumidityPin) * 100 / 1023;
    stat.concat(humidity);
    sendSMS(stat, "+98" + lastNumber);
    
  }else if(s.equals("help")){
    String help = "commands : ";
    help += "gman_status, gman_callme, gman_ov1, gman_ov2, gman_cv1, gman_cv2, gman_water, gman_nowater";
    sendSMS(help, "+98" + lastNumber);
    
  }else if(s.equals("gman_callme")){
    
    mySerial.println("ATD+98" + lastNumber + ";");
    
  }
}

void readSMS(char ch){
  int commandsLength = sizeof(commands) / sizeof(Command);
  for(int i = 0; i < commandsLength; i++){
    if(ch == commands[i].cmd.charAt(commands[i].pos)){
      commands[i].pos++;
    }else{
      commands[i].pos=0;
    }

    if(commands[i].pos == commands[i].cmd.length()){
      runCmd(commands[i].cmd);
      commands[i].pos = 0;
    }
  }

}

String cmd = "";
void updateSmsSerial()
{
  delay(500);
  while (Serial.available()) 
  {

    cmd+=(char)Serial.read();
 
    if(cmd!=""){
      cmd.trim();  // Remove added LF in transmit
      if (cmd.equals("S")) {
        sendSMS("Hello from sim800", "+989229205534");
      }else if(cmd.equals("C")){
        call();
      }else {
        mySerial.print(cmd);
        mySerial.println("");
      }
    }
  }

  while(mySerial.available()) 
  {
    char ch = mySerial.read();
    updateNumber(ch);
    readSMS(ch);
    Serial.write(ch);//Forward what Software Serial received to Serial Port
  }
  
}

void sendSMS(String message, String number){
  mySerial.println("AT+CMGF=1");
  delay(500);
  mySerial.println( "AT+CMGS=\"" + number + "\"\r");
  delay(500);
  mySerial.print(message);
  delay(500);
  mySerial.write(26);
}

void call(){
  mySerial.println("ATD+989229205534;"); //  change ZZ with country code and xxxxxxxxxxx with phone number to dial
  updateSmsSerial();
  delay(20000); // wait for 20 seconds...
  mySerial.println("ATH"); //hang up
  updateSmsSerial();
}



void setup() {
  pinMode(valve1, OUTPUT);
  pinMode(valve2, OUTPUT);
  pinMode(dirtHumidityPin, INPUT);
  for(int i = 0; i < indicatorSize; i++){
    pinMode(indicatorDigits[i], OUTPUT);
  }
  closeValves();
  
  Serial.begin(9600);


// ---------------------sms setup-------------------------------
  initialCommands();
  pinMode(gsmResetPin, OUTPUT);
  digitalWrite(gsmResetPin, HIGH);

  mySerial.begin(9600);
  Serial.println("Initializing...");
  delay(1000);
  
  mySerial.println("AT");                 // Sends an ATTENTION command, reply should be OK
  updateSmsSerial();
  mySerial.println("AT+CMGF=1");          // Configuration for sending SMS
  updateSmsSerial();
  mySerial.println("AT+CNMI=1,2,0,0,0");  // Configuration for receiving SMS
  updateSmsSerial();
  
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

//  Serial.println(hum/idity);//
  humidity /= 8;
  showHumidityValue(humidity);

  updateSmsSerial();
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
