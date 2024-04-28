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
"""Process all binary files in a directory.

This looks for all files in the directory with the '.bin' extension, then
processes it.
"""

import argparse
import re
from datetime import datetime
from os import linesep, makedirs
from pathlib import Path
import sys
from typing import TextIO

symbol_pattern = re.compile(r"[a-z0-9_]*")


def symbol_type(arg_value: str, /) -> str:
    """Symbol type for the argument parser.

    :param arg_value: Raw argument value.
    :return: Processed argument value.
    """
    if not symbol_pattern.fullmatch(arg_value):
        raise argparse.ArgumentTypeError("invalid format")
    return arg_value


def path_to_symbol(name: str, /) -> str:
    """Get a symbol out of a file path.

    :param file_name: File name to get a symbol from.
    :return: File name.
    """
    symbol = name.casefold().translate(str.maketrans("-.", "__"))
    return symbol_type(symbol)


def write_c_source(contents: bytes, fp: TextIO, /, *, symbol: str) -> None:
    """Write the C source for a contents.

    :param contents: Contents for which to write the source.
    :param fp: File pointer to write the source to.
    :param symbol: Symbol to write the contents as.
    """
    dt = datetime.utcnow().isoformat(timespec="seconds") + "Z"

    fp.write(
        f"/* This file was generated on {dt} by "
        + f"contrib/process_binary_files.py. */{linesep}{linesep}"
        + f"#include <cahute/cdefs.h>{linesep}{linesep}"
        + f"size_t const {symbol}_size = {len(contents)};{linesep}"
        + f"cahute_u8 const {symbol}[] = "
        + "{"
        + linesep,
    )

    last_line = ""
    for offset in range(0, len(contents), 8):
        if last_line:
            fp.write(last_line + linesep)

        last_line = (
            "    " + ", ".join(map(str, contents[offset : offset + 8])) + ","
        )

    # Remove the last comma.
    if last_line:
        fp.write(last_line[:-1] + linesep)

    fp.write("}" + f";{linesep}")


argument_parser = argparse.ArgumentParser(
    prog=Path(__file__).name,
    description="Process a binary file.",
)
argument_parser.add_argument("output_path", type=Path)
argument_parser.add_argument("source_path", type=Path)
argument_parser.add_argument("--prefix", type=symbol_type, default="cahute_")

if __name__ == "__main__":
    args = argument_parser.parse_args()
    source_path = args.source_path
    output_path = args.output_path

    if not source_path.name.endswith(".bin"):
        print("Expected a source path with a .bin extension!", file=sys.stderr)
        exit(1)

    if not output_path.name.endswith(".c"):
        print("Expected an output path with a .c extension!", file=sys.stderr)
        exit(1)

    base_name = source_path.name[:-4]

    makedirs(output_path.parent, exist_ok=True)

    symbol = args.prefix + path_to_symbol(base_name)
    with source_path.open("rb") as fp:
        contents = fp.read()

    with output_path.open("w") as fp:
        write_c_source(contents, fp, symbol=symbol)
