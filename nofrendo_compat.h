/*
 * nofrendo_compat.h
 * Forces correct include order: noftypes.h must come before osd.h.
 * Include this instead of <nes/nes.h> or <osd.h> directly.
 */
#pragma once

#include <noftypes.h>
#include <nofconfig.h>
#include <nes/nes.h>
