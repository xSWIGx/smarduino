#include <SPI.h>
#include <Ethernet.h>
// необходимые библиотеки нужно скачать и скопировать в папку библиотек Arduino
#include <RCSwitch.h> //   http://code.google.com/p/rc-switch/ для розеток
#include <TimedAction.h> //http://playground.arduino.cc//Code/TimedAction выполнение действий по времени (таймеры)
// #include <Twitter.h> //http://playground.arduino.cc/Code/TwitterLibrary Твиттер
// #include "portMapping.h" //https://github.com/deverick/Arduino-Upnp-PortMapping сетевые функции, здесь не задействованы



// инициализация сети
byte ip[] = { 192,168,1, 55 };                        // IP Address
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // MAC Address
byte servergoogle[] = { 213, 180, 204, 3 }; // Google (Yandex)
byte servermail[] = { 94, 100, 177, 1 }; // smtp.mail.ru
byte gateway[] = { 192, 168, 1, 1 }; // шлюз
byte subnet[] = { 255, 255, 255, 0 }; // маска сети
byte mydns[] = { 192, 168, 1, 1 }; // DNS-сервер
char* statusString[] = {"Socket #1 is ON", "Socket #2 is ON", "Socket #3 is ON", "Socket #4 is ON", "Socket #1 is OFF", "Socket #2 is OFF", "Socket #3 is OFF", "Socket #4 is OFF", "Switched Livolo #1", "Switched Livolo #2", "Switched Livolo #4", "Switched Livolo #5", "Switched Livolo #7", "Switched Livolo #3", "All Livolo is OFF", "System started", "Camera is ON", "Camera is OFF", "Modem reset", "Front door open", "Water leak", "OK"};
byte currentStatus=21; // статус на старте
long unsigned sensorTimeout=0;
EthernetServer server(80);                           // Server Port 80
EthernetClient mail; // на выход для Твиттера и почты
// PortMapClass portmap; // для определения внешнего IP

// авторизация твиттера, нужен токен
// Twitter twitter("здесь ваш токен для Twitter");
 
//проверяем соединение каждые две минуты
//TimedAction timedAction = TimedAction(120000,checkLine);

// RCSwitch configuration
RCSwitch mySwitch = RCSwitch();

byte txPin = 8; // pin connected to RF transmitter (pin 8)
byte i; // packet counter for Livolo (0 - 100)
byte pulse;
// int incomingByte = 0;   // for incoming serial data. Used for test
byte lineDead = 0; // счетчик перезагрузок модема, когда нет интернета
byte lineRest = 0; // счетчик циклов паузы между попытками перезагрузок

// массив команд
byte button1[45]={44, 1, 2, 4, 2, 4, 2, 4, 3, 5, 2, 4, 2, 4, 3, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2};
byte button2[43]={43, 1, 2, 4, 2, 4, 2, 4, 3, 5, 2, 4, 2, 4, 3, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 5, 3, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2};
byte button3[41]={40, 1, 2, 4, 2, 4, 2, 4, 3, 5, 2, 4, 2, 4, 3, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 5, 3, 5, 3, 4, 2, 4, 2, 4, 2};
byte button4[43]={42, 1, 2, 4, 2, 4, 2, 4, 3, 5, 2, 4, 2, 4, 3, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 5, 3, 4, 2, 4, 2, 4, 2};
byte button5[43]={42, 1, 2, 4, 2, 4, 2, 4, 3, 5, 2, 4, 2, 4, 3, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 5, 2, 4, 3, 4, 2, 4, 2, 4, 2, 4, 2};
byte button7[41]={40, 1, 2, 4, 2, 4, 2, 4, 3, 5, 2, 4, 2, 4, 3, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 5, 3, 4, 2, 5, 3, 4, 2, 4, 2};
byte button11[41]={40, 1, 2, 4, 2, 4, 2, 4, 3, 5, 2, 4, 2, 4, 3, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 4, 2, 5, 3, 4, 2, 5, 2, 4, 3, 4, 2};



/**
 * Setup
 */
void setup() {
  Ethernet.begin(mac, ip, mydns, gateway, subnet);
  server.begin();
  mySwitch.enableTransmit(txPin); // Transmitter on pin 8
  mySwitch.enableReceive(0);  // Receiver on inerrupt 0 => that is pin #2  
  pinMode(3, OUTPUT); // инициализация реле камеры
  pinMode(5, OUTPUT); // инициализация реле модема
  digitalWrite(3, HIGH);
  digitalWrite(5, HIGH);
  pinMode(txPin, OUTPUT); //инициализация передатчика
  delay(120000); // ждем, пока очнется сеть (загрузка модема и подключение)
  //sendMail(15); // пишем себе письмо при каждом старте  
  
}

