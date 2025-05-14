#include <DHT.h>
#include <Fuzzy.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 5
#define DHTTYPE DHT22
#define IN1 7
#define IN2 8
#define EN   6

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
Fuzzy* fuzzy = new Fuzzy();

// enums cho readability
typedef enum { _V_COLD, _COLD, _WARM, _HOT, _V_HOT } TempLevel;
typedef enum { _LOW_H, _MED_H, _HIGH_H } HumLevel;
typedef enum { _V_LOW, _LOW, _MEDIUM, _HIGH, _V_HIGH } PowerLevel;

// hàm tiện thêm fuzzy sets
void addFuzzySets(FuzzyInput* in, FuzzySet* sets[], int n) {
  for(int i = 0; i < n; i++) in->addFuzzySet(sets[i]);
}
void addFuzzySets(FuzzyOutput* out, FuzzySet* sets[], int n) {
  for(int i = 0; i < n; i++) out->addFuzzySet(sets[i]);
}

void setup() {
  Serial.begin(9600);
  lcd.init(); lcd.backlight();
  dht.begin();
  delay(2000);

  //--- khai báo fuzzy sets nhiệt độ ---
  const float tVC[4] = {0,   0,   20, 26.7};
  const float tC [4] = {20, 26.7, 32.2, 35};
  const float tW [4] = {30, 32.2, 40.6, 45};
  const float tH [4] = {40, 40.6, 54.4, 60};
  const float tVH[4] = {54.4, 60,  65, 65};
  FuzzySet* fTemp[] = {
    new FuzzySet(tVC[0], tVC[1], tVC[2], tVC[3]),
    new FuzzySet(tC [0], tC [1], tC [2], tC [3]),
    new FuzzySet(tW [0], tW [1], tW [2], tW [3]),
    new FuzzySet(tH [0], tH [1], tH [2], tH [3]),
    new FuzzySet(tVH[0],tVH[1],tVH[2],tVH[3])
  };

  //--- khai báo fuzzy sets độ ẩm ---
  const float hL [4] = {0,   0,   30, 50};
  const float hM [4] = {30,  50,  70, 90};
  const float hH [4] = {70,  90, 100,100};
  FuzzySet* fHum[] = {
    new FuzzySet(hL[0], hL[1], hL[2], hL[3]),
    new FuzzySet(hM[0], hM[1], hM[2], hM[3]),
    new FuzzySet(hH[0], hH[1], hH[2], hH[3])
  };

  //--- khai báo fuzzy sets công suất quạt ---
  const float pVL[4] = {0,   0,   20, 30};
  const float pL [4] = {20,  30,  40, 50};
  const float pM [4] = {35,  40,  60, 65};
  const float pH [4] = {55,  60,  80, 85};
  const float pVH[4] = {80,  90, 100,100};
  FuzzySet* fPower[] = {
    new FuzzySet(pVL[0],pVL[1],pVL[2],pVL[3]),
    new FuzzySet(pL [0],pL [1],pL [2],pL [3]),
    new FuzzySet(pM [0],pM [1],pM [2],pM [3]),
    new FuzzySet(pH [0],pH [1],pH [2],pH [3]),
    new FuzzySet(pVH[0],pVH[1],pVH[2],pVH[3])
  };

  //--- tạo input/output fuzzy
  FuzzyInput* tempInput = new FuzzyInput(1);
  FuzzyInput* humInput  = new FuzzyInput(2);
  FuzzyOutput* powerOut = new FuzzyOutput(1);

  addFuzzySets(tempInput, fTemp,   5);
  addFuzzySets(humInput,  fHum,    3);
  addFuzzySets(powerOut,  fPower,  5);

  fuzzy->addFuzzyInput(tempInput);
  fuzzy->addFuzzyInput(humInput);
  fuzzy->addFuzzyOutput(powerOut);

  //--- thêm luật mờ ---
  // 1. very_hot AND high_humidity -> very_high power
  {
    auto* ant = new FuzzyRuleAntecedent();
    ant->joinWithAND(fTemp[_V_HOT], fHum[_HIGH_H]);
    auto* cons = new FuzzyRuleConsequent();
    cons->addOutput(fPower[_V_HIGH]);
    fuzzy->addFuzzyRule(new FuzzyRule(1, ant, cons));
  }
  // 2. hot AND medium_humidity -> high power
  {
    auto* ant = new FuzzyRuleAntecedent();
    ant->joinWithAND(fTemp[_HOT], fHum[_MED_H]);
    auto* cons = new FuzzyRuleConsequent();
    cons->addOutput(fPower[_HIGH]);
    fuzzy->addFuzzyRule(new FuzzyRule(2, ant, cons));
  }
  // 3. hot AND high_humidity -> very_high power
  {
    auto* ant = new FuzzyRuleAntecedent();
    ant->joinWithAND(fTemp[_HOT], fHum[_HIGH_H]);
    auto* cons = new FuzzyRuleConsequent();
    cons->addOutput(fPower[_V_HIGH]);
    fuzzy->addFuzzyRule(new FuzzyRule(3, ant, cons));
  }
  // 4. warm AND medium_humidity -> medium power
  {
    auto* ant = new FuzzyRuleAntecedent();
    ant->joinWithAND(fTemp[_WARM], fHum[_MED_H]);
    auto* cons = new FuzzyRuleConsequent();
    cons->addOutput(fPower[_MEDIUM]);
    fuzzy->addFuzzyRule(new FuzzyRule(4, ant, cons));
  }
  // 5. warm AND high_humidity -> medium power
  {
    auto* ant = new FuzzyRuleAntecedent();
    ant->joinWithAND(fTemp[_WARM], fHum[_HIGH_H]);
    auto* cons = new FuzzyRuleConsequent();
    cons->addOutput(fPower[_MEDIUM]);
    fuzzy->addFuzzyRule(new FuzzyRule(5, ant, cons));
  }
  // 6. cold OR very_cold -> very_low power
  {
    auto* ant = new FuzzyRuleAntecedent();
    ant->joinWithOR(fTemp[_V_COLD], fTemp[_COLD]);
    auto* cons = new FuzzyRuleConsequent();
    cons->addOutput(fPower[_V_LOW]);
    fuzzy->addFuzzyRule(new FuzzyRule(6, ant, cons));
  }

  //--- thiết lập motor ---
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(EN,   OUTPUT);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void loop() {
  float T = dht.readTemperature();
  float H = dht.readHumidity();
  if (isnan(T) || isnan(H)) {
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Sensor error!");
    Serial.println("DHT read failed");
    delay(2000);
    return;
  }
  float HI = dht.computeHeatIndex(T, H, false);

  // fuzzy process
  fuzzy->setInput(1, HI);
  fuzzy->setInput(2, H);
  fuzzy->fuzzify();
  float P = fuzzy->defuzzify(1);

  // điều khiển PWM
  analogWrite(EN, (uint8_t)((P/100.0)*255));

  // debug ra Serial
  Serial.print("T="); Serial.print(T,1);
  Serial.print("C H="); Serial.print(H,1);
  Serial.print("% HI="); Serial.print(HI,1);
  Serial.print("C P="); Serial.print(P,1); Serial.println("%");

  // hiển thị lên LCD
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("T:"); lcd.print(T,1);
  lcd.print("C H:"); lcd.print(H,0); lcd.print("%");
  lcd.setCursor(0,1);
  lcd.print("HI:"); lcd.print(HI,1);
  lcd.print("C P:"); lcd.print(P,0); lcd.print("%");

  delay(2000);
}
