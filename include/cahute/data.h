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

#ifndef CAHUTE_DATA_H
#define CAHUTE_DATA_H 1
#include "cdefs.h"

CAHUTE_BEGIN_NAMESPACE
CAHUTE_DECLARE_TYPE(cahute_data)

#define CAHUTE_DATA_TYPE_PROGRAM 1

struct cahute__data_content_program {
    int cahute_data_content_program_encoding;

    size_t cahute_data_content_program_name_size;
    void *cahute_data_content_program_name;

    size_t cahute_data_content_program_password_size;
    void *cahute_data_content_program_password;

    size_t cahute_data_content_program_size;
    void *cahute_data_content_program_content;

    /* TODO: mode */
};

union cahute__data_content {
    struct cahute__data_content_program cahute_data_content_program;
};

struct cahute_data {
    struct cahute_data *cahute_data_next;
    int cahute_data_type;
    union cahute__data_content cahute_data_content;
};

CAHUTE_BEGIN_DECLS

CAHUTE_EXTERN(void) cahute_destroy_data(cahute_data *cahute__data);

CAHUTE_END_DECLS
CAHUTE_END_NAMESPACE

#endif /* CAHUTE_DATA_H */
