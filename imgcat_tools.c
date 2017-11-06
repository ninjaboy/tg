/*
 * Copyright (c) 2014, 2017, Eddie Antonio Santos <easantos@ualberta.ca>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * print_image() prints an image to the screen, in whatever format is most
 * suitable.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "imgcat_tools.h"

static bool iterm2_passthrough(const char *filename);
static bool print_base64(const char *filename);
static void print_osc();
static void print_st();

void print_iterm_image(const char * filename) {
     iterm2_passthrough(filename);
}

/**
 * Pass-through to iTerm2's inline image feature.
 *
 * https://iterm2.com/documentation-images.html
 *
 * Based on iTerm's included script, **also** called imgcat:
 * https://raw.githubusercontent.com/gnachman/iTerm2/master/tests/imgcat
 */
static bool iterm2_passthrough(const char *filename) {
    /* TODO: support max width. */
    print_osc();
    printf("1337;File=inline=1:");
    print_base64(filename);
    print_st();
    return true;
}

static void print_base64_char(uint8_t c) {
    static const char b64_encode_table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    uint8_t index = c & 0b00111111;
    putchar(b64_encode_table[index]);
}

/**
 * Open the file and prints its contents in "canonical" base64.
 * See Table 1 in RFC4648: https://tools.ietf.org/html/rfc4648#page-6
 */
static bool print_base64(const char *filename) {
    FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        return false;
    }
    int c;
    uint8_t leftover_bits = 0;

    enum { byte_0, byte_1, byte_2, max_states } state = byte_0;

    /* Convert each character into base64. */
    while ((c = fgetc(file)) != EOF) {
        uint8_t lower, upper;
        switch (state) {
            case byte_0:
                print_base64_char(c >> 2);
                break;
            case byte_1:
                lower = (leftover_bits & 0b11) << 4;
                upper = (c & 0b11110000) >> 4;
                print_base64_char(upper | lower);
                break;
            case byte_2:
                upper = (leftover_bits & 0b00001111) << 2;
                lower = (c & 0b11000000) >> 6;
                print_base64_char(upper | lower);
                print_base64_char(c & 0b00111111);
                break;
            default:
                assert(false);
        }
        /* Save the leftover bits. */
        leftover_bits = c;
        /* Move to the next state. */
        state = (state + 1) % max_states;
    }

    /* Pad the end, if needed. */
    uint8_t upper = 0;
    switch (state) {
        case byte_0:
            /* No padding necessary: last character ended on byte boundary. */
            break;
        case byte_1:
            upper = (c & 0b11110000) >> 4;
            print_base64_char(upper);
            printf("==");
            break;
        case byte_2:
            upper = (leftover_bits & 0b00001111) << 2;
            print_base64_char(upper);
            putchar(' ');
            break;
        default:
            assert(false);
    }

    fclose(file);
    return true;
}

/**
 * Print the start of an operating system command (OSC).
 */
static void print_osc() {
    printf("\033]");
}

/**
 * Print the string terminator (ST), which in iTerm's case is simply the ASCII
 * bell.
 */
static void print_st() {
    printf("\007\n");
}