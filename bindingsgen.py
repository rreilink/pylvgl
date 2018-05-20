from collections import namedtuple
from itertools import chain
import re
import glob

FunctionDef = namedtuple('FunctionDef', 'name restype args customcode')
Argument = namedtuple('Argument', 'name type')
EnumItem = namedtuple('EnumItem', 'name value')

def parse(filename, functions, enums):
    '''
    Parse H-file given by filename; add all function definitions to functions
    dict (dict of name->FunctionDef namedtuples) and add all enum definitions to
    enums dict (dict of name->list of EnumItem namedtuples)
    '''
    with open(filename, 'r') as file:
        c = file.read()
        
    # C-code preprocessing
    c = re.sub('/\*.*?\*/', '', c, flags = re.DOTALL) # strip comments
    c = re.sub('#.*', '', c)                          # strip preprocessor  
    
    c = re.findall('extern "C" {(.*)}', c, re.DOTALL)[0] # strip extern "C" { }

    # Remove all enum definitions and store them (as str) into cenums list
    cenums = []
    c = re.sub('typedef\s+enum\s+{(.*?)}\s*([a-zA-Z0-9_]+);', lambda x: cenums.append(x.groups()) or '', c, flags = re.DOTALL)

    c = re.sub('{.*?}', '{}', c, flags = re.DOTALL)   # strip internals of struct and functions
    
    
    # Process enums
    for enumcode, enumname in cenums:
        value = 0
        values = []
        for item in enumcode.split(','):
            item = re.sub('\s+', '', item) # remove all whitespace (also internal)
            if not item:
                break
            name, sep, valuestr = item.partition('=')
            if valuestr:
                value = int(valuestr[2:],16) if valuestr.startswith('0x') else int(valuestr)
            values.append(EnumItem(name, value))
            value += 1
        
        enums[enumname] = values


    # Find method definitions
    for static, restype, ptr, name, argsstr, end in re.findall('(static\s+inline\s+)?(bool|void|lv_[a-zA-Z0-9_]+_t)(\s+\*)?\s+([a-zA-Z0-9_]+)\((.*?)\)\s*({|;)', c):

        if '(' in argsstr:
            print(f'{name} cannot be bound since it has a function pointer as argument')
            continue
        args = []
        if argsstr.strip() != 'void':
            for arg in argsstr.split(','):
                arg = arg.strip()
                
                const, argtype, ptr, argname = re.match(
                        '(const\s+)?([A-Za-z0-9_]+)\s*(\*+| )\s*([A-Za-z0-9_]+)$', arg).groups()
                argtype += ptr.strip()
                
                args.append(Argument(argname, argtype))

        functions[name]=(FunctionDef(name, restype + ptr.strip(), args, None))
        
    return functions, enums


def objectname(functionname):
    match =  re.match('(lv_[a-zA-Z0-9]+)_[a-zA-Z0-9_]+', function.name)
    return match.group(1) if match else None
    
functions = {}
enums = {}
for filename in ['lvgl/lv_core/lv_obj.h'] + [f for f in glob.glob('lvgl/lv_objx/*.h') if not f.endswith('lv_objx_templ.h')]:
    parse(filename, functions, enums)

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
    
    # Do not work since they require lv_obj_t == NULL which is not implemented
    # lv_obj_getchildren is implemented instead which returns a list
    'lv_obj_get_child',
    'lv_obj_get_child_back',
    
    }

#
# Custom code for certain functions
#

# lv_obj_get_child and lv_obj_get_child_back are not really Pythonic. This
# implementation returns a list of children
functions['lv_obj_get_children'] = FunctionDef('lv_obj_get_children', None, None, '''
    lv_obj_t *child = NULL;
    pylv_obj_Object *pychild;
    PyObject *ret = PyList_New(0);
    if (!ret) return NULL;
    
    while (1) {
        child = lv_obj_get_child(self->ref, child);
        if (!child) break;
        pychild = lv_obj_get_free_ptr(child);
        
        if (!pychild) {
            // Child is not known to Python, create a new Object instance
            pychild = PyObject_New(pylv_obj_Object, &pylv_obj_Type);
            pychild -> ref = child;
            lv_obj_set_free_ptr(child, pychild);
        }
        
        // Child that is known in Python
        if (PyList_Append(ret, (PyObject *)pychild)) { // PyList_Append increases refcount
            // If PyList_Append fails, how to clean up?
            return NULL;
        }
    }
    
    return ret;

''')


