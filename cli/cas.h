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

#ifndef CAS_H
#define CAS_H 1
#include <stdio.h>
#include <cahute.h>

/* Model. */
#define MODEL_UNKNOWN 0
#define MODEL_7700    1
#define MODEL_9700    2
#define MODEL_9750    4
#define MODEL_9800    8
#define MODEL_9850    16
#define MODEL_9950    32
#define MODEL_ANY     64

/* Input or output type. */
#define MEDIUM_UNKNOWN 0
#define MEDIUM_CTF     1
#define MEDIUM_CAS     2
#define MEDIUM_FXP     4
#define MEDIUM_BMP     8
#define MEDIUM_GIF     16
#define MEDIUM_COM     32

/* CASIOLINK model. */
#define HEADER_FORMAT_UNKNOWN 0
#define HEADER_FORMAT_CAS40   1
#define HEADER_FORMAT_CAS50   2
#define HEADER_FORMAT_RAW     4

/* Number format. */
#define NUMBER_FORMAT_BASIC 0
#define NUMBER_FORMAT_SPACE 1
#define NUMBER_FORMAT_DEC   2
#define NUMBER_FORMAT_OCT   3
#define NUMBER_FORMAT_HEX   4

/* File type, for conversions. */
#define FILE_TYPE_SSMONO  1
#define FILE_TYPE_SSCOL   2
#define FILE_TYPE_OLDPROG 3
#define FILE_TYPE_EDITOR  4

/**
 * Additional data for the CTF medium.
 *
 * @property glossary Flag to write the glossary to the output file.
 * @property nice Whether to use the "nice" token instead of the default one.
 */
struct ctf_medium {
    int glossary;
    int nice;
};

/**
 * Additional data for the CASIOLINK medium.
 *
 * @property header_format Header format for every entry in the CASIOLINK file.
 * @property status Whether to emit a message when a block is read or written.
 */
struct cas_medium {
    int header_format;
    int status;
};

/**
 * Additional data for the Bitmap medium.
 *
 * @property inverse Whether to read or write BMP in reverse order.
 */
struct bmp_medium {
    int inverse;
};

/**
 * Additional data for the GIF medium.
 *
 * @property inverse Whether to read or write GIF in reverse order.
 */
struct gif_medium {
    int inverse;
};

/**
 * Additional data for the COM medium.
 *
 * @property serial_flags Flags to pass to initialize the serial link.
 * @property serial_speed Serial speed to set.
 * @property pause Whether to pause before the communication with the
 *           calculator is established.
 * @property inline_protocol Whether inline communication should
 *           be established.
 * @property overwrite Whether to automatically overwrite.
 */
struct com_medium {
    unsigned long serial_flags;
    unsigned long serial_speed;
    int pause;
    int inline_protocol;
    int overwrite;
};

/**
 * Medium available data.
 *
 * @property ctf Additional data for CTF medium.
 * @property cas Additional data for CASIOLINK file.
 * @property bmp Additional data for Bitmap image.
 * @property gif Additional data for GIF image.
 * @property com Additional data for serial port.
 */
union medium_data {
    struct ctf_medium ctf;
    struct cas_medium cas;
    struct bmp_medium bmp;
    struct gif_medium gif;
    struct com_medium com;
};

/**
 * Medium.
 *
 * @property type Type of medium.
 * @property data Additional data for the medium.
 * @property path Path or device name to the medium.
 */
struct medium {
    int type;
    union medium_data data;
    char const *path;
};

/**
 * Listing properties.
 *
 * @property number_format Number format, as a ``NUMBER_FORMAT_*`` constant.
 * @property nice Use nice format.
 * @property password Whether to show password.
 */
struct list_format {
    int number_format;
    int nice;
    int password;
};

/**
 * All listing formats.
 *
 * @property * Listing properties for a given type.
 */
struct list_formats {
    struct list_format oldprog;
    struct list_format editor;
    struct list_format fn;
    struct list_format ssmono;
    struct list_format sscol;
    struct list_format varmem;
    struct list_format defmem;
    struct list_format allmem;
    struct list_format sd;
    struct list_format lr;
    struct list_format matrix;
    struct list_format rectab;
    struct list_format fntab;
    struct list_format poly;
    struct list_format simul;
    struct list_format zoom;
    struct list_format dyna;
    struct list_format graphs;
    struct list_format range;
    struct list_format backup;
    struct list_format end;
    struct list_format raw;
    struct list_format text;
    struct list_format desc;
};

/**
 * Conversion chained list node.
 *
 * @param next Next conversion node.
 * @param source_type Source file type.
 * @param dest_type Destination file type.
 * @param after Whether the conversion should be done afterwards.
 */
struct conversion {
    struct conversion *next;
    int source_type;
    int dest_type;
    int after;
};

/**
 * Parsed arguments structure.
 *
 * @property model Calculator model.
 * @property should_list_files Whether we should list the obtained files.
 * @property should_list_types Whether we should list the obtained file types.
 * @property verbose Whether verbose mode has been enabled.
 * @property should_output Whether we should output the file.
 * @property pager Whether a terminal pager should be used.
 * @property in_type Input medium type.
 * @property out_type Output medium type.
 */
struct args {
    int model;
    int should_list_files;
    int should_list_types;
    int verbose;
    int should_output;
    int pager;
    struct medium in;
    struct medium out;
    struct list_formats list;
    struct conversion *conversions;
};

extern int parse_args(int argc, char **argv, struct args *args);
extern void free_args(struct args *args);

#endif /* CAS_H */
