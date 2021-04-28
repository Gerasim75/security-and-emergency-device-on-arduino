<h1 align=center>Security and emergency GSM alarm system with SMS-controlled relays</h1>
<p>
   &nbsp;The program runs on ATmega328 controllers and uses the SIM800L GSM module.
   The program is flashed to the controller from the Platformio environment.
   The program code is commented in detail.
</p>
<h2 align=center>Installing the program on the Arduino Nano controller from the Platformio environment</h2>
<ul>
  <li>Open the program in a new Platformio project
  <li>Uncomment in the platformio.ini file the connection port for your operating system, by default Unix
  <li>Upload the program to the controller using the standard Platformio tool
</ul>
<h2 align=center>Installing the program on the Arduino Nano controller from the ArduinoIDE environment</h2>
<ul>
  <li>Create a new project in the ArduinoIDE environment
  <li>Add to the project via the library manager: <a href="https://github.com/PaulStoffregen/SoftwareSerial/archive/refs/heads/master.zip">SoftwareSerial.h</a>, <a href="https://github.com/PaulStoffregen/EEPROM/archive/refs/heads/master.zip">EEPROM.h</a>, <a href="https://github.com/pythonista/CyberLib/archive/refs/heads/master.zip">CyberLib.h</a>
  <li>Select the Arduino Nano card in the project settings
  <li>Copy the program code from src/main.cpp to the project file
  <li>Upload the program to the controller using the standard ArduinoIDE tool
</ul>
<h2 align=center>Description of the device operation</h2>
<p>
  &nbsp;The emergency security device is designed to monitor emergency situations and, in addition, security situations, and in the event of such situations, to notify and, if appropriate equipment is installed, to block them until the causes of the accident are identified and eliminated.
</p>
&nbsp;This device is able to track:
<ul>
  <li>leakage of natural gas and carbon monoxide,
  <li>the appearance of smoke and high temperature in the event of a fire,
  <li>water leakage with the corresponding blocking of the water supply,
  <li>vibrations (for example, walls), broken glass,
  <li>no voltage in the network with automatic transition to autonomous mode robots from the battery,
  <li>unauthorized entry into the premises,
  <li>movement in the room (subject to the installation of appropriate sensors).
</ul>
 &nbsp;The sensors of security zones 1 and 2 work in pairs according to the "Corridor" principle - a delay time is set for zone 1 and when entering through zone 1, zone 2 maintains a pause set for zone 1, after which an alarm occurs, and when entering through zone 2 bypassing zone 1, the alarm occurs instantly.
 <br/>
 &nbsp;Almost any sensors presented in this market segment are compatible with the offered equipment. Also, with the help of this device, you can remotely control any mechanisms and equipment using SMS messages, this can be lighting, irrigation system, heating system, air conditioning, emergency power supply, etc.
<p>&nbsp;
  After installing and configuring this equipment, the administrator's phone number and the backup phone number are recorded, after which the device operation parameters are set from the administrator's phone number with the appropriate commands.
  </p>
  <h3 align=center>Settings</h3>
<p>&nbsp;
  <b>Setting the Administrator number::</b> call to the number of the SIM card installed in the device, the device will respond with a sound and LED alarm, press the button on the device board until the call ends. The device will respond with a long beep and notify you by SMS about the acceptance of the admin number.
</p>
<p>&nbsp;
  <b>Setting the backup number:</b> send an SMS from the admin number with the backup phone number in the format "+380*********". The device will notify you by SMS about the acceptance of the backup number. There can be 4 more double numbers. To delete a number, send an SMS with the backup number to the device again.
</p>
&nbsp;<b>SMS commands for setting parameters:</b>
<ul>
  <li><b>«On 00»</b> - The arming delay time is set. Valid values are from 00 to 99 (seconds). Example: an SMS with the text "On 25" will set the arming delay for 25 seconds.
  <li><b>«Of 00»</b> - The time of the delay of working out for disarming is set.
  <li><b>«Bl 00»</b> - The threshold of the balance of funds on the account is set for notification. If you lower the balance below the set value, the device will notify you by SMS once a day. Valid values are from 00 to 99.
  <li><b>«*000#»</b> - The network operator's balance code is set. Example: <b> "*111#" </b> - Kyivstar.
  <li><b>«11»</b> - Switch on the controlled relay No. 1.
  <li><b>«10»</b> - Switch off the controlled relay No. 1.
  <li><b>«21»</b> - Switch on the controlled relay No. 2.
  <li><b>«20»</b> - Switch off the controlled relay No. 2.
</ul>
&nbsp;<b>This device has five operating modes::</b>
<ol>
  <li>All six zones are active                               {1.1.1.1.1.1}
  <li>Only security zones are active                       {1.1.1.0.0.0}
  <li>Only emergency zones are active                      {0.0.0.1.1.1}
  <li>Emergency zones and one security zone are active             {1.0.0.1.1.1}
  <li>Security zones and one emergency zone are active (fire)    {1.1.1.1.0.0}
</ol>
&nbsp;<b>Assigning zones:</b>
<ul>
  <li>Zone 1, 2, 3 - security
  <li>Zone 4 - fire, smoke
  <li>Zone 5 - gas leak, smoke
  <li>Zone 6 - water leakage
</ul>
  &nbsp;<b>Operating mode:</b>
<p>&nbsp;
  It is selected by sending an SMS from the phone number of the administrator or understudy to the device with the corresponding operating mode number. Example: an SMS with the text <b>"3"</b> will arm the device in mode 3, with further arming and disarming of the device from the keyboard or an empty call, the mode selected by SMS is automatically loaded, after which this mode is registered in memory until the next shift using SMS. To change the modes via SMS, you do not need to disarm the device, just send an SMS with the number of the desired mode of operation and the device will notify you of the change of mode of operation. To disarm the device via SMS, send an SMS with the text <b> "0"</b>to the device number. Also, arming and disarming is performed by an empty call to the device. Optionally, the device can be switched to control using remote controls instead of a keyboard.
</p>
&nbsp;<b>Light and sound alarm system:</b>
<ul>
  <li>three short beeps - no communication with the GSM module
  <li>one long beep - successful loading of the controller, mode change, confirmation of acceptance of the admin/backup number
  <li>repeated series of short beeps and lights - incoming call
</ul>
<p>&nbsp;
Video review of the device functionality on the project website <a href="https://www.robot.zp.ua/index.php/articles/7-avarijno-okhrannoe-gsm-ustrojstvo-s-udalenno-upravlyaemymi-rele">www.robot.zp.ua</a>
</p>