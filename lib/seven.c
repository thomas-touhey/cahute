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

/* TIMEOUT_PACKET_START is the timeout before a packet starts.
 * TIMEOUT_PACKET_CONTENTS is the timeout in between bytes for a packet. */
#define TIMEOUT_PACKET_START    0
#define TIMEOUT_PACKET_CONTENTS 2000

#define IS_ASCII_HEX_DIGIT(C) \
    (((C) >= '0' && (C) <= '9') || ((C) >= 'A' && (C) <= 'F'))
#define ASCII_HEX_TO_NIBBLE(C) ((C) >= 'A' ? (C) - 'A' + 10 : (C) - '0')
#define CONDITIONAL_ASCII_HEX_DIGIT(C) \
    (IS_ASCII_HEX_DIGIT(C) ? ASCII_HEX_TO_NIBBLE(C) : 0)
#define CONDITIONAL_ASCII_DEC_DIGIT(C) (isdigit(C) ? (C) - '0' : 0)

#define PACKET_TYPE_COMMAND  1  /* 0x01 */
#define PACKET_TYPE_DATA     2  /* 0x02 */
#define PACKET_TYPE_ROLESWAP 3  /* 0x03 */
#define PACKET_TYPE_CHECK    5  /* 0x05 */
#define PACKET_TYPE_ACK      6  /* 0x06 */
#define PACKET_TYPE_NAK      21 /* 0x15 */
#define PACKET_TYPE_TERM     24 /* 0x18 */

#define PACKET_SUBTYPE_CHECK_INIT    0 /* '00' */
#define PACKET_SUBTYPE_CHECK_REGULAR 1 /* '01' */

#define PACKET_SUBTYPE_ACK_BASIC             0 /* '00' */
#define PACKET_SUBTYPE_ACK_CONFIRM_OVERWRITE 1 /* '01' */
#define PACKET_SUBTYPE_ACK_EXTENDED          2 /* '02' */
#define PACKET_SUBTYPE_ACK_TERM              3 /* '03' */

#define PACKET_SUBTYPE_NAK_RESEND           1 /* '01' */
#define PACKET_SUBTYPE_NAK_OVERWRITE        2 /* '02' */
#define PACKET_SUBTYPE_NAK_REJECT_OVERWRITE 3 /* '03' */

#define PACKET_SUBTYPE_TERM_BASIC 0 /* '00' */

#define EXPECT_PACKET(TYPE, SUBTYPE) \
    if (link->protocol_state.seven.last_packet_type != (TYPE) \
        || link->protocol_state.seven.last_packet_subtype != (SUBTYPE)) { \
        msg(ll_info, \
            "Expected a packet of type %02X and subtype %02X, " \
            "got a packet of type %02X and subtype %02X.", \
            (TYPE), \
            (SUBTYPE), \
            link->protocol_state.seven.last_packet_type, \
            link->protocol_state.seven.last_packet_subtype); \
        return CAHUTE_ERROR_UNKNOWN; \
    }

#define EXPECT_PACKET_OR_FAIL(TYPE, SUBTYPE) \
    if (link->protocol_state.seven.last_packet_type != (TYPE) \
        || link->protocol_state.seven.last_packet_subtype != (SUBTYPE)) { \
        msg(ll_info, \
            "Expected a packet of type %02X and subtype %02X, " \
            "got a packet of type %02X and subtype %02X.", \
            (TYPE), \
            (SUBTYPE), \
            link->protocol_state.seven.last_packet_type, \
            link->protocol_state.seven.last_packet_subtype); \
        err = CAHUTE_ERROR_UNKNOWN; \
        goto fail; \
    }

#define EXPECT_BASIC_ACK \
    EXPECT_PACKET(PACKET_TYPE_ACK, PACKET_SUBTYPE_ACK_BASIC)
#define EXPECT_BASIC_ACK_OR_FAIL \
    EXPECT_PACKET_OR_FAIL(PACKET_TYPE_ACK, PACKET_SUBTYPE_ACK_BASIC)

/**
 * Copy a string from a payload to a buffer, while null-terminating it
 * and detecting \xFF characters as end of strings.
 *
 * SECURITY: The destination buffer is expected to be at least
 * ``max_size + 1`` long.
 *
 * @param bufp Pointer to the buffer pointer for where to copy the data.
 *        This method will increment the pointer to after the end of the
 *        copied string with the null terminator, so that other strings or
 *        pieces of data can be copied after.
 * @param raw Raw data from which to get the string.
 * @param max_size Maximum size to read from raw data.
 * @return Pointer to the obtained string.
 */
CAHUTE_INLINE(char *)
cahute_seven_store_string(void **bufp, cahute_u8 const *raw, size_t max_size) {
    char *buf = *bufp, *result = buf;

    for (; max_size--; raw++) {
        int byte = *raw;

        if (!byte || byte >= 128)
            break;

        *buf++ = byte;
    }

    *buf++ = '\0';
    *bufp = buf;
    return result;
}

/**
 * Obtain a 8-bit integer from raw data, if available.
 *
 * SECURITY: The raw buffer is expected to be at least 2 bytes long.
 *
 * @param buf Buffer from which to get the 8-bit integer.
 * @return Integer, or 0 if no integer could be decoded.
 */
CAHUTE_INLINE(int) cahute_get_byte_hex(cahute_u8 const *raw) {
    if (!IS_ASCII_HEX_DIGIT(raw[0]) || !IS_ASCII_HEX_DIGIT(raw[1]))
        return 0;

    return (ASCII_HEX_TO_NIBBLE(raw[0]) << 4) | ASCII_HEX_TO_NIBBLE(raw[1]);
}

/**
 * Obtain a 8-bit integer from raw data, if available.
 *
 * SECURITY: The raw buffer is expected to be at least 2 bytes long.
 *
 * @param buf Buffer from which to get the 8-bit integer.
 * @return Integer, or 0 if no integer could be decoded.
 */
CAHUTE_INLINE(int) cahute_get_byte_dec(cahute_u8 const *raw) {
    if (!isdigit(raw[0]) || !isdigit(raw[1]))
        return 0;

    return (raw[0] - '0') * 10 + (raw[1] - '0');
}

/**
 * Obtain a 32-bit integer from raw data, if available.
 *
 * SECURITY: The raw buffer is expected to be at least 8 bytes long.
 *
 * @param buf Buffer from which to get the 32-bit integer.
 * @return Integer, or 0 if no integer could be decoded.
 */
CAHUTE_INLINE(unsigned long) cahute_get_long_hex(cahute_u8 const *raw) {
    if (!IS_ASCII_HEX_DIGIT(raw[0]) || !IS_ASCII_HEX_DIGIT(raw[1])
        || !IS_ASCII_HEX_DIGIT(raw[2]) || !IS_ASCII_HEX_DIGIT(raw[3])
        || !IS_ASCII_HEX_DIGIT(raw[4]) || !IS_ASCII_HEX_DIGIT(raw[5])
        || !IS_ASCII_HEX_DIGIT(raw[6]) || !IS_ASCII_HEX_DIGIT(raw[7]))
        return 0;

    return (ASCII_HEX_TO_NIBBLE(raw[0]) << 28)
           | (ASCII_HEX_TO_NIBBLE(raw[1]) << 24)
           | (ASCII_HEX_TO_NIBBLE(raw[2]) << 20)
           | (ASCII_HEX_TO_NIBBLE(raw[3]) << 16)
           | (ASCII_HEX_TO_NIBBLE(raw[4]) << 12)
           | (ASCII_HEX_TO_NIBBLE(raw[5]) << 8)
           | (ASCII_HEX_TO_NIBBLE(raw[6]) << 4) | ASCII_HEX_TO_NIBBLE(raw[7]);
}

/**
 * Obtain a 32-bit integer from raw data, if available.
 *
 * SECURITY: The raw buffer is expected to be at least 8 bytes long.
 *
 * @param buf Buffer from which to get the 32-bit integer.
 * @return Integer, or 0 if no integer could be decoded.
 */
CAHUTE_INLINE(unsigned long) cahute_get_long_dec(cahute_u8 const *raw) {
    unsigned long x = 0;

    if (!isdigit(raw[0]) || !isdigit(raw[1]) || !isdigit(raw[2])
        || !isdigit(raw[3]) || !isdigit(raw[4]) || !isdigit(raw[5])
        || !isdigit(raw[6]) || !isdigit(raw[7]))
        return 0;

    x = (raw[0] - '0') * 10 + raw[1] - '0';
    x = x * 10 + raw[2] - '0';
    x = x * 10 + raw[3] - '0';
    x = x * 10 + raw[4] - '0';
    x = x * 10 + raw[5] - '0';
    x = x * 10 + raw[6] - '0';
    x = x * 10 + raw[7] - '0';

    return x;
}

/**
 * Apply 0x5C padding to source data and write to a destination buffer.
 *
 * SECURITY: The destination buffer is assumed to have at least data_size*2
 * bytes available. Assertions regarding the data size must be done in the
 * caller.
 *
 * @param buf Destination buffer.
 * @param data Source data to apply padding to.
 * @param data_size Size of the source data to apply padding to.
 * @return Size of the unpadded data.
 */
CAHUTE_INLINE(int)
cahute_seven_pad(cahute_u8 *buf, cahute_u8 const *data, size_t data_size) {
    cahute_u8 *orig = buf;
    cahute_u8 const *p;

    for (p = data; data_size--; p++) {
        int byte = *p;

        if (byte < 32) {
            *buf++ = '\\';
            *buf++ = 32 + byte;
        } else if (byte == '\\') {
            *buf++ = '\\';
            *buf++ = '\\';
        } else
            *buf++ = byte;
    }

    return (size_t)(buf - orig);
}

/**
 * Apply reverse 0x5C padding to source data and write to a destination buffer.
 *
 * SECURITY: The destination buffer is assumed to have at least data_size
 * bytes available (worst case scenario in which no padding is present).
 * Assertions regarding the data size must be done in the caller.
 *
 * @param buf Destination buffer.
 * @param data Source data to apply reverse padding to.
 * @param data_size Size of the source data to apply padding to.
 * @return Size of the padded data.
 */
