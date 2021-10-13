#!/usr/bin/env python3
import sys
import os
import shutil
import glob

if len(sys.argv)<=1:
    sys.argv = ['setup.py', 'build']

from distutils.core import setup, Extension


sources = ['lvglmodule.c']
for path in 'lv_core', 'lv_draw', 'lv_hal', 'lv_misc', 'lv_widgets', 'lv_themes', 'lv_font':
    sources.extend(glob.glob('lvgl/src/'+ path + '/*.c'))

module1 = Extension('lvgl',
    sources = sources,
    extra_compile_args = [] if os.name =='nt' else ["-g","-Wno-unused-function"]
    )

dist = setup (name = 'lvgl',
       version = '0.1',
       description = 'lvgl bindings',
       ext_modules = [module1])

for output in dist.get_command_obj('build_ext').get_outputs():
    shutil.copy(output, '.')
