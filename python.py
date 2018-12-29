import collections
import re

from bindingsgen import Object, BindingsGenerator, c_ast, stripstart, generate_c, CustomMethod, type_repr, flatten_struct

# TODO: should be common for all bindings generators
skipfunctions = {

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
#    'lv_obj_report_style_mod',
    
}

# TODO: remove these
paramtypes_miss = collections.Counter()
restypes_miss = collections.Counter()


class PythonObject(Object):
        
    TYPECONV = {
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
    

    
    def init(self):
        
        if self.name == 'obj':
            self.base = 'NULL'
        else:
            self.base = '&pylv_' + self.ancestor.name + '_Type'

    def get_structfields(self, recurse = False):
        actionfields = [f'PyObject *{action};' for action in self.get_std_actions()]
        myfields = actionfields + self.customstructfields
        
        if not myfields and not recurse:
            return []
        
        return (self.ancestor.get_structfields(True) if self.ancestor else []) + myfields


    def build_methodcode(self, method):
        object = self
        
        if isinstance(method, CustomMethod):
            return '' # Custom implementation is in lvglmodule_template.c
    
        
        startCode = f'''
static PyObject*
py{method.decl.name}(pylv_Obj *self, PyObject *args, PyObject *kwds)
{{
'''

        notImplementedCode = startCode + '    PyErr_SetString(PyExc_NotImplementedError, "not implemented");\n    return NULL;\n}\n'
        
        paramnames = []
        paramctypes = []
        paramfmt = ''
        for param in method.decl.type.args.params:
            paramtype = type_repr(param.type)
            # request_enum also registers the use of this enum with the bindingsgenerator
            if self.bindingsgenerator.request_enum(paramtype):
                fmt, ctype = 'i', 'int'
            else:
                try:
                    fmt, ctype = self.TYPECONV[paramtype]
                except KeyError:
                    # TODO: raise exception like micropython does
                    print(f'{method.decl.name}: Parameter type not found >{paramtype}< ')
                    paramtypes_miss.update((paramtype,))
                    return notImplementedCode;
            paramnames.append(param.name)
            paramctypes.append(ctype)
            paramfmt += fmt
        
        restype = type_repr(method.decl.type.type)
        
        if restype == 'void':
            resfmt, resctype = None, None
        else:
            if self.bindingsgenerator.request_enum(restype):
                resfmt, resctype = 'i', 'int'
            else:
                try:
                    resfmt, resctype = self.TYPECONV[restype]
                except KeyError:
                    print(f'{method.decl.name}: Return type not found >{restype}< ')
                    restypes_miss.update((restype,))
                    return notImplementedCode            
    
        # First argument should always be a reference to the object itself
        assert paramctypes and paramctypes[0] == 'pylv_Obj *'
        paramnames.pop(0)
        paramctypes.pop(0)
        paramfmt = paramfmt[2:]
        
        code = startCode
        kwlist = ''.join('"%s", ' % name for name in paramnames)
        code += f'    static char *kwlist[] = {{{kwlist}NULL}};\n';
        
        crefvarlist = ''
        cvarlist = ''
        for name, ctype in zip(paramnames, paramctypes):
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
        
        code += f'    if (!PyArg_ParseTupleAndKeywords(args, kwds, "{paramfmt}", kwlist {crefvarlist})) return NULL;\n'
        
        callcode = f'{method.decl.name}(self->ref{cvarlist})'
        
        if resctype == 'pylv_Obj *':
            # Result of function is an lv_obj; find or create the corresponding Python
            # object using pyobj_from_lv helper
            code += f'''
    LVGL_LOCK
    lv_obj_t *result = {callcode};
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
'''
    
        elif resctype == 'Style_Object *':
            code += f'    return Style_From_lv_style({callcode});\n'
        
        elif resctype is None:
            code += f'''
    LVGL_LOCK         
    {callcode};
    LVGL_UNLOCK
    Py_RETURN_NONE;
'''
        else:
            code += f'''
    LVGL_LOCK        
    {resctype} result = {callcode};
    LVGL_UNLOCK
'''
            if resfmt == 'p': # Py_BuildValue does not support 'p' (which is supported by PyArg_ParseTuple..)
                code += '    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}\n'
            else:
                code += f'    return Py_BuildValue("{resfmt}", result);\n'
   
            
        
        return code + '}\n';

    def build_actioncallbackcode(self, action, attrname):
        obj = self
        return f'''
lv_res_t pylv_{obj.name}_{action}_callback(lv_obj_t* obj) {{
    pylv_{obj.pyname} *pyobj;
    PyObject *handler;
    PyGILState_STATE gstate;

    gstate = PyGILState_Ensure();
    
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {{
        handler = pyobj->{attrname};
        if (handler) {{
            if (unlock) unlock(unlock_arg); 
            PyObject_CallFunctionObjArgs(handler, NULL);
            if (PyErr_Occurred()) PyErr_Print();
            
            PyGILState_Release(gstate);
            if (lock) lock(lock_arg); 
            return LV_RES_OK;

        }}

    }}
    PyGILState_Release(gstate);
    return LV_RES_OK;
}}
'''

    @property
    def pyname(self): # The name of the class in Python lv_obj --> Obj
        return self.name.title()   

    @property
    def structcode(self):
        structfields = self.get_structfields()   
        if structfields:
            structfieldscode = '\n    '.join(structfields)

            # Additional fields (or pylv_Obj, the root object struct)
            return f'typedef struct {{\n    {structfieldscode}\n}} pylv_{self.pyname};\n\n'
        else:
            # Same fields as ancestors, use typedef to generate new type name
            return f'typedef pylv_{self.ancestor.pyname} pylv_{self.pyname};\n\n'

    @property
    def methodscode(self):
        # Method definitions for the object methods (see also _methodcode function)
        code = ''
        actiongetset = set()
        for action in self.get_std_actions():
            actiongetset.add(self.lv_name + '_get_' + action)
            actiongetset.add(self.lv_name + '_set_' + action)
            code += self.build_actioncallbackcode(action, action)
            code += f'''

static PyObject *
pylv_{self.name}_get_{action}(pylv_{self.pyname} *self, PyObject *args, PyObject *kwds)
{{
    static char *kwlist[] = {{NULL}};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist)) return NULL;   
    
    PyObject *action = self->{action};
    if (!action) Py_RETURN_NONE;

    Py_INCREF(action);
    return action;
}}

static PyObject *
pylv_{self.name}_set_{action}(pylv_{self.pyname} *self, PyObject *args, PyObject *kwds)
{{
    static char *kwlist[] = {{"action", NULL}};
    PyObject *action, *tmp;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist , &action)) return NULL;
    
    tmp = self->{action};
    if (action == Py_None) {{
        self->{action} = NULL;
    }} else {{
        self->{action} = action;
        Py_INCREF(action);
        lv_{self.name}_set_{action}(self->ref, pylv_{self.name}_{action}_callback);
    }}
    Py_XDECREF(tmp); // Old action (tmp) could be NULL

    Py_RETURN_NONE;
}}

            
'''
            
        for method in self.methods.values():
            if method.decl.name not in actiongetset:
                code += self.build_methodcode(method)
                
        return code
    
    @property
    def methodtablecode(self):
        # Method table
        methodtablecode = ''
    
        for methodname, method in self.methods.items():
            
            methodtablecode += f'    {{"{methodname}", (PyCFunction) py{method.decl.name}, METH_VARARGS | METH_KEYWORDS, NULL}},\n'

    
        return methodtablecode

class PythonBindingsGenerator(BindingsGenerator):
    templatefile = 'lvglmodule_template.c'
    objectclass = PythonObject
    outputfile = 'lvglmodule.c'

    def customize(self):
        # TODO: remove these reordering construct (only required to show equality of the bindings generator)
        # All this reordering code is written such, that no items can accidentally be removed or added while reordering
        
        # reorder objects    
        for name in 'obj win label lmeter btnm chart cont led kb img bar arc line tabview mbox gauge page ta btn ddlist preload list slider sw cb roller'.split():
            self.objects[name] = self.objects.pop(name)
        
        
        
        objects = self.objects
        objects['btnm'].customstructfields.append('const char **map;')
        objects['obj'].customstructfields.extend(['PyObject_HEAD', 'lv_obj_t *ref;', 'PyObject *signal_func;', 'lv_signal_func_t orig_c_signal_func;'])
        objects['btn'].customstructfields.append(f'PyObject *actions[LV_BTN_ACTION_NUM];')


        for custom in ('lv_obj_get_children', 'lv_obj_get_signal_func', 'lv_obj_set_signal_func', 'lv_label_get_letter_pos', 'lv_label_get_letter_on', 'lv_btnm_set_map', 'lv_list_add', 'lv_btn_set_action', 'lv_btn_get_action','lv_obj_get_type', 'lv_list_focus'):
            
            obj, method = re.match('lv_([A-Za-z0-9]+)_(\w+)$', custom).groups()
            objects[obj].methods[method] = CustomMethod(custom)

        for function in skipfunctions:
            obj, method = re.match('lv_([A-Za-z0-9]+)_(\w+)$', function).groups()
            del objects[obj].methods[method]
    
        self.request_enum('lv_btn_action_t') # This enum is used in the custom implementation in Btn.set_action/get_action
    
    
    def deref_typedef(self, typestr):
        '''
        Given a type as string representation (e.g. lv_opa_t), recursively
        dereference it using the typedefs in self.parseresult
        '''
        while True:
            typedef = self.parseresult.typedefs.get(typestr)
            if not typedef or not isinstance(typedef.type.type, c_ast.IdentifierType):
                return typestr
            typestr = type_repr(typedef.type)
    
    def get_STYLE_GETSET(self):
        
        t_types = {
            'uint8_t:1': None,
            'uint8_t': 'uint8',
            'lv_color16_t': 'uint16',
            'int16_t': 'int16',
            'const lv_font_t *': None,
            
        
            }
        
        return ''.join(
            f'   {{"{name.replace(".", "_")}", (getter) Style_get_{t_types[self.deref_typedef(dtype)]}, (setter) Style_set_{t_types[self.deref_typedef(dtype)]}, "{name}", (void*)offsetof(lv_style_t, {name})}},\n' 
            for dtype, name in flatten_struct(self.parseresult.structs['lv_style_t'])
            if t_types[self.deref_typedef(dtype)] is not None
            )
    def get_BTN_CALLBACKS(self):
        # Button callback handlers
        ret = ''
        
        btncallbacknames = []
        for i, (name, value) in enumerate(list(self.parseresult.enums['lv_btn_action_t'].items())[:-1]): # last is LV_BTN_ACTION_NUM
            assert value == i
            actionname = 'action_' + stripstart(name, 'LV_BTN_ACTION_').lower()
            ret += self.objects['btn'].build_actioncallbackcode(actionname, f'actions[{i}]')
            btncallbacknames.append(f'pylv_btn_{actionname}_callback')
            
        ret += f'static lv_action_t pylv_btn_action_callbacks[LV_BTN_ACTION_NUM] = {{{", ".join(btncallbacknames)}}};'
        return ret
        
        
    def get_ENUM_ASSIGNMENTS(self):
        ret = ''
    
        for enumname, enum in self.parseresult.enums.items():
            if enumname in self.used_enums:
                for name, value in enum.items():
                    pyname = stripstart(name, 'LV_')
                    ret += f'    PyModule_AddIntConstant(module, "{pyname}", {value});\n'
        return ret
    def get_STYLE_ASSIGNMENTS(self):

        return ''.join(
            f'    PyModule_AddObject(module, "{stripstart(decl.declname, "lv_")}",Style_From_lv_style(&{decl.declname}));\n'
            for decl in self.parseresult.declarations.values()
            if ' '.join(decl.type.names) == 'lv_style_t'
            )

    def get_FONT_ASSIGNMENTS(self):
        return ''.join(
            f'    PyModule_AddObject(module, "{stripstart(decl.declname, "lv_")}", Font_From_lv_font(&{decl.declname}));\n'
            for decl in self.parseresult.declarations.values()
            if ' '.join(decl.type.names) == 'lv_font_t'
            )
    def get_SYMBOL_ASSIGNMENTS(self):
        return ''.join(
            f'    PyModule_AddStringConstant(module, "{name}", {value});\n'
            for name, value in self.parseresult.defines.items()
            if name.startswith('SYMBOL_')
            )


if __name__ == '__main__':
    import sourceparser
    try:
        parseresult
    except NameError:
        parseresult = sourceparser.LvglSourceParser().parse_sources('lvgl')
    
    g = PythonBindingsGenerator(parseresult)
    g.generate()
