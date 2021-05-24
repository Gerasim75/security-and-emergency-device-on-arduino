/// My_feacture

//// Голосовые уведомления звонком, киррилические смс, 16 выходов реле.

/////////////////////////// Охранно-аварийная gsm-сигнализация с sms-управляемыми реле ////////////////////////////////

#include  <SoftwareSerial.h>                                
#include  <EEPROM.h> 
#include  <CyberLib.h> 
#include  "SecurityCircuit.h"
#include  "function.h"

//#define _SS_MAX_RX_BUFF 128                             // регулируем размер RX буфера SoftwareSerial
                        
#define ZONE_1 8           // Вход                 
#define ZONE_2 9           // Вход                     
#define ZONE_3 10          // Вход                          
#define ZONE_4 11          // Вход пожар
#define ZONE_5 12          // Вход газ
#define ZONE_6 19          // Вход вода
#define KEYBOARD_PIN 18    // Вход клавиатура 
#define VOLT_PIN 17        // Вход сеть
#define BUZZER_PIN 6       // На плате
#define BUTTON_PIN 16      // На плате
#define DELAY_PLAY_TRACK 3 // Пауза для воспроизведения трека
                                                          
SoftwareSerial SIM800(14, 15);                                  // RX, TX (пины подключения сим-модуля) 

String operator_code = "";                                      // Переменная кода оператора сим
String _response     =  "";                                     // Переменная для хранения ответа модуля
String phones[5] = {"","","","",""};                            // Телефоны админа
String temp = "";
int AT_counter = 1;
int balance_send;
int dataPin  = 2;  //Пин подключен к DS входу 74HC595
int latchPin = 3;  //Пин подключен к ST_CP входу 74HC595
int clockPin = 4;  //Пин подключен к SH_CP входу 74HC595
unsigned long timer_ring = millis();                            // Инициализируем таймер для входящего звонка
unsigned long timer_balance = millis();                         // Инициализируем таймер для отправки уведомлений о балансе на сим карте
unsigned long timer_alarm_time = millis();                      // Инициализируем таймер сирены
unsigned long timer_delay = millis();                           // Таймер задержки срабатывания
unsigned long last_update  =  millis();                         // Время последнего обновления      
unsigned long update_period =  20000;                           // Проверять каждые 20 секунд
unsigned long time_alarm  =  90000;                             // Время работы сирены
unsigned int timer_delay_off;                                   // Время задержки сработки тревоги в миллисекундах.
unsigned int timer_delay_on;                                    // Время задержки постановки на охрану в миллисекундах.
unsigned long time_balance = 86400000;                          // Время периодичности проверки баланса в миллисекундах (сутки 86400000 мс)
long counter_errors = 0;                                        // Счетчик итераций при ошибке обработки входящего смс 
unsigned long alarm_timer = 0;
bool flag_zone1 = false;
bool flag_delay_zone1 = false;
bool flag_keyboard = false;
bool flag_balance = false;
bool flag_loop = false;
bool flag_call = false;                                         // Флаг входящего звонка  
bool flag_volt = false;                                         // Флаг отсутствия напряжения в сети
bool flag_tel = false;                                          // Флаг исходящего звонка
bool mode = false;                                              // Режим охраны вкл/выкл
bool flag_alarm = false;                                        // Счетчик звонков о сработках
bool flag_timer_sms = false;                                    // Флаг обработки входящего смс
bool hasmsg  =  false;                                          // Флаг наличия сообщений к удалению
bool flag_alarm_timer = 0;
byte macros_id = 0;                                             // Переменная хранения макросов
bool macros_1[6] = {1,1,1,1,1,1};                               // Активны все зоны
bool macros_2[6] = {1,1,1,0,0,0};                               // Активны только охранные зоны
bool macros_3[6] = {0,0,0,1,1,1};                               // Активны только аварийные зоны
bool macros_4[6] = {1,0,0,1,1,1};                               // Активны аварийные зоны и одна охранная
bool macros_5[6] = {1,1,1,1,0,0};                               // Активны охранные зоны и одна аварийная (пожар)
byte byteToSend1 = 0;                                           // Создаем пустой байт B00000000 для первого сдвигового регистра
byte byteToSend2 = 0;                                           // Создаем пустой байт B00000000 для второго сдвигового регистра
byte triggered[6] = {9,9,9,9,9,9};                              // Массив адресов сработавших датчиков
byte counter_triggered = 0;                                     // Счетчик сработок датчиков
byte counter_admins = 5;                                        // Количество администраторов
SecurityCircuit zone[6];                                        // Массив объектов типа "охранный контур"

///////////////////////////////////////// Функция настроек контроллера /////////////////////////////////////////

void setup() 
{
  D2_Out;
  D3_Out;                                    // клапан газа
  D4_Out;
  D5_Out;                                    // смс реле 1
  D6_Out;
  D7_Out;                                    // смс реле 2
  D8_In;
  D9_In;
  D10_In;
  D11_In;
  D12_In;
  D13_Out;
  D16_In;
  D17_In;
  D18_In;
  D19_In;

  zone[0].pin_ = ZONE_1;
  zone[1].pin_ = ZONE_2;
  zone[2].pin_ = ZONE_3;
  zone[3].pin_ = ZONE_4;
  zone[4].pin_ = ZONE_5;
  zone[5].pin_ = ZONE_6;

  Serial.begin(9600);                                  // Скорость обмена данными с компьютером
  SIM800.begin(9600);                                  // Скорость обмена данными с сим модулем
  
  InitialZones();
  InitialEeprom();
  
  balance_send = ((String)temp).toInt();               ////////////////////////////////////////////////////////////////
  temp = F("Systema v5.3.2 Status OFF. Phones - ");    //
  for(int i = 0; i < counter_admins; i ++ )            //
  {                                                    //
    if(phones[i][0] == '+')                            //
    {                                                  //
      temp += phones[i];                               //
      if(phones[i + 1][0] == '+')                      //
      {                                                //
        temp += F(", ");                               //
      }                                                //      
    }                                                  //   Формируем сообщение о перезагрузке контроллера
  }                                                    //
  temp += F(". ");                                     //
  temp += F("Time ON ");                               //
  temp += timer_delay_on/1000;                         //
  temp += F(". Time OFF ");                            //  
  temp += timer_delay_off/1000;                        //
  temp += F(". Min balance ");                         //
  temp += balance_send;                                //
  temp += F(".Code ");                                 //
  temp += operator_code;                               //
  temp += F(".");                                      ////////////////////////////////////////////////////////////////

  TestModem();                                         // Проверяем работоспособность сим-модуля
  InitialModem();                                      // Инициализируем сим-модуль                                       
  delay(1000);
  SendSMS(temp);                                       // Отправляем смс уведомление о перезагрузке контроллера администратору
  tone(BUZZER_PIN, 1915);                              // Сигнализируем динамиком о прохождении этапа загрузки контроллера
  delay(1000);
  noTone(6);
  last_update  =  millis();                            // Обнуляем таймер    
}

/////////////////////////////////////////////// Главная функция ////////////////////////////////////////////

void loop() 
{ 
  flag_loop = true;
  GetKeyboard();                                      // Ожидаем данные с клавиатуры
  GetBalanceSim();                                    // Проверяем баланс на сим карте
  SetDingDong();                                      // Свето-звуковая индикация входящего звонка 
  GetNewSMS();                                        // Получаем входящие смс
  
  if (SIM800.available())   {                         // Если модем, что-то отправил...
    _response  =  WaitResponse();                     // Получаем ответ от модема для анализа
    _response.trim();                                 // Убираем лишние пробелы в начале и конце
    if (_response.indexOf("+CMTI:") > -1)             // Если пришло сообщение об отправке SMS
    {    
     flag_timer_sms = true;      
    }
    if (_response.startsWith("RING"))                // Есть входящий вызов
    { 
      GetIncomingCall();                             // Обрабатываем его
    }  
    else                                             // Если нет
    {
      flag_call = false;                             // Опускаем флаг входящего вызова
    }
  }
  if(Serial.available())                             // Ожидаем команды по Serial...
  {                         
    SIM800.write(Serial.read());                     // ...и отправляем полученную команду модему
  }
  if(SIM800.available())                             // Ожидаем команды от модема ...
  {                         
    Serial.write(SIM800.read());                     // ...и отправляем полученную команду в Serial    
  }
  if(!mode)                                          // Если снято с охраны (режим выкл)
  { 
    flag_delay_zone1 = false;
    flag_zone1 = false;
    flag_tel = false;  
    flag_alarm_timer = false; 
    flag_alarm = false;                              // Опускаем флаг сообщений о сработке
    counter_triggered = 0;
    for(int i = 0; i < 6; i ++ )
    {
      zone[i].alarm_ = false;
      zone[i].send_alarm_ = false;
      triggered[i] = 9;
    }            
    D2_Low;                                          // Выключаем аварийный сигнал                 
    D13_Low;                                         // Выключаем светодиод
    noTone(6);                                       // Выключаем сирену
    //Serial.println(F("!mode"));                    // Если нужно отправляем сообщение в монитор порта 
  }
  else                                               // Если стоит на охране (режим вкл)
  {
    if((alarm_timer + time_alarm) < millis())
    {
      D2_Low;                                        //  Если время работы сирены вышло отключаем ее
    }
//    Serial.println(F("mode"));                     // Если нужно отправляем сообщение в монитор порта               
    D13_High;                                        // Включаем светодиод
    GetSensors();                                    // Смотрим на датчики
    AlarmMessages();                                 // Отправляем необходимые уведомления
  }
  GetVoltage();                                      // Контролируем питание системы
}