CAHUTE_INLINE(int)
cahute_seven_unpad(cahute_u8 *buf, cahute_u8 const *data, size_t data_size) {
    cahute_u8 *orig = buf;
    cahute_u8 const *p;

    for (p = data; data_size--; p++) {
        int byte = *p;

        if (byte == '\\') {
            /* If we've arrived at the end, we ignore the char. */
            if (!data_size)
                break;

            byte = *++p;
            data_size--;

            *buf++ = byte == '\\' ? '\\' : byte - 32;
        } else
            *buf++ = byte;
    }

    return (size_t)(buf - orig);
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

/* ---
 * Basic packet exchange utilities.
 * --- */

/**
 * Receive and decode a Protocol 7.00 packet, and store it into the link.
 *
 * This function should not be used directly, but with
 * ``cahute_seven_send_and_receive``.
 *
 * SECURITY: This method expects the protocol buffer to be at least 1030
 * bytes long, as represented by the ``SEVEN_MINIMUM_BUFFER_SIZE``
 * constant. This should be guaranteed when initializing the link protocol
 * in ``link.c``.
 *
 * @param link Link to use to receive the Protocol 7.00 packet.
 * @return Cahute error.
 */
CAHUTE_LOCAL(int) cahute_seven_receive(cahute_link *link) {
    struct cahute_seven_state *state = &link->protocol_state.seven;
    cahute_u8 buf[540], *state_data = link->protocol_buffer;
    size_t packet_size, data_size = 0;
    int err;

    /* The packet is at least 6 bytes long: type (1 B)
     * + subtype (2 B) + ex (1 B) + checksum (2 B). */
    err = cahute_read_from_link(
        link,
        buf,
        6,
        TIMEOUT_PACKET_START,
        TIMEOUT_PACKET_CONTENTS
    );
    if (err)
        return err;

    /* We assume the packet is of basic or extended format from here.
     * We want to check the basic format of the packet, complete the raw
     * packet, and check the checksum before any other treatment. */
    if (!IS_ASCII_HEX_DIGIT(buf[1]) || !IS_ASCII_HEX_DIGIT(buf[2])
        || (buf[3] != '0' && buf[3] != '1')) {
        msg(ll_error, "Invalid format for the usual packet header.");
        msg(ll_info, "Data read so far is the following:");
        mem(ll_info, buf, 6);
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (buf[3] == '0')
        packet_size = 6;
    else {
        /* Packet is extended, there is at least 10 bytes: Type (1 B)
         * + Subtype (2 B) + Extended (1 B) + Data size (4 B)
         * + Checksum (2 B). */
        err = cahute_read_from_link(
            link,
            &buf[6],
            4,
            TIMEOUT_PACKET_CONTENTS,
            TIMEOUT_PACKET_CONTENTS
        );
        if (err == CAHUTE_ERROR_TIMEOUT_START)
            return CAHUTE_ERROR_TIMEOUT;
        if (err)
            return err;

        if (!IS_ASCII_HEX_DIGIT(buf[4]) || !IS_ASCII_HEX_DIGIT(buf[5])
            || !IS_ASCII_HEX_DIGIT(buf[6]) || !IS_ASCII_HEX_DIGIT(buf[7])) {
            msg(ll_error, "Invalid format for the data size.");
            msg(ll_info, "Data read so far is the following:");
            mem(ll_info, buf, 10);
            return CAHUTE_ERROR_UNKNOWN;
        }

        data_size =
            ((ASCII_HEX_TO_NIBBLE(buf[4]) << 12)
             | (ASCII_HEX_TO_NIBBLE(buf[5]) << 8)
             | (ASCII_HEX_TO_NIBBLE(buf[6]) << 4)
             | ASCII_HEX_TO_NIBBLE(buf[7]));

        if (data_size == 0 || data_size > 528
            || data_size > SEVEN_MINIMUM_BUFFER_SIZE) {
            msg(ll_error,
                "Invalid data size %" CAHUTE_PRIuSIZE
                " for the extended packet.",
                data_size);
            msg(ll_info, "Data read so far is the following:");
            mem(ll_info, buf, 10);

            if (data_size)
                cahute_skip_from_link(
                    link,
                    data_size,
                    TIMEOUT_PACKET_CONTENTS,
                    TIMEOUT_PACKET_CONTENTS
                );

            return CAHUTE_ERROR_SIZE;
        }

        /* We want to read the rest of the packet here, with the rest of the
         * data (since we've already read 2 bytes of data) and the checksum. */
        err = cahute_read_from_link(
            link,
            &buf[10],
            data_size,
            TIMEOUT_PACKET_CONTENTS,
            TIMEOUT_PACKET_CONTENTS
        );
        if (err == CAHUTE_ERROR_TIMEOUT_START)
            return CAHUTE_ERROR_TIMEOUT;
        if (err)
            return err;

        packet_size = 10 + data_size;
    }

    msg(ll_info, "Received packet data is the following:");
    mem(ll_info, buf, packet_size);

    if (!IS_ASCII_HEX_DIGIT(buf[packet_size - 2])
        || !IS_ASCII_HEX_DIGIT(buf[packet_size - 1])) {
        msg(ll_error, "Invalid checksum format for the following packet:");
        mem(ll_error, buf, packet_size);
        return CAHUTE_ERROR_CORRUPT;
    }

    /* We want to compute the checksum and check if it's valid or not. */
    {
        unsigned int obtained_checksum =
            ((ASCII_HEX_TO_NIBBLE(buf[packet_size - 2]) << 4)
             | ASCII_HEX_TO_NIBBLE(buf[packet_size - 1]));
        unsigned int computed_checksum =
            cahute_seven_checksum(&buf[1], packet_size - 3);

        if (obtained_checksum != computed_checksum) {
            msg(ll_error,
                "Obtained checksum 0x%02X does not match computed checksum "
                "0x%02X.",
                obtained_checksum,
                computed_checksum);
            return CAHUTE_ERROR_CORRUPT;
        }
    }

    /* Now that we've decoded data, we're able to parse it a bit better. */
    state->last_packet_type = buf[0];
    state->last_packet_subtype =
        ((ASCII_HEX_TO_NIBBLE(buf[1]) << 4) | ASCII_HEX_TO_NIBBLE(buf[2]));

    if (data_size) {
        link->protocol_buffer_size =
            cahute_seven_unpad(state_data, &buf[8], data_size);
    } else
        link->protocol_buffer_size = 0;

    return CAHUTE_OK;
}

#define SEND_FLAG_DISABLE_CHECKSUM 0x00000001 /* Disable checksum flow. */
#define SEND_FLAG_DISABLE_RECEIVE  0x00000002 /* Disable packet reception. */

/**
 * Send a raw Protocol 7.00 packet, receive a response and store it into
 * the link.
 *
 * As opposed to the basic send or receive functions, this function supports
 * receiving invalid checksums and re-sending packets in such cases.
 *
 * This function should not be used directly, but with either
 * ``cahute_seven_send_basic`` or ``cahute_seven_send_extended``.
 *
 * @param link Link to use to send and receive the Protocol 7.00 packet.
 * @param flags Flags, as or'd `SEND_FLAG_*` constants.
 * @param raw_packet Raw packet data to send.
 * @param raw_packet_size Size of the raw packet data to send.
 * @return Cahute error.
 */
CAHUTE_LOCAL(int)
cahute_seven_send_and_receive(
    cahute_link *link,
    unsigned long flags,
    cahute_u8 const *raw_packet,
    size_t raw_packet_size
) {
    int err, correct = 0;
    int tries = 3;

    if (flags & SEND_FLAG_DISABLE_CHECKSUM) {
        /* No retries and no checksum flow if this flag is on, we directly
         * return CAHUTE_ERROR_CORRUPT! */
        tries = 1;
    }

    for (; tries > 0; tries--) {
        msg(ll_info, "Sending the following packet to the device:");
        mem(ll_info, raw_packet, raw_packet_size);

        if ((err = cahute_write_to_link(link, raw_packet, raw_packet_size)))
            return err;

        if (flags & SEND_FLAG_DISABLE_RECEIVE) {
            /* We don't want to receive the response here, so we consider the
             * flow to be correct! */
            correct = 1;
            break;
        }

        msg(ll_info, "Packet sent successfully, now waiting for response.");
        if ((err = cahute_seven_receive(link)))
            return err;

        if (link->protocol_state.seven.last_packet_type == PACKET_TYPE_NAK
            && link->protocol_state.seven.last_packet_subtype
                   == PACKET_SUBTYPE_NAK_RESEND) {
            /* The checksum may have been invalidated by the medium, we want
             * to try to resend. */
            continue;
        }

        correct = 1;
        break;
    }

    if (!correct) {
        /* All attempts at sending the packet have unfortunately failed. */
        return CAHUTE_ERROR_CORRUPT;
    }

    return CAHUTE_OK;
}

/**
 * Send a basic Protocol 7.00 packet and receive its response.
 *
 * This function also takes care of receiving the associated response packet.
 *
 * @param link Link to use to send the Protocol 7.00 packet.
 * @param flags Flags, as or'd `SEND_FLAG_*` constants.
 * @param type Numeric type (*T*) of the packet to send.
 * @param subtype Numeric subtype (*ST*) of the packet to send.
 * @return Cahute error.
 */
CAHUTE_LOCAL(int)
cahute_seven_send_basic(
    cahute_link *link,
    unsigned long flags,
    int type,
    int subtype
) {
    cahute_u8 packet[6];

    packet[0] = type & 255;
    cahute_seven_set_ascii_hex(&packet[1], subtype);
    packet[3] = '0';
    cahute_seven_set_ascii_hex(
        &packet[4],
        cahute_seven_checksum(&packet[1], 3)
    );

    return cahute_seven_send_and_receive(link, flags, packet, 6);
}

/**
 * Send an extended Protocol 7.00 packet and receive its response.
 *
 * Note that this function only supports sending up to 1028 bytes (maximum
 * data packet size), and handles the 0x5C padding.
 *
 * This function also takes care of receiving the associated response packet.
 *
 * @param link Link to use to send the Protocol 7.00 packet.
 * @param flags Flags, as or'd `SEND_FLAG_*` constants.
 * @param type Numeric type (*T*) of the packet to send.
 * @param subtype Numeric subtype (*ST*) of the packet to send.
 * @param data Data to send.
 * @param data_size Size of the data to send.
 * @return Cahute error.
 */
CAHUTE_LOCAL(int)
cahute_seven_send_extended(
    cahute_link *link,
    unsigned long flags,
    int type,
    int subtype,
    cahute_u8 *data,
    size_t data_size
) {
    cahute_u8 packet[2066];

    if (data_size > 1028) {
        msg(ll_error,
            "Tried to send an extended Protocol 7.00 packet with more than "
            "1028o: %" CAHUTE_PRIuSIZE "o!",
            data_size);
        return CAHUTE_ERROR_UNKNOWN;
    }

    data_size = cahute_seven_pad(&packet[8], data, data_size);

    packet[0] = type & 255;
    cahute_seven_set_ascii_hex(&packet[1], subtype);
    packet[3] = '1';
    cahute_seven_set_ascii_hex(&packet[4], (data_size >> 8) & 255);
    cahute_seven_set_ascii_hex(&packet[6], data_size & 255);

    cahute_seven_set_ascii_hex(
        &packet[8 + data_size],
        cahute_seven_checksum(&packet[1], 7 + data_size)
    );

    return cahute_seven_send_and_receive(link, flags, packet, 10 + data_size);
}

/**
 * Send a command using Protocol 7.00 and get the response.
 *
 * @param link Link on which to send the command.
 * @param code Command code.
 * @param overwrite The overwrite mode, amongst the 'SEVEN_OVERWRITE_*'
 *        constants. Setting this to 0 is fine for most commands.
 * @param datatype Data type for main memory related commands.
 *        Setting this to 0 for other commands is fine.
 * @param filesize Announced file size when sending data.
 *        Setting this to 0 is fine for other commands.
 * @param param1 First parameter, NULL if not relevant.
 * @param param2 Second parameter, NULL if not relevant.
 * @param param3 Third parameter, NULL if not relevant.
 * @param param4 Fourth parameter, NULL if not relevant.
 * @param param5 Fifth parameter, NULL if not relevant.
 * @param param6 Sixth parameter, NULL if not relevant.
 * @return Cahute error, or 0 if no error has occurred.
 */
CAHUTE_LOCAL(int)
cahute_seven_send_command(
    cahute_link *link,
    int code,
    int overwrite,
    int datatype,
    unsigned long filesize,
    char const *param1,
    char const *param2,
    char const *param3,
    char const *param4,
    char const *param5,
    char const *param6
) {
    cahute_u8 buf[256], *p = buf;
    size_t length1 = param1 ? strlen(param1) : 0;
    size_t length2 = param2 ? strlen(param2) : 0;
    size_t length3 = param3 ? strlen(param3) : 0;
    size_t length4 = param4 ? strlen(param4) : 0;
    size_t length5 = param5 ? strlen(param5) : 0;
    size_t length6 = param6 ? strlen(param6) : 0;

    if (!overwrite && !datatype && !filesize && !param1 && !param2 && !param3
        && !param4 && !param5 && !param6) {
        /* Since we don't actually use the payload, we can just send a
         * basic packet here! */
        return cahute_seven_send_basic(link, 0, PACKET_TYPE_COMMAND, code);
    }

    if (length1 + length2 + length3 + length4 + length5 + length6 > 232) {
        msg(ll_error,
            "Combined lengths of the parameters cannot exceed 232 bytes!");
        return CAHUTE_ERROR_UNKNOWN;
    }

    /* We need to keep the last command code in case the command is followed
     * by a data transfer, in which the command code must be set as the
     * data packets' subtype. */
    link->protocol_state.seven.last_command = code;

    cahute_seven_set_ascii_hex(p, overwrite & 255);
    cahute_seven_set_ascii_hex(&p[2], datatype & 255);
    cahute_seven_set_ascii_hex(&p[4], (filesize >> 24) & 255);
    cahute_seven_set_ascii_hex(&p[6], (filesize >> 16) & 255);
    cahute_seven_set_ascii_hex(&p[8], (filesize >> 8) & 255);
    cahute_seven_set_ascii_hex(&p[10], filesize & 255);
    cahute_seven_set_ascii_hex(&p[12], length1 & 255);
    cahute_seven_set_ascii_hex(&p[14], length2 & 255);
    cahute_seven_set_ascii_hex(&p[16], length3 & 255);
    cahute_seven_set_ascii_hex(&p[18], length4 & 255);
    cahute_seven_set_ascii_hex(&p[20], length5 & 255);
    cahute_seven_set_ascii_hex(&p[22], length6 & 255);

    p += 24;

    if (length1) {
        memcpy(p, param1, length1);
        p += length1;
    }
    if (length2) {
        memcpy(p, param2, length2);
        p += length2;
    }
    if (length3) {
        memcpy(p, param3, length3);
        p += length3;
    }
    if (length4) {
        memcpy(p, param4, length4);
        p += length4;
    }
    if (length5) {
        memcpy(p, param5, length5);
        p += length5;
    }
    if (length6) {
        memcpy(p, param6, length6);
        p += length6;
    }

    return cahute_seven_send_extended(
        link,
        0,
        PACKET_TYPE_COMMAND,
        code,
        buf,
        (size_t)(p - buf)
    );
}

/**
 * Decode a command payload.
 *
 * @param link Link to use to decode the payload.
 * @param overwritep Pointer to the overwrite to define.
 * @param datatypep Pointer to the data type to define.
 * @param param1p Pointer to the first parameter pointer to define.
 * @param param1_sizep Pointer to the first parameter size to define.
 * @param param2p Pointer to the second parameter pointer to define.
 * @param param2_sizep Pointer to the second parameter size to define.
 * @param param3p Pointer to the third parameter pointer to define.
 * @param param3_sizep Pointer to the third parameter size to define.
 * @param param4p Pointer to the fourth parameter pointer to define.
 * @param param4_sizep Pointer to the fourth parameter size to define.
 * @param param5p Pointer to the fifth parameter pointer to define.
 * @param param5_sizep Pointer to the fifth parameter size to define.
 * @param param6p Pointer to the sixth parameter pointer to define.
 * @param param6_sizep Pointer to the sixth parameter size to define.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
cahute_seven_decode_command(
    cahute_link *link,
    int *overwritep,
    int *datatypep,
    unsigned long *filesizep,
    cahute_u8 const **param1p,
    size_t *param1_sizep,
    cahute_u8 const **param2p,
    size_t *param2_sizep,
    cahute_u8 const **param3p,
    size_t *param3_sizep,
    cahute_u8 const **param4p,
    size_t *param4_sizep,
    cahute_u8 const **param5p,
    size_t *param5_sizep,
    cahute_u8 const **param6p,
    size_t *param6_sizep
) {
    cahute_u8 const *buf = link->protocol_buffer;
    cahute_u8 const *param1 = NULL, *param2 = NULL, *param3 = NULL,
                    *param4 = NULL, *param5 = NULL, *param6 = NULL;
    size_t param1_size = 0, param2_size = 0, param3_size = 0, param4_size = 0,
           param5_size = 0, param6_size = 0;
    unsigned long filesize;
    int overwrite, datatype;

    if (link->protocol_buffer_size < 24) {
        msg(ll_error,
            "Protocol buffer too small (%" CAHUTE_PRIuSIZE " < 24).",
            link->protocol_buffer_size);
        return CAHUTE_ERROR_UNKNOWN;
    }

    /* All 24 first bytes must be ASCII-HEX. */
    if (!IS_ASCII_HEX_DIGIT(buf[0]) || !IS_ASCII_HEX_DIGIT(buf[1])
        || !IS_ASCII_HEX_DIGIT(buf[2]) || !IS_ASCII_HEX_DIGIT(buf[3])
        || !IS_ASCII_HEX_DIGIT(buf[4]) || !IS_ASCII_HEX_DIGIT(buf[5])
        || !IS_ASCII_HEX_DIGIT(buf[6]) || !IS_ASCII_HEX_DIGIT(buf[7])
        || !IS_ASCII_HEX_DIGIT(buf[8]) || !IS_ASCII_HEX_DIGIT(buf[9])
        || !IS_ASCII_HEX_DIGIT(buf[10]) || !IS_ASCII_HEX_DIGIT(buf[11])
        || !IS_ASCII_HEX_DIGIT(buf[12]) || !IS_ASCII_HEX_DIGIT(buf[13])
        || !IS_ASCII_HEX_DIGIT(buf[14]) || !IS_ASCII_HEX_DIGIT(buf[15])
        || !IS_ASCII_HEX_DIGIT(buf[16]) || !IS_ASCII_HEX_DIGIT(buf[17])
        || !IS_ASCII_HEX_DIGIT(buf[18]) || !IS_ASCII_HEX_DIGIT(buf[19])
        || !IS_ASCII_HEX_DIGIT(buf[20]) || !IS_ASCII_HEX_DIGIT(buf[21])
        || !IS_ASCII_HEX_DIGIT(buf[22]) || !IS_ASCII_HEX_DIGIT(buf[23]))
        return CAHUTE_ERROR_UNKNOWN;

    param1_size =
        (ASCII_HEX_TO_NIBBLE(buf[12]) << 4) | ASCII_HEX_TO_NIBBLE(buf[13]);
    param2_size =
        (ASCII_HEX_TO_NIBBLE(buf[14]) << 4) | ASCII_HEX_TO_NIBBLE(buf[15]);
    param3_size =
        (ASCII_HEX_TO_NIBBLE(buf[16]) << 4) | ASCII_HEX_TO_NIBBLE(buf[17]);
    param4_size =
        (ASCII_HEX_TO_NIBBLE(buf[18]) << 4) | ASCII_HEX_TO_NIBBLE(buf[19]);
    param5_size =
        (ASCII_HEX_TO_NIBBLE(buf[20]) << 4) | ASCII_HEX_TO_NIBBLE(buf[21]);
    param6_size =
        (ASCII_HEX_TO_NIBBLE(buf[22]) << 4) | ASCII_HEX_TO_NIBBLE(buf[23]);

    if (link->protocol_buffer_size
        != 24 + param1_size + param2_size + param3_size + param4_size
               + param5_size + param6_size)
        return CAHUTE_ERROR_UNKNOWN;

    overwrite =
        (ASCII_HEX_TO_NIBBLE(buf[0]) << 4) | ASCII_HEX_TO_NIBBLE(buf[1]);
    datatype =
        (ASCII_HEX_TO_NIBBLE(buf[2]) << 4) | ASCII_HEX_TO_NIBBLE(buf[3]);
    filesize =
        ((ASCII_HEX_TO_NIBBLE(buf[4]) << 28)
         | (ASCII_HEX_TO_NIBBLE(buf[5]) << 24)
         | (ASCII_HEX_TO_NIBBLE(buf[6]) << 20)
         | (ASCII_HEX_TO_NIBBLE(buf[7]) << 16)
         | (ASCII_HEX_TO_NIBBLE(buf[8]) << 12)
         | (ASCII_HEX_TO_NIBBLE(buf[9]) << 8)
         | (ASCII_HEX_TO_NIBBLE(buf[10]) << 4) | ASCII_HEX_TO_NIBBLE(buf[11]));

    param1 = &buf[24];
    param2 = param1 + param1_size;
    param3 = param2 + param2_size;
    param4 = param3 + param3_size;
    param5 = param4 + param4_size;
    param6 = param5 + param5_size;

    if (overwritep)
        *overwritep = overwrite;
    if (datatypep)
        *datatypep = datatype;
    if (filesizep)
        *filesizep = filesize;
    if (param1p)
        *param1p = param1;
    if (param1_sizep)
        *param1_sizep = param1_size;
    if (param2p)
        *param2p = param2;
    if (param2_sizep)
        *param2_sizep = param2_size;
    if (param3p)
        *param3p = param3;
    if (param3_sizep)
        *param3_sizep = param3_size;
    if (param4p)
        *param4p = param4;
    if (param4_sizep)
        *param4_sizep = param4_size;
    if (param5p)
        *param5p = param5;
    if (param5_sizep)
        *param5_sizep = param5_size;
    if (param6p)
        *param6p = param6;
    if (param6_sizep)
        *param6_sizep = param6_size;
    return CAHUTE_OK;
}

/* ---
 * Flow implementations.
 * --- */

/**
 * Initiate the Protocol 7.00 communication.
 *
 * For more information on this flow, see :ref:`seven-init-link`.
 *
 * @param link Link on which to initiate the Protocol 7.00 communication.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int) cahute_seven_initiate(cahute_link *link) {
    int err;

    err = cahute_seven_send_basic(
        link,
        0,
        PACKET_TYPE_CHECK,
        PACKET_SUBTYPE_CHECK_INIT
    );
    if (err)
        return err;

    if (link->protocol_state.seven.last_packet_type != PACKET_TYPE_ACK
        || link->protocol_state.seven.last_packet_subtype
               != PACKET_SUBTYPE_ACK_BASIC) {
        msg(ll_error, "Calculator did not answer a basic ACK.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    return CAHUTE_OK;
}

/**
 * Terminate the Protocol 7.00 communication.
 *
 * For more information on this flow, see :ref:`seven-terminate-link`.
 *
 * @param link Link on which to initiate the Protocol 7.00 communication.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int) cahute_seven_terminate(cahute_link *link) {
    int err;

    err = cahute_seven_send_basic(
        link,
        0,
        PACKET_TYPE_TERM,
        PACKET_SUBTYPE_TERM_BASIC
    );
    if (err)
        return err;

    EXPECT_BASIC_ACK;
    return CAHUTE_OK;
}

#define SEND_DATA_FLAG_DISABLE_SHIFTING 0x00000001 /* Disable shifting. */

/**
 * Send data from a FILE stream.
 *
 * Note that packet shifting is enabled only when not disabled explicitely
 * (e.g. for sensitive payloads, such as with command 0x56 "Upload and run"),
 * or when not on a reliable enough medium (i.e. not serial).
 *
 * Also note that the command code to use as data packet subtypes has already
 * been set as `link->protocol_state.seven.last_command` by
 * :c:func:`cahute_seven_send_command`, so we use that instead of requiring
 * it in the parameters.
 *
 * @param link Link with which to send the data.
 * @param flags OR'd `SEND_DATA_FLAG_*` constants.
 * @param filep FILE object to read data from.
 * @param size Size of the data to send.
 * @param progress_func Function to display progress.
 * @param progress_cookie Cookie to pass to the progress function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
cahute_seven_send_data(
    cahute_link *link,
    unsigned long flags,
    FILE *filep,
    size_t size,
    cahute_progress_func *progress_func,
    void *progress_cookie
) {
    cahute_u8 buf[264];
    size_t last_packet_size = size & 255;
    unsigned long packet_count = (size >> 8) + !!last_packet_size;
    unsigned long i, loop_send_flags = 0;
    int err, shifted = 0;

    last_packet_size = last_packet_size ? last_packet_size : 256;
    cahute_seven_set_ascii_hex(buf, (packet_count >> 8) & 255);
    cahute_seven_set_ascii_hex(&buf[2], packet_count & 255);

    if (packet_count >= 3 && (~link->flags & CAHUTE_LINK_FLAG_SERIAL)
        && (~flags & SEND_DATA_FLAG_DISABLE_SHIFTING)) {
        /* We are about to start packet shifting.
         * For more information, please consult the following:
         * https://cahuteproject.org/topics/protocols/seven/flows.html
         * #packet-shifting */
        buf[4] = '0';
        buf[5] = '0';
        buf[6] = '0';
        buf[7] = '1';

        if (!fread(&buf[8], 256, 1, filep)) {
            msg(ll_error,
                "Could not read file data: %s (%d)",
                strerror(errno),
                errno);
            return CAHUTE_ERROR_UNKNOWN;
        }

        err = cahute_seven_send_extended(
            link,
            SEND_FLAG_DISABLE_RECEIVE,
            PACKET_TYPE_DATA,
            link->protocol_state.seven.last_command,
            buf,
            264
        );
        if (err)
            return err;

        shifted = 1;
        loop_send_flags |= SEND_FLAG_DISABLE_CHECKSUM;

        if (progress_func)
            (*progress_func)(progress_cookie, 1, packet_count);
    }

    /* General loop for all packets except the last one. */
    for (i = 1 + shifted; i < packet_count; i++) {
        cahute_seven_set_ascii_hex(&buf[4], (i >> 8) & 255);
        cahute_seven_set_ascii_hex(&buf[6], i & 255);

        if (!fread(&buf[8], 256, 1, filep)) {
            msg(ll_error,
                "Could not read file data: %s (%d)",
                strerror(errno),
                errno);

            if (shifted) {
                msg(ll_error,
                    "An error has occurred while we were using packet "
                    "shifting; the link is now irrecoverable.");
                link->flags |= CAHUTE_LINK_FLAG_IRRECOVERABLE;
            }

            return CAHUTE_ERROR_UNKNOWN;
        }

        msg(ll_info, "Sending data packet %lu/%lu.", i, packet_count);
        err = cahute_seven_send_extended(
            link,
            loop_send_flags,
            PACKET_TYPE_DATA,
            link->protocol_state.seven.last_command,
            buf,
            264
        );
        if (err) {
            if (shifted) {
                msg(ll_error,
                    "An error has occurred while we were using packet "
                    "shifting; the link is now irrecoverable.");
                link->flags |= CAHUTE_LINK_FLAG_IRRECOVERABLE;
            }

            return err;
        }

        EXPECT_BASIC_ACK;

        if (progress_func)
            (*progress_func)(progress_cookie, i, packet_count);
    }

    /* If we have been using packet shifting, we want to normalize the
     * exchange before the last packet. */
    if (shifted) {
        if ((err = cahute_seven_receive(link)))
            return err;

        EXPECT_BASIC_ACK;
    }

    /* Send the last packet. */
    cahute_seven_set_ascii_hex(&buf[4], (packet_count >> 8) & 255);
    cahute_seven_set_ascii_hex(&buf[6], packet_count & 255);

    if (!fread(&buf[8], last_packet_size, 1, filep)) {
        msg(ll_error,
            "Could not read file data: %s (%d)",
            strerror(errno),
            errno);
        return CAHUTE_ERROR_UNKNOWN;
    }

    msg(ll_info,
        "Sending data packet %lu/%lu (last).",
        packet_count,
        packet_count);
    err = cahute_seven_send_extended(
        link,
        0,
        PACKET_TYPE_DATA,
        link->protocol_state.seven.last_command,
        buf,
        8 + last_packet_size
    );
    if (err)
        return err;

    if (link->protocol_state.seven.last_packet_type != PACKET_TYPE_ACK) {
        msg(ll_error, "Calculator did not answer with an ACK.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    switch (link->protocol_state.seven.last_packet_subtype) {
    case PACKET_SUBTYPE_ACK_BASIC:
        /* Basic (most common) case, where the link can resume. */
        break;

    case PACKET_SUBTYPE_ACK_TERM:
        /* The link is terminated at the end of the data exchange flow.
         * Apart from that, the packet flow went great! */
        msg(ll_info,
            "Calculator terminated the link following the data transfer.");

        link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
        break;

    default:
        msg(ll_error,
            "Unhandled ACK subtype %02X at the end of data transfer.",
            link->protocol_state.seven.last_packet_subtype);
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (progress_func)
        (*progress_func)(progress_cookie, packet_count, packet_count);

    return CAHUTE_OK;
}

/**
 * Send data from a buffer.
 *
 * Note that packet shifting is enabled only when not disabled explicitely
 * (e.g. for sensitive payloads, such as with command 0x56 "Upload and run"),
 * or when not on a reliable enough medium (i.e. not serial).
 *
 * Also note that the command code to use as data packet subtypes has already
 * been set as `link->protocol_state.seven.last_command` by
 * :c:func:`cahute_seven_send_command`, so we use that instead of requiring
 * it in the parameters.
 *
 * @param link Link with which to send the data.
 * @param flags OR'd `SEND_DATA_FLAG_*` constants.
 * @param data Buffer to read data from.
 * @param size Size of the data to send.
 * @param progress_func Function to display progress.
 * @param progress_cookie Cookie to pass to the progress function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
cahute_seven_send_data_from_buf(
    cahute_link *link,
    unsigned long flags,
    cahute_u8 const *data,
    size_t size,
    cahute_progress_func *progress_func,
    void *progress_cookie
) {
    cahute_u8 buf[264];
    size_t last_packet_size = size & 255;
    unsigned long packet_count = (size >> 8) + !!last_packet_size;
    unsigned long i, loop_send_flags = 0;
    int err, shifted = 0;

    last_packet_size = last_packet_size ? last_packet_size : 256;
    cahute_seven_set_ascii_hex(buf, (packet_count >> 8) & 255);
    cahute_seven_set_ascii_hex(&buf[2], packet_count & 255);

    if (packet_count >= 3 && (~link->flags & CAHUTE_LINK_FLAG_SERIAL)
        && (~flags & SEND_DATA_FLAG_DISABLE_SHIFTING)) {
        /* We are about to start packet shifting.
         * For more information, please consult the following:
         * https://cahuteproject.org/topics/protocols/seven/flows.html
         * #packet-shifting */
        buf[4] = '0';
        buf[5] = '0';
        buf[6] = '0';
        buf[7] = '1';

        memcpy(&buf[8], data, 256);
        data += 256;

        err = cahute_seven_send_extended(
            link,
            SEND_FLAG_DISABLE_RECEIVE,
            PACKET_TYPE_DATA,
            link->protocol_state.seven.last_command,
            buf,
            264
        );
        if (err)
            return err;

        shifted = 1;
        loop_send_flags |= SEND_FLAG_DISABLE_CHECKSUM;

        if (progress_func)
            (*progress_func)(progress_cookie, 1, packet_count);
    }

    /* General loop for all packets except the last one. */
    for (i = 1 + shifted; i < packet_count; i++) {
        cahute_seven_set_ascii_hex(&buf[4], (i >> 8) & 255);
        cahute_seven_set_ascii_hex(&buf[6], i & 255);
        memcpy(&buf[8], data, 256);
        data += 256;

        msg(ll_info, "Sending data packet %lu/%lu.", i, packet_count);
        err = cahute_seven_send_extended(
            link,
            loop_send_flags,
            PACKET_TYPE_DATA,
            link->protocol_state.seven.last_command,
            buf,
            264
        );
        if (err) {
            if (shifted) {
                msg(ll_error,
                    "An error has occurred while we were using packet "
                    "shifting; the link is now irrecoverable.");
                link->flags |= CAHUTE_LINK_FLAG_IRRECOVERABLE;
            }

            return err;
        }

        EXPECT_BASIC_ACK;

        if (progress_func)
            (*progress_func)(progress_cookie, i, packet_count);
    }

    /* If we have been using packet shifting, we want to normalize the
     * exchange before the last packet. */
    if (shifted) {
        if ((err = cahute_seven_receive(link)))
            return err;

        EXPECT_BASIC_ACK;
    }

    /* Send the last packet. */
    cahute_seven_set_ascii_hex(&buf[4], (packet_count >> 8) & 255);
    cahute_seven_set_ascii_hex(&buf[6], packet_count & 255);
    memcpy(&buf[8], data, last_packet_size);

    msg(ll_info,
        "Sending data packet %lu/%lu (last).",
        packet_count,
        packet_count);
    err = cahute_seven_send_extended(
        link,
        0,
        PACKET_TYPE_DATA,
        link->protocol_state.seven.last_command,
        buf,
        8 + last_packet_size
    );
    if (err)
        return err;

    if (link->protocol_state.seven.last_packet_type != PACKET_TYPE_ACK) {
        msg(ll_error, "Calculator did not answer with an ACK.");
        return CAHUTE_ERROR_UNKNOWN;
    }

    switch (link->protocol_state.seven.last_packet_subtype) {
    case PACKET_SUBTYPE_ACK_BASIC:
        /* Basic (most common) case, where the link can resume. */
        break;

    case PACKET_SUBTYPE_ACK_TERM:
        /* The link is terminated at the end of the data exchange flow.
         * Apart from that, the packet flow went great! */
        msg(ll_info,
            "Calculator terminated the link following the data transfer.");

        link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
        break;

    default:
        msg(ll_error,
            "Unhandled ACK subtype %02X at the end of data transfer.",
            link->protocol_state.seven.last_packet_subtype);
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (progress_func)
        (*progress_func)(progress_cookie, packet_count, packet_count);

    return CAHUTE_OK;
}

