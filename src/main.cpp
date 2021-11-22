#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <OneWire.h>
#include <DallasTemperature.h>
// #include <Encoder.h>
#include <button.h>
#include <Wire.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_I2CDevice.h>

#define USE_SETTINGS true

// Pis defines
#define ONE_WIRE_BUS 4
#define LED 2
#define BUTTON_0 12
#define BUTTON_1 13

// Display defines
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32


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

// Display SSD1306
#define COLUMS 20
#define ROWS   4
#define LCD_SPACE_SYMBOL 0x20  //space symbol from the LCD ROM, see p.9 of GDM2004D datasheet

LiquidCrystal_I2C lcd(PCF8574A_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);

// Encoder
// Encoder encoder(ENC_PIN_1, ENC_PIN_2);
// Button button(ENC_BUTTON);
// int16_t oldPosition = -999;

// Button
uint16_t count = 0;
boolean click = false;

// Sensor
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire, 1);

// Telegram
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
uint64_t bot_lasttime; // last time messages' scan has been done
const uint64_t BOT_MTBS = 1000; // mean time between scan messages


void wifi_setup();
void telegram_setup();
void pins_setup();
void time_setup();
void sensor_setup();
void displaySetup();

void handleNewMessages(int numNewMessages);
void ICACHE_RAM_ATTR handleKey();
void ICACHE_RAM_ATTR handleKey_1();

void setup() {
	Serial.begin(115200);
	wifi_setup();
	telegram_setup();
	time_setup();
	pins_setup();
	sensor_setup();
	displaySetup();

	lcd.setCursor(0, 3);
	lcd.print(count);
	lcd.setCursor(0, 0);
	lcd.print(WiFi.localIP().toString() + " " + String(WIFI_SSID));
	lcd.setCursor(10, 2);
	lcd.print(WiFi.RSSI());
}

void loop() {
	// int16_t newPosition = encoder.read();
	// if (newPosition != oldPosition){
	// 	oldPosition = newPosition;
	// 	Serial.print("Position encoder: ");
	// 	Serial.println(newPosition);
	// }

	// if (button.click() == true) {
	// 	// encoder.write(0);
	// 	oldPosition ++;
	// 	Serial.println("button: ");
	// 	Serial.println(oldPosition);

	// }

	if (millis() - bot_lasttime > BOT_MTBS){
		int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

		while (numNewMessages) {
			Serial.println(String(bot.messages->from_name)+"("+String(bot.messages->from_id)+"):\t"+String(bot.messages->text));
			
			lcd.setCursor(0, 1);
			lcd.print("                   ");
			lcd.setCursor(0, 1);
			lcd.print(String(bot.messages->text));
			lcd.setCursor(10, 2);
			lcd.print(WiFi.RSSI());
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
	// if (click == true) {
	// 	lcd.setCursor(0, 2);
	// 	lcd.print(count);
	// }
}


void wifi_setup(){
	Serial.print("\nConnecting to Wifi SSID ");
	Serial.print(WIFI_SSID);
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	while (WiFi.status() != WL_CONNECTED){
		Serial.print(".");
		delay(500);
	}
	Serial.print("\nWiFi connected. IP address: ");
	Serial.println(WiFi.localIP());
}

void telegram_setup(){
	secured_client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
}

void pins_setup(){
	pinMode(LED, OUTPUT);
	digitalWrite(LED, 1);
	pinMode(BUTTON_0, INPUT_PULLUP);
   	attachInterrupt(digitalPinToInterrupt(BUTTON_0), handleKey, RISING);
	pinMode(BUTTON_1, INPUT_PULLUP);
   	attachInterrupt(digitalPinToInterrupt(BUTTON_1), handleKey_1, RISING);
}

void time_setup(){
	Serial.print("Retrieving time: ");
	configTime(0, 0, "pool.ntp.org"); // get UTC time via NTP
	time_t now = time(nullptr);
	while (now < 24 * 3600) {
		Serial.print(".");
		delay(100);
		now = time(nullptr);
	}
	Serial.println(now);
}

void sensor_setup(){
	sensors.begin();
	sensors.setResolution(12);
}

void displaySetup(){
	lcd.begin(COLUMS, ROWS, LCD_5x8DOTS, 4, 5);
	lcd.print(F("Display OK..."));
}

void handleNewMessages(int numNewMessages) {
	for (int i = 0; i < numNewMessages; i++) {
		bot.sendMessage(bot.messages[i].chat_id, bot.messages[i].text, "");
	}
}

void ICACHE_RAM_ATTR handleKey(){
	detachInterrupt(BUTTON_0); 
	// click = true;
	count ++;
	lcd.setCursor(0, 3);
	lcd.print(count);
	attachInterrupt(digitalPinToInterrupt(BUTTON_0), handleKey, RISING);
}

void ICACHE_RAM_ATTR handleKey_1(){
	detachInterrupt(BUTTON_1); 
	// click = true;
	count --;
	lcd.setCursor(0, 3);
	lcd.print(count);
	attachInterrupt(digitalPinToInterrupt(BUTTON_1), handleKey_1, RISING);
}