#include <Arduino.h>
#include <WiFi.h> 
#include <BluetoothSerial.h>

#include<esp_gap_bt_api.h>

//////////////declaratii variabile si obiecte
const int L = 40;
const char* SSID = "SSIDretea";
const char* PAROLA = "parolaretea";
const char* hostname_nou = "Echipa_x_y"; 
byte RSSIBluetooth; 
//x se va înlocui cu prefixul echipei
//y se va înlocui cu numărul echipei

byte addr[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; //initializare adresa MAC BT clasic
BluetoothSerial ESP_BT;
struct SSID_RSSI_MAXIM {int nr_retele; 
char* ssid_maxim[L];
};

////////////////functii Wi-Fi
SSID_RSSI_MAXIM scanare_retele() //functie pentru scanarea retelelor Wi-Fi disponibile in raza de acoperire a modulului Wi-Fi
{WiFi.disconnect(); //deconectare de la orice alta retea
int maxRSSI = -130;
int nr_retele_open = 0;
char ssid_nivel_maxim[L];
int n = WiFi.scanNetworks();
if (n==0)
  Serial.print("Nu exista retele disponibile");
else
  for (int i = 0; i < n; ++i) {
      Serial.print("Nr. crt.\tSSID\tRSSI\ttip autentificare"); 
      Serial.print(i + 1);
      Serial.print("\t");
      Serial.print(WiFi.SSID(i));
      Serial.print("\t");
      Serial.print(WiFi.RSSI(i));
      Serial.print("\t");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
      delay(10);
      if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) // daca reteaua nu este securizata prin parola
        {nr_retele_open++;
        if (WiFi.RSSI(i)>=maxRSSI)//se cauta reteaua open cu cel mai bun RSSI
          {strcpy(ssid_nivel_maxim, WiFi.SSID(i).c_str()); //preluare SSID a retelei cu RSSI maxim
          maxRSSI = WiFi.RSSI(i);}
        }
    }
    return {nr_retele_open, ssid_nivel_maxim};
}

void initWiFi_open(char ssid[L]) //functie pentru initializarea comunicatiei Wi-Fi si conectarea la o retea securizata
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid);
  Serial.print("Conectare la rețeaua Wi-Fi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}


void initWiFi() //functie pentru initializarea comunicatiei Wi-Fi si conectarea la o retea securizata
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PAROLA);
  Serial.print("Conectare la rețeaua Wi-Fi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void verificare_status()
{
unsigned long mom_antMili = 0;
unsigned long interval = 30000; //30 de secunde
unsigned long mom_curentMili = millis();
// daca s-a pierdut conexiunea
if ((WiFi.status() != WL_CONNECTED) && (mom_curentMili - mom_antMili >=interval)) {
  Serial.print(millis());
  Serial.println("Reconectare la reteaua Wi-Fi...");
  WiFi.disconnect();
  WiFi.reconnect();
  mom_antMili = mom_curentMili;
  }

}

/////////functii si structuri Bluetooth

void gap_callback (esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
  if (event == ESP_BT_GAP_READ_RSSI_DELTA_EVT) RSSIBluetooth = param->read_rssi_delta.rssi_delta;
}

//SPP service callback function (to get remote MAC address)
void spp_callback (esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
  if (event == ESP_SPP_SRV_OPEN_EVT) memcpy(addr, param->srv_open.rem_bda, 6);
}
////////////functia setup
void setup() {
  char ssid_max[40];
  Serial.begin(115200); //monitor serial pentru depanare
  ESP_BT.begin(hostname_nou); // initializare Bluetooth

  esp_bt_gap_register_callback (gap_callback);
  ESP_BT.register_callback(spp_callback); //serviciu ssp, preia adresa MAC

  WiFi.mode(WIFI_STA); //modulul de comunicatii Wi-Fi functioneaza in modul statie
  //WIFI_AP pentru punct de acces
  //WIFI_STA_AP pentru modul hibrid
  WiFi.setHostname(hostname_nou); //setare nume nou pentru host
  SSID_RSSI_MAXIM struct_scanare = scanare_retele();  // declarare tip structura
  strcpy(ssid_max, struct_scanare.ssid_maxim[L]);//apelare functie scanare retele si preluare SSID al retelei cu RSSI maxim
  int nr_retele_open = struct_scanare.nr_retele;
  if (nr_retele_open==0) //daca avem retele deschise, ne conectam la cea mai puternica
    initWiFi_open(ssid_max);
  else // altfel, folosim credentialele unei retele securizate (trebuie sa fie in raza de acoperire)
    initWiFi();
  
}

void loop() {
 verificare_status();
 char buf[50];
 int RSSIWiFi = WiFi.RSSI();
 if (ESP_BT.hasClient()) {
    esp_bt_gap_read_rssi_delta (addr);
    sprintf(buf, "RSSI Wi-FI: %d, RSSI Bluetooth: %d", RSSIWiFi, RSSIBluetooth);
    ESP_BT.println(buf);}
  else
  {sprintf(buf, "RSSI Wi-FI: %d; Dispozitiv Bluetooth neconectat", RSSIWiFi);
  ESP_BT.println(buf);}
}