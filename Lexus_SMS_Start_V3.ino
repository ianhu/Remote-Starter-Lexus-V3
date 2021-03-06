// Visual Micro is in vMicro>General>Tutorial Mode
// 
/*
Name:       Lexus_SMS_Start_V2.ino
Created:	30/08/2018 11:17:59
Author:     ian-PC\ian
*/

#include <LowPower.h>
#include <Blynk.h>
#include <SoftwareSerial.h>


// SIM900 Shield Serial Pins
SoftwareSerial SIM900(7, 8);

// Set to true if you want confirmation text messages
boolean respond = true;

// flag to make sure you can not start vehicle while it's already running

int runflag = 0;

// Key fob pins
int lock = 13;				// Remote Lock Button Rail
int unlock = 1;				// Remote Unlock Button Rail
int Trunk = 12;				// Trunk Button Rail




// define what Pin does what function


//int spare = 3;					// Spare Function
int brake_pedal_depressed = 4;  // Relay K1 - Controls when the Brake Pedal is pressed / depressed for starting the car
int starter = 5;                // Relay K2 - Signal used to engange the POWER button in the car
int parking_lights = 6;         // Relay K3 - Flashes the parking lights
int SIM900_POWER = 9;           // Power for GSM
int Key_relay = 2;              // Relay K4 Controls the operation of turning the Key power on and 
int headlights = 11;			// Connected to the Front Headlights


// Replace with the number of the controlling phone
String myPhoneNum = "+44xxxxxxxxxxx";


unsigned long carStartTime; // time the car was sent start command

void setup()
{

	// configure pins to connect to relays

	pinMode(lock, OUTPUT);
	pinMode(unlock, OUTPUT);
	pinMode(Trunk, OUTPUT);

	pinMode(brake_pedal_depressed, OUTPUT);
	pinMode(starter, OUTPUT);
	pinMode(parking_lights, OUTPUT);
	pinMode(Key_relay, OUTPUT);
	pinMode(headlights, OUTPUT);
	
	
	pinMode(SIM900_POWER, OUTPUT);


	digitalWrite(brake_pedal_depressed, HIGH);  // Initially set High/OFF
	digitalWrite(starter, HIGH);                // Initially set High/OFF
	digitalWrite(parking_lights, HIGH);         // Initially set High/OFF
	digitalWrite(Key_relay, HIGH);              // Initially set High/OFF
	digitalWrite(headlights, HIGH);             // Initially set High/OFF	
	digitalWrite(lock, HIGH);					// Initially set High/OFF	
	digitalWrite(unlock, HIGH);					// Initially set High/OFF	
//	digitalWrite(spare_0, HIGH;					// Initially set High/OFF	
//	digitalWrite(spare_1, HIGH);				// Initially set High/OFF	
	digitalWrite(Trunk, HIGH);					// Initially set High/OFF	
	delay(2000);



	// initialize serial communications and wait for port to open:
	digitalWrite(SIM900_POWER, LOW); // Power ON SIM900



	Serial.begin(9600); // for serial monitor
	Serial.println("Starting.");
	SIM900.begin(19200); // Serial for GSM shield
	SIM900poweron();  // turn on shield

	//Wake up modem with an AT command
	sendCommand("AT", "OK", 1000);
	if (sendCommand("AT", "OK", 1000) == 1) {
		Serial.println("Module started");
	}
	if (sendCommand("AT+CNMI=0,0,0,0,0", "OK", 1000) == 1) {
		Serial.println("SMS Delivery Reports - disabled");
	}
	if (sendCommand("AT+CMGF=1", "OK", 1000) == 1) {
		Serial.println("Text mode enabled");
	}
	if (sendCommand("AT+CSCLK=2", "OK", 1000) == 1) {
		Serial.println("Sleeping enabled");
	}

	//  sensors.begin();
}