////////////////////////////////////////////// Конец главной функции //////////////////////////////////////////////

/////////////////////////////////////////// Функция оповещения звонком ////////////////////////////////////////////

void Call(String & num, byte adress)
{
  String comand = F("ATD");
  SendATCommand("AT+COLP=0", true);                // Режим ожидания ответа
  //SendATCommand("AT + VTD = 4", true);             // Длительность тоновых сигналов для AT + VTD. Значение параметра 1..255
  comand = comand + num;                           // Собираем команду
  comand = comand + ";";                           // Добавляем символ в конец команды
  SendATCommand(comand, true);                     // Отправляем команду вызова
  //delay(3000);                                     // Пауза до сброса вызова
  //SendATCommand("ATH0", true);                     // Сбрасываем вызов через 30 сек

 switch(adress)
 {
    case 0:
      SIM800.print(F("AT+CREC=4,\"C:\\User\\1.amr\",0,80"));   // Проигрываем звуковой файл
      delay(DELAY_PLAY_TRACK);                                 // Делаем паузу для воспроизведения трека 
      SendATCommand("ATH0", true);                             // Сбрасываем вызов
      break;
    case 1:
      SIM800.print(F("AT+CREC=4,\"C:\\User\\2.amr\",0,80"));   // Проигрываем звуковой файл
      delay(DELAY_PLAY_TRACK);                                 // Делаем паузу для воспроизведения трека 
      SendATCommand("ATH0", true);                             // Сбрасываем вызов
      break;
    case 2:
      SIM800.print(F("AT+CREC=4,\"C:\\User\\3.amr\",0,80"));   // Проигрываем звуковой файл
      delay(DELAY_PLAY_TRACK);                                 // Делаем паузу для воспроизведения трека 
      SendATCommand("ATH0", true);                             // Сбрасываем вызов
         break;
    case 3:
      SIM800.print(F("AT+CREC=4,\"C:\\User\\4.amr\",0,80"));   // Проигрываем звуковой файл
      delay(DELAY_PLAY_TRACK);                                 // Делаем паузу для воспроизведения трека 
      SendATCommand("ATH0", true);                             // Сбрасываем вызов
      break;
    case 4:
      SIM800.print(F("AT+CREC=4,\"C:\\User\\5.amr\",0,80"));   // Проигрываем звуковой файл
      delay(DELAY_PLAY_TRACK);                                 // Делаем паузу для воспроизведения трека 
      SendATCommand("ATH0", true);                             // Сбрасываем вызов
      break;
    case 5:
      SIM800.print(F("AT+CREC=4,\"C:\\User\\6.amr\",0,80"));   // Проигрываем звуковой файл
      delay(DELAY_PLAY_TRACK);                                 // Делаем паузу для воспроизведения трека 
      SendATCommand("ATH0", true);                             // Сбрасываем вызов
      break;
   }

 //  sendATCommand(AT+CPAMR="001.amr",0)           // Воспроизвести файл в сторону удаленного абонента
//delay(3000);                                     // Пауза до сброса вызова
//   sendATCommand("ATH0", true);                  // Сбрасываем вызов
  
}

//////////////////////////////////////// Функция отправки команды сим модулю /////////////////////////////////////

String SendATCommand(String cmd, bool waiting) 
{
  String _resp  =  "";                                            // Переменная для хранения результата
  SIM800.println(cmd);                                            // Отправляем команду модулю
  if (waiting)                                                    // Если необходимо дождаться ответа...
  {                                                 
    _resp  =  WaitResponse();                                     // ... ждем, когда будет передан ответ
    // Если Echo Mode выключен (ATE0), то эти 3 строки можно закомментировать
    if (_resp.startsWith(cmd))                                    
    {                                  
      _resp  =  _resp.substring(_resp.indexOf("\r", cmd.length()) + 2); // Убираем из ответа дублирующуюся команду
    }
  }
  return _resp;                                                   // Возвращаем результат. Пусто, если проблема
}

//////////////////////////////////////// Функция ожидания ответа от сим модуля ///////////////////////////////////////////////////////

String WaitResponse()                                             // Функция ожидания ответа и возврата полученного результата
{                                          
  String _resp  =  "";                                            // Переменная для хранения результата
  unsigned long _timeout  =  millis() + 10000;                    // Переменная для отслеживания таймаута (10 секунд)
  while (!SIM800.available() && millis() < _timeout)  {};         // Ждем ответа 10 секунд, если пришел ответ или наступил таймаут, то...
  if (SIM800.available())                                         // Если есть, что считывать...
  {                                       
    _resp  =  SIM800.readString();                                // ... считываем и запоминаем
  }
  return _resp;                                                   // ... возвращаем результат. Пусто, если проблема
}

///////////////////////////////////////////// Функция контроля датчиков //////////////////////////////////////////////////////////////

void GetSensors()
{
  for(int i = 0; i < 6; i ++ )                           // Проходим по сенсорам
  {
    if(zone[i].ReadPin())                                // Если есть сработка
    {
      flag_alarm = true;                                 // Поднимаем флаг сработки
      triggered[counter_triggered] = zone[i].adress_;    // Записываем адрес зоны в массив сработок
      counter_triggered ++ ;                             // Увеличиваем счетчик адресов
    }
  }
  if(triggered[0] == 0 && !flag_zone1)                   // Если первой сработала зона 1 и это первая сработка
  {
    timer_delay = millis();                              // Стартуем таймер задержки
    flag_zone1 = true;                                   // Поднимаем флаг сработки в зоне 1
  }
}

/////////////////////////////////////////////// Функция уведомлений //////////////////////////////////////////////////////////////////////
                                                                 
void AlarmMessages()
{
  if(flag_alarm)
  {
    for(int i = 0; i < counter_triggered; i ++ )
    {
      if(zone[triggered[i]].alarm_ == true && zone[triggered[i]].send_alarm_ == false) // Если поднят флаг сработки датчика и уведомление не отправлялось
      {
        if(i == 0 && triggered[i] == 0)                                                // Если сработка в зоне 1
        {
          if(millis() - timer_delay > timer_delay_off)                                 // Если сработал таймер
          {     
            ActivateRelay(zone[triggered[i]].pin_alarm_1_, zone[triggered[i]].pin_alarm_2_); // Активируем реле тревоги
            sendSMSinPDU(zone[triggered[i]].message_alarm_);                           // Отправляем смс уведомление
            if(!flag_tel)                                                              // Если еще не было звонка администратору
            {
              flag_tel = true;                                                         // Поднимаем флаг звонка  
              Call(phones[0], zone[i].adress_);                                        // Звоним администратору
            }
            zone[triggered[i]].send_alarm_ = true;                                     // Поднимаем флаг отправленного уведомления
            flag_delay_zone1 = true;                                                   // Поднимаем флаг задержки сработки
            break;                 
          }
        }
        else
        {
          if(triggered[i] == 1 && flag_zone1 && !flag_delay_zone1) break;
          ActivateRelay(zone[triggered[i]].pin_alarm_1_, zone[triggered[i]].pin_alarm_2_);   // Активируем реле тревоги
          sendSMSinPDU(zone[triggered[i]].message_alarm_);                             // Отправляем смс уведомление
          if(!flag_tel)                                                                // Если еще не было звонка администратору
          {
            flag_tel = true;                                                           // Поднимаем флаг звонка  
            Call(phones[0], zone[i].adress_);                                          // Звоним администратору
          }
          zone[triggered[i]].send_alarm_ = true;                                       // Поднимаем флаг отправленного уведомления
          break;                                                                       // Выходим из цикла
        }
      }
    }
  }
}
     
