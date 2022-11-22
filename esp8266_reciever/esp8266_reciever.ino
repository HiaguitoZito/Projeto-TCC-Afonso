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
LiquidCrystal_I2C lcd(0x27,16,2);

// Porta do servidor.
WiFiServer server(80);

// Configuração da rede para o dispositivo "Ipv4, gateway padrão e mascara de subrede".
IPAddress ip(192,168,101,101);
IPAddress gateway(192,168,101,1);
IPAddress subnet(255,255,255,0);

// Valores Atribuidos durante a autenticação:
double Soil = 0;
double Max = 0;
double Height = 0;

int Effective = 0;
int target = 0;

// Variaveis de autenticação:
bool isSoilSet = false;
bool isMaxSet = false;
bool isHeightSet = false;
bool isEffectiveSet = false;
bool isConfigEff = false;
bool Reconfig = false;
bool okPressed = false;
bool okPressed2 = false;

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
  EEPROM.get(sizeof(double), Max);
  EEPROM.get(sizeof(double)*2, Soil);
  EEPROM.get(sizeof(double)*3, Height);
  EEPROM.get(sizeof(double)*4, Effective);
  EEPROM.get(sizeof(double)*4+sizeof(int), target);
  if(Height != Height) Height = 0;
  if(Max != Max) Max = 0;
  if(Soil != Soil) Soil = 0;
  if(Effective != Effective) Effective = 0;

  isMaxSet = Max != 0;
  isSoilSet = Soil != 0;
  isHeightSet = Height != 0;
  isEffectiveSet = Effective != 0;
}

String Mensagens[8] = {"Coloque o implemento","no ponto inicial","Coloque o implemento","no Solo","Coloque o implemento","no ponto maximo","Altura do","Implemento"}; // Mensagens de configuração.

bool isDrawn = false; // Variavel para checar se foi desenhado na tela.



int i = 0;



bool isDoingPass = false;
bool v1 = false;

int SenhaTemp[4];
int i2 = 0;

