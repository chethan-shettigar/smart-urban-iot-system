/*
 * Smart Dustbin System (IoT-Based Waste Segregation)
 *
 * This project implements an automated waste segregation system using ESP32.
 * It detects dry, wet, and metal waste using sensors and directs them into
 * respective compartments using a stepper motor mechanism.
 *
 * Features:
 * - Automatic waste detection (Dry / Wet / Metal)
 * - Servo-controlled lid mechanism
 * - Stepper motor-based bin positioning
 * - LCD display for real-time status
 * - Blynk IoT integration for remote monitoring and control
 * - Waste count tracking and reset functionality
 *
 * Applications:
 * - Smart Cities
 * - Automated Waste Management Systems
 */

// Blynk Template and Auth Token
#define BLYNK_TEMPLATE_ID "TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "SMART DUSTBIN SYSTEM"
#define BLYNK_AUTH_TOKEN "YOUR_AUTH_TOKEN"

// Wi-Fi credentials
char ssid[] = "YOUR_WIFI";     // Wi-Fi SSID
char pass[] = "YOUR_PASSWORD"; // Wi-Fi Password

#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

// LCD Configuration
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Servo Motor
Servo lidServo;

// Stepper Motor Configuration
#define IN1 2
#define IN2 5
#define IN3 18
#define IN4 19
#define STEPS_PER_REV 4096 // 28BYJ-48 motor in half-step mode

// Positions for each compartment
const int DEFAULT_POSITION = 0;
const int DRY_WASTE_POSITION = 1015;
const int WET_WASTE_POSITION = 2040;
const int METAL_WASTE_POSITION = 3050;

// Sensor and Buzzer Pins
#define DRY_SENSOR 34
#define WET_SENSOR 25
#define METAL_SENSOR 26
#define BUZZER 23

// Motor Control Variables
const int STEP_DELAY = 2;
int currentPosition = DEFAULT_POSITION;
int stepIndex = 0;

// Global variables for waste count
int metalCount = 0;
int dryCount = 0;
int wetCount = 0;

// Virtual reset button in Blynk
#define RESET_PIN V4  // Virtual pin for the reset button
#define BUZZER_PIN V5 // Virtual pin for controlling the buzzer

// Track last screen update
unsigned long lastIdleUpdate = 0;

// Variable to store buzzer state (on/off)
bool buzzerState = true; // Default: buzzer is ON

void setup()
{
    Serial.begin(115200);

    // Connect to Wi-Fi and Blynk
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

    // Initialize LCD
    lcd.init();
    lcd.backlight();
    displayWelcomeMessage();

    // Initialize Servo Motor
    lidServo.attach(4);
    lidServo.write(0); // Close lid initially

    // Set sensor pins
    pinMode(DRY_SENSOR, INPUT);
    pinMode(WET_SENSOR, INPUT);
    pinMode(METAL_SENSOR, INPUT);
    pinMode(BUZZER, OUTPUT);

    // Set Stepper motor pins
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);

    Serial.println("Setup complete. Waiting for waste detection...");
}

void loop()
{
    Blynk.run(); // Keep Blynk running

    // Check sensors and update counters
    if (digitalRead(DRY_SENSOR) == LOW)
    { // Dry waste detected
        processWaste("Dry Waste", DRY_WASTE_POSITION);
    }
    else if (digitalRead(WET_SENSOR) == LOW)
    { // Wet waste detected
        processWaste("Wet Waste", WET_WASTE_POSITION);
    }
    else if (digitalRead(METAL_SENSOR) == LOW)
    { // Metal waste detected
        processWaste("Metal Waste", METAL_WASTE_POSITION);
    }
    else
    {
        // If idle for more than 5 seconds, display the welcome message
        if (millis() - lastIdleUpdate > 5000)
        {
            displayWelcomeMessage();
            lastIdleUpdate = millis();
        }
    }

    delay(1000); // Check every 1 second
}

void displayWelcomeMessage()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Smart Bin Ready");
    lcd.setCursor(0, 1);
    lcd.print("Keep it Clean!");
}

void processWaste(const char *type, int targetPosition)
{
    // Display waste detection message
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Smart Sorting...");
    lcd.setCursor(0, 1);
    lcd.print(type);
    lcd.print(" Detected!");

    if (buzzerState)
    {
        activateBuzzer(); // Activate buzzer if the buzzer state is ON
    }

    moveToPosition(targetPosition);
    operateLid(true);                 // Open the lid
    delay(2000);                      // Wait for waste disposal
    operateLid(false);                // Close the lid
    moveToPosition(DEFAULT_POSITION); // Move back to the default position

    updateWasteCount(type); // Update waste count for the type detected

    // After disposal, show thank you message for 2 seconds
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Thank You!");
    lcd.setCursor(0, 1);
    lcd.print("Be Eco-Friendly!");
    delay(2000);

    // Display updated item count on the LCD
    displayItemCount();
}

