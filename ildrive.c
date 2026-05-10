// ------------------------------------------------------------------------------
// ILPER 1.35 for Linux
// Copyright (c) 2008-2009  J-F Garnier
// Copyright (c) 2011-2012  Ch. Gottheimer
// Copyright (c) 2026       J. Taillandier
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// ------------------------------------------------------------------------------
//
// ildrive.c    Disk drive module
// (used in Emu41/Emu71/ILper)
//
// Based on previous personal work:
// 1986: ILPER4 video+disc, (6502 assembler)
// 1988: ILPER5 discil module
// 1993: ILPER ported on PC (8086 assembler)
// 1997: rewriten in C and included in Emu41
// 2008: rewriten in VB for the standalone ILPER Windows version using the PILBox!
// Oct 2009: bug fix in copybuf() and exchbuf() (VB issue...)
// 2011: ported on Linux by Ch. Gottheimer
// 2026: ported to macOS and thoroughly revised by J. Taillandier
// ------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/fcntl.h>

#include "ilper.h"

#define AID 16 // Accessory ID = mass storage

// 640KB floppy geometry descriptor (80 tracks, 2 sides, 16 sect/track)
static const unsigned char lif_desc[] = {0, 0, 0, 80, 0, 0, 0, 2, 0, 0, 0, 16};
static const int nbe = 2560; // number of 256-byte sectors

// drive_t embeds hpil_dev_t as its FIRST field so that (drive_t *) and
// (hpil_dev_t *) can be freely cast between each other.
typedef struct {
    hpil_dev_t base; // HP-IL protocol state — MUST remain first
    int status;      // HP-IL drive status byte
    int devl, devt;  // listener & talker command codes (DDL/DDT)
    int oc;          // byte pointer within current sector
    int pe;          // current sector number
    int pe0;         // sector pointer accumulator (for seek command)
    int fpt;         // flag: pointer byte count
    int flpwr;       // flag: partial-write mode
    int ptout;       // byte counter for multi-byte output sequences
    char buf0[256];
    char buf1[256];
    char *hdiscfile; // path to disk image file (NULL = drive disabled)
} drive_t;

static drive_t drive1;
static drive_t drive2;

int ilhdisc1_address(void)
{
    return hpil_dev_address(&drive1.base);
}
int ilhdisc2_address(void)
{
    return hpil_dev_address(&drive2.base);
}
int ilhdisc1_active(void)
{
    return drive1.hdiscfile != NULL;
}
int ilhdisc2_active(void)
{
    return drive2.hdiscfile != NULL;
}

// *****************************************************************
// hardware access functions
// *****************************************************************

// ******************************************
// rrec(d)
//
// read one sector no pe (256 bytes) into buf0
// ******************************************
static void rrec(drive_t *d)
{
    int fs;

    if (0 == access(d->hdiscfile, R_OK)) {
        if (-1 != (fs = open(d->hdiscfile, O_RDONLY))) {
            if ((-1 == lseek(fs, d->pe * 256, SEEK_SET)) ||
                (sizeof(d->buf0) != read(fs, d->buf0, sizeof(d->buf0)))) {
                memset(d->buf0, 255, sizeof(d->buf0));
            }
            close(fs);
        } else {
            d->status = 20;
        }
    }
}

// ******************************************
// wrec(d)
//
// write buf0 to one sector no pe (256 bytes)
// ******************************************
static void wrec(drive_t *d)
{
    int fs;

    if (0 == access(d->hdiscfile, W_OK)) {
        if (-1 != (fs = open(d->hdiscfile, O_WRONLY))) {
            if ((-1 == lseek(fs, d->pe * 256, SEEK_SET)) ||
                (sizeof(d->buf0) != write(fs, d->buf0, sizeof(d->buf0)))) {
                d->status = 24;
            }
            close(fs);
        } else {
            d->status = 29;
        }
    }
}

