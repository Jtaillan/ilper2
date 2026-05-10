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
// ilper.h   ILPER prototypes
//
// 2011: ported on Linux by Ch. Gottheimer
// ---------------------------------------------------------------------------------

#ifndef __ILPER_H__
#define __ILPER_H__

#include "ilbase.h" // defines ETO and hpil_dev_t

#include <wchar.h>
#include <ncurses.h>

// From roman8.c
extern const wchar_t roman8_table[256];
extern int roman8_to_utf8(wchar_t wc, char *buf);

// From ilmain.c
extern char lif_file1[];
extern char lif_file2[];
extern char strpr[];
extern WINDOW *wtv;

// From scope.c
extern int ilscope_active(void);
extern void scope_open(const char *path);
extern void scope_close(void);
extern void scope_log(int in_frame, int out_frame);

// From ilper7.c
extern void init_hpil(void);
extern int hpil_transmit(int);

// From ildisp.c
extern int ILdisplay(int);
extern void init_ildisplay(void);
extern void set_display_encoding(int roman8);
extern void DisplayStr(char);
extern void ClrDisplay(void);
extern void refresh_display(void);
extern int ildisplay_address(void);
extern void ildisp_kbuf_push(int ch);
extern void set_view_size(int rows, int cols);
extern int get_view_rows(void);
extern int get_view_cols(void);

// From ildrive.c
extern void init_ilhdisc(char *, char *);
extern int ILhdisc1(int);
extern int ILhdisc2(int);
extern int ilhdisc1_active(void);
extern int ilhdisc2_active(void);
extern int ilhdisc1_address(void);
extern int ilhdisc2_address(void);

// From ilprint.c
extern int ILprint(int);
extern void init_ilprint(char *path);
extern void close_ilprint(void);
extern int ilprint_active(void);
extern int ilprint_address(void);

#endif //  __ILPER_H__
