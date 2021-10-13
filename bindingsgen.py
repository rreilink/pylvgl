'''
Generate the Python bindings module for LVGL


This script requires Python >=3.6 for formatted string literals
'''


from itertools import chain
import re
import glob
import sys
import collections
import copy

assert sys.version_info > (3,6)

from sourceparser import c_ast, c_generator, generate_c, type_repr, stripstart




def astnode_equals(a, b):
    '''
    Helper function to check if two ast nodes are equal (disrecarding their coord)
    '''
    #TODO: use cparser to string
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
    

class MissingConversionException(ValueError):
    pass

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


class CustomMethod(c_ast.FuncDef):
    '''
    For methods which have a custom implementation in the template
    Build an empty c_ast.FuncDef
    '''
    def __init__(self, name):
        super().__init__(c_ast.Decl(name, [], [], [], c_ast.FuncDecl(c_ast.ParamList([]), None, None),None,None,None), None, None)

class Struct:
    '''
    Representation of an Lvgl struct type for which to generate bindings
    
    To be overridden by language-specific classes

    '''
    def __init__(self, name, decls, bindingsgenerator):
        self.name = name
        self.decls = decls
        self.bindingsgenerator = bindingsgenerator

class Object:
    '''
    Representation of an Lvgl object for which to generate bindings
    
    To be overridden by language-specific classes
    '''
    def __init__(self, obj, ancestor, bindingsgenerator):
        self.name = obj.name
        self.lv_name = 'lv_' + self.name
        self.ancestor = ancestor
        self.methods = obj.methods.copy()
        self.bindingsgenerator = bindingsgenerator
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
            if not method.body or not method.body.block_items or len(method.body.block_items) != 1:
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


    def get_std_actions(self):
        actions = []
        setterstart = self.lv_name + '_set_'

        for method in self.methods.values():
            params = method.decl.type.args.params
            if len(params) == 2 and type_repr(params[1].type) == 'lv_action_t':
                actions.append(stripstart(method.decl.name, setterstart))
            
        
        return actions

class StyleSetter:
    '''
    Representation of an Lvgl style set function for which to generate bindings
    
    To be overridden by language-specific classes

    '''
    def __init__(self, name, decl, bindingsgenerator):
        self.name = name
        self.decl = decl
        self.bindingsgenerator = bindingsgenerator


class BindingsGenerator:
    templatefile = None
    objectclass = None
    structclass = None
    stylesetterclass = None
    outputfile = None
    sourcepath = 'lvgl'
    
    def __init__(self, parseresult):
        self.parseresult = copy.deepcopy(parseresult)
    
    def request_enum(self, name):
        result = self.parseresult.enums.get(name)
        if result is not None:
            self.used_enums[name] = result
        return result
    
    def generate(self):
        self.used_enums = collections.OrderedDict()
        self.request_enum('lv_protect_t') # The source code of lv_obj uses uint8 as argument for lv_obj_set_protect instead of lv_protect_t
        
        self.objects = objects = collections.OrderedDict()
        for name, object in self.parseresult.objects.items():
            ancestor = objects[object.ancestor.name] if object.ancestor else None
            objects[name] = self.objectclass(object, ancestor, self)

        self.structs = structs = collections.OrderedDict()
        for name, struct in self.parseresult.structs.items():
            structs[name] = self.structclass(name, struct.decls, self)
        
        self.stylesetters = stylesetters = collections.OrderedDict()
        for name, function in self.parseresult.style_functions.items():
            if name.startswith('lv_style_set_'):
                stylesetters[name] = self.stylesetterclass(name, function.decl, self)
        
        
        
        self.customize()
        
        #
        # Fill in the template
        #
        with open(self.templatefile) as templatefile:
            template = templatefile.read()
        
        class AttributeMapper:
            '''
            Helper class that allows using an object in str.format_map, 
            converting all item[value] to item.value access
            '''
            def __init__(self, item):
                self.item = item
            def __getitem__(self, name):
                return getattr(self.item, name)
            
        def substitute_per_item(match):
            '''
            Given a template, fill it in for each item and return the concatenated result
            what 'each item' is, is defined by the category, this should be the name of an
            attribute of the BindingsGenerator object            
            '''
            category = match.group(1)
            template = match.group(2)
            ret = ''
            for item in getattr(self, category).values():
                if not isinstance(item, collections.abc.Mapping):
                    item = AttributeMapper(item)
                    
                ret += template.format_map(item)
        
            return ret
        
        # Substitute per-item sections
        modulecode = re.sub(r'<<<(\w+):(.*?)>>>', substitute_per_item, template, flags = re.DOTALL)
        
        # Substitute general fields
        modulecode = re.sub(r'<<(.*?)>>', lambda x: getattr(self, 'get_' + x.group(1))(), modulecode)


        with open(self.outputfile, 'w') as modulefile:
            modulefile.write(modulecode)

    def customize(self):
        pass



