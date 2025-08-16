/*********
  Motion Simon Says by Ariel Lai
*********/

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_SSD1306.h>

// I2C Configuration
#define I2C_SDA 21  // ESP32 default
#define I2C_SCL 22  // ESP32 default
#define I2C_SPEED 100000  // 100kHz for stability

// OLED Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C

// MPU6050 Configuration
#define MPU_ADDRESS 0x68

// Button Configuration
#define BUTTON_PIN 15  // Change this to your button pin

// Game Configuration
#define MAX_LEVEL 10
#define MOVE_DELAY 800  // Time between sequence moves (ms)
#define INPUT_TIMEOUT 3000  // Time to wait for player input (ms)

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_MPU6050 mpu;

// constants
const int PITCH_THRESHOLD = 25;
const int ROLL_THRESHOLD = 15;

// Game variables
int sequence[MAX_LEVEL];
int currentLevel = 1;
bool gameStarted = false;
bool gameOver = false;
unsigned long lastInputTime = 0;

void setup() {
  Serial.begin(115200);
  delay(2000);  // Wait for serial instead of while(!Serial)

  // Initialize button
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize I2C with retries
  bool i2c_initialized = initializeI2C();
  if (!i2c_initialized) {
    Serial.println("I2C initialization failed!");
    while (1) {
      delay(100);
    }
  }

  // Initialize MPU6050 with retries
  if (!initializeMPU()) {
    Serial.println("MPU6050 initialization failed!");
    while (1) {
      delay(100);
    }
  }

  // Initialize OLED with retries (non-critical)
  if (!initializeOLED()) {
    Serial.println("OLED initialization failed - continuing without display");
  }

  // Configure MPU6050
  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // Initial display
  showStartScreen();
}

bool initializeI2C() {
  for (int i = 0; i < 5; i++) {
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(I2C_SPEED);
    
    // Test communication with MPU6050
    Wire.beginTransmission(MPU_ADDRESS);
    if (Wire.endTransmission() == 0) {
      return true;
    }
    delay(100);
  }
  return false;
}

bool initializeMPU() {
  for (int i = 0; i < 5; i++) {
    if (mpu.begin(MPU_ADDRESS)) {
      return true;
    }
    delay(100);
  }
  return false;
}

bool initializeOLED() {
  for (int i = 0; i < 5; i++) {
    if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
      return true;
    }
    delay(100);
  }
  return false;
}

void loop() {
  static uint32_t lastRecovery = 0;
  
  // Automatic I2C recovery every 60 seconds
  if(millis() - lastRecovery > 60000) {
    Wire.end();
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(I2C_SPEED);
    lastRecovery = millis();
  }

  // Check if button is pressed to start the game
  if (!gameStarted && digitalRead(BUTTON_PIN) == LOW) {
    delay(200); // Debounce
    startGame();
  }

  // Game logic
  if (gameStarted && !gameOver) {
    checkPlayerInput();
  }

  // Reset game if over and button pressed
  if (gameOver && digitalRead(BUTTON_PIN) == LOW) {
    delay(200); // Debounce
    resetGame();
  }

  // Add small delay to prevent I2C congestion
  delay(50); 
}

void showStartScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Simon Says Game");
  display.println("");
  display.println("Press the button");
  display.println("to start!");
  display.display();
}

void showGameOverScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Game Over!");
  display.print("Level: ");
  display.println(currentLevel - 1);
  display.println("");
  display.println("Press button");
  display.println("to play again");
  display.display();
}

void startGame() {
  gameStarted = true;
  gameOver = false;
  currentLevel = 1;
  randomSeed(analogRead(0)); // Seed random number generator
  
  // Generate the first sequence
  for (int i = 0; i < MAX_LEVEL; i++) {
    sequence[i] = random(4); // 0=Up, 1=Down, 2=Left, 3=Right
  }
  
  playSequence();
}

void resetGame() {
  gameOver = false;
  showStartScreen();
  gameStarted = false;
}

