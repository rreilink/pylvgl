import sys
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
       version = '1.0',
       description = 'lvgl bindings',
       ext_modules = [module1])

shutil.copy('build/lib.macosx-10.7-x86_64-3.6/lvgl.cpython-36m-darwin.so', '.')
