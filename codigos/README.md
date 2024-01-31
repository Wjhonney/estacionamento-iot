# Códigos

## Parte 1
Nessa parte foi implementada:
* leitura de entradas utilizando protocolo iic (i2c)
* Contagem do número de sinais com nível lógico alto
* Exibição desses valores no display LCD utilizando o protocolo i2c

Encontrei dificildades em implementar a exibição de dados no display utilizando o protocolo i2c no simlador SimuIDE. Decidi procurar um outro simulador para execultar apenas esse tópico. Pois estou utilizando o Serial fornecido pelo SimulIDE e a exibição dos dados no display LCD na implementação real é simples.

Utilizei o Tinkercad para a simulação do LCD, utilizei somas sucessivas pela variável 'rodada', no Simulide utilizei essa variável para controlar a quantidade de impressões por simulção.

## Parte 2
Nessa parte foi execultada o seguinte teste:
* Envio de dados para o servidor web

Utilizei [esse vídeo do YT](https://www.youtube.com/watch?v=6szx9XdqSPQ&ab_channel=UtehStr) como referência principal. Nele é utilizado [essa biblioteca](https://github.com/mobizt/Firebase-ESP-Client), ela é resposável por implementar o transito de dados entre o esp8266 e o Firebase.

Foi Utilizado o Firebase Realtime Database como banco de dados.
