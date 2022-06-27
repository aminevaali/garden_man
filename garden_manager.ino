#include <limits.h>
#include <SoftwareSerial.h>

//------------------------watering codes-------------------------
#define SECOND 1000UL
#define MINUTE 60UL*SECOND
#define HOUR 60UL*MINUTE

const short valve1 = 6;
const short valve2 = 5;
const int dirtHumidityPin = A1;

const unsigned long wateringTime1 = 30UL * MINUTE; // 0.5 hour by milliseconds
const unsigned long wateringTime2 = 40UL * MINUTE;

bool watering1 = false;
bool watering2 = false;
bool chainValves = true;
bool suspend = false; // this variable is used to suspend automatic watering
unsigned long t1 = 0, t2 = 0;
const int HUMIDITY_TO_WATERING = 768; // 750% dryness = 20% humidity
unsigned short dryCounter = 0;
const unsigned short DRYCOUNTERLIMIT = 7;

void openValve1();
void closeValve1();
void openValve2();
void closeValve2();
void showHumidityValue(unsigned long);



//----------------------sms codes---------------------------
SoftwareSerial mySerial(3,2);
const int gsmResetPin = 4;


const int ALLOWED_NUM_SIZE = 3;
String allowedNumbers[ALLOWED_NUM_SIZE] = {"+989027732097", "+989229205534", "+989132227550"};


String numcheck = "\"+98";
unsigned short numPos = 0;
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
      lastNumber = "+98";
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
  unsigned short pos = 0;

  Command(String str){
    cmd = str;
  }

  Command(){
    
  }
};


Command commands[] = {Command("gmn_stat"), Command("gmn_callme"), Command("gmn_ov1")
    , Command("gmn_ov2"), Command("gmn_cv1"), Command("gmn_cv2"), Command("gmn_rstgsm"),
    Command("gmn_suspend"), Command("gmn_resume"), Command("gmn_chainon"),
    Command("gmn_chainoff"), Command("gmn_ping"), Command("gmn_help")
    }; 
const int COMMANDS_COUNT = sizeof(commands) / sizeof(commands[0]);