// ******************************************
// format_disc(d)
//
// "format" a LIF image file
// ******************************************
static void format_disc(drive_t *d)
{
    char buf2[256];
    int fs;

    d->status = 0;
    memset(buf2, 255, sizeof(buf2));

    if (-1 != (fs = open(d->hdiscfile, O_WRONLY | O_TRUNC | O_CREAT, 0644))) {
        int i;

        for (i = 0; i < nbe; i++)
            if (sizeof(buf2) != write(fs, buf2, sizeof(buf2))) {
                d->status = 29;
                break;
            }
        close(fs);
    }
}

// ******************************************
// clrdrv(d)
//
// Clear drive, reset internal pointers
// ******************************************
static void clrdrv(drive_t *d)
{
    d->fpt = 0;
    d->pe = 0;
    d->oc = 0;
}

// ******************************************
// indata(d, frame)
//
// receive one HP-IL frame as listener;
// frame includes the END bit (bit 9 = 0x200)
// ******************************************
static void indata(drive_t *d, int frame)
{
    int n = frame; // keep full frame so bit 9 (END) is accessible

    switch (d->devl) {
    case 0:
    case 2:
    case 6:
        d->buf0[d->oc] = n & 255;
        d->oc = d->oc + 1;
        if (d->oc > 255) {
            d->oc = 0;
            wrec(d);
            d->pe = d->pe + 1;
            if (d->flpwr != 0) {
                rrec(d);
            }
        } else {
            if (n & 0x200) {
                // END
                wrec(d);
                if (d->flpwr == 0) {
                    d->pe = d->pe + 1;
                }
            }
        }
        break;
    case 1:
        d->buf1[d->oc] = n & 255;
        d->oc = d->oc + 1;
        if (d->oc > 255)
            d->oc = 0;
        break;
    case 3:
        d->oc = n & 255;
        break;
    case 4:
        n = n & 255;
        if (d->fpt != 0) {
            d->pe0 = d->pe0 & 0xFF00;
            d->pe0 = d->pe0 | n;
            if (d->pe0 < nbe) {
                d->pe = d->pe0;
                d->status = 0;
            } else {
                d->status = 28;
            }
            d->fpt = 0;
        } else {
            d->pe0 = d->pe0 & 255;
            d->pe0 = d->pe0 | (n << 8);
            d->fpt = d->fpt - 1;
        }
    }
}

// ******************************************
// outdta(d)
//
// send one data byte from disc according to DDT cmd;
// returns 0-255 for data or ETO when done.
// The caller (drive_on_send) converts ETO → -1 for the base engine.
// ******************************************
static int outdta(drive_t *d)
{
    int frame;

    switch (d->devt) {
    case 0:
    case 2:
        frame = (int)d->buf0[d->oc] & 255;
        d->oc = d->oc + 1;
        if (d->oc > 255) {
            d->oc = 0;
            rrec(d);
            d->pe = d->pe + 1;
        }
        break;
    case 1:
        frame = (int)d->buf1[d->oc] & 255;
        d->oc = d->oc + 1;
        if (d->oc > 255) {
            d->oc = 0;
            d->devt = 15;
        }
        break;
    case 3:
        switch (d->ptout) {
        case 0:
            frame = d->pe >> 8;
            break;
        case 1:
            frame = d->pe & 255;
            break;
        case 2:
            frame = d->oc & 255;
            break;
        default:
            frame = ETO;
        }
        d->ptout = d->ptout + 1;
        break;
    case 6:
        if (d->ptout < 12) {
            frame = lif_desc[d->ptout];
            d->ptout = d->ptout + 1;
        } else {
            frame = ETO;
        }
        break;
    case 7:
        switch (d->ptout) {
        case 0:
            frame = nbe >> 8;
            break;
        case 1:
            frame = nbe & 255;
            break;
        default:
            frame = ETO;
        }
        d->ptout = d->ptout + 1;
        break;
    default:
        frame = ETO;
    }

    return frame;
}

// ******************************************
// copybuf(d)
//
// copy buffer 0 to buffer 1
// ******************************************
static void copybuf(drive_t *d)
{
    d->oc = 0;
    memcpy(d->buf1, d->buf0, sizeof(d->buf1));
}

