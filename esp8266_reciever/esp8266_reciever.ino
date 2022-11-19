#ifdef ESP32
  #include "WiFi.h"
#else
  #include <ESP8266WiFi.h>
#endif
#include "ESPAsyncWebServer.h"
#include "SPI.h"
#include "LiquidCrystal_I2C.h"
#include "Arduino.h"
#include <ESP_EEPROM.h>

#define Up 14
#define Ok 12
#define Down 13

const int Senha = 1234;

// Credenciais da rede.
const char* ssid = "ESPsoftAP_01";
const char* password = "12345678";

// Objeto do display de cristal.
LiquidCrystal_I2C lcd(0x27,20,2);

// Porta do servidor.
WiFiServer server(80);

// Configuração da rede para o dispositivo "Ipv4, gateway padrão e mascara de subrede".
IPAddress ip(192,168,101,101);
IPAddress gateway(192,168,101,1);
IPAddress subnet(255,255,255,0);



void setup() {
  Serial.begin(115200);
  Serial.println("Teste");
  
  WiFi.config(ip,gateway,subnet); // Configura a rede.
  WiFi.begin(ssid, password); // Começa a conexão usando o nome da rede e a senha.
  pinMode(15,OUTPUT);
  pinMode(2,OUTPUT);
  pinMode(Up,INPUT);
  pinMode(Down,INPUT);
  pinMode(OK,INPUT);
  while(WiFi.status() != WL_CONNECTED){ // Faz um loop esperando a conexão na rede
    Serial.print(".");
    delay(500);
  }
  
  server.begin(); // Inicializa o servidor
  
  // Inicializa o display.
  lcd.init();    
  lcd.backlight();
  
  EEPROM.begin(512);
}

String Mensagens[6] = {"Coloque o implemento","no ponto inicial","Coloque o implemento","no Solo","Coloque o implemento","no ponto maximo"}; // Mensagens de configuração.

boolean isDrawn = false; // Variavel para checar se foi desenhado na tela.

// Valores Atribuidos durante a autenticação:
double Min = 0;
double Soil = 0;
double Max = 0;

// Variaveis de autenticação:
boolean isMinSet = false;
boolean isSoilSet = false;
boolean isMaxSet = false;
boolean Reconfig = false;

int i = 0;

int target = 0;
boolean tryLoad = false;

boolean isDoingPass = false;
bool v1 = false;

int SenhaTemp[4];
int i2 = 0;

