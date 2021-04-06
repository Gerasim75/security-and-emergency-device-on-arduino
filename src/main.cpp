
#include  <SoftwareSerial.h>                                
#include  <EEPROM.h> 
#include  <CyberLib.h> 

//#define _SS_MAX_RX_BUFF 128                             // регулируем размер RX буфера SoftwareSerial
                        
#define Zona_1 8          // Вход                 
#define Zona_2 9          // Вход                     
#define Zona_3 10         // Вход                          
#define Zona_4 11         // Вход пожар
#define Zona_5 12         // Вход газ
#define Zona_6 19         // Вход вода
#define KlavaPin 18       // Вход клавиатура           
//#define LedPin 13         // Выход светодиод
#define VoltPin 17        // Вход сеть
#define BuzzerPin 6       // На плате
#define KnopkaPin 16      // На плате
                                       
class Sensor {                         // Объявляем класс Sensor
  public:
  byte Adress;
  byte Pin;                            // Номер пина ввода
  byte PinAlarm1 = 0;                  // Номер пина вывода тревоги 1
  byte PinAlarm2 = 0;                  // Номер пина вывода тревоги 2
  bool Status = false;                 // Статус ввода (активен - не активен)
  bool Alarm = false;                  // Тревога (вкл - выкл)
  bool sendAlarm = false;              // Уведомление отправлено - не отправлено
  char messageAlarm[20];               // Сообщение о сработке датчика
  
  bool readPin()
  {
    if(digitalRead(Pin) == LOW && Status && !Alarm) // Если сработал пин, ввод активен и нет тревоги
    {
      Alarm = true;                             // Активировать тревогу
      return true; 
    }
    else
      return false;
  } 
 };   
                   
SoftwareSerial SIM800(14, 15);                                  // RX, TX (пины подключения сим-модуля) 

String kod_operatora = "";                                      // Переменная кода оператора сим
String _response     =  "";                                     // Переменная для хранения ответа модуля
String phones[5] = {"","","","",""};                            // Телефон админа
String temp = "";
int AT_counter = 1;
int balance_send;
unsigned long timerRing = millis();                             // Инициализируем таймер для входящего звонка             ???????????????????????????
unsigned long timerBalance = millis();                          // Инициализируем таймер для отправки уведомлений о балансе на сим карте
unsigned long timer_alarm_time = millis();                      // Инициализируем таймер сирены
unsigned long timer_zaderjka = millis();                        // Таймер задержки срабатывания
unsigned long last_Update  =  millis();                         // Время последнего обновления      
unsigned long updatePeriod =  20000;                            // Проверять каждые 20 секунд
unsigned long time_alarm  =  90000;                             // Время работы сирены
unsigned int time_zaderjka_off;                                 // Время задержки сработки тревоги в миллисекундах.
unsigned int time_zaderjka_on;                                  // Время задержки постановки на охрану в миллисекундах.
unsigned long time_balance = 86400000;                          // Время периодичности проверки баланса в миллисекундах (сутки 86400000 мс)
long counter_Errors = 0;                                        // Счетчик итераций при ошибке обработки входящего смс 
unsigned long alarm_timer = 0;
bool flag_zona1 = false;
bool flag_zaderjka_zona1 = false;
bool flagKlava = false;
bool flagBalance = false;
bool flagloop = false;
bool flagCall = false;                                          // Флаг входящего звонка  
bool flagVolt = false;                                          // Флаг отсутствия напряжения в сети
bool flagTel = false;                                           // Флаг исходящего звонка
bool rejim = false;                                             // Режим охраны вкл/выкл
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
byte srabotki[6] = {9,9,9,9,9,9};                               // Массив адресов сработавших датчиков
byte counter_srabotki = 0;                                      // Счетчик сработок датчиков
byte counter_admins = 5;                                        // Количество администраторов

Sensor zona[6];

/////////////////////////////// Прототипы функций ////////////////////////////////////////
void Call(String & num);
String sendATCommand(String cmd, bool waiting);
String waitResponse();
void sensors();
void alarm_messages();
void parseSMS(String & msg);
void setLedState(String & result, String & msgphone);
void sendSMS(String message);
float Balans(String & kod);
float getFloatFromString(String & str);
void balanceSim();
void initialZones();
void initialMacros();
void voltage();
void testModem();
void initialModem();
void getNewSMS();
void klava();
void ActivateRele(byte Alarm1, byte Alarm2);
void initialEeprom();
void dingDong();
void incomingCall();

///////////////////////////////////////// Функция настроек контроллера /////////////////////////////////////////

