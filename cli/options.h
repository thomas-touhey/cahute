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

#ifndef OPTIONS_H
#define OPTIONS_H   1
#define GETOPT_FAIL 21 /* Character in case of failure (NAK). */

/* Cross-platform getopt() implementation with additional compatibility options
 * for implementing different command-line styles.
 *
 * This implementation supports the following option parsing flags:
 *
 * ``GETOPT_FLAG_SEMICOLON``
 *     Enable ":" as an alternative attribute and parameter option separator
 *     to "=". */

#define GETOPT_FLAG_SEMICOLON 1

/* Flags are regrouped within styles you should use:
 *
 * ``GETOPT_STYLE_POSIX``
 *     Traditional POSIX option parsing style. Options can only start with
 *     "-", short options combine, long options use "=" or read from the next
 *     parameter.
 *
 * ``GETOPT_STYLE_CAS``
 *     CaS option parsing style. Options can only start with "-", short
 *     and long options can have both attributes and values, ":" is enabled
 *     as an alternative to "=". */

#define GETOPT_STYLE_POSIX 0
#define GETOPT_STYLE_CAS   GETOPT_FLAG_SEMICOLON

/* Option flags.
 *
 * ``OPTION_FLAG_PARAMETER_REQUIRED``
 *     The option requires a parameter.
 *
 * ``OPTION_FLAG_ATTRIBUTE_REQUIRED``
 *     The option requires an attribute.
 *     This flag is a no-op unless attributes are enabled using
 *     ``GETOPT_FLAG_ATTRIBUTE``.
 *
 * ``OPTION_FLAG_ATTRIBUTE_OPTIONAL``
 *     The option accepts an attribute.
 *     This flag is a no-op unless attributes are enabled using
 *     ``GETOPT_FLAG_ATTRIBUTE``.
 */

#define OPTION_FLAG_PARAMETER_REQUIRED 2
#define OPTION_FLAG_ATTRIBUTE_REQUIRED 8
#define OPTION_FLAG_ATTRIBUTE_OPTIONAL 4

/**
 * Short option definition.
 *
 * @property character ASCII character code.
 * @property flags Flags for the short option.
 */
struct short_option {
    int character;
    unsigned long flags;
};

#define SHORT_OPTION_SENTINEL \
    { 0, 0 }

/**
 * Long option definition.
 *
 * @property name Long option name.
 * @property flags Flags for the long option.
 * @property character ASCII character code to return in such case.
 */
struct long_option {
    char const *name;
    unsigned long flags;
    int character;
};

#define LONG_OPTION_SENTINEL \
    { NULL, 0, 0 }

/**
 * Option parsing state.
 *
 * The positional parameters will be set starting at the provided argv + 1,
 * e.g.::
 *
 *     "./command" "-ab" "pos1" "--some-other-option" "pos2"
 *     (becomes)
 *     "./command" "pos1" "pos2" ...
 *
 * @param short_options Defined short options at initialization.
 * @param long_options Defined long options at initialization.
 * @param flags Flags defining the behaviour.
 * @param argc Current argument count.
 * @param argv Current argument values.
 * @param posc Current count of positional arguments.
 * @param posv Positional argument values.
 * @param current Current short options we are working on.
 */
struct option_parser_state {
    struct short_option const *short_options;
    struct long_option const *long_options;
    unsigned long flags;

    int argc;
    char **argv;

    int posc;
    char **posv;
    char *current;
};

/**
 * Initialize the option parsing state.
 *
 * @param state State to initialize.
 * @param flags_or_style Flags.
 * @param short_options Short options to use.
 * @param long_options Long options to use.
 * @param argc Argument count.
 * @param argv Argument values.
 */
extern void init_option_parser(
    struct option_parser_state *state,
    unsigned long flags_or_style,
    struct short_option const *short_options,
    struct long_option const *long_options,
    int argc,
    char **argv
);

/**
 * Get the next option, using an option parsing state.
 *
 * WARNING: With this function, only invalid usage of existing options
 * (missing attribute or parameter) is reported, invalid options only
 * get ignored!
 *
 * @param state State to use for parsing the next option.
 * @param charp Pointer to the option character to set.
 * @param attrp Pointer to the attribute to set.
 * @param paramp Pointer to the parameter to set.
 * @return Whether an option has been found (1) or not (0).
 */
extern int parse_next_option(
    struct option_parser_state *state,
    int *optp,
    int *optoptp,
    char **attrp,
    char **paramp
);

/**
 * Get the positional parameters.
 *
 * @param state State to use to get the positional parameters.
 * @param paramcp Parameter count pointer.
 * @param paramvp Parameter values pointer.
 */
extern void update_positional_parameters(
    struct option_parser_state *state,
    int *paramcp,
    char ***paramvp
);

#endif /* OPTIONS_H */
