#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <FastLED.h>
#include <Time.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <WiFi.h>

// Pin definitions
#define TFT_DC    27
#define TFT_CS    15
#define TFT_MOSI  23
#define TFT_CLK   18
#define TFT_RST   4
#define TFT_MISO  19

//  Touch SPI
#define TOUCH_SCLK  25
#define TOUCH_CS    33
#define TOUCH_MOSI  32
#define TOUCH_MISO  39
#define TOUCH_IRQ   35
FASTLED_USING_NAMESPACE

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

CRGB bgColor = CRGB::DarkGreen;
CRGB secondaryColor1 = CRGB::Red;
CRGB secondaryColor2 = CRGB::Blue;
CRGB secondaryColor3 = CRGB::WhiteSmoke;
int variability = 50;
int speed = random16(variability) + 10;
int speed_from_web = 10;
uint8_t fade_amount = 16;
uint8_t chance_amount = 16;

#define DATA_PIN 14
#define LED_TYPE WS2811
#define COLOR_ORDER RGB
#define NUM_LEDS 500
CRGB leds[NUM_LEDS];

#define FPS 20

const char *ssid = "ESP32";
AsyncWebServer server(80);

int mode = 0;
int palette_index = 0;
bool gHueState = false;
uint8_t gHue = 0;
uint8_t brightness_value = 255;
CRGBPalette16 currentPalette = RainbowColors_p;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>

<head>
  <title>LED Christmas Tree Control</title>
  <style>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #f4f4f4;
            padding: 20px;
            color: #333;
        }

        h1 {
            color: #4CAF50;
        }

        label {
            display: block;
            margin: 15px 0 5px;
        }

        input[type=color], input[type=range] {
            width: 100%;
            padding: 8px;
            border: 1px solid #ddd;
            border-radius: 4px;
            box-sizing: border-box;
        }

        button {
            background-color: #4CAF50;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 16px;
        }

        button:hover {
            background-color: #45a049;
        }

        .control-group {
            margin-bottom: 20px;
        }
    </style>
  </style>
</head>

