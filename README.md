Monitoramento de Culturas com ESP32 - Documentação
==================================================

Este projeto implementa uma aplicação de monitoramento de dados ambientais e do solo para agricultura utilizando a placa ESP32 e sensores de umidade do solo, luminosidade, umidade e temperatura do ar, e temperatura do solo. O código coleta esses dados, os analisa e envia os resultados para um servidor MQTT, possibilitando o monitoramento remoto.

Funcionalidades Principais
--------------------------

1.  **Conexão com Wi-Fi**: O código se conecta a uma rede Wi-Fi especificada pelo usuário.
2.  **Coleta de Dados dos Sensores**: Os sensores conectados à placa ESP32 coletam dados de umidade e temperatura do ar, umidade e temperatura do solo, e luminosidade.
3.  **Cálculo de Médias e Análise**: Realiza múltiplas leituras de cada sensor, calcula a média e analisa os valores para determinar o status dos parâmetros (baixo, normal ou alto).
4.  **Envio de Dados via MQTT**: Os dados calculados são enviados ao servidor MQTT em tópicos específicos.
5.  **Interação com o Usuário**: O código permite que o usuário insira informações de culturas monitoradas e escolha o intervalo de medições.

Sensores e Pinos Utilizados
---------------------------

-   **Sensor de Umidade do Solo** - Pino 36
-   **Sensor de Luminosidade** - Pino 36
-   **Sensor de Umidade e Temperatura do Ar (DHT22)** - Pino 18
-   **Sensor de Temperatura do Solo (DS18B20)** - Pino 26

Dependências
------------

Este código utiliza as seguintes bibliotecas:

-   `WiFi.h` - Para conexão com a rede Wi-Fi.
-   `WiFiClientSecure.h` - Para comunicação segura via MQTT.
-   `PubSubClient.h` - Para conexão e publicação no servidor MQTT.
-   `DHT.h` - Para ler dados do sensor DHT22.
-   `OneWire.h` e `DallasTemperature.h` - Para leitura do sensor DS18B20.

Certifique-se de instalar essas bibliotecas no Arduino IDE antes de carregar o código na ESP32.

Estrutura do Código
-------------------

### 1\. Variáveis Globais

-   **`ssid`, `password`**: Armazenam o SSID e a senha da rede Wi-Fi.
-   **`crops[]`**: Array para armazenar os nomes das culturas monitoradas.
-   **`uuid`**: Identificador único para a aplicação.
-   **`client`**: Instância da conexão MQTT.

### 2\. Funções

#### `listWiFiNetworks()`

-   **Descrição**: Lista redes Wi-Fi disponíveis e permite que o usuário escolha a rede à qual deseja conectar.
-   **Uso**: Executado no `setup()` para configurar a conexão Wi-Fi.

#### `connectWiFi()`

-   **Descrição**: Conecta a ESP32 à rede Wi-Fi especificada pelo usuário.
-   **Uso**: Chamado após o usuário selecionar a rede Wi-Fi para estabelecer a conexão.

#### `reconnectMQTT()`

-   **Descrição**: Reestabelece a conexão com o servidor MQTT caso ela esteja desconectada.
-   **Uso**: Chamado no `setup()` e periodicamente durante o `loop()` para garantir a conectividade MQTT.

#### `sendToMQTT(String topic, String payload)`

-   **Descrição**: Publica uma mensagem JSON no servidor MQTT no tópico especificado.
-   **Parâmetros**:
    -   `topic`: Tópico MQTT para o qual enviar a mensagem.
    -   `payload`: Conteúdo da mensagem em formato JSON.
-   **Uso**: Envia as leituras médias e status dos sensores.

#### `calibrateSoilMoisture(int rawValue)`

-   **Descrição**: Converte a leitura analógica do sensor de umidade do solo em um valor percentual calibrado.
-   **Parâmetros**:
    -   `rawValue`: Valor lido do sensor de umidade do solo.
-   **Retorno**: Percentual de umidade do solo calibrado.

#### `analyzeStatus(float value, String type)`

-   **Descrição**: Determina o status (baixo, normal, alto) do valor medido com base em intervalos predefinidos.
-   **Parâmetros**:
    -   `value`: Valor medido.
    -   `type`: Tipo do dado analisado (e.g., "Umidade do Solo", "Temperatura do Solo").
-   **Retorno**: Status do valor medido.

### 3\. Funções `setup()` e `loop()`

#### `setup()`

1.  Inicializa o monitor serial e os sensores.
2.  Configura o Wi-Fi e solicita informações de culturas e UUID.
3.  Conecta ao servidor MQTT.

#### `loop()`

1.  **Coleta de Dados**: Lê os dados dos sensores em 10 intervalos, calcula a média e imprime as leituras.
2.  **Análise dos Dados**: Determina o status dos valores médios dos sensores.
3.  **Envio via MQTT**: Publica os dados médios e status nos tópicos MQTT.
4.  **Interação com o Usuário**: Pergunta ao usuário se ele deseja realizar uma nova medição imediatamente ou esperar duas horas.

Exemplo de Configuração e Uso
-----------------------------

1.  **Conectar o Hardware**: Conecte os sensores de acordo com os pinos especificados.
2.  **Configurar o Wi-Fi**: O código solicita o nome e a senha da rede Wi-Fi, e tenta se conectar.
3.  **Inserir Dados das Culturas**: Insira o nome de três culturas para monitoramento.
4.  **Monitorar Dados**: O ESP32 coletará e enviará dados ao servidor MQTT periodicamente.

🤝 Contribuição
---------------

Contribuições são bem-vindas! Siga as etapas no arquivo `CONTRIBUTING.md` para começar.

📄 Licença
----------

Este projeto está licenciado sob a Licença MIT. Veja o arquivo `LICENSE` para mais detalhes.

* * * * *

### 🏆 Arquitetado e desenvolvido por Vinicius Prudencio - VMB - Challenge FIAP 2024