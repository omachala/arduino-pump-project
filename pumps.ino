#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// #include <IRremote.h>

#define DECODE_NEC

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_D1 8
#define OLED_D0 9
#define OLED_DC 10
#define OLED_CS 11
#define OLED_RESET 12
#define DIODE_GREEN 13
#define WATER_SENSOR_BOTTOM 6
#define BUTTON 2
#define PUMPS_STANDBY 7

// int INLET_PUMP2 = 2;
// int OUTLET_PUMP2 = 3;
// int INLET_PUMP1 = 6;
// int OUTLET_PUMP1 = 7;

// int INLET_PUMP_SPEED = 2;
// int OUTLET_PUMP_SPEED = 3;
int INLET_PUMP_ON = 4;
int OUTLET_PUMP_ON = 5;

int UV_LAMP = A0;

int WATER_ANALOG_SENSOR_POWER = 3;
int WATER_ANALOG_SENSOR = A5;

bool isPause = false;

// bool runInlet = false;
// bool runOutlet = false;

// bool isOverflow;
// bool isDry;

// this combination of default values prevents the initial start-up of the water pumps
// bool lastWaterSensorTopState = true;
// bool lastWaterSensorBottomState = false;

// http://adafruit.github.io/Adafruit-GFX-Library/html/class_adafruit___g_f_x.html
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
                         OLED_D1, OLED_D0, OLED_DC, OLED_RESET, OLED_CS);

static const unsigned char PROGMEM tick[] = {
    0x00, 0xf0, 0x00, 0x07, 0xfe, 0x00, 0x0f, 0xff, 0x00, 0x1f, 0xff, 0x80, 0x3f, 0xfe, 0xc0, 0x7f,
    0xfc, 0x60, 0x7f, 0xf8, 0x60, 0x7f, 0xf0, 0xe0, 0xff, 0xf1, 0xf0, 0xff, 0xe1, 0xf0, 0xe7, 0xc3,
    0xf0, 0xc1, 0x87, 0xf0, 0x60, 0x0f, 0xe0, 0x70, 0x0f, 0xe0, 0x7c, 0x1f, 0xe0, 0x3e, 0x3f, 0xc0,
    0x1f, 0xff, 0x80, 0x0f, 0xff, 0x00, 0x07, 0xfe, 0x00, 0x00, 0xf0, 0x00};

void setup()
{
  // pinMode(LED, OUTPUT);
  pinMode(DIODE_GREEN, OUTPUT);
  pinMode(WATER_SENSOR_BOTTOM, INPUT);
  pinMode(BUTTON, INPUT);
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //Initialize with the I2C address 0x3C.
  // displayRewriteInletOutlet(true, true);
  // digitalWrite(LED, HIGH);
  // pinMode(INLET_PUMP, INPUT_PULLUP);
  // pinMode(OUTLET_PUMP, INPUT_PULLUP);

  // pinMode(INLET_PUMP_SPEED, OUTPUT);
  // pinMode(OUTLET_PUMP_SPEED, OUTPUT);

  // Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));
  // IrReceiver.begin(IR_REMOVE_RECIEVER, ENABLE_LED_FEEDBACK, USE_DEFAULT_FEEDBACK_LED_PIN);
  // Serial.print(F("Ready to receive IR signals at pin "));
  // Serial.println(IR_REMOVE_RECIEVER);

  pinMode(INLET_PUMP_ON, OUTPUT);
  pinMode(OUTLET_PUMP_ON, OUTPUT);
  pinMode(OUTLET_PUMP_ON, INPUT);
  pinMode(UV_LAMP, OUTPUT);

  pinMode(WATER_ANALOG_SENSOR_POWER, OUTPUT);
  digitalWrite(WATER_ANALOG_SENSOR_POWER, LOW);
}

unsigned long uvLampMillis = 0;
const long uvLampInterval = 10000;
bool uvLampOn = true;

void readButton()
{
  if (digitalRead(BUTTON) == HIGH)
  {
    switchPause();
    delay(300);
  }
}

bool isInletWaterSensorActivated()
{
  digitalWrite(WATER_ANALOG_SENSOR_POWER, HIGH);
  delay(10);
  int inletWaterLevel = analogRead(WATER_ANALOG_SENSOR);
  digitalWrite(WATER_ANALOG_SENSOR_POWER, LOW);
  return inletWaterLevel > 300;
}

bool isOutletWaterSensorActivated()
{
  return digitalRead(WATER_SENSOR_BOTTOM) == LOW;
}

const long waterSensorInterval = 5000;
unsigned long waterSensorMillis = 0;

