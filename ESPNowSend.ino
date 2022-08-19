#include <esp_now.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <OneWire.h>
#include <DallasTemperature.h>

//wifi management branch new one -----   next update

//set Reciever MacAddress  94:B9:7E:D9:3B:98

uint8_t broadcastAddress[] = {0x94, 0xB9, 0x7E, 0xD9, 0x3B, 0x98};

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  char a[32]; //board name/device
  int b;  //int value
  float c;  //float value
  bool d;  //bool value
  float e;
  float f;
  float g;
} struct_message;

// Create a struct_message called myData
struct_message myData;

//Manage Wifi channels

constexpr char WIFI_SSID[] = "eir76484769";

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
    for (uint8_t i = 0; i < n; i++) {
      if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
        return WiFi.channel(i);
      }
    }
  }
  return 0;
}

esp_now_peer_info_t peerInfo;

//set up DSB1820 sensor
OneWire oneWire(4);
DallasTemperature sensors(&oneWire);

//Deep sleep parameters

#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  1200
RTC_DATA_ATTR int bootCount = 0;


void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {

  Serial.println("OnDataSent Fired");
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");


}


void setup() {
  Serial.begin(115200);
  delay(1000);
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  Serial.println("Sender node");
  WiFi.mode(WIFI_STA);

  //get channel of current network
  int32_t ssidChan = getWiFiChannel(WIFI_SSID);
  Serial.print("SSID Wifi chan: ");
  Serial.println(ssidChan);

  //set channel of this unit to same as current network channel
  // esp_wifi_set_channel(4, WIFI_SECOND_CHAN_NONE);
  WiFi.printDiag(Serial);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ssidChan, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial);

  //initiate ESP0-now
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initialisting ESP-NOW");
    return;
  }


  //set up call back for ESPnow after data send
  esp_now_register_send_cb(OnDataSent);

  //set up peer to link to reciever
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);

  //add peer
  esp_now_add_peer(&peerInfo);

  //send start up message via mqtt

  strcpy(myData.a, "Board_1_Startup");
  myData.b = 1;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));


  //start Sensors
  sensors.begin();
}

void loop() {

  sensors.requestTemperatures();
  Serial.print("Temp reading:");
  Serial.println(sensors.getTempCByIndex(0));

  strcpy(myData.a, "Board_1");
  myData.c = sensors.getTempCByIndex(0);

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }

  Serial.println("Going to sleep now");
  Serial.flush();
  esp_deep_sleep_start();
}