<body>
  <h1>LED Christmas Tree Control</h1>

  <label for="functionSelect">Select Function:</label>
  <select id="functionSelect">
      <option value="0">Normal</option>
      <option value="1">Color Waves</option>
      <option value="2">Twinkling Stars</option>
      <option value="3">Candy Cane</option>
      <option value="4">Falling Snow</option>
      <option value="5">Rising Sparkles</option>
      <option value="6">Meteor Rain</option>
  </select>
  <button onclick="sendFunction()">Apply Function</button>

  <br><br>

  <label for="hueChange">Enable gHue Increment:</label>
  <input type="checkbox" id="hueChange" checked>
  <button onclick="sendHueChange()">Apply</button>

  <br><br>

  <label for="paletteSelect">Select Color Palette:</label>
  <select id="paletteSelect">
      <option value="0">RainbowColors</option>
      <option value="1">RainbowStripeColors</option>
      <option value="2">CloudColors</option>
      <option value="3">LavaColors</option>
      <option value="4">OceanColors</option>
      <option value="5">ForestColors</option>
      <option value="6">PartyColors</option>
      <option value="7">HeatColors</option>
  </select>
  <button onclick="sendPalette()">Change Palette</button>

  <br><br>

  <label for="baseColor">Base Color:</label>
  <input type="color" id="baseColor" name="baseColor" onchange="changeBaseColor(this.value)">

  <br><br>

  <label for="secondaryColor1">Secondary Color 1:</label>
  <input type="color" id="secondaryColor1" name="secondaryColor1" onchange="changeSecondaryColor1(this.value)">

  <br><br>

  <label for="secondaryColor2">Secondary Color 2:</label>
  <input type="color" id="secondaryColor2" name="secondaryColor2" onchange="changeSecondaryColor2(this.value)">

  <br><br>

  <label for="secondaryColor2">Secondary Color 3:</label>
  <input type="color" id="secondaryColor3" name="secondaryColor3" onchange="changeSecondaryColor3(this.value)">

  <br><br>


  <label for="flickerRate">Per Second Update Rate:</label>
  <input type="range" id="flickerRate" name="flickerRate" min="1" max="100" onchange="changeFlickerRate(this.value)">

  <br><br>

  <label for="brightness">Brightness:</label>
  <input type="range" id="brightness" name="brightness" min="0" max="255" onchange="changeBrightness(this.value)">

  <br><br>

  <label for="fade_amount">Fade Amount:</label>
  <input type="range" id="fade_amount" name="fade_amount" min="0" max="255" onchange="changeFadeAmount(this.value)">
  
  <br><br>

  <label for="chance_amount">Chance Amount:</label>
  <input type="range" id="chance_amount" name="chance_amount" min="0" max="255" onchange="changeChanceAmount(this.value)">
  
  <br><br>

  <label for="max_snow">Max Snow:</label>
  <input type="range" id="max_snow" name="max_snow" min="0" max="50" onchange="changeMaxSnow(this.value)">
  
  <br><br>

  <label for="variability">Speed Variability:</label>
  <input type="range" id="variability" name="variability" min="0" max="200" onchange="changeVariability(this.value)">

  <script>
    // JavaScript functions to handle changes and send requests to ESP32

    function sendFunction() {

      var functionValue = document.getElementById("functionSelect").value;

      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            console.log('LED function changed to: ' + functionValue);
        }
      };
      var url = "http://192.168.4.1/setLEDFunction";
      xhttp.open("POST", url, true);
      xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
      xhttp.send("function=" + functionValue);
    }

    function sendHueChange() {
      var toggle = document.getElementById("hueChange").checked;
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
          if (this.readyState == 4 && this.status == 200) {
            console.log('gHue incrementation toggled: ' + toggle);
          }
      };
      var url = "http://192.168.4.1/toggleHueIncrement";
      xhttp.open("POST", url, true);
      xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
      xhttp.send("hueIncrement=" + toggle);
    }

    function sendPalette() {
      var paletteValue = document.getElementById("paletteSelect").value;
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
          if (this.readyState == 4 && this.status == 200) {
            console.log('Color palette changed to: ' + paletteValue);
          }
      };
      var url = "http://192.168.4.1/setColorPalette";
      xhttp.open("POST", url, true);
      xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
      xhttp.send("palette=" + paletteValue);
    }


    function changeBaseColor(color) {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          console.log('Base color changed to: ' + color);
        }
      };

      // Specify your ESP32 endpoint here
      var url = "http://192.168.4.1/setBaseColor";

      xhttp.open("POST", url, true);
      xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
      xhttp.send("color=" + color);
    }


    function changeSecondaryColor1(color) {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          console.log('Secondary color 1 changed to: ' + color);
        }
      };

      // Specify your ESP32 endpoint here
      var url = "http://192.168.4.1/setSecondaryColor1";

      xhttp.open("POST", url, true);
      xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
      xhttp.send("color=" + color);
    }

    function changeSecondaryColor2(color) {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          console.log('Secondary color 2 changed to: ' + color);
        }
      };

      // Specify your ESP32 endpoint here
      var url = "http://192.168.4.1/setSecondaryColor2";

      xhttp.open("POST", url, true);
      xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
      xhttp.send("color=" + color);
    }

    function changeSecondaryColor3(color) {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          console.log('Secondary color 3 changed to: ' + color);
        }
      };

      // Specify your ESP32 endpoint here
      var url = "http://192.168.4.1/setSecondaryColor3";

      xhttp.open("POST", url, true);
      xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
      xhttp.send("color=" + color);
    }

    function changeFlickerRate(rate) {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          console.log('Rate changed to: ' + rate);
        }
      };

      // Specify your ESP32 endpoint here
      var url = "http://192.168.4.1/setRate";

      xhttp.open("POST", url, true);
      xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
      xhttp.send("rate=" + rate);
    }

    function changeBrightness(brightness) {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          console.log('Rate changed to: ' + rate);
        }
      };

      // Specify your ESP32 endpoint here
      var url = "http://192.168.4.1/setBrightness";

      xhttp.open("POST", url, true);
      xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
      xhttp.send("brightness=" + brightness);
    }

    function changeFadeAmount(fade_amount) {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          console.log('Fade amount changed to: ' + fade_amount);
        }
      };

      // Specify your ESP32 endpoint here
      var url = "http://192.168.4.1/setFadeAmount";

      xhttp.open("POST", url, true);
      xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
      xhttp.send("fade_amount=" + fade_amount);
    }

    function changeVariability(variability) {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          console.log('Speed variability changed to: ' + variability);
        }
      };

      // Specify your ESP32 endpoint here
      var url = "http://192.168.4.1/setVariability";

      xhttp.open("POST", url, true);
      xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
      xhttp.send("variability=" + variability);
    }

    function changeChanceAmount(chance_amount) {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          console.log('Chance amount changed to: ' + chance_amount);
        }
      };

      // Specify your ESP32 endpoint here
      var url = "http://192.168.4.1/setChanceAmount";

      xhttp.open("POST", url, true);
      xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
      xhttp.send("chance_amount=" + chance_amount);
    }

    function changeMaxSnow(max_snow) {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          console.log('Max snow changed to: ' + max_snow);
        }
      };

      // Specify your ESP32 endpoint here
      var url = "http://192.168.4.1/setMaxSnow";

      xhttp.open("POST", url, true);
      xhttp.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
      xhttp.send("max_snow=" + max_snow);
    }
  </script>
