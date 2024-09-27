#include <Sipeed_GC0328.h>       
#include <WiFi.h>                
#include <PubSubClient.h>        
#include <Face.h>                

Sipeed_GC0328 camera;           
face_t faces[5];                 
KPUClass kpu;                    

const char* ssid = "my_SSID";
const char* password = "my_PASSWORD";

WiFiClient espClient;
PubSubClient client(espClient);
const char* mqtt_server = "broker.hivemq.com";

int motionPin = 5;               

void setup() {
  Serial.begin(115200);
  camera.begin(CAMERA_R320x240);  
  setupWiFi();                    
  client.setServer(mqtt_server, 1883);
  pinMode(motionPin, INPUT);      

  kpu.begin("/sd/face_model.kfpkg");
}

void loop() {
  if (digitalRead(motionPin) == HIGH) {
    Serial.println("Motion detected, capturing image...");
    camera.run();
    uint8_t* img = camera.getRGB565();  

    int face_count = kpu.faceDetect(img, faces, 5);
    if (face_count > 0) {
      Serial.print("Detected ");
      Serial.print(face_count);
      Serial.println(" face(s).");

      for (int i = 0; i < face_count; i++) {
        if (faces[i].score > 85) {
          sendMQTTMessage("Known visitor detected!");
        } else {
          sendMQTTMessage("Unknown visitor detected!");
        }
      }
    } else {
      Serial.println("No face detected.");
    }
  }

  // MQTT communication
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void sendMQTTMessage(String message) {
  if (client.connect("Maixduino_Client")) {
    client.publish("home/visitor", message.c_str());
    Serial.println("Message sent: " + message);
  } else {
    Serial.println("MQTT Connection Failed!");
  }
}

void reconnect() {
  while (!client.connected()) {
    if (client.connect("Maixduino_Client")) {
      Serial.println("MQTT connected");
    } else {
      delay(5000);
    }
  }
}