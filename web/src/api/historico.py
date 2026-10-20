import requests
import matplotlib.pyplot as plt
from datetime import datetime

def obter_dados_luminosidade(lastN):
    url = f"http://54.236.175.117:8666/STH/v1/contextEntities/type/Lamp/id/urn:ngsi-ld:Lamp:001/attributes/luminosity?lastN={lastN}"

    headers = {
        'fiware-service': 'smart',
        'fiware-servicepath': '/'
    }

    response = requests.get(url, headers=headers, timeout=10)

    if response.status_code == 200:
        data = response.json()
        luminosity_data = data['contextResponses'][0]['contextElement']['attributes'][0]['values']
        return luminosity_data
    else:
        print(f"Erro ao obter dados: {response.status_code}")
        return []

def obter_dados_temperatura(lastN):
    url = f"http://54.236.175.117:8666/STH/v1/contextEntities/type/Lamp/id/urn:ngsi-ld:Lamp:001/attributes/temperature?lastN={lastN}"

    headers = {
        'fiware-service': 'smart',
        'fiware-servicepath': '/'
    }

    response = requests.get(url, headers=headers, timeout=10)

    if response.status_code == 200:
        data = response.json()
        temperature_data = data['contextResponses'][0]['contextElement']['attributes'][0]['values']
        return temperature_data
    else:
        print(f"Erro ao obter dados: {response.status_code}")
        return []

def obter_dados_umidade(lastN):
    url = f"http://54.236.175.117:8666/STH/v1/contextEntities/type/Lamp/id/urn:ngsi-ld:Lamp:001/attributes/humidity?lastN={lastN}"

    headers = {
        'fiware-service': 'smart',
        'fiware-servicepath': '/'
    }

    response = requests.get(url, headers=headers, timeout=10)

    if response.status_code == 200:
        data = response.json()
        humidity_data = data['contextResponses'][0]['contextElement']['attributes'][0]['values']
        return humidity_data
    else:
        print(f"Erro ao obter dados: {response.status_code}")
        return []

def plotar_luminosidade(lastN):
    dados = obter_dados_luminosidade(lastN)

    if not dados:
        print("Nenhum dado de luminosidade para plotar")
        return

    tempos = [datetime.fromisoformat(d['recvTime']) for d in dados]
    valores = [d['attrValue'] for d in dados]

    plt.figure(figsize=(10, 5))
    plt.plot(tempos, valores, marker='o', linestyle='-', color='orange')
    plt.title('Luminosidade')
    plt.xlabel('Horário (UTC)')
    plt.ylabel('Luminosidade (%)')
    plt.grid(True)
    plt.gcf().autofmt_xdate()
    plt.tight_layout()
    plt.show()

def plotar_temperatura(lastN):
    dados = obter_dados_temperatura(lastN)

    if not dados:
        print("Nenhum dado de temperatura para plotar")
        return

    tempos = [datetime.fromisoformat(d['recvTime']) for d in dados]
    valores = [d['attrValue'] for d in dados]

    plt.figure(figsize=(10, 5))
    plt.plot(tempos, valores, marker='o', linestyle='-', color='red')
    plt.title('Temperatura')
    plt.xlabel('Horário (UTC)')
    plt.ylabel('Temperatura (°C)')
    plt.grid(True)
    plt.gcf().autofmt_xdate()
    plt.tight_layout()
    plt.show()

def plotar_umidade(lastN):
    dados = obter_dados_umidade(lastN)

    if not dados:
        print("Nenhum dado de umidade para plotar")
        return

    tempos = [datetime.fromisoformat(d['recvTime']) for d in dados]
    valores = [d['attrValue'] for d in dados]

    plt.figure(figsize=(10, 5))
    plt.plot(tempos, valores, marker='o', linestyle='-', color='blue')
    plt.title('Umidade')
    plt.xlabel('Horário (UTC)')
    plt.ylabel('Umidade (%)')
    plt.grid(True)
    plt.gcf().autofmt_xdate()
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    plotar_luminosidade(30)
    plotar_temperatura(30)
    plotar_umidade(30)
