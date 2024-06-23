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

#define JOYSTICK_DEADZONE 2000
#define ROTATION_SPEED 0.05f

#define SHIP_SIZE 8

#define MIN_BULLET_SPEED 0
#define MAX_BULLET_SPEED 20
#define MAX_BULLET_DISTANCE 100
#define BULLET_SIZE 1

#define PLANET_SIZE 5
#define GRAVITATIONAL_CONSTANT 2.0f
#define MAX_PLANETS 3
#define SIDE_BUFFER 10
#define LEFT_SIDE_BUFFER 35
#define MIN_PLANET_DISTANCE 20

#define BUZZER 4
#define SOUND_DURATION_MS 10

struct ship {
  float positionx;
  float positiony;
  float direction;
  int size;
  bool fireBlaster;
  int bulletSpeed;
};

struct bullet {
  float positionx;
  float positiony;
  float old_positionx;  // to check collision tunneling
  float old_positiony;  // to check collision tunneling
  float direction;
  float velocityX;
  float velocityY;
  int distanceTraveled;
  bool active;
};

struct planet {
  float positionx;
  float positiony;
  int size;
  float mass;
  bool isTarget;
};

ship playerShip;
bullet playerBullet;
planet planets[MAX_PLANETS];

enum SoundType {
  FIRE,
  SCORE,
  CRASH,
  NONE
};

struct sound {
  SoundType sound;
  bool newSound;
  unsigned long soundEnd;
};

sound gameSound;

unsigned long points = 0; // Point counter

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

  // store high score
  preferences.begin("highScore", false);

  // trigger high score reset
  if (!digitalRead(BUTTON_A) && !digitalRead(BUTTON_B)) {

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

  // get high score
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

  // Init sound
  gameSound = {
    NONE,
    false,
    0
  };

  points = 0;

  resetGameObjects();

}

bool isPlanetOverlapping(float x, float y, int index) {
  for (int i = 0; i < index; i++) {
    float dx = x - planets[i].positionx;
    float dy = y - planets[i].positiony;
    float distanceSquared = dx * dx + dy * dy;
    if (distanceSquared < MIN_PLANET_DISTANCE * MIN_PLANET_DISTANCE) {
      return true;
    }
  }
  return false;
}

void resetGameObjects() {

  // Init ship
  playerShip = {
    7,                   // positionx
    SCREEN_HEIGHT / 2.0f, // positiony
    0.0f,                 // direction (in radians)
    SHIP_SIZE,            // size
    false,                // blaster
    10                    // bullet speed
  };

  // Init planets
  for (int i = 0; i < MAX_PLANETS; i++) {
    float x, y;
    do {
      x = random(LEFT_SIDE_BUFFER, SCREEN_WIDTH - SIDE_BUFFER);
      y = random(SIDE_BUFFER, SCREEN_HEIGHT - SIDE_BUFFER);
    } while (isPlanetOverlapping(x, y, i));

    planets[i] = {
      x,                                            // x
      y,                                            // y
      PLANET_SIZE,                                  // size
      PI * (float)PLANET_SIZE * (float)PLANET_SIZE, // mass
      false                                         // isTarget
    };
  }

  // Select the planet with the highest x-coordinate as the target
  int rightmostPlanetIndex = 0;
  float maxX = planets[0].positionx;

  for (int i = 1; i < MAX_PLANETS; i++) {
      if (planets[i].positionx > maxX) {
          maxX = planets[i].positionx;
          rightmostPlanetIndex = i;
      }
  }

  planets[rightmostPlanetIndex].isTarget = true;

}


void loop() {
  handleController();
  applyGravity();
  updateBullet();
  checkCollision();
  render();
  playSound();

  delay(20);  // Adjust for desired frame rate
}

void handleController() {
  int joystickX = analogRead(JOYSTICK_Y) - 2048;
  int joystickY = analogRead(JOYSTICK_X) - 2048;

  // Rotate ship
  if (abs(joystickX) > JOYSTICK_DEADZONE) {
    float rotationAmount = (float)joystickX / 2048.0f * ROTATION_SPEED;
    playerShip.direction += rotationAmount;
    
    // Keep direction between 0 and 2π
    if (playerShip.direction < 0) playerShip.direction += 2 * PI;
    if (playerShip.direction >= 2 * PI) playerShip.direction -= 2 * PI;
  }

  // Set bullet speed
  if (abs(joystickY) > JOYSTICK_DEADZONE) {
    if (joystickY < 0) {
      playerShip.bulletSpeed += 1;
    }
    else {
      playerShip.bulletSpeed -= 1;
    }

    if (playerShip.bulletSpeed > MAX_BULLET_SPEED) playerShip.bulletSpeed = MAX_BULLET_SPEED;
    if (playerShip.bulletSpeed < MIN_BULLET_SPEED) playerShip.bulletSpeed = MIN_BULLET_SPEED;
  }

  // Handle reset button
  if (!digitalRead(BUTTON_B)) {
    playerBullet.active = false;

    gameSound.newSound = true;
    gameSound.sound = CRASH;    
  }  

  // Handle blaster firing
  if (!digitalRead(BUTTON_A) && !playerBullet.active) {
    playerShip.fireBlaster = true;

    gameSound.newSound = true;
    gameSound.sound = FIRE;
  } else {
    playerShip.fireBlaster = false;
  }

  // Fire bullet if fireBlaster is true and no active bullet
  if (playerShip.fireBlaster && !playerBullet.active) {

    // position
    playerBullet.positionx = playerShip.positionx;
    playerBullet.positiony = playerShip.positiony;

    // direction
    playerBullet.direction = playerShip.direction;

    // velocity
    playerBullet.velocityX = cos(playerBullet.direction) * (float)playerShip.bulletSpeed / 4.0;
    playerBullet.velocityY = sin(playerBullet.direction) * (float)playerShip.bulletSpeed / 4.0;

    playerBullet.distanceTraveled = 0;
    playerBullet.active = true;
  }  
}

