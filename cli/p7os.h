/* ****************************************************************************
 * Copyright (C) 2017, 2024 Thomas Touhey <thomas@touhey.fr>
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

#ifndef P7OS_H
#define P7OS_H 1
#include <cahute.h>
#define COMMAND_NONE   0
#define COMMAND_BACKUP 1
#define COMMAND_FLASH  2

/**
 * Parsed argument structure.
 *
 * @property command Selected subcommand.
 * @property upload_uexe Whether to upload the Update.EXE to the calculator.
 * @property erase_flash Whether to erase flash before writing data.
 * @property display_progress Whether to display a progress bar or not.
 * @property uexe_data Update.EXE data.
 * @property uexe_allocated_data Allocated Update.EXE data.
 * @property uexe_size Update.EXE size.
 * @property system_data System data, for COMMAND_FLASH.
 * @property system_size System size, for COMMAND_FLUSH.
 * @property output_fp Output file pointer, for COMMAND_BACKUP.
 */
struct args {
    int command;
    int upload_uexe;
    int erase_flash;
    int display_progress;

    cahute_u8 const *uexe_data;
    cahute_u8 *uexe_allocated_data;
    size_t uexe_size;

    cahute_u8 *system_data;
    size_t system_size;

    FILE *output_fp;
};

extern int parse_args(int ac, char **av, struct args *args);
extern void free_args(struct args *args);

#endif /* P7OS_H */
