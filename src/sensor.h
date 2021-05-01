class Sensor {                          // declaring a class Sensor
  public:
  byte adress_;
  byte pin_;                            // Input pin number
  byte pin_alarm_1_ = 0;                // Alarm output pin number 1
  byte pin_alarm_2_ = 0;                // Alarm output pin number 2
  bool status_ = false;                 // Input status (active - inactive)
  bool alarm_ = false;                  // Alarm (on - off)
  bool send_alarm_ = false;             // Notification (sent - not sent)
  char message_alarm_[20];              // Message about sensor activation
  
  bool ReadPin()
  {
    if(digitalRead(pin_) == LOW && status_ && !alarm_) // If the pin is triggered, the input is active and there is no alarm
    {
      alarm_ = true;                             // Activate the alarm
      return true; 
    }
    else
      return false;
  } 
 };   