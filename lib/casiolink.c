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

/* ---
 * Common utilities to decode CASIOLINK data.
 * --- */

/**
 * Read the first 40 bytes of a CASIOLINK header to determine the type.
 *
 * @param data First 40 bytes of the CASIOLINK header, including the 0x3A.
 * @return Variant.
 */
CAHUTE_EXTERN(int)
cahute_casiolink_determine_header_variant(cahute_u8 const *data) {
    /* We want to try to determine the currently selected variant based
     * on the header's content. */
    if (!memcmp(&data[1], "ADN1", 4) || !memcmp(&data[1], "ADN2", 4)
        || !memcmp(&data[1], "BKU1", 4) || !memcmp(&data[1], "END1", 4)
        || !memcmp(&data[1], "FCL1", 4) || !memcmp(&data[1], "FMV1", 4)
        || !memcmp(&data[1], "MCS1", 4) || !memcmp(&data[1], "MDL1", 4)
        || !memcmp(&data[1], "REQ1", 4) || !memcmp(&data[1], "REQ2", 4)
        || !memcmp(&data[1], "SET1", 4)) {
        /* The type seems to be a CAS100 header type we can use. */
        return CAHUTE_CASIOLINK_VARIANT_CAS100;
    }

    if (!memcmp(&data[1], "END\xFF", 4) || !memcmp(&data[1], "FNC", 4)
        || !memcmp(&data[1], "IMG", 4) || !memcmp(&data[1], "MEM", 4)
        || !memcmp(&data[1], "REQ", 4) || !memcmp(&data[1], "TXT", 4)
        || !memcmp(&data[1], "VAL", 4)) {
        /* The type seems to be a CAS50 header type.
         * This means that we actually have 10 more bytes to read for
         * a full header.
         *
         * NOTE: The '4' in the memcmp() calls above are intentional,
         * as the NUL character ('\0) is actually considered as part of
         * the CAS50 header type. */
        return CAHUTE_CASIOLINK_VARIANT_CAS50;
    }

    /* By default, we consider the header to be a CAS40 header. */
    return CAHUTE_CASIOLINK_VARIANT_CAS40;
}