void loop()
{
	//Wake up modem with two AT commands
	sendCommand("AT", "OK", 1000);
	sendCommand("AT", "OK", 2000);
	// Check if it's currently registered to Speakout
	if (sendCommand("AT+CREG?", "+CREG: 0,1", 1000)) {

		Serial.println("Connected to the home cell network");

		// Try to get the first SMS. Reply prefixed with +CMGR: if there's a new SMS
		if (sendCommand("AT+CMGR=1", "+CMGR: ", 1000) == 1) {
			String serialContent = "";
			char serialCharacter;

			while (SIM900.available()) {
				serialCharacter = SIM900.read();
				serialContent.concat(serialCharacter);
				delay(10);
			}

			// Dividing up the new SMS
			String smsNumber = serialContent.substring(14, 27);

			String smsMessage = serialContent.substring(55, serialContent.length() - 8); //53 to 55
			smsMessage.trim();
			smsMessage.toLowerCase();

			Serial.println("New SMS Message");
			Serial.println(smsNumber);
			Serial.println(smsMessage);

			// Delete all SMS messages in memory
			sendCommand("AT+CMGD=1,4", "OK", 1000);

			// Check if it's coming from my phone
			if (smsNumber == myPhoneNum)
			{
				if (smsMessage == "start") {
					if (runflag == 0) {
						carStart();
					}
					if (respond) { sendSms("LEXUS Command: Car Started"); }
				}
				else if (smsMessage == "stop") {
					if (runflag == 1) {
						carStop();
					}
					if (respond) { sendSms("LEXUS Command: Car Stopped"); }
				}
				else if (smsMessage == "lock") {
					carLock();
					if (respond) { sendSms("LEXUS Command: Doors Locked"); }
				}
				else if (smsMessage == "unlock") {
					carUnlock();
					if (respond) { sendSms("LEXUS Command: Doors Unlocked"); }
				}
				else if (smsMessage == "trunk") {
					carTrunk();
					if (respond) { sendSms("LEXUS Command: Trunk Popped"); }
				}
				else if (smsMessage == "finder") {
					if (respond) { sendSms("LEXUS Command: Finder Activated.\n Flashing the headlights");
					flash_headlights();
					 }
				}
				else if (smsMessage == "rspon") {
					respond = true;
					sendSms("Respond to commands: On");
				}
				else if (smsMessage == "rspoff") {
					respond = false;
					sendSms("Respond to commands: Off");
				}

				else if (smsMessage == "ping") {
					sendSms("Pong!");
				}
				else {
					if (respond) { sendSms("LEXUS Command: Invalid command"); }
					Serial.println("Invalid command");
				}
			}
		}
		else {
			Serial.println("No SMS messages");
		}
	}
	else {
		Serial.println("Not connected to home cell network");
	}


	delay(500);
	//	 LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  


	if (runflag == 1 && millis() > carStartTime + (9UL * 60UL * 1000UL)) {
		carStop();
		sendSms("LEXUS Command: Car Stopped time limit exceeded");
	}

}


void sendSms(String message)
{
	SIM900.print("AT+CMGF=1\r");
	delay(1000);
	SIM900.println("AT+CMGS=\"+44xxxxxxxxxx\"\r");
	delay(100);
	SIM900.println(message);
	delay(100);
	SIM900.println((char)26);
	delay(1000);
	SIM900.println();
	delay(5000);
}

void carLock() {
	
	  digitalWrite(lock, LOW);
	  delay(1000);
	  digitalWrite(lock, HIGH);
	  delay(1000);
	 Serial.println("Doors locked");
}

void carUnlock() {
		digitalWrite(unlock, HIGH);
		delay(1000);
		digitalWrite(unlock, LOW);
		delay(1000);
		Serial.println("Doors unlocked");
}


