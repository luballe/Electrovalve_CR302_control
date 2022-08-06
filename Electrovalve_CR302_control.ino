
#include "RTClib.h"

#define UNDEFINED  -1
#define CLOSED      0
#define OPENING     1
#define OPEN        2
#define CLOSING     3

#define SWITCH_CLOSE 0
#define SWITCH_AUTO  1
#define SWITCH_OPEN  2

//RTC_DS1307 rtc;
RTC_DS3231 rtc;
DateTime now;
static const unsigned long REFRESH_INTERVAL = 1000; // ms
static unsigned long lastRefreshTime = 0;

bool alarm_on = false;
unsigned int cur_year,cur_month,cur_day,cur_hour,cur_minute,cur_second;
unsigned int alarm_on_hour,  alarm_on_minute,  alarm_on_second;
unsigned int alarm_off_hour, alarm_off_minute, alarm_off_second;
unsigned long _cur_time,_alarm_on,_alarm_off;

const int pushButton      =  2;   // push button pin
const int powerSignalPin  =  3;   // send signal to turn on / off the valve
const int openSensor      =  4;   // open  Sensor pin
const int closeSensor     =  5;   // close Sensor pin
const int powerSensor     =  6;   // power Sensor pin
const int outputSignalPin =  7;   // output signal pin
const int ledPin          =  13;  // the number of the led pin

int pushButtonState;
int openSensorState;
int closeSensorState;
int powerSensorState;

int previousValveState =  UNDEFINED;
int currentValveState  =  UNDEFINED;

bool tested = false;

int cur_switch_status  = SWITCH_AUTO;
int prev_switch_status = SWITCH_CLOSE;