// ******************************************
// exchbuf(d)
//
// exchange buffers 0 and 1
// ******************************************
static void exchbuf(drive_t *d)
{
    char tmp[256];

    d->oc = 0;
    memcpy(tmp, d->buf1, sizeof(tmp));
    memcpy(d->buf1, d->buf0, sizeof(d->buf1));
    memcpy(d->buf0, tmp, sizeof(d->buf0));
}

// --------------------------------------------------------------------------
// HP-IL protocol: delegate to hpil_dev_t base via callbacks
// --------------------------------------------------------------------------

static void drive_on_receive(hpil_dev_t *dev, int frame)
{
    indata((drive_t *)dev, frame);
}

static int drive_on_send(hpil_dev_t *dev)
{
    int frame = outdta((drive_t *)dev);
    return (frame == ETO) ? -1 : frame;
}

static int drive_on_status(hpil_dev_t *dev)
{
    drive_t *d = (drive_t *)dev;
    int s = d->status;
    d->status &= 0xE0; // clear error bits after reading
    return s;
}

static void drive_on_clear(hpil_dev_t *dev)
{
    clrdrv((drive_t *)dev);
}

static int drive_on_cmd(hpil_dev_t *dev, int frame)
{
    drive_t *d = (drive_t *)dev;
    int n = frame & 0xFF;

    switch (n >> 5) {
    case 5: // DDL
        n &= 31;
        if ((dev->state & 0xC0) == 0x80) { // listener only
            d->devl = n;
            switch (n) {
            case 1:
                d->flpwr = 0;
                break;
            case 2:
                d->oc = 0;
                d->flpwr = 0;
                break;
            case 4:
                d->flpwr = 0;
                d->fpt = 0;
                break;
            case 5:
                format_disc(d);
                break;
            case 6:
                d->flpwr = 0x80;
                rrec(d);
                break;
            case 7:
                d->fpt = 0;
                d->pe = 0;
                d->oc = 0;
                break;
            case 8:
                wrec(d);
                if (d->flpwr == 0)
                    d->pe = d->pe + 1;
                break;
            case 9:
                copybuf(d);
                break;
            case 10:
                exchbuf(d);
                break;
            }
        }
        break;
    case 6: // DDT
        n &= 31;
        if (dev->state & 0x40) { // talker only
            d->devt = n;
            d->ptout = 0;
            switch (n) {
            case 0:
                d->flpwr = 0;
                break;
            case 2:
                rrec(d);
                d->oc = 0;
                d->flpwr = 0;
                d->pe = d->pe + 1;
                break;
            case 4:
                exchbuf(d);
                break;
            }
        }
        break;
    }
    return frame;
}

// ******************************************
// init_drive(d, defaddr, device_id, filename)
//
// init one drive instance
// ******************************************
static void init_drive(drive_t *d, int defaddr, const char *device_id,
                       char *filename)
{
    d->base.device_id = device_id;
    d->base.defaddr = defaddr;
    d->base.aid = AID;
    d->base.on_receive = drive_on_receive;
    d->base.on_send = drive_on_send;
    d->base.on_status = drive_on_status;
    d->base.on_clear = drive_on_clear;
    d->base.on_cmd = drive_on_cmd;
    hpil_dev_init(&d->base);

    d->hdiscfile = (filename && *filename) ? filename : NULL;
    d->status = 0;
}

// ******************************************
// init_ilhdisc(file1, file2)
//
// init both virtual HPIL drive devices
// ******************************************
void init_ilhdisc(char *file1, char *file2)
{
    init_drive(&drive1, 2, "HDRIVE1\r", file1);
    init_drive(&drive2, 3, "HDRIVE2\r", file2);
}

// ******************************************
// ILhdisc1/2(frame)
//
// manage one HP-IL frame for the drive
// ******************************************
int ILhdisc1(int frame)
{
    return hpil_dev_process(&drive1.base, frame);
}
int ILhdisc2(int frame)
{
    return hpil_dev_process(&drive2.base, frame);
}
