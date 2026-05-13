/* Arduino Nofrendo - CoreS3
 * ROM Selector Edition
 *
 * On boot, scans /roms/ on the SD card and displays a scrollable list.
 * Navigate with the StickS3 controller (UP/DOWN to scroll, A or START to launch).
 * The selected ROM's VFS path is passed directly into nofrendo_main().
 *
 * SD card layout:
 *   /roms/mario.nes
 *   /roms/metroid.nes
 *   /roms/zelda.nes
 *   ... (any .nes files)
 *
 * SD VFS note:
 *   SD.begin() without a custom mountPoint mounts at /sdcard (ESP32 default).
 *   nofrendo calls fopen() on argv[0], so we must pass "/sdcard/roms/xxx.nes".
 *   SD_VFS_PREFIX in rom_selector.h defaults to "/sdcard" to match this.
 *
 *   If you want shorter paths, call SD.begin() with a custom mount point:
 *     SD.begin(SD_CS_PIN, SPI, 40000000, "/sd")
 *   and add before the rom_selector.h include:
 *     #define SD_VFS_PREFIX "/sd"
 */

#include <M5Unified.h>
#include <SD.h>
#include "hw_config.h"

// Override VFS prefix to "/sd" by mounting SD there (shorter paths, same result).
// Remove these two lines to use the default /sdcard mount point instead.
#define SD_VFS_PREFIX "/sd"
#define SD_MOUNT_POINT "/sd"

#include "rom_selector.h"   // ← ROM selector UI (uses SD_VFS_PREFIX above)

extern "C" {
#include <nofrendo.h>
void controller_init();     // forward — safe to call early; guarded against double-init
}

extern void display_begin();

// SD card CS pin for M5Stack CoreS3
#define SD_CS_PIN 4

void setup()
{
    Serial.begin(115200);

    // display_begin() calls M5.begin() internally — must come first
    display_begin();

    // Mount SD card.
    // We use a custom mount point "/sd" so paths are short and predictable.
    // SD_VFS_PREFIX above is set to match.
#ifdef SD_MOUNT_POINT
    if (!SD.begin(SD_CS_PIN, SPI, 40000000, SD_MOUNT_POINT)) {
#else
    if (!SD.begin(SD_CS_PIN)) {
#endif
        Serial.println("SD card mount failed! Check card is inserted.");
        M5.Display.setRotation(3);
        M5.Display.fillScreen(TFT_BLACK);
        M5.Display.setTextColor(TFT_RED, TFT_BLACK);
        M5.Display.setTextSize(2);
        M5.Display.setCursor(20, 100);
        M5.Display.print("SD Mount Failed!");
        M5.Display.setTextSize(1);
        M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Display.setCursor(20, 125);
        M5.Display.print("Check SD card is inserted.");
        while (true) { delay(1000); }
    }
    Serial.println("SD card mounted OK.");

    // Init controller early so the ROM selector can receive input.
    // controller_init() is idempotent — a second call from osd_init() is a no-op.
    controller_init();

    // Run selector — blocks until user picks a ROM.
    // Returns e.g. "/sd/roms/mario.nes"  (VFS path nofrendo can fopen)
    String selected_rom = rom_selector_run();

    // Sanity-check: verify the file is accessible via the VFS path we'll hand to nofrendo.
    // We fopen() directly rather than using SD.open() because the path includes the VFS
    // prefix (/sd/...) which SD.open() doesn't understand (it wants /roms/... relative paths).
    FILE *f = fopen(selected_rom.c_str(), "rb");
    if (!f) {
        Serial.printf("ROM VFS open failed: %s\n", selected_rom.c_str());
        M5.Display.fillScreen(TFT_BLACK);
        M5.Display.setTextColor(TFT_RED, TFT_BLACK);
        M5.Display.setTextSize(1);
        M5.Display.setCursor(8, 90);
        M5.Display.print("ROM open failed:");
        M5.Display.setCursor(8, 106);
        M5.Display.print(selected_rom.c_str());
        M5.Display.setCursor(8, 130);
        M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
        M5.Display.print("Check SD mount point matches");
        M5.Display.setCursor(8, 146);
        M5.Display.print("SD_VFS_PREFIX in rom_selector.h");
        while (true) { delay(1000); }
    }
    // Read file size for the log
    fseek(f, 0, SEEK_END);
    long rom_size = ftell(f);
    fclose(f);
    Serial.printf("ROM confirmed: %ld bytes  →  %s\n", rom_size, selected_rom.c_str());

    // ── Launch emulator ───────────────────────────────────────────────────────
    // nofrendo_main() takes argv[0] as the ROM VFS path and calls fopen() on it.
    static char romPathBuf[256];
    strncpy(romPathBuf, selected_rom.c_str(), sizeof(romPathBuf) - 1);
    romPathBuf[sizeof(romPathBuf) - 1] = '\0';

    char *argv[1] = { romPathBuf };

    Serial.printf("NoFrendo start! ROM: %s\n", romPathBuf);
    int rc = nofrendo_main(1, argv);
    Serial.printf("NoFrendo returned: %d\n", rc);
}

void loop() {}
