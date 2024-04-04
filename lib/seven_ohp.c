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
#define TIMEOUT_PACKET_CONTENTS 2000 /* Timeout for the rest of the packet. */

#define IS_ASCII_HEX_DIGIT(C) \
    (((C) >= '0' && (C) <= '9') || ((C) >= 'A' && (C) <= 'F'))
#define ASCII_HEX_TO_NIBBLE(C) ((C) >= 'A' ? (C) - 'A' + 10 : (C) - '0')

#define PACKET_TYPE_ACK   6  /* 0x06 */
#define PACKET_TYPE_FRAME 11 /* 0x0B */
#define PACKET_TYPE_CHECK 22 /* 0x16 */

/* Recognized packet headers for alignment. */
CAHUTE_LOCAL_DATA(char const *)
alignment_sequences[] = {
    "\x0BTYP01",
    "\x0BTYPZ1",
    "\x0BTYPZ2",
    "\x16"
    "CAL00"
};
CAHUTE_LOCAL_DATA(size_t const)
alignment_sequence_count = sizeof(alignment_sequences) / sizeof(char const *);

/**
 * Compute a Protocol 7.00 packet checksum.
 *
 * @param data Data to compute the checksum for.
 * @param size Size of the data to compute the checksum for.
 * @return Obtained checksum.
 */
CAHUTE_INLINE(unsigned int)
cahute_seven_checksum(cahute_u8 const *data, size_t size) {
    int checksum = 0;
    size_t i;

    for (i = 0; i < size; i++)
        checksum += data[i];

    return (unsigned int)(~checksum + 1) & 255;
}

/**
 * Compute an 2-byte ASCII-HEX number representation on a given buffer.
 *
 * @param buf Buffer on which to represent the number.
 * @param number Number to represent.
 */
CAHUTE_INLINE(void)
cahute_seven_set_ascii_hex(cahute_u8 *buf, unsigned int number) {
    unsigned int higher = (number >> 4) & 15, lower = number & 15;

    buf[0] = higher > 9 ? 'A' + higher - 10 : '0' + higher;
    buf[1] = lower > 9 ? 'A' + lower - 10 : '0' + lower;
}

/**
 * Receive and decode a Protocol 7.00 screenstreaming packet, and store it
 * into the link.
 *
 * Note that if we receive a frame packet, we store its content directly
 * into the data buffer if we have enough capacity in it.
 *
 * @param link Link to use to receive the Protocol 7.00 packet.
 * @param align Whether we should align ourselves. to the beginning of the next
 *        packet, to avoid missing bytes from corrupting the whole connection.
 * @param timeout Timeout before the first byte.
 * @return Cahute error.
 */
