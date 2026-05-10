// ------------------------------------------------------------------------------
// ILPER 2.0 for Linux/macOS
// Copyright (c) 2026       J. Taillandier
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// ------------------------------------------------------------------------------
//
// ilbase.c   HP-IL device base: common protocol state machine
//
// Implements the addressing, talker/listener state machine, and SRQ logic
// shared by all virtual HP-IL devices.  Device-specific behaviour is
// provided through callbacks in hpil_dev_t.
// ------------------------------------------------------------------------------

#include "ilbase.h"

void hpil_dev_init(hpil_dev_t *dev)
{
    dev->adr = dev->defaddr;
    dev->state = 0;
    dev->ptsdi = 0;
    dev->srq = 0;
}

int hpil_dev_address(hpil_dev_t *dev)
{
    return dev->adr ? (dev->adr & 0x1F) : -1;
}

// DOE frames (data): DAB / DAB(SRQ) / END / END(SRQ)
static int process_doe(hpil_dev_t *dev, int frame)
{
    if (!(dev->state & 0x80))
        return frame; // not addressed

    if (dev->state & 0x40) {
        // active talker
        if (dev->state & 0x02) {
            if (dev->state & 0x01) {
                // SDI: emit successive characters from device_id
                if (dev->ptsdi > 0) {
                    char c = dev->device_id[dev->ptsdi - 1];
                    frame = (int)(unsigned char)c;
                    if (c == '\r')
                        dev->ptsdi = 0;
                    else
                        dev->ptsdi++;
                }
                if (dev->ptsdi == 0) {
                    frame = ETO;
                    dev->state = 0x40;
                }
            } else {
                // SDA: emit data bytes via on_send
                int byte = dev->on_send ? dev->on_send(dev) : -1;
                if (byte >= 0) {
                    frame = byte;
                } else {
                    frame = ETO;
                    dev->state = 0x40;
                }
            }
        } else if (dev->state & 0x04) {
            // SST second byte
            frame = dev->on_status2(dev);
            dev->state &= ~0x04; // → 0xC0; ETO follows on next DOE
        } else {
            // SAI or single-byte SST done: send ETO
            frame = ETO;
            dev->state = 0x40;
        }
    } else {
        // listener: deliver full frame (on_receive may need the END bit)
        if (dev->on_receive)
            dev->on_receive(dev, frame);
    }
    return frame;
}

// CMD frames (0x400–0x4FF)
static int process_cmd(hpil_dev_t *dev, int frame)
{
    int n = frame & 0xFF;

    switch (n >> 5) {
    case 0:
        if (n == 4) { // SDC: selective device clear (addressed only)
            if ((dev->state & 0x80) && dev->on_clear)
                dev->on_clear(dev);
        } else if (n == 20) { // DCL: device clear (all)
            if (dev->on_clear)
                dev->on_clear(dev);
        }
        break;
    case 1: // LAD / UNL
        n &= 31;
        if (n == 31) { // UNL: unlisten
            if (!(dev->state & 0x40))
                dev->state = 0;
        } else if (n == (dev->adr & 31)) {
            dev->state = 0x80;
        }
        break;
    case 2: // TAD / UNT
        n &= 31;
        if (n == (dev->adr & 31)) {
            dev->state = 0x40;
        } else if (dev->state & 0x40) {
            dev->state = 0;
        }
        break;
    case 4:
        n &= 31;
        if (n == 16) { // IFC: interface clear
            dev->state = 0;
        } else if (n == 26) { // AAU: assign fixed default address
            dev->adr = dev->defaddr;
        } else if (dev->on_cmd) {
            frame = dev->on_cmd(dev, frame);
        }
        break;
    default: // DDL (5), DDT (6), SAD (3), unrecognised (7)
        if (dev->on_cmd)
            frame = dev->on_cmd(dev, frame);
        break;
    }
    return frame;
}

// RDY frames (0x500–0x5FF)
static int process_rdy(hpil_dev_t *dev, int frame)
{
    int n = frame & 0xFF;

    if (n <= 127) {
        if (!(dev->state & 0x40))
            return frame; // not a talker — pass through unchanged
        if (n == 66) {    // NRD: abort transfer
            dev->ptsdi = 0;
            dev->state &= ~0x02;
        } else if (n == 96) { // SDA: send data
            int byte = dev->on_send ? dev->on_send(dev) : -1;
            if (byte >= 0) {
                frame = byte;
                dev->state = 0xC2;
            } else {
                frame = ETO;
                dev->state = 0x40;
            }
        } else if (n == 97) { // SST: send status byte(s)
            frame = dev->on_status ? dev->on_status(dev) : 0;
            dev->state = dev->on_status2 ? 0xC4 : 0xC0;
        } else if (n == 98) { // SDI: send device ID
            frame = (int)(unsigned char)dev->device_id[0];
            dev->ptsdi = 2;
            dev->state = 0xC3;
        } else if (n == 99) { // SAI: send accessory ID
            frame = dev->aid;
            dev->state = 0xC0;
        }
    } else if (n < 0x9E) {
        // AAD: claim next sequential address if not yet AAD-assigned
        if (!(dev->adr & 0x80)) {
            dev->adr = n;
            frame = frame + 1;
        }
    }
    return frame;
}

int hpil_dev_process(hpil_dev_t *dev, int frame)
{
    if ((frame & 0x400) == 0) {
        frame = process_doe(dev, frame);
    } else if ((frame & 0x700) == 0x400) {
        frame = process_cmd(dev, frame);
    } else if ((frame & 0x700) == 0x500) {
        frame = process_rdy(dev, frame);
    } else if ((frame & 0x700) == 0x600) {
        // IDY: set SRQ bit when device has data pending
        if (dev->srq)
            frame |= 0x100;
    }
    return frame;
}
