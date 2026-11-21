#include <ESP8266WiFi.h>
#include <DataStore.h>
#include <TaskSync.h>



const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";
SdFat sdFat;



void setup()
{
  delay(1000);
  
  Serial.begin(115200);
  Serial.println("");
  Serial.println(F("Test_TaskSync"));

  WiFi.mode(WIFI_OFF);
  delay(1000);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print(F("Connecting"));
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print(F("Connected to "));
  Serial.println(ssid);
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());

  timeKeeper.begin();

  if (!sdFat.begin(16, SPI_HALF_SPEED))
    sdFat.errorHalt("sd.begin");

  dataStore.begin();
  new TaskSync();
}

void loop()
{
  updateManager.UpdateAll();
}
