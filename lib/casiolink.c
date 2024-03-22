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

#include <string.h>
#include "internals.h"

#define PACKET_TYPE_ACK          0x06
#define PACKET_TYPE_ESTABLISHED  0x13
#define PACKET_TYPE_START        0x16
#define PACKET_TYPE_INVALID_DATA 0x24
#define PACKET_TYPE_CORRUPTED    0x2B
#define PACKET_TYPE_HEADER       0x3A
#define PACKET_TYPE_DATA         0x3E

/**
 * Compute a checksum for a zone.
 *
 * @param data Buffer to read from.
 * @param size Size of the buffer to read from.
 * @return Computed checksum.
 */
CAHUTE_INLINE(int)
cahute_casiolink_checksum(cahute_u8 const *data, size_t size) {
    int checksum = 0;

    for (; size; size--)
        checksum += *data++;

    return (~checksum + 1) & 255;
}

/**
 * Check if the last received data for a link is an end packet.
 *
 * @param link Link to check.
 * @return 1 if the last received data is an end data, 0 otherwise.
 */
CAHUTE_INLINE(int) cahute_casiolink_is_end(cahute_link *link) {
    switch (link->protocol_state.casiolink.variant) {
    case CAHUTE_CASIOLINK_VARIANT_CAS40:
        if (!memcmp(&link->protocol_buffer[1], "\x17\xFF", 2))
            return 1;
        break;

    case CAHUTE_CASIOLINK_VARIANT_CAS50:
        if (!memcmp(&link->protocol_buffer[1], "END", 4))
            return 1;
        break;

    case CAHUTE_CASIOLINK_VARIANT_CAS100:
        if (!memcmp(&link->protocol_buffer[1], "END1", 4))
            return 1;
        break;
    }

    return 0;
}

