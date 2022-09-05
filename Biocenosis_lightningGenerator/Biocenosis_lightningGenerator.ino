#include "PCA9685.h"
#include <esp_now.h>
#include <WiFi.h>
//78:21:84:7D:67:A8 ABAJO
//78:21:84:7F:1D:88 ARRIBA

uint8_t broadcastAddress[] = {0x78, 0x21, 0x84, 0x7D, 0x67, 0xA8}; // ABAJO
#define NCHAN 16
#define NSPARKS 30

TwoWire *wi0;

PCA9685 ledArray(0x40);
//const PI=3.141592; already defined
// CICLOS
// periodo del ciclo global aprox 5-10 minutos pero arrancamos probando con 3 minutos
unsigned long mainperiod=10*60*1000;
// peridos de cada una de las probabilidades como armonicos 3 4 y 5 del periodo principal
unsigned long pevent = (long) mainperiod/3;
unsigned long pdistance = (long) mainperiod/4;
unsigned long pintensity = (long) mainperiod/5;
// intervalo medio entre rayos meanevent y amplitud de modulacion 
float meanevent = 5000.0;
float modevent = -3000.0;
float tevent; // el parametro modulado
// distancia media de la tormenta y amplitud de modulacion
float meandistance = 500.0;
float moddistance = -500.0; //mean + modd < 2000
float tdistance;
// intensidad media de la tormenta y amplitud de modulacion
float meanintensity = 0.95;
float modintensity = 0.05;
float tintensity;
//luego para cada rayo se calcula
// intensity intensidad particular tomada de una distribucion con valor medio igual a la intensidad de la tormenta
// distance distancia idem
// bright se calcula como (int) 4095*intensity/sqrt(distance+1)
// de tal forma que para distance=0 da la intensidad maxima 4095, y para distance = 2000 da intensidad maxima 100 
// la ubicacion se calcula a posteriori
unsigned long currentMillis;
unsigned long lastevent=0;
unsigned long lastspark=0;
unsigned long sparkinterval = 5; // parametro a ajustar -> duracion de la chispa
long bright;
float refractory = 1000; // tiempo refractario durante el cual no se produce otro rayo
float nextevent = 0.0; // tiempo al siguiente evento
float meaninter = 50.0; // parametro a ajustar -> Intervalo medio entre chispas
float mdecay = 100.0; // parametro a ajustar -> tiempo de decaimiento de intensidad de las chispas
float sparktime[NSPARKS]; // tiempo de ocurrencia de las chispas
float sparkamplitude[NSPARKS]; // intensidad de las chispas
int kount;
bool eventon = false;
bool sparkon = false;
bool monitorspark = false;
bool monitorevent = true;
bool monitormodulation=true;

typedef struct struct_message {
    uint8_t intensity; // intensity value between 0 and 100
    uint8_t distance; // distance in 10 x meters < 200
    uint8_t vchan[4];  // amplitude one byte per channel 0-100 each
    uint8_t spwidth; // spatial width number of channels between 1 and 16
    uint8_t spduration; // duration in seconds of the sparks up to 250
} struct_message;
byte startmarker = 255; // start of message 

// Crea struct_message para enviar a teensy y a esp32 de abajo
struct_message lightning;
// para el caso eventual de que reciba parametros de abajo
// hay que definir otra estructura por ahora usamos la misma
struct_message incoming;

esp_now_peer_info_t peerInfo;

String success;

int spcon [] PROGMEM = {
#include "spcon.h"
}; //spark connectivity between channels 16x16

int spini [] PROGMEM = {
#include "spini.h"
}; //spark initiation 16x1

// cargar un .h con la influencia de cada canal de uz en cada canal de audio
int wdir [] PROGMEM = {
#include "wdir.h"
}; //weight of each channel on audio direction 16x4 

int ruleta_ini[NCHAN];
int ruleta[NCHAN*NCHAN]; 
int spsum[NCHAN];
float chanE[NCHAN]; // energia de cada canal
float Etot; // energia todal de la chispa
int Echan; // cantidad de canales con energia > 0.1
uint8_t vc[4]; //peso de la chispa en cada canal de audio

