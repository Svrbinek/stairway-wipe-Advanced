#pragma once
#include "wled.h"

/*
 * Uživatelské modifikace (Usermods) umožňují přidat vlastní funkčnost do WLED jednodušeji.
 * Více informací: https://github.com/Aircoookie/WLED/wiki/Add-own-functionality
 *
 * Toto je příklad uživatelského módu pro efekt "stírání schodů".
 */

class StairwayWipeUsermod : public Usermod {
  private:
    // Proměnné
    unsigned long lastTime = 0;
    unsigned long stateStartTime = 0; // Čas zahájení aktuálního stavu
    byte wipeState = 0; // 0: neaktivní, 1: stírání, 2: statický efekt
    unsigned long timeStaticStart = 0;
    uint16_t previousUserVar0 = 0;

    byte lastWipeState = 255; // Poslední uložený stav, pro kontrolu změny
    uint16_t lastUserVar0 = 0; // Poslední uložená hodnota userVar0
    uint16_t lastPreviousUserVar0 = 0; // Poslední hodnota previousUserVar0

    // Odkomentujte, pokud chcete reverzní efekt při vypínání
    #define STAIRCASE_WIPE_OFF

    // Počet stupňů (segmentů)
    uint8_t numSteps = 15;
    // Počet LED na stupeň
    uint16_t ledsPerStep = 60;
    // Barva pro okrajové LED
    uint32_t edgeColor = 0x0000FF;
    // Barva pro přechod
    uint32_t fadeColor = 0xFFFFFF;
    // Zpoždění pro přechod v ms
    uint16_t fadeDelay = 50;
    // Inkrement jasu pro přechod
    uint16_t fadeIncrement = 10;

    // Pole pro sledování aktivity a jasu každého stupně
    bool isActive[15] = {false};
    bool isFadingIn[15] = {false};
    uint8_t currentBrightness[15] = {0};

    // Vrátí začáteční index LED pro daný stupeň
    uint16_t getStepStartIndex(uint8_t step) {
        return step * ledsPerStep;
    }

    // Škáluje 32bitovou barvu podle jasu
    uint32_t scaleColor(uint32_t color, uint8_t brightness) {
        uint8_t r = ((color >> 16) & 0xFF) * brightness / 255;
        uint8_t g = ((color >> 8) & 0xFF) * brightness / 255;
        uint8_t b = (color & 0xFF) * brightness / 255;
        return (r << 16) | (g << 8) | b;
    }

    // Nastaví barvu pro LED daného stupně s určitým jasem
    void setStepColor(uint8_t step, uint32_t color, uint8_t brightness) {
        uint16_t startIndex = getStepStartIndex(step);
        for (uint16_t i = 0; i < ledsPerStep; i++) {
            uint16_t pixelIndex = startIndex + i;
            if (pixelIndex < strip.getLengthTotal()) {
                strip.setPixelColor(pixelIndex, scaleColor(color, brightness));
            }
        }
    }

    // Nastaví okrajové LED (první a poslední) daného stupně s určitou barvou a jasem
    void setEdgeLEDs(uint8_t step, uint32_t color, uint8_t brightness) {
        uint16_t startIndex = getStepStartIndex(step);
        if (startIndex < strip.getLengthTotal()) {
            strip.setPixelColor(startIndex, scaleColor(color, brightness)); // První LED
        }
        if (startIndex + ledsPerStep - 1 < strip.getLengthTotal()) {
            strip.setPixelColor(startIndex + ledsPerStep - 1, scaleColor(color, brightness)); // Poslední LED
        }
    }

    // Aktualizuje efekt přechodu pro všechny aktivní stupně
    void updateFade() {
        if (millis() - lastTime < fadeDelay) return;

        lastTime = millis();
        for (uint8_t step = 0; step < numSteps; step++) {
            if (!isActive[step]) continue;

            uint8_t& brightness = currentBrightness[step];
            if (isFadingIn[step]) {
                brightness = min(brightness + fadeIncrement, 255);
            } else {
                brightness = max(brightness - fadeIncrement, 0);
            }

            setStepColor(step, fadeColor, brightness);

            if (brightness == 255 || brightness == 0) {
                isFadingIn[step] = false;
                if (brightness == 0) {
                    setEdgeLEDs(step, edgeColor, 255); // Obnoví okrajové LED při zhasínání
                    isActive[step] = false;
                }
            }
        }
        strip.trigger(); // Aplikuje změny na LED pásku
    }

  public:
    void setup() override {
      // Inicializace
      Serial.println("Usermod setup initialized.");
    }