#define RECEIVE_DATA_FLAG_DISABLE_SHIFTING 0x00000001 /* Disable shifting. */

/**
 * Accept and receive data to a FILE stream.
 *
 * This command starts by sending ACK in order to accept the command that
 * is accompanied with data, then receives and acknowledges all data packets
 * with the exception of the last one. This way, the caller can send a
 * different acknowledgement (e.g. with subtype '03'), or check that it
 * receives a roleswap or another command.
 *
 * @param link Link with which to receive the data.
 * @param flags Flags.
 * @param filep FILE object to write data to.
 * @param size Size of the data to receive.
 * @param code Command code of the corresponding flow.
 * @param progress_func Function to display progress.
 * @param progress_cookie Cookie to pass to the progress function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
cahute_seven_receive_data(
    cahute_link *link,
    unsigned long flags,
    FILE *filep,
    size_t size,
    int command_code,
    cahute_progress_func *progress_func,
    void *progress_cookie
) {
    cahute_u8 const *buf = link->protocol_buffer;
    unsigned long packet_count = 0;
    unsigned int i;
    int err;

    for (i = 1; size; i++) {
        unsigned int read_packet_count, read_packet_i;

        msg(ll_info, "Requesting packet %u/%u.", i, packet_count);

        err = cahute_seven_send_basic(
            link,
            0,
            PACKET_TYPE_ACK,
            PACKET_SUBTYPE_ACK_BASIC
        );
        if (err)
            return err;

        EXPECT_PACKET(PACKET_TYPE_DATA, command_code);
        if (link->protocol_buffer_size < 9) {
            msg(ll_error,
                "Data packet doesn't contain metadata and at least one byte.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        if (!IS_ASCII_HEX_DIGIT(buf[0]) || !IS_ASCII_HEX_DIGIT(buf[1])
            || !IS_ASCII_HEX_DIGIT(buf[2]) || !IS_ASCII_HEX_DIGIT(buf[3])
            || !IS_ASCII_HEX_DIGIT(buf[4]) || !IS_ASCII_HEX_DIGIT(buf[5])
            || !IS_ASCII_HEX_DIGIT(buf[6]) || !IS_ASCII_HEX_DIGIT(buf[7])) {
            msg(ll_error, "Data packet has invalid format.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        read_packet_i =
            ((ASCII_HEX_TO_NIBBLE(buf[4]) << 12)
             | (ASCII_HEX_TO_NIBBLE(buf[5]) << 8)
             | (ASCII_HEX_TO_NIBBLE(buf[6]) << 4)
             | ASCII_HEX_TO_NIBBLE(buf[7]));
        if (read_packet_i != i) {
            msg(ll_error,
                "Unexpected sequence number (expected %u, got %u)",
                i,
                read_packet_i);
            return CAHUTE_ERROR_UNKNOWN;
        }

        read_packet_count =
            ((ASCII_HEX_TO_NIBBLE(buf[0]) << 12)
             | (ASCII_HEX_TO_NIBBLE(buf[1]) << 8)
             | (ASCII_HEX_TO_NIBBLE(buf[2]) << 4)
             | ASCII_HEX_TO_NIBBLE(buf[3]));
        if (i == 1)
            packet_count = read_packet_count;
        else if (read_packet_count != packet_count) {
            msg(ll_error,
                "Packet count was not consistent between packets "
                "(initial: 1/%u, current: %u/%u)",
                packet_count,
                i,
                read_packet_count);
            return CAHUTE_ERROR_UNKNOWN;
        }

        size_t current_size = link->protocol_buffer_size - 8;
        if (i < read_packet_count) {
            if (current_size >= size) {
                msg(ll_error,
                    "Packet too much data for the expected total size of "
                    "the data flow (expected: %" CAHUTE_PRIuSIZE
                    ", got: %" CAHUTE_PRIuSIZE ")",
                    size,
                    current_size);
                return CAHUTE_ERROR_UNKNOWN;
            }
        } else if (current_size < size) {
            msg(ll_error,
                "Last packet did not contain enough bytes to finish the "
                "data flow (expected: %" CAHUTE_PRIuSIZE
                ", got: %" CAHUTE_PRIuSIZE ").",
                size,
                current_size);
            return CAHUTE_ERROR_UNKNOWN;
        } else if (current_size > size) {
            msg(ll_error,
                "Last packet contained too many bytes to finish the data "
                "flow (expected: %" CAHUTE_PRIuSIZE ", got: %" CAHUTE_PRIuSIZE
                " )",
                size,
                current_size);
            return CAHUTE_ERROR_UNKNOWN;
        }

        /* Write what is in the current packet. */
        if (!fwrite(&buf[8], current_size, 1, filep)) {
            msg(ll_error,
                "Could not write file data: %s (%d)",
                strerror(errno),
                errno);
            return CAHUTE_ERROR_UNKNOWN;
        }

        size -= current_size;

        if (progress_func)
            (*progress_func)(progress_cookie, i, packet_count);
    }

    return CAHUTE_OK;
}

