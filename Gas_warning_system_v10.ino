/*
 * Project no 1
 * Gas Warning System
 */

 
 
#define pin_red_LED 9
#define pin_green_LED 10
#define pin_gas_sensor A0
#define pin_buzzer 11
#define pin_SMS 12
#define pin_switch 8


int i = 0; //Auxiliary variable
int j = 0; //Auxiliary variable
int gas_threshold = 700; //Set gas threshold (min 0 - max 1023)
volatile int alarm_state = 1;
int sensor_value = 0;
String destination_number1 = "000000000";
char message1[] = "Gas system armed";
char message2[] = "Alarm - gas level exceed";
char message3[] = "Daily test";
char date_AT[11];
char time_AT[9];
char send_time[8] = "22:44:0"; //Set time to send daily SMS

void setup() 
{
  pinMode(pin_red_LED, OUTPUT);
  pinMode(pin_green_LED, OUTPUT);
  pinMode(pin_gas_sensor, INPUT);
  pinMode(pin_buzzer, OUTPUT);
  pinMode(pin_SMS, OUTPUT);
  pinMode(pin_switch, INPUT_PULLUP);

  digitalWrite(pin_red_LED, 0);
  digitalWrite(pin_green_LED, 0);
  digitalWrite(pin_buzzer, 0);
  digitalWrite(pin_SMS, 0);

  Serial.begin(9600); //Computer communication
  Serial1.begin(9600); //GSM modem communication
  
  delay(1000); //Time to connect to the network

  //connection_OK dodac
  send_SMS(message1); //SMS as a test system (13 seconds takes)
  
  //delay(1000); //Worming sensor up
}

void loop() 
{
  switch(alarm_state)
  {
    case 1:    //Waiting
    Serial.println("Case1");
    digitalWrite(pin_green_LED, 1);
    sensor_value = analogRead(pin_gas_sensor);
    delay(500); //Measure every half second
    Serial.print("Gas value: ");
    Serial.println(sensor_value);
    
    if (sensor_value > gas_threshold)
      alarm_state = 2;
      
    get_time();
    if (check_time()) //Check time for sending daily SMS
      send_SMS(message3);
      
    if (received_SMS == '0') //Check incoming SMS for system halt command 
    alarm_state = 4;
    break;


    case 2:  //Alarming
    Serial.println("Case2");
    sensor_value = analogRead(pin_gas_sensor);
    Serial.print("Gas value: ");
    Serial.println(sensor_value);
    if (reset_time() == 0)
    {
      digitalWrite(pin_green_LED, 0);
      digitalWrite(pin_red_LED, 1); //LED blinking
      digitalWrite(pin_buzzer, 1);
      delay(100);
      digitalWrite(pin_red_LED, 0);
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
      send_SMS(message2); //Send SMS only one time
      j++;
    }

    if (received_SMS == 0)//Check incoming SMS for system halt command 
      alarm_state = 4;
    break;


    case 3:  //Reset
    Serial.println("Case3");
    i = 0;
    j = 0;
    alarm_state = 1;
    break;

    case 4:  //Halt system
    digitalWrite(pin_red_LED, 1);
    digitalWrite(pin_green_LED, 0);
    digitalWrite(pin_buzzer, 0);
    while(1);
    break;
  }
}


char received_SMS()
{
  return 1;
}


bool check_time()
{
  int k = 0;
  while (k < 7)
  {
    if (time_AT[k] == send_time[k])
      k++;
    else
      return 0;
  }
  return 1;
}


void get_time()
{
  Serial1.println("AT+CCLK?");
  delay(1000);
  char readed_data[60];
  int k = 0;
  while(Serial1.available() > 0)
  {
    readed_data[k] = Serial1.read();
    k++;
  }

  Serial.print("Current date: ");
  date_AT[0] = readed_data[35];
  date_AT[1] = readed_data[36];
  date_AT[2] = '.';
  date_AT[3] = readed_data[32];
  date_AT[4] = readed_data[33];
  date_AT[5] = '.';
  date_AT[6] = '2';
  date_AT[7] = '0';
  date_AT[8] = readed_data[29];
  date_AT[9] = readed_data[30];
  date_AT[10] = '\0'; // terminate the array
  Serial.println(date_AT);

  Serial.print("Current time: ");
  time_AT[0] = readed_data[38];
  time_AT[1] = readed_data[39];
  time_AT[2] = ':';
  time_AT[3] = readed_data[41];
  time_AT[4] = readed_data[42];
  time_AT[5] = ':';
  time_AT[6] = readed_data[44];
  time_AT[7] = readed_data[45];
  time_AT[8] = '\0'; // terminate the array
  Serial.println(time_AT);
}


void send_SMS(char a[]) //SMS sending
{
  Serial1.println("AT");
  delay(2000);
  while (modem_response() > 0);
  Serial1.println("AT+CMGF=1"); //It is needed to send AT+CLTS=1;&W
  delay(2000);                  //Then restart modem, AT+CFUN=1,1
  while (modem_response() > 0);
  /*
  Serial1.print("AT+CMGS=\"");
  Serial1.print(destination_number1);
  Serial1.println("\"");
  delay(2000);
  Serial1.print(a);
  Serial1.write(26); //special "CTRL-Z" character
  delay(5000);
  Serial.println("SMS sent");
  */
  Serial1.println("AT");
  delay(2000);
  while (modem_response() > 0);
  Serial1.println("AT+CREG?"); //Registration status
  delay(2000);
  while (modem_response() > 0);
  Serial1.println("AT+COPS?"); //Operator name
  delay(2000);
  while (modem_response() > 0);
  Serial1.println("AT+CSQ"); //Signal strenght
  delay(2000);
  while (modem_response() > 0);
}


bool modem_response()
{
  while (Serial1.available() > 0)
  {
    Serial.write(Serial1.read());
  }
  return Serial1.available();
}



bool reset_time() // Check the switch press time
{
  while (digitalRead(pin_switch) == 0)
  {
    i++;
    delay(200);
    digitalWrite(pin_buzzer, 1);
    Serial.println(i);
    if (i>10) //Waits 2 seconds
    {
      digitalWrite(pin_buzzer, 0);
      return 1;
    }
  }
  digitalWrite(pin_buzzer, 0);
  i = 0;
  return 0;
}
