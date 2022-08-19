#include <esp_now.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// speeed management upgrade

//------------------------------------ESPNOW SET UP STUFF-----------------------------------
// set Reciever MacAddress  94:B9:7E:D9:3B:98

uint8_t broadcastAddress[] = {0x94, 0xB9, 0x7E, 0xD9, 0x3B, 0x98};

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message
{
  char a[32]; // board name/device
  int b;      // int value
  float c;    // float value
  bool d;     // bool value
  float e;
  float f;
  float g;
} struct_message;

// Create a struct_message called myData
struct_message myData;
esp_now_peer_info_t peerInfo;
int retry = 0;

//--------------------------------WIFI MANAGEMENT------------------------------

constexpr char WIFI_SSID[] = "eir76484769";

int32_t setWiFiChannel(const char *ssid)
{
  Serial.print("Get WifiChan hit");
  if (int32_t n = WiFi.scanNetworks())
  {
    for (uint8_t i = 0; i < n; i++)
    {
      Serial.println(WiFi.SSID(i));
      if (!strcmp(ssid, WiFi.SSID(i).c_str()))
      {
        Serial.println("-----------getWifiChan ---------------");
        WiFi.printDiag(Serial);
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_channel(WiFi.channel(i), WIFI_SECOND_CHAN_NONE);
        esp_wifi_set_promiscuous(false);
        WiFi.printDiag(Serial);
        return WiFi.channel(i);
      }
    }
  }
  return 0;
}

//--------------------------------SENSOR SET UP------------------------------
// set up DSB1820 sensor
OneWire oneWire(4);
DallasTemperature sensors(&oneWire);

//----------------------------DEEP SLEEP SET UP-----------------------------
// Deep sleep parameters

#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 5 //1200
RTC_DATA_ATTR int resetChan = 0;
int mStart, mEnd, mDiff = 0;

//-------------------------------ON DATA SENT ---------------------------------

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{

  Serial.println("OnDataSent Fired");
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");

  if (status != ESP_NOW_SEND_SUCCESS)
  {
    // set flag to reset wifi channel on next load set channel of current network
    resetChan = 1;
    esp_sleep_enable_timer_wakeup(5 * uS_TO_S_FACTOR);
    Serial.println("Going to sleep now for 5 seconds then will try again");
    Serial.flush();
    esp_deep_sleep_start();
  }
  else
  {
    mEnd = millis();
    mDiff = mEnd - mStart;
    Serial.print("Going to sleep now. Last Cycle took (ms)");
    Serial.println(mDiff);
    Serial.flush();
    esp_deep_sleep_start();
  }
}

void readAndSend()
{

  sensors.requestTemperatures();
  Serial.print("Temp reading:");
  Serial.println(sensors.getTempCByIndex(0));

  strcpy(myData.a, "Board_1");
  myData.c = sensors.getTempCByIndex(0);

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
}

void setup()
{
  mStart = millis();
  Serial.begin(115200);

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  Serial.println("Sender node");
  WiFi.mode(WIFI_STA);


  if (resetChan != 0)
  {
    int32_t ssidChan = setWiFiChannel(WIFI_SSID);
    resetChan = 0;
  }

  // initiate ESP0-now
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initialisting ESP-NOW");
    return;
  }

  // set up call back for ESPnow after data send
  esp_now_register_send_cb(OnDataSent);

  // set up peer to link to reciever
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);

  // add peer
  esp_now_add_peer(&peerInfo);

  // start Sensors
  sensors.begin();
}

void loop()
{
  readAndSend();
  delay(5000);
}
