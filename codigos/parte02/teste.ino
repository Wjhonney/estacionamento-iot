#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Preservando a compatibilidade entre ESP32 e ESP8266
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// Credenciais Wifi
#define WIFI_SSID "MINHAREDE"
#define WIFI_PASSWORD "MINHASENHA"

// 2. API key
#define API_KEY "CHAVEAPI"

// 3. Email e senha já cadatrada
#define USER_EMAIL "MEUEMAIL"
#define USER_PASSWORD "MINHASENHA"

// 4. RTDB URL
#define DATABASE_URL "URL"

#define IICinicial 80  // endereço i2c inicial
#define LCDinicial 32  // endereço LCD inicial
#define Vagas 6
#define Andares 3

LiquidCrystal_I2C* VetorLCD[Andares];

uint8_t dadosRecebidos;

FirebaseData fbdo; // padrão firebase
FirebaseAuth auth; // padrão firebase
FirebaseConfig config; // padrão firebase
String path; // Variavel auxiliar

void inicializaLCD() {
  for (uint8_t i = 0; i < Andares; i++) {
    VetorLCD[i] = new LiquidCrystal_I2C(LCDinicial + i, 16, 2);
    VetorLCD[i]->init();
    VetorLCD[i]->backlight();
    VetorLCD[i]->print("Ocupadas/Livres");
  }
}

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
  Serial.println("-----------------------------");
}

String uint8ParaStringBinaria(uint8_t valor) {
  String stringBinaria = "";
  for (int i = 7; i >= 0; i--) {
    if (bitRead(valor, i)) {
      stringBinaria += "1";
    } else {
      stringBinaria += "0";
    }
  }
  return stringBinaria;
}

void setup() {
  Wire.begin();
  Serial.begin(9600);
  inicializaLCD();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Conectando ao Wi-Fi");
  unsigned long ms = millis();
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Conectado com IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Cliente Firebase v%s\n\n", FIREBASE_CLIENT_VERSION);

  config.api_key = API_KEY;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.database_url = DATABASE_URL;

  Firebase.reconnectNetwork(true);

  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

  fbdo.setResponseSize(4096);

  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
}

void loop() {

  if (Firebase.ready()) {
    path = "/UsersData/";
    path += auth.token.uid.c_str();
  }
    
  for (uint8_t i = 0; i < Andares; i++) {
    Wire.requestFrom(i + IICinicial, 1);  // solicita 1 byte do dispositivo i

    while (Wire.available()) {
      dadosRecebidos = Wire.read();
    }

    Serial.print("Dados recebidos do dispositivo 0x");
    Serial.print(i + IICinicial, HEX);
    Serial.print(": ");
    Serial.println(dadosRecebidos, BIN);  // Imprime os dados como número binário

    // Conta os bits em nível lógico alto
    Serial.print("Quantidade de bits em alto: ");
    Serial.println(contarBitsAltos(dadosRecebidos));
    imprimeLinha();



    VetorLCD[i]->setCursor(0, 1);
    VetorLCD[i]->print("                ");
    VetorLCD[i]->setCursor(0, 1);
    VetorLCD[i]->print(String(contarBitsAltos(dadosRecebidos)) + "/" + String(Vagas));

    String stringBinaria = uint8ParaStringBinaria(dadosRecebidos);

    path += "/Estacionamento" + to_string(i);

    // Envia a string binária para o Firebase
    if (Firebase.setString(dadosFirebase, path, stringBinaria)) {
      Serial.println("Valor do sensor enviado com sucesso para o Firebase!");
    } else {
      Serial.println("Falha ao enviar o valor do sensor para o Firebase: " + dadosFirebase.errorReason());
    }
  }
  delay(5000);
}
