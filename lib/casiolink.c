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

/* 1-character program names for the PZ CAS40 data.
 * \xCD is ro and \xCE is theta. */
CAHUTE_LOCAL_DATA(cahute_u8 const *)
pz_program_names =
    (cahute_u8 const *)"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ\xCD\xCE";

/* The MDL1 command for Graph 100 / AFX used for initialization when
 * in sender / control mode. The speed and parity are inserted into the
 * copy of this buffer before the checksum is recomputed and placed into
 * the last byte. */
CAHUTE_LOCAL_DATA(cahute_u8 const *)
default_mdl1_payload =
    (cahute_u8 const *)":MDL1GY351\xFF" "000000N1.03\0\0\x01\0\0\0\x04\0\0\0"
    "\x01\0\x03\xFF\xFF\xFF\xFF\0";

/* TIMEOUT_INIT is the timeout before reading the response to the START
 * packet, at link initiation.
 * TIMEOUT_PACKET_TYPE is the timeout before reading the packet type, i.e.
 * the first byte, while TIMEOUT_PACKET_CONTENTS is the timeout before
 * reading any of the following bytes. */
#define TIMEOUT_INIT            500
#define TIMEOUT_PACKET_CONTENTS 2000

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
CAHUTE_INLINE(cahute_u8)
cahute_casiolink_checksum(cahute_u8 const *data, size_t size) {
    int checksum = 0;

    for (; size; size--)
        checksum += *data++;

    return (~checksum + 1) & 255;
}

/**
 * Answer a received CAS100 MDL1 header with the appropriate header,
 * then apply the serial settings provided within the header, then
 * handle the mutual acknowledgement.
 *
 * This flow is described in "Initiate the connection using the CAS100
 * header", assuming the link is in receive mode, and is currently in the
 * state where it has received the initial MDL1 header in the link's
 * data buffer.
 *
 * Note that we actually answer with the same MDL1 to make the calculator
 * believe we are compatible with them.
 *
 * @param link Link for which to handle the exchange.
 * @return Cahute error.
 */
