#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <UniversalTelegramBot.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <button.h>
#include <Wire.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_I2CDevice.h>
#include <my_chars.h>

#define USE_SETTINGS true

// Pis defines
#define WIRE_DATA 4
#define WIRE_CLOCK 5
#define LED 2
#define BUTTON_0 12
#define BUTTON_1 13

// Display defines
#define SCREEN_ADDRESS 0x3C


#if USE_SETTINGS
#include <settings.h>
#else
//Wifi settings
#define WIFI_SSID "My_SSID"
#define WIFI_PASSWORD "My_pass"

// Telegram settings
#define BOT_TOKEN "Bot_token"
#define CHAT_ID "Chat_ID"
#endif

// Display
#define COLUMS 20
#define ROWS   4
#define LCD_SPACE_SYMBOL 0x20  //space symbol from the LCD ROM, see p.9 of GDM2004D datasheet

uint8_t customChar[8] = {
	B00000,
	B01111,
	B00000,
	B00111,
	B00000,
	B00011,
	B00000,
	B00001};

LiquidCrystal_I2C lcd(PCF8574A_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);

// Button
uint16_t count = 0;
boolean click = false;


// Time
const uint32_t utcOffsetInSeconds = 3600 * 5;
const uint16_t ntpUpdateSeconds = 3600; 
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds, ntpUpdateSeconds);


// Sensor
OneWire oneWire(WIRE_DATA);
DallasTemperature sensors(&oneWire, 1);

// Telegram
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
uint64_t bot_lasttime; // last time messages' scan has been done
const uint64_t BOT_MTBS = 5000; // mean time between scan messages

void wifi_setup();
void telegram_setup();
void pins_setup();
void time_setup();
void sensor_setup();
void displaySetup();

void handleNewMessages(int numNewMessages);
void IRAM_ATTR handleKey();
void IRAM_ATTR handleKey_1();

void setup() {
	Serial.begin(115200);
	displaySetup();
	delay(1000);
	wifi_setup();
	delay(1000);
	telegram_setup();
	delay(1000);
	time_setup();
	delay(1000);
	pins_setup();
	delay(1000);
	sensor_setup();
	delay(1000);
	

	lcd.setCursor(0, 3);
	lcd.print(count);
	lcd.setCursor(0, 0);
	lcd.print(WiFi.localIP().toString() + " " + WIFI_SSID);
	lcd.setCursor(10, 2);
	lcd.print(WiFi.RSSI());
}

void loop() {
	if (millis() - bot_lasttime > BOT_MTBS){
		int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

		lcd.setCursor(10, 2);
		lcd.print(WiFi.RSSI());

		while (numNewMessages) {
			Serial.println(String(bot.messages->from_name)+"("+String(bot.messages->from_id)+"):\t"+String(bot.messages->text));
			
			lcd.clear();
			lcd.setCursor(0, 0);
			lcd.print(String(bot.messages->text));
			numNewMessages = bot.getUpdates(bot.last_message_received + 1);
			if (bot.messages->text == "/temp_home"){
				sensors.requestTemperatures();
				float tempC = sensors.getTempCByIndex(0);
				if(tempC != DEVICE_DISCONNECTED_C) {
					bot.sendMessage(CHAT_ID, String(tempC)+" C");
					lcd.setCursor(0, 2);
					lcd.print(String(tempC)+" C");
				}
				else 
					bot.sendMessage(CHAT_ID, "Error");
			}
			if (bot.messages->text == "/led_on"){
				digitalWrite(LED, 0);
				bot.sendMessage(CHAT_ID, "State LED ON");
			}
			if (bot.messages->text == "/led_off"){
				digitalWrite(LED, 1);
				bot.sendMessage(CHAT_ID, "State LED OFF");
			}
			if (bot.messages->text == "/led_state"){
				bot.sendMessage(CHAT_ID, "State LED " + String(!digitalRead(LED)));
			}
		}
		bot_lasttime = millis();
	}
}


void wifi_setup(){
	Serial.print("\nConnecting to Wifi SSID ");
	Serial.print(WIFI_SSID);
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("WiFi");
	// uint8_t lcdPointPosition = 4;
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	while (WiFi.status() != WL_CONNECTED){
		Serial.print(".");
		lcd.print(".");
		delay(1000);
	}
	Serial.print("\nWiFi connected. IP address: ");
	Serial.println(WiFi.localIP());
}

void telegram_setup(){
	secured_client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
	lcd.clear();
	lcd.setCursor(0, 0);
	if (bot.name) {
		lcd.println("Telegram OK");
		lcd.print("bot name: " + bot.name);
	}
	else lcd.println("Telegram ERROR");
}

void pins_setup(){
	pinMode(LED, OUTPUT);
	digitalWrite(LED, 1);
	pinMode(BUTTON_0, INPUT_PULLUP);
   	attachInterrupt(digitalPinToInterrupt(BUTTON_0), handleKey, RISING);
	pinMode(BUTTON_1, INPUT_PULLUP);
   	attachInterrupt(digitalPinToInterrupt(BUTTON_1), handleKey_1, RISING);
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Pins OK");
}

void time_setup(){
	timeClient.begin();
	lcd.clear();
	lcd.setCursor(0, 0);
	if (timeClient.update()) {
		lcd.println("Time OK");
		lcd.print(timeClient.getFormattedTime());
	}
	else lcd.println("Time ERROR");
}

void sensor_setup(){
	sensors.begin();
	sensors.setResolution(12);
	sensors.requestTemperatures();
	float tempC = sensors.getTempCByIndex(0);
	if(tempC != DEVICE_DISCONNECTED_C) {
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.println("Sensor OK");
		lcd.print(String(tempC)+" C");
	}
}

void displaySetup(){
	lcd.begin(COLUMS, ROWS, LCD_5x8DOTS, WIRE_DATA, WIRE_CLOCK);
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Display OK");
	lcd.createChar(0, WiFiLevel0);
	lcd.createChar(1, WiFiLevel1);
	lcd.createChar(2, WiFiLevel2);
	lcd.createChar(3, WiFiLevel3);
}

void handleNewMessages(int numNewMessages) {
	for (int i = 0; i < numNewMessages; i++) {
		bot.sendMessage(bot.messages[i].chat_id, bot.messages[i].text, "");
	}
}

void IRAM_ATTR handleKey(){
	detachInterrupt(BUTTON_0); 
	// click = true;
	count ++;
	lcd.setCursor(0, 3);
	lcd.print(count);
	attachInterrupt(digitalPinToInterrupt(BUTTON_0), handleKey, RISING);
}

void IRAM_ATTR handleKey_1(){
	detachInterrupt(BUTTON_1); 
	// click = true;
	count --;
	lcd.setCursor(0, 3);
	lcd.print(count);
	attachInterrupt(BUTTON_1, handleKey_1, RISING);
}

void IRAM_ATTR_22 handleKey_1(){
	detachInterrupt(BUTTON_1); 
	// click = true;
	count --;
	lcd.setCursor(0, 3);
	lcd.print(count);
	attachInterrupt(BUTTON_1, handleKey_1, RISING);
}