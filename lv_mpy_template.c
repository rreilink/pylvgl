
/*
 * Auto-Generated file, DO NOT EDIT!
 *
 * Command line:
 * gen_mpy.py -X anim -X group -I pycparser/utils/fake_libc_include ../lv_objx/lv_arc.h ../lv_objx/lv_bar.h ../lv_objx/lv_btn.h ../lv_objx/lv_btnm.h ../lv_objx/lv_calendar.h ../lv_objx/lv_cb.h ../lv_objx/lv_chart.h ../lv_objx/lv_cont.h ../lv_objx/lv_ddlist.h ../lv_objx/lv_gauge.h ../lv_objx/lv_imgbtn.h ../lv_objx/lv_img.h ../lv_objx/lv_kb.h ../lv_objx/lv_label.h ../lv_objx/lv_led.h ../lv_objx/lv_line.h ../lv_objx/lv_list.h ../lv_objx/lv_lmeter.h ../lv_objx/lv_mbox.h ../lv_objx/lv_objx_templ.h ../lv_objx/lv_page.h ../lv_objx/lv_preload.h ../lv_objx/lv_roller.h ../lv_objx/lv_slider.h ../lv_objx/lv_sw.h ../lv_objx/lv_tabview.h ../lv_objx/lv_ta.h ../lv_objx/lv_win.h
 *
 * Preprocessing command:
 * gcc -E -std=c99 -I pycparser/utils/fake_libc_include -include ../lv_objx/lv_arc.h -include ../lv_objx/lv_bar.h -include ../lv_objx/lv_btn.h -include ../lv_objx/lv_btnm.h -include ../lv_objx/lv_calendar.h -include ../lv_objx/lv_cb.h -include ../lv_objx/lv_chart.h -include ../lv_objx/lv_cont.h -include ../lv_objx/lv_ddlist.h -include ../lv_objx/lv_gauge.h -include ../lv_objx/lv_imgbtn.h -include ../lv_objx/lv_img.h -include ../lv_objx/lv_kb.h -include ../lv_objx/lv_label.h -include ../lv_objx/lv_led.h -include ../lv_objx/lv_line.h -include ../lv_objx/lv_list.h -include ../lv_objx/lv_lmeter.h -include ../lv_objx/lv_mbox.h -include ../lv_objx/lv_objx_templ.h -include ../lv_objx/lv_page.h -include ../lv_objx/lv_preload.h -include ../lv_objx/lv_roller.h -include ../lv_objx/lv_slider.h -include ../lv_objx/lv_sw.h -include ../lv_objx/lv_tabview.h -include ../lv_objx/lv_ta.h -include ../lv_objx/lv_win.h ../lv_objx/lv_arc.h
 *
 * Generating Objects: <<<objects:{name}({ancestorname}), >>>
 */

/*
 * Mpy includes
 */

#include <string.h>
#include "py/obj.h"
#include "py/runtime.h"

/*
 * lvgl includes
 */

#include "../lv_objx/lv_arc.h"
#include "../lv_objx/lv_bar.h"
#include "../lv_objx/lv_btn.h"
#include "../lv_objx/lv_btnm.h"
#include "../lv_objx/lv_calendar.h"
#include "../lv_objx/lv_cb.h"
#include "../lv_objx/lv_chart.h"
#include "../lv_objx/lv_cont.h"
#include "../lv_objx/lv_ddlist.h"
#include "../lv_objx/lv_gauge.h"
#include "../lv_objx/lv_imgbtn.h"
#include "../lv_objx/lv_img.h"
#include "../lv_objx/lv_kb.h"
#include "../lv_objx/lv_label.h"
#include "../lv_objx/lv_led.h"
#include "../lv_objx/lv_line.h"
#include "../lv_objx/lv_list.h"
#include "../lv_objx/lv_lmeter.h"
#include "../lv_objx/lv_mbox.h"
#include "../lv_objx/lv_objx_templ.h"
#include "../lv_objx/lv_page.h"
#include "../lv_objx/lv_preload.h"
#include "../lv_objx/lv_roller.h"
#include "../lv_objx/lv_slider.h"
#include "../lv_objx/lv_sw.h"
#include "../lv_objx/lv_tabview.h"
#include "../lv_objx/lv_ta.h"
#include "../lv_objx/lv_win.h"


/*
 * Helper functions
 */

typedef lv_obj_t* (*lv_create)(lv_obj_t * par, const lv_obj_t * copy);

typedef struct mp_lv_obj_t {
    mp_obj_base_t base;
    lv_obj_t *lv_obj;
    mp_obj_t *action;
} mp_lv_obj_t;

STATIC inline lv_obj_t *mp_to_lv(mp_obj_t *mp_obj)
{
    if (mp_obj == NULL || mp_obj == mp_const_none) return NULL;
    mp_lv_obj_t *mp_lv_obj = MP_OBJ_TO_PTR(mp_obj);
    return mp_lv_obj->lv_obj;
}

STATIC inline mp_obj_t *mp_to_lv_action(mp_obj_t *mp_obj)
{
    if (mp_obj == NULL || mp_obj == mp_const_none) return NULL;
    mp_lv_obj_t *mp_lv_obj = MP_OBJ_TO_PTR(mp_obj);
    return mp_lv_obj->action;
}