</body>

</html>

)rawliteral";


void setPallette(int i) {
  switch (i) {
    case 0:
      currentPalette = RainbowColors_p;
      break;
    case 1:
      currentPalette = RainbowStripeColors_p;
      break;
    case 2:
      currentPalette = CloudColors_p;
      break;
    case 3:
      currentPalette = LavaColors_p;
      break;
    case 4:
      currentPalette = OceanColors_p;
      break;
    case 5:
      currentPalette = ForestColors_p;
      break;
    case 6:
      currentPalette = PartyColors_p;
      break;
    case 7:
      currentPalette = HeatColors_p;
      break;
  }
}

void colorWaves(uint8_t& gHue, bool increment_gHue, uint8_t brightness) {
  if (increment_gHue) {
    gHue++;
  }
  fill_palette(leds, NUM_LEDS, gHue, 7, currentPalette, brightness, LINEARBLEND);
}

void twinklingStars(CRGB color1) {
  fadeToBlackBy(leds, NUM_LEDS, 20);
  int pos = random(NUM_LEDS);
  leds[pos] += color1; // cool white color
}

void candyCane(CRGB color1, CRGB color2) {
  static uint8_t stripePattern = 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = (i + stripePattern) % 4 < 2 ? color1 : color2;
  }
  stripePattern++;
}

struct Snowflake {
    int position;
    int speed;
};

const int MAX_SNOWFLAKES_LIMIT = 50; // Set this to a reasonable upper limit
Snowflake snowflakes[MAX_SNOWFLAKES_LIMIT];
byte snowflakeChance = 20;       // Chance of new snowflake (0-255)
int maxSnowflakes = 10;

void initializeSnowflakes() {
    for (int i = 0; i < maxSnowflakes; i++) {
        snowflakes[i].position = -1; // Initialize all as inactive
        snowflakes[i].speed = 0;
    }
}

void updateSnowflakeSpeed() {
    for (int i = 0; i < maxSnowflakes; i++) {
        if (snowflakes[i].position != -1) {
            // Increase the speed as the snowflake moves down the tree
            snowflakes[i].speed = map(snowflakes[i].position, 0, NUM_LEDS - 1, 1, 5);
        }
    }
}

void fallingSnowEffect() {
    // Clear the LEDs
    fill_solid(leds, NUM_LEDS, CRGB::Black);

    // Move each active snowflake down (towards the start of the LED strip)
    for (int i = 0; i < maxSnowflakes; i++) {
        if (snowflakes[i].position != -1) {
            leds[snowflakes[i].position] = CRGB::Black; // Clear the current position

            snowflakes[i].position -= snowflakes[i].speed; // Move the snowflake down

            // Check if the snowflake has reached the bottom
            if (snowflakes[i].position < 0) {
                snowflakes[i].position = -1; // Deactivate the snowflake
            } else {
                leds[snowflakes[i].position] = CRGB::White; // Draw the snowflake at the new position
            }
        }
    }

    // Activate a new snowflake at the top (the end of the LED strip)
    if (random8() < snowflakeChance) {
        for (int i = 0; i < maxSnowflakes; i++) {
            if (snowflakes[i].position == -1) {
                snowflakes[i].position = NUM_LEDS - 1; // Start from the top of the tree
                break;
            }
        }
    }

    FastLED.show();
}

