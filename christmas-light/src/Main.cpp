#include <Arduino.h>

#include <NeoPixelAnimator.h>

#include <NeoPixelBus.h>
#include <WifiConfig.h>
#include <ESP8266mDNS.h>
#include <OTA.h>
#include <Ticker.h>






const uint16_t PixelCount = 150; // this example assumes 4 pixels, making it smaller will cause a failure
NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(PixelCount);
// For Esp8266, the Pin is omitted and it uses GPIO3 due to DMA hardware use.
// There are other Esp8266 alternative methods that provide more pin options, but also have
// other side effects.
//NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount);
//
// NeoEsp8266Uart800KbpsMethod uses GPI02 instead

NeoPixelAnimator animations(PixelCount); // NeoPixel animation management object


//
//#define colorSaturation 128
//
//// three element pixels, in different order and speeds
//NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(PixelCount);
//const RgbColor CylonEyeColor(RgbColor(0xAA,0xAA,0xAA));
//// for esp8266 omit the pin
////NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount);
//
//NeoPixelAnimator animations(4); // only ever need 2 animations
//
//uint16_t lastPixel = 0; // track the eye position
//int8_t moveDir = 1; // track the direction of movement
//
//// uncomment one of the lines below to see the effects of
//// changing the ease function on the movement animation
//AnimEaseFunction moveEase =
////      NeoEase::Linear;
////      NeoEase::QuadraticInOut;
////      NeoEase::CubicInOut;
//        NeoEase::QuarticInOut;
////      NeoEase::QuinticInOut;
////      NeoEase::SinusoidalInOut;
////      NeoEase::ExponentialInOut;
////      NeoEase::CircularInOut;
//
//void FadeAll(uint8_t darkenBy)
//{
//    RgbColor color;
//    for (uint16_t indexPixel = 0; indexPixel < strip.PixelCount(); indexPixel++)
//    {
//        color = strip.GetPixelColor(indexPixel);
//        color.Darken(darkenBy);
//        strip.SetPixelColor(indexPixel, color);
//    }
//}
//
//void FadeAnimUpdate(const AnimationParam& param)
//{
//    if (param.state == AnimationState_Completed)
//    {
//        FadeAll(10);
//        animations.RestartAnimation(param.index);
//    }
//}
//
//void MoveAnimUpdate(const AnimationParam& param)
//{
//    // apply the movement animation curve
//    float progress = moveEase(param.progress);
//
//    // use the curved progress to calculate the pixel to effect
//    uint16_t nextPixel;
//    if (moveDir > 0)
//    {
//        nextPixel = progress * PixelCount;
//    }
//    else
//    {
//        nextPixel = (1.0f - progress) * PixelCount;
//    }
//
//    // if progress moves fast enough, we may move more than
//    // one pixel, so we update all between the calculated and
//    // the last
//    if (lastPixel != nextPixel)
//    {
//        for (uint16_t i = lastPixel + moveDir; i != nextPixel; i += moveDir)
//        {
//            strip.SetPixelColor(i, CylonEyeColor);
//        }
//    }
//    strip.SetPixelColor(nextPixel, CylonEyeColor);
//
//    lastPixel = nextPixel;
//
//    if (param.state == AnimationState_Completed)
//    {
//        // reverse direction of movement
//        moveDir *= -1;
//
//        // done, time to restart this position tracking animation/timer
//        animations.RestartAnimation(param.index);
//    }
//}
//
//void SetupAnimations()
//{
//    // fade all pixels providing a tail that is longer the faster
//    // the pixel moves.
//    animations.StartAnimation(0, 5, FadeAnimUpdate);
//
//    // take several seconds to move eye fron one side to the other
//    animations.StartAnimation(1, 5000, MoveAnimUpdate);
//}

// what is stored for state is specific to the need, in this case, the colors.
// Basically what ever you need inside the animation update function
struct MyAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
};

// one entry per pixel to match the animation timing manager
MyAnimationState animationState[PixelCount];

void SetRandomSeed()
{
    uint32_t seed;

    // random works best with a seed that can use 31 bits
    // analogRead on a unconnected pin tends toward less than four bits
    seed = analogRead(0);
    delay(1);

    for (int shifts = 3; shifts < 31; shifts += 3)
    {
        seed ^= analogRead(0) << shifts;
        delay(1);
    }

    // Serial.println(seed);
    randomSeed(seed);
}

// simple blend function
void BlendAnimUpdate(const AnimationParam& param)
{
    // this gets called for each animation on every time step
    // progress will start at 0.0 and end at 1.0
    // we use the blend function on the RgbColor to mix
    // color based on the progress given to us in the animation
    RgbColor updatedColor = RgbColor::LinearBlend(
            animationState[param.index].StartingColor,
            animationState[param.index].EndingColor,
            param.progress);
    // apply the color to the strip
    strip.SetPixelColor(param.index, updatedColor);
}

void PickRandom(float luminance)
{
    // pick random count of pixels to animate
    uint16_t count = random(PixelCount);
    while (count > 0)
    {
        // pick a random pixel
        uint16_t pixel = random(PixelCount);

        // pick random time and random color
        // we use HslColor object as it allows us to easily pick a color
        // with the same saturation and luminance
        uint16_t time = random(100, 1000);
        float color = random(120, 360) / 360.0f;

        int show = random(100);
        float lum = show > 50 ? luminance : (show / 1000.0f);
        animationState[pixel].StartingColor = strip.GetPixelColor(pixel);
        animationState[pixel].EndingColor = HslColor(color, 1.0f, lum);

        animations.StartAnimation(pixel, time, BlendAnimUpdate);

        count--;
    }
}


void setup()
{
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

//    SetupAnimations();
    SetRandomSeed();
}

void loop()
{
    OTA.handle();
    // this is all that is needed to keep it running
    // and avoiding using delay() is always a good thing for
    // any timing related routines
//    animations.UpdateAnimations();
//    strip.Show();
    if (animations.IsAnimating())
    {
        // the normal loop just needs these two to run the active animations
        animations.UpdateAnimations();
        strip.Show();
    }
    else
    {
        // no animations runnning, start some
        //
        PickRandom(0.02f); // 0.0 = black, 0.25 is normal, 0.5 is bright
    }
}