CAHUTE_LOCAL(int) cahute_casiolink_handle_mdl1(cahute_link *link) {
    unsigned long new_serial_speed = 0;
    unsigned long new_serial_flags = link->medium.serial_flags;
    int mdl_correct = 1;
    cahute_u8 *buf = link->data_buffer;
    int err;

    /* We want to store the provided information. */
    memcpy(
        link->protocol_state.casiolink.raw_device_info,
        &buf[5],
        CASIOLINK_RAW_DEVICE_INFO_BUFFER_SIZE
    );
    link->protocol_state.casiolink.flags |=
        CASIOLINK_FLAG_DEVICE_INFO_OBTAINED;

    /* Send the MDL1 answer now. */
    err = cahute_write_to_link(link, buf, 40);
    if (err)
        return err;

    /* We should actually be receiving an acknowledgement, since we are
     * sending the same packet the calculator sent. */
    err = cahute_read_from_link(link, buf, 1, 0, 0);
    if (err)
        return err;

    if (buf[0] != PACKET_TYPE_ACK) {
        cahute_u8 const send_buf[] = {PACKET_TYPE_CORRUPTED};

        err = cahute_write_to_link(link, send_buf, 1);
        if (err)
            return err;

        return CAHUTE_ERROR_UNKNOWN;
    }

    /* We want to decode the serial parameters, to ensure that we
     * can actually decode them. */
    new_serial_flags = link->medium.serial_flags;
    new_serial_flags &= ~CAHUTE_SERIAL_PARITY_MASK;

    if (!memcmp(&buf[11], "038400", 6))
        new_serial_speed = 38400;
    else {
        msg(ll_error, "Unsupported new serial speed:");
        mem(ll_error, &buf[11], 6);
        mdl_correct = 0;
    }

    if (buf[17] == 'N')
        new_serial_flags |= CAHUTE_SERIAL_PARITY_OFF;
    else if (buf[17] == 'E')
        new_serial_flags |= CAHUTE_SERIAL_PARITY_EVEN;
    else if (buf[17] == 'O')
        new_serial_flags |= CAHUTE_SERIAL_PARITY_ODD;
    else {
        msg(ll_error, "Unsupported new serial parity:");
        mem(ll_error, &buf[17], 1);
        mdl_correct = 0;
    }

    if (!mdl_correct) {
        cahute_u8 const send_buf[] = {PACKET_TYPE_CORRUPTED};

        err = cahute_write_to_link(link, send_buf, 1);
        if (err)
            return err;

        return CAHUTE_ERROR_UNKNOWN;
    }

    /* We are ok with the sent MDL1, we can now send an acknowledgement.
     * The acknowledgement is already in our buffer, we can use that. */
    err = cahute_write_to_link(link, buf, 1);
    if (err)
        return err;

    /* Only now that the exchange has taken place, we want to set
     * the serial params. */
    err = cahute_set_serial_params_to_link(
        link,
        new_serial_flags,
        new_serial_speed
    );
    if (err) {
        msg(ll_error,
            "Could not set the serial params; that makes our "
            "connection irrecoverable!");
        link->flags |= CAHUTE_LINK_FLAG_IRRECOVERABLE;
        return err;
    }

    return CAHUTE_OK;
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
 * @param timeout Timeout before the first packet.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_LOCAL(int)
cahute_casiolink_receive_raw_data(cahute_link *link, unsigned long timeout) {
    cahute_u8 *buf = link->data_buffer;
    size_t buf_capacity = link->data_buffer_capacity;
    size_t buf_size, part_count = 1, part_repeat = 1;
    size_t part_sizes[5];
    int packet_type, err, variant = 0, checksum, checksum_alt;
    int log_part_data = 1, is_al_end = 0, is_end = 0, is_final = 0;
    int expected_data_packet_type = PACKET_TYPE_HEADER;

restart_reception:
    part_sizes[0] = 0;

    do {
        err = cahute_read_from_link(
            link,
            buf,
            1,
            timeout,
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

        msg(ll_info, "Received the following header:");
        mem(ll_info, buf, buf_size);
    } else {
        /* We want to try to determine the currently selected variant based
         * on the header's content. */
        if (!memcmp(&buf[1], "ADN1", 4) || !memcmp(&buf[1], "ADN2", 4)
            || !memcmp(&buf[1], "BKU1", 4) || !memcmp(&buf[1], "END1", 4)
            || !memcmp(&buf[1], "FCL1", 4) || !memcmp(&buf[1], "FMV1", 4)
            || !memcmp(&buf[1], "MCS1", 4) || !memcmp(&buf[1], "MDL1", 4)
            || !memcmp(&buf[1], "REQ1", 4) || !memcmp(&buf[1], "REQ2", 4)
            || !memcmp(&buf[1], "SET1", 4)) {
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
     * - 'part_sizes[...]' and 'part_count' if there's multiple parts.
     * - 'part_count' to 0 if there's no data part associated with the
     *    header.
     *
     * Note that if 'part_repeat' is set, it represents how much times the
     * last data size is read. */
    switch (variant) {
    case CAHUTE_CASIOLINK_VARIANT_CAS40:
        if (!memcmp(&buf[1], "\x17\x17", 2)) {
            /* CAS40 AL End */
            part_count = 0;
            is_al_end = 1;
        } else if (!memcmp(&buf[1], "\x17\xFF", 2)) {
            /* CAS40 End */
            part_count = 0;
            is_end = 1;
        } else if (!memcmp(&buf[1], "A1", 2)) {
            /* CAS40 Dynamic Graph */
            part_sizes[0] = ((size_t)buf[4] << 8) | buf[5];
            if (part_sizes[0] > 2)
                part_sizes[0] -= 2;

            is_final = 1;
        } else if (!memcmp(&buf[1], "AA", 2)) {
            /* CAS40 Dynamic Graph in Bulk */
            part_sizes[0] = ((size_t)buf[4] << 8) | buf[5];
            if (part_sizes[0] > 2)
                part_sizes[0] -= 2;
        } else if (!memcmp(&buf[1], "AD", 2)) {
            /* CAS40 All Memories */
            part_repeat = ((size_t)buf[5] << 8) | buf[6];
            part_sizes[0] = 22;
            is_final = 1;
        } else if (!memcmp(&buf[1], "AL", 2)) {
            /* CAS40 All */
            part_count = 0;
            link->flags |= CAHUTE_LINK_FLAG_ALMODE;
        } else if (!memcmp(&buf[1], "AM", 2)) {
            /* CAS40 Variable Memories */
            part_repeat = ((size_t)buf[5] << 8) | buf[6];
            part_sizes[0] = 22;
            is_final = 1;
        } else if (!memcmp(&buf[1], "BU", 2)) {
            /* CAS40 Backup */
            if (!memcmp(&buf[3], "TYPEA00", 7))
                part_sizes[0] = 32768;
            else if (!memcmp(&buf[3], "TYPEA02", 7))
                part_sizes[0] = 32768;

            is_final = 1;
        } else if (!memcmp(&buf[1], "DC", 2)) {
            /* CAS40 Color Screenshot. */
            unsigned int width = buf[3], height = buf[4];

            if (!memcmp(&buf[5], "\x11UWF\x03", 4)) {
                part_repeat = 3;
                part_sizes[0] = 1 + ((width >> 3) + !!(width & 7)) * height;
            }

            log_part_data = 0;
            is_final = 1;
        } else if (!memcmp(&buf[1], "DD", 2)) {
            /* CAS40 Monochrome Screenshot. */
            unsigned int width = buf[3], height = buf[4];

            if (!memcmp(&buf[5], "\x10\x44WF", 4))
                part_sizes[0] = ((width >> 3) + !!(width & 7)) * height;

            log_part_data = 0;
            is_final = 1;
        } else if (!memcmp(&buf[1], "DM", 2)) {
            /* CAS40 Defined Memories */
            part_repeat = ((size_t)buf[5] << 8) | buf[6];
            part_sizes[0] = 22;
            is_final = 1;
        } else if (!memcmp(&buf[1], "EN", 2)) {
            /* CAS40 Single Editor Program */
            part_sizes[0] = ((size_t)buf[4] << 8) | buf[5];
            if (part_sizes[0] >= 2)
                part_sizes[0] -= 2;

            is_final = 1;
        } else if (!memcmp(&buf[1], "EP", 2)) {
            /* CAS40 Single Password Protected Editor Program */
            part_sizes[0] = ((size_t)buf[4] << 8) | buf[5];
            if (part_sizes[0] >= 2)
                part_sizes[0] -= 2;
            is_final = 1;
        } else if (!memcmp(&buf[1], "F1", 2)) {
            /* CAS40 Single Function */
            part_sizes[0] = ((size_t)buf[4] << 8) | buf[5];
            if (part_sizes[0] >= 2)
                part_sizes[0] -= 2;

            is_final = 1;
        } else if (!memcmp(&buf[1], "F6", 2)) {
            /* CAS40 Multiple Functions */
            part_sizes[0] = ((size_t)buf[4] << 8) | buf[5];
            if (part_sizes[0] >= 2)
                part_sizes[0] -= 2;

            is_final = 1;
        } else if (!memcmp(&buf[1], "FN", 2)) {
            /* CAS40 Single Editor Program in Bulk */
            part_sizes[0] = ((size_t)buf[4] << 8) | buf[5];
            if (part_sizes[0] >= 2)
                part_sizes[0] -= 2;
        } else if (!memcmp(&buf[1], "FP", 2)) {
            /* CAS40 Single Password Protected Editor Program in Bulk */
            part_sizes[0] = ((size_t)buf[4] << 8) | buf[5];
            if (part_sizes[0] >= 2)
                part_sizes[0] -= 2;
        } else if (!memcmp(&buf[1], "G1", 2)) {
            /* CAS40 Graph Function */
            part_sizes[0] = ((size_t)buf[4] << 8) | buf[5];
            if (part_sizes[0] >= 2)
                part_sizes[0] -= 2;

            is_final = 1;
        } else if (!memcmp(&buf[1], "GA", 2)) {
            /* CAS40 Graph Function in Bulk */
            part_sizes[0] = ((size_t)buf[4] << 8) | buf[5];
            if (part_sizes[0] >= 2)
                part_sizes[0] -= 2;
        } else if (!memcmp(&buf[1], "GF", 2)) {
            /* CAS40 Factor */
            part_sizes[0] = 2 + buf[6] * 10;
            is_final = 1;
        } else if (!memcmp(&buf[1], "GR", 2)) {
            /* CAS40 Range */
            part_sizes[0] = 92;
            is_final = 1;
        } else if (!memcmp(&buf[1], "GT", 2)) {
            /* CAS40 Function Table */
            part_count = 3;
            part_repeat = ((size_t)buf[7] << 8) | buf[8];
            part_sizes[0] = buf[6];
            if (part_sizes[0] >= 2)
                part_sizes[0] -= 2;

            part_sizes[1] = 32;
            part_sizes[2] = 22;
            is_final = 1;
        } else if (!memcmp(&buf[1], "M1", 2)) {
            /* CAS40 Single Matrix */
            unsigned int width = buf[5], height = buf[6];

            part_sizes[0] = 14;
            part_repeat = width * height + 1; /* Sentinel data part. */
            is_final = 1;
        } else if (!memcmp(&buf[1], "MA", 2)) {
            /* CAS40 Single Matrix in Bulk */
            unsigned int width = buf[5], height = buf[6];

            part_sizes[0] = 14;
            part_repeat = width * height;
        } else if (!memcmp(&buf[1], "P1", 2)) {
            /* CAS40 Single Numbered Program. */
            part_sizes[0] = ((size_t)buf[4] << 8) | buf[5];
            if (part_sizes[0] >= 2)
                part_sizes[0] -= 2;

            is_final = 1;
        } else if (!memcmp(&buf[1], "PD", 2)) {
            /* CAS40 Polynomial Equation */
            part_sizes[0] = buf[6] * 10 + 12;
            is_final = 1;
        } else if (!memcmp(&buf[1], "PZ", 2)) {
            /* CAS40 Multiple Numbered Programs */
            part_count = 2;
            part_sizes[0] = 190;
            part_sizes[1] = ((size_t)buf[4] << 8) | buf[5];
            if (part_sizes[1] >= 2)
                part_sizes[1] -= 2;

            is_final = 1;
        } else if (!memcmp(&buf[1], "RT", 2)) {
            /* CAS40 Recursion Table */
            part_count = 3;
            part_repeat = ((size_t)buf[7] << 8) | buf[8];
            part_sizes[0] = buf[6];
            if (part_sizes[0] >= 2)
                part_sizes[0] -= 2;

            part_sizes[1] = 22;
            part_sizes[2] = 32;
            is_final = 1;
        } else if (!memcmp(&buf[1], "SD", 2)) {
            /* CAS40 Simultaneous Equations */
            part_repeat = buf[5] * buf[6] + 1;
            part_sizes[0] = 14;
            is_final = 1;
        } else if (!memcmp(&buf[1], "SR", 2)) {
            /* CAS40 Paired Variable Data */
            part_repeat = ((size_t)buf[5] << 8) | buf[6];
            part_sizes[0] = 32;
            is_final = 1;
        } else if (!memcmp(&buf[1], "SS", 2)) {
            /* CAS40 Single Variable Data */
            part_repeat = ((size_t)buf[5] << 8) | buf[6];
            part_sizes[0] = 22;
            is_final = 1;
        }

        break;

    case CAHUTE_CASIOLINK_VARIANT_CAS50:
        if (!memcmp(&buf[1], "END\xFF", 4)) {
            /* End packet for CAS50. */
            part_count = 0;
            is_end = 1;
        } else if (!memcmp(&buf[1], "VAL", 4)) {
            unsigned int height = ((unsigned int)buf[7] << 8) | buf[8];
            unsigned int width = ((unsigned int)buf[9] << 8) | buf[10];

            /* Variable data use size as W*H, or only W, or only H depending
             * on the case. */
            if (!width)
                width = 1;

            part_sizes[0] = 14;
            part_repeat = height * width;
        } else {
            /* For other packets, the size should always be located at
             * offset 6 of the header, i.e. offset 7 of the buffer. */
            part_sizes[0] = ((size_t)buf[7] << 24) | ((size_t)buf[8] << 16)
                            | ((size_t)buf[9] << 8) | buf[10];

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
        if (!memcmp(&buf[1], "BKU1", 4)) {
            /* Backup packet for CAS100. */
            part_sizes[0] = ((size_t)buf[9] << 24) | ((size_t)buf[10] << 16)
                            | ((size_t)buf[11] << 8) | buf[12];
        } else if (!memcmp(&buf[1], "END1", 4)) {
            /* End packet for CAS100. */
            part_count = 0;
            is_end = 1;
        } else if (!memcmp(&buf[1], "MCS1", 4)) {
            /* Main memory packet for CAS100. */
            part_sizes[0] = ((size_t)buf[8] << 8) | buf[9];
            if (!part_sizes[0])
                part_count = 0;
        } else if (!memcmp(&buf[1], "MDL1", 4)) {
            /* Initialization packet for CAS100. */
            err = cahute_casiolink_handle_mdl1(link);
            if (err)
                return err;

            /* From here, we go back to the beginning. */
            goto restart_reception;
        } else if (!memcmp(&buf[1], "SET1", 4)) {
            /* TODO */
            part_count = 0;
        }

        break;

    default:
        msg(ll_error, "Unhandled variant 0x%08X.", variant);
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (part_count && !part_sizes[0]) {
        /* 'part_count' and 'part_sizes[0]' were left to their default values
         * of 1 and 0 respectively, which means they have not been set to
         * a found type. */
        cahute_u8 send_buf[1] = {PACKET_TYPE_INVALID_DATA};

        /* The type has not been recognized, therefore we cannot determine
         * the size of the data to read (or the number of data parts). */
        err = cahute_write_to_link(link, send_buf, 1);
        if (err)
            return err;

        CAHUTE_RETURN_IMPL(
            "Could not determine the data length out of the header."
        );
    }

    if (part_count) {
        size_t total_size = buf_size;
        size_t part_i;

        for (part_i = 0; part_i < part_count; part_i++)
            total_size += part_sizes[part_i] * part_repeat;

        if (total_size > buf_capacity) {
            msg(ll_error,
                "Cannot get %" CAHUTE_PRIuSIZE "B into a %" CAHUTE_PRIuSIZE
                "B data buffer.",
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
        size_t part_i, index, total;

        /* There is data to be read.
         * The method to transfer data here varies depending on the variant:
         *
         * - For CAS40 and CAS50, the data is provided in multiple packets
         *   depending on the part count & size using PACKET_TYPE_HEADER.
         * - For CAS100, the data is provided in multiple packets containing
         *   1024 bytes of data each, using PACKET_TYPE_DATA. */
        buf = &buf[buf_size];

        index = 1;
        total = part_count - 1 + part_repeat;
        for (part_i = 0; part_i < total; part_i++, index++) {
            size_t part_size =
                part_sizes[part_i >= part_count ? part_count - 1 : part_i];

            msg(ll_info,
                "Reading data part %d/%d (%" CAHUTE_PRIuSIZE "o).",
                index,
                total,
                part_size);

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

            if (tmp_buf[0] != expected_data_packet_type) {
                msg(ll_error,
                    "Expected 0x3A (':') packet type, got 0x%02X.",
                    buf[0]);
                return CAHUTE_ERROR_UNKNOWN;
            }

            if (part_size) {
                size_t part_size_left = part_size;
                cahute_u8 *p = buf;

                /* Use a loop to be able to follow the transfer progress
                 * using logs. */
                while (part_size_left) {
                    size_t to_read =
                        part_size_left > 512 ? 512 : part_size_left;

                    err = cahute_read_from_link(
                        link,
                        p,
                        to_read,
                        TIMEOUT_PACKET_CONTENTS,
                        TIMEOUT_PACKET_CONTENTS
                    );
                    if (err == CAHUTE_ERROR_TIMEOUT_START)
                        return CAHUTE_ERROR_TIMEOUT;
                    if (err)
                        return err;

                    part_size_left -= to_read;
                    p += to_read;
                }
            }

            if (part_size) {
                /* For color screenshots, sometimes the first byte is not
                 * taken into account in the checksum calculation, as it's
                 * metadata for the sheet and not the "actual data" of the
                 * sheet. But sometimes it also gets the checksum right!
                 * In any case, we want to compute and check both checksums
                 * to see if at least one matches. */
                checksum = cahute_casiolink_checksum(buf, part_size);
                checksum_alt =
                    cahute_casiolink_checksum(buf + 1, part_size - 1);
            } else {
                checksum = 0;
                checksum_alt = 0;
            }

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

            if (checksum != tmp_buf[1] && checksum_alt != tmp_buf[1]) {
                cahute_u8 const send_buf[] = {PACKET_TYPE_INVALID_DATA};

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
            if (log_part_data
                && part_size <= 4096) /* Let's not flood the terminal. */
                mem(ll_info, buf, part_size);

            buf += part_size;
            buf_size += part_size;
        }
    }

    link->protocol_state.casiolink.last_variant = variant;
    link->data_buffer_size = buf_size;

    if (is_al_end || (is_end && (~link->flags & CAHUTE_LINK_FLAG_ALMODE))) {
        /* The packet was an end packet. */
        link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
        msg(ll_info, "Received data was a sentinel!");
        return CAHUTE_ERROR_TERMINATED;
    }

    if (is_final && (~link->flags & CAHUTE_LINK_FLAG_ALMODE)) {
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
    cahute_u8 *buf = link->data_buffer;
    int checksum, err;

    if (link->flags & CAHUTE_LINK_FLAG_RECEIVER) {
        /* Expect an initiation flow. */
        err = cahute_read_from_link(link, buf, 1, 0, 0);
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

        /* In the CAS100 variant, we actually expect an additional flow here,
         * being the MDL1 flow. */
        if (link->protocol_state.casiolink.variant
            == CAHUTE_CASIOLINK_VARIANT_CAS100) {
            err = cahute_read_from_link(
                link,
                buf,
                40,
                0,
                TIMEOUT_PACKET_CONTENTS
            );
            if (err)
                return err;

            msg(ll_info, "Received data for MDL1 is the following:");
            mem(ll_info, buf, 40);

            if (memcmp(buf, "\x3AMDL1", 5))
                err = CAHUTE_ERROR_UNKNOWN;

            if (!err) {
                checksum = cahute_casiolink_checksum(buf + 1, 38);
                if (buf[39] != checksum)
                    err = CAHUTE_ERROR_CORRUPT;
            }

            if (err) {
                cahute_u8 send_buf[1] = {PACKET_TYPE_CORRUPTED};

                msg(ll_error,
                    "Unknown or invalid packet when MDL1 was expected:");
                mem(ll_error, buf, 40);

                err = cahute_write_to_link(link, send_buf, 1);
                if (err)
                    return err;

                return err;
            }

            err = cahute_casiolink_handle_mdl1(link);
            if (err)
                return err;
        }

        return CAHUTE_OK;
    }

    /* Initial handshake, common to all CASIOLINK variants. */
    {
        int initial_attempts = 6, attempts;

        msg(ll_info,
            "Making the initial handshake (%d attempts, %lums for each).",
            initial_attempts,
            TIMEOUT_INIT);
        for (attempts = initial_attempts; attempts > 0; attempts--) {
            /* Make the initiation flow. */
            buf[0] = PACKET_TYPE_START;
            err = cahute_write_to_link(link, buf, 1);
            if (err)
                return err;

            err = cahute_read_from_link(link, buf, 1, TIMEOUT_INIT, 0);
            if (err == CAHUTE_ERROR_TIMEOUT_START)
                continue;

            if (err)
                return err;

            if (buf[0] != PACKET_TYPE_ESTABLISHED) {
                msg(ll_error,
                    "Expected ESTABLISHED packet (0x%02X), got 0x%02X.",
                    PACKET_TYPE_ESTABLISHED,
                    buf[0]);

                return CAHUTE_ERROR_UNKNOWN;
            }

            break;
        }

        if (attempts <= 0) {
            msg(ll_error, "No response after %d attempts.");
            return CAHUTE_ERROR_TIMEOUT_START;
        }
    }

    /* In the CAS100 variant, we actually need to initiate an additional
     * flow here, being the MDL1 flow. */
    if (link->protocol_state.casiolink.variant
        == CAHUTE_CASIOLINK_VARIANT_CAS100) {
        char serial_params[7];

        memcpy(buf, default_mdl1_payload, 40);

        /* NOTE: sprintf() adds a terminating zero, but we don't care,
         * since we override buf[17] right after. */
        sprintf(serial_params, "%06lu", link->medium.serial_speed);
        switch (link->medium.serial_flags & CAHUTE_SERIAL_PARITY_MASK) {
        case CAHUTE_SERIAL_PARITY_EVEN:
            serial_params[6] = 'E';
            break;

        case CAHUTE_SERIAL_PARITY_ODD:
            serial_params[6] = 'O';
            break;

        default:
            serial_params[6] = 'N';
        }

        memcpy(&buf[11], serial_params, 7);

        buf[39] = cahute_casiolink_checksum(&buf[1], 38);

        err = cahute_write_to_link(link, buf, 40);
        if (err)
            return err;

        err = cahute_read_from_link(link, buf, 40, 0, TIMEOUT_PACKET_CONTENTS);
        if (err)
            return err;

        msg(ll_info, "Received data for MDL1 is the following:");
        mem(ll_info, buf, 40);

        err = 0;
        if (memcmp(buf, "\x3AMDL1", 5) || memcmp(&buf[11], serial_params, 7))
            err = CAHUTE_ERROR_UNKNOWN;

        if (!err) {
            checksum = cahute_casiolink_checksum(buf + 1, 38);
            if (buf[39] != checksum)
                err = CAHUTE_ERROR_CORRUPT;
        }

        if (err) {
            cahute_u8 send_buf[1] = {PACKET_TYPE_CORRUPTED};

            msg(ll_error, "Unknown or invalid packet when MDL1 was expected:");
            mem(ll_error, buf, 40);

            err = cahute_write_to_link(link, send_buf, 1);
            if (err)
                return err;

            return err;
        }

        /* We want to store the received MDL1 packet here.
         * TODO: We may want to check the speed here. */
        memcpy(
            link->protocol_state.casiolink.raw_device_info,
            &buf[5],
            CASIOLINK_RAW_DEVICE_INFO_BUFFER_SIZE
        );
        link->protocol_state.casiolink.flags |=
            CASIOLINK_FLAG_DEVICE_INFO_OBTAINED;

        /* Send the acknowledgement. */
        buf[0] = PACKET_TYPE_ACK;

        err = cahute_write_to_link(link, buf, 1);
        if (err)
            return err;

        /* Receive the initial acknowledgement. */
        err = cahute_read_from_link(link, buf, 1, 0, 0);
        if (err)
            return err;

        if (buf[0] != PACKET_TYPE_ACK)
            return CAHUTE_ERROR_UNKNOWN;
    }

    return CAHUTE_OK;
}

/**
 * Terminate the connection, for any CASIOLINK variant.
 *
 * This must be called while the link is in sender / active mode.
 *
 * @param link Link for which to terminate the connection.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int) cahute_casiolink_terminate(cahute_link *link) {
    cahute_u8 buf[50];
    size_t buf_size;

    if (link->flags & CAHUTE_LINK_FLAG_TERMINATED)
        return CAHUTE_OK;

    buf_size = 40;
    memset(buf, 0xFF, 50);
    buf[0] = ':';

    switch (link->protocol_state.casiolink.variant) {
    case CAHUTE_CASIOLINK_VARIANT_CAS40:
        buf[1] = 0x17;
        buf[2] = 0xFF;
        break;

    case CAHUTE_CASIOLINK_VARIANT_CAS50:
        buf[1] = 'E';
        buf[2] = 'N';
        buf[3] = 'D';
        buf[4] = 0xFF;

        buf_size = 50;
        break;

    case CAHUTE_CASIOLINK_VARIANT_CAS100:
        buf[1] = 'E';
        buf[2] = 'N';
        buf[3] = 'D';
        buf[4] = '1';
        break;

    default:
        msg(ll_error,
            "Unhandled variant 0x%08X.",
            link->protocol_state.casiolink.variant);
        return CAHUTE_ERROR_UNKNOWN;
    }

    buf[buf_size - 1] = cahute_casiolink_checksum(&buf[1], buf_size - 2);

    msg(ll_info, "Sending the following end packet:");
    mem(ll_info, buf, buf_size);

    return cahute_write_to_link(link, buf, buf_size);
}

/**
 * Receive data.
 *
 * @param link Link to the device.
 * @param datap Data to allocate.
 * @param timeout Timeout to apply.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int)
cahute_casiolink_receive_data(
    cahute_link *link,
    cahute_data **datap,
    unsigned long timeout
) {
    cahute_u8 const *buf = link->data_buffer;
    int err;

    do {
        err = cahute_casiolink_receive_raw_data(link, timeout);
        if (err == CAHUTE_ERROR_TIMEOUT_START) {
            msg(ll_error, "No data received in a timely matter, exiting.");
            break;
        }

        if (err)
            return err;

        switch (link->protocol_state.casiolink.last_variant) {
        case CAHUTE_CASIOLINK_VARIANT_CAS40:
            if (!memcmp(&buf[1], "P1", 2))
                return cahute_create_program(
                    datap,
                    CAHUTE_TEXT_ENCODING_LEGACY_8,
                    NULL, /* No program name, this is anonymous. */
                    0,
                    NULL, /* No password. */
                    0,
                    &buf[40],
                    link->data_buffer_size - 41
                );

            if (!memcmp(&buf[1], "PZ", 2)) {
                cahute_data *data = NULL;
                cahute_u8 const *content, *names = pz_program_names;
                int i = 0;

                /* We want to copy all 38 programs. */
                buf = &buf[40];
                content = &buf[190];

                *datap = data;
                for (i = 1; i < 39; i++) {
                    size_t program_length = ((size_t)buf[1] << 8) | buf[2];

                    err = cahute_create_program(
                        datap,
                        CAHUTE_TEXT_ENCODING_LEGACY_8,
                        names++,
                        1,
                        NULL, /* No password. */
                        0,
                        content,
                        program_length ? program_length - 1 : 0
                    );
                    if (err) {
                        cahute_destroy_data(data);
                        return err;
                    }

                    datap = &(*datap)->cahute_data_next;
                    buf += 5;
                    content += program_length;
                }

                *datap = data;
                return CAHUTE_OK;
            }

            break;

        case CAHUTE_CASIOLINK_VARIANT_CAS50:
            if (!memcmp(&buf[1], "TXT", 4)) {
                size_t data_size = ((size_t)buf[7] << 24)
                                   | ((size_t)buf[8] << 16)
                                   | ((size_t)buf[9] << 8) | buf[10];
                size_t name_size = 8;
                size_t pw_size = 8;

                if (data_size >= 2)
                    data_size -= 2;

                for (; name_size && buf[10 + name_size] == 255; name_size--)
                    ;
                for (; pw_size && buf[26 + pw_size] == 255; pw_size--)
                    ;

                if (!memcmp(&buf[5], "PG", 2))
                    return cahute_create_program(
                        datap,
                        CAHUTE_TEXT_ENCODING_LEGACY_8,
                        &buf[11],
                        name_size,
                        &buf[27],
                        pw_size,
                        &buf[50],
                        data_size
                    );
            }
            break;

        case CAHUTE_CASIOLINK_VARIANT_CAS100:
            if (!memcmp(&buf[1], "MCS1", 4)) {
                cahute_u8 const *p;
                size_t name_size, group_size;

                for (p = &buf[11]; p < &buf[19] && *p != 0xFF; p++)
                    ;
                name_size = (size_t)(p - &buf[11]);

                for (p = &buf[19]; p < &buf[27] && *p != 0xFF; p++)
                    ;
                group_size = (size_t)(p - &buf[19]);

                err = cahute_mcs_decode_data(
                    datap,
                    &buf[19],
                    group_size,
                    NULL, /* MCS1 packet does not present a directory. */
                    0,
                    &buf[11],
                    name_size,
                    &buf[40],
                    link->data_buffer_size - 40,
                    buf[10]
                );
                if (err != CAHUTE_ERROR_IMPL)
                    return err;
            }
            /* TODO */
            break;

        default:
            msg(ll_error,
                "Unhandled variant 0x%08X.",
                link->protocol_state.casiolink.last_variant);
            return CAHUTE_ERROR_UNKNOWN;
        }

        /* If the data was final, we still need to break here. */
        if (link->flags & CAHUTE_LINK_FLAG_TERMINATED)
            return CAHUTE_ERROR_TERMINATED;
    } while (1);

    return CAHUTE_ERROR_UNKNOWN;
}

/**
 * Receive a frame through screen capture.
 *
 * @param link Link for which to receive screens.
 * @param frame Function to call back.
 * @param timeout Timeout to apply.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int)
cahute_casiolink_receive_screen(
    cahute_link *link,
    cahute_frame *frame,
    unsigned long timeout
) {
    cahute_u8 *buf = link->data_buffer;
    size_t sheet_size;
    int err;

    do {
        err = cahute_casiolink_receive_raw_data(link, timeout);
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
                    frame->cahute_frame_format =
                        CAHUTE_PICTURE_FORMAT_1BIT_MONO_CAS50;
                else
                    continue;

                frame->cahute_frame_height = buf[3];
                frame->cahute_frame_width = buf[4];
                frame->cahute_frame_data = &buf[40];
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

                    frame->cahute_frame_format =
                        CAHUTE_PICTURE_FORMAT_1BIT_TRIPLE_CAS50;
                } else
                    continue;

                frame->cahute_frame_height = buf[3];
                frame->cahute_frame_width = buf[4];
                frame->cahute_frame_data = &buf[40];
            } else
                continue;

            break;

        default:
            continue;
        }

        /* Frame is ready! */
        break;
    } while (1);

    /* We actually unset the fact that the link is terminated here, since
     * every screen is actually its own exchange. */
    link->flags &= ~CAHUTE_LINK_FLAG_TERMINATED;

    return CAHUTE_OK;
}

/**
 * Produce generic device information using the optionally stored
 * device information.
 *
 * @param link Link from which to get the cached EACK response.
 * @param infop Pointer to set to the allocated device information structure.
 * @return Cahute error, or 0 if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_casiolink_make_device_info(
    cahute_link *link,
    cahute_device_info **infop
) {
    cahute_device_info *info = NULL;
    char *buf;
    cahute_u8 const *raw_info;

    if (~link->protocol_state.casiolink.flags
        & CASIOLINK_FLAG_DEVICE_INFO_OBTAINED) {
        /* We don't have a 'generic device information'. */
        CAHUTE_RETURN_IMPL("No generic device with CASIOLINK.");
    }

    info = malloc(sizeof(cahute_device_info) + 20);
    if (!info)
        return CAHUTE_ERROR_ALLOC;

    buf = (void *)(&info[1]);
    raw_info = link->protocol_state.seven.raw_device_info;

    info->cahute_device_info_flags = CAHUTE_DEVICE_INFO_FLAG_OS;
    info->cahute_device_info_rom_capacity = 0;
    info->cahute_device_info_rom_version = "";

    info->cahute_device_info_flash_rom_capacity =
        ((unsigned long)raw_info[20] << 24)
        | ((unsigned long)raw_info[19] << 16)
        | ((unsigned long)raw_info[18] << 8) | raw_info[17];
    info->cahute_device_info_ram_capacity =
        ((unsigned long)raw_info[24] << 24)
        | ((unsigned long)raw_info[23] << 16)
        | ((unsigned long)raw_info[22] << 8) | raw_info[21];

    info->cahute_device_info_bootcode_version = "";
    info->cahute_device_info_bootcode_offset = 0;
    info->cahute_device_info_bootcode_size = 0;

    memcpy(buf, &raw_info[13], 4);
    buf[4] = 0;
    info->cahute_device_info_os_version = buf;
    buf += 5;

    info->cahute_device_info_os_offset = 0;
    info->cahute_device_info_os_size = 0;

    info->cahute_device_info_product_id = "";
    info->cahute_device_info_username = "";
    info->cahute_device_info_organisation = "";

    memcpy(buf, raw_info, 6);
    buf[6] = 0;
    info->cahute_device_info_hwid = buf;
    buf += 7;

    info->cahute_device_info_cpuid = "";

    *infop = info;
    return CAHUTE_OK;
}
