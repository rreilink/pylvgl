'''
Generate the Python bindings module for LittlevGL


This script requires Python >=3.6 for formatted string literals and order-
preserving dictionaries
'''


from collections import namedtuple, Counter
from itertools import chain
import re
import glob

FunctionDef = namedtuple('FunctionDef', 'name restype args contents customcode')
Argument = namedtuple('Argument', 'name type')
EnumItem = namedtuple('EnumItem', 'name value')
StructItem = namedtuple('StructItem', 'name type')

def preprocess(filename):
    '''
    Preprocess H-file given by filename, stripping comments, preprocessor 
    directives, returning everything within the 'extern "C" {' ... '}'
    clause
    
    '''
    
    with open(filename, 'r') as file:
        c = file.read()
        
    # C-code preprocessing
    c = re.sub(r'/\*.*?\*/', '', c, flags = re.DOTALL) # strip comments
    c = re.sub(r'^\s*#.*', '', c, flags = re.MULTILINE)                          # strip preprocessor  
    
    c = re.sub(r'.*extern "C" {(.*)}.*', r'\1', c, flags = re.DOTALL) # get contents of extern "C" { } if exists (otherwise keep all code)
    cc = c
    c = re.sub(r'"([^"\\]|(\\\\)*\\.)*"', '""', c, flags=re.DOTALL)     # strip strings (replace by empty strings)

    assert '@' not in c
    
    # Replace all closing braces } at top-level by @
    
    result = ''
    level = 0
    for ch in c:
        if ch == '{':
            level += 1
        elif ch == '}':
            assert level > 0
            level -= 1
            if level == 0:
                ch = '@'
        result += ch
  
    return result
        

def parse(filename, functions, enums):
    '''
    Parse H-file given by filename; add all function definitions to functions
    dict (dict of name->FunctionDef namedtuples) and add all enum definitions to
    enums dict (dict of name->list of EnumItem namedtuples)
    '''

    c = preprocess(filename)
    
    # Remove all enum definitions and store them (as str) into cenums list
    cenums = []
    c = re.sub(r'typedef\s+enum\s+{(.*?)@\s*([a-zA-Z0-9_]+);', lambda x: cenums.append(x.groups()) or '', c, flags = re.DOTALL)

    
    # Process enums
    for enumcode, enumname in cenums:
        value = 0
        values = []
        for item in enumcode.split(','):
            item = re.sub(r'\s+', '', item) # remove all whitespace (also internal)
            if not item:
                break
            name, sep, valuestr = item.partition('=')
            if valuestr:
                value = int(valuestr[2:],16) if valuestr.startswith('0x') else int(valuestr)
            values.append(EnumItem(name, value))
            value += 1
        
        enums[enumname] = values

    

    # Find method definitions
    for static, restype, resptr, name, argsstr, contents in re.findall(r'(static\s+inline\s+)?(bool|void|lv_\w+_t)(\s+\*)?\s+(\w+)\(([\w\s*&,()]*?)\)\s*(?:{([^@]*)@|;)', c, re.DOTALL):

        if '(' in argsstr:
            print(f'{name} cannot be bound since it has a function pointer as argument')
            continue
            
        args = []
        if argsstr.strip() != 'void':
            for arg in argsstr.split(','):
                arg = arg.strip()
                
                const, argtype, argptr, argname = re.match(
                        r'(const\s+)?([A-Za-z0-9_]+)\s*(\*+| )\s*([A-Za-z0-9_]+)$', arg).groups()
                argtype += argptr.strip()
                
                args.append(Argument(argname, argtype))

        functions[name]=(FunctionDef(name, restype + resptr.strip(), args, contents, False))



# Use C-files to get class hierarchy    
functions_c = {}
enums_c = {}
for filename in [f for f in glob.glob('lvgl/lv_objx/*.c') if not f.endswith('lv_objx_templ.c')]:
    parse(filename, functions_c, enums_c)

ancestors = {}
for function in functions_c.values():
    if function.name.endswith('_create'):
        results = re.findall('(lv_[A-Za-z0-9_]+)_create\s*\(\s*par', function.contents)
        assert len(results) == 1
        ancestors[function.name[:-7]] = results[0]

# Sort the ancestors dictorary such that no object comes before its ancestor
sortedancestors = {'lv_obj': None}
while ancestors:
    remaining = list(ancestors.items())
    for obj, anch in remaining:
        if anch in sortedancestors:
            sortedancestors[obj] = anch
            ancestors.pop(obj)

