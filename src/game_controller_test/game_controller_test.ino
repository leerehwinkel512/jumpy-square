#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Define the pins for the buttons and joystick
#define BUTTON_A 19
#define BUTTON_B 18
#define JOYSTICK_X 39
#define JOYSTICK_Y 36

#define SCREEN_WIDTH 128 // OLED display width
#define SCREEN_HEIGHT 64 // OLED display height
#define OLED_RESET    -1 // No reset pin
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(9600);

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);

  // Init OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Loop forever if fail to init OLED
  }

  // Display a welcome message
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Welcome to Test Game!");  
  display.display();
  delay(2000);
  display.clearDisplay();
}

void loop() {
  display.clearDisplay();

  // Read the state of the buttons
  bool buttonA = !digitalRead(BUTTON_A); // Active low
  bool buttonB = !digitalRead(BUTTON_B); // Active low

  // Read the joystick values
  int joystickX = analogRead(JOYSTICK_X);
  int joystickY = analogRead(JOYSTICK_Y);

  // Display the joystick values
  display.setCursor(0, 0);
  display.print("Joy X: ");
  display.println(joystickX);
  display.print("Joy Y: ");
  display.println(joystickY);

  // Check if button A is pressed
  if (buttonA) {
    display.println("Button A pressed!");
  }

  // Check if button B is pressed
  if (buttonB) {
    display.println("Button B pressed!");
  }

  // Update the display
  display.display();
  delay(100); // Short delay for debounce
}
