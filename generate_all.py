#!/usr/bin/env python3
import sourceparser
from micropython import MicroPythonBindingsGenerator
from python import PythonBindingsGenerator

parseresult = sourceparser.LvglSourceParser().parse_sources('lvgl')
#mpy_gen = MicroPythonBindingsGenerator(parseresult)
#mpy_gen.generate()
py_gen = PythonBindingsGenerator(parseresult)
py_gen.generate()

