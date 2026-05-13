#include <limits.h>

extern "C" {
#include <noftypes.h>
#include <nofrendo.h>
#include <nes/nes.h>
}

#include "hw_config.h"
#include <M5Unified.h>

static int16_t frame_width, frame_height;
int16_t bg_color = 0x0000;
extern uint16_t myPalette[];

// NES native resolution — fits perfectly on CoreS3 320x240
// 32px black bars on each side, 1:1 vertical, no scaling
static const int source_view_width  = 256;
static const int source_view_height = 240;

// Line buffer — one row of pixels pre-converted to RGB565
// Allocated in PSRAM so it doesn't eat internal RAM
static uint16_t *line_buf = nullptr;

extern void display_begin()
{
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(3);  // landscape
    M5.Display.fillScreen(TFT_BLACK);
    bg_color = TFT_BLACK;
}

extern "C" void display_init()
{
    frame_width  = M5.Display.width();   // 320
    frame_height = M5.Display.height();  // 240

    // Allocate line buffer in PSRAM
    line_buf = (uint16_t *)heap_caps_malloc(
        source_view_width * sizeof(uint16_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );
    if (!line_buf) {
        // Fall back to internal RAM if PSRAM fails
        line_buf = (uint16_t *)malloc(source_view_width * sizeof(uint16_t));
    }
}

extern "C" void display_write_frame(const uint8_t *data[])
{
    if (!line_buf) return;

    // Center the 256-wide NES frame in the 320-wide display
    const int x_offset = (frame_width - source_view_width) / 2;

    M5.Display.startWrite();
    M5.Display.setAddrWindow(x_offset, 0, source_view_width, frame_height);

    for (int32_t y = 0; y < frame_height; y++)
    {
        // 1:1 vertical — both NES and CoreS3 are 240 lines
        const uint8_t *src_row = data[y];

        // Convert entire row to RGB565 into line buffer
        // pushPixels() needs bytes swapped vs writeColor()
        for (int32_t x = 0; x < source_view_width; x++)
        {
            uint16_t c = myPalette[src_row[x] & 0x3F];
            line_buf[x] = (c >> 8) | (c << 8);  // swap bytes
        }

        // Push entire row in one SPI transaction
        M5.Display.pushPixels(line_buf, source_view_width);
    }

    M5.Display.endWrite();
}

extern "C" void display_clear()
{
    M5.Display.fillScreen(bg_color);
}