void updateBullet() {
  if (playerBullet.active) {

    // Update ship position based on velocity
    playerBullet.old_positionx = playerBullet.positionx;
    playerBullet.old_positiony = playerBullet.positiony;

    playerBullet.positionx += playerBullet.velocityX;
    playerBullet.positiony += playerBullet.velocityY;
    
    playerBullet.distanceTraveled += 1;
    
    // Deactivate bullet if it has traveled the maximum distance
    if (playerBullet.distanceTraveled >= MAX_BULLET_DISTANCE) {
      playerBullet.active = false;

      gameSound.newSound = true;
      gameSound.sound = CRASH;
    }

    // check for reset if bullet way outside play box
    if ( abs(playerBullet.positionx) > 1000 || abs(playerBullet.positiony) > 1000 ) {
      playerBullet.active = false;

      gameSound.newSound = true;
      gameSound.sound = CRASH;
    }
  }
}

void applyGravity() {
  if (playerBullet.active) {
    for (int i = 0; i < MAX_PLANETS; i++) {
      float dx = planets[i].positionx - playerBullet.positionx;
      float dy = planets[i].positiony - playerBullet.positiony;
      float distanceSquared = dx * dx + dy * dy;
      
      // Avoid division by zero and limit maximum gravitational force
      if (distanceSquared < 1) distanceSquared = 1;
      
      float force = (GRAVITATIONAL_CONSTANT * planets[i].mass) / distanceSquared;
      
      // Apply gravitational force to bullet's velocity components
      playerBullet.velocityX += (dx / sqrt(distanceSquared)) * force;
      playerBullet.velocityY += (dy / sqrt(distanceSquared)) * force;
    }
  }
}

void checkCollision() {
  if (playerBullet.active) {

    // Number of discreet steps to check
    const int steps = 10;

    for (int step = 0; step <= steps; step++) {
      // Calculate the position to check
      float checkX = playerBullet.old_positionx + (playerBullet.positionx - playerBullet.old_positionx) * step / steps;
      float checkY = playerBullet.old_positiony + (playerBullet.positiony - playerBullet.old_positiony) * step / steps;

      for (int i = 0; i < MAX_PLANETS; i++) {
        float dx = checkX - planets[i].positionx;
        float dy = checkY - planets[i].positiony;
        float distanceSquared = dx*dx + dy*dy;

        // Check if this point is inside the planet's circle
        if (distanceSquared <= planets[i].size * planets[i].size) {
          playerBullet.active = false;
          if (planets[i].isTarget) {
            points++; // increase points

            gameSound.newSound = true;
            gameSound.sound = SCORE;

            resetGameObjects();
          } else {
            gameOver();
          }
          return;
        }
      }
    }    
  }
}

void render() {
  display.clearDisplay();

  // Display points
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(SCREEN_WIDTH - 60, 0); // Position the text at the top-right corner
  display.print("Score: ");
  display.print(points);   

  // Draw ship (triangle)
  int x1 = playerShip.positionx + playerShip.size * cos(playerShip.direction);
  int y1 = playerShip.positiony + playerShip.size * sin(playerShip.direction);
  int x2 = playerShip.positionx + playerShip.size/2 * cos(playerShip.direction + 2.5);
  int y2 = playerShip.positiony + playerShip.size/2 * sin(playerShip.direction + 2.5);
  int x3 = playerShip.positionx + playerShip.size/2 * cos(playerShip.direction - 2.5);
  int y3 = playerShip.positiony + playerShip.size/2 * sin(playerShip.direction - 2.5);
  display.fillTriangle(x1, y1, x2, y2, x3, y3, WHITE);

  // Draw bullet
  if (playerBullet.active) {
    display.fillCircle(playerBullet.positionx, playerBullet.positiony, BULLET_SIZE, WHITE);
  }

  // Draw planets
  for (int i = 0; i < MAX_PLANETS; i++) {

    //place X on target planet
    if (planets[i].isTarget) {
      display.fillCircle(planets[i].positionx, planets[i].positiony, planets[i].size, WHITE);
      display.setTextSize(1);
      display.setTextColor(BLACK);
      display.setCursor(planets[i].positionx - 3, planets[i].positiony - 3);
      display.print("X");
    } else {
      display.fillCircle(planets[i].positionx, planets[i].positiony, planets[i].size, WHITE);
    }

  }

  // display velocity
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Speed: ");
  display.print(playerShip.bulletSpeed);

  display.display();
}

void playSound() {

  if (gameSound.newSound) {

    switch (gameSound.sound) {
      case FIRE:
        ledcWriteTone(BUZZER, 500); // Mid tone
        break;
      case SCORE:
        ledcWriteTone(BUZZER, 1000); // High tone
        break;
      case CRASH:
        ledcWriteTone(BUZZER, 200); // Low tone
        break;        
      default:
        ledcWriteTone(BUZZER, 0); // No sound
        break;
    }

    gameSound.newSound = false;
    gameSound.soundEnd = millis() + SOUND_DURATION_MS;
  }

  unsigned long currentTime = millis();

  if (currentTime > gameSound.soundEnd) {
    ledcWriteTone(BUZZER, 0);
  }

}

void gameOver() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("GAME OVER");

  if (points > highScore){
    preferences.putInt("highScore", points);
    display.setCursor(0, SCREEN_HEIGHT - 10);
    display.println("New High Score!");
  }

  display.display();  

  // Delay then restart game
  ledcWriteTone(BUZZER, 250);
  delay(500);
  ledcWriteTone(BUZZER, 0);
  delay(1000);  
  startGame();
}