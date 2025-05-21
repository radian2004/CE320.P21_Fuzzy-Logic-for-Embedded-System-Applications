#include <DHT.h>
#include <Fuzzy.h>
#include <LiquidCrystal_I2C.h> 

// ============== < DEFINES > ==============
#define DHTTYPE DHT22
// #define DHTPIN 5
// #define EN 6
// #define IN1 7
// #define IN2 8

#define DHTPIN 15
#define EN 18
#define IN1 5
#define IN2 17

#define MOTOR_RES 255



// Heat index set ranges
#define VERY_COLD_SET 0, 0, 20, 26.7
#define COLD_SET 20, 26.7, 32.2, 35
#define WARM_SET 30, 32.2, 40.6, 45
#define HOT_SET 40, 40.6, 54.4, 60
#define VERY_HOT_SET 54.4, 60, 65, 65

// Power set ranges
#define VERY_LOW_SET 0, 0, 20, 30
#define LOW_SET 20, 30, 40, 50
#define MEDIUM_SET 35, 40, 60, 65
#define HIGH_SET 55, 60, 80, 85
#define VERY_HIGH_SET 80, 90, 100, 100

// =========================================

// ============ < GLOBAL VARS > ============
LiquidCrystal_I2C lcd(0x27, 16, 2); 
DHT dht(DHTPIN, DHTTYPE);
Fuzzy fuzzy;

// Fuzzy sets
static FuzzySet heat_index_fsets[] = {
    {VERY_COLD_SET}, {COLD_SET}, {WARM_SET}, {HOT_SET}, {VERY_HOT_SET}
};
static FuzzySet power_fsets[] = {
    {VERY_LOW_SET}, {LOW_SET}, {MEDIUM_SET}, {HIGH_SET}, {VERY_HIGH_SET}
};

// Fuzzy input and output
static FuzzyInput heat_index(1);
static FuzzyOutput power(1);

// =========================================

// ======== < FUNCTION PROTOTYPES > ========
void addFuzzySets(FuzzyInput *input, FuzzySet *fuzzy_sets, int count);
void addFuzzySets(FuzzyOutput *output, FuzzySet *fuzzy_sets, int count);
void addFuzzyRules(FuzzySet *heat_index_fsets, FuzzySet *power_fsets, Fuzzy *fuzzy);
float computeHI(float T_C, float H);

// =========================================

// =============== < SETUP > ===============
void setup() {
    Serial.begin(9600);
    dht.begin();
    lcd.init(); // Khởi tạo LCD
    lcd.backlight(); // Bật đèn nền LCD
    delay(2000); // Wait for sensor to stabilize

    // Check sensor
    if (isnan(dht.readTemperature()) || isnan(dht.readHumidity())) {
        Serial.println("DHT sensor error! Check wiring.");
        while (true); // Stop if sensor fails
    }

    // Add fuzzy sets
    addFuzzySets(&heat_index, heat_index_fsets, 5);
    addFuzzySets(&power, power_fsets, 5);

    // Add to fuzzy system
    fuzzy.addFuzzyInput(&heat_index);
    fuzzy.addFuzzyOutput(&power);

    // Add rules
    addFuzzyRules(heat_index_fsets, power_fsets, &fuzzy);

    // Motor setup
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(EN, OUTPUT);
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
}

// =========================================

// =============== < LOOP > ================
void loop() {
    static uint8_t cycle = 0;

    if (++cycle >= 5) { // Read every 10 seconds (5 cycles * 2s delay)
        cycle = 0;
        float T = dht.readTemperature();
        float H = dht.readHumidity();

        // Check for invalid readings
        if (isnan(T) || isnan(H) || T < 0 || T > 60 || H < 0 || H > 100) {
            Serial.println("Sensor error!");
            return;
        }

        float HI = computeHI(T, H);
        fuzzy.setInput(1, HI);
        fuzzy.fuzzify();
        float P = fuzzy.defuzzify(1);

        // Control motor
        uint8_t pwm = (uint8_t)round((P / 100.0f) * MOTOR_RES);
        analogWrite(EN, pwm);

        // Verbose serial output as requested
        Serial.print("Temperature = ");
        Serial.print(T, 1);
        Serial.print("°C ");
        Serial.print("Humidity = ");
        Serial.print(H, 1);
        Serial.print("% ");
        Serial.print("Heat Index = ");
        Serial.print(HI, 1);
        Serial.print("°C ");
        Serial.print("Power = ");
        Serial.print(P, 1);
        Serial.println("%");

        lcd.clear();
        lcd.setCursor(1, 0);
        lcd.print("T:");
        lcd.print(T, 1);
        lcd.write(223); // Ký hiệu độ (°)
        lcd.print("C H:");
        lcd.print(H, 0);
        lcd.print("%");
        lcd.setCursor(0, 1);
        lcd.print("HI:");
        lcd.print(HI, 1); // Sửa từ HIc thành HI
        lcd.write(223); // Ký hiệu độ (°)
        lcd.print("C P:");
        lcd.print((int)round(P));
        lcd.print("%");
    }
    delay(1000);
}

// =========================================

// ======= < FUNCTION DEFINITIONS > ========
void addFuzzySets(FuzzyInput *input, FuzzySet *fuzzy_sets, int count) {
    for (int i = 0; i < count; i++) {
        input->addFuzzySet(&fuzzy_sets[i]);
    }
}

void addFuzzySets(FuzzyOutput *output, FuzzySet *fuzzy_sets, int count) {
    for (int i = 0; i < count; i++) {
        output->addFuzzySet(&fuzzy_sets[i]);
    }
}

void addFuzzyRules(FuzzySet *heat_index_fsets, FuzzySet *power_fsets, Fuzzy *fuzzy) {
    for (int i = 0; i < 5; i++) {
        auto *antecedence = new FuzzyRuleAntecedent();
        antecedence->joinSingle(&heat_index_fsets[i]);
        auto *consequence = new FuzzyRuleConsequent();
        consequence->addOutput(&power_fsets[i]);
        fuzzy->addFuzzyRule(new FuzzyRule(i + 1, antecedence, consequence));
    }
}

float computeHI(float T_C, float H) {
    // Convert to Fahrenheit
    float T_F = T_C * 9.0f / 5.0f + 32.0f;
    // NOAA Heat Index formula
    float HI_F = -42.379f + 2.04901523f * T_F + 10.14333127f * H - 0.22475541f * T_F * H - 
                 0.00683783f * T_F * T_F - 0.05481717f * H * H + 0.00122874f * T_F * T_F * H + 
                 0.00085282f * T_F * H * H - 0.00000199f * T_F * T_F * H * H;
    // Convert back to Celsius
    return (HI_F - 32.0f) * 5.0f / 9.0f;
}

// =========================================