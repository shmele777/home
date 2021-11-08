#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <settings.h>

// Wifi network station credentials
#ifndef WIFI_SSID
#define WIFI_SSID "My_SSID"
#endif 

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "My_pass"
#endif

#ifndef BOT_TOKEN
#define BOT_TOKEN "Bot_token"
#endif


#ifndef CHAT_ID
#define CHAT_ID "Chat_ID"
#endif


#define ONE_WIRE_BUS 4
#define LED 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

const unsigned long BOT_MTBS = 1000; // mean time between scan messages

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime; // last time messages' scan has been done

void handleNewMessages(int numNewMessages)
{
  for (int i = 0; i < numNewMessages; i++)
  {
    bot.sendMessage(bot.messages[i].chat_id, bot.messages[i].text, "");
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  // attempt to connect to Wifi network:
  Serial.print("Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  secured_client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
  
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  Serial.print("Retrieving time: ");
  configTime(0, 0, "pool.ntp.org"); // get UTC time via NTP
  time_t now = time(nullptr);
  while (now < 24 * 3600)
  {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 1);
}

void loop()
{
  if (millis() - bot_lasttime > BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages)
    {
      Serial.println(String(bot.messages->from_name)+"("+String(bot.messages->from_id)+"):\t"+String(bot.messages->text));
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      if (bot.messages->text == "/temp_home"){
        sensors.requestTemperatures();
        float tempC = sensors.getTempCByIndex(0);
        if(tempC != DEVICE_DISCONNECTED_C) bot.sendMessage(CHAT_ID, String(tempC)+" C");
        else bot.sendMessage(CHAT_ID, "Error");
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
