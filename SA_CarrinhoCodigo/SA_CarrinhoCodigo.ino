#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define BUTTON_PIN 23  // GPIO23 pin connected to the button
#define LED_PIN 26     // GPIO26 pin connected to the LED
#define POT 33
#define LED_PWM 17

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
const char* deviceID = "690292cc672bc066e5e68079";
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
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_PWM, OUTPUT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  currentState = digitalRead(BUTTON_PIN);
  StaticJsonDocument<300> msmJson;
  msmJson["botao"] = currentState;
  serializeJson(msmJson, mensagemJson);


  StaticJsonDocument<300> potJson;
  potJson["potenciometro"] = analogRead(POT);
  serializeJson(potJson, mensagemPotJson);

  if ((millis() - tempo) > 6000) {
    leituraPot = analogRead(POT);
    Serial.println("Enviou o valor do potênciometro: " + String(leituraPot));
    client.publish(topicoEnvia, mensagemPotJson);
    tempo = millis();
  }

  if (lastState == LOW && currentState == HIGH) {
    Serial.println("O botão está pressionado");
    client.publish(topicoEnvia, mensagemJson);
  } else if (lastState == HIGH && currentState == LOW) {
    Serial.println("O botão foi solto");
    client.publish(topicoEnvia, mensagemJson);
  }

  // Save the last state
  lastState = currentState;
  client.loop();
}

///////////////////////////////////////////////////////////////
// Function to handle incoming MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("]: ");

  char JSONMessage[length];
  for (int i = 0; i < length; i++) {
    JSONMessage[i] = (char)payload[i];
  }
  Serial.println("Parsing start: ");
  Serial.println(JSONMessage);
  StaticJsonDocument<300> JSONBuffer;
  deserializeJson(JSONBuffer, JSONMessage);

  DeserializationError error = deserializeJson(JSONBuffer, JSONMessage);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
  }

  const char* variable = JSONBuffer["variable"];
  int value = JSONBuffer["value"];

  Serial.print("variavel: ");
  Serial.println(variable);
  Serial.print("Valor: ");
  Serial.println(value);
  Serial.println();

  if (String(topic) == topicoRecebe) {
    digitalWrite(LED_PIN, value);
  }
  if (String(topic) == "esp32/LED") {
    analogWrite(LED_PWM, map(value, 0, 100, 0, 255));
    Serial.println("Comando LED: " + String(map(value, 0, 100, 0, 255)));
  }

  // Print the payload as a string
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
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
