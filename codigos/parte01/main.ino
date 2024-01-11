#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define IICinicial 80  // endereço i2c inicial
#define LCDinicial 32  // endereço LCD inicial
#define Vagas 6
#define Andares 3

LiquidCrystal_I2C* VetorLCD[Andares];

uint8_t dadosRecebidos;

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

void setup() {
  Wire.begin();
  Serial.begin(9600);

  inicializaLCD();
}

void loop() {
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
  }
  delay(1000);  // Aguarda 1 segundo antes da próxima leitura
}
