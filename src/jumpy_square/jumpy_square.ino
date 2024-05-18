// code: https://github.com/leerehwinkel512/jumpy-square

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <vector>

#define SCREEN_WIDTH 128 // OLED display width
#define SCREEN_HEIGHT 64 // OLED display height
#define OLED_RESET    -1 // No reset pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BUTTON_PIN 0       // Change this to your button's pin
#define GRAVITY 2          // Gravity effect on the player
#define JUMP_FORCE -12     // Jump force of the player
#define PLAYER_SIZE 5      // Size of the player square
#define OBSTACLE_SIZE 10   // Size of the square obstacle
#define OBSTACLE_SPEED 1   // Speed of obstacles
#define MIN_OBS_DELAY 250  // Min time delay between obstacles
#define MAX_OBS_DELAY 2000 // Max time delay between obstacles
#define PLAYER_GROUND (SCREEN_HEIGHT - PLAYER_SIZE) // Ground level for the player

// position of obstacles
struct Obstacle {
  int x, y;
};

std::vector<Obstacle> obstacles; // holds vector of obstacles on screen

unsigned long lastObstacleTime = 0; // time since last obstacle added
unsigned long nextObstacleTime = 0; // time when the next obstacle should be added

// player position and speed
int playerY = PLAYER_GROUND;
int velocityY = 0;

unsigned long points = 0; // point counter


void setup() {
  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);  // init button

  // init oled
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // loop forever if fail to init oled
  }

  showStartupLogo();
}

void loop() {
  handleJump();
  updatePlayer();
  updateObstacles();
  checkCollisions();
  updatePoints();
  render();
}

void showStartupLogo() {
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Jumpy");
  display.print("Square");
  
  display.display();
  delay(3000);
}

void handleJump() {
  // if button pressed, and player on the ground... then set velocity to jump force
  if (digitalRead(BUTTON_PIN) == LOW) {
    if (playerY == PLAYER_GROUND) {
      velocityY = JUMP_FORCE; // Apply jump force
    }
  }
}

void updatePlayer() {
  // move player in direction of velocity, then update velocity for gravity
  playerY += velocityY;
  velocityY += GRAVITY;

  // Ground collision, set velocity to 0
  if (playerY > PLAYER_GROUND) {
    playerY = PLAYER_GROUND;
    velocityY = 0;
  }
}

void updatePoints() {
  points++; // Increment points over time
}

void updateObstacles() {
  unsigned long currentTime = millis();

  if (currentTime >= nextObstacleTime) {
      // Place the top of the obstacle at a position where its bottom aligns with the bottom of the screen
      obstacles.push_back({SCREEN_WIDTH, SCREEN_HEIGHT - OBSTACLE_SIZE});

      // random delay for next obstacle
      nextObstacleTime = currentTime + random(MIN_OBS_DELAY, MAX_OBS_DELAY);
  }

  // loop over all obstacles in the vector and update their position
  for (int i = 0; i < obstacles.size(); i++) {

    obstacles[i].x -= OBSTACLE_SPEED; // Move obstacle to the left

    // Remove obstacle if it goes off screen
    if (obstacles[i].x < -OBSTACLE_SIZE) {
      obstacles.erase(obstacles.begin() + i);
      i--;
    }
  }
}

void checkCollisions() {

  // check each obstacle for a collision with player
  for (Obstacle obs : obstacles) {

    // check collision
    if (playerY < obs.y + OBSTACLE_SIZE && playerY + PLAYER_SIZE > obs.y &&
        SCREEN_WIDTH/2 < obs.x + OBSTACLE_SIZE && SCREEN_WIDTH/2 + PLAYER_SIZE > obs.x) {

      // Collision detected, end game
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.println("GAME OVER");
      display.display();

      // delay then restart game
      delay(3000);
      restartGame();
    }
  }
}

void restartGame() {
  obstacles.clear();           // Clear all obstacles
  playerY = PLAYER_GROUND;     // Reset player position
  velocityY = 0;               // Reset player velocity
  points = 0;                  // Reset points
  lastObstacleTime = millis(); // Reset the obstacle timer
  nextObstacleTime = millis(); // Reset the obstacle timer
}

void render() {
  display.clearDisplay();

  // draw player
  display.fillRect(SCREEN_WIDTH/2 - PLAYER_SIZE/2, playerY, PLAYER_SIZE, PLAYER_SIZE, WHITE);

  // draw each obstacle
  for (Obstacle obs : obstacles) {
      display.fillRect(obs.x - OBSTACLE_SIZE/2, obs.y, OBSTACLE_SIZE, OBSTACLE_SIZE, WHITE);
  }

  // Display points
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(SCREEN_WIDTH - 30, 0); // Position the text at the top-right corner
  display.print(points);  

  display.display();
}
