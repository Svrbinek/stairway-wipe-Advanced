This improved user mode is based on the original light control usermod, which was the first in WLED usermod.
https://github.com/Aircoookie/WLED/tree/master/usermods/stairway_wipe_basic
This is improved in that it can turn on the lights even when the stairs go out from any direction. Turns off with HTTP API commands U0 = 0 & RV = 0 or U0 = 0 & RV = 1 lights up with U0 = 1 or U0 = 2
The effects of the blue extreme LEDs start automatically when the effect ends with an internal call to the preset. Stair initialization also requires calling a preset before starting (grupping setting 60).
This staircase contains 15 steps of 60 LEDs SK6812 on each step (1 meter).
The trigger is controlled by VL53L0X 2x 2 sensors (top and bottom) and by ESP Home, the logic is in Node RED and everything under Home assistant.
Therefore, 2 ESP32 modules are required. One is for WLED and the other for sensors.

sample https://youtu.be/fVVrSglzSgY
