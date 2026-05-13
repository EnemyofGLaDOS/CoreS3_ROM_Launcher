#pragma once
// M5Stack CoreS3 Hardware Configuration
// Display: ILI9342 320x240

#define CONFIG_LCD_TYPE_AUTO
#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  240
#define CONFIG_HW_LCD_WIDTH     320
#define CONFIG_HW_LCD_HEIGHT    240

// Audio — disabled
#define CONFIG_AUDIO_DISABLED   1

// PSRAM is present (OPI 8MB)
#define CONFIG_HW_PSRAM_ENABLED 1

// Filesystem — use SD card
#define FILESYSTEM SD
#define filesystem SD

#define NOFRENDO_PSRAM 1
