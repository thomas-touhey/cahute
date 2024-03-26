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

/* TIMEOUT_PACKET_TYPE is the timeout before reading the packet type, i.e.
 * the first byte, while TIMEOUT_PACKET_CONTENTS is the timeout before
 * reading any of the following bytes. */
#define TIMEOUT_PACKET_TYPE     0
#define TIMEOUT_PACKET_CONTENTS 1000

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
    size_t buf_size, part_count = 1, part_repeat = 1;
    size_t part_sizes[2];
    int packet_type, err, variant = 0, checksum, checksum_alt;
    int log_part_data = 1, is_end = 0, is_final = 0;

    part_sizes[0] = 0;

    do {
        err = cahute_read_from_link(
            link,
            buf,
            1,
            TIMEOUT_PACKET_TYPE,
            TIMEOUT_PACKET_CONTENTS
        );
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

            link->flags &= ~CAHUTE_LINK_FLAG_TERMINATED;
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

    err = cahute_read_from_link(
        link,
        &buf[1],
        buf_size - 1,
        TIMEOUT_PACKET_CONTENTS,
        TIMEOUT_PACKET_CONTENTS
    );
    if (err == CAHUTE_ERROR_TIMEOUT_START)
        return CAHUTE_ERROR_TIMEOUT;
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
            !memcmp(&buf[1], "END\xFF", 4) || !memcmp(&buf[1], "FNC", 4)
            || !memcmp(&buf[1], "IMG", 4) || !memcmp(&buf[1], "MEM", 4)
            || !memcmp(&buf[1], "REQ", 4) || !memcmp(&buf[1], "TXT", 4)
            || !memcmp(&buf[1], "VAL", 4)) {
            /* The type seems to be a CAS50 header type.
             * This means that we actually have 10 more bytes to read for
             * a full header.
             *
             * NOTE: The '4' in the memcmp() calls above are intentional,
             * as the NUL character ('\0) is actually considered as part of
             * the CAS50 header type. */
            msg(ll_info, "Variant is determined to be CAS50.");

            err = cahute_read_from_link(
                link,
                &buf[40],
                10,
                TIMEOUT_PACKET_CONTENTS,
                TIMEOUT_PACKET_CONTENTS
            );
            if (err) {
                msg(ll_info, "Reading failed. The header base was:");
                mem(ll_info, buf, 40);
                if (err == CAHUTE_ERROR_TIMEOUT_START)
                    return CAHUTE_ERROR_TIMEOUT;
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
     * type and, especially, the size of the associated data.
     *
     * NOTE: These sections can either define:
     *
     * - 'part_sizes[0]' only, if there's only one part.
     * - 'part_sizes[...]' and 'part_count' if there's multiple parts,
     *   and/or with 'part_repeat' set if the number of repetitions is
     *   arbitrary.
     * - 'part_count' to 0 if there's no data part associated with the
     *    header. */
    switch (variant) {
    case CAHUTE_CASIOLINK_VARIANT_CAS40:
        if (!memcmp(&buf[1], "\x17\xFF", 2)) {
            /* End packet for CAS40. */
            part_count = 0;
            is_end = 1;
        } else if (!memcmp(&buf[1], "DD", 2)) {
            int width = buf[3], height = buf[4];

            /* Monochrome Screenshot. */
            if (!memcmp(&buf[5], "\x10\x44WF", 4))
                part_sizes[0] = ((width >> 3) + !!(width & 7)) * height;

            log_part_data = 0;
            is_final = 1;
        } else if (!memcmp(&buf[1], "DC", 2)) {
            int width = buf[3], height = buf[4];

            /* Color Screenshot. */
            if (!memcmp(&buf[5], "\x11UWF\x03", 4)) {
                part_repeat = 3;
                part_sizes[0] = 1 + ((width >> 3) + !!(width & 7)) * height;
            }

            log_part_data = 0;
            is_final = 1;
        } else if (!memcmp(&buf[1], "P1", 2)) {
            /* Single Numbered Program. */
            part_sizes[0] = ((buf[4] << 8) | buf[5]) - 2;
        } else if (!memcmp(&buf[1], "PZ", 2)) {
            /* Multiple Numbered Programs */
            part_count = 2;
            part_sizes[0] = 190;
            part_sizes[1] = ((buf[4] << 8) | buf[5]) - 2;
        }

        /* TODO */
        break;

    case CAHUTE_CASIOLINK_VARIANT_CAS50:
        if (!memcmp(&buf[1], "END\xFF", 4)) {
            /* End packet for CAS50. */
            part_count = 0;
            is_end = 1;
        } else if (!memcmp(&buf[1], "VAL", 4)) {
            int height = (buf[7] << 8) | buf[8];
            int width = (buf[9] << 8) | buf[10];

            /* Variable data use size as W*H, or only W, or only H depending
             * on the case. */
            if (!width)
                width = 1;

            part_sizes[0] = 14;
            part_repeat = height * width;
        } else {
            /* For other packets, the size should always be located at
             * offset 6 of the header, i.e. offset 7 of the buffer. */
            part_sizes[0] =
                ((buf[7] << 24) | (buf[8] << 16) | (buf[9] << 8) | buf[10]);
            if (part_sizes[0] > 2)
                part_sizes[0] -= 2;
            else
                part_count = 0;

            if (!memcmp(&buf[1], "MEM\0BU", 6)) {
                /* Backups are guaranteed to be the final (and only) file
                 * sent in the communication. */
                is_final = 1;
            }
        }
        break;

    case CAHUTE_CASIOLINK_VARIANT_CAS100:
        if (!memcmp(&buf[1], "END1", 4)) {
            /* End packet for CAS100. */
            part_count = 0;
            is_end = 1;
        }

        /* TODO */
        break;
    }

    if (part_count && !part_sizes[0]) {
        /* 'part_count' and 'part_sizes[0]' were left to their default values
         * of 1 and 0 respectively, which means they have not been set to
         * a found type. */
        cahute_u8 send_buf[1] = {PACKET_TYPE_INVALID_DATA};

        msg(ll_error,
            "Could not determine the data length out of the header.");

        /* The type has not been recognized, therefore we cannot determine
         * the size of the data to read (or the number of data parts). */
        err = cahute_write_to_link(link, send_buf, 1);
        if (err)
            return err;

        return CAHUTE_ERROR_IMPL;
    }

    if (part_count) {
        size_t total_size = buf_size;
        size_t part_i;

        for (part_i = 0; part_i < part_count; part_i++)
            total_size += part_sizes[part_i] * part_repeat;

        if (total_size > buf_capacity) {
            msg(ll_error,
                "Cannot get %" CAHUTE_PRIuSIZE "B into a %" CAHUTE_PRIuSIZE
                "B protocol buffer.",
                total_size,
                buf_capacity);

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
    }

    /* Acknowledge the file so that we can actually receive it. */
    {
        cahute_u8 const send_buf[] = {PACKET_TYPE_ACK};

        err = cahute_write_to_link(link, send_buf, 1);
        if (err)
            return err;
    }

    if (part_count) {
        cahute_u8 tmp_buf[2];
        size_t part_i, part_j, index, total;

        /* There is data to be read.
         * The method to transfer data here varies depending on the variant:
         *
         * - For CAS40 and CAS50, the data is provided in multiple packets
         *   depending on the part count & size using PACKET_TYPE_HEADER.
         * - For CAS100, the data is provided in multiple packets containing
         *   1024 bytes of data each, using PACKET_TYPE_DATA. */
        switch (variant) {
        case CAHUTE_CASIOLINK_VARIANT_CAS40:
        case CAHUTE_CASIOLINK_VARIANT_CAS50:
            buf = &buf[buf_size];

            index = 1;
            total = part_count * part_repeat;
            for (part_j = 0; part_j < part_repeat; part_j++) {
                for (part_i = 0; part_i < part_count; part_i++, index++) {
                    size_t part_size = part_sizes[part_i];

                    err = cahute_read_from_link(
                        link,
                        tmp_buf,
                        1,
                        TIMEOUT_PACKET_CONTENTS,
                        TIMEOUT_PACKET_CONTENTS
                    );
                    if (err == CAHUTE_ERROR_TIMEOUT_START)
                        return CAHUTE_ERROR_TIMEOUT;
                    if (err)
                        return err;

                    if (tmp_buf[0] != PACKET_TYPE_HEADER) {
                        msg(ll_error,
                            "Expected 0x3A (':') packet type, got 0x%02X.",
                            buf[0]);
                        return CAHUTE_ERROR_UNKNOWN;
                    }

                    msg(ll_info,
                        "Reading data part %d/%d (%" CAHUTE_PRIuSIZE "o).",
                        index,
                        total,
                        part_size);

                    err = cahute_read_from_link(
                        link,
                        buf,
                        part_size,
                        TIMEOUT_PACKET_CONTENTS,
                        TIMEOUT_PACKET_CONTENTS
                    );
                    if (err == CAHUTE_ERROR_TIMEOUT_START)
                        return CAHUTE_ERROR_TIMEOUT;
                    if (err)
                        return err;

                    /* Read and check the checksum. */
                    err = cahute_read_from_link(
                        link,
                        tmp_buf + 1,
                        1,
                        TIMEOUT_PACKET_CONTENTS,
                        TIMEOUT_PACKET_CONTENTS
                    );
                    if (err == CAHUTE_ERROR_TIMEOUT_START)
                        return CAHUTE_ERROR_TIMEOUT;
                    if (err)
                        return err;

                    /* For color screenshots, sometimes the first byte is not
                     * taken into account in the checksum calculation, as it's
                     * metadata for the sheet and not the "actual data" of the
                     * sheet. But sometimes it also gets the checksum right!
                     * In any case, we want to compute and check both checksums
                     * to see if at least one matches. */
                    checksum = cahute_casiolink_checksum(buf, part_size);
                    checksum_alt =
                        cahute_casiolink_checksum(buf + 1, part_size - 1);

                    if (checksum != tmp_buf[1] && checksum_alt != tmp_buf[1]) {
                        cahute_u8 const send_buf[] = {PACKET_TYPE_INVALID_DATA
                        };

                        msg(ll_warn,
                            "Invalid checksum (expected: 0x%02X, computed: "
                            "0x%02X).",
                            tmp_buf[1],
                            checksum);
                        mem(ll_info, buf, part_size);

                        msg(ll_error, "Transfer will abort.");
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

                    msg(ll_info,
                        "Data part %d/%d received and acknowledged.",
                        index,
                        total);
                    if (log_part_data)
                        mem(ll_info, buf, part_size);

                    buf += part_size;
                    buf_size += part_size;
                }
            }
            break;

        default:
            /* TODO */
            return CAHUTE_ERROR_IMPL;
        }
    }

    link->protocol_state.casiolink.last_variant = variant;
    link->protocol_buffer_size = buf_size;

    if (is_end) {
        /* The packet was an end packet. */
        link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
        msg(ll_info, "Received data was a sentinel!");
        return CAHUTE_ERROR_GONE;
    }

    if (is_final) {
        /* The packet was a final one in the communication. */
        link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
        msg(ll_info, "Received data was final!");
    }

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
        err = cahute_read_from_link(link, buf, 1, TIMEOUT_PACKET_TYPE, 0);
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

        err = cahute_read_from_link(link, buf, 1, TIMEOUT_PACKET_TYPE, 0);
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

    if (link->flags & CAHUTE_LINK_FLAG_TERMINATED)
        return CAHUTE_OK;

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
    size_t sheet_size;
    int err;

    do {
        err = cahute_casiolink_receive_data(link);
        if (err == CAHUTE_ERROR_TIMEOUT_START) {
            msg(ll_error, "No data received in a timely matter, exiting.");
            break;
        }

        if (err)
            return err;

        switch (link->protocol_state.casiolink.last_variant) {
        case CAHUTE_CASIOLINK_VARIANT_CAS40:
            if (!memcmp(&buf[1], "DD", 2)) {
                if (!memcmp(&buf[5], "\x10\x44WF", 4))
                    frame.cahute_frame_format =
                        CAHUTE_PICTURE_FORMAT_1BIT_MONO_CAS50;
                else
                    continue;

                frame.cahute_frame_height = buf[3];
                frame.cahute_frame_width = buf[4];
                frame.cahute_frame_data = &buf[40];
            } else if (!memcmp(&buf[1], "DC", 2)) {
                if (!memcmp(&buf[5], "\x11UWF\x03", 5)) {
                    sheet_size = buf[3] * ((buf[4] >> 3) + !!(buf[4] & 7));

                    /* Check that the color codes are all known, i.e. that
                     * they all are between 1 and 4 included. */
                    if (buf[40] < 1 || buf[40] > 4) {
                        msg(ll_warn,
                            "Unknown color code 0x%02X for sheet 1, skipping.",
                            buf[40]);
                        continue;
                    }
                    if (buf[40 + sheet_size + 1] < 1
                        || buf[40 + sheet_size + 1] > 4) {
                        msg(ll_warn,
                            "Unknown color code 0x%02X for sheet 2, skipping.",
                            buf[40 + sheet_size + 1]);
                        continue;
                    }
                    if (buf[40 + sheet_size + sheet_size + 2] < 1
                        || buf[40 + sheet_size + sheet_size + 2] > 4) {
                        msg(ll_warn,
                            "Unknown color code 0x%02X for sheet 3, skipping.",
                            buf[40 + sheet_size + sheet_size + 1]);
                        continue;
                    }

                    frame.cahute_frame_format =
                        CAHUTE_PICTURE_FORMAT_1BIT_TRIPLE_CAS50;
                } else
                    continue;

                frame.cahute_frame_height = buf[3];
                frame.cahute_frame_width = buf[4];
                frame.cahute_frame_data = &buf[40];
            } else
                continue;

            break;

        default:
            continue;
        }

        if ((*callback)(cookie, &frame))
            return CAHUTE_ERROR_INT;
    } while (1);

    return CAHUTE_OK;
}