CAHUTE_LOCAL(int)
cahute_seven_ohp_receive(cahute_link *link, int align, unsigned long timeout) {
    struct cahute_seven_ohp_state *state = &link->protocol_state.seven_ohp;
    cahute_u8 buf[50], *state_data = link->data_buffer;
    size_t packet_size;
    int err;

    if (align) {
        size_t to_complete = 6;

        /* We're aligning ourselves to receive a known packet.
         *
         * This is useful, if not necessary, because the calculator seems to
         * skip a few bytes sometimes, desynchronizing the input, meaning we
         * can't get frames from the desynchronization onward.
         *
         * With screenstreaming packet alignment, we only skip two frames
         * (the one cut short and the next one which has started too early
         * and for which the start was considered as part of the last packet),
         * then are able to recover.
         *
         * The sequences we can be expecting */
        while (1) {
            err = cahute_read_from_link(
                link,
                &buf[6 - to_complete],
                to_complete,
                timeout,
                TIMEOUT_PACKET_CONTENTS
            );
            if (err)
                return err;

            /* We look at every alignment sequence for every completion. */
            for (to_complete = 0; to_complete < 6; to_complete++) {
                size_t count = alignment_sequence_count;
                char const **p;

                for (p = alignment_sequences; count--; p++)
                    if (!memcmp(&buf[to_complete], *p, 6 - to_complete))
                        goto sequence_found;
            }

sequence_found:
            if (!to_complete)
                break;

            /* If we have found 2 matching bytes at the end of the buffer,
             * then we have 4 chars to complete.
             * This means we must move 6 - 4 = 2 bytes from index 4 onwards
             * to the beginning of the buffer. */
            if (to_complete < 6)
                memmove(buf, &buf[to_complete], 6 - to_complete);
        }
    } else {
        /* We just need to fill the initial 6 bytes in the buffer. */
        err = cahute_read_from_link(
            link,
            buf,
            6,
            TIMEOUT_PACKET_CONTENTS,
            TIMEOUT_PACKET_CONTENTS
        );
        if (err)
            return err;
    }

    state->last_packet_type = buf[0];
    memcpy(state->last_packet_subtype, &buf[1], 5);
    link->data_buffer_size = 0;

    packet_size = 6;
    if (buf[0] == PACKET_TYPE_CHECK || buf[0] == PACKET_TYPE_ACK) {
        /* Nothing to process, we just need the packet type and subtype to
         * be set, and that's already done above. */
    } else if (buf[0] == PACKET_TYPE_FRAME) {
        int width, height, format = -1;
        size_t frame_length = 0, expected_size;

        /* The subtype represents the kind of frame we have. */
        if (!memcmp(&buf[1], "TYP01", 5)) {
            width = 128;
            height = 64;
            format = CAHUTE_PICTURE_FORMAT_1BIT_MONO;
            frame_length = 1024;
        } else if (!memcmp(&buf[1], "TYPZ1", 5) || !memcmp(&buf[1], "TYPZ2", 5)) {
            if (buf[5] == '1') {
                /* The Frame Length (FL) field is 6 bytes long. */
                err = cahute_read_from_link(
                    link,
                    &buf[6],
                    18,
                    TIMEOUT_PACKET_CONTENTS,
                    TIMEOUT_PACKET_CONTENTS
                );
                if (err == CAHUTE_ERROR_TIMEOUT_START)
                    return CAHUTE_ERROR_TIMEOUT;
                if (err)
                    return err;

                if (!IS_ASCII_HEX_DIGIT(buf[6]) || !IS_ASCII_HEX_DIGIT(buf[7])
                    || !IS_ASCII_HEX_DIGIT(buf[8])
                    || !IS_ASCII_HEX_DIGIT(buf[9])
                    || !IS_ASCII_HEX_DIGIT(buf[10])
                    || !IS_ASCII_HEX_DIGIT(buf[11]))
                    return CAHUTE_ERROR_CORRUPT;

                packet_size += 18;
                frame_length =
                    ((ASCII_HEX_TO_NIBBLE(buf[6]) << 20)
                     | (ASCII_HEX_TO_NIBBLE(buf[7]) << 16)
                     | (ASCII_HEX_TO_NIBBLE(buf[8]) << 12)
                     | (ASCII_HEX_TO_NIBBLE(buf[9]) << 8)
                     | (ASCII_HEX_TO_NIBBLE(buf[10]) << 4)
                     | ASCII_HEX_TO_NIBBLE(buf[11]));
            } else {
                /* The Frame Length (FL) field is 8 bytes long. */
                err = cahute_read_from_link(
                    link,
                    &buf[6],
                    20,
                    TIMEOUT_PACKET_CONTENTS,
                    TIMEOUT_PACKET_CONTENTS
                );
                if (err == CAHUTE_ERROR_TIMEOUT_START)
                    return CAHUTE_ERROR_TIMEOUT;
                if (err)
                    return err;

                if (!IS_ASCII_HEX_DIGIT(buf[6]) || !IS_ASCII_HEX_DIGIT(buf[7])
                    || !IS_ASCII_HEX_DIGIT(buf[8])
                    || !IS_ASCII_HEX_DIGIT(buf[9])
                    || !IS_ASCII_HEX_DIGIT(buf[10])
                    || !IS_ASCII_HEX_DIGIT(buf[11])
                    || !IS_ASCII_HEX_DIGIT(buf[12])
                    || !IS_ASCII_HEX_DIGIT(buf[13]))
                    return CAHUTE_ERROR_CORRUPT;

                packet_size += 20;
                frame_length =
                    ((ASCII_HEX_TO_NIBBLE(buf[6]) << 28)
                     | (ASCII_HEX_TO_NIBBLE(buf[7]) << 24)
                     | (ASCII_HEX_TO_NIBBLE(buf[8]) << 20)
                     | (ASCII_HEX_TO_NIBBLE(buf[9]) << 16)
                     | (ASCII_HEX_TO_NIBBLE(buf[10]) << 12)
                     | (ASCII_HEX_TO_NIBBLE(buf[11]) << 8)
                     | (ASCII_HEX_TO_NIBBLE(buf[12]) << 4)
                     | ASCII_HEX_TO_NIBBLE(buf[13]));
            }

            if (!IS_ASCII_HEX_DIGIT(buf[packet_size - 12])
                || !IS_ASCII_HEX_DIGIT(buf[packet_size - 11])
                || !IS_ASCII_HEX_DIGIT(buf[packet_size - 10])
                || !IS_ASCII_HEX_DIGIT(buf[packet_size - 9])
                || !IS_ASCII_HEX_DIGIT(buf[packet_size - 8])
                || !IS_ASCII_HEX_DIGIT(buf[packet_size - 7])
                || !IS_ASCII_HEX_DIGIT(buf[packet_size - 6])
                || !IS_ASCII_HEX_DIGIT(buf[packet_size - 5])) {
                /* The header is corrupted.
                 * We however still want to skip the frame length and the
                 * checksum in order to fall back on our feet on next
                 * packet reception. */
                err = cahute_skip_from_link(
                    link,
                    frame_length + 2,
                    TIMEOUT_PACKET_CONTENTS,
                    TIMEOUT_PACKET_CONTENTS
                );
                if (err == CAHUTE_ERROR_TIMEOUT_START)
                    return CAHUTE_ERROR_TIMEOUT;
                if (err)
                    return err;

                return CAHUTE_ERROR_CORRUPT;
            }

            height =
                ((ASCII_HEX_TO_NIBBLE(buf[packet_size - 12]) << 12)
                 | (ASCII_HEX_TO_NIBBLE(buf[packet_size - 11]) << 8)
                 | (ASCII_HEX_TO_NIBBLE(buf[packet_size - 10]) << 4)
                 | ASCII_HEX_TO_NIBBLE(buf[packet_size - 9]));
            width =
                ((ASCII_HEX_TO_NIBBLE(buf[packet_size - 8]) << 12)
                 | (ASCII_HEX_TO_NIBBLE(buf[packet_size - 7]) << 8)
                 | (ASCII_HEX_TO_NIBBLE(buf[packet_size - 6]) << 4)
                 | ASCII_HEX_TO_NIBBLE(buf[packet_size - 5]));

            if (!memcmp(&buf[packet_size - 4], "1RC2", 4)) {
                format = CAHUTE_PICTURE_FORMAT_16BIT_R5G6B5;
                expected_size = width * height * 2;
            } else if (!memcmp(&buf[packet_size - 4], "1RC3", 4)) {
                format = CAHUTE_PICTURE_FORMAT_4BIT_RGB_PACKED;
                expected_size = width * height;
                expected_size = (expected_size >> 1) + (expected_size & 1);
            } else if (!memcmp(&buf[packet_size - 4], "1RM2", 4)) {
                format = CAHUTE_PICTURE_FORMAT_1BIT_DUAL;
                expected_size = ((width >> 3) + !!(width & 7)) * height << 1;
            } else {
                msg(ll_warn, "The following Frame Format was unknown:");
                mem(ll_warn, &buf[packet_size - 4], 4);
            }
        } else {
            msg(ll_error, "The following subtype was unknown:");
            mem(ll_error, &buf[1], 5);
            msg(ll_error, "The format and length could not be determined.");
            msg(ll_error, "This will likely break the link.");
        }

        /* We now have the following data:
         * - A format (that may be undefined, by being < 0).
         * - A frame length (that may be undefined, by being set to 0).
         * - A width and a height (that is unset if any of the two properties
         *   above is undefined).
         *
         * We must do the following checks to ensure that we can store the
         * packet correctly for later processing:
         * - The frame length is known. Otherwise, at least skip the checksum
         *   and return a CAHUTE_ERROR_UNKNOWN.
         * - The format is known. Otherwise, skip the frame length and the
         *   checksum and return a CAHUTE_ERROR_UNKNOWN.
         * - The size matches one the one we expect from a frame with
         *   the provided format and dimensions. Otherwise, skip the frame
         *   length and the checksum and return a CAHUTE_ERROR_UNKNOWN.
         * - The frame length fits within our data buffer. Otherwise,
         *   skip the frame length and the checksum and return a
         *   CAHUTE_ERROR_UNKNOWN.
         *
         * We can then read the frame data into the data buffer and
         * compute our checksum. If it does not match the checksum present
         * at the end of the frame, we return a CAHUTE_ERROR_CORRUPT. */

        if (!frame_length) {
            /* The message has likely already been displayed here, we don't
             * need to print another one. */
            err = cahute_skip_from_link(
                link,
                2,
                TIMEOUT_PACKET_CONTENTS,
                TIMEOUT_PACKET_CONTENTS
            );
            if (err == CAHUTE_ERROR_TIMEOUT_START)
                return CAHUTE_ERROR_TIMEOUT;
            if (err)
                return err;

            return CAHUTE_ERROR_UNKNOWN;
        }

        if (format < 0) {
            /* Same as above, the message has likely already been displayed. */
            err = cahute_skip_from_link(
                link,
                frame_length + 2,
                TIMEOUT_PACKET_CONTENTS,
                TIMEOUT_PACKET_CONTENTS
            );
            if (err == CAHUTE_ERROR_TIMEOUT_START)
                return CAHUTE_ERROR_TIMEOUT;
            if (err)
                return err;

            return CAHUTE_ERROR_UNKNOWN;
        }

        if (format == CAHUTE_PICTURE_FORMAT_1BIT_MONO)
            expected_size = ((width >> 3) + !!(width & 7)) * height;
        else if (format == CAHUTE_PICTURE_FORMAT_1BIT_DUAL)
            expected_size = ((width >> 3) + !!(width & 7)) * height * 2;
        else if (format == CAHUTE_PICTURE_FORMAT_4BIT_RGB_PACKED) {
            expected_size = width * height;
            expected_size = (expected_size >> 1) + (expected_size & 1);
        } else if (format == CAHUTE_PICTURE_FORMAT_16BIT_R5G6B5)
            expected_size = width * height * 2;
        else {
            /* This may be an implementation oversight, it's targeted towards
             * contributors to this function / protocol :-) */
            msg(ll_info, "Picture type is: %d", format);
            CAHUTE_RETURN_IMPL("No size estimation method for found format.");
        }

        if (expected_size != frame_length) {
            msg(ll_error,
                "Frame length %zuo did not match expected size %zuo for "
                "a %dx%d picture (format: %d).",
                frame_length,
                expected_size,
                width,
                height,
                format);

            err = cahute_skip_from_link(
                link,
                frame_length + 2,
                TIMEOUT_PACKET_CONTENTS,
                TIMEOUT_PACKET_CONTENTS
            );
            if (err == CAHUTE_ERROR_TIMEOUT_START)
                return CAHUTE_ERROR_TIMEOUT;
            if (err)
                return err;

            return CAHUTE_ERROR_UNKNOWN;
        }

        if (frame_length > link->data_buffer_capacity) {
            msg(ll_info,
                "Frame length %zuo exceeded data buffer capacity "
                "%zuo.",
                frame_length,
                link->data_buffer_capacity);

            /* We still want to skip the frame length and the
             * checksum in order to fall back on our feet on next
             * packet reception. */
            err = cahute_skip_from_link(
                link,
                frame_length + 2,
                TIMEOUT_PACKET_CONTENTS,
                TIMEOUT_PACKET_CONTENTS
            );
            if (err == CAHUTE_ERROR_TIMEOUT_START)
                return CAHUTE_ERROR_TIMEOUT;
            if (err)
                return err;

            return CAHUTE_ERROR_SIZE;
        }

        /* We are now able to read the data from the link to the protocol
         * buffer! */
        err = cahute_read_from_link(
            link,
            state_data,
            frame_length,
            TIMEOUT_PACKET_CONTENTS,
            TIMEOUT_PACKET_CONTENTS
        );
        if (err == CAHUTE_ERROR_TIMEOUT_START)
            return CAHUTE_ERROR_TIMEOUT;
        if (err)
            return err;

        state->picture_width = width;
        state->picture_height = height;
        state->picture_format = format;
        link->data_buffer_size = frame_length;
    } else {
        msg(ll_error, "Unknown packet type %d (0x%02X).", buf[0], buf[0]);

        /* Skip the checksum. */
        err = cahute_skip_from_link(
            link,
            2,
            TIMEOUT_PACKET_CONTENTS,
            TIMEOUT_PACKET_CONTENTS
        );
        if (err == CAHUTE_ERROR_TIMEOUT_START)
            return CAHUTE_ERROR_TIMEOUT;
        if (err)
            return err;

        return CAHUTE_ERROR_UNKNOWN;
    }

    msg(ll_info, "Received the following packet header:");
    mem(ll_info, buf, packet_size);

    /* We can now compute the checksum.
     * Note that adding checksums works, i.e.
     * checksum(A) + checksum(B) == checksum(AB). */
    err = cahute_read_from_link(
        link,
        &buf[packet_size],
        2,
        TIMEOUT_PACKET_CONTENTS,
        TIMEOUT_PACKET_CONTENTS
    );
    if (err == CAHUTE_ERROR_TIMEOUT_START)
        return CAHUTE_ERROR_TIMEOUT;
    if (err)
        return err;

    if (!IS_ASCII_HEX_DIGIT(buf[packet_size])
        || !IS_ASCII_HEX_DIGIT(buf[packet_size + 1]))
        return CAHUTE_ERROR_CORRUPT;

    {
        unsigned int obtained_checksum =
            ((ASCII_HEX_TO_NIBBLE(buf[packet_size]) << 4)
             | ASCII_HEX_TO_NIBBLE(buf[packet_size + 1]));
        unsigned int computed_checksum =
            cahute_seven_checksum(&buf[1], packet_size - 1);

        if (link->data_buffer_size) {
            computed_checksum +=
                cahute_seven_checksum(state_data, link->data_buffer_size);
            computed_checksum &= 255;
        }

        if (obtained_checksum != computed_checksum) {
            msg(ll_error,
                "Obtained checksum 0x%02X does not match computed "
                "checksum 0x%02X.",
                obtained_checksum,
                computed_checksum);

            return CAHUTE_ERROR_CORRUPT;
        }
    }

    return CAHUTE_OK;
}

