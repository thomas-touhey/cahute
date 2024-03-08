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

#include "options.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
) {
    state->flags = flags_or_style;
    state->short_options = short_options;
    state->long_options = long_options;

    /* The first argument is always the name of the command, we only want
     * the arguments that we can parse here. */
    state->argc = argc - 1;
    state->argv = argv + 1;
    state->posv = argv + 1;
    state->posc = 0;
    state->current = NULL;
}

/**
 * Get the next option, using an option parsing state.
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
) {
    int opt = GETOPT_FAIL, optopt = 0, c, ret = 1;
    char *attr = NULL, *param = NULL;

    while (state->current || state->argc) {
        if (state->current && (c = *state->current++)) {
            /* We are currently processing a short options component, and as
             * such, we need to continue yielding from it. */
            struct short_option const *short_opt;

            optopt = c;

            for (short_opt = state->short_options; short_opt->character;
                 short_opt++) {
                if (short_opt->character != c)
                    continue;

                /* The option corresponds! We start checking for the attribute
                 * before anything else, because it's going to condition
                 * parsing of other options. */
                if (short_opt->flags
                    & (OPTION_FLAG_ATTRIBUTE_REQUIRED
                       | OPTION_FLAG_ATTRIBUTE_OPTIONAL)) {
                    attr = state->current;
                    state->current = NULL;
                }

                /* The option corresponds! We now need to check for a
                 * parameter. */
                if (~short_opt->flags & OPTION_FLAG_PARAMETER_REQUIRED) {
                    /* No parameter to read with the option. */
                } else if (!state->argc) {
                    /* There isn't a parameter to provide... */
                    attr = NULL;
                    goto end;
                } else {
                    /* There is a parameter to provide, we pop it from the
                     * parameter array and stop it here! */
                    param = *state->argv++;
                    state->argc--;
                }

                if (attr) {
                    /* The ``-o<arg>`` is the default one, but if the form
                     * is actually ``-o:<arg>`` or ``-o=<arg>``, we want to
                     * skip the next character. */
                    if (*attr == '='
                        || ((state->flags & GETOPT_FLAG_SEMICOLON)
                            && *attr == ':'))
                        attr++;

                    if (!*attr)
                        attr = NULL;

                    if ((short_opt->flags & OPTION_FLAG_ATTRIBUTE_REQUIRED)
                        && !attr) {
                        /* Oh no, there is no attribute while one was required! */
                        param = NULL;
                        goto end;
                    }
                }

                /* Everything looks good for the option! */
                opt = c;
                optopt = 0;
                goto end;
            }

            /* We have not found the option! We just ignore the option
             * and continue on looking. */
            continue;
        }

        /* We have finished reading the short options array, or this is
         * basically a no-op. */
        state->current = NULL;

        while (state->argc) {
            char *arg = *state->argv++;

            state->argc--;

            if (arg[0] != '-') {
                /* That's a positional parameter. */
                *state->posv++ = arg;
                state->posc++;
                continue;
            }

            if (arg[1] == '-') {
                struct long_option const *long_opt;
                char *result;

                arg += 2;

                /* That's a long option, that can contain a value directly
                 * in the argument using '=' or, if enabled, ':', or in the
                 * next parameter. Thankfully, we can test for that! */
                result = strchr(arg, '=');

                if (state->flags & GETOPT_FLAG_SEMICOLON) {
                    char *scresult = strchr(arg, ':');

                    if (!result || (scresult && scresult < result))
                        result = scresult;
                }

                if (result) {
                    *result = '\0'; /* Terminate the name for comparisons. */
                    param = result + 1;
                }

                /* We can now look for the long option. */
                for (long_opt = state->long_options; long_opt->name;
                     long_opt++) {
                    if (strcmp(arg, long_opt->name))
                        continue;

                    optopt = long_opt->character;

                    /* We have found the long option! Now we actually want
                     * to check for the attribute and parameter. */
                    if (long_opt->flags & OPTION_FLAG_ATTRIBUTE_REQUIRED) {
                        /* The value we have previously set as param is in
                         * fact the attribute! */
                        if (!param) {
                            /* Missing attribute. */
                            goto end;
                        }

                        attr = param;
                        param = NULL;
                    } else if (long_opt->flags & OPTION_FLAG_ATTRIBUTE_OPTIONAL) {
                        /* The value we have previously set as param is in
                         * fact the attribute, but not mandatory! */
                        attr = param;
                        param = NULL;
                    }

                    /* NOTE: We don't raise a flag for extraneous param for
                     * a long option that requires one. */
                    if (!param
                        && (long_opt->flags & OPTION_FLAG_PARAMETER_REQUIRED
                        )) {
                        if (!state->argc) {
                            /* Missing parameter to take as the param,
                             * unfortunately. */
                            goto end;
                        }

                        param = *state->argv++;
                        state->argc--;
                    }

                    /* Everything is nominal regarding the long option,
                     * we can return it safely! */
                    opt = optopt;
                    optopt = 0;
                    goto end;
                }

                /* We have not found the long option, unfortunately.
                 * In this case, we just ignore the option, and continue
                 * on with our journey. */
                continue;
            }

            /* Since we don't have a long option, that means we have one or
             * more short options. We actually want to use the first
             * condition to process this.
             *
             * Note that the 'break' here is only for the inner loop,
             * not the outer loop! */
            state->current = &arg[1];
            break;
        }
    }

    /* All argument values have been processed! */
    ret = 0;
    opt = 0;
    optopt = 0;

end:
    if (optp)
        *optp = opt;
    if (optoptp)
        *optoptp = optopt;
    if (attrp)
        *attrp = attr;
    if (paramp)
        *paramp = param;

    return ret;
}

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
) {
    if (paramcp)
        *paramcp = state->posc;
    if (paramvp)
        *paramvp = state->posv - state->posc;
}
