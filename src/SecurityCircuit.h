class SecurityCircuit {                 // Объявляем класс типа SecurityCircuit
  public:
  byte adress_;                         // Номер зоны
  byte pin_;                            // Номер пина подключения датчика
  byte pin_alarm_1_ = 0;                // Номер пина подключения реле тревоги 1
  byte pin_alarm_2_ = 0;                // Номер пина подключения реле тревоги 2
  bool status_ = false;                 // Статус ввода (активен - не активен)
  bool alarm_ = false;                  // Тревога (вкл - выкл)
  bool send_alarm_ = false;             // Уведомление отправлено - не отправлено
  char message_alarm_[20];              // Текст уведомления о сработке датчика
  
  bool ReadPin()
  {
    if(digitalRead(pin_) == LOW && status_ && !alarm_) // Если сработал датчик, ввод активен и тревога не активирована
    {
      alarm_ = true;                    // Активировать тревогу
      return true; 
    }
    else
      return false;
  } 
 };   
