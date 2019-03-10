from bindingsgen import Object, BindingsGenerator, c_ast, stripstart, generate_c
import re
import collections

def get_arg_type(arg): # TODO: merge with type_repr
    indirect_level = 0
    while isinstance(arg, c_ast.PtrDecl):
        indirect_level += 1
        arg = arg.type
    return '{quals}{type}{indirection}'.format(
        quals=''.join('%s ' % qual for qual in arg.quals) if hasattr(arg, 'quals') else '',
        type=generate_c(arg),
        indirection='*' * indirect_level)


def get_arg_name(arg):
    if isinstance(arg, c_ast.PtrDecl) or isinstance(arg, c_ast.FuncDecl):
        return get_arg_name(arg.type)
    return arg.declname if hasattr(arg, 'declname') else arg.name

mp_to_lv = {
    'bool'                      : 'mp_obj_is_true',
    'char*'                     : '(char*)mp_obj_str_get_str',
    'const char*'               : 'mp_obj_str_get_str',
    'lv_obj_t*'                 : 'mp_to_lv',
    'const lv_obj_t*'           : 'mp_to_lv',
    'struct _lv_obj_t*'         : 'mp_to_lv',
    'const struct _lv_obj_t*'   : 'mp_to_lv',
    'uint8_t'                   : '(uint8_t)mp_obj_int_get_checked',
    'uint16_t'                  : '(uint16_t)mp_obj_int_get_checked',
    'uint32_t'                  : '(uint32_t)mp_obj_int_get_checked',
    'int8_t'                    : '(int8_t)mp_obj_int_get_checked',
    'int16_t'                   : '(int16_t)mp_obj_int_get_checked',
    'int32_t'                   : '(int32_t)mp_obj_int_get_checked',
#    'lv_action_t'               : handle_lv_action
    # TODO: these could come from typedefs
    'lv_coord_t'                : '(int16_t)mp_obj_int_get_checked',
    'lv_align_t'                : '(uint8_t)mp_obj_int_get_checked',
    'lv_opa_t'                  : '(uint8_t)mp_obj_int_get_checked',
    

}

lv_to_mp = {
    'bool'                      : 'convert_to_bool',
    'char*'                     : 'convert_to_str',
    'const char*'               : 'convert_to_str',
    'lv_obj_t*'                 : 'lv_to_mp',
    'const lv_obj_t*'           : 'lv_to_mp',
    'struct _lv_obj_t*'         : 'lv_to_mp',
    'const struct _lv_obj_t*'   : 'lv_to_mp',
    'uint8_t'                   : 'mp_obj_new_int_from_uint',
    'uint16_t'                  : 'mp_obj_new_int_from_uint',
    'uint32_t'                  : 'mp_obj_new_int_from_uint',
    'int8_t'                    : 'mp_obj_new_int',
    'int16_t'                   : 'mp_obj_new_int',
    'int32_t'                   : 'mp_obj_new_int',
    # TODO: these could come from typedefs
    'lv_res_t'                  : 'mp_obj_new_int_from_uint',
    'lv_coord_t'                : 'mp_obj_new_int',
    'lv_opa_t'                  : 'mp_obj_new_int_from_uint',
    

}

class MissingConversionException(ValueError):
    pass



def gen_func_error(method, exp):
    return("""
/*
 * Function NOT generated:
 * {problem}
 * {method}
 */
    
""".format(method=generate_c(method), problem=exp))




