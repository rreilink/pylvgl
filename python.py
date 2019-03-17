import collections
import re

from bindingsgen import Object, Struct, BindingsGenerator, c_ast, stripstart, generate_c, CustomMethod, type_repr, flatten_struct, MissingConversionException

# TODO: should be common for all bindings generators
skipfunctions = {

    # Don't tamper with deleting objects which have Python references
    'lv_obj_del',
    'lv_obj_clean',
    
    # user_data is used to store reference to Python objects, don't tamper with that!
    'lv_obj_get_user_data',
    
    # Just use Python attributes for custom properties of objects
    'lv_obj_allocate_ext_attr',
    'lv_obj_get_ext_attr',
    
    # Do not work since they require lv_obj_t == NULL which is not implemented
    # lv_obj_getchildren is implemented instead which returns a list
    'lv_obj_get_child',
    'lv_obj_get_child_back',
    
}


class PythonObject(Object):
        
    TYPECONV = {
        'lv_obj_t*': ('O!', 'pylv_Obj *'),     # special case: conversion from/to Python object
        'bool':      ('p', 'int'),
        'char':      ('c', 'char'),
        'uint8_t':   ('b', 'unsigned char'),
        'const char*':     ('s', 'const char *'),
        'uint16_t':  ('H', 'unsigned short int'), 
        'int16_t':   ('h', 'short int'),
        'uint32_t':  ('I', 'unsigned int'),
        'int32_t':  ('I', 'int'),
        }
    
    TYPECONV_PARAMETER = TYPECONV.copy()
    TYPECONV_PARAMETER.update({'const lv_obj_t*': ('O!', 'pylv_Obj *')})
    
    TYPECONV_RETURN = TYPECONV.copy()
    TYPECONV_RETURN.update({'char*':     ('s', 'char *')})
    

    
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
       
        paramnames = []
        paramctypes = []
        paramfmt = ''
        for param in method.decl.type.args.params:
            paramtype = type_repr(param.type)
            paramtype_derefed = self.bindingsgenerator.deref_typedef(paramtype)
            
            # For params, if the param type is e.g. "lv_obj_t*", "const lv_obj_t*" is allowed, too
            # However, we do not add "const lv_obj_t*" to the TYPECONV dict, since "const lv_obj_t*" would not be a valid return type
            
            try:
                fmt, ctype = self.TYPECONV_PARAMETER[paramtype_derefed]
            except KeyError:
                raise MissingConversionException(f'{method.decl.name}: Parameter type not found >{paramtype}< ')
            
            paramnames.append(param.name)
            paramctypes.append(ctype)
            paramfmt += fmt
        
        restype = type_repr(method.decl.type.type)
        
        if restype == 'void':
            resfmt, resctype = None, None
        else:
            try:
                resfmt, resctype = self.TYPECONV_RETURN[self.bindingsgenerator.deref_typedef(restype)]
            except KeyError:
                raise MissingConversionException(f'{method.decl.name}: Return type not found >{restype}< ')
    
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
                try:
                    code += self.build_methodcode(method)
                except MissingConversionException as e:
                    print(e)
                    code += f'''
static PyObject*
py{method.decl.name}(pylv_Obj *self, PyObject *args, PyObject *kwds)
{{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: {e}");
    return NULL;
}}
'''

        return code
    
    @property
    def methodtablecode(self):
        # Method table
        methodtablecode = ''
    
        for methodname, method in self.methods.items():
            
            methodtablecode += f'    {{"{methodname}", (PyCFunction) py{method.decl.name}, METH_VARARGS | METH_KEYWORDS, "{generate_c(method.decl)}"}},\n'

    
        return methodtablecode


class PythonStruct(Struct):
    TYPES = {
        'int8_t': ('struct_get_int8', 'struct_set_int8'),
        'uint8_t': ('struct_get_uint8', 'struct_set_uint8'),

        'int16_t': ('struct_get_int16', 'struct_set_int16'),
        'uint16_t': ('struct_get_uint16', 'struct_set_uint16'),

        'int32_t': ('struct_get_int32', 'struct_set_int32'),
        'uint32_t': ('struct_get_uint32', 'struct_set_uint32'),

    
    }


    @property
    def getset(self):
        code = ''
        for decl in self.decls:
            typestr = self.bindingsgenerator.deref_typedef(type_repr(decl.type))
            if decl.bitsize is None:
                try:
                    getter, setter = self.TYPES[typestr]
                except KeyError:
                    print(typestr)
                    getter, setter = 'struct_get_blob', 'struct_set_blob'
                    
                closure = f'(void*)offsetof(lv_{self.name}, {decl.name})'
                
            else: # bit fields unsupported so far
                getter, setter, closure = 'NULL', 'NULL', 'NULL'
            
            typedoc = generate_c(decl.type).replace('\n', ' ')
            code += f'    {{"{decl.name}", (getter) {getter}, (setter) {setter}, "{typedoc} {decl.name}", {closure}}},\n'
            
        return code
#
#    {"y", (getter) struct_get_int16, (setter) struct_set_int16, "y", (void*)offsetof(lv_point_t, y)},


class PythonBindingsGenerator(BindingsGenerator):
    templatefile = 'lvglmodule_template.c'
    objectclass = PythonObject
    structclass = PythonStruct
    outputfile = 'lvglmodule.c'

    def customize(self):
        
        
        objects = self.objects
        objects['obj'].customstructfields.extend(['PyObject_HEAD', 'lv_obj_t *ref;', 'PyObject *event;', 'lv_event_cb_t orig_c_event_cb;'])

        for custom in ('lv_obj_get_children', 'lv_label_get_letter_pos', 'lv_label_get_letter_on', 'lv_list_add' ,'lv_obj_get_type', 'lv_list_focus'):
            
            obj, method = re.match('lv_([A-Za-z0-9]+)_(\w+)$', custom).groups()
            objects[obj].methods[method] = CustomMethod(custom)

        for function in skipfunctions:
            obj, method = re.match('lv_([A-Za-z0-9]+)_(\w+)$', function).groups()
            del objects[obj].methods[method]
    
        self.request_enum('lv_btn_action_t') # This enum is used in the custom implementation in Btn.set_action/get_action
    
    @property
    def struct_inttypes(self):
        
        types = [{'type': f'uint{x}','min' : 0, 'max': (1<<x)-1} for x in (8,16,32)] + \
            [{'type': f'int{x}','min' : -(1<<(x-1)), 'max': (1<<(x-1))-1} for x in (8,16,32)]
            
        return {type['type']: type for type in types}
    
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
    
           
    def get_ENUM_ASSIGNMENTS(self):
        ret = ''    
        for enumname, enum in self.parseresult.enums.items():

            items = ''.join(f', "{name}", {value}' for name, value in enum.items())
            ret += f'    PyModule_AddObject(module, "{enumname}", build_enum("{enumname}"{items}, NULL));\n'
        return ret

    def get_SYMBOL_ASSIGNMENTS(self):
        return ''.join(
            f'    PyModule_AddStringMacro(module, {name});\n'
            for name in self.parseresult.defines
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
