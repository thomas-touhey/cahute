/* ****************************************************************************
 * Copyright (C) 2016-2017, 2024 Thomas Touhey <thomas@touhey.fr>
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

#include "p7screen.h"
#include <string.h>
#include <SDL.h>

static Uint32 const dual_pixels[] = {0xFFFFFF, 0xAAAAAA, 0x777777, 0x000000};
static Uint32 const multiple_cas50_colors[] = {
    0x000000, /* Unused. */
    0x000080,
    0x008000,
    0xFFFFFF,
    0xFF8000
};

/**
 * Display cookie.
 *
 * @property window Window that contains the surface.
 * @property renderer Renderer for the window.
 * @property texture Texture that covers the window.
 * @property saved_width Saved width from when the window was first opened.
 * @property saved_height Saved height from when the window was first opened.
 * @property zoom Zoom with which to draw the window.
 */
struct display_cookie {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    int saved_width;
    int saved_height;
    int zoom;
};

static char const error_notfound[] =
    "Could not connect to the calculator.\n"
    "- Is it plugged in and in PROJ mode?\n"
    "- Have you tried unplugging, plugging and selecting Projector on "
    "pop-up?\n"
    "- Have you tried changing the cable?\n";

static char const error_noaccess[] =
    "Could not get access to the calculator.\n"
    "Install the appropriate udev rule, or run as root.\n";

static char const error_unplanned[] =
    "The calculator didn't act as planned.\n"
    "Stop receive mode on calculator and start it again before re-running "
    "p7screen.\n";

/**
 * Update the texture pixels with the frame contents.
 *
 * Both the destination format and all source formats are organized into
 * lines first, columns seconds, which means a picture with pixels ABCDEF
 * represent the following:
 *
 *     A   B   C
 *     D   E   F
 *
 * However, the destination format also includes a zoom, which means that
 * the above picture will need to be represented the following for a zoom
 * of 3:
 *
 *     *  [A]  A   A   B   B   B   C   C   C
 *     /   A   A   A   B   B   B   C   C   C
 *     /   A   A   A   B   B   B   C   C   C
 *     *  [D]  D   D   E   E   E   F   F   F
 *     /   D   D   D   E   E   E   F   F   F
 *     /   D   D   D   E   E   E   F   F   F
 *
 * For every line we read from the source format, we compute the origin
 * of the destination line into ``oy``, which will be:
 *
 * - The index of the line, starting at 0...
 * - ... multiplied by the zoom (number of destination lines per source
 *   line)...
 * - ... multiplied by the width (number of pixels in one line)...
 * - ... multiplied by the zoom again (number of destination pixels per
 *   source pixel).
 *
 * Once we have this, the idea is that we only compute the first destination
 * line per source line, and copy it for the rest of the destination lines
 * corresponding to the same source line. In the example, lines prefixed
 * with "*" are computed directly, and lines prefixed with "/" are copied
 * from lines prefixed with "*".
 *
 * Then, for every pixel in the source frame, we copy it zoom times into
 * the destination frame.
 *
 * The basic template for every case is the following:
 *
 *     for (int y = 0, oy = 0; y < height; y++, oy += zoom_line_size) {
 *         for (int x = 0, ox = 0; x < width; x++, ox += zoom) {
 *              Uint32 pixel = ...;  // format-specific stuff
 *
 *              for (int dx = zoom - 1; dx >= 0; dx--)
 *                  pixels[oy + ox + dx] = pixel;
 *         }
 *
 *         for (int py = oy + zoom_line_size - line_size; py > oy;
 *              py -= line_size)
 *             memcpy(&pixels[py], &pixels[oy], line_size << 2);
 *     }
 *
 * Note that we use 'line_size << 2' at the end, and not 'line_size',
 * because we are dealing with 32-bit integers and not 8-bit integers here,
 * so we need to multiply by 4 the memory size to copy.
 *
 * Also note that we prefer the "one loop by format" instead of "one loop
 * for every format", because not having additional conditions in the main
 * body of the loop makes it faster.
 *
 * Also note that since we're on picture format conversions, with potentially
 * a lot of pixels, we want to use arithmetic or bitwise operations as much
 * as possible, since conditional operations disrupt the pipeline.
 *
 * @param pixels Pixels array to update, using sRGB colors on 32-bits.
 * @param frame Frame to update the pixels with.
 * @param zoom Zoom to draw the frame with.
 */
