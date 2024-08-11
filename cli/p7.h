/* ****************************************************************************
 * Copyright (C) 2016-2017, 2024 Thomas Touhey <thomas@touhey.fr>
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

#ifndef P7_H
#define P7_H 1
#include <stdio.h>
#include "common.h"
#define DEFAULT_STORAGE "fls0"

#define COMMAND_LIST_SERIAL 1
#define COMMAND_SEND        2
#define COMMAND_GET         3
#define COMMAND_COPY        4
#define COMMAND_DELETE      5
#define COMMAND_LIST        6
#define COMMAND_RESET       7
#define COMMAND_OPTIMIZE    8
#define COMMAND_INFO        9
#define COMMAND_IDLE        10

/**
 * Parsed argument structure.
 *
 * The combinations for the operations on the file system are the following:
 *
 * - SEND a {local_source_file}'s content to a {distant_target_name}, in the
 *   {distant_target_directory_name} directory on {storage_name}.
 * - GET {distant_source_name}, in the {distant_source_directory_name}
 *   directory on {storage_name}, to {local_target_path}.
 * - COPY {distant_source_name}, in the {distant_source_directory_name}
 *   directory, to {distant_target_name}, in the
 *   {distant_target_directory_name}, on {storage_name}.
 * - DELETE {distant_target_name}, in the {distant_target_directory_name}
 *   directory, on {storage_name}.
 * - LIST files in the {distant_target_directory_name} directory
 *   on {storage_name}.
 * - RESET {storage_name}.
 * - OPTIMIZE {storage_name}.
 *
 * General properties:
 *
 * @property command Selected subcommand.
 * @property nice_display Whether nice display is enabled or not.
 * @property force Whether to force overwrite or not.
 *
 * Connection properties:
 *
 * @property serial_flags Serial flags to define.
 * @property serial_speed Speed to use.
 * @property new_serial_flags Serial flags to update the connection to.
 * @property new_serial_speed Serial speed to update the connection to.
 * @property no_init If a connection is established, whether to initialize
 *           the connection (0) or not (1).
 * @property no_term If a connection is established, whether to terminate
 *           the connection (0) or not (1).
 * @property change_serial Whether to set new serial attributes or not.
 * @property serial_name Serial device's name or path.
 *
 * Distant filesystem properties:
 *
 * @property storage_name Optional storage name for operations with the
 *           calculator.
 * @property distant_source_directory_name Optional source directory name for
 *           copy, or directory name when getting a file.
 * @property distant_source_name Source file name for copy, or file name
 *           when getting a file.
 * @property distant_target_directory_name Optional target directory name for
 *           copy, or directory name when uploading a file.
 * @property distant_target_name Target file name for copy, or file name
 *           when sending a file.
 *
 * Local filesystem properties:
 *
 * @property local_source_path Path to the local file when uploading a file.
 * @property local_source_file Local file object for uploading a file.
 * @property local_target_path Path to the local file when downloading a file.
 */
struct args {
    int command;
    int nice_display;
    int force;

    /* Connection-related parameters. */
    unsigned long serial_flags;
    unsigned long serial_speed;
    unsigned long new_serial_flags;
    unsigned long new_serial_speed;
    int no_init;
    int no_term;
    int change_serial;
    char const *serial_name;

    /* Calculator storage related parameters. */
    char const *storage_name;
    char const *distant_source_directory_name;
    char const *distant_source_name;
    char const *distant_target_directory_name;
    char const *distant_target_name;

    /* Local data.
     * The local source fp may be defined without the local source path being
     * defined, which means we are actually targetting a special stream,
     * such as stdin. Same for the target with stdout or stderr. */
    char const *local_source_path;
    char const *local_target_path;
    cahute_file *local_source_file;
};

extern int parse_args(int ac, char **av, struct args *args);

#endif /* P7_H */
