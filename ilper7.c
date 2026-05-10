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
// ----------------------------------------------------------------------------------
//
// ilper7.c   HP-IL frame router and scope logger
//
// Based on previous personal work:
// 1986: ILPER4 (6502 assembler)
// 1988: ILPER5 modular version (6502 assembler)
// 1993: ILPER6 ported on PC (8086 assembler)
// 1997: rewriten in C and included in Emu41
// 2008: rewriten in VB for the standalone ILPER Windows version using the PILBox!
// 2009: released as free software
// 2011: ported on Linux by Ch. Gottheimer
// 2026: ported to macOS and thoroughly revised
// ---------------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>

#include "ilper.h"

// ******************************************
// hpil_transmit()
//
// transmit the frame to all the
// internal virtual devices
// ******************************************
int hpil_transmit(int frame)
{
    int in_frame = frame;

    frame = ILdisplay(frame);
    if (ilhdisc1_active())
        frame = ILhdisc1(frame);
    if (ilhdisc2_active())
        frame = ILhdisc2(frame);
    if (ilprint_active())
        frame = ILprint(frame);

    scope_log(in_frame, frame);
    return frame;
}

// ******************************************
// init_hpil()
//
// init the virtual devices at application startup
// ******************************************
void init_hpil(void)
{
    init_ildisplay();
    init_ilprint(strpr);
    init_ilhdisc(lif_file1, lif_file2);
}
