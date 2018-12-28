
import sourceparser
from micropython import MicroPythonBindingsGenerator
from python import PythonBindingsGenerator

parseresult = sourceparser.LvglSourceParser().parse_sources('lvgl', extended_files=True)
mpy_gen = MicroPythonBindingsGenerator(parseresult)
mpy_gen.generate()

# TODO: when we decide on whether or not to parse extended c-files
# (those other than the ones defining objects), we can do away with
# only one time parsing

parseresult = sourceparser.LvglSourceParser().parse_sources('lvgl')
py_gen = PythonBindingsGenerator(parseresult)
py_gen.generate()

