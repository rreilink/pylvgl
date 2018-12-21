'''
Generate the Python bindings module for LittlevGL


This script requires Python >=3.6 for formatted string literals
'''


from collections import namedtuple, Counter
from itertools import chain
import re
import glob
import sys
import collections

assert sys.version_info > (3,6)

import sourceparser
c_ast = sourceparser.c_ast

def astnode_equals(a, b):
    '''
    Helper function to check if two ast nodes are equal (disrecarding their coord)
    '''
    
    # Must be of the same type
    if type(a) != type(b):
        return False
    
    # All atributes must be equal
    if a.attr_names != b.attr_names:
        return False
    
    for n in a.attr_names:
        if getattr(a, n) != getattr(b, n):
            return False
    
    # All children must be equal 
    children_a = a.children()
    children_b = b.children()
    if len(children_a) != len(children_b):
        return False
    
    for (name_a, child_a), (name_b, child_b) in zip(children_a, children_b):
        if name_a != name_b:
            return False
        if not astnode_equals(child_a, child_b):
            return False
    
    return True
    

def type_repr(x):
    ptrs = ''
    while isinstance(x, c_ast.PtrDecl):
        x = x.type
        ptrs += '*'
    if isinstance(x, c_ast.FuncDecl):
        return f'<function{ptrs}>'
    assert isinstance(x, c_ast.TypeDecl)
    return ' '.join(x.type.names) + ptrs 

def flatten_struct(s, prefix=''):
    '''
    Given a struct-of-structs c_ast.Struct object, yield pairs of
    (datatype, "parent.child.subchild")
    '''
    for d in s.decls:
        if isinstance(d.type.type, c_ast.Struct):
            yield from flatten_struct(d.type.type, prefix+d.name+'.')
        elif isinstance(d.type.type, c_ast.IdentifierType):
            bitsize = ':' + d.bitsize.value if d.bitsize else ''
            yield (' '.join(d.type.type.names)+bitsize, prefix+d.name)



def stripstart(s, start):
    assert s.startswith(start)
    return s[len(start):]


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

paramtypes_miss = Counter()
restypes_miss = Counter()


class CustomMethod(c_ast.FuncDef):
    '''
    For methods which have a custom implementation in the template
    Build an empty c_ast.FuncDef
    '''
    def __init__(self, name):
        super().__init__(c_ast.Decl(name, [], [], [], c_ast.FuncDecl(c_ast.ParamList([]), None, None),None,None,None), None, None)