///////////////////////////////////////////////// Фунция парсинга СМС /////////////////////////////////////////////////

void ParseSMS(String & msg)                                     // Парсим SMS
{ 
                                  
  String msgheader   =  "";
  String msgbody     =  "";
  String msgphone    =  "";

  msg  =  msg.substring(msg.indexOf("+CMGR: "));
  msgheader  =  msg.substring(0, msg.indexOf("\r"));            // Выдергиваем телефон
  msgbody  =  msg.substring(msgheader.length() + 2);
  msgbody  =  msgbody.substring(0, msgbody.lastIndexOf("OK"));  // Выдергиваем текст SMS
  msgbody.trim();
  int firstIndex  =  msgheader.indexOf("\",\"") + 3;
  int secondIndex  =  msgheader.indexOf("\",\"", firstIndex);
  msgphone  =  msgheader.substring(firstIndex, secondIndex);

  if (msgphone == phones[0] || msgphone == phones[1] || msgphone == phones[2]
   || msgphone == phones[3] || msgphone == phones[4])           // Если телефон админа, то...
  {     
    SetLedState(msgbody, msgphone);                             // ...выполняем команду
  }
}
 
///////////////////////////////////////// Функция чтения кода команды из смс ///////////////////////////////////////////

void SetLedState(String & result, String & msgphone)   
{
  bool correct  =  false;                          // Для оптимизации кода, переменная корректности команды
  if (result.length()  ==  1)                      // Если в сообщении одна цифра
  {
    int ledState  =  ((String)result[0]).toInt();  // Получаем первую цифру команды - состояние (0 - выкл, 1 - вкл)
    if (ledState  >=  0 && ledState  <=  5) 
    {                                              // Если все нормально, исполняем команду
      if (ledState == 1) 
      {
        EEPROM.write(106, 0);                      // Записываем его в память  
        macros_id = 0;
        InitialMacros();                           // Инициализируем макрос
        sendSMSinPDU(F("Режим 1 Вкл"));            // Отправляем смс с результатом администратору 
        mode = true;       
      }
      if(ledState == 2) 
      {
        EEPROM.write(106, 1);                      // Записываем его в память
        macros_id = 1;
        InitialMacros();                           // Инициализируем макрос 
        sendSMSinPDU(F("Режим 2 Вкл"));            // Отправляем смс с результатом администратору 
        mode = true;              
      }
      if(ledState == 3)
      {
        EEPROM.write(106, 2);                      // Записываем его в память
        macros_id = 2;
        InitialMacros();                           // Инициализируем макрос   
        sendSMSinPDU(F("Режим 3 Вкл"));            // Отправляем смс с результатом администратору 
        mode = true;            
      }
      if(ledState == 4)
      { 
        EEPROM.write(106, 3);                      // Записываем его в память
        macros_id = 3;
        InitialMacros();                           // Инициализируем макрос
        sendSMSinPDU(F("Режим 4 Вкл"));            // Отправляем смс с результатом администратору    
        mode = true;               
      }
      if(ledState == 5) 
      {
        EEPROM.write(106, 4);                      // Записываем его в память
        macros_id = 4;
        InitialMacros();                           // Инициализируем макрос  
        sendSMSinPDU(F("Режим 5 Вкл"));            // Отправляем смс с результатом администратору
        mode = true;
      }
      if(ledState == 0) 
      {
        mode = false;                              // Снимаем с охраны
        sendSMSinPDU(F("Режим Выкл"));             // Отправляем смс с результатом администратору
      }     
      tone(BUZZER_PIN, 1915);                      // Сигнализируем динамиком о смене режима охраны
      delay(1000);
      noTone(6); 
      correct  =  true;                            // Флаг корректности команды
    }
    if (!correct) 
    {
      sendSMSinPDU(F("Некорректная команда!"));    // Отправляем смс с результатом администратору
    }
  }
  else
  {
    if(result.length()  ==  13 && msgphone == phones[0])    // Если в сообщении номер телефона и отправитель админ 1
    {
      bool flag_number = false;
      for(int k = 1; k < counter_admins; k ++ )             // Проходим по списку номеров
      {
        if(phones[k] == result)                             // Если номер есть в позиции k
         {
          flag_number = true;                               // Поднимаем флаг наличия номера
          switch(k)                                         // Ищем его в памяти EEPROM и удаляем
          {
            case 1:
                  for(int i = 20; i < 33; i ++ )
                    {
                      EEPROM.write(i, (byte)' ');           // Перезаписываем номер админа 2 пробелами                       
                    }
                    phones[k] = " ";                        // Очищаем переменную
                    tone(BUZZER_PIN, 1915);                 // Сигнализируем динамиком о удалении номера дублера
                    delay(1000);
                    noTone(6);
                    sendSMSinPDU(F("Номер 2 удален!"));
                    break;
            case 2:
                  for(int i = 40; i < 53; i ++ )
                    {
                      EEPROM.write(i, (byte)' ');           // Перезаписываем номер админа 3 пробелами
                    }
                    phones[k] = " ";                        // Очищаем переменную
                    tone(BUZZER_PIN, 1915);                 // Сигнализируем динамиком о удалении номера дублера
                    delay(1000);
                    noTone(6);
                    sendSMSinPDU(F("Номер 3 удален!"));
                    break;
            case 3:
                  for(int i = 60; i < 73; i ++ )
                   {
                     EEPROM.write(i, (byte)' ');            // Перезаписываем номер админа 4 пробелами
                   }
                   phones[k] = " ";                         // Очищаем переменную
                   tone(BUZZER_PIN, 1915);                  // Сигнализируем динамиком о удалении номера дублера
                   delay(1000);
                   noTone(6);
                   sendSMSinPDU(F("Номер 4 удален!"));
                   break;
            case 4:
                  for(int i = 80; i < 93; i ++ )
                  {
                    EEPROM.write(i, (byte)' ');             // Перезаписываем номер админа 5 пробелами
                  }
                  phones[k] = " ";                          // Очищаем переменную
                  tone(BUZZER_PIN, 1915);                   // Сигнализируем динамиком о удалении номера дублера
                  delay(1000); 
                  noTone(6);
                  sendSMSinPDU(F("Номер 5 удален!"));
                  break;
            default:                                        // Если что-то пошло не так
            sendSMSinPDU(F("Номер не удален!"));  
          }
        }
      }
      if(flag_number)
      {
        result = " ";
        return;
      }
      for(int k = 1; k < counter_admins;k ++ )              // Проходим по списку номеров
      {
        if (phones[k][0] != '+' && result[0] == '+' && !flag_number) // Если есть место в позиции k, номер валидный и еще не записан
        {
          switch(k)
          {
            case 1:
                  for(int i = 20, j = 0; i < 33; i ++ , j ++ )
                    {
                      EEPROM.write(i, (byte)result[j]);     // Записываем в память номер админа 2                       
                    }
                    phones[k] = result;                     // Записываем в переменную номер админа 2
                    tone(BUZZER_PIN, 1915);                 // Сигнализируем динамиком о принятии номера дублера
                    delay(1000);
                    noTone(6);
                    sendSMSinPDU(F("Номер 2 принят!")); 
                    flag_number = true;                     // Поднимаем флаг записи номера
                    break;
            case 2:
                  for(int i = 40, j = 0; i < 53; i ++ , j ++ )
                    {
                      EEPROM.write(i, (byte)result[j]);     // Записываем в память номер админа 3
                    }
                    phones[k] = result;                     // Записываем в переменную номер админа 3
                    tone(BUZZER_PIN, 1915);                 // Сигнализируем динамиком о принятии номера дублера
                    delay(1000);
                    noTone(6);
                    sendSMSinPDU(F("Номер 3 принят!"));
                    flag_number = true;                     // Поднимаем флаг записи номера
                    break;
            case 3:
                  for(int i = 60, j = 0; i < 73; i ++ , j ++ )
                   {
                      EEPROM.write(i, (byte)result[j]);     // Записываем в память номер админа 4
                   }
                   phones[k] = result;                      // Записываем в переменную номер админа 4
                   tone(BUZZER_PIN, 1915);                  // Сигнализируем динамиком о принятии номера дублера
                   delay(1000);
                   noTone(6);
                   sendSMSinPDU(F("Номер 4 принят!"));
                   flag_number = true;                      // Поднимаем флаг записи номера
                   break;
            case 4:
                  for(int i = 80, j = 0; i < 93; i ++ , j ++ )
                  {
                      EEPROM.write(i, (byte)result[j]);     // Записываем в память номер админа 5
                  }
                  phones[k] = result;                       // Записываем в переменную номер админа
                  tone(BUZZER_PIN, 1915);                   // Сигнализируем динамиком о принятии номера дублера
                  delay(1000);
                  noTone(6);
                  sendSMSinPDU(F("Номер 5 принят!"));
                  flag_number = true;                       // Поднимаем флаг записи номера
                  break;
            default:
            sendSMSinPDU(F("Номер не принят!"));  
          }
        }                 
      }                            
    }
    else
    {
      if(result.length()  ==  5)                 // Если в сообщении 5-значная команда
      {
        String command = "";                     // Объявляем переменную для записи команд
        String item = "";                        // Объявляем переменную для записи значений
        command += result[0];                    // Получаем команду
        command += result[1];              
        item += result[3];                       // Получаем значение
        item += result[4]; 
        if(command == "On")                      // Если команда "задержка включения режима охраны"
        {
          for(int i = 100, j = 0; i < 102; i ++ , j ++ )
          {
            EEPROM.write(i, (byte)item[j]);            // Записываем время задержки в память EEPROM           
          }
          timer_delay_on = item.toInt() * 1000;        // Записываем время задержки в переменную
          item += F(" секунд до Вкл");                 // Формируем уведомление
          sendSMSinPDU(item);                          // Отправляем уведомление 
        }
        if(command == "Of")                            // Если команда "задержка выключения режима охраны"
        {
          for(int i = 102, j = 0; i < 104; i ++ , j ++ )
          {
            EEPROM.write(i, (byte)item[j]);            // Записываем время задержки в память EEPROM            
          }
          timer_delay_off = item.toInt() * 1000;       // Записываем время задержки в переменную
          item += F(" секунд до Выкл");                // Формируем уведомление
          sendSMSinPDU(item);                          // Отправляем уведомление
        } 
            
        if(command == "Bl")                            // Если команда "минимальный баланс для уведомления"
        {
          for(int i = 104, j = 0; i < 106; i ++ , j ++ )
          {
            EEPROM.write(i, (byte)item[j]);            // Записываем минимальный баланс для уведомления в память EEPROM            
          }
          balance_send = item.toInt();                 // Записываем минимальный баланс для уведомления в переменную
          item += F(" грн. на счете");                 // Формируем уведомление
          sendSMSinPDU(item);                          // Отправляем уведомление
        }
        if(result[0] == '*' && result[4] == '#')       // Если команда "код проверки баланса" 
        {
          operator_code = "";                          // Очищаем переменную хранения кода проверки баланса
          for(int i = 107, j = 0; i < 112; i ++ , j ++ )
          {
            EEPROM.write(i, (byte)result[j]);          // Записываем код проверки баланса в память EEPROM
            operator_code += result[j];                // Записываем код проверки баланса в переменную           
          }  
          sendSMSinPDU(operator_code);                 // Отправляем уведомление      
        }
      }
      else
      {
        if(result.length()  ==  2 || result.length()  ==  3)    // Если в сообщении 2х или 3х-значная команда
        {
          SetControlledRelay(result);
        }
      }
    }
  }
}

