//==============================INCLUDES==============================================
#include <Servo.h>
//==============================DECLARATIONS==========================================

int pos = 0, buttonstate = 0;
const int buttonpin[8] = {FIRST, SECOND....., EIGHTH};
const int servopin[8] = {FIRST, SECOND....., EIGHTH};
const int ledpin[8] = {FIRST, SECOND....., EIGHTH};


//=================================FUNCTIONS=======================================
void lockopen (int servopin)
{
  Servo servo;
  servo.attach(servopin);
  
 for (pos = 0; pos <= 180; pos++)  
     {
      servo.write (pos);
      delay (15);
      } 
}

void lockclose (int servopin)
{
    Servo servo;
    servo.attach(servopin);
  
    for  (pos = 180; pos >=1; pos--)
    {
      servo.write (pos);
      delay (15);
    }
    
}
//==================================SETUP====================================

void setup()
{
  for (int i = 0; i < 8; i++)
    }
     pinMode(ledpin[i], OUTPUT);
     pinMode (buttonpin[i], INPUT);
    }
}
//=================================MAIN========================================

void loop ()
{  
  for ( ; ; ){
  for (int i = 0; i < 8; i++){
  if (digitalRead(buttonpin[i]) == HIGH)
  goto pressed;
  }
} 
 pressed:   
      lockopen(servopin[i]); //OPEN
      digitalWrite (ledpin[i], HIGH);
      
      delay (10000); //WAIT FOR ENTER
      
      digitalWrite (ledpin[i], LOW);
      lockclose(servopin[i]); //CLOSE
    
 } 
