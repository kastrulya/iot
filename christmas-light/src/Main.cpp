#include <Arduino.h>

#include <NeoPixelAnimator.h>

#include <NeoPixelBus.h>
#include <WifiConfig.h>
#include <ESP8266mDNS.h>
#include <OTA.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <ESP8266WebServer.h>

ESP8266WiFiMulti WiFiMulti;

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name = "viewport" content = "width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0">
<title>ESP8266 WebSocket Demo</title>
<style>
"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }"
</style>
<script>
var websock;
function start() {
  websock = new WebSocket('ws://' + window.location.hostname + ':81/');
  websock.onopen = function(evt) { console.log('websock open'); };
  websock.onclose = function(evt) { console.log('websock close'); };
  websock.onerror = function(evt) { console.log(evt); };
  websock.onmessage = function(evt) {
    console.log(evt);
    var e = document.getElementById('ledstatus');
    if (evt.data === 'ledon') {
      e.style.color = 'red';
    }
    else if (evt.data === 'ledoff') {
      e.style.color = 'black';
    }
    else {
      console.log('unknown event');
    }
  };
}
function buttonclick(e) {
  websock.send(e.id);
}
function sendColor(color) {
    var c = color.slice(1);
    websock.send(c);
}
</script>
</head>
<body onload="javascript:start();">
<h1>ESP8266 WebSocket Demo</h1>
<div id="ledstatus"><b>LED</b></div>
<button id="ledon"  type="button" onclick="buttonclick(this);">On</button>
<button id="ledoff" type="button" onclick="buttonclick(this);">Off</button>
<input id="color" type="color" onchange="sendColor(color.value);">
</body>
</html>
)rawliteral";

const uint16_t PixelCount = 150; // this example assumes 4 pixels, making it smaller will cause a failure

#define colorSaturation 128

// three element pixels, in different order and speeds
NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(PixelCount);
RgbColor CylonEyeColor(RgbColor(0xAA, 0xAA, 0xAA));
// for esp8266 omit the pin
//NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount);

NeoPixelAnimator animations(2); // only ever need 2 animations

uint16_t lastPixel = 0; // track the eye position
int8_t moveDir = 1; // track the direction of movement

// uncomment one of the lines below to see the effects of
// changing the ease function on the movement animation
AnimEaseFunction moveEase =
//      NeoEase::Linear;
//      NeoEase::QuadraticInOut;
//      NeoEase::CubicInOut;
//        NeoEase::QuarticInOut;
//      NeoEase::QuinticInOut;
        NeoEase::SinusoidalInOut;
//      NeoEase::ExponentialInOut;
//      NeoEase::CircularInOut;

void FadeAll(uint8_t darkenBy) {
    RgbColor color;
    for (uint16_t indexPixel = 0; indexPixel < strip.PixelCount(); indexPixel++) {
        color = strip.GetPixelColor(indexPixel);
        color.Darken(darkenBy);
        strip.SetPixelColor(indexPixel, color);
    }
}

void FadeAnimUpdate(const AnimationParam &param) {
    if (param.state == AnimationState_Completed) {
        FadeAll(10);
        animations.RestartAnimation(param.index);
    }
}

void MoveAnimUpdate(const AnimationParam &param) {
    // apply the movement animation curve
    float progress = moveEase(param.progress);

    // use the curved progress to calculate the pixel to effect
    uint16_t nextPixel;
    if (moveDir == 0)
        return;
    if (moveDir > 0) {
        nextPixel = progress * PixelCount;
    } else {
        nextPixel = (1.0f - progress) * PixelCount;
    }

    // if progress moves fast enough, we may move more than
    // one pixel, so we update all between the calculated and
    // the last
    if (lastPixel != nextPixel) {
        for (uint16_t i = lastPixel + moveDir; i != nextPixel; i += moveDir) {
            strip.SetPixelColor(i, CylonEyeColor);
        }
    }
    strip.SetPixelColor(nextPixel, CylonEyeColor);
//    if (nextPixel < 140 && nextPixel > 10) {
    strip.SetPixelColor(nextPixel + 10, CylonEyeColor);
//    }


    lastPixel = nextPixel;

    if (param.state == AnimationState_Completed) {
        // reverse direction of movement
        moveDir *= -1;

        // done, time to restart this position tracking animation/timer
        animations.RestartAnimation(param.index);
    }
}

void SetupAnimations() {
    // fade all pixels providing a tail that is longer the faster
    // the pixel moves.
    animations.StartAnimation(0, 5, FadeAnimUpdate);

    // take several seconds to move eye fron one side to the other
    animations.StartAnimation(1, 5000, MoveAnimUpdate);
}

const char LEDON[] = "ledon";
const char LEDOFF[] = "ledoff";

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    Serial.printf("webSocketEvent(%d, %d, ...)\r\n", num, type);
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\r\n", num);
            break;
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(num);
            Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
            webSocket.sendTXT(num, "ledon", strlen("ledon"));

            // Send the current LED status
            if (moveDir != 0) {
                webSocket.sendTXT(num, LEDON, strlen(LEDON));
            } else {
                webSocket.sendTXT(num, LEDOFF, strlen(LEDOFF));
            }
        }
            break;
        case WStype_TEXT:
            Serial.printf("[%u] get Text: %s\r\n", num, payload);

            if (strcmp(LEDON, (const char *) payload) == 0) {
                moveDir = 1;
            } else if (strcmp(LEDOFF, (const char *) payload) == 0) {
                moveDir = 0;
            } else {
                const char *hex = (const char *) payload;
                uint32_t color = (uint32_t)strtol(hex, NULL, 16);
                //payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24);
                CylonEyeColor = HtmlColor(color);
            }
