#include "tty.h"
#include "mem.h"
#include "ansi.h"
#include "string.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

/**
 * VGA buffer starts at 0xb8000 and provides 256k display memory.
 */

Terminal term;

static inline uint8_t 
vga_entry_color(VgaColor fg, VgaColor bg) {
    return fg | bg << 4;
}

static inline uint16_t 
vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t)(uc) | (uint16_t)(color) << 8;
}


static size_t const VGA_WIDTH = 80;
static size_t const VGA_HEIGHT = 25;

/* after set up paging the IO space is mapped from KERN_BASE. */
#define TERMBUF_START (KERN_BASE + 0xb8000)
#define TERMBUF_SIZE (VGA_HEIGHT * VGA_WIDTH)

static void set_bg_color(VgaColor bg) {
    term.color |= bg << 4;
}

static void set_fg_color(VgaColor fg) {
    term.color = vga_entry_color(fg, term.color >> 4);
}


void tty_clear() {
    term.row = 0;
    term.column = 0;
    for (size_t y = 0; y < VGA_HEIGHT; ++y)
        for (size_t x = 0; x < VGA_WIDTH; ++x)
            term.buffer[y * VGA_WIDTH + x] = vga_entry(' ', term.color);
}

void newline() {
    term.column = 0;
    term.row += 1;
}

void tty_init() {
    term.row = 0;
    term.column = 0;
    term.color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    term.buffer = (uint16_t *)(TERMBUF_START);
    tty_clear();
    tty_write_string("melonos 0.0.1\n");
}

void tty_set_cursor(uint16_t x, uint16_t y) {
    term.cursor = term.buffer + (y * VGA_WIDTH + x );
}

void tty_set_color(uint8_t color) { term.color = color; }

void put_entry_at(char c) {
    size_t x = term.column;
    size_t y = term.row;
    term.buffer[y * VGA_WIDTH + x] = vga_entry(c, term.color);
}

void tty_putchar(char c) {
    if (term.row > VGA_HEIGHT) {
        tty_clear();
    }

    if (c == '\n') {
        newline();
        return;
    }

    put_entry_at(c);
    if (++term.column == VGA_WIDTH) {
        term.column = 0;
        if (++term.row == VGA_HEIGHT)
            term.row = 0;
    }
}

static void ansi_cntl(const ANSIState * ansi) {
    switch(ansi->tag) {
        case ANSI_COLOR: 
            switch (ansi->value.color) {
            case ANSIColor_BBLK: 
                set_bg_color(VGA_COLOR_BLACK);
                break;
            case ANSIColor_BRED: 
                set_bg_color(VGA_COLOR_RED);
                break;
            case ANSIColor_BGRN: 
                set_bg_color(VGA_COLOR_GREEN);
                break;
            case ANSIColor_BYLW: 
                set_bg_color(VGA_COLOR_LIGHT_BROWN);
                break;
            case ANSIColor_BBLU: 
                set_bg_color(VGA_COLOR_BLUE);
                break;
            case ANSIColor_BMAG:
                set_bg_color(VGA_COLOR_LIGHT_MAGENTA);
                break;
            case ANSIColor_BCYA:
                set_bg_color(VGA_COLOR_CYAN);
                break;
            case ANSIColor_BWHT:
                set_bg_color(VGA_COLOR_WHITE);
                break;
            case ANSIColor_BDEF:
                set_bg_color(VGA_COLOR_BLACK);
                break;
            case ANSIColor_FBLK:
                set_fg_color(VGA_COLOR_BLACK);
                break;
            case ANSIColor_FRED:
                set_fg_color(VGA_COLOR_RED);
                break;
            case ANSIColor_FGRN:
                set_fg_color(VGA_COLOR_GREEN);
                break;
            case ANSIColor_FYLW:
                set_fg_color(VGA_COLOR_LIGHT_BROWN);
                break;
            case ANSIColor_FBLU:
                set_fg_color(VGA_COLOR_BLUE);
                break;
            case ANSIColor_FMAG:
                set_fg_color(VGA_COLOR_LIGHT_MAGENTA);
                break;
            case ANSIColor_FCYA:
                set_fg_color(VGA_COLOR_CYAN);
                break;
            case ANSIColor_FWHT:
                set_fg_color(VGA_COLOR_WHITE);
                break;
            case ANSIColor_FDEF:
                set_fg_color(VGA_COLOR_WHITE);
                break;
            case ANSIColor_RES:
                term.color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                break;
            default: break;
            }
            break;
        case ANSI_CURSOR : break;
        case ANSI_ERASE : break;
    }
}



/*! Write a single char to the screen. If it encounters an escape code, consume 
 *  it first then print the next character.
 * */
const char *tty_writec(const char *data) {
    ANSIState ansi;

    if (*data == '\033') { // ansi
        const char *p;
        if ((p = ansi_parse(&ansi, data)) != 0) {
            ansi_cntl(&ansi);
            data = p;
        }
    }

    tty_putchar(*data++);
    return data;
}

/*! Write null terminated string to the tty. When encounters escape code, 
 *  consume the escape code first.
 *
 *  @data   the string to write
 *  @size   size of character to write
 *  @return the pointer to the character one after the last character been printed.
 * */
const char *tty_write_string(const char *data) {
    const char *end = data + strlen(data);
    const char *p;
    while (data < end) {
        p =  tty_writec(data); 
        data = p;
    }
    return data;
}


static void print_uint(uint32_t n, int base) {
    static const char digits[] = "0123456789ABCDEF";

    char buf[32];
    int i = 0;

    do {
        buf[i++] = digits[n % base];
        n /= base;
    } while(n);

    switch (base) {
        case 2:
            buf[i++] = 'x';
            buf[i++] = 'b';
            break;
        case 8:
            buf[i++] = 'o';
            buf[i++] = '0';
            break;
        case 16:
            buf[i++] = 'x';
            buf[i++] = '0';
            break;
        default:
            break;
    }
    buf[i++] = '\0';

    tty_write_string(strrev(buf));
}


static void print_int(int n, int base) {
    if (n < 0) {
        n = -n;
        tty_putchar('-');
    }
    print_uint(n, base);
}


static void print_hex(int n) {
    print_int(n, 16);
}


static void print_uhex(uint32_t n) {
    print_uint(n, 16);
}


/*! printf on vga tty. supports %d, %x, %p, %s, %c.
 *  The implementation should always use `tty_write_string` because it handles
 *  ansi escapes properly.
 *
 *  @fmt formatted string
 *  @... data to be formatted
 * */ 
void tty_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    while(*fmt) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == 'd') {
                print_int(va_arg(args, int), 10);
                fmt++;
            } else if (*fmt == 'x') {
                print_uhex(va_arg(args, int));
                fmt++;
            } else if (*fmt == 'p') {
                print_uhex(va_arg(args, uint32_t));
                fmt++;
            } else if (*fmt == 'c') {
                tty_putchar(va_arg(args, int));
                fmt++;
            } else if (*fmt == 's') {
                fmt = tty_write_string(va_arg(args, const char *));
            }
        } else {
            fmt = tty_writec(fmt);
        }
    }
    va_end(args);
}

void repl() {
    tty_write_string("msh 0.0.1\n");
}