/**
 * Accept and receive data to a buffer.
 *
 * This command starts by sending ACK in order to accept the command that
 * is accompanied with data, then receives and acknowledges all data packets
 * with the exception of the last one. This way, the caller can send a
 * different acknowledgement (e.g. with subtype '03'), or check that it
 * receives a roleswap or another command.
 *
 * @param link Link with which to receive the data.
 * @param flags Flags.
 * @param buf Buffer into which to write the received data.
 * @param size Expected and buffer size.
 * @param command_code Command code of the corresponding flow.
 * @param progress_func Function to display progress.
 * @param progress_cookie Cookie to pass to the progress function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
cahute_seven_receive_data_into_buf(
    cahute_link *link,
    unsigned long flags,
    cahute_u8 *buf,
    size_t size,
    int command_code,
    cahute_progress_func *progress_func,
    void *progress_cookie
) {
    cahute_u8 const *p_buf = link->protocol_buffer;
    unsigned long packet_count = 0;
    unsigned long read_packet_count, read_packet_i;
    unsigned long i, loop_send_flags = 0;
    size_t current_size;
    int err, shifted = 0;

    if (!size)
        return CAHUTE_OK;

    /* Read the first data packet to determine the number of packets. */
    {
        msg(ll_info, "Requesting first packet.");
        err = cahute_seven_send_basic(
            link,
            0,
            PACKET_TYPE_ACK,
            PACKET_SUBTYPE_ACK_BASIC
        );
        if (err)
            return err;

        EXPECT_PACKET(PACKET_TYPE_DATA, command_code);
        if (link->protocol_buffer_size < 9) {
            msg(ll_error,
                "Data packet doesn't contain metadata and at least one byte.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        if (!IS_ASCII_HEX_DIGIT(p_buf[0]) || !IS_ASCII_HEX_DIGIT(p_buf[1])
            || !IS_ASCII_HEX_DIGIT(p_buf[2]) || !IS_ASCII_HEX_DIGIT(p_buf[3])
            || !IS_ASCII_HEX_DIGIT(p_buf[4]) || !IS_ASCII_HEX_DIGIT(p_buf[5])
            || !IS_ASCII_HEX_DIGIT(p_buf[6])
            || !IS_ASCII_HEX_DIGIT(p_buf[7])) {
            msg(ll_error, "Data packet has invalid format.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        read_packet_i = (ASCII_HEX_TO_NIBBLE(p_buf[4]) << 12)
                        | (ASCII_HEX_TO_NIBBLE(p_buf[5]) << 8)
                        | (ASCII_HEX_TO_NIBBLE(p_buf[6]) << 4)
                        | ASCII_HEX_TO_NIBBLE(p_buf[7]);
        if (read_packet_i != 1) {
            msg(ll_error,
                "Unexpected sequence number %u for first packet.",
                read_packet_i);
            return CAHUTE_ERROR_UNKNOWN;
        }

        read_packet_count = (ASCII_HEX_TO_NIBBLE(p_buf[0]) << 12)
                            | (ASCII_HEX_TO_NIBBLE(p_buf[1]) << 8)
                            | (ASCII_HEX_TO_NIBBLE(p_buf[2]) << 4)
                            | ASCII_HEX_TO_NIBBLE(p_buf[3]);
        if (!read_packet_count) {
            msg(ll_info,
                "Unexpected packet count %u in first packet.",
                read_packet_count);
            return CAHUTE_ERROR_UNKNOWN;
        }

        packet_count = read_packet_count;

        current_size = link->protocol_buffer_size - 8;
        if (current_size >= size) {
            msg(ll_error,
                "Packet too much data for the expected total size of "
                "the data flow (expected: %" CAHUTE_PRIuSIZE
                ", got: %" CAHUTE_PRIuSIZE ")",
                size,
                current_size);
            return CAHUTE_ERROR_UNKNOWN;
        }

        /* Write what is in the current packet. */
        memcpy(buf, &p_buf[8], current_size);
        buf += current_size;
        size -= current_size;

        if (progress_func)
            (*progress_func)(progress_cookie, 1, packet_count);
    }

    /* If the conditions are met, start packet shifting! */
    if (packet_count >= 3 && (~link->flags & CAHUTE_LINK_FLAG_SERIAL)
        && (~flags & RECEIVE_DATA_FLAG_DISABLE_SHIFTING)) {
        /* We are about to start packet shifting.
         * For more information, please consult the following:
         * https://cahuteproject.org/topics/protocols/seven/flows.html
         * #packet-shifting */
        err = cahute_seven_send_basic(
            link,
            SEND_FLAG_DISABLE_RECEIVE,
            PACKET_TYPE_ACK,
            PACKET_SUBTYPE_ACK_BASIC
        );
        if (err)
            return err;

        shifted = 1;
        loop_send_flags |= SEND_FLAG_DISABLE_CHECKSUM;
    }

    /* Read all middle packets in the flow. */
    for (i = 2; size && i < read_packet_count - shifted; i++) {
        msg(ll_info, "Requesting packet %u/%u.", i, packet_count);

        err = cahute_seven_send_basic(
            link,
            loop_send_flags,
            PACKET_TYPE_ACK,
            PACKET_SUBTYPE_ACK_BASIC
        );
        if (err)
            return err;

        EXPECT_PACKET(PACKET_TYPE_DATA, command_code);
        if (link->protocol_buffer_size < 9) {
            msg(ll_error,
                "Data packet doesn't contain metadata and at least one byte.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        if (!IS_ASCII_HEX_DIGIT(p_buf[0]) || !IS_ASCII_HEX_DIGIT(p_buf[1])
            || !IS_ASCII_HEX_DIGIT(p_buf[2]) || !IS_ASCII_HEX_DIGIT(p_buf[3])
            || !IS_ASCII_HEX_DIGIT(p_buf[4]) || !IS_ASCII_HEX_DIGIT(p_buf[5])
            || !IS_ASCII_HEX_DIGIT(p_buf[6])
            || !IS_ASCII_HEX_DIGIT(p_buf[7])) {
            msg(ll_error, "Data packet has invalid format.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        read_packet_i =
            ((ASCII_HEX_TO_NIBBLE(p_buf[4]) << 12)
             | (ASCII_HEX_TO_NIBBLE(p_buf[5]) << 8)
             | (ASCII_HEX_TO_NIBBLE(p_buf[6]) << 4)
             | ASCII_HEX_TO_NIBBLE(p_buf[7]));
        if (read_packet_i != i) {
            msg(ll_error,
                "Unexpected sequence number (expected %u, got %u)",
                i,
                read_packet_i);
            return CAHUTE_ERROR_UNKNOWN;
        }

        read_packet_count =
            ((ASCII_HEX_TO_NIBBLE(p_buf[0]) << 12)
             | (ASCII_HEX_TO_NIBBLE(p_buf[1]) << 8)
             | (ASCII_HEX_TO_NIBBLE(p_buf[2]) << 4)
             | ASCII_HEX_TO_NIBBLE(p_buf[3]));
        if (read_packet_count != packet_count) {
            msg(ll_error,
                "Packet count was not consistent between packets "
                "(initial: 1/%u, current: %u/%u)",
                packet_count,
                i,
                read_packet_count);
            return CAHUTE_ERROR_UNKNOWN;
        }

        current_size = link->protocol_buffer_size - 8;
        if (current_size >= size) {
            msg(ll_error,
                "Packet too much data for the expected total size of "
                "the data flow (expected: %" CAHUTE_PRIuSIZE
                ", got: %" CAHUTE_PRIuSIZE ")",
                size,
                current_size);
            return CAHUTE_ERROR_UNKNOWN;
        }

        /* Write what is in the current packet. */
        memcpy(buf, &p_buf[8], current_size);
        buf += current_size;
        size -= current_size;

        if (progress_func)
            (*progress_func)(progress_cookie, i, packet_count);
    }

    /* If we have been using packet shifting, we want to normalize the
     * exchange before the last packet. */
    if (shifted) {
        msg(ll_info, "Requesting packet %u/%u.", packet_count - 1, packet_count
        );

        if ((err = cahute_seven_receive(link)))
            return err;

        EXPECT_PACKET(PACKET_TYPE_DATA, command_code);
        if (link->protocol_buffer_size < 9) {
            msg(ll_error,
                "Data packet doesn't contain metadata and at least one byte.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        if (!IS_ASCII_HEX_DIGIT(p_buf[0]) || !IS_ASCII_HEX_DIGIT(p_buf[1])
            || !IS_ASCII_HEX_DIGIT(p_buf[2]) || !IS_ASCII_HEX_DIGIT(p_buf[3])
            || !IS_ASCII_HEX_DIGIT(p_buf[4]) || !IS_ASCII_HEX_DIGIT(p_buf[5])
            || !IS_ASCII_HEX_DIGIT(p_buf[6])
            || !IS_ASCII_HEX_DIGIT(p_buf[7])) {
            msg(ll_error, "Data packet has invalid format.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        read_packet_i =
            ((ASCII_HEX_TO_NIBBLE(p_buf[4]) << 12)
             | (ASCII_HEX_TO_NIBBLE(p_buf[5]) << 8)
             | (ASCII_HEX_TO_NIBBLE(p_buf[6]) << 4)
             | ASCII_HEX_TO_NIBBLE(p_buf[7]));
        if (read_packet_i != packet_count - 1) {
            msg(ll_error,
                "Unexpected sequence number (expected %u, got %u)",
                packet_count - 1,
                read_packet_i);
            return CAHUTE_ERROR_UNKNOWN;
        }

        read_packet_count =
            ((ASCII_HEX_TO_NIBBLE(p_buf[0]) << 12)
             | (ASCII_HEX_TO_NIBBLE(p_buf[1]) << 8)
             | (ASCII_HEX_TO_NIBBLE(p_buf[2]) << 4)
             | ASCII_HEX_TO_NIBBLE(p_buf[3]));
        if (read_packet_count != packet_count) {
            msg(ll_error,
                "Packet count was not consistent between packets "
                "(initial: 1/%u, current: %u/%u)",
                packet_count,
                packet_count - 1,
                read_packet_count);
            return CAHUTE_ERROR_UNKNOWN;
        }

        current_size = link->protocol_buffer_size - 8;
        if (current_size >= size) {
            msg(ll_error,
                "Packet too much data for the expected total size of "
                "the data flow (expected: %" CAHUTE_PRIuSIZE
                ", got: %" CAHUTE_PRIuSIZE ")",
                size,
                current_size);
            return CAHUTE_ERROR_UNKNOWN;
        }

        /* Write what is in the current packet. */
        memcpy(buf, &p_buf[8], current_size);
        buf += current_size;
        size -= current_size;

        if (progress_func)
            (*progress_func)(progress_cookie, packet_count - 1, packet_count);
    }

    /* Read the last data packet. */
    if (packet_count > 1) {
        msg(ll_info, "Requesting packet %u/%u.", packet_count, packet_count);

        err = cahute_seven_send_basic(
            link,
            loop_send_flags,
            PACKET_TYPE_ACK,
            PACKET_SUBTYPE_ACK_BASIC
        );
        if (err)
            return err;

        EXPECT_PACKET(PACKET_TYPE_DATA, command_code);
        if (link->protocol_buffer_size < 9) {
            msg(ll_error,
                "Data packet doesn't contain metadata and at least one byte.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        if (!IS_ASCII_HEX_DIGIT(p_buf[0]) || !IS_ASCII_HEX_DIGIT(p_buf[1])
            || !IS_ASCII_HEX_DIGIT(p_buf[2]) || !IS_ASCII_HEX_DIGIT(p_buf[3])
            || !IS_ASCII_HEX_DIGIT(p_buf[4]) || !IS_ASCII_HEX_DIGIT(p_buf[5])
            || !IS_ASCII_HEX_DIGIT(p_buf[6])
            || !IS_ASCII_HEX_DIGIT(p_buf[7])) {
            msg(ll_error, "Data packet has invalid format.");
            return CAHUTE_ERROR_UNKNOWN;
        }

        read_packet_i =
            ((ASCII_HEX_TO_NIBBLE(p_buf[4]) << 12)
             | (ASCII_HEX_TO_NIBBLE(p_buf[5]) << 8)
             | (ASCII_HEX_TO_NIBBLE(p_buf[6]) << 4)
             | ASCII_HEX_TO_NIBBLE(p_buf[7]));
        if (read_packet_i != packet_count) {
            msg(ll_error,
                "Unexpected sequence number (expected %u, got %u)",
                i,
                read_packet_i);
            return CAHUTE_ERROR_UNKNOWN;
        }

        read_packet_count =
            ((ASCII_HEX_TO_NIBBLE(p_buf[0]) << 12)
             | (ASCII_HEX_TO_NIBBLE(p_buf[1]) << 8)
             | (ASCII_HEX_TO_NIBBLE(p_buf[2]) << 4)
             | ASCII_HEX_TO_NIBBLE(p_buf[3]));
        if (read_packet_count != packet_count) {
            msg(ll_error,
                "Packet count was not consistent between packets "
                "(initial: 1/%u, current: %u/%u)",
                packet_count,
                i,
                read_packet_count);
            return CAHUTE_ERROR_UNKNOWN;
        }

        current_size = link->protocol_buffer_size - 8;
        if (current_size < size) {
            msg(ll_error,
                "Last packet did not contain enough bytes to finish the "
                "data flow (expected: %" CAHUTE_PRIuSIZE
                ", got: %" CAHUTE_PRIuSIZE ").",
                size,
                current_size);
            return CAHUTE_ERROR_UNKNOWN;
        } else if (current_size > size) {
            msg(ll_error,
                "Last packet contained too many bytes to finish the data "
                "flow (expected: %" CAHUTE_PRIuSIZE ", got: %" CAHUTE_PRIuSIZE
                " )",
                size,
                current_size);
            return CAHUTE_ERROR_UNKNOWN;
        }

        /* Write what is in the current packet. */
        memcpy(buf, &p_buf[8], current_size);

        if (progress_func)
            (*progress_func)(progress_cookie, i, packet_count);
    }

    return CAHUTE_OK;
}

/**
 * Request device information using Protocol 7.00.
 *
 * This stores the raw information in ``raw_device_info`` from the link's
 * Protocol 7.00 peer state, so that it can be exploited later if need be.
 *
 * For more information on this flow, see :ref:`seven-get-device-information`.
 *
 * @param link Link on which to request device information.
 * @return Cahute error.
 */
CAHUTE_EXTERN(int) cahute_seven_discover(cahute_link *link) {
    int err;

    err = cahute_seven_send_command(
        link,
        0x01,
        0,
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    );
    if (err)
        return err;

    EXPECT_PACKET(PACKET_TYPE_ACK, PACKET_SUBTYPE_ACK_EXTENDED);

    if (link->protocol_buffer_size > SEVEN_RAW_DEVICE_INFO_BUFFER_SIZE) {
        msg(ll_error,
            "Could not store obtained device information (got "
            "%" CAHUTE_PRIuSIZE "/%" CAHUTE_PRIuSIZE " bytes)",
            link->protocol_buffer_size,
            SEVEN_RAW_DEVICE_INFO_BUFFER_SIZE);
        return CAHUTE_ERROR_SIZE;
    }

    memcpy(
        link->protocol_state.seven.raw_device_info,
        link->protocol_buffer,
        link->protocol_buffer_size
    );
    link->protocol_state.seven.raw_device_info_size =
        link->protocol_buffer_size;
    link->protocol_state.seven.flags |= SEVEN_FLAG_DEVICE_INFO_REQUESTED;

    return CAHUTE_OK;
}

/**
 * Produce generic device information using the optionally stored
 * EACK information from discovery.
 *
 * For more information on this flow, see :ref:`seven-get-device-information`.
 *
 * @param link Link from which to get the cached EACK response.
 * @param infop Pointer to set to the allocated device information structure.
 * @return Cahute error, or 0 if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_seven_make_device_info(cahute_link *link, cahute_device_info **infop) {
    cahute_device_info *info = NULL;
    void *buf;
    cahute_u8 const *raw_info;
    size_t raw_info_size;

    if (~link->protocol_state.seven.flags & SEVEN_FLAG_DEVICE_INFO_REQUESTED) {
        /* We don't have a 'generic device information' if discovery has
         * been disabled. */
        return CAHUTE_ERROR_IMPL;
    }

    info = malloc(sizeof(cahute_device_info) + 200);
    if (!info)
        return CAHUTE_ERROR_ALLOC;

    buf = (void *)(&info[1]);
    raw_info = link->protocol_state.seven.raw_device_info;
    raw_info_size = link->protocol_state.seven.raw_device_info_size;

    info->cahute_device_info_flags = 0;
    if (raw_info[50] == '.')
        info->cahute_device_info_flags |= CAHUTE_DEVICE_INFO_FLAG_PREPROG;
    if (raw_info[66] == '.')
        info->cahute_device_info_flags |= CAHUTE_DEVICE_INFO_FLAG_BOOTCODE;
    if (raw_info[98] == '.')
        info->cahute_device_info_flags |= CAHUTE_DEVICE_INFO_FLAG_OS;

    info->cahute_device_info_hwid =
        cahute_seven_store_string(&buf, raw_info, 8);
    info->cahute_device_info_cpuid =
        cahute_seven_store_string(&buf, &raw_info[8], 16);
    info->cahute_device_info_rom_capacity =
        cahute_get_long_dec(&raw_info[24]) * 1024;
    info->cahute_device_info_flash_rom_capacity =
        cahute_get_long_dec(&raw_info[32]) * 1024;
    info->cahute_device_info_ram_capacity =
        cahute_get_long_dec(&raw_info[40]) * 1024;

    info->cahute_device_info_rom_version =
        cahute_seven_store_string(&buf, &raw_info[48], 16);

    info->cahute_device_info_bootcode_version =
        cahute_seven_store_string(&buf, &raw_info[64], 16);
    info->cahute_device_info_bootcode_offset =
        cahute_get_long_hex(&raw_info[80]);
    info->cahute_device_info_bootcode_size =
        cahute_get_long_dec(&raw_info[88]) * 1024;

    info->cahute_device_info_os_version =
        cahute_seven_store_string(&buf, &raw_info[96], 16);
    info->cahute_device_info_os_offset = cahute_get_long_hex(&raw_info[112]);
    info->cahute_device_info_os_size =
        cahute_get_long_dec(&raw_info[120]) * 1024;
    info->cahute_device_info_product_id =
        cahute_seven_store_string(&buf, &raw_info[132], 16);

    if (raw_info_size == 164) {
        info->cahute_device_info_username =
            cahute_seven_store_string(&buf, &raw_info[148], 16);
        info->cahute_device_info_organisation = "";
    } else {
        info->cahute_device_info_username =
            cahute_seven_store_string(&buf, &raw_info[148], 20);
        info->cahute_device_info_organisation =
            cahute_seven_store_string(&buf, &raw_info[168], 20);
    }

    *infop = info;
    return CAHUTE_OK;
}

/**
 * Negotiate new serial parameters with the passive side.
 *
 * @param link Link to write to.
 * @param flags Serial flags to negotiate.
 * @param speed Speed to negotiate.
 * @return Cahute error, or 0 if no error has occurred.
 */
CAHUTE_EXTERN(int)
cahute_seven_negotiate_serial_params(
    cahute_link *link,
    unsigned long flags,
    unsigned long speed
) {
    char baudrate_buf[20];
    char const *parity, *stopbits;
    int err;

    sprintf(baudrate_buf, "%lu", speed);

    switch (flags & CAHUTE_SERIAL_PARITY_MASK) {
    case CAHUTE_SERIAL_PARITY_EVEN:
        parity = "EVEN";
        break;
    case CAHUTE_SERIAL_PARITY_ODD:
        parity = "ODD";
        break;
    default:
        parity = "NONE";
        break;
    }

    switch (flags & CAHUTE_SERIAL_STOP_MASK) {
    case CAHUTE_SERIAL_STOP_TWO:
        stopbits = "2";
        break;
    default:
        stopbits = "1";
        break;
    }

    err = cahute_seven_send_command(
        link,
        0x02,
        0,
        0,
        0,
        baudrate_buf,
        parity,
        stopbits,
        NULL,
        NULL,
        NULL
    );
    if (err)
        return err;

    EXPECT_BASIC_ACK;
    return CAHUTE_OK;
}

/* ---
 * Use case implementations.
 * --- */

/**
 * Request the available capacity on a storage device on the calculator.
 *
 * @param link Link on which to send the file.
 * @param storage Name of the storage device.
 * @param capacityp Pointer to the capacity to set.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_seven_request_storage_capacity(
    cahute_link *link,
    char const *storage,
    unsigned long *capacityp
) {
    int err;
    unsigned long capacity;

    err = cahute_seven_send_command(
        link,
        0x4B,
        0,
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        storage,
        NULL
    );
    if (err)
        return err;

    EXPECT_BASIC_ACK;

    err = cahute_seven_send_basic(link, 0, PACKET_TYPE_ROLESWAP, 0);
    if (err)
        return err;

    EXPECT_PACKET(PACKET_TYPE_COMMAND, 0x4C);
    err = cahute_seven_decode_command(
        link,
        NULL,      /* Overwrite. */
        NULL,      /* Data type. */
        &capacity, /* File size. */
        NULL,
        NULL, /* D1 */
        NULL,
        NULL, /* D2 */
        NULL,
        NULL, /* D3 */
        NULL,
        NULL, /* D4 */
        NULL,
        NULL, /* D5 */
        NULL,
        NULL /* D6 */
    );
    if (err)
        return err;

    err = cahute_seven_send_basic(
        link,
        0,
        PACKET_TYPE_ACK,
        PACKET_SUBTYPE_ACK_BASIC
    );
    if (err)
        return err;

    EXPECT_PACKET(PACKET_TYPE_ROLESWAP, 0);

    if (capacityp)
        *capacityp = capacity;

    return CAHUTE_OK;
}

/**
 * Request for a storage device to be optimized by the calculator.
 *
 * @param link Link on which to make the request.
 * @param storage Name of the storage device.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_seven_optimize_storage(cahute_link *link, char const *storage) {
    int err;

    err = cahute_seven_send_command(
        link,
        0x51,
        0,
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        storage,
        NULL
    );
    if (err)
        return err;

    EXPECT_BASIC_ACK;
    return CAHUTE_OK;
}

/**
 * Send a file to storage on the calculator.
 *
 * @param link Link on which to send the file.
 * @param flags File sending flags.
 * @param directory Optional name of the directory.
 * @param name Name of the file.
 * @param storage Name of the storage device.
 * @param filep File pointer to read from.
 * @param file_size Size of the file.
 * @param overwrite_func Overwrite confirmation function.
 * @param overwrite_cookie Cookie to pass to the overwrite confirmation
 *        function.
 * @param progress_func Function to call to signify progress.
 * @param progress_cookie Cookie to pass to the progress function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_seven_send_file_to_storage(
    cahute_link *link,
    unsigned long flags,
    char const *directory,
    char const *name,
    char const *storage,
    FILE *filep,
    size_t file_size,
    cahute_confirm_overwrite_func *overwrite_func,
    void *overwrite_cookie,
    cahute_progress_func *progress_func,
    void *progress_cookie
) {
    int err, should_upload_data = 1;

    if (flags & CAHUTE_SEND_FILE_FLAG_OPTIMIZE) {
        unsigned long capacity = 0;

        msg(ll_info, "Requesting storage capacity.");
        err = cahute_seven_request_storage_capacity(link, storage, &capacity);
        if (err)
            return err;

        msg(ll_info, "Storage capacity is %lud.", capacity);
        if ((size_t)capacity < file_size) {
            msg(ll_info, "Storage capacity is insufficient for file!.");
            msg(ll_info, "Requesting storage optimization.");
            err = cahute_seven_optimize_storage(link, storage);
            if (err)
                return err;
        }
    }

    err = cahute_seven_send_command(
        link,
        0x45,
        flags & CAHUTE_SEND_FILE_FLAG_FORCE ? 2 : 0, /* Overwrite mode. */
        0,
        file_size & 0xFFFFFFFF,
        directory,
        name,
        NULL,
        NULL,
        storage,
        NULL
    );
    if (err)
        return err;

    if (link->protocol_state.seven.last_packet_type == PACKET_TYPE_NAK
        && link->protocol_state.seven.last_packet_subtype
               == PACKET_SUBTYPE_NAK_OVERWRITE) {
        /* The device is currently requesting whether we want to overwrite
          * an existing file, which means we want to check on our side. */
        int should_overwrite = 0;

        if (overwrite_func)
            should_overwrite = (*overwrite_func)(overwrite_cookie);

        if (!should_overwrite) {
            /* We want to reject overwrite. */
            should_upload_data = 0;
            err = cahute_seven_send_basic(
                link,
                0,
                PACKET_TYPE_NAK,
                PACKET_SUBTYPE_NAK_REJECT_OVERWRITE
            );
        } else {
            /* We want to confirm overwrite! */
            err = cahute_seven_send_basic(
                link,
                0,
                PACKET_TYPE_ACK,
                PACKET_SUBTYPE_ACK_CONFIRM_OVERWRITE
            );
        }

        if (err)
            return err;
    }

    /* Whether we've been through an overwrite confirmation flow and confirmed
     * it, rejected it, or if no overwrite confirmation was requested, at this
     * point, we expect an acknowledgement to be the last packet to have
     * been received. */
    EXPECT_BASIC_ACK;

    if (should_upload_data && file_size
        && (err = cahute_seven_send_data(
                link,
                0,
                filep,
                file_size,
                progress_func,
                progress_cookie
            )))
        return err;

    return CAHUTE_OK;
}

/**
 * Request a file from storage on the calculator.
 *
 * @param link Link to the device.
 * @param directory Optional name of the directory.
 * @param name Name of the file.
 * @param storage Name of the storage device.
 * @param filep File pointer to write to.
 * @param progress_func Function to call to signify progress.
 * @param progress_cookie Cookie to pass to the progress function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_seven_request_file_from_storage(
    cahute_link *link,
    char const *directory,
    char const *name,
    char const *storage,
    FILE *filep,
    cahute_progress_func *progress_func,
    void *progress_cookie
) {
    int err;
    unsigned long filesize;

    /* Active sends 0x44 command.
     * Active receives ACK. */
    err = cahute_seven_send_command(
        link,
        0x44,
        0,
        0,
        0,
        directory,
        name,
        NULL,
        NULL,
        storage,
        NULL
    );
    if (err)
        return err;

    EXPECT_BASIC_ACK;

    /* Active sends roleswap.
     * Passive sends command 0x45 with file size. */
    err = cahute_seven_send_basic(link, 0, PACKET_TYPE_ROLESWAP, 0);
    EXPECT_PACKET(PACKET_TYPE_COMMAND, 0x45);

    err = cahute_seven_decode_command(
        link,
        NULL,
        NULL,
        &filesize,
        NULL,
        NULL, /* D1 */
        NULL,
        NULL, /* D2 */
        NULL,
        NULL, /* D3 */
        NULL,
        NULL, /* D4 */
        NULL,
        NULL, /* D5 */
        NULL,
        NULL /* D6 */
    );
    if (err)
        return err;

    /* Active sends ACK.
     * Data flow occurs from passive to active.
     * Last ACK is not yet sent. */
    err = cahute_seven_receive_data(
        link,
        0,
        filep,
        filesize,
        0x45,
        progress_func,
        progress_cookie
    );
    if (err)
        return err;

    /* Active sends ACK for last data packet.
     * Passive sends ROLESWAP.
     * We are back to initial situation. */
    err = cahute_seven_send_basic(
        link,
        0,
        PACKET_TYPE_ACK,
        PACKET_SUBTYPE_ACK_BASIC
    );
    if (err)
        return err;

    EXPECT_PACKET(PACKET_TYPE_ROLESWAP, 0);
    return CAHUTE_OK;
}

/**
 * Request for a file to be copied on the calculator.
 *
 * @param link Link on which to make the request.
 * @param source_directory Directory in which the source file is present.
 * @param source_name Name of the source file.
 * @param target_directory Directory in which the copy should be placed into.
 * @param target_name Name of the copied file.
 * @param storage Name of the storage device.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_seven_copy_file_on_storage(
    cahute_link *link,
    char const *source_directory,
    char const *source_name,
    char const *target_directory,
    char const *target_name,
    char const *storage
) {
    int err;

    err = cahute_seven_send_command(
        link,
        0x48,
        0,
        0,
        0,
        source_directory,
        source_name,
        target_directory,
        target_name,
        storage,
        NULL
    );
    if (err)
        return err;

    EXPECT_BASIC_ACK;
    return CAHUTE_OK;
}

/**
 * List files and directories from the calculator.
 *
 * @param link Link to the device.
 * @param storage Name of the storage device on which to list files and
 *        directories.
 * @param callback Callback function for every entry.
 * @param cookie Cookie to pass to the callback.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_seven_list_storage_entries(
    cahute_link *link,
    char const *storage,
    cahute_list_storage_entry_func *callback,
    void *cookie
) {
    cahute_storage_entry entry;
    cahute_u8 const *raw_directory_name, *raw_file_name;
    size_t raw_directory_name_size, raw_file_name_size;
    char directory_name_buf[24], file_name_buf[24];
    int err, should_skip = 0;

    err = cahute_seven_send_command(
        link,
        0x4D,
        0,
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        storage,
        NULL
    );
    if (err)
        return err;

    EXPECT_BASIC_ACK;

    err = cahute_seven_send_basic(link, 0, PACKET_TYPE_ROLESWAP, 0);
    if (err)
        return err;

    while (link->protocol_state.seven.last_packet_type == PACKET_TYPE_COMMAND
    ) {
        if (link->protocol_state.seven.last_packet_subtype != 0x4E) {
            /* The command is not "Transfer file information".
             * We just try to ACK and skip it here. */
            msg(ll_error,
                "Unhandled command %02X for file listing.",
                link->protocol_state.seven.last_packet_subtype);
            goto skip_entry;
        }

        if (should_skip)
            continue;

        err = cahute_seven_decode_command(
            link,
            NULL,
            NULL,
            &entry.cahute_storage_entry_size,
            &raw_directory_name,
            &raw_directory_name_size,
            &raw_file_name,
            &raw_file_name_size,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL, /* &raw_storage, although we aren't interested. */
            NULL, /* &raw_storage_size. */
            NULL,
            NULL
        );
        if (err)
            return err;

        /* We need to check if the directory and file name are human-readable,
         * if they are present. */
        if (!raw_directory_name || !raw_directory_name_size)
            entry.cahute_storage_entry_directory = NULL;
        else if (raw_directory_name_size >= sizeof(directory_name_buf) - 1)
            goto skip_entry; /* We cannot yield this entry. */
        else {
            cahute_u8 const *p = raw_directory_name;
            size_t size;

            for (size = raw_directory_name_size; size; size--, p++)
                if (!isascii(*p) || (!isgraph(*p) && !isblank(*p)))
                    goto skip_entry;

            entry.cahute_storage_entry_directory = directory_name_buf;
            memcpy(
                directory_name_buf,
                raw_directory_name,
                raw_directory_name_size
            );
            directory_name_buf[raw_directory_name_size] = '\0';
        }

        if (!raw_file_name || !raw_file_name_size)
            entry.cahute_storage_entry_name = NULL;
        else if (raw_file_name_size >= sizeof(file_name_buf) - 1)
            goto skip_entry; /* We cannot yield this entry. */
        else {
            cahute_u8 const *p = raw_file_name;
            size_t size;

            for (size = raw_file_name_size; size; size--, p++)
                if (!isascii(*p) || (!isgraph(*p) && !isblank(*p)))
                    goto skip_entry;

            entry.cahute_storage_entry_name = file_name_buf;
            memcpy(file_name_buf, raw_file_name, raw_file_name_size);
            file_name_buf[raw_file_name_size] = '\0';
        }

        if (!entry.cahute_storage_entry_directory
            && !entry.cahute_storage_entry_name)
            goto skip_entry;

        if ((*callback)(cookie, &entry)) {
            /* The callback has requested that we interrupt the file listing.
             * However, we can't really do that, because Protocol 7.00 has
             * no option to interrupt and rebecome passive as far as we know.
             * So we just set a flag to not process the entry. */
            should_skip = 1;
        }

skip_entry:
        err = cahute_seven_send_basic(
            link,
            0,
            PACKET_TYPE_ACK,
            PACKET_SUBTYPE_ACK_BASIC
        );
        if (err)
            return err;
    }

    EXPECT_PACKET(PACKET_TYPE_ROLESWAP, 0);
    return should_skip ? CAHUTE_ERROR_INT : CAHUTE_OK;
}

