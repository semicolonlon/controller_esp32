#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <ArduinoJson.h>

#define PIN_AXIS_X 3
#define PIN_AXIS_Y 4
#define PIN_JOY_SW 2
#define PIN_SW1 5
#define LED 6

#define SERVICE_UUID           "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_NOTIFY   "12345678-1234-5678-1234-56789abcdef1"
#define CHARACTERISTIC_WRITE    "12345678-1234-5678-1234-56789abcdef2"
#define DEVICE_NAME "Julius_Caesar"

BLEServer* pServer = nullptr;
BLECharacteristic* pNotifyCharacteristic = nullptr;
bool deviceConnected = false;
unsigned long lastSend = 0;

class WriteCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) override {
    String value = pCharacteristic->getValue().c_str();
    if (value.length() > 0) {
      Serial.print("[BLE] Received command: ");
      Serial.println(value);

      if (value == "LED_ON") digitalWrite(LED, HIGH);
      if (value == "LED_OFF") digitalWrite(LED, LOW);
    }
  }
};

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    Serial.println("[BLE] Connected");
  }

  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    Serial.println("[BLE] Disconnected");
    BLEDevice::startAdvertising();
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(PIN_AXIS_X, INPUT);
  pinMode(PIN_AXIS_Y, INPUT);
  pinMode(PIN_JOY_SW, INPUT_PULLUP);
  pinMode(PIN_SW1, INPUT_PULLUP);
  pinMode(LED, OUTPUT);

  BLEDevice::init(DEVICE_NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);

  pNotifyCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_NOTIFY,
    BLECharacteristic::PROPERTY_NOTIFY
  );

  BLECharacteristic* pWriteCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_WRITE,
    BLECharacteristic::PROPERTY_WRITE
  );
  pWriteCharacteristic->setCallbacks(new WriteCallbacks());

  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("[BLE] Advertising started");
}
void loop() {
  if (!deviceConnected) return;

  unsigned long now = millis();
  if (now - lastSend >= 16) {
    lastSend = now;

    int x_raw = analogRead(PIN_AXIS_X);
    int y_raw = analogRead(PIN_AXIS_Y);

    float x_val = (x_raw / 2047.5f) - 1.0f;
    float y_val = (y_raw / 2047.5f) - 1.0f;

    bool joy_sw = digitalRead(PIN_JOY_SW) == LOW;
    bool sw1 = digitalRead(PIN_SW1) == HIGH;

    StaticJsonDocument<128> doc;
    doc["x"] = -x_val;
    doc["y"] = y_val;
    doc["joy"] = joy_sw;
    doc["sw1"] = sw1;

    String json;
    
    serializeJson(doc, json);

    pNotifyCharacteristic->setValue(json.c_str());
    pNotifyCharacteristic->notify();

    Serial.println("[Send] " + json);
  }
}