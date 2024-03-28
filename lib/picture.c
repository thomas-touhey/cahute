/* ****************************************************************************
 * Copyright (C) 2024 Thomas Touhey <thomas@touhey.fr>
 *
 * This software is governed by the CeCILL 2.1 license under French law and
 * abiding by the rules of distribution of free software. You can use, modify
 * and/or redistribute the software under the terms of the CeCILL 2.1 license
 * as circulated by CEA, CNRS and INRIA at the following
 * URL: https://cecill.info
 *
 * As a counterpart to the access to the source code and rights to copy, modify
 * and redistribute granted by the license, users are provided only with a
 * limited warranty and the software's author, the holder of the economic
 * rights, and the successive licensors have only limited liability.
 *
 * In this respect, the user's attention is drawn to the risks associated with
 * loading, using, modifying and/or developing or reproducing the software by
 * the user in light of its specific status of free software, that may mean
 * that it is complicated to manipulate, and that also therefore means that it
 * is reserved for developers and experienced professionals having in-depth
 * computer knowledge. Users are therefore encouraged to load and test the
 * software's suitability as regards their requirements in conditions enabling
 * the security of their systems and/or data to be ensured and, more generally,
 * to use and operate it in the same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL 2.1 license and that you accept its terms.
 * ************************************************************************* */

#include "internals.h"

CAHUTE_LOCAL_DATA(cahute_u32 const)
dual_pixels[] = {0xFFFFFF, 0xAAAAAA, 0x777777, 0x000000};
CAHUTE_LOCAL_DATA(cahute_u32 const)
multiple_cas50_colors[] = {
    0x000000, /* Unused. */
    0x000080,
    0x008000,
    0xFFFFFF,
    0xFF8000
};