void loop() {
  // Recebe Informação do outro ESP8266.
  WiFiClient client = server.available(); // cria um novo client e chega se ele está disponivel.
  if(!client) return; // se o client não estiver disponivel não executara o codigo abaixo e tentara novamente.
  client.setNoDelay(1); // Deixa a comunicação entre os dois ESP8266 sem atraso.
  String answer = client.readStringUntil('\r'); // Lê a resposta do ESP8266 secundario.
  client.flush(); // Limpa o buffer e pointers do cliente para não usar memória sem motivo.
  if(Reconfig){
      String mensagem[3] = {"Reconfig. Max.", "Reconfig. Solo", "Reconfig. Altura"};
          lcd.setCursor(0,0);
          lcd.print(mensagem[i]);
          isDrawn = true;
        if(!okPressed2) i += digitalRead(Down)-digitalRead(Up); // Se você apertar para baixo vai adicionar 1 para variavel "i" o contrario acontecera se apertar para cima.
        if(!okPressed2) while(digitalRead(Down)-digitalRead(Up) != 0); // Continua no "if statement" se o operador não soltar o botão de baixo/cima, para evitar double click.

        // Mantem a variavel i entre 0 e 2.
        i = i > 3 ? 3 : i; 
        i = i < 0 ? 0 : i;
        
        if(!okPressed2)isDrawn = digitalRead(Up) || digitalRead(Down) ? false : true; // Se o operador apertar qualquer botão ele ira atualizar a tela.
        if(okPressed2){
              lcd.clear();
              Height += digitalRead(Up)-digitalRead(Down);
              lcd.setCursor(0,0);
              lcd.print(String(Height) + " CM");
              if(digitalRead(Ok)){
                EEPROM.put(sizeof(double)*3,Height);
                boolean ok1 = EEPROM.commit();
                Serial.print(ok1 ? "Saved" : "Discarted");
                while(digitalRead(Ok));
                isDrawn = false;
                okPressed = false;
                okPressed2 = false;
                Reconfig = false;
            }
        }
        
        if(digitalRead(Ok) && !okPressed2){ // Se o operador confirmar ele ira atribuir o novo valor a Variavel selecionada.
          if(i == 0) {
            Reconfig = false;
            Max = answer.toInt();
            EEPROM.put(sizeof(double),Max);
            boolean ok1 = EEPROM.commit();
            Serial.print(ok1 ? "Saved" : "Discarted");
            double valueGet;
            EEPROM.get(sizeof(double),valueGet);
            Serial.println(String(valueGet));
            isDrawn = false;
          }
          else if(i == 1) {
            Reconfig = false;
            Soil = answer.toInt();
            EEPROM.put(sizeof(double)*2,Soil);
            boolean ok1 = EEPROM.commit();
            Serial.print(ok1 ? "Saved" : "Discarted");
            double valueGet;
            EEPROM.get(sizeof(double)*2,valueGet);
            Serial.println(String(valueGet));
            isDrawn = false;
          }else{
            okPressed2 = true;
            isDrawn = true;
          }
          i = 0;
          while(digitalRead(Ok)); // Continua no if se o operador não soltar o Ok.
        }
      isDrawn = false;
    }else if(!isSoilSet || !isMaxSet || !isHeightSet){ // Checa se qualquer autenticação ainda não foi feita, Se não foi feita ira começar o processo.
    if(!isSoilSet){
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
        EEPROM.put(sizeof(double),Soil);
        boolean ok1 = EEPROM.commit();
            Serial.print(ok1 ? "Saved" : "Discarted");
        while(digitalRead(Ok));
        isDrawn = false;
      }
    }
    else if(!isMaxSet){
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
        EEPROM.put(sizeof(double)*2,Max);
        boolean ok1 = EEPROM.commit();
            Serial.print(ok1 ? "Saved" : "Discarted");
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
      bool isP = false;
      if(okPressed2){
          lcd.clear();
          Height += digitalRead(Up)-digitalRead(Down);
          lcd.setCursor(0,0);
          lcd.print(String(Height) + " CM");
          if(digitalRead(Ok)){
          EEPROM.put(sizeof(double)*3,Height);
          boolean ok1 = EEPROM.commit();
          Serial.print(ok1 ? "Saved" : "Discarted");
          while(digitalRead(Ok));
          isDrawn = false;
          okPressed = false;
          okPressed2 = false;
          Reconfig = false;
        }
      }
    }
  }else{ // Se ja aconteceu o processo de autenticação então ele começara a execução do programa.
    if(digitalRead(Up) && digitalRead(Down)) Reconfig = true;
    lcd.clear();
    lcd.setCursor(0,0);
    double result = -(Height/(Max+Soil))*(answer.toInt()-Soil);
    if(target >= result) {
      digitalWrite(15, HIGH);
      digitalWrite(2,HIGH);
    }
    else {
      digitalWrite(15,LOW);
      digitalWrite(2,LOW);
    }
    digitalWrite(0, result < target && result > Effective ? LOW : HIGH);
    if(!isDoingPass){
      lcd.print(String(result) + " CM");
      lcd.setCursor(0,1);
      lcd.print("Alv: " + String(target) + " Eftv: " + String(Effective));
    }
    if(digitalRead(Ok) && !isDoingPass) {
      isDoingPass = true;
      while(digitalRead(Ok));
    }
    if(isDoingPass){
      if(!v1 && !isConfigEff){
        if(i2 < 4){
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Senha: ");
          lcd.setCursor(0,1);
          int temp = 0;
          temp += digitalRead(Up)-digitalRead(Down);
          SenhaTemp[i2] += temp;
          SenhaTemp[i2] = SenhaTemp[i2] > 9 ? 9 : SenhaTemp[i2];
          SenhaTemp[i2] = SenhaTemp[i2] < 0 ? 0 : SenhaTemp[i2];
          if(digitalRead(Ok) && i2 < 4)i2 += 1;
          lcd.print(String(SenhaTemp[0]) + String(SenhaTemp[1]) + String(SenhaTemp[2]) + String(SenhaTemp[3]));
        }
        if(i2 == 4) {
          if((SenhaTemp[0]*1000+SenhaTemp[1]*100+SenhaTemp[2]*10+SenhaTemp[3]) == Senha){
            v1 = true;
          }else{
            for(int d = 0; d < 4; d++) SenhaTemp[d] = 0;
            isDoingPass = false;
            i2 = 0;
          }
        }
        while(digitalRead(Ok));
      }
      
      if(v1){
        lcd.clear();
        lcd.setCursor(0,1);
        lcd.print(String(target));
        lcd.setCursor(0,0);
        lcd.print("Alvo:");
        target += digitalRead(Up)-digitalRead(Down);
        if(digitalRead(Ok)){
          EEPROM.put(sizeof(double)*4+sizeof(int), target);
          EEPROM.commit();
          v1 = false;
          isConfigEff = true;
          for(int d = 0; d < 4; d++) SenhaTemp[d] = 0;
          i2 = 0;
          while(digitalRead(Ok));
        }
      }
      if(isConfigEff){
        lcd.clear();
        Effective += digitalRead(Up)-digitalRead(Down);
        lcd.setCursor(0,1);
        lcd.print(String(Effective) + " CM");
        lcd.setCursor(0,0);
        lcd.print("Efetivo:");
        if(digitalRead(Ok)){
          EEPROM.put(sizeof(double)*4,Effective);
          EEPROM.commit();
          isConfigEff = false;
          isDoingPass = false;
          i2 = 0;
        }
      }
      while(digitalRead(Ok));
    }
    
    if(Reconfig) lcd.clear();
  }
}