static void
update_texture_pixels(Uint32 *pixels, cahute_frame const *frame, int zoom) {
    int width = frame->cahute_frame_width;
    int height = frame->cahute_frame_height;
    int line_size = zoom * width;
    int zoom_line_size = line_size * zoom;
    int y, oy, x, ox, dx, py, mask;
    cahute_u8 const *data = frame->cahute_frame_data;
    cahute_u8 const *data2;
    cahute_u8 const *data3;
    Uint32 color1, color2, color3;

    switch (frame->cahute_frame_format) {
    case CAHUTE_PICTURE_FORMAT_1BIT_MONO:
        for (y = 0, oy = 0; y < height; y++, oy += zoom_line_size) {
            /* The mask will be right-shifted every pixel, unless it's 1,
             * which leads to the mask being reset to 128 using
             * '(mask & 1) << 7'. */
            mask = 128;

            for (x = 0, ox = 0; x < width; x++, ox += zoom) {
                Uint32 pixel = *data & mask ? 0x000000 : 0xFFFFFF;

                for (dx = zoom - 1; dx >= 0; dx--)
                    pixels[oy + ox + dx] = pixel;

                /* Go to the next byte if we're resetting the mask from
                 * 1 back to 128. */
                data += mask & 1;
                mask = (mask >> 1) | ((mask & 1) << 7);
            }

            for (py = oy + zoom_line_size - line_size; py > oy;
                 py -= line_size)
                memcpy(&pixels[py], &pixels[oy], line_size << 2);

            /* The start of the next line is aligned, if we are not aligned
             * already, we need to align ourselves. */
            data += (~mask & 128) >> 7;
        }
        break;

    case CAHUTE_PICTURE_FORMAT_1BIT_MONO_CAS50:
        for (y = 0, oy = 0; y < height; y++, oy += zoom_line_size) {
            for (x = 0, ox = 0; x < width; x++, ox += zoom) {
                Uint32 pixel =
                    data[((127 - x) >> 3) * 64 + y] & (128 >> (x & 7))
                        ? 0
                        : 0xFFFFFF;

                for (dx = zoom - 1; dx >= 0; dx--)
                    pixels[oy + ox + dx] = pixel;
            }

            for (py = oy + zoom_line_size - line_size; py > oy;
                 py -= line_size)
                memcpy(&pixels[py], &pixels[oy], line_size << 2);
        }
        break;

    case CAHUTE_PICTURE_FORMAT_1BIT_DUAL:
        data2 = data + height * ((width >> 3) + !!(width & 7));

        for (y = 0, oy = 0; y < height; y++, oy += zoom_line_size) {
            /* Same logic as for 1-bit monochrome encoding. */
            mask = 128;

            for (x = 0, ox = 0; x < width; x++, ox += zoom) {
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
                int index = *data & mask;
                index = (index << 1) | (*data2 & mask);
                index >>= 7 - (x & 7);
                Uint32 pixel = dual_pixels[index & 3];

                for (dx = zoom - 1; dx >= 0; dx--)
                    pixels[oy + ox + dx] = pixel;

                data += mask & 1;
                data2 += mask & 1;
                mask = (mask >> 1) | ((mask & 1) << 7);
            }

            for (py = oy + zoom_line_size - line_size; py > oy;
                 py -= line_size)
                memcpy(&pixels[py], &pixels[oy], line_size << 2);

            data += (~mask & 128) >> 7;
        }
        break;

    case CAHUTE_PICTURE_FORMAT_1BIT_TRIPLE_CAS50:
        color1 = multiple_cas50_colors[*data++];
        data2 = data + height * ((width >> 3) + !!(width & 7));
        color2 = multiple_cas50_colors[*data2++];
        data3 = data2 + height * ((width >> 3) + !!(width & 7));
        color3 = multiple_cas50_colors[*data3++];

        for (y = 0, oy = 0; y < height; y++, oy += zoom_line_size) {
            for (x = 0, ox = 0; x < width; x++, ox += zoom) {
                Uint32 pixel;

                if (data3[((width - 1 - x) >> 3) * height + (height - 1 - y)]
                    & (128 >> (x & 7)))
                    pixel = color3;
                else if (data2[((width - 1 - x) >> 3) * height + (height - 1 - y)] & (128 >> (x & 7)))
                    pixel = color2;
                else if (data[((width - 1 - x) >> 3) * height + (height - 1 - y)] & (128 >> (x & 7)))
                    pixel = color1;
                else
                    pixel = 0xFFFFFF;

                for (dx = zoom - 1; dx >= 0; dx--)
                    pixels[oy + ox + dx] = pixel;
            }

            for (py = oy + zoom_line_size - line_size; py > oy;
                 py -= line_size)
                memcpy(&pixels[py], &pixels[oy], line_size << 2);
        }
        break;

    case CAHUTE_PICTURE_FORMAT_4BIT_RGB_PACKED:
        mask = 240; /* 0b11110000 */

        for (y = 0, oy = 0; y < height; y++, oy += zoom_line_size) {
            for (x = 0, ox = 0; x < width; x++, ox += zoom) {
                int raw_value = *data & mask;
                Uint32 pixel = 0x000000;

                /* R bit in high and low nibble is 136 = 0b1000 1000.
                 * G bit in high and low nibble is 68 = 0b0100 0100.
                 * B bit in high and low nibble is 34 = 0b0010 0010. */
                if (raw_value & 136)
                    pixel |= 0xFF0000;
                if (raw_value & 68)
                    pixel |= 0x00FF00;
                if (raw_value & 34)
                    pixel |= 0x0000FF;

                for (dx = zoom - 1; dx >= 0; dx--)
                    pixels[oy + ox + dx] = pixel;

                data += mask & 1;
                mask = ~mask & 255;
            }

            for (py = oy + zoom_line_size - line_size; py > oy;
                 py -= line_size)
                memcpy(&pixels[py], &pixels[oy], line_size << 2);

            /* No end-of-line conditional re-alignment here, we are on a
             * packed format. */
        }
        break;

    case CAHUTE_PICTURE_FORMAT_16BIT_R5G6B5:
        for (y = 0, oy = 0; y < height; y++, oy += zoom_line_size) {
            for (x = 0, ox = 0; x < width; x++, ox += zoom) {
                /* We have a 16-bit integer being 0bRRRRRGGGGGGBBBBB.
                 * We need to extract these using masks, and place it
                 * at the right ranks in the resulting 24-bit RGB pixel. */
                unsigned long raw = (data[0] << 8) | data[1];
                Uint32 pixel = ((raw >> 11) & 31) << 19
                               | ((raw >> 5) & 63) << 10 | (raw & 31) << 3;

                for (dx = zoom - 1; dx >= 0; dx--)
                    pixels[oy + ox + dx] = pixel;

                data += 2;
            }

            for (py = oy + zoom_line_size - line_size; py > oy;
                 py -= line_size)
                memcpy(&pixels[py], &pixels[oy], line_size << 2);
        }
        break;

    default:
        fprintf(
            stderr,
            "!! Unhandled picture format %d\n",
            frame->cahute_frame_format
        );
    }
}

