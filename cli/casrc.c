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

#include "casrc.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "common.h"

#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64) \
    || defined(__WINDOWS__)
# define POSIX_ENABLED 0
#elif defined(__unix__) && __unix__ \
    || (defined(__APPLE__) || defined(__MACH__))
# define POSIX_ENABLED 1
#else
# define POSIX_ENABLED 0
#endif

/**
 * Allocate a new casrc database.
 *
 * @param dbp Pointer to set to the allocated and initialized casrc database.
 * @return 0 if there were no errors, other otherwise.
 */
int create_casrc_database(struct casrc_database **dbp) {
    struct casrc_database *db;

    if (!(db = malloc(sizeof(struct casrc_database)))) {
        fprintf(stderr, "Could not allocate the casrc database.\n");
        return 1;
    }

    db->settings = NULL;
    db->macros = NULL;
    *dbp = db;
    return 0;
}

/**
 * Destroy (free) the allocated casrc database.
 *
 * @param db Database to destroy.
 */
void destroy_casrc_database(struct casrc_database *db) {
    struct casrc_setting *setting;
    struct casrc_setting *macro;

    setting = db->settings;
    while (setting) {
        struct casrc_setting *setting_to_free = setting;
        struct casrc_property *property;

        setting = setting->next;
        property = setting_to_free->properties;
        while (property) {
            struct casrc_property *property_to_free = property;

            property = property->next;
            free(property_to_free);
        }

        free(setting_to_free);
    }

    macro = db->macros;
    while (macro) {
        struct casrc_setting *macro_to_free = macro;
        struct casrc_property *diff;

        macro = macro->next;
        diff = macro_to_free->properties;
        while (diff) {
            struct casrc_property *diff_to_free = diff;

            diff = diff->next;
            free(diff_to_free);
        }

        free(macro_to_free);
    }

    free(db);
}

/**
 * Get a list of properties from a macro name.
 *
 * @param db casrc database to access.
 * @param key Key of the macro to get.
 * @param diffp Pointer to the pointer to the first diff to set.
 * @return 0 if there were no errors, other otherwise.
 */
static int get_casrc_macro_properties(
    struct casrc_database *db,
    char const *key,
    struct casrc_property **diffp
) {
    struct casrc_setting *node = db->macros;
    int cmp_result = 1;

    for (; node; node = node->next) {
        char const *s = key, *t = node->name;

        /* We must make a case insensitive match without modifying the
         * key, since it may be interpreted as "key=value" later where
         * the value has the right to be any case and could be interpreted
         * differently. */
        do {
            cmp_result = *t++ - tolower(*s++);
        } while (s[-1] && !cmp_result);

        if (!cmp_result) {
            *diffp = node->properties;
            return 0;
        }

        if (cmp_result > 0)
            break;
    }

    *diffp = NULL;
    return 1;
}

/**
 * Get the next component in a comma-separated string, and update the
 * string pointer.
 *
 * @param linep Line pointer.
 * @param comp Component pointer.
 * @return 0 if there were no errors, other otherwise.
 */
static inline int get_next_comma_component(char **linep, char **comp) {
    char *orig = *linep, *line;

    for (line = orig; *line && *line != ','; line++)
        ;
    if (*line == ',')
        *line++ = '\0';

    if (line == orig) {
        /* We are at the end of the string. */
        *comp = NULL;
        return 1;
    }

    *linep = line;
    *comp = orig;
    return 0;
}

/**
 * Get the key/value pair from a component.
 *
 * @param raw Raw pointer.
 * @param keyp Pointer to the key.
 * @param valuep Pointer to the value.
 * @param setp Pointer to the set boolean.
 * @return 0 if there were no errors, other otherwise.
 */
