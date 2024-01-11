#include <Wire.h>

#define IICinicial 80 // endereço i2c inicial

uint8_t dadosRecebidos;
uint8_t rodada = 0;

int contarBitsAltos(uint8_t dado) {
  int contador = 0;
  for (int i = 0; i < 8; i++) {
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
}

void loop() {
  Serial.print("Rodada: ");
  Serial.println(rodada);
  for (int i = IICinicial; i < IICinicial + 3; i++) {
    Wire.requestFrom(i, 1);  // solicita 1 byte do dispositivo i

    while (Wire.available()) {
      dadosRecebidos = Wire.read();
    }

    Serial.print("Dados recebidos do dispositivo 0x");
    Serial.print(i, HEX);
    Serial.print(": ");
    Serial.println(dadosRecebidos, BIN);  // Imprime os dados como número binário

    // Conta os bits em nível lógico alto
    Serial.print("Quantidade de bits em alto: ");
    Serial.println(contarBitsAltos(dadosRecebidos));
    imprimeLinha();
  }
  rodada++;
  delay(1000);  // Aguarda 1 segundo antes da próxima leitura
}
