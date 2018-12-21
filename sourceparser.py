#from pycparser import c_parser, c_ast, parse_file, CParser


#import pycparser.ply.lex as lex
#import pycparser.ply.cpp
import pycparser
import pycparser.ply.cpp
from pycparser import c_ast
import copy
import os
import re
import glob
import collections

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
    
    def evalexpr(self,tokens):
        # tokens = tokenize(line)
        # Search for defined macros
        i = 0
        while i < len(tokens):
            if tokens[i].type == self.t_ID and tokens[i].value == 'defined':
                j = i + 1
                needparen = False
                result = "0L"
                while j < len(tokens):
                    if tokens[j].type in self.t_WS:
                        j += 1
                        continue
                    elif tokens[j].type == self.t_ID:
                        if tokens[j].value in self.macros:
                            result = "1"
                        else:
                            result = "0"
                        if not needparen: break
                    elif tokens[j].value == '(':
                        needparen = True
                    elif tokens[j].value == ')':
                        break
                    else:
                        self.error(self.source,tokens[i].lineno,"Malformed defined()")
                    j += 1
                tokens[i].type = self.t_INTEGER
                tokens[i].value = self.t_INTEGER_TYPE(result)
                del tokens[i+1:j+1]
            i += 1
        tokens = self.expand_macros(tokens)
        for i,t in enumerate(tokens):
            if t.type == self.t_ID:
                tokens[i] = copy.copy(t)
                tokens[i].type = self.t_INTEGER
                tokens[i].value = self.t_INTEGER_TYPE("0")
            elif t.type == self.t_INTEGER:
                tokens[i] = copy.copy(t)
                # Strip off any trailing suffixes
                tokens[i].value = str(tokens[i].value)
                while tokens[i].value[-1] not in "0123456789abcdefABCDEF":
                    tokens[i].value = tokens[i].value[:-1]

        expr = "".join([str(x.value) for x in tokens])
        expr = expr.replace("&&"," and ")
        expr = expr.replace("||"," or ")
        expr = expr.replace("!"," not ")
        expr = expr.replace("not =", "!=")
        try:
            result = eval(expr)
        except Exception:
            self.error(self.source,tokens[0].lineno,"Couldn't evaluate expression " + expr)
            result = 0
        return result
        
        
    def include(self,tokens):
        # Try to extract the filename and then process an include file
        if not tokens:
            return
        if tokens:
            if tokens[0].value != '<' and tokens[0].type != self.t_STRING:
                tokens = self.expand_macros(tokens)

            if tokens[0].value == '<':
                # Include <...>
                i = 1
                while i < len(tokens):
                    if tokens[i].value == '>':
                        break
                    i += 1
                else:
                    print("Malformed #include <...>")
                    return
                filename = "".join([x.value for x in tokens[1:i]])
                path = self.path + [""] + self.temp_path
            elif tokens[0].type == self.t_STRING:
                filename = tokens[0].value[1:-1]
                path = self.temp_path + [""] + self.path
            else:
                print("Malformed #include statement")
                return
        if filename in self.SKIPINCLUDES:
            return
            
        for p in path:
            iname = os.path.join(p,filename)
            try:
                data = open(iname,"r", encoding='utf-8').read()
                dname = os.path.dirname(iname)
                if dname:
                    self.temp_path.insert(0,dname)
                for tok in self.parsegen(data,filename):
                    yield tok
                if dname:
                    del self.temp_path[0]
                break
            except IOError:
                pass
        else:
            print("Couldn't find '%s'" % filename)

ParseResult = collections.namedtuple('ParseResult', 'enums functions declarations structs objects defines')

LvglObject = collections.namedtuple('LvglObject', 'name methods ancestor')

def flatten_ast(node):
    yield node
    for name, child in node.children():
        yield from flatten_ast(child)

class LvglSourceParser:
    SOURCE_PREPEND = \