/**
 * Request for a file to be deleted on the calculator.
 *
 * @param link Link on which to make the request.
 * @param directory Directory name in which to delete the file.
 * @param name Name of the file to delete.
 * @param storage Name of the storage device.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_seven_delete_file_from_storage(
    cahute_link *link,
    char const *directory,
    char const *name,
    char const *storage
) {
    int err;

    err = cahute_seven_send_command(
        link,
        0x46,
        0,
        0,
        0,
        directory,
        name,
        NULL,
        NULL,
        storage,
        NULL
    );
    if (err)
        return err;

    EXPECT_BASIC_ACK;
    return CAHUTE_OK;
}

/**
 * Request for a storage device to be reset on the calculator.
 *
 * @param link Link to the device.
 * @param storage Name of the storage device.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_seven_reset_storage(cahute_link *link, char const *storage) {
    int err;

    err = cahute_seven_send_command(
        link,
        0x4A,
        0,
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        storage,
        NULL
    );
    if (err)
        return err;

    EXPECT_BASIC_ACK;
    return CAHUTE_OK;
}

/**
 * Backup the ROM from the calculator using Protocol 7.00.
 *
 * @param link Link to the calculator.
 * @param romp Pointer to the ROM to allocate.
 * @param sizep Pointer to the ROM size to define.
 * @param progress_func Function to display progress.
 * @param progress_cookie Cookie to pass to the progress function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_seven_backup_rom(
    cahute_link *link,
    cahute_u8 **romp,
    size_t *sizep,
    cahute_progress_func *progress_func,
    void *progress_cookie
) {
    unsigned long filesize;
    cahute_u8 *rom = NULL;
    size_t rom_size;
    int err = CAHUTE_OK;

    *romp = NULL;
    *sizep = 0;

    err = cahute_seven_send_command(
        link,
        0x4F,
        0,
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    );
    if (err)
        return err;

    EXPECT_BASIC_ACK;

    err = cahute_seven_send_basic(link, 0, PACKET_TYPE_ROLESWAP, 0);
    if (err)
        return err;

    EXPECT_PACKET(PACKET_TYPE_COMMAND, 0x50);

    err = cahute_seven_decode_command(
        link,
        NULL,
        NULL,
        &filesize,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    );
    if (err)
        return err;

    rom_size = (size_t)filesize;
    if (rom_size) {
        rom = malloc(rom_size);
        if (!rom) {
            link->flags |= CAHUTE_LINK_FLAG_IRRECOVERABLE;
            return CAHUTE_ERROR_ALLOC;
        }

        err = cahute_seven_receive_data_into_buf(
            link,
            RECEIVE_DATA_FLAG_DISABLE_SHIFTING,
            rom,
            rom_size,
            0x50,
            progress_func,
            progress_cookie
        );
        if (err)
            goto fail;
    }

    /* Active sends ACK for last data packet.
     * Passive sends ROLESWAP.
     * We are back to initial situation. */
    err = cahute_seven_send_basic(
        link,
        0,
        PACKET_TYPE_ACK,
        PACKET_SUBTYPE_ACK_BASIC
    );
    if (err)
        goto fail;

    EXPECT_PACKET_OR_FAIL(PACKET_TYPE_ROLESWAP, 0);

    *romp = rom;
    *sizep = rom_size;
    return CAHUTE_OK;

