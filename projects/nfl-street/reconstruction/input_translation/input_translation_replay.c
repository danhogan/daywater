/* GNNE69 0x8003DDEC: bounded projection of the observed cardinal-input path. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "../kinematics/kinematics.h"
#include "input_translation.h"

#ifndef NFLSTREET_INPUT_TRANSLATION_NO_MAIN
static int hex_digit(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

static int read_hex(const char *text, uint8_t *out, size_t length)
{
    size_t i;
    if (strlen(text) != length * 2) return 0;
    for (i = 0; i < length; ++i) {
        int high = hex_digit(text[2 * i]);
        int low = hex_digit(text[2 * i + 1]);
        if (high < 0 || low < 0) return 0;
        out[i] = (uint8_t)((high << 4) | low);
    }
    return 1;
}
#endif

static uint32_t be32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 |
           (uint32_t)bytes[2] << 8 | bytes[3];
}

static void put_be32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)(value >> 24);
    bytes[1] = (uint8_t)(value >> 16);
    bytes[2] = (uint8_t)(value >> 8);
    bytes[3] = (uint8_t)value;
}

/* The original function has other branches and game-service side effects. */
int nflstreet_project_directional_input(const uint8_t input[0x20],
                                    uint8_t actor[0x30])
{
    uint32_t x_bits = be32(input + 4);
    uint32_t y_bits = be32(input + 8);
    uint32_t heading;
    float x, y;
    unsigned i;
    if (be32(input) != 0 || be32(input + 0x14) != 0) return 0;
    for (i = 0x0c; i < 0x14; ++i) if (input[i]) return 0;
    for (i = 0x18; i < 0x20; ++i) if (input[i]) return 0;
    memcpy(&x, &x_bits, sizeof x);
    memcpy(&y, &y_bits, sizeof y);
    if (!isfinite(x) || !isfinite(y) || fabsf(x)>1.0f || fabsf(y)>1.0f ||
        (x==0.0f&&y==0.0f)) return 0;
    heading = nflstreet_angle_from_vector(y, x);
    {
        volatile float yy=y*y;
        float magnitude = sqrtf(fmaf(x,x,yy));
        if(magnitude>1.0f)magnitude=1.0f;
        uint32_t magnitude_bits;
        memcpy(&magnitude_bits, &magnitude, sizeof magnitude_bits);
        put_be32(actor, magnitude_bits); /* +0x200: requested magnitude */
    }
    put_be32(actor + 4, heading); /* +0x204: command heading */
    put_be32(actor + 8, heading); /* +0x208: requested heading */
    actor[0x0e] = 1;            /* +0x20E: observed movement status */
    return 1;
}

/* Preserve the old cardinal-only interface's rejection contract. */
int nflstreet_project_cardinal_input(const uint8_t input[0x20],uint8_t actor[0x30]){
    uint32_t xb=be32(input+4),yb=be32(input+8);float x,y;memcpy(&x,&xb,4);memcpy(&y,&yb,4);
    if(!((yb==0&&x!=0.0f&&fabsf(x)<=1.0f)||(xb==0&&y>0.0f&&y<=1.0f)))return 0;
    return nflstreet_project_directional_input(input,actor);
}

#ifndef NFLSTREET_INPUT_TRANSLATION_NO_MAIN
int main(int argc, char **argv)
{
    uint8_t input[0x20], actor[0x30];
    unsigned i;
    if (argc != 3 || !read_hex(argv[1], input, sizeof input) ||
        !read_hex(argv[2], actor, sizeof actor)) {
        fputs("usage: input_translation_replay 64_INPUT_HEX 96_ACTOR_HEX\n", stderr);
        return 2;
    }
    if (!nflstreet_project_cardinal_input(input, actor)) {
        fputs("input takes an unverified translation branch\n", stderr);
        return 3;
    }
    for (i = 0; i < sizeof actor; ++i) printf("%02x", actor[i]);
    putchar('\n');
    return 0;
}
#endif