// Function to update waste count
void updateWasteCount(const char *type)
{
    if (strcmp(type, "Metal Waste") == 0)
    {
        metalCount++;
        Blynk.virtualWrite(V1, metalCount);
    }
    else if (strcmp(type, "Dry Waste") == 0)
    {
        dryCount++;
        Blynk.virtualWrite(V2, dryCount);
    }
    else if (strcmp(type, "Wet Waste") == 0)
    {
        wetCount++;
        Blynk.virtualWrite(V3, wetCount);
    }
}

// Function to display the count of items on the LCD
void displayItemCount()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Items:");
    lcd.setCursor(0, 1);
    lcd.print("M: ");
    lcd.print(metalCount);
    lcd.print(" D: ");
    lcd.print(dryCount);
    lcd.print(" W: ");
    lcd.print(wetCount);
    delay(2000);
}

// Function to activate the buzzer
void activateBuzzer()
{
    digitalWrite(BUZZER, HIGH);
    delay(1000);
    digitalWrite(BUZZER, LOW);
}

// Function to open/close the lid
void operateLid(bool open)
{
    if (open)
    {
        lidServo.write(90); // Open lid
    }
    else
    {
        lidServo.write(0); // Close lid
    }
}

// Function to move the stepper motor to the target position
void moveToPosition(int targetPosition)
{
    if (targetPosition != currentPosition)
    {
        int stepsNeeded = calculateOptimalSteps(targetPosition, currentPosition);
        stepMotor(stepsNeeded);
        currentPosition = targetPosition % STEPS_PER_REV;
    }
    else
    {
        Serial.println("No movement needed. Already at the target position.");
    }
}

// Function to calculate the shortest step distance
int calculateOptimalSteps(int target, int current)
{
    int steps = target - current;
    if (steps > STEPS_PER_REV / 2)
    {
        steps -= STEPS_PER_REV;
    }
    else if (steps < -STEPS_PER_REV / 2)
    {
        steps += STEPS_PER_REV;
    }
    return steps;
}

// Function to step the motor
void stepMotor(int steps)
{
    if (steps == 0)
        return;

    int direction = (steps > 0) ? 1 : -1;
    for (int i = 0; i < abs(steps); i++)
    {
        updateStepIndex(direction);
        executeStep(stepIndex);
        delay(STEP_DELAY);
    }
}

// Function to update step index
void updateStepIndex(int direction)
{
    if (direction == 1)
    {
        stepIndex = (stepIndex + 1) % 4;
    }
    else
    {
        stepIndex = (stepIndex - 1 + 4) % 4;
    }
}

// Step sequence for the stepper motor
const int stepSequence[4][4] = {
    {HIGH, HIGH, LOW, LOW},
    {LOW, HIGH, HIGH, LOW},
    {LOW, LOW, HIGH, HIGH},
    {HIGH, LOW, LOW, HIGH}};

// Function to execute a step for the motor
void executeStep(int step)
{
    setMotorPins(stepSequence[step]);
}

// Function to set motor pins for a given step
void setMotorPins(const int pins[4])
{
    digitalWrite(IN1, pins[0]);
    digitalWrite(IN2, pins[1]);
    digitalWrite(IN3, pins[2]);
    digitalWrite(IN4, pins[3]);
}

// Blynk function to reset the counts
BLYNK_WRITE(RESET_PIN)
{
    metalCount = 0;
    dryCount = 0;
    wetCount = 0;
    Blynk.virtualWrite(V1, metalCount);
    Blynk.virtualWrite(V2, dryCount);
    Blynk.virtualWrite(V3, wetCount);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Data Reset!");
    lcd.setCursor(0, 1);
    lcd.print("Fresh Start 🚀");
    delay(2000);
    displayWelcomeMessage();
}

// Blynk function to control buzzer remotely
BLYNK_WRITE(BUZZER_PIN)
{
    buzzerState = param.asInt(); // If 1, buzzer ON; if 0, buzzer OFF
    Serial.print("Buzzer state: ");
    Serial.println(buzzerState ? "ON" : "OFF");
}