fail:
    if (rom)
        free(rom);

    return err;
}

/**
 * Upload and run a program on the calculator.
 *
 * @param link Link to the device.
 * @param program Program to upload and run.
 * @param program_size Size of the program to upload and run.
 * @param load_address Address at which to load the program.
 * @param start_address Address at which to start the program.
 * @param progress_func Function to display progress.
 * @param progress_cookie Cookie to pass to the progress function.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_seven_upload_and_run_program(
    cahute_link *link,
    cahute_u8 const *program,
    size_t program_size,
    unsigned long load_address,
    unsigned long start_address,
    cahute_progress_func *progress_func,
    void *progress_cookie
) {
    cahute_u8 command_payload[24];
    int err;

    if (program_size > 0xFFFFFFFF || load_address > 0xFFFFFFFF
        || start_address > 0xFFFFFFFF)
        return CAHUTE_ERROR_IMPL;

    /* Prepare the command payload. */
    cahute_seven_set_ascii_hex(&command_payload[0], program_size >> 24);
    cahute_seven_set_ascii_hex(
        &command_payload[2],
        (program_size >> 16) & 255
    );
    cahute_seven_set_ascii_hex(&command_payload[4], (program_size >> 8) & 255);
    cahute_seven_set_ascii_hex(&command_payload[6], program_size & 255);
    cahute_seven_set_ascii_hex(&command_payload[8], load_address >> 24);
    cahute_seven_set_ascii_hex(
        &command_payload[10],
        (load_address >> 16) & 255
    );
    cahute_seven_set_ascii_hex(
        &command_payload[12],
        (load_address >> 8) & 255
    );
    cahute_seven_set_ascii_hex(&command_payload[14], load_address & 255);
    cahute_seven_set_ascii_hex(&command_payload[16], start_address >> 24);
    cahute_seven_set_ascii_hex(
        &command_payload[18],
        (start_address >> 16) & 255
    );
    cahute_seven_set_ascii_hex(
        &command_payload[20],
        (start_address >> 8) & 255
    );
    cahute_seven_set_ascii_hex(&command_payload[22], start_address & 255);

    err = cahute_seven_send_extended(
        link,
        0,
        PACKET_TYPE_COMMAND,
        0x56,
        command_payload,
        24
    );
    if (err)
        return err;

    EXPECT_BASIC_ACK;

    link->protocol_state.seven.last_command = 0x56;
    return cahute_seven_send_data_from_buf(
        link,
        0,
        program,
        program_size,
        progress_func,
        progress_cookie
    );
}

