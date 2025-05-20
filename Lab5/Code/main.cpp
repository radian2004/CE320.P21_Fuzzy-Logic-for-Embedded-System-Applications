// Arduino Fuzzy Logic Temperature Controller
// DHT22, LCD I2C, 2 nút nhấn (INPUT_PULLUP), 5 LED bar (pins 2-6), PWM quạt/LED

#include <DHT.h>
#include <Fuzzy.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// --- cấu hình phần cứng ---
#define DHTPIN       10       // chân DHT22
#define DHTTYPE      DHT22
#define BUTTON_UP    11       // nút tăng nhiệt độ (INPUT_PULLUP)
#define BUTTON_DOWN  12       // nút giảm nhiệt độ (INPUT_PULLUP)
#define FAN_PIN      6        // chân PWM quạt/LED
const int ledBarPins[5] = {2, 3, 4, 5, 6};

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Fuzzy controller
Fuzzy fuzzy;
FuzzyInput* errorInput;
FuzzyOutput* fanOutput;

// Tập mờ đầu vào (error = desired - actual)
FuzzySet *err_LN, *err_SN, *err_ZE, *err_SP, *err_LP;
// Tập mờ đầu ra (PWM)
FuzzySet *out_OFF, *out_LOW, *out_MED, *out_HIGH, *out_MAX;

// Biến toàn cục
float desiredTemp = 28.0;              // nhiệt độ mong muốn
unsigned long lastBtnTime = 0;
const unsigned long debounce = 200;    // ms chống rung

// Thiết lập fuzzy logic
void setupFuzzy() {
  // Input: error
  errorInput = new FuzzyInput(1);
  err_LN = new FuzzySet(-12, -9.5, -7, -4.5);
  err_SN = new FuzzySet(-6, -3.5, -2.5, -1);
  err_ZE = new FuzzySet(-1, 0, 0, 1);
  err_SP = new FuzzySet(1, 2.5, 2.5, 5);
  err_LP = new FuzzySet(3.5, 5.5, 5.5, 7.5);
  errorInput->addFuzzySet(err_LN);
  errorInput->addFuzzySet(err_SN);
  errorInput->addFuzzySet(err_ZE);
  errorInput->addFuzzySet(err_SP);
  errorInput->addFuzzySet(err_LP);
  fuzzy.addFuzzyInput(errorInput);

  // Output: PWM
  fanOutput = new FuzzyOutput(1);
  out_OFF  = new FuzzySet(0, 0, 0, 0);
  out_LOW  = new FuzzySet(0, 50, 50, 100);
  out_MED  = new FuzzySet(90, 102, 102, 140);
  out_HIGH = new FuzzySet(130, 160, 160, 205);
  out_MAX  = new FuzzySet(200, 225, 225, 255);
  fanOutput->addFuzzySet(out_OFF);
  fanOutput->addFuzzySet(out_LOW);
  fanOutput->addFuzzySet(out_MED);
  fanOutput->addFuzzySet(out_HIGH);
  fanOutput->addFuzzySet(out_MAX);
  fuzzy.addFuzzyOutput(fanOutput);

  // Luật: LN→MAX, SN→HIGH, ZE→MED, SP→LOW, LP→OFF
  auto addRule = [&](int i, FuzzySet* inS, FuzzySet* outS) {
    FuzzyRuleAntecedent* a = new FuzzyRuleAntecedent();
    a->joinSingle(inS);
    FuzzyRuleConsequent* c = new FuzzyRuleConsequent();
    c->addOutput(outS);
    fuzzy.addFuzzyRule(new FuzzyRule(i, a, c));
  };
  addRule(1, err_LN, out_MAX);
  addRule(2, err_SN, out_HIGH);
  addRule(3, err_ZE, out_MED);
  addRule(4, err_SP, out_LOW);
  addRule(5, err_LP, out_OFF);
}

void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.init(); lcd.backlight();

  pinMode(BUTTON_UP,   INPUT);
  pinMode(BUTTON_DOWN, INPUT);
  pinMode(FAN_PIN,     OUTPUT);
  for (int i = 0; i < 5; i++) pinMode(ledBarPins[i], OUTPUT);

  setupFuzzy();
}

void loop() {
  // Debounce nút nhấn
  if (millis() - lastBtnTime > debounce) {
    if (digitalRead(BUTTON_UP) == HIGH) { desiredTemp += 0.5; lastBtnTime = millis(); }
    if (digitalRead(BUTTON_DOWN) == HIGH) { desiredTemp -= 0.5; lastBtnTime = millis(); }
  }
  desiredTemp = constrain(desiredTemp, 16.0, 35.0);

  // Đọc nhiệt độ
  float actualTemp = dht.readTemperature();
  if (isnan(actualTemp)) {
    lcd.clear(); lcd.setCursor(0,0); lcd.print("DHT22 Error"); delay(1000);
    return;
  }

  // Tính error rồi fuzzify
  float err = desiredTemp - actualTemp;
  fuzzy.setInput(1, err);
  fuzzy.fuzzify();
  float pwmVal = fuzzy.defuzzify(1);

  // Xuất PWM
  analogWrite(FAN_PIN, (int)pwmVal);

  // LED bar hiển thị mức công suất (0-5)
  int level = round(pwmVal / 255.0 * 5);
  for (int i = 0; i < 5; i++) digitalWrite(ledBarPins[i], i < level);

  // Phần trăm công suất
  float pct = pwmVal / 255.0 * 100.0;

  // LCD hiển thị
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Act:"); lcd.print(actualTemp,1);
  lcd.print((char)223); lcd.print("C");
  lcd.setCursor(0,1);
  lcd.print("Des:"); lcd.print(desiredTemp,1);
  lcd.print((char)223); lcd.print("C");

  // Serial debug
  Serial.print("Act="); Serial.print(actualTemp,1);
  Serial.print("C Des="); Serial.print(desiredTemp,1);
  Serial.print("C Err="); Serial.print(err,1);
  Serial.print(" PWM="); Serial.print(pwmVal,0);
  Serial.print(" ( "); Serial.print(pct,1); Serial.println("% )");

  delay(500);
}