/**
 * Receive a CASIOLINK header and associated data.
 *
 * If the header type is unknown or if the data length could not be determined,
 * the packet is rejected using ``PACKET_TYPE_INVALID_DATA_TYPE``, and the
 * function returns ``CAHUTE_ERROR_IMPL``.
 *
 * Note that the buffer capacity is assumed to be at least
 * CASIOLINK_MINIMUM_BUFFER_SIZE (50), which should have been guaranteed
 * in protocol initialization in link.c.
 *
 * @param link Link on which to receive the CASIOLINK packet.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_LOCAL(int) cahute_casiolink_receive_data(cahute_link *link) {
    cahute_u8 *buf = link->protocol_buffer;
    size_t buf_capacity = link->protocol_buffer_capacity;
    size_t buf_size, data_size = 0;
    int packet_type, err, type_found = 0, variant = 0, checksum;

    do {
        err = cahute_read_from_link(link, buf, 1);
        if (err)
            return err;

        packet_type = buf[0];
        if (packet_type == PACKET_TYPE_START) {
            cahute_u8 const send_buf[] = {PACKET_TYPE_ESTABLISHED};

            /* The sender is either re-initializing the connection, or
             * starting the connection and we did not check when creating
             * the link, either way we can just answer here and restart. */
            err = cahute_write_to_link(link, send_buf, 1);
            if (err)
                return err;

            continue;
        }

        if (packet_type != PACKET_TYPE_HEADER) {
            msg(ll_info,
                "Expected 0x3A (':') packet type, got 0x%02X.",
                packet_type);
            return CAHUTE_ERROR_UNKNOWN;
        }

        break;
    } while (1);

    /* The header is at least 40 bytes long, including the packet type, so
     * we want to read at least that. */
    if (link->protocol_state.casiolink.variant
        == CAHUTE_CASIOLINK_VARIANT_CAS50)
        buf_size = 50;
    else
        buf_size = 40;

    err = cahute_read_from_link(link, &buf[1], buf_size - 1);
    if (err)
        return err;

    if (link->protocol_state.casiolink.variant
        != CAHUTE_CASIOLINK_VARIANT_AUTO) {
        variant = link->protocol_state.casiolink.variant;
    } else {
        /* We want to try to determine the currently selected variant based
         * on the header's content. */
        if (!memcmp(&buf[1], "ADN1", 4) || !memcmp(&buf[1], "ADN2", 4)
            || !memcmp(&buf[1], "END1", 4) || !memcmp(&buf[1], "FCL1", 4)
            || !memcmp(&buf[1], "FMV1", 4) || !memcmp(&buf[1], "MDL1", 4)
            || !memcmp(&buf[1], "REQ1", 4) || !memcmp(&buf[1], "REQ2", 4)) {
            /* The type seems to be a CAS100 header type we can use. */
            variant = CAHUTE_CASIOLINK_VARIANT_CAS100;

            msg(ll_info, "Variant is determined to be CAS100.");
            msg(ll_info, "Received the following header:");
            mem(ll_info, buf, 40);
        } else if (
            !memcmp(&buf[1], "END", 4) || !memcmp(&buf[1], "FNC", 4)
            || !memcmp(&buf[1], "IMG", 4) || !memcmp(&buf[1], "MEM", 4)
            || !memcmp(&buf[1], "REQ", 4) || !memcmp(&buf[1], "VAL", 4)) {
            /* The type seems to be a CAS50 header type.
             * This means that we actually have 10 more bytes to read for
             * a full header.
             *
             * NOTE: The '4' in the memcmp() calls above are intentional,
             * as the NUL character ('\0) is actually considered as part of
             * the CAS50 header type. */
            msg(ll_info, "Variant is determined to be CAS50.");

            err = cahute_read_from_link(link, &buf[40], 10);
            if (err) {
                msg(ll_info, "Reading failed. The header base was:");
                mem(ll_info, buf, 40);
                return err;
            }

            buf_size += 10;
            variant = CAHUTE_CASIOLINK_VARIANT_CAS50;

            msg(ll_info, "Received the following header:");
            mem(ll_info, buf, 50);
        } else {
            /* By default, we consider the header to be a CAS40 header. */
            variant = CAHUTE_CASIOLINK_VARIANT_CAS40;

            msg(ll_info, "Variant is determined to be CAS40.");
            msg(ll_info, "Received the following header:");
            mem(ll_info, buf, 40);
        }
    }

    /* We have a full header and the size of said header, and it always ends
     * with a checksum which we can check.
     *
     * Note that the packet type is ignored in the checksum computation. */
    {
        checksum = cahute_casiolink_checksum(buf + 1, buf_size - 2);
        if (buf[buf_size - 1] != checksum) {
            cahute_u8 send_buf[1] = {PACKET_TYPE_CORRUPTED};

            msg(ll_error,
                "Invalid checksum (expected: 0x%02X, computed: 0x%02X), "
                "transfer will abort.",
                buf[buf_size - 1],
                checksum);

            link->flags |= CAHUTE_LINK_FLAG_IRRECOVERABLE;

            err = cahute_write_to_link(link, send_buf, 1);
            if (err)
                return err;

            return CAHUTE_ERROR_CORRUPT;
        }
    }

    /* We have a variant and a full header from which we can determine the
     * type and, especially, the size of the associated data. */
    switch (variant) {
    case CAHUTE_CASIOLINK_VARIANT_CAS40:
        if (!memcmp(&buf[1], "\x17\xFF", 2)) {
            /* End packet for CAS40. */
            type_found = 1;
        } else if (!memcmp(&buf[1], "DD", 2)) {
            int width = buf[3], height = buf[4];

            /* Monochrome Screenshot. */
            if (!memcmp(&buf[5], "\x10\x44WF", 4)) {
                type_found = 1;
                data_size = ((width >> 3) + !!(width & 7)) * height;
            }
        }

        /* TODO */
        break;

    case CAHUTE_CASIOLINK_VARIANT_CAS50:
        if (!memcmp(&buf[1], "END", 4)) {
            /* End packet for CAS50. */
            type_found = 1;
        }

        /* TODO */
        break;

    case CAHUTE_CASIOLINK_VARIANT_CAS100:
        if (!memcmp(&buf[1], "END1", 4)) {
            /* End packet for CAS100. */
            type_found = 1;
        }

        /* TODO */
        break;
    }

    if (!type_found) {
        cahute_u8 send_buf[1] = {PACKET_TYPE_INVALID_DATA};

        msg(ll_error,
            "Could not determine the data length out of the header.");
        link->flags |= CAHUTE_LINK_FLAG_TERMINATED;

        /* The type has not been recognized, therefore we cannot determine
         * the size of the data to read (or the number of data parts). */
        err = cahute_write_to_link(link, send_buf, 1);
        if (err)
            return err;

        return CAHUTE_ERROR_IMPL;
    }

    if (data_size && data_size + buf_size > buf_capacity) {
        msg(ll_error,
            "Cannot get %" CAHUTE_PRIuSIZE "B into a %" CAHUTE_PRIuSIZE
            "B protocol buffer.",
            data_size + buf_size,
            buf_capacity);

        link->flags |= CAHUTE_LINK_FLAG_TERMINATED;

        {
            cahute_u8 send_buf[1] = {PACKET_TYPE_INVALID_DATA};

            /* We actually send like we don't recognize the data, in
                * order not to make the link irrecoverable. */
            err = cahute_write_to_link(link, send_buf, 1);
            if (err)
                return err;
        }

        return CAHUTE_ERROR_SIZE;
    }

    /* Acknowledge the file so that we can actually receive it. */
    {
        cahute_u8 const send_buf[] = {PACKET_TYPE_ACK};

        err = cahute_write_to_link(link, send_buf, 1);
        if (err)
            return err;
    }

    if (data_size) {
        cahute_u8 tmp_buf[2];

        /* There is data to be read.
         * The method to transfer data here varies depending on the variant:
         *
         * - For CAS40 and CAS50, the data is provided in one packet using
         *   PACKET_TYPE_HEADER.
         * - For CAS100, the data is provided in multiple packets containing
         *   1024 bytes of data each, using PACKET_TYPE_DATA. */
        switch (variant) {
        case CAHUTE_CASIOLINK_VARIANT_CAS40:
        case CAHUTE_CASIOLINK_VARIANT_CAS50:
            err = cahute_read_from_link(link, tmp_buf, 1);
            if (err)
                return err;

            if (buf[0] != PACKET_TYPE_HEADER) {
                msg(ll_error,
                    "Expected 0x3A (':') packet type, got 0x%02X.",
                    buf[0]);
                return CAHUTE_ERROR_UNKNOWN;
            }

            err = cahute_read_from_link(link, &buf[buf_size], data_size);
            if (err)
                return err;

            /* Read and check the checksum. */
            err = cahute_read_from_link(link, tmp_buf + 1, 1);
            if (err)
                return err;

            checksum = cahute_casiolink_checksum(&buf[buf_size], data_size);
            if (checksum != tmp_buf[1]) {
                cahute_u8 const send_buf[] = {PACKET_TYPE_INVALID_DATA};

                msg(ll_error,
                    "Invalid checksum (expected: 0x%02X, computed: 0x%02X), "
                    "transfer will abort.",
                    buf[buf_size - 1],
                    checksum);

                link->flags |= CAHUTE_LINK_FLAG_IRRECOVERABLE;

                err = cahute_write_to_link(link, send_buf, 1);
                if (err)
                    return err;

                return CAHUTE_ERROR_CORRUPT;
            }

            /* Acknowledge the data. */
            {
                cahute_u8 const send_buf[] = {PACKET_TYPE_ACK};

                err = cahute_write_to_link(link, send_buf, 1);
                if (err)
                    return err;
            }

            buf_size += data_size;
            break;

        default:
            /* TODO */
            return CAHUTE_ERROR_IMPL;
        }
    }

    /* TODO: "If the data is a backup or a screen capture, it's the end of
     * the transfer, exit the loop". */

    link->protocol_state.casiolink.last_variant = variant;
    link->protocol_buffer_size = buf_size;
    return CAHUTE_OK;
}

