#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Digitals
#define BUTTON 2
#define WATER_SENSOR_TOP 3
#define WATER_SENSOR_BOTTOM 4
#define INLET_PUMP_ON 7
#define OLED_D1 8
#define OLED_D0 9
#define OLED_DC 10
#define OLED_CS 11
#define OLED_RESET 12
#define DIODE_GREEN 13

// Constants
long shortSensorTimeout = 1000;
long longSensorTimeout = 45000; // 60000

// Variables
int unsigned waterLevel = 0;
long unsigned waterSensorInterval = shortSensorTimeout;
bool isPause = false;

bool inletPumpOn = false;

long unsigned lastInletPumpRun = 0;
long unsigned lastInletPumpRest = 0;
long unsigned lastInletPumpChange = 0;
long unsigned lastPumpStateCheck = 0;
float nextCheckLoad = 0;

// http://adafruit.github.io/Adafruit-GFX-Library/html/class_adafruit___g_f_x.html
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_D1, OLED_D0, OLED_DC, OLED_RESET, OLED_CS);


void setup()
{
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  pinMode(WATER_SENSOR_TOP, INPUT);
  pinMode(WATER_SENSOR_BOTTOM, INPUT);
  pinMode(BUTTON, INPUT);

  pinMode(DIODE_GREEN, OUTPUT);
  pinMode(INLET_PUMP_ON, OUTPUT);
}



bool isTopWaterSensorUp()
{
  return digitalRead(WATER_SENSOR_TOP) == LOW;
}

bool isTopWaterSensorDown()
{
  return digitalRead(WATER_SENSOR_TOP) == HIGH;
}

bool isBottomWaterSensorUp()
{
  return digitalRead(WATER_SENSOR_BOTTOM) == LOW;
}

bool isBottomWaterSensorDown()
{
  return digitalRead(WATER_SENSOR_BOTTOM) == HIGH;
}

void turnDiodeOn()
{
  digitalWrite(DIODE_GREEN, HIGH);
}

void turnDiodeOff()
{
  digitalWrite(DIODE_GREEN, LOW);
}

void turnInletPumpOn()
{
  if (inletPumpOn) return;
  digitalWrite(INLET_PUMP_ON, HIGH);
  inletPumpOn = true;
  turnDiodeOn();
  waterSensorInterval = shortSensorTimeout;
  if (lastInletPumpRun > 0) lastInletPumpRest = millis() - lastInletPumpChange;
  lastInletPumpChange = millis();
}

void turnInletPumpOff()
{
  if (!inletPumpOn) return;
  digitalWrite(INLET_PUMP_ON, LOW);
  inletPumpOn = false;
  turnDiodeOff();
  waterSensorInterval = longSensorTimeout;
  lastInletPumpRun = millis() - lastInletPumpChange;
  lastInletPumpChange = millis();
}

void displayWriteBorder()
{
  display.drawRoundRect(0, 0, 128, 64, 5, WHITE);
}

void displayWritePause()
{
  display.clearDisplay();
  displayWriteBorder();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(35, 25);
  display.println("PAUSE");
  display.display();
}

void displayWriteWaterLevelBar(int level, bool fill)
{
  if (level > 3)
  {
    level = 3;
  }
  int top = ((3 - level) * 15) + 5;
  if (fill)
  {
    display.fillRoundRect(10, top, 44, 10, 3, WHITE);
  }
  else
  {
    display.drawRoundRect(10, top, 44, 10, 3, WHITE);
  }
}

int getHourFlow() {
  long unsigned roundInterval = lastInletPumpRun + lastInletPumpRest;
  float roundsPerHour = 3600000 / roundInterval;
  float pumpFlowLitersPerHour = 110.0;
  float flowPerRound = (lastInletPumpRun / 1000.0) * (pumpFlowLitersPerHour / 3600.0);
  int flow = round(flowPerRound * roundsPerHour);
  return flow;
}

void syncDisplay()
{
  display.clearDisplay();
  //  displayWriteBorder();
  displayWritePumpState(inletPumpOn);
  displayWriteWaterLevel(waterLevel);
  displayWriteLoadLine(nextCheckLoad);
  display.display();
}

void displayWritePumpState(bool on)
{
  if (on)
  {
    display.fillRoundRect(74, 5, 44, 40, 3, WHITE);
  }
  else
  {
    display.drawRoundRect(74, 5, 44, 40, 3, WHITE);
  }
  display.setTextSize(2);
  display.setTextColor(on ? BLACK : WHITE);
  display.setCursor(on ? 85 : 79, 10);
  display.println(on ? "ON" : "OFF");
  display.setTextSize(2);
  display.setCursor(85, 28);
  String flowText = "";
  flowText.concat(getHourFlow());
  display.println(flowText);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(85, 50);
  display.println("PUMP");
}

void displayWriteWaterLevel(int level)
{
  displayWriteWaterLevelBar(1, level >= 1);
  displayWriteWaterLevelBar(2, level >= 2);
  displayWriteWaterLevelBar(3, level >= 3);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(17, 50);
  display.println("LEVEL");
}

void displayWriteLoadLine(float done) {
  int lineLength = round(128 * done);
  display.drawFastHLine(0, 63, lineLength, WHITE);
}

void syncNextCheckLoad() {
  float timeDiff = millis() - lastPumpStateCheck;
  float newLoad = timeDiff / waterSensorInterval;
  if (newLoad != nextCheckLoad) {
    nextCheckLoad = newLoad;
    syncDisplay();
  }
}


void readButton()
{
  if (digitalRead(BUTTON) == HIGH)
  {
    switchPause();
    delay(300);
  }
}

void syncPumpState(bool force = false)
{
  unsigned long now = millis();
  if (force || (now - lastPumpStateCheck >= waterSensorInterval))
  {
    lastPumpStateCheck = now;
    bool shouldIntetPumpRun = isTopWaterSensorDown();
    if (shouldIntetPumpRun)
    {
      turnInletPumpOn();
    }
    else
    {
      turnInletPumpOff();
    }

  }
}

void syncWaterLevel() {
  if (isBottomWaterSensorDown() && isTopWaterSensorDown()) waterLevel = 1;
  if (isBottomWaterSensorUp() && isTopWaterSensorDown()) waterLevel = 2;
  if (isBottomWaterSensorUp() && isTopWaterSensorUp()) waterLevel = 3;
  syncDisplay();
}

void switchPause()
{
  if (isPause)
  {
    pauseOff();
  }
  else
  {
    pauseOn();
  }
}

void pauseOn()
{
  isPause = true;
  displayWritePause();
  turnInletPumpOff();
  turnDiodeOff();
  lastInletPumpRun = 0;
  lastInletPumpRest = 0;
  lastInletPumpChange = 0;
  lastPumpStateCheck = 0;
}

void pauseOff()
{
  isPause = false;
  syncPumpState(true);
}

// This is a safety feature. If the pump runs 30% longer than in the previous round
// then it is assumed that the water sensor is stuck or has stopped working and therefore pause the system.
void checkInletPumpRunningTime()
{
  if (inletPumpOn && lastInletPumpRun > 0 && lastInletPumpChange > 0)
  {
    long runTime = millis() - lastInletPumpChange;
    if ((float)runTime > (1.3 * (float)lastInletPumpRun))
    {
      pauseOn();
    }
  }
}

bool isInit = true;
void loop()
{
  readButton();
  if (!isPause) {
    syncNextCheckLoad();
    syncPumpState(isInit);
    syncWaterLevel();
    checkInletPumpRunningTime();
    if (isInit) isInit = false;
  }
  delay(100);
}
