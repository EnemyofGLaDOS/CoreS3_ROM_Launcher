#pragma once
/*
 * rom_selector.h  —  CoreS3 NES ROM Selector
 *
 * Scans /roms/ on the SD card for .nes files, displays a scrollable list
 * on the CoreS3 320x240 screen, and lets the user navigate with the
 * StickS3 ESP-NOW controller (same one used in-game).
 *
 * Controls:
 *   D-Pad Up / Down  →  scroll list
 *   A or Start       →  launch selected ROM
 *
 * Call rom_selector_run() from setup() before nofrendo_main().
 * Returns the full VFS path of the chosen ROM (e.g. "/sdcard/roms/smb.nes").
 * Pass that string as argv[0] to nofrendo_main().
 *
 * SD VFS note:
 *   The ESP32 Arduino SD library mounts at /sdcard by default.
 *   nofrendo calls fopen() on argv[0], so the path must match that mount point.
 *   SD.begin() is called in setup() without a custom mount point, so we use /sdcard.
 *
 * Dependencies: M5Unified, SD, controller_read_input() from controller.cpp
 */

#include <M5Unified.h>
#include <SD.h>
#include <Arduino.h>
#include <vector>
#include <string>
#include <algorithm>

// ── Controller bit masks (active LOW — matches controller.cpp) ───────────────
#define BTN_UP     (1 << 0)
#define BTN_DOWN   (1 << 1)
#define BTN_LEFT   (1 << 2)
#define BTN_RIGHT  (1 << 3)
#define BTN_SELECT (1 << 4)
#define BTN_START  (1 << 5)
#define BTN_A      (1 << 6)
#define BTN_B      (1 << 7)

// Active LOW: button pressed = bit is 0
#define BTN_PRESSED(state, mask) (((state) & (mask)) == 0)

extern "C" uint32_t controller_read_input();

// ── Layout constants ──────────────────────────────────────────────────────────
static const int RS_SCREEN_W     = 320;
static const int RS_SCREEN_H     = 240;
static const int RS_HEADER_H     = 36;   // height of title bar
static const int RS_FOOTER_H     = 24;   // height of hint bar at bottom
static const int RS_ROW_H        = 22;   // height of each ROM row
static const int RS_LIST_Y       = RS_HEADER_H + 4;
static const int RS_LIST_H       = RS_SCREEN_H - RS_HEADER_H - RS_FOOTER_H - 8;
static const int RS_VISIBLE_ROWS = RS_LIST_H / RS_ROW_H;  // ~8 rows

// ── Colours (RGB565) ──────────────────────────────────────────────────────────
static const uint16_t COL_BG         = 0x0000;  // black background
static const uint16_t COL_HEADER_BG  = 0xA800;  // dark red
static const uint16_t COL_HEADER_FG  = 0xFFFF;  // white
static const uint16_t COL_FOOTER_BG  = 0x2104;  // very dark grey
static const uint16_t COL_FOOTER_FG  = 0xC618;  // light grey
static const uint16_t COL_SEL_BG     = 0x07FF;  // cyan highlight
static const uint16_t COL_SEL_FG     = 0x0000;  // black text on highlight
static const uint16_t COL_ROW_FG     = 0xFFFF;  // white text
static const uint16_t COL_ALT_BG     = 0x1082;  // very dark grey alternate row
static const uint16_t COL_SCROLLBAR  = 0x4228;  // dark grey scrollbar track
static const uint16_t COL_SCROLLTHM  = 0x07FF;  // cyan scrollbar thumb

// ── Scrollbar geometry ────────────────────────────────────────────────────────
static const int RS_SCROLL_W  = 6;
static const int RS_SCROLL_X  = RS_SCREEN_W - RS_SCROLL_W - 2;
static const int RS_SCROLL_Y  = RS_LIST_Y;
static const int RS_SCROLL_H  = RS_LIST_H;

// ── Helpers ───────────────────────────────────────────────────────────────────

// Strip directory path, return just the filename without extension
static String rs_basename_no_ext(const String &path) {
    int slash = path.lastIndexOf('/');
    String name = (slash >= 0) ? path.substring(slash + 1) : path;
    int dot = name.lastIndexOf('.');
    if (dot > 0) name = name.substring(0, dot);
    return name;
}