////////////////////////////////////// Функция обработки смс команд для 16 управляемых реле ////////////////////////////////////////////////////////

void SetControlledRelay(String & result)
{
  int command;                                          // Объявляем переменную для записи команд
  command = atoi(result.c_str());                       // Получаем значение в переменную и конвертируем в число
  switch (command)
  {
  case 10:
    bitWrite(byteToSend1, 0, 0);                        // Устанавливаем значение байта B*******0 (выключаем реле 1)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 1 Выкл"));                     // Отправляем уведомление о выключении реле 1
    break;
  case 11:
    bitWrite(byteToSend1, 0, 1);                        // Устанавливаем значение байта B*******1 (включаем реле 1)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 1 Вкл"));                      // Отправляем уведомление о включении реле 1
    break;
  case 20:
    bitWrite(byteToSend1, 1, 0);                        // Устанавливаем значение байта B******0* (выключаем реле 2)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 2 Выкл"));                     // Отправляем уведомление о выключении реле 2
    break;
  case 21:
    bitWrite(byteToSend1, 1, 1);                        // Устанавливаем значение байта B******1* (включаем реле 2)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 2 Вкл"));                      // Отправляем уведомление о включении реле 2
    break;
  case 30:
    bitWrite(byteToSend1, 2, 0);                        // Устанавливаем значение байта B*****0** (выключаем реле 3)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 3 Выкл"));                     // Отправляем уведомление о выключении реле 3
    break;
  case 31:
    bitWrite(byteToSend1, 2, 1);                        // Устанавливаем значение байта B*****1** (включаем реле 3)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 3 Вкл"));                      // Отправляем уведомление о включении реле 3
    break;
  case 40:
    bitWrite(byteToSend1, 3, 0);                        // Устанавливаем значение байта B****0*** (выключаем реле 4)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
   shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1);  // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 4 Выкл"));                     // Отправляем уведомление о выключении реле 4
    break;
  case 41:
    bitWrite(byteToSend1, 3, 1);                        // Устанавливаем значение байта B****1*** (включаем реле 4)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 4 Вкл"));                      // Отправляем уведомление о включении реле 4
    break;
  case 50:
    bitWrite(byteToSend1, 4, 0);                        // Устанавливаем значение байта B***0**** (выключаем реле 5)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 5 Выкл"));                     // Отправляем уведомление о выключении реле 5
    break;
  case 51:
    bitWrite(byteToSend1, 4, 1);                        // Устанавливаем значение байта B***1**** (включаем реле 5)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 5 Вкл"));                      // Отправляем уведомление о включении реле 5
    break;
  case 60:
    bitWrite(byteToSend1, 5, 0);                        // Устанавливаем значение байта B**0***** (выключаем реле 6)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 6 Выкл"));                     // Отправляем уведомление о выключении реле 6
    break;
  case 61:
    bitWrite(byteToSend1, 5, 1);                        // Устанавливаем значение байта B**1***** (включаем реле 6)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 6 Вкл"));                      // Отправляем уведомление о включении реле 6
    break;
  case 70:
    bitWrite(byteToSend1, 6, 0);                        // Устанавливаем значение байта B*0****** (выключаем реле 7)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 7 Выкл"));                     // Отправляем уведомление о выключении реле 7
    break;
  case 71:
    bitWrite(byteToSend1, 6, 1);                        // Устанавливаем значение байта B*1****** (включаем реле 7)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 7 Вкл"));                      // Отправляем уведомление о включении реле 7
    break;
  case 80:
    bitWrite(byteToSend1, 7, 0);                        // Устанавливаем значение байта B0******* (выключаем реле 8)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 8 Выкл"));                     // Отправляем уведомление о выключении реле 8
    break;
  case 81:
    bitWrite(byteToSend1, 7, 1);                        // Устанавливаем значение байта B1******* (включаем реле 8)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 8 Вкл"));                      // Отправляем уведомление о включении реле 8
    break;
  case 90:
    bitWrite(byteToSend2, 0, 0);                        // Устанавливаем значение байта B*******0 (выключаем реле 9)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 9 Выкл"));                     // Отправляем уведомление о выключении реле 9
    break;
  case 91:
    bitWrite(byteToSend2, 0, 1);                        // Устанавливаем значение байта B*******1 (включаем реле 9)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 9 Вкл"));                      // Отправляем уведомление о включении реле 9
    break;
    ////////////////////////////////////////////////
  case 100:
    bitWrite(byteToSend2, 1, 0);                        // Устанавливаем значение байта B******0* (выключаем реле 10)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 10 Выкл"));                    // Отправляем уведомление о выключении реле 10
    break;
  case 101:
    bitWrite(byteToSend2, 1, 1);                        // Устанавливаем значение байта B******1* (включаем реле 10)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 10 Вкл"));                     // Отправляем уведомление о включении реле 10
    break;
  case 110:
    bitWrite(byteToSend2, 2, 0);                        // Устанавливаем значение байта B*****0** (выключаем реле 11)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 11 Выкл"));                    // Отправляем уведомление о выключении реле 11
    break;
  case 111:
    bitWrite(byteToSend2, 2, 1);                        // Устанавливаем значение байта B*****1** (включаем реле 11)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 11 Вкл"));                     // Отправляем уведомление о включении реле 11
    break;
  case 120:
    bitWrite(byteToSend2, 3, 0);                        // Устанавливаем значение байта B****0*** (выключаем реле 12)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
   shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1);  // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 12 Выкл"));                    // Отправляем уведомление о выключении реле 12
    break;
  case 121:
    bitWrite(byteToSend2, 3, 1);                        // Устанавливаем значение байта B****1*** (включаем реле 12)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 12 Вкл"));                     // Отправляем уведомление о включении реле 12
    break;
  case 130:
    bitWrite(byteToSend2, 4, 0);                        // Устанавливаем значение байта B***0**** (выключаем реле 13)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 13 Выкл"));                    // Отправляем уведомление о выключении реле 13
    break;
  case 131:
    bitWrite(byteToSend2, 4, 1);                        // Устанавливаем значение байта B***1**** (включаем реле 13)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 13 Вкл"));                     // Отправляем уведомление о включении реле 13
    break;
  case 140:
    bitWrite(byteToSend2, 5, 0);                        // Устанавливаем значение байта B**0***** (выключаем реле 14)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 14 Выкл"));                    // Отправляем уведомление о выключении реле 14
    break;
  case 141:
    bitWrite(byteToSend2, 5, 1);                        // Устанавливаем значение байта B**1***** (включаем реле 14)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 14 Вкл"));                     // Отправляем уведомление о включении реле 14
    break;
  case 150:
    bitWrite(byteToSend2, 6, 0);                        // Устанавливаем значение байта B*0****** (выключаем реле 15)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 15 Выкл"));                    // Отправляем уведомление о выключении реле 15
    break;
  case 151:
    bitWrite(byteToSend2, 6, 1);                        // Устанавливаем значение байта B*1****** (включаем реле 15)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 15 Вкл"));                     // Отправляем уведомление о включении реле 15
    break;
  case 160:
    bitWrite(byteToSend2, 7, 0);                        // Устанавливаем значение байта B0******* (выключаем реле 16)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 16 Выкл"));                    // Отправляем уведомление о выключении реле 16
    break;
  case 161:
    bitWrite(byteToSend2, 7, 1);                        // Устанавливаем значение байта B1******* (включаем реле 16)
    digitalWrite(latchPin, LOW);                        // Открываем сдвиговый регистр для записи
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend1); // Записываем первый байт
    shiftOut(dataPin, clockPin, LSBFIRST, byteToSend2); // Записываем второй байт
    digitalWrite(latchPin, HIGH);                       // Закрываем защелку сдвигового регистра (активируем пины)
    sendSMSinPDU(F("Реле 16 Вкл"));                     // Отправляем уведомление о включении реле 16
    break;
  default:
    break;
  }
}

