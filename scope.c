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
// scope.c   HP-IL frame scope logger
//
// Decodes HP-IL frames into human-readable mnemonics and writes them to a log
// file, collapsing consecutive identical lines into a single "(xN)" entry.
//
// Public interface:
//   scope_open(path)       open (or reopen) the log file
//   scope_close()          flush and close
//   ilscope_active()       returns 1 if log file is open
//   scope_log(in, out)     log one frame, showing "IN -> OUT" when modified
//
// 2026: extracted from ilmain.c and ilper7.c by J. Taillandier
// ------------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>

#include "ilper.h"

static FILE *fscope = NULL;
static char scope_last[40] = "";
static int scope_count = 0;

int ilscope_active(void)
{
    return fscope != NULL;
}

// ******************************************
// scope_flush()
//
// write the buffered line with its repeat count,
// then reset the buffer
// ******************************************
static void scope_flush(void)
{
    if (!fscope || scope_count == 0)
        return;
    if (scope_count > 1)
        fprintf(fscope, "%s (x%d)\n", scope_last, scope_count);
    else
        fprintf(fscope, "%s\n", scope_last);
    scope_count = 0;
    scope_last[0] = '\0';
}

// ******************************************
// ILMnemo()
//
// return a human-readable mnemonic for frame
// ******************************************
static char *ILMnemo(int frame)
{
    static char stmp[12];
    int n = frame & 0xFF;
    int addr;

    switch (frame >> 8) {

    case 0: // DAB — data byte with ASCII translation
        snprintf(stmp, sizeof(stmp), "DAB %02X '%c'", n,
                 (n >= 0x20 && n <= 0x7E) ? (char)n : '.');
        break;

    case 1: // DAB(SRQ)
        snprintf(stmp, sizeof(stmp), "%-11s", "DAB(SRQ)");
        break;

    case 2: // END
        snprintf(stmp, sizeof(stmp), "%-11s", "END");
        break;

    case 3: // END(SRQ)
        snprintf(stmp, sizeof(stmp), "%-11s", "END(SRQ)");
        break;

    case 4: // CMD — groups in spec order: ACG, UCG, LAG, TAG, SAG
        if ((n & 0x70) == 0x00) {
            // ACG: x000xxxx — NUL/GTL/SDC/PPD/GET/ELN (low), PPE 0-15 (high)
            if (n & 0x80) {
                // PPE 0-15: 1000sbbb
                snprintf(stmp, sizeof(stmp), "PPE %02X     ", n & 0x0F);
            } else {
                switch (n) {
                case 0x00:
                    snprintf(stmp, sizeof(stmp), "%-11s", "NUL");
                    break;
                case 0x01:
                    snprintf(stmp, sizeof(stmp), "%-11s", "GTL");
                    break;
                case 0x04:
                    snprintf(stmp, sizeof(stmp), "%-11s", "SDC");
                    break;
                case 0x05:
                    snprintf(stmp, sizeof(stmp), "%-11s", "PPD");
                    break;
                case 0x08:
                    snprintf(stmp, sizeof(stmp), "%-11s", "GET");
                    break;
                case 0x0F:
                    snprintf(stmp, sizeof(stmp), "%-11s", "ELN");
                    break;
                default:
                    snprintf(stmp, sizeof(stmp), "CMD %02X     ", n);
                    break;
                }
            }
        } else if ((n & 0xE0) == 0xA0) {
            // ACG/DDL: 101xxxxx — DDL 0-31
            snprintf(stmp, sizeof(stmp), "DDL %02X     ", n & 0x1F);
        } else if ((n & 0xE0) == 0xC0) {
            // ACG/DDT: 110xxxxx — DDT 0-31
            snprintf(stmp, sizeof(stmp), "DDT %02X     ", n & 0x1F);
        } else if ((n & 0x70) == 0x10) {
            // UCG: x001xxxx — NOP/LLO/DCL/PPU/EAR (low), IFC/REN/NRE/AAU/LPD (high)
            switch (n) {
            case 0x10:
                snprintf(stmp, sizeof(stmp), "%-11s", "NOP");
                break;
            case 0x11:
                snprintf(stmp, sizeof(stmp), "%-11s", "LLO");
                break;
            case 0x14:
                snprintf(stmp, sizeof(stmp), "%-11s", "DCL");
                break;
            case 0x15:
                snprintf(stmp, sizeof(stmp), "%-11s", "PPU");
                break;
            case 0x18:
                snprintf(stmp, sizeof(stmp), "%-11s", "EAR");
                break;
            case 0x90:
                snprintf(stmp, sizeof(stmp), "%-11s", "IFC");
                break;
            case 0x92:
                snprintf(stmp, sizeof(stmp), "%-11s", "REN");
                break;
            case 0x93:
                snprintf(stmp, sizeof(stmp), "%-11s", "NRE");
                break;
            case 0x9A:
                snprintf(stmp, sizeof(stmp), "%-11s", "AAU");
                break;
            case 0x9B:
                snprintf(stmp, sizeof(stmp), "%-11s", "LPD");
                break;
            default:
                snprintf(stmp, sizeof(stmp), "CMD %02X     ", n);
                break;
            }
        } else if ((n & 0xE0) == 0x20) {
            // LAG: 001xxxxx — LAD 0-30, UNL
            addr = n & 0x1F;
            if (addr == 31)
                snprintf(stmp, sizeof(stmp), "%-11s", "UNL");
            else
                snprintf(stmp, sizeof(stmp), "LAD %02X     ", addr);
        } else if ((n & 0xE0) == 0x40) {
            // TAG: 010xxxxx — TAD 0-30, UNT
            addr = n & 0x1F;
            if (addr == 31)
                snprintf(stmp, sizeof(stmp), "%-11s", "UNT");
            else
                snprintf(stmp, sizeof(stmp), "TAD %02X     ", addr);
        } else if ((n & 0xE0) == 0x60) {
            // SAG: 011xxxxx — SAD 0-30
            snprintf(stmp, sizeof(stmp), "SAD %02X     ", n & 0x1F);
        } else {
            snprintf(stmp, sizeof(stmp), "CMD %02X     ", n);
        }
        break;

    case 5: // RDY — groups in spec order: RFC, ARG, AAG
        if (n == 0x00) {
            // RFC: 00000000
            snprintf(stmp, sizeof(stmp), "%-11s", "RFC");
        } else if ((n & 0xC0) == 0x40) {
            // ARG: 01xxxxxx — EOT(ETO/ETE), NRD, SOT(SDA/SST/SDI/SAI/TCT)
            switch (n) {
            case 0x40:
                snprintf(stmp, sizeof(stmp), "%-11s", "ETO");
                break;
            case 0x41:
                snprintf(stmp, sizeof(stmp), "%-11s", "ETE");
                break;
            case 0x42:
                snprintf(stmp, sizeof(stmp), "%-11s", "NRD");
                break;
            case 0x60:
                snprintf(stmp, sizeof(stmp), "%-11s", "SDA");
                break;
            case 0x61:
                snprintf(stmp, sizeof(stmp), "%-11s", "SST");
                break;
            case 0x62:
                snprintf(stmp, sizeof(stmp), "%-11s", "SDI");
                break;
            case 0x63:
                snprintf(stmp, sizeof(stmp), "%-11s", "SAI");
                break;
            case 0x64:
                snprintf(stmp, sizeof(stmp), "%-11s", "TCT");
                break;
            default:
                snprintf(stmp, sizeof(stmp), "RDY %02X     ", n);
                break;
            }
        } else if (n & 0x80) {
            // AAG: 1xxxxxxx — AAD/NAA/IAA, AEP/IEP, ZES/AES/NES/IES, AMP/NMP/IMP
            addr = n & 0x1F;
            switch (n >> 5) {
            case 4: // 100aaaaa — AAD 0-30, IAA (addr=31)
                if (addr == 31)
                    snprintf(stmp, sizeof(stmp), "%-11s", "IAA");
                else
                    snprintf(stmp, sizeof(stmp), "AAD %02X     ", addr);
                break;
            case 5: // 101aaaaa — AEP 0-30, IEP (addr=31)
                if (addr == 31)
                    snprintf(stmp, sizeof(stmp), "%-11s", "IEP");
                else
                    snprintf(stmp, sizeof(stmp), "AEP %02X     ", addr);
                break;
            case 6: // 110aaaaa — ZES (addr=0), AES 1-30, IES (addr=31)
                if (addr == 0)
                    snprintf(stmp, sizeof(stmp), "%-11s", "ZES");
                else if (addr == 31)
                    snprintf(stmp, sizeof(stmp), "%-11s", "IES");
                else
                    snprintf(stmp, sizeof(stmp), "AES %02X     ", addr);
                break;
            case 7: // 111aaaaa — AMP 0-30, IMP (addr=31)
                if (addr == 31)
                    snprintf(stmp, sizeof(stmp), "%-11s", "IMP");
                else
                    snprintf(stmp, sizeof(stmp), "AMP %02X     ", addr);
                break;
            default:
                snprintf(stmp, sizeof(stmp), "RDY %02X     ", n);
                break;
            }
        } else {
            snprintf(stmp, sizeof(stmp), "RDY %02X     ", n);
        }
        break;

    case 6: // IDY
        snprintf(stmp, sizeof(stmp), "IDY %02X     ", n);
        break;

    case 7: // IDY(SRQ)
        snprintf(stmp, sizeof(stmp), "IDY(SRQ) %02X", n);
        break;
    }

    return stmp;
}