static inline void get_key_value_pair(
    char *raw,
    char const **keyp,
    char const **valuep,
    int *setp
) {
    char *equ, *p;

    /* Remove whitespaces at the beginning of the raw string. */
    for (; *raw == ' ' || *raw == '\t'; raw++)
        ;

    /* Check if the "no-" prefix is there. */
    if (tolower(raw[0]) == 'n' && tolower(raw[1]) == 'o'
        && tolower(raw[2]) == '-') {
        *setp = 0;
        raw += 3;
    } else
        *setp = 1;

    /* Remove whitespaces at the end of the raw string. */
    p = &raw[strlen(raw)] - 1;
    for (; *p == ' ' || *p == '\t'; p--)
        ;
    *++p = '\0';

    /* Look for the equal sign. */
    equ = strchr(raw, '=');
    if (!equ) {
        /* Apply case insensitivity to the key. */
        for (p = raw; *p; p++)
            *p = tolower(*p);

        *keyp = raw;
        *valuep = "";
        return;
    }

    /* Remove whitespace before the equal sign. */
    for (p = equ - 1; p > raw && (*p == ' ' || *p == '\t'); p--)
        ;
    *++p = '\0';

    /* Remove whitespace after the equal sign. */
    for (p = equ + 1; *p && (*p == ' ' || *p == '\t'); p++)
        ;

    *keyp = raw;
    *valuep = p;

    /* Apply case insensitivty to the key. */
    for (p = raw; *p; p++)
        *p = tolower(*p);
}

/**
 * Define a setting or macro using a given casrc line.
 *
 * @param db Database to use to obtain macros.
 * @param macros Macro linked list to which to add the macro.
 * @param name Name of the macro to define.
 * @param line Line to define; note that it will be modified.
 * @param reset Whether to reset the setting.
 * @return 0 if there were no errors, other otherwise.
 */
int define_casrc_setting(
    struct casrc_database *db,
    struct casrc_setting **macrosp,
    char const *name,
    char *line,
    int reset
) {
    struct casrc_setting **macrop, *macro;
    struct casrc_property **diffp, *diff;
    char *com, *buf;
    int comp;

    /* Find the entry corresponding to the macro. */
    for (macrop = macrosp;
         (*macrop) && (comp = strcmp(name, (*macrop)->name)) > 0;
         macrop = &(*macrop)->next)
        ;
    if (!*macrop || comp < 0) {
        /* We want to create a new macro to insert right there. */
        macro = malloc(sizeof(struct casrc_setting) + strlen(name) + 1);
        if (!macro) {
            fprintf(stderr, "malloc() for macro failed.\n");
            return 1;
        }

        buf = &((char *)macro)[sizeof(struct casrc_setting)];
        strcpy(buf, name);

        macro->next = *macrop;
        macro->name = buf;
        macro->properties = NULL;

        *macrop = macro;
        diffp = &macro->properties;
    } else {
        /* We have found an exact match, we want to replace the macro. */
        macro = *macrop;
        diffp = &macro->properties;

        if (reset) {
            for (diff = macro->properties; diff;) {
                struct casrc_property *diff_to_free = diff;

                diff = diff->next;
                free(diff_to_free);
            }

            macro->properties = NULL;
        } else {
            /* Place the cursor at the end of the existing properties. */
            while (*diffp)
                diffp = &(*diffp)->next;
        }
    }

    /* Now that the pointer to the diffs to define is ready, we can
     * decode the property diffs and assign them to it. */
    while (!get_next_comma_component(&line, &com)) {
        struct casrc_property *other_diff;
        char const *key, *value;
        size_t key_len, value_len;
        int set;

        if (strcmp(name, com)
            && !get_casrc_macro_properties(db, com, &other_diff)) {
            /* We're expanding another macro! */
            for (; other_diff; other_diff = other_diff->next) {
                key_len = strlen(other_diff->name);
                value_len = strlen(other_diff->value);
                diff = malloc(
                    sizeof(struct casrc_property) + key_len + value_len + 2
                );
                if (!diff) {
                    fprintf(stderr, "malloc() for property diff failed.\n");
                    return 1;
                }

                buf = &((char *)diff)[sizeof(struct casrc_property)];
                strcpy(buf, other_diff->name);
                strcpy(&buf[key_len + 1], other_diff->value);

                diff->next = NULL;
                diff->name = buf;
                diff->value = &buf[key_len + 1];
                diff->unset = other_diff->unset;

                *diffp = diff;
                diffp = &diff->next;
            }

            continue;
        }

        /* We have a key/value pair, which we can parse. */
        get_key_value_pair(com, &key, &value, &set);
        if (!*key) {
            /* Empty key, we want to just ignore the entry. */
            continue;
        }

        /* We can create our new node here! */
        key_len = strlen(key);
        value_len = strlen(value);
        diff = malloc(sizeof(struct casrc_property) + key_len + value_len + 2);
        if (!diff) {
            fprintf(stderr, "malloc() for property diff failed.\n");
            return 1;
        }

        buf = &((char *)diff)[sizeof(struct casrc_property)];
        strcpy(buf, key);
        strcpy(&buf[key_len + 1], value);

        diff->next = NULL;
        diff->name = buf;
        diff->value = &buf[key_len + 1];
        diff->unset = !set;

        *diffp = diff;
        diffp = &diff->next;
    }


    return 0;
}

