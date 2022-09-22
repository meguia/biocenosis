#include <esp_now.h>
#include <WiFi.h>
//78:21:84:7D:67:A8 ABAJO
//78:21:84:7F:1D:88 ARRIBAx


//uint8_t broadcastAddress[] = {0x78, 0x21, 0x84, 0x7D, 0x67, 0xA8}; // ABAJO
uint8_t broadcastAddress[] = {0x78, 0x21, 0x84, 0x7F, 0x1D, 0x88}; // ARRIBA
//uint8_t broadcastAddress[] = {0x24, 0x0A, 0xC4, 0x80, 0x5B, 0xAC}; // LIBRE
float cumstorm=0.0;
float threshold_storm=5.0;
float stormdecay=0.97;

typedef struct struct_message {
    uint8_t intensity; // intensity value between 0 and 100
    uint8_t distance; // distance in 10 x meters < 200
    uint8_t vchan[4];  // amplitude one byte per channel 0-100 each
    uint8_t spwidth; // spatial width number of channels between 1 and 16
    uint8_t spduration; // duration in seconds of the sparks up to 255
} struct_message;

byte message_blackout[2] = {1,0};

String success;
unsigned int time_interval = 400;
unsigned long previousMillis = 0; 
// Create a struct_message for sending not used 
struct_message lightning;

// Create a struct_message to hold incoming struct
struct_message incoming;

esp_now_peer_info_t peerInfo;

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incoming, incomingData, sizeof(incoming));
//  Serial.print("Bytes received: ");
//  Serial.println(len);
//    Serial.print(incoming.intensity);
//    Serial.print(",");
//  Serial.print(incoming.spwidth);
//  Serial.print(",");
//    Serial.print(incoming.spduration);
//    Serial.print(",");
//  Serial.println(incoming.distance);
  cumstorm += (float) incoming.spduration*(incoming.intensity+1.0)/100.0;
}



void setup() {
  Serial.begin(115200);
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  Serial2.begin(115200);
  
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  else {
    Serial.println("ESP-NOW started");
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

    // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  else {
    Serial.println("peer added");
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  //not sending
  // aca hacer algo con lo que recibimos cada cierto tiempo? 
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= time_interval) {
    previousMillis = currentMillis;
    cumstorm *= stormdecay;
    Serial.println(cumstorm);
    if (cumstorm>threshold_storm) {
      Serial2.write(1);
      cumstorm = 0.0;
    }
  }  
  
}