    void loop() override {
      // Podmíněný ladicí výstup: pouze při změně
      if (wipeState != lastWipeState || userVar0 != lastUserVar0 || previousUserVar0 != lastPreviousUserVar0) {
        unsigned long stateDuration = millis() - stateStartTime; // Doba trvání předchozího stavu
        Serial.println((String)"Aktuální stav: userVar0: " + userVar0 +
                       ", previousUserVar0: " + previousUserVar0 +
                       ", wipeState: " + wipeState +
                       ", Trvání stavu: " + stateDuration + " ms");
        lastWipeState = wipeState;
        lastUserVar0 = userVar0;
        lastPreviousUserVar0 = previousUserVar0;
        stateStartTime = millis(); // Aktualizace času pro nový stav
      }

      // Logika efektů
      if (userVar0 > 0) {
        if ((previousUserVar0 == 1 && userVar0 == 2) || (previousUserVar0 == 2 && userVar0 == 1))
          wipeState = 3; // Přepnutí na vypnutí při změně směru
        previousUserVar0 = userVar0;
      } else {
        resetuj();
      }

      // Zpracování stavu
      switch(wipeState) {
        case 0: { // Neaktivní
          if (userVar0 > 0) {
            startWipe();
            wipeState = 1;
          }
          break;
        }
        case 1: { // Stírání
            updateFade();
            break;
        }
        case 2: { // Statický efekt
          if (userVar1 > 0) { // Časovač
            if (millis() - timeStaticStart > userVar1 * 1000)
              wipeState = 3;
          }
          break;
        }
        case 3: { // Přepnutí na vypnutí
          effectCurrent = FX_MODE_COLOR_WIPE;
          strip.timebase = 360 + (255 - effectSpeed) * 75 - millis(); // Správné načasování
          colorUpdated(CALL_MODE_NOTIFICATION);
          wipeState = 4;
          break;
        }
        default: {
          if (millis() + strip.timebase > (725 + (255 - effectSpeed) * 150))
            turnOff(); // Dokončení vypnutí
          if (userVar0 != 6) {
            userVar0 = previousUserVar0;
            wipeState = 0;
          }
          break;
        }
      }
    }

    void readFromJsonState(JsonObject& root) override {
      userVar0 = root["user0"] | userVar0; // Pokud existuje klíč "user0" v JSONu, aktualizuje, jinak ponechá starou hodnotu.
    }

    uint16_t getId() override {
      return USERMOD_ID_EXAMPLE;
    }

    void resetuj() {
      wipeState = 0;
      if (previousUserVar0) {
        #ifdef STAIRCASE_WIPE_OFF
        userVar0 = 6; // Speciální hodnota pro wipeState 4
        wipeState = 3;
        #else
        turnOff();
        #endif
        previousUserVar0 = 0;
      }
    }

    void startWipe() {
      applyPreset(2, true);
      bri = briLast; // Zapnutí světel
      strip.setTransition(0); // Bez přechodu
      effectCurrent = FX_MODE_COLOR_WIPE;
      effectSpeed = 240;
      strip.resetTimebase(); // Správné načasování

      // Nastavení směru
      Segment& seg = strip.getSegment(0);
      bool doReverse = (userVar0 == 2);
      seg.setOption(1, doReverse);

      colorUpdated(CALL_MODE_NOTIFICATION);

      // Ladicí výstupy
      Serial.println("Efekt stírání spuštěn.");
      Serial.println((String)"Jas: " + bri + ", Rychlost: " + effectSpeed + ", Směr: " + (doReverse ? "Reverzní" : "Normální"));
      Serial.println((String)"Přechod: " + strip.getTransition() + ", Aktuální efekt: " + effectCurrent);
    }

    void turnOff() {
      #ifdef STAIRCASE_WIPE_OFF
      strip.setTransition(0); // Okamžité vypnutí po dokončení stírání.
      #else
      strip.setTransition(4000); // Pomalu zeslabující efekt.
      #endif
      bri = 0;
      applyPreset(3, true);
      colorUpdated(CALL_MODE_NOTIFICATION);
      wipeState = 0;
      userVar0 = 0;
      previousUserVar0 = 0;
    }

    // Statické řetězce pro ušetření paměti flash
    static const char _name[];
    static const char _enabled[];

    // Deklarace statické metody pro publikování MQTT zprávy
    static void publishMqtt(const char* state, bool retain);
};

// Definice statických řetězců
const char StairwayWipeUsermod::_name[]    PROGMEM = "StairwayWipeUsermod";
const char StairwayWipeUsermod::_enabled[] PROGMEM = "enabled";

// Implementace statické metody pro publikování MQTT zprávy
void StairwayWipeUsermod::publishMqtt(const char* state, bool retain) {
#ifndef WLED_DISABLE_MQTT
  // Zkontrolujte, zda je MQTT připojeno, jinak by to mohlo způsobit pád 8266
  if (WLED_MQTT_CONNECTED) {
    char subuf[64];
    strcpy(subuf, mqttDeviceTopic);
    strcat_P(subuf, PSTR("/example"));
    mqtt->publish(subuf, 0, retain, state);
  }
#endif
}
