/*
 * Quando for copiar, dê os creditos a Eletrogate
 * Gustavo Nery - Eletrogate
 * Créditos a Sinric por disponibilizar o exemplo de chamada ao servidor
 */

#include <ESP8266WiFi.h>
#include <SinricPro.h>
#include <SinricProDimSwitch.h>

#define WIFI_SSID         "Coloque_Aqui_o_nome_do_seu_WiFi"    
#define WIFI_PASS         "A_Senha_do_seu_WiFi"
#define APP_KEY           "Coloque aqui a chave do app, que você acha lá no site do Sinric Pro"      // O seu App Key é algo como "de0bxxxx-1x3x-4x3x-ax2x-5dabxxxxxxxx"
#define APP_SECRET        "Coloque o senha do app, está na parte de credenciais"   // O seu App Secret é algo como "5f36xxxx-x3x7-4x3x-xexe-e86724a9xxxx-4c4axxxx-3x3x-x5xe-x9x3-333d65xxxxxx"
 
#define LAMPADA_ID        "Copie e cole aqui O ID do seu dispositivo"    // Algo como "5dc1564130xxxxxxxxxxxxxx"

#define BAUD_RATE         115200                // CONFIGURE O SEU SERIAL PARA 115200

#define Disparo 12  // Pino que fará o acionamento do triac
#define Detector 14 // Pino que fará a detecção de cruzamento de zero

volatile int ajuste = 0;
volatile bool Aux = 0;
unsigned long time1;

// A estrutura para guardar todos os estados e valores de ajuste do Dimmer.
struct {
  bool powerState = false;
  int powerLevel = 0;
} device_state;

bool onPowerState(const String &deviceId, bool &state) {
  Serial.printf("O dispositivo %s esta %s \r\n", deviceId.c_str(), state ? "ligado" : "desligado");
  device_state.powerState = state;
  device_state.powerState == 0 ? ajuste = 0 : ajuste = device_state.powerLevel;
  return true; // request handled properly
}

bool onPowerLevel(const String &deviceId, int &powerLevel) {
  device_state.powerLevel = powerLevel;
  Serial.printf("O nivel do dispositivo %s foi alterado para %d\r\n", deviceId.c_str(), device_state.powerLevel);
  ajuste = device_state.powerLevel;
  return true;
}

bool onAdjustPowerLevel(const String &deviceId, int &levelDelta) {
  device_state.powerLevel += levelDelta;
  Serial.printf("O nivel do dispositivo %s foi alterado para %i to %d\r\n", deviceId.c_str(), levelDelta, device_state.powerLevel);
  ajuste = device_state.powerLevel;
  levelDelta = device_state.powerLevel;
  return true;
}

void setupWiFi() {
  Serial.printf("\r\n[Wifi]: Conectando");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  IPAddress localIP = WiFi.localIP();
  Serial.printf("Conectado!\r\n[WiFi]: O endereço IP e %d.%d.%d.%d\r\n", localIP[0], localIP[1], localIP[2], localIP[3]);
}

void setupSinricPro() {
  SinricProDimSwitch &meuDimmer = SinricPro[LAMPADA_ID];

  meuDimmer.onPowerState(onPowerState);
  meuDimmer.onPowerLevel(onPowerLevel);
  meuDimmer.onAdjustPowerLevel(onAdjustPowerLevel);

  SinricPro.onConnected([]() {
    Serial.printf("Conectado ao servidor SinricPro\r\n");
  });
  SinricPro.onDisconnected([]() {
    Serial.printf("Disconectado do servidor SinricPro\r\n");
  });
  SinricPro.begin(APP_KEY, APP_SECRET);
}

void setup() {
  Serial.begin(BAUD_RATE); Serial.printf("\r\n\r\n"); // Iniciando a comunicação Serial
  pinMode(Detector, INPUT_PULLUP); // Setando o Detector como entrada
  pinMode(Disparo, OUTPUT); // Saída para controle do disparo
  attachInterrupt(digitalPinToInterrupt(Detector), interrupt, RISING); // Configurando a interrupção
  setupWiFi(); // Configurações padrões da Sinric
  setupSinricPro();
}

void loop() {

  if (Aux && ((micros() - time1) >= (100 - ajuste) * 76)) { // Controle do tempo de disparo do TRIAC
    digitalWrite(Disparo, HIGH); // Dispara o TRIAC
    attachInterrupt(digitalPinToInterrupt(Detector), interrupt, RISING);  // Ligando a interrupcao novamente (isso desabilita o timer)
    Aux = 0; 
    SinricPro.handle(); // Uma chamada para o servidor Sinric para obter o valor de ajuste
  }

}

ICACHE_RAM_ATTR void interrupt() // Rotina de interrupcao
{
  digitalWrite(Disparo, LOW);  // Desliga o pino de disparo para que so ligue depois do tempo determinado
  detachInterrupt(digitalPinToInterrupt(Detector)); //  Desativa a interrupcao para que seja possivel usar o timer
  time1 = micros();
  Aux = 1;
}