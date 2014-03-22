#include <SPI.h>
#include <Ethernet.h>
#include <strin1g.h>

#define comandBuffSize 12

byte mac[] =     { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[]  =     {
  192, 168, 1, 6 };
byte gateway[] = {
  192, 168, 1, 1 };
byte subnet[]  = {
  255, 255, 255, 0 };


char comandBuff[comandBuffSize]; 
int charsReceived = 0;

boolean isConnected = 0, isAuthenticated = 0; 


unsigned long timeOfLastActivity; 
unsigned long allowedConnectTime = 300000; 

EthernetServer server(23); 
EthernetClient client = 0;






void setup()
{
  Serial.begin(9600);
  // setting pins 0 to 9 as outputs
  // pins 10-13 are used by the Ethernet Shield
  for(int  i= 0; i < 10; i++)  pinMode(i, OUTPUT);

  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
}

void loop()
{
  if (server.available() && !isConnected) {
    isConnected = 1;
    client = server.available();
    client.println("\nWellcome to the Home telnet server v1.0");
    authentication();
  }

  if (client.connected() && client.available() && isAuthenticated) getReceivedText();
  if(isConnected) checkConnectionTimeout();

}


void nextCommand()
{
  timeOfLastActivity = millis();
  client.flush();
  charsReceived = 0; //count of characters received
  client.print("\ncommand>");
}


void checkConnectionTimeout()
{
  if(millis() - timeOfLastActivity > allowedConnectTime) {
    client.println();
    client.println("Timeout disconnect.");
    client.stop();
    isConnected = 0;
    isAuthenticated = 0;
  }
}


void getReceivedText()
{
  char c;
  int charsWaiting;

  charsWaiting = client.available();
  do {
    c = client.read();
    comandBuff[charsReceived] = c;
    charsReceived++;
    charsWaiting--;
  }
  while(charsReceived <= comandBuffSize && c != 0x0d && charsWaiting > 0);

  if(c == 0x0d) {
    readingCommand();
    nextCommand();
  }

  if(charsReceived >= comandBuffSize) {
    client.println();
    error();
    nextCommand();
  }

}  


void readingCommand()
{
  // look at first character and decide what to do
  switch (comandBuff[0]) {
  case 'a' : 
    doAnalogCommand();        
    break;
  case 'd' : 
    doDigitalCommand();       
    break;
  case 'p' : 
    setPinMode();             
    break;
  case 'c' : 
    checkCloseConnection();   
    break;
  case '?' : 
    printHelpMessage();       
    break;
  case 0x0d :                          
    break;  //ignore a carriage return
  default: 
    error();                    
    break;
  }
}


void doDigitalCommand()
// if we got here, comandBuff[0] = 'd'
{
  switch (comandBuff[1]) {
  case 'r' : 
    readDigitalPins(); 
    break;
  case 'w' : 
    writeDigitalPin(); 
    break;
  default: 
    error();             
    break;
  }
}


void readDigitalPins()
// if we got here, comandBuff[0] = 'd' and comandBuff[1] = 'r'
{
  int pin;
  if (comandBuff[2] == 0x0d) {
    // output the valueof each digital pin
    for (int i = 0; i < 10; i++) outputPinState(i);
  }
  else {
    pin = parseDigit(comandBuff[2]);
    if(pin >=0 && pin <=9) outputPinState(pin);
    else error();
  }
}  


void outputPinState(int pin)
{
  client.print("digital pin ");
  client.print(pin);
  client.print(" is ");
  if (digitalRead(pin)) {
    client.println("HIGH");
  }
  else
    client.println("LOW");
}


void writeDigitalPin()
// if we got here, comandBuff[0] = 'd' and comandBuff[1] = 'w'
{
  int pin = -1;
  int pinSetting = -1;
  if (comandBuff[3] == '=' && comandBuff[6] == 0x0d) {
    //if yes, get the pin number, setting, and set the pin
    pin = parseDigit(comandBuff[2]);
    pinSetting = parsePinSetting();
    if(pin > -1 && pinSetting == 0) {
      digitalWrite(pin, LOW);
      client.println("OK");
    }
    if(pin > -1 && pinSetting == 1) {
      digitalWrite(pin, HIGH);
      client.println("OK");
    }
    if(pin < 0 || pinSetting < 0) error();
  }
  else error();
}


int parsePinSetting()
//look in the text buffer to find the pin setting
//return -1 if not valid
{
  int pinSetting = -1;
  if(comandBuff[4] == 'l' && comandBuff[5] == 'o') pinSetting = 0;
  if(comandBuff[4] == 'h' && comandBuff[5] == 'i') pinSetting = 1;
  return pinSetting;
}


void doAnalogCommand()
// if we got here, comandBuff[0] = 'a'
{
  switch (comandBuff[1]) {
  case 'r' : 
    readAnalogPins(); 
    break;
  case 'w' : 
    writeAnalogPin(); 
    break;
  default: 
    error(); 
    break;
  }
}


void readAnalogPins()
// if we got here, comandBuff[0] = 'a' and comandBuff[1] = 'r'
// check comandBuff[2] is a CR then
// output the value of each analog input pin
{
  if(comandBuff[2] == 0x0d) {
    for (int i = 0; i < 6; i++) {
      client.print("analog input ");
      client.print(i);
      client.print(" is ");
      client.println(analogRead(i));
    }
  }
  else error();
}


void writeAnalogPin()
// if we got here, comandBuff[0] = 'a' and comandBuff[1] = 'w'
{
  int pin = -1;
  int pwmSetting = -1;
  if (comandBuff[3] == '=') {
    //if yes, get the pin number, setting, and set the pin
    pin = parseDigit(comandBuff[2]);
    if(pin == 3 || pin == 5 || pin == 6 || pin == 9) {
      pwmSetting = parsepwmSetting();
      if(pwmSetting >= 0 && pwmSetting <= 255) {
        analogWrite(pin,pwmSetting);
        client.println("OK");
      }
      else error();
    }
    else error();
  }
  else error();
}


int parsepwmSetting()
{
  int pwmSetting = 0;
  int textPosition = 4;  //start at comandBuff[4]
  int digit;
  do {
    digit = parseDigit(comandBuff[textPosition]); //look for a digit in comandBuff
    if (digit >= 0 && digit <=9) {              //if digit found
      pwmSetting = pwmSetting * 10 + digit;     //shift previous result and add new digit
    }
    else pwmSetting = -1;
    textPosition++;                             //go to the next position in comandBuff
  }
  //if not at end of comandBuff and not found a CR and not had an error, keep going
  while(textPosition < 7 && comandBuff[textPosition] != 0x0d && pwmSetting > -1);
  //if value is not followed by a CR, return an error
  if(comandBuff[textPosition] != 0x0d) pwmSetting = -1;  
  return pwmSetting;
}

void setPinMode()
// if we got here, comandBuff[0] = 'p'
{
  int pin = -1;
  int pinModeSetting = -1;
  if (comandBuff[1] == 'm' && comandBuff[3] == '=' && comandBuff[6] == 0x0d) {
    //if yes, get the pin number, setting, and set the pin
    pin = parseDigit(comandBuff[2]);
    pinModeSetting = parseModeSetting();
    if(pin > -1 && pinModeSetting == 0) {
      pinMode(pin, OUTPUT);
      client.println("OK");
    }
    if(pin > -1 && pinModeSetting == 1) {
      pinMode(pin, INPUT);
      client.println("OK");
    }
    if(pin < 0 || pinModeSetting < 0) error();
  }
  else error();
}


int parseModeSetting()
//look in the text buffer to find the pin setting
//return -1 if not valid
{
  int pinSetting = -1;
  if(comandBuff[4] == 'o' && comandBuff[5] == 'u') pinSetting = 0;
  if(comandBuff[4] == 'i' && comandBuff[5] == 'n') pinSetting = 1;
  return pinSetting;
}


int parseDigit(char c)
{
  int digit = -1;
  digit = (int) c - 0x30; // subtracting 0x30 from ASCII code gives value
  if(digit < 0 || digit > 9) digit = -1;
  return digit;
}


void error()
{
  client.println("Unrecognized command.  ? for help.");
}


void checkCloseConnection()
// if we got here, comandBuff[0] = 'c', check the next two
// characters to make sure the command is valid
{
  if (comandBuff[1] == 'l' && comandBuff[2] == 0x0d)
    closeConnection();
  else
    error();
}


void closeConnection()
{
  client.println("\nBye.\n");
  client.stop();
  isConnected = 0;
  isAuthenticated = 0
}

boolean authentication (void)
{
  struct 
  {
    char login[10];
    char password[10];  
  } 
  authData[10] =
  {
    "admin", "ltybc" //your passwords below


  };


  struct 
  {
    char login[11];
    char password[11];  
  } 
  authBuffer;

  char c;
  int Received, i, counter = 0;

  for ( ; counter < 3 && !isAuthenticated; counter++) { //Max 3 attemps

    //=======================LOGIN=================

    client.flush();
    client.println ("Login>");
    timeOfLastActivity = millis();
    Received = 0;
    delay(300);
    for ( ;!client.available() ; )  checkConnectionTimeout();
    while (Received < 10 && client.connected())
    {
      c = client.read();
      if (c != -1) {
        if (c == '\r') continue;
        if (c == '\n') break;
        authBuffer.login[Received] = c;
        Received++;
      } 
      else delay(10);

    }
    authBuffer.login[Received]='\0';

    //=======================PASSWORD=================   

    client.flush();
    client.println ("Password>");
    timeOfLastActivity = millis();
    Received = 0;
    delay(300);
    for ( ;!client.available() ; )  checkConnectionTimeout();
    while (Received < 10 && client.connected())
    {
      c = client.read();
      if (c != -1) {
        if (c == '\r') continue;
        if (c == '\n') break;
        authBuffer.password[Received] = c;
        Received++;
      } 
      else delay(10);

    }
    authBuffer.password[Received]='\0';


    for (i = 0; i < 10; i++) if (!strcmp (authData[i].login, authBuffer.login) && !strcmp (authData[i].password, authBuffer.password))   //matching
    { 
      isAuthenticated = 1; 
      break; 
    }

    if (isAuthenticated){
      nextCommand();
      return 1;
    } 
    else {
      client.println ("Wrong login or password!");
    }
  }
  closeConnection();
  return 0;
}


void printHelpMessage()
{
  client.println("\nSupported commands:\n");
  client.println("  dr       -digital read:   returns state of digital pins 0 to 9");
  client.println("  dr4      -digital read:   returns state of pin 4 only");
  client.println("  ar       -analog read:    returns all analog inputs");
  client.println("  dw0=hi   -digital write:  turn pin 0 on  valid pins are 0 to 9");
  client.println("  dw0=lo   -digital write:  turn pin 0 off valid pins are 0 to 9");
  client.println("  aw3=222  -analog write:   set digital pin 3 to PWM value 222");
  client.println("                              allowed pins are 3,5,6,9");
  client.println("                              allowed PWM range 0 to 255");
  client.println("  pm0=in   -pin mode:       set pin 0 to INPUT  valid pins are 0 to 9");
  client.println("  pm0=ou   -pin mode:       set pin 0 to OUTPUT valid pins are 0 to 9");
  client.println("  cl       -close connection");
  client.println("  ?        -print this help message");
}