/**
 * Flash a sector using the fxRemote method.
 *
 * This method sends the data using 0x70 commands, by buffers of 0x3FC bytes
 * (size of the protocol buffer to fxRemote), and once the whole buffer is
 * copied at address 0x88030000, request a copy to the real flash location
 * using command 0x71.
 *
 * @param link Link to the device.
 * @param addr Base address of the sector to write.
 * @param data Data to write to the sector.
 * @param size Size of the data to write to the sector.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_LOCAL(int)
cahute_seven_flash_sector_using_fxremote_method(
    cahute_link *link,
    unsigned long addr,
    cahute_u8 const *data,
    size_t size
) {
    cahute_u8 buf[0x404];
    unsigned long upload_offset = 0x88030000;
    unsigned long upload_left = size;
    int err;

    /* Send data using the 0x3FC sized buffer. */
    for (; upload_left >= 0x3FC;
         upload_offset += 0x3FC, upload_left -= 0x3FC, data += 0x3FC) {
        buf[0] = (upload_offset >> 24) & 255;
        buf[1] = (upload_offset >> 16) & 255;
        buf[2] = (upload_offset >> 8) & 255;
        buf[3] = upload_offset & 255;
        buf[4] = 0;
        buf[5] = 0;
        buf[6] = 0x03;
        buf[7] = 0xFC;
        memcpy(&buf[8], data, 0x3FC);

        err = cahute_seven_send_extended(
            link,
            0,
            PACKET_TYPE_COMMAND,
            0x70,
            buf,
            0x404
        );
        if (err)
            return err;

        EXPECT_BASIC_ACK;
    }

    /* Leftover data (% 0x3FC). */
    if (upload_left) {
        buf[0] = (upload_offset >> 24) & 255;
        buf[1] = (upload_offset >> 16) & 255;
        buf[2] = (upload_offset >> 8) & 255;
        buf[3] = upload_offset & 255;
        buf[4] = 0;
        buf[5] = 0;
        buf[6] = (upload_left >> 8) & 255;
        buf[7] = upload_left & 255;
        memcpy(&buf[8], data, upload_left);

        err = cahute_seven_send_extended(
            link,
            0,
            PACKET_TYPE_COMMAND,
            0x70,
            buf,
            8 + upload_left
        );
        if (err)
            return err;

        EXPECT_BASIC_ACK;
    }

    /* Copy data from the buffer to the flash. */
    buf[0] = (addr >> 24) & 255;
    buf[1] = (addr >> 16) & 255;
    buf[2] = (addr >> 8) & 255;
    buf[3] = addr & 255;
    buf[4] = (size >> 24) & 255;
    buf[5] = (size >> 16) & 255;
    buf[6] = (size >> 8) & 255;
    buf[7] = size & 255;
    buf[8] = 0x88;
    buf[9] = 0x03;
    buf[10] = 0x00;
    buf[11] = 0x00;

    err = cahute_seven_send_extended(
        link,
        0,
        PACKET_TYPE_COMMAND,
        0x71,
        buf,
        12
    );
    if (err)
        return err;

    EXPECT_BASIC_ACK;

    return CAHUTE_OK;
}

