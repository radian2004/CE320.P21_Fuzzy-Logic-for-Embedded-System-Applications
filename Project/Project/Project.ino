#include <DHT.h>
#include <Fuzzy.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

#define DHTPIN 5
#define DHTTYPE DHT22
#define IN1 7
#define IN2 8
#define EN 6

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
Fuzzy fuzzy;

// enums cho readability
enum TempLevel
{
  VCOLD,
  COLD,
  WARM,
  HOT,
  VHOT
};
enum HumLevel
{
  LOWH,
  MEDH,
  HIGHH
};
enum PowerLevel
{
  VPWR,
  LOWP,
  MEDP,
  HIGHP,
  VIGHP
};

// 1. khai báo tĩnh các fuzzy set object
static FuzzySet fTempObjs[5] = {
    {0, 0, 20, 26.7},     // very cold
    {20, 26.7, 32.2, 35}, // cold
    {30, 32.2, 40.6, 45}, // warm
    {40, 40.6, 54.4, 60}, // hot
    {54.4, 60, 65, 65}    // very hot
};
static FuzzySet fHumObjs[3] = {
    {0, 0, 30, 50},    // low humidity
    {30, 50, 70, 90},  // medium humidity
    {70, 90, 100, 100} // high humidity
};
static FuzzySet fPwrObjs[5] = {
    {0, 0, 20, 30},    // very low power
    {20, 30, 40, 50},  // low power
    {35, 40, 60, 65},  // medium power
    {55, 60, 80, 85},  // high power
    {80, 90, 100, 100} // very high power
};

// 2. mảng con trỏ tới các fuzzy set
FuzzySet *fTemp[5] = {&fTempObjs[0], &fTempObjs[1], &fTempObjs[2], &fTempObjs[3], &fTempObjs[4]};
FuzzySet *fHum[3] = {&fHumObjs[0], &fHumObjs[1], &fHumObjs[2]};
FuzzySet *fPwr[5] = {&fPwrObjs[0], &fPwrObjs[1], &fPwrObjs[2], &fPwrObjs[3], &fPwrObjs[4]};

// 3. helper thêm fuzzy sets
void addFuzzySets(FuzzyInput *in, FuzzySet *sets[], int n)
{
  for (int i = 0; i < n; ++i)
    in->addFuzzySet(sets[i]);
}
void addFuzzySets(FuzzyOutput *out, FuzzySet *sets[], int n)
{
  for (int i = 0; i < n; ++i)
    out->addFuzzySet(sets[i]);
}

// 4. mô tả luật
struct RuleDesc
{
  TempLevel t;
  HumLevel h;
  PowerLevel p;
};
static const RuleDesc rules[] = {
    {VHOT, HIGHH, VIGHP},
    {HOT, MEDH, HIGHP},
    {HOT, HIGHH, VIGHP},
    {WARM, MEDH, MEDP},
    {WARM, HIGHH, MEDP},
    // very cold or cold + low humidity => very low power
    {VCOLD, LOWH, VPWR},
    {COLD, LOWH, VPWR}};

void setupFuzzy()
{
  static FuzzyInput tempIn(1), humIn(2);
  static FuzzyOutput powerOut(1);

  addFuzzySets(&tempIn, fTemp, 5);
  addFuzzySets(&humIn, fHum, 3);
  addFuzzySets(&powerOut, fPwr, 5);

  fuzzy.addFuzzyInput(&tempIn);
  fuzzy.addFuzzyInput(&humIn);
  fuzzy.addFuzzyOutput(&powerOut);

  uint16_t id = 1;
  for (auto &r : rules)
  {
    auto *ant = new FuzzyRuleAntecedent();
    if ((r.t == VCOLD || r.t == COLD) && r.h == LOWH && r.p == VPWR)
    {
      ant->joinWithOR(&fTempObjs[VCOLD], &fTempObjs[COLD]);
    }
    else
    {
      ant->joinWithAND(&fTempObjs[r.t], &fHumObjs[r.h]);
    }
    auto *cons = new FuzzyRuleConsequent();
    cons->addOutput(&fPwrObjs[r.p]);
    fuzzy.addFuzzyRule(new FuzzyRule(id++, ant, cons));
  }
}

// 5. tính heat index đúng đơn vị celsius
float computeHI(float T_C, float H)
{
  // chuyển °C -> °F
  float T_F = T_C * 9.0f / 5.0f + 32.0f;
  // công thức NOAA bậc 4, kết quả °F
  float HI_F = -42.379f + 2.04901523f * T_F + 10.14333127f * H - 0.22475541f * T_F * H - 0.00683783f * T_F * T_F - 0.05481717f * H * H + 0.00122874f * T_F * T_F * H + 0.00085282f * T_F * H * H - 0.00000199f * T_F * T_F * H * H;
  // chuyển ngược về °C
  return (HI_F - 32.0f) * 5.0f / 9.0f;
}

void setup()
{
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  dht.begin();
  delay(2000);
  setupFuzzy();

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(EN, OUTPUT);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void loop()
{
  float T = dht.readTemperature();
  float H = dht.readHumidity();
  if (isnan(T) || isnan(H))
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("sensor error");
    delay(2000);
    return;
  }

  float HIc = computeHI(T, H);
  fuzzy.setInput(1, HIc);
  fuzzy.setInput(2, H);
  fuzzy.fuzzify();
  float P = fuzzy.defuzzify(1);

  uint8_t pwm = (uint8_t)round((P / 100.0f) * 255);
  analogWrite(EN, pwm);

  // debug serial
  Serial.print("Temperature = ");
  Serial.print(T, 1);
  Serial.print("°C ");
  Serial.print("Humidity = ");
  Serial.print(H, 1);
  Serial.print("% ");
  Serial.print("Heat Index = ");
  Serial.print(HIc, 1);
  Serial.print("°C ");
  Serial.print("Power = ");
  Serial.print(P, 1);
  Serial.println("%");

  // hiển thị LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" T:");
  lcd.print(T, 1);
  lcd.write(223);
  lcd.print("C H:");
  lcd.print(H, 0);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("HI:");
  lcd.print(HIc, 1);
  lcd.write(223);
  lcd.print("C P:");
  lcd.print((int)round(P));
  lcd.print("%");

  delay(2000);
}