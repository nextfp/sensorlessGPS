#include <Arduino.h>
#include <M5Unified.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_TX "beb5483f-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_RX "beb5483e-36e1-4688-b7f5-ea07361b26a9"

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
BLECharacteristic *pRxCharacteristic;
bool deviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
    // Serial.println("Connected");
  };

  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
    // Serial.println("Disconnected");
  }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
  String geolocation[3];

  void splitString(String input)
  {
    for (int i = 0; i < 3; i++)
    {
      geolocation[i] = "";
    }

    int currentIndex = 0;
    int startIndex = 0;
    int endIndex = input.indexOf(',');

    while (endIndex != -1)
    {
      geolocation[currentIndex++] = input.substring(startIndex, endIndex);
      startIndex = endIndex + 1;
      endIndex = input.indexOf(',', startIndex);
    }

    // 最後の区切り文字の後の部分を追加
    geolocation[currentIndex++] = input.substring(startIndex);
  }

  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0)
    {
      String message = value.c_str();
      // Serial.println("Received: " + message);
      splitString(message);
      for (int i = 0; i < 3; i++)
      {
        // Serial.println(geolocation[i]);
      }
      M5.Lcd.setCursor(10, 10);
      M5.Lcd.println("緯度:" + geolocation[0]);
      M5.Lcd.setCursor(10, 60);
      M5.Lcd.println("経度:" + geolocation[1]);
      M5.Lcd.setCursor(10, 110);
      M5.Lcd.println("速度:" + geolocation[2]);
    }
  }
};

void setup()
{
  M5.begin();
  M5.Power.begin();
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextSize(4);
  // Serial.begin(115200);

  BLEDevice::init("ESP32_BLE_SERVER"); // この名前がスマホなどに表示される
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_TX,
      BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
  pTxCharacteristic->addDescriptor(new BLE2902());
  pRxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE); // 読み書き可能にする

  pRxCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // iPhone接続の問題に役立つ
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  // Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop()
{
  delay(2000);
  if (deviceConnected)
  {
    // データを送信
    // Serial.println("Sending data");
    String str(millis() / 1000);
    pTxCharacteristic->setValue(str.c_str());
    pTxCharacteristic->notify();
  }
}