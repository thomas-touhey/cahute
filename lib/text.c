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
#include "chars.h"

/**
 * Estimate the size of a given Unicode character in UTF-8.
 *
 * @param code Unicode character codepoint.
 * @return Size.
 */
CAHUTE_INLINE(size_t) cahute_get_utf8_char_size(unsigned long code) {
    if (code < 128)
        return 1;
    else if (code < 2048)
        return 2;
    else if (code < 65536)
        return 3;
    else if (code < 2097152)
        return 4;
    else if (code < 67108864)
        return 5;

    return 6;
}

/**
 * Put a Unicode character in a UTF-8 buffer.
 *
 * WARNING: The buffer is assumed to be large enough.
 *
 * @param buf Buffer to write the character into.
 * @param code Code to write as UTF-8.
 * @return Size of the written data.
 */
CAHUTE_LOCAL(size_t) cahute_put_utf8_char(cahute_u8 *buf, unsigned long code) {
    if (code < 128) {
        *buf = code & 127;
        return 1;
    }

    if (code < 2048) {
        *buf++ = 192 | ((code >> 6) & 31);
        *buf = 128 | (code & 63);
        return 2;
    }

    if (code < 65536) {
        *buf++ = 224 | ((code >> 12) & 15);
        *buf++ = 128 | ((code >> 6) & 63);
        *buf = 128 | (code & 63);
        return 3;
    }

    if (code < 2097152) {
        *buf++ = 240 | ((code >> 18) & 7);
        *buf++ = 128 | ((code >> 12) & 63);
        *buf++ = 128 | ((code >> 6) & 63);
        *buf = 128 | (code & 63);
        return 4;
    }

    if (code < 67108864) {
        *buf++ = 248 | ((code >> 24) & 3);
        *buf++ = 128 | ((code >> 18) & 63);
        *buf++ = 128 | ((code >> 12) & 63);
        *buf++ = 128 | ((code >> 6) & 63);
        *buf = 128 | (code & 63);
        return 5;
    }

    /* NOTE: We ignore the MSb in case it's set, as it shouldn't be if the
     * input is considered valid. */
    *buf++ = 252 | ((code >> 30) & 1);
    *buf++ = 128 | ((code >> 24) & 63);
    *buf++ = 128 | ((code >> 18) & 63);
    *buf++ = 128 | ((code >> 12) & 63);
    *buf++ = 128 | ((code >> 6) & 63);
    *buf = 128 | (code & 63);
    return 6;
}

/**
 * Get the next character in a given UTF-8 stream.
 *
 * WARNING: data_size is assumed to be at least 1 here.
 *
 * @param data Source data buffer.
 * @param data_size Data size in the provided buffer.
 * @param resultp Pointer to the read character to set.
 * @param lenp Pointer to the sequence length to set.
 * @return Cahute error, or 0 if ok.
 */
