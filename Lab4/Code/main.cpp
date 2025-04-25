#include <Fuzzy.h>

// Define pins
#define POT_PIN 32
#define LED_1 5
#define LED_2 17
#define LED_3 16
#define LED_4 4
#define LED_5 0
#define BUTTON_1 18
#define BUTTON_2 19
#define BUTTON_3 21

// Initialize fuzzy logic object
Fuzzy* fuzzy = new Fuzzy();

// Fuzzy sets for CurrentSpeed (0-70)
FuzzySet* slowSpeed = new FuzzySet(0, 0, 12, 20);
FuzzySet* mediumSpeed = new FuzzySet(15, 25, 25, 35);
FuzzySet* fastSpeed = new FuzzySet(30, 40, 40, 70);

// Fuzzy sets for DeltaV (-70 to +70)
FuzzySet* largeNegDelta = new FuzzySet(-70, -70, -55, -35);
FuzzySet* smallNegDelta = new FuzzySet(-35, -35, -15, -1);
FuzzySet* smallPosDelta = new FuzzySet(1, 15, 35, 35);
FuzzySet* largePosDelta = new FuzzySet(35, 55, 70, 70);

// Fuzzy sets for Distance (1-3)
FuzzySet* notNearDist = new FuzzySet(1, 1, 1.5, 2);
FuzzySet* nearDist = new FuzzySet(1.5, 2, 2, 2.5);
FuzzySet* veryNearDist = new FuzzySet(2.5, 3, 3, 3);

// Fuzzy sets for PWM Reduction (-18 to +18)
FuzzySet* fastDecrease = new FuzzySet(-18, -18, -13, -10);
FuzzySet* slowDecrease = new FuzzySet(-10, -10, -7, -3);
FuzzySet* slowIncrease = new FuzzySet(3, 7, 10, 10);
FuzzySet* fastIncrease = new FuzzySet(10, 13, 18, 18);

// Fuzzy inputs and output
FuzzyInput* currentSpeedInput = new FuzzyInput(1);
FuzzyInput* deltaVInput = new FuzzyInput(2);
FuzzyInput* distanceInput = new FuzzyInput(3);
FuzzyOutput* pwmReductionOutput = new FuzzyOutput(1);

void setup() {
  Serial.begin(115200);

  // Configure pins
  for (int i = LED_1; i <= LED_5; i++) pinMode(i, OUTPUT);


  // Add fuzzy sets to inputs and output
  currentSpeedInput->addFuzzySet(slowSpeed);
  currentSpeedInput->addFuzzySet(mediumSpeed);
  currentSpeedInput->addFuzzySet(fastSpeed);
  
  deltaVInput->addFuzzySet(largeNegDelta);
  deltaVInput->addFuzzySet(smallNegDelta);
  deltaVInput->addFuzzySet(smallPosDelta);
  deltaVInput->addFuzzySet(largePosDelta);
  
  distanceInput->addFuzzySet(notNearDist);
  distanceInput->addFuzzySet(nearDist);
  distanceInput->addFuzzySet(veryNearDist);
  
  pwmReductionOutput->addFuzzySet(fastDecrease);
  pwmReductionOutput->addFuzzySet(slowDecrease);
  pwmReductionOutput->addFuzzySet(slowIncrease);
  pwmReductionOutput->addFuzzySet(fastIncrease);

  // Add to fuzzy system
  fuzzy->addFuzzyInput(currentSpeedInput);
  fuzzy->addFuzzyInput(deltaVInput);
  fuzzy->addFuzzyInput(distanceInput);
  fuzzy->addFuzzyOutput(pwmReductionOutput);

  // Setup fuzzy rules
  setupFuzzyRules();
  
  Serial.println("Fuzzy Speed Controller Ready");
}