/**
 * Loop
 */
void loop() {
  if (lineRest > 60) {lineDead = 0; lineRest = 0;} // обнуляем счетчики количества безуспешных попыток соединения и таймаута ожидания соединения
 // timedAction.check(); // проверяем соединение
  char* command = httpServer(); // запускаем вебсервер
  if (mySwitch.available()) { // проверяем датчики
    int value = mySwitch.getReceivedValue();
    if (value != 0) {
      switch (mySwitch.getReceivedValue()) {
      case 13753680: // сработал датчик двери
      if ((millis() - sensorTimeout) > 2000) {txButton(button5); sensorTimeout=millis();} // нажимаем кнопку снова, только если с прошлого нажатия больше 2 сек (длина пакета датчика)
      break;  
      case 1918288: // сработал датчик протечки
      sendMail(20);
      break;
    }      
   } 
      mySwitch.resetAvailable();
  }  

}

/**
 * Command dispatcher // обработчик команд
 */
void processCommand(char* command) {
  if        (strcmp(command, "1-on") == 0) { // первая розетка
    mySwitch.send(83029, 24);
    currentStatus=0;
  } else if (strcmp(command, "1-off") == 0) {
    mySwitch.send(83028, 24);
    currentStatus=4;
  } else if (strcmp(command, "2-on") == 0) { // вторая розетка
    mySwitch.send(86101, 24);
    currentStatus=1;
  } else if (strcmp(command, "2-off") == 0) {
    mySwitch.send(86100, 24);
    currentStatus=5;
  } else if (strcmp(command, "3-on") == 0) { // третья розетка
    mySwitch.send(70741, 24);
    currentStatus=2;
  } else if (strcmp(command, "3-off") == 0) {
    mySwitch.send(70740, 24);
    currentStatus=6;
  } else if (strcmp(command, "4-on") == 0) { // четвертая розетка
    mySwitch.send(21589, 24);
    currentStatus=3;
  } else if (strcmp(command, "4-off") == 0) {
    mySwitch.send(21588, 24);
    currentStatus=7;
  } else if (strcmp(command, "mod") == 0) {
    modRestart();
  } else if (strcmp(command, "cam-on") == 0) {
    digitalWrite(3, LOW); // включение камеры. По умолчанию реле в режиме NO (камера всегда выключена)
    currentStatus=16;
    sendMail(currentStatus);
  } else if (strcmp(command, "cam-off") == 0) {
    digitalWrite(3, HIGH); // выключение камеры
    currentStatus=17;
    sendMail(currentStatus);
  } else if (strcmp(command, "liv-1") == 0) {
    txButton(button1); // Livolo кнопка 1 комната
    currentStatus=8;
  } else if (strcmp(command, "liv-2") == 0) {
    txButton(button2); // Livolo 2 комната
    currentStatus=9;
  } else if (strcmp(command, "liv-4") == 0) {
    txButton(button4); // Livolo 4 прихожая
    currentStatus=10;
  } else if (strcmp(command, "liv-5") == 0) {
    txButton(button5); // Livolo 5 гардероб
    currentStatus=11;
  } else if (strcmp(command, "liv-7") == 0) {
    txButton(button7); // Livolo 7 ванная
    currentStatus=12;
  } else if (strcmp(command, "liv-3") == 0) {
    txButton(button3); // Livolo 3 кухня
    currentStatus=13;
  } else if (strcmp(command, "liv11") == 0) {
    txButton(button11); // Выключить все Livolo
    currentStatus=14;
  } 
}

void httpResponseHome(EthernetClient c) {
  c.println("HTTP/1.1 200 OK");
  c.println("Content-Type: text/html");
  c.println();
  c.println("<html>");
  c.println("<head>");
    c.println(statusString[currentStatus]); // подтверждение команды на веб-страничке
  c.println("</body>");
  c.println("</html>");
//  currentStatus=21;
}

/**
 * HTTP Redirect to homepage
 */
void httpResponseRedirect(EthernetClient c) {
  c.println("HTTP/1.1 301 Found");
  c.println("Location: /");
  c.println();
}

/**
 * Process HTTP requests, parse first request header line and 
 * call processCommand with GET query string (everything after
 * the ? question mark in the URL).
 */
