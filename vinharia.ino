#include <LiquidCrystal_I2C.h>  // Biblioteca para LCD I2C
#include <RTClib.h>             // Biblioteca para Relógio em Tempo Real
#include <Wire.h>               // Biblioteca para comunicação I2C
#include <EEPROM.h>
#include "DHT.h"

//  Configurações de Opções 
#define LOG_OPTION 0     // Opção para LER o log na inicialização (1=Sim, 0=Não)
#define SERIAL_OPTION 1  // Opção de comunicação serial: 0 para desligado, 1 para ligado
#define UTC_OFFSET -3    // Ajuste de fuso horário para UTC-3 (Horário de Brasília)

// Configurações dos Pinos 
#define DHTPIN 2         // Pino do sensor DHT
#define RED_LED_PIN 5    // Pino para o LED Vermelho (Alarme)
#define YELLOW_LED_PIN 3 // Pino para o LED Amarelo (Atenção)
#define GREEN_LED_PIN 4  // Pino para o LED Verde (Normal)
#define BUZZER_PIN 6     // Pino para o Buzzer

// Configuração do Sensor
#define DHTTYPE DHT22    // ALTERAR para DHT11
DHT dht(DHTPIN, DHTTYPE);

// Configuração dos Periféricos I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS1307 RTC;

// Configurações da EEPROM
const int maxRecords = 100;
const int recordSize = 8;
int endAddress = maxRecords * recordSize;
int currentAddress = 0;
int lastLoggedMinute = -1;

// Alarme (Vermelho)
float trigger_t_min = 14.0; // Mínimo de temperatura
float trigger_t_max = 30.0; // Máximo de temperatura
float trigger_u_min = 50.0; // Mínimo de umidade
float trigger_u_max = 85.0; // Máximo de umidade

// Atenção (Amarelo)
// Define uma "zona segura" dentro dos limites.
// Fora dela, mas antes do alarme, o LED amarelo acende.
float warning_t_min = trigger_t_min + 2.0; // Ex: 16.0
float warning_t_max = trigger_t_max - 1.0; // Ex: 29.0
float warning_u_min = trigger_u_min + 5.0; // Ex: 55.0
float warning_u_max = trigger_u_max - 5.0; // Ex: 80.0

// --- Variáveis de Controle de Alarme ---
// 0 = Normal (Verde), 1 = Atenção (Amarelo), 2 = Alarme (Vermelho)
int lastAlarmState = 0;

// Logo
byte wineGlass[8] = {
  B11101,
  B10101,
  B10001,
  B01110,
  B00100,
  B00100,
  B01110,
  B11111
};

void setup() {
  // Configura os pinos dos LEDs e Buzzer como saída
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // Começa com o LED Verde ligado (supõe normal) e buzzer desligado
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(YELLOW_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, HIGH);
  noTone(BUZZER_PIN);
  lastAlarmState = 0; // Define o estado inicial como Normal

  dht.begin();
  Serial.begin(9600);

  lcd.init();      // Inicialização do LCD
  lcd.backlight(); // Ligando o backlight do LCD

  // Exibe a Tela de Inicialização
  lcd.createChar(0, wineGlass); // Salva o caractere customizado na memória 0
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Vinheria Agnello");
  lcd.setCursor(7, 1); // Posição central da segunda linha
  lcd.write(byte(0));  // Desenha a taça
  delay(3000);         // Mostra a tela por 3 segundos
  lcd.clear();

  RTC.begin(); // Inicialização do Relógio em Tempo Real
  EEPROM.begin();

  //  Ajuste do Relógio (RTC)
  // ele ajusta o relógio para a data e hora especificadas.
  if (!RTC.isrunning()) {
    Serial.println("RTC não está rodando! Ajustando hora...");
    // Ajusta para 01 de Novembro de 2025, 14:45 (Horário Local UTC-3)
    // O RTC usa UTC, então somamos 3 horas (14:45 + 3h = 17:45)
    // Formato: Ano, Mês, Dia, Hora (UTC), Minuto, Segundo
    RTC.adjust(DateTime(2025, 11, 1, 14, 48, 0));
  }


  // Se a opção de log estiver ativa, imprime o log na inicialização
  if (LOG_OPTION) {
    get_log();
  }
}