void setup() 
{
  D2_Out;
  D3_Out;
  D4_Out;
  D5_Out;
  D6_Out;
  D7_Out;
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

  zona[0].Pin = Zona_1;
  zona[1].Pin = Zona_2;
  zona[2].Pin = Zona_3;
  zona[3].Pin = Zona_4;
  zona[4].Pin = Zona_5;
  zona[5].Pin = Zona_6;

  Serial.begin(9600);                                         // Скорость обмена данными с компьютером
  SIM800.begin(9600);                                         // Скорость обмена данными с сим модулем
  
  initialZones();
  initialEeprom();
  
   balance_send = ((String)temp).toInt();               ////////////////////////////////////////////////////////////////
   temp = F("Systema v5.3 Status OFF. Phones - ");      //
   for(int i = 0; i < counter_admins; i ++ )            //
   {                                                    //
    if(phones[i][0] == '+')                             //
    {                                                   //
      temp += phones[i];                                //
      if(phones[i + 1][0] == '+')                       //
      {                                                 //
        temp += F(", ");                                //
      }                                                 //      
    }                                                   //   Формируем сообщение о перезагрузке контроллера
   }                                                    //
   temp += F(". ");                                     //
   temp += F("Time ON ");                               //
   temp += time_zaderjka_on/1000;                       //
   temp += F(". Time OFF ");                            //  
   temp += time_zaderjka_off/1000;                      //
   temp += F(". Min balance ");                         //
   temp += balance_send;                                //
   temp += F(".Kod ");                                  //
   temp += kod_operatora;                               //
   temp += F(".");                                      ////////////////////////////////////////////////////////////////

  testModem();                                                // Проверяем работоспособность сим-модуля
  initialModem();                                             // Инициализируем сим-модуль                                       
  delay(1000);
  sendSMS(temp);                                              // Отправляем смс уведомление о перезагрузке контроллера администратору
  tone(BuzzerPin, 1915);                                      // Сигнализируем динамиком о прохождении этапа загрузки контроллера
  delay(1000);
  noTone(6);
  last_Update  =  millis();                                   // Обнуляем таймер    
}

/////////////////////////////////////////////// Главная функция ////////////////////////////////////////////

void loop() 
{ 
  flagloop = true;
  klava();                                            // Ожидаем данные с клавиатуры
  balanceSim();                                       // Проверяем баланс на сим карте
  dingDong();                                         // Свето-звуковая индикация входящего звонка 
  getNewSMS();                                        // Получаем входящие смс
  
  if (SIM800.available())   {                         // Если модем, что-то отправил...
    _response  =  waitResponse();                     // Получаем ответ от модема для анализа
    _response.trim();                                 // Убираем лишние пробелы в начале и конце

    if (_response.indexOf("+CMTI:") > -1)            // Если пришло сообщение об отправке SMS
    {    
     flag_timer_sms = true;      
    }
    if (_response.startsWith("RING"))                // Есть входящий вызов
    { 
      incomingCall();                                // Обрабатываем его
    }  
    else                                             // Если нет
    {
      flagCall = false;                              // Опускаем флаг входящего вызова
    }
  }
  if (Serial.available())  {                         // Ожидаем команды по Serial...
    SIM800.write(Serial.read());                     // ...и отправляем полученную команду модему
  }
  if (SIM800.available())  {                         // Ожидаем команды от модема ...
    Serial.write(SIM800.read());                     // ...и отправляем полученную команду в Serial    
  }

  if(!rejim)                                         // Если снято с охраны (режим выкл)
  { 
    flag_zaderjka_zona1 = false;
    flag_zona1 = false;
    flagTel = false;  
    flag_alarm_timer = false; 
    flag_alarm = false;                              // Опускаем флаг сообщений о сработке
    counter_srabotki = 0;
    for(int i = 0; i < 6; i ++ )
    {
      zona[i].Alarm = false;
      zona[i].sendAlarm = false;
      srabotki[i] = 9;
    }            
      D2_Low;                                         // Выключаем аварийный сигнал                 
      D13_Low;                                        // Выключаем светодиод
      noTone(6);                                      // Выключаем сирену
 //    Serial.println(F("!rejim"));                   // Если нужно отправляем сообщение в монитор порта 
  }
  else                                                // Если стоит на охране (режим вкл)
  {
    if((alarm_timer + time_alarm) < millis())
      {
        D2_Low;                                       //  Если время работы сирены вышло отключаем ее
      }
//    Serial.println(F("rejim"));                     // Если нужно отправляем сообщение в монитор порта               
    D13_High;                                         // Включаем светодиод
    sensors();                                        // Смотрим на датчики
    alarm_messages();                                 // Отправляем необходимые уведомления
  }
    voltage();                                        // Контролируем питание системы
}

////////////////////////////////////////////// Конец главной функции //////////////////////////////////////////////

/////////////////////////////////////////// Функция оповещения звонком ////////////////////////////////////////////

void Call(String & num)
{
  String comand = F("ATD");
   sendATCommand("AT+COLP=0", true);                // Режим ожидания ответа
//sendATCommand("AT + VTD = 4", true);              // Длительность тоновых сигналов для AT + VTD. Значение параметра 1..255
   comand = comand + num;                           // Собираем команду
   comand = comand + ";";                           // Добавляем символ в конец команды
   sendATCommand(comand, true);                     // Отправляем команду вызова
//delay(3000);                                      // Пауза до сброса вызова
//   sendATCommand("ATH0", true);                   // Сбрасываем вызов через 30 сек
  
  }

//////////////////////////////////////// Функция отправки команды сим модулю /////////////////////////////////////

String sendATCommand(String cmd, bool waiting) {
  String _resp  =  "";                                            // Переменная для хранения результата
  SIM800.println(cmd);                                            // Отправляем команду модулю
  if (waiting) {                                                  // Если необходимо дождаться ответа...
    _resp  =  waitResponse();                                     // ... ждем, когда будет передан ответ
    // Если Echo Mode выключен (ATE0), то эти 3 строки можно закомментировать
    if (_resp.startsWith(cmd)) {                                  // Убираем из ответа дублирующуюся команду
     _resp  =  _resp.substring(_resp.indexOf("\r", cmd.length()) + 2);
    }
  }
  return _resp;                                                   // Возвращаем результат. Пусто, если проблема
}

