#!/usr/bin/env python
# *****************************************************************************
# Copyright (C) 2024 Thomas Touhey <thomas@touhey.fr>
#
# This software is governed by the CeCILL 2.1 license under French law and
# abiding by the rules of distribution of free software. You can use, modify
# and/or redistribute the software under the terms of the CeCILL 2.1 license
# as circulated by CEA, CNRS and INRIA at the following
# URL: https://cecill.info
#
# As a counterpart to the access to the source code and rights to copy, modify
# and redistribute granted by the license, users are provided only with a
# limited warranty and the software's author, the holder of the economic
# rights, and the successive licensors have only limited liability.
#
# In this respect, the user's attention is drawn to the risks associated with
# loading, using, modifying and/or developing or reproducing the software by
# the user in light of its specific status of free software, that may mean
# that it is complicated to manipulate, and that also therefore means that it
# is reserved for developers and experienced professionals having in-depth
# computer knowledge. Users are therefore encouraged to load and test the
# software's suitability as regards their requirements in conditions enabling
# the security of their systems and/or data to be ensured and, more generally,
# to use and operate it in the same conditions as regards security.
#
# The fact that you are presently reading this means that you have had
# knowledge of the CeCILL 2.1 license and that you accept its terms.
# *****************************************************************************
"""Process the character references.

This script requires the ``toml`` package to be installed.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass, field
from logging import getLogger
from os import makedirs
from pathlib import Path
from typing import Any, Iterator, Literal

import toml

CharacterTableKey = Literal["legacy", "9860"]
"""Type representing a character table."""

DEFAULT_OUTPUT_PATH = Path(__file__).parent.parent / "lib" / "chars.c"
"""Default output path."""

DEFAULT_REFERENCE_PATH = Path(__file__).parent / "chars.toml"
"""Default path to the character reference."""

MULTI_BYTE_LEADERS: dict[CharacterTableKey, tuple[int, ...]] = {
    "legacy": (0x00, 0x7F, 0xF7),
    "9860": (0x00, 0x7F, 0xE5, 0xE6, 0xE7, 0xF7, 0xF9),
}
"""Multi-byte leaders per encoding."""

logger = getLogger(__name__)
"""Logger."""


@dataclass
class Character:
    """Data regarding a given character."""

    def _get_unicode(self, /) -> list[list[int]]:
        """Get the Unicode sequences."""
        return self.__dict__.get("unicode")

    def _set_unicode(self, value: Any, /) -> None:
        """Validate the Unicode sequences defined in the object.

        :param value: Value to set for the Unicode sequences.
        :return: Sanitized Unicode characters.
        """
        if value is None:
            value = []
        elif isinstance(value, list) and len(value) > 0 and isinstance(value[0], int):
            value = [value]

        self.__dict__["unicode"] = value

    def _get_cat(self, /) -> list[str]:
        """Get the CAT sequences."""
        return self.__dict__.get("cat")

    def _set_cat(self, value: Any, /) -> Any:
        """Validate the CAT sequences defined in the object.

        :param value: Value to set for the CAT sequences.
        :return: Sanitized CAT sequences.
        """
        if value is None:
            value = []
        elif isinstance(value, str):
            value = [value]

        self.__dict__["cat"] = value

    def _get_symbol(self) -> str:
        """Symbol name for the code.

        :return: The symbol name.
        """
        return f"char_{self.table or 'all'}_{self.code:04X}"

    code: int
    """Character code."""

    name: str
    """Character name."""

    table: CharacterTableKey | None = None
    """Specific character table to which the character applies."""

    code_legacy: int | None = None
    """Equivalent code in the legacy table, if the table is 9860."""

    code_9860: int | None = None
    """Equivalent code in the fx-9860G table, if the table is legacy."""

    opcode: list[int] | None = None
    """Characters to resolve the character as for display purposes."""

    unicode: list[list[int]] = field(default_factory=list)
    """Unicode character sequences."""

    cat: list[str] = field(default_factory=list)
    """CAT sequences equivalent to the character."""

    symbol: str = ""
    """Symbol defined for the class."""

    def __getattribute__(self, key):
        if key == "unicode":
            return self._get_unicode()
        elif key == "cat":
            return self._get_cat()
        elif key == "symbol":
            return self._get_symbol()

        return super().__getattribute__(key)

    def __setattr__(self, key, value):
        if key == "unicode":
            return self._set_unicode(value)
        elif key == "cat":
            return self._set_cat(value)
        elif key == "symbol":
            return

        return super().__setattr__(key, value)


@dataclass
class SequenceParsingTree:
    """Parsing tree for a character."""

    subtrees: dict[tuple[int, ...], SequenceParsingTree] = field(default_factory=dict)
    """Subtrees to match."""

    leaf: Character | None = None
    """Leaf to take if none of the subtrees match."""

    def add_character(self, sequence: tuple[int, ...], character: Character, /) -> None:
        """Add the character in the sequence.

        Say we want to insert the character as the sequence [1, 2, 3, 4]:

        * If there is a subtree with that exact sequence, we want to set the
          leaf on it to the character.
        * If there is a subtree being the prefix of the sequence, e.g. [1, 2],
          we want to recursively add the children into the tree as the [3, 4]
          sequence.
        * If any subtree has a common prefix, e.g. [1, 2, 5, 6, 7]
          ([1, 2] prefix), we want to transform the following::

              {[1, 2, 5, 6, 7]: Tree(...)}

          Into the following::

              {[1, 2]: Tree(subtrees={
                  [5, 6, 7]: Tree(...),
                  [3, 4]: Tree(leaf=character),
              })}

        * Otherwise, we want to create the new subtree with the sequence
          name.

        :param sequence: Sequence to add the character as.
        :param character: Character to reference as the sequence in the
            parsing tree.
        :raises ValueError: A character is already defined for the sequence.
        """
        for common_len in range(len(sequence), 0, -1):
            try:
                subtree_key = next(
                    key
                    for key in self.subtrees
                    if key[:common_len] == sequence[:common_len]
                )
            except StopIteration:
                continue

            if subtree_key == sequence:
                leaf = self.subtrees[subtree_key].leaf
                if leaf is not None and leaf.code != character.code:
                    raise ValueError(
                        "sequence already used by " + f"character 0x{leaf.code:04X}",
                    )

                if leaf is None:
                    self.subtrees[subtree_key].leaf = character
            elif subtree_key == sequence[:common_len]:
                # A subtree might exist, we may want to add a character
                # recursively on it.
                self.subtrees[subtree_key].add_character(
                    sequence[common_len:],
                    character,
                )
            else:
                subtree = self.subtrees.pop(subtree_key)

                self.subtrees[subtree_key[:common_len]] = self.__class__(
                    subtrees={
                        sequence[common_len:]: self.__class__(leaf=character),
                        subtree_key[common_len:]: subtree,
                    },
                )

            return

        self.subtrees[sequence] = SequenceParsingTree(leaf=character)

    def print(self, *, indent: str = "") -> None:
        """Print the tree.

        :param indent: Indentation.
        """
        if self.leaf is not None:
            print(f"{indent}<leaf: character {self.leaf.id}>")

        for key, subtree in self.subtrees.items():
            print(f"{indent}<subtree: {key!r}>")
            subtree.print(indent=indent + "  ")


@dataclass
class RawCharacterReference:
    """Raw character reference."""

    chars: list[Character] = field(default_factory=list)
    """Character reference."""


@dataclass
class CharacterTable:
    """Character table definition."""

    characters: dict[int, Character] = field(default_factory=dict)
    """List of characters in the reference."""

    cat_parsing_tree: SequenceParsingTree = field(
        default_factory=SequenceParsingTree,
    )
    """CAT sequence parsing tree."""

    unicode_parsing_tree: SequenceParsingTree = field(
        default_factory=SequenceParsingTree,
    )
    """Unicode sequence parsing tree."""


@dataclass
class CharacterReference:
    """Character reference."""

    tables: dict[CharacterTableKey, CharacterTable]
    """Character tables."""

    @classmethod
    def from_toml_file(
        cls: type[CharacterReference],
        path: str | Path,
        /,
    ) -> CharacterReference:
        """Produce a character reference from a TOML file.

        :param path: Path to the TOML file.
        :return: Decoded character reference.
        """
        is_invalid = False
        tables = {
            "legacy": CharacterTable(),
            "9860": CharacterTable(),
        }

        raw_data = toml.load(path)

        raw_chars = []
        for raw_char_data in raw_data['chars']:
            raw_chars.append(Character(**raw_char_data))

        raw_ref = RawCharacterReference(chars=raw_chars)
        for char in raw_ref.chars:
            for table_key in ("legacy", "9860"):
                if char.table is not None and char.table != table_key:
                    continue

                table = tables[table_key]
                if char.code in table.characters:
                    is_invalid = True
                    logger.warning(
                        "Duplicate character 0x%04X in character table %s.",
                        char.code,
                        table_key,
                    )
                    continue

                leaders = MULTI_BYTE_LEADERS[table_key]
                leader = (char.code >> 8) & 255
                if leader not in leaders:
                    is_invalid = True
                    logger.warning(
                        "Unsupported leader 0x%02X for character 0x%04X in "
                        + "character table %s.",
                        leader,
                        char.code,
                        table_key,
                    )
                    continue

                table.characters[char.code] = char

                for sequence in char.cat:
                    try:
                        table.cat_parsing_tree.add_character(
                            tuple(sequence.encode("ascii")),
                            char,
                        )
                    except ValueError as exc:
                        logger.warning(
                            'Could not add CAT sequence "%s" for char '
                            "0x%04X in table %s: %s.",
                            sequence,
                            char.code,
                            table_key,
                            str(exc),
                        )
                        is_invalid = True

                for sequence in char.unicode:
                    try:
                        table.unicode_parsing_tree.add_character(
                            tuple(sequence),
                            char,
                        )
                    except ValueError as exc:
                        logger.warning(
                            'Could not add Unicode sequence "%s" for char '
                            "0x%04X in table %s: %s.",
                            "[" + ", ".join(f"0x{n:02X}" for n in sequence) + "]",
                            char.code,
                            table_key,
                            str(exc),
                        )
                        is_invalid = True

        if is_invalid:
            logger.error(
                "One or more errors have occurred while parsing the " "reference.",
            )
            raise ValueError()

        return cls(tables=tables)


def get_sequence_parsing_tree_lines(
    tree: SequenceParsingTree,
    /,
    *,
    kind: Literal["byte", "u32"] = "byte",
    symbol: str,
) -> Iterator[str]:
    """Get chars.c lines to define a byte parsing tree.

    :param tree: Parsing tree to represent.
    :param tree_type: C type of the parsing tree.
    :param match_type: C type for the match node.
    :param symbol: Name of the symbol to define the tree as.
    :return: Iterator for the lines required to define the tree.
    """
    if kind == "u32":
        tree_type = "cahute_u32_parsing_tree"
        match_type = "cahute_u32_match"
        seq_cast = "(cahute_u32 const [])"
    else:
        tree_type = "cahute_byte_parsing_tree"
        match_type = "cahute_byte_match"
        seq_cast = "(cahute_u8 const [])"

    def explore_tree(
        symbol: str,
        tree: SequenceParsingTree,
        /,
        *,
        is_local: bool = True,
    ) -> Iterator[str]:
        """Explore the trees.

        :param tree: Tree to yield lines for.
        :param suffix: Suffix to apply to the symbol name.
        :return: Line iterator.
        """
        for i, (sequence, subtree) in enumerate(tree.subtrees.items()):
            yield from explore_tree(symbol + f"_{i}", subtree)
            yield ""
            yield f"CAHUTE_LOCAL_DATA(struct {match_type} const) {symbol}_m{i} = " + "{"

            if i == 0:  # Next node.
                yield "    NULL,"
            else:
                yield f"    &{symbol}_m{i - 1},"

            yield f"    &{symbol}_{i},"  # Subtree.
            yield f"    {seq_cast}" + "{" + ", ".join(
                map(str, sequence)
            ) + "},"  # Sequence.
            yield f"    {len(sequence)}"  # Sequence length.

            yield "};"
            yield ""

        if is_local:
            yield f"CAHUTE_LOCAL_DATA(struct {tree_type} const) {symbol} = " + "{"
        else:
            yield f"struct {tree_type} const {symbol} = " + "{"

        if tree.subtrees:
            yield f"    &{symbol}_m{len(tree.subtrees) - 1},"
        else:
            yield "    NULL,"

        if tree.leaf:
            yield f"    &{tree.leaf.symbol}"
        else:
            yield "    NULL"

        yield "};"
        yield ""

    yield from explore_tree(symbol, tree, is_local=False)


def get_chars_c_lines(*, ref: CharacterReference) -> Iterator[str]:
    """Get the chars.c lines.

    :param ref: Reference to produce the chars.c from.
    :param fp: Stream to which to output the file.
    """
    yield '#include <chars.h>'
    yield ""

    # ---
    # Define every character, so that they can be referenced by later
    # functions.
    # ---

    chars_per_symbol = {}
    for table in ref.tables.values():
        for char in table.characters.values():
            chars_per_symbol[char.symbol] = char

    for symbol, char in sorted(chars_per_symbol.items()):
        # See ``cahute_char_entry`` in ``lib/chars.h`` for more information.

        yield f"CAHUTE_LOCAL_DATA(struct cahute_char_entry const) {symbol} = " + "{"

        # Legacy character code.
        if char.table is None or char.table == "legacy":
            yield f"    {char.code},"
        elif char.code_legacy is not None:
            yield f"    {char.code_legacy},"
        else:
            yield "    0,"

        # fx-9860G character code.
        if char.table is None or char.table == "9860":
            yield f"    {char.code},"
        elif char.code_9860 is not None:
            yield f"    {char.code_9860},"
        else:
            yield "    0,"

        if char.unicode and char.unicode[0]:
            yield "    (cahute_u32 const []){" + ", ".join(
                map(str, char.unicode[0])
            ) + "},"
        else:
            yield "    NULL,"

        if char.cat:
            yield "    (char const []){" + ", ".join(
                str(ord(x)) for x in char.cat[0]
            ) + "},"
        else:
            yield "    NULL,"

        if char.opcode is not None:
            yield "    (cahute_u16 const []){" + ", ".join(map(str, char.opcode)) + "},"
        else:
            yield "    NULL,"

        if char.unicode:
            yield f"    {len(char.unicode[0])},"
        else:
            yield "    0,"

        if char.cat:
            yield f"    {len(char.cat[0])},"
        else:
            yield "    0,"

        if char.opcode is not None:
            yield f"    {len(char.opcode)}"
        else:
            yield "    0"

        yield "};"
        yield ""

    # ---
    # Export all tables directly.
    # ---

    for table_key, table in ref.tables.items():
        for lead in MULTI_BYTE_LEADERS[table_key]:
            yield f"struct cahute_char_entry const *cahute_chars_{table_key}_{lead:02X}[] = " + "{"
            for index in range(256):
                suffix = "," if index < 255 else ""
                code = (lead << 8) | index
                if code in table.characters:
                    yield f"  &{table.characters[code].symbol}{suffix}"
                else:
                    yield f"  NULL{suffix}"

            yield "};"
            yield ""

    # ---
    # Export the CAT parsing trees.
    # ---

    for table_key, table in ref.tables.items():
        yield from get_sequence_parsing_tree_lines(
            table.cat_parsing_tree,
            symbol=f"cahute_cat_{table_key}_parsing_tree",
        )

        yield ""

    # ---
    # Export the Unicode parsing tree.
    # ---

    for table_key, table in ref.tables.items():
        yield from get_sequence_parsing_tree_lines(
            table.unicode_parsing_tree,
            symbol=f"cahute_unicode_{table_key}_parsing_tree",
            kind="u32",
        )
        yield ""


argument_parser = argparse.ArgumentParser(
    prog=Path(__file__).name,
    description="Produce the character source file from the reference.",
)
argument_parser.add_argument("path", type=Path, nargs="?")
argument_parser.add_argument("--reference", type=Path, default=DEFAULT_REFERENCE_PATH)

if __name__ == "__main__":
    args = argument_parser.parse_args()
    output_path = args.path or DEFAULT_OUTPUT_PATH
    ref_path = args.reference

    makedirs(output_path.parent, exist_ok=True)

    try:
        ref = CharacterReference.from_toml_file(ref_path)
    except ValueError:
        exit(1)

    with open(output_path, "w") as fp:
        for line in get_chars_c_lines(ref=ref):
            print(line, file=fp)