void carStart()
{
	Serial.println("Car Starting");

	digitalWrite(headlights, LOW);
	digitalWrite(Key_relay, LOW);
	delay(1000);

	runflag = 1;

	for (int x = 0; x <= 5; x++) {

		digitalWrite(parking_lights, LOW);
		delay(200);
		digitalWrite(parking_lights, HIGH);
		delay(200);
	}
	//Ensure Flasher lights are off
	digitalWrite(parking_lights, HIGH);
	//Headlights off
	digitalWrite(headlights, HIGH);


	delay(3000);
	digitalWrite(brake_pedal_depressed, 0);
	delay(3000);
	digitalWrite(starter, 0);
	delay(1200);
	digitalWrite(starter, 1);
	delay(1200);
	carStartTime = millis();

	//Ensure the brake pedal is off
	digitalWrite(brake_pedal_depressed, 1);

	//Send update SMS to inform car START request recieved and processed
	//sms.println("AT+CMGF=1");
	//            delay(1000);
	//             sms.println("AT+CMGS=\"+447738238350\"\r"); //Replace X with mobile number
	delay(1000);
	//             sms.println("Lexus-Start: The car is running"); // The SMS text to send.
	delay(1000);
	//             sms.println((char)26); //ASCII code to CTRL+Z (send)
	delay(1000);

}

void carStop()

{
	Serial.println("Car Stop Recieved");

	runflag = 0;


	//Press the START/STOP Button to turn off car
		
	digitalWrite(starter, 1);
	delay(2000);

	//Now Key power off
	digitalWrite(Key_relay, HIGH);
	delay(1000);


	//Send update SMS to inform car STOP request recieved and processed


}


void carTrunk() 
{
	 digitalWrite(Trunk, HIGH);
	 delay(2000);
	 digitalWrite(Trunk, LOW);
	 delay(3000);
	 digitalWrite(Trunk, HIGH);
		Serial.println("Trunk Released");
}

void flash_headlights() 
{

	// Used to locate car
	digitalWrite(headlights, LOW);
	delay(500);
	digitalWrite(headlights, HIGH);
	delay(500);
	digitalWrite(headlights, LOW);
	delay(500);
	digitalWrite(headlights, HIGH);
	delay(500);
	digitalWrite(headlights, LOW);
	delay(500);
	digitalWrite(headlights, HIGH);
	delay(500);
	digitalWrite(headlights, LOW);
	delay(3000);
	digitalWrite(headlights, HIGH);
	//  digitalWrite(starterPin, LOW);
	Serial.println("Car Finder Activated");
}


void SIM900poweron()
// Software equivalent of pressing the GSM shield "power" button
{
	//Wake up modem with an AT command
	sendCommand("AT", "OK", 1000);
	if (sendCommand("AT", "OK", 2000) == 0) {
		digitalWrite(9, HIGH);
		delay(1000);
		digitalWrite(9, LOW);
		delay(7000);
	}

}

int sendCommand(char* ATcommand, char* expected_answer, unsigned int timeout) {


	int answer = 0;
	int responsePos = 0;
	char response[100];
	unsigned long previous;

	memset(response, '\0', 100);    // Clears array

	delay(100);

	while (SIM900.available() > 0) SIM900.read();    // Clean the input buffer

	SIM900.println(ATcommand);    // Send the AT command 


	responsePos = 0;
	previous = millis();

	// this loop waits for the answer
	do {
		// if there are data in the UART input buffer, reads it and checks for the asnwer
		if (SIM900.available() != 0) {
			response[responsePos] = SIM900.read();
			responsePos++;
			// check if the desired answer is in the response of the module
			if (strstr(response, expected_answer) != NULL)
			{
				answer = 1;
			}
		}
		// Waits for the asnwer with time out
	} while ((answer == 0) && ((millis() - previous) < timeout));
	return answer;
}


/*
void sendSms(String message) {
Serial.println("Sending message");
char smsCommand[23];
String commandStr = "AT+CMGS=\""+myPhoneNum+"\r";
commandStr.toCharArray(smsCommand, 23);
Serial.println(message);

if (sendCommand(smsCommand, ">", 1000)) {

SIM900.println(message);
SIM900.println((char)26);
SIM900.println();
delay(2000);
}
}
*/
