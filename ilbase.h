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
// ilbase.h   HP-IL device base: common protocol state machine
//
// All virtual HP-IL devices share the same protocol (addressing, talker/
// listener state machine, SDI/SST/SAI/SDA/NRD/AAD/AAU/IFC/IDY SRQ).
// This module implements that common layer.  Each device provides callbacks
// for the operations that are specific to it.
// ------------------------------------------------------------------------------

#ifndef ILBASE_H
#define ILBASE_H

#define ETO 0x540 // End Transfer Out (HP-IL RDY frame)

typedef struct hpil_dev hpil_dev_t;

// HP-IL device base structure.
// Embed as the FIRST field of every device struct so that (device_t *) and
// (hpil_dev_t *) can be freely cast between each other.
struct hpil_dev {
    // identity — set once at device initialisation, never changed
    const char *device_id; // e.g. "DISPLAY\r"
    int defaddr;           // fixed HP-IL address assigned by AAU
    int aid;               // Accessory ID returned by SAI

    // HP-IL protocol state — managed exclusively by hpil_dev_process()
    // state bits:  bit7=addressed  bit6=talker  bit1=SDA mode  bit0=SDI mode
    int adr;   // current address (0 = use defaddr; bit7 = AAD address taken)
    int state; // combined state + mode flags
    int ptsdi; // SDI output pointer (1-based index into device_id[])
    int srq;   // non-zero → set SRQ bit (bit 0) in every IDY frame

    // Device callbacks — all optional (NULL = no-op / default behaviour)
    //   on_receive : called for each data byte received as listener
    //                frame is the raw HP-IL frame value (bit 9 = END flag)
    void (*on_receive)(hpil_dev_t *, int frame);
    //   on_send    : called each time the device must emit a data byte
    //                return value 0..255 = byte to send; -1 = no more data (→ ETO)
    int (*on_send)(hpil_dev_t *);
    //   on_status  : called for SST; return the first status byte (always 0x80
    //                for system status). If on_status2 is non-NULL, a second
    //                byte is sent afterwards.
    int (*on_status)(hpil_dev_t *);
    //   on_status2 : optional; return the second SST byte. Set on_status2 to
    //                non-NULL only when two-byte status is required.
    int (*on_status2)(hpil_dev_t *);
    //   on_clear   : called for SDC (if addressed) and DCL
    void (*on_clear)(hpil_dev_t *);
    //   on_cmd     : called for CMD frames not handled by the base
    //                (DDL, DDT, and any unrecognised group)
    //                must return the (possibly modified) frame
    int (*on_cmd)(hpil_dev_t *, int frame);
};

// Initialise a device: set adr=defaddr, state=0, ptsdi=0, srq=0.
// Callbacks and identity fields must already be set before calling this.
void hpil_dev_init(hpil_dev_t *dev);

// Process one HP-IL frame through the common protocol engine.
// Returns the (possibly modified) frame to be forwarded on the loop.
int hpil_dev_process(hpil_dev_t *dev, int frame);

// Return the current 5-bit HP-IL address, or -1 if not yet addressed.
int hpil_dev_address(hpil_dev_t *dev);

#endif // ILBASE_H