// Scan /roms/ directory on SD for .nes files, return sorted list of full paths.
//
// FIX: entry.name() behaviour differs across ESP32 Arduino SD lib versions:
//   - Old (1.x): returns just the filename  → "mario.nes"
//   - New (2.x): returns the full path      → "/roms/mario.nes"
// We normalise both cases into a canonical "/roms/<filename>" path using the
// ORIGINAL (non-lowercased) name so that fopen() succeeds on a case-sensitive FS.
static std::vector<String> rs_scan_roms() {
    std::vector<String> roms;
    File dir = SD.open("/roms");
    if (!dir || !dir.isDirectory()) {
        Serial.println("rom_selector: /roms directory not found on SD card");
        return roms;
    }

    File entry;
    while ((entry = dir.openNextFile())) {
        if (!entry.isDirectory()) {
            // Get the raw name as returned by the library (may or may not include path)
            String rawName = String(entry.name());

            // Isolate just the filename portion (handles both old and new SD lib)
            int lastSlash = rawName.lastIndexOf('/');
            String fileName = (lastSlash >= 0) ? rawName.substring(lastSlash + 1) : rawName;

            // Check extension on a lowercase copy — but build the path from the ORIGINAL
            String fileNameLower = fileName;
            fileNameLower.toLowerCase();

            if (fileNameLower.endsWith(".nes")) {
                // Always build a clean, canonical path from just the filename
                String fullPath = "/roms/" + fileName;
                roms.push_back(fullPath);
                Serial.printf("rom_selector: found %s\n", fullPath.c_str());
            }
        }
        entry.close();
    }
    dir.close();

    // Sort alphabetically (case-insensitive)
    std::sort(roms.begin(), roms.end(), [](const String &a, const String &b) {
        String al = a; al.toLowerCase();
        String bl = b; bl.toLowerCase();
        return al < bl;
    });

    return roms;
}

// Draw the full selector UI
static void rs_draw(const std::vector<String> &roms, int selected, int scroll_top) {
    auto &disp = M5.Display;

    // ── Header ────────────────────────────────────────────────────────────────
    disp.fillRect(0, 0, RS_SCREEN_W, RS_HEADER_H, COL_HEADER_BG);
    disp.setTextColor(COL_HEADER_FG, COL_HEADER_BG);
    disp.setTextSize(2);
    disp.setCursor(8, 10);
    disp.print("NES ROMs");

   // ROM count and battery top-right
    disp.setTextSize(1);
    int bat = M5.Power.getBatteryLevel();  // 0-100
    char countbuf[32];
    snprintf(countbuf, sizeof(countbuf), "%d ROMs  BAT:%d%%", (int)roms.size(), bat);
    disp.setCursor(RS_SCREEN_W - strlen(countbuf) * 6 - 10, 14);
    disp.print(countbuf);

    // ── Background ────────────────────────────────────────────────────────────
    disp.fillRect(0, RS_HEADER_H, RS_SCREEN_W, RS_SCREEN_H - RS_HEADER_H - RS_FOOTER_H, COL_BG);

    // ── ROM list ──────────────────────────────────────────────────────────────
    for (int i = 0; i < RS_VISIBLE_ROWS; i++) {
        int rom_idx = scroll_top + i;
        int row_y   = RS_LIST_Y + i * RS_ROW_H;

        if (rom_idx >= (int)roms.size()) {
            // Empty row
            disp.fillRect(0, row_y, RS_SCROLL_X - 1, RS_ROW_H, COL_BG);
            continue;
        }

        bool is_selected = (rom_idx == selected);
        uint16_t row_bg = is_selected ? COL_SEL_BG : (i % 2 == 0 ? COL_BG : COL_ALT_BG);
        uint16_t row_fg = is_selected ? COL_SEL_FG : COL_ROW_FG;

        disp.fillRect(0, row_y, RS_SCROLL_X - 1, RS_ROW_H, row_bg);

        // Truncate name to fit within scrollbar boundary
        String display_name = rs_basename_no_ext(roms[rom_idx]);
        // Max chars at textSize(1) = 6px wide, 4px left margin, 10px for arrow
        int max_chars = (RS_SCROLL_X - 1 - 18) / 6;
        if ((int)display_name.length() > max_chars)
            display_name = display_name.substring(0, max_chars - 2) + "..";

        disp.setTextColor(row_fg, row_bg);
        disp.setTextSize(1);
        // Vertically center text in row (row_h=22, char_h=8 → offset=7)
        disp.setCursor(8, row_y + (RS_ROW_H - 8) / 2);
        disp.print(display_name);

        // Selection arrow
        if (is_selected) {
            disp.setCursor(RS_SCROLL_X - 10, row_y + (RS_ROW_H - 8) / 2);
            disp.print(">");
        }
    }

    // ── Scrollbar ─────────────────────────────────────────────────────────────
    disp.fillRect(RS_SCROLL_X, RS_SCROLL_Y, RS_SCROLL_W, RS_SCROLL_H, COL_SCROLLBAR);
    if (!roms.empty()) {
        int total   = (int)roms.size();
        int thumb_h = max(12, RS_SCROLL_H * RS_VISIBLE_ROWS / total);
        int thumb_y = RS_SCROLL_Y + (RS_SCROLL_H - thumb_h) * scroll_top / max(1, total - RS_VISIBLE_ROWS);
        disp.fillRect(RS_SCROLL_X, thumb_y, RS_SCROLL_W, thumb_h, COL_SCROLLTHM);
    }

    // ── Footer ────────────────────────────────────────────────────────────────
    int footer_y = RS_SCREEN_H - RS_FOOTER_H;
    disp.fillRect(0, footer_y, RS_SCREEN_W, RS_FOOTER_H, COL_FOOTER_BG);
    disp.setTextColor(COL_FOOTER_FG, COL_FOOTER_BG);
    disp.setTextSize(1);
    disp.setCursor(8, footer_y + (RS_FOOTER_H - 8) / 2);
    disp.print("UP/DOWN: scroll    A or START: launch");
}