/**
 * Initiate the connection, for any CASIOLINK variant.
 *
 * Note that this function is to be called while being either the sender or
 * the receiver.
 *
 * @param link Link for which to initiate the connection.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int) cahute_casiolink_initiate(cahute_link *link) {
    cahute_u8 buf[1];
    int err;

    if (link->flags & CAHUTE_LINK_FLAG_RECEIVER) {
        /* Expect an initiation flow. */
        err = cahute_read_from_link(link, buf, 1);
        if (err)
            return err;

        if (buf[0] != PACKET_TYPE_START) {
            msg(ll_error,
                "Expected START packet (0x%02X), got 0x%02X.",
                PACKET_TYPE_START,
                buf[0]);

            return CAHUTE_ERROR_UNKNOWN;
        }

        buf[0] = PACKET_TYPE_ESTABLISHED;
        err = cahute_write_to_link(link, buf, 1);
        if (err)
            return err;
    } else {
        /* Make the initiation flow. */
        buf[0] = PACKET_TYPE_START;
        err = cahute_write_to_link(link, buf, 1);
        if (err)
            return err;

        err = cahute_read_from_link(link, buf, 1);
        if (err)
            return err;

        if (buf[0] != PACKET_TYPE_ESTABLISHED) {
            msg(ll_error,
                "Expected ESTABLISHED packet (0x%02X), got 0x%02X.",
                PACKET_TYPE_ESTABLISHED,
                buf[0]);

            return CAHUTE_ERROR_UNKNOWN;
        }
    }

    return CAHUTE_OK;
}

