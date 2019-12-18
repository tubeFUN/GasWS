/*
 * Gas Warning System
 * https://github.com/tubeFUN/GasWS/blob/master/README.md
 */

  
#define pin_red_LED 9
#define pin_green_LED 10
#define pin_gas_sensor A0
#define pin_buzzer 11
#define pin_switch 8


int j = 0; //Auxiliary variable
int gas_threshold = 700; //Set gas threshold (min 0 - max 1023)
volatile int alarm_state = 1;
int sensor_value = 0;
String destination_number1 = "500000000"; //9-digits format
char message1[] = "Gas system armed"; //Message send after initialization
char message2[] = "Alarm - gas level exceed"; // Send after exceeding the gas set threshold
char message4[] = "Level"; //Allowed SMS - Current gas level
char message5[] = "Halt"; //Allowed SMS - System halt
char message6[] = "Start"; //Allowed SMS - System start
char message8[] = "System halted"; //Message sent after system halting
char message9[] = "System restarted"; //Message sent after system restating
char message10[] = "Status"; //Allowed SMS with current date, time and gas leve
char date_AT[11]; //Date read from GSM network
char time_AT[9]; //Time read from GSM network
char readed_SMS[160]; //Readed SMS


void setup() 
{
  pinMode(pin_red_LED, OUTPUT);
  pinMode(pin_green_LED, OUTPUT);
  pinMode(pin_gas_sensor, INPUT);
  pinMode(pin_buzzer, OUTPUT);
  pinMode(pin_switch, INPUT_PULLUP);

  digitalWrite(pin_red_LED, 0);
  digitalWrite(pin_green_LED, 0);
  digitalWrite(pin_buzzer, 0);
  
  Serial.begin(9600); //Communication with computer
  Serial1.begin(9600); //Communication with GSM modem
   
  Serial.println("Welcome to Gas Warning System monitor");
  Serial.println("Network connecting and warming sensor up...");

  Serial.print("Buffer size: ");
  Serial.println(Serial.availableForWrite());//Checking serials buffer size
  /*
   * To receive long SMS (160 characters) it is needed to extend serials buffer size.
   * To do this in location:
   * C:\Program Files (x86)\Arduino\hardware\arduino\avr\cores\arduino\HardwareSerial.h
   * change buffer size to:
   * #define SERIAL_TX_BUFFER_SIZE 254
   * #define SERIAL_RX_BUFFER_SIZE 254
   */
   
  delay(1000); //Time required to connect to the network and worming sensor up

  /* 
   *  Initializing and checking GSM modem
   *  To configure network time updating permanently to the modem
   *  it is needed to send AT+CLTS=1;&W
   *  then restart the modem, AT+CFUN=1,1
   */
  
  Serial1.println("AT");
  while (modem_response() > 0); 
  Serial1.println("AT+CMGF=1"); // Configuring SMS TEXT mode
  while (modem_response() > 0);
  Serial1.println("AT+CREG?"); //Registration status
  while (modem_response() > 0);
  Serial1.println("AT+COPS?"); //Operator name
  while (modem_response() > 0);
  Serial1.println("AT+CSQ"); //Signal strenght
  while (modem_response() > 0);
  Serial1.println("AT+CNMI=1,2,0,0,0"); // Forwarding SMS directly to the serial
  while (modem_response() > 0);

  send_SMS(message1); //SMS as a test system
}


void loop() 
{
  switch(alarm_state)
  {
    //111111111111111111111111111111111111111111111111111111111111111111
    case 1:    //State 1 - Waiting
    Serial.println("Case1");
    digitalWrite(pin_green_LED, 1);
    sensor_value = analogRead(pin_gas_sensor);
    delay(2000); //Measure every two second
    Serial.print("Gas value: ");
    Serial.println(sensor_value);
    
    if (sensor_value > gas_threshold)
      alarm_state = 2;

    if (receive_SMS()) //Checking if a new SMS is available
    {
      if (check_message(message5)) //Check incoming SMS for system halt command
      {
        alarm_state = 4;
        send_SMS(message8); //Send SMS informing about system halt
      }
      
      if (check_message(message10)) //Check incoming SMS for system status command
      {
        get_time();
        delay(500);
        sensor_value = analogRead(pin_gas_sensor);
        char sensor_value_char[4];
        delay(200);
        char temp_array[80];
        sprintf(sensor_value_char, "%d", sensor_value); //int to char conversion
        strcpy (temp_array, "Day: ");
        strcat (temp_array, date_AT);
        strcat (temp_array, "\n");
        strcat (temp_array, "Time: ");
        strcat (temp_array, time_AT);
        strcat (temp_array, "\n");
        strcat (temp_array, "Gas level: ");
        strcat (temp_array, sensor_value_char);
        Serial.println(temp_array);
        send_SMS(temp_array); //Send SMS status information
      }

      if (check_message(message4)) //Check incoming SMS for gas level command
      {
        sensor_value = analogRead(pin_gas_sensor);
        char sensor_value_char[4];
        delay(200);
        char temp_array[20];
        sprintf(sensor_value_char, "%d", sensor_value); //int to char conversion
        strcpy (temp_array, "Gas level: ");
        strcat (temp_array, sensor_value_char);
        Serial.println(temp_array);
        send_SMS(temp_array); //Send SMS informing about gas level
      }
    }
    break;


    //222222222222222222222222222222222222222222222222222222222222222222
    case 2:  //State 2 - Alarming
    Serial.println("Case2");
    sensor_value = analogRead(pin_gas_sensor);
    Serial.print("Gas value: ");
    Serial.println(sensor_value);
    if (reset_time() == 0)
    {
      digitalWrite(pin_green_LED, 0);
      digitalWrite(pin_red_LED, 1); //Set blinking LED
      digitalWrite(pin_buzzer, 1);
      delay(100);
      digitalWrite(pin_red_LED, 0);     //Set blinking LED
      if (digitalRead(pin_switch) == 0) //Continuous buzzer sound during
        digitalWrite(pin_buzzer, 1);    //button pressed
      else
        digitalWrite(pin_buzzer, 0);
      delay(100);
    }
    else
      alarm_state = 3;

    if (j == 0)
    {
      digitalWrite(pin_buzzer, 1);
      send_SMS(message2); //Sending only one SMS
      j++;
    }

    if (receive_SMS())
    {
      if (check_message(message5)) //Check incoming SMS for system halt command
      {
        alarm_state = 4;
        send_SMS(message8); //Send SMS informing about system halt
      }
    }
    break;


    //33333333333333333333333333333333333333333333333333333333333333333
    case 3:  //State 3 - Reset
    Serial.println("Case3");
    j = 0;
    alarm_state = 1;
    break;


    //44444444444444444444444444444444444444444444444444444444444444444
    case 4:  //State 4 - System halt
    Serial.println("Case4");
    digitalWrite(pin_red_LED, 1);
    digitalWrite(pin_green_LED, 0);
    digitalWrite(pin_buzzer, 0);
    delay(1000);
    if (receive_SMS()) //Checking if SMS is available
    {
      if (check_message(message6)) //Check incoming SMS for system start command
      {
        alarm_state = 1;
        send_SMS(message9); //Send SMS informing about system restart
      }
    }
    break;
  }
}



