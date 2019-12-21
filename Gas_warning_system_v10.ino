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
int sensor_value = 0; //Gas sensor value
String destination_number1 = "500000000"; //9-digits format destination number
String destination_number2 = "*124*#"; //USSD code (ORANGE, POLAND)
char message1[] = "Welcome. Gas system armed"; //Message sent after initialization
char message2[] = "Alarm - gas level exceed"; // Message sent after exceeding the gas set threshold
char message4[] = "Level"; //Message read out - Current gas level
char message5[] = "Halt"; //Message read out - System halt
char message6[] = "Start"; //Message read out - System start
char message8[] = "System halted"; //Message sent after system halting
char message9[] = "System restarted"; //Message sent after system restating
char message10[] = "Status"; //Message read out - Current date, time and gas leve
char message11[] = "Account"; //Message read out - Account balance
char date_AT[11]; //Date read from GSM network
char time_AT[9]; //Time read from GSM network
char readed_SMS[260]; //Readed SMS


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
  
  Serial1.println("AT"); //Check modem status
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
  Serial1.println("AT+CPMS?"); //Preferred SMS storage. Used memory
  while (modem_response() > 0);

  send_SMS(message1); //SMS as a test system
}


void loop() 
{
  switch(alarm_state)
  {
    //==================================================================
    //State 1 - Waiting
    case 1:    
    Serial.println("State 1 - Waiting");
    digitalWrite(pin_green_LED, 1);
    sensor_value = analogRead(pin_gas_sensor);//Read gas value
    delay(2000); //Measure every two second
    Serial.print("Gas value: ");
    Serial.println(sensor_value);
    
    if (sensor_value > gas_threshold)//Go to stage two after crossing the gas threshold.
      alarm_state = 2;

    if (receive_SMS()) //Checking if a new SMS is available
    {
      if (check_message(message5)) //Check incoming SMS for system halt command
      {
        alarm_state = 4;
        send_SMS(message8); //Send SMS informing about system halt
      }
      
      else if (check_message(message10)) //Check incoming SMS for system status command
      {
        get_time(); //Get time from GSM network
        delay(500);
        sensor_value = analogRead(pin_gas_sensor); //Read gas value
        char sensor_value_char[4];
        delay(200);
        char temp_array[80];
        sprintf(sensor_value_char, "%d", sensor_value); //Int to char conversion
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

      else if (check_message(message4)) //Check incoming SMS for gas level command
      {
        sensor_value = analogRead(pin_gas_sensor);//Read gas value
        char sensor_value_char[4];
        delay(200);
        char temp_array[20];
        sprintf(sensor_value_char, "%d", sensor_value); //Int to char conversion
        strcpy (temp_array, "Gas level: ");
        strcat (temp_array, sensor_value_char);
        Serial.println(temp_array);
        send_SMS(temp_array); //Send SMS informing about gas level
      }
      
      else if (check_message(message11)) //Check incoming SMS for accound status command
      {
        check_account(destination_number2);//Make call to USSD code
        if (receive_SMS()) //Checking if a new SMS is available
        {
          Serial.println(readed_SMS);
          send_SMS(readed_SMS);
        }
      }
      /*
      else //Send mismatched messages to destination_number1
      {
        Serial.println(readed_SMS);
        send_SMS(readed_SMS);
      }
      */    
    }
    break;


    //==================================================================
    //State 2 - Alarming
    case 2:  
    Serial.println("State 2 - Alarming");
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
    else//Go to stage three after reset pushed.
      alarm_state = 3;

    if (j == 0)
    {
      digitalWrite(pin_buzzer, 1);
      send_SMS(message2); //Send SMS informing about gas level exceed
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


    //==================================================================
    //State 3 - Reset
    case 3:  
    Serial.println("State 3 - Reset");
    j = 0;
    alarm_state = 1;
    break;


    //==================================================================
    //State 4 - System halt
    case 4:  
    Serial.println("State 4 - System halt");
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
  char CNMI_message[360];
  while (Serial1.available() > 0)
  {
    CNMI_message[k] = Serial1.read();
    //Serial.print(CNMI_message[k]);
    k++;
    CNMI_message[k] = '\0'; //Terminate string array
  }

  /*
   * Parsing CNMI message to obtain raw SMS.
   * Information about sender number and arrival date and time are discarded.
   */
   
  if (k > 0)
  {
    k = 0; //Looking for raw SMS start from CNMI message
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
    Serial.print("CNMI Message: ");
    Serial.println(CNMI_message);//CNMI Message
    Serial.print("Readed SMS: ");
    Serial.println(readed_SMS);//Raw SMS
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
  char CCLK_message[60]; //Array to gather answer from the network
  int k = 0;
  while(Serial1.available() > 0)
  {
    if (k < 59)
    {
      CCLK_message[k] = Serial1.read();//Saving answer to the array
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


void send_SMS(char a[]) //Sending SMS
{
  Serial1.print("AT+CMGS=\""); //Set destination number
  Serial1.print(destination_number1);
  Serial1.println("\"");
  delay(2000);
  Serial1.print(a); //SMS message to send
  Serial1.write(26); //Special "CTRL-Z" character to send SMS
  delay(5000);
  Serial.println("SMS sent");
  Serial1.println("AT+CMGDA=\"DEL ALL\""); //Delete All SMS
  delay(5000); //Time required to delete 1 message
}


bool modem_response() //Redirect GSM modem answers to computer Serial
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


void check_account(String code) //Making a call. Sending USSD code
{
  Serial1.print("ATD+ "); //AT command to dial number
  Serial1.print(code); //Number
  Serial1.println(';');//Modifier at the end separates the dial string into multiple dial commands
  delay(10000); //Wait for 10 seconds...
  //mySerial.println("ATH"); //hang up if necessary
}
