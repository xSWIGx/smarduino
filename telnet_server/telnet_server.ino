#include <SPI.h>
#include <Ethernet.h>
#include <strin1g.h>

#define comandBuffSize 20
#define STANDART 0
#define DIGITAL 1
#define ANALOG 2
#define MODE 3

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = {
  192, 168, 1, 6 };
byte gateway[] = {
  192, 168, 1, 1 };
byte subnet[] = {
  255, 255, 255, 0 };

char comandBuff[comandBuffSize];
int charsReceived = 0, type = 0;

boolean isConnected = 0, isAuthenticated = 0, pinstate[10];

unsigned long timeOfLastActivity;
unsigned long allowedConnectTime = 300000;

EthernetServer server(23);
EthernetClient client = 0;



void setup()
{
  Serial.begin(9600);
  // setting pins 0 to 9 as outputs
  // pins 10-13 are used by the Ethernet Shield
  for (int i = 0; i < 10; i++)  { 
    pinMode(i, OUTPUT); 
    pinstate[i] = 1; 
  }

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
  if (isConnected) checkConnectionTimeout();

}

boolean authentication(void)
{
  struct
  {
    char login[10];
    char password[10];
  }
  authData[10] =
  {
    //your passwords below "login", "password"
    "admin", "1q2w3e"

  };

  struct
  {
    char login[11];
    char password[11];
  }
  authBuffer;

  char c;
  int Received, i, counter = 0;

  for (; counter < 3 && !isAuthenticated; counter++) { //Max 3 attemps

    //=======================LOGIN=================

    client.flush();
    client.print("Login> ");
    timeOfLastActivity = millis();
    Received = 0;
    delay(300);
    for (; !client.available();)  checkConnectionTimeout();
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
    authBuffer.login[Received] = '\0';

    //=======================PASSWORD=================   

      client.flush();
    client.print("Password> ");
    timeOfLastActivity = millis();
    Received = 0;
    delay(300);
    for (; !client.available();)  checkConnectionTimeout();
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
    authBuffer.password[Received] = '\0';


    for (i = 0; i < 10; i++) if (!strcmp(authData[i].login, authBuffer.login) && !strcmp(authData[i].password, authBuffer.password))   //matching
    {
      isAuthenticated = 1;
      break;
    }

    if (isAuthenticated){
      client.println("Accessed!");
      client.println("Print ? or help to see help message.");
      nextCommand();
      return 1;
    }
    else {
      client.println("Wrong login or password!");
    }
  }
  closeConnection();
  return 0;
}

void nextCommand()
{
  timeOfLastActivity = millis();
  client.flush();
  charsReceived = 0; //count of characters received

  switch (type)
  {
  case 0:
    client.print("\ncommand> ");
    break;
  case DIGITAL:
    client.print("\ndigital> ");
    break;
  case ANALOG:
    client.print("\nanalog> ");
    break;
  case MODE:
    client.print("\npin mode> ");
    break;
  }
}