void setup(void)
{

  randomSeed(analogRead(0));
  // inicializa serial USB
  Serial.begin(115200);
  while(!Serial);
  // inicializa Serial comunicacion con Teensy
  Serial2.begin(115200); // RTX2 TX2
  while(!Serial2);
  
  // inicializa I2C
  Wire.begin();
  // Notar que invertimos
  wi0 = &Wire;

  delay(1000);
  
  ledArray.set_wire(wi0);
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Inicia ESP-NOW
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

  nextTime(.5);
  // ruleta inicial 
  cumsum(spini,ruleta_ini,NCHAN); 
  // ruleta por fila se calcula solo una vez
  cumsum_byrow(spcon,ruleta,NCHAN,NCHAN); 
  // suma de cada fila de la matriz de conectividad se calcula 1 vez
  sumrow(spcon,spsum,NCHAN,NCHAN);
  // tiempo del primer evento 
  tevent = meanevent + modevent;
  nextevent = refractory + nextTime(1.0/tevent);
  Serial.print("Struct SIZE : ");
  Serial.println(sizeof(lightning));
  if (monitorevent) {
    Serial.println("Tinterval,Int,Dist,CH1,CH2,CH3,CH4,Edecay,Tdecay"); 
  }
}

void loop(void)
{
  currentMillis = millis();
  if (currentMillis - lastevent >= nextevent) {
    //se produce un rayo precalculo las chispas
    tintensity = meanintensity + modintensity*cos(2*PI*(float)currentMillis/pintensity);  // entre 0.2 y 1.0
    tdistance = meandistance + moddistance*cos(2*PI*(float)currentMillis/pdistance); // entre 0 y 2km
    
    float intensity = tintensity*random(80,120)/100.0;  // una forma trucha de poner una variacion entre el 80 y el 120 porciento del valor medio
    float distance = tdistance*random(60,130)/100.0;
    bright = (int) (4095*intensity); //  brillo en el cielo depende de intensity e inversamente prop a distancia (sqrt?)
    long Edecay;
    if (distance<800) {
      Edecay = random(20,30); // si es cercano mantener en el rango 40-60
    }
    else {
      Edecay = random(30,50);
    }
    long Tdecay;
    if (intensity>0.8) {
      Tdecay= random(350,650);
    }
    else if (intensity>0.6 && intensity <= 0.8) {
      Tdecay= random(200,350);
    }
    else {
      Tdecay= random(100,200);
    }
    // depende de la intensidad. para alta intensidad rango 200-300
    // para baja intensidad 20 - 40. Estaba 80,150, probe 200,300, ahora,mas. 300,400
    // setup con mas intensidad max Edecay 20-30 y Tdecay 300-500. mas cortos de 50 a 100
    generate_event(Tdecay, Edecay);
    //calcular direccion
    for (int j=0;j<=4;j++) {
      float dirchan = 0.0;
      for (int i=0;i<=NCHAN;i++) {
        dirchan += chanE[i]*wdir[4*i+j]/(4.0*Etot);
      }
      vc[j] = (uint8_t) 100*dirchan;  
    }
    //armamos el struct de datos
    lightning.intensity = (uint8_t) bright/40; //between 0 - 100 
    lightning.distance = (uint16_t) distance/10;
    for (int i=0;i<4;i++) { 
      lightning.vchan[i] = vc[i];
    }    
    lightning.spwidth = (uint8_t) Echan; 
    lightning.spduration = (uint8_t) Tdecay/4; 
    
    //manda info a la teensy
    // mandamos intensity, distance y dirchan
    Serial2.write(startmarker);
    Serial2.write((byte*)&lightning, sizeof(lightning));
    //mandar info a la esp32 de abajo por wifi
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &lightning, sizeof(lightning));
    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }

    
    eventon = true;
    kount = 0 ;
    if (monitorevent) {
      Serial.print(nextevent);
      Serial.print(",");
      Serial.print(intensity);
      Serial.print(",");
      Serial.print(distance);
      Serial.print(",");
      Serial.print(bright);
      Serial.print(",");
      Serial.print(lightning.intensity);
      Serial.print(",");
      Serial.print(lightning.distance);
      Serial.print(",");
      for (int j=0;j<4;j++) {
        Serial.print(vc[j]);
        Serial.print(",");
      }
      Serial.print(Edecay);
      Serial.print(",");
      Serial.println(Tdecay);
    }
    lastevent = currentMillis;
    lastspark = currentMillis;
    nextevent = refractory + nextTime(1.0/meanevent);
  }
  if (eventon) {
    if (kount<NSPARKS) {
      if (currentMillis - lastspark >= sparktime[kount]) {
        // se produce chispa prendemos la chispa
        //Serial.print(sparktime[kount]);
        //Serial.print(",");
        //Serial.println(sparkamplitude[kount]);
        sparkon = true;
        // mandar al LED sparkamplitude[kount]
        if (monitorspark) { 
          Serial.print(currentMillis-lastevent);
          Serial.print(" > ");
        }  
        for (int i=0; i<NCHAN; i++) {
          int value = (int) bright*chanE[i]*sparkamplitude[kount];
          value = value <= 4095 ? value : 4095;
          ledArray.setPWM(i, 0, value);
          if (monitorspark) {
            Serial.print(value);
            Serial.print(",");
          }
        }  
        if (monitorspark) {
          Serial.println();
        }
        kount++;
        lastspark = currentMillis;
      }
       if (sparkon && (currentMillis - lastspark >= sparkinterval)) {
        // apagamos la chispa
        //mandar al LED 0
        for (int i=0; i<NCHAN; i++) {
          ledArray.setPWM(i, 0, 0);
        } 
        sparkon = false;
       }
       // lo que sigue es solo para monitorear en tiempo real comentar
       //if (sparkon) {
       // Serial.println(sparkamplitude[kount-1]);
       //}
       //else {
       // Serial.println(0.0);
       //}
       // comentar hasta aca 
    }
    else {
      // termina evento
      for (int i=0; i<NCHAN; i++) {
          ledArray.setPWM(i, 0, 0);
      } // por si quedo prendido al final 
      eventon = false;  
    }
  }
  else {
    //aca solo actuaiza el valor de la tormenta, buscar de hacerlo afuera despues
    tevent = meanevent + modevent*cos(2*PI*(float)currentMillis/pevent);
  }
  delay(2);
}