// ******************************************
// log_mnemo(mnemo)
//
// append a mnemonic string to the scope log,
// collapsing consecutive identical lines
// ******************************************
static void log_mnemo(const char *mnemo)
{
    if (scope_count > 0 && strcmp(mnemo, scope_last) == 0) {
        scope_count++;
        return;
    }
    scope_flush();
    strncpy(scope_last, mnemo, sizeof(scope_last) - 1);
    scope_last[sizeof(scope_last) - 1] = '\0';
    scope_count = 1;
}

// ******************************************
// scope_open(path)
//
// open (or reopen) the scope log file
// ******************************************
void scope_open(const char *path)
{
    if (fscope)
        fclose(fscope);
    fscope = NULL;
    if (path && path[0] != '\0') {
        fscope = fopen(path, "w");
        if (fscope)
            setlinebuf(fscope);
    }
}

// ******************************************
// scope_close()
//
// flush the pending line and close the log file
// ******************************************
void scope_close(void)
{
    if (!fscope)
        return;
    scope_flush();
    fclose(fscope);
    fscope = NULL;
}

// strip trailing spaces in-place
static void rtrim(char *s)
{
    int n = (int)strlen(s);
    while (n > 0 && s[n - 1] == ' ')
        s[--n] = '\0';
}

// ******************************************
// scope_log(in_frame, out_frame)
//
// log one HP-IL frame to the scope file.
// if the frame was modified by a device,
// logs "IN -> OUT"; otherwise logs the frame.
// ILMnemo() uses a static buffer, so in_frame
// must be decoded and copied before calling it
// again for out_frame.
// ******************************************
void scope_log(int in_frame, int out_frame)
{
    char line[40];

    if (!fscope)
        return;

    if (in_frame == out_frame) {
        log_mnemo(ILMnemo(out_frame));
        return;
    }

    // frame was modified: log "IN -> OUT"
    char in_mnemo[16];
    strncpy(in_mnemo, ILMnemo(in_frame), sizeof(in_mnemo) - 1);
    in_mnemo[sizeof(in_mnemo) - 1] = '\0';
    rtrim(in_mnemo);
    snprintf(line, sizeof(line), "%s -> %s", in_mnemo, ILMnemo(out_frame));
    log_mnemo(line);
}
