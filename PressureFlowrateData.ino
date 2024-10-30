const int baudRate = 9600; 
const int delayRead = 1000;
const int numReadings = 10; // Number of readings for averaging

const float trueVoltage = 5;
const float minVoltage = 0.5 * 1024 / trueVoltage;
const float maxVoltage = 4.5 * 1024 / trueVoltage;
const float minPressure = 5.0;
const float maxPressure = 300.0;

// Pressure sensor pins
const int pressureInput1 = A0; // Pressure Sensor 1
const int pressureInput2 = A1; // Pressure Sensor 2
float pressureReadings1[numReadings]; // Array for sensor 1 readings
float pressureReadings2[numReadings]; // Array for sensor 2 readings
int currentIndex = 0; // Current index for averaging
float totalPressure1 = 0; // Total of current readings for sensor 1
float totalPressure2 = 0; // Total of current readings for sensor 2
float averagePressure1 = 0; // Average pressure for sensor 1
float averagePressure2 = 0; // Average pressure for sensor 2

// Flow sensor
#define FLOWSENSOR_PIN 2
volatile uint8_t lastflowpinstate;
volatile float flowrate;
volatile float flowpermin;
volatile uint32_t lastflowratetimer = 0; // Timer to track time between pulses
volatile uint32_t currentTimer = 0; // To capture pulse timing intervals

void setup() {
  Serial.begin(baudRate);
  
  // Initialize pressure readings arrays
  for (int i = 0; i < numReadings; i++) {
    pressureReadings1[i] = 0;
    pressureReadings2[i] = 0;
  }
  
  // Setup flow sensor
  pinMode(FLOWSENSOR_PIN, INPUT);
  digitalWrite(FLOWSENSOR_PIN, HIGH);
  lastflowpinstate = digitalRead(FLOWSENSOR_PIN);
  useInterrupt(true);
}

void useInterrupt(boolean v) {
  if (v) {
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
  } else {
    TIMSK0 &= ~_BV(OCIE0A);
  }
}

SIGNAL(TIMER0_COMPA_vect) {
  uint8_t x = digitalRead(FLOWSENSOR_PIN);
  
  // Only act on a change from LOW to HIGH (a pulse)
  if (x != lastflowpinstate && x == HIGH) {
    // Calculate time between pulses
    currentTimer = millis();
    flowrate = 1000.0 / (currentTimer - lastflowratetimer); // Frequency in Hz
    flowpermin = flowrate / 7.5; // Convert to L/min
    lastflowratetimer = currentTimer; // Update last pulse time
  }
  lastflowpinstate = x;
}

void loop() {
  // Read sensor values and calculate pressures
  float sensorVal1 = (float)analogRead(pressureInput1);
  float sensorVal2 = (float)analogRead(pressureInput2);

  // Convert readings to pressure
  float pressureVal1 = ((maxPressure - minPressure) / (maxVoltage - minVoltage)) * sensorVal1 + minPressure - ((maxPressure - minPressure) / (maxVoltage - minVoltage)) * minVoltage;
  float pressureVal2 = ((maxPressure - minPressure) / (maxVoltage - minVoltage)) * sensorVal2 + minPressure - ((maxPressure - minPressure) / (maxVoltage - minVoltage)) * minVoltage;

  // Update the averaging process for both sensors
  totalPressure1 -= pressureReadings1[currentIndex]; // Subtract the oldest reading for sensor 1
  totalPressure2 -= pressureReadings2[currentIndex]; // Subtract the oldest reading for sensor 2
  
  pressureReadings1[currentIndex] = pressureVal1; // Store the new reading for sensor 1
  pressureReadings2[currentIndex] = pressureVal2; // Store the new reading for sensor 2

  totalPressure1 += pressureReadings1[currentIndex]; // Add the new reading for sensor 1
  totalPressure2 += pressureReadings2[currentIndex]; // Add the new reading for sensor 2

  // Move to the next index and wrap around
  currentIndex = (currentIndex + 1) % numReadings;

  // Calculate the average pressures
  averagePressure1 = totalPressure1 / numReadings;
  averagePressure2 = totalPressure2 / numReadings;

  // Calculate pressure difference
  float pressureDifference = averagePressure1 - averagePressure2;

  // Print the average pressures and flow rate
  Serial.print("Pressure Sensor 1: ");
  Serial.print(averagePressure1);
  Serial.print(", Pressure Sensor 2: ");
  Serial.print(averagePressure2);
  Serial.print(", Pressure Difference: ");
  Serial.print(pressureDifference);
  Serial.print(", Flow Rate (L/min): ");
  Serial.println(flowpermin);

  delay(delayRead);
}