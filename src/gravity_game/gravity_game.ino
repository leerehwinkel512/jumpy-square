#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Preferences.h>

Preferences preferences;
int highScore;

#define BUTTON_A 19
#define BUTTON_B 18
#define JOYSTICK_X 39
#define JOYSTICK_Y 36

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SHIP_SIZE 10
#define PLANET_RADIUS 3
#define JOYSTICK_DEADZONE 400
#define THRUST_POWER 0.2
#define GRAVITY_STRENGTH 0.05
#define BUZZER 4

struct GameObject {
  float x;
  float y;
  float vx;
  float vy;
  int size;
};

GameObject ship;
GameObject planet;
unsigned long gameTime = 0;
bool gameOver = false;

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  // init buzzer
  if (!ledcAttachChannel(BUZZER, 2000, 8, 0)) { // pin, freq, resolution, channel
    Serial.println(F("Buzzer pwm init failed"));
    for (;;);  
    }

  // get high score
  preferences.begin("highScore", false);
  highScore = preferences.getInt("highScore", 0);

  showStartupLogo();
  startGame();
}

void showStartupLogo() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Gravity");
  display.println("Game");
  display.setTextSize(1);
  display.println();
  display.print("High Score: ");
  display.print(highScore);
  display.display();
  delay(3000);
}

void startGame() {
  ship = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 4, 0, 0, SHIP_SIZE};
  planet = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 0, 0, PLANET_RADIUS};
  gameTime = 0;
  gameOver = false;
}


void loop() {
  
  updateShipMovement();
  applyGravity();
  checkCollision();
  render();

  delay(100);
}


void updateShipMovement() {
  // Rotate ship based on joystick
  int joystickX = analogRead(JOYSTICK_X) - 2048;
  if (abs(joystickX) > JOYSTICK_DEADZONE) {
    float rotation = map(joystickX, -2048, 2047, -PI/30, PI/30);
    float cosR = cos(rotation);
    float sinR = sin(rotation);
    float newVx = ship.vx * cosR - ship.vy * sinR;
    float newVy = ship.vx * sinR + ship.vy * cosR;
    ship.vx = newVx;
    ship.vy = newVy;
  }

  // Apply thrust
  if (!digitalRead(BUTTON_A)) {
    ship.vx += THRUST_POWER * (ship.vx / sqrt(ship.vx*ship.vx + ship.vy*ship.vy));
    ship.vy += THRUST_POWER * (ship.vy / sqrt(ship.vx*ship.vx + ship.vy*ship.vy));
  }

  // Update position
  ship.x += ship.vx;
  ship.y += ship.vy;

  // Wrap around screen
  if (ship.x < 0) ship.x = SCREEN_WIDTH;
  if (ship.x > SCREEN_WIDTH) ship.x = 0;
  if (ship.y < 0) ship.y = SCREEN_HEIGHT;
  if (ship.y > SCREEN_HEIGHT) ship.y = 0;
}

void applyGravity() {
  float dx = planet.x - ship.x;
  float dy = planet.y - ship.y;
  float distSq = dx*dx + dy*dy;
  float dist = sqrt(distSq);
  float force = GRAVITY_STRENGTH / distSq;
  ship.vx += force * dx / dist;
  ship.vy += force * dy / dist;
}

void checkCollision() {
  float dx = planet.x - ship.x;
  float dy = planet.y - ship.y;
  float distance = sqrt(dx*dx + dy*dy);
  if (distance < PLANET_RADIUS + SHIP_SIZE/2) {
    gameOver = true;
  }
}

void render() {
  display.clearDisplay();

  // Draw planet
  display.fillCircle(planet.x, planet.y, PLANET_RADIUS, WHITE);

  // Draw ship (triangle)
  float angle = atan2(ship.vy, ship.vx);
  int x1 = ship.x + SHIP_SIZE * cos(angle);
  int y1 = ship.y + SHIP_SIZE * sin(angle);
  int x2 = ship.x + SHIP_SIZE/2 * cos(angle + 2.5);
  int y2 = ship.y + SHIP_SIZE/2 * sin(angle + 2.5);
  int x3 = ship.x + SHIP_SIZE/2 * cos(angle - 2.5);
  int y3 = ship.y + SHIP_SIZE/2 * sin(angle - 2.5);
  display.fillTriangle(x1, y1, x2, y2, x3, y3, WHITE);

  // Display game time
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Time: ");
  display.print(gameTime / 50);  // Assuming 50 frames per second

  display.display();
}

void showGameOver() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("GAME OVER");
  display.setTextSize(1);
  display.print("Time: ");
  display.println(gameTime / 50);
  
  if (gameTime / 50 > highScore) {
    highScore = gameTime / 50;
    preferences.putInt("highScore", highScore);
    display.println("New High Score!");
  }
  
  display.println("Press A to restart");
  display.display();

  if (!digitalRead(BUTTON_A)) {
    delay(200);  // Debounce
    startGame();
  }
}

void playSound(int frequency, int duration) {
  ledcWriteTone(0, frequency);
  delay(duration);
  ledcWriteTone(0, 0);
}