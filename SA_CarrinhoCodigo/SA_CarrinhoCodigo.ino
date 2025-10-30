#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define FRENTE 32  // GPIO23 pin connected to the button
#define ESQUERDA 34    // GPIO26 pin connected to the LED
#define DIREITA 35
#define TRAS 33

// Variables for JSON
char mensagemJson[100];
char mensagemPotJson[100];

// Variables for the button
int lastState = HIGH;  // Previous state of the input pin
int currentState;      // Current state of the input pin
int leituraPot;
int saidaLED;

// Replace with your network credentials
const char* ssid = "senai";
const char* password = "senai123";

// Replace with your Wnology device credentials
const char* deviceID = "67e6da63952b442620da5101";
const char* accessKey = "7feed72d-42bb-486e-8f23-1c0aba55d63d";
const char* accessSecret = "265f3ed59f42ff6d277e8db7fc6bbe287840c8b654b61875b9399b9565212c69";

// Wnology MQTT server
const char* mqtt_server = "broker.wnology.io";
const int mqtt_port = 1883;

unsigned long tempo;

const char* topicoEnvia = "senai/equipe1/envia";
const char* topicoRecebe = "senai/equipe1/recebe"; 

WiFiClient espClient;
PubSubClient client(espClient);

// Function prototypes
void callback(char* topic, byte* payload, unsigned int length);
void setup_wifi();
void reconnect();

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  pinMode(FRENTE, OUTPUT);
  pinMode(ESQUERDA, OUTPUT);
  pinMode(DIREITA, OUTPUT);
  pinMode(TRAS, OUTPUT);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();
  
  if ((millis() - tempo) > 6000) {
    // Cria um JSON com o status dos LEDs
    StaticJsonDocument<256> dados;
    dados["frente"] = analogRead(FRENTE);
    dados["tras"] = analogRead(TRAS);
    dados["esquerda"] = analogRead(ESQUERDA);
    dados["direita"] = analogRead(DIREITA);

    char mensagem[256];
    serializeJson(dados, mensagem);

    Serial.println("Publicando estados atuais dos LEDs:");
    Serial.println(mensagem);

    client.publish(topicoEnvia, mensagem);

    tempo = millis();
  }
}

///////////////////////////////////////////////////////////////
// Function to handle incoming MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida em ");
  Serial.println(topic);

  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.println("Payload recebido: " + msg);

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, msg);
  if (error) {
    Serial.print("Erro ao decodificar JSON: ");
    Serial.println(error.f_str());
    return;
  }

  const char* variable = doc["variable"];
  int value = doc["value"];

  Serial.print("Comando recebido -> Variável: ");
  Serial.print(variable);
  Serial.print(" | Valor: ");
  Serial.println(value);

  // Converte o valor (0–100) para PWM (0–255)
  int pwmValue = map(value, 0, 100, 0, 255);

  // Decide qual LED ajustar
  if (strcmp(variable, "frente") == 0) {
    analogWrite(FRENTE, pwmValue);
  } else if (strcmp(variable, "tras") == 0) {
    analogWrite(TRAS, pwmValue);
  } else if (strcmp(variable, "direita") == 0) {
    analogWrite(DIREITA, pwmValue);
  } else if (strcmp(variable, "esquerda") == 0) {
    analogWrite(ESQUERDA, pwmValue);
  } else {
    Serial.println("Variável desconhecida recebida!");
  }

  Serial.println("PWM ajustado para: " + String(pwmValue));
}

// Function to connect to Wi-Fi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

// Function to connect to MQTT server
void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    if (client.connect(deviceID, accessKey, accessSecret)) {
      Serial.println("conectado");
      client.subscribe("esp32/LED");  // Subscribe to topic
      client.subscribe(topicoRecebe);
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}