class MicroPythonObject(Object):
    def init(self):
        self.enums = collections.OrderedDict()
    def prune_derived_methods(self):
        pass # Do not prune derived methods
    
    
    def reorder_defs_decls(self):
        defs = []
        decls = []
        for name, method in self.methods.items():
            if method.body is None:
                decls.append((name, method))
            else:
                defs.append((name, method))
        
        self.methods.clear()
        self.methods.update(defs)
        self.methods.update(decls)
        
    
    @property
    def ancestorname(self):
        return self.ancestor.name if self.ancestor else 'None'
        
    @property
    def methodscode(self):
        code = ''
        prune = []
        for methodname, method in self.methods.items():
            try:
                code += self.bindingsgenerator.gen_mp_func(method.decl, self.name)
            except MissingConversionException as exp:
                code += gen_func_error(method.decl, exp)
                prune.append(methodname)
        
        for methodname in prune:
            self.methods.pop(methodname)
        return code
        
    @property
    def methodtablecode(self):
        # Method table
        code = ''
    
        for methodname, method in self.methods.items():
            
            code += f'    {{ MP_OBJ_NEW_QSTR(MP_QSTR_{methodname}), MP_ROM_PTR(&mp_{method.decl.name}_obj) }},\n'

        if self.ancestor:
            code += self.ancestor.methodtablecode + ',\n'
        
        # TODO: use enum.name / shortname for consistency
        for enumname, enum in self.enums.items():
            long, short = re.match('([A-Za-z0-9]+_(\w+))', enumname).groups() # todo: this should be in the bindings generator
            code += f'    {{ MP_OBJ_NEW_QSTR(MP_QSTR_{short}), MP_ROM_PTR(&mp_LV_{long}_type) }},\n'
        
        return code[:-2] # skip last ,\n


class MicroPythonEnum:
    def __init__(self, name, enum):
        self.name = name
        self.enum = enum

    @property
    def locals_dict_table(self):
        code = ''
    
        for itemname, value in self.enum.items():
            code += f'    {{ MP_OBJ_NEW_QSTR(MP_QSTR_{itemname}), MP_ROM_PTR(MP_ROM_INT({value})) }},\n'
            
        return code[:-2] # skip last ,\n
    @property
    def shortname(self):
        return stripstart(self.name, 'LV_')




class MicroPythonCallback:
    lv_callback_return_type_pattern = re.compile('^((void)|(lv_res_t))')
    lv_base_obj_pattern = re.compile('^(struct _){0,1}lv_obj_t')
    
    def __init__(self, typedef, bindingsgenerator):
        self.typedef = typedef
        self.name = typedef.name
        self.bindingsgenerator = bindingsgenerator
        
    @property
    def callback_code(self):
        try:
            return self.build_callbackcode()
        except MissingConversionException as exp:
            return gen_func_error(self.typedef.type.type, exp)

    def build_callback_func_arg(self, arg, index, func):
        arg_type = get_arg_type(arg.type)
        if not arg_type in lv_to_mp:
            self.bindingsgenerator.try_generate_type(arg_type)
            if not arg_type in lv_to_mp:
                raise MissingConversionException("Callback: Missing conversion to %s" % arg_type)
        return lv_to_mp[arg_type](arg, index, func, obj_name) if callable(lv_to_mp[arg_type]) else \
            'args[{i}] = {convertor}(arg{i});'.format(
                convertor = lv_to_mp[arg_type],
                i = index) 
    
    def build_callbackcode(self):
        func = self.typedef.type.type
        args = func.args.params
        if len(args) < 1 or hasattr(args[0].type.type, 'names') and self.lv_base_obj_pattern.match(args[0].type.type.names[0]):
            raise MissingConversionException("Callback: First argument of callback function must be lv_obj_t")
        func_name = get_arg_name(func.type)
        return_type = get_arg_type(func.type)
        if not self.lv_callback_return_type_pattern.match(return_type):
            raise MissingConversionException("Callback: Can only handle callbaks that return lv_res_t or void")
            

        code = ("""
/*
 * Callback function {func_name}
 * {func_prototype}
 */

STATIC {return_type} {func_name}_callback({func_args})
{{
    mp_obj_t args[{num_args}];
    {build_args}
    mp_obj_t action = mp_to_lv_action(args[0]);
    mp_obj_t arg_list = mp_obj_new_list({num_args}, args);
    bool schedule_result = mp_sched_schedule(action, arg_list);
    return{return_value};
}}

""".format(
            func_prototype = generate_c(func),
            func_name = func_name,
            return_type = return_type,
            func_args = ', '.join(["%s arg%s" % (get_arg_type(arg.type), i) for i,arg in enumerate(args)]),
            num_args=len(args),
            build_args="\n    ".join([self.build_callback_func_arg(arg, i, func) for i,arg in enumerate(args)]),
            return_value='' if return_type=='void' else ' schedule_result? LV_RES_OK: LV_RES_INV'))

        # Do this after the previous statement, since that may raise a MissingConversionException,
        # in which case we should *not* register the callback
        def register_callback(arg, index, func, obj_name):
            return """set_action(args[0], args[{i}]);
    {arg} = &{func_name}_callback;""".format(
                i = index,
                arg = generate_c(arg),
                func_name = func_name)
        
        mp_to_lv[func_name] = register_callback
        return code