/**
 * Read a casrc file into the provided database.
 *
 * This is used to read the defaults for CaS.
 *
 * @param db casrc database to feed with the data from the casrc file.
 * @param filep FILE pointer to the opened casrc file for reading.
 * @return 0 if there were no errors, other otherwise.
 */
int read_casrc_file(struct casrc_database *db, FILE *filep) {
    char *line = NULL;
    size_t line_capacity = 0;
    int ret = 0;

    while (1) {
        char *p, *q;
        char *name;
        cahute_ssize read;
        int is_macro = 0;

        errno = 0;
        read = portable_getdelim(&line, &line_capacity, '\n', filep);
        if (read < 0) {
            /* The read failed. */
            if (errno) {
                fprintf(
                    stderr,
                    "Error while reading file: %s (%d)\n",
                    strerror(errno),
                    errno
                );
                ret = 1;
            }
            break;
        }

        /* Skip initial whitespace. */
        for (p = line; *p && *p != '\n' && isspace(*p); p++)
            ;

        if (!*p || *p == '#' || *p == ';' || *p == '\n') {
            /* Comment or empty line, we can ignore it. */
            continue;
        }

        /* Special macro prefix for identifying macros instead of being
         * used as a property name. */
        if (tolower(p[0]) == 'm' && tolower(p[1]) == 'a'
            && tolower(p[2]) == 'c' && tolower(p[3]) == 'r'
            && tolower(p[4]) == 'o' && isspace(p[5])) {
            is_macro = 1;
            p += 6;

            /* Skip whitespace after the macro. */
            for (; *p && *p != '\n' && isspace(*p); p++)
                ;
        }

        /* Read the name. */
        name = p;
        for (; *p && !isspace(*p) && *p != '=' && *p != ':'; p++)
            *p = tolower(*p);

        /* We need to place a '\0' at the end of the name, while being able
         * to skip all spaces up to the "=" and ":" to the value. */
        q = p;
        for (; *p && isspace(*p); p++)
            ;
        if (*p == '=' || *p == ':')
            p++;

        if (p == q) {
            /* Empty name, we want to ignore this. */
            continue;
        }

        *q = '\0';

        /* Now we have the value surrounding by spaces and tabulations,
         * which can be processed by ``define_casrc_setting()``. */
        q = p;
        for (; *p && *p != '\n'; p++)
            ;
        *p = '\0';

        if (is_macro)
            ret = define_casrc_setting(db, &db->macros, name, q, 1);
        else
            ret = define_casrc_setting(db, &db->settings, name, q, 0);

        if (ret)
            break;
    }

    free(line);
    return ret;
}

#if POSIX_ENABLED
/**
 * Obtain the maximum path size on the current OS.
 *
 * @return Maximum path size.
 */