/**
 * Send a basic Protocol 7.00 Screenstreaming packet.
 *
 * @param link Link on which to send the packet.
 * @param type Type of the packet to send.
 * @param subtype Subtype of the packet to send, 5 bytes long.
 * @return Cahute error, or 0 if no error has occurred.
 */
CAHUTE_LOCAL(int)
cahute_seven_ohp_send_basic(
    cahute_link *link,
    int type,
    cahute_u8 const *subtype
) {
    cahute_u8 buf[8];

    buf[0] = type;
    memcpy(&buf[1], subtype, 5);
    cahute_seven_set_ascii_hex(&buf[6], cahute_seven_checksum(&buf[1], 5));

    msg(ll_info, "Sending the following packet:");
    mem(ll_info, buf, 8);

    return cahute_write_to_link(link, buf, 8);
}

/**
 * Receive a frame through screenstreaming.
 *
 * @param link Link for which to receive screens.
 * @param frame Function to call back.
 * @param timeout Timeout to apply.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int)
cahute_seven_ohp_receive_screen(
    cahute_link *link,
    cahute_frame *frame,
    unsigned long timeout
) {
    struct cahute_seven_ohp_state *state = &link->protocol_state.seven_ohp;
    int err;

    while (1) {
        err = cahute_seven_ohp_receive(link, 1, timeout);
        switch (err) {
        case CAHUTE_OK:
            /* Continue. */
            break;

        case CAHUTE_ERROR_CORRUPT:
            /* In case of checksum error, we just continue receiving
             * packets. */
            msg(ll_warn, "Missed a frame due to corruption.");
            continue;

        default:
            return err;
        }

        switch (state->last_packet_type) {
        case PACKET_TYPE_FRAME:
            frame->cahute_frame_width = state->picture_width;
            frame->cahute_frame_height = state->picture_height;
            frame->cahute_frame_format = state->picture_format;
            frame->cahute_frame_data = link->data_buffer;

            return CAHUTE_OK;

        case PACKET_TYPE_CHECK:
            err = cahute_seven_ohp_send_basic(
                link,
                PACKET_TYPE_ACK,
                (cahute_u8 *)"02001"
            );
            if (err)
                return err;

            break;

        default:
            msg(ll_error,
                "Unexpected packet of type %d (0x%02X), exiting.",
                state->last_packet_type,
                state->last_packet_type);
            return CAHUTE_ERROR_UNKNOWN;
        }
    }

    /* We shouldn't have arrived here. */
    return CAHUTE_ERROR_UNKNOWN;
}
