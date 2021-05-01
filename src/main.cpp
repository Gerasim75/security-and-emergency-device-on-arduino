/// My_feacture

//// Голосовые уведомления звонком

/////////////////////////// Охранно-аварийная gsm-сигнализация с sms-управляемыми реле ////////////////////////////////

#include  <SoftwareSerial.h>                                
#include  <EEPROM.h> 
#include  <CyberLib.h> 
#include  "sensor.h"
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
#define DELAY_PLAY_TRACK 3 // Пауза для проигрыша трека
                                                          
SoftwareSerial SIM800(14, 15);                                  // RX, TX (пины подключения сим-модуля) 

String operator_code = "";                                      // Переменная кода оператора сим
String _response     =  "";                                     // Переменная для хранения ответа модуля
String phones[5] = {"","","","",""};                            // Телефон админа
String temp = "";
int AT_counter = 1;
int balance_send;
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
byte triggered[6] = {9,9,9,9,9,9};                              // Массив адресов сработавших датчиков
byte counter_triggered = 0;                                     // Счетчик сработок датчиков
byte counter_admins = 5;                                        // Количество администраторов
Sensor zone[6];

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
        if(i == 0 && triggered[i] == 0)                                          // Если сработка в зоне 1
        {
          if(millis() - timer_delay > timer_delay_off)                           // Если сработал таймер
          {     
            ActivateRelay(zone[triggered[i]].pin_alarm_1_, zone[triggered[i]].pin_alarm_2_); // Активируем реле тревоги
            SendSMS(zone[triggered[i]].message_alarm_);                          // Отправляем смс уведомление
            if(!flag_tel)                                                        // Если еще не было звонка администратору
            {
              flag_tel = true;                                                   // Поднимаем флаг звонка  
              Call(phones[0], zone[i].adress_);                                  // Звоним администратору
            }
            zone[triggered[i]].send_alarm_ = true;                               // Поднимаем флаг отправленного уведомления
            flag_delay_zone1 = true;                                             // Поднимаем флаг задержки сработки
            break;                 
          }
        }
        else
        {
          if(triggered[i] == 1 && flag_zone1 && !flag_delay_zone1) break;
          ActivateRelay(zone[triggered[i]].pin_alarm_1_, zone[triggered[i]].pin_alarm_2_);   // Активируем реле тревоги
          SendSMS(zone[triggered[i]].message_alarm_);                                 // Отправляем смс уведомление
          if(!flag_tel)                                                               // Если еще не было звонка администратору
          {
            flag_tel = true;                                                          // Поднимаем флаг звонка  
            Call(phones[0], zone[i].adress_);                                         // Звоним администратору
          }
          zone[triggered[i]].send_alarm_ = true;                                      // Поднимаем флаг отправленного уведомления
          break;                                                                      // Выходим из цикла
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
        SendSMS(F("mode 1 ON"));                   // Отправляем смс с результатом администратору 
        mode = true;       
      }
      if(ledState == 2) 
      {
        EEPROM.write(106, 1);                    // Записываем его в память
        macros_id = 1;
        InitialMacros();                         // Инициализируем макрос 
        SendSMS(F("mode 2 ON"));                 // Отправляем смс с результатом администратору 
        mode = true;              
      }
      if(ledState == 3)
      {
        EEPROM.write(106, 2);                    // Записываем его в память
        macros_id = 2;
        InitialMacros();                         // Инициализируем макрос   
        SendSMS(F("mode 3 ON"));                 // Отправляем смс с результатом администратору 
        mode = true;            
      }
      if(ledState == 4)
      { 
        EEPROM.write(106, 3);                    // Записываем его в память
        macros_id = 3;
        InitialMacros();                         // Инициализируем макрос
        SendSMS(F("mode 4 ON"));                 // Отправляем смс с результатом администратору    
        mode = true;               
      }
      if(ledState == 5) 
      {
        EEPROM.write(106, 4);                    // Записываем его в память
        macros_id = 4;
        InitialMacros();                         // Инициализируем макрос  
        SendSMS(F("mode 5 ON"));                 // Отправляем смс с результатом администратору
        mode = true;
      }
      if(ledState == 0) 
      {
        mode = false;                            // Снимаем с охраны
        SendSMS(F("mode OFF"));                  // Отправляем смс с результатом администратору
      }     
      tone(BUZZER_PIN, 1915);                    // Сигнализируем динамиком о смене режима охраны
      delay(1000);
      noTone(6); 
      correct  =  true;                          // Флаг корректности команды
    }
    if (!correct) 
    {
      SendSMS(F("Incorrect command!"));          // Отправляем смс с результатом администратору
    }
  }
  else
    {
      if(result.length()  ==  13 && msgphone == phones[0])  // Если в сообщении номер телефона и отправитель админ 1
      {
        bool flag_number = false;
        for(int k = 1; k < counter_admins; k ++ )       // Проходим по списку номеров
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
                      tone(BUZZER_PIN, 1915);           // Сигнализируем динамиком о удалении номера дублера
                      delay(1000);
                      noTone(6);
                      SendSMS(F("Number 2 deleted!"));
                      break;
              case 2:
                    for(int i = 40; i < 53; i ++ )
                      {
                        EEPROM.write(i, (byte)' ');     // Перезаписываем номер админа 3 пробелами
                      }
                      phones[k] = " ";                  // Очищаем переменную
                      tone(BUZZER_PIN, 1915);           // Сигнализируем динамиком о удалении номера дублера
                      delay(1000);
                      noTone(6);
                      SendSMS(F("Number 3 deleted!"));
                      break;
              case 3:
                    for(int i = 60; i < 73; i ++ )
                     {
                        EEPROM.write(i, (byte)' ');     // Перезаписываем номер админа 4 пробелами
                     }
                     phones[k] = " ";                   // Очищаем переменную
                     tone(BUZZER_PIN, 1915);            // Сигнализируем динамиком о удалении номера дублера
                     delay(1000);
                     noTone(6);
                     SendSMS(F("Number 4 deleted!"));
                     break;
              case 4:
                    for(int i = 80; i < 93; i ++ )
                    {
                        EEPROM.write(i, (byte)' ');           // Перезаписываем номер админа 5 пробелами
                    }
                    phones[k] = " ";                          // Очищаем переменную
                    tone(BUZZER_PIN, 1915);                   // Сигнализируем динамиком о удалении номера дублера
                    delay(1000); 
                    noTone(6);
                    SendSMS(F("Number 5 deleted!"));
                    break;
              default:                                        // Если что-то пошло не так
              SendSMS(F("Number not deleted!"));  
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
                      tone(BUZZER_PIN, 1915);                 // Сигнализируем динамиком о принятии номера дублера
                      delay(1000);
                      noTone(6);
                      SendSMS(F("Number 2 accepted!")); 
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
                      SendSMS(F("Number 3 accepted!"));
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
                     SendSMS(F("Number 4 accepted!"));
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
                    SendSMS(F("Number 5 accepted!"));
                    flag_number = true;                       // Поднимаем флаг записи номера
                    break;
              default:
              SendSMS(F("Number not accepted!"));  
            }
          }                 
        }                            
      }
      else
        {
            if(result.length()  ==  5)                   // Если в сообщении 5-значная команда
            {
              String command = "";                       // Объявляем переменную для записи команд
              String item = "";                          // Объявляем переменную для записи значений
              command += result[0];                      // Получаем команду
              command += result[1];              
              item += result[3];                         // Получаем значение
              item += result[4]; 
              if(command == "On")                        // Если команда "задержка включения"
              {
                  for(int i = 100, j = 0; i < 102; i ++ , j ++ )
                  {
                    EEPROM.write(i, (byte)item[j]);      // Записываем время задержки в память EEPROM           
                  }
                  timer_delay_on = item.toInt() * 1000;  // Записываем время задержки в переменную
                  item += F(" seс on");                  // Формируем уведомление
                  SendSMS(item);                         // Отправляем уведомление 
              }
              if(command == "Of")                        // Если команда "задержка выключения"
              {
                  for(int i = 102, j = 0; i < 104; i ++ , j ++ )
                  {
                    EEPROM.write(i, (byte)item[j]);      // Записываем время задержки в память EEPROM            
                  }
                  timer_delay_off = item.toInt() * 1000; // Записываем время задержки в переменную
                  item += F(" seс of");                  // Формируем уведомление
                  SendSMS(item);                         // Отправляем уведомление
              } 
              
              if(command == "Bl")                        // Если команда "минимальный баланс для уведомления"
              {
                  for(int i = 104, j = 0; i < 106; i ++ , j ++ )
                  {
                    EEPROM.write(i, (byte)item[j]);      // Записываем минимальный баланс для уведомления в память EEPROM            
                  }
                  balance_send = item.toInt();           // Записываем минимальный баланс для уведомления в переменную
                  item += F(" grn bl");                  // Формируем уведомление
                  SendSMS(item);                         // Отправляем уведомление
              }

             if(result[0] == '*' && result[4] == '#')         // Если команда "код проверки баланса" 
             {
              operator_code = "";                             // Очищаем переменную хранения кода проверки баланса
                  for(int i = 107, j = 0; i < 112; i ++ , j ++ )
                  {
                    EEPROM.write(i, (byte)result[j]);         // Записываем код проверки баланса в память EEPROM
                    operator_code += result[j];               // Записываем код проверки баланса в переменную           
                  }  
                  SendSMS(operator_code);                     // Отправляем уведомление      
             }
            }
             else
            {
              if(result.length()  ==  2)                      // Если в сообщении 2-значная команда
              {
                String command = "";                          // Объявляем переменную для записи команд
                command += result[0];                         // Получаем значение в переменную    
                command += result[1];
                if(command == "10")
                {
                  D5_Low;                                     // Выключаем реле 2
                  SendSMS(F("Relay 1 OFF"));                  // Отправляем уведомление о выключении реле 1
                }
                if(command == "11") 
                {
                  D5_High;                                    // Включаем реле 1
                  SendSMS(F("Relay 1 ON"));                   // Отправляем уведомление о включении реле 1
                }                
                if(command == "21") 
                {   
                  D7_High;                                    // Включаем реле 2
                  SendSMS(F("Relay 2 ON"));                   // Отправляем уведомление о включении реле 2
                }
                if(command == "20")                           // Если команда выключить реле 2
                {    
                  D7_Low;                                     // Выключаем реле 2
                  SendSMS(F("Relay 2 OFF"));                  // Отправляем уведомление о выключении реле 2
                }
              }
            }
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
    String message  =  F("Balance of sim  =  ");
    float balance = GetBalans(operator_code);              // Извлекаем баланс
    delay(3000);
    if(balance <= balance_send)                            // Порог баланса для оповещения
    {
      message += (String)balance;                          // Добавляем баланс в сообщение
      SendSMS(message);                                    // Отправляем уведомление о балансе администратору
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
       strcpy(zone[i].message_alarm_,"Triggered to zone 1!"); // Формируем уведомление о сработке
       zone[i].pin_alarm_1_ = 3;                              // При сработке задействуем реле тревоги
       break;
      case 1:                                                 // Для зоны 2 (охранная)
       strcpy(zone[i].message_alarm_,"Triggered to zone 2!"); // Формируем уведомление о сработке
       zone[i].pin_alarm_1_ = 3;                              // При сработке задействуем реле тревоги
       break;
      case 2:                                                 // Для зоны 3 (охранная)
       strcpy(zone[i].message_alarm_,"Triggered to zone 3!"); // Формируем уведомление о сработке  
       zone[i].pin_alarm_1_ = 3;                              // При сработке задействуем реле тревоги
       break;
      case 3:                                                 // Для зоны 4 (аварийная "Пожар")
       strcpy(zone[i].message_alarm_,"Wildfire!!!");          // Формируем уведомление о сработке 
       zone[i].pin_alarm_1_ = 3;                              // При сработке задействуем реле тревоги
       zone[i].pin_alarm_2_ = 1;                              // При сработке задействуем реле выключения клапана газа
       break;
      case 4:                                                 // Для зоны 5 (аварийная "Газ")
       strcpy(zone[i].message_alarm_,"Gas leakage!!!");       // Формируем уведомление о сработке 
       zone[i].pin_alarm_1_ = 3;                              // При сработке задействуем реле тревоги
       zone[i].pin_alarm_2_ = 1;                              // При сработке задействуем реле выключения клапана газа
       break;
      case 5:                                                 // Для зоны 6 (аварийная "Вода")
       strcpy(zone[i].message_alarm_,"Water leak!");          // Формируем уведомление о сработке 
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
      SendSMS(F("Battery operation!"));                                              // Отправляем уведомление о работе от АКБ администратору  
    }
 }
 else                                                                                // Если датчик в покое (напряжение есть, работа от сети 220в)
 {
    if(flag_volt)                                                                    // Если перед этим была сработка
    {
      flag_volt = false;                                                             // Опускаем флаг сработок
      SendSMS(F("Operation from 220 volts!"));                                       // Отправляем уведомление о восстановлении 
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
          SendSMS(F("mode ON"));                                    // Отправляем смс с результатом администратору
        }
        else                                                        // Если снимаем с охраны
        {
          mode = !mode;                                             // Переключаем режим на противоположный
          SendSMS(F("mode OFF"));                                   // Отправляем смс с результатом администратору
        }
      }
      else                                                          // Если флаг обрыва поднят
      {
        flag_keyboard = false;                                      // Поднимаем флаг обрыва
        SendSMS(F("Keyboard ON"));                                  // Отправляем смс о подключении клавиатуры        
      }
    }
    else                                                            // Если сигнал не появился через 1,5 сек
    {
      if(!flag_keyboard)                                            // Если флаг обрыва не поднят                   
      {
        flag_keyboard = true;                                       // Поднимаем флаг обрыва
        SendSMS(F("Keyboard OFF"));                                 // Отправляем смс об отсутствии клавиатуры
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
      temp += (char)EEPROM.read(i);                             // Получаем из EEPROM вермя задержки выкл
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
      SendSMS(F("mode OFF"));                      // Отправляем смс с результатом администратору
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
      SendSMS(F("mode ON"));                       // Отправляем смс с результатом администратору
      return;      
    } 
    timer_ring = millis();                         // Включаем таймер для визуализации звонка  
                                                                         // Если длина номера больше 6 цифр, 
    if (inner_phone.length()  >=  7 && digitalRead(BUTTON_PIN) == HIGH)  // и нажата кнопка на плате
    {
      phones[0] = inner_phone;

      for(int i = 0; i < 13; i ++ )
      {
        EEPROM.write(i, (byte)phones[0][i]);     // Записываем номер в память EEPROM
      }  
      SIM800.print(F("AT+CREC=4,\"C:\\User\\admin.amr\",0,80"));   // Проигрываем звуковой файл
      delay(DELAY_PLAY_TRACK);                   // Делаем паузу для воспроизведения трека  
      SendATCommand("ATH", true);                // Сбрасываем звонок         
      tone(BUZZER_PIN, 1915);                    // Сигнализируем динамиком о принятии номера администратора
      delay(1000);
      noTone(6); 
      SendSMS(F("Administrator number accepted!"));  
    }
  }
}