void setup() {
  Serial.begin(9600);
  // initialize sensor pins as inputs:
  pinMode(pushButton, INPUT);
  pinMode(openSensor, INPUT);
  pinMode(closeSensor, INPUT);
  pinMode(powerSensor, INPUT);

  // initialize pins as outputs:
  pinMode(powerSignalPin, OUTPUT);
  pinMode(outputSignalPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

//  if (rtc.lostPower()) {
//    Serial.println("RTC is NOT running, let's set the time!");
//    // When time needs to be set on a new device, or after a power loss, the
//    // following line sets the RTC to the date & time this sketch was compiled
//    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
//    // This line sets the RTC with an explicit date & time, for example to set
//    // January 21, 2014 at 3am you would call:
//    // rtc.adjust(DateTime(2022, 8, 4, 19, 32, 0));
//  }

  // When time needs to be re-set on a previously configured device, the
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // This line sets the RTC with an explicit date & time, for example to set
  // January 21, 2014 at 3am you would call:
  //rtc.adjust(DateTime(2022, 8, 5, 8, 45, 0));

  // Alarm on time
  alarm_on_hour    = 4;
  alarm_on_minute  = 0;
  alarm_on_second  = 0;

  // Alarm off time
  alarm_off_hour   = 6;
  alarm_off_minute = 30;
  alarm_off_second = 0;
}

void loop() {
  
  // read valve's state
  pushButtonState  = digitalRead(pushButton);
  powerSensorState = digitalRead(powerSensor);
  openSensorState  = digitalRead(openSensor);
  closeSensorState = digitalRead(closeSensor);


  // Valve just enegized, reset to close
  if(openSensorState == LOW && closeSensorState == LOW){
    if(!tested){
      digitalWrite(powerSignalPin, HIGH);
      digitalWrite(outputSignalPin, LOW);
      while(!(openSensorState == LOW && closeSensorState == HIGH)){
//        previousValveState = CLOSING;
        currentValveState = CLOSING;
        pushButtonState  = digitalRead(pushButton);
        powerSensorState = digitalRead(powerSensor);
        closeSensorState = digitalRead(closeSensor);
        openSensorState  = digitalRead(openSensor);
        printStatus();
        delay(10);        
      }
//      digitalWrite(ledPin,LOW);
      digitalWrite(powerSignalPin, LOW);
      tested = true;
    }
  }

  // Valve in a intermediate state: Openning o Closing..
  else if(openSensorState == HIGH && closeSensorState == HIGH){
    if(previousValveState == UNDEFINED){
//      previousValveState = CLOSING;
      currentValveState = CLOSING;
    }
    else if(previousValveState == CLOSED){
      currentValveState = OPENING;
//      Serial.println("Opening");
    }
    else if(previousValveState == OPEN){
      currentValveState = CLOSING;
//      Serial.println("Closing");
    }
    else if(previousValveState == CLOSING){
      currentValveState = CLOSING;
//      Serial.println("Closing");
    }
    else if(previousValveState == OPENING){
      currentValveState = OPENING;
//      Serial.println("Opening");
    }
  }
  else if(openSensorState == LOW && closeSensorState == HIGH){
    currentValveState = CLOSED;
//    Serial.println("Closed");
  }
  else if(openSensorState == HIGH && closeSensorState == LOW){
    currentValveState = OPEN;
//    Serial.println("Open");
  }


  // Button pressed
  if(pushButtonState == HIGH){
    if(prev_switch_status == SWITCH_AUTO && cur_switch_status == SWITCH_CLOSE){
      prev_switch_status = SWITCH_CLOSE;
      cur_switch_status = SWITCH_AUTO;
    }
    else if(prev_switch_status == SWITCH_CLOSE && cur_switch_status == SWITCH_AUTO){
      prev_switch_status = SWITCH_AUTO;
      cur_switch_status = SWITCH_OPEN;
    }
    else if(prev_switch_status == SWITCH_AUTO && cur_switch_status == SWITCH_OPEN){
      prev_switch_status = SWITCH_OPEN;
      cur_switch_status = SWITCH_AUTO;
    }
    else if(prev_switch_status == SWITCH_OPEN && cur_switch_status == SWITCH_AUTO){
      prev_switch_status = SWITCH_AUTO;
      cur_switch_status = SWITCH_CLOSE;
    }
    delay(500);
  }
  
  
  if(cur_switch_status==SWITCH_AUTO){
    // Alarm ON
    if(alarm_on){
      // Valve closed, let's opening...
      if(previousValveState == CLOSED && currentValveState == CLOSED){
        digitalWrite(powerSignalPin, HIGH);
        digitalWrite(outputSignalPin, HIGH);
        digitalWrite(ledPin,LOW);
      }    
    }
    // Alarm OFF
    else{
      // Valve open, let's closing...
      if(previousValveState == OPEN && currentValveState == OPEN){
        digitalWrite(powerSignalPin, HIGH);
        digitalWrite(outputSignalPin, LOW);
        digitalWrite(ledPin,LOW);
      }
    }
  }
  else if(cur_switch_status==SWITCH_OPEN){
    // Valve closed, let's opening...
    if(previousValveState == CLOSED && currentValveState == CLOSED){
      digitalWrite(powerSignalPin, HIGH);
      digitalWrite(outputSignalPin, HIGH);
      digitalWrite(ledPin,LOW);
    }
  }
  else if(cur_switch_status==SWITCH_CLOSE){
    // Valve open, let's closing...
    if(previousValveState == OPEN && currentValveState == OPEN){
      digitalWrite(powerSignalPin, HIGH);
      digitalWrite(outputSignalPin, LOW);
      digitalWrite(ledPin,LOW);
    }
  }
  
  // Valve just open, turn valve off
  if(previousValveState == OPENING && currentValveState == OPEN){
    digitalWrite(powerSignalPin, LOW);
    digitalWrite(outputSignalPin, LOW);
    digitalWrite(ledPin,HIGH);
  }
  // Valve just closed, turn valve off
  else if(previousValveState == CLOSING && currentValveState == CLOSED){
    digitalWrite(powerSignalPin, LOW);
    digitalWrite(outputSignalPin, LOW);
    digitalWrite(ledPin,LOW);
  }

  // Valve opening, turn valve on
  else if(previousValveState == OPENING && currentValveState == OPENING){
    digitalWrite(powerSignalPin, HIGH);
    digitalWrite(outputSignalPin, HIGH);
    digitalWrite(ledPin,LOW);
  }
  
  // Valve closing, turn valve on
  else if(previousValveState == CLOSING && currentValveState == CLOSING){
    digitalWrite(powerSignalPin, HIGH);
    digitalWrite(outputSignalPin, LOW);
    digitalWrite(ledPin,LOW);
  }

  if(millis() - lastRefreshTime >= REFRESH_INTERVAL)
  {
    lastRefreshTime += REFRESH_INTERVAL;
    call_rtc();
    check_alarm();
  }


  // Print the valve state
  printStatus();

  if(previousValveState != currentValveState){
   previousValveState = currentValveState;
  }

  delay(10);

}
void call_rtc(){
  now = rtc.now();
  cur_year   = now.year();
  cur_month  = now.month();
  cur_day    = now.day();
  cur_hour   = now.hour();
  cur_minute = now.minute();
  cur_second = now.second();
}

void check_alarm(){
  _cur_time = cur_hour*3600L + cur_minute*60L + cur_second; // The "L" is a must. if it's not present, the result will be integer and the probably (for values greater than 65536, after 18:13) truncated
  _alarm_on = alarm_on_hour*3600L + alarm_on_minute*60L + alarm_on_second;
  _alarm_off = alarm_off_hour*3600L + alarm_off_minute*60L + alarm_off_second;
  
  if (_cur_time >= long(_alarm_on) && _cur_time <= long(_alarm_off)){
    alarm_on = true;        
  }
  else{
    alarm_on = false;
  }
}

void show_time(){
  Serial.print(" ");
  Serial.print(cur_year, DEC);
  Serial.print('/');
  Serial.print(cur_month, DEC);
  Serial.print('/');
  Serial.print(cur_day, DEC);
  Serial.print(" ");
  Serial.print(cur_hour, DEC);
  Serial.print(':');
  Serial.print(cur_minute, DEC);
  Serial.print(':');
  Serial.print(cur_second, DEC);

  Serial.print(" Alarm:");
  if (alarm_on)
    Serial.print(" ON");
  else
    Serial.print(" OFF");

  Serial.print(' ');
  Serial.print(_cur_time,DEC);
  Serial.print(' ');
  Serial.print(_alarm_on, DEC);
  Serial.print(' ');
  Serial.print(_alarm_off, DEC);
}


void printStatus(){
  if(cur_switch_status==SWITCH_CLOSE){
    Serial.print("CLOSE ");
  }
  else if(cur_switch_status==SWITCH_AUTO){
    Serial.print("AUTO ");
  }
  else if(cur_switch_status==SWITCH_OPEN){
    Serial.print("OPEN ");
  }
  else{
    Serial.print(cur_switch_status,DEC);
    Serial.print(" ");
  }
  Serial.print(pushButtonState);
  Serial.print(powerSensorState);
  Serial.print(openSensorState);
  Serial.print(closeSensorState);
  Serial.print("_");
  Serial.print(previousValveState);
  Serial.print("_");
  Serial.print(currentValveState);
  show_time();
  Serial.println(); 
  
}

void bigPrint(uint64_t n){
  //Print unsigned long long integers (uint_64t)
  //CC (BY-NC) 2020
  //M. Eric Carr / paleotechnologist.net
  unsigned char temp;
  String result=""; //Start with a blank string
  if(n==0){Serial.print(0);return;} //Catch the zero case
  while(n){
    temp = n % 10;
    result=String(temp)+result; //Add this digit to the left of the string
    n=(n-temp)/10;      
    }//while
  Serial.print(result);
}