ancestors = sortedancestors

# Use H-files to get external function definitions    
functions = {}
enums = {}
for filename in ['lvgl/lv_core/lv_obj.h'] + [f for f in glob.glob('lvgl/lv_objx/*.h') if not f.endswith('lv_objx_templ.h')]:
    parse(filename, functions, enums)

assert(len(functions) == 328)

skipfunctions = {
    # Not present in the C-files, only in the headers
    'lv_ddlist_close_en',
    'lv_ta_get_cursor_show',
    
    # Don't tamper with deleting objects which have Python references
    'lv_obj_del',
    'lv_obj_clean',
    
    # free_ptr is used to store reference to Python objects, don't tamper with that!
    'lv_obj_set_free_ptr',
    'lv_obj_get_free_ptr',
    
    # Just use Python attributes for custom properties of objects
    'lv_obj_set_free_num',
    'lv_obj_get_free_num',
    'lv_obj_allocate_ext_attr',
    'lv_obj_get_ext_attr',
    
    # Do not work since they require lv_obj_t == NULL which is not implemented
    # lv_obj_getchildren is implemented instead which returns a list
    'lv_obj_get_child',
    'lv_obj_get_child_back',
    
    # Not compatible since it is like a 'class method' (it does not have 
    # lv_obj_t* as first argument). Implemented as lvgl.report_style_mod
    'lv_obj_report_style_mod',
    
    # does nothing special; covered by superclass lv_btnm_set_map
    'lv_kb_set_map',
    }

#
# Mark which functions have a custom implementation (in lvglmodule_template.c)
#
for custom in ('lv_obj_get_children', ):
    functions[custom] = FunctionDef(custom, None, None, None, True)


allfunctions = functions

#
# Grouping of functions with objects and selection of functions
#

def objectname(functionname):
    match =  re.match(r'(lv_[a-zA-Z0-9]+)_[a-zA-Z0-9_]+', function.name)
    return match.group(1) if match else None

class Object:
    def __init__(self, name, ancestor):
        self.name = name
        self.ancestor = ancestor
        self.functions = []
        self.structfields = []
    def PyObject_name(self):
        return 'pylv_' + self.name[3:].title()

    def __str__(self):
        return self.name


# Step 1: determine which objects exist based on the _create functions
objects = {}
for name, ancestor in sortedancestors.items():
    objects[name] = Object(name, objects.get(ancestor))

#functions = sorted(f for f in functions.values() if not any(re.match(exp, f.name) for exp in skipfunctions))

# Step 2: distribute the functions over the objects
for function in functions.values():
    object = objects.get(objectname(function.name))
    if object is None:
        print(function.name)
    elif not function.name.endswith('_create') and function.name not in skipfunctions:
        object.functions.append(function)


# Step 3: custom options
objects['lv_obj'].structfields.extend(['PyObject_HEAD', 'lv_obj_t *ref;'])


typeconv = {
    'lv_obj_t*': ('O!', 'pylv_Obj *'),     # special case: conversion from/to Python object
    'lv_style_t*': ('O!', 'Style_Object *'), # special case: conversion from/to Python Style object
    'bool':      ('p', 'int'),
    'uint8_t':   ('b', 'unsigned char'),
    'lv_opa_t':  ('b', 'unsigned char'),
    'lv_color_t': ('H', 'unsigned short int'),
    'char':      ('c', 'char'),
    'char*':     ('s', 'char *'),
    'lv_coord_t':('h', 'short int'),
    'uint16_t':  ('H', 'unsigned short int'), 
    'int16_t':   ('h', 'short int'),
    'uint32_t':  ('I', 'unsigned int'),
    
    }

argtypes_miss = Counter()
restypes_miss = Counter()

