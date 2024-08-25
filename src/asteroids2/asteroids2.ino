#include <SPI.h>
#include <TFT_eSPI.h>
#include <Arduino.h>
#include <Preferences.h>

Preferences preferences;
int highScore;

#define BUTTON_THRUST 32
#define BUTTON_FIRE 33
#define JOYSTICK_X 39
#define JOYSTICK_Y 36

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320

// Initialize TFT display
TFT_eSPI tft = TFT_eSPI();

#define JOYSTICK_DEADZONE 2000
#define ROTATION_SPEED 0.12f

#define SHIP_SIZE 12

#define MAX_SHIP_SPEED 4.0f
#define SHIP_THRUST 0.2f

#define BULLET_SIZE 1
#define MAX_BULLET_DISTANCE 300
#define MAX_BULLETS 3
#define BULLET_SPEED 2.0f

#define BIG_ASTEROID_SIZE 16
#define SMALL_ASTEROID_SIZE 8
#define MAX_ASTEROIDS 10
#define INITIAL_ASTEROIDS 4
#define ASTEROID_SPEED 3.0f

#define BUZZER 25
#define SOUND_DURATION_MS 10

#define SAFE_ZONE_SIZE 150 // Size of the safe zone around the ship

struct Ship {
  float x, y;
  float renderx, rendery;
  float dx, dy;
  float angle;
};

struct Bullet {
  float x, y;
  float renderx, rendery;  
  float dx, dy;
  int distance;
  bool active;
};

Bullet bullets[MAX_BULLETS];

struct Asteroid {
  float x, y;
  float renderx, rendery;  
  float dx, dy;
  int size;
  bool active;
};

Ship ship;
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

  // Init TFT
  tft.init();
  tft.setRotation(1); // Landscape mode
  tft.fillScreen(TFT_BLACK);

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
    
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(10, 10);
    tft.println("High Score Reset!");
    delay(1000);
    tft.fillScreen(TFT_BLACK);
  }    

  highScore = preferences.getInt("highScore", 0);

  showStartupLogo();
  startGame();
}

void showStartupLogo() {
  tft.setTextSize(3);
  tft.setTextColor(TFT_PINK);
  tft.setCursor(10, 20);
  tft.println("Asteroids");
  tft.setTextSize(2);
  tft.setCursor(10, 80);
  tft.print("High Score: ");
  tft.print(highScore);
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
  tft.fillScreen(TFT_BLACK);

  ship = {SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, 0, 0, 0};

  for (int i = 0; i < MAX_BULLETS; i++) {
    bullets[i] = {0, 0, 0, 0, 0, 0, 0, false};
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
      x, y,
      cos(angle), sin(angle),
      BIG_ASTEROID_SIZE, true
    };
  }
}


unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 60;

void loop() {
  unsigned long currentTime = millis();

  handleInput();
  updateGame();
  checkCollisions();  

  // periodic screen update
  if (currentTime - lastUpdate >= UPDATE_INTERVAL) {
    clearGameObjects();  
    render();
    lastUpdate = currentTime;
  }

  playSound();

  delay(30);  // Adjust for desired frame rate
}

