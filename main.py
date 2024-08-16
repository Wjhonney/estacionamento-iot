import firebase_admin
from firebase_admin import credentials
from firebase_admin import db
from firebase_admin import auth
from datetime import datetime
import bisect
import numpy as np

# Carrega as credenciais do arquivo de chave privada
cred = credentials.Certificate("dado sensível")
firebase_admin.initialize_app(cred, {
    'databaseURL': 'dado sensível'
})

# função para autenticar o usuário
def authenticate_user(email, password):
    try:
        user = auth.get_user_by_email(email)
        if user:
            user = auth.update_user(
                user.uid,
                email=email,
                password=password
            )
            print('Senha atualizada com sucesso')
            return user.uid
    except:
        return None


# credenciais
email = "dado sensível"
password = "dado sensível"

# Autentica o usuário
uid = authenticate_user(email, password)
if uid:
    print("Usuário autenticado com sucesso! UID:", uid)
else:
    print("Falha na autenticação do usuário.")


ref = db.reference('/UsersData/' + uid)
bd_carga = ref.get()
print(bd_carga)

TOLERANCIA = 0.5 # Tempo de tolerancia entre uma medida e outra, em minutos
class Historico:
    def __init__(self, ano_inicial):
        self.ano_inicial = ano_inicial
        self.vetor_temporal = []
        self.matriz_vagas = []

    def inserir_vetor(self, leitura):
        # Verificando se o ano é maior ou igual ao ano inicial
        if isinstance(leitura, dict) and 'data' in leitura:
            if leitura['data'].year >= self.ano_inicial:
                # Inserindo ordenadamente no vetor temporal
                bisect.insort(self.vetor_temporal, leitura['data'])
                # Inserindo a situação correspondente na matriz de vagas
                self.inserir_matriz(leitura)


    def inserir_matriz(self, leitura):
        # Encontrando a posição da data no vetor temporal
        if isinstance(leitura, dict) and 'data' in leitura and 'situacao' in leitura:
            posicao = self.vetor_temporal.index(leitura['data'])
            # Inserindo a situação na posição correspondente da matriz de vagas
            self.matriz_vagas.insert(posicao, leitura['situacao'])

    def imprimir(self):
        for data in self.vetor_temporal:
            print(data.strftime("%Y-%m-%dT%H:%M:%S"))

        for linha in self.matriz_vagas:
            print(linha)

class Estacionamento:
    def __init__(self, nome, numero_de_vagas, historico):
        self.nome = nome
        self.numero_de_vagas = numero_de_vagas
        self.historico = historico
        self.media_ocupacao_p_vaga = None
        self.media_desocupacao_p_vaga = None


    def calcular_tempo_medio(self, ocupacao):
        # Calcular o tempo médio de ocupação para cada vaga
        duracoes_por_vaga = []

        for vaga in range(self.historico.matriz_vagas.shape[1]): # [linha, coluna]
            duracoes = []
            inicio = None
            ant = self.historico.vetor_temporal[0]

            for i in range(self.historico.matriz_vagas.shape[0]):
                if (self.historico.vetor_temporal[i] - ant).total_seconds() <= TOLERANCIA * (60):
                    if self.historico.matriz_vagas[i, vaga] == ocupacao and inicio is None:
                        inicio = self.historico.vetor_temporal[i]
                    elif self.historico.matriz_vagas[i, vaga] == (not ocupacao) and inicio is not None:
                        duracao = (self.historico.vetor_temporal[
                                       i] - inicio).total_seconds() / 60  # duração em minutos
                        duracoes.append(duracao)
                        inicio = None
                else:
                    if inicio is not None:
                        duracao = (ant - inicio).total_seconds() / 60  # duração em minutos
                        duracoes.append(duracao)
                        inicio = None
                    if self.historico.matriz_vagas[i, vaga] == ocupacao:
                        inicio = self.historico.vetor_temporal[i]

                ant = self.historico.vetor_temporal[i]

            # Caso o carro ainda esteja na vaga na última entrada do vetor_temporal
            if inicio is not None and (self.historico.vetor_temporal[-1] - ant).total_seconds() <= TOLERANCIA * (60):
                duracao = (self.historico.vetor_temporal[-1] - inicio).total_seconds() / 60
                duracoes.append(duracao)

            duracoes_por_vaga.append(duracoes)

        # Calcular a média das durações para cada vaga
        medias_por_vaga = [np.mean(duracoes) if duracoes else 0 for duracoes in duracoes_por_vaga]

        return medias_por_vaga

def int_para_vetor_bits(numero, tamanho):
    # Inicializa um vetor de bits com zeros
    vetor_bits = [0] * tamanho

    # Converte o número inteiro para binário
    binario = bin(numero)[2:]  # Remove o prefixo '0b'

    # Preenche o vetor de bits com os valores do número binário
    for i, bit in enumerate(reversed(binario)):
        vetor_bits[tamanho - 1 - i] = int(bit)

    return vetor_bits

# Lista para armazenar os bd_carga de cada estacionamento
estacionamentos = []

# converter a string de data em tipo date e int para vetor de bits
for estacionamento, registros in bd_carga.items():
    for chave, leitura in registros.items():
        if isinstance(leitura, dict) and 'data' in leitura and 'situacao' in leitura:
            # Converte a string de data em tipo date
            data_string = leitura['data']
            data_obj = datetime.strptime(data_string, "%Y-%m-%dT%H:%M:%S")
            leitura['data'] = data_obj
            # Converte inteiro em vetor de bits
            leitura['situacao'] = int_para_vetor_bits(leitura['situacao'], registros['NumeroDeVagasNoAndar'])

#Preenche vetor e matriz de histórico com bd_carga
for estacionamento, registros in bd_carga.items():
    if isinstance(registros, dict) and 'NumeroDeVagasNoAndar' in registros:
        aux = Estacionamento(estacionamento, registros['NumeroDeVagasNoAndar'], Historico(2024))
        estacionamentos.append(aux)
        for k, v in registros.items():
            if k != 'NumeroDeVagasNoAndar':
                aux.historico.inserir_vetor(v)
        aux.historico.matriz_vagas = np.array(aux.historico.matriz_vagas)



#calcula medias
for estacionamento in estacionamentos:
    estacionamento.media_ocupacao_p_vaga = estacionamento.calcular_tempo_medio(True)
    estacionamento.media_desocupacao_p_vaga = estacionamento.calcular_tempo_medio(False)
    print(estacionamento.nome)
    print("Ocupacao por vaga: " + str(estacionamento.media_ocupacao_p_vaga))
    print("Descupacao por vaga: " + str(estacionamento.media_desocupacao_p_vaga))
    print(estacionamento.historico.matriz_vagas)
