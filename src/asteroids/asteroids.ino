#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Preferences.h>

Preferences preferences;
int highScore;

#define BUTTON_THRUST 19
#define BUTTON_FIRE 18
#define JOYSTICK_X 39
#define JOYSTICK_Y 36

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define JOYSTICK_DEADZONE 2000
#define ROTATION_SPEED 0.20f

#define SHIP_SIZE 4
#define MAX_SHIP_SPEED 3.0f

#define BULLET_SIZE 1
#define MAX_BULLET_DISTANCE 100
#define MAX_BULLETS 3

#define BIG_ASTEROID_SIZE 6
#define SMALL_ASTEROID_SIZE 3
#define MAX_ASTEROIDS 20
#define INITIAL_ASTEROIDS 1

#define BUZZER 4
#define SOUND_DURATION_MS 10

#define SAFE_ZONE_SIZE 30 // Size of the safe zone around the ship

struct Ship {
  float x, y;
  float dx, dy;
  float angle;
};

struct Bullet {
  float x, y;
  float dx, dy;
  int distance;
  bool active;
};

Bullet bullets[MAX_BULLETS];

struct Asteroid {
  float x, y;
  float dx, dy;
  int size;
  bool active;
};

Ship ship;
Bullet bullet;
Asteroid asteroids[MAX_ASTEROIDS];

bool fireButtonPressed = false;

enum SoundType {
  FIRE,
  EXPLODE,
  THRUST,
  NONE
};

struct Sound {
  SoundType type;
  bool newSound;
  unsigned long soundEnd;
};

Sound gameSound;

unsigned long score = 0;
int level = 1;

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_THRUST, INPUT_PULLUP);
  pinMode(BUTTON_FIRE, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  // init buzzer
  if (!ledcAttachChannel(BUZZER, 2000, 8, 0)) { // pin, freq, resolution, channel
    Serial.println(F("Buzzer pwm init failed"));
    for (;;);  
    }

  // high score in asteriods name space
  preferences.begin("asteroids", false);    

  // trigger high score reset
  if (!digitalRead(BUTTON_THRUST) && !digitalRead(BUTTON_FIRE)) {

    // Save the reset high score
    preferences.putInt("highScore", 0);
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("High Score Reset!");
    display.display();
    
    delay(1000);
  }    

  highScore = preferences.getInt("highScore", 0);

  showStartupLogo();
  startGame();
}

void showStartupLogo() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Asteroids");
  display.setTextSize(1);
  display.println();
  display.print("High Score: ");
  display.print(highScore);
  display.display();
  delay(3000);
}

void startGame() {
  gameSound = {NONE, false, 0};
  score = 0;
  level = 1;
  resetGameObjects();
}

bool isInSafeZone(float x, float y) {
  float shipCenterX = SCREEN_WIDTH / 2.0f;
  float shipCenterY = SCREEN_HEIGHT / 2.0f;
  return (abs(x - shipCenterX) < SAFE_ZONE_SIZE && abs(y - shipCenterY) < SAFE_ZONE_SIZE);
}

void resetGameObjects() {
  ship = {SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, 0, 0, 0};
  bullet = {0, 0, 0, 0, 0, false};

  for (int i = 0; i < MAX_BULLETS; i++) {
    bullets[i] = {0, 0, 0, 0, 0, false};
  }  

  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    asteroids[i].active = false;
  }

  for (int i = 0; i < INITIAL_ASTEROIDS + level - 1; i++) {
    float x, y;
    do {
      x = random(SCREEN_WIDTH);
      y = random(SCREEN_HEIGHT);
    } while (isInSafeZone(x, y));

    float angle = random(360) * PI / 180.0f;
    asteroids[i] = {
      x, y,
      cos(angle), sin(angle),
      BIG_ASTEROID_SIZE, true
    };
  }
}

void loop() {
  handleInput();
  updateGame();
  checkCollisions();
  render();
  playSound();

  delay(20);  // Adjust for desired frame rate
}

void handleInput() {
  int joystickX = analogRead(JOYSTICK_X) - 2048;

  if (abs(joystickX) > JOYSTICK_DEADZONE) {
    ship.angle += (float)joystickX / 2048.0f * ROTATION_SPEED;
    if (ship.angle < 0) ship.angle += 2 * PI;
    if (ship.angle >= 2 * PI) ship.angle -= 2 * PI;
  }

  if (!digitalRead(BUTTON_THRUST)) {
    float thrustX = cos(ship.angle) * 0.2f;
    float thrustY = sin(ship.angle) * 0.2f;
    ship.dx += thrustX;
    ship.dy += thrustY;

    float speed = sqrt(ship.dx * ship.dx + ship.dy * ship.dy);
    if (speed > MAX_SHIP_SPEED) {
      ship.dx = (ship.dx / speed) * MAX_SHIP_SPEED;
      ship.dy = (ship.dy / speed) * MAX_SHIP_SPEED;
    }

    gameSound.newSound = true;
    gameSound.type = THRUST;
  }

  // Handle fire button
  bool currentFireButtonState = !digitalRead(BUTTON_FIRE);
  if (currentFireButtonState && !fireButtonPressed) {
    fireBullet();
  }
  fireButtonPressed = currentFireButtonState;
}