class Object:
    '''
    Representation of an Lvgl object for which to generate bindings
    
    To be overridden by language-specific classes
    '''
    def __init__(self, obj, ancestor, enums):
        self.name = obj.name
        self.lv_name = 'lv_' + self.name
        self.ancestor = ancestor
        self.methods = obj.methods.copy()
        self.enums = enums # reference to enums found by BindingsGenerator.parse()
        self.customstructfields = []
        
        self.prune_derived_methods()
        
        self.init()
    
    def init(self):
        pass
    
    def reorder_methods(self, *swaps):
        '''
        Reorder the methods of this object. This is used purely to check that
        a new bindings generator yields the same output as a previous one
        '''
        # TODO:REMOVE
        for item, after in swaps:
            # Could be way more efficient by not creating new list/ordereddict every time, but it's for testing only anyway
            methods = list(self.methods.items())
            names = list(self.methods.keys())
            i1 = names.index(item)
            item = methods.pop(i1)
            self.methods = collections.OrderedDict(methods)
            
            methods = list(self.methods.items())
            names = list(self.methods.keys())
            i2 = 0 if after == 0 else names.index(after)+1
            methods.insert(i2, item)
            
        
            self.methods = collections.OrderedDict(methods)
    
    def prune_derived_methods(self):
        '''
        In lvgl source, some method are just a call to a superclass's method
        Since this is already covered by inheritance, we can prune those methods.
        
        We do this by looking at the Abstract Syntax Tree (ast) of each method.
        If it only contains a call to a method with the same name, on any of its
        ancestors, that method can be pruned.
        '''
        prunelist = []
        
        for methodname, method in self.methods.items():
            if not method.body.block_items or len(method.body.block_items) != 1:
                continue
            
            method_contents = method.body.block_items[0]
            
            # params = list of parameter names (list of c_ast.ID objects)
            params = [c_ast.ID(param.name) for param in method.decl.type.args.params]
            
            
            ancestor = self.ancestor
            while ancestor:
                function_name = f'lv_{ancestor.name}_{methodname}'
                
                # Define two options which should be pruned: either with or without return
                expect1 = c_ast.FuncCall(c_ast.ID(function_name),c_ast.ExprList(params) )
                expect2 = c_ast.Return(expect1)
                
                if astnode_equals(method_contents, expect1) or astnode_equals(method_contents, expect2):
                    prunelist.append(methodname)
                
                ancestor = ancestor.ancestor
            
        for item in prunelist:
            del self.methods[item]

    
    
    def __getitem__(self, name):
        '''
        Allow an Object instance to be used in format, giving access to the fields
        '''
        return getattr(self, name)

    def get_std_actions(self):
        actions = []
        setterstart = self.lv_name + '_set_'

        for method in self.methods.values():
            params = method.decl.type.args.params
            if len(params) == 2 and type_repr(params[1].type) == 'lv_action_t':
                actions.append(stripstart(method.decl.name, setterstart))
            
        
        return actions

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
            if paramtype in self.enums:
                fmt, ctype = 'i', 'int'
            else:
                try:
                    fmt, ctype = self.TYPECONV[paramtype]
                except KeyError:
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
            if restype in self.enums:
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


class BindingsGenerator:
    templatefile = None
    objectclass = None
    outputfile = None
    
    def parse(self):
        self.parseresult = sourceparser.LvglSourceParser().parse_sources('lvgl')
    
    def generate(self):
        
        self.objects = objects = collections.OrderedDict()
        for name, object in self.parseresult.objects.items():
            ancestor = objects[object.ancestor.name] if object.ancestor else None
            objects[name] = self.objectclass(object, ancestor, self.parseresult.enums)

        
        self.customize(objects)
        
        #
        # Fill in the template
        #
        with open(self.templatefile) as templatefile:
            template = templatefile.read()
        
        
        def substitute_per_object(match):
            '''
            Given a template, fill it in for each object and return the concatenated result
            '''
            
            template = match.group(1)
            ret = ''
            for obj in objects.values():
                ret += template.format_map(obj)
        
            return ret
        
        # Substitute per-object sections
        modulecode = re.sub(r'<<<(.*?)>>>', substitute_per_object, template, flags = re.DOTALL)
        
        # Substitute general fields
        modulecode = re.sub(r'<<(.*?)>>', lambda x: getattr(self, 'get_' + x.group(1))(), modulecode)


        with open(self.outputfile, 'w') as modulefile:
            modulefile.write(modulecode)

    def customize(self, objects):
        pass


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
    'lv_obj_report_style_mod',
    
}


# Remove functions that were not discovered in earlier bindings generator
# TODO:REMOVE
skipfunctions.update({
    'lv_obj_animate',
    'lv_win_get_title',
    'lv_label_get_text',
    'lv_btnm_get_map',
    'lv_img_get_file_name',
    'lv_mbox_get_text',
    'lv_ta_get_text',
    'lv_ddlist_get_options',
    'lv_ddlist_close',
    'lv_list_get_btn_text',
    'lv_cb_get_text',
})
        
        

