'''
This script checks whether all function definitions match with their declarations

It also checks whether all defined functions have been declared before use (except
for static functions), and whether all functions which are declared, are defined
somewhere.
'''

from sourceparser import LvglSourceParser, pycparser, c_ast
from bindingsgen import generate_c, astnode_equals
import glob


s = LvglSourceParser()

alldecls = {}
alldefs = {}
for filename in glob.glob('lvgl/**', recursive=True):
    
    if not filename.endswith('.c') or filename.startswith('lvgl/micropython'):
        continue
    
    ast, defines = s.parse_files([filename])
    
    decls = {}
    print(filename)
    
    for item in ast.ext:
        d = None
        if isinstance(item, c_ast.FuncDef):
            d = item.decl
            alldefs[d.name] = d
            
            if d.name not in decls and 'static' not in d.storage:
                # All non-static functions should be declared
                print('Not previously declared:', generate_c(d))
        elif isinstance(item, c_ast.Decl) and isinstance(item.type, c_ast.FuncDecl):
            d = item
            alldecls[d.name] = d
        
        if d:
            name = d.name
            known = decls.get(name)
            if known:
                if not astnode_equals(d, known):
                    print('  ', generate_c(known))
                    print('  ', generate_c(d))
            else:
                decls[name] = d

print('The following declared functions are not defined:')
for name, decl in alldecls.items():
    if not name in alldefs:
        print (generate_c(decl))
        

