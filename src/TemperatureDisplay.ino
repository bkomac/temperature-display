#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <ArduinoJson.h>
#include <DHT22.h>
#include <SPI.h>
#include <SoftwareSerial.h>

String appVer = "1.0.3";
String appId = "ARDAD23FD";

// TFT
#define TFT_CS 10
#define TFT_RST 8 // Reset
#define TFT_DC 9  // A0

#define TFT_SCLK 13 // SCL
#define TFT_MOSI 11 // SDA
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
#define ST7735_GRAY 0x8410
#define ST7735_YELLOW 0xFFE0

// Serial
const byte RxPin = 3; // tx Pin on HC12
const byte TxPin = 4; // rx Pin on HC12

SoftwareSerial HC12(RxPin, TxPin);

// DHT
#define DHTPIN 7
DHT22 dht(DHTPIN);

String temp = "00";
long heartBeat = 10; // in seconds
long lastSend = millis();
long lastRecieve = millis();

long measureInterval = 300; // in seconds
long lastMeasureTime = millis();
int lastMeasure = 0;

void setup(void) {

  Serial.begin(9600);
  HC12.begin(19200);

  tft.initR(INITR_144GREENTAB);
  tft.fillScreen(ST7735_WHITE);
  tft.setRotation(1);

  tft.fillCircle(tft.width() / 2, tft.height() / 2, 40, ST7735_GREEN);

  tft.setTextColor(ST7735_GREEN);
  tft.setCursor(37, 60);
  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
  tft.println("ver." + appVer);
  delay(1000);
  tft.fillScreen(ST7735_BLACK);
}

void loop() {

  delay(300);

  String readString = "";
  while (HC12.available()) {
    delay(1);
    char c = HC12.read();

    if (c == "\n")
      break;
    readString += c;
  }

  if (readString.length() > 0) {
    Serial.print("\n >> Recieve: ");
    Serial.print(readString);

    DynamicJsonBuffer jsonBuffer;
    JsonObject &root = jsonBuffer.parseObject(readString);

    if (root["temp"] != "") {
      temp = root["temp"].asString();
    }
    blink();
  }

  DHT22_ERROR_t errorCode;
  errorCode = dht.readData();
  delay(500);
  float h;
  float t;
  errorCode = DHT_ERROR_NONE;
  if (errorCode == DHT_ERROR_NONE) {
    t = dht.getTemperatureC();
    h = dht.getHumidity();

    sendData(t, h);

    tft.setCursor(43, 7);
    tft.setTextSize(1);
    tft.setTextColor(ST7735_RED);
    tft.println("Hot water");
    tft.setCursor(35, 17);
    tft.println("temperature");

    if (temp.length() > 0) {
      temp.trim();
      tft.setTextSize(7);

      tft.setCursor(29, 37);

      tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
      tft.print(temp);

      tft.setTextSize(1);
      String deg = (String)((char)247) + "C";
      tft.print(deg);

      measureTrend(temp);
    }
    tft.setCursor(10, 95);
    tft.setTextSize(1);
    tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
    tft.print("Room temp. / hum.");

    tft.setCursor(10, 107);
    tft.setTextSize(2);
    tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
    String tx = (String)t;
    tx = tx.substring(0, tx.length() - 1);
    String tem = (String)tx;
    tft.print(tem);
    tft.setTextSize(1);
    tem = (String)((char)247) + "C";
    tft.print(tem);

    /* tft.setCursor(90, 95);
      tft.setTextSize(1);
      tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
      tft.print("Hum:"); */

    tft.setCursor(90, 107);
    tft.setTextSize(2);
    tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
    String hx = (String)h;
    hx = (String)hx.substring(0, hx.length() - 3);
    String hum = (String)hx;
    tft.print(hum);
    tft.setTextSize(1);
    hum = "%";
    tft.print(hum);

  } else {
    Serial.print("error: ");
    Serial.println(errorCode);
    tft.setTextSize(1);
    tft.setCursor(30, 40);
    tft.println("Error:");
    tft.setCursor(50, 60);
    tft.println(errorCode);
  }
  // testfillcircles(40, ST7735_GREEN);

  // tft.invertDisplay(false);
}

void measureTrend(String tt) {
  if ((lastMeasureTime + (measureInterval * 1000)) < millis() ||
      lastMeasure == 0) {
    lastMeasure = tt.toInt();
    lastMeasureTime = millis();
    Serial.print("\nLast measure: " + (String)lastMeasure);
  }

  Serial.print("\nTrend ... ");
  if ((lastMeasure - tt.toInt()) > 0) {
    printCh(25);
  } else if ((lastMeasure - tt.toInt()) < 0) {
    printCh(24);
  } else {
    printCh(18);
  }
}

void blink() {
  tft.setCursor(107, 65);
  tft.setTextSize(3);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.print(".");
  delay(500);
  tft.setCursor(107, 65);
  tft.setTextSize(3);
  tft.setTextColor(ST7735_BLACK, ST7735_BLACK);
  tft.print(".");
}

void printCh(int ch) {
  tft.setTextSize(3);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setCursor(6, 53);
  tft.print((char)ch);
}

void sendData(float t, float h) {
  if ((lastSend + (heartBeat * 1000)) > millis())
    return;

  lastSend = millis();

  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();

  root["sensor_type"] = "DHT22";
  root["id"] = appId;
  root["ver"] = appVer;
  root["name"] = "TemperatureDisplay";

  JsonObject &data = root.createNestedObject("data");
  data["temp"] = t;
  data["hum"] = h;

  String content;
  root.printTo(content);
  content = content + "\n";
  Serial.print("\nSending ... \n");

  char cont_ch[content.length() + 2];
  content.toCharArray(cont_ch, content.length() + 2, 0);

  delay(100);
  Serial.println(cont_ch);
  HC12.write(cont_ch);
}

void testfillcircles(uint8_t radius, uint16_t color) {
  for (int16_t x = radius; x < tft.width(); x += radius * 2) {
    for (int16_t y = radius; y < tft.height(); y += radius * 2) {
      tft.fillCircle(x, y, radius, color);
    }
  }
}