// Draw a "launching..." overlay
static void rs_draw_launching(const String &rom_path) {
    auto &disp = M5.Display;
    String name = rs_basename_no_ext(rom_path);

    disp.fillRect(20, 90, 280, 60, 0x07FF);  // cyan box
    disp.setTextColor(0x0000, 0x07FF);
    disp.setTextSize(2);
    disp.setCursor(30, 100);
    disp.print("Loading...");
    disp.setTextSize(1);
    int max_chars = (280 - 10) / 6;
    if ((int)name.length() > max_chars) name = name.substring(0, max_chars - 2) + "..";
    disp.setCursor(30, 122);
    disp.print(name);
}

// ── Main entry point ──────────────────────────────────────────────────────────
/*
 * Blocks until the user selects a ROM, then returns the full VFS path.
 *
 * VFS path format: "/sdcard/roms/mario.nes"
 *   - The ESP32 Arduino SD library mounts the SD card at /sdcard by default.
 *   - nofrendo calls fopen(argv[0]) directly, so the path must use /sdcard.
 *   - If you called SD.begin(SD_CS_PIN, SPI, 40000000, "/sd") to override the
 *     mount point to /sd, change SD_VFS_PREFIX below to "/sd".
 *
 * To override: #define SD_VFS_PREFIX "/sd"  before including this header.
 */
#ifndef SD_VFS_PREFIX
  #define SD_VFS_PREFIX "/sdcard"
#endif

static String rom_selector_run() {
    auto &disp = M5.Display;
    disp.setRotation(3);

    std::vector<String> roms = rs_scan_roms();

    // ── Error state: no ROMs found ────────────────────────────────────────────
    if (roms.empty()) {
        disp.fillScreen(COL_BG);
        disp.setTextColor(0xF800, COL_BG);  // red
        disp.setTextSize(2);
        disp.setCursor(20, 80);
        disp.print("No ROMs found!");
        disp.setTextSize(1);
        disp.setTextColor(COL_ROW_FG, COL_BG);
        disp.setCursor(20, 110);
        disp.print("Copy .nes files to /roms/");
        disp.setCursor(20, 125);
        disp.print("on your SD card and reboot.");
        Serial.println("rom_selector: no .nes files found in /roms/");
        while (true) { delay(1000); }  // halt — nothing to do
    }

    int selected   = 0;
    int scroll_top = 0;

    // Initial draw
    rs_draw(roms, selected, scroll_top);

    // ── Input debounce state ──────────────────────────────────────────────────
    uint32_t last_input           = 0xFFFFFFFF;  // neutral (active LOW)
    uint32_t last_move_ms         = 0;
    const uint32_t REPEAT_DELAY_MS = 400;
    const uint32_t REPEAT_RATE_MS  = 120;
    bool repeating = false;

    while (true) {
        uint32_t now   = millis();
        uint32_t input = controller_read_input();

        bool up_held   = BTN_PRESSED(input, BTN_UP);
        bool down_held = BTN_PRESSED(input, BTN_DOWN);
        bool confirm   = BTN_PRESSED(input, BTN_A) || BTN_PRESSED(input, BTN_START);

        bool up_edge      = up_held   && !BTN_PRESSED(last_input, BTN_UP);
        bool down_edge    = down_held && !BTN_PRESSED(last_input, BTN_DOWN);
        bool confirm_edge = confirm   && !(BTN_PRESSED(last_input, BTN_A) || BTN_PRESSED(last_input, BTN_START));

        bool moved = false;

        // ── Navigation with auto-repeat ───────────────────────────────────────
        if (up_edge || down_edge) {
            moved     = true;
            repeating = false;
            last_move_ms = now;
            if (up_edge)   selected--;
            if (down_edge) selected++;
        } else if ((up_held || down_held) && !confirm) {
            uint32_t elapsed   = now - last_move_ms;
            uint32_t threshold = repeating ? REPEAT_RATE_MS : REPEAT_DELAY_MS;
            if (elapsed >= threshold) {
                moved        = true;
                repeating    = true;
                last_move_ms = now;
                if (up_held)   selected--;
                if (down_held) selected++;
            }
        } else {
            repeating = false;
        }

        // Clamp selection
        if (selected < 0) selected = 0;
        if (selected >= (int)roms.size()) selected = (int)roms.size() - 1;

        // Scroll to keep selection visible
        if (selected < scroll_top)
            scroll_top = selected;
        if (selected >= scroll_top + RS_VISIBLE_ROWS)
            scroll_top = selected - RS_VISIBLE_ROWS + 1;

        if (moved) rs_draw(roms, selected, scroll_top);

        // ── Launch ────────────────────────────────────────────────────────────
        if (confirm_edge) {
            rs_draw_launching(roms[selected]);
            delay(600);

            // Build the VFS path nofrendo's fopen() expects
            String vfs_path = String(SD_VFS_PREFIX) + roms[selected];
            Serial.printf("rom_selector: launching %s\n", vfs_path.c_str());
            return vfs_path;
        }

        last_input = input;
        delay(16);  // ~60 Hz poll rate
    }
}
