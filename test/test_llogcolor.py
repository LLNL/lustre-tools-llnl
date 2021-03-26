"""A test suite for comparing the text output
of llogcolor for an assumed-to-be-correct output.

The pager is skipped for these tests, and the
color control codes go strait to a file, so they get saved.
"""

import filecmp
import pathlib
import sys
import tempfile
import unittest

# now I need to import llogcolor
# this isn't the normal way to import,
# but there are 2 issues.
# 1. this is not a python only project, I don't want
# to structure all of lustre-tools-llnl as a python package
# 2. the file to be tested doesn't have '.py' suffix
# from https://stackoverflow.com/questions/2601047/
from importlib.util import spec_from_loader, module_from_spec
from importlib.machinery import SourceFileLoader

path_to_llogcolor = str(pathlib.Path("../scripts/llogcolor").resolve())
spec = spec_from_loader(
    "llogcolor", SourceFileLoader("llogcolor", path_to_llogcolor)
)
llogcolor = module_from_spec(spec)
spec.loader.exec_module(llogcolor)

#### paths to input logs ###
# small and simple log file, all of the same thread ID
in_simple = "llogcolor_files/sample_logs"

### paths to output logs ###
# [-P, -C, in_simple]
out_P_C_simple = "llogcolor_files/sample_logs"
# [-C, in_simple]
out_P_simple = "llogcolor_files/out_simple_color"


def cmp_stdout_to_ref(test_args, reference):
    """Compare stdout to reference file.
    run llogcolor with test_args and write
    the output to a temporary file.
    Then compare that file to a reference file.
    Return True if the files match and False otherwise.
    """
    # stdout will be changed to the temporary file
    # to receive the llogcolor output
    # so save the real stdout now
    real_stdout = sys.stdout

    # create a temporary file
    # and aim stdout at it
    fd, filename = tempfile.mkstemp()
    f = open(filename, "w")
    sys.stdout = f

    # run llogcolor with test_args and save the return value
    ret_val = llogcolor.main(test_args=test_args)

    # put stdout back to normal
    sys.stdout = real_stdout
    # not sure if this is neccesary, but want it
    # it all written befor the compare
    f.close()

    files_match = filecmp.cmp(reference, filename)

    success = ((ret_val is None) or (ret_val == 0)) and files_match
    return success


class TestTextOutput(unittest.TestCase):
    """Tests that create output and then compare
    that output to a reference file.
    """

    def test_P_C_simple(self):
        """read a log file in, and just send it unmodified to stdout"""
        # no pager, no color
        test_args = ["-P", "-C", in_simple]
        reference = out_P_C_simple

        self.assertTrue(cmp_stdout_to_ref(test_args, reference))

    def test_P_simple(self):
        """read a log file in, and send the text to stdout.
        In this case the text includes the terminal color codes
        However, all the lines will have the same color code.
        """
        # no pager
        test_args = ["-P", in_simple]
        reference = out_P_simple

        self.assertTrue(cmp_stdout_to_ref(test_args, reference))


if __name__ == "__main__":
    unittest.main()