'''
typedef int uint8_t;
typedef int int8_t;
typedef int uint16_t;
typedef int int16_t;
typedef int uint32_t;
typedef int int32_t;
typedef _Bool bool; 

'''

    def __init__(self):
        self.lexer = pycparser.ply.lex.lex(module = pycparser.ply.cpp)

    def parse_file(self, filename):
        
        # Parse with Python C-preprocessor
        with open(filename, 'r', encoding='utf-8') as file:
            input = file.read()
    
        cpp = CPP(self.lexer)
                
        cpp.add_path(os.path.dirname(filename))
        cpp.parse(self.SOURCE_PREPEND + input,filename)
        
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
        defines = {
            name: ''.join(tok.value for tok in macro.value)
            for name, macro in cpp.macros.items()
        
            }
    
        return pycparser.CParser().parse(preprocessed, filename), defines
        
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
        defines = collections.OrderedDict()
        declarations = collections.OrderedDict()
        structs = collections.OrderedDict()
        
        filenames = [f for f in glob.glob(os.path.join(path, 'lv_objx/*.c')) if not f.endswith('lv_objx_templ.c')]
        
        filenames.insert(0, os.path.join(path, 'lv_core/lv_obj.c'))
        
        for filename in filenames:
            print('Parsing', filename)
            ast, filedefines = self.parse_file(filename)
            
            defines.update(filedefines)
            
            for item in ast.ext:
                if isinstance(item, c_ast.FuncDef):
                    # C function
                    
                    # Skip static functions, but not static inline functions
                    if (not 'static' in item.decl.storage) or ('inline' in item.decl.funcspec):
                        functions[item.decl.name] = item

                        
                elif isinstance(item, c_ast.Typedef):
                    # C enum
                    if isinstance(item.type.type, c_ast.Enum):
                        value = 0
                        enum = collections.OrderedDict()
                        for enumitem in item.type.type.values.enumerators:
                            if enumitem.value is not None:
                                v = enumitem.value.value
                                if v.lower().startswith('0x'):
                                    value = int(v[2:], 16)
                                else:
                                    value = int(v)
                            enum[enumitem.name] = value
                            value += 1
                        
                        enums[item.name] = enum    
                    elif isinstance(item.type, c_ast.TypeDecl) and isinstance(item.type.type, c_ast.Struct):
                        structs[item.type.declname] = item.type.type
                elif isinstance(item, c_ast.Decl) and isinstance(item.type, c_ast.TypeDecl):
                    declarations[item.type.declname] = item.type

        objects = self.determine_objects(functions)
        
        return ParseResult(enums, functions, declarations, structs, objects, defines)

    @staticmethod
    def determine_ancestor(create_function):
        
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
    def determine_objects(cls, functions):
        # Stage 1: find all objects and the name of their ancestor
        objects = {}
        for function_name, function in functions.items():
            match =  re.match(r'lv_([a-zA-Z0-9]+)_create$', function_name)
            if match:
                name = match.group(1)
                objects[name] = cls.determine_ancestor(function)
    
        # Stage 2: sort objects, such that each object comes after its ancestors
        sortedobjects = collections.OrderedDict(obj = None)
        objects.pop('obj')
        while objects:
            remaining = list(objects.items())
            for obj, anch in remaining:
                if anch in sortedobjects:
                    sortedobjects[obj] = anch
                    objects.pop(obj)
        
        # Stage 3: Create LvglObject items
        objects = collections.OrderedDict()
        for name, ancestorname in sortedobjects.items():
            ancestor = None if ancestorname is None else objects[ancestorname]
            objects[name] = LvglObject(name, collections.OrderedDict(), ancestor)
    
        # Stage 4: Assign methods to objects
        for function_name, function in functions.items():
            match =  re.match(r'lv_([a-zA-Z0-9]+)_([a-zA-Z0-9_]+)$', function_name)
            if match:
                name, method = match.groups()
                if name in objects and method != 'create':
                    objects[name].methods[method] = function
        
        return objects

if __name__ == '__main__':
    result = LvglSourceParser().parse_sources('lvgl')

    def type_repr(x):
        ptrs = ''
        while isinstance(x, c_ast.PtrDecl):
            x = x.type
            ptrs += '*'
        if isinstance(x, c_ast.FuncDecl):
            return f'<function{ptrs}>'
        assert isinstance(x, c_ast.TypeDecl)
        return ' '.join(x.type.names) + ptrs 

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
    