allfunctions = functions

#
# Grouping of functions with objects and selection of functions
#

# Step 1: determine which objects exist based on the _create functions
objects = {}
for function in functions.values():
    if function.name.endswith('_create'):
        objects[objectname(function.name)] = []

#functions = sorted(f for f in functions.values() if not any(re.match(exp, f.name) for exp in skipfunctions))

# Step 2: distribute the functions over the objects
for function in functions.values():
    objectfunctions = objects.get(objectname(function.name))
    if objectfunctions is None:
        print(function.name)
    elif not function.name.endswith('_create') and function.name not in skipfunctions:
        objectfunctions.append(function)

# Step 3: collect only those functions that belong to objects, store in a list
functions = list(chain(*objects.values()))


# Writing of the output file

typeconv = {
    'lv_obj_t*': ('O!', '!'), # special case: conversion from/to Python object
    'bool':      ('p', 'int'),
    'uint8_t':   ('b', 'unsigned char'),
    'char':      ('c', 'char'),
    'char*':     ('s', 'char *'),
    'lv_coord_t':('h', 'short int'),
    'uint16_t':  ('H', 'unsigned short int'), 
    'int16_t':   ('h', 'short int'),
    'uint32_t':  ('I', 'unsigned int'),
    
    }

def functioncode(function):
    notImplementedCode = '    PyErr_SetString(PyExc_NotImplementedError, "not implemented");\n    return NULL;'

    if function.customcode:
        return function.customcode
    
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
                print(f'{function.name}: Type not found >{argtype}< ')
                return notImplementedCode
        argnames.append(argname)
        argctypes.append(ctype)
        argfmt += fmt
    
    assert argctypes and argctypes[0] == '!'
    
    retfmt, retctype = typeconv.get(function.restype, (None, None))
    
    argnames.pop(0)
    argctypes.pop(0)
    argfmt = argfmt[2:]
    
    code = ''
    kwlist = ''.join('"%s", ' % name for name in argnames)
    code += f'    static char *kwlist[] = {{{kwlist}NULL}};\n';
    
    crefvarlist = ''
    cvarlist = ''
    for name, ctype in zip(argnames, argctypes):
        if ctype == '!' : # Object, convert from Python
            print(f'lv_obj_t for {function.name}')
            code += f'     pylv_obj_Object * {name};\n'
            crefvarlist += f', &pylv_obj_Type, &{name}'
            cvarlist += f', {name}->ref'

        else:
            code += f'    {ctype} {name};\n'
            crefvarlist += f', &{name}'
            cvarlist += f', {name}'
    
    code += f'    if (!PyArg_ParseTupleAndKeywords(args, kwds, "{argfmt}", kwlist {crefvarlist})) return NULL;\n'
    
    callcode = f'{function.name}(self->ref{cvarlist})'
    
    if retctype == '!':
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
        
    elif retctype is None:
        code += f'    {callcode};\n'
        code += f'    Py_RETURN_NONE;\n'
    else:
        code += f'    {retctype} result = {callcode};\n';
        code += f'    return Py_BuildValue("{retfmt}", result);\n'
   
            
        
    return code;