char*  httpServer() {
  EthernetClient client = server.available();
  if (client) {
    char sReturnCommand[32];
    int nCommandPos=-1;
    sReturnCommand[0] = '\0';
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if ((c == '\n') || (c == ' ' && nCommandPos>-1)) {
          sReturnCommand[nCommandPos] = '\0';
          if (strcmp(sReturnCommand, "\0") == 0) {
              httpResponseHome(client);
          } else {
            processCommand(sReturnCommand);
            httpResponseRedirect(client);
          }
          break;
        }
        if (nCommandPos>-1) {
          sReturnCommand[nCommandPos++] = c;
        }
        if (c == '?' && nCommandPos == -1) {
          nCommandPos = 0;
        }
      }
      if (nCommandPos > 30) {
//        httpResponse414(client);
        sReturnCommand[0] = '\0';
        break;
      }
    }
    if (nCommandPos!=-1) {
      sReturnCommand[nCommandPos] = '\0';
    }
    // give the web browser time to receive the data
    delay(1);
    client.stop();
    
    return sReturnCommand;
  }
  return '\0';
}

// передача команды. В инвертированном виде - то ли передатчик, то ли логика этого требует
// каждый импульс содержит сто идентичных пакетов (для длительности около 1 сек)

void txButton(byte cmd[]) { 
for (pulse= 0; pulse <= 100; pulse=pulse+1) {
for (i = 1; i < cmd[0]+1; i = i + 1) { 

  switch(cmd[i]) {
   case 1: // старт
   digitalWrite(txPin, HIGH);
   delayMicroseconds(550);
   digitalWrite(txPin, LOW);
   break;
   case 2: // ноль
   digitalWrite(txPin, LOW);
   delayMicroseconds(110);
   digitalWrite(txPin, HIGH);
   break;   
   case 3: // единица
   digitalWrite(txPin, LOW);
   delayMicroseconds(303);
   digitalWrite(txPin, HIGH);
   break;      
   case 4: // пауза
   digitalWrite(txPin, HIGH);
   delayMicroseconds(110);
   digitalWrite(txPin, LOW);
   break;      
   case 5: // низкий
   digitalWrite(txPin, HIGH);
   delayMicroseconds(290);
   digitalWrite(txPin, LOW);
   break;      
  } 
  }
 } 
 digitalWrite(txPin, LOW); // выключение передатчика, чтобы не мешал другим устройствам
}

void modRestart() {
  
    digitalWrite(5, LOW); // рестарт модема. По умолчанию реле в режиме NC (модем всегда включен), на время рестарта - переключение
    delay(15000);
    digitalWrite(5, HIGH);  
    delay(120000);
    sendMail(18);
}

void checkLine(){
  if (lineDead < 2) {
    if (mail.connect(servergoogle, 80)) {
		// связь есть, закрываем сеанс
		mail.stop();
	} else {
		// связи нет, рестарт
                modRestart();
                lineDead = lineDead + 1; // увеличиваем счетчки неуспешных попыток соединения
	}
  } // linedead if
   else { lineRest = lineRest + 1; } // каждый цикл - минута таймаута без проверки соединения
}

void sendMail(byte statusStringN){
  
    if (mail.connect(servergoogle, 80)) {
    mail.stop();
//                twitter.post(statusString[statusStringN]); // в твиттер
 		mail.connect(servermail, 25);// идем на smtp.mail.ru		
		mail.println("EHLO ArduinoMail"); // привет 
                delay(2000); // пауза ожидания отклика сервера	
		mail.println("AUTH LOGIN"); // старт авторизации
                delay(2000); // wait for a response		
		mail.println("ваш логин в кодировке BASE64"); // логин
                delay(2000); // wait for a response                
		mail.println("ваш пароль в кодировке BASE64="); // пароль
                delay(2000); // wait for a response        
		mail.println("MAIL From: адрес@arduino"); // адрес отправителя (Arduino)
                delay(2000); // wait for a response
                mail.println("RCPT To: получатель@mail"); // адрес получателя
                delay(2000); // wait for a response
                mail.println("DATA"); //  
                delay(2000); // wait for a response
                mail.println("To: получатель@mail"); // получатель в поле To письма
                mail.println("Subject: Arduino status update"); // тема письма
                mail.println();
                mail.println(statusString[statusStringN]); // тело письма
                mail.println("."); // end email
                mail.println("QUIT"); // terminate connection 
                delay(2000); // wait for a response
                mail.println();
                mail.stop();  
              } else {mail.stop();}
}