class PythonBindingsGenerator(BindingsGenerator):
    templatefile = 'lvglmodule_template.c'
    objectclass = PythonObject
    outputfile = 'lvglmodule.c'

    def customize(self, objects):
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
    
        # Reorder methods to enable comparison of output with previous binding generator
        # TODO:REMOVE
        objects['lmeter'].reorder_methods(('get_style_bg', 'get_scale_angle'))
        objects['win'].reorder_methods(('set_sb_mode', 'set_btn_size'))
        objects['img'].reorder_methods(('set_src', 0), ('set_auto_size', 'set_file'), ('get_upscale', 'get_auto_size'))
        objects['line'].reorder_methods(('set_upscale', 'set_y_invert'), ('get_upscale', 'get_y_inv'))
        objects['gauge'].reorder_methods(('set_critical_value', 'set_value'), ('get_critical_value', 'get_needle_count'))
        objects['page'].reorder_methods(('get_scrl', 0), ('set_rel_action', 'get_scrl'), ('set_pr_action', 'set_rel_action'), ('set_sb_mode', 'set_pr_action'), ('set_style', 'set_scrl_layout'), ('get_sb_mode', 'set_style'))
    
        objects['sw'].reorder_methods(('get_state', 'set_style'))
        objects['cb'].reorder_methods(('set_text', 0), ('set_style', 'set_action'))
    
        # Inconsistency between lv_ddlist_open argument name (lvgl issue #660)
        # TODO:REMOVE
        objects['ddlist'].methods['open'].decl.type.args.params[1].name = 'anim'
    
    
    def get_STYLE_GETSET(self):
        
        t_types = {
            'uint8_t:1': None,
            'uint8_t': 'uint8',
            'lv_color_t': 'uint16',
            'lv_opa_t': 'uint8',
            'lv_coord_t': 'int16',
            'lv_border_part_t': None,
            'const lv_font_t *': None,
        
            }
        
        return ''.join(
            f'   {{"{name.replace(".", "_")}", (getter) Style_get_{t_types[dtype]}, (setter) Style_set_{t_types[dtype]}, "{name}", (void*)offsetof(lv_style_t, {name})}},\n' 
            for dtype, name in flatten_struct(self.parseresult.structs['lv_style_t'])
            if t_types[dtype] is not None
            )
    def get_BTN_CALLBACKS(self):
        # Button callback handlers
        ret = ''
        
        btncallbacknames = []
        for i, (name, value) in enumerate(list(self.parseresult.enums['lv_btn_action_t'].items())[:-1]): # last is LV_BNT_ACTION_NUM
            assert value == i
            actionname = 'action_' + stripstart(name, 'LV_BTN_ACTION_').lower()
            ret += self.objects['btn'].build_actioncallbackcode(actionname, f'actions[{i}]')
            btncallbacknames.append(f'pylv_btn_{actionname}_callback')
            
        ret += f'static lv_action_t pylv_btn_action_callbacks[LV_BTN_ACTION_NUM] = {{{", ".join(btncallbacknames)}}};'
        return ret
        
        
    def get_ENUM_ASSIGNMENTS(self):
        ret = ''
        
        #TODO:REMOVE this reordering is not required. Check enums which are not in this list
        order = ['lv_design_mode_t', 'lv_res_t', 'lv_signal_t', 'lv_protect_t', 
            'lv_align_t', 'lv_anim_builtin_t', 'lv_slider_style_t', 'lv_list_style_t', 'lv_kb_mode_t',
            'lv_kb_style_t', 'lv_bar_style_t', 'lv_tabview_style_t', 'lv_sw_style_t', 'lv_mbox_style_t',
            'lv_label_long_mode_t','lv_label_align_t', 'lv_cb_style_t', 'lv_sb_mode_t', 'lv_page_style_t',
            'lv_win_style_t', 'lv_btn_state_t', 'lv_btn_action_t', 'lv_btn_style_t', 'lv_cursor_type_t',
            'lv_ta_style_t', 'lv_roller_style_t', 'lv_chart_type_t', 'lv_btnm_style_t', 'lv_ddlist_style_t',
            'lv_layout_t']
        
        #print(set(self.parseresult.enums)-set(order))
        
        for enumname in order:
            enum = self.parseresult.enums[enumname]
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
##
g = PythonBindingsGenerator()
g.parse()
##
g.generate()