/**
 * Flash an image on the calculator.
 *
 * This uses command 0x76 to get information regarding the calculator model
 * from the bootloader, clears all sectors related to the ROM using
 * command 0x72, then copy all sectors from 0xA0020000 to 0xA0280000 excluded,
 * then the initial system sector at 0xA0010000, and send the final
 * 0x78 command.
 *
 * See :ref:`flash-the-calculator-using-fxremote` for more information.
 *
 * @param link Link to the device.
 * @param flags Flags.
 * @param system System to flash on the device.
 * @param system_size Size of the system to flash on the device.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_seven_flash_system_using_fxremote_method(
    cahute_link *link,
    unsigned long flags,
    cahute_u8 const *system,
    size_t system_size
) {
    cahute_u8 const *initial_sector;
    unsigned long addr, initial_sector_size;
    unsigned long bootloader_size;
    unsigned long max_addr;
    int err;

    /* Use fxRemote-specific command 76 to get special data.
     * While we don't want to exploit it, we want to still do it for
     * the Update.EXE to recognize us as fxRemote. */
    err = cahute_seven_send_basic(link, 0, PACKET_TYPE_COMMAND, 0x76);
    if (err)
        return err;

    EXPECT_BASIC_ACK;

    /* The previous packet sends both an ACK and a data packet. */
    err = cahute_seven_receive(link);
    if (err)
        return err;

    /* Clear all system-related sectors of the flash. */
    if (flags & CAHUTE_FLASH_FLAG_RESET_SMEM)
        max_addr = 0xA0400000;
    else
        max_addr = 0xA0280000;

    for (addr = 0xA0010000; addr < max_addr; addr += 0x10000) {
        cahute_u8 buf[4];

        buf[0] = (addr >> 24) & 255;
        buf[1] = (addr >> 16) & 255;
        buf[2] = (addr >> 8) & 255;
        buf[3] = addr & 255;

        err = cahute_seven_send_extended(
            link,
            0,
            PACKET_TYPE_COMMAND,
            0x72,
            buf,
            4
        );
        if (err)
            return err;

        EXPECT_BASIC_ACK;
    }

    /* Skip the bootloader section. */
    bootloader_size = system_size >= 0x10000 ? 0x10000 : system_size;
    system += bootloader_size;
    system_size -= bootloader_size;

    /* Write all sectors. */
    initial_sector = system;
    initial_sector_size = system_size > 0x10000 ? 0x10000 : system_size;

    system += initial_sector_size;
    system_size -= initial_sector_size;

    for (addr = 0xA0020000; system_size; addr += 0x10000) {
        unsigned long sector_size =
            system_size > 0x10000 ? 0x10000 : system_size;

        err = cahute_seven_flash_sector_using_fxremote_method(
            link,
            addr,
            system,
            sector_size
        );
        if (err)
            return err;

        system += sector_size;
        system_size -= sector_size;
    }

    if (initial_sector_size) {
        err = cahute_seven_flash_sector_using_fxremote_method(
            link,
            0xA0010000,
            initial_sector,
            initial_sector_size
        );
        if (err)
            return err;
    }

    /* We can request termination. */
    err = cahute_seven_send_basic(link, 0, PACKET_TYPE_COMMAND, 0x78);
    if (err)
        return err;

    EXPECT_BASIC_ACK;

    /* Everything should be good! */
    return CAHUTE_OK;
}