static inline size_t unix_path_max(void) {
    cahute_ssize path_max;

# ifdef PATH_MAX
    path_max = PATH_MAX;
# else
    path_max = pathconf("/", _PC_PATH_MAX);
    if (path_max <= 0)
        path_max = 4096;
# endif

    return (size_t)++path_max;
}
#endif

/**
 * Read the default casrc file into the provided database.
 *
 * @param db casrc database to feed with the data from the default (system
 *        or user casrc file).
 * @return 0 if there were no errors, other otherwise.
 */
int load_default_casrc(struct casrc_database *db) {
    FILE *filep = NULL;
    int ret;

#if POSIX_ENABLED
    /* Try to read the casrc from the user's home directory. */
    {
        char const *home = getenv("HOME");

        if (home) {
            char *pathbuf = malloc(unix_path_max() + 2);
            int len;

            if (!pathbuf) {
                fprintf(stderr, "malloc() failed for path\n");
                return 1;
            }

            len = sprintf(pathbuf, "%s", home);
            if (pathbuf[len - 1] != '/')
                pathbuf[len++] = '/';
            sprintf(&pathbuf[len], ".casrc");

            filep = fopen(pathbuf, "rb");

            if (filep) {
                free(pathbuf);
                goto read;
            }

            free(pathbuf);
        }
    }

    /* Read the system casrc next. */
    filep = fopen("/etc/system.casrc", "rb");
    if (filep)
        goto read;
#endif

    /* If we have not managed to load a casrc file, that's okay. */
    return 0;

read:
    ret = read_casrc_file(db, filep);
    fclose(filep);
    return ret;
}

/**
 * Get a setting from the database.
 *
 * @param db casrc database to read from.
 * @param name Name of the setting to define.
 * @return casrc setting, or NULL if the setting was not found.
 */
struct casrc_setting *
get_casrc_setting(struct casrc_database *db, char const *name) {
    struct casrc_setting *setting;
    int comp = 1;

    for (setting = db->settings;
         setting && (comp = strcmp(name, setting->name)) > 0;
         setting = setting->next)
        ;
    if (comp)
        return NULL;

    return setting;
}

/**
 * Get a property from a database.
 *
 * @param db Default setting to read from, or NULL if none.
 * @param setting_name Name of the setting to extract.
 * @param property_name Name of the property to extract.
 * @return Value of the setting, or NULL if not found.
 */
char const *get_casrc_property(
    struct casrc_database *db,
    char const *setting_name,
    char const *property_name
) {
    struct casrc_setting *setting;
    struct casrc_property *property;
    char const *result = NULL;
    int comp = 1;

    for (setting = db->settings;
         setting && (comp = strcmp(setting_name, setting->name)) > 0;
         setting = setting->next)
        ;
    if (comp)
        return NULL;

    for (property = setting->properties; property; property = property->next) {
        if (strcmp(property->name, property_name))
            continue;

        if (property->unset)
            result = NULL;
        else
            result = property->value;
    }

    return result;
}

/**
 * Get a property from a set of settings.
 *
 * @param default_setting Default setting to read from, or NULL if none.
 * @param override_setting Override setting to read from, or NULL if none.
 * @param name Name of the property to extract.
 * @return Value of the setting, or NULL if not found.
 */
char const *get_casrc_setting_property(
    struct casrc_setting *default_setting,
    struct casrc_setting *override_setting,
    char const *name
) {
    struct casrc_property *property;
    char const *result = NULL;

    if (default_setting) {
        for (property = default_setting->properties; property;
             property = property->next) {
            if (strcmp(property->name, name))
                continue;

            if (property->unset)
                result = NULL;
            else
                result = property->value;
        }
    }

    if (override_setting) {
        for (property = override_setting->properties; property;
             property = property->next) {
            if (strcmp(property->name, name))
                continue;

            if (property->unset)
                result = NULL;
            else
                result = property->value;
        }
    }

    return result;
}
