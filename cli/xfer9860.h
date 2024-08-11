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

#ifndef XFER9860_H
#define XFER9860_H 1
#include <cahute.h>

#define OPERATION_UPLOAD   1
#define OPERATION_DOWNLOAD 2
#define OPERATION_INFO     3
#define OPERATION_OPTIMIZE 4

/**
 * Parsed arguments structure.
 *
 * The operations are the following:
 *
 * - UPLOAD contents of a {local_source_file} as {distant_target_name} on
 *   storage device "fls0".
 * - DOWNLOAD contents of {distant_source_name} from storage device "fls0"
 *   into {local_target_path}.
 * - Get INFO regarding the calculator.
 * - OPTIMIZE the "fls0" storage device.
 *
 * @property operation Selected operation.
 * @property throttle Throttle time in seconds.
 * @property distant_source_name Distant file name for download.
 * @property distant_target_name Distant file name for upload.
 * @property local_source_path Path of the local source file for upload.
 * @property local_target_path Path of the local target file for download.
 * @property local_source_file Local source file for upload.
 */
struct args {
    int operation;
    int throttle;
    char const *distant_source_name;
    char const *distant_target_name;
    char const *local_source_path;
    char const *local_target_path;
    cahute_file *local_source_file;
};

extern int parse_args(int argc, char **argv, struct args *args);

#endif /* XFER9860_H */