void loop() {
  // Leitura de Data e Hora com Fuso
  DateTime now = RTC.now();
  int offsetSeconds = UTC_OFFSET * 3600;
  DateTime adjustedTime = DateTime(now.unixtime() + offsetSeconds);

  // --- Leitura dos Sensores ---
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  //  Verificação de Falha na Leitura 
  if (isnan(humidity) || isnan(temperature)) {
    if (SERIAL_OPTION) {
      Serial.println("Falha ao ler o sensor DHT!");
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Falha no Sensor");
    lcd.setCursor(0, 1);
    lcd.print("Verificar DHT22!"); // ALTERADO para DHT22
    
    // Aciona o alarme de falha (Pisca Vermelho e bipa)
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(RED_LED_PIN, HIGH);
    tone(BUZZER_PIN, 1000);
    delay(500);
    digitalWrite(RED_LED_PIN, LOW);
    noTone(BUZZER_PIN);
    delay(500);
    return; // Pula o resto do loop e tenta de novo
  }

  //  Atualiza os Alarmes (LEDs/Buzzer) 
  updateAlarms(temperature, humidity);

  //  Lógica de Log (1 vez por minuto) 
  if (adjustedTime.minute() != lastLoggedMinute) {
    lastLoggedMinute = adjustedTime.minute();

    // Verificar se os valores estão FORA dos triggers de ALARME (Vermelho)
    if (temperature < trigger_t_min || temperature > trigger_t_max || humidity < trigger_u_min || humidity > trigger_u_max) {

      // Converter valores para int para armazenamento
      int tempInt = (int)(temperature * 100);
      int humiInt = (int)(humidity * 100);

      // Escrever dados na EEPROM
      // Usamos now.unixtime() (tempo UTC puro) para o log, para ser consistente
      EEPROM.put(currentAddress, now.unixtime());
      EEPROM.put(currentAddress + 4, tempInt);
      EEPROM.put(currentAddress + 6, humiInt);

      // Atualiza o endereço para o próximo registro
      getNextAddress();
    }
  }

  // Comunicação Serial 
  if (SERIAL_OPTION) {
    Serial.print(adjustedTime.day());
    Serial.print("/");
    Serial.print(adjustedTime.month());
    Serial.print("/");
    Serial.print(adjustedTime.year());
    Serial.print(" ");
    Serial.print(adjustedTime.hour() < 10 ? "0" : "");
    Serial.print(adjustedTime.hour());
    Serial.print(":");
    Serial.print(adjustedTime.minute() < 10 ? "0" : "");
    Serial.print(adjustedTime.minute());
    Serial.print(":");
    Serial.print(adjustedTime.second() < 10 ? "0" : "");
    Serial.print(adjustedTime.second());
    Serial.print(" | T: ");
    Serial.print(temperature);
    Serial.print("C | U: ");
    Serial.print(humidity);
    Serial.println("%");
  }

  //  Display LCD
  
  if (lastAlarmState == 2) {
      // ESTADO CRÍTICO (Vermelho) - Estático (não alterna)
      
      lcd.clear(); // Limpa o LCD
      
      // Determina a frase crítica e o valor
      bool tempHigh = temperature > trigger_t_max;
      bool tempLow = temperature < trigger_t_min;
      bool humiHigh = humidity > trigger_u_max;
      bool humiLow = humidity < trigger_u_min;
      
      String criticalPhrase = "ALARME! Verif."; 
      String criticalValueLine = "";            
      
      if (tempHigh) {
          criticalPhrase = "TEMP. MUITO ALTA";
          criticalValueLine = "T: " + String(temperature, 1) + "C CRITICO";
      } else if (tempLow) {
          criticalPhrase = "TEMP. MUITO BAIXA";
          criticalValueLine = "T: " + String(temperature, 1) + "C CRITICO";
      } else if (humiHigh) {
          criticalPhrase = "UMIDADE ALTA!";
          criticalValueLine = "U: " + String(humidity, 1) + "% CRITICO";
      } else if (humiLow) {
          criticalPhrase = "UMIDADE BAIXA!";
          criticalValueLine = "U: " + String(humidity, 1) + "% CRITICO";
      } else {
          criticalPhrase = "ALERTA MAXIMO";
          criticalValueLine = "Verifique os limites";
      }

      // Imprime a frase na Linha 0 (sobrescrevendo a data/hora)
      lcd.setCursor(0, 0);
      lcd.print(criticalPhrase);
      // Limpa o resto da linha 0
      for(int i = criticalPhrase.length(); i < 16; i++) {
        lcd.print(" ");
      }

      // Imprime o valor na Linha 1
      lcd.setCursor(0, 1);
      lcd.print(criticalValueLine);
      // Limpa o resto da linha 1
      for(int i = criticalValueLine.length(); i < 16; i++) {
        lcd.print(" ");
      }
      
  } else if (lastAlarmState == 1) {
      // ATENÇÃO (Amarelo) - Substitui data/hora por "ATENÇÃO!"
      
      // Linha 0: Mensagem de ATENÇÃO
      lcd.setCursor(0, 0);
      lcd.print("!! ATENCAO !!");
      lcd.print("     "); // Limpa o resto da linha

      // Linha 1: T e U (Sem status extra)
      lcd.setCursor(0, 1);
      lcd.print("T:");
      lcd.print(temperature, 1);
      lcd.print("C U:");
      lcd.print(humidity, 1);
      lcd.print("%             "); // IMPRESSÃO LIMPA, SEM STATUS EXTRA "ATENCAO"
      
  } else {
      // NORMAL (Verde) - Mostra Data/Hora/T/U (Sem status extra)

      // Linha 0: Data e Hora
      lcd.setCursor(0, 0);
      lcd.print(adjustedTime.day() < 10 ? "0" : "");
      lcd.print(adjustedTime.day());
      lcd.print("/");
      lcd.print(adjustedTime.month() < 10 ? "0" : "");
      lcd.print(adjustedTime.month());
      lcd.print("/");
      lcd.print(String(adjustedTime.year()).substring(2)); // Mostra só 2 dígitos do ano
      lcd.print(" ");
      lcd.print(adjustedTime.hour() < 10 ? "0" : "");
      lcd.print(adjustedTime.hour());
      lcd.print(":");
      lcd.print(adjustedTime.minute() < 10 ? "0" : "");
      lcd.print(adjustedTime.minute());
      lcd.print("  "); // Limpa o final da linha
      
      // Linha 1: T e U (Sem status extra)
      lcd.setCursor(0, 1);
      lcd.print("T:");
      lcd.print(temperature, 1);
      lcd.print("C U:");
      lcd.print(humidity, 1);
      lcd.print("%             "); // IMPRESSÃO LIMPA, SEM STATUS EXTRA "OK"
  }

  delay(1000); // Aguarda 1 segundo antes de repetir o loop
}


void updateAlarms(float t, float h) {
  int currentAlarmState;

  // 1. Determina o estado atual
  // Condição de ALARME (Vermelho)
  if (t < trigger_t_min || t > trigger_t_max || h < trigger_u_min || h > trigger_u_max) {
    currentAlarmState = 2; // Estado 2 = Vermelho
  }
  // Condição NORMAL (Verde)
  else if (t >= warning_t_min && t <= warning_t_max && h >= warning_u_min && h <= warning_u_max) {
    currentAlarmState = 0; // Estado 0 = Verde
  }
  // Condição de ATENÇÃO (Amarelo)
  else {
    currentAlarmState = 1; // Estado 1 = Amarelo
  }

  // 2. Atua com base no estado atual e no estado anterior
  switch (currentAlarmState) {
    case 0: // NORMAL (Verde)
      digitalWrite(GREEN_LED_PIN, HIGH);
      digitalWrite(YELLOW_LED_PIN, LOW);
      digitalWrite(RED_LED_PIN, LOW);
      noTone(BUZZER_PIN);
      break;

    case 1: // ATENÇÃO (Amarelo)
      digitalWrite(GREEN_LED_PIN, LOW);
      digitalWrite(YELLOW_LED_PIN, HIGH);
      digitalWrite(RED_LED_PIN, LOW);

      // Se ACABOU de entrar no estado Amarelo (vindo do Verde)
      if (lastAlarmState == 0) {
        tone(BUZZER_PIN, 500); // Bipa para chamar a atenção
      } else {
        // Se já estava em Amarelo (ou vindo do Vermelho), fica em silêncio.
        // O bip de 1 seg é garantido pelo delay(1000) no loop principal.
        noTone(BUZZER_PIN);
      }
      break;

    case 2: // ALARME (Vermelho)
      digitalWrite(GREEN_LED_PIN, LOW);
      digitalWrite(YELLOW_LED_PIN, LOW);
      digitalWrite(RED_LED_PIN, HIGH);
      tone(BUZZER_PIN, 1000); // Buzzer contínuo
      break;
  }

  // Salva o estado atual para a próxima verificação
  lastAlarmState = currentAlarmState;
}


// Funções Auxiliares 

void getNextAddress() {
  currentAddress += recordSize;
  if (currentAddress >= endAddress) {
    currentAddress = 0; // Volta para o começo (log circular)
  }
}

void get_log() {
  Serial.println("--- Log Armazenado na EEPROM ---");
  Serial.println("Timestamp (UTC)\t\tTemp (C)\tUmid (%)");

  // CORREÇÃO: 'startAddress' não estava definido, substituído por 0
  for (int address = 0; address < endAddress; address += recordSize) {
    long timeStamp;
    int tempInt, humiInt;

    EEPROM.get(address, timeStamp);
    EEPROM.get(address + 4, tempInt);
    EEPROM.get(address + 6, humiInt);

    float temperature = tempInt / 100.0;
    float humidity = humiInt / 100.0;

    // 0xFFFFFFFF é o valor de bytes vazios na EEPROM
    if (timeStamp != 0xFFFFFFFF && timeStamp != 0) {
      DateTime dt = DateTime(timeStamp);
      Serial.print(dt.timestamp(DateTime::TIMESTAMP_FULL)); // Formato ISO 8601
      Serial.print("\t");
      Serial.print(temperature);
      Serial.print("\t\t");
      Serial.print(humidity);
      Serial.println();
    }
  }
  Serial.println("--- Fim do Log ---");
}