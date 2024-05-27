#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define BUTTON_A 19
#define JOYSTICK_X 39
#define JOYSTICK_Y 36

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define PLAYER_WIDTH 10
#define PLAYER_HEIGHT 5
#define BULLET_WIDTH 2
#define BULLET_HEIGHT 5
#define INVADER_WIDTH 7
#define INVADER_HEIGHT 7
#define JOYSTICK_DEADZONE 400
#define PLAYER_SPEED 10
#define INVADER_SPEED 10
#define INVADER_NEAR_TOLERANCE 20 // tolerance to shoot
#define INVADER_SHOOT_CHANCE 3  // chance to shoot when near
#define BULLET_SPEED 6

struct GameObject {
  int x;
  int y;
  int width;
  int height;
  bool exploding; // Flag to indicate explosion state
};

unsigned long points = 0; // Point counter

GameObject player;
GameObject bullet;
GameObject invader;
GameObject invaderBullet1; 
GameObject invaderBullet2;
GameObject invaderBullet3;  
int invaderDirection;

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_A, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  showStartupLogo();
  startGame();
}

void showStartupLogo() {
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Invader");
  display.print("Square");

  display.display();
  delay(3000);
}

void loop() {
  
  updatePlayerMovement();
  handleShooting();
  handleCollisions();
  moveInvader();
  render();

  delay(100);
}


void startGame() {
  player = {SCREEN_WIDTH / 2 - PLAYER_WIDTH / 2, SCREEN_HEIGHT - PLAYER_HEIGHT - 5, PLAYER_WIDTH, PLAYER_HEIGHT};
  bullet = {0, -1, BULLET_WIDTH, BULLET_HEIGHT}; // Start off-screen
  invader = {30, 10, INVADER_WIDTH, INVADER_HEIGHT};
  invaderBullet1 = {0, -1, BULLET_WIDTH, BULLET_HEIGHT}; // Start off-screen
  invaderBullet2 = {0, -1, BULLET_WIDTH, BULLET_HEIGHT}; // Start off-screen
  invaderBullet3 = {0, -1, BULLET_WIDTH, BULLET_HEIGHT}; // Start off-screen
  invaderDirection = 1; // 1 for right, -1 for left
  
  points = 0;
}

void updatePlayerMovement() {
  int joystickX = analogRead(JOYSTICK_X) - 2048; // Center at 0
  if (abs(joystickX) > JOYSTICK_DEADZONE) {
    player.x -= map(joystickX, -2048, 2047, -PLAYER_SPEED, PLAYER_SPEED); // Correctly scaled and inverted
  }
  player.x = max(0, min(player.x, SCREEN_WIDTH - PLAYER_WIDTH)); // Prevent out of bounds
}

void handleShooting() {

  // player shoot
  bool buttonA = !digitalRead(BUTTON_A);
  if (buttonA && bullet.y == -1) {
    bullet.x = player.x + PLAYER_WIDTH / 2 - BULLET_WIDTH / 2;
    bullet.y = player.y - BULLET_HEIGHT;
  }
  if (bullet.y != -1) {
    bullet.y -= BULLET_SPEED;
    if (bullet.y < 0) bullet.y = -1; // Reset if it goes off screen
  }
  
  // Invader shoot 1
  if (abs((invader.x + INVADER_WIDTH / 2) - (player.x + PLAYER_WIDTH / 2)) <= INVADER_NEAR_TOLERANCE && invaderBullet1.y == -1) {
      if (random(0, INVADER_SHOOT_CHANCE) == 0) {  // Adds a random chance to shoot
          invaderBullet1.x = invader.x + INVADER_WIDTH / 2 - BULLET_WIDTH / 2;
          invaderBullet1.y = invader.y + INVADER_HEIGHT;
      }
  }
  if (invaderBullet1.y != -1) {
      invaderBullet1.y += BULLET_SPEED;
      if (invaderBullet1.y > SCREEN_HEIGHT) invaderBullet1.y = -1; // Reset if it goes off screen
  }

  // Invader shoot 2
  if (abs((invader.x + INVADER_WIDTH / 2) - (player.x + PLAYER_WIDTH / 2)) <= INVADER_NEAR_TOLERANCE && invaderBullet2.y == -1) {
      if (random(0, INVADER_SHOOT_CHANCE) == 0) {  // Adds a random chance to shoot
          invaderBullet2.x = invader.x + INVADER_WIDTH / 2 - BULLET_WIDTH / 2;
          invaderBullet2.y = invader.y + INVADER_HEIGHT;
      }
  }
  if (invaderBullet2.y != -1) {
      invaderBullet2.y += BULLET_SPEED;
      if (invaderBullet2.y > SCREEN_HEIGHT) invaderBullet2.y = -1; // Reset if it goes off screen
  }

  // Invader shoot 3
  if (abs((invader.x + INVADER_WIDTH / 2) - (player.x + PLAYER_WIDTH / 2)) <= INVADER_NEAR_TOLERANCE && invaderBullet3.y == -1) {
      if (random(0, INVADER_SHOOT_CHANCE) == 0) {  // Adds a random chance to shoot
          invaderBullet3.x = invader.x + INVADER_WIDTH / 2 - BULLET_WIDTH / 2;
          invaderBullet3.y = invader.y + INVADER_HEIGHT;
      }
  }
  if (invaderBullet3.y != -1) {
      invaderBullet3.y += BULLET_SPEED;
      if (invaderBullet3.y > SCREEN_HEIGHT) invaderBullet3.y = -1; // Reset if it goes off screen
  } 

}