/**
 * Callback to display the screen frame.
 *
 * @param cookie Display cookie.
 * @param frame Frame to display.
 * @return Whether we want to interrupt the flow.
 */
static int
display_frame(struct display_cookie *cookie, cahute_frame const *frame) {
    int width, height, format, zoom;
    SDL_Event event;

    width = frame->cahute_frame_width;
    height = frame->cahute_frame_height;
    format = frame->cahute_frame_format;
    zoom = cookie->zoom;

    if (format != CAHUTE_PICTURE_FORMAT_1BIT_MONO
        && format != CAHUTE_PICTURE_FORMAT_1BIT_MONO_CAS50
        && format != CAHUTE_PICTURE_FORMAT_1BIT_DUAL
        && format != CAHUTE_PICTURE_FORMAT_1BIT_TRIPLE_CAS50
        && format != CAHUTE_PICTURE_FORMAT_4BIT_RGB_PACKED
        && format != CAHUTE_PICTURE_FORMAT_16BIT_R5G6B5) {
        fprintf(stderr, "Unsupported format %d.\n", format);
        return 1;
    }

    if (!cookie->window) {
        /* We haven't got a window, our objective is to create one,
         * with a renderer and a texture. First, let's create the
         * window. */
        cookie->window = SDL_CreateWindow(
            "p7screen",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            width * zoom,
            height * zoom,
            0
        );
        if (!cookie->window) {
            fprintf(
                stderr,
                "Couldn't create the window: %s\n",
                SDL_GetError()
            );
            return 1;
        }

        /* Then let's create the renderer. */
        cookie->renderer =
            SDL_CreateRenderer(cookie->window, -1, SDL_RENDERER_SOFTWARE);
        if (!cookie->renderer) {
            fprintf(
                stderr,
                "Couldn't create the renderer: %s\n",
                SDL_GetError()
            );
            return 1;
        }

        /* Update and read the event queue, ignoring events for now.
	    * This is needed for MacOS to show and update the window. */
        SDL_PumpEvents();
        while (SDL_PollEvent(&event) != 0) {}

        /* Finally, create the texture we're gonna use for drawing
            * the picture as a classic ARGB pixel matric (8 bits per
            * component). */
        cookie->texture = SDL_CreateTexture(
            cookie->renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            width * zoom,
            height * zoom
        );
        if (!cookie->texture) {
            fprintf(
                stderr,
                "Couldn't create the texture: %s\n",
                SDL_GetError()
            );
            return 1;
        }

        cookie->saved_width = width;
        cookie->saved_height = height;

        puts("Turn off your calculator (SHIFT+AC) when you have finished.\n");
    } else if (cookie->saved_width != width || cookie->saved_height != height) {
        /* The dimensions have changed somehow, we don't support this. */
        fprintf(stderr, "Unmanaged dimensions changed.\n");
        return 1;
    }

    /* Copy the data. */
    {
        Uint32 *texture_pixels;
        int pitch;

        SDL_LockTexture(
            cookie->texture,
            NULL,
            (void **)&texture_pixels,
            &pitch
        );

        update_texture_pixels(texture_pixels, frame, cookie->zoom);

        SDL_UnlockTexture(cookie->texture);
    }

    SDL_RenderCopy(cookie->renderer, cookie->texture, NULL, NULL);
    SDL_RenderPresent(cookie->renderer);

    return 0;
}