void runCmd(String s){
  if(!checkLastNumber()){
    sendSMS("disallowed number!\n" + lastNumber, "+989027732"); //TODO use report function instead
    return;
  }

  Serial.print("\n\ncmd : ");
  Serial.println(s);
  Serial.println("----");
  delay(500);

  if(s.equals("gmn_stat")){
    sendStatus();
  }else if(s.equals("gmn_callme")){
    mySerial.println("ATD" + lastNumber + ";");
  }else if(s.equals("gmn_ov1")){
    openValve1();
    t1 = millis();
    if(chainValves){
      sendSMS("Valve1 opened\nvalves chain=yes", lastNumber);
    }else{
      sendSMS("Valve1 opened\nvalves chain=no", lastNumber);
    }
  }else if(s.equals("gmn_ov2")){
    openValve2();
    t2 = millis();
    sendSMS("valve2 opened", lastNumber);
  }else if(s.equals("gmn_cv1")){
    closeValve1();
    t1 = millis();
    sendSMS("valve1 closed", lastNumber);
  }else if(s.equals("gmn_cv2")){
    closeValve2();
    t2 = millis();
    sendSMS("valve2 closed", lastNumber);
  }else if(s.equals("gmn_rstgsm")){
    resetGsm();
    delay(1000);
    sendSMS("gsm reset done", lastNumber);
  }else if(s.equals("gmn_suspend")){
    suspend = true;
    sendSMS("automatic watering suspended", lastNumber);
  }else if(s.equals("gmn_resume")){
    suspend = false;
    sendSMS("automatic watering enabled", lastNumber);
  }
  else if(s.equals("gmn_chainon")){
    chainValves = true;
    sendSMS("Watering chain enabled", lastNumber);
  }else if(s.equals("gmn_chainoff")){
    chainValves = false;
    sendSMS("Watering chain disabled", lastNumber);
  }else if(s.equals("gmn_rstconf")){
    chainValves = true;
    suspend = false;
    sendSMS("settings reset done", lastNumber);
  }else if(s.equals("gmn_ping")){
    sendSMS("pong", lastNumber);
  }else if(s.equals("gmn_help")){
    sendHelp();
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

void resetGsm(){
  digitalWrite(gsmResetPin, LOW);
  delay(500);
  digitalWrite(gsmResetPin, HIGH);
  delay(MINUTE);

  mySerial.println("AT");                 // Sends an ATTENTION command, reply should be OK
  updateSmsSerial();
  mySerial.println("AT+CMGF=1");          // Configuration for sending SMS
  updateSmsSerial();
  mySerial.println("AT+CNMI=1,2,0,0,0");  // Configuration for receiving SMS
  updateSmsSerial();
}

void updateSmsSerial()
{
  delay(500);
  while(Serial.available()){
  mySerial.write(Serial.read());
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
  mySerial.println( "AT+CMGS=\"" + number + "\"");
  delay(500);
  mySerial.print(message);
  delay(500);
  mySerial.write(26);
  delay(500);
}

void sendStatus(){

  mySerial.println("AT+CMGF=1");
  delay(500);
  mySerial.println( "AT+CMGS=\"" + lastNumber + "\"");
  delay(500);
  
  mySerial.print("valve1: ");
  mySerial.print(watering1? "open" : "closed");
  mySerial.print("(");
  mySerial.print((millis() - t1) / 60000); // 60000 milliseconds = 1 minute
  mySerial.println("min)");

  mySerial.print("valve2: ");
  mySerial.print(watering2? "open" : "closed");
  mySerial.print("(");
  mySerial.print((millis() - t2) / 60000); // 60000 milliseconds = 1 minute
  mySerial.println("min)");

  mySerial.print("humidity: ");
  long dryness = analogRead(dirtHumidityPin) * 100UL / 1023UL;
  mySerial.println(100 - dryness);

  mySerial.print("valves chain: ");
  mySerial.println(chainValves? "yes" : "no");

  mySerial.print("suspend: ");
  mySerial.print(suspend? "yes" : "no");
  
  delay(500);
  mySerial.write(26);
  delay(500);
    
}

void sendHelp(){
  mySerial.println("AT+CMGF=1");
  delay(500);
  mySerial.println( "AT+CMGS=\"" + lastNumber + "\"");
  delay(500);
  
  mySerial.println("Commands:");
  for(int i = 0; i < COMMANDS_COUNT; i++){
    mySerial.println(commands[i].cmd);
  }
  
  delay(500);
  mySerial.write(26);
  delay(500);
}

//void report(String message){
//  for(int i = 0; i < REPORTING_NUM_SIZE; i++){
//    sendSMS(message, reportingNumbers[i]);
//    delay(6000);
//  }
//
//}

void call(String number){
  mySerial.println("ATD" + number + ";");
//  delay(20000); // wait for 20 seconds...
//  mySerial.println("ATH"); //hang up
}



void setup() {
   Serial.begin(9600);


// ---------------------watering setup---------------------------
  pinMode(valve1, OUTPUT);
  pinMode(valve2, OUTPUT);
  pinMode(dirtHumidityPin, INPUT);
  
  closeValves();
  
 

// ---------------------sms setup-------------------------------
  pinMode(gsmResetPin, OUTPUT);
  digitalWrite(gsmResetPin, HIGH);

  delay(MINUTE); // delay to ensure gsm is ready
  mySerial.begin(9600);
  Serial.println("Initializing...");
  delay(1000);
  
  mySerial.println("AT");                 // Sends an ATTENTION command, reply should be OK
  updateSmsSerial();
  mySerial.println("AT+CMGF=1");          // Configuration for sending SMS
  updateSmsSerial();
  mySerial.println("AT+CNMI=1,2,0,0,0");  // Configuration for receiving SMS
  updateSmsSerial();
  delay(2000);
  sendSMS("gman started working...", "+989027732097"); //TODO use report function instead
  delay(1000);
}

void loop() {
  updateSmsSerial();
  
  
  int humidity = analogRead(dirtHumidityPin);
  if(humidity >= HUMIDITY_TO_WATERING){
    if(dryCounter < DRYCOUNTERLIMIT){
      dryCounter++;
    }
  }else{
    dryCounter = 0;
  }

  
  if(watering1){
    if(millis() - t1 >= wateringTime1){
     closeValve1();
     t1 = millis();
     
     if(chainValves == true){
       openValve2();
       t2 = millis();
       sendSMS("valve2 is opened after valve1 because chain is on", "+989027732097"); //TODO use report function instead
       delay(5000);
     }

     if(humidity >= HUMIDITY_TO_WATERING){
        suspend = true;
        sendSMS("sensor error\nautomatic watering suspended", "+989027732097");
     }
     
    }
  }else if(watering2){
    if(millis() - t2 >= wateringTime2){
      closeValve2();
      t2 = millis();
    }
  }else{
    if(!suspend && dryCounter == DRYCOUNTERLIMIT){
      openValve1();
      t1 = millis();
      sendSMS("Valve1 was opened automatically", "+989027732097"); //TODO use report function instead
    }
  }
}

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
