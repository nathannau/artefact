#include <Arduino.h>
#include <FastLED.h>

// J'ai 6 pin d'entrées et 1 pin pour du WS2812B
#define PIN_BTN1 GPIO_NUM_35
#define PIN_BTN2 GPIO_NUM_32
#define PIN_BTN3 GPIO_NUM_33
#define PIN_BTN4 GPIO_NUM_25
#define PIN_BTN5 GPIO_NUM_26
#define PIN_BTN6 GPIO_NUM_27
#define PIN_WS2812B GPIO_NUM_13

#define NUM_LEDS 7

#define NB_TO_START 3
#define NB_TO_BREAK 1

CRGB leds[NUM_LEDS];

// Cycle :
// Attente au moins 3 boutons appuyés
// Echo de chaque diode à tour de role en 3s/echo (blanc) => Cycle1()
// Echo des paires 1-2-5, 1-3-6, 1-4-7 en 2s/echo (blanc) => Cycle2()
// Echo des paires 1-2-4-6, 1-3-5-7 en 1s/echo (blanc) => Cycle3()
// Echo de toutes les diodes en blanc-rouge-vert-bleu-(7 couleurs)... 1,5s/echo => Cycle4()
// Mode principale : Allumage de toutes diode arc-en-ciel variable (2s/transition)        => { CycleMain()
//                   Allumage de diode 1 arc-en-ciel inverse fixe+blanc (2s/transition)      { CycleMain()

// Si perd tout le bouton off
//      avant le mode principal => tout en rouge clignotant on off (0,2s/pediode)x300 => Boom !
//      pendant le mode principal => tout off

int getNbBtnPressed();
bool cycle1();
bool cycle2();
bool cycle3();
bool cycle4();
void cycleMain();
void boom();
void stopAll(CRGB color = CRGB::Black);

int16_t computePhaseState(ulong start, ulong upDuration, ulong downDuration);
CRGB transition(uint8_t from[3], uint8_t to[3], uint8_t p);
uint8_t transition(uint8_t from, uint8_t to, uint8_t p);

void setup()
{
  // init serial
  Serial.begin(9600);
  // configure pins
  pinMode(PIN_BTN1, INPUT);
  pinMode(PIN_BTN2, INPUT);
  pinMode(PIN_BTN3, INPUT);
  pinMode(PIN_BTN4, INPUT);
  pinMode(PIN_BTN5, INPUT);
  pinMode(PIN_BTN6, INPUT);
  // pinMode(PIN_WS2812B, OUTPUT);

  // init FastLED
  FastLED.addLeds<WS2812B, PIN_WS2812B, GRB>(leds, NUM_LEDS);
}

void loop()
{
  // Attente au moins 3 boutons appuyés
  stopAll();
  FastLED.show();

  while (getNbBtnPressed() < NB_TO_START)
    delay(100);

  if (!cycle1() || !cycle2() || !cycle3() || !cycle4())
  {
    boom();
  }
  else
  {
    cycleMain();
  }
  stopAll();
}

int getNbBtnPressed()
{
  int nbBtnPressed = 0;
  if (digitalRead(PIN_BTN1) == HIGH)
    nbBtnPressed++;
  if (digitalRead(PIN_BTN2) == HIGH)
    nbBtnPressed++;
  if (digitalRead(PIN_BTN3) == HIGH)
    nbBtnPressed++;
  if (digitalRead(PIN_BTN4) == HIGH)
    nbBtnPressed++;
  if (digitalRead(PIN_BTN5) == HIGH)
    nbBtnPressed++;
  if (digitalRead(PIN_BTN6) == HIGH)
    nbBtnPressed++;
  return nbBtnPressed;
}

void stopAll(CRGB color)
{
  for (uint8_t i = 0; i < NUM_LEDS; i++)
    leds[i] = color;
  FastLED.show();
}

// start : millis() au début de la phase
// demiDuration : durée de la demi phase en ms
// return : 0 en debut de phase, 255 en demi phase, puis 0 en fin de
int16_t computePhaseState(ulong start, ulong upDuration, ulong downDuration)
{
  ulong now = millis();
  ulong delta = now - start;
  if (delta <= upDuration)
    return 255 * delta / upDuration;
  delta -= upDuration;
  if (delta <= downDuration)
    return 255 * (downDuration - delta) / downDuration;
  return -1;
}

// Echo de chaque diode à tour de role en 3s/echo (blanc)
bool cycle1()
{
  ulong start = millis();

  for (uint8_t i = 0; i < 7; i++)
  {
    for (int16_t p = 0; p != -1; p = computePhaseState(start, 1500, 1500))
    {
      if (getNbBtnPressed() < NB_TO_BREAK)
        return false;
      leds[i] = CRGB(p, p, p);
      FastLED.show();
      delay(5);
    }
    leds[i] = CRGB::Black;
  }
  FastLED.show();
  return true;
}