with open('lvglmodule.c', 'w') as file:
    file.write('''
#include "Python.h"
#include "lvgl/lvgl.h"

/* Forward declaration of type objects */
''')
    
    for obj in objects:
        file.write(f'static PyTypeObject py{obj}_Type;\n')

    file.write('''
/* Methods and object definitions */
''')

    #
    # Object definition, Type definition, methods and method table for each object
    #
    for obj, functions in objects.items():
        file.write(f'''

typedef struct {{
    PyObject_HEAD
    lv_obj_t *ref;
}} py{obj}_Object;

static void
py{obj}_dealloc(pylv_obj_Object *self) 
{{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}}

static int
py{obj}_init(py{obj}_Object *self, PyObject *args, PyObject *kwds) 
{{
    static char *kwlist[] = {{"parent", "copy", NULL}};
    pylv_obj_Object *parent;
    py{obj}_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &py{obj}_Type, &copy)) {{
        return -1;
    }}   
    self->ref = {obj}_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}}

''')

        # Method definitions for the object methods (see also functioncode function)
        for func in functions:
            file.write(f'''
static PyObject*
py{func.name}(py{obj}_Object *self, PyObject *args, PyObject *kwds)
{{
{functioncode(func)}
}}
            
''')

        # Method table
        file.write(f'static PyMethodDef py{obj}_methods[] = {{\n')
        
        for func in functions:
            methodname = func.name[len(obj)+1:]
            
            file.write(f'    {{"{methodname}", (PyCFunction) py{func.name}, METH_VARARGS | METH_KEYWORDS, NULL}},\n')
    
        file.write('    {NULL}  /* Sentinel */\n};')
        
        objclass = obj[3:].title()
        
        file.write(f'''
static PyTypeObject py{obj}_Type = {{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.{objclass}",
    .tp_doc = "lvgl {obj[3:]}",
    .tp_basicsize = sizeof(py{obj}_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) py{obj}_init,
    .tp_dealloc = (destructor) py{obj}_dealloc,
    .tp_methods = py{obj}_methods,
}};
''')


    
    file.write('''

static PyObject *
pylv_scr_act(PyObject *self, PyObject *args) {
    return lv_obj_get_free_ptr(lv_scr_act());
}

static PyObject *
poll(PyObject *self, PyObject *args) {
    lv_tick_inc(1);
    lv_task_handler();
    Py_RETURN_NONE;
}

static PyMethodDef lvglMethods[] = {
    {"scr_act",  pylv_scr_act, METH_NOARGS, NULL},
    {"poll", poll, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};



static struct PyModuleDef lvglmodule = {
    PyModuleDef_HEAD_INIT,
    "lvgl",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    lvglMethods
};


char framebuffer[LV_HOR_RES * LV_VER_RES * 2];
int redraw = 0;


/* disp_flush should copy from the VDB (virtual display buffer to the screen.
 * In our case, we copy to the framebuffer
 */
static void disp_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_p) {

    char *dest = framebuffer + ((y1)*LV_HOR_RES + x1) * 2;
    char *src = (char *) color_p;
    
    for(int32_t y = y1; y<=y2; y++) {
        memcpy(dest, src, 2*(x2-x1+1));
        src += 2*(x2-x1+1);
        dest += 2*LV_HOR_RES;
    }
    redraw++;
    
    lv_flush_ready();
}

static lv_disp_drv_t driver = {0};

PyMODINIT_FUNC
PyInit_lvgl(void) {

    PyObject *module = NULL;
    
    module = PyModule_Create(&lvglmodule);
    if (!module) goto error;
''')

    # Initialization of the types
    for obj in objects:
        if obj != 'lv_obj':
            file.write(f'    py{obj}_Type.tp_base = &pylv_obj_Type;\n')
        file.write(f'    if (PyType_Ready(&py{obj}_Type) < 0) return NULL;\n')


    # Adding of the types to the module
    for obj in objects:
        objclass = obj[3:].title()
        file.write(f'    Py_INCREF(&py{obj}_Type);\n')
        file.write(f'    PyModule_AddObject(module, "{objclass}", (PyObject *) &py{obj}_Type);\n')
     
    # Adding of the enum constants to the module
    for name, items in enums.items():
        for name, value in items:
            assert name.startswith('LV_')
            file.write(f'    PyModule_AddObject(module, "{name[3:]}", PyLong_FromLong({value}));\n')
        
    file.write('''

    PyModule_AddObject(module, "framebuffer", PyMemoryView_FromMemory(framebuffer, LV_HOR_RES * LV_VER_RES * 2, PyBUF_READ));
    PyModule_AddObject(module, "HOR_RES", PyLong_FromLong(LV_HOR_RES));
    PyModule_AddObject(module, "VER_RES", PyLong_FromLong(LV_VER_RES));

    driver.disp_flush = disp_flush;

    lv_init();
    
    lv_disp_drv_register(&driver);



    // Create a Python object for the active screen lv_obj and register it
    pylv_obj_Object * pyact = PyObject_New(pylv_obj_Object, &pylv_obj_Type);
    lv_obj_t *act = lv_scr_act();
    pyact -> ref = act;
    lv_obj_set_free_ptr(act, pyact);


    return module;
    
error:
    Py_XDECREF(module);
    return NULL;
}
''')