/**
 * Determine the number, size and type of data packets following a header.
 *
 * @param data CASIOLINK header, including the 0x3A.
 * @param variant Variant for which to determine the elements.
 * @param desc Data description to fill.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_casiolink_determine_data_description(
    cahute_u8 const *data,
    int variant,
    cahute_casiolink_data_description *desc
) {
    desc->flags = 0;
    desc->packet_type = PACKET_TYPE_HEADER;
    desc->part_count = 1;
    desc->last_part_repeat = 1;
    desc->part_sizes[0] = 0;

    switch (variant) {
    case CAHUTE_CASIOLINK_VARIANT_CAS40:
        if (!memcmp(&data[1], "\x17\x17", 2)) {
            /* CAS40 AL End */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_AL_END;
            desc->part_count = 0;
        } else if (!memcmp(&data[1], "\x17\xFF", 2)) {
            /* CAS40 End */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_END;
            desc->part_count = 0;
        } else if (!memcmp(&data[1], "A1", 2)) {
            /* CAS40 Dynamic Graph */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
            if (desc->part_sizes[0] > 2)
                desc->part_sizes[0] -= 2;
        } else if (!memcmp(&data[1], "AA", 2)) {
            /* CAS40 Dynamic Graph in Bulk */
            desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
            if (desc->part_sizes[0] > 2)
                desc->part_sizes[0] -= 2;
        } else if (!memcmp(&data[1], "AD", 2)) {
            /* CAS40 All Memories */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->last_part_repeat = ((size_t)data[5] << 8) | data[6];
            desc->part_sizes[0] = 22;
        } else if (!memcmp(&data[1], "AL", 2)) {
            /* CAS40 All */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_AL;
            desc->part_count = 0;
        } else if (!memcmp(&data[1], "AM", 2)) {
            /* CAS40 Variable Memories */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->last_part_repeat = ((size_t)data[5] << 8) | data[6];
            desc->part_sizes[0] = 22;
        } else if (!memcmp(&data[1], "BU", 2)) {
            /* CAS40 Backup */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            if (!memcmp(&data[3], "TYPEA00", 7))
                desc->part_sizes[0] = 32768;
            else if (!memcmp(&data[3], "TYPEA02", 7))
                desc->part_sizes[0] = 32768;
        } else if (!memcmp(&data[1], "DC", 2)) {
            /* CAS40 Color Screenshot. */
            unsigned int width = data[3], height = data[4];

            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL
                           | CAHUTE_CASIOLINK_DATA_FLAG_NO_LOG;
            if (!memcmp(&data[5], "\x11UWF\x03", 4)) {
                desc->last_part_repeat = 3;
                desc->part_sizes[0] =
                    1 + ((width >> 3) + !!(width & 7)) * height;
            }
        } else if (!memcmp(&data[1], "DD", 2)) {
            /* CAS40 Monochrome Screenshot. */
            unsigned int width = data[3], height = data[4];

            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL
                           | CAHUTE_CASIOLINK_DATA_FLAG_NO_LOG;
            if (!memcmp(&data[5], "\x10\x44WF", 4))
                desc->part_sizes[0] = ((width >> 3) + !!(width & 7)) * height;
        } else if (!memcmp(&data[1], "DM", 2)) {
            /* CAS40 Defined Memories */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->last_part_repeat = ((size_t)data[5] << 8) | data[6];
            desc->part_sizes[0] = 22;
        } else if (!memcmp(&data[1], "EN", 2)) {
            /* CAS40 Single Editor Program */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
            if (desc->part_sizes[0] >= 2)
                desc->part_sizes[0] -= 2;
        } else if (!memcmp(&data[1], "EP", 2)) {
            /* CAS40 Single Password Protected Editor Program */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
            if (desc->part_sizes[0] >= 2)
                desc->part_sizes[0] -= 2;
        } else if (!memcmp(&data[1], "F1", 2)) {
            /* CAS40 Single Function */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
            if (desc->part_sizes[0] >= 2)
                desc->part_sizes[0] -= 2;
        } else if (!memcmp(&data[1], "F6", 2)) {
            /* CAS40 Multiple Functions */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
            if (desc->part_sizes[0] >= 2)
                desc->part_sizes[0] -= 2;
        } else if (!memcmp(&data[1], "FN", 2)) {
            /* CAS40 Single Editor Program in Bulk */
            desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
            if (desc->part_sizes[0] >= 2)
                desc->part_sizes[0] -= 2;
        } else if (!memcmp(&data[1], "FP", 2)) {
            /* CAS40 Single Password Protected Editor Program in Bulk */
            desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
            if (desc->part_sizes[0] >= 2)
                desc->part_sizes[0] -= 2;
        } else if (!memcmp(&data[1], "G1", 2)) {
            /* CAS40 Graph Function */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
            if (desc->part_sizes[0] >= 2)
                desc->part_sizes[0] -= 2;
        } else if (!memcmp(&data[1], "GA", 2)) {
            /* CAS40 Graph Function in Bulk */
            desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
            if (desc->part_sizes[0] >= 2)
                desc->part_sizes[0] -= 2;
        } else if (!memcmp(&data[1], "GF", 2)) {
            /* CAS40 Factor */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->part_sizes[0] = 2 + data[6] * 10;
        } else if (!memcmp(&data[1], "GR", 2)) {
            /* CAS40 Range */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->part_sizes[0] = 92;
        } else if (!memcmp(&data[1], "GT", 2)) {
            /* CAS40 Function Table */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->part_count = 3;
            desc->last_part_repeat = ((size_t)data[7] << 8) | data[8];
            desc->part_sizes[0] = data[6];
            if (desc->part_sizes[0] >= 2)
                desc->part_sizes[0] -= 2;

            desc->part_sizes[1] = 32;
            desc->part_sizes[2] = 22;
        } else if (!memcmp(&data[1], "M1", 2)) {
            /* CAS40 Single Matrix */
            unsigned int width = data[5], height = data[6];

            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->part_sizes[0] = 14;
            desc->last_part_repeat =
                width * height + 1; /* Sentinel data part. */
        } else if (!memcmp(&data[1], "MA", 2)) {
            /* CAS40 Single Matrix in Bulk */
            unsigned int width = data[5], height = data[6];

            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->part_sizes[0] = 14;
            desc->last_part_repeat = width * height;
        } else if (!memcmp(&data[1], "P1", 2)) {
            /* CAS40 Single Numbered Program. */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->part_sizes[0] = ((size_t)data[4] << 8) | data[5];
            if (desc->part_sizes[0] >= 2)
                desc->part_sizes[0] -= 2;
        } else if (!memcmp(&data[1], "PD", 2)) {
            /* CAS40 Polynomial Equation */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->part_sizes[0] = data[6] * 10 + 12;
        } else if (!memcmp(&data[1], "PZ", 2)) {
            /* CAS40 Multiple Numbered Programs */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->part_count = 2;
            desc->part_sizes[0] = 190;
            desc->part_sizes[1] = ((size_t)data[4] << 8) | data[5];
            if (desc->part_sizes[1] >= 2)
                desc->part_sizes[1] -= 2;
        } else if (!memcmp(&data[1], "RT", 2)) {
            /* CAS40 Recursion Table */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->part_count = 3;
            desc->last_part_repeat = ((size_t)data[7] << 8) | data[8];
            desc->part_sizes[0] = data[6];
            if (desc->part_sizes[0] >= 2)
                desc->part_sizes[0] -= 2;

            desc->part_sizes[1] = 22;
            desc->part_sizes[2] = 32;
        } else if (!memcmp(&data[1], "SD", 2)) {
            /* CAS40 Simultaneous Equations */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->last_part_repeat = data[5] * data[6] + 1;
            desc->part_sizes[0] = 14;
        } else if (!memcmp(&data[1], "SR", 2)) {
            /* CAS40 Paired Variable Data */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->last_part_repeat = ((size_t)data[5] << 8) | data[6];
            desc->part_sizes[0] = 32;
        } else if (!memcmp(&data[1], "SS", 2)) {
            /* CAS40 Single Variable Data */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            desc->last_part_repeat = ((size_t)data[5] << 8) | data[6];
            desc->part_sizes[0] = 22;
        }

        break;

    case CAHUTE_CASIOLINK_VARIANT_CAS50:
        if (!memcmp(&data[1], "END\xFF", 4)) {
            /* End packet for CAS50. */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_END;
            desc->part_count = 0;
        } else if (!memcmp(&data[1], "VAL", 4)) {
            unsigned int height = ((unsigned int)data[7] << 8) | data[8];
            unsigned int width = ((unsigned int)data[9] << 8) | data[10];

            /* Variable data use size as W*H, or only W, or only H depending
             * on the case. */
            if (!width)
                width = 1;

            desc->part_sizes[0] = 14;
            desc->last_part_repeat = height * width;
        } else {
            /* For other packets, the size should always be located at
             * offset 6 of the header, i.e. offset 7 of the buffer. */
            desc->part_sizes[0] = ((size_t)data[7] << 24)
                                  | ((size_t)data[8] << 16)
                                  | ((size_t)data[9] << 8) | data[10];

            if (desc->part_sizes[0] > 2)
                desc->part_sizes[0] -= 2;
            else
                desc->part_count = 0;

            if (!memcmp(&data[1], "MEM\0BU", 6)) {
                /* Backups are guaranteed to be the final (and only) file
                 * sent in the communication. */
                desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_FINAL;
            }
        }
        break;

    case CAHUTE_CASIOLINK_VARIANT_CAS100:
        if (!memcmp(&data[1], "BKU1", 4)) {
            /* Backup packet for CAS100. */
            desc->part_sizes[0] = ((size_t)data[9] << 24)
                                  | ((size_t)data[10] << 16)
                                  | ((size_t)data[11] << 8) | data[12];
        } else if (!memcmp(&data[1], "END1", 4)) {
            /* End packet for CAS100. */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_END;
            desc->part_count = 0;
        } else if (!memcmp(&data[1], "MCS1", 4)) {
            /* Main memory packet for CAS100. */
            desc->part_sizes[0] = ((size_t)data[8] << 8) | data[9];
            if (!desc->part_sizes[0])
                desc->part_count = 0;
        } else if (!memcmp(&data[1], "MDL1", 4)) {
            /* Initialization packet for CAS100. */
            desc->flags |= CAHUTE_CASIOLINK_DATA_FLAG_MDL;
            desc->part_count = 0;
        } else if (!memcmp(&data[1], "SET1", 4)) {
            /* TODO */
            desc->part_count = 0;
        }

        break;

    default:
        msg(ll_error, "Unhandled variant 0x%08X.", variant);
        return CAHUTE_ERROR_UNKNOWN;
    }

    if (desc->part_count && !desc->part_sizes[0]) {
        /* 'part_count' and 'part_sizes[0]' were left to their default values
         * of 1 and 0 respectively, which means they have not been set to
         * a found type. */
        return CAHUTE_ERROR_UNKNOWN;
    }

    return CAHUTE_OK;
}

