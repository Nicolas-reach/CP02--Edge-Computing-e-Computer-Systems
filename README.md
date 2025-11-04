# 🍷 Vinheria Agnello: Monitor de Adega e Alerta de Clima

## 📋 Descrição do Projeto

Este projeto Arduino monitora as condições críticas de **temperatura** e **umidade** dentro de uma adega ou ambiente de armazenamento de vinhos.  
Utiliza um **sensor DHT22 (ou DHT11)** para leitura de dados, um **Módulo RTC DS1307** para data/hora, e exibe as informações em um **LCD 16x2**, com **LEDs indicadores** e **buzzer de alarme**.  
O sistema também **registra automaticamente na EEPROM** os eventos em que as condições de alarme são atingidas.
---
## Integrantes
- **Nicolas Forcione de Oliveira e Souza** – RM566998  
- **Alexandre Constantino Furtado Junior** – RM567188  
- **Enrico Dellatorre da Fonseca** – RM566824  
- **Leonardo Batista de Souza** – RM568558  
- **Matheus Freitas dos Santos** – RM567337  

---
## Imagem 
<img width="693" height="575" alt="Image" src="https://github.com/user-attachments/assets/c8764310-44a8-41d9-8602-8e2e998d6012" />

---
## Vídeo

https://github.com/user-attachments/assets/44fdc6ca-dad8-475e-a781-ae2e077a3b42

---

## ✨ Funcionalidades Principais

- **Monitoramento em Tempo Real:** Leitura e exibição contínua de temperatura (°C) e umidade (%).  
- **Relógio em Tempo Real (RTC):** Exibição precisa de data e hora (fuso UTC-3).  
- **Sistema de Alerta em 3 Níveis:**
  - 🟢 **Verde (Normal):** Condições seguras.  
  - 🟡 **Amarelo (Atenção):** Condições próximas aos limites.  
  - 🔴 **Vermelho (Alarme):** Condições críticas, com aviso sonoro.  
- **Registro de Eventos Críticos (EEPROM):** Grava data/hora e valores de temperatura/umidade ao atingir nível vermelho.  
- **Detecção de Falha:** Alerta visual e sonoro caso o sensor DHT apresente erro de leitura.

---

## 🛠️ Hardware Necessário

| Componente                     | Função                                 |
|--------------------------------|----------------------------------------|
| Arduino Uno (ou compatível)    | Controlador principal                  |
| Módulo RTC DS1307              | Relógio de tempo real                  |
| Sensor DHT22 (ou DHT11)        | Leitura de temperatura e umidade       |
| Display LCD 16x2 (I2C 0x27)    | Exibição dos dados                     |
| LED Verde, Amarelo e Vermelho  | Indicação de status                    |
| Buzzer (passivo ou ativo)      | Alerta sonoro                          |
| Resistores                     | Proteção dos LEDs e sensor             |

---

## 📌 Ligações (Pinos do Arduino)

| Componente      | Pino Arduino | Função               |
|-----------------|---------------|----------------------|
| DHT (Data)      | D2            | Leitura T/U          |
| LED Amarelo     | D3            | Sinal de Atenção     |
| LED Verde       | D4            | Sinal Normal         |
| LED Vermelho    | D5            | Sinal de Alarme      |
| Buzzer          | D6            | Alerta Sonoro        |
| RTC/LCD (SDA)   | A4 (SDA)      | Comunicação I2C      |
| RTC/LCD (SCL)   | A5 (SCL)      | Comunicação I2C      |

---

## 📚 Bibliotecas Necessárias

Instale as seguintes bibliotecas na IDE do Arduino:

- [`LiquidCrystal_I2C`](https://github.com/johnrickman/LiquidCrystal_I2C)
- [`RTClib`](https://github.com/adafruit/RTClib)
- `Wire` *(já incluída na IDE do Arduino)*
- [`DHT sensor library`](https://github.com/adafruit/DHT-sensor-library)
- `EEPROM` *(já incluída na IDE do Arduino)*

---

## ⚙️ Configurações e Limites (Constantes)

As configurações podem ser alteradas diretamente nas constantes do código:

| Constante         | Valor Padrão | Descrição |
|-------------------|--------------|------------|
| `UTC_OFFSET`      | -3 | Fuso horário (UTC-3 = Brasília) |
| `SERIAL_OPTION`   | 1 | Ativa comunicação serial (0 = off) |
| `DHTTYPE`         | DHT22 | Tipo de sensor DHT |
| `trigger_t_min`   | 14.0 | Temperatura mínima crítica |
| `trigger_t_max`   | 30.0 | Temperatura máxima crítica |
| `trigger_u_min`   | 50.0 | Umidade mínima crítica |
| `trigger_u_max`   | 85.0 | Umidade máxima crítica |
| `warning_t_min`   | 16.0 | Limite de atenção (mínimo) |
| `warning_t_max`   | 29.0 | Limite de atenção (máximo) |
| `warning_u_min`   | 55.0 | Umidade de atenção (mínima) |
| `warning_u_max`   | 80.0 | Umidade de atenção (máxima) |

---

## 🕒 Configuração do RTC

Durante a primeira execução, ative a linha abaixo no `setup()` para ajustar a data e hora:

```cpp
RTC.adjust(DateTime(2025, 11, 1, 17, 45, 0));