//////////////////////////////////////// Функция ожидания ответа от сим модуля ///////////////////////////////////////////////////////

String waitResponse()                                             // Функция ожидания ответа и возврата полученного результата
{                                          
  String _resp  =  "";                                            // Переменная для хранения результата
  unsigned long _timeout  =  millis() + 10000;                    // Переменная для отслеживания таймаута (10 секунд)
  while (!SIM800.available() && millis() < _timeout)  {};         // Ждем ответа 10 секунд, если пришел ответ или наступил таймаут, то...
  if (SIM800.available()) {                                       // Если есть, что считывать...
    _resp  =  SIM800.readString();                                // ... считываем и запоминаем
  }
  return _resp;                                                   // ... возвращаем результат. Пусто, если проблема
}

///////////////////////////////////////////// Функция контроля датчиков //////////////////////////////////////////////////////////////

void sensors()
{
  for(int i = 0; i < 6; i ++ )                           // Проходим по сенсорам
  {
    if(zona[i].readPin())                                // Если есть сработка
    {
      flag_alarm = true;                                 // Поднимаем флаг сработки
      srabotki[counter_srabotki] = zona[i].Adress;       // Записываем адрес зоны в массив сработок
      counter_srabotki ++ ;                              // Увеличиваем счетчик адресов
    }
   }
    if(srabotki[0] == 0 && !flag_zona1)                  // Если первой сработала зона 1 и это первая сработка
      {
        timer_zaderjka = millis();                       // Стартуем таймер задержки
        flag_zona1 = true;                               // Поднимаем флаг сработки в зоне 1
      }
}

/////////////////////////////////////////////// Функция уведомлений //////////////////////////////////////////////////////////////////////
                                                                 
void alarm_messages()
{
  if(flag_alarm)
  {
     for(int i = 0; i < counter_srabotki; i ++ )
     {
     if(zona[srabotki[i]].Alarm == true && zona[srabotki[i]].sendAlarm == false)    // Если поднят флаг сработки датчика и уведомление не отправлялось
      {
          if(i == 0 && srabotki[i] == 0)                                                    // Если сработка в зоне 1
            {
               if(millis() - timer_zaderjka > time_zaderjka_off)                            // Если сработал таймер
                 {     
                    ActivateRele(zona[srabotki[i]].PinAlarm1, zona[srabotki[i]].PinAlarm2); // Активируем реле тревоги
                    sendSMS(zona[srabotki[i]].messageAlarm);                                // Отправляем смс уведомление
                    if(!flagTel)                                                            // Если еще не было звонка администратору
                    {
                      flagTel = true;                                                       // Поднимаем флаг звонка  
                      Call(phones[0]);                                                      // Звоним администратору
                    }
                    zona[srabotki[i]].sendAlarm = true;                                     // Поднимаем флаг отправленного уведомления
                    flag_zaderjka_zona1 = true;                                             // Поднимаем флаг задержки сработки
                    break;                 
                 }
            }
          else
            {
               if(srabotki[i] == 1 && flag_zona1 && !flag_zaderjka_zona1) break;
              ActivateRele(zona[srabotki[i]].PinAlarm1, zona[srabotki[i]].PinAlarm2);        // Активируем реле тревоги
              sendSMS(zona[srabotki[i]].messageAlarm);                                       // Отправляем смс уведомление
              if(!flagTel)                                                                   // Если еще не было звонка администратору
              {
                flagTel = true;                                                              // Поднимаем флаг звонка  
                Call(phones[0]);                                                             // Звоним администратору
              }
              zona[srabotki[i]].sendAlarm = true;                                            // Поднимаем флаг отправленного уведомления
              break;                                                                         // Выходим из цикла
            }
      }
    }
  }
}
     
///////////////////////////////////////////////// Фунция парсинга СМС /////////////////////////////////////////////////

void parseSMS(String & msg)                                     // Парсим SMS
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
    setLedState(msgbody, msgphone);                             // ...выполняем команду
  }
}
 
///////////////////////////////////////// Функция чтения кода команды из смс ///////////////////////////////////////////

