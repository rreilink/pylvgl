import sys
import os
import shutil
import glob

sys.argv = ['setup.py', 'build']

from distutils.core import setup, Extension


sources = ['lvglmodule.c']
for path in 'lv_core', 'lv_draw', 'lv_hal', 'lv_misc', 'lv_objx', 'lv_themes', 'lv_misc/lv_fonts':
    sources.extend(glob.glob('lvgl/'+ path + '/*.c'))

module1 = Extension('lvgl',
    sources = sources
    )

setup (name = 'lvgl',
       version = '0.1',
       description = 'lvgl bindings',
       ext_modules = [module1])

if os.name == 'nt':
    shutil.copy(r'build\lib.win-amd64-3.6\lvgl.cp36-win_amd64.pyd', '.')

else:
    shutil.copy('build/lib.macosx-10.7-x86_64-3.6/lvgl.cpython-36m-darwin.so', '.')
    