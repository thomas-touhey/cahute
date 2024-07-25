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

#ifndef INTERNAL_CHARS_H
#define INTERNAL_CHARS_H 1
#include "internals.h"

/**
 * FONTCHARACTER entry.
 *
 * @property code_legacy Code in the legacy table (0 if undefined).
 * @property code_9860 Code in the fx-9860G table (0 if undefined).
 * @property unicode Unicode sequence corresponding to the character.
 * @property unicode_len Length of the Unicode sequence.
 * @property cat CAT sequence for the character.
 * @property cat_len CAT sequence length.
 * @property opcode Characters, if the character is a multi-sequence.
 * @property opcode_len Multi-character sequence length.
 */
struct cahute_char_entry {
    unsigned int code_legacy;
    unsigned int code_9860;

    cahute_u32 const *unicode;
    char const *cat;
    cahute_u16 const *opcode;

    size_t unicode_len;
    size_t cat_len;
    size_t opcode_len;
};

/**
 * Byte parsing tree.
 *
 * This tree contains the nodes to match from this point on, and the
 * character entry to use if no nodes were matched.
 *
 * @property matches Nodes to match to update the tree.
 * @property entry Character entry to fall back to.
 */
struct cahute_byte_parsing_tree {
    struct cahute_byte_match const *matches;
    struct cahute_char_entry const *entry;
};

/**
 * Byte matching node for a parsing tree.
 *
 * This node contains the sequence to match to go to the matching tree.
 *
 * @property next Next node if no match was found.
 * @property subtree Tree to use if the sequence matches.
 * @property sequence Sequence to match.
 * @property sequence_len Size of the sequence to match.
 */
struct cahute_byte_match {
    struct cahute_byte_match const *next;
    struct cahute_byte_parsing_tree const *subtree;
    cahute_u8 const *sequence;
    size_t sequence_len;
};

/**
 * 32-bit integer parsing tree.
 *
 * Equivalent of ``cahute_byte_match`` for 32-bit integer sequences.
 *
 * @property matches Nodes to match to update the tree.
 * @property entry Character entry to fall back to.
 */
struct cahute_u32_parsing_tree {
    struct cahute_u32_match const *matches;
    struct cahute_char_entry const *entry;
};

/**
 * 32-bit matching node for a parsing tree.
 *
 * Equivalent of ``cahute_byte_match`` for 32-bit integer sequences.
 *
 * @property next Next node if no match was found.
 * @property subtree Tree to use if the sequence matches.
 * @property sequence Sequence to match.
 * @property sequence_len Size of the sequence to match.
 */
struct cahute_u32_match {
    struct cahute_u32_match const *next;
    struct cahute_u32_parsing_tree const *subtree;
    cahute_u32 const *sequence;
    size_t sequence_len;
};

/* ---
 * Definitions made in chars.c
 * --- */

extern struct cahute_char_entry const *cahute_chars_legacy_00[];
extern struct cahute_char_entry const *cahute_chars_legacy_7F[];
extern struct cahute_char_entry const *cahute_chars_legacy_F7[];

extern struct cahute_char_entry const *cahute_chars_9860_00[];
extern struct cahute_char_entry const *cahute_chars_9860_7F[];
extern struct cahute_char_entry const *cahute_chars_9860_E5[];
extern struct cahute_char_entry const *cahute_chars_9860_E6[];
extern struct cahute_char_entry const *cahute_chars_9860_E7[];
extern struct cahute_char_entry const *cahute_chars_9860_F7[];
extern struct cahute_char_entry const *cahute_chars_9860_F9[];

extern struct cahute_u32_parsing_tree const cahute_unicode_legacy_parsing_tree;
extern struct cahute_u32_parsing_tree const cahute_unicode_9860_parsing_tree;

extern struct cahute_byte_parsing_tree const cahute_cat_legacy_parsing_tree;
extern struct cahute_byte_parsing_tree const cahute_cat_9860_parsing_tree;

#endif /* INTERNAL_CHARS_H */
