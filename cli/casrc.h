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

#ifndef CASRC_H
#define CASRC_H 1
#include <stdio.h>

/**
 * Node representing a setting property.
 *
 * @param next Next property node for the setting, or NULL if this is the
 *        last property for the node.
 * @param name Name of the property, lowercased for case-insensitive matching
 *        purposes.
 * @param value Value of the property, lowercased for easier processing.
 */
struct casrc_property {
    struct casrc_property *next;
    char *name;
    char *value;
};

/**
 * Node representing a setting in the database.
 *
 * @param next Next setting in the database, or NULL if this is the last
 *        setting for the database.
 * @param name Name of the property, lowercased for case-insensitive
 *        matching purposes.
 * @param properties Properties in the setting.
 */
struct casrc_setting {
    struct casrc_setting *next;
    char *name;
    struct casrc_property *properties;
};

/**
 * Node representing a property diff for a macro.
 *
 * @param next Next property diff for the macro, or NULL if this is the
 *        last one.
 * @param name Name of the property to set or unset.
 * @param value If the property should be set, the value to define the
 *        property with.
 * @param unset Whether to set (0) the property, or unset (non-0).
 */
struct casrc_property_diff {
    struct casrc_property_diff *next;
    char *name;
    char *value;
    int unset;
};

/**
 * Node representing a macro in the database.
 *
 * @param next Next macro in the database, or NULL if this is the last one.
 * @param name Name of the macro for evaluation, lowercased for
 *        case-insensitive matching purposes.
 * @param diffs Property diffs in the macro.
 */
struct casrc_macro {
    struct casrc_macro *next;
    char *name;
    struct casrc_property_diff *diffs;
};

/**
 * casrc database.
 *
 * @property settings Settings, as alphabetically-sorted named sets of
 *           properties.
 * @property macros Macros, as alphabetically-sorted named sets of
 *           property diffs.
 */
struct casrc_database {
    struct casrc_setting *settings;
    struct casrc_macro *macros;
};

/**
 * Allocate a new casrc database.
 *
 * @param dbp Pointer to set to the allocated and initialized casrc database.
 * @return 0 if there were no errors, other otherwise.
 */
extern int create_casrc_database(struct casrc_database **dbp);

/**
 * Destroy (free) the allocated casrc database.
 *
 * @param db Database to destroy.
 */
extern void destroy_casrc_database(struct casrc_database *db);

/**
 * Read a casrc file into the provided database.
 *
 * This is used to read the defaults for CaS.
 *
 * @param db casrc database to feed with the data from the casrc file.
 * @param filep FILE pointer to the opened casrc file for reading.
 * @return 0 if there were no errors, other otherwise.
 */
extern int read_casrc_file(struct casrc_database *db, FILE *filep);

/**
 * Read the default casrc file into the provided database.
 *
 * @param db casrc database to feed with the data from the default (system
 *        or user casrc file).
 * @return 0 if there were no errors, other otherwise.
 */
extern int load_default_casrc(struct casrc_database *db);

/**
 * Define a macro using a given casrc line.
 *
 * @param db casrc database to modify.
 * @param name Key of the macro to define.
 * @param line Line to define.
 * @return 0 if there were no errors, other otherwise.
 */
extern int
define_casrc_macro(struct casrc_database *db, char const *name, char *line);

/**
 * Define all properties for a given casrc setting.
 *
 * This is used by CaS' command-line parsing, since options '-i', '-o', '-l'
 * and '-m' at least are of the same format as a composant in the casrc file.
 *
 * Note that this function clears all existing properties for the given
 * setting before defining the new properties.
 *
 * @param db casrc database to modify.
 * @param name Key of the setting to define.
 * @param line Line to define.
 * @param reset Whether to reset all properties of the setting before applying
 *        the diffs, or not.
 * @return 0 if there were no errors, other otherwise.
 */
extern int define_casrc_setting(
    struct casrc_database *db,
    char const *name,
    char *line,
    int reset
);

/**
 * Get a setting from the database.
 *
 * @param db casrc database to read from.
 * @param name Name of the setting to define.
 * @return casrc setting, or NULL if the setting was not found.
 */
extern struct casrc_setting *
get_casrc_setting(struct casrc_database *db, char const *name);

/**
 * Get a property from a database.
 *
 * @param db Default setting to read from, or NULL if none.
 * @param setting_name Name of the setting to extract.
 * @param property_name Name of the property to extract.
 * @return Value of the setting, or NULL if not found.
 */
extern char const *get_casrc_property(
    struct casrc_database *db,
    char const *setting_name,
    char const *property_name
);

/**
 * Get a property from a set of settings.
 *
 * @param default_setting Default setting to read from, or NULL if none.
 * @param override_setting Override setting to read from, or NULL if none.
 * @param name Name of the property to extract.
 * @return Value of the setting, or NULL if not found.
 */
extern char const *get_casrc_setting_property(
    struct casrc_setting *default_setting,
    struct casrc_setting *override_setting,
    char const *name
);

#endif /* CASRC_H */
