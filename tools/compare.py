PYTHON2 = '/opt/local/bin/python2.7'
PYTHON36 = '/opt/local/bin/python3.6' # Python 3.6 or higher, with order-preserving dictionaries

import subprocess
import re


with open('../lvgl/micropython/lv_mpy.c') as file:
    original = file.read()
##
command = re.findall(r' \* gen_mpy\.py\s+(.*)$', original, re.MULTILINE)[0]

py2_result = subprocess.check_output([PYTHON2, '../../tools/gen_mpy.py'] + command.split(), cwd = '../lvgl/micropython').decode()

assert py2_result == original

py3_result = subprocess.check_output([PYTHON36, '../../tools/gen_mpy.py'] + command.split(), cwd = '../lvgl/micropython').decode()

##
# py2_result and py3_ result should have the same lines, but the order could
# be different due to dependence of gen_py output on dictionary order
#
# furthermore, since the last item in a list of 
# {
#   {a},
#   {b},
#   {c},
# }
# has no comma after '{c}', the lines are not exactly identical. Use regex 
# replace to replace all }, at the end of the line by }

py2_result_modified = re.sub('},$','}', py2_result, flags = re.MULTILINE)
py3_result_modified = re.sub('},$','}', py3_result, flags = re.MULTILINE)


p2_result_modified_sorted = sorted(py2_result_modified.splitlines())
p3_result_modified_sorted = sorted(py3_result_modified.splitlines())

assert p2_result_modified_sorted == p3_result_modified_sorted
with open ('lv_mpy3.c', 'w') as file:
    file.write(py3_result)