//            else {
//                Serial.println("Unknown command");
//            }
            // send data to all connected clients
            webSocket.broadcastTXT(payload, length);
            break;
        case WStype_BIN:
            Serial.printf("[%u] get binary length: %u\r\n", num, length);
            hexdump(payload, length);

            // echo data back to browser
            webSocket.sendBIN(num, payload, length);
            break;
        default:
            Serial.printf("Invalid WStype [%d]\r\n", type);
            break;
    }
}

void handleRoot() {
    server.send(200, "text/html", INDEX_HTML);
}

void handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send_P(404, "text/plain", message.c_str());
}

void setup() {
    Serial.begin(115200);
    strip.Begin();
    strip.Show();

    WiFi.mode(WIFI_STA);
    WiFi.begin(WLAN_SSID, WLAN_PASS);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    OTA.init();

    // Set up mDNS responder:
    // - first argument is the domain name, in this example
    //   the fully-qualified domain name is "esp8266.local"
    // - second argument is the IP address to advertise
    //   we send our IP address on the WiFi network
    if (MDNS.begin("lights")) {
        MDNS.addService("http", "tcp", 80);
        MDNS.addService("ws", "tcp", 81);
    }
    Serial.println("mDNS responder started");

    server.on("/", handleRoot);
    server.onNotFound(handleNotFound);
    server.begin();

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    SetupAnimations();
//    SetRandomSeed();
}

//void loop() {
//    OTA.handle();
//    // this is all that is needed to keep it running
//    // and avoiding using delay() is always a good thing for
//    // any timing related routines
////    animations.UpdateAnimations();
////    strip.Show();
//}

void loop(void) {
    OTA.handle();
    webSocket.loop();
    server.handleClient();

    animations.UpdateAnimations();
    strip.Show();
}

/*********************************
 *      RANDOM
 */

//
//NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(PixelCount);
//// For Esp8266, the Pin is omitted and it uses GPIO3 due to DMA hardware use.
//// There are other Esp8266 alternative methods that provide more pin options, but also have
//// other side effects.
////NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount);
////
//// NeoEsp8266Uart800KbpsMethod uses GPI02 instead
//
//NeoPixelAnimator animations(PixelCount); // NeoPixel animation management object
//
//
//
//
//// what is stored for state is specific to the need, in this case, the colors.
//// Basically what ever you need inside the animation update function
//struct MyAnimationState
//{
//    RgbColor StartingColor;
//    RgbColor EndingColor;
//};
//
//// one entry per pixel to match the animation timing manager
//MyAnimationState animationState[PixelCount];
//
//void SetRandomSeed()
//{
//    uint32_t seed;
//
//    // random works best with a seed that can use 31 bits
//    // analogRead on a unconnected pin tends toward less than four bits
//    seed = analogRead(0);
//    delay(1);
//
//    for (int shifts = 3; shifts < 31; shifts += 3)
//    {
//        seed ^= analogRead(0) << shifts;
//        delay(1);
//    }
//
//    // Serial.println(seed);
//    randomSeed(seed);
//}
//
//// simple blend function
//void BlendAnimUpdate(const AnimationParam& param)
//{
//    // this gets called for each animation on every time step
//    // progress will start at 0.0 and end at 1.0
//    // we use the blend function on the RgbColor to mix
//    // color based on the progress given to us in the animation
//    RgbColor updatedColor = RgbColor::LinearBlend(
//            animationState[param.index].StartingColor,
//            animationState[param.index].EndingColor,
//            param.progress);
//    // apply the color to the strip
//    strip.SetPixelColor(param.index, updatedColor);
//}
//
//void PickRandom(float luminance)
//{
//    // pick random count of pixels to animate
//    uint16_t count = random(PixelCount);
//    while (count > 0)
//    {
//        // pick a random pixel
//        uint16_t pixel = random(PixelCount);
//
//        // pick random time and random color
//        // we use HslColor object as it allows us to easily pick a color
//        // with the same saturation and luminance
//        uint16_t time = random(100, 1000);
//        float color = random(120, 360) / 360.0f;
//
//        int show = random(100);
//        float lum = show > 50 ? luminance : (show / 1000.0f);
//        animationState[pixel].StartingColor = strip.GetPixelColor(pixel);
//        animationState[pixel].EndingColor = HslColor(color, 1.0f, lum);
//
//        animations.StartAnimation(pixel, time, BlendAnimUpdate);
//
//        count--;
//    }
//}
//
//
//void setup()
//{
//    strip.Begin();
//    strip.Show();
//
//    WiFi.mode(WIFI_STA);
//    WiFi.begin(WLAN_SSID, WLAN_PASS);
//    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
//        Serial.println("Connection Failed! Rebooting...");
//        delay(5000);
//        ESP.restart();
//    }
//
//    OTA.init();
//
////    SetupAnimations();
//    SetRandomSeed();
//}
//
//void loop()
//{
//    OTA.handle();
//    // this is all that is needed to keep it running
//    // and avoiding using delay() is always a good thing for
//    // any timing related routines
////    animations.UpdateAnimations();
////    strip.Show();
//    if (animations.IsAnimating())
//    {
//        // the normal loop just needs these two to run the active animations
//        animations.UpdateAnimations();
//        strip.Show();
//    }
//    else
//    {
//        // no animations runnning, start some
//        //
//        PickRandom(0.02f); // 0.0 = black, 0.25 is normal, 0.5 is bright
//    }
//}
//