def functioncode(function, object):
    startCode = f'''
static PyObject*
py{function.name}(pylv_Obj *self, PyObject *args, PyObject *kwds)
{{
'''

    notImplementedCode = startCode + '    PyErr_SetString(PyExc_NotImplementedError, "not implemented");\n    return NULL;\n}\n'

    if function.customcode:
        return '' # Custom implementation is in lvglmodule_template.c
    
    argnames = []
    argctypes = []
    argfmt = ''
    for argname, argtype in function.args:
        if argtype in enums:
            fmt, ctype = 'i', 'int'
        else:
            try:
                fmt, ctype = typeconv[argtype]
            except KeyError:
                print(f'{function.name}: Argument type not found >{argtype}< ', [','.join(arg.name for arg in function.args)])
                argtypes_miss.update((argtype,))
                return notImplementedCode;
        argnames.append(argname)
        argctypes.append(ctype)
        argfmt += fmt
    
    
    if function.restype == 'void':
        resfmt, resctype = None, None
    else:
        if function.restype in enums:
            resfmt, resctype = 'i', 'int'
        else:
            try:
                resfmt, resctype = typeconv[function.restype]
            except KeyError:
                print(f'{function.name}: Return type not found >{function.restype}< ')
                restypes_miss.update((function.restype,))
                return notImplementedCode            

    # First argument should always be a reference to the object itself
    assert argctypes and argctypes[0] == 'pylv_Obj *'
    argnames.pop(0)
    argctypes.pop(0)
    argfmt = argfmt[2:]
    
    code = startCode
    kwlist = ''.join('"%s", ' % name for name in argnames)
    code += f'    static char *kwlist[] = {{{kwlist}NULL}};\n';
    
    crefvarlist = ''
    cvarlist = ''
    for name, ctype in zip(argnames, argctypes):
        code += f'    {ctype} {name};\n'
        if ctype == 'pylv_Obj *' : # Object, convert from Python
            crefvarlist += f', &pylv_obj_Type, &{name}'
            cvarlist += f', {name}->ref'
        
        elif ctype == 'Style_Object *': # Style object
            crefvarlist += f', &Style_Type, &{name}'
            cvarlist += f', {name}->ref'
            
        else:
            crefvarlist += f', &{name}'
            cvarlist += f', {name}'
    
    code += f'    if (!PyArg_ParseTupleAndKeywords(args, kwds, "{argfmt}", kwlist {crefvarlist})) return NULL;\n'
    
    callcode = f'{function.name}(self->ref{cvarlist})'
    
    if resctype == 'pylv_Obj *':
        # Result of function is an lv_obj; find the corresponding Python
        # object using lv_obj_get_free_ptr
        code += f'''
    lv_obj_t *result = {callcode};
    
    if (!result) Py_RETURN_NONE;
    
    PyObject *retobj = lv_obj_get_free_ptr(result);
    if (retobj) {{
        Py_INCREF(retobj);
        return retobj;
    }} else {{
        PyErr_SetString(PyExc_RuntimeError, "resulting object not known in Python");
        return NULL;
    }}
'''
    
    elif resctype == 'Style_Object *':
        code += f'    return Style_From_lv_style({callcode});\n'
        xcode = f'''
    lv_style_t *result = {callcode};
    if (!result) Py_RETURN_NONE;
    
    Style_Object *retobj = PyObject_New(Style_Object, &Style_Type);
    if (retobj) retobj->ref = result;
    return (PyObject *)retobj;
'''
    
    elif resctype is None:
        code += f'    {callcode};\n'
        code += f'    Py_RETURN_NONE;\n'
    else:
        code += f'    {resctype} result = {callcode};\n';
        if resfmt == 'p': # Py_BuildValue does not support 'p' (which is supported by PyArg_ParseTuple..)
            code += '    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}\n'
        else:
            code += f'    return Py_BuildValue("{resfmt}", result);\n'
   
            
        
    return code + '}\n';


#
# Object definition, methods and method table for each object
#
objectdefcode = ''

for obj in objects.values():

    # Method definitions for the object methods (see also functioncode function)
    for func in obj.functions:
        objectdefcode += functioncode(func, obj)

    # Method table
    objectdefcode += f'static PyMethodDef py{obj}_methods[] = {{\n'
    
    for func in obj.functions:
        methodname = func.name[len(obj.name)+1:]
        
        objectdefcode += f'    {{"{methodname}", (PyCFunction) py{func.name}, METH_VARARGS | METH_KEYWORDS, NULL}},\n'

    objectdefcode += '    {NULL}  /* Sentinel */\n};'
    
    objclass = obj.name[3:].title()
    
    objectdefcode += f'''
static PyTypeObject py{obj}_Type = {{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.{objclass}",
    .tp_doc = "lvgl {obj.name[3:]}",
    .tp_basicsize = sizeof({obj.PyObject_name()}),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) py{obj}_init,
    .tp_dealloc = (destructor) py{obj}_dealloc,
    .tp_methods = py{obj}_methods,
}};
'''


fields = {}
fields['OBJECT_DEFINITIONS'] = objectdefcode