void risingSparklesEffect() {
    const uint8_t sparkleChance = 5; // Chance of new sparkle
    const uint8_t sparkleFade = 80;  // Sparkle fade out speed

    // Shift everything up one pixel per frame
    for (int i = 0; i < NUM_LEDS - 1; i++) {
        leds[i] = leds[i + 1];
    }

    // Randomly create new sparkle at the bottom
    if (random8() < sparkleChance) {
        leds[NUM_LEDS - 1] = CHSV(random8(160, 200), 255, 255); // Cool color range
    } else {
        leds[NUM_LEDS - 1] = CRGB::Black;
    }

    // Fade the LEDs
    fadeToBlackBy(leds, NUM_LEDS, sparkleFade);
}

void setAll(CRGB color) {
    for(int i = 0; i < NUM_LEDS; i++) {
        leds[i] = color;
    }
    FastLED.show();
}

void fadeToBlack(int ledNo, byte fadeValue) {
    leds[ledNo].fadeToBlackBy(fadeValue);
}

void meteorRain(CRGB color, byte meteorSize, byte meteorTrailDecay, bool meteorRandomDecay, int SpeedDelay) {
    setAll(CRGB::Black);

    for(int i = 0; i < NUM_LEDS + meteorSize; i++) {
        // Fade brightness all LEDs one step
        for(int j = 0; j < NUM_LEDS; j++) {
            if((!meteorRandomDecay) || (random(10) > 5)) {
                fadeToBlack(j, meteorTrailDecay);        
            }
        }
        
        // Draw meteor
        for(int j = 0; j < meteorSize; j++) {
            if((i - j < NUM_LEDS) && (i - j >= 0)) {
                leds[i - j] = color;
            } 
        }
   
        FastLED.show();
        delay(SpeedDelay);
    }
}

void glitter(CRGB color1, CRGB color2, CRGB color3)
{

  if (random8() < 255)
  {
    // set random LED to white
    leds[random16(NUM_LEDS)] = color1;

    // set random LED to red
    leds[random16(NUM_LEDS)] = color2;

    // set random LED to red
    leds[random16(NUM_LEDS)] = color3;
  }
}

// Helper function that blends one uint8_t toward another by a given amount
void nblendU8TowardU8(uint8_t &cur, const uint8_t target, uint8_t amount)
{
  if (cur == target)
    return;

  if (cur < target)
  {
    uint8_t delta = target - cur;
    delta = scale8_video(delta, amount);
    cur += delta;
  }
  else
  {
    uint8_t delta = cur - target;
    delta = scale8_video(delta, amount);
    cur -= delta;
  }
}

// Blend one CRGB color toward another CRGB color by a given amount.
// Blending is linear, and done in the RGB color space.
// This function modifies 'cur' in place.
CRGB fadeTowardColor(CRGB &cur, const CRGB &target, uint8_t amount)
{
  nblendU8TowardU8(cur.red, target.red, amount);
  nblendU8TowardU8(cur.green, target.green, amount);
  nblendU8TowardU8(cur.blue, target.blue, amount);
  return cur;
}

// Fade an entire array of CRGBs toward a given background color by a given amount
// This function modifies the pixel array in place.
void fadeTowardColor(CRGB *L, uint16_t N, const CRGB &bgColor, uint8_t fadeAmount)
{
  for (uint16_t i = 0; i < N; i++)
  {
    fadeTowardColor(L[i], bgColor, fadeAmount);
  }
}

// Function to convert a single hexadecimal character to an integer
uint8_t hexCharToUint(char hexChar)
{
  if (hexChar >= '0' && hexChar <= '9')
  {
    return hexChar - '0';
  }
  else if (hexChar >= 'A' && hexChar <= 'F')
  {
    return 10 + (hexChar - 'A');
  }
  else if (hexChar >= 'a' && hexChar <= 'f')
  {
    return 10 + (hexChar - 'a');
  }
  else
  {
    return 0;
  }
}