STATIC inline void set_action(mp_obj_t *mp_obj, mp_obj_t *action)
{
    if (mp_obj == NULL || mp_obj == mp_const_none) return;
    mp_lv_obj_t *mp_lv_obj = MP_OBJ_TO_PTR(mp_obj);
    mp_lv_obj->action = action;
}

STATIC inline const mp_obj_type_t *get_BaseObj_type();

STATIC inline mp_obj_t *lv_to_mp(lv_obj_t *lv_obj)
{
    mp_lv_obj_t *self = lv_obj_get_free_ptr(lv_obj);
    if (!self) 
    {
        self = m_new_obj(mp_lv_obj_t);
        *self = (mp_lv_obj_t){
            .base = {get_BaseObj_type()},
            .lv_obj = lv_obj,
            .action = NULL
        };
    }
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t make_new(
    lv_create create,
    const mp_obj_type_t *type,
    size_t n_args,
    size_t n_kw,
    const mp_obj_t *args)
{
    mp_arg_check_num(n_args, n_kw, 1, 2, false);
    mp_lv_obj_t *self = m_new_obj(mp_lv_obj_t);
    lv_obj_t *parent = mp_to_lv(args[0]);
    lv_obj_t *copy = n_args > 1? mp_to_lv(args[1]): NULL;
    *self = (mp_lv_obj_t){
        .base = {type}, 
        .lv_obj = create(parent, copy),
        .action = NULL
    };
    lv_obj_set_free_ptr(self->lv_obj, self);
    return MP_OBJ_FROM_PTR(self);
}

STATIC inline mp_obj_t convert_to_bool(bool b)
{
    return b? mp_const_true: mp_const_false;
}

STATIC inline mp_obj_t convert_to_str(const char *str)
{
    return mp_obj_new_str(str, strlen(str));
}


<<<enums:
    
/*
 * lvgl LV_{name} object definitions
 */

STATIC const mp_rom_map_elem_t LV_{name}_locals_dict_table[] = {{
{locals_dict_table}
}};

STATIC MP_DEFINE_CONST_DICT(LV_{name}_locals_dict, LV_{name}_locals_dict_table);

STATIC void LV_{name}_print(const mp_print_t *print,
    mp_obj_t self_in,
    mp_print_kind_t kind)
{{
    mp_printf(print, "lvgl LV_{name}");
}}



STATIC const mp_obj_type_t mp_LV_{name}_type = {{
    {{ &mp_type_type }},
    .name = MP_QSTR_LV_{name},
    .print = LV_{name}_print,
    
    .locals_dict = (mp_obj_dict_t*)&LV_{name}_locals_dict,
}};
    
>>><<<callbacks:{callback_code}>>><<<objects:{methodscode}
    
/*
 * lvgl {name} object definitions
 */

STATIC const mp_rom_map_elem_t {name}_locals_dict_table[] = {{
{methodtablecode}
}};

STATIC MP_DEFINE_CONST_DICT({name}_locals_dict, {name}_locals_dict_table);

STATIC void {name}_print(const mp_print_t *print,
    mp_obj_t self_in,
    mp_print_kind_t kind)
{{
    mp_printf(print, "lvgl {name}");
}}


STATIC mp_obj_t {name}_make_new(
    const mp_obj_type_t *type,
    size_t n_args,
    size_t n_kw,
    const mp_obj_t *args)
{{
    return make_new(&lv_{name}_create, type, n_args, n_kw, args);           
}}


STATIC const mp_obj_type_t mp_{name}_type = {{
    {{ &mp_type_type }},
    .name = MP_QSTR_{name},
    .print = {name}_print,
    .make_new = {name}_make_new,
    .locals_dict = (mp_obj_dict_t*)&{name}_locals_dict,
}};
    
>>>
STATIC inline const mp_obj_type_t *get_BaseObj_type()
{
    return &mp_obj_type;
}


/* 
 *
 * Global Module Functions
 *
 */

<<<global_functions:{code}>>>

/*
 * lvgl module definitions
 * User should implement lv_mp_init. Display can be initialized there, if needed.
 */

extern void lv_mp_init();

STATIC mp_obj_t _lv_mp_init()
{
    lv_mp_init();
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(lv_mp_init_obj, _lv_mp_init);

STATIC const mp_rom_map_elem_t lvgl_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_lvgl) },
    { MP_OBJ_NEW_QSTR(MP_QSTR___init__), MP_ROM_PTR(&lv_mp_init_obj) },
<<<objects:    {{ MP_OBJ_NEW_QSTR(MP_QSTR_{name}), MP_ROM_PTR(&mp_{name}_type) }},
>>><<<global_functions_with_code:    {{ MP_OBJ_NEW_QSTR(MP_QSTR_{shortname}), MP_ROM_PTR(&mp_{name}_obj) }},
>>><<<global_enums:    {{ MP_OBJ_NEW_QSTR(MP_QSTR_{name}), MP_ROM_PTR(&mp_LV_{name}_type) }},
>>>};


STATIC MP_DEFINE_CONST_DICT (
    mp_module_lvgl_globals,
    lvgl_globals_table
);

const mp_obj_module_t mp_module_lvgl = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_lvgl_globals
};