void evaluateWaterSensor(bool force = false)
{
  unsigned long waterSensorCurrentMillis = millis();
  if (force || (waterSensorCurrentMillis - waterSensorMillis >= waterSensorInterval))
  {
    waterSensorMillis = waterSensorCurrentMillis;
    bool activateInletPump = !isInletWaterSensorActivated();
    bool activateOutletPump = isOutletWaterSensorActivated();
    syncPumpRelays(activateInletPump, activateOutletPump);
    displayRewriteInletOutlet(activateInletPump, activateOutletPump);
    digitalWrite(DIODE_GREEN, activateInletPump || activateOutletPump ? HIGH : LOW);
  }
}

void switchPause()
{
  isPause = !isPause;
  if (isPause)
  {
    displayWritePause();
    syncPumpRelays(false, false);
    digitalWrite(DIODE_GREEN, LOW);
  }
  else
  {
    evaluateWaterSensor(true);
  }
}

// void readWaterSensor()
// {
//   int waterSensorTopState = digitalRead(WATER_SENSOR_TOP);       // down = HIGH, up = LOW
//   int waterSensorBottomState = digitalRead(WATER_SENSOR_BOTTOM); // down = HIGH, up = LOW

//   if (waterSensorTopState != lastWaterSensorTopState)
//   {
//     lastWaterSensorTopState = waterSensorTopState;
//     isOverflow = waterSensorTopState == LOW;
//     resolvePumpState(isOverflow, isDry);
//     syncPumpRelays(runInlet, runOutlet);
//     displayRewriteInletOutlet(runInlet, runOutlet);
//   }

//   if (waterSensorBottomState != lastWaterSensorBottomState)
//   {
//     lastWaterSensorBottomState = waterSensorBottomState;
//     isDry = waterSensorBottomState == HIGH;
//     resolvePumpState(isOverflow, isDry);
//     syncPumpRelays(runInlet, runOutlet);
//     displayRewriteInletOutlet(runInlet, runOutlet);
//   }

//   if (!isOverflow && !isDry)
//   {
//     digitalWrite(DIODE_GREEN, HIGH);
//   }
//   else
//   {
//     digitalWrite(DIODE_GREEN, LOW);
//   }
// }

// bool uvLamp(bool on)
// {
//   digitalWrite(UV_LAMP, on ? LOW : HIGH);
//   return !on;
// }

// void resolvePumpState(bool overflow, bool dry)
// {
//   if (overflow)
//   {
//     runOutlet = true;
//     runInlet = false;
//   }
//   if (dry)
//   {
//     runInlet = true;
//     runOutlet = false;
//   }
//   if (!overflow && !dry)
//   {
//     runInlet = true;
//     runOutlet = true;
//   }
//   if (overflow && dry)
//   {
//     // cat has destroyed the project
//     runInlet = false;
//     runOutlet = false;
//   }
// }

void syncPumpRelays(bool inlet, bool outlet)
{
  digitalWrite(INLET_PUMP_ON, inlet ? HIGH : LOW);
  digitalWrite(OUTLET_PUMP_ON, outlet ? HIGH : LOW);
}

void displayRewriteInletOutlet(bool inletActive, bool outletActive)
{
  // display.stopscroll();
  display.clearDisplay();
  displayWriteInlet(inletActive);
  displayWriteOutlet(outletActive);
  // displayWriteStatusBar();
  displayWriteBorder();
  display.display();
  // display.startscrollright(0x0F, 0x0F);
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

void displayWriteBorder()
{
  // display.drawLine(0, 0, 127, 0, WHITE);
  // display.drawLine(0, 0, 0, 63, WHITE);
  // display.drawLine(127, 0, 127, 63, WHITE);
  // display.drawLine(1, 63, 127, 63, WHITE);
  display.drawRoundRect(0, 0, 128, 64, 5, WHITE);
}

void displayWriteInlet(bool active)
{
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(5, 5);
  display.println("Inlet");
  // display.setCursor(80, 5);
  // display.println(active ? "100" : "  0");
  if (active)
  {
    display.drawBitmap(80, 5, tick, 20, 20, WHITE);
  }
  // display.setTextSize(1);
  // display.setCursor(118, 12);
  // display.println("%");
}

void displayWriteOutlet(bool active)
{
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(5, 30);
  display.println("Outlet");
  if (active)
  {
    display.drawBitmap(80, 30, tick, 20, 20, WHITE);
  }
  // display.setCursor(80, 30);
  // display.println(active ? "100" : "  0");
  // display.setTextSize(1);
  // display.setCursor(118, 37);
  // display.println("%");
}

// void displayWriteStatusBar()
// {
// display.fillRect(0, 55, 128, 64, WHITE);
// display.setTextSize(1);
// display.setTextColor(BLACK);
// display.setCursor(5, 56);
// display.println("--> --> --> --> --> -->");
// }

void loop()
{
  if (!isPause)
  {
    evaluateWaterSensor();
  }
  readButton();
}