void fireBullet() {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!bullets[i].active) {
      bullets[i].x = ship.x;
      bullets[i].y = ship.y;
      bullets[i].dx = cos(ship.angle) * 4;
      bullets[i].dy = sin(ship.angle) * 4;
      bullets[i].distance = 0;
      bullets[i].active = true;

      gameSound.newSound = true;
      gameSound.type = FIRE;
      return;  // Exit after firing one bullet
    }
  }
}

void updateGame() {
  // Update ship position
  ship.x += ship.dx;
  ship.y += ship.dy;
  wrapAround(ship.x, ship.y);

  // Update bullets
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      bullets[i].x += bullets[i].dx;
      bullets[i].y += bullets[i].dy;
      bullets[i].distance += 4;
      wrapAround(bullets[i].x, bullets[i].y);

      if (bullets[i].distance >= MAX_BULLET_DISTANCE) {
        bullets[i].active = false;
      }
    }
  }

  // Update asteroids
  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    if (asteroids[i].active) {
      asteroids[i].x += asteroids[i].dx;
      asteroids[i].y += asteroids[i].dy;
      wrapAround(asteroids[i].x, asteroids[i].y);
    }
  }

  // Check if all asteroids are destroyed
  bool allDestroyed = true;
  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    if (asteroids[i].active) {
      allDestroyed = false;
      break;
    }
  }

  if (allDestroyed) {
    level++;
    resetGameObjects();
  }
}

void wrapAround(float &x, float &y) {
  if (x < 0) x = SCREEN_WIDTH;
  if (x > SCREEN_WIDTH) x = 0;
  if (y < 0) y = SCREEN_HEIGHT;
  if (y > SCREEN_HEIGHT) y = 0;
}

void checkCollisions() {
  // Check bullet collisions with asteroids
  for (int b = 0; b < MAX_BULLETS; b++) {
    if (bullets[b].active) {
      for (int a = 0; a < MAX_ASTEROIDS; a++) {
        if (asteroids[a].active) {
          if (distance(bullets[b].x, bullets[b].y, asteroids[a].x, asteroids[a].y) < asteroids[a].size) {
            bullets[b].active = false;
            score++;

            if (asteroids[a].size == BIG_ASTEROID_SIZE) {
              splitAsteroid(a);
            } else {
              asteroids[a].active = false;
            }

            gameSound.newSound = true;
            gameSound.type = EXPLODE;
            break;
          }
        }
      }
    }
  }

  // Check ship collision with asteroids
  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    if (asteroids[i].active) {
      if (distance(ship.x, ship.y, asteroids[i].x, asteroids[i].y) < asteroids[i].size + SHIP_SIZE / 2) {
        gameOver();
        return;
      }
    }
  }
}

float distance(float x1, float y1, float x2, float y2) {
  return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

void splitAsteroid(int index) {
  asteroids[index].active = false;
  
  for (int i = 0; i < 2; i++) {
    int newIndex = findInactiveAsteroid();
    if (newIndex != -1) {
      float angle = random(360) * PI / 180.0f;
      asteroids[newIndex] = {
        asteroids[index].x, asteroids[index].y,
        cos(angle), sin(angle),
        SMALL_ASTEROID_SIZE, true
      };
    }
  }
}

int findInactiveAsteroid() {
  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    if (!asteroids[i].active) {
      return i;
    }
  }
  return -1;
}

void render() {
  display.clearDisplay();

  // Draw ship
  int x1 = ship.x + SHIP_SIZE * cos(ship.angle);
  int y1 = ship.y + SHIP_SIZE * sin(ship.angle);
  int x2 = ship.x + SHIP_SIZE/2 * cos(ship.angle + 2.5);
  int y2 = ship.y + SHIP_SIZE/2 * sin(ship.angle + 2.5);
  int x3 = ship.x + SHIP_SIZE/2 * cos(ship.angle - 2.5);
  int y3 = ship.y + SHIP_SIZE/2 * sin(ship.angle - 2.5);
  display.fillTriangle(x1, y1, x2, y2, x3, y3, WHITE);

  // Draw bullets
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      display.fillCircle(bullets[i].x, bullets[i].y, BULLET_SIZE, WHITE);
    }
  }

  // Draw asteroids
  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    if (asteroids[i].active) {
      display.drawCircle(asteroids[i].x, asteroids[i].y, asteroids[i].size, WHITE);
    }
  }

  // Display score
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Score: ");
  display.print(score);

  display.display();
}

void playSound() {
  if (gameSound.newSound) {
    switch (gameSound.type) {
      case FIRE:
        ledcWriteTone(BUZZER, 500);
        break;
      case EXPLODE:
        ledcWriteTone(BUZZER, 200);
        break;
      case THRUST:
        ledcWriteTone(BUZZER, 100);
        break;
      default:
        ledcWriteTone(BUZZER, 0);
        break;
    }

    gameSound.newSound = false;
    gameSound.soundEnd = millis() + SOUND_DURATION_MS;
  }

  if (millis() > gameSound.soundEnd) {
    ledcWriteTone(BUZZER, 0);
  }
}

void gameOver() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  display.println("GAME OVER");
  display.setTextSize(1);
  display.setCursor(10, 40);
  display.print("Score: ");
  display.println(score);

  if (score > highScore) {
    highScore = score;
    preferences.putInt("highScore", highScore);
    display.setCursor(10, 50);
    display.println("New High Score!");
  }

  display.display();

  ledcWriteTone(BUZZER, 250);
  delay(500);
  ledcWriteTone(BUZZER, 0);
  delay(2000);

  startGame();
}