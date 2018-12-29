#from pycparser import c_parser, c_ast, parse_file, CParser


#import pycparser.ply.lex as lex
#import pycparser.ply.cpp

import sys
sys.path.insert(0, '/Users/rob/Projecten/python/pycparser/')

import pycparser
import pycparser.ply.cpp
from pycparser import c_ast, c_generator

import copy
import os
import re
import glob
import collections

def type_repr(x):
    ptrs = ''
    while isinstance(x, c_ast.PtrDecl):
        x = x.type
        ptrs += '*'
    if isinstance(x, c_ast.FuncDecl):
        return f'<function{ptrs}>'
    assert isinstance(x, c_ast.TypeDecl)
    return ' '.join(x.type.names) + ptrs 

class CPP(pycparser.ply.cpp.Preprocessor):
    '''
    This extends the pycparser PLY-based included c preprocessor with 
    these modifications:
      - fix #if evaluation of != 
      - open included source files with utf-8 decoding
      - defined(x) returns "0" or "1" as opposed to "0L" or "1L" which are no
        valid Python 3 expressions
      - define endianness macros
      - skip inclusion of files in CPP.SKIPINCLUDES
    '''
    
    SKIPINCLUDES = ['stdbool.h', 'stdint.h', 'stddef.h', 'string.h', 'stdio.h']
    
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.define('__ORDER_LITTLE_ENDIAN__ 0')
        self.define('__ORDER_BIT_ENDIAN__ 1')
        self.define('__BYTE_ORDER__ 1')
    
    def read_include_file(self, filepath):
        if filepath in self.SKIPINCLUDES:
            return ''
        return super().read_include_file(filepath)

ParseResult = collections.namedtuple('ParseResult', 'enums functions declarations typedefs structs objects global_functions defines')

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

    def parse_files(self, filenames):
        
        # Parse with Python C-preprocessor
        input = ''
        for filename in filenames:
            with open(filename, 'r', encoding='utf-8') as file:
                input += file.read()
    
        cpp = CPP(self.lexer)
                
        cpp.add_path(os.path.dirname(filenames[0]))
        
        typedefs = ''.join(f'typedef {type} {name};\n' for name, type in self.TYPEDEFS.items())
        
        cpp.parse(typedefs + input,filename)
        
        preprocessed = ''
        
        while True:
            tok = cpp.token()
            if not tok: break
            #print(tok.value, end='')
            preprocessed += tok.value
    
        # All macros which are used in the C-source will have been expanded,
        # unused macros are not. Expand them all for consistency
        for macro in cpp.macros.values():
            cpp.expand_macros(macro.value)
        
        # Convert the macro tokens to a string
        defines = collections.OrderedDict([
            (name, ''.join(tok.value for tok in macro.value))
            for name, macro in cpp.macros.items()
        
            ])
    
        return pycparser.CParser().parse(preprocessed, filename), defines
    
    @staticmethod
    def enum_to_dict(enum_node):
        value = 0
        enum = collections.OrderedDict()
        for enumitem in enum_node.values.enumerators:
            if enumitem.value is not None:
                v = enumitem.value.value
                if v.lower().startswith('0x'):
                    value = int(v[2:], 16)
                else:
                    value = int(v)
            enum[enumitem.name] = value
            value += 1
        return enum
        
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
        
        filenames = [f for f in glob.glob(os.path.join(path, 'lv_objx/*.h')) if not f.endswith('lv_objx_templ.h')]

        ast, defines = self.parse_files(filenames)
        
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
                
                if isinstance(item.type.type, c_ast.Enum):
                    # Earlier versions of lvgl use typedef enum { ... } lv_enum_name_t
                    
                    
                    enums[item.name] = self.enum_to_dict(item.type.type)
                    
                elif isinstance(item.type, c_ast.TypeDecl) and isinstance(item.type.type, c_ast.Struct):
                    # typedef struct { ... } lv_struct_name_t;
                    structs[item.type.declname] = item.type.type
                    
                elif (isinstance(item.type, c_ast.TypeDecl) and isinstance(item.type.type, c_ast.IdentifierType) and 
                        isinstance(previous_item, c_ast.Decl) and isinstance(previous_item.type, c_ast.Enum)):
                    
                    # typedef lv_enum_t ...; directly after an enum definition
                    # newer lvgl uses this to define enum variables as uint8_t

                    enums[item.name] = self.enum_to_dict(previous_item.type)
                                            
                    
            elif isinstance(item, c_ast.Decl) and isinstance(item.type, c_ast.TypeDecl):
                declarations[item.type.declname] = item.type

            previous_item = item
    
        
        objects, global_functions = self.determine_objects(functions, typedefs)
        
        
        return ParseResult(enums, functions, declarations, typedefs, structs, objects, global_functions, defines)

    @staticmethod
    def determine_ancestor(create_function):
        return None
        # Get the name of the first argument to the create function
        firstargument = create_function.decl.type.args.params[0].name
        
        results = []
        
        for node in flatten_ast(create_function):
            if isinstance(node, c_ast.FuncCall) and isinstance(node.name, c_ast.ID):
                name = node.name.name
                match = re.match('lv_([A-Za-z0-9]+)_create$', name)
                
                # A _create function is called, with the same first argument as our _create
                if match and node.args:
                    arg = node.args.exprs[0]
                    if isinstance(arg, c_ast.ID) and arg.name == firstargument:
                        results.append(match.group(1))
            
        # assert at most 1 match; return None in case of no match
        assert len(results) <= 1
        
        return results[0] if results else None
    
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
            
            if name in objects and type_repr(function.decl.type.args.params[0].type) == 'lv_obj_t*':
                if method != 'create':
                    objects[name].methods[method] = function
            else:
                global_functions[function_name] = function
                
        return objects, global_functions

if __name__ == '__main__':
    result = LvglSourceParser().parse_sources('lvgl')



    print ('##################################')
    print ('#            OBJECTS             #')
    print ('##################################')
    for object in result.objects.values():
        print (f'{object.name}({object.ancestor.name if object.ancestor else ""})')
        for methodname, method in object.methods.items():
            args = ','.join(
                type_repr(param.type)+ ' ' + param.name 
                for param in method.decl.type.args.params)
            
            print(f'  {methodname}({args})')

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

