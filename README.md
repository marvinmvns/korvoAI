# Atom Echo Voice Assistant

[![PlatformIO](https://img.shields.io/badge/PlatformIO-5.0+-orange?logo=platformio)](https://platformio.org/)
[![ESP32](https://img.shields.io/badge/ESP32--PICO-blue?logo=espressif)](https://www.espressif.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![OpenAI](https://img.shields.io/badge/OpenAI-Whisper%20%2B%20GPT--4o-412991?logo=openai)](https://openai.com/)
[![ElevenLabs](https://img.shields.io/badge/ElevenLabs-TTS-black)](https://elevenlabs.io/)

Assistente de voz standalone rodando no M5Stack Atom Echo. Pressione o botão, fale, e receba uma resposta por voz.

<p align="center">
  <img src="https://static-cdn.m5stack.com/resource/docs/products/atom/atomecho/atomecho_01.webp" width="300" alt="M5Stack Atom Echo">
</p>

> **[English version below](#english-version)**

---

## Como Funciona

1. **Pressione e segure o botão** → LED fica azul, microfone começa a gravar
2. **Solte o botão** → Áudio é enviado para o Whisper (OpenAI) para transcrição
3. **Texto transcrito vai pro GPT-4o-mini** → Gera uma resposta curta
4. **Resposta vai pro ElevenLabs** → Converte texto em voz
5. **Atom Echo reproduz o áudio** → LED fica magenta durante reprodução

Tudo acontece via streaming quando possível, então a latência é razoável considerando que é um ESP32.

## Hardware

- **M5Stack Atom Echo** (ESP32-PICO com microfone PDM e speaker integrados)

Só isso. Não precisa de nada mais.

## Requisitos

- [PlatformIO](https://platformio.org/) (CLI ou extensão do VS Code)
- Conta na [OpenAI](https://platform.openai.com/) com API key
- Conta na [ElevenLabs](https://elevenlabs.io/) com API key
- Rede WiFi 2.4GHz

## Instalação

```bash
# Clone o repositório
git clone https://github.com/seu-usuario/atomecho.git
cd atomecho

# Compile e faça upload
pio run --target upload

# Monitore a serial (opcional, mas útil)
pio device monitor
```

## Configuração

Na primeira vez (ou após reset), o Atom Echo cria uma rede WiFi chamada **AtomEcho_Setup**.

1. Conecte nessa rede pelo celular ou computador
2. Um portal de configuração abre automaticamente
3. Preencha:
   - SSID e senha da sua rede WiFi
   - OpenAI API Key
   - ElevenLabs API Key
   - Voice ID do ElevenLabs (opcional, padrão: Rachel)

As configurações ficam salvas na memória flash do ESP32.

### Vozes do ElevenLabs

O Voice ID padrão é `21m00Tcm4TlvDq8ikWAM` (Rachel). Funciona bem com português. Pra usar outra voz:

1. Vá em **Voices** no painel do ElevenLabs
2. Clique na voz desejada
3. Copie o **Voice ID** (não é o nome, é um hash tipo `21m00Tcm4TlvDq8ikWAM`)

### Keys Hardcoded (Alternativa)

Se preferir não usar o portal, defina as chaves direto no código. Edite `src/ConfigManager.cpp`:

```cpp
#define FORCE_OPENAI_KEY "sk-proj-sua-chave-aqui"
#define FORCE_ELEVEN_KEY "sua-chave-elevenlabs"
#define FORCE_VOICE_ID "id-da-voz"
```

## Uso

| Ação | Resultado |
|------|-----------|
| Pressionar e segurar o botão | Inicia gravação (LED azul) |
| Soltar o botão | Processa e responde |
| Pressionar durante reprodução | Interrompe o áudio |
| Segurar botão no boot | Factory reset |

### LEDs

| Cor | Estado |
|-----|--------|
| Vermelho | Inicializando / Erro |
| Verde | Pronto |
| Azul | Gravando |
| Magenta | Reproduzindo áudio |

## Arquitetura

```
main.cpp
├── ConfigManager      → WiFi + credenciais (WiFiManager + Preferences)
├── AudioManager       → Captura de áudio via I2S (PDM)
├── OpenAIClient       → Whisper (STT) + GPT-4o-mini (chat)
└── ElevenLabsClient   → TTS streaming
    └── AudioFileSourceHTTPSPost → Stream HTTP customizado para ESP8266Audio
```

### Streaming de TTS

O áudio do ElevenLabs é reproduzido em streaming direto do HTTP response. Não baixa o arquivo todo antes de tocar. Isso economiza RAM (que é escassa no ESP32) e reduz a latência percebida.

A classe `AudioFileSourceHTTPSPost` é um adapter customizado que implementa a interface do ESP8266Audio para fazer POST requests HTTPS e streamar a resposta.

### Gerenciamento de Memória

O ESP32-PICO tem ~320KB de RAM, então o código é cuidadoso com alocações:

- Buffers de áudio são alocados dinamicamente e liberados logo após uso
- O histórico de conversa é podado automaticamente quando a memória fica baixa
- Objetos de áudio são criados/destruídos a cada interação

## Dependências

Gerenciadas automaticamente pelo PlatformIO:

| Biblioteca | Função |
|------------|--------|
| `M5Atom` | SDK do hardware |
| `FastLED` | Controle do LED RGB |
| `WiFiManager` | Portal de configuração WiFi |
| `ArduinoJson` | Parsing de JSON |
| `ESP8266Audio` | Reprodução de áudio (MP3, streaming) |

## Problemas Comuns

**LED vermelho após boot**
- Verifique as credenciais no Serial Monitor
- Confirme que a rede WiFi é 2.4GHz (ESP32 não suporta 5GHz)

**Áudio cortado ou falhando**
- Heap pode estar baixo. Respostas muito longas consomem mais memória
- O modelo está configurado pra respostas curtas (max 150 tokens)

**"Error: Connection"**
- Problema de rede. Verifique o WiFi
- APIs da OpenAI/ElevenLabs podem estar instáveis

**Factory reset**
- Segure o botão enquanto conecta o USB
- LED fica branco, configurações são apagadas
- Reinicia automaticamente no modo de setup

## Limitações

- Gravação máxima de 10 segundos por vez
- Só funciona com WiFi (não tem modo offline)
- Respostas limitadas a ~2 frases (proposital, pra caber na memória)
- Não suporta conversas muito longas (histórico é podado)

## Custos de API

Valores aproximados por interação:
- **Whisper**: ~$0.006/minuto de áudio
- **GPT-4o-mini**: ~$0.00015/1K tokens input, $0.0006/1K output
- **ElevenLabs**: Depende do plano (tem tier gratuito com limite)

Uma interação típica custa menos de $0.01.

---

# English Version

Standalone voice assistant running on M5Stack Atom Echo. Press the button, speak, get a voice response.

## How It Works

1. **Press and hold the button** → LED turns blue, microphone starts recording
2. **Release the button** → Audio is sent to Whisper (OpenAI) for transcription
3. **Transcribed text goes to GPT-4o-mini** → Generates a short response
4. **Response goes to ElevenLabs** → Converts text to speech
5. **Atom Echo plays the audio** → LED turns magenta during playback

Everything streams when possible, so latency is reasonable for an ESP32.

## Hardware

- **M5Stack Atom Echo** (ESP32-PICO with built-in PDM microphone and speaker)

That's it. Nothing else needed.

## Requirements

- [PlatformIO](https://platformio.org/) (CLI or VS Code extension)
- [OpenAI](https://platform.openai.com/) account with API key
- [ElevenLabs](https://elevenlabs.io/) account with API key
- 2.4GHz WiFi network

## Installation

```bash
# Clone the repository
git clone https://github.com/your-username/atomecho.git
cd atomecho

# Build and upload
pio run --target upload

# Monitor serial output (optional but useful)
pio device monitor
```

## Configuration

On first boot (or after reset), Atom Echo creates a WiFi network called **AtomEcho_Setup**.

1. Connect to this network from your phone or computer
2. A configuration portal opens automatically
3. Fill in:
   - Your WiFi SSID and password
   - OpenAI API Key
   - ElevenLabs API Key
   - ElevenLabs Voice ID (optional, defaults to Rachel)

Settings are stored in the ESP32's flash memory.

### ElevenLabs Voices

Default Voice ID is `21m00Tcm4TlvDq8ikWAM` (Rachel). Works well with multiple languages. To use a different voice:

1. Go to **Voices** in the ElevenLabs dashboard
2. Click on your desired voice
3. Copy the **Voice ID** (it's a hash like `21m00Tcm4TlvDq8ikWAM`, not the name)

### Hardcoded Keys (Alternative)

If you prefer not to use the portal, define keys directly in code. Edit `src/ConfigManager.cpp`:

```cpp
#define FORCE_OPENAI_KEY "sk-proj-your-key-here"
#define FORCE_ELEVEN_KEY "your-elevenlabs-key"
#define FORCE_VOICE_ID "voice-id"
```

## Usage

| Action | Result |
|--------|--------|
| Press and hold button | Start recording (blue LED) |
| Release button | Process and respond |
| Press during playback | Stop audio |
| Hold button on boot | Factory reset |

### LED Status

| Color | State |
|-------|-------|
| Red | Initializing / Error |
| Green | Ready |
| Blue | Recording |
| Magenta | Playing audio |

## Architecture

```
main.cpp
├── ConfigManager      → WiFi + credentials (WiFiManager + Preferences)
├── AudioManager       → Audio capture via I2S (PDM)
├── OpenAIClient       → Whisper (STT) + GPT-4o-mini (chat)
└── ElevenLabsClient   → TTS streaming
    └── AudioFileSourceHTTPSPost → Custom HTTP stream for ESP8266Audio
```

### TTS Streaming

ElevenLabs audio is played directly from the HTTP response stream. It doesn't download the entire file before playing. This saves RAM (which is scarce on ESP32) and reduces perceived latency.

The `AudioFileSourceHTTPSPost` class is a custom adapter that implements the ESP8266Audio interface to make HTTPS POST requests and stream the response.

### Memory Management

ESP32-PICO has ~320KB of RAM, so the code is careful with allocations:

- Audio buffers are dynamically allocated and freed immediately after use
- Conversation history is automatically pruned when memory runs low
- Audio objects are created/destroyed on each interaction

## Dependencies

Managed automatically by PlatformIO:

| Library | Purpose |
|---------|---------|
| `M5Atom` | Hardware SDK |
| `FastLED` | RGB LED control |
| `WiFiManager` | WiFi configuration portal |
| `ArduinoJson` | JSON parsing |
| `ESP8266Audio` | Audio playback (MP3, streaming) |

## Troubleshooting

**Red LED after boot**
- Check credentials in Serial Monitor
- Confirm WiFi is 2.4GHz (ESP32 doesn't support 5GHz)

**Audio cutting out or failing**
- Heap might be low. Long responses consume more memory
- Model is configured for short responses (max 150 tokens)

**"Error: Connection"**
- Network issue. Check WiFi
- OpenAI/ElevenLabs APIs might be unstable

**Factory reset**
- Hold button while plugging in USB
- LED turns white, settings are erased
- Automatically restarts in setup mode

## Limitations

- Maximum 10 seconds recording per interaction
- WiFi only (no offline mode)
- Responses limited to ~2 sentences (intentional, to fit in memory)
- Doesn't support very long conversations (history is pruned)

## API Costs

Approximate values per interaction:
- **Whisper**: ~$0.006/minute of audio
- **GPT-4o-mini**: ~$0.00015/1K input tokens, $0.0006/1K output tokens
- **ElevenLabs**: Depends on plan (has free tier with limits)

A typical interaction costs less than $0.01.

## License

MIT
# atomechogptelevenlabs
# korvoAI
