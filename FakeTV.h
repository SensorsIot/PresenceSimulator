/* Taken from
 *  https://learn.adafruit.com/fake-tv-light-for-engineers/arduino-sketch
 *  
 */

void fakeTVsub() {
  if (millis() - entryTV > holdTime) {
    // Read next 16-bit (5/6/5) color
    hi = pgm_read_byte(&colors[pixelNum * 2    ]);
    lo = pgm_read_byte(&colors[pixelNum * 2 + 1]);
    if (++pixelNum >= numPixels) pixelNum = 0;

    // Expand to 24-bit (8/8/8)
    r8 = (hi & 0xF8) | (hi >> 5);
    g8 = (hi << 5) | ((lo & 0xE0) >> 3) | ((hi & 0x06) >> 1);
    b8 = (lo << 3) | ((lo & 0x1F) >> 2);
    // Apply gamma correction, further expand to 16/16/16
    nr = (uint8_t)pgm_read_byte(&gamma8[r8]) * 257; // New R/G/B
    ng = (uint8_t)pgm_read_byte(&gamma8[g8]) * 257;
    nb = (uint8_t)pgm_read_byte(&gamma8[b8]) * 257;

    totalTime = random(250, 2500);    // Semi-random pixel-to-pixel time
    fadeTime  = random(0, totalTime); // Pixel-to-pixel transition time
    if (random(10) < 3) fadeTime = 0; // Force scene cut 30% of time
    holdTime  = totalTime - fadeTime; // Non-transition time

    startTime = millis();
    for (;;) {
      elapsed = millis() - startTime;
      if (elapsed >= fadeTime) elapsed = fadeTime;
      if (fadeTime) {
        r = map(elapsed, 0, fadeTime, pr, nr); // 16-bit interp
        g = map(elapsed, 0, fadeTime, pg, ng);
        b = map(elapsed, 0, fadeTime, pb, nb);
      } else { // Avoid divide-by-zero in map()
        r = nr;
        g = ng;
        b = nb;
      }
      for (i = 0; i < NUM_LEDS; i++) {
        r8   = r >> 8; // Quantize to 8-bit
        g8   = g >> 8;
        b8   = b >> 8;
        frac = (i << 8) / NUM_LEDS; // LED index scaled to 0-255
        if ((r8 < 255) && ((r & 0xFF) >= frac)) r8++; // Boost some fraction
        if ((g8 < 255) && ((g & 0xFF) >= frac)) g8++; // of LEDs to handle
        if ((b8 < 255) && ((b & 0xFF) >= frac)) b8++; // interp > 8bit
        strip.setPixelColor(i, r8, g8, b8);
      }
      yield();
      strip.show();
      if (elapsed >= fadeTime) break;
    }

    pr = nr; // Prev RGB = new RGB
    pg = ng;
    pb = nb;
    entryTV = millis();
  }
}
