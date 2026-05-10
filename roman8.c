// ------------------------------------------------------------------------------
// ILPER 2.0 for Linux/macOS
//
// roman8.c   HP Roman-8 to Unicode translation table
//
// Index : Roman-8 byte value (0x00-0xFF)
// Value : Unicode code point (wchar_t)
//         L' ' = not used or undefined (display as space)
//
// Bytes 0x00-0x7F : standard ASCII — not used via this table (direct passthrough)
// Bytes 0x80-0xA0 : undefined in HP Roman-8 → space
// Bytes 0xA1-0xFF : HP Roman-8 extended characters
//
// 2026: Ported to macOS and thoroughly revised by J. Taillandier (new file)
// ------------------------------------------------------------------------------

#include <wchar.h>
#include "ilper.h"

// ******************************************
// roman8_to_utf8(wc, buf)
//
// encode a Unicode code point to UTF-8
// returns number of bytes written (1-3),
// or 0 on failure. buf must be >= 4 bytes.
// No locale dependency — works anywhere.
// ******************************************
int roman8_to_utf8(wchar_t wc, char *buf)
{
    if (wc < 0x80) {
        buf[0] = (char)wc;
        return 1;
    }
    if (wc < 0x800) {
        buf[0] = (char)(0xC0 | (wc >> 6));
        buf[1] = (char)(0x80 | (wc & 0x3F));
        return 2;
    }
    if (wc < 0x10000) {
        buf[0] = (char)(0xE0 | (wc >> 12));
        buf[1] = (char)(0x80 | ((wc >> 6) & 0x3F));
        buf[2] = (char)(0x80 | (wc & 0x3F));
        return 3;
    }
    return 0;
}