///////////////////////////////////////// Функция получения БАЛАНСА СИМ карты в меню /////////////////////////////

float GetBalans(String & code) 
{
bool flag = true;
_response  =  SendATCommand("AT+CUSD=1,\"" + code + "\"", true);   // Отправляем USSD-запрос баланса
 while(flag)                                              // Запускаем безконечный цикл для ожидания ответа на запрос
 {
  if (SIM800.available())                                 // Если модем, что-то отправил...
  {
    _response  =  WaitResponse();                         // Получаем ответ от модема для анализа 
    _response.trim();                                     // Убираем лишние пробелы в начале и конц
    if (_response.startsWith("+CUSD:"))                   // Если пришло уведомление о USSD-ответе (балансе на счету)
    {   
      if (_response.indexOf("\"")  >  -1)                 // Если ответ содержит кавычки, значит есть сообщение (предохранитель от "пустых" USSD-ответов)
      {       
        String msgBalance  =  _response.substring(_response.indexOf("\"") + 1, 50);  // Получаем непосредственно текст
        msgBalance  =  msgBalance.substring(0, msgBalance.indexOf("\""));
        float balance  =  GetFloatFromString(msgBalance);                            // Извлекаем информацию о балансе     
        flag = false;                                                                // Выходим из цикла в основное меню
        return balance;
      }
    }
    else
    {
      return 0;
    }
  }
 }
 return 0;
}
///////////////////////// Функция ИЗВЛЕЧЕНИЯ ЦИФР из сообщения - для парсинга баланса из USSD-запроса ////////////////

float GetFloatFromString(String & str) {            
  bool flag  =  false;
  String result  =  "";
  str.replace(",", ".");                                   // Если в качестве разделителя десятичных используется запятая - меняем её на точку.
  for (unsigned int i  =  0; i < str.length(); i ++ ) 
  {
    if (isDigit(str[i]) || (str[i]  ==  (char)46 && flag))   
    {                                                      // Если начинается группа цифр (при этом, на точку без цифр не обращаем внимания),
      if (result  ==  "" && i  >  0 && (String)str[i - 1]  ==  "-") 
      {                                                    // Нельзя забывать, что баланс может быть отрицательным
        result += "-";                                     // Добавляем знак в начале
      }
      result += str[i];                                    // начинаем собирать их вместе
      if (!flag) flag  =  true;                            // Выставляем флаг, который указывает на то, что сборка числа началась.
    }
    else  
    {                                                      // Если цифры закончились и флаг говорит о том, что сборка уже была,
      if (str[i] != (char)32) 
      {                                                    // Если порядок числа отделен пробелом - игнорируем его, иначе...
        if (flag) break;                                   // ...считаем, что все.
      }
    }
  }
  return result.toFloat();                                 // Возвращаем полученное число.
}

/////////////////////////////////////////////// Функция уведомления о балансе сим ////////////////////////////////////////////////////////////////
                                                                                   
void GetBalanceSim()
{
// if((millis()-timer_balance) > 20000)                       // Периодичность проверки баланса
// if((millis()-timer_balance) > time_balance || !flag_balance)  // Периодичность проверки баланса (если сработал таймер или первая проверка)
if((millis() - timer_balance) > time_balance)
  {
    String message  =  F("Баланс сим карты  =  ");
    float balance = GetBalans(operator_code);              // Извлекаем баланс
    delay(3000);
    if(balance <= balance_send)                            // Порог баланса для оповещения
    {
      message += (String)balance;                          // Добавляем баланс в сообщение
      sendSMSinPDU(message);                               // Отправляем уведомление о балансе администратору
    }
    timer_balance = millis();                              // Сбрасываем счетчик
    flag_balance = true;                                   // Поднимаем флаг проведенной проверки баланса
  }
}

void InitialZones()////////////////////////////// Функция инициализации зон ////////////////////////////////////////////////////
  {
///////////////////////// 0  Нет тревоги    
///////////////////////// 3  Выход реле тревоги                                
///////////////////////// 1  Выход клапан газа                                  
///////////////////////// 2  Выход клапан воды                           

    for(int i = 0; i < 6; i ++ )
    {
      zone[i].adress_ = i;                                    // Инициализируем адрес зоны
      switch (i)                                              // Инициализируем пины тревоги и уведомления о сработках
      {
      case 0:                                                 // Для зоны 1 (охранная)
       strcpy(zone[i].message_alarm_,"Сработка в зоне 1!");   // Формируем уведомление о сработке
       zone[i].pin_alarm_1_ = 3;                              // При сработке задействуем реле тревоги
       break;
      case 1:                                                 // Для зоны 2 (охранная)
       strcpy(zone[i].message_alarm_,"Сработка в зоне 2!");   // Формируем уведомление о сработке
       zone[i].pin_alarm_1_ = 3;                              // При сработке задействуем реле тревоги
       break;
      case 2:                                                 // Для зоны 3 (охранная)
       strcpy(zone[i].message_alarm_,"Сработка в зоне 3!");   // Формируем уведомление о сработке  
       zone[i].pin_alarm_1_ = 3;                              // При сработке задействуем реле тревоги
       break;
      case 3:                                                 // Для зоны 4 (аварийная "Пожар")
       strcpy(zone[i].message_alarm_,"Пожар!!!");             // Формируем уведомление о сработке 
       zone[i].pin_alarm_1_ = 3;                              // При сработке задействуем реле тревоги
       zone[i].pin_alarm_2_ = 1;                              // При сработке задействуем реле выключения клапана газа
       break;
      case 4:                                                 // Для зоны 5 (аварийная "Газ")
       strcpy(zone[i].message_alarm_,"Утечка газа!!!");       // Формируем уведомление о сработке 
       zone[i].pin_alarm_1_ = 3;                              // При сработке задействуем реле тревоги
       zone[i].pin_alarm_2_ = 1;                              // При сработке задействуем реле выключения клапана газа
       break;
      case 5:                                                 // Для зоны 6 (аварийная "Вода")
       strcpy(zone[i].message_alarm_,"Протечка воды!");       // Формируем уведомление о сработке 
       zone[i].pin_alarm_1_ = 3;                              // При сработке задействуем реле тревоги
       zone[i].pin_alarm_2_ = 2;                              // При сработке задействуем реле выключения клапана воды 
       break;
      }
    }
    InitialMacros();
  }

