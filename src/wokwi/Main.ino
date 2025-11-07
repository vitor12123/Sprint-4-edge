#include <Wire.h>
#include <MPU6050.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Configurações WiFi - ADAPTE PARA SUA REDE
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Configurações MQTT - USE O IP DA SUA VM
const char* mqtt_server = "20.57.132.118";  // ← COLOCAR IP DA SUA VM AQUI!
const int mqtt_port = 1883;
const char* mqtt_topic = "sensor/mpu6050";

MPU6050 mpu;
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long prevTime = 0;
float vx = 0, vy = 0, vz = 0;

void setup_wifi() {
  Serial.print("Conectando à rede ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi conectado!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    
    if (client.connect("ESP32_MPU6050")) {
      Serial.println("conectado!");
    } else {
      Serial.print("falha, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Inicializar WiFi
  setup_wifi();
  
  // Configurar MQTT
  client.setServer(mqtt_server, mqtt_port);

  Serial.println("Iniciando MPU6050...");
  mpu.initialize();

  if (mpu.testConnection()) {
    Serial.println("MPU6050 conectado com sucesso!");
  } else {
    Serial.println("Falha ao conectar MPU6050");
  }

  prevTime = millis();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int16_t ax, ay, az;
  int16_t gx, gy, gz;

  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float accelX = ax / 16384.0;
  float accelY = ay / 16384.0;
  float accelZ = az / 16384.0;

  unsigned long currentTime = millis();
  float dt = (currentTime - prevTime) / 1000.0; 
  prevTime = currentTime;

  vx += accelX * 9.81 * dt;
  vy += accelY * 9.81 * dt;
  vz += accelZ * 9.81 * dt;

  // Criar mensagem JSON para enviar via MQTT
  String payload = "{";
  payload += "\"aceleracao\": {";
  payload += "\"x\":" + String(accelX, 2) + ",";
  payload += "\"y\":" + String(accelY, 2) + ",";
  payload += "\"z\":" + String(accelZ, 2);
  payload += "},";
  payload += "\"velocidade\": {";
  payload += "\"x\":" + String(vx, 2) + ",";
  payload += "\"y\":" + String(vy, 2) + ",";
  payload += "\"z\":" + String(vz, 2);
  payload += "}";
  payload += "}";

  // Publicar no MQTT
  if (client.publish(mqtt_topic, payload.c_str())) {
    Serial.println("Dados enviados via MQTT!");
  } else {
    Serial.println("Falha ao enviar via MQTT");
  }

  Serial.println(payload);
  delay(1000);  // Envia a cada 1 segundo
}