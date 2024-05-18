#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <vector>

#define SCREEN_WIDTH 128 // OLED display width
#define SCREEN_HEIGHT 64 // OLED display height
#define OLED_RESET    -1 // No reset pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BUTTON_PIN 0       // Change this to your button's pin
#define GRAVITY 0.25        // Gravity effect on the player, now a float
#define JUMP_FORCE -1.0    // Jump force of the player, now a float
#define PLAYER_SIZE 5      // Size of the player square
#define OBSTACLE_WIDTH 10  // Width of the obstacle
#define OBSTACLE_GAP 20    // Gap between the top and bottom obstacles
#define OBSTACLE_SPEED 2   // Speed of obstacles
#define MIN_OBS_DELAY 500 // Min time delay between obstacles
#define MAX_OBS_DELAY 2000 // Max time delay between obstacles
#define PLAYER_START_X (SCREEN_WIDTH / 4) // Player's starting x position

// Position of obstacles
struct Obstacle {
  int x, gapY;
};

std::vector<Obstacle> obstacles; // Holds vector of obstacles on screen

unsigned long lastObstacleTime = 0; // Time since last obstacle added
unsigned long nextObstacleTime = 0; // Time when the next obstacle should be added

// Player position and speed
float playerY = 5; // Now a float
float velocityY = 0;               // Now a float

unsigned long points = 0; // Point counter

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Init button
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Loop forever if fail to init OLED
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
  display.println("Flappy");
  display.print("Square");
  display.display();
  delay(3000);
}

void handleJump() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    velocityY = JUMP_FORCE;
  }
}

void updatePlayer() {
  playerY += velocityY;
  velocityY += GRAVITY;
  if (playerY < 0) {
    playerY = 0;
    gameOver();
  }
  if (playerY > SCREEN_HEIGHT - PLAYER_SIZE) {
    playerY = SCREEN_HEIGHT - PLAYER_SIZE;
    gameOver();
  }
}

void updatePoints() {
  points++;
}

void updateObstacles() {
  unsigned long currentTime = millis();
  if (currentTime >= nextObstacleTime) {
    int gapY = random(OBSTACLE_GAP, SCREEN_HEIGHT - OBSTACLE_GAP);
    obstacles.push_back({SCREEN_WIDTH, gapY});
    nextObstacleTime = currentTime + random(MIN_OBS_DELAY, MAX_OBS_DELAY);
  }
  for (int i = 0; i < obstacles.size(); i++) {
    obstacles[i].x -= OBSTACLE_SPEED;
    if (obstacles[i].x < -OBSTACLE_WIDTH) {
      obstacles.erase(obstacles.begin() + i);
      i--;
    }
  }
}

void checkCollisions() {
  for (Obstacle obs : obstacles) {
    if ((playerY < obs.gapY - OBSTACLE_GAP / 2 || playerY + PLAYER_SIZE > obs.gapY + OBSTACLE_GAP / 2) &&
        (PLAYER_START_X + PLAYER_SIZE > obs.x && PLAYER_START_X < obs.x + OBSTACLE_WIDTH)) {
      gameOver();
    }
  }
}

void gameOver() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("GAME OVER");
  display.display();
  delay(3000);
  restartGame();
}

void restartGame() {
  obstacles.clear();
  playerY = 5;
  velocityY = 0;
  points = 0;
  lastObstacleTime = millis();
  nextObstacleTime = millis();
}

void render() {
  display.clearDisplay();
  display.fillRect(PLAYER_START_X, playerY, PLAYER_SIZE, PLAYER_SIZE, WHITE);
  for (Obstacle obs : obstacles) {
    display.fillRect(obs.x, 0, OBSTACLE_WIDTH, obs.gapY - OBSTACLE_GAP / 2, WHITE);
    display.fillRect(obs.x, obs.gapY + OBSTACLE_GAP / 2, OBSTACLE_WIDTH, SCREEN_HEIGHT - (obs.gapY + OBSTACLE_GAP / 2), WHITE);
  }
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(SCREEN_WIDTH - 30, 0);
  display.print(points);
  display.display();
}