void handleCollisions() {

  // player hit invader
  if (bullet.y != -1 && bullet.x < invader.x + INVADER_WIDTH && bullet.x + BULLET_WIDTH > invader.x &&
      bullet.y < invader.y + INVADER_HEIGHT && bullet.y + BULLET_HEIGHT > invader.y) {
    invader.exploding = true; // Set explosion state
    bullet.y = -1; // Reset bullet

    points++; // increase points

    invader.x = random(0, SCREEN_WIDTH - INVADER_WIDTH); // Reset invader position randomly
  }

  // invader hit player 1
  if (invaderBullet1.y != -1 && invaderBullet1.x < player.x + PLAYER_WIDTH && invaderBullet1.x + BULLET_WIDTH > player.x &&
      invaderBullet1.y < player.y + PLAYER_HEIGHT && invaderBullet1.y + BULLET_HEIGHT > player.y) {
    player.exploding = true; // Set explosion state
    invaderBullet1.y = -1; // Reset invader bullet
  }

  // invader hit player 2
  if (invaderBullet2.y != -1 && invaderBullet2.x < player.x + PLAYER_WIDTH && invaderBullet2.x + BULLET_WIDTH > player.x &&
      invaderBullet2.y < player.y + PLAYER_HEIGHT && invaderBullet2.y + BULLET_HEIGHT > player.y) {
    player.exploding = true; // Set explosion state
    invaderBullet2.y = -1; // Reset invader bullet
  }

  // invader hit player 3
  if (invaderBullet3.y != -1 && invaderBullet3.x < player.x + PLAYER_WIDTH && invaderBullet3.x + BULLET_WIDTH > player.x &&
      invaderBullet3.y < player.y + PLAYER_HEIGHT && invaderBullet3.y + BULLET_HEIGHT > player.y) {
    player.exploding = true; // Set explosion state
    invaderBullet3.y = -1; // Reset invader bullet
  } 

}

void moveInvader() {
  invader.x += invaderDirection * INVADER_SPEED;
  
  // Ensure the invader reverses direction before disappearing off the screen
  if (invader.x <= 0) {
    invader.x = 0; // Correct position to be fully visible
    invaderDirection = 1; // Change direction to move right
  } else if (invader.x >= SCREEN_WIDTH - INVADER_WIDTH) {
    invader.x = SCREEN_WIDTH - INVADER_WIDTH; // Correct position to be fully visible
    invaderDirection = -1; // Change direction to move left
  }
}

void render() {
  display.clearDisplay();  // Clear the previous display content

  // player
  display.fillRect(player.x, player.y, player.width, player.height, WHITE);
  if (player.exploding) {
    display.fillCircle(player.x + PLAYER_WIDTH / 2, player.y + PLAYER_HEIGHT / 2, 10, WHITE);
    gameOver();
  }

  // player bullet
  if (bullet.y != -1) {
    display.fillRect(bullet.x, bullet.y, bullet.width, bullet.height, WHITE);
  }
  
  // invader
  display.fillRect(invader.x, invader.y, invader.width, invader.height, WHITE);
  if (invader.exploding) {
    display.fillCircle(invader.x + INVADER_WIDTH / 2, invader.y + INVADER_HEIGHT / 2, 10, WHITE);
    invader.exploding = false; // Reset explosion state
    delay(1); // Display explosion for a short time
  }

  // invader bullet 1
  if (invaderBullet1.y != -1) {
    display.fillRect(invaderBullet1.x, invaderBullet1.y, BULLET_WIDTH, BULLET_HEIGHT, WHITE);
  }

  // invader bullet 2
  if (invaderBullet2.y != -1) {
    display.fillRect(invaderBullet2.x, invaderBullet2.y, BULLET_WIDTH, BULLET_HEIGHT, WHITE);
  }

  // invader bullet 3
  if (invaderBullet3.y != -1) {
    display.fillRect(invaderBullet3.x, invaderBullet3.y, BULLET_WIDTH, BULLET_HEIGHT, WHITE);
  }

  // Display points
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(SCREEN_WIDTH - 30, 0); // Position the text at the top-right corner
  display.print(points);  

  display.display();  // Update the display with new content
}

void gameOver() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("GAME OVER");
  display.display();

  // Delay then restart game
  delay(3000);
  startGame();
}