/**
 * Decode a CASIOLINK data.
 *
 * @param final_datap Pointer to the data to create.
 * @param file File object to read from.
 * @param offsetp Pointer to the offset in the file to read from, to set to
 *        the offset after the data afterwards.
 * @param variant Variant to read the data with.
 * @param check_data Whether to check type and checksums for the data parts.
 * @return Error, or 0 if successful.
 */
CAHUTE_EXTERN(int)
cahute_casiolink_decode_data(
    cahute_data **final_datap,
    cahute_file *file,
    unsigned long *offsetp,
    int variant,
    int check_data
) {
    cahute_data *data = NULL;
    cahute_data **datap = &data;
    cahute_u8 header_buf[50];
    size_t header_size = 40;
    cahute_casiolink_data_description desc;
    unsigned long offset = *offsetp;
    int err;

    /* Read the header from the buffer, and extract the variant if need be. */
    if (variant == CAHUTE_CASIOLINK_VARIANT_AUTO) {
        err = cahute_read_from_file(file, offset, header_buf, 40);
        if (err)
            goto fail;

        variant = cahute_casiolink_determine_header_variant(header_buf);
        if (variant == CAHUTE_CASIOLINK_VARIANT_CAS50) {
            err =
                cahute_read_from_file(file, offset + 40, &header_buf[40], 10);
            if (err)
                goto fail;

            header_size = 50;
        }
    } else {
        header_size = variant == CAHUTE_CASIOLINK_VARIANT_CAS50 ? 50 : 40;

        err = cahute_read_from_file(file, offset, header_buf, header_size);
        if (err)
            goto fail;
    }

    offset += header_size;

    /* Only check the header if not already checked, i.e. in the case of files
     * and not in the case of links. */
    if (check_data) {
        unsigned int obtained_checksum = header_buf[header_size - 1];
        unsigned int expected_checksum;

        if (header_buf[0] != PACKET_TYPE_HEADER) {
            msg(ll_error,
                "Header type 0x%02X is not the expected 0x%02X.",
                header_buf[0],
                PACKET_TYPE_HEADER);
            err = CAHUTE_ERROR_CORRUPT;
            goto fail;
        }

        expected_checksum =
            cahute_casiolink_checksum(&header_buf[1], header_size - 2);
        if (header_buf[header_size - 1] != expected_checksum) {
            msg(ll_error,
                "Header checksum 0x%02X is different from expected checksum "
                "%02X.",
                obtained_checksum,
                expected_checksum);
            err = CAHUTE_ERROR_CORRUPT;
            goto fail;
        }
    }

    /* We need to get the data description in order to at least place the
     * offset after the current header and data part, even if it is not
     * implemented, in order for file reading to use CAHUTE_ERROR_IMPL errors
     * to skip unimplemented file types.
     *
     * NOTE: We only update ``*offsetp`` here and NOT ``offset``, because
     * ``offset`` is actually used in data decoding later on in the
     * function. */
    err = cahute_casiolink_determine_data_description(
        header_buf,
        variant,
        &desc
    );
    if (err)
        goto fail;

    {
        unsigned long total_size = 0;
        size_t part_i;

        if (desc.part_count) {
            for (part_i = 0; part_i < desc.part_count - 1; part_i++)
                total_size += desc.part_sizes[part_i] + 2;

            total_size += (desc.part_sizes[desc.part_count - 1] + 2)
                          * desc.last_part_repeat;
        }

        *offsetp = offset + total_size;
    }

    /* Only check data if not already checked, i.e. in the case of files and
     * not in the case of links (where all data is not available in one go,
     * i.e. the receiver must determine the data description, check the packet
     * type and checksums and acknowledge every part of the data). */
    if (check_data) {
        unsigned long offset_check = offset;

        if (desc.part_count) {
            cahute_u8 tmp_buf[256];
            size_t part_i, total_parts;

            total_parts = desc.part_count - 1 + desc.last_part_repeat;
            for (part_i = 0; part_i < total_parts; part_i++) {
                size_t part_size =
                    desc.part_sizes
                        [part_i >= desc.part_count ? desc.part_count - 1
                                                   : part_i];
                unsigned int checksum = 0;

                err = cahute_read_from_file(file, offset_check++, tmp_buf, 1);
                if (err)
                    goto fail;

                if (tmp_buf[0] != desc.packet_type) {
                    err = CAHUTE_ERROR_CORRUPT;
                    goto fail;
                }

                /* NOTE: Since we read in a buffered mode and not in a full
                 * buffer mode, we reproduce the behaviour of
                 * ``cahute_casiolink_checksum()`` ourselves here. */
                while (part_size) {
                    size_t to_read = part_size > sizeof(tmp_buf)
                                         ? sizeof(tmp_buf)
                                         : part_size;
                    size_t i;

                    err = cahute_read_from_file(
                        file,
                        offset_check,
                        tmp_buf,
                        to_read
                    );
                    if (err)
                        goto fail;

                    offset_check += to_read;
                    for (i = 0; i < to_read; i++)
                        checksum += tmp_buf[i];

                    part_size -= to_read;
                }

                err = cahute_read_from_file(file, offset_check++, tmp_buf, 1);
                if (err)
                    goto fail;

                if (tmp_buf[0] != ((~checksum + 1) & 255)) {
                    err = CAHUTE_ERROR_CORRUPT;
                    goto fail;
                }
            }
        }
    }

    /* File decoding time!
     * The file type decoding can count on the following variables to be
     * available and set:
     *
     * - ``variant`` and ``desc``, if need be.
     * - ``header_buf``, of 40 or 50 bytes as documented in ``header_size``.
     * - ``file`` and ``offset``, the offset being set to the offset right
     *   after the complete header (i.e. at the first data part, if there are
     *   some).
     *
     * From here, either the file type decoding goes along or goes to "fail",
     * which assumes "err" to be set, or it goes to "data_ready", with
     * the following variables expected to be set:
     *
     * - ``datap`` to the pointer where to set the next data, or to one of
     *   the data that could lead to the last pointer by going through the
     *   chain;
     * - ``data`` to the pointer to the first data read, to set as the
     *   result of the function. */
    switch (variant) {
    case CAHUTE_CASIOLINK_VARIANT_CAS40:
        if (!memcmp(&header_buf[1], "P1", 2)) {
            size_t program_size = ((size_t)header_buf[4] << 8) | header_buf[5];

            /* CAS40 Single Numbered Program. */
            err = cahute_create_program_from_file(
                datap,
                CAHUTE_TEXT_ENCODING_LEGACY_8,
                NULL, /* No program name, this is anonymous. */
                0,
                NULL, /* No password. */
                0,
                file,
                offset + 1,
                program_size
            );
            if (err)
                goto fail;

            goto data_ready;
        }

        if (!memcmp(&header_buf[1], "PZ", 2)) {
            cahute_u8 programs_header[190];
            cahute_u8 const *buf = programs_header;
            cahute_u8 const *names = pz_program_names;
            int i = 0;

            /* CAS40 Multiple Numbered Programs
             * This is made of 38 programs, with all 5-byte headers placed
             * consecutively in a first data part, then all contents placed
             * consecutively in a second data part. */
            err =
                cahute_read_from_file(file, offset + 1, programs_header, 190);
            if (err)
                goto fail;

            offset += 193; /* Content, 2 packet types, 1 checksum. */
            for (i = 1; i < 39; i++) {
                size_t program_length = ((size_t)buf[1] << 8) | buf[2];

                if (program_length >= 2)
                    program_length -= 2;

                err = cahute_create_program_from_file(
                    datap,
                    CAHUTE_TEXT_ENCODING_LEGACY_8,
                    names++,
                    1,
                    NULL, /* No password. */
                    0,
                    file,
                    offset,
                    program_length
                );
                if (err)
                    goto fail;

                datap = &(*datap)->cahute_data_next;
                buf += 5;
                offset += program_length;
            }

            goto data_ready;
        }

        break;

    case CAHUTE_CASIOLINK_VARIANT_CAS50:
        if (!memcmp(&header_buf[1], "TXT", 4)) {
            size_t data_size = ((size_t)header_buf[7] << 24)
                               | ((size_t)header_buf[8] << 16)
                               | ((size_t)header_buf[9] << 8) | header_buf[10];

            if (data_size >= 2)
                data_size -= 2;

            if (!memcmp(&header_buf[5], "PG", 2)) {
                err = cahute_create_program_from_file(
                    datap,
                    CAHUTE_TEXT_ENCODING_LEGACY_8,
                    &header_buf[11],
                    8,
                    &header_buf[27],
                    8,
                    file,
                    offset + 1,
                    data_size
                );

                if (err)
                    goto fail;

                goto data_ready;
            }
        }
        break;

    case CAHUTE_CASIOLINK_VARIANT_CAS100:
        if (!memcmp(&header_buf[1], "MCS1", 4)) {
            size_t size = ((size_t)header_buf[8] << 8) | header_buf[9];

            err = cahute_mcs_decode_data(
                datap,
                &header_buf[19],
                8,
                NULL, /* MCS1 packet does not present a directory. */
                0,
                &header_buf[11],
                8,
                file,
                offset + 1,
                size,
                header_buf[10]
            );
            if (err && err != CAHUTE_ERROR_IMPL)
                goto fail;

            goto data_ready;
        }
        break;

    default:
        msg(ll_error, "Unhandled variant 0x%08X.", variant);
        err = CAHUTE_ERROR_UNKNOWN;
        goto fail;
    }

    msg(ll_error, "Unhandled data with the following header:");
    mem(ll_error, header_buf, header_size);

fail:
    cahute_destroy_data(data);
    return err;

data_ready:
    while (*datap)
        datap = &(*datap)->cahute_data_next;

    /* NOTE: ``*offsetp`` was updated earlier, to be correctly set even in
     * the case of invalid or unsupported data types. */
    *datap = *final_datap;
    *final_datap = data;

    return CAHUTE_OK;
}

