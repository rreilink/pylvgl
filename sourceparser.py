
import sys
sys.path.insert(0, './pycparser/')

import pycparser
import pycparser.ply.cpp
from pycparser import c_ast, c_generator

import copy
import os
import re
import collections

# TODO: this should go into a utils.py
def generate_c(node):
    return c_generator.CGenerator().visit(node)


def type_repr(arg, noqualifiers=False):
    ptrs = ''
    while isinstance(arg, c_ast.PtrDecl):
        ptrs += '*'
        arg = arg.type
    
    if noqualifiers:
        quals=''
    else:
        quals = ''.join('%s ' % qual for qual in sorted(arg.quals)) if hasattr(arg, 'quals') else ''
    
    return f'{quals}{generate_c(arg)}{ptrs}'

def stripstart(s, start):
    if not s.startswith(start):
        raise ValueError(f'{s!r} does not start with {start!r}')
    return s[len(start):]



ParseResult = collections.namedtuple('ParseResult', 'enums functions declarations typedefs structs objects global_functions style_functions defines')

LvglObject = collections.namedtuple('LvglObject', 'name methods ancestor')

def flatten_ast(node):
    yield node
    for name, child in node.children():
        yield from flatten_ast(child)

class LvglSourceParser:
    TYPEDEFS = {
        'uint8_t' : 'unsigned int',
        'int8_t' : 'int',
        'uint16_t' : 'unsigned int',
        'int16_t' :'int',
        'uint32_t' : 'unsigned int',
        'int32_t':  'int',
        'bool': '_Bool'
    }

    def __init__(self):
        self.lexer = pycparser.ply.lex.lex(module = pycparser.ply.cpp)

    def parse_file(self, filename):
        if os.name == 'nt':
            args = ['-Ipycparser/utils/fake_libc_include']
            cpp_path = r'C:\Program Files\LLVM\bin\clang.exe'
        else:
            args = ['-I./pycparser/utils/fake_libc_include']
            cpp_path = 'gcc'
        
        # TODO: preprocessor for Windows
        return pycparser.parse_file(filename, use_cpp=True, cpp_path=cpp_path, cpp_args=['-E'] + args)
        
    @staticmethod
    def enum_to_dict(enum_node):
        '''
        Given a c enum ast node, determine:
        - the name of the enum in python, using the common prefix of all enum items
        - a dictionary with as keys the names as they should be in Python, and as values the C enum constant names
        '''
        items = [enumitem.name for enumitem in enum_node.values.enumerators if not enumitem.name.startswith('_')]
        
        prefix = os.path.commonprefix(items)
        prefix = "_".join(prefix.split("_")[:-1])
        enumname = stripstart(prefix, 'LV_')
        
        return enumname, {stripstart(item, prefix + '_'): item for item in items}
        
    def parse_sources(self, path):
        '''
        Parse all lvgl sources which are required for generating the bindings
        
        returns: ParseResult (collections.namedtuple) with the following fields:
            enums: dict of enum name -> pycparser.c_ast.FuncDef object
            functions: dict of enum name -> value
            objects: dict of object name -> LvglObject object
            defines: dict of name -> string representation of evaluated #define
        
        '''
        enums = collections.OrderedDict()
        functions = collections.OrderedDict()

        declarations = collections.OrderedDict()
        structs = collections.OrderedDict()
        typedefs = collections.OrderedDict()

        ast = self.parse_file(os.path.join(path, 'lvgl.h'))
        
        previous_item = None
        # TODO: this whole filtering of items could be done in the bindings generator to allow for extending bindings generator without changing the sourceparser
        for item in ast.ext:
            if isinstance(item, c_ast.Decl) and isinstance(item.type, c_ast.FuncDecl):
                # C function
                if item.name not in functions:
                    # If it is already in there, it might be a FuncDef and we want to keep that one
                    functions[item.name] = c_ast.FuncDef(item, None, None)

            elif isinstance(item, c_ast.FuncDef):
                functions[item.decl.name] = item
                
                        
            elif isinstance(item, c_ast.Typedef) and not item.name in self.TYPEDEFS:
                # Do not register typedefs which are defined in self.TYPEDEFS, these are
                # to be treated as basic types by the bindings generators
                typedefs[item.name] = item
                
                if isinstance(item.type, c_ast.TypeDecl) and (isinstance(item.type.type, c_ast.Struct) or isinstance(item.type.type, c_ast.Union)):
                    # typedef struct { ... } lv_struct_name_t;
                    try:
                        structs[stripstart(item.type.declname,'lv_')] = item.type.type
                    except ValueError:  # If name does not start with lv_
                        pass
                    
                elif (isinstance(item.type, c_ast.TypeDecl) and isinstance(item.type.type, c_ast.IdentifierType) and 
                        isinstance(previous_item, c_ast.Decl) and isinstance(previous_item.type, c_ast.Enum)):
                    
                    # typedef lv_enum_t ...; directly after an enum definition
                    # newer lvgl uses this to define enum variables as uint8_t
                    enumname, enum = self.enum_to_dict(previous_item.type)
                    
                    enums[enumname] = enum
                                            
                    
            elif isinstance(item, c_ast.Decl) and isinstance(item.type, c_ast.TypeDecl):
                if not item.type.declname.startswith('lv_'):
                    print(item.type.declname)
                else:
                    declarations[stripstart(item.type.declname, 'lv_')] = item.type
                

            previous_item = item
    
        
        objects, global_functions = self.determine_objects(functions, typedefs)
        
        # Transfer the lv_style_xxx functions from global_functions to style_functions
        style_functions_list = [name for name in global_functions if name.startswith('lv_style_')]
        style_functions = collections.OrderedDict()
        for name in style_functions_list:
            style_functions[name] = global_functions.pop(name)
        
        
        
        # Find defines in color.h and symbol_def.h
        defines = collections.OrderedDict() # There is no OrderedSet in Python, so let's use OrderedDict with None values
        for filename in 'src/lv_misc/lv_color.h', 'src/lv_font/lv_symbol_def.h':
            with open(os.path.join(path, filename), 'rt', encoding='utf-8') as file:
                code = file.read()
                
                # TODO: proper way to filter out e.g. LV_COLOR_SET_R(...)
                # original regexp was '^\s*#define\s+(\w+)'
                for define in re.findall(r'^\s*#define\s+(\w+)\s', code,flags = re.MULTILINE):
                    if not define.startswith('_'):
                        defines[define] = None
        
        return ParseResult(enums, functions, declarations, typedefs, structs, objects, global_functions, style_functions, defines)

  
    @classmethod
    def determine_objects(cls, functions, typedefs):
        # Stage 1: find all objects and the name of their ancestor
        # use the typedefs of lv_xxx_ext_t to determine which objects exist,
        # and to determine their ancestors
        objects = {}
        lv_ext_pattern = re.compile('^lv_([^_]+)_ext_t')
        
        for typedef in typedefs.values():
            match = lv_ext_pattern.match(typedef.name)
            if match:
                try:
                    parent_ext_name = ' '.join(typedef.type.type.decls[0].type.type.names)
                except AttributeError:
                    parent_ext_name = ''

                parentmatch = lv_ext_pattern.match(parent_ext_name)
                if parentmatch:
                    objects[match.group(1)] = parentmatch.group(1)
                else:
                    objects[match.group(1)] = 'obj'
        
    
        # Stage 2: sort objects, such that each object comes after its ancestors
        sortedobjects = collections.OrderedDict(obj = None)

        while objects:
            remaining = list(objects.items())
            reordered = False
            for obj, anch in remaining:
                if anch in sortedobjects:
                    sortedobjects[obj] = anch
                    objects.pop(obj)
                    reordered = True
            
            if not reordered:
                raise RuntimeError('No ancestor defined for:', remaining)
        
        # Stage 3: Create LvglObject items
        objects = collections.OrderedDict()
        for name, ancestorname in sortedobjects.items():
            ancestor = None if ancestorname is None else objects[ancestorname]
            objects[name] = LvglObject(name, collections.OrderedDict(), ancestor)
    
        # Stage 4: Assign methods to objects; those who do not belong to an
        #    object go into global_functions
        global_functions = collections.OrderedDict()
        for function_name, function in functions.items():
            match =  re.match(r'lv_([a-zA-Z0-9]+)_([a-zA-Z0-9_]+)$', function_name)
            
            name, method = match.groups() if match else (None, None)
            
            if name in objects and type_repr(function.decl.type.args.params[0].type) in ('lv_obj_t*', 'const lv_obj_t*'):
                if method != 'create':
                    objects[name].methods[method] = function
            else:
                global_functions[function_name] = function
                
        return objects, global_functions

if __name__ == '__main__':
    result = LvglSourceParser().parse_sources('lvgl')


    def args_to_str(args):
        return ','.join(
                ('...' if isinstance(param, pycparser.c_ast.EllipsisParam) else
                type_repr(param.type)+ ' ' + param.name )
                for param in args.params) 

    print ('##################################')
    print ('#            OBJECTS             #')
    print ('##################################')
    for object in result.objects.values():
        print (f'{object.name}({object.ancestor.name if object.ancestor else ""})')
        for methodname, method in object.methods.items():
            print(f'  {methodname}({args_to_str(method.decl.type.args)})')

    print ('##################################')
    print ('#             ENUMS              #')
    print ('##################################')

    for name, value in result.enums.items():
        print (name, '=', value)

    print ('##################################')
    print ('#            DEFINES             #')
    print ('##################################')

    for name, value in result.defines.items():
        print (name, '=', value)

    print ('##################################')
    print ('#        STYLE FUNCTIONS         #')
    print ('##################################')

    for name, function in result.style_functions.items():
        print (f'{name}({args_to_str(function.decl.type.args)})')