void playSequence() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Level ");
  display.println(currentLevel);
  display.display();
  delay(1000);
  
  for (int i = 0; i < currentLevel; i++) {
    showArrow(sequence[i]);
    delay(MOVE_DELAY);
    display.clearDisplay();
    display.display();
    delay(300);
  }
  
  lastInputTime = millis();
}

void showArrow(int direction) {
  switch(direction) {
    case 0: // Up
      UpArrow();
      break;
    case 1: // Down
      DownArrow();
      break;
    case 2: // Left
      LeftArrow();
      break;
    case 3: // Right
      RightArrow();
      break;
  }
}

void checkPlayerInput() {
  static int currentMove = 0;
  
  // Check for timeout
  if (millis() - lastInputTime > INPUT_TIMEOUT) {
    gameOver = true;
    showGameOverScreen();
    return;
  }
  
  // Check player input
  if (isTiltUp()) {
    handlePlayerMove(0, currentMove);
  } 
  else if (isTiltDown()) {
    handlePlayerMove(1, currentMove);
  } 
  else if (isTiltLeft()) {
    handlePlayerMove(2, currentMove);
  } 
  else if (isTiltRight()) {
    handlePlayerMove(3, currentMove);
  }
}

void handlePlayerMove(int move, int &currentMove) {
  UpArrow(); // Show feedback
  delay(300);
  display.clearDisplay();
  display.display();
  
  if (move == sequence[currentMove]) {
    currentMove++;
    lastInputTime = millis();
    
    if (currentMove >= currentLevel) {
      // Completed the level
      currentMove = 0;
      currentLevel++;
      
      if (currentLevel > MAX_LEVEL) {
        // Game won
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 0);
        display.println("You Win!");
        display.println("Perfect Score!");
        display.display();
        delay(3000);
        gameOver = true;
        showGameOverScreen();
      } else {
        // Next level
        playSequence();
      }
    }
  } else {
    // Wrong move
    gameOver = true;
    showGameOverScreen();
  }
}

// Arrow display functions
void UpArrow() {
  display.clearDisplay();
  display.fillRect(60, 32, 8, 16, WHITE);
  display.fillTriangle(64, 16, 54, 32, 74, 32, WHITE);
  display.display();
}

void DownArrow() {
  display.clearDisplay();
  display.fillRect(60, 16, 8, 16, WHITE);
  display.fillTriangle(64, 48, 54, 32, 74, 32, WHITE);
  display.display();
}

void LeftArrow() {
  display.clearDisplay();
  display.fillRect(64, 28, 16, 8, WHITE);
  display.fillTriangle(48, 32, 64, 22, 64, 42, WHITE);
  display.display();
}

void RightArrow() {
  display.clearDisplay();
  display.fillRect(48, 28, 16, 8, WHITE);
  display.fillTriangle(80, 32, 64, 22, 64, 42, WHITE);
  display.display();
}

// Motion detection functions
bool isTiltUp() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float ax = a.acceleration.x;
  float ay = a.acceleration.y;
  float az = a.acceleration.z;
  float pitch = atan2(-ax, sqrt(ay*ay + az*az))*180/PI;
  return pitch < -PITCH_THRESHOLD;
}

bool isTiltDown() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float ax = a.acceleration.x;
  float ay = a.acceleration.y;
  float az = a.acceleration.z;
  float pitch = atan2(-ax, sqrt(ay*ay + az*az))*180/PI;
  return pitch > PITCH_THRESHOLD;
}

bool isTiltLeft() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float ax = a.acceleration.x;
  float ay = a.acceleration.y;
  float az = a.acceleration.z;
  float roll = atan2(ay, az)*180/PI;
  return roll < -ROLL_THRESHOLD;
}

bool isTiltRight() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float ax = a.acceleration.x;
  float ay = a.acceleration.y;
  float az = a.acceleration.z;
  float roll = atan2(ay, az)*180/PI;
  return roll > ROLL_THRESHOLD;
}