# Definition of object structs
objectstructs = ''
for obj in objects.values():
    structfields = (obj.ancestor.structfields if obj.ancestor else []) + obj.structfields
    structfieldscode = '\n    '.join(structfields)
    if obj.ancestor and not obj.structfields:
        # Same fields as ancestors
        objectstructs += f'typedef {obj.ancestor.PyObject_name()} {obj.PyObject_name()};\n\n'
    else:
        # Additional fields (or pylv_Obj, the root object struct)
        objectstructs += f'''\
typedef struct {{
    {structfieldscode}
}} {obj.PyObject_name()};

'''
fields['OBJECT_STRUCTS'] = objectstructs



enumcode = ''
# Adding of the enum constants to the module
for name, items in enums.items():
    for name, value in items:
        assert name.startswith('LV_')
        enumcode += (f'    PyModule_AddObject(module, "{name[3:]}", PyLong_FromLong({value}));\n')

fields['ENUM_ASSIGNMENTS'] = enumcode

# Style
def generate_style_getset_table(stylestruct):

    # Recursively flatten struct-within-struct:
    # `struct { int a; int b;} c;` becomes `int c.a; int c.b;`
    # (which is invalid C-code but it is parsed appropriately later)
    def flatten_struct_contents(match):
        contents, name = match.groups()
        contents = re.sub(r'([A-Za-z0-9_\.]+\s*(:\s*[0-9]+\s*)?;)', name + r'.\1', contents)
        return contents
    
    while True:
        stylestruct, replacements = re.subn(r'struct\s{([^{}]*)}\s*([a-zA-Z0-9_]*);', flatten_struct_contents, stylestruct)
        if not replacements:
            break
    
    # Generate the table of getters and setters
    # The closure field of PyGetSetDef is used as offset into the lv_style_t struct
    table = ''
    t_types = {
        'uint8_t:1': None,
        'uint8_t': 'uint8',
        'lv_color_t': 'uint16',
        'lv_opa_t': 'uint8',
        'lv_coord_t': 'int16',
        'lv_border_part_t': None,
        'const lv_font_t *': None,
    
        }
    for statement in stylestruct.strip().split(';'):
        if not statement:
            continue
        type, name, bits = re.match(r'\s*(.*)\s+([A-Za-z0-9_\.]+)\s*(:\s*[0-9]+\s*)?', statement, re.DOTALL).groups()

        t_type = t_types[type+(bits or '')]
        
        if t_type:
            table += f'   {{"{name.replace(".", "_")}", (getter) Style_get_{t_type}, (setter) Style_set_{t_type}, "{name}", (void*)offsetof(lv_style_t, {name})}},\n'

    return table

style_h_code = preprocess('lvgl/lv_core/lv_style.h')
stylestruct = re.findall(r'typedef\s+struct\s*{(.*?)@\s*lv_style_t\s*;', style_h_code, re.DOTALL)[0]

fields['STYLE_GETSET'] = generate_style_getset_table(stylestruct)

# Create module attributes for the existing styles defined in lv_style.h
style_assignments = ''
for style in re.findall(r'extern\s+lv_style_t\s+lv_([A-Za-z0-9_]+)\s*;', style_h_code):
    style_assignments += f'    PyModule_AddObject(module, "{style}",Style_From_lv_style(&lv_{style}));\n'
    

fields['STYLE_ASSIGNMENTS'] = style_assignments

#
# Fill in the template
#
with open('lvglmodule_template.c') as templatefile:
    template = templatefile.read()


def substitute_per_object(match):
    '''
    Given a template, fill it in for each object and return the concatenated result
    '''
    
    template = match.group(1)
    ret = ''
    for obj in objects.values():
        assert obj.name.startswith('lv_')
        if obj.name == 'lv_obj':
            base = 'NULL'
        else:
            base = '&py' + obj.ancestor.name + '_Type'
        
        ret += template.format(**{
            'obj' :obj.name, 
            'typename' : obj.name[3:].title(),
            'base': base,
            'pylv_Obj': obj.PyObject_name(),
            }
            )

    return ret

# Substitute per-object sections
modulecode = re.sub(r'<<<(.*?)>>>', substitute_per_object, template, flags = re.DOTALL)

# Substitute general fields
modulecode = re.sub(r'<<(.*?)>>', lambda x: fields[x.group(1)], modulecode)


with open('lvglmodule.c', 'w') as modulefile:
    modulefile.write(modulecode)