/**
 * Terminate the connection, for any CASIOLINK variant.
 *
 * Note that this function is to be called whether we are the sender or
 * receiver.
 *
 * @param link Link for which to terminate the connection.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int) cahute_casiolink_terminate(cahute_link *link) {
    int err;

    if (link->flags & CAHUTE_LINK_FLAG_RECEIVER) {
        /* We need to receive an END header. */
        err = cahute_casiolink_receive_data(link);
        if (err)
            return err;

        if (!cahute_casiolink_is_end(link)) {
            msg(ll_warn, "Last received packet was not an END packet!");
            return CAHUTE_ERROR_UNKNOWN;
        }
    } else {
        cahute_u8 buf[50];
        size_t buf_size = 40;

        memset(buf, '\xFF', 50);
        buf[0] = ':';

        switch (link->protocol_state.casiolink.variant) {
        case CAHUTE_CASIOLINK_VARIANT_CAS40:
            buf[1] = '\x17';
            buf[2] = '\xFF';
            break;

        case CAHUTE_CASIOLINK_VARIANT_CAS50:
            buf[1] = 'E';
            buf[2] = 'N';
            buf[3] = 'D';
            buf[4] = '\0';

            buf_size = 50;
            break;

        case CAHUTE_CASIOLINK_VARIANT_CAS100:
            buf[1] = 'E';
            buf[2] = 'N';
            buf[3] = 'D';
            buf[4] = '1';
            break;
        }

        /* Compute the checksum as well! */
        {
            cahute_u8 const *p = buf + 1;
            size_t left = buf_size - 2;
            int checksum = 0;

            for (p = buf + 1, left = buf_size - 2; left; p++, left--)
                checksum += *p;

            buf[buf_size - 1] = checksum & 255;
        }

        err = cahute_write_to_link(link, buf, buf_size);
        if (err)
            return err;
    }

    return CAHUTE_OK;
}

/**
 * Call a function for every frame received through screenstreaming.
 *
 * @param link Link for which to receive screens.
 * @param callback Function to call back.
 * @param cookie Cookie with which to call back the function.
 */
CAHUTE_EXTERN(int)
cahute_casiolink_get_screen(
    cahute_link *link,
    cahute_process_frame_func *callback,
    void *cookie
) {
    cahute_frame frame;
    cahute_u8 *buf = link->protocol_buffer;
    int err;

    do {
        err = cahute_casiolink_receive_data(link);
        if (err)
            return err;

        switch (link->protocol_state.casiolink.last_variant) {
        case CAHUTE_CASIOLINK_VARIANT_CAS40:
            if (memcmp(&buf[1], "DD", 2))
                continue;

            if (!memcmp(&buf[5], "\x10\x44WF", 4))
                frame.cahute_frame_format =
                    CAHUTE_PICTURE_FORMAT_1BIT_MONO_CAS50;
            else
                continue;

            frame.cahute_frame_height = buf[3];
            frame.cahute_frame_width = buf[4];
            frame.cahute_frame_data = &buf[40];
            break;

        default:
            continue;
        }

        if ((*callback)(cookie, &frame))
            break;
    } while (1);

    /* TODO */
    return CAHUTE_ERROR_IMPL;
}
