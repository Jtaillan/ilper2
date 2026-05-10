// ------------------------------------------------------------------------------
// ILPER 2.0 for Linux/macOS
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
// ilprint.c   HP-IL printer module
//
// Emulates a simple HP-IL printer (AID=0x22, 80 columns, HP Escape sequences, no graphics).
// All received data bytes are logged to a configurable file.
//
// 2020: Ported to macOS and thoroughly revised (new file)
// ------------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>

#include "ilper.h"

// HP-IL device identity
#define AID 0x22  // Accessory ID = printer (80 col, HP Escape sequences, no graphics)
#define DEFADDR 4 // default address after AAU

static FILE *fprinter = NULL; // log file for printer output

int ilprint_active(void)
{
    return fprinter != NULL;
}

// ******************************************
// print_char(n)
//
// write one data byte to printer.log,
// converting HP41 special characters
// ******************************************
static void print_char(int n)
{
    static int last_cr = 0; // 1 after a CR, to absorb CR+LF as one newline
    char mb[4 + 1];
    int mblen;

    if (!fprinter)
        return;

    // line endings: CR+LF → single newline
    if (n == 13) {
        fputc('\n', fprinter);
        last_cr = 1;
        return;
    }
    if (n == 10) {
        if (!last_cr)
            fputc('\n', fprinter);
        last_cr = 0;
        return;
    }
    last_cr = 0;

    if (n < 32 || n == 127)
        return; // control codes: ignored

    // HP Roman-8 extended (>= 128): Unicode → UTF-8
    if (n >= 128) {
        if (roman8_table[n] == L' ')
            return; // not mapped: skip
        mblen = roman8_to_utf8(roman8_table[n], mb);
        if (mblen > 0)
            fwrite(mb, 1, mblen, fprinter);
        return;
    }

    // printable ASCII (32-127)
    fputc(n, fprinter);
}

// --------------------------------------------------------------------------
// HP-IL protocol: delegate to hpil_dev_t base via callbacks
// --------------------------------------------------------------------------

static void print_on_receive(hpil_dev_t *dev, int frame)
{
    (void)dev;
    print_char(frame & 0xFF);
}

static void print_on_clear(hpil_dev_t *dev)
{
    (void)dev;
    if (fprinter)
        fputc('\n', fprinter);
}

static hpil_dev_t print_dev = {
    .device_id = "PRINTER\r",
    .defaddr = DEFADDR,
    .aid = AID,
    .on_receive = print_on_receive,
    .on_send = NULL,   // printer never sends data
    .on_status = NULL, // status always 0
    .on_clear = print_on_clear,
    .on_cmd = NULL, // no DDL/DDT
};

int ilprint_address(void)
{
    return hpil_dev_address(&print_dev);
}

// ******************************************
// init_ilprint()
//
// init the virtual HP-IL printer device
// and open printer.log in append mode
// ******************************************
void init_ilprint(char *path)
{
    hpil_dev_init(&print_dev);

    if (fprinter)
        fclose(fprinter);
    fprinter = NULL;
    if (path && path[0] != '\0') {
        fprinter = fopen(path, "a");
        if (fprinter)
            setlinebuf(fprinter);
    }
}

// ******************************************
// close_ilprint()
//
// close printer.log
// ******************************************
void close_ilprint(void)
{
    if (fprinter) {
        fclose(fprinter);
        fprinter = NULL;
    }
}

// ******************************************
// ILprint(frame)
//
// manage one HP-IL frame for the printer
// returns the (possibly modified) frame
// ******************************************
int ILprint(int frame)
{
    return hpil_dev_process(&print_dev, frame);
}
