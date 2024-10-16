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

#ifndef COMMON_H
#define COMMON_H 1
#include <cahute.h>
#include <compat.h>

extern char const *get_current_log_level(void);
extern void set_log_level(char const *loglevel);

extern void print_content(
    void const *data,
    size_t data_size,
    int encoding,
    int dest_encoding
);

extern int parse_serial_attributes(
    char const *raw,
    unsigned long *flagsp,
    unsigned long *speedp
);

extern int
read_file_contents(char const *path, cahute_u8 **datap, size_t *sizep);

/* Portable getdelim() implementation. */
extern cahute_ssize
portable_getdelim(char **sp, size_t *np, int delim, FILE *filep);

/* Portable strnlen() implementation. */
extern size_t portable_strnlen(char const *s, size_t maxlen);

#endif /* COMMON_H */