CAHUTE_INLINE(int)
cahute_get_utf8_char(
    cahute_u8 const *data,
    size_t data_size,
    cahute_u32 *resultp,
    size_t *lenp
) {
    int byte = *data;

    if ((byte & 128) == 0) {
        /* Format: 0xxxxxxx */
        *lenp = 1;
        *resultp = byte;
    } else if ((byte & 224) == 192) {
        /* Format: 110xxxxx 10xxxxxx */
        if (data_size < 2)
            return CAHUTE_ERROR_TRUNC;
        if ((data[1] & 192) != 128)
            return CAHUTE_ERROR_INVALID;

        *resultp = ((data[0] & 31) << 6) | (data[1] & 63);
        *lenp = 2;
    } else if ((byte & 240) == 224) {
        /* Format: 1110xxxx 10xxxxxx 10xxxxxx */
        if (data_size < 3)
            return CAHUTE_ERROR_TRUNC;
        if ((data[1] & 192) != 128 || (data[2] & 192) != 128)
            return CAHUTE_ERROR_INVALID;

        *resultp =
            ((data[0] & 15) << 12) | ((data[1] & 63) << 6) | (data[2] & 63);
        *lenp = 3;
    } else if ((byte & 248) == 240) {
        /* Format: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
        if (data_size < 4)
            return CAHUTE_ERROR_TRUNC;
        if ((data[1] & 192) != 128 || (data[2] & 192) != 128
            || (data[3] & 192) != 128)
            return CAHUTE_ERROR_INVALID;

        *resultp = ((data[0] & 7) << 18) | ((data[1] & 63) << 12)
                   | ((data[2] & 63) << 6) | (data[3] & 63);
        *lenp = 4;
    } else if ((byte & 252) == 248) {
        /* Format: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
        if (data_size < 5)
            return CAHUTE_ERROR_TRUNC;
        if ((data[1] & 192) != 128 || (data[2] & 192) != 128
            || (data[3] & 192) != 128 || (data[4] & 192) != 128)
            return CAHUTE_ERROR_INVALID;

        *resultp = ((data[0] & 3) << 24) | ((data[1] & 63) << 18)
                   | ((data[2] & 63) << 12) | ((data[3] & 63) << 6)
                   | (data[4] & 63);
        *lenp = 5;
    } else if ((byte & 254) == 252) {
        /* Format: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
        if (data_size < 6)
            return CAHUTE_ERROR_TRUNC;
        if ((data[1] & 192) != 128 || (data[2] & 192) != 128
            || (data[3] & 192) != 128 || (data[4] & 192) != 128
            || (data[5] & 192) != 128)
            return CAHUTE_ERROR_INVALID;

        *resultp = ((data[0] & 1) << 30) | ((data[1] & 63) << 24)
                   | ((data[2] & 63) << 18) | ((data[3] & 63) << 12)
                   | ((data[4] & 63) << 6) | (data[5] & 63);
        *lenp = 6;
    } else
        return CAHUTE_ERROR_INVALID;

    return CAHUTE_OK;
}

/**
 * Get the character entry corresponding to a legacy code.
 *
 * The returned value is NULL if no such character was found.
 *
 * @param code Code for which to get the entry.
 * @return Character entry for the code.
 */
CAHUTE_INLINE(struct cahute_char_entry const *)
cahute_get_legacy_char_entry(unsigned int code) {
    switch ((code >> 8) & 255) {
    case 0x00:
        return cahute_chars_legacy_00[code & 255];
    case 0x7F:
        return cahute_chars_legacy_7F[code & 255];
    case 0xF7:
        return cahute_chars_legacy_F7[code & 255];
    default:
        return NULL;
    }
}

/**
 * Get the Unicode sequence from a given legacy char.
 *
 * This may return ``CAHUTE_ERROR_INCOMPAT`` if the character itself, or one
 * of the characters designated by the opcode, does not have a Unicode
 * equivalent.
 *
 * @param code Character code.
 * @param seq Sequence to write in.
 * @param seq_lenp Pointer to the sequence length. This is updated if no
 *        error has occurred, to the size of the data.
 */
CAHUTE_INLINE(int)
cahute_get_legacy_unicode_sequence(
    unsigned int code,
    cahute_u32 *seq,
    size_t *seq_lenp
) {
    struct cahute_char_entry const *entry = cahute_get_legacy_char_entry(code);

    if (!entry)
        return CAHUTE_ERROR_INVALID;

    if (entry->unicode) {
        if (entry->unicode_len > *seq_lenp)
            return CAHUTE_ERROR_SIZE;

        *seq_lenp = entry->unicode_len;
        memcpy(seq, entry->unicode, entry->unicode_len << 2);
    } else if (!entry->opcode)
        return CAHUTE_ERROR_INCOMPAT;
    else {
        cahute_u16 const *opcode = entry->opcode;
        size_t i, opcode_len = entry->opcode_len;
        size_t estimate = 0;

        /* First, check that all characters pointed at by the opcode actually
         * define a Unicode sequence, and compute an estimate. */
        for (i = 0; i < opcode_len; i++) {
            struct cahute_char_entry const *sub_entry =
                cahute_get_legacy_char_entry(opcode[i]);

            if (!sub_entry)
                return CAHUTE_ERROR_INVALID;
            if (!sub_entry->unicode_len)
                return CAHUTE_ERROR_INCOMPAT;

            estimate += sub_entry->unicode_len;
        }

        if (estimate > *seq_lenp)
            return CAHUTE_ERROR_SIZE;

        /* We have determined the sequence length is correct, we can now
         * compute the full sequence. */
        *seq_lenp = estimate;
        for (i = 0; i < opcode_len; i++) {
            struct cahute_char_entry const *sub_entry =
                cahute_get_legacy_char_entry(opcode[i]);

            memcpy(seq, sub_entry->unicode, sub_entry->unicode_len << 2);
            seq += sub_entry->unicode_len;
        }
    }

    return CAHUTE_OK;
}

/**
 * Get the next legacy character from a variable-size input.
 *
 * WARNING: data_size is assumed to be at least 1 here.
 *
 * @param data Data buffer.
 * @param data_size Data buffer size.
 * @param codep Code to define.
 * @param seq_lenp Pointer to the read sequence length to define.
 * @return Cahute error.
 */
CAHUTE_INLINE(int)
cahute_get_variable_size_legacy_char(
    cahute_u8 const *data,
    size_t data_size,
    unsigned int *codep,
    size_t *seq_lenp
) {
    int byte = *data;

    if (byte != 0x7F && byte != 0xF7) {
        *codep = byte;
        *seq_lenp = 1;
    } else if (data_size < 2)
        return CAHUTE_ERROR_TRUNC;
    else {
        *codep = (byte << 8) | data[1];
        *seq_lenp = 2;
    }

    return CAHUTE_OK;
}

/**
 * Get the character entry corresponding to a fx-9860G code.
 *
 * The returned value is NULL if no such character was found.
 *
 * @param code Code for which to get the entry.
 * @return Character entry for the code.
 */
CAHUTE_INLINE(struct cahute_char_entry const *)
cahute_get_9860_char_entry(unsigned int code) {
    switch ((code >> 8) & 255) {
    case 0x00:
        return cahute_chars_9860_00[code & 255];
    case 0x7F:
        return cahute_chars_9860_7F[code & 255];
    case 0xE5:
        return cahute_chars_9860_E5[code & 255];
    case 0xE6:
        return cahute_chars_9860_E6[code & 255];
    case 0xE7:
        return cahute_chars_9860_E7[code & 255];
    case 0xF7:
        return cahute_chars_9860_F7[code & 255];
    case 0xF9:
        return cahute_chars_9860_F9[code & 255];
    default:
        return NULL;
    }
}

/**
 * Get the Unicode sequence from a given fx-9860G char.
 *
 * This may return ``CAHUTE_ERROR_INCOMPAT`` if the character itself, or one
 * of the characters designated by the opcode, does not have a Unicode
 * equivalent.
 *
 * @param code Character code.
 * @param seq Sequence to write in.
 * @param seq_lenp Pointer to the sequence length. This is updated if no
 *        error has occurred, to the size of the data.
 */
CAHUTE_INLINE(int)
cahute_get_9860_unicode_sequence(
    unsigned int code,
    cahute_u32 *seq,
    size_t *seq_lenp
) {
    struct cahute_char_entry const *entry = cahute_get_9860_char_entry(code);

    if (!entry)
        return CAHUTE_ERROR_INVALID;

    if (entry->unicode) {
        if (entry->unicode_len > *seq_lenp)
            return CAHUTE_ERROR_SIZE;

        *seq_lenp = entry->unicode_len;
        memcpy(seq, entry->unicode, entry->unicode_len << 2);
    } else if (!entry->opcode)
        return CAHUTE_ERROR_INCOMPAT;
    else {
        cahute_u16 const *opcode = entry->opcode;
        size_t i, opcode_len = entry->opcode_len;
        size_t estimate = 0;

        /* First, check that all characters pointed at by the opcode actually
         * define a Unicode sequence, and compute an estimate. */
        for (i = 0; i < opcode_len; i++) {
            struct cahute_char_entry const *sub_entry =
                cahute_get_9860_char_entry(opcode[i]);

            if (!sub_entry)
                return CAHUTE_ERROR_INVALID;
            if (!sub_entry->unicode_len)
                return CAHUTE_ERROR_INCOMPAT;

            estimate += sub_entry->unicode_len;
        }

        if (estimate > *seq_lenp)
            return CAHUTE_ERROR_SIZE;

        /* We have determined the sequence length is correct, we can now
         * compute the full sequence. */
        *seq_lenp = estimate;
        for (i = 0; i < opcode_len; i++) {
            struct cahute_char_entry const *sub_entry =
                cahute_get_9860_char_entry(opcode[i]);

            memcpy(seq, sub_entry->unicode, sub_entry->unicode_len << 2);
            seq += sub_entry->unicode_len;
        }
    }

    return CAHUTE_OK;
}

/**
 * Get the next fx-9860G character from a variable-size input.
 *
 * WARNING: data_size is assumed to be at least 1 here.
 *
 * @param data Data buffer.
 * @param data_size Data buffer size.
 * @param codep Code to define.
 * @param seq_lenp Pointer to the read sequence length to define.
 * @return Cahute error.
 */
CAHUTE_INLINE(int)
cahute_get_variable_size_9860_char(
    cahute_u8 const *data,
    size_t data_size,
    unsigned int *codep,
    size_t *seq_lenp
) {
    int byte = *data;

    if (byte != 0x7F && byte != 0xE5 && byte != 0xE6 && byte != 0xE7
        && byte != 0xF7 && byte != 0xF9) {
        *codep = byte;
        *seq_lenp = 1;
    } else if (data_size < 2)
        return CAHUTE_ERROR_TRUNC;
    else {
        *codep = (byte << 8) | data[1];
        *seq_lenp = 2;
    }

    return CAHUTE_OK;
}

/* Defined conversions loops. */
#define CONV_UNICODE 1 /* Unicode-based character conversion loop. */
#define CONV_CASIO   2 /* Legacy or 9860 based character conversion loop. */

/* Table from which to get the final code. */
#define TBL_LEGACY 1
#define TBL_9860   2

/**
 * Convert text from one encoding to the other.
 *
 * WARNING: If you update this function to update or remove a character
 * conversion, please also update the table in the
 * :c:func:`cahute_convert_text` function documentation.
 *
 * @param bufp Pointer to the destination pointer.
 * @param buf_sizep Pointer to the destination size.
 * @param datap Pointer to the source data pointer.
 * @param data_sizep Pointer to the source data size.
 * @param dest_encoding Destination encoding.
 * @param source_encoding Source encoding.
 * @return Cahute error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_convert_text(
    void **bufp,
    size_t *buf_sizep,
    void const **datap,
    size_t *data_sizep,
    int dest_encoding,
    int source_encoding
) {
    cahute_u8 *buf = *bufp;
    cahute_u8 const *data = *datap;
    size_t buf_size = *buf_sizep;
    size_t data_size = *data_sizep;
    int host_big_endian = cahute_be16toh(0x1234) == 0x1234;
    struct cahute_u32_parsing_tree const *unicode_parsing_tree = NULL;
    int conv, tbl;
    int err = CAHUTE_OK;

    if (!data_size)
        return CAHUTE_OK;
    if (!buf_size)
        return CAHUTE_ERROR_SIZE;

    /* Preprocess the destination encoding if necessary. */
    switch (dest_encoding) {
    case CAHUTE_TEXT_ENCODING_LEGACY_16_HOST:
        dest_encoding = host_big_endian ? CAHUTE_TEXT_ENCODING_LEGACY_16_BE
                                        : CAHUTE_TEXT_ENCODING_LEGACY_16_LE;
        break;

    case CAHUTE_TEXT_ENCODING_9860_16_HOST:
        dest_encoding = host_big_endian ? CAHUTE_TEXT_ENCODING_9860_16_BE
                                        : CAHUTE_TEXT_ENCODING_9860_16_LE;
        break;

    case CAHUTE_TEXT_ENCODING_UTF32_HOST:
        dest_encoding = host_big_endian ? CAHUTE_TEXT_ENCODING_UTF32_BE
                                        : CAHUTE_TEXT_ENCODING_UTF32_LE;
    }

    /* Preprocess the source encoding if necessary. */
    switch (source_encoding) {
    case CAHUTE_TEXT_ENCODING_LEGACY_16_HOST:
        source_encoding = host_big_endian ? CAHUTE_TEXT_ENCODING_LEGACY_16_BE
                                          : CAHUTE_TEXT_ENCODING_LEGACY_16_LE;
        break;

    case CAHUTE_TEXT_ENCODING_9860_16_HOST:
        source_encoding = host_big_endian ? CAHUTE_TEXT_ENCODING_9860_16_BE
                                          : CAHUTE_TEXT_ENCODING_9860_16_LE;
        break;

    case CAHUTE_TEXT_ENCODING_UTF32_HOST:
        source_encoding = host_big_endian ? CAHUTE_TEXT_ENCODING_UTF32_BE
                                          : CAHUTE_TEXT_ENCODING_UTF32_LE;
    }

    /* Compute the character conversion and parsing trees to use in the
     * main conversion loop.
     *
     * If the selected conversion loop is ``CONV_CASIO``, the following
     * variables must be defined:
     *
     *   ``unicode_parsing_tree``: Tree to use for parsing Unicode. */
    conv = CONV_CASIO;
    tbl = TBL_9860;
    switch (dest_encoding) {
    case CAHUTE_TEXT_ENCODING_LEGACY_8:
    case CAHUTE_TEXT_ENCODING_LEGACY_16_BE:
    case CAHUTE_TEXT_ENCODING_LEGACY_16_LE:
        tbl = TBL_LEGACY;

        switch (source_encoding) {
        case CAHUTE_TEXT_ENCODING_LEGACY_8:
        case CAHUTE_TEXT_ENCODING_LEGACY_16_BE:
        case CAHUTE_TEXT_ENCODING_LEGACY_16_LE:
        case CAHUTE_TEXT_ENCODING_9860_8:
        case CAHUTE_TEXT_ENCODING_9860_16_BE:
        case CAHUTE_TEXT_ENCODING_9860_16_LE:
            break;

        case CAHUTE_TEXT_ENCODING_UTF32_BE:
        case CAHUTE_TEXT_ENCODING_UTF32_LE:
        case CAHUTE_TEXT_ENCODING_UTF8:
            unicode_parsing_tree = &cahute_unicode_legacy_parsing_tree;
            break;

        default:
            CAHUTE_RETURN_IMPL("Unimplemented conversion.");
        }
        break;

    case CAHUTE_TEXT_ENCODING_9860_8:
    case CAHUTE_TEXT_ENCODING_9860_16_BE:
    case CAHUTE_TEXT_ENCODING_9860_16_LE:
        switch (source_encoding) {
        case CAHUTE_TEXT_ENCODING_LEGACY_8:
        case CAHUTE_TEXT_ENCODING_LEGACY_16_BE:
        case CAHUTE_TEXT_ENCODING_LEGACY_16_LE:
        case CAHUTE_TEXT_ENCODING_9860_8:
        case CAHUTE_TEXT_ENCODING_9860_16_BE:
        case CAHUTE_TEXT_ENCODING_9860_16_LE:
            break;

        case CAHUTE_TEXT_ENCODING_UTF32_BE:
        case CAHUTE_TEXT_ENCODING_UTF32_LE:
        case CAHUTE_TEXT_ENCODING_UTF8:
            unicode_parsing_tree = &cahute_unicode_9860_parsing_tree;
            break;

        default:
            CAHUTE_RETURN_IMPL("Unimplemented conversion.");
        }
        break;

    case CAHUTE_TEXT_ENCODING_UTF32_BE:
    case CAHUTE_TEXT_ENCODING_UTF32_LE:
    case CAHUTE_TEXT_ENCODING_UTF8:
        conv = CONV_UNICODE;
        switch (source_encoding) {
        case CAHUTE_TEXT_ENCODING_LEGACY_8:
        case CAHUTE_TEXT_ENCODING_LEGACY_16_BE:
        case CAHUTE_TEXT_ENCODING_LEGACY_16_LE:
        case CAHUTE_TEXT_ENCODING_9860_8:
        case CAHUTE_TEXT_ENCODING_9860_16_BE:
        case CAHUTE_TEXT_ENCODING_9860_16_LE:
        case CAHUTE_TEXT_ENCODING_UTF32_BE:
        case CAHUTE_TEXT_ENCODING_UTF32_LE:
        case CAHUTE_TEXT_ENCODING_UTF8:
            break;

        default:
            CAHUTE_RETURN_IMPL("Unimplemented conversion.");
        }
        break;


    default:
        CAHUTE_RETURN_IMPL("Unimplemented conversion.");
    }

    /* The conversion loop and parameters have been chosen in the above switch,
     * we can now operate the loop. */
    switch (conv) {
    case CONV_CASIO: {
        size_t source_len = 0;

        for (; data_size; data += source_len, data_size -= source_len) {
            unsigned int code;
            struct cahute_char_entry const *entry = NULL;

            /* Read the source character. */
            err = CAHUTE_ERROR_TRUNC;
            switch (source_encoding) {
            case CAHUTE_TEXT_ENCODING_LEGACY_8:
                err = cahute_get_variable_size_legacy_char(
                    data,
                    data_size,
                    &code,
                    &source_len
                );
                if (err)
                    goto end;

                if (!code || code == 0xFF) {
                    /* End character. */
                    data += source_len;
                    data_size -= source_len;
                    err = CAHUTE_ERROR_TERMINATED;
                    goto end;
                }

                entry = cahute_get_legacy_char_entry(code);
                if (!entry) {
                    err = CAHUTE_ERROR_INVALID;
                    goto end;
                }
                break;

            case CAHUTE_TEXT_ENCODING_9860_8:
                err = cahute_get_variable_size_9860_char(
                    data,
                    data_size,
                    &code,
                    &source_len
                );
                if (err)
                    goto end;

                if (!code || code == 0xFF) {
                    /* End character. */
                    data += source_len;
                    data_size -= source_len;
                    err = CAHUTE_ERROR_TERMINATED;
                    goto end;
                }

                entry = cahute_get_9860_char_entry(code);
                if (!entry) {
                    err = CAHUTE_ERROR_INVALID;
                    goto end;
                }
                break;

            case CAHUTE_TEXT_ENCODING_LEGACY_16_BE:
                if (data_size < 2)
                    goto end;

                source_len = 2;
                code = (data[0] << 8) | data[1];

                if (!code || code == 0xFF) {
                    /* End character. */
                    data += source_len;
                    data_size -= source_len;
                    err = CAHUTE_ERROR_TERMINATED;
                    goto end;
                }

                entry = cahute_get_legacy_char_entry(code);
                if (!entry) {
                    err = CAHUTE_ERROR_INVALID;
                    goto end;
                }
                break;

            case CAHUTE_TEXT_ENCODING_LEGACY_16_LE:
                if (data_size < 2)
                    goto end;

                source_len = 2;
                code = (data[1] << 8) | data[0];

                if (!code || code == 0xFF) {
                    /* End character. */
                    data += source_len;
                    data_size -= source_len;
                    err = CAHUTE_ERROR_TERMINATED;
                    goto end;
                }

                entry = cahute_get_legacy_char_entry(code);
                if (!entry) {
                    err = CAHUTE_ERROR_INVALID;
                    goto end;
                }
                break;

            case CAHUTE_TEXT_ENCODING_9860_16_BE:
                if (data_size < 2)
                    goto end;

                source_len = 2;
                code = (data[0] << 8) | data[1];

                if (!code || code == 0xFF) {
                    /* End character. */
                    data += source_len;
                    data_size -= source_len;
                    err = CAHUTE_ERROR_TERMINATED;
                    goto end;
                }

                entry = cahute_get_9860_char_entry(code);
                if (!entry) {
                    err = CAHUTE_ERROR_INVALID;
                    goto end;
                }
                break;

            case CAHUTE_TEXT_ENCODING_9860_16_LE:
                if (data_size < 2)
                    goto end;

                source_len = 2;
                code = (data[1] << 8) | data[0];

                if (!code || code == 0xFF) {
                    /* End character. */
                    data += source_len;
                    data_size -= source_len;
                    err = CAHUTE_ERROR_TERMINATED;
                    goto end;
                }

                entry = cahute_get_9860_char_entry(code);
                if (!entry) {
                    err = CAHUTE_ERROR_INVALID;
                    goto end;
                }
                break;

            default:
                CAHUTE_RETURN_IMPL("Unimplemented reading for CASIO conv loop."
                );
            }

            if (tbl == TBL_LEGACY)
                code = entry->code_legacy;
            else
                code = entry->code_9860;

            if (!code) {
                /* No translation for the current code! */
                err = CAHUTE_ERROR_INCOMPAT;
                goto end;
            }

            /* Write to the final output. */
            err = CAHUTE_ERROR_SIZE;
            switch (dest_encoding) {
            case CAHUTE_TEXT_ENCODING_LEGACY_8:
            case CAHUTE_TEXT_ENCODING_9860_8:
                if (code > 0xFF) {
                    if (buf_size < 2)
                        goto end;

                    *buf++ = (code >> 8) & 255;
                    *buf++ = code & 255;
                    buf_size -= 2;
                } else if (buf_size < 1)
                    goto end;
                else {
                    *buf++ = code & 255;
                    buf_size--;
                }
                break;

            case CAHUTE_TEXT_ENCODING_LEGACY_16_BE:
            case CAHUTE_TEXT_ENCODING_9860_16_BE:
                if (buf_size < 2)
                    goto end;

                *buf++ = (code >> 8) & 255;
                *buf++ = code & 255;
                buf_size -= 2;
                break;

            case CAHUTE_TEXT_ENCODING_LEGACY_16_LE:
            case CAHUTE_TEXT_ENCODING_9860_16_LE:
                if (buf_size < 2)
                    goto end;

                *buf++ = code & 255;
                *buf++ = (code >> 8) & 255;
                buf_size -= 2;
                break;

            default:
                CAHUTE_RETURN_IMPL(
                    "Unimplemented writing for CASIO conv. loop."
                );
            }
        }
    } break;

    case CONV_UNICODE: {
        cahute_u32 seq[100];
        size_t source_len = 0;
        size_t i, seq_len = 0;
        unsigned int code;

        for (; data_size; data += source_len, data_size -= source_len) {
            /* Read the source Unicode sequence. */
            err = CAHUTE_ERROR_TRUNC;
            switch (source_encoding) {
            case CAHUTE_TEXT_ENCODING_LEGACY_8:
                err = cahute_get_variable_size_legacy_char(
                    data,
                    data_size,
                    &code,
                    &source_len
                );
                if (err)
                    goto end;

                if (!code || code == 0xFF) {
                    /* End character. */
                    data += source_len;
                    data_size -= source_len;
                    err = CAHUTE_ERROR_TERMINATED;
                    goto end;
                }

                seq_len = sizeof(seq) >> 2;
                err = cahute_get_legacy_unicode_sequence(code, seq, &seq_len);
                if (err)
                    goto end;
                break;

            case CAHUTE_TEXT_ENCODING_LEGACY_16_BE:
                if (data_size < 2)
                    goto end;

                source_len = 2;
                seq_len = sizeof(seq) >> 2;
                code = (data[0] << 8) | data[1];

                if (!code || code == 0xFF) {
                    /* End character. */
                    data += source_len;
                    data_size -= source_len;
                    err = CAHUTE_ERROR_TERMINATED;
                    goto end;
                }

                err = cahute_get_legacy_unicode_sequence(code, seq, &seq_len);
                if (err)
                    goto end;

                break;

            case CAHUTE_TEXT_ENCODING_LEGACY_16_LE:
                if (data_size < 2)
                    goto end;

                source_len = 2;
                seq_len = sizeof(seq) >> 2;
                code = (data[1] << 8) | data[0];

                if (!code || code == 0xFF) {
                    /* End character. */
                    data += source_len;
                    data_size -= source_len;
                    err = CAHUTE_ERROR_TERMINATED;
                    goto end;
                }

                err = cahute_get_legacy_unicode_sequence(code, seq, &seq_len);
                if (err)
                    goto end;

                break;

            case CAHUTE_TEXT_ENCODING_9860_8:
                err = cahute_get_variable_size_9860_char(
                    data,
                    data_size,
                    &code,
                    &source_len
                );
                if (err)
                    goto end;

                if (!code || code == 0xFF) {
                    /* End character. */
                    data += source_len;
                    data_size -= source_len;
                    err = CAHUTE_ERROR_TERMINATED;
                    goto end;
                }

                seq_len = sizeof(seq) >> 2;
                err = cahute_get_9860_unicode_sequence(code, seq, &seq_len);
                if (err)
                    goto end;
                break;

            case CAHUTE_TEXT_ENCODING_9860_16_BE:
                if (data_size < 2)
                    goto end;

                source_len = 2;
                seq_len = sizeof(seq) >> 2;
                code = (data[0] << 8) | data[1];

                if (!code || code == 0xFF) {
                    /* End character. */
                    data += source_len;
                    data_size -= source_len;
                    err = CAHUTE_ERROR_TERMINATED;
                    goto end;
                }

                err = cahute_get_9860_unicode_sequence(code, seq, &seq_len);
                if (err)
                    goto end;

                break;

            case CAHUTE_TEXT_ENCODING_9860_16_LE:
                if (data_size < 2)
                    goto end;

                source_len = 2;
                seq_len = sizeof(seq) >> 2;
                code = (data[1] << 8) | data[0];

                if (!code || code == 0xFF) {
                    /* End character. */
                    data += source_len;
                    data_size -= source_len;
                    err = CAHUTE_ERROR_TERMINATED;
                    goto end;
                }

                err = cahute_get_9860_unicode_sequence(code, seq, &seq_len);
                if (err)
                    goto end;

                break;

            case CAHUTE_TEXT_ENCODING_UTF32_BE:
                if (data_size < 4)
                    goto end;

                seq[0] = (data[0] << 24) | (data[1] << 16) | (data[2] << 8)
                         | data[3];
                seq_len = 1;
                source_len = 4;

                if (!seq[0]) {
                    /* End character. */
                    data += source_len;
                    data_size -= source_len;
                    err = CAHUTE_ERROR_TERMINATED;
                    goto end;
                }
                break;

            case CAHUTE_TEXT_ENCODING_UTF32_LE:
                if (data_size < 4)
                    goto end;

                seq[0] = (data[3] << 24) | (data[2] << 16) | (data[1] << 8)
                         | data[0];
                seq_len = 1;
                source_len = 4;

                if (!seq[0]) {
                    /* End character. */
                    data += source_len;
                    data_size -= source_len;
                    err = CAHUTE_ERROR_TERMINATED;
                    goto end;
                }
                break;

            case CAHUTE_TEXT_ENCODING_UTF8:
                err = cahute_get_utf8_char(
                    data,
                    data_size,
                    &seq[0],
                    &source_len
                );
                if (err)
                    goto end;

                if (!seq[0]) {
                    /* End character. */
                    data += source_len;
                    data_size -= source_len;
                    err = CAHUTE_ERROR_TERMINATED;
                    goto end;
                }

                seq_len = 1;
                break;

            default:
                CAHUTE_RETURN_IMPL(
                    "Unimplemented reading for Unicode conv loop."
                );
            }

            /* Write the Unicode sequence to the destination. */
            err = CAHUTE_ERROR_SIZE;
            switch (dest_encoding) {
            case CAHUTE_TEXT_ENCODING_UTF32_BE:
                if (buf_size < (seq_len << 2))
                    goto end;

                for (i = 0; i < seq_len; i++) {
                    unsigned long codepoint = seq[i];

                    *buf++ = (codepoint >> 24) & 255;
                    *buf++ = (codepoint >> 16) & 255;
                    *buf++ = (codepoint >> 8) & 255;
                    *buf++ = codepoint & 255;
                    buf_size -= 4;
                }
                break;

            case CAHUTE_TEXT_ENCODING_UTF32_LE:
                if (buf_size < (seq_len << 2))
                    goto end;

                for (i = 0; i < seq_len; i++) {
                    unsigned long codepoint = seq[i];

                    *buf++ = codepoint & 255;
                    *buf++ = (codepoint >> 8) & 255;
                    *buf++ = (codepoint >> 16) & 255;
                    *buf++ = (codepoint >> 24) & 255;
                    buf_size -= 4;
                }
                break;

            case CAHUTE_TEXT_ENCODING_UTF8: {
                size_t sz = 0;

                for (i = 0; i < seq_len; i++)
                    sz += cahute_get_utf8_char_size(seq[i]);

                if (buf_size < sz)
                    goto end;

                for (i = 0; i < seq_len; i++) {
                    sz = cahute_put_utf8_char(buf, seq[i]);
                    buf += sz;
                    buf_size -= sz;
                }
            } break;

            default:
                CAHUTE_RETURN_IMPL(
                    "Unimplemented writing for Unicode conv. loop."
                );
            }
        }
    } break;

    default:
        CAHUTE_RETURN_IMPL("Unimplemented conversion loop.");
    }

    err = CAHUTE_OK;
end:
    if (err && err == CAHUTE_ERROR_INVALID) {
        msg(ll_info,
            "Unable to parse from encoding %d, starting from:",
            source_encoding);
        mem(ll_info, data, data_size > 20 ? 20 : data_size);
    }

    *bufp = buf;
    *buf_sizep = buf_size;
    *datap = data;
    *data_sizep = data_size;
    return err;
}

/**
 * Convert text from any encoding into an UTF-8 buffer, with a NUL character.
 *
 * This is a shortcut to calling ``cahute_convert_text`` directly.
 *
 * @param buf Buffer in which to write.
 * @param buf_size Size of the buffer in which to write.
 * @param data Data from which to read.
 * @param data_size Size of the data from which to read.
 * @param encoding Encoding of the source data.
 * @return Cahute error, or 0 if the conversion has worked successfully.
 */
CAHUTE_EXTERN(int)
cahute_convert_to_utf8(
    char *buf,
    size_t buf_size,
    void const *data,
    size_t data_size,
    int encoding
) {
    int err;

    err = cahute_convert_text(
        (void **)&buf,
        &buf_size,
        &data,
        &data_size,
        CAHUTE_TEXT_ENCODING_UTF8,
        encoding
    );
    if (err)
        return err;

    /* We need to place the sentinel. */
    if (!buf_size)
        return CAHUTE_ERROR_SIZE;

    *buf = '\0';
    return CAHUTE_OK;
}
