"""
This file is part of lustre-tools-llnl.

SPDX-License-Identifier: GPL-2.0
See [LICENSE-GPL-2.0](https://github.com/LLNL/lustre-tools-llnl/LICENSE-GPL-2.0)

Copyright (c) 2011, Lawrence Livermore National Security, LLC.
Produced at the Lawrence Livermore National Laboratory.
LLNL-CODE-468512

A test script for llogcolor.
Set up to simulate calling llogcolor
from the command line.

The reference files are visually checked,
and in some cases verified idential to the
output from the original python2 version of
llogcolor.

"""

import pathlib
import subprocess
import unittest

llogcolor_path = pathlib.Path("../scripts/llogcolor")


def cmp_stdout_to_ref(args, input_files, output_file):
    """Compare the output from llogcolor
    to a reference file of an assumed good output.
    """
    # run llogcolor, and capture its output
    cmd = subprocess.run(
        [llogcolor_path] + args + input_files,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=True,
    )

    # check that the output matches the reference
    with open(output_file, "r") as f:
        reference_text = f.read()
        if cmd.stdout.decode() != reference_text:
            return False
    return True


class TestTextOutput(unittest.TestCase):
    """Tests that create output and then compare
    that output to a reference file.
    """

    # each element will be run as a separate test
    subtest_params = [
        {
            "msg": "read a log file in, and just send it unmodified to stdout",
            "flags": ["-P", "-C"],
            "input_files": ["llogcolor_files/input-single_tid-once"],
            "output_file": "llogcolor_files/input-single_tid-once",
        },
        {
            "msg": (
                "read a log file in, and send the text to stdout. "
                "In this case the text includes the terminal color codes "
                "However, all the lines will have the same color code."
            ),
            "flags": ["-P"],
            "input_files": ["llogcolor_files/input-single_tid-once"],
            "output_file": "llogcolor_files/output-color-single_tid-once",
        },
        {
            "msg": (
                "read a log file in, and send the text to stdout. "
                "assign differnet colors to differnt threads."
            ),
            "flags": ["-P"],
            "input_files": ["llogcolor_files/input-multi_tid-once"],
            "output_file": "llogcolor_files/output-color-multi_tid-once",
        },
        {
            "msg": (
                "read a log file in, and send the text to stdout. "
                "assign differnet colors to differnt threads. "
                "check that thread gets assigned the same color when "
                "processed the second time."
            ),
            "flags": ["-P"],
            "input_files": ["llogcolor_files/input-multi_tid-twice"],
            "output_file": "llogcolor_files/output-color-multi_tid-twice",
        },
    ]

    def test_compare_to_reference(self):
        """Check that llogcolor, when given the proper inputs
        produces the proper output, by comparing the output
        to a known reference.
        """
        for params in self.subtest_params:
            with self.subTest(**params):
                self.assertTrue(
                    cmp_stdout_to_ref(
                        params["flags"],
                        params["input_files"],
                        params["output_file"],
                    )
                )


if __name__ == "__main__":
    unittest.main()