///////////////////////////////////////// Функция инициализации макросов /////////////////////////////////////////////

  void InitialMacros()      
  {
    switch (macros_id)
    {
     case 0:
      for(int i = 0; i < 6; i ++ )
      {
        zone[i].status_ = macros_1[i];   // Активны все зоны
      }
      break;
     case 1:
      for(int i = 0; i < 6; i ++ )
      {
        zone[i].status_ = macros_2[i];   // Активны только аварийные зоны        
      }
      break;
     case 2:
      for(int i = 0; i < 6; i ++ )
      {
        zone[i].status_ = macros_3[i];   // Активны только охранные зоны        
      }
      break;
     case 3:
      for(int i = 0; i < 6; i ++ )
      {
        zone[i].status_ = macros_4[i];   // Активны аварийные зоны и две охранные      
      }
      break;  
    default:
      for(int i = 0; i < 6; i ++ )
      {
        zone[i].status_ = macros_1[i];   // Активны все зоны       
      }
      break;  
    }  
      
  }

////////////////////////////////////////////// Функция уведомления о напряжении в сети 220 в ////////////////////////////////////////////////////

void GetVoltage()
{
 if(digitalRead(VOLT_PIN) == HIGH)                                                   // Если датчик напряжения в сработке (напряжения нет, работа от акб)
 {  
    if(!flag_volt)                                                                   // Если это первая сработка
    {
      flag_volt = true;                                                              // Поднимаем флаг сработок
      sendSMSinPDU(F("Работа от АКБ!"));                                             // Отправляем уведомление о работе от АКБ администратору  
    }
 }
 else                                                                                // Если датчик в покое (напряжение есть, работа от сети 220в)
 {
    if(flag_volt)                                                                    // Если перед этим была сработка
    {
      flag_volt = false;                                                             // Опускаем флаг сработок
      sendSMSinPDU(F("Работа от сети 220 вольт!"));                                  // Отправляем уведомление о восстановлении 
                                                                                     // работы от сети 220 в администратору 
    }
    return;                                                                          // Выходим в главную функцию
 }
}
/////////////////////////////////////////// Функция проверки доступности сим-модуля //////////////////////////////

void TestModem()
{
  do {              // При включении питания МК загрузится раньше сим модуля поэтому ждем загрузки и ответа на команду
    _response  =  SendATCommand("AT", true);                  // Отправили AT для настройки скорости обмена данными
    _response.trim();                                         // Убираем пробельные символы в начале и конце
    if(AT_counter == 3)
    {
      tone(BUZZER_PIN, 1915);                                 // Сигнализируем звуком об отсутсвии связи с сим-модулем
      delay(150);
      noTone(6);
      delay(20);
      tone(BUZZER_PIN, 1915);                  
      delay(150);
      noTone(6);
      delay(20);
      tone(BUZZER_PIN, 1915);                  
      delay(150);
      noTone(6);
      delay(1000);
      AT_counter = 0;
      if(flag_loop) break;
    }
    if(_response != "OK")AT_counter ++ ;   
  } while (_response != "OK");                                // Не пускать дальше, пока модуль не вернет ОК
}

//////////////////////////////////// Функция стартовой инициализации сим-модуля ///////////////////////////////////////

void InitialModem()
{  
  SendATCommand("AT+CMGDA=\"DEL ALL\"", true);                    // Удаляем все SMS, чтобы не забивать память
  delay(1000);
                                                                  // Команды настройки модема при каждом запуске
//SendATCommand("AT + DDET = 1", true);                           // Включаем DTMF
  SendATCommand("AT+CLIP=1", true);                               // Включаем АОН
  delay(1000);
  SendATCommand("AT+CMGF=1;&W", true);                            // Включаем текстовый режима SMS (Text mode) и сразу сохраняем значение (AT&W)!
  delay(1000);
}

///////////////////////////////////////// Функция получения новых смс сообщений ///////////////////////////////////////

void GetNewSMS()
  { 
   if (millis() - last_update > update_period || flag_timer_sms) { // Пора проверить наличие новых сообщений
    do {
      _response  =  SendATCommand("AT+CMGL=\"REC UNREAD\"", true);// Отправляем запрос чтения непрочитанных сообщений
      delay(1000);
      if (_response.indexOf("+CMGL: ")  >  -1) 
      {                                                           // Если есть хоть одно, получаем его индекс
        int msgIndex  =  _response.substring(_response.indexOf("+CMGL: ") + 7, 
        _response.indexOf("\"REC UNREAD\"", _response.indexOf("+CMGL: "))).toInt();
        int i  =  0;                                              // Объявляем счетчик попыток
        counter_errors = 0;
        do 
        {
          i ++ ;                                                  // Увеличиваем счетчик
          Serial.println (i);
          _response  =  SendATCommand("AT+CMGR=" + (String)msgIndex + ",1", true);  // Пробуем получить текст SMS по индексу
          _response.trim();                                       // Убираем пробелы в начале/конце
          if (_response.endsWith("OK")) 
          {                                                       // Если ответ заканчивается на "ОК"
            if (!hasmsg) hasmsg  =  true;                         // Ставим флаг наличия сообщений для удаления
            SendATCommand("AT+CMGR=" + (String)msgIndex, true);   // Делаем сообщение прочитанным
            SendATCommand("\n", true);                            // Перестраховка - вывод новой строки
            ParseSMS(_response);                                  // Отправляем текст сообщения на обработку
            flag_timer_sms = false;
            break;                                                // Выход из do{}
          }
          else 
          {                                                       // Если сообщение не заканчивается на OK
            SendATCommand("\n", true);                            // Отправляем новую строку и повторяем попытку
          }
        } while (i < 10);
        break;
      }
      else 
      {
        counter_errors ++ ;
        last_update  =  millis();                                 // Обнуляем таймер 
        if (hasmsg || counter_errors > 10) 
        {
          SendATCommand("AT+CMGDA=\"DEL ALL\"", true);            // Удаляем все сообщения
          hasmsg  =  false;         
        }
        flag_timer_sms = false; 
        break;
      }
    } while (1);   
   }
  }

/////////////////////////////////////// Функция работы с клавиатурой /////////////////////////////////////////////////

void GetKeyboard()
{
  if(digitalRead(KEYBOARD_PIN) == LOW)                              // Если пропал сигнал с клавиатуры
  {
    delay(1500);
    if(digitalRead(KEYBOARD_PIN) == HIGH)                           // Если сигнал появился через 1,5 сек, сигнал валидный (смена режима)
    { 
      if(!flag_keyboard)                                            // Флаг обрыва не поднят
      {
        if(!mode)                                                   // Если ставим на охрану
        {
          delay(timer_delay_on);                                    // Ждем указанное время до постановки
          mode = !mode;                                             // Переключаем режим на противоположный
          sendSMSinPDU(F("Режим охраны Вкл"));                      // Отправляем смс с результатом администратору
        }
        else                                                        // Если снимаем с охраны
        {
          mode = !mode;                                             // Переключаем режим на противоположный
          sendSMSinPDU(F("Режим охраны Выкл"));                     // Отправляем смс с результатом администратору
        }
      }
      else                                                          // Если флаг обрыва поднят
      {
        flag_keyboard = false;                                      // Поднимаем флаг обрыва
        sendSMSinPDU(F("Клавиатура Вкл"));                          // Отправляем смс о подключении клавиатуры        
      }
    }
    else                                                            // Если сигнал не появился через 1,5 сек
    {
      if(!flag_keyboard)                                            // Если флаг обрыва не поднят                   
      {
        flag_keyboard = true;                                       // Поднимаем флаг обрыва
        sendSMSinPDU(F("Клавиатура Выкл"));                         // Отправляем смс об отсутствии клавиатуры
      }
    }
  }
}

///////////////////////////////////////// Функция активации реле ////////////////////////////////////////////////////////

void ActivateRelay(byte Alarm1, byte Alarm2)
{
      switch (Alarm1)                          // Если к датчику подключена тревога 1
      {
        case 0:                                // Нет тревоги
          break;
        case 1:
                       
        D3_High;                               // Клапан отсечки газа
          break;
        case 2:
          D4_High;                             // Клапан отсечки воды
          break;
        case 3:
          D2_High;                             // Тревога
          if(!flag_alarm_timer)
            {                    
              alarm_timer = millis();
              flag_alarm_timer = true;
            }
          break;
      }
      switch (Alarm2)                          // Если к датчику подключена тревога 2  
      {
        case 0:                                // Нет тревоги
          break;
        case 1:
          D3_High;                             // Клапан отсечки газа
          break;
        case 2:
          D4_High;                             // Клапан отсечки воды 
          break;
        case 3:
          D2_High;                             // Тревога
          if(!flag_alarm_timer)
            {                    
              alarm_timer = millis();
              flag_alarm_timer = true;
            }
          break;
      }
}

