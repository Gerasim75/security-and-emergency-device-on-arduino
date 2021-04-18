<h1 align=center>Охранно-аварийная GSM-сигнализация с SMS-управляемыми реле</h1>
<p>
   &nbsp;Программа работает на контроллерах ATmega328 и использует GSM модуль SIM800L.
   Программа прошиватеся на контроллер из среды Platformio.
   Код программы подробно прокомментирован.
</p>
<h2 align=center>Установка программы на контроллер Arduino Nano из среды Platformio</h2>
<ul>
  <li>Открыть программу в новом проекте Platformio
  <li>Раскомментировать в файле platformio.ini порт подключения для своей операционной системы, по умолчанию Unix
  <li>Загрузить программу в контроллер штатным средством Platformio
</ul>
<h2 align=center>Установка программы на контроллер Arduino Nano из среды ArduinoIDE</h2>
<ul>
  <li>Создать новый проект в среде ArduinoIDE
  <li>Добавить в проект через менеджер библиотек: <a href="https://github.com/PaulStoffregen/SoftwareSerial/archive/refs/heads/master.zip">SoftwareSerial.h</a>, <a href="https://github.com/PaulStoffregen/EEPROM/archive/refs/heads/master.zip">EEPROM.h</a>, <a href="https://github.com/pythonista/CyberLib/archive/refs/heads/master.zip">CyberLib.h</a>
  <li>Выбрать в настройках проекта плату Arduino Nano
  <li>Скопировать код программы из src/main.cpp в файл проекта
  <li>Загрузить программу в контроллер штатным средством ArduinoIDE
</ul>
<h2 align=center>Описание работы устройства</h2>
<p>
  &nbsp;Аварийно охранное устройство предназначено для отслеживания аварийных и как дополнение, охранных ситуаций, а в случае возникновения таких, оповещения и если установлено соответствующее оборудование, то блокирования до выявления и устранения  причин аварии.
</p>
&nbsp;Данное устройство способно отслеживать:
<ul>
  <li>утечку природного и угарного газа,
  <li>появление дыма и высокой температуры в случае пожара,
  <li>протечку воды с соответствующей блокировкой водоснабжения,
  <li>вибрации (например стены), разбитие стекла,
  <li>отсутствие напряжения в сети с автоматическим переходом в автономный режим роботы от АКБ,
  <li>проникновение в помещение,
  <li>движение в помещении (при условии установки соответствующих датчиков).
</ul>
 &nbsp;Датчики охранных зон 1 и 2 работают в паре по принципу «Коридор» - на зону 1 устанавливается время задержки срабатывания и при проникновении через зону 1 , зона 2 выдерживает паузу установленную для зоны 1, после чего происходит тревога, а при проникновении через зону 2 минуя зону 1 тревога происходит мгновенно.
 <br/>
 &nbsp;С предлагаемым оборудованием совместимы практически любые датчики представленные в этом сегменте рынка. Также с помощью данного устройства можно управлять дистанционно любыми механизмами и оборудованием с помощью смс сообщений, это может быть освещение, поливная система, система отопления, кондиционер, аварийное электроснабжение и др.
<p>&nbsp;
  После установки и настройки данного оборудования производится фиксирование номера телефона администратора и номера телефона дублера, после чего с номера телефона администратора соответствующими командами выставляются параметры работы устройства.
  </p>
  <h3 align=center>Установки</h3>
<p>&nbsp;
  <b>Установка номера администратора:</b> звонок на номер карточки установленной в прибор, прибор отреагирует звуковой и светодиодной сигнализацией, нажать на кнопку на плате прибора до отбоя звонка. Прибор ответит длинным звуковым сигналом и оповестит по смс о принятии номера админа.
</p>
<p>&nbsp;
  <b>Установка номера дублера:</b> отправить с номера админа смс с номером телефона дублера в формате «+380*********». Прибор оповестит по смс о принятии номера дублера. Дублеров может быть еще 4 номера. Для удаления номера отправить СМС с номером дублера на устройство повторно.
</p>
&nbsp;<b>Команды смс для настройки параметров:</b>
<ul>
  <li><b>«On 00»</b> - Выставляется время задержки постановки на охрану. Допустимые значения от 00 до 99 (секунды). Пример: смс с текстом «On 25» установит задержку постановки на охрану на 25 секунд.
  <li><b>«Of 00»</b> - Выставляется время задержки сработки для снятия с охраны.
  <li><b>«Bl 00»</b> - Выставляется порог баланса средств на счету для оповещения. При понижении баланса ниже установленного, устройство оповестит об этом по смс один раз в сутки. Допустимые значения от 00 до 99.
  <li><b>«*000#»</b> - Выставляется код баланса оператора сети. Пример: <b>«*111#»</b> - Киевстар.
  <li><b>«11»</b> - Включить управляемое реле №1.
  <li><b>«10»</b> - Выключить управляемое реле №1.
  <li><b>«21»</b> - Включить управляемое реле №2.
  <li><b>«20»</b> - Выключить управляемое реле №2.
</ul>
&nbsp;<b>В данном устройстве создано пять режимов работы:</b>
<ol>
  <li>Активны все шесть зон                               {1.1.1.1.1.1}
  <li>Активны только охранные зоны                        {1.1.1.0.0.0}
  <li>Активны только аварийные зоны                       {0.0.0.1.1.1}
  <li>Активны аварийные зоны и одна охранная              {1.0.0.1.1.1}
  <li>Активны охранные зоны и одна аварийная  (пожар)     {1.1.1.1.0.0}
</ol>
&nbsp;<b>Назначение зон:</b>
<ul>
  <li>Зона 1, 2, 3 - охранные
  <li>Зона 4 - пожар, дым
  <li>Зона 5 - утечка газа, дым
  <li>Зона 6 - протечка воды
</ul>
  &nbsp;<b>Режим работы:</b>
<p>&nbsp;
  Выбирается отправкой смс с номера телефона админа или дублера на прибор с соответствующим номером режима работы. Пример: смс с текстом <b>«3»</b> поставит прибор на охрану в режиме 3, при дальнейших постановках на охрану и снятии с охраны прибора с клавиатуры или пустым звонком автоматически загружается режим выбранный по смс, после чего этот режим прописывается в память до следующей смены с помощью смс. Для смены режимов с помощью смс не обязательно снимать устройство с охраны, просто отправляется смс с номером желаемого режима работы и устройство оповестит вас о смене режима работы. Для снятия устройства с охраны с помощью смс отправить на номер устройства смс с текстом <b>«0»</b>. Так же постановка на охрану и снятие с охраны производится пустым звонком на устройство. Опционно устройство можно перевести на управление с помощью радио-брелоков вместо клавиатуры.
</p>
&nbsp;<b>Светозвуковая сигнализация:</b>
<ul>
  <li>три коротких звуковых сигнала - отсутствует связь с GSM-модулем
  <li>один длинный звуковой сигнал - успешная загрузка контроллера, смена режима, подтверждение принятия номера админа/дублера
  <li>повторяющиеся серии коротких звуковых и световых сигналов - входящий звонок
</ul>
<p>&nbsp;
Видеообзор функционала устройства на сайте проекта <a href="https://www.robot.zp.ua">www.robot.zp.ua</a>
</p>

