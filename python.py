import collections
import re
import keyword

from bindingsgen import Object, Struct, BindingsGenerator, c_ast, stripstart, generate_c, CustomMethod, type_repr, flatten_struct, MissingConversionException

# TODO: should be common for all bindings generators
skipfunctions = {
    
    # user_data is used to store reference to Python objects, don't tamper with that!
    'lv_obj_get_user_data',
    'lv_obj_set_user_data',
    'lv_obj_get_user_data_ptr',
    
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
        'int32_t':   ('I', 'int'),
        'int':       ('I', 'int'),
        }
    # TODO: from structs!

    TYPECONV.update({'const lv_style_t*':     ('O&', 'lv_style_t *')})

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
        myfields = self.customstructfields
        
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
    if (check_alive(self)) return NULL;
'''
       
        paramnames = []
        paramctypes = []
        paramfmts = []
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
            paramfmts.append(fmt)
        
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
        paramfmts.pop(0)
        
        code = startCode
        kwlist = ''.join('"%s", ' % name for name in paramnames)
        code += f'    static char *kwlist[] = {{{kwlist}NULL}};\n';
        
        crefvarlist = ''
        cvarlist = ''
        for name, ctype, fmt in zip(paramnames, paramctypes, paramfmts):
            code += f'    {ctype} {name};\n'
            if ctype == 'pylv_Obj *' : # Object, convert from Python
                crefvarlist += f', &pylv_obj_Type, &{name}'
                cvarlist += f', {name}->ref'
            elif fmt == 'O&': # struct
                crefvarlist += f', py{ctype.rstrip(" *")}_arg_converter, &{name}'
                cvarlist += f', {name}'
            else:
                crefvarlist += f', &{name}'
                cvarlist += f', {name}'
        
        code += f'    if (!PyArg_ParseTupleAndKeywords(args, kwds, "{"".join(paramfmts)}", kwlist {crefvarlist})) return NULL;\n'
        
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
        elif resfmt == 'O&':
            code += f'''
    LVGL_LOCK        
    {restype} result = {callcode};
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
'''
        else:
            code += f'''
    LVGL_LOCK        
    {restype} result = {callcode};
    LVGL_UNLOCK
'''
            if resfmt == 'p': # Py_BuildValue does not support 'p' (which is supported by PyArg_ParseTuple..)
                code += '    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}\n'
            else:
                code += f'    return Py_BuildValue("{resfmt}", result);\n'
   
            
        
        return code + '}\n';


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
            
        for method in self.methods.values():
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
            alias = methodname
            if keyword.iskeyword(alias):
                alias += '_'
            methodtablecode += f'    {{"{alias}", (PyCFunction) py{method.decl.name}, METH_VARARGS | METH_KEYWORDS, "{generate_c(method.decl)}"}},\n'

    
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
    
    def __init__(self, *args, basename=None, subpath='', **kwargs):
        super().__init__(*args, **kwargs)
        
        # in case of sub structs, basename refers to the name of the original c type
        # and subpath refers to the path from the original c type to this struct
        # e.g. name = 'style_t_body_border', basename='style_t', subpath='body.border.'
        self.basename = basename or self.name
        self.subpath = subpath
    
    def get_flattened_decls(self):
        '''
        In case of a union { struct { ... } }, that struct can have no name,
        and its members are to be regarded as members of the union for the
        bindings. This generator function yields the 'normal' self.decls,
        combined with the decls of inner structs that are unnamed.
        '''
        for decl in self.decls:
            if decl.name is None:
                yield from decl.type.decls
            else:
                yield decl
    
    def get_substructs(self):
        '''
        yield the PythonStruct's for all members of this struct that are again structs.
        
        This is done recursively
        '''
        for decl in self.get_flattened_decls():
            if isinstance(decl.type.type, (c_ast.Struct, c_ast.Union)):
                substruct = PythonStruct(self.name + '_' + decl.name, decl.type.type, self.bindingsgenerator, basename=self.basename, subpath=self.subpath + decl.name + '.')
                yield substruct
                yield from substruct.get_substructs()

    @property
    def getset(self):
   
        
        code = f'static PyGetSetDef pylv_{self.name}_getset[] = {{\n'
        bitfieldscode = ''
        

        for decl in self.get_flattened_decls():
            assert decl.name
            if self.subpath:
                offsetcode = f'(offsetof(lv_{self.basename}, {self.subpath}{decl.name})-offsetof(lv_{self.basename}, {self.subpath[:-1]}))'
            else:
                offsetcode = f'offsetof(lv_{self.basename}, {decl.name})'
            
 
            
            if isinstance(decl.type.type, (c_ast.Struct, c_ast.Union)):
                getter, setter = 'struct_get_struct', 'struct_set_struct'
                # the closure for struct & blob is a struct of PyTypObject*, offset (w.r.t. base type), size (in bytes)
                closure = f'& ((struct_closure_t){{ &pylv_{self.name}_{decl.name}_Type, {offsetcode}, sizeof(((lv_{self.basename} *)0)->{self.subpath}{decl.name})}})'
            elif decl.bitsize is not None:
                # Needs a custom getter & setter function
                getsetname = f'struct_bitfield_{self.name}_{decl.name}'
                getter, setter = 'get_' + getsetname, 'set_' + getsetname
                closure = 'NULL'
                assert(self.bindingsgenerator.deref_typedef(generate_c(decl.type.type)) in ('uint8_t', 'uint16_t', 'uint32_t')) # the following only supports unsigned values for bitfields
                bitfieldscode += f'''