void handleInput() {
  int joystickX = analogRead(JOYSTICK_X) - 2048;

  if (abs(joystickX) > JOYSTICK_DEADZONE) {
    ship.angle += (float)joystickX / 2048.0f * ROTATION_SPEED;
    if (ship.angle < 0) ship.angle += 2 * PI;
    if (ship.angle >= 2 * PI) ship.angle -= 2 * PI;
  }

  if (!digitalRead(BUTTON_THRUST)) {
    // Calculate thrust vector
    float thrustX = cos(ship.angle) * SHIP_THRUST;
    float thrustY = sin(ship.angle) * SHIP_THRUST;
    
    // Apply thrust
    ship.dx += thrustX;
    ship.dy += thrustY;
    
    // Calculate current speed
    float speed = sqrt(ship.dx * ship.dx + ship.dy * ship.dy);
    
    // Limit to max speed
    if (speed > MAX_SHIP_SPEED) {
        float scaleFactor = MAX_SHIP_SPEED / speed;
        ship.dx *= scaleFactor;
        ship.dy *= scaleFactor;
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

      float movementX = bullets[i].dx * BULLET_SPEED;
      float movementY = bullets[i].dy * BULLET_SPEED;
      bullets[i].x += movementX;
      bullets[i].y += movementY;
      bullets[i].distance += sqrt(movementX * movementX + movementY * movementY);

      wrapAround(bullets[i].x, bullets[i].y);

      if (bullets[i].distance >= MAX_BULLET_DISTANCE) {
        bullets[i].active = false;
      }
    }
  }

  // Update asteroids
  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    if (asteroids[i].active) {
      float movementX = asteroids[i].dx * ASTEROID_SPEED;
      float movementY = asteroids[i].dy * ASTEROID_SPEED;
      asteroids[i].x += movementX;
      asteroids[i].y += movementY;
      
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

      int clearSize;
      int clearX;
      int clearY;

      clearSize = (asteroids[index].size * 2) + 4;  
      clearX = asteroids[index].renderx - (clearSize / 2);
      clearY = asteroids[index].rendery - (clearSize / 2);
      tft.fillRect(clearX, clearY, clearSize, clearSize, TFT_BLACK);

      float angle = random(360) * PI / 180.0f;
      asteroids[newIndex] = {
        asteroids[index].x, asteroids[index].y,
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

  // Draw ship
  ship.renderx = ship.x;
  ship.rendery = ship.y;
  int x1 = ship.x + SHIP_SIZE * cos(ship.angle);
  int y1 = ship.y + SHIP_SIZE * sin(ship.angle);
  int x2 = ship.x + SHIP_SIZE/2 * cos(ship.angle + 2.5);
  int y2 = ship.y + SHIP_SIZE/2 * sin(ship.angle + 2.5);
  int x3 = ship.x + SHIP_SIZE/2 * cos(ship.angle - 2.5);
  int y3 = ship.y + SHIP_SIZE/2 * sin(ship.angle - 2.5);
  tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_CYAN);

  // Draw bullets
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i].active) {
      bullets[i].renderx = bullets[i].x;
      bullets[i].rendery = bullets[i].y;
      tft.fillCircle(bullets[i].x, bullets[i].y, BULLET_SIZE, TFT_CYAN);
    }
  }

  // Draw asteroids
  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    if (asteroids[i].active) {
      asteroids[i].renderx = asteroids[i].x;
      asteroids[i].rendery = asteroids[i].y;      
      tft.fillCircle(asteroids[i].x, asteroids[i].y, asteroids[i].size, TFT_YELLOW);
    }
  }

  // Display score
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.print("Score: ");
  tft.print(score);
}


void clearGameObjects() {

  int clearSize;
  int clearX;
  int clearY;

  // Clear ship
  clearSize = (int)(SHIP_SIZE * 2.5);
  clearX = (int)(ship.renderx - clearSize / 2);
  clearY = (int)(ship.rendery - clearSize / 2);
  tft.fillRect(clearX, clearY, clearSize, clearSize, TFT_BLACK);

  // Clear bullets
  for (int i = 0; i < MAX_BULLETS; i++) {
    clearSize = (BULLET_SIZE * 2) + 2;
    clearX = bullets[i].renderx - (clearSize / 2);
    clearY = bullets[i].rendery - (clearSize / 2);
    tft.fillRect(clearX, clearY, clearSize, clearSize, TFT_BLACK);
  }

  // Clear asteroids
  for (int i = 0; i < MAX_ASTEROIDS; i++) {
    clearSize = (asteroids[i].size * 2) + 4;  
    clearX = asteroids[i].renderx - (clearSize / 2);
    clearY = asteroids[i].rendery - (clearSize / 2);
    tft.fillRect(clearX, clearY, clearSize, clearSize, TFT_BLACK);      
  }

  // Display score
  clearX = 10;
  clearY = 10;
  int clearWidth = 100;
  int clearHeight = 20;
  tft.fillRect(clearX, clearY, clearWidth, clearHeight, TFT_BLACK);
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
  tft.fillScreen(TFT_BLACK);

  // Display "GAME OVER" text
  tft.setTextSize(3);
  tft.setTextColor(TFT_RED);
  tft.setCursor(50, 40);
  tft.println("GAME OVER");

  // Display the score
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(50, 100);
  tft.print("Score: ");
  tft.println(score);

  // If you want to display the high score as well
  tft.setCursor(50, 130);
  tft.print("High Score: ");
  tft.println(highScore);

  if (score > highScore) {
    highScore = score;
    preferences.putInt("highScore", highScore);

    // Set text properties for the new high score message
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(50, 160);

    // Display the new high score message
    tft.println("New High Score!");
  }

  ledcWriteTone(BUZZER, 250);
  delay(500);
  ledcWriteTone(BUZZER, 0);
  delay(2000);

  startGame();
}