void loop() {
  if(!tryLoad){
    EEPROM.get(0, Min);
    EEPROM.get(sizeof(double), Max);
    EEPROM.get(sizeof(double)*2, Soil);
    Serial.println("Min: " + String(Min));
    Serial.println("Max: " + String(Max));
    Serial.println("Soil: " + String(Soil));

    if(Min != 0 || Max != 0 || Soil != 0){
      isMinSet = true;
      isMaxSet = true;
      isSoilSet = true;
    }
    tryLoad = true;
  }
  // Recebe Informação do outro ESP8266.
  WiFiClient client = server.available(); // cria um novo client e chega se ele está disponivel.
  if(!client) return; // se o client não estiver disponivel não executara o codigo abaixo e tentara novamente.
  client.setNoDelay(1); // Deixa a comunicação entre os dois ESP8266 sem atraso.
  String answer = client.readStringUntil('\r'); // Lê a resposta do ESP8266 secundario.
  client.flush(); // Limpa o buffer e pointers do cliente para não usar memória sem motivo.
  if(Reconfig){
      String mensagem[3] = {"Reconfig. Min.", "Reconfig. Max.", "Reconfig. Solo"};
        if(!isDrawn){ // Checa para ver se ja foi escrito na tela, se sim não ira escrever denovo pois não tem necessidade.
          lcd.setCursor(0,0);
          lcd.print(mensagem[i]);
          isDrawn = true;
        }
        i += digitalRead(Down)-digitalRead(Up); // Se você apertar para baixo vai adicionar 1 para variavel "i" o contrario acontecera se apertar para cima.
        while(digitalRead(Down)-digitalRead(Up) != 0); // Continua no "if statement" se o operador não soltar o botão de baixo/cima, para evitar double click.

        // Mantem a variavel i entre 0 e 2.
        i = i > 2 ? 2 : i; 
        i = i < 0 ? 0 : i;
        
        isDrawn = digitalRead(Up) || digitalRead(Down) ? false : true; // Se o operador apertar qualquer botão ele ira atualizar a tela.
        
        if(digitalRead(Ok)){ // Se o operador confirmar ele ira atribuir o novo valor a Variavel selecionada.
          Reconfig = false;
          if(i == 0) {
            Min = answer.toInt();
            EEPROM.put(0,Min);
            boolean ok1 = EEPROM.commit();
            Serial.print(ok1 ? "Saved" : "Discarted");
            double valueGet;
            EEPROM.get(0,valueGet);
            Serial.println(String(valueGet));
          }
          else if(i == 1) {
            Max = answer.toInt();
            EEPROM.put(sizeof(double),Max);
            boolean ok1 = EEPROM.commit();
            Serial.print(ok1 ? "Saved" : "Discarted");
            double valueGet;
            EEPROM.get(sizeof(double),valueGet);
            Serial.println(String(valueGet));
          }
          else if(i == 2) {
            Soil = answer.toInt();
            EEPROM.put(sizeof(double)*2,Soil);
            boolean ok1 = EEPROM.commit();
            Serial.print(ok1 ? "Saved" : "Discarted");
            double valueGet;
            EEPROM.get(sizeof(double)*2,valueGet);
            Serial.println(String(valueGet));
          }
          i = 0;
          isDrawn = false;
          while(digitalRead(Ok)); // Continua no if se o operador não soltar o Ok.
        }
      isDrawn = false;
    }else if(!isMinSet || !isSoilSet || !isMaxSet){ // Checa se qualquer autenticação ainda não foi feita, Se não foi feita ira começar o processo.
    if(!isMinSet){
      if(!isDrawn){ // Checa para ver se foi escrito na tela e não precisar ficar escrevendo um monte de vezes sem nescessidade.
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(Mensagens[0]);
        lcd.setCursor(0,1);
        lcd.print(Mensagens[1]);
        isDrawn = true;
      }
      if(digitalRead(Ok)){ // Checa de o botão "Ok" foi pressionado para guardar as informações adiquiridas.
        isMinSet = true;
        Min = answer.toInt();
        EEPROM.put(0,Min);
        boolean ok1 = EEPROM.commit();
            Serial.print(ok1 ? "Saved" : "Discarted");
        while(digitalRead(Ok));
        isDrawn = false;
      }
    }else if(!isSoilSet){
      if(!isDrawn){ // Checa para ver se foi escrito na tela e não precisar ficar escrevendo um monte de vezes sem nescessidade.
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(Mensagens[2]);
        lcd.setCursor(0,1);
        lcd.print(Mensagens[3]);
        isDrawn = true;
      }
      if(digitalRead(Ok)){ // Checa de o botão "Ok" foi pressionado para guardar as informações adiquiridas.
        isSoilSet = true;
        Soil = answer.toInt();
        EEPROM.put(1,Soil);
        boolean ok1 = EEPROM.commit();
            Serial.print(ok1 ? "Saved" : "Discarted");
        while(digitalRead(Ok));
        isDrawn = false;
      }
    }
    else{
      if(!isDrawn){ // Checa para ver se foi escrito na tela e não precisar ficar escrevendo um monte de vezes sem nescessidade.
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(Mensagens[4]);
        lcd.setCursor(0,1);
        lcd.print(Mensagens[5]);
        isDrawn = true;
      }
      if(digitalRead(Ok)){ // Checa de o botão "Ok" foi pressionado para guardar as informações adiquiridas.
        isMaxSet = true;
        Max = answer.toInt();
        EEPROM.put(2,Max);
        boolean ok1 = EEPROM.commit();
            Serial.print(ok1 ? "Saved" : "Discarted");
      }
    }
  }else{ // Se ja aconteceu o processo de autenticação então ele começara a execução do programa.
    if(digitalRead(Up) && digitalRead(Down)) Reconfig = true;
    lcd.clear();
    lcd.setCursor(0,0);
    double result = 80/(Max-Min)*answer.toInt()-80/(Max-Min)*Soil;
    if(result > target + 1 || result < target - 1) {
      digitalWrite(15, HIGH);
      digitalWrite(2,HIGH);
    }
    else {
      digitalWrite(15,LOW);
      digitalWrite(2,LOW);
    }
    if(!isDoingPass){
      lcd.print(String(result) + " CM");
      lcd.setCursor(0,1);
      lcd.print("Alvo: " + String(target));
    }
    bool temp2 = false;
    if(digitalRead(Ok) && !isDoingPass) {
      isDoingPass = true;
      temp2 = true;
    }
    if(isDoingPass){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Senha: ");
      lcd.setCursor(0,1);
      int temp = 0;
      temp += digitalRead(Up)-digitalRead(Down);
      if(digitalRead(Ok) && !temp2)i2 += 1;
      SenhaTemp[i2] += temp;
      lcd.print(String(SenhaTemp[0]) + String(SenhaTemp[1]) + String(SenhaTemp[2]) + String(SenhaTemp[3]));
      bool temp3 = false;
      if(digitalRead(Ok) && v1 == 0) {
        if(i2 == 4) {
          if(SenhaTemp[0]*1000+SenhaTemp[1]*100+SenhaTemp[2]*10+SenhaTemp[3] == Senha){
            v1 = true;
            temp3 = true;
          }else{
            isDoingPass = false;
            for(int d = 0; d < 4; d++) SenhaTemp[d] = 0;
            i2 = 0;
          }
        }
      }
      Serial.println(String(v1));
      Serial.println(String(isDoingPass));
      if(v1 != 0){
        lcd.clear();
        lcd.setCursor(0,1);
        lcd.print(String(target));
        target += digitalRead(Up)-digitalRead(Down);
        if(digitalRead(Ok) && !temp3){
          isDoingPass = false;
          v1 = false;
          for(int d = 0; d < 4; d++) SenhaTemp[d] = 0;
          i2 = 0;
        }
        temp3 = false;
      }
      temp2 = false;
      while(digitalRead(Ok));
    }
    if(Reconfig) lcd.clear();
  }
}