static PyObject *
{getter}(StructObject *self, void *closure)
{{
    return PyLong_FromLong(((lv_{self.basename}*)(self->data))->{self.subpath}{decl.name} );
}}

static int
{setter}(StructObject *self, PyObject *value, void *closure)
{{
    long v;
    if (long_to_int(value, &v, 0, {2**int(decl.bitsize.value)-1})) return -1;
    ((lv_{self.basename}*)(self->data))->{self.subpath}{decl.name} = v;
    return 0;
}}

'''
                    
            else:
                typestr = self.bindingsgenerator.deref_typedef(type_repr(decl.type))

                if typestr in self.TYPES:
                    getter, setter = self.TYPES[typestr]
                    closure = f'(void*){offsetcode}'
                elif typestr.startswith('lv_') and (stripstart(typestr,'lv_') in self.bindingsgenerator.structs):
                    getter, setter = 'struct_get_struct', 'struct_set_struct'
                    closure = f'& ((struct_closure_t){{ &py{typestr}_Type, {offsetcode}, sizeof({typestr})}})'
                else:
                    # default: blob type
                    getter, setter = 'struct_get_struct', 'struct_set_struct'
                    closure = f'& ((struct_closure_t){{ &Blob_Type, {offsetcode}, sizeof(((lv_{self.basename} *)0)->{self.subpath}{decl.name})}})'

            
            typedoc = generate_c(decl.type).replace('\n', ' ') + (f':{decl.bitsize.value}' if decl.bitsize else '')
            code += f'    {{"{decl.name}", (getter) {getter}, (setter) {setter}, "{typedoc} {decl.name}", {closure}}},\n'
        
        code += '    {NULL}\n};\n'

        
        return bitfieldscode + code


class PythonBindingsGenerator(BindingsGenerator):
    templatefile = 'lvglmodule_template.c'
    objectclass = PythonObject
    structclass = PythonStruct
    outputfile = 'lvglmodule.c'

    def customize(self):
        # Create self.substructs , which is a collection of derived structs (i.e. structs within structs like lv_style_t_body_border)
        self.substructs = collections.OrderedDict()
        for struct in self.structs.values():
            self.substructs.update((sub.name, sub) for sub in struct.get_substructs())
            
        # self.allstructs = self.structs + self.substructs
        self.allstructs = self.structs.copy()
        self.allstructs.update(self.substructs)
        
        
        objects = self.objects
        objects['obj'].customstructfields.extend(['PyObject_HEAD', 'PyObject *weakreflist;', 'lv_obj_t *ref;', 'PyObject *event_cb;', 'lv_signal_cb_t orig_signal_cb;'])

        for custom in ('lv_obj_get_children', 'lv_obj_set_event_cb', 'lv_label_get_letter_pos', 'lv_label_get_letter_on', 'lv_list_add' ,'lv_obj_get_type', 'lv_list_focus'):
            
            obj, method = re.match('lv_([A-Za-z0-9]+)_(\w+)$', custom).groups()
            objects[obj].methods[method] = CustomMethod(custom)

        for function in skipfunctions:
            obj, method = re.match('lv_([A-Za-z0-9]+)_(\w+)$', function).groups()
            del objects[obj].methods[method]
        
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
            if typedef:
                if isinstance(typedef.type.type, c_ast.IdentifierType):
                    typestr = type_repr(typedef.type)
                    continue # Continue dereferencing
                if isinstance(typedef.type.type, c_ast.Enum):
                    return 'int' # Enum is represented by an int
            
            return typestr
    
           
    def get_ENUM_ASSIGNMENTS(self):
        ret = ''    
        for enumname, enum in self.parseresult.enums.items():

            items = ''.join(f', "{name}", {value}' for name, value in enum.items())
            ret += f'    PyModule_AddObject(module, "{enumname}", build_constclass(\'d\', "{enumname}"{items}, NULL));\n'
        return ret

    def get_SYMBOL_ASSIGNMENTS(self):
        
        skip = {'LV_SYMBOL_DEF_H', 'LV_SYMBOL_GLYPH_FIRST', 'LV_SYMBOL_GLYPH_LAST'}
        
        items = ''.join(f', "{name[10:]}", {name}' for name in self.parseresult.defines if name.startswith('LV_SYMBOL_') and name not in skip)
        return f'    PyModule_AddObject(module, "SYMBOL", build_constclass(\'s\', "SYMBOL"{items}, NULL));\n'

    def get_COLOR_ASSIGNMENTS(self):
        
        skip = {'LV_COLOR_H', 'LV_COLOR_MAKE'}
        
        items = ''.join(f', "{name[9:]}", {name}' for name in self.parseresult.defines if name.startswith('LV_COLOR_') and name not in skip)
        return f'    PyModule_AddObject(module, "COLOR", build_constclass(\'C\', "COLOR"{items}, NULL));\n'
    
    def get_LV_COLOR_TYPE(self):
        return 'py' + self.deref_typedef('lv_color_t') + '_Type'
    
    def get_GLOBALS_ASSIGNMENTS(self):
        code = ''
        for name, type in self.parseresult.declarations.items():
            typename = type_repr(type)
            code += f'   PyModule_AddObject(module, "{name}", pystruct_from_c(&py{typename}_Type, &{type.declname}, sizeof({typename}), 0));\n'
            
        return code

if __name__ == '__main__':
    import sourceparser
    #try:
    #    parseresult
    #except NameError:
    parseresult = sourceparser.LvglSourceParser().parse_sources('lvgl')
    
    g = PythonBindingsGenerator(parseresult)
    g.generate()