bool receive_SMS()
{
  int k = 0;
  char CNMI_message[160];
  while (Serial1.available() > 0)
  {
    CNMI_message[k] = Serial1.read();
    Serial.print(CNMI_message[k]);
    k++;
    CNMI_message[k] = '\0'; //Terminate string array
  }
  
  /*
   * Parsing CNMI message to obtain raw SMS.
   * Information about sender number and arrival date and time are discarded.
   */
   
  if (k > 0)
  {
    k = 0; //Looking for SMS start
    while (!(CNMI_message[k] == '"' && CNMI_message[k + 1] == '\r'))
    {
      k++;
    }
    int i = 0;                             //k + 2 is '\n'
    while (!(CNMI_message[k + 3] == '\r')) //Looking for end of SMS 
    {
      readed_SMS[i] = CNMI_message[k + 3];
      k++;
      i++;
    }
    readed_SMS[i] = '\0';
    Serial.println(readed_SMS);
    return 1;
  }
  return 0;
}
 

/*
 * Comparing readed SMS with declared message
 * If SMS is recognized, function returns 1, otherwise it returns 0.
 * For loop searches until it find NULL character in the declared message.
 */
bool check_message(char a[]) 
{
  for (int i=0; a[i] != '\0'; i++)
  {
    if (a[i] != readed_SMS[i])
    {
      return 0;
    }
  }
  return 1;
}


void get_time() //Getting the date and time from the network
{
  Serial1.println("AT+CCLK?");
  delay(1000); //Wait for answer from the network
  char CCLK_message[60]; //Table to gather answer from the network
  int k = 0;
  while(Serial1.available() > 0)
  {
    if (k < 59)
    {
      CCLK_message[k] = Serial1.read();
      k++;
      CCLK_message[k] = '\0'; //Terminate string array
    }
  }
  //Serial.print("Current date: ");
  date_AT[0] = CCLK_message[35];
  date_AT[1] = CCLK_message[36];
  date_AT[2] = '.';
  date_AT[3] = CCLK_message[32];
  date_AT[4] = CCLK_message[33];
  date_AT[5] = '.';
  date_AT[6] = '2';
  date_AT[7] = '0';
  date_AT[8] = CCLK_message[29];
  date_AT[9] = CCLK_message[30];
  date_AT[10] = '\0'; // Terminate the array
  //Serial.println(date_AT);

  //Serial.print("Current time: ");
  time_AT[0] = CCLK_message[38];
  time_AT[1] = CCLK_message[39];
  time_AT[2] = ':';
  time_AT[3] = CCLK_message[41];
  time_AT[4] = CCLK_message[42];
  time_AT[5] = ':';
  time_AT[6] = CCLK_message[44];
  time_AT[7] = CCLK_message[45];
  time_AT[8] = '\0'; // Terminate the array
  //Serial.println(time_AT);
}


void send_SMS(char a[]) //SMS sending
{
  Serial1.print("AT+CMGS=\""); //Set destination number
  Serial1.print(destination_number1);
  Serial1.println("\"");
  delay(2000);
  Serial1.print(a); //SMS message to send
  Serial1.write(26); //Special "CTRL-Z" character to send SMS
  delay(5000);
  Serial.println("SMS sent");
}


bool modem_response() //Redirect modem answers to computer Serial
{
  delay(2000);
  while (Serial1.available() > 0)
  {
    Serial.write(Serial1.read());
  }
  return 0;
}


bool reset_time() // Check the reset switch press time. 2 seconds reset the system
{
  int k = 0;
  while (digitalRead(pin_switch) == 0)
  {
    k++;
    delay(200);
    digitalWrite(pin_buzzer, 1);
    Serial.println(k);
    if (k>10) //Waits 2 seconds
    {
      digitalWrite(pin_buzzer, 0);
      return 1;
    }
  }
  digitalWrite(pin_buzzer, 0);
  return 0;
}
