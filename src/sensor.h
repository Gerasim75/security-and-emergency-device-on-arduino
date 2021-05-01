class Sensor {                          // Объявляем класс Sensor
  public:
  byte adress_;
  byte pin_;                            // Номер пина ввода
  byte pin_alarm_1_ = 0;                // Номер пина вывода тревоги 1
  byte pin_alarm_2_ = 0;                // Номер пина вывода тревоги 2
  bool status_ = false;                 // Статус ввода (активен - не активен)
  bool alarm_ = false;                  // Тревога (вкл - выкл)
  bool send_alarm_ = false;             // Уведомление отправлено - не отправлено
  char message_alarm_[20];              // Сообщение о сработке датчика
  
  bool ReadPin()
  {
    if(digitalRead(pin_) == LOW && status_ && !alarm_) // Если сработал пин, ввод активен и нет тревоги
    {
      alarm_ = true;                    // Активировать тревогу
      return true; 
    }
    else
      return false;
  } 
 }; 