#include <Arduino.h>
#include <FastLED.h>

// LED do Atom Echo
#define LED_PIN 27
#define NUM_LEDS 12
CRGB leds[NUM_LEDS];

void setup() {
  // NÃO inicializa Serial para testar standalone

  // Inicializa LED
  FastLED.addLeds<SK6812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);

  // Pisca vermelho 3x no boot (confirma que iniciou)
  for (int i = 0; i < 3; i++) {
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    FastLED.show();
    delay(200);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    delay(200);
  }
}

void loop() {
  // Ciclo de cores infinito
  static uint8_t hue = 0;

  // Arco-íris rotativo
  fill_rainbow(leds, NUM_LEDS, hue, 7);
  FastLED.show();

  hue++;
  delay(20);
}
