# Health Plus – Edge Computing & IoT com FIWARE

Projeto acadêmico desenvolvido como continuação do sistema **Health Plus**, integrando **IoT, ESP32, sensores físicos e FIWARE via MQTT**.

A proposta é transformar a experiência digital do aplicativo em uma solução híbrida entre **software e dispositivo físico**, permitindo monitoramento e interação em tempo real com hábitos saudáveis.

---

## Descrição do Projeto

O **Health Plus IoT** é a continuação do aplicativo web de saúde e bem-estar.

Nesta etapa, o sistema utiliza **ESP32** para monitorar ações físicas do usuário e sincronizar essas informações com a plataforma digital.

O dispositivo físico simula uma **planta inteligente / tamagotchi da saúde**, que responde ao progresso do usuário por meio de sinais visuais e sonoros.

O projeto integra:

- aplicativo web
- dispositivo físico
- sensores
- display OLED
- buzzer
- botões
- comunicação MQTT
- plataforma FIWARE

---

## Objetivo

O objetivo é criar uma experiência de saúde conectada entre o mundo físico e digital.

O sistema monitora interações do usuário e envia informações para a plataforma, permitindo:

- atualização do progresso das missões
- feedback em tempo real
- alertas sonoros
- visualização de status no display
- evolução da planta virtual

---

## Conceito da Solução

A ideia do projeto é funcionar como um **assistente físico de hábitos saudáveis**, inspirado em um tamagotchi.

Exemplos de uso:

- lembrete para beber água
- alerta de meditação
- confirmação de exercício realizado
- aviso de meta concluída
- acompanhamento da evolução da planta

---

## Visão Geral do Projeto

### Foto da Simulação
<img width="503" height="412" alt="image" src="https://github.com/user-attachments/assets/ac0ad157-1c6f-4998-899e-acd925b1cc72" />

#### Link da simulação: https://wokwi.com/projects/462495996135387137

---

## Arquitetura do Projeto

<img width="744" height="1602" alt="ArquiteturaSprint drawio" src="https://github.com/user-attachments/assets/e25951a5-410f-45a3-b91c-83fa6aad8f47" />



---

## Funcionalidades

- Comunicação com **ESP32**
- Integração com **FIWARE**
- Comunicação via **MQTT**
- Exibição de mensagens no **display OLED**
- Alertas por **buzzer**
- Botões para interação do usuário
- Registro de hábitos
- Sincronização com o app web
- Feedback visual da evolução da planta

---

## Componentes Utilizados

- **ESP32**
- **Display OLED (I2C)**
- **Buzzer**
- **2 Botões**
- **Módulo sensor / comunicação**
- **Jumpers**
- **Protoboard virtual**
- **Broker MQTT**
- **FIWARE**

---

## Componentes da Simulação

### Botões
Os botões simulam interações do usuário, como:

- registrar hábito concluído
- navegar entre telas
- confirmar ações

---

### Buzzer
O buzzer é utilizado para:

- lembretes sonoros
- alarmes de hábito
- confirmação de ação
- notificações de missão completa

---

### Display OLED
O display mostra informações em tempo real, como:

- missão atual
- progresso diário
- mensagens da Bloom
- status da planta
- alertas

---

## Integração com FIWARE

O sistema envia e recebe dados utilizando **MQTT** integrado ao **FIWARE**.

Exemplos de dados transmitidos:

- status do usuário
- missão concluída
- progresso diário
- notificações
- evolução da planta

---

## Tópicos MQTT

```text
Subscribe: /TEF/healthplus/cmd
Publish: /TEF/healthplus/attrs
Status: /TEF/healthplus/status
Alertas: /TEF/healthplus/alert
```

---

## Fluxo do Sistema

```text
Usuário interage no botão
        ↓
ESP32 processa
        ↓
Display atualiza
        ↓
MQTT publica evento
        ↓
FIWARE recebe dados
        ↓
Aplicativo web atualiza progresso
        ↓
Planta evolui
```

---

## Como Reproduzir o Projeto

### 1. Monte o circuito
Monte conforme a simulação do Wokwi / imagem do projeto.

### 2. Abra o código
Abra o projeto na **Arduino IDE** ou no **Wokwi**.

### 3. Instale as bibliotecas
```cpp
WiFi.h
PubSubClient.h
Wire.h
Adafruit_GFX.h
Adafruit_SSD1306.h
```

### 4. Faça upload para o ESP32
Envie o código para a placa.

### 5. Configure Wi-Fi e MQTT
Defina no código:

- SSID
- senha
- broker
- tópicos MQTT

### 6. Execute
Interaja com os botões e acompanhe os dados no display e na plataforma.

---

## Tecnologias Utilizadas

- **ESP32**
- **C++ / Arduino**
- **MQTT**
- **FIWARE**
- **Wokwi**
- **IoT**
- **Edge Computing**

---

## Equipe de Desenvolvimento

| RM | Nome |
|----|------|
| RM 567947 | Lara Mofid Essa Alssabak |
| RM 567355 | Maria Luisa Boucinhas Franco |
| RM 568459 | Maria Luiza Kochnoff da Matta |
| RM 567825 | Roberta Moreira dos Santos |

---

## Objetivo Acadêmico

Este projeto foi desenvolvido com foco em:

- Edge Computing
- IoT
- comunicação entre hardware e software
- integração com FIWARE
- MQTT
- sistemas embarcados
- experiência do usuário



