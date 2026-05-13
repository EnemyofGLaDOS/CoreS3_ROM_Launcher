# CoreS3_ROM_Launcher
CoreS3 ROM Launcher is a scrollable NES ROM selector for the M5Stack CoreS3, controlled via an M5StickS3 over ESP-NOW.

Features
ROM selector UI — scrollable list of .nes files loaded from the SD card's /roms/ directory, with alternating row colors, a cyan scrollbar, and battery percentage displayed in the header Wireless controller — navigates the menu and plays games via ESP-NOW from a StickS3; no wires, no pairing steps Auto-repeat scrolling — hold up/down on the d-pad to scroll quickly through large ROM libraries Auto-launch — select a ROM and it loads instantly into the nofrendo NES emulator Timeout safety — if the controller goes out of range mid-game, all buttons release automatically so gameplay doesn't get stuck
Hardware Required ComponentNotesM5Stack CoreS3Main unit — (display), SD card, M5StickS3 (Controller) — transmits button state over ESP-NOWMicroSD cardFAT32 formatted, .nes ROMs in /roms/ folder
SD Card Setup /roms/ supermario.nes metroid.nes zelda.nes ... Any .nes file dropped in /roms/ will appear in the launcher automatically.
Controls InputActionD-Pad Up / DownScroll ROM listA or StartLaunch selected ROMD-Pad (in-game)MovementA / B (in-game)NES A / B buttonsSelect / Start (in-game)NES Select / Start
Building & Flashing
Install Arduino IDE 2.x with the M5Stack ESP32 board package Install libraries: M5Unified, arduino-nofrendo Apply the patches in library_patches/ to the nofrendo library source (see below) Open CoreS3_NES_GameLauncher.ino and flash to the CoreS3 Flash the StickS3 controller firmware separately
Based On
arduino-nofrendo — NES emulator port by moononournation Original Nofrendo emulator by Matthew Conte
This firmware is for personal use only. You are responsible for ensuring you own a legitimate copy of your own roms.