///////////////////////////////// Функция инициализации энергонезависимой памяти ///////////////////////////////////////

void InitialEeprom()
{
  for(int i = 0; i < 13; i ++ )
   {
      phones[0] += (char)EEPROM.read(i);                        // Получаем из EEPROM номер админа
   }
  for(int i = 20; i < 33; i ++ )
   {
      phones[1] += (char)EEPROM.read(i);                        // Получаем из EEPROM номер админа 2
   }
   for(int i = 40; i < 53; i ++ )
   {
      phones[2] += (char)EEPROM.read(i);                        // Получаем из EEPROM номер админа 3
   }
   for(int i = 60; i < 73; i ++ )
   {
      phones[3] += (char)EEPROM.read(i);                        // Получаем из EEPROM номер админа 4
   }
   for(int i = 80; i < 93; i ++ )
   {
      phones[4] += (char)EEPROM.read(i);                        // Получаем из EEPROM номер админа 5
   }
   for(int i = 100; i < 102; i ++ )
   {
      temp += (char)EEPROM.read(i);                             // Получаем из EEPROM вермя задержки вкл
   }
   timer_delay_on = ((String)temp).toInt()*1000;                // Инициализируем цифровую переменную задержки включения
   temp = "";                                                   // Обнуляем строковую переменную задержки включения
   for(int i = 102; i < 104; i ++ )
   {
      temp += (char)EEPROM.read(i);                             // Получаем из EEPROM вермя задержки в��кл
   }
   timer_delay_off = ((String)temp).toInt()*1000;               // Инициализируем цифровую переменную задержки выключения
   temp = "";                                                   // Обнуляем строковую переменную задержки выключения
  for(int i = 104; i < 106; i ++ )
  {
      temp += (char)EEPROM.read(i);                             // Получаем из EEPROM баланс
  }
   macros_id = EEPROM.read(106);                                // Получаем из EEPROM текущий макрос

  for(int i = 107, j = 0; i < 112; i ++ , j ++ )
  {
    operator_code += (char)EEPROM.read(i);                      // Получаем из EEPROM код оператора  
  }
}

////////////////////////////////////// Функция индикации входящего звонка /////////////////////////////////////////

void SetDingDong()     
{  
  if((millis() - timer_ring < 3000))                       
  {
    D13_Inv;                                        // Переключаем светодиод
    tone(BUZZER_PIN, 1915);                         // Включаем пищалку
    delay(50);                                      // Пауза
    D13_Inv;                                        // Переключаем светодиод
    noTone(6);                                      // Выключаем пищалку
    delay(50);                                      // Пауза
  }
}

//////////////////////////////////////////// Функция обработки входящих звонков ///////////////////////////////////

void GetIncomingCall()    
{
  bool flag_admin = false;
  flag_call = true;                                     // Поднимаем флаг звонка
  int phone_index  =  _response.indexOf("+CLIP: \"");   // Есть ли информация об определении номера, если да, то phone_index > -1
  String inner_phone  =  "";                            // Переменная для хранения определенного номера
  if (phone_index  >=  0)                               // Если информация была найдена
  {
    phone_index += 8;                                                                       // Парсим строку и 
    inner_phone  =  _response.substring(phone_index, _response.indexOf("\"", phone_index)); // получаем номер     
  } 
  for(int i = 0; i < counter_admins; i ++ )
  {
    if(inner_phone == phones[i])
    {
      flag_admin = true;
      break;
    }
  }
  if(mode)                                         // Если включен режим охраны
  {
    if(flag_admin)                                 // Если звонит админ
    {
      mode = !mode;                                // Сменить режим охраны
      SendATCommand("ATA", true);                  // Ответить на звонок
      SIM800.print(F("AT+CREC=4,\"C:\\User\\nerejim.amr\",0,80"));   // Проигрываем звуковой файл
      delay(DELAY_PLAY_TRACK);                     // Делаем паузу для воспроизведения трека
      SendATCommand("ATH", true);                  // Сбрасываем звонок
      sendSMSinPDU(F("Снято с охраны!"));               // Отправляем смс с результатом администратору
      return;                                      // и выходим
    }
    else                                           // Звонит не админ
    {
      SendATCommand("ATH", true);                  // Сбрасываем звонок
      return;                                      // и выходим
    }
  }
  else
  {
    if(flag_admin)                                 // Если звонит админ
    {
      mode = !mode;                                // Сменить режим охраны
      SIM800.print(F("AT+CREC=4,\"C:\\User\\rejim.amr\",0,80"));   // Проигрываем звуковой файл
      delay(DELAY_PLAY_TRACK);                     // Делаем паузу для воспроизведения трека
      SendATCommand("ATH", true);                  // Сбрасываем звонок
      sendSMSinPDU(F("Поставлено на охрану!"));    // Отправляем смс с результатом администратору
      return;      
    } 
    timer_ring = millis();                         // Включаем таймер для визуализации звонка  
                                                                         // Если длина номера больше 6 цифр, 
    if (inner_phone.length()  >=  7 && digitalRead(BUTTON_PIN) == HIGH)  // и нажата кнопка на плате
    {
      phones[0] = inner_phone;

      for(int i = 0; i < 13; i ++ )
      {
        EEPROM.write(i, (byte)phones[0][i]);       // Записываем номер в память EEPROM
      }  
      SIM800.print(F("AT+CREC=4,\"C:\\User\\admin.amr\",0,80"));   // Проигрываем звуковой файл
      delay(DELAY_PLAY_TRACK);                     // Делаем паузу для воспроизведения трека  
      SendATCommand("ATH", true);                  // Сбрасываем звонок         
      tone(BUZZER_PIN, 1915);                      // Сигнализируем динамиком о принятии номера администратора
      delay(1000);
      noTone(6); 
      sendSMSinPDU(F("Номер администратора принят!"));  
    }
  }
}

//////////////////////////////////////////// Функция отправки СМС сообщений ///////////////////////////////////////////

void SendSMS(String message)
{
  for(int i = 0; i < counter_admins; i ++ )
  {
    if(phones[i][0] == '+')
    {
      SendATCommand("AT+CMGS=\"" + phones[i] + "\"", true);          // Переходим в режим ввода текстового сообщения
      SendATCommand(message + "\r\n" + (String)((char)26), true);    // После текста отправляем перенос строки и Ctrl + Z
      delay(2500);
    }
  } 
}

//////////////////////////////////////////// Функция отправки СМС сообщений в кириллице ///////////////////////////////////////////

void sendSMSinPDU(String message)
{
  //Serial.println("Отправляем сообщение: " + message);

  for(int i = 0; i < counter_admins; i ++ )
  {
    if(phones[i][0] == '+')
    {
  // ============ Подготовка PDU-пакета =============================================================================================
 
      String *ptrphone = &phones[i];                                // Указатель на переменную с телефонным номером
      String *ptrmessage = &message;                                // Указатель на переменную с сообщением

      String PDUPack;                                               // Переменная для хранения PDU-пакета
      String *ptrPDUPack = &PDUPack;                                // Создаем указатель на переменную с PDU-пакетом

      int PDUlen = 0;                                               // Переменная для хранения длины PDU-пакета без SCA
      int *ptrPDUlen = &PDUlen;                                     // Указатель на переменную для хранения длины PDU-пакета без SCA

      getPDUPack(ptrphone, ptrmessage, ptrPDUPack, ptrPDUlen);      // Функция формирующая PDU-пакет, и вычисляющая длину пакета без SCA

      Serial.println("PDU-pack: " + PDUPack);
      Serial.println("PDU length without SCA:" + (String)PDUlen);

  // ============ Отправка PDU-сообщения ============================================================================================
      SendATCommand("AT+CMGF=0", true);                             // Включаем PDU-режим
      SendATCommand("AT+CMGS=" + (String)PDUlen, true);             // Отправляем длину PDU-пакета
      SendATCommand(PDUPack + (String)((char)26), true);            // После PDU-пакета отправляем Ctrl+Z
      delay(2500);
    }
  }
}


void getPDUPack(String *phone, String *message, String *result, int *PDUlen)
{
  // Поле SCA добавим в самом конце, после расчета длины PDU-пакета
  *result += "01";                                // Поле PDU-type - байт 00000001b
  *result += "00";                                // Поле MR (Message Reference)
  *result += getDAfield(phone, true);             // Поле DA
  *result += "00";                                // Поле PID (Protocol Identifier)
  *result += "08";                                // Поле DCS (Data Coding Scheme)
  //*result += "";                                // Поле VP (Validity Period) - не используется

  String msg = StringToUCS2(*message);            // Конвертируем строку в UCS2-формат

  *result += byteToHexString(msg.length() / 2);   // Поле UDL (User Data Length). Делим на 2, так как в UCS2-строке каждый закодированный символ представлен 2 байтами.
  *result += msg;

  *PDUlen = (*result).length() / 2;               // Получаем длину PDU-пакета без поля SCA
  *result = "00" + *result;                       // Добавляем поле SCA
}

