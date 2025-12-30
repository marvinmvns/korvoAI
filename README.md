# Korvo AI - Assistente de Voz com ESP32

Este projeto implementa um assistente de voz completo utilizando a placa de desenvolvimento **ESP32-Korvo V1.1**. Ele integra reconhecimento de fala (OpenAI Whisper via WebSocket), inteligência artificial conversacional (OpenAI GPT) e síntese de voz natural (ElevenLabs) para criar uma experiência de interação fluida e responsiva.

![ESP32-Korvo V1.1](https://github.com/espressif/esp-skainet/raw/master/docs/_static/esp32-korvo-v1.1-front-view.png)

## Funcionalidades

*   **Reconhecimento de Voz (STT):** Captura áudio em tempo real e transcreve usando a API da OpenAI (Whisper).
*   **Inteligência Artificial (LLM):** Processa o texto transcrito com modelos GPT para gerar respostas inteligentes e contextuais.
*   **Síntese de Voz (TTS):** Converte a resposta da IA em áudio de alta qualidade usando a API da ElevenLabs.
*   **Feedback Visual:** Utiliza o anel de LEDs RGB da placa para indicar os estados do assistente (Ouvindo, Processando, Falando).
*   **Gerenciamento de Configuração:** Portal WiFi (WiFiManager) para configuração fácil de credenciais WiFi e chaves de API.
*   **Cancelamento de Eco (Básico):** Lógica de software para evitar que o assistente "ouça a si mesmo" durante a resposta.

## Hardware Necessário

*   **Placa de Desenvolvimento:** [ESP32-Korvo V1.1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md)
    *   Módulo ESP32-WROVER-E (16MB Flash / 8MB PSRAM)
    *   Codec de Áudio ES8311 + ADC ES7210
    *   Array de Microfones (3 mic)
    *   Saída para Speaker (3W) e Fone de Ouvido
    *   12 LEDs RGB (WS2812)
    *   Botões de função
*   **Cabo USB:** Para alimentação e programação.
*   **Speaker:** Conectado à saída de speaker da placa (conector J12).

## Estrutura do Projeto

*   `src/`
    *   `main.cpp`: Lógica principal, máquina de estados e loop de controle.
    *   `AudioManager`: Gerencia captura de áudio (I2S) e hardware de som (ES8311/ES7210).
    *   `TranscriptionClient`: Cliente WebSocket para envio de áudio para a OpenAI.
    *   `LLMClient`: Cliente HTTP para chat com a OpenAI (GPT).
    *   `ElevenLabsStreamClient`: Cliente de streaming para TTS da ElevenLabs.
    *   `ConfigManager`: Gerencia preferências e portal WiFi.
    *   `AudioRingBuffer`: Buffer circular para áudio em PSRAM.

## Configuração e Instalação

### Pré-requisitos
*   [PlatformIO](https://platformio.org/) (Extensão VS Code recomendada).
*   Chave de API da **OpenAI**.
*   Chave de API da **ElevenLabs** e um **Voice ID**.

### Passo a Passo

1.  **Clonar o Repositório:**
    ```bash
    git clone https://github.com/marvinmvns/korvoAI.git
    cd korvoAI
    ```

2.  **Compilar e Upload:**
    *   Conecte a placa via USB.
    *   Abra o projeto no PlatformIO.
    *   Selecione o ambiente `env:esp32-korvo-v1_1`.
    *   Clique em "Upload".

3.  **Configuração Inicial (WiFiManager):**
    *   Ao iniciar pela primeira vez (ou se não houver WiFi configurado), a placa criará um ponto de acesso chamado **`KORVO_Setup`**.
    *   Conecte-se a essa rede com seu celular ou computador.
    *   Acesse `192.168.4.1` no navegador.
    *   Configure:
        *   Sua rede WiFi e senha.
        *   **OpenAI Key:** Sua chave da OpenAI (`sk-...`).
        *   **ElevenLabs Key:** Sua chave da ElevenLabs (`sk_...`).
        *   **Voice ID:** ID da voz desejada na ElevenLabs.

4.  **Uso:**
    *   O anel de LEDs indicará o status:
        *   **Verde:** Pronto / Ocioso.
        *   **Ciano (Pulsando):** Ouvindo (detectou voz).
        *   **Laranja:** Processando (aguardando IA).
        *   **Arco-íris:** Falando (reproduzindo resposta).
    *   Fale naturalmente com o assistente. O sistema detecta automaticamente quando você começa e para de falar (VAD).

## Botões e Controles

A placa possui botões que podem ser usados para controle manual (configuração atual):

*   **REC (Gravar):** Se mantido pressionado durante a inicialização, reseta as configurações (WiFi e Keys) e reinicia em modo AP.
*   **MODE:** Alterna modos (função exemplo no código atual).

## Detalhes Técnicos

*   **Microfone:** O sistema utiliza os microfones integrados via ADC ES7210.
*   **Speaker:** O áudio é reproduzido via DAC ES8311.
*   **PSRAM:** O uso de PSRAM é **obrigatório** devido aos buffers de áudio grandes necessários para streaming fluido.

## Créditos e Referências

*   Baseado no hardware ESP32-Korvo da Espressif.
*   Bibliotecas utilizadas: ArduinoJson, WebSockets, WiFiManager, FastLED.

---
Desenvolvido por Marvin.