// Function to convert a two-character hexadecimal string to an unsigned byte (uint8_t)
uint8_t hexStringToUint8(String hexString)
{
  return 16 * hexCharToUint(hexString.charAt(0)) + hexCharToUint(hexString.charAt(1));
}

// Function to convert a hex color string to a CRGB object
CRGB hexToCRGB(String hexColor)
{
  if (hexColor.length() < 6)
  {
    // Return black or some default color if the string is too short
    return CRGB::Black;
  }

  uint8_t r = hexStringToUint8(hexColor.substring(1, 3));
  uint8_t g = hexStringToUint8(hexColor.substring(3, 5));
  uint8_t b = hexStringToUint8(hexColor.substring(5, 7));

  return CRGB(r, g, b);
}

void setup(void)
{
  Serial.begin(115200);

  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 20);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  WiFi.mode(WIFI_AP);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  tft.println("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  tft.print("AP IP address: ");
  Serial.println(IP);
  tft.println(IP);
  
  initializeSnowflakes();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html); });

  server.on("/setLEDFunction", HTTP_POST, [](AsyncWebServerRequest *request)
            {
      String func;
      if (request->hasParam("function", true)) {
          func = request->getParam("function", true)->value();
          Serial.println("Mode = " + func);
          mode = func.toInt();
          tft.println("Mode = " + func);
      }
      request->send(200, "text/plain", "Mode set to " + func); });

  server.on("/toggleHueIncrement", HTTP_POST, [](AsyncWebServerRequest *request)
            {
      String hue_tog;
      if (request->hasParam("hueIncrement", true)) {
          hue_tog = request->getParam("hueIncrement", true)->value();
          Serial.println("Hue incrementation toggled = " + hue_tog);
          gHueState = (hue_tog == "true");
          tft.println("Hue incrementation toggled = " + hue_tog);
      }
      request->send(200, "text/plain", "Hue incrementation toggled " + hue_tog); });

  server.on("/setColorPalette", HTTP_POST, [](AsyncWebServerRequest *request)
            {
      String palette;
      if (request->hasParam("palette", true)) {
          palette = request->getParam("palette", true)->value();
          Serial.println("Palette = " + palette);
          palette_index = palette.toInt();
          setPallette(palette_index);
          tft.println("Palette = " + palette);
      }
      request->send(200, "text/plain", "Palette set to " + palette); });

  server.on("/setBaseColor", HTTP_POST, [](AsyncWebServerRequest *request)
            {
      String color;
      if (request->hasParam("color", true)) {
          color = request->getParam("color", true)->value();
          Serial.println("Base Color = " + color);
          bgColor = hexToCRGB(color);
          tft.println("Base Color = " + color);
      }
      request->send(200, "text/plain", "Base color set to " + color); });

  server.on("/setSecondaryColor1", HTTP_POST, [](AsyncWebServerRequest *request)
            {
      String color;
      if (request->hasParam("color", true)) {
          color = request->getParam("color", true)->value();
          Serial.println("Secondary Color 1 = " + color);
          secondaryColor1 = hexToCRGB(color);
          tft.println("Secondary Color 1 = " + color);
      }
      request->send(200, "text/plain", "Secondary color 1 set to " + color); });

  server.on("/setSecondaryColor2", HTTP_POST, [](AsyncWebServerRequest *request)
            {
      String color;
      if (request->hasParam("color", true)) {
          color = request->getParam("color", true)->value();
          Serial.println("Secondary Color 2 = " + color);
          secondaryColor2 = hexToCRGB(color);
          tft.println("Secondary Color 2 = " + color);
      }
      request->send(200, "text/plain", "Secondary color 2 set to " + color); });

  server.on("/setSecondaryColor3", HTTP_POST, [](AsyncWebServerRequest *request)
            {
      String color;
      if (request->hasParam("color", true)) {
          color = request->getParam("color", true)->value();
          Serial.println("Secondary Color 3 = " + color);
          secondaryColor3 = hexToCRGB(color);
          tft.println("Secondary Color 3 = " + color);
      }
      request->send(200, "text/plain", "Secondary color 3 set to " + color); });

  server.on("/setRate", HTTP_POST, [](AsyncWebServerRequest *request)
            {
      String rate;
      if (request->hasParam("rate", true)) {
          rate = request->getParam("rate", true)->value();
          Serial.println("Update Rate = " + rate);
          speed_from_web = rate.toInt();
          tft.println("Update Rate = " + rate);
      }
      request->send(200, "text/plain", "Rate set to " + rate); });

  server.on("/setBrightness", HTTP_POST, [](AsyncWebServerRequest *request)
            {
      String brightness;
      if (request->hasParam("brightness", true)) {
          brightness = request->getParam("brightness", true)->value();
          Serial.println("Brightness = " + brightness);
          FastLED.setBrightness(brightness.toInt());
          brightness_value = brightness.toInt();
          tft.println("Brightness = " + brightness);
      }
      request->send(200, "text/plain", "Brightness set to " + brightness); });

  server.on("/setFadeAmount", HTTP_POST, [](AsyncWebServerRequest *request)
            {
      String _fade_amount;
      if (request->hasParam("fade_amount", true)) {
          _fade_amount = request->getParam("fade_amount", true)->value();
          Serial.println("Fade Back To Base Amount = " + _fade_amount);
          fade_amount = _fade_amount.toInt();
          tft.println("Fade Back To Base Amount = " + _fade_amount);
      }
      request->send(200, "text/plain", "Fade amount set to " + fade_amount); });

  server.on("/setChanceAmount", HTTP_POST, [](AsyncWebServerRequest *request)
            {
      String _chance_amount;
      if (request->hasParam("chance_amount", true)) {
          _chance_amount = request->getParam("chance_amount", true)->value();
          Serial.println("Chance Amount = " + _chance_amount);
          chance_amount = _chance_amount.toInt();
          tft.println("Chance Amount = " + _chance_amount);
          snowflakeChance = chance_amount;
          initializeSnowflakes();
      }
      request->send(200, "text/plain", "Chance amount set to " + chance_amount); });

  server.on("/setMaxSnow", HTTP_POST, [](AsyncWebServerRequest *request)
            {
      String _max_snow;
      if (request->hasParam("max_snow", true)) {
          _max_snow = request->getParam("max_snow", true)->value();
          Serial.println("Max Snow = " + _max_snow);
          maxSnowflakes = _max_snow.toInt();
          tft.println("Max Snow = " + _max_snow);
          initializeSnowflakes();
      }
      request->send(200, "text/plain", "Max Snow set to " + _max_snow); });

  server.on("/setVariability", HTTP_POST, [](AsyncWebServerRequest *request)
            {
      String _variability;
      if (request->hasParam("variability", true)) {
          _variability = request->getParam("variability", true)->value();
          Serial.println("Speed Variability = " + _variability);
          variability = _variability.toInt();
          tft.println("Speed Variability = " + _variability);
      }
      request->send(200, "text/plain", "Variability amount set to " + variability); });

  ElegantOTA.begin(&server);
  server.begin();
  Serial.println("HTTP server started");
  tft.println("HTTP server started");
}

void loop(void)
{
  ElegantOTA.loop();

  if (mode == 0) {
    glitter(secondaryColor1, secondaryColor2, secondaryColor3);

    // fade all existing pixels toward bgColor by "16" (out of 255)
    fadeTowardColor(leds, NUM_LEDS, bgColor, fade_amount);
  } else if (mode == 1) {
    colorWaves(gHue, gHueState, brightness_value);
  } else if (mode == 2) {
    twinklingStars(secondaryColor1);
  } else if (mode == 3) {
    candyCane(secondaryColor1, secondaryColor2);
  } else if (mode == 4) {
    updateSnowflakeSpeed();
    fallingSnowEffect();
  } else if (mode == 5) {
    risingSparklesEffect();
  } else if (mode == 6) {
    meteorRain(secondaryColor1, 10, 64, true, 30);
  }

  // send the 'leds' array out to the actual LED strip
  FastLED.show();

  speed = random16(variability) + speed_from_web;

  // insert a delay to keep the framerate modest
  delay(1000 / speed);
}