String getDAfield(String *phone, bool fullnum) 
{
  String result = "";
  for (int i = 0; i <= (*phone).length(); i++) {  // Оставляем только цифры
    if (isDigit((*phone)[i])) {
      result += (*phone)[i];
    }
  }
  int phonelen = result.length();                 // Количество цифр в телефоне
  if (phonelen % 2 != 0) result += "F";           // Если количество цифр нечетное, добавляем F

  for (int i = 0; i < result.length(); i += 2) {  // Попарно переставляем символы в номере
    char symbol = result[i + 1];
    result = result.substring(0, i + 1) + result.substring(i + 2);
    result = result.substring(0, i) + (String)symbol + result.substring(i);
  }

  result = fullnum ? "91" + result : "81" + result; // Добавляем формат номера получателя, поле PR
  result = byteToHexString(phonelen) + result;    // Добавляем длиу номера, поле PL

  return result;
}

// =================================== Блок декодирования UCS2 в читаемую строку UTF-8 =================================

//////////////////////////////////////////////////// Функция декодирования UCS2 строки /////////////////////////////////////////////////////

String UCS2ToString(String s) 
{                       
  String result = "";
  unsigned char c[5] = "";                            // Массив для хранения результата
  for (int i = 0; i < s.length() - 3; i += 4) {       // Перебираем по 4 символа кодировки
    unsigned long code = (((unsigned int)HexSymbolToChar(s[i])) << 12) +    // Получаем UNICODE-код символа из HEX представления
                         (((unsigned int)HexSymbolToChar(s[i + 1])) << 8) +
                         (((unsigned int)HexSymbolToChar(s[i + 2])) << 4) +
                         ((unsigned int)HexSymbolToChar(s[i + 3]));
    if (code <= 0x7F) {                               // Теперь в соответствии с количеством байт формируем символ
      c[0] = (char)code;
      c[1] = 0;                                       // Не забываем про завершающий ноль
    } else if (code <= 0x7FF) {
      c[0] = (char)(0xC0 | (code >> 6));
      c[1] = (char)(0x80 | (code & 0x3F));
      c[2] = 0;
    } else if (code <= 0xFFFF) {
      c[0] = (char)(0xE0 | (code >> 12));
      c[1] = (char)(0x80 | ((code >> 6) & 0x3F));
      c[2] = (char)(0x80 | (code & 0x3F));
      c[3] = 0;
    } else if (code <= 0x1FFFFF) {
      c[0] = (char)(0xE0 | (code >> 18));
      c[1] = (char)(0xE0 | ((code >> 12) & 0x3F));
      c[2] = (char)(0x80 | ((code >> 6) & 0x3F));
      c[3] = (char)(0x80 | (code & 0x3F));
      c[4] = 0;
    }
    result += String((char*)c);                       // Добавляем полученный символ к результату
  }
  return (result);
}

unsigned char HexSymbolToChar(char c) 
{
  if      ((c >= 0x30) && (c <= 0x39)) return (c - 0x30);
  else if ((c >= 'A') && (c <= 'F'))   return (c - 'A' + 10);
  else                                 return (0);
}

// =================================== Блок кодирования строки в представление UCS2 =================================
String StringToUCS2(String s)
{
  String output = "";                                               // Переменная для хранения результата

  for (int k = 0; k < s.length(); k++) {                            // Начинаем перебирать все байты во входной строке
    byte actualChar = (byte)s[k];                                   // Получаем первый байт
    unsigned int charSize = getCharSize(actualChar);                // Получаем длину символа - кличество байт.

    // Максимальная длина символа в UTF-8 - 6 байт плюс завершающий ноль, итого 7
    char symbolBytes[charSize + 1];                                 // Объявляем массив в соответствии с полученным размером
    for (int i = 0; i < charSize; i++)  symbolBytes[i] = s[k + i];  // Записываем в массив все байты, которыми кодируется символ
    symbolBytes[charSize] = '\0';                                   // Добавляем завершающий 0

    unsigned int charCode = symbolToUInt(symbolBytes);              // Получаем DEC-представление символа из набора байтов
    if (charCode > 0)  {                                            // Если все корректно преобразовываем его в HEX-строку
      // Остается каждый из 2 байт перевести в HEX формат, преобразовать в строку и собрать в кучу
      output += byteToHexString((charCode & 0xFF00) >> 8) +
                byteToHexString(charCode & 0xFF);
    }
    k += charSize - 1;                                              // Передвигаем указатель на начало нового символа
    if (output.length() >= 280) break;                              // Строка превышает 70 (4 знака на символ * 70 = 280) символов, выходим
  }
  return output;                                                    // Возвращаем результат
}

///////////////////////////////////////////////// Функция получения количества байт, которыми кодируется символ /////////////////////////////////////

unsigned int getCharSize(unsigned char b) 
{ 
  // По правилам кодирования UTF-8, по старшим битам первого октета вычисляется общий размер символа
  // 1  0xxxxxxx - старший бит ноль (ASCII код совпадает с UTF-8) - символ из системы ASCII, кодируется одним байтом
  // 2  110xxxxx - два старших бита единицы - символ кодируется двумя байтами
  // 3  1110xxxx - 3 байта и т.д.
  // 4  11110xxx
  // 5  111110xx
  // 6  1111110x

  if (b < 128) return 1;             // Если первый байт из системы ASCII, то он кодируется одним байтом

  // Дальше нужно посчитать сколько единиц в старших битах до первого нуля - таково будет количество байтов на символ.
  // При помощи маски, поочереди исключаем старшие биты, до тех пор пока не дойдет до нуля.
  for (int i = 1; i <= 7; i++) {
    if (((b << i) & 0xFF) >> 7 == 0) {
      return i;
    }
  }
  return 1;
}

///////////////////////////////////////////////// Функция для получения DEC-представления символа ////////////////////////////////////////////////////

unsigned int symbolToUInt(const String& bytes) 
{  
  unsigned int charSize = bytes.length();         // Количество байт, которыми закодирован символ
  unsigned int result = 0;
  if (charSize == 1) {
    return bytes[0]; // Если символ кодируется одним байтом, сразу отправляем его
  }
  else  {
    unsigned char actualByte = bytes[0];
    // У первого байта оставляем только значимую часть 1110XXXX - убираем в начале 1110, оставляем XXXX
    // Количество единиц в начале совпадает с количеством байт, которыми кодируется символ - убираем их
    // Например (для размера 2 байта), берем маску 0xFF (11111111) - сдвигаем её (>>) на количество ненужных бит (3 - 110) - 00011111
    result = actualByte & (0xFF >> (charSize + 1)); // Было 11010001, далее 11010001&(11111111>>(2+1))=10001
    // Каждый следующий байт начинается с 10XXXXXX - нам нужны только по 6 бит с каждого последующего байта
    // А поскольку остался только 1 байт, резервируем под него место:
    result = result << (6 * (charSize - 1)); // Было 10001, далее 10001<<(6*(2-1))=10001000000

    // Теперь у каждого следующего бита, убираем ненужные биты 10XXXXXX, а оставшиеся добавляем к result в соответствии с расположением
    for (int i = 1; i < charSize; i++) {
      actualByte = bytes[i];
      if ((actualByte >> 6) != 2) return 0; // Если байт не начинается с 10, значит ошибка - выходим
      // В продолжение примера, берется существенная часть следующего байта
      // Например, у 10011111 убираем маской 10 (биты в начале), остается - 11111
      // Теперь сдвигаем их на 2-1-1=0 сдвигать не нужно, просто добавляем на свое место
      result |= ((actualByte & 0x3F) << (6 * (charSize - 1 - i)));
      // Было result=10001000000, actualByte=10011111. Маской actualByte & 0x3F (10011111&111111=11111), сдвигать не нужно
      // Теперь "пристыковываем" к result: result|11111 (10001000000|11111=10001011111)
    }
    return result;
  }
}

////////////////////////////////////////// Функция преобразования числового значения байта в шестнадцатиричное (HEX) /////////////////////////

String byteToHexString(byte i) 
{ 
  String hex = String(i, HEX);
  if (hex.length() == 1) hex = "0" + hex;
  hex.toUpperCase();
  return hex;
}