void setLedState (String & result, String & msgphone)   
{
  bool correct  =  false;                          // Для оптимизации кода, переменная корректности команды
  if (result.length()  ==  1)                      // Если в сообщении одна цифра
  {
    int ledState  =  ((String)result[0]).toInt();  // Получаем первую цифру команды - состояние (0 - выкл, 1 - вкл)
    if (ledState  >=  0 && ledState  <=  5) 
    {                                            // Если все нормально, исполняем команду
      if (ledState == 1) 
      {
        EEPROM.write(106, 0);                    // Записываем его в память  
        macros_id = 0;
        initialMacros();                         // Инициализируем макрос
        sendSMS(F("Rejim 1 ON"));                // Отправляем смс с результатом администратору 
        rejim = true;       
      }
      if(ledState == 2) 
      {
        EEPROM.write(106, 1);                    // Записываем его в память
        macros_id = 1;
        initialMacros();                         // Инициализируем макрос 
        sendSMS(F("Rejim 2 ON"));                // Отправляем смс с результатом администратору 
        rejim = true;              
      }
      if(ledState == 3)
      {
        EEPROM.write(106, 2);                    // Записываем его в память
        macros_id = 2;
        initialMacros();                         // Инициализируем макрос   
        sendSMS(F("Rejim 3 ON"));                // Отправляем смс с результатом администратору 
        rejim = true;            
      }
      if(ledState == 4)
      { 
        EEPROM.write(106, 3);                    // Записываем его в память
        macros_id = 3;
        initialMacros();                         // Инициализируем макрос
        sendSMS(F("Rejim 4 ON"));                // Отправляем смс с результатом администратору    
        rejim = true;               
      }
      if(ledState == 5) 
      {
        EEPROM.write(106, 4);                    // Записываем его в память
        macros_id = 4;
        initialMacros();                         // Инициализируем макрос  
        sendSMS(F("Rejim 5 ON"));                // Отправляем смс с результатом администратору
        rejim = true;
      }
      if(ledState == 0) 
      {
        rejim = false;                           // Снимаем с охраны
        sendSMS(F("Rejim OFF"));                 // Отправляем смс с результатом администратору
      }
       
      tone(BuzzerPin, 1915);                     // Сигнализируем динамиком о смене режима охраны
      delay(1000);
      noTone(6); 
      correct  =  true;                          // Флаг корректности команды
    }
    if (!correct) 
    {
      sendSMS(F("Incorrect command!"));          // Отправляем смс с результатом администратору
    }
  }
  else
    {
      if(result.length()  ==  13 && msgphone == phones[0])  // Если в сообщении номер телефона и отправитель админ 1
      {
        bool flag_number = false;
        for(int k = 1; k < counter_admins;k ++ )        // Проходим по списку номеров
        {
          if(phones[k] == result)                       // Если номер есть в позиции k
           {
            flag_number = true;                         // Поднимаем флаг наличия номера
            switch(k)                                   // Ищем его в памяти EEPROM и удаляем
            {
              case 1:
                    for(int i = 20; i < 33; i ++ )
                      {
                        EEPROM.write(i, (byte)' ');     // Перезаписываем номер админа 2 пробелами                       
                      }
                      phones[k] = " ";                  // Очищаем переменную
                      tone(BuzzerPin, 1915);            // Сигнализируем динамиком о удалении номера дублера
                      delay(1000);
                      noTone(6);
                      sendSMS(F("Nomer 2 udalen!"));
                      break;
              case 2:
                    for(int i = 40; i < 53; i ++ )
                      {
                        EEPROM.write(i, (byte)' ');     // Перезаписываем номер админа 3 пробелами
                      }
                      phones[k] = " ";                  // Очищаем переменную
                      tone(BuzzerPin, 1915);            // Сигнализируем динамиком о удалении номера дублера
                      delay(1000);
                      noTone(6);
                      sendSMS(F("Nomer 3 udalen!"));
                      break;
              case 3:
                    for(int i = 60; i < 73; i ++ )
                     {
                        EEPROM.write(i, (byte)' ');     // Перезаписываем номер админа 4 пробелами
                     }
                     phones[k] = " ";                   // Очищаем переменную
                     tone(BuzzerPin, 1915);             // Сигнализируем динамиком о удалении номера дублера
                     delay(1000);
                     noTone(6);
                     sendSMS(F("Nomer 4 udalen!"));
                     break;
              case 4:
                    for(int i = 80; i < 93; i ++ )
                    {
                        EEPROM.write(i, (byte)' ');           // Перезаписываем номер админа 5 пробелами
                    }
                    phones[k] = " ";                          // Очищаем переменную
                    tone(BuzzerPin, 1915);                    // Сигнализируем динамиком о удалении номера дублера
                    delay(1000); 
                    noTone(6);
                    sendSMS(F("Nomer 5 udalen!"));
                    break;
              default:                                        // Если что-то пошло не так
              sendSMS(F("Nomer ne udalen!"));  
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
          if (phones[k][0] != '+' && result[0] == '+' && !flag_number)// Если есть место в позиции k, номер валидный и еще не записан
          {
            switch(k)
            {
              case 1:
                    for(int i = 20, j = 0; i < 33; i ++ , j ++ )
                      {
                        EEPROM.write(i, (byte)result[j]);     // Записываем в память номер админа 2                       
                      }
                      phones[k] = result;                     // Записываем в переменную номер админа 2
                      tone(BuzzerPin, 1915);                  // Сигнализируем динамиком о принятии номера дублера
                      delay(1000);
                      noTone(6);
                      sendSMS(F("Nomer 2 prinyat!")); 
                      flag_number = true;                     // Поднимаем флаг записи номера
                      break;
              case 2:
                    for(int i = 40, j = 0; i < 53; i ++ , j ++ )
                      {
                        EEPROM.write(i, (byte)result[j]);     // Записываем в память номер админа 3
                      }
                      phones[k] = result;                     // Записываем в переменную номер админа 3
                      tone(BuzzerPin, 1915);                  // Сигнализируем динамиком о принятии номера дублера
                      delay(1000);
                      noTone(6);
                      sendSMS(F("Nomer 3 prinyat!"));
                      flag_number = true;                     // Поднимаем флаг записи номера
                      break;
              case 3:
                    for(int i = 60, j = 0; i < 73; i ++ , j ++ )
                     {
                        EEPROM.write(i, (byte)result[j]);     // Записываем в память номер админа 4
                     }
                     phones[k] = result;                      // Записываем в переменную номер админа 4
                     tone(BuzzerPin, 1915);                   // Сигнализируем динамиком о принятии номера дублера
                     delay(1000);
                     noTone(6);
                     sendSMS(F("Nomer 4 prinyat!"));
                     flag_number = true;                      // Поднимаем флаг записи номера
                     break;
              case 4:
                    for(int i = 80, j = 0; i < 93; i ++ , j ++ )
                    {
                        EEPROM.write(i, (byte)result[j]);     // Записываем в память номер админа 5
                    }
                    phones[k] = result;                       // Записываем в переменную номер админа
                    tone(BuzzerPin, 1915);                    // Сигнализируем динамиком о принятии номера дублера
                    delay(1000);
                    noTone(6);
                    sendSMS(F("Nomer 5 prinyat!"));
                    flag_number = true;                       // Поднимаем флаг записи номера
                    break;
              default:
              sendSMS(F("Nomer ne prinyat!"));  
            }
          }                 
        }                            
      }
      else
        {
            if(result.length()  ==  5)                        // Если в сообщении 5-значная команда
            {
              String komanda = "";                            // Объявляем переменную для записи команд
              String znachenie = "";                          // Объявляем переменную для записи значений
              komanda += result[0];                           // Получаем команду
              komanda += result[1];              
              znachenie += result[3];                         // Получаем значение
              znachenie += result[4]; 
              if(komanda == "On")                             // Если команда "задержка включения"
              {
                  for(int i = 100, j = 0; i < 102; i ++ , j ++ )
                  {
                    EEPROM.write(i, (byte)znachenie[j]);      // Записываем время задержки в память EEPROM           
                  }
                  time_zaderjka_on = znachenie.toInt() * 1000;// Записываем время задержки в переменную
                  znachenie += F(" sek on");                  // Формируем уведомление
                  sendSMS(znachenie);                         // Отправляем уведомление 
              }
              if(komanda == "Of")                             // Если команда "задержка выключения"
              {
                  for(int i = 102, j = 0; i < 104; i ++ , j ++ )
                  {
                    EEPROM.write(i, (byte)znachenie[j]);      // Записываем время задержки в память EEPROM            
                  }
                  time_zaderjka_off = znachenie.toInt() * 1000; // Записываем время задержки в переменную
                  znachenie += F(" sek of");                  // Формируем уведомление
                  sendSMS(znachenie);                         // Отправляем уведомление
              } 
              
              if(komanda == "Bl")                             // Если команда "минимальный баланс для уведомления"
              {
                  for(int i = 104, j = 0; i < 106; i ++ , j ++ )
                  {
                    EEPROM.write(i, (byte)znachenie[j]);      // Записываем минимальный баланс для уведомления в память EEPROM            
                  }
                  balance_send = znachenie.toInt();           // Записываем минимальный баланс для уведомления в переменную
                  znachenie += F(" grn bl");                  // Формируем уведомление
                  sendSMS(znachenie);                         // Отправляем уведомление
              }

             if(result[0] == '*' && result[4] == '#')         // Если команда "код проверки баланса" 
             {
              kod_operatora = "";                             // Очищаем переменную хранения кода проверки баланса
                  for(int i = 107, j = 0; i < 112; i ++ , j ++ )
                  {
                    EEPROM.write(i, (byte)result[j]);         // Записываем код проверки баланса в память EEPROM
                    kod_operatora += result[j];               // Записываем код проверки баланса в переменную           
                  }  
                  sendSMS(kod_operatora);                     // Отправляем уведомление      
             }
            }
             else
            {
              if(result.length()  ==  2)                      // Если в сообщении 2-значная команда
              {
                String komanda = "";                          // Объявляем переменную для записи команд
                komanda += result[0];                         // Получаем значение в переменную    
                komanda += result[1];
                if(komanda == "10")
                {
                  D5_Low;                                     // Выключаем реле 2
                  sendSMS(F("Rele 1 OFF"));                   // Отправляем уведомление о выключении реле 1
                }
                if(komanda == "11") 
                {
                  D5_High;                                    // Включаем реле 1
                  sendSMS(F("Rele 1 ON"));                    // Отправляем уведомление о включении реле 1
                }                
                if(komanda == "21") 
                {   
                  D7_High;                                    // Включаем реле 2
                  sendSMS(F("Rele 2 ON"));                    // Отправляем уведомление о включении реле 2
                }
                if(komanda == "20")                           // Если команда выключить реле 2
                {    
                  D7_Low;                                     // Выключаем реле 2
                  sendSMS(F("Rele 2 OFF"));                   // Отправляем уведомление о выключении реле 2
                }
              }
            }
        }
      }

}

//////////////////////////////////////////// Функция отправки СМС сообщений ///////////////////////////////////////////

void sendSMS(String message)
{
  for(int i = 0; i < counter_admins; i ++ )
  {
    if(phones[i][0] == '+')
    {
      sendATCommand("AT+CMGS=\"" + phones[i] + "\"", true);          // Переходим в режим ввода текстового сообщения
      sendATCommand(message + "\r\n" + (String)((char)26), true);    // После текста отправляем перенос строки и Ctrl + Z
      delay(2500);
    }
  } 
}

///////////////////////////////////////// Функция получения БАЛАНСА СИМ карты в меню /////////////////////////////

float Balans(String & kod) 
{
bool flag = true;
_response  =  sendATCommand("AT+CUSD=1,\"" + kod + "\"", true);   // Отправляем USSD-запрос баланса

 while(flag)                                              // Запускаем безконечный цикл для ожидания ответа на запрос /////////////////////////////// Оптимизировать выход из цикла
 {
  if (SIM800.available())                                 // Если модем, что-то отправил...
  {
   _response  =  waitResponse();                          // Получаем ответ от модема для анализа 
   _response.trim();                                      // Убираем лишние пробелы в начале и конц
   if (_response.startsWith("+CUSD:"))                    // Если пришло уведомление о USSD-ответе (балансе на счету)
    {   
      if (_response.indexOf("\"")  >  -1)                 // Если ответ содержит кавычки, значит есть сообщение (предохранитель от "пустых" USSD-ответов)
      {       
        String msgBalance  =  _response.substring(_response.indexOf("\"") + 1, 50);  // Получаем непосредственно текст
        msgBalance  =  msgBalance.substring(0, msgBalance.indexOf("\""));
        float balance  =  getFloatFromString(msgBalance);                          // Извлекаем информацию о балансе     
        flag = false;                                                              // Выходим из цикла в основное меню
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

float getFloatFromString(String & str) {            
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
                                                                                   
void balanceSim()
{
// if((millis()-timerBalance) > 20000)                       // Периодичность проверки баланса
// if((millis()-timerBalance) > time_balance || !flagBalance)  // Периодичность проверки баланса (если сработал таймер или первая проверка)
if((millis() - timerBalance) > time_balance)
  {
    String message  =  F("Balans na sim  =  ");
    float balance = Balans(kod_operatora);                 // Извлекаем баланс
    delay(3000);
    if(balance <= balance_send)                            // Порог баланса для оповещения
    {
      message += (String)balance;                          // Добавляем баланс в сообщение
      sendSMS(message);                                    // Отправляем уведомление о балансе администратору
    }
      timerBalance = millis();                             // Сбрасываем счетчик
      flagBalance = true;                                  // Поднимаем флаг проведенной проверки баланса
  }
}

void initialZones()////////////////////////////// Функция инициализации зон ////////////////////////////////////////////////////
  {
///////////////////////// 0  Нет тревоги    
///////////////////////// 3  Выход реле тревоги                                
///////////////////////// 1  Выход клапан газа                                  
///////////////////////// 2  Выход клапан воды                           

    for(int i = 0; i < 6; i ++ )
    {
      zona[i].Adress = i;                                    // Инициализируем адрес зоны
      switch (i)                                             // Инициализируем пины тревоги и уведомления о сработках
      {
      case 0:                                                // Для зоны 1 (охранная)
       strcpy(zona[i].messageAlarm,"Srabotka v zone 1!");    // Формируем уведомление о сработке
       zona[i].PinAlarm1 = 3;                                // При сработке задействуем реле тревоги
       break;
      case 1:                                                // Для зоны 2 (охранная)
       strcpy(zona[i].messageAlarm,"Srabotka v zone 2!");    // Формируем уведомление о сработке
       zona[i].PinAlarm1 = 3;                                // При сработке задействуем реле тревоги
       break;
      case 2:                                                // Для зоны 3 (охранная)
       strcpy(zona[i].messageAlarm,"Srabotka v zone 3!");    // Формируем уведомление о сработке  
       zona[i].PinAlarm1 = 3;                                // При сработке задействуем реле тревоги
       break;
      case 3:                                                // Для зоны 4 (аварийная "Пожар")
       strcpy(zona[i].messageAlarm,"Vozgoranie!!!");         // Формируем уведомление о сработке 
       zona[i].PinAlarm1 = 3;                                // При сработке задействуем реле тревоги
       zona[i].PinAlarm2 = 1;                                // При сработке задействуем реле выключения клапана газа
       break;
      case 4:                                                // Для зоны 5 (аварийная "Газ")
       strcpy(zona[i].messageAlarm,"Utechka gaza!!!");       // Формируем уведомление о сработке 
       zona[i].PinAlarm1 = 3;                                // При сработке задействуем реле тревоги
       zona[i].PinAlarm2 = 1;                                // При сработке задействуем реле выключения клапана газа
       break;
      case 5:                                                // Для зоны 6 (аварийная "Вода")
       strcpy(zona[i].messageAlarm,"Protechka vody!");       // Формируем уведомление о сработке 
       zona[i].PinAlarm1 = 3;                                // При сработке задействуем реле тревоги
       zona[i].PinAlarm2 = 2;                                // При сработке задействуем реле выключения клапана воды 
       break;
      }
    }
    initialMacros();
  }

///////////////////////////////////////// Функция инициализации макросов /////////////////////////////////////////////

  void initialMacros()      
  {
    switch (macros_id)
    {
     case 0:
      for(int i = 0; i < 6; i ++ )
      {
        zona[i].Status = macros_1[i];   // Активны все зоны
      }
      break;
     case 1:
      for(int i = 0; i < 6; i ++ )
      {
        zona[i].Status = macros_2[i];   // Активны только аварийные зоны        
      }
      break;
     case 2:
      for(int i = 0; i < 6; i ++ )
      {
        zona[i].Status = macros_3[i];   // Активны только охранные зоны        
      }
      break;
     case 3:
      for(int i = 0; i < 6; i ++ )
      {
        zona[i].Status = macros_4[i];   // Активны аварийные зоны и две охранные      
      }
      break;  
    default:
      for(int i = 0; i < 6; i ++ )
      {
        zona[i].Status = macros_1[i];   // Активны все зоны       
      }
      break;  
    }  
      
  }

////////////////////////////////////////////// Функция уведомления о напряжении в сети 220 в ////////////////////////////////////////////////////

void voltage()
{
 if(digitalRead(VoltPin) == HIGH)                                                    // Если датчик напряжения в сработке (напряжения нет, работа от акб)
 {  
    if(!flagVolt)                                                                    // Если это первая сработка
    {
      flagVolt = true;                                                               // Поднимаем флаг сработок
      sendSMS(F("Rabota ot AKB!"));                                                  // Отправляем уведомление о работе от АКБ администратору  
    }
 }
 else                                                                                // Если датчик в покое (напряжение есть, работа от сети 220в)
 {
    if(flagVolt)                                                                     // Если перед этим была сработка
    {
      flagVolt = false;                                                              // Опускаем флаг сработок
      sendSMS(F("Rabota ot 220v!"));                                                 // Отправляем уведомление о восстановлении 
                                                                                     // работы от сети 220 в администратору 
    }
    return;                                                                          // Выходим в главную функцию
 }
}
/////////////////////////////////////////// Функция проверки доступности сим-модуля //////////////////////////////

void testModem()
{
  do {              // При включении питания МК загрузится раньше сим модуля поэтому ждем загрузки и ответа на команду
    _response  =  sendATCommand("AT", true);                  // Отправили AT для настройки скорости обмена данными
    _response.trim();                                         // Убираем пробельные символы в начале и конце
    if(AT_counter == 3)
    {
       tone(BuzzerPin, 1915);                                 // Сигнализируем звуком об отсутсвии связи с сим-модулем
       delay(150);
       noTone(6);
       delay(20);
       tone(BuzzerPin, 1915);                  
       delay(150);
       noTone(6);
       delay(20);
       tone(BuzzerPin, 1915);                  
       delay(150);
       noTone(6);
       delay(1000);
       AT_counter = 0;
       if(flagloop) break;
    }
    if(_response != "OK")AT_counter ++ ;   
  } while (_response != "OK");                                    // Не пускать дальше, пока модуль не вернет ОК
}

//////////////////////////////////// Функция стартовой инициализации сим-модуля ///////////////////////////////////////

void initialModem()
{  
  sendATCommand("AT+CMGDA=\"DEL ALL\"", true);                    // Удаляем все SMS, чтобы не забивать память
  delay(1000);
  // Команды настройки модема при каждом запуске
 
//sendATCommand("AT + DDET = 1", true);                           // Включаем DTMF
  sendATCommand("AT+CLIP=1", true);                               // Включаем АОН
  delay(1000);
  sendATCommand("AT+CMGF=1;&W", true);                            // Включаем текстовый режима SMS (Text mode) и сразу сохраняем значение (AT&W)!
  delay(1000);
}

///////////////////////////////////////// Функция получения новых смс сообщений ///////////////////////////////////////

void getNewSMS()
  { 
   if (millis() - last_Update > updatePeriod || flag_timer_sms) {       // Пора проверить наличие новых сообщений
    do {
      _response  =  sendATCommand("AT+CMGL=\"REC UNREAD\"", true); // Отправляем запрос чтения непрочитанных сообщений
      delay(1000);
      if (_response.indexOf("+CMGL: ")  >  -1) 
      {                                                           // Если есть хоть одно, получаем его индекс
        int msgIndex  =  _response.substring(_response.indexOf("+CMGL: ") + 7, _response.indexOf("\"REC UNREAD\"", _response.indexOf("+CMGL: "))).toInt();
        int i  =  0;                                                // Объявляем счетчик попыток
        counter_Errors = 0;
        do 
        {
          i ++ ;                                                  // Увеличиваем счетчик
          Serial.println (i);
          _response  =  sendATCommand("AT+CMGR=" + (String)msgIndex + ",1", true);  // Пробуем получить текст SMS по индексу
          _response.trim();                                       // Убираем пробелы в начале/конце
          if (_response.endsWith("OK")) 
          {                                                       // Если ответ заканчивается на "ОК"
            if (!hasmsg) hasmsg  =  true;                         // Ставим флаг наличия сообщений для удаления
            sendATCommand("AT+CMGR=" + (String)msgIndex, true);   // Делаем сообщение прочитанным
            sendATCommand("\n", true);                            // Перестраховка - вывод новой строки
            parseSMS(_response);                                  // Отправляем текст сообщения на обработку
            flag_timer_sms = false;
            break;                                                // Выход из do{}
          }
          else 
          {                                                       // Если сообщение не заканчивается на OK
            sendATCommand("\n", true);                            // Отправляем новую строку и повторяем попытку
          }
        } while (i < 10);
        break;
      }
      else 
      {
        counter_Errors ++ ;
        last_Update  =  millis();                                 // Обнуляем таймер ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        if (hasmsg || counter_Errors > 10) 
        {
          sendATCommand("AT+CMGDA=\"DEL ALL\"", true);            // Удаляем все сообщения
          hasmsg  =  false;         
        }
        flag_timer_sms = false; 
        break;
      }

    } while (1);   
   }
  }

/////////////////////////////////////// Функция работы с клавиатурой /////////////////////////////////////////////////

void klava()
{
  if(digitalRead(KlavaPin) == LOW)                                  // Если пропал сигнал с клавиатуры
  {
    delay(1500);
    if(digitalRead(KlavaPin) == HIGH)                               // Если сигнал появился через 1,5 сек, сигнал валидный (смена режима)
    { 
      if(!flagKlava)                                                // Флаг обрыва не поднят
      {
        if(!rejim)                                                  // Если ставим на охрану
        {
          delay(time_zaderjka_on);                                  // Ждем указанное время до постановки
          rejim = !rejim;                                           // Переключаем режим на противоположный
          sendSMS(F("Rejim ON"));                                   // Отправляем смс с результатом администратору
        }
        else                                                        // Если снимаем с охраны
        {
          rejim = !rejim;                                           // Переключаем режим на противоположный
          sendSMS(F("Rejim OFF"));                                  // Отправляем смс с результатом администратору
        }
      }
      else                                                          // Если флаг обрыва поднят
      {
        flagKlava = false;                                          // Поднимаем флаг обрыва
        sendSMS(F("Klava ON"));                                     // Отправляем смс о подключении клавиатуры        
      }
    }
    else                                                            // Если сигнал не появился через 1,5 сек
    {
      if(!flagKlava)                                                // Если флаг обрыва не поднят                   
      {
        flagKlava = true;                                           // Поднимаем флаг обрыва
        sendSMS(F("Klava OFF"));                                    // Отправляем смс об отсутствии клавиатуры
      }
    }
  }
}

///////////////////////////////////////// Функция активации реле ////////////////////////////////////////////////////////

void ActivateRele(byte Alarm1, byte Alarm2)
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

void initialEeprom()
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
      temp += (char)EEPROM.read(i);                            // Получаем из EEPROM вермя задержки вкл
   }
   time_zaderjka_on = ((String)temp).toInt()*1000;             // Инициализируем цифровую переменную задержки включения
   temp = "";                                                  // Обнуляем строковую переменную задержки включения
   for(int i = 102; i < 104; i ++ )
   {
      temp += (char)EEPROM.read(i);                            // Получаем из EEPROM вермя задержки выкл
   }
   time_zaderjka_off = ((String)temp).toInt()*1000;            // Инициализируем цифровую переменную задержки выключения
   temp = "";                                                  // Обнуляем строковую переменную задержки выключения
  for(int i = 104; i < 106; i ++ )
  {
      temp += (char)EEPROM.read(i);                            // Получаем из EEPROM баланс
  }
   macros_id = EEPROM.read(106);                               // Получаем из EEPROM текущий макрос

  for(int i = 107, j = 0; i < 112; i ++ , j ++ )
  {
    kod_operatora += (char)EEPROM.read(i);                     // Получаем из EEPROM код оператора  
  }
}

////////////////////////////////////// Функция индикации входящего звонка /////////////////////////////////////////

void dingDong()     {  
  if((millis() - timerRing < 3000))                       
  {
     // digitalWrite(LedPin, HIGH);
     D13_Inv;                                         // Переключаем светодиод
      tone(BuzzerPin, 1915);                          // Включаем пищалку
      delay(50);                                      // Пауза
    //  digitalWrite(LedPin, LOW);
    D13_Inv;                                          // Переключаем светодиод
      noTone(6);                                      // Выключаем пищалку
      delay(50);                                      // Пауза
  }
}

//////////////////////////////////////////// Функция обработки входящих звонков ///////////////////////////////////

void incomingCall()    {
  bool flagAdmin = false;
  flagCall = true;                                     // Поднимаем флаг звонка
  int phoneindex  =  _response.indexOf("+CLIP: \"");   // Есть ли информация об определении номера, если да, то phoneindex > -1
  String innerPhone  =  "";                            // Переменная для хранения определенного номера
  if (phoneindex  >=  0)                               // Если информация была найдена
  {
        phoneindex += 8;                                                                   // Парсим строку и 
        innerPhone  =  _response.substring(phoneindex, _response.indexOf("\"", phoneindex)); // получаем номер     
  } 
  for(int i = 0; i < counter_admins; i ++ )
  {
    if(innerPhone == phones[i])
    {
      flagAdmin = true;
      break;
    }
  }
  if(rejim)                                        // Если включен режим охраны
  {
    if(flagAdmin)                                  // Если звонит админ
    {
      rejim = !rejim;                              // Сменить режим охраны
      sendATCommand("ATH", true);                  // сбросить звонок
      sendSMS(F("Rejim OFF"));                     // Отправляем смс с результатом администратору
      return;                                      // и выйти 
    }
    else                                           // Звонит не админ
    {
      sendATCommand("ATH", true);                  // сбросить звонок
      return;                                      // и выйти
    }
  }
  else
  {
    if(flagAdmin)                                  // Если звонит админ
    {
      rejim = !rejim;                              // Сменить режим охраны
      sendATCommand("ATH", true);                  // сбросить звонок
      sendSMS(F("Rejim ON"));                      // Отправляем смс с результатом администратору
      return;      
    } 
        timerRing = millis();                      // Включаем таймер для визуализации звонка  
                                                                    // Если длина номера больше 6 цифр, 
      if (innerPhone.length()  >=  7 && digitalRead(KnopkaPin) == HIGH)   // и нажата кнопка на плате
      {
        phones[0] = innerPhone;

        for(int i = 0; i < 13; i ++ )
        {
          EEPROM.write(i, (byte)phones[0][i]);                      // Записываем номер в память EEPROM
        }      
        sendATCommand("ATH", true);                                 // Сбрасываем звонок
          delay(3000);          
          tone(BuzzerPin, 1915);                                    // Сигнализируем динамиком о принятии номера администратора
          delay(1000);
          noTone(6); 
        
        sendSMS(F("Nomer admina prinyat!"));  
      }
  }
}
