#include <Wire.h>

#include <LiquidCrystal_I2C.h>

#include <time.h>

#include <WiFi.h>

#include <esp_system.h>

#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>
/*------------------------------------------------------------------------------------------------------------------------------------------------
                                ----------------------------------------------
                                |     Prêambulo - constantes e variáveis     |
                                ----------------------------------------------
------------------------------------------------------------------------------------------------------------------------------------------------*/

#define GPIO_BOTAO 5
#define TEMPO_DEBOUNCE 10  //ms

bool simulacao = true;
unsigned long tempoDaUltimaSituacao = 0;
unsigned long tempoEntreMudancaDeSituacao = 90000;  // tempo entre geracao de numeros aleatórios

// 1. Credenciais WI-FI
#define WIFI_SSID "Dado Sensivel"
#define WIFI_PASSWORD "Dado Sensivel"
// 2. API key
#define API_KEY "Dado Sensivel"
// 3. Email e password essa conta já deve existir
#define USER_EMAIL "Dado Sensivel"
#define USER_PASSWORD "Dado Sensivel"
// 4. RTDB URL
#define DATABASE_URL "Dado Sensivel"
// 6. Objeto Firebase Data
FirebaseData fbdo;
// 7. Dados da autenticaçao
FirebaseAuth auth;
// 8. Dados da configuraçao Firebase
FirebaseConfig config;
//Endereços i2c dos expansores de porta e i2c lcd
const uint8_t IICinicial = 32;  // endereço i2c inicial
const uint8_t LCDinicial = 39;  // endereço LCD inicial
const uint8_t Vagas = 6;
const uint8_t Andares = 3;
// declara vetor de objeto LDC
LiquidCrystal_I2C* VetorLCD[Andares];
// variável de transporte dos disposítivos i2c para o microcontrolador
uint8_t dadosRecebidos;
// variavel auxiliar para informar o diretorio do dados postado no Firebase
String pathRetorno;
String path;
/*------------------------------------------------------------------------------------------------------------------------------------------------
                                -------------------------------------------
                                |     Prêambulo - subrotinas e funções    |
                                -------------------------------------------
------------------------------------------------------------------------------------------------------------------------------------------------*/
/* Função ISR (chamada quando há geração da
interrupção) */
void IRAM_ATTR funcao_ISR() {
  delay(50);  // efeito debounce
  simulacao = ~simulacao;
}

void inicializaLCD() {
  for (uint8_t i = 0; i < Andares; i++) {
    VetorLCD[i] = new LiquidCrystal_I2C(LCDinicial - i, 16, 2);
    VetorLCD[i]->init();
    VetorLCD[i]->backlight();
    VetorLCD[i]->print("Iniciado");
  }
}
// conta a quantidades de 1s em dado
int contarBitsAltos(uint8_t dado) {
  int contador = 0;
  for (uint8_t i = 0; i < 8; i++) {
    if (dado & (1 << i)) {
      contador++;
    }
  }
  return contador;
}

void imprimeLinha() {
  Serial.println("----------------------------------------------------------");
}

// imprime endereço e dados recebidos do dispositivo i2c
void exibirDadosRecebidos(uint8_t i) {
  Serial.print("Dados recebidos do dispositivo 0x");
  Serial.print(i + IICinicial, HEX);
  Serial.print(": ");
  Serial.println(dadosRecebidos, BIN);  // Imprime os dados como número binário
}

void exibirMapaLCD(uint8_t dados, int andar) {
  // Calcula a quantidade de vagas por linha
  int vagasPorLinha = Vagas / 2;

  VetorLCD[andar]->setCursor(0, 0);
  VetorLCD[andar]->print("                ");

  VetorLCD[andar]->setCursor(0, 1);
  VetorLCD[andar]->print("                ");


  // Define a posição inicial no LCD para a parte inferior
  VetorLCD[andar]->setCursor(16 - vagasPorLinha, 0);

  // Exibe as vagas ocupadas na parte inferior do LCD
  for (int i = 0; i < vagasPorLinha; i++) {
    if (dados & (1 << i)) {
      VetorLCD[andar]->write(byte(0));  // Quadrado preenchido
    } else {
      VetorLCD[andar]->write(byte(1));  // Quadrado vazio
    }
  }

  // Define a posição inicial no LCD para a parte superior
  VetorLCD[andar]->setCursor(16 - vagasPorLinha, 1);

  // Exibe as vagas ocupadas na parte superior do LCD
  for (int i = vagasPorLinha; i < Vagas; i++) {
    if (dados & (1 << i)) {
      VetorLCD[andar]->write(byte(0));  // Quadrado preenchido
    } else {
      VetorLCD[andar]->write(byte(1));  // Quadrado vazio
    }
  }
}