/**
 * Main entry point of the program.
 *
 * @param argc Argument count.
 * @param argv Argument values.
 * @return Exit status code.
 */
int main(int ac, char **av) {
    cahute_link *link = NULL;
    struct args args;
    struct display_cookie cookie;
    int bus, address, err, ret = 0;

    if (!parse_args(ac, av, &args))
        return 0;

    if ((err = find_usb_calculator(1, &bus, &address))
        || (err = cahute_open_usb_link(&link, CAHUTE_USB_OHP, bus, address))) {
        switch (err) {
        case CAHUTE_ERROR_NOT_FOUND:
            fprintf(stderr, error_notfound);
            break;

        case CAHUTE_ERROR_PRIV:
            fprintf(stderr, error_noaccess);
            break;

        default:
            fprintf(stderr, error_unplanned);
            break;
        }

        return 1;
    }

    /* Initialize the SDL. */
    if (SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        cahute_close_link(link);
        return 3;
    }
    atexit(SDL_Quit);

    cookie.window = NULL;
    cookie.renderer = NULL;
    cookie.texture = NULL;
    cookie.saved_width = -1;
    cookie.saved_height = -1;
    cookie.zoom = args.zoom;

    err = cahute_receive_screen(
        link,
        (cahute_process_frame_func *)display_frame,
        &cookie
    );
    if (err) {
        ret = 1;
        switch (err) {
        case CAHUTE_ERROR_INT:
            /* Interrupted; the error message was already displayed. */
            break;

        case CAHUTE_ERROR_GONE:
            ret = 0;
            break;

        default:
            fprintf(stderr, error_unplanned);
            break;
        }
    }

    if (cookie.texture)
        SDL_DestroyTexture(cookie.texture);
    if (cookie.renderer)
        SDL_DestroyRenderer(cookie.renderer);
    if (cookie.window)
        SDL_DestroyWindow(cookie.window);

    cahute_close_link(link);
    return ret;
}
