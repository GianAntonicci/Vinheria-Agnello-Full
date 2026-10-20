import requests
import matplotlib.pyplot as plt

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
	