String obtemDataeHoraAtual() {
  time_t agora = time(nullptr);
  struct tm* timeinfo = localtime(&agora);

  char buffer[26];  // Como ISO 8601: YYYY-MM-DDThh:mm:ss
  strftime(buffer, sizeof(buffer), "%FT%T", timeinfo);

  return String(buffer);
}
/*------------------------------------------------------------------------------------------------------------------------------------------------
                                -------------------------------------------
                                |      Predefiniçoes - funçao setup       |
                                -------------------------------------------
------------------------------------------------------------------------------------------------------------------------------------------------*/
void setup() {
  Wire.begin(22, 21);
  Serial.begin(9600);

  pinMode(GPIO_BOTAO, INPUT);
  attachInterrupt(GPIO_BOTAO, funcao_ISR, RISING);
  randomSeed(esp_random());

  // configura displays
  Serial.println();
  Serial.println("Estou configurando os displays.");
  inicializaLCD();

  // Define os caracteres personalizados para o LCD
  byte customChar[2][8] = {
    { B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111 },  // Quadrado preenchido
    { B11111, B10001, B10001, B10001, B10001, B10001, B10001, B11111 }   // Quadrado vazio
  };
  //cria os novos caracteres
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < Andares; j++) {
      VetorLCD[j]->createChar(i, customChar[i]);
    }
  }

  // configuracao wifi
  Serial.println();
  Serial.println("Estou configurando a conexão com o Wifi.");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  unsigned long ms = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  //configuracao firebase
  Serial.println();
  Serial.println("Estou configurando a conexão com o Firebase.");
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  /* informa a Api key */
  config.api_key = API_KEY;
  /* define as credenciais */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  /* informa a RTDB URL */
  config.database_url = DATABASE_URL;
  // configurações do Firebase
  Firebase.reconnectNetwork(true);
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);
  fbdo.setResponseSize(4096);
  config.token_status_callback = tokenStatusCallback;  // see addons/TokenHelper.h
  Firebase.begin(&config, &auth);

  // verificara se esta tudo certo com o Firebase
  if (Firebase.ready()) {
    pathRetorno = "/UsersData/";
    pathRetorno += auth.token.uid.c_str();
    pathRetorno += "/Estacionamento";
  }

  for (uint8_t i = 0; i < Andares; i++) {
    path = pathRetorno;
    path += String(i) + "/NumeroDeVagasNoAndar";
    Firebase.RTDB.setInt(&fbdo, path, Vagas);
  }

  //configura hora
  Serial.println();
  Serial.println("Estou configurando a data e hora atual.");
  // Configurar NTP (Network Time Protocol) para obter hora atual
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  // Configura o fuso horário para Brasília (BRT, UTC-3)
  setenv("TZ", "BRT3BRST,M10.3.0/0,M2.3.0/0", 1);
  Serial.println("Waiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println();
}
/*------------------------------------------------------------------------------------------------------------------------------------------------
                                --------------------------------------
                                |      Principal - funçao main       |
                                --------------------------------------
------------------------------------------------------------------------------------------------------------------------------------------------*/
void loop() {
  
  for (uint8_t i = 0; i < Andares; i++) { //itera para todos os andares
    path = pathRetorno;

    if (simulacao == false) {  // solicita e recebe 1 byte do dispositivo i
      Wire.requestFrom(i + IICinicial, 1);
      // recebe os dados
      while (Wire.available()) {
        dadosRecebidos = Wire.read();
      }
      // normaliza dados recebidos
      dadosRecebidos = ~dadosRecebidos;
    } else { // gera um valor aleatório
      dadosRecebidos = esp_random() % 64;  // valor aleatório entre 0 e 63
    }

    // exibi o endereço e dados recebidos
    exibirDadosRecebidos(i);

    // Conta os bits em nível lógico alto
    Serial.print("Quantidade de bits em alto: ");
    Serial.println(contarBitsAltos(dadosRecebidos));

    // imprime mapa do estacionamento no LCD
    exibirMapaLCD(dadosRecebidos, i);

    path += String(i);

    // Cria o JSON para enviar ao Firebase
    FirebaseJson json;
    String dataHoraAtual = obtemDataeHoraAtual();
    json.set("/data", dataHoraAtual);
    json.set("/situacao", dadosRecebidos);

    // Envia a stringBinária para o Firebase
    if (Firebase.RTDB.pushJSON(&fbdo, path.c_str(), &json)) {
      Serial.println("Dados adicionados com sucesso!");
    } else {
      Serial.println("Falha! Mais informacoes do erro: " + fbdo.errorReason());
    }
    imprimeLinha();
  }
  imprimeLinha();
  delay(10000);
}