void generate_event(long Tdecay, long Edecay) {
  float cumtime=0.0;
  int chini, lastchan, index, dice;
  // aca determinar el recorrido del rayo
  dice = random(ruleta_ini[NCHAN-1]);
  for (chini=0; dice > ruleta_ini[chini]; chini++){} 
  // resetea a cero la energia de cada canal
  for (int i=0; i<NCHAN; i++) {
    chanE[i]=0.0;
  }
  // Energia inicial
  float E = 1.0; // en el programa viene de afuera
  chanE[chini] = E; // chint es el resultado final la intensidad de cada canal
  lastchan = chini;
  while (E>0.01) {
    // 
    //float dE = min((float)spsum[lastchan]/Edecay,1.0); 
    //float dE = (float)spsum[lastchan]/Edecay; <= 1.0 ? (float)spsum[lastchan]/Edecay : 1.0; 
    float dE = (float)spsum[lastchan]/Edecay; 
    dE = dE <= 1.0 ? dE : 1.0; 
    E *= dE;
    dice = random(ruleta[lastchan*NCHAN+NCHAN-1]);
    for (index=lastchan*NCHAN; dice > ruleta[index]; index++){} // busca donde supera dice
    lastchan = index - lastchan*NCHAN;
    chanE[lastchan] += E;
  }
  Etot = 0.0;
  Echan = 0;
  for (int i=0;i<NCHAN;i++) {
    Etot+=chanE[i];
    if (chanE[i]>0.1) Echan++;
  }
  // dinamica temporal 
  for (int i=0;i<NSPARKS;i++) {
    sparktime[i] =nextTime(1.0/meaninter);
    sparkamplitude[i] = exp(-cumtime/Tdecay);
    cumtime += sparktime[i];
  }
}


void cumsum(int *arr, int *cumsum, int len) {
  // suma acumulativa del array arr de length len escrita en array cumsum
  cumsum[0]=arr[0];
  for (int i=1; i<len; i++) {
    cumsum[i] = cumsum[i-1]+arr[i];
  }
}

void cumsum_byrow(int *darray_flat, int *sumrow2d, int nrows, int ncols){
  // suma acumulativa de las filas de la matriz de nrows x ncols flattened por rows darray_flat
  // escrita en el array 2D sumrow2d de nrows x ncols flattened
  for (int i=0; i<nrows; i++) {
    sumrow2d[i*ncols]=darray_flat[i*ncols];
    for (int j=1; j<ncols; j++) {
      sumrow2d[i*ncols+j] = sumrow2d[i*ncols+j-1]+darray_flat[i*ncols+j];
    }
  }
}



void sumrow(int darray_flat[], int sumrow[], int nrows, int ncols) {
  // toma la matriz de nrows x ncols flattened por rows darray_flat
  // y suma por filas, el resultado lo devuelve en el array sumrow de largo nrows
  for (int i=0; i<nrows; i++) {
    sumrow[i]=0;
    for (int j=0; j<ncols; j++) {
      sumrow[i] += darray_flat[i*ncols+j];
    } 
  }
}

float nextTime(float rateParameter)
{
  return -log(1.0f - random(RAND_MAX) / ((float)RAND_MAX + 1)) / rateParameter;
}

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
  Serial.print("Bytes received: ");
  Serial.println(len);
}