class MicroPythonGlobal:
    def __init__(self, function, bindingsgenerator):
        self.bindingsgenerator = bindingsgenerator
        self.name = function.decl.name
        self.shortname = stripstart(self.name, 'lv_')
        self.function = function
        self.code_generated = None

    @property
    def code(self):
        try:
            self.code_generated = True
            return self.bindingsgenerator.gen_mp_func(self.function.decl, None)
        except MissingConversionException as exp:
            self.code_generated = False
            return gen_func_error(self.function.decl, exp)

        
class MicroPythonBindingsGenerator(BindingsGenerator):
    templatefile = 'lv_mpy_template.c'
    objectclass = MicroPythonObject
    outputfile = 'lv_mpy.c'
    sourcepath = 'lvgl'


    def try_generate_type(self, type):
        #TODO: mp_to_lv and lv_to_mp not global, but class or instance members
        
        if type in mp_to_lv:
            return True

        orig_type = type
        while type in self.parseresult.typedefs:
            type = get_arg_type(self.parseresult.typedefs[type].type.type)
        
        # todo: no need to add to mp_to_lv and vv
        if type in mp_to_lv:
            mp_to_lv[orig_type] = mp_to_lv[type]
            lv_to_mp[orig_type] = lv_to_mp[type]
        
        return False

    def gen_mp_func(self, func, obj_name):
        # print("/*\n{ast}\n*/").format(ast=func)
        args = func.type.args.params
        # Handle the case of a single function argument which is "void"
        if len(args)==1 and get_arg_type(args[0].type) == "void":
            param_count = 0
        else:
            param_count = len(args)
        return_type = get_arg_type(func.type.type)
        if return_type == "void":        
            build_result = ""
            build_return_value = "mp_const_none" 
        else:
            if not return_type in lv_to_mp:
                self.try_generate_type(return_type)
                if not return_type in lv_to_mp:
                    raise MissingConversionException("Missing convertion from %s" % return_type)
            build_result = "%s res = " % return_type
            build_return_value = lv_to_mp[return_type](func, obj_name) if callable(lv_to_mp[return_type]) else \
                "%s(res)" % lv_to_mp[return_type]
        return("""
/*
 * lvgl extension definition for:
 * {print_func}
 */
 
STATIC mp_obj_t mp_{func}(size_t n_args, const mp_obj_t *args)
{{
    {build_args}
    {build_result}{func}({send_args});
    return {build_return_value};
}}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_{func}_obj, {count}, {count}, mp_{func});

 
""".format(func=func.name, 
             print_func=generate_c(func),
             count=param_count, 
             build_args="\n    ".join([self.build_mp_func_arg(arg, i, func, obj_name) for i,arg in enumerate(args) if arg.name]), 
             send_args=", ".join(arg.name for arg in args if arg.name),
             build_result=build_result,
             build_return_value=build_return_value))


    def build_mp_func_arg(self, arg, index, func, obj_name):
        arg_type = get_arg_type(arg.type)
        if not arg_type in mp_to_lv:
            self.try_generate_type(arg_type)
            if not arg_type in mp_to_lv:
                raise MissingConversionException("Missing conversion to %s" % arg_type)
        return mp_to_lv[arg_type](arg, index, func, obj_name) if callable(mp_to_lv[arg_type]) else \
            '{var} = {convertor}(args[{i}]);'.format(
                var = generate_c(arg),
                convertor = mp_to_lv[arg_type],
                i = index) 

    def customize(self):
        # TODO: remove these reordering construct (only required to show equality of the bindings generator)
        # All this reordering code is written such, that no items can accidentally be removed or added while reordering
        
        # reorder objects    
        for name in 'obj arc cont btn label bar btnm cb line chart page ddlist lmeter gauge img kb led list mbox preload roller slider sw win tabview ta'.split():
            self.objects[name] = self.objects.pop(name)
        
        
        # reorder global functions
        self.reordered_globals = reordered_globals = self.parseresult.global_functions.copy()

        for name in 'lv_color_to1 lv_color_to8 lv_color_to16 lv_color_to32 lv_color_mix lv_color_brightness lv_area_copy lv_area_get_width lv_area_get_height lv_font_get_height lv_color_hsv_to_rgb lv_color_rgb_to_hsv lv_area_set lv_area_set_width lv_area_set_height lv_area_set_pos lv_area_get_size lv_area_intersect lv_area_join lv_area_is_point_on lv_area_is_on lv_area_is_in lv_font_init lv_font_add lv_font_is_monospace lv_font_get_bitmap lv_font_get_width lv_font_get_real_width lv_font_get_bpp lv_font_get_bitmap_continuous lv_font_get_bitmap_sparse lv_font_get_width_continuous lv_font_get_width_sparse lv_font_builtin_init lv_anim_init lv_anim_create lv_anim_del lv_anim_speed_to_time lv_anim_path_linear lv_anim_path_ease_in_out lv_anim_path_step lv_style_init lv_style_copy lv_style_mix lv_style_anim_create lv_mem_init lv_mem_alloc lv_mem_free lv_mem_realloc lv_mem_defrag lv_mem_monitor lv_mem_get_size lv_ll_init lv_ll_ins_head lv_ll_ins_prev lv_ll_ins_tail lv_ll_rem lv_ll_clear lv_ll_chg_list lv_ll_get_head lv_ll_get_tail lv_ll_get_next lv_ll_get_prev lv_ll_move_before lv_init lv_scr_load lv_scr_act lv_layer_top lv_layer_sys lv_disp_drv_init lv_disp_drv_register lv_disp_set_active lv_disp_get_active lv_disp_next lv_disp_flush lv_disp_fill lv_disp_map lv_disp_mem_blend lv_disp_mem_fill lv_disp_is_mem_blend_supported lv_disp_is_mem_fill_supported lv_tick_inc lv_tick_get lv_tick_elaps lv_indev_drv_init lv_indev_drv_register lv_indev_next lv_indev_read lv_group_create lv_group_del lv_group_add_obj lv_group_remove_obj lv_group_focus_obj lv_group_focus_next lv_group_focus_prev lv_group_focus_freeze lv_group_send_data lv_group_set_style_mod_cb lv_group_set_style_mod_edit_cb lv_group_set_focus_cb lv_group_set_editing lv_group_set_click_focus lv_group_mod_style lv_group_get_focused lv_group_get_style_mod_cb lv_group_get_style_mod_edit_cb lv_group_get_focus_cb lv_group_get_editing lv_group_get_click_focus lv_indev_init lv_indev_get_act lv_indev_get_type lv_indev_reset lv_indev_reset_lpr lv_indev_enable lv_indev_set_cursor lv_indev_set_group lv_indev_set_button_points lv_indev_get_point lv_indev_get_key lv_indev_is_dragging lv_indev_get_vect lv_indev_get_inactive_time lv_indev_wait_release lv_txt_get_size lv_txt_get_next_line lv_txt_get_width lv_txt_is_cmd lv_txt_ins lv_txt_cut lv_fs_init lv_fs_add_drv lv_fs_open lv_fs_close lv_fs_remove lv_fs_read lv_fs_write lv_fs_seek lv_fs_tell lv_fs_trunc lv_fs_size lv_fs_rename lv_fs_dir_open lv_fs_dir_read lv_fs_dir_close lv_fs_free lv_fs_get_letters lv_fs_get_ext lv_fs_up lv_fs_get_last lv_draw_aa_get_opa lv_draw_aa_ver_seg lv_draw_aa_hor_seg lv_draw_rect lv_draw_label lv_draw_img lv_draw_line lv_draw_triangle'.split():
            f = reordered_globals.pop(name, None)
            if f:
                reordered_globals[name] = f        


        
        # naming issues: HAL_INDEV_TYPE and PRELOADER_TYPE enums (their members do not match the name of the type)
        
        # reorder enums (TODO: remove)
        reordered_enums = self.parseresult.enums.copy()
        for name in 'BORDER SHADOW DESIGN RES SIGNAL PROTECT ALIGN ANIM ARC_STYLE LAYOUT INDEV_TYPE INDEV_STATE BTN_STATE BTN_ACTION BTN_STYLE TXT_FLAG TXT_CMD_STATE LABEL_LONG LABEL_ALIGN BAR_STYLE BTNM_STYLE CB_STYLE CHART_TYPE SB_MODE PAGE_STYLE DDLIST_STYLE FS_RES FS_MODE IMG_SRC IMG_CF KB_MODE KB_STYLE LIST_STYLE MBOX_STYLE PRELOAD_TYPE_SPINNING PRELOAD_STYLE ROLLER_STYLE SLIDER_STYLE SW_STYLE WIN_STYLE TABVIEW_BTNS_POS TABVIEW_STYLE CURSOR TA_STYLE'.split():

            reordered_enums[name] = reordered_enums.pop(name)      
        

        
        # reorder typedefs
        self.reordered_typedefs = reordered_typedefs = self.parseresult.typedefs.copy()
        for name in 'lv_anim_path_t lv_anim_fp_t lv_anim_cb_t lv_design_func_t lv_signal_func_t lv_action_t lv_group_style_mod_func_t lv_group_focus_cb_t lv_btnm_action_t lv_img_decoder_info_f_t lv_img_decoder_open_f_t lv_img_decoder_read_line_f_t lv_img_decoder_close_f_t lv_tabview_action_t'.split():
            reordered_typedefs[name] = reordered_typedefs.pop(name)

        # TODO: this is only necessary, remove
        for o in self.objects.values():
            o.reorder_defs_decls()
                
        self.global_enums = collections.OrderedDict()
        self.enums = collections.OrderedDict()
        
        from os.path import commonprefix
        enums = {}

        
        for enum_name, enum in reordered_enums.items():
            match = re.match(r'([A-Za-z0-9]+)_\w+$', enum_name)  # TODO: matching of enums to objects should be done in bindings generator
            e = self.enums[enum_name] = MicroPythonEnum(enum_name, enum)
            
            object = self.objects.get(match.group(1).lower()) if match else None
            if object:
                object.enums[enum_name] = e #enum # TODO: also store MicroPythonEnum object here?
            else:
                self.global_enums[enum_name] = e
                
        self.global_functions = {name: MicroPythonGlobal(function, self) for name, function in self.reordered_globals.items()}
        
        
    @property
    def callbacks(self):
        return {item.name: MicroPythonCallback(item, self)
            for item in self.reordered_typedefs.values()
            if isinstance(item.type, c_ast.PtrDecl) and isinstance(item.type.type, c_ast.FuncDecl) }
 
    @property
    def global_functions_with_code(self):
        return {name: item for name, item in self.global_functions.items() if item.code_generated}    

if __name__ == '__main__':
    import sourceparser
    try:
        parseresult
    except NameError:
        parseresult = sourceparser.LvglSourceParser().parse_sources('lvgl')
    
    g = MicroPythonBindingsGenerator(parseresult)
    g.generate()