// Echo des paires 1-2-5, 1-3-6, 1-4-7 en 2s/echo (blanc)
bool cycle2()
{
  ulong start = millis();

  for (uint8_t i = 0; i < 3; i++)
  {
    for (int16_t p = 0; p != -1; p = computePhaseState(start, 1000, 1000))
    {
      if (getNbBtnPressed() < NB_TO_BREAK)
        return false;
      leds[0] = leds[i + 1] = leds[i + 4] = CRGB(p, p, p);
      FastLED.show();
      delay(5);
    }
    leds[i] = leds[i + 1] = leds[i + 4] = CRGB::Black;
  }
  FastLED.show();
  return true;
}

// Echo des paires 1-2-4-6, 1-3-5-7 en 1s/echo (blanc)
bool cycle3()
{
  ulong start = millis();

  for (uint8_t i = 0; i < 2; i++)
  {
    for (int16_t p = 0; p != -1; p = computePhaseState(start, 500, 500))
    {
      if (getNbBtnPressed() < NB_TO_BREAK)
        return false;
      leds[0] = leds[i + 1] = leds[i + 3] = leds[i + 5] = CRGB(p, p, p);
      FastLED.show();
      delay(5);
    }
    leds[i] = leds[i + 1] = leds[i + 3] = leds[i + 5] = CRGB::Black;
  }
  FastLED.show();
  return true;
}

// Echo de toutes les diodes en blanc-rouge-vert-bleu-(7 couleurs)... 1,5s/echo
bool cycle4()
{
  ulong start = millis();
  uint8_t colors[8][3] = {
      {1, 1, 1},
      {0, 0, 1},
      {0, 1, 1},
      {0, 1, 0},
      {1, 1, 0},
      {1, 0, 0},
      {1, 0, 1},
      {1, 1, 1},
  };
  for (uint8_t i = 0; i < 8; i++)
  {
    uint8_t *color = colors[i];
    for (int16_t p = 0; p != -1; p = computePhaseState(start, 750, 750))
    {
      if (getNbBtnPressed() < NB_TO_BREAK)
        return false;
      stopAll(CRGB(p * color[0], p * color[1], p * color[2]));
      delay(5);
    }
  }
  stopAll();
  FastLED.show();
  return true;
}

// Mode principale : Allumage de toutes diode arc-en-ciel variable (2s/transition)
//                   Allumage de diode 1 arc-en-ciel inverse fixe+blanc (2s/transition)
void cycleMain()
{
  uint8_t colors[7][3] = {
      {1, 1, 1},
      {0, 0, 1},
      {0, 1, 1},
      {0, 1, 0},
      {1, 1, 0},
      {1, 0, 0},
      {1, 0, 1},
  };

  ulong start = millis();
  // starting
  for (int16_t p = 0; p != -1; p = computePhaseState(start, 2000, 0))
  {
    if (getNbBtnPressed() < NB_TO_BREAK)
      return;
    for (uint8_t i = 0; i < 7; i++)
      leds[i] = CRGB(p * colors[i][0], p * colors[i][1], p * colors[i][2]);
    FastLED.show();
    delay(5);
  }

  uint32_t offset = 0;
  while (true)
  {
    start = millis();
    // starting
    for (int16_t p = 0; p != -1; p = computePhaseState(start, 2000, 0))
    {
      if (getNbBtnPressed() < NB_TO_BREAK)
        return;
      for (uint8_t i = 0; i < 7; i++)
        if (i == 0)
        {
          uint8_t from = offset % 2 == 0 ? 0 : (7 - (offset / 2) % 7) + 1;
          uint8_t to = offset % 2 == 0 ? (7 - (offset / 2) % 7) + 1 : 0;
          leds[i] = transition(colors[from], colors[to], p);
        }
        else
        {
          uint8_t from = (i - 1 + offset) % 7 + 1;
          uint8_t to = (i + offset) % 7 + 1;
          leds[i] = transition(colors[from], colors[to], p);
        }
      FastLED.show();
      delay(5);
    }
    offset = offset + 1;
  }
  FastLED.show();
  delay(5);
}

CRGB transition(uint8_t from[3], uint8_t to[3], uint8_t p)
{
  return CRGB(transition(from[0], to[0], p),
              transition(from[1], to[1], p),
              transition(from[2], to[2], p));
}
uint8_t transition(uint8_t from, uint8_t to, uint8_t p)
{
  return from == to ? to * 255 : to > from ? p
                                           : 255 - p;
}

//  avant le mode principal => tout en rouge clignotant on off (0,2s/pediode)x300 => Boom !
void boom()
{
  for (int i = 0; i < 300; i++)
  {
    stopAll(CRGB::Red);
    delay(100);
    stopAll(CRGB::Black);
    delay(100);
  }
}