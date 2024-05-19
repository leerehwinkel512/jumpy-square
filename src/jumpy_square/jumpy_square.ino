#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <vector>

#define SCREEN_WIDTH 128 // OLED display width
#define SCREEN_HEIGHT 64 // OLED display height
#define OLED_RESET    -1 // No reset pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BUTTON_PIN 0            // Change this to your button's pin
#define GRAVITY 0.75            // Gravity effect on the player
#define JUMP_FORCE -8           // Jump force of the player
#define PLAYER_SIZE 5           // Size of the player square
#define MIN_PLATFORM_WIDTH 30   // Minimum width of the platform
#define MAX_PLATFORM_WIDTH 50   // Maximum width of the platform
#define PLATFORM_HEIGHT 2       // Height of the platform
#define PLATFORM_GAP 2         // Minimum vertical gap between platforms
#define PLATFORM_SPEED 3        // Speed of platforms
#define MIN_PLATFORM_DELAY 100  // Min time delay between platforms
#define MAX_PLATFORM_DELAY 250 // Max time delay between platforms
#define PLAYER_START_X (SCREEN_WIDTH / 4) // Player's starting x position

// Position of platforms
struct Platform {
  int x, y, width;
};

std::vector<Platform> platforms; // Holds vector of platforms on screen

unsigned long lastPlatformTime = 0; // Time since last platform added
unsigned long nextPlatformTime = 0; // Time when the next platform should be added

// Player position and speed
float playerY;
float velocityY = 0;

unsigned long points = 0; // Point counter

void setup() {
  Serial.begin(9600);

  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Init button

  // Init OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Loop forever if fail to init OLED
  }

  showStartupLogo();
  startGame();
}

void loop() {
  handleJump();
  updatePlayer();
  updatePlatforms();
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
  // If button pressed, apply jump force only if standing on a platform
  if (digitalRead(BUTTON_PIN) == LOW && velocityY == 0) {
    for (Platform plat : platforms) {
      if (playerY + PLAYER_SIZE == plat.y && 
          PLAYER_START_X + PLAYER_SIZE > plat.x && PLAYER_START_X < plat.x + plat.width) {
        velocityY = JUMP_FORCE;
        break;
      }
    }
  }
}

void updatePlayer() {
  // Move player in direction of velocity, then update velocity for gravity
  playerY += velocityY;
  velocityY += GRAVITY;

  // Bottom collision, end game
  if (playerY > SCREEN_HEIGHT - PLAYER_SIZE) {
    gameOver();
  }
}

void updatePoints() {
  points++; // Increment points over time
}

void updatePlatforms() {
  unsigned long currentTime = millis();

  if (currentTime >= nextPlatformTime) {
    // Add a new platform with a random y position not higher than the middle of the screen
    int platformY = random(SCREEN_HEIGHT / 2, SCREEN_HEIGHT - PLATFORM_HEIGHT - PLATFORM_GAP);
    int platformWidth = random(MIN_PLATFORM_WIDTH, MAX_PLATFORM_WIDTH);

    // Ensure no overlap on the x-axis
    bool overlap = false;
    if (!platforms.empty()) {
      Platform lastPlatform = platforms.back();
      if (lastPlatform.x < SCREEN_WIDTH && lastPlatform.x + lastPlatform.width > SCREEN_WIDTH) {
        overlap = true;
      }
    }

    if (!overlap) {
      platforms.push_back({SCREEN_WIDTH, platformY, platformWidth});
      // Random delay for next platform
      nextPlatformTime = currentTime + random(MIN_PLATFORM_DELAY, MAX_PLATFORM_DELAY);
    } else {
      // Delay next platform addition by 10 ms
      nextPlatformTime = currentTime + 400;
    }
  }

  // Loop over all platforms in the vector and update their position
  for (int i = 0; i < platforms.size(); i++) {
    platforms[i].x -= PLATFORM_SPEED; // Move platform to the left

    // Remove platform if it goes off screen
    if (platforms[i].x < -platforms[i].width) {
      platforms.erase(platforms.begin() + i);
      i--;
    }
  }
}

void checkCollisions() {
  // Check each platform for a collision with player
  for (Platform plat : platforms) {
    // Check if the player is falling and lands on a platform
    if (velocityY > 0 && playerY + PLAYER_SIZE <= plat.y && playerY + PLAYER_SIZE + velocityY >= plat.y &&
        PLAYER_START_X + PLAYER_SIZE > plat.x && PLAYER_START_X < plat.x + plat.width) {
      // Landed on platform, reset velocity
      playerY = plat.y - PLAYER_SIZE;
      velocityY = 0;
    }
  }
}

void gameOver() {
  // Display game over message
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("GAME OVER");
  display.display();

  // Delay then restart game
  delay(3000);
  startGame();
}

void startGame() {
  platforms.clear();                // Clear all platforms
  int startPlatformY = SCREEN_HEIGHT - PLATFORM_HEIGHT - PLATFORM_GAP;
  platforms.push_back({PLAYER_START_X, startPlatformY, MAX_PLATFORM_WIDTH}); // Add initial platform
  playerY = startPlatformY - PLAYER_SIZE; // Start player on the initial platform
  velocityY = 0;                    // Reset player velocity
  points = 0;                       // Reset points
  lastPlatformTime = millis();      // Reset the platform timer
  nextPlatformTime = millis();      // Reset the platform timer
}

void render() {
  display.clearDisplay();

  // Draw player
  display.fillRect(PLAYER_START_X, playerY, PLAYER_SIZE, PLAYER_SIZE, WHITE);

  // Draw each platform
  for (Platform plat : platforms) {
    display.fillRect(plat.x, plat.y, plat.width, PLATFORM_HEIGHT, WHITE);
  }

  // Display points
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(SCREEN_WIDTH - 30, 0); // Position the text at the top-right corner
  display.print(points);

  display.display();
}