/**
 * Convert a picture from a source to a destination format.
 *
 * @param dest_uncasted Destination picture data, uncasted.
 * @param dest_format Format to use when writing picture data to the
 *        destination.
 * @param src_uncasted Source picture data, uncasted.
 * @param src_format Format to use when reading picture data from the source.
 * @param width Picture width.
 * @param height Picture height.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_convert_picture(
    void *dest_uncasted,
    int dest_format,
    void const *src_uncasted,
    int src_format,
    int width,
    int height
) {
    cahute_u32 *dest;
    cahute_u32 color1, color2, color3;
    cahute_u8 const *src;
    cahute_u8 const *src2;
    cahute_u8 const *src3;
    int x, y, mask;

    if (dest_format != CAHUTE_PICTURE_FORMAT_32BIT_ARGB_HOST)
        CAHUTE_RETURN_IMPL(
            "This function does not support converting to anything other "
            "than 32-bit ARGB in host endianness for now."
        );

    dest = (cahute_u32 *)dest_uncasted;
    src = (cahute_u8 *)src_uncasted;

    switch (src_format) {
    case CAHUTE_PICTURE_FORMAT_1BIT_MONO:
        for (y = 0; y < height; y++) {
            /* The mask will be right-shifted every pixel, unless it's 1,
             * which leads to the mask being reset to 128 using
             * '(mask & 1) << 7'. */
            mask = 128;

            for (x = 0; x < width; x++) {
                *dest++ = *src & mask ? 0x000000 : 0xFFFFFF;

                /* Go to the next byte if we're resetting the mask from
                 * 1 back to 128. */
                src += mask & 1;
                mask = (mask >> 1) | ((mask & 1) << 7);
            }

            /* The start of the next line is aligned, if we are not aligned
             * already, we need to align ourselves. */
            src += (~mask & 128) >> 7;
        }
        break;

    case CAHUTE_PICTURE_FORMAT_1BIT_MONO_CAS50:
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                *dest++ = src[((127 - x) >> 3) * 64 + y] & (128 >> (x & 7))
                              ? 0
                              : 0xFFFFFF;
            }
        }
        break;

    case CAHUTE_PICTURE_FORMAT_1BIT_DUAL:
        src2 = src + height * ((width >> 3) + !!(width & 7));

        for (y = 0; y < height; y++) {
            /* Same logic as for 1-bit monochrome encoding. */
            mask = 128;

            for (x = 0; x < width; x++) {
                /* We obtain the first bit and the second bit, then we need
                 * to place the first bit in the before-last rank (i.e. 0bX0)
                 * and the second bit to the last rank (i.e. 0bX).
                 *
                 * In order to do this, we determine the position of the
                 * bit in the original byte using '7 - (x & 7)'. Here's
                 * the table showing that it works:
                 *
                 * +-------+-------------+------+
                 * | x & 7 | 7 - (x & 7) | Rank |
                 * +-------+-------------+------+
                 * |     0 |           7 |    7 |
                 * |     1 |           6 |    6 |
                 * |     2 |           5 |    5 |
                 * |     3 |           4 |    4 |
                 * |     4 |           3 |    3 |
                 * |     5 |           2 |    2 |
                 * |     6 |           1 |    1 |
                 * |     7 |           0 |    0 |
                 * +-------+-------------+-----+
                 *
                 * Since both the first and second bit have the same rank,
                 * we can just shift the first bit a rank to the left, then
                 * shift the whole number down to rank 0 to obtain the 2-bit
                 * result.
                 *
                 * NOTE: Before shifting the first bit left, we need to
                 * ensure that the operation is run on an integer more
                 * than 8-bit long, otherwise the bit will be lost. */
                int index = *src & mask;
                index = (index << 1) | (*src2 & mask);
                index >>= 7 - (x & 7);

                *dest++ = dual_pixels[index & 3];

                src += mask & 1;
                src2 += mask & 1;
                mask = (mask >> 1) | ((mask & 1) << 7);
            }

            src += (~mask & 128) >> 7;
            src2 += (~mask & 128) >> 7;
        }
        break;

    case CAHUTE_PICTURE_FORMAT_1BIT_TRIPLE_CAS50:
        color1 = multiple_cas50_colors[*src++];
        src2 = src + height * ((width >> 3) + !!(width & 7));
        color2 = multiple_cas50_colors[*src2++];
        src3 = src2 + height * ((width >> 3) + !!(width & 7));
        color3 = multiple_cas50_colors[*src3++];

        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                cahute_u32 pixel;

                if (src3[((width - 1 - x) >> 3) * height + (height - 1 - y)]
                    & (128 >> (x & 7)))
                    pixel = color3;
                else if (src2[((width - 1 - x) >> 3) * height + (height - 1 - y)] & (128 >> (x & 7)))
                    pixel = color2;
                else if (src[((width - 1 - x) >> 3) * height + (height - 1 - y)] & (128 >> (x & 7)))
                    pixel = color1;
                else
                    pixel = 0xFFFFFF;

                *dest++ = pixel;
            }
        }
        break;

    case CAHUTE_PICTURE_FORMAT_4BIT_RGB_PACKED:
        mask = 240; /* 0b11110000 */

        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                int raw_value = *src & mask;
                cahute_u32 pixel = 0x000000;

                /* R bit in high and low nibble is 136 = 0b1000 1000.
                 * G bit in high and low nibble is 68 = 0b0100 0100.
                 * B bit in high and low nibble is 34 = 0b0010 0010. */
                if (raw_value & 136)
                    pixel |= 0xFF0000;
                if (raw_value & 68)
                    pixel |= 0x00FF00;
                if (raw_value & 34)
                    pixel |= 0x0000FF;

                *dest++ = pixel;

                src += mask & 1;
                mask = ~mask & 255;
            }

            /* No end-of-line conditional re-alignment here, we are on a
             * packed format. */
        }
        break;

    case CAHUTE_PICTURE_FORMAT_16BIT_R5G6B5:
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                /* We have a 16-bit integer being 0bRRRRRGGGGGGBBBBB.
                 * We need to extract these using masks, and place it
                 * at the right ranks in the resulting 24-bit RGB pixel. */
                unsigned long raw = (src[0] << 8) | src[1];

                *dest++ = ((raw >> 11) & 31) << 19 | ((raw >> 5) & 63) << 10
                          | (raw & 31) << 3;

                src += 2;
            }
        }
        break;

    default:
        msg(ll_info, "Picture format identifier was: %d", src_format);
        CAHUTE_RETURN_IMPL("Unhandled picture format for conversion.");
    }

    return CAHUTE_OK;
}

/**
 * Convert a frame to a picture format.
 *
 * @param dest Destination picture data.
 * @param dest_format Format to write with in the destination picture data.
 * @param frame Source frame.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_convert_picture_from_frame(
    void *dest,
    int dest_format,
    cahute_frame const *frame
) {
    return cahute_convert_picture(
        dest,
        dest_format,
        frame->cahute_frame_data,
        frame->cahute_frame_format,
        frame->cahute_frame_width,
        frame->cahute_frame_height
    );
}