void checkConnectionTimeout()
{
  if (millis() - timeOfLastActivity > allowedConnectTime) {
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
  while (charsReceived < comandBuffSize && client.connected())
  {
    c = client.read();
    if (c != -1) {
      //if (c == '\r') continue;
      if (c == 0x0d) break;
      comandBuff[charsReceived] = c;
      charsReceived++;
    }
    else delay(10);

  }
  comandBuff[charsReceived] = '\0';

  if (c == 0x0d) {
    readingCommand();
    nextCommand();
  }

  if (charsReceived >= comandBuffSize) {
    client.println();
    error();
    nextCommand();
  }
}

void readingCommand()
{
  char st[] = "set";
  char cls[] = "close";
  char dgt[] = "digital";
  char anl[] = "analog";
  char md[] = "mode";
  char rd[] = "read";
  char wr[] = "write";
  char bk[] = "back";
  char hp[] = "help";

  if (comandBuff[0] == '\0') return;
  else if (!strcmp(hp, comandBuff) || comandBuff[0] == '?') printHelpMessage();
  else if (!strcmp(cls, comandBuff)) closeConnection();
  else if (!strcmp(dgt, comandBuff)) type = DIGITAL;
  else if (!strcmp(anl, comandBuff)) type = ANALOG;
  else if (!strcmp(md, comandBuff)) type = MODE;
  else  if (type != STANDART) {
    switch (type){
    case DIGITAL:
      if (cmp(rd, comandBuff)) readDigital();
      else if (cmp(wr, comandBuff)) writeDigitalPin();
      else if (!strcmp(bk, comandBuff)) type = STANDART;
      else error();
      break;
    case ANALOG:
      if (cmp(rd, comandBuff)) readAnalogPins();
      else if (cmp(wr, comandBuff)) writeAnalogPin();
      else if (!strcmp(bk, comandBuff)) type = STANDART;
      else error();
      break;
    case MODE:
      if (cmp(st, comandBuff)) setPinMode();
      else if (cmp(rd, comandBuff)) readPinMode();
      else if (!strcmp(bk, comandBuff)) type = STANDART;
      else error();
      break;
    }
  }
  else error();
}

void readDigital()
{
  int pin = parseDigit(comandBuff[5]);
  comandBuff[5] = '\0';
  if (pin == -1) {
    // output the valueof each digital pin
    for (int i = 0; i < 10; i++) outputPinState(i, 0);
  }
  else {
    if (pin >= 0 && pin <= 9) outputPinState(pin, 0);
    else error();
  }
}

void outputPinState(int pin, int mode)
{
  if (!mode) client.print("digital pin ");
  else client.print("analog input ");
  client.print(pin);
  client.print(" is ");
  if (!mode){
    if (digitalRead(pin)) {
      client.println("HIGH");
    }
    else
      client.println("LOW");
  }
  else client.println(analogRead(pin));
}

void writeDigitalPin()
{
  int pin = -1;
  int pinSetting = -1;
  if (comandBuff[5] == 0x20 && comandBuff[7] == 0x20) {
    //if yes, get the pin number, setting, and set the pin
    pin = parseDigit(comandBuff[6]);
    pinSetting = parsePinSetting();
    if (pin > -1 && pinSetting == 0) {
      digitalWrite(pin, LOW);
      client.println("OK");
    }
    if (pin > -1 && pinSetting == 1) {
      digitalWrite(pin, HIGH);
      client.println("OK");
    }
    if (pin < 0 || pinSetting < 0) error();
  }
  else error();
}


int parsePinSetting()
{
  int pinSetting = -1;
  if (comandBuff[8] == 'l' && comandBuff[9] == 'o') pinSetting = 0;
  if (comandBuff[8] == 'h' && comandBuff[9] == 'i') pinSetting = 1;
  return pinSetting;
}

void readAnalogPins()
{
  int pin = parseDigit(comandBuff[5]);
  comandBuff[5] = '\0';

  if (pin == -1)   for (int i = 0; i < 6; i++) outputPinState(i, 1);
  else   if (pin >= 0 && pin < 6) outputPinState(pin, 1);
  else error();
}


void writeAnalogPin()
{
  int pin = -1;
  int pwmSetting = -1;
  //if yes, get the pin number, setting, and set the pin
  pin = parseDigit(comandBuff[6]);
  if (pin == 3 || pin == 5 || pin == 6 || pin == 9) {
    pwmSetting = parsepwmSetting();
    if (pwmSetting >= 0 && pwmSetting <= 255) {
      analogWrite(pin, pwmSetting);
      client.println("OK");
    }
    else error();
  }
  else error();
}


int parsepwmSetting()
{
  int pwmSetting = 0;
  int textPosition = 8;  //start at comandBuff[8]
  int digit;
  do {
    digit = parseDigit(comandBuff[textPosition]); //look for a digit in comandBuff
    if (digit >= 0 && digit <= 9) {              //if digit found
      pwmSetting = pwmSetting * 10 + digit;     //shift previous result and add new digit
    }
    else pwmSetting = -1;
    comandBuff[textPosition] = '\0';
    textPosition++;                             //go to the next position in comandBuff
  }
  //if not at end of comandBuff and not found a CR and not had an error, keep going
  while (textPosition < 11 && comandBuff[textPosition] != '\0' && pwmSetting > -1);
  //if value is not followed by a CR, return an error
  if (comandBuff[textPosition] != '\0') pwmSetting = -1;
  return pwmSetting;
}

void readPinMode()
{
  int pin = parseDigit(comandBuff[5]);
  comandBuff[5] = '\0';
  if (pin == -1) {
    for (int i = 0; i < 10; i++) {
      client.print("pin ");
      client.print(i);
      client.print(" is ");
      if (pinstate[i]) client.println("OUTPUT"); 
      else client.println("INPUT");
    }

  }
  else {
    if (pin >= 0 && pin <= 9) {
      client.print("pin ");
      client.print(pin);
      client.print(" is ");
      if (pinstate[pin]) client.println("OUTPUT");
      else client.println("INPUT");
    }
    else error();
  }
}

void setPinMode()
{
  int pin = -1;
  int pinModeSetting = -1;

  pin = parseDigit(comandBuff[4]);
  comandBuff[4] = '\0';
  pinModeSetting = parseModeSetting();
  if (pin > -1 && pinModeSetting == 0) {
    pinstate[pin] = 1;
    pinMode(pin, OUTPUT);
    client.println("OK");
  }
  if (pin > -1 && pinModeSetting == 1) {
    pinstate[pin] = 0;
    pinMode(pin, INPUT);
    client.println("OK");
  }
  if (pin < 0 || pinModeSetting < 0) error();
}


int parseModeSetting()
{
  int pinSetting = -1;
  if (comandBuff[6] == 'o' && comandBuff[7] == 'u') pinSetting = 0;
  if (comandBuff[6] == 'i' && comandBuff[7] == 'n') pinSetting = 1;
  return pinSetting;
}


int parseDigit(char c)
{
  int digit = -1;
  digit = (int)c - 0x30; // subtracting 0x30 from ASCII code gives value
  if (digit < 0 || digit > 9) digit = -1;
  return digit;
}


void error()
{
  client.println("Unrecognized command.  ? for help.");
}


void closeConnection()
{
  client.println("\nBye.\n");
  client.stop();
  isConnected = 0;
  isAuthenticated = 0;
}



int cmp(char *str1, char *str2)
{
  int len1, len2, n;
  len1 = strlen(str1);
  len2 = strlen(str2);
  if (len2 < len1) return 0;
  for (n = 0; n < len2; n++) {
    if (str1[n] == str2[n])   continue;
    else if (str1[n] == '\0') return 1;
    else return 0;
  }

}


void printHelpMessage()
{
  client.print("You are in ");
  switch (type){
  case STANDART:
    client.println("MAIN menu!\n");
    client.println("Supported commands:\n");
    client.println("  digital       -operations with digital pins");
    client.println("  analog        -operations with analog pins");
    client.println("  mode          -operations with pins modes");
    client.println("  close         -close connection");
    client.println("  help or ?     -help message");
    break;

  case DIGITAL:
    client.println("DIGITAL menu!");
    client.println("Supported commands:\n");
    client.println("  read                 -read all pins state");
    client.println("  read <pin number>    -read pin state");
    client.println("  write <pin number> <hight/low>  -write pin state");
    client.println("                              allowed pins are from 0 to 9");
    client.println("                              pins from 10 to 13 are used by ethernet shield");
    client.println("  back                 -return to MAIN menu");
    client.println("  close                -close connection");
    client.println("  help or ?            -help message");
    break;

  case ANALOG:
    client.println("ANALOG menu!");
    client.println("Supported commands:\n");
    client.println("  read                 -read all pins state");
    client.println("  read <pin number>    -read pin state");
    client.println("  write <pin number> <value>  -write pin state");
    client.println("                              allowed pins are from 0 to 9");
    client.println("                              pins from 10 to 13 are used by ethernet shield");
    client.println("  back                 -return to MAIN menu");
    client.println("  close                -close connection");
    client.println("  help or ?            -help message");
    break;

  case MODE:
    client.println("ANALOG menu!");
    client.println("Supported commands:\n");
    client.println("  set <pin number> <input/output>      -set pin mode 3, 5, 6, 9 avaliable");
    client.println("  state                  -read all pins mode");
    client.println("  state <pin number>     -read pin mode");
    client.println("                              allowed pins are from 0 to 6");
    client.println("                              pins from 10 to 13 are used by ethernet shield");
    client.println("  back                   -return to MAIN menu");
    client.println("  close                  -close connection");
    client.println("  help or ?              -help message");
    break;
  }
}


