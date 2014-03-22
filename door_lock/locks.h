#include <Servo.h>

int pos;

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
