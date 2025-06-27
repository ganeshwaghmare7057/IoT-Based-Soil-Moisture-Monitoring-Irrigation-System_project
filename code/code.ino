#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

#define DHTPIN D6  // Pin connected to the DHT11 data pin
#define DHTTYPE DHT11  // Define sensor type (DHT11)
DHT dht(DHTPIN, DHTTYPE);

// Soil moisture sensor connected to A0
#define SOIL_MOISTURE_PIN A0

// Define the D4 pin (Relay/Motor Control Pin)
#define MOTOR_CONTROL_PIN D4

// Wi-Fi and MQTT Broker Info
const char* ssid = "vivo";
const char* password = "123456789";
const char* mqtt_server = "dev.coppercloud.in";  // Broker address
const char* mqtt_topic = "dhanshreekamble094@gmail.com/soil/project";             // Topic to publish data
const char* control_topic = "dhanshreekamble094@gmail.com/soil/motor/project"; // Topic to control motor

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // Start Serial communication for debugging
  Serial.begin(115200);
  delay(10);

  // Initialize the motor control pin as output
  pinMode(MOTOR_CONTROL_PIN, OUTPUT);
  digitalWrite(MOTOR_CONTROL_PIN, LOW); // Make sure the motor is off initially

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Initialize DHT11 sensor
  dht.begin();

  // Set up MQTT client
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);
  
  // Subscribe to the control topic
  client.subscribe(control_topic);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Handle incoming MQTT messages
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // Check if the received message is "on" or "off" to control the motor
  if (String(topic) == control_topic) {
    if (message == "onMoter") {
      Serial.println("Turning motor ON");
      digitalWrite(MOTOR_CONTROL_PIN, HIGH); // Turn motor ON
    } else if (message == "offMoter") {
      Serial.println("Turning motor OFF");
      digitalWrite(MOTOR_CONTROL_PIN, LOW); // Turn motor OFF
    }
  }
}

void reconnect() {
  // Loop until connected to MQTT broker
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Once connected, subscribe to the control topic
      client.subscribe(control_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Read soil moisture
  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);

  // Map the soil moisture value from 0-1023 to 0-100 (percentage)
  int soilMoisturePercentage = map(soilMoistureValue, 0, 1023, 0, 100);
  
  // Read temperature and humidity from DHT11
  float temperature = dht.readTemperature(); // Temperature in Celsius
  float humidity = dht.readHumidity();       // Humidity percentage
  
  // Check if readings failed and try again
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Create a JSON string with sensor data
  String payload = "{\"soil_moisture\":" + String(soilMoisturePercentage) +
                   ",\"temperature\":" + String(temperature) +
                   ",\"humidity\":" + String(humidity) + "}";

  // Send data to MQTT broker
  if (client.publish(mqtt_topic, payload.c_str())) {
    Serial.println("Data sent to MQTT broker");
  } else {
    Serial.println("Failed to send data");
  }

  // Wait for 1 second before sending data again
  delay(1000); // Increased delay to give time for reading to be updated
}