/* ---
 * Protocol related utilities.
 * --- */

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
    size_t buf_size;
    int packet_type, err, variant = 0, checksum, checksum_alt;
    cahute_casiolink_data_description desc;

restart_reception:
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
        variant = cahute_casiolink_determine_header_variant(buf);
        switch (variant) {
        case CAHUTE_CASIOLINK_VARIANT_CAS40:
            msg(ll_info, "Variant is determined to be CAS40.");
            msg(ll_info, "Received the following header:");
            mem(ll_info, buf, 40);
            break;

        case CAHUTE_CASIOLINK_VARIANT_CAS50:
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

            msg(ll_info, "Received the following header:");
            mem(ll_info, buf, 50);
            break;

        case CAHUTE_CASIOLINK_VARIANT_CAS100:
            msg(ll_info, "Variant is determined to be CAS100.");
            msg(ll_info, "Received the following header:");
            mem(ll_info, buf, 40);
            break;

        default:
            msg(ll_error, "Unknown variant %d.", variant);
            return CAHUTE_ERROR_UNKNOWN;
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

    err = cahute_casiolink_determine_data_description(buf, variant, &desc);
    if (err) {
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

    if (desc.flags & CAHUTE_CASIOLINK_DATA_FLAG_MDL) {
        err = cahute_casiolink_handle_mdl1(link);
        if (err)
            return err;

        /* From here, we go back to the beginning. */
        goto restart_reception;
    }

    if (desc.part_count) {
        size_t total_size = buf_size;
        size_t part_i;

        for (part_i = 0; part_i < desc.part_count - 1; part_i++)
            total_size += desc.part_sizes[part_i] + 2;

        total_size +=
            (desc.part_sizes[desc.part_count - 1] + 2) * desc.last_part_repeat;

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

    if (desc.part_count) {
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
        total = desc.part_count - 1 + desc.last_part_repeat;
        for (part_i = 0; part_i < total; part_i++, index++) {
            size_t part_size =
                desc.part_sizes
                    [part_i >= desc.part_count ? desc.part_count - 1 : part_i];

            msg(ll_info,
                "Reading data part %d/%d (%" CAHUTE_PRIuSIZE "o).",
                index,
                total,
                part_size);

            err = cahute_read_from_link(
                link,
                buf,
                1,
                TIMEOUT_PACKET_CONTENTS,
                TIMEOUT_PACKET_CONTENTS
            );
            if (err == CAHUTE_ERROR_TIMEOUT_START)
                return CAHUTE_ERROR_TIMEOUT;
            if (err)
                return err;

            if (*buf != desc.packet_type) {
                msg(ll_error,
                    "Expected 0x%02X packet type, got 0x%02X.",
                    desc.packet_type,
                    buf[0]);
                return CAHUTE_ERROR_UNKNOWN;
            }

            if (part_size) {
                size_t part_size_left = part_size;
                cahute_u8 *p = &buf[1];

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

                /* For color screenshots, sometimes the first byte is not
                 * taken into account in the checksum calculation, as it's
                 * metadata for the sheet and not the "actual data" of the
                 * sheet. But sometimes it also gets the checksum right!
                 * In any case, we want to compute and check both checksums
                 * to see if at least one matches. */
                checksum = cahute_casiolink_checksum(&buf[1], part_size);
                checksum_alt =
                    cahute_casiolink_checksum(buf + 1, part_size - 1);
            } else {
                checksum = 0;
                checksum_alt = 0;
            }

            /* Read and check the checksum. */
            err = cahute_read_from_link(
                link,
                &buf[1 + part_size],
                1,
                TIMEOUT_PACKET_CONTENTS,
                TIMEOUT_PACKET_CONTENTS
            );
            if (err == CAHUTE_ERROR_TIMEOUT_START)
                return CAHUTE_ERROR_TIMEOUT;
            if (err)
                return err;

            if (checksum != buf[1 + part_size]
                && checksum_alt != buf[1 + part_size]) {
                cahute_u8 const send_buf[] = {PACKET_TYPE_INVALID_DATA};

                msg(ll_warn,
                    "Invalid checksum (expected: 0x%02X, computed: "
                    "0x%02X).",
                    buf[part_size],
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
            if ((~desc.flags & CAHUTE_CASIOLINK_DATA_FLAG_NO_LOG)
                && part_size <= 4096) /* Let's not flood the terminal. */
                mem(ll_info, buf, part_size);

            buf += part_size + 2;
            buf_size += part_size + 2;
        }
    }

    link->protocol_state.casiolink.last_variant = variant;
    link->data_buffer_size = buf_size;

    if (desc.flags & CAHUTE_CASIOLINK_DATA_FLAG_AL)
        link->flags |= CAHUTE_LINK_FLAG_ALMODE;

    if ((desc.flags & CAHUTE_CASIOLINK_DATA_FLAG_AL_END)
        || ((desc.flags & CAHUTE_CASIOLINK_DATA_FLAG_END)
            && (~link->flags & CAHUTE_LINK_FLAG_ALMODE))) {
        /* The packet was an end packet. */
        link->flags |= CAHUTE_LINK_FLAG_TERMINATED;
        msg(ll_info, "Received data was a sentinel!");
        return CAHUTE_ERROR_TERMINATED;
    }

    if ((desc.flags & CAHUTE_CASIOLINK_DATA_FLAG_FINAL)
        && (~link->flags & CAHUTE_LINK_FLAG_ALMODE)) {
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
    cahute_file file;
    unsigned long offset = 0;
    int err;

    do {
        err = cahute_casiolink_receive_raw_data(link, timeout);
        if (err == CAHUTE_ERROR_TIMEOUT_START) {
            msg(ll_error, "No data received in a timely matter, exiting.");
            break;
        }

        if (err)
            return err;

        cahute_populate_file_from_memory(
            &file,
            link->data_buffer,
            link->data_buffer_size
        );
        offset = 0;

        err = cahute_casiolink_decode_data(
            datap,
            &file,
            &offset,
            link->protocol_state.casiolink.last_variant,
            0 /* Already checked in ``cahute_casiolink_receive_raw_data()`` */
        );
        if (!err || err != CAHUTE_ERROR_IMPL)
            return err;

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