void setupFuzzyRules() {
  // Rule 1-12: For NotNear distance
  addRule(slowSpeed, largePosDelta, notNearDist, fastIncrease);
  addRule(slowSpeed, smallPosDelta, notNearDist, slowIncrease);
  addRule(slowSpeed, largeNegDelta, notNearDist, fastDecrease);
  addRule(slowSpeed, smallNegDelta, notNearDist, slowDecrease);
  
  addRule(mediumSpeed, largePosDelta, notNearDist, fastIncrease);
  addRule(mediumSpeed, smallPosDelta, notNearDist, slowIncrease);
  addRule(mediumSpeed, largeNegDelta, notNearDist, fastDecrease);
  addRule(mediumSpeed, smallNegDelta, notNearDist, slowDecrease);
  
  addRule(fastSpeed, largePosDelta, notNearDist, slowIncrease);
  addRule(fastSpeed, smallPosDelta, notNearDist, slowIncrease);
  addRule(fastSpeed, largeNegDelta, notNearDist, fastDecrease);
  addRule(fastSpeed, smallNegDelta, notNearDist, slowDecrease);

  // Rule 13-15: For Near distance
  addRule(slowSpeed, NULL, nearDist, slowDecrease);
  addRule(mediumSpeed, NULL, nearDist, slowDecrease);
  addRule(fastSpeed, NULL, nearDist, fastDecrease);

  // Rule 16-18: For VeryNear distance
  addRule(slowSpeed, NULL, veryNearDist, slowDecrease);
  addRule(mediumSpeed, NULL, veryNearDist, fastDecrease);
  addRule(fastSpeed, NULL, veryNearDist, fastDecrease);
}

void addRule(FuzzySet* speedSet, FuzzySet* deltaSet, FuzzySet* distSet, FuzzySet* outputSet) {
  FuzzyRuleAntecedent* antecedent = new FuzzyRuleAntecedent();
  
  if (deltaSet == NULL) {
    // Only use speed and distance
    antecedent->joinWithAND(speedSet, distSet);
  } else {
    // Use all three inputs
    FuzzyRuleAntecedent* temp = new FuzzyRuleAntecedent();
    temp->joinWithAND(speedSet, deltaSet);
    antecedent->joinWithAND(temp, distSet);
  }

  FuzzyRuleConsequent* consequent = new FuzzyRuleConsequent();
  consequent->addOutput(outputSet);

  static int ruleNum = 1;
  fuzzy->addFuzzyRule(new FuzzyRule(ruleNum++, antecedent, consequent));
}

void updateLEDs(int speed) {
  // Turn off all LEDs first
  for (int i = LED_1; i <= LED_5; i++) digitalWrite(i, LOW);
  
  // Calculate how many LEDs to light (0-5)
  int leds = map(speed, 0, 70, 0, 5);
  for (int i = 0; i < leds; i++) digitalWrite(LED_1 + i, HIGH);
}

int distanceLevel = 1;
float currentSpeed = 0;
void loop() {
  
  static int lastDistance = 0;
  
  // Read inputs 
  int expectedSpeed = analogRead(POT_PIN) / 14.6; // Convert 0-1023 to 0-70
  int deltaV = expectedSpeed - currentSpeed;
  
  // Read distance level from buttons (1=NotNear, 2=Near, 3=VeryNear)

int BT1 = digitalRead(BUTTON_1); // pin 11
int BT2 = digitalRead(BUTTON_2); // pin 12
int BT3 = digitalRead(BUTTON_3); // pin 13



if (BT1 == HIGH) {
  distanceLevel = 1; // NotNear
} else if (BT2 == HIGH) {
  distanceLevel = 2; // Near
} else if (BT3 == HIGH) {
  distanceLevel = 3; // VeryNear
}


  // Set fuzzy inputs
  fuzzy->setInput(1, currentSpeed);
  fuzzy->setInput(2, deltaV);
  fuzzy->setInput(3, distanceLevel);
  
  // Run fuzzy logic
  fuzzy->fuzzify();
  float pwmAdjustment = fuzzy->defuzzify(1);
  
  // Update current speed (with smoothing)
  currentSpeed += pwmAdjustment * 0.2;
  currentSpeed = constrain(currentSpeed, 0, 70);
  
  // Update LEDs
  updateLEDs(currentSpeed);
  
  // Print to Serial Plotter
  Serial.print("Current:"); Serial.print(currentSpeed);
  Serial.print(",Expected:"); Serial.print(expectedSpeed);
  Serial.print(",Delta:"); Serial.print(deltaV);
  Serial.print(",Distance:"); Serial.print(distanceLevel);
  Serial.print(",Adjust:"); Serial.println(pwmAdjustment);
  
  delay(50);
}