const wchar_t roman8_table[256] = {

    // 0x80-0xA0 : undefined / not used
    [0x80] = L' ',
    [0x81] = L' ',
    [0x82] = L' ',
    [0x83] = L' ',
    [0x84] = L' ',
    [0x85] = L' ',
    [0x86] = L' ',
    [0x87] = L' ',
    [0x88] = L' ',
    [0x89] = L' ',
    [0x8A] = L' ',
    [0x8B] = L' ',
    [0x8C] = L' ',
    [0x8D] = L' ',
    [0x8E] = L' ',
    [0x8F] = L' ',
    [0x90] = L' ',
    [0x91] = L' ',
    [0x92] = L' ',
    [0x93] = L' ',
    [0x94] = L' ',
    [0x95] = L' ',
    [0x96] = L' ',
    [0x97] = L' ',
    [0x98] = L' ',
    [0x99] = L' ',
    [0x9A] = L' ',
    [0x9B] = L' ',
    [0x9C] = L' ',
    [0x9D] = L' ',
    [0x9E] = L' ',
    [0x9F] = L' ',
    [0xA0] = L' ',

    // 0xA1-0xAF: accented uppercase, diacritics
    [0xA1] = L'À', // 161  A grave uppercase
    [0xA2] = L'Â', // 162  A circumflex uppercase
    [0xA3] = L'È', // 163  E grave uppercase
    [0xA4] = L'Ê', // 164  E circumflex uppercase
    [0xA5] = L'Ë', // 165  E umlaut uppercase
    [0xA6] = L'Î', // 166  I circumflex uppercase
    [0xA7] = L'Ï', // 167  I umlaut uppercase
    [0xA8] = L'´', // 168  acute accent
    [0xA9] = L'`', // 169  grave accent
    [0xAA] = L'ˆ', // 170  circumflex accent
    [0xAB] = L'¨', // 171  umlaut
    [0xAC] = L'˜', // 172  tilde
    [0xAD] = L'Ù', // 173  U grave uppercase
    [0xAE] = L'Û', // 174  U circumflex uppercase
    [0xAF] = L'₤', // 175  lira sign

    // 0xB0-0xBF: currency symbols, special punctuation
    [0xB0] = L'¯', // 176  macron
    [0xB1] = L' ', // 177  not used
    [0xB2] = L' ', // 178  not used
    [0xB3] = L'°', // 179  degree sign
    [0xB4] = L'Ç', // 180  C cedilla uppercase
    [0xB5] = L'ç', // 181  c cedilla lowercase
    [0xB6] = L'Ñ', // 182  N tilde uppercase
    [0xB7] = L'ñ', // 183  n tilde lowercase
    [0xB8] = L'¡', // 184  inverted exclamation mark
    [0xB9] = L'¿', // 185  inverted question mark
    [0xBA] = L'¤', // 186  generic currency sign
    [0xBB] = L'£', // 187  pound sterling
    [0xBC] = L'¥', // 188  yen
    [0xBD] = L'§', // 189  paragraph sign
    [0xBE] = L'ƒ', // 190  florin
    [0xBF] = L'¢', // 191  cent

    // 0xC0-0xCF: lowercase vowels (circumflex, acute, grave, umlaut)
    [0xC0] = L'â', // 192  a circumflex
    [0xC1] = L'ê', // 193  e circumflex
    [0xC2] = L'ô', // 194  o circumflex
    [0xC3] = L'û', // 195  u circumflex
    [0xC4] = L'á', // 196  a acute
    [0xC5] = L'é', // 197  e acute
    [0xC6] = L'ó', // 198  o acute
    [0xC7] = L'ú', // 199  u acute
    [0xC8] = L'à', // 200  a grave
    [0xC9] = L'è', // 201  e grave
    [0xCA] = L'ò', // 202  o grave
    [0xCB] = L'ù', // 203  u grave
    [0xCC] = L'ä', // 204  a umlaut
    [0xCD] = L'ë', // 205  e umlaut
    [0xCE] = L'ö', // 206  o umlaut
    [0xCF] = L'ü', // 207  u umlaut

    // 0xD0-0xDF: Nordic letters, accented uppercase
    [0xD0] = L'Å', // 208  A ring uppercase
    [0xD1] = L'î', // 209  i circumflex
    [0xD2] = L'Ø', // 210  O stroke uppercase
    [0xD3] = L'Æ', // 211  AE uppercase
    [0xD4] = L'å', // 212  a ring lowercase
    [0xD5] = L'í', // 213  i acute
    [0xD6] = L'ø', // 214  o stroke lowercase
    [0xD7] = L'æ', // 215  ae lowercase
    [0xD8] = L'Ä', // 216  A umlaut uppercase
    [0xD9] = L'ì', // 217  i grave
    [0xDA] = L'Ö', // 218  O umlaut uppercase
    [0xDB] = L'Ü', // 219  U umlaut uppercase
    [0xDC] = L'É', // 220  E acute uppercase
    [0xDD] = L'ï', // 221  i umlaut
    [0xDE] = L'ß', // 222  German eszett
    [0xDF] = L'Ô', // 223  O circumflex uppercase

    // 0xE0-0xEF: extended Latin characters
    [0xE0] = L'Á', // 224  A acute uppercase
    [0xE1] = L'Ã', // 225  A tilde uppercase
    [0xE2] = L'ã', // 226  a tilde lowercase
    [0xE3] = L'Ð', // 227  Eth uppercase
    [0xE4] = L'đ', // 228  d stroke lowercase
    [0xE5] = L'Í', // 229  I acute uppercase
    [0xE6] = L'Ì', // 230  I grave uppercase
    [0xE7] = L'Ó', // 231  O acute uppercase
    [0xE8] = L'Ò', // 232  O grave uppercase
    [0xE9] = L'Õ', // 233  O tilde uppercase
    [0xEA] = L'õ', // 234  o tilde lowercase
    [0xEB] = L'Š', // 235  S caron uppercase
    [0xEC] = L'š', // 236  s caron lowercase
    [0xED] = L'Ú', // 237  U acute uppercase
    [0xEE] = L'Ÿ', // 238  Y umlaut uppercase
    [0xEF] = L'ÿ', // 239  y umlaut lowercase

    // 0xF0-0xFF: miscellaneous symbols, fractions
    [0xF0] = L'Þ', // 240  thorn uppercase (Icelandic)
    [0xF1] = L'þ', // 241  thorn lowercase (Icelandic)
    [0xF2] = L' ', // 242  not used
    [0xF3] = L' ', // 243  not used
    [0xF4] = L' ', // 244  not used
    [0xF5] = L' ', // 245  not used
    [0xF6] = L'-', // 246  hyphen
    [0xF7] = L'¼', // 247  one quarter
    [0xF8] = L'½', // 248  one half
    [0xF9] = L'ª', // 249  feminine ordinal indicator
    [0xFA] = L'º', // 250  masculine ordinal indicator
    [0xFB] = L'«', // 251  left guillemet
    [0xFC] = L'■', // 252  filled square
    [0xFD] = L'»', // 253  right guillemet
    [0xFE] = L'±', // 254  plus or minus
    [0xFF] = L' ', // 255  not used
};
