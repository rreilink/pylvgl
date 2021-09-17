#include "Python.h"
#include "structmember.h"
#include "lvgl/lvgl.h"


#if LV_COLOR_DEPTH != 16
#error Only 16 bits color depth is currently supported
#endif


/* Note on the lvgl lock and the GIL:
 *
 * Any attempt to aquire the lock should be with the GIL released. Otherwise,
 * The following situation could occur:
 *
 * Thread 1:                    Thread 2 (lv_poll called)
 *   has the GIL                  has the lvgl lock
 *   waits for lvgl lock          process callback --> aquire GIL
 *
 * This would be a deadlock situation
 */

#define LVGL_LOCK \
    if (lock) { \
        Py_BEGIN_ALLOW_THREADS \
        lock(lock_arg); \
        Py_END_ALLOW_THREADS \
    }

#define LVGL_UNLOCK \
    if (unlock) { unlock(unlock_arg); }
 

/****************************************************************
 * Object struct definitions                                    *
 ****************************************************************/

typedef struct {
    PyObject_HEAD
    PyObject *weakreflist;
    lv_obj_t *ref;
    PyObject *event_cb;
    lv_signal_cb_t orig_signal_cb;
} pylv_Obj;

typedef pylv_Obj pylv_Cont;

typedef pylv_Cont pylv_Btn;

typedef pylv_Btn pylv_Imgbtn;

typedef pylv_Obj pylv_Label;

typedef pylv_Obj pylv_Img;

typedef pylv_Obj pylv_Line;

typedef pylv_Cont pylv_Page;

typedef pylv_Page pylv_List;

typedef pylv_Obj pylv_Chart;

typedef pylv_Obj pylv_Table;

typedef pylv_Btn pylv_Checkbox;

typedef pylv_Obj pylv_Cpicker;

typedef pylv_Obj pylv_Bar;

typedef pylv_Bar pylv_Slider;

typedef pylv_Obj pylv_Led;

typedef pylv_Obj pylv_Btnmatrix;

typedef pylv_Btnmatrix pylv_Keyboard;

typedef pylv_Obj pylv_Dropdown;

typedef pylv_Page pylv_Roller;

typedef pylv_Page pylv_Textarea;

typedef pylv_Img pylv_Canvas;

typedef pylv_Obj pylv_Win;

typedef pylv_Obj pylv_Tabview;

typedef pylv_Page pylv_Tileview;

typedef pylv_Cont pylv_Msgbox;

typedef pylv_Cont pylv_Objmask;

typedef pylv_Obj pylv_Linemeter;

typedef pylv_Linemeter pylv_Gauge;

typedef pylv_Bar pylv_Switch;

typedef pylv_Obj pylv_Arc;

typedef pylv_Arc pylv_Spinner;

typedef pylv_Obj pylv_Calendar;

typedef pylv_Textarea pylv_Spinbox;



/****************************************************************
 * Forward declaration of type objects                          *
 ****************************************************************/

PyObject *typesdict = NULL;


static PyTypeObject pylv_obj_Type;

static PyTypeObject pylv_cont_Type;

static PyTypeObject pylv_btn_Type;

static PyTypeObject pylv_imgbtn_Type;

static PyTypeObject pylv_label_Type;

static PyTypeObject pylv_img_Type;

static PyTypeObject pylv_line_Type;

static PyTypeObject pylv_page_Type;

static PyTypeObject pylv_list_Type;

static PyTypeObject pylv_chart_Type;

static PyTypeObject pylv_table_Type;

static PyTypeObject pylv_checkbox_Type;

static PyTypeObject pylv_cpicker_Type;

static PyTypeObject pylv_bar_Type;

static PyTypeObject pylv_slider_Type;

static PyTypeObject pylv_led_Type;

static PyTypeObject pylv_btnmatrix_Type;

static PyTypeObject pylv_keyboard_Type;

static PyTypeObject pylv_dropdown_Type;

static PyTypeObject pylv_roller_Type;

static PyTypeObject pylv_textarea_Type;

static PyTypeObject pylv_canvas_Type;

static PyTypeObject pylv_win_Type;

static PyTypeObject pylv_tabview_Type;

static PyTypeObject pylv_tileview_Type;

static PyTypeObject pylv_msgbox_Type;

static PyTypeObject pylv_objmask_Type;

static PyTypeObject pylv_linemeter_Type;

static PyTypeObject pylv_gauge_Type;

static PyTypeObject pylv_switch_Type;

static PyTypeObject pylv_arc_Type;

static PyTypeObject pylv_spinner_Type;

static PyTypeObject pylv_calendar_Type;

static PyTypeObject pylv_spinbox_Type;



static PyTypeObject pylv_mem_monitor_t_Type;

static PyTypeObject pylv_mem_buf_t_Type;

static PyTypeObject pylv_ll_t_Type;

static PyTypeObject pylv_task_t_Type;

static PyTypeObject pylv_sqrt_res_t_Type;

static PyTypeObject pylv_color1_t_Type;

static PyTypeObject pylv_color8_t_Type;

static PyTypeObject pylv_color16_t_Type;

static PyTypeObject pylv_color32_t_Type;

static PyTypeObject pylv_color_hsv_t_Type;

static PyTypeObject pylv_point_t_Type;

static PyTypeObject pylv_area_t_Type;

static PyTypeObject pylv_disp_buf_t_Type;

static PyTypeObject pylv_disp_drv_t_Type;

static PyTypeObject pylv_disp_t_Type;

static PyTypeObject pylv_indev_data_t_Type;

static PyTypeObject pylv_indev_drv_t_Type;

static PyTypeObject pylv_indev_proc_t_Type;

static PyTypeObject pylv_indev_t_Type;

static PyTypeObject pylv_font_glyph_dsc_t_Type;

static PyTypeObject pylv_font_t_Type;

static PyTypeObject pylv_anim_path_t_Type;

static PyTypeObject pylv_anim_t_Type;

static PyTypeObject pylv_draw_mask_common_dsc_t_Type;

static PyTypeObject pylv_draw_mask_line_param_t_Type;

static PyTypeObject pylv_draw_mask_angle_param_t_Type;

static PyTypeObject pylv_draw_mask_radius_param_t_Type;

static PyTypeObject pylv_draw_mask_fade_param_t_Type;

static PyTypeObject pylv_draw_mask_map_param_t_Type;

static PyTypeObject pylv_style_list_t_Type;

static PyTypeObject pylv_draw_rect_dsc_t_Type;

static PyTypeObject pylv_draw_label_dsc_t_Type;

static PyTypeObject pylv_draw_label_hint_t_Type;

static PyTypeObject pylv_draw_line_dsc_t_Type;

static PyTypeObject pylv_img_header_t_Type;

static PyTypeObject pylv_img_dsc_t_Type;

static PyTypeObject pylv_img_transform_dsc_t_Type;

static PyTypeObject pylv_fs_drv_t_Type;

static PyTypeObject pylv_fs_file_t_Type;

static PyTypeObject pylv_fs_dir_t_Type;

static PyTypeObject pylv_img_decoder_t_Type;

static PyTypeObject pylv_img_decoder_dsc_t_Type;

static PyTypeObject pylv_draw_img_dsc_t_Type;

static PyTypeObject pylv_realign_t_Type;

static PyTypeObject pylv_obj_t_Type;

static PyTypeObject pylv_obj_type_t_Type;

static PyTypeObject pylv_hit_test_info_t_Type;

static PyTypeObject pylv_get_style_info_t_Type;

static PyTypeObject pylv_get_state_info_t_Type;

static PyTypeObject pylv_group_t_Type;

static PyTypeObject pylv_theme_t_Type;

static PyTypeObject pylv_font_fmt_txt_glyph_dsc_t_Type;

static PyTypeObject pylv_font_fmt_txt_cmap_t_Type;

static PyTypeObject pylv_font_fmt_txt_kern_pair_t_Type;

static PyTypeObject pylv_font_fmt_txt_kern_classes_t_Type;

static PyTypeObject pylv_font_fmt_txt_dsc_t_Type;

static PyTypeObject pylv_cont_ext_t_Type;

static PyTypeObject pylv_btn_ext_t_Type;

static PyTypeObject pylv_imgbtn_ext_t_Type;

static PyTypeObject pylv_label_ext_t_Type;

static PyTypeObject pylv_img_ext_t_Type;

static PyTypeObject pylv_line_ext_t_Type;

static PyTypeObject pylv_page_ext_t_Type;

static PyTypeObject pylv_list_ext_t_Type;

static PyTypeObject pylv_chart_series_t_Type;

static PyTypeObject pylv_chart_cursor_t_Type;

static PyTypeObject pylv_chart_axis_cfg_t_Type;

static PyTypeObject pylv_chart_ext_t_Type;

static PyTypeObject pylv_table_cell_format_t_Type;

static PyTypeObject pylv_table_ext_t_Type;

static PyTypeObject pylv_checkbox_ext_t_Type;

static PyTypeObject pylv_cpicker_ext_t_Type;

static PyTypeObject pylv_bar_anim_t_Type;

static PyTypeObject pylv_bar_ext_t_Type;

static PyTypeObject pylv_slider_ext_t_Type;

static PyTypeObject pylv_led_ext_t_Type;

static PyTypeObject pylv_btnmatrix_ext_t_Type;

static PyTypeObject pylv_keyboard_ext_t_Type;

static PyTypeObject pylv_dropdown_ext_t_Type;

static PyTypeObject pylv_roller_ext_t_Type;

static PyTypeObject pylv_textarea_ext_t_Type;

static PyTypeObject pylv_canvas_ext_t_Type;

static PyTypeObject pylv_win_ext_t_Type;

static PyTypeObject pylv_tabview_ext_t_Type;

static PyTypeObject pylv_tileview_ext_t_Type;

static PyTypeObject pylv_msgbox_ext_t_Type;

static PyTypeObject pylv_objmask_mask_t_Type;

static PyTypeObject pylv_objmask_ext_t_Type;

static PyTypeObject pylv_linemeter_ext_t_Type;

static PyTypeObject pylv_gauge_ext_t_Type;

static PyTypeObject pylv_switch_ext_t_Type;

static PyTypeObject pylv_arc_ext_t_Type;

static PyTypeObject pylv_spinner_ext_t_Type;

static PyTypeObject pylv_calendar_date_t_Type;

static PyTypeObject pylv_calendar_ext_t_Type;

static PyTypeObject pylv_spinbox_ext_t_Type;

static PyTypeObject pylv_img_cache_entry_t_Type;

static PyTypeObject pylv_color1_t_ch_Type;

static PyTypeObject pylv_color8_t_ch_Type;

static PyTypeObject pylv_color16_t_ch_Type;

static PyTypeObject pylv_color32_t_ch_Type;

static PyTypeObject pylv_indev_proc_t_types_Type;

static PyTypeObject pylv_indev_proc_t_types_pointer_Type;

static PyTypeObject pylv_indev_proc_t_types_keypad_Type;

static PyTypeObject pylv_draw_mask_line_param_t_cfg_Type;

static PyTypeObject pylv_draw_mask_angle_param_t_cfg_Type;

static PyTypeObject pylv_draw_mask_radius_param_t_cfg_Type;

static PyTypeObject pylv_draw_mask_fade_param_t_cfg_Type;

static PyTypeObject pylv_draw_mask_map_param_t_cfg_Type;

static PyTypeObject pylv_img_transform_dsc_t_cfg_Type;

static PyTypeObject pylv_img_transform_dsc_t_res_Type;

static PyTypeObject pylv_img_transform_dsc_t_tmp_Type;

static PyTypeObject pylv_label_ext_t_dot_Type;

static PyTypeObject pylv_page_ext_t_scrlbar_Type;

static PyTypeObject pylv_page_ext_t_edge_flash_Type;

static PyTypeObject pylv_table_cell_format_t_s_Type;

static PyTypeObject pylv_cpicker_ext_t_knob_Type;

static PyTypeObject pylv_textarea_ext_t_cursor_Type;


/****************************************************************
 * Custom type: Ptr                                             * 
 *                                                              *
 * Objects of this type are used as keys in struct_dict         *
 * A Ptr holds a void* value, can be (in)-equality-compared and *
 * can be hashed                                                *
 ****************************************************************/
typedef struct {
    PyObject_HEAD
    const void *ptr;
} PtrObject;
static PyTypeObject Ptr_Type;

static PyObject* Ptr_repr(PyObject *self) {
    return PyUnicode_FromFormat("<%s object at %p = %p>",
                        self->ob_type->tp_name, self, ((PtrObject *)self)->ptr);
}

Py_hash_t Ptr_hash(PtrObject *self) {
    Py_hash_t x;
    x = (Py_hash_t)self->ptr;
    if (x == -1)
        x = -2;
    return x;
}

static PyObject *
Ptr_richcompare(PtrObject *obj1, PtrObject *obj2, int op)
{
    PyObject *result = Py_NotImplemented;

    if ((Py_TYPE(obj1) == &Ptr_Type) && (Py_TYPE(obj2) == &Ptr_Type)) {

        switch (op) {
        case Py_EQ: 
            result = (obj1->ptr == obj2->ptr) ? Py_True : Py_False; 
            break;
        case Py_NE:
            result = (obj1->ptr != obj2->ptr) ? Py_True : Py_False; 
            break;
        default:
            ;
        }
    }
    Py_INCREF(result);
    return result;
 }

static PyTypeObject Ptr_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Ptr",
    .tp_doc = "lvgl pointer",
    .tp_basicsize = sizeof(PtrObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = NULL, // cannot be instantiated
    .tp_repr = (reprfunc) Ptr_repr,
    .tp_hash = (hashfunc) Ptr_hash,
    .tp_richcompare = (richcmpfunc) Ptr_richcompare,
};

PyObject *PtrObject_fromptr(const void *ptr) {
    PtrObject *ob = PyObject_New(PtrObject, &Ptr_Type);
    if (ob) ob->ptr = ptr;
    return (PyObject*) ob;
}


/****************************************************************
 * Helper functons                                              *  
 ****************************************************************/

static void (*lock)(void*) = NULL;
static void* lock_arg = 0;

static void (*unlock)(void*) = NULL;
static void* unlock_arg = 0;

/* 
 * This function itself is not thread-safe
 */
void lv_set_lock_unlock( void (*flock)(void *), void * flock_arg, 
            void (*funlock)(void *), void * funlock_arg)
{
    lock_arg = flock_arg;
    unlock_arg = funlock_arg;
    lock = flock;
    unlock = funlock;
}



/* This signal handler is critical in the deallocation process of lvgl and
 * associated Python objects. It is called when an lvgl object is deleted either
 * via obj.del_() or when any of its ancestors is deleted or cleared
 *
 * The order in which things are done here (and in the pylv_xx_dealloc function)
 * is very critical to ensure no access-after-free errors.
 *
 * Two distinct cases can happen: either there are still other Python references
 * to the Python object or there are no other references, and the DECREF in 
 * this signal handler will cause the Python object be deleted, too.
 *
 * For the case when there are still references left, py_obj->ref will be set
 * to NULL so that any access will raise a RuntimeError
 *
 *
 * (the case that there never was a Python object for this lvgl object cannot
 * happen, since then this signal handler was never installed)
 */
static lv_res_t pylv_signal_cb(lv_obj_t * obj, lv_signal_t sign, void * param)
{
    pylv_Obj* py_obj = (pylv_Obj*)(obj->user_data);
    
    // store a reference to the original signal callback, since during the
    // CLEANUP signal, py_obj may get deallocated and then this reference is gone
    lv_signal_cb_t orig_signal_cb = py_obj->orig_signal_cb;
        
    if (py_obj) {
        if (sign == LV_SIGNAL_CLEANUP) {
            // mark object as deleted
            py_obj->ref = NULL; 

            // restore original signal callback to make sure that this callback
            // cannot be called again (the Python object may be deleted by then)
            lv_obj_set_signal_cb(obj, py_obj->orig_signal_cb); 
            
            // remove reference to Python object
            obj->user_data = NULL;
            Py_DECREF(py_obj); 
        }

    }
    return orig_signal_cb(obj, sign, param);
}

int check_alive(pylv_Obj* obj) {
    if (!obj->ref) {
        PyErr_SetString(PyExc_RuntimeError, "the underlying C object has been deleted");
        return -1;
    }
    return 0;
}

static void install_signal_cb(pylv_Obj * py_obj) {
    py_obj->orig_signal_cb = lv_obj_get_signal_cb(py_obj->ref);       /*Save to old signal function*/
    lv_obj_set_signal_cb(py_obj->ref, pylv_signal_cb);
}



/* Given an lvgl lv_obj, return the accompanying Python object. If the 
 * accompanying object already exists, it is returned (with ref count increased).
 * If the lv_obj is not yet known to Python, a new Python object is created,
 * with the appropriate type (which is determined using lv_obj_get_type and the
 * typesdict dictionary of lv_obj_type name (string) --> Python Type
 *
 * Returns a new reference
 */

PyObject * pyobj_from_lv(lv_obj_t *obj) {
    pylv_Obj *pyobj;
    lv_obj_type_t objtype;
    const char *objtype_str;
    PyTypeObject *tp = NULL;

    if (!obj) {
        Py_RETURN_NONE;
    }
    
    pyobj = obj->user_data;
    
    if (!pyobj) {
        // Python object for this lv object does not yet exist. Create a new one
        // Be sure to zero out the memory
        
        lv_obj_get_type(obj, &objtype);
        objtype_str = objtype.type[0];
        if (objtype_str) {
            tp = (PyTypeObject *)PyDict_GetItemString(typesdict, objtype_str); // borrowed reference
        }
        if (!tp) tp = &pylv_obj_Type; // Default to Obj (should not happen; lv_obj_get_type failed or result not found in typesdict)

        pyobj = PyObject_Malloc(tp->tp_basicsize);
        if (!pyobj) return NULL;
        memset(pyobj, 0, tp->tp_basicsize);
        PyObject_Init((PyObject *)pyobj, tp);
        pyobj -> ref = obj;
        obj->user_data = pyobj;
        install_signal_cb(pyobj);
        // reference count for pyobj is 1 -- the reference stored in the lvgl object user_data
    }

    Py_INCREF(pyobj); // increase reference count of returned object
    
    return (PyObject *)pyobj;
}

/* lvgl.Style class
 *
 *
 *
 *
 *
 */

typedef struct {
    PyObject_HEAD
    lv_style_t *style;
} StyleObject;
static PyTypeObject Style_Type;


static PyObject*
Style_repr(StyleObject *self) {
    return PyUnicode_FromFormat("<%s object at %p referencing %p>", Py_TYPE(self)->tp_name, self, self->style);
}

static int
Style_init(StyleObject *self, PyObject *args, PyObject *kwds)
{
    self->style = PyMem_Malloc(sizeof(lv_style_t));
    lv_style_init(self->style);
    return 0;
}


static PyObject *
pylv_Style_set_radius(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_radius(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_size(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_size(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_transform_width(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_transform_width(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_transform_height(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_transform_height(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_transform_angle(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_transform_angle(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_transform_zoom(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_transform_zoom(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_pad_top(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_pad_top(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_pad_bottom(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_pad_bottom(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_pad_left(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_pad_left(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_pad_right(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_pad_right(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_pad_inner(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_pad_inner(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_margin_top(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_margin_top(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_margin_bottom(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_margin_bottom(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_margin_left(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_margin_left(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_margin_right(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_margin_right(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_bg_main_stop(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_bg_main_stop(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_bg_grad_stop(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_bg_grad_stop(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_border_width(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_border_width(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_outline_width(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_outline_width(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_outline_pad(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_outline_pad(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_shadow_width(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_shadow_width(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_shadow_ofs_x(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_shadow_ofs_x(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_shadow_ofs_y(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_shadow_ofs_y(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_shadow_spread(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_shadow_spread(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_value_letter_space(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_value_letter_space(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_value_line_space(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_value_line_space(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_value_ofs_x(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_value_ofs_x(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_value_ofs_y(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_value_ofs_y(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_text_letter_space(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_text_letter_space(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_text_line_space(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_text_line_space(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_line_width(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_line_width(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_line_dash_width(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_line_dash_width(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_line_dash_gap(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_line_dash_gap(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_transition_time(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_transition_time(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_transition_delay(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_transition_delay(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_transition_prop_1(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_transition_prop_1(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_transition_prop_2(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_transition_prop_2(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_transition_prop_3(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_transition_prop_3(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_transition_prop_4(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_transition_prop_4(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_transition_prop_5(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_transition_prop_5(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_transition_prop_6(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_transition_prop_6(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_scale_width(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_scale_width(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_scale_border_width(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_scale_border_width(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_scale_end_border_width(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_scale_end_border_width(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_scale_end_line_width(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_scale_end_line_width(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_pad_all(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_pad_all(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_pad_hor(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_pad_hor(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_pad_ver(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_pad_ver(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_margin_all(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_margin_all(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_margin_hor(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_margin_hor(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject *
pylv_Style_set_margin_ver(PyObject *self, PyObject *args) {
    int value;
    int state;
    if (!PyArg_ParseTuple(args, "ii", &state, &value)) return NULL;
    LVGL_LOCK
    lv_style_set_margin_ver(((StyleObject*)self)->style, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}


static PyMethodDef Style_methods[] = {

    {"set_radius", (PyCFunction) pylv_Style_set_radius, METH_VARARGS, "test doc"},

    {"set_size", (PyCFunction) pylv_Style_set_size, METH_VARARGS, "test doc"},

    {"set_transform_width", (PyCFunction) pylv_Style_set_transform_width, METH_VARARGS, "test doc"},

    {"set_transform_height", (PyCFunction) pylv_Style_set_transform_height, METH_VARARGS, "test doc"},

    {"set_transform_angle", (PyCFunction) pylv_Style_set_transform_angle, METH_VARARGS, "test doc"},

    {"set_transform_zoom", (PyCFunction) pylv_Style_set_transform_zoom, METH_VARARGS, "test doc"},

    {"set_pad_top", (PyCFunction) pylv_Style_set_pad_top, METH_VARARGS, "test doc"},

    {"set_pad_bottom", (PyCFunction) pylv_Style_set_pad_bottom, METH_VARARGS, "test doc"},

    {"set_pad_left", (PyCFunction) pylv_Style_set_pad_left, METH_VARARGS, "test doc"},

    {"set_pad_right", (PyCFunction) pylv_Style_set_pad_right, METH_VARARGS, "test doc"},

    {"set_pad_inner", (PyCFunction) pylv_Style_set_pad_inner, METH_VARARGS, "test doc"},

    {"set_margin_top", (PyCFunction) pylv_Style_set_margin_top, METH_VARARGS, "test doc"},

    {"set_margin_bottom", (PyCFunction) pylv_Style_set_margin_bottom, METH_VARARGS, "test doc"},

    {"set_margin_left", (PyCFunction) pylv_Style_set_margin_left, METH_VARARGS, "test doc"},

    {"set_margin_right", (PyCFunction) pylv_Style_set_margin_right, METH_VARARGS, "test doc"},

    {"set_bg_main_stop", (PyCFunction) pylv_Style_set_bg_main_stop, METH_VARARGS, "test doc"},

    {"set_bg_grad_stop", (PyCFunction) pylv_Style_set_bg_grad_stop, METH_VARARGS, "test doc"},

    {"set_border_width", (PyCFunction) pylv_Style_set_border_width, METH_VARARGS, "test doc"},

    {"set_outline_width", (PyCFunction) pylv_Style_set_outline_width, METH_VARARGS, "test doc"},

    {"set_outline_pad", (PyCFunction) pylv_Style_set_outline_pad, METH_VARARGS, "test doc"},

    {"set_shadow_width", (PyCFunction) pylv_Style_set_shadow_width, METH_VARARGS, "test doc"},

    {"set_shadow_ofs_x", (PyCFunction) pylv_Style_set_shadow_ofs_x, METH_VARARGS, "test doc"},

    {"set_shadow_ofs_y", (PyCFunction) pylv_Style_set_shadow_ofs_y, METH_VARARGS, "test doc"},

    {"set_shadow_spread", (PyCFunction) pylv_Style_set_shadow_spread, METH_VARARGS, "test doc"},

    {"set_value_letter_space", (PyCFunction) pylv_Style_set_value_letter_space, METH_VARARGS, "test doc"},

    {"set_value_line_space", (PyCFunction) pylv_Style_set_value_line_space, METH_VARARGS, "test doc"},

    {"set_value_ofs_x", (PyCFunction) pylv_Style_set_value_ofs_x, METH_VARARGS, "test doc"},

    {"set_value_ofs_y", (PyCFunction) pylv_Style_set_value_ofs_y, METH_VARARGS, "test doc"},

    {"set_text_letter_space", (PyCFunction) pylv_Style_set_text_letter_space, METH_VARARGS, "test doc"},

    {"set_text_line_space", (PyCFunction) pylv_Style_set_text_line_space, METH_VARARGS, "test doc"},

    {"set_line_width", (PyCFunction) pylv_Style_set_line_width, METH_VARARGS, "test doc"},

    {"set_line_dash_width", (PyCFunction) pylv_Style_set_line_dash_width, METH_VARARGS, "test doc"},

    {"set_line_dash_gap", (PyCFunction) pylv_Style_set_line_dash_gap, METH_VARARGS, "test doc"},

    {"set_transition_time", (PyCFunction) pylv_Style_set_transition_time, METH_VARARGS, "test doc"},

    {"set_transition_delay", (PyCFunction) pylv_Style_set_transition_delay, METH_VARARGS, "test doc"},

    {"set_transition_prop_1", (PyCFunction) pylv_Style_set_transition_prop_1, METH_VARARGS, "test doc"},

    {"set_transition_prop_2", (PyCFunction) pylv_Style_set_transition_prop_2, METH_VARARGS, "test doc"},

    {"set_transition_prop_3", (PyCFunction) pylv_Style_set_transition_prop_3, METH_VARARGS, "test doc"},

    {"set_transition_prop_4", (PyCFunction) pylv_Style_set_transition_prop_4, METH_VARARGS, "test doc"},

    {"set_transition_prop_5", (PyCFunction) pylv_Style_set_transition_prop_5, METH_VARARGS, "test doc"},

    {"set_transition_prop_6", (PyCFunction) pylv_Style_set_transition_prop_6, METH_VARARGS, "test doc"},

    {"set_scale_width", (PyCFunction) pylv_Style_set_scale_width, METH_VARARGS, "test doc"},

    {"set_scale_border_width", (PyCFunction) pylv_Style_set_scale_border_width, METH_VARARGS, "test doc"},

    {"set_scale_end_border_width", (PyCFunction) pylv_Style_set_scale_end_border_width, METH_VARARGS, "test doc"},

    {"set_scale_end_line_width", (PyCFunction) pylv_Style_set_scale_end_line_width, METH_VARARGS, "test doc"},

    {"set_pad_all", (PyCFunction) pylv_Style_set_pad_all, METH_VARARGS, "test doc"},

    {"set_pad_hor", (PyCFunction) pylv_Style_set_pad_hor, METH_VARARGS, "test doc"},

    {"set_pad_ver", (PyCFunction) pylv_Style_set_pad_ver, METH_VARARGS, "test doc"},

    {"set_margin_all", (PyCFunction) pylv_Style_set_margin_all, METH_VARARGS, "test doc"},

    {"set_margin_hor", (PyCFunction) pylv_Style_set_margin_hor, METH_VARARGS, "test doc"},

    {"set_margin_ver", (PyCFunction) pylv_Style_set_margin_ver, METH_VARARGS, "test doc"},

    {NULL}
};

static PyTypeObject Style_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Style",
    .tp_doc = "lvgl style",
    .tp_basicsize = sizeof(StyleObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) Style_init,
    //.tp_dealloc = (destructor) Style_dealloc,
    .tp_repr = (reprfunc) Style_repr,
    .tp_methods = Style_methods
};


static int pylv_style_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    isinst = PyObject_IsInstance(obj, (PyObject*)&Style_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be a Style object");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_style_t **)target = (void *)((StyleObject*)obj) -> style;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}


 
/* Given a pointer to a c struct, return the Python struct object
 * of that struct.
 *
 * This is only possible for struct pointers that already have an
 * associated Python object, i.e. the global ones and those
 * created from Python
 *
 * The module global struct_dict dictionary stores all struct
 * objects known
 */
 
static PyObject* struct_dict;

static PyObject *pystruct_from_lv(const void *c_struct) {
    PyObject *ret;
    PyObject *ptr;
    ptr = PtrObject_fromptr(c_struct);
    if (!ptr) return NULL;
    
    ret = PyDict_GetItem(struct_dict, ptr);
    Py_DECREF(ptr);

    if (ret) {
        Py_INCREF(ret); // ret is a borrowed reference; so need to increase ref count
    } else {
        PyErr_SetString(PyExc_RuntimeError, "the returned C struct is unknown to Python");
    }
    return ret;
}






/****************************************************************
 * Custom types: structs                                        *  
 ****************************************************************/
typedef struct {
    PyObject_HEAD
    char *data;
    size_t size;
    PyObject *owner; // NULL = reference to global C data, self=allocated @ init, other object=sharing from that object; decref owner when we are deallocated
    bool readonly;
} StructObject;


static PyObject*
Struct_repr(StructObject *self) {
    return PyUnicode_FromFormat("<%s struct at %p %sdata = %p (%d bytes) owner = %p>", Py_TYPE(self)->tp_name, self, (self->readonly? "(readonly) " : ""), self->data, self->size, self->owner);
}

static void
Struct_dealloc(StructObject *self)
{
    if (self->owner == (PyObject *)self) {
        PyMem_Free(self->data);
    } else {
        Py_XDECREF(self->owner); // owner could be NULL if data is global, in that case this statement has no effect
    }
    Py_TYPE(self)->tp_free((PyObject *) self);
}

// Provide a read-write buffer to the binary data in this struct
static int Struct_getbuffer(PyObject *exporter, Py_buffer *view, int flags) {
    StructObject *self = (StructObject*)exporter;
    return PyBuffer_FillInfo(view, exporter, self->data, self->size, self->readonly, flags);
}

static PyBufferProcs Struct_bufferprocs = {
    (getbufferproc)Struct_getbuffer,
    NULL,
};

// Register a Struct object in the global struct_dict dictionary, such that if
// a C function returns a pointer to the C struct, the associated Python object
// can be returned
//
// Also, if lvgl takes or releases a reference to a C struct, a reference to the
// associated Python object should be taken resp. freed. This requires all
// C structure pointers to be referrable to Python objects
//
// returns 0 on success, -1 on error with exception set
static int Struct_register(StructObject *obj) {
    PyObject *ptr_pyobj;
    ptr_pyobj = PtrObject_fromptr(obj->data);

    if (!ptr_pyobj) return -1;
    
    if (PyDict_SetItem(struct_dict, ptr_pyobj, (PyObject*) obj)) { // todo: use weak references
        Py_DECREF(ptr_pyobj);
        return -1;
    }

    Py_DECREF(ptr_pyobj);
    return 0;
}

// Helper to create struct object for global lvgl variables
// This also adds those Python objects to struct_dict so that they can be
// returned from object calls
static PyObject *
pystruct_from_c(PyTypeObject *type, const void* ptr, size_t size, bool copy) {
    StructObject *ret = 0;

    ret = (StructObject*)PyObject_New(StructObject, type);
    if (!ret) return NULL;

    if (copy) {
        ret->data = PyMem_Malloc(size);
        if (!ret->data) {
            Py_DECREF(ret);
            return NULL;
        }
        memcpy(ret->data, ptr, size);
        ret->owner = (PyObject *)ret; // This Python object is the owner of the data; free the data on delete
    } else {
        ret->owner = NULL; // owner = NULL means: global data, do not free
        ret->data = (void*)ptr; // cast const to non-const, but we set readonly to prevent writing
    }
    ret->size = size;
    ret->readonly = 1;
    
    if (Struct_register(ret)<0) {
        Py_DECREF(ret);
        return NULL;
    }


    return (PyObject*)ret;


}


// Struct members whose type is unsupported, get / set a 'blob', which stores
// a reference to the data, which can be copied but not accessed otherwise

static PyTypeObject Blob_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.blob",
    .tp_doc = "lvgl data blob",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // cannot be instantiated
    .tp_repr = (reprfunc) Struct_repr,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_as_buffer = &Struct_bufferprocs
};



static int long_to_int(PyObject *value, long *v, long min, long max) {
    long r = PyLong_AsLong(value);
    if ((r == -1) && PyErr_Occurred()) return -1;
    if ((r<min) || (r>max)) {
        PyErr_Format(PyExc_ValueError, "value out of range %ld..%ld", min, max);
        return -1;
    }
    *v = r;
    return 0;
}   


static int struct_check_readonly(StructObject *self) {
    if (self->readonly) {
        PyErr_SetString(PyExc_ValueError, "setting attribute on read-only struct");
        return -1;
    }
    return 0;
}


/* struct member getter/setter for [u]int(8|16|32)_t */

static PyObject *
struct_get_uint8(StructObject *self, void *closure)
{
    return PyLong_FromLong(*((uint8_t*)((char*)self->data + (int)closure) ));
}

static int
struct_set_uint8(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (struct_check_readonly(self)) return -1;
    if (long_to_int(value, &v, 0, 255)) return -1;
    
    *((uint8_t*)((char*)self->data + (int)closure) ) = v;
    return 0;
}

static PyObject *
struct_get_uint16(StructObject *self, void *closure)
{
    return PyLong_FromLong(*((uint16_t*)((char*)self->data + (int)closure) ));
}

static int
struct_set_uint16(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (struct_check_readonly(self)) return -1;
    if (long_to_int(value, &v, 0, 65535)) return -1;
    
    *((uint16_t*)((char*)self->data + (int)closure) ) = v;
    return 0;
}

static PyObject *
struct_get_uint32(StructObject *self, void *closure)
{
    return PyLong_FromLong(*((uint32_t*)((char*)self->data + (int)closure) ));
}

static int
struct_set_uint32(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (struct_check_readonly(self)) return -1;
    if (long_to_int(value, &v, 0, 4294967295)) return -1;
    
    *((uint32_t*)((char*)self->data + (int)closure) ) = v;
    return 0;
}

static PyObject *
struct_get_int8(StructObject *self, void *closure)
{
    return PyLong_FromLong(*((int8_t*)((char*)self->data + (int)closure) ));
}

static int
struct_set_int8(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (struct_check_readonly(self)) return -1;
    if (long_to_int(value, &v, -128, 127)) return -1;
    
    *((int8_t*)((char*)self->data + (int)closure) ) = v;
    return 0;
}

static PyObject *
struct_get_int16(StructObject *self, void *closure)
{
    return PyLong_FromLong(*((int16_t*)((char*)self->data + (int)closure) ));
}

static int
struct_set_int16(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (struct_check_readonly(self)) return -1;
    if (long_to_int(value, &v, -32768, 32767)) return -1;
    
    *((int16_t*)((char*)self->data + (int)closure) ) = v;
    return 0;
}

static PyObject *
struct_get_int32(StructObject *self, void *closure)
{
    return PyLong_FromLong(*((int32_t*)((char*)self->data + (int)closure) ));
}

static int
struct_set_int32(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (struct_check_readonly(self)) return -1;
    if (long_to_int(value, &v, -2147483648, 2147483647)) return -1;
    
    *((int32_t*)((char*)self->data + (int)closure) ) = v;
    return 0;
}


/* struct member getter/setter for type 'struct' for sub-structs and unions */
typedef struct {
  PyTypeObject *type;
  size_t offset;
  size_t size;
} struct_closure_t;

static PyObject *
struct_get_struct(StructObject *self, struct_closure_t *closure) {
    StructObject *ret;    
    ret = (StructObject*)PyObject_New(StructObject, closure->type);
    if (ret) {
        ret->owner = self->owner;
        if (self->owner) Py_INCREF(self->owner); // owner could be NULL if data is C global
        ret->data = self->data + closure->offset;
        ret->size = closure->size;
        ret->readonly = self->readonly;
    }
    return (PyObject*)ret;

}


/* Generic setter for atrributes which are a struct
 *
 * Setting can be via either an object of the same type, or via a dict,
 * which could be passed as a keyword argument dict to a constructor of the struct
 * for the appropriate type
 *
 * NOTE: if setting items via a dict fails, some items may have been set already
 */
static int
struct_set_struct(StructObject *self, PyObject *value, struct_closure_t *closure) {

    PyObject *attr = NULL;
    
    if (struct_check_readonly(self)) return -1;

    
    if (PyDict_Check(value)) {
        // Set attribute sub-items from dictionary items
    
        // get a struct object for the attribute we are setting
        attr = struct_get_struct(self, closure);
        if (!attr) return -1;
        
        // Iterate over the value dictionary
        PyObject *dict_key, *dict_value;
        Py_ssize_t pos = 0;
        
        while (PyDict_Next(value, &pos, &dict_key, &dict_value)) {
            // Set the attribute on the attr attribute
            if (PyObject_SetAttr(attr, dict_key, dict_value)) {
                Py_DECREF(attr);
                return -1;
            }
        }  
        
        Py_DECREF(attr);
        return 0;
        
    }
    
    
    int isinstance = PyObject_IsInstance(value, (PyObject *)closure->type);
    
    if (isinstance == -1) return -1; // error in PyObject_IsInstance
    if (!isinstance) {
        PyErr_Format(PyExc_TypeError, "value should be an instance of '%s' or a dict", closure->type->tp_name);
        return -1;
    }
    if(closure->size != ((StructObject *)value)->size) {
        // Should only happen in case of Blob type; but just check always to be sure
        PyErr_SetString(PyExc_ValueError, "data size mismatch");
        return -1;
    }
    memcpy(self->data + closure->offset, ((StructObject *)value)->data, closure->size);
    
    return 0;
}


static int
struct_init(StructObject *self, PyObject *args, PyObject *kwds, PyTypeObject *type, size_t size) 
{
    StructObject *copy = NULL;
    // copy is a positional-only argument
    if (!PyArg_ParseTuple(args, "|O!", type, &copy)) return -1;
    
    self->size = size;
    self->data = PyMem_Malloc(size);
    if (!self->data) return -1;
    self->readonly = 0;
    
    Struct_register(self);
    
    if (copy) {
        assert(self->size == copy->size); // should be same size, since same type
        memcpy(self->data, copy->data, self->size);
    } else {
        memset(self->data, 0, self->size);
    }
    
    self->owner = (PyObject *)self;

    if (kwds) {
        // all keyword arguments are attribute-assignments
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        
        while (PyDict_Next(kwds, &pos, &key, &value)) {
            if (PyObject_SetAttr((PyObject*)self, key, value)) return -1;
        }   
    }

    return 0;
}


static int
pylv_mem_monitor_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_mem_monitor_t_Type, sizeof(lv_mem_monitor_t));
}

static PyGetSetDef pylv_mem_monitor_t_getset[] = {
    {"total_size", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t total_size", (void*)offsetof(lv_mem_monitor_t, total_size)},
    {"free_cnt", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t free_cnt", (void*)offsetof(lv_mem_monitor_t, free_cnt)},
    {"free_size", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t free_size", (void*)offsetof(lv_mem_monitor_t, free_size)},
    {"free_biggest_size", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t free_biggest_size", (void*)offsetof(lv_mem_monitor_t, free_biggest_size)},
    {"used_cnt", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t used_cnt", (void*)offsetof(lv_mem_monitor_t, used_cnt)},
    {"max_used", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t max_used", (void*)offsetof(lv_mem_monitor_t, max_used)},
    {"used_pct", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t used_pct", (void*)offsetof(lv_mem_monitor_t, used_pct)},
    {"frag_pct", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t frag_pct", (void*)offsetof(lv_mem_monitor_t, frag_pct)},
    {NULL}
};


static PyTypeObject pylv_mem_monitor_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.mem_monitor_t",
    .tp_doc = "lvgl mem_monitor_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_mem_monitor_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_mem_monitor_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_mem_monitor_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_mem_monitor_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_mem_monitor_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_mem_monitor_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_mem_buf_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_mem_buf_t_Type, sizeof(lv_mem_buf_t));
}



static PyObject *
get_struct_bitfield_mem_buf_t_used(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_mem_buf_t*)(self->data))->used );
}

static int
set_struct_bitfield_mem_buf_t_used(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_mem_buf_t*)(self->data))->used = v;
    return 0;
}

static PyGetSetDef pylv_mem_buf_t_getset[] = {
    {"p", (getter) struct_get_struct, (setter) struct_set_struct, "void p", & ((struct_closure_t){ &Blob_Type, offsetof(lv_mem_buf_t, p), sizeof(((lv_mem_buf_t *)0)->p)})},
    {"size", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t size", (void*)offsetof(lv_mem_buf_t, size)},
    {"used", (getter) get_struct_bitfield_mem_buf_t_used, (setter) set_struct_bitfield_mem_buf_t_used, "uint8_t:1 used", NULL},
    {NULL}
};


static PyTypeObject pylv_mem_buf_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.mem_buf_t",
    .tp_doc = "lvgl mem_buf_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_mem_buf_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_mem_buf_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_mem_buf_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_mem_buf_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_mem_buf_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_mem_buf_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_ll_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_ll_t_Type, sizeof(lv_ll_t));
}

static PyGetSetDef pylv_ll_t_getset[] = {
    {"n_size", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t n_size", (void*)offsetof(lv_ll_t, n_size)},
    {"head", (getter) struct_get_struct, (setter) struct_set_struct, "lv_ll_node_t head", & ((struct_closure_t){ &Blob_Type, offsetof(lv_ll_t, head), sizeof(((lv_ll_t *)0)->head)})},
    {"tail", (getter) struct_get_struct, (setter) struct_set_struct, "lv_ll_node_t tail", & ((struct_closure_t){ &Blob_Type, offsetof(lv_ll_t, tail), sizeof(((lv_ll_t *)0)->tail)})},
    {NULL}
};


static PyTypeObject pylv_ll_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.ll_t",
    .tp_doc = "lvgl ll_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_ll_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_ll_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_ll_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_ll_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_ll_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_ll_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_task_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_task_t_Type, sizeof(lv_task_t));
}



static PyObject *
get_struct_bitfield_task_t_prio(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_task_t*)(self->data))->prio );
}

static int
set_struct_bitfield_task_t_prio(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 7)) return -1;
    ((lv_task_t*)(self->data))->prio = v;
    return 0;
}

static PyGetSetDef pylv_task_t_getset[] = {
    {"period", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t period", (void*)offsetof(lv_task_t, period)},
    {"last_run", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t last_run", (void*)offsetof(lv_task_t, last_run)},
    {"task_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_task_cb_t task_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_task_t, task_cb), sizeof(((lv_task_t *)0)->task_cb)})},
    {"user_data", (getter) struct_get_struct, (setter) struct_set_struct, "void user_data", & ((struct_closure_t){ &Blob_Type, offsetof(lv_task_t, user_data), sizeof(((lv_task_t *)0)->user_data)})},
    {"repeat_count", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t repeat_count", (void*)offsetof(lv_task_t, repeat_count)},
    {"prio", (getter) get_struct_bitfield_task_t_prio, (setter) set_struct_bitfield_task_t_prio, "uint8_t:3 prio", NULL},
    {NULL}
};


static PyTypeObject pylv_task_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.task_t",
    .tp_doc = "lvgl task_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_task_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_task_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_task_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_task_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_task_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_task_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_sqrt_res_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_sqrt_res_t_Type, sizeof(lv_sqrt_res_t));
}

static PyGetSetDef pylv_sqrt_res_t_getset[] = {
    {"i", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t i", (void*)offsetof(lv_sqrt_res_t, i)},
    {"f", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t f", (void*)offsetof(lv_sqrt_res_t, f)},
    {NULL}
};


static PyTypeObject pylv_sqrt_res_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.sqrt_res_t",
    .tp_doc = "lvgl sqrt_res_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_sqrt_res_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_sqrt_res_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_sqrt_res_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_sqrt_res_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_sqrt_res_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_sqrt_res_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_color1_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_color1_t_Type, sizeof(lv_color1_t));
}

static PyGetSetDef pylv_color1_t_getset[] = {
    {"full", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t full", (void*)offsetof(lv_color1_t, full)},
    {"ch", (getter) struct_get_struct, (setter) struct_set_struct, "union  {   uint8_t blue : 1;   uint8_t green : 1;   uint8_t red : 1; } ch", & ((struct_closure_t){ &pylv_color1_t_ch_Type, offsetof(lv_color1_t, ch), sizeof(((lv_color1_t *)0)->ch)})},
    {NULL}
};


static PyTypeObject pylv_color1_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.color1_t",
    .tp_doc = "lvgl color1_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_color1_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_color1_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_color1_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_color1_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_color1_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_color1_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_color8_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_color8_t_Type, sizeof(lv_color8_t));
}

static PyGetSetDef pylv_color8_t_getset[] = {
    {"ch", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   uint8_t blue : 2;   uint8_t green : 3;   uint8_t red : 3; } ch", & ((struct_closure_t){ &pylv_color8_t_ch_Type, offsetof(lv_color8_t, ch), sizeof(((lv_color8_t *)0)->ch)})},
    {"full", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t full", (void*)offsetof(lv_color8_t, full)},
    {NULL}
};


static PyTypeObject pylv_color8_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.color8_t",
    .tp_doc = "lvgl color8_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_color8_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_color8_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_color8_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_color8_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_color8_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_color8_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_color16_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_color16_t_Type, sizeof(lv_color16_t));
}

static PyGetSetDef pylv_color16_t_getset[] = {
    {"ch", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   uint16_t blue : 5;   uint16_t green : 6;   uint16_t red : 5; } ch", & ((struct_closure_t){ &pylv_color16_t_ch_Type, offsetof(lv_color16_t, ch), sizeof(((lv_color16_t *)0)->ch)})},
    {"full", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t full", (void*)offsetof(lv_color16_t, full)},
    {NULL}
};


static PyTypeObject pylv_color16_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.color16_t",
    .tp_doc = "lvgl color16_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_color16_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_color16_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_color16_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_color16_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_color16_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_color16_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_color32_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_color32_t_Type, sizeof(lv_color32_t));
}

static PyGetSetDef pylv_color32_t_getset[] = {
    {"ch", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   uint8_t blue;   uint8_t green;   uint8_t red;   uint8_t alpha; } ch", & ((struct_closure_t){ &pylv_color32_t_ch_Type, offsetof(lv_color32_t, ch), sizeof(((lv_color32_t *)0)->ch)})},
    {"full", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t full", (void*)offsetof(lv_color32_t, full)},
    {NULL}
};


static PyTypeObject pylv_color32_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.color32_t",
    .tp_doc = "lvgl color32_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_color32_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_color32_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_color32_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_color32_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_color32_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_color32_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_color_hsv_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_color_hsv_t_Type, sizeof(lv_color_hsv_t));
}

static PyGetSetDef pylv_color_hsv_t_getset[] = {
    {"h", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t h", (void*)offsetof(lv_color_hsv_t, h)},
    {"s", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t s", (void*)offsetof(lv_color_hsv_t, s)},
    {"v", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t v", (void*)offsetof(lv_color_hsv_t, v)},
    {NULL}
};


static PyTypeObject pylv_color_hsv_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.color_hsv_t",
    .tp_doc = "lvgl color_hsv_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_color_hsv_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_color_hsv_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_color_hsv_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_color_hsv_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_color_hsv_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_color_hsv_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_point_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_point_t_Type, sizeof(lv_point_t));
}

static PyGetSetDef pylv_point_t_getset[] = {
    {"x", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t x", (void*)offsetof(lv_point_t, x)},
    {"y", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t y", (void*)offsetof(lv_point_t, y)},
    {NULL}
};


static PyTypeObject pylv_point_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.point_t",
    .tp_doc = "lvgl point_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_point_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_point_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_point_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_point_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_point_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_point_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_area_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_area_t_Type, sizeof(lv_area_t));
}

static PyGetSetDef pylv_area_t_getset[] = {
    {"x1", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t x1", (void*)offsetof(lv_area_t, x1)},
    {"y1", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t y1", (void*)offsetof(lv_area_t, y1)},
    {"x2", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t x2", (void*)offsetof(lv_area_t, x2)},
    {"y2", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t y2", (void*)offsetof(lv_area_t, y2)},
    {NULL}
};


static PyTypeObject pylv_area_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.area_t",
    .tp_doc = "lvgl area_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_area_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_area_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_area_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_area_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_area_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_area_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_disp_buf_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_disp_buf_t_Type, sizeof(lv_disp_buf_t));
}



static PyObject *
get_struct_bitfield_disp_buf_t_last_area(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_disp_buf_t*)(self->data))->last_area );
}

static int
set_struct_bitfield_disp_buf_t_last_area(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_disp_buf_t*)(self->data))->last_area = v;
    return 0;
}



static PyObject *
get_struct_bitfield_disp_buf_t_last_part(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_disp_buf_t*)(self->data))->last_part );
}

static int
set_struct_bitfield_disp_buf_t_last_part(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_disp_buf_t*)(self->data))->last_part = v;
    return 0;
}

static PyGetSetDef pylv_disp_buf_t_getset[] = {
    {"buf1", (getter) struct_get_struct, (setter) struct_set_struct, "void buf1", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_buf_t, buf1), sizeof(((lv_disp_buf_t *)0)->buf1)})},
    {"buf2", (getter) struct_get_struct, (setter) struct_set_struct, "void buf2", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_buf_t, buf2), sizeof(((lv_disp_buf_t *)0)->buf2)})},
    {"buf_act", (getter) struct_get_struct, (setter) struct_set_struct, "void buf_act", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_buf_t, buf_act), sizeof(((lv_disp_buf_t *)0)->buf_act)})},
    {"size", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t size", (void*)offsetof(lv_disp_buf_t, size)},
    {"area", (getter) struct_get_struct, (setter) struct_set_struct, "lv_area_t area", & ((struct_closure_t){ &pylv_area_t_Type, offsetof(lv_disp_buf_t, area), sizeof(lv_area_t)})},
    {"flushing", (getter) struct_get_struct, (setter) struct_set_struct, "int flushing", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_buf_t, flushing), sizeof(((lv_disp_buf_t *)0)->flushing)})},
    {"flushing_last", (getter) struct_get_struct, (setter) struct_set_struct, "int flushing_last", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_buf_t, flushing_last), sizeof(((lv_disp_buf_t *)0)->flushing_last)})},
    {"last_area", (getter) get_struct_bitfield_disp_buf_t_last_area, (setter) set_struct_bitfield_disp_buf_t_last_area, "uint32_t:1 last_area", NULL},
    {"last_part", (getter) get_struct_bitfield_disp_buf_t_last_part, (setter) set_struct_bitfield_disp_buf_t_last_part, "uint32_t:1 last_part", NULL},
    {NULL}
};


static PyTypeObject pylv_disp_buf_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.disp_buf_t",
    .tp_doc = "lvgl disp_buf_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_disp_buf_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_disp_buf_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_disp_buf_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_disp_buf_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_disp_buf_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_disp_buf_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_disp_drv_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_disp_drv_t_Type, sizeof(lv_disp_drv_t));
}



static PyObject *
get_struct_bitfield_disp_drv_t_antialiasing(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_disp_drv_t*)(self->data))->antialiasing );
}

static int
set_struct_bitfield_disp_drv_t_antialiasing(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_disp_drv_t*)(self->data))->antialiasing = v;
    return 0;
}



static PyObject *
get_struct_bitfield_disp_drv_t_rotated(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_disp_drv_t*)(self->data))->rotated );
}

static int
set_struct_bitfield_disp_drv_t_rotated(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_disp_drv_t*)(self->data))->rotated = v;
    return 0;
}



static PyObject *
get_struct_bitfield_disp_drv_t_sw_rotate(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_disp_drv_t*)(self->data))->sw_rotate );
}

static int
set_struct_bitfield_disp_drv_t_sw_rotate(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_disp_drv_t*)(self->data))->sw_rotate = v;
    return 0;
}



static PyObject *
get_struct_bitfield_disp_drv_t_dpi(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_disp_drv_t*)(self->data))->dpi );
}

static int
set_struct_bitfield_disp_drv_t_dpi(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1023)) return -1;
    ((lv_disp_drv_t*)(self->data))->dpi = v;
    return 0;
}

static PyGetSetDef pylv_disp_drv_t_getset[] = {
    {"hor_res", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t hor_res", (void*)offsetof(lv_disp_drv_t, hor_res)},
    {"ver_res", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t ver_res", (void*)offsetof(lv_disp_drv_t, ver_res)},
    {"buffer", (getter) struct_get_struct, (setter) struct_set_struct, "lv_disp_buf_t buffer", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_drv_t, buffer), sizeof(((lv_disp_drv_t *)0)->buffer)})},
    {"antialiasing", (getter) get_struct_bitfield_disp_drv_t_antialiasing, (setter) set_struct_bitfield_disp_drv_t_antialiasing, "uint32_t:1 antialiasing", NULL},
    {"rotated", (getter) get_struct_bitfield_disp_drv_t_rotated, (setter) set_struct_bitfield_disp_drv_t_rotated, "uint32_t:2 rotated", NULL},
    {"sw_rotate", (getter) get_struct_bitfield_disp_drv_t_sw_rotate, (setter) set_struct_bitfield_disp_drv_t_sw_rotate, "uint32_t:1 sw_rotate", NULL},
    {"dpi", (getter) get_struct_bitfield_disp_drv_t_dpi, (setter) set_struct_bitfield_disp_drv_t_dpi, "uint32_t:10 dpi", NULL},
    {"flush_cb", (getter) struct_get_struct, (setter) struct_set_struct, "void flush_cb(struct _disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) flush_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_drv_t, flush_cb), sizeof(((lv_disp_drv_t *)0)->flush_cb)})},
    {"rounder_cb", (getter) struct_get_struct, (setter) struct_set_struct, "void rounder_cb(struct _disp_drv_t *disp_drv, lv_area_t *area) rounder_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_drv_t, rounder_cb), sizeof(((lv_disp_drv_t *)0)->rounder_cb)})},
    {"set_px_cb", (getter) struct_get_struct, (setter) struct_set_struct, "void set_px_cb(struct _disp_drv_t *disp_drv, uint8_t *buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa) set_px_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_drv_t, set_px_cb), sizeof(((lv_disp_drv_t *)0)->set_px_cb)})},
    {"monitor_cb", (getter) struct_get_struct, (setter) struct_set_struct, "void monitor_cb(struct _disp_drv_t *disp_drv, uint32_t time, uint32_t px) monitor_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_drv_t, monitor_cb), sizeof(((lv_disp_drv_t *)0)->monitor_cb)})},
    {"wait_cb", (getter) struct_get_struct, (setter) struct_set_struct, "void wait_cb(struct _disp_drv_t *disp_drv) wait_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_drv_t, wait_cb), sizeof(((lv_disp_drv_t *)0)->wait_cb)})},
    {"clean_dcache_cb", (getter) struct_get_struct, (setter) struct_set_struct, "void clean_dcache_cb(struct _disp_drv_t *disp_drv) clean_dcache_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_drv_t, clean_dcache_cb), sizeof(((lv_disp_drv_t *)0)->clean_dcache_cb)})},
    {"gpu_wait_cb", (getter) struct_get_struct, (setter) struct_set_struct, "void gpu_wait_cb(struct _disp_drv_t *disp_drv) gpu_wait_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_drv_t, gpu_wait_cb), sizeof(((lv_disp_drv_t *)0)->gpu_wait_cb)})},
    {"gpu_blend_cb", (getter) struct_get_struct, (setter) struct_set_struct, "void gpu_blend_cb(struct _disp_drv_t *disp_drv, lv_color_t *dest, const lv_color_t *src, uint32_t length, lv_opa_t opa) gpu_blend_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_drv_t, gpu_blend_cb), sizeof(((lv_disp_drv_t *)0)->gpu_blend_cb)})},
    {"gpu_fill_cb", (getter) struct_get_struct, (setter) struct_set_struct, "void gpu_fill_cb(struct _disp_drv_t *disp_drv, lv_color_t *dest_buf, lv_coord_t dest_width, const lv_area_t *fill_area, lv_color_t color) gpu_fill_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_drv_t, gpu_fill_cb), sizeof(((lv_disp_drv_t *)0)->gpu_fill_cb)})},
    {"color_chroma_key", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t color_chroma_key", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_disp_drv_t, color_chroma_key), sizeof(lv_color16_t)})},
    {"user_data", (getter) struct_get_struct, (setter) struct_set_struct, "lv_disp_drv_user_data_t user_data", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_drv_t, user_data), sizeof(((lv_disp_drv_t *)0)->user_data)})},
    {NULL}
};


static PyTypeObject pylv_disp_drv_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.disp_drv_t",
    .tp_doc = "lvgl disp_drv_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_disp_drv_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_disp_drv_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_disp_drv_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_disp_drv_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_disp_drv_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_disp_drv_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_disp_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_disp_t_Type, sizeof(lv_disp_t));
}



static PyObject *
get_struct_bitfield_disp_t_del_prev(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_disp_t*)(self->data))->del_prev );
}

static int
set_struct_bitfield_disp_t_del_prev(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_disp_t*)(self->data))->del_prev = v;
    return 0;
}



static PyObject *
get_struct_bitfield_disp_t_inv_p(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_disp_t*)(self->data))->inv_p );
}

static int
set_struct_bitfield_disp_t_inv_p(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1023)) return -1;
    ((lv_disp_t*)(self->data))->inv_p = v;
    return 0;
}

static PyGetSetDef pylv_disp_t_getset[] = {
    {"driver", (getter) struct_get_struct, (setter) struct_set_struct, "lv_disp_drv_t driver", & ((struct_closure_t){ &pylv_disp_drv_t_Type, offsetof(lv_disp_t, driver), sizeof(lv_disp_drv_t)})},
    {"refr_task", (getter) struct_get_struct, (setter) struct_set_struct, "lv_task_t refr_task", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_t, refr_task), sizeof(((lv_disp_t *)0)->refr_task)})},
    {"scr_ll", (getter) struct_get_struct, (setter) struct_set_struct, "lv_ll_t scr_ll", & ((struct_closure_t){ &pylv_ll_t_Type, offsetof(lv_disp_t, scr_ll), sizeof(lv_ll_t)})},
    {"act_scr", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_obj_t act_scr", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_t, act_scr), sizeof(((lv_disp_t *)0)->act_scr)})},
    {"prev_scr", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_obj_t prev_scr", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_t, prev_scr), sizeof(((lv_disp_t *)0)->prev_scr)})},
    {"scr_to_load", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_obj_t scr_to_load", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_t, scr_to_load), sizeof(((lv_disp_t *)0)->scr_to_load)})},
    {"top_layer", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_obj_t top_layer", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_t, top_layer), sizeof(((lv_disp_t *)0)->top_layer)})},
    {"sys_layer", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_obj_t sys_layer", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_t, sys_layer), sizeof(((lv_disp_t *)0)->sys_layer)})},
    {"del_prev", (getter) get_struct_bitfield_disp_t_del_prev, (setter) set_struct_bitfield_disp_t_del_prev, "uint8_t:1 del_prev", NULL},
    {"bg_color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t bg_color", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_disp_t, bg_color), sizeof(lv_color16_t)})},
    {"bg_img", (getter) struct_get_struct, (setter) struct_set_struct, "void bg_img", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_t, bg_img), sizeof(((lv_disp_t *)0)->bg_img)})},
    {"bg_opa", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t bg_opa", (void*)offsetof(lv_disp_t, bg_opa)},
    {"inv_areas", (getter) struct_get_struct, (setter) struct_set_struct, "lv_area_t32 inv_areas", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_t, inv_areas), sizeof(((lv_disp_t *)0)->inv_areas)})},
    {"inv_area_joined", (getter) struct_get_struct, (setter) struct_set_struct, "uint8_t32 inv_area_joined", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_t, inv_area_joined), sizeof(((lv_disp_t *)0)->inv_area_joined)})},
    {"inv_p", (getter) get_struct_bitfield_disp_t_inv_p, (setter) set_struct_bitfield_disp_t_inv_p, "uint32_t:10 inv_p", NULL},
    {"last_activity_time", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t last_activity_time", (void*)offsetof(lv_disp_t, last_activity_time)},
    {NULL}
};


static PyTypeObject pylv_disp_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.disp_t",
    .tp_doc = "lvgl disp_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_disp_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_disp_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_disp_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_disp_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_disp_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_disp_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_indev_data_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_indev_data_t_Type, sizeof(lv_indev_data_t));
}

static PyGetSetDef pylv_indev_data_t_getset[] = {
    {"point", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t point", & ((struct_closure_t){ &pylv_point_t_Type, offsetof(lv_indev_data_t, point), sizeof(lv_point_t)})},
    {"key", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t key", (void*)offsetof(lv_indev_data_t, key)},
    {"btn_id", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t btn_id", (void*)offsetof(lv_indev_data_t, btn_id)},
    {"enc_diff", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t enc_diff", (void*)offsetof(lv_indev_data_t, enc_diff)},
    {"state", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_indev_state_t state", (void*)offsetof(lv_indev_data_t, state)},
    {NULL}
};


static PyTypeObject pylv_indev_data_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.indev_data_t",
    .tp_doc = "lvgl indev_data_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_indev_data_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_indev_data_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_indev_data_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_indev_data_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_indev_data_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_indev_data_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_indev_drv_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_indev_drv_t_Type, sizeof(lv_indev_drv_t));
}

static PyGetSetDef pylv_indev_drv_t_getset[] = {
    {"type", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_indev_type_t type", (void*)offsetof(lv_indev_drv_t, type)},
    {"read_cb", (getter) struct_get_struct, (setter) struct_set_struct, "bool read_cb(struct _lv_indev_drv_t *indev_drv, lv_indev_data_t *data) read_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_indev_drv_t, read_cb), sizeof(((lv_indev_drv_t *)0)->read_cb)})},
    {"feedback_cb", (getter) struct_get_struct, (setter) struct_set_struct, "void feedback_cb(struct _lv_indev_drv_t *, uint8_t) feedback_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_indev_drv_t, feedback_cb), sizeof(((lv_indev_drv_t *)0)->feedback_cb)})},
    {"user_data", (getter) struct_get_struct, (setter) struct_set_struct, "lv_indev_drv_user_data_t user_data", & ((struct_closure_t){ &Blob_Type, offsetof(lv_indev_drv_t, user_data), sizeof(((lv_indev_drv_t *)0)->user_data)})},
    {"disp", (getter) struct_get_struct, (setter) struct_set_struct, "struct _disp_t disp", & ((struct_closure_t){ &Blob_Type, offsetof(lv_indev_drv_t, disp), sizeof(((lv_indev_drv_t *)0)->disp)})},
    {"read_task", (getter) struct_get_struct, (setter) struct_set_struct, "lv_task_t read_task", & ((struct_closure_t){ &Blob_Type, offsetof(lv_indev_drv_t, read_task), sizeof(((lv_indev_drv_t *)0)->read_task)})},
    {"drag_limit", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t drag_limit", (void*)offsetof(lv_indev_drv_t, drag_limit)},
    {"drag_throw", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t drag_throw", (void*)offsetof(lv_indev_drv_t, drag_throw)},
    {"gesture_min_velocity", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t gesture_min_velocity", (void*)offsetof(lv_indev_drv_t, gesture_min_velocity)},
    {"gesture_limit", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t gesture_limit", (void*)offsetof(lv_indev_drv_t, gesture_limit)},
    {"long_press_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t long_press_time", (void*)offsetof(lv_indev_drv_t, long_press_time)},
    {"long_press_rep_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t long_press_rep_time", (void*)offsetof(lv_indev_drv_t, long_press_rep_time)},
    {NULL}
};


static PyTypeObject pylv_indev_drv_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.indev_drv_t",
    .tp_doc = "lvgl indev_drv_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_indev_drv_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_indev_drv_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_indev_drv_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_indev_drv_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_indev_drv_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_indev_drv_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_indev_proc_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_indev_proc_t_Type, sizeof(lv_indev_proc_t));
}



static PyObject *
get_struct_bitfield_indev_proc_t_long_pr_sent(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_indev_proc_t*)(self->data))->long_pr_sent );
}

static int
set_struct_bitfield_indev_proc_t_long_pr_sent(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_indev_proc_t*)(self->data))->long_pr_sent = v;
    return 0;
}



static PyObject *
get_struct_bitfield_indev_proc_t_reset_query(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_indev_proc_t*)(self->data))->reset_query );
}

static int
set_struct_bitfield_indev_proc_t_reset_query(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_indev_proc_t*)(self->data))->reset_query = v;
    return 0;
}



static PyObject *
get_struct_bitfield_indev_proc_t_disabled(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_indev_proc_t*)(self->data))->disabled );
}

static int
set_struct_bitfield_indev_proc_t_disabled(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_indev_proc_t*)(self->data))->disabled = v;
    return 0;
}



static PyObject *
get_struct_bitfield_indev_proc_t_wait_until_release(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_indev_proc_t*)(self->data))->wait_until_release );
}

static int
set_struct_bitfield_indev_proc_t_wait_until_release(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_indev_proc_t*)(self->data))->wait_until_release = v;
    return 0;
}

static PyGetSetDef pylv_indev_proc_t_getset[] = {
    {"state", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_indev_state_t state", (void*)offsetof(lv_indev_proc_t, state)},
    {"types", (getter) struct_get_struct, (setter) struct_set_struct, "union  {   struct    {     lv_point_t act_point;     lv_point_t last_point;     lv_point_t vect;     lv_point_t drag_sum;     lv_point_t drag_throw_vect;     struct _lv_obj_t *act_obj;     struct _lv_obj_t *last_obj;     struct _lv_obj_t *last_pressed;     lv_gesture_dir_t gesture_dir;     lv_point_t gesture_sum;     uint8_t drag_limit_out : 1;     uint8_t drag_in_prog : 1;     lv_drag_dir_t drag_dir : 3;     uint8_t gesture_sent : 1;   } pointer;   struct    {     lv_indev_state_t last_state;     uint32_t last_key;   } keypad; } types", & ((struct_closure_t){ &pylv_indev_proc_t_types_Type, offsetof(lv_indev_proc_t, types), sizeof(((lv_indev_proc_t *)0)->types)})},
    {"pr_timestamp", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t pr_timestamp", (void*)offsetof(lv_indev_proc_t, pr_timestamp)},
    {"longpr_rep_timestamp", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t longpr_rep_timestamp", (void*)offsetof(lv_indev_proc_t, longpr_rep_timestamp)},
    {"long_pr_sent", (getter) get_struct_bitfield_indev_proc_t_long_pr_sent, (setter) set_struct_bitfield_indev_proc_t_long_pr_sent, "uint8_t:1 long_pr_sent", NULL},
    {"reset_query", (getter) get_struct_bitfield_indev_proc_t_reset_query, (setter) set_struct_bitfield_indev_proc_t_reset_query, "uint8_t:1 reset_query", NULL},
    {"disabled", (getter) get_struct_bitfield_indev_proc_t_disabled, (setter) set_struct_bitfield_indev_proc_t_disabled, "uint8_t:1 disabled", NULL},
    {"wait_until_release", (getter) get_struct_bitfield_indev_proc_t_wait_until_release, (setter) set_struct_bitfield_indev_proc_t_wait_until_release, "uint8_t:1 wait_until_release", NULL},
    {NULL}
};


static PyTypeObject pylv_indev_proc_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.indev_proc_t",
    .tp_doc = "lvgl indev_proc_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_indev_proc_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_indev_proc_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_indev_proc_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_indev_proc_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_indev_proc_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_indev_proc_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_indev_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_indev_t_Type, sizeof(lv_indev_t));
}

static PyGetSetDef pylv_indev_t_getset[] = {
    {"driver", (getter) struct_get_struct, (setter) struct_set_struct, "lv_indev_drv_t driver", & ((struct_closure_t){ &pylv_indev_drv_t_Type, offsetof(lv_indev_t, driver), sizeof(lv_indev_drv_t)})},
    {"proc", (getter) struct_get_struct, (setter) struct_set_struct, "lv_indev_proc_t proc", & ((struct_closure_t){ &pylv_indev_proc_t_Type, offsetof(lv_indev_t, proc), sizeof(lv_indev_proc_t)})},
    {"cursor", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_obj_t cursor", & ((struct_closure_t){ &Blob_Type, offsetof(lv_indev_t, cursor), sizeof(((lv_indev_t *)0)->cursor)})},
    {"group", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_group_t group", & ((struct_closure_t){ &Blob_Type, offsetof(lv_indev_t, group), sizeof(((lv_indev_t *)0)->group)})},
    {"btn_points", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t btn_points", & ((struct_closure_t){ &Blob_Type, offsetof(lv_indev_t, btn_points), sizeof(((lv_indev_t *)0)->btn_points)})},
    {NULL}
};


static PyTypeObject pylv_indev_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.indev_t",
    .tp_doc = "lvgl indev_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_indev_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_indev_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_indev_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_indev_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_indev_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_indev_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_font_glyph_dsc_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_font_glyph_dsc_t_Type, sizeof(lv_font_glyph_dsc_t));
}

static PyGetSetDef pylv_font_glyph_dsc_t_getset[] = {
    {"adv_w", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t adv_w", (void*)offsetof(lv_font_glyph_dsc_t, adv_w)},
    {"box_w", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t box_w", (void*)offsetof(lv_font_glyph_dsc_t, box_w)},
    {"box_h", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t box_h", (void*)offsetof(lv_font_glyph_dsc_t, box_h)},
    {"ofs_x", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t ofs_x", (void*)offsetof(lv_font_glyph_dsc_t, ofs_x)},
    {"ofs_y", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t ofs_y", (void*)offsetof(lv_font_glyph_dsc_t, ofs_y)},
    {"bpp", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t bpp", (void*)offsetof(lv_font_glyph_dsc_t, bpp)},
    {NULL}
};


static PyTypeObject pylv_font_glyph_dsc_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.font_glyph_dsc_t",
    .tp_doc = "lvgl font_glyph_dsc_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_font_glyph_dsc_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_font_glyph_dsc_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_font_glyph_dsc_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_font_glyph_dsc_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_font_glyph_dsc_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_font_glyph_dsc_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_font_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_font_t_Type, sizeof(lv_font_t));
}



static PyObject *
get_struct_bitfield_font_t_subpx(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_font_t*)(self->data))->subpx );
}

static int
set_struct_bitfield_font_t_subpx(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_font_t*)(self->data))->subpx = v;
    return 0;
}

static PyGetSetDef pylv_font_t_getset[] = {
    {"get_glyph_dsc", (getter) struct_get_struct, (setter) struct_set_struct, "bool get_glyph_dsc(const struct _lv_font_struct *, lv_font_glyph_dsc_t *, uint32_t letter, uint32_t letter_next) get_glyph_dsc", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_t, get_glyph_dsc), sizeof(((lv_font_t *)0)->get_glyph_dsc)})},
    {"get_glyph_bitmap", (getter) struct_get_struct, (setter) struct_set_struct, "const uint8_t *get_glyph_bitmap(const struct _lv_font_struct *, uint32_t) get_glyph_bitmap", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_t, get_glyph_bitmap), sizeof(((lv_font_t *)0)->get_glyph_bitmap)})},
    {"line_height", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t line_height", (void*)offsetof(lv_font_t, line_height)},
    {"base_line", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t base_line", (void*)offsetof(lv_font_t, base_line)},
    {"subpx", (getter) get_struct_bitfield_font_t_subpx, (setter) set_struct_bitfield_font_t_subpx, "uint8_t:2 subpx", NULL},
    {"underline_position", (getter) struct_get_int8, (setter) struct_set_int8, "int8_t underline_position", (void*)offsetof(lv_font_t, underline_position)},
    {"underline_thickness", (getter) struct_get_int8, (setter) struct_set_int8, "int8_t underline_thickness", (void*)offsetof(lv_font_t, underline_thickness)},
    {"dsc", (getter) struct_get_struct, (setter) struct_set_struct, "void dsc", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_t, dsc), sizeof(((lv_font_t *)0)->dsc)})},
    {"user_data", (getter) struct_get_struct, (setter) struct_set_struct, "lv_font_user_data_t user_data", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_t, user_data), sizeof(((lv_font_t *)0)->user_data)})},
    {NULL}
};


static PyTypeObject pylv_font_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.font_t",
    .tp_doc = "lvgl font_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_font_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_font_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_font_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_font_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_font_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_font_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_anim_path_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_anim_path_t_Type, sizeof(lv_anim_path_t));
}

static PyGetSetDef pylv_anim_path_t_getset[] = {
    {"cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_anim_path_cb_t cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_anim_path_t, cb), sizeof(((lv_anim_path_t *)0)->cb)})},
    {"user_data", (getter) struct_get_struct, (setter) struct_set_struct, "void user_data", & ((struct_closure_t){ &Blob_Type, offsetof(lv_anim_path_t, user_data), sizeof(((lv_anim_path_t *)0)->user_data)})},
    {NULL}
};


static PyTypeObject pylv_anim_path_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.anim_path_t",
    .tp_doc = "lvgl anim_path_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_anim_path_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_anim_path_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_anim_path_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_anim_path_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_anim_path_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_anim_path_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_anim_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_anim_t_Type, sizeof(lv_anim_t));
}



static PyObject *
get_struct_bitfield_anim_t_early_apply(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_anim_t*)(self->data))->early_apply );
}

static int
set_struct_bitfield_anim_t_early_apply(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_anim_t*)(self->data))->early_apply = v;
    return 0;
}



static PyObject *
get_struct_bitfield_anim_t_playback_now(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_anim_t*)(self->data))->playback_now );
}

static int
set_struct_bitfield_anim_t_playback_now(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_anim_t*)(self->data))->playback_now = v;
    return 0;
}



static PyObject *
get_struct_bitfield_anim_t_run_round(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_anim_t*)(self->data))->run_round );
}

static int
set_struct_bitfield_anim_t_run_round(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_anim_t*)(self->data))->run_round = v;
    return 0;
}

static PyGetSetDef pylv_anim_t_getset[] = {
    {"var", (getter) struct_get_struct, (setter) struct_set_struct, "void var", & ((struct_closure_t){ &Blob_Type, offsetof(lv_anim_t, var), sizeof(((lv_anim_t *)0)->var)})},
    {"exec_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_anim_exec_xcb_t exec_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_anim_t, exec_cb), sizeof(((lv_anim_t *)0)->exec_cb)})},
    {"start_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_anim_start_cb_t start_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_anim_t, start_cb), sizeof(((lv_anim_t *)0)->start_cb)})},
    {"ready_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_anim_ready_cb_t ready_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_anim_t, ready_cb), sizeof(((lv_anim_t *)0)->ready_cb)})},
    {"user_data", (getter) struct_get_struct, (setter) struct_set_struct, "lv_anim_user_data_t user_data", & ((struct_closure_t){ &Blob_Type, offsetof(lv_anim_t, user_data), sizeof(((lv_anim_t *)0)->user_data)})},
    {"path", (getter) struct_get_struct, (setter) struct_set_struct, "lv_anim_path_t path", & ((struct_closure_t){ &pylv_anim_path_t_Type, offsetof(lv_anim_t, path), sizeof(lv_anim_path_t)})},
    {"start", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t start", (void*)offsetof(lv_anim_t, start)},
    {"current", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t current", (void*)offsetof(lv_anim_t, current)},
    {"end", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t end", (void*)offsetof(lv_anim_t, end)},
    {"time", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t time", (void*)offsetof(lv_anim_t, time)},
    {"act_time", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t act_time", (void*)offsetof(lv_anim_t, act_time)},
    {"playback_delay", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t playback_delay", (void*)offsetof(lv_anim_t, playback_delay)},
    {"playback_time", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t playback_time", (void*)offsetof(lv_anim_t, playback_time)},
    {"repeat_delay", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t repeat_delay", (void*)offsetof(lv_anim_t, repeat_delay)},
    {"repeat_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t repeat_cnt", (void*)offsetof(lv_anim_t, repeat_cnt)},
    {"early_apply", (getter) get_struct_bitfield_anim_t_early_apply, (setter) set_struct_bitfield_anim_t_early_apply, "uint8_t:1 early_apply", NULL},
    {"playback_now", (getter) get_struct_bitfield_anim_t_playback_now, (setter) set_struct_bitfield_anim_t_playback_now, "uint8_t:1 playback_now", NULL},
    {"run_round", (getter) get_struct_bitfield_anim_t_run_round, (setter) set_struct_bitfield_anim_t_run_round, "uint8_t:1 run_round", NULL},
    {"time_orig", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t time_orig", (void*)offsetof(lv_anim_t, time_orig)},
    {NULL}
};


static PyTypeObject pylv_anim_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.anim_t",
    .tp_doc = "lvgl anim_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_anim_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_anim_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_anim_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_anim_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_anim_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_anim_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_draw_mask_common_dsc_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_draw_mask_common_dsc_t_Type, sizeof(lv_draw_mask_common_dsc_t));
}

static PyGetSetDef pylv_draw_mask_common_dsc_t_getset[] = {
    {"cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_draw_mask_xcb_t cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_draw_mask_common_dsc_t, cb), sizeof(((lv_draw_mask_common_dsc_t *)0)->cb)})},
    {"type", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_draw_mask_type_t type", (void*)offsetof(lv_draw_mask_common_dsc_t, type)},
    {NULL}
};


static PyTypeObject pylv_draw_mask_common_dsc_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.draw_mask_common_dsc_t",
    .tp_doc = "lvgl draw_mask_common_dsc_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_draw_mask_common_dsc_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_draw_mask_common_dsc_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_draw_mask_common_dsc_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_draw_mask_common_dsc_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_draw_mask_common_dsc_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_draw_mask_common_dsc_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_draw_mask_line_param_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_draw_mask_line_param_t_Type, sizeof(lv_draw_mask_line_param_t));
}



static PyObject *
get_struct_bitfield_draw_mask_line_param_t_flat(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_draw_mask_line_param_t*)(self->data))->flat );
}

static int
set_struct_bitfield_draw_mask_line_param_t_flat(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_draw_mask_line_param_t*)(self->data))->flat = v;
    return 0;
}



static PyObject *
get_struct_bitfield_draw_mask_line_param_t_inv(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_draw_mask_line_param_t*)(self->data))->inv );
}

static int
set_struct_bitfield_draw_mask_line_param_t_inv(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_draw_mask_line_param_t*)(self->data))->inv = v;
    return 0;
}

static PyGetSetDef pylv_draw_mask_line_param_t_getset[] = {
    {"dsc", (getter) struct_get_struct, (setter) struct_set_struct, "lv_draw_mask_common_dsc_t dsc", & ((struct_closure_t){ &pylv_draw_mask_common_dsc_t_Type, offsetof(lv_draw_mask_line_param_t, dsc), sizeof(lv_draw_mask_common_dsc_t)})},
    {"cfg", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_point_t p1;   lv_point_t p2;   lv_draw_mask_line_side_t side : 2; } cfg", & ((struct_closure_t){ &pylv_draw_mask_line_param_t_cfg_Type, offsetof(lv_draw_mask_line_param_t, cfg), sizeof(((lv_draw_mask_line_param_t *)0)->cfg)})},
    {"origo", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t origo", & ((struct_closure_t){ &pylv_point_t_Type, offsetof(lv_draw_mask_line_param_t, origo), sizeof(lv_point_t)})},
    {"xy_steep", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t xy_steep", (void*)offsetof(lv_draw_mask_line_param_t, xy_steep)},
    {"yx_steep", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t yx_steep", (void*)offsetof(lv_draw_mask_line_param_t, yx_steep)},
    {"steep", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t steep", (void*)offsetof(lv_draw_mask_line_param_t, steep)},
    {"spx", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t spx", (void*)offsetof(lv_draw_mask_line_param_t, spx)},
    {"flat", (getter) get_struct_bitfield_draw_mask_line_param_t_flat, (setter) set_struct_bitfield_draw_mask_line_param_t_flat, "uint8_t:1 flat", NULL},
    {"inv", (getter) get_struct_bitfield_draw_mask_line_param_t_inv, (setter) set_struct_bitfield_draw_mask_line_param_t_inv, "uint8_t:1 inv", NULL},
    {NULL}
};


static PyTypeObject pylv_draw_mask_line_param_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.draw_mask_line_param_t",
    .tp_doc = "lvgl draw_mask_line_param_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_draw_mask_line_param_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_draw_mask_line_param_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_draw_mask_line_param_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_draw_mask_line_param_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_draw_mask_line_param_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_draw_mask_line_param_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_draw_mask_angle_param_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_draw_mask_angle_param_t_Type, sizeof(lv_draw_mask_angle_param_t));
}

static PyGetSetDef pylv_draw_mask_angle_param_t_getset[] = {
    {"dsc", (getter) struct_get_struct, (setter) struct_set_struct, "lv_draw_mask_common_dsc_t dsc", & ((struct_closure_t){ &pylv_draw_mask_common_dsc_t_Type, offsetof(lv_draw_mask_angle_param_t, dsc), sizeof(lv_draw_mask_common_dsc_t)})},
    {"cfg", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_point_t vertex_p;   lv_coord_t start_angle;   lv_coord_t end_angle; } cfg", & ((struct_closure_t){ &pylv_draw_mask_angle_param_t_cfg_Type, offsetof(lv_draw_mask_angle_param_t, cfg), sizeof(((lv_draw_mask_angle_param_t *)0)->cfg)})},
    {"start_line", (getter) struct_get_struct, (setter) struct_set_struct, "lv_draw_mask_line_param_t start_line", & ((struct_closure_t){ &pylv_draw_mask_line_param_t_Type, offsetof(lv_draw_mask_angle_param_t, start_line), sizeof(lv_draw_mask_line_param_t)})},
    {"end_line", (getter) struct_get_struct, (setter) struct_set_struct, "lv_draw_mask_line_param_t end_line", & ((struct_closure_t){ &pylv_draw_mask_line_param_t_Type, offsetof(lv_draw_mask_angle_param_t, end_line), sizeof(lv_draw_mask_line_param_t)})},
    {"delta_deg", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t delta_deg", (void*)offsetof(lv_draw_mask_angle_param_t, delta_deg)},
    {NULL}
};


static PyTypeObject pylv_draw_mask_angle_param_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.draw_mask_angle_param_t",
    .tp_doc = "lvgl draw_mask_angle_param_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_draw_mask_angle_param_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_draw_mask_angle_param_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_draw_mask_angle_param_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_draw_mask_angle_param_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_draw_mask_angle_param_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_draw_mask_angle_param_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_draw_mask_radius_param_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_draw_mask_radius_param_t_Type, sizeof(lv_draw_mask_radius_param_t));
}

static PyGetSetDef pylv_draw_mask_radius_param_t_getset[] = {
    {"dsc", (getter) struct_get_struct, (setter) struct_set_struct, "lv_draw_mask_common_dsc_t dsc", & ((struct_closure_t){ &pylv_draw_mask_common_dsc_t_Type, offsetof(lv_draw_mask_radius_param_t, dsc), sizeof(lv_draw_mask_common_dsc_t)})},
    {"cfg", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_area_t rect;   lv_coord_t radius;   uint8_t outer : 1; } cfg", & ((struct_closure_t){ &pylv_draw_mask_radius_param_t_cfg_Type, offsetof(lv_draw_mask_radius_param_t, cfg), sizeof(((lv_draw_mask_radius_param_t *)0)->cfg)})},
    {"y_prev", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t y_prev", (void*)offsetof(lv_draw_mask_radius_param_t, y_prev)},
    {"y_prev_x", (getter) struct_get_struct, (setter) struct_set_struct, "lv_sqrt_res_t y_prev_x", & ((struct_closure_t){ &pylv_sqrt_res_t_Type, offsetof(lv_draw_mask_radius_param_t, y_prev_x), sizeof(lv_sqrt_res_t)})},
    {NULL}
};


static PyTypeObject pylv_draw_mask_radius_param_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.draw_mask_radius_param_t",
    .tp_doc = "lvgl draw_mask_radius_param_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_draw_mask_radius_param_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_draw_mask_radius_param_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_draw_mask_radius_param_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_draw_mask_radius_param_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_draw_mask_radius_param_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_draw_mask_radius_param_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_draw_mask_fade_param_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_draw_mask_fade_param_t_Type, sizeof(lv_draw_mask_fade_param_t));
}

static PyGetSetDef pylv_draw_mask_fade_param_t_getset[] = {
    {"dsc", (getter) struct_get_struct, (setter) struct_set_struct, "lv_draw_mask_common_dsc_t dsc", & ((struct_closure_t){ &pylv_draw_mask_common_dsc_t_Type, offsetof(lv_draw_mask_fade_param_t, dsc), sizeof(lv_draw_mask_common_dsc_t)})},
    {"cfg", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_area_t coords;   lv_coord_t y_top;   lv_coord_t y_bottom;   lv_opa_t opa_top;   lv_opa_t opa_bottom; } cfg", & ((struct_closure_t){ &pylv_draw_mask_fade_param_t_cfg_Type, offsetof(lv_draw_mask_fade_param_t, cfg), sizeof(((lv_draw_mask_fade_param_t *)0)->cfg)})},
    {NULL}
};


static PyTypeObject pylv_draw_mask_fade_param_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.draw_mask_fade_param_t",
    .tp_doc = "lvgl draw_mask_fade_param_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_draw_mask_fade_param_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_draw_mask_fade_param_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_draw_mask_fade_param_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_draw_mask_fade_param_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_draw_mask_fade_param_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_draw_mask_fade_param_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_draw_mask_map_param_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_draw_mask_map_param_t_Type, sizeof(lv_draw_mask_map_param_t));
}

static PyGetSetDef pylv_draw_mask_map_param_t_getset[] = {
    {"dsc", (getter) struct_get_struct, (setter) struct_set_struct, "lv_draw_mask_common_dsc_t dsc", & ((struct_closure_t){ &pylv_draw_mask_common_dsc_t_Type, offsetof(lv_draw_mask_map_param_t, dsc), sizeof(lv_draw_mask_common_dsc_t)})},
    {"cfg", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_area_t coords;   const lv_opa_t *map; } cfg", & ((struct_closure_t){ &pylv_draw_mask_map_param_t_cfg_Type, offsetof(lv_draw_mask_map_param_t, cfg), sizeof(((lv_draw_mask_map_param_t *)0)->cfg)})},
    {NULL}
};


static PyTypeObject pylv_draw_mask_map_param_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.draw_mask_map_param_t",
    .tp_doc = "lvgl draw_mask_map_param_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_draw_mask_map_param_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_draw_mask_map_param_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_draw_mask_map_param_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_draw_mask_map_param_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_draw_mask_map_param_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_draw_mask_map_param_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_style_list_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_style_list_t_Type, sizeof(lv_style_list_t));
}



static PyObject *
get_struct_bitfield_style_list_t_style_cnt(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->style_cnt );
}

static int
set_struct_bitfield_style_list_t_style_cnt(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 63)) return -1;
    ((lv_style_list_t*)(self->data))->style_cnt = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_has_local(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->has_local );
}

static int
set_struct_bitfield_style_list_t_has_local(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->has_local = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_has_trans(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->has_trans );
}

static int
set_struct_bitfield_style_list_t_has_trans(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->has_trans = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_skip_trans(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->skip_trans );
}

static int
set_struct_bitfield_style_list_t_skip_trans(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->skip_trans = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_ignore_trans(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->ignore_trans );
}

static int
set_struct_bitfield_style_list_t_ignore_trans(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->ignore_trans = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_valid_cache(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->valid_cache );
}

static int
set_struct_bitfield_style_list_t_valid_cache(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->valid_cache = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_ignore_cache(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->ignore_cache );
}

static int
set_struct_bitfield_style_list_t_ignore_cache(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->ignore_cache = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_radius_zero(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->radius_zero );
}

static int
set_struct_bitfield_style_list_t_radius_zero(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->radius_zero = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_opa_scale_cover(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->opa_scale_cover );
}

static int
set_struct_bitfield_style_list_t_opa_scale_cover(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->opa_scale_cover = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_clip_corner_off(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->clip_corner_off );
}

static int
set_struct_bitfield_style_list_t_clip_corner_off(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->clip_corner_off = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_transform_all_zero(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->transform_all_zero );
}

static int
set_struct_bitfield_style_list_t_transform_all_zero(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->transform_all_zero = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_pad_all_zero(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->pad_all_zero );
}

static int
set_struct_bitfield_style_list_t_pad_all_zero(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->pad_all_zero = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_margin_all_zero(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->margin_all_zero );
}

static int
set_struct_bitfield_style_list_t_margin_all_zero(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->margin_all_zero = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_blend_mode_all_normal(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->blend_mode_all_normal );
}

static int
set_struct_bitfield_style_list_t_blend_mode_all_normal(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->blend_mode_all_normal = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_bg_opa_transp(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->bg_opa_transp );
}

static int
set_struct_bitfield_style_list_t_bg_opa_transp(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->bg_opa_transp = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_bg_opa_cover(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->bg_opa_cover );
}

static int
set_struct_bitfield_style_list_t_bg_opa_cover(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->bg_opa_cover = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_border_width_zero(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->border_width_zero );
}

static int
set_struct_bitfield_style_list_t_border_width_zero(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->border_width_zero = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_border_side_full(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->border_side_full );
}

static int
set_struct_bitfield_style_list_t_border_side_full(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->border_side_full = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_border_post_off(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->border_post_off );
}

static int
set_struct_bitfield_style_list_t_border_post_off(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->border_post_off = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_outline_width_zero(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->outline_width_zero );
}

static int
set_struct_bitfield_style_list_t_outline_width_zero(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->outline_width_zero = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_pattern_img_null(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->pattern_img_null );
}

static int
set_struct_bitfield_style_list_t_pattern_img_null(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->pattern_img_null = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_shadow_width_zero(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->shadow_width_zero );
}

static int
set_struct_bitfield_style_list_t_shadow_width_zero(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->shadow_width_zero = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_value_txt_str(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->value_txt_str );
}

static int
set_struct_bitfield_style_list_t_value_txt_str(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->value_txt_str = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_img_recolor_opa_transp(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->img_recolor_opa_transp );
}

static int
set_struct_bitfield_style_list_t_img_recolor_opa_transp(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->img_recolor_opa_transp = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_text_space_zero(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->text_space_zero );
}

static int
set_struct_bitfield_style_list_t_text_space_zero(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->text_space_zero = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_text_decor_none(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->text_decor_none );
}

static int
set_struct_bitfield_style_list_t_text_decor_none(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->text_decor_none = v;
    return 0;
}



static PyObject *
get_struct_bitfield_style_list_t_text_font_normal(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_list_t*)(self->data))->text_font_normal );
}

static int
set_struct_bitfield_style_list_t_text_font_normal(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_list_t*)(self->data))->text_font_normal = v;
    return 0;
}

static PyGetSetDef pylv_style_list_t_getset[] = {
    {"style_list", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_list", & ((struct_closure_t){ &Blob_Type, offsetof(lv_style_list_t, style_list), sizeof(((lv_style_list_t *)0)->style_list)})},
    {"style_cnt", (getter) get_struct_bitfield_style_list_t_style_cnt, (setter) set_struct_bitfield_style_list_t_style_cnt, "uint32_t:6 style_cnt", NULL},
    {"has_local", (getter) get_struct_bitfield_style_list_t_has_local, (setter) set_struct_bitfield_style_list_t_has_local, "uint32_t:1 has_local", NULL},
    {"has_trans", (getter) get_struct_bitfield_style_list_t_has_trans, (setter) set_struct_bitfield_style_list_t_has_trans, "uint32_t:1 has_trans", NULL},
    {"skip_trans", (getter) get_struct_bitfield_style_list_t_skip_trans, (setter) set_struct_bitfield_style_list_t_skip_trans, "uint32_t:1 skip_trans", NULL},
    {"ignore_trans", (getter) get_struct_bitfield_style_list_t_ignore_trans, (setter) set_struct_bitfield_style_list_t_ignore_trans, "uint32_t:1 ignore_trans", NULL},
    {"valid_cache", (getter) get_struct_bitfield_style_list_t_valid_cache, (setter) set_struct_bitfield_style_list_t_valid_cache, "uint32_t:1 valid_cache", NULL},
    {"ignore_cache", (getter) get_struct_bitfield_style_list_t_ignore_cache, (setter) set_struct_bitfield_style_list_t_ignore_cache, "uint32_t:1 ignore_cache", NULL},
    {"radius_zero", (getter) get_struct_bitfield_style_list_t_radius_zero, (setter) set_struct_bitfield_style_list_t_radius_zero, "uint32_t:1 radius_zero", NULL},
    {"opa_scale_cover", (getter) get_struct_bitfield_style_list_t_opa_scale_cover, (setter) set_struct_bitfield_style_list_t_opa_scale_cover, "uint32_t:1 opa_scale_cover", NULL},
    {"clip_corner_off", (getter) get_struct_bitfield_style_list_t_clip_corner_off, (setter) set_struct_bitfield_style_list_t_clip_corner_off, "uint32_t:1 clip_corner_off", NULL},
    {"transform_all_zero", (getter) get_struct_bitfield_style_list_t_transform_all_zero, (setter) set_struct_bitfield_style_list_t_transform_all_zero, "uint32_t:1 transform_all_zero", NULL},
    {"pad_all_zero", (getter) get_struct_bitfield_style_list_t_pad_all_zero, (setter) set_struct_bitfield_style_list_t_pad_all_zero, "uint32_t:1 pad_all_zero", NULL},
    {"margin_all_zero", (getter) get_struct_bitfield_style_list_t_margin_all_zero, (setter) set_struct_bitfield_style_list_t_margin_all_zero, "uint32_t:1 margin_all_zero", NULL},
    {"blend_mode_all_normal", (getter) get_struct_bitfield_style_list_t_blend_mode_all_normal, (setter) set_struct_bitfield_style_list_t_blend_mode_all_normal, "uint32_t:1 blend_mode_all_normal", NULL},
    {"bg_opa_transp", (getter) get_struct_bitfield_style_list_t_bg_opa_transp, (setter) set_struct_bitfield_style_list_t_bg_opa_transp, "uint32_t:1 bg_opa_transp", NULL},
    {"bg_opa_cover", (getter) get_struct_bitfield_style_list_t_bg_opa_cover, (setter) set_struct_bitfield_style_list_t_bg_opa_cover, "uint32_t:1 bg_opa_cover", NULL},
    {"border_width_zero", (getter) get_struct_bitfield_style_list_t_border_width_zero, (setter) set_struct_bitfield_style_list_t_border_width_zero, "uint32_t:1 border_width_zero", NULL},
    {"border_side_full", (getter) get_struct_bitfield_style_list_t_border_side_full, (setter) set_struct_bitfield_style_list_t_border_side_full, "uint32_t:1 border_side_full", NULL},
    {"border_post_off", (getter) get_struct_bitfield_style_list_t_border_post_off, (setter) set_struct_bitfield_style_list_t_border_post_off, "uint32_t:1 border_post_off", NULL},
    {"outline_width_zero", (getter) get_struct_bitfield_style_list_t_outline_width_zero, (setter) set_struct_bitfield_style_list_t_outline_width_zero, "uint32_t:1 outline_width_zero", NULL},
    {"pattern_img_null", (getter) get_struct_bitfield_style_list_t_pattern_img_null, (setter) set_struct_bitfield_style_list_t_pattern_img_null, "uint32_t:1 pattern_img_null", NULL},
    {"shadow_width_zero", (getter) get_struct_bitfield_style_list_t_shadow_width_zero, (setter) set_struct_bitfield_style_list_t_shadow_width_zero, "uint32_t:1 shadow_width_zero", NULL},
    {"value_txt_str", (getter) get_struct_bitfield_style_list_t_value_txt_str, (setter) set_struct_bitfield_style_list_t_value_txt_str, "uint32_t:1 value_txt_str", NULL},
    {"img_recolor_opa_transp", (getter) get_struct_bitfield_style_list_t_img_recolor_opa_transp, (setter) set_struct_bitfield_style_list_t_img_recolor_opa_transp, "uint32_t:1 img_recolor_opa_transp", NULL},
    {"text_space_zero", (getter) get_struct_bitfield_style_list_t_text_space_zero, (setter) set_struct_bitfield_style_list_t_text_space_zero, "uint32_t:1 text_space_zero", NULL},
    {"text_decor_none", (getter) get_struct_bitfield_style_list_t_text_decor_none, (setter) set_struct_bitfield_style_list_t_text_decor_none, "uint32_t:1 text_decor_none", NULL},
    {"text_font_normal", (getter) get_struct_bitfield_style_list_t_text_font_normal, (setter) set_struct_bitfield_style_list_t_text_font_normal, "uint32_t:1 text_font_normal", NULL},
    {NULL}
};


static PyTypeObject pylv_style_list_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.style_list_t",
    .tp_doc = "lvgl style_list_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_style_list_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_style_list_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_style_list_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_style_list_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_style_list_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_style_list_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_draw_rect_dsc_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_draw_rect_dsc_t_Type, sizeof(lv_draw_rect_dsc_t));
}



static PyObject *
get_struct_bitfield_draw_rect_dsc_t_border_post(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_draw_rect_dsc_t*)(self->data))->border_post );
}

static int
set_struct_bitfield_draw_rect_dsc_t_border_post(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_draw_rect_dsc_t*)(self->data))->border_post = v;
    return 0;
}



static PyObject *
get_struct_bitfield_draw_rect_dsc_t_pattern_repeat(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_draw_rect_dsc_t*)(self->data))->pattern_repeat );
}

static int
set_struct_bitfield_draw_rect_dsc_t_pattern_repeat(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_draw_rect_dsc_t*)(self->data))->pattern_repeat = v;
    return 0;
}

static PyGetSetDef pylv_draw_rect_dsc_t_getset[] = {
    {"radius", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t radius", (void*)offsetof(lv_draw_rect_dsc_t, radius)},
    {"bg_color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t bg_color", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_draw_rect_dsc_t, bg_color), sizeof(lv_color16_t)})},
    {"bg_grad_color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t bg_grad_color", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_draw_rect_dsc_t, bg_grad_color), sizeof(lv_color16_t)})},
    {"bg_grad_dir", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_grad_dir_t bg_grad_dir", (void*)offsetof(lv_draw_rect_dsc_t, bg_grad_dir)},
    {"bg_main_color_stop", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t bg_main_color_stop", (void*)offsetof(lv_draw_rect_dsc_t, bg_main_color_stop)},
    {"bg_grad_color_stop", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t bg_grad_color_stop", (void*)offsetof(lv_draw_rect_dsc_t, bg_grad_color_stop)},
    {"bg_opa", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t bg_opa", (void*)offsetof(lv_draw_rect_dsc_t, bg_opa)},
    {"bg_blend_mode", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_blend_mode_t bg_blend_mode", (void*)offsetof(lv_draw_rect_dsc_t, bg_blend_mode)},
    {"border_color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t border_color", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_draw_rect_dsc_t, border_color), sizeof(lv_color16_t)})},
    {"border_width", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t border_width", (void*)offsetof(lv_draw_rect_dsc_t, border_width)},
    {"border_side", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t border_side", (void*)offsetof(lv_draw_rect_dsc_t, border_side)},
    {"border_opa", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t border_opa", (void*)offsetof(lv_draw_rect_dsc_t, border_opa)},
    {"border_blend_mode", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_blend_mode_t border_blend_mode", (void*)offsetof(lv_draw_rect_dsc_t, border_blend_mode)},
    {"border_post", (getter) get_struct_bitfield_draw_rect_dsc_t_border_post, (setter) set_struct_bitfield_draw_rect_dsc_t_border_post, "uint8_t:1 border_post", NULL},
    {"outline_color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t outline_color", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_draw_rect_dsc_t, outline_color), sizeof(lv_color16_t)})},
    {"outline_width", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t outline_width", (void*)offsetof(lv_draw_rect_dsc_t, outline_width)},
    {"outline_pad", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t outline_pad", (void*)offsetof(lv_draw_rect_dsc_t, outline_pad)},
    {"outline_opa", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t outline_opa", (void*)offsetof(lv_draw_rect_dsc_t, outline_opa)},
    {"outline_blend_mode", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_blend_mode_t outline_blend_mode", (void*)offsetof(lv_draw_rect_dsc_t, outline_blend_mode)},
    {"shadow_color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t shadow_color", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_draw_rect_dsc_t, shadow_color), sizeof(lv_color16_t)})},
    {"shadow_width", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t shadow_width", (void*)offsetof(lv_draw_rect_dsc_t, shadow_width)},
    {"shadow_ofs_x", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t shadow_ofs_x", (void*)offsetof(lv_draw_rect_dsc_t, shadow_ofs_x)},
    {"shadow_ofs_y", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t shadow_ofs_y", (void*)offsetof(lv_draw_rect_dsc_t, shadow_ofs_y)},
    {"shadow_spread", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t shadow_spread", (void*)offsetof(lv_draw_rect_dsc_t, shadow_spread)},
    {"shadow_opa", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t shadow_opa", (void*)offsetof(lv_draw_rect_dsc_t, shadow_opa)},
    {"shadow_blend_mode", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_blend_mode_t shadow_blend_mode", (void*)offsetof(lv_draw_rect_dsc_t, shadow_blend_mode)},
    {"pattern_image", (getter) struct_get_struct, (setter) struct_set_struct, "void pattern_image", & ((struct_closure_t){ &Blob_Type, offsetof(lv_draw_rect_dsc_t, pattern_image), sizeof(((lv_draw_rect_dsc_t *)0)->pattern_image)})},
    {"pattern_font", (getter) struct_get_struct, (setter) struct_set_struct, "lv_font_t pattern_font", & ((struct_closure_t){ &Blob_Type, offsetof(lv_draw_rect_dsc_t, pattern_font), sizeof(((lv_draw_rect_dsc_t *)0)->pattern_font)})},
    {"pattern_recolor", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t pattern_recolor", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_draw_rect_dsc_t, pattern_recolor), sizeof(lv_color16_t)})},
    {"pattern_opa", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t pattern_opa", (void*)offsetof(lv_draw_rect_dsc_t, pattern_opa)},
    {"pattern_recolor_opa", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t pattern_recolor_opa", (void*)offsetof(lv_draw_rect_dsc_t, pattern_recolor_opa)},
    {"pattern_repeat", (getter) get_struct_bitfield_draw_rect_dsc_t_pattern_repeat, (setter) set_struct_bitfield_draw_rect_dsc_t_pattern_repeat, "uint8_t:1 pattern_repeat", NULL},
    {"pattern_blend_mode", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_blend_mode_t pattern_blend_mode", (void*)offsetof(lv_draw_rect_dsc_t, pattern_blend_mode)},
    {"value_str", (getter) struct_get_struct, (setter) struct_set_struct, "char value_str", & ((struct_closure_t){ &Blob_Type, offsetof(lv_draw_rect_dsc_t, value_str), sizeof(((lv_draw_rect_dsc_t *)0)->value_str)})},
    {"value_font", (getter) struct_get_struct, (setter) struct_set_struct, "lv_font_t value_font", & ((struct_closure_t){ &Blob_Type, offsetof(lv_draw_rect_dsc_t, value_font), sizeof(((lv_draw_rect_dsc_t *)0)->value_font)})},
    {"value_opa", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t value_opa", (void*)offsetof(lv_draw_rect_dsc_t, value_opa)},
    {"value_color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t value_color", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_draw_rect_dsc_t, value_color), sizeof(lv_color16_t)})},
    {"value_ofs_x", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t value_ofs_x", (void*)offsetof(lv_draw_rect_dsc_t, value_ofs_x)},
    {"value_ofs_y", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t value_ofs_y", (void*)offsetof(lv_draw_rect_dsc_t, value_ofs_y)},
    {"value_letter_space", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t value_letter_space", (void*)offsetof(lv_draw_rect_dsc_t, value_letter_space)},
    {"value_line_space", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t value_line_space", (void*)offsetof(lv_draw_rect_dsc_t, value_line_space)},
    {"value_align", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_align_t value_align", (void*)offsetof(lv_draw_rect_dsc_t, value_align)},
    {"value_blend_mode", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_blend_mode_t value_blend_mode", (void*)offsetof(lv_draw_rect_dsc_t, value_blend_mode)},
    {NULL}
};


static PyTypeObject pylv_draw_rect_dsc_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.draw_rect_dsc_t",
    .tp_doc = "lvgl draw_rect_dsc_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_draw_rect_dsc_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_draw_rect_dsc_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_draw_rect_dsc_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_draw_rect_dsc_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_draw_rect_dsc_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_draw_rect_dsc_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_draw_label_dsc_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_draw_label_dsc_t_Type, sizeof(lv_draw_label_dsc_t));
}

static PyGetSetDef pylv_draw_label_dsc_t_getset[] = {
    {"color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t color", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_draw_label_dsc_t, color), sizeof(lv_color16_t)})},
    {"sel_color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t sel_color", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_draw_label_dsc_t, sel_color), sizeof(lv_color16_t)})},
    {"sel_bg_color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t sel_bg_color", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_draw_label_dsc_t, sel_bg_color), sizeof(lv_color16_t)})},
    {"font", (getter) struct_get_struct, (setter) struct_set_struct, "lv_font_t font", & ((struct_closure_t){ &Blob_Type, offsetof(lv_draw_label_dsc_t, font), sizeof(((lv_draw_label_dsc_t *)0)->font)})},
    {"opa", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t opa", (void*)offsetof(lv_draw_label_dsc_t, opa)},
    {"line_space", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t line_space", (void*)offsetof(lv_draw_label_dsc_t, line_space)},
    {"letter_space", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t letter_space", (void*)offsetof(lv_draw_label_dsc_t, letter_space)},
    {"sel_start", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t sel_start", (void*)offsetof(lv_draw_label_dsc_t, sel_start)},
    {"sel_end", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t sel_end", (void*)offsetof(lv_draw_label_dsc_t, sel_end)},
    {"ofs_x", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t ofs_x", (void*)offsetof(lv_draw_label_dsc_t, ofs_x)},
    {"ofs_y", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t ofs_y", (void*)offsetof(lv_draw_label_dsc_t, ofs_y)},
    {"bidi_dir", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_bidi_dir_t bidi_dir", (void*)offsetof(lv_draw_label_dsc_t, bidi_dir)},
    {"flag", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_txt_flag_t flag", (void*)offsetof(lv_draw_label_dsc_t, flag)},
    {"decor", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_text_decor_t decor", (void*)offsetof(lv_draw_label_dsc_t, decor)},
    {"blend_mode", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_blend_mode_t blend_mode", (void*)offsetof(lv_draw_label_dsc_t, blend_mode)},
    {NULL}
};


static PyTypeObject pylv_draw_label_dsc_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.draw_label_dsc_t",
    .tp_doc = "lvgl draw_label_dsc_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_draw_label_dsc_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_draw_label_dsc_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_draw_label_dsc_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_draw_label_dsc_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_draw_label_dsc_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_draw_label_dsc_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_draw_label_hint_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_draw_label_hint_t_Type, sizeof(lv_draw_label_hint_t));
}

static PyGetSetDef pylv_draw_label_hint_t_getset[] = {
    {"line_start", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t line_start", (void*)offsetof(lv_draw_label_hint_t, line_start)},
    {"y", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t y", (void*)offsetof(lv_draw_label_hint_t, y)},
    {"coord_y", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t coord_y", (void*)offsetof(lv_draw_label_hint_t, coord_y)},
    {NULL}
};


static PyTypeObject pylv_draw_label_hint_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.draw_label_hint_t",
    .tp_doc = "lvgl draw_label_hint_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_draw_label_hint_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_draw_label_hint_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_draw_label_hint_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_draw_label_hint_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_draw_label_hint_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_draw_label_hint_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_draw_line_dsc_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_draw_line_dsc_t_Type, sizeof(lv_draw_line_dsc_t));
}



static PyObject *
get_struct_bitfield_draw_line_dsc_t_blend_mode(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_draw_line_dsc_t*)(self->data))->blend_mode );
}

static int
set_struct_bitfield_draw_line_dsc_t_blend_mode(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_draw_line_dsc_t*)(self->data))->blend_mode = v;
    return 0;
}



static PyObject *
get_struct_bitfield_draw_line_dsc_t_round_start(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_draw_line_dsc_t*)(self->data))->round_start );
}

static int
set_struct_bitfield_draw_line_dsc_t_round_start(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_draw_line_dsc_t*)(self->data))->round_start = v;
    return 0;
}



static PyObject *
get_struct_bitfield_draw_line_dsc_t_round_end(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_draw_line_dsc_t*)(self->data))->round_end );
}

static int
set_struct_bitfield_draw_line_dsc_t_round_end(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_draw_line_dsc_t*)(self->data))->round_end = v;
    return 0;
}



static PyObject *
get_struct_bitfield_draw_line_dsc_t_raw_end(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_draw_line_dsc_t*)(self->data))->raw_end );
}

static int
set_struct_bitfield_draw_line_dsc_t_raw_end(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_draw_line_dsc_t*)(self->data))->raw_end = v;
    return 0;
}

static PyGetSetDef pylv_draw_line_dsc_t_getset[] = {
    {"color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t color", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_draw_line_dsc_t, color), sizeof(lv_color16_t)})},
    {"width", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t width", (void*)offsetof(lv_draw_line_dsc_t, width)},
    {"dash_width", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t dash_width", (void*)offsetof(lv_draw_line_dsc_t, dash_width)},
    {"dash_gap", (getter) struct_get_int16, (setter) struct_set_int16, "lv_style_int_t dash_gap", (void*)offsetof(lv_draw_line_dsc_t, dash_gap)},
    {"opa", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t opa", (void*)offsetof(lv_draw_line_dsc_t, opa)},
    {"blend_mode", (getter) get_struct_bitfield_draw_line_dsc_t_blend_mode, (setter) set_struct_bitfield_draw_line_dsc_t_blend_mode, "lv_blend_mode_t:2 blend_mode", NULL},
    {"round_start", (getter) get_struct_bitfield_draw_line_dsc_t_round_start, (setter) set_struct_bitfield_draw_line_dsc_t_round_start, "uint8_t:1 round_start", NULL},
    {"round_end", (getter) get_struct_bitfield_draw_line_dsc_t_round_end, (setter) set_struct_bitfield_draw_line_dsc_t_round_end, "uint8_t:1 round_end", NULL},
    {"raw_end", (getter) get_struct_bitfield_draw_line_dsc_t_raw_end, (setter) set_struct_bitfield_draw_line_dsc_t_raw_end, "uint8_t:1 raw_end", NULL},
    {NULL}
};


static PyTypeObject pylv_draw_line_dsc_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.draw_line_dsc_t",
    .tp_doc = "lvgl draw_line_dsc_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_draw_line_dsc_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_draw_line_dsc_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_draw_line_dsc_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_draw_line_dsc_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_draw_line_dsc_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_draw_line_dsc_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_img_header_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_img_header_t_Type, sizeof(lv_img_header_t));
}



static PyObject *
get_struct_bitfield_img_header_t_cf(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_img_header_t*)(self->data))->cf );
}

static int
set_struct_bitfield_img_header_t_cf(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 31)) return -1;
    ((lv_img_header_t*)(self->data))->cf = v;
    return 0;
}



static PyObject *
get_struct_bitfield_img_header_t_always_zero(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_img_header_t*)(self->data))->always_zero );
}

static int
set_struct_bitfield_img_header_t_always_zero(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 7)) return -1;
    ((lv_img_header_t*)(self->data))->always_zero = v;
    return 0;
}



static PyObject *
get_struct_bitfield_img_header_t_reserved(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_img_header_t*)(self->data))->reserved );
}

static int
set_struct_bitfield_img_header_t_reserved(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_img_header_t*)(self->data))->reserved = v;
    return 0;
}



static PyObject *
get_struct_bitfield_img_header_t_w(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_img_header_t*)(self->data))->w );
}

static int
set_struct_bitfield_img_header_t_w(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 2047)) return -1;
    ((lv_img_header_t*)(self->data))->w = v;
    return 0;
}



static PyObject *
get_struct_bitfield_img_header_t_h(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_img_header_t*)(self->data))->h );
}

static int
set_struct_bitfield_img_header_t_h(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 2047)) return -1;
    ((lv_img_header_t*)(self->data))->h = v;
    return 0;
}

static PyGetSetDef pylv_img_header_t_getset[] = {
    {"cf", (getter) get_struct_bitfield_img_header_t_cf, (setter) set_struct_bitfield_img_header_t_cf, "uint32_t:5 cf", NULL},
    {"always_zero", (getter) get_struct_bitfield_img_header_t_always_zero, (setter) set_struct_bitfield_img_header_t_always_zero, "uint32_t:3 always_zero", NULL},
    {"reserved", (getter) get_struct_bitfield_img_header_t_reserved, (setter) set_struct_bitfield_img_header_t_reserved, "uint32_t:2 reserved", NULL},
    {"w", (getter) get_struct_bitfield_img_header_t_w, (setter) set_struct_bitfield_img_header_t_w, "uint32_t:11 w", NULL},
    {"h", (getter) get_struct_bitfield_img_header_t_h, (setter) set_struct_bitfield_img_header_t_h, "uint32_t:11 h", NULL},
    {NULL}
};


static PyTypeObject pylv_img_header_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.img_header_t",
    .tp_doc = "lvgl img_header_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_img_header_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_img_header_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_img_header_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_img_header_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_img_header_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_img_header_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_img_dsc_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_img_dsc_t_Type, sizeof(lv_img_dsc_t));
}

static PyGetSetDef pylv_img_dsc_t_getset[] = {
    {"header", (getter) struct_get_struct, (setter) struct_set_struct, "lv_img_header_t header", & ((struct_closure_t){ &pylv_img_header_t_Type, offsetof(lv_img_dsc_t, header), sizeof(lv_img_header_t)})},
    {"data_size", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t data_size", (void*)offsetof(lv_img_dsc_t, data_size)},
    {"data", (getter) struct_get_struct, (setter) struct_set_struct, "uint8_t data", & ((struct_closure_t){ &Blob_Type, offsetof(lv_img_dsc_t, data), sizeof(((lv_img_dsc_t *)0)->data)})},
    {NULL}
};


static PyTypeObject pylv_img_dsc_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.img_dsc_t",
    .tp_doc = "lvgl img_dsc_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_img_dsc_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_img_dsc_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_img_dsc_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_img_dsc_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_img_dsc_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_img_dsc_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_img_transform_dsc_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_img_transform_dsc_t_Type, sizeof(lv_img_transform_dsc_t));
}

static PyGetSetDef pylv_img_transform_dsc_t_getset[] = {
    {"cfg", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   const void *src;   lv_coord_t src_w;   lv_coord_t src_h;   lv_coord_t pivot_x;   lv_coord_t pivot_y;   int16_t angle;   uint16_t zoom;   lv_color_t color;   lv_img_cf_t cf;   bool antialias; } cfg", & ((struct_closure_t){ &pylv_img_transform_dsc_t_cfg_Type, offsetof(lv_img_transform_dsc_t, cfg), sizeof(((lv_img_transform_dsc_t *)0)->cfg)})},
    {"res", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_color_t color;   lv_opa_t opa; } res", & ((struct_closure_t){ &pylv_img_transform_dsc_t_res_Type, offsetof(lv_img_transform_dsc_t, res), sizeof(((lv_img_transform_dsc_t *)0)->res)})},
    {"tmp", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_img_dsc_t img_dsc;   int32_t pivot_x_256;   int32_t pivot_y_256;   int32_t sinma;   int32_t cosma;   uint8_t chroma_keyed : 1;   uint8_t has_alpha : 1;   uint8_t native_color : 1;   uint32_t zoom_inv;   lv_coord_t xs;   lv_coord_t ys;   lv_coord_t xs_int;   lv_coord_t ys_int;   uint32_t pxi;   uint8_t px_size; } tmp", & ((struct_closure_t){ &pylv_img_transform_dsc_t_tmp_Type, offsetof(lv_img_transform_dsc_t, tmp), sizeof(((lv_img_transform_dsc_t *)0)->tmp)})},
    {NULL}
};


static PyTypeObject pylv_img_transform_dsc_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.img_transform_dsc_t",
    .tp_doc = "lvgl img_transform_dsc_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_img_transform_dsc_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_img_transform_dsc_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_img_transform_dsc_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_img_transform_dsc_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_img_transform_dsc_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_img_transform_dsc_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_fs_drv_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_fs_drv_t_Type, sizeof(lv_fs_drv_t));
}

static PyGetSetDef pylv_fs_drv_t_getset[] = {
    {"letter", (getter) struct_get_struct, (setter) struct_set_struct, "char letter", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, letter), sizeof(((lv_fs_drv_t *)0)->letter)})},
    {"file_size", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t file_size", (void*)offsetof(lv_fs_drv_t, file_size)},
    {"rddir_size", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t rddir_size", (void*)offsetof(lv_fs_drv_t, rddir_size)},
    {"ready_cb", (getter) struct_get_struct, (setter) struct_set_struct, "bool ready_cb(struct _lv_fs_drv_t *drv) ready_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, ready_cb), sizeof(((lv_fs_drv_t *)0)->ready_cb)})},
    {"open_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t open_cb(struct _lv_fs_drv_t *drv, void *file_p, const char *path, lv_fs_mode_t mode) open_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, open_cb), sizeof(((lv_fs_drv_t *)0)->open_cb)})},
    {"close_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t close_cb(struct _lv_fs_drv_t *drv, void *file_p) close_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, close_cb), sizeof(((lv_fs_drv_t *)0)->close_cb)})},
    {"remove_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t remove_cb(struct _lv_fs_drv_t *drv, const char *fn) remove_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, remove_cb), sizeof(((lv_fs_drv_t *)0)->remove_cb)})},
    {"read_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t read_cb(struct _lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br) read_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, read_cb), sizeof(((lv_fs_drv_t *)0)->read_cb)})},
    {"write_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t write_cb(struct _lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw) write_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, write_cb), sizeof(((lv_fs_drv_t *)0)->write_cb)})},
    {"seek_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t seek_cb(struct _lv_fs_drv_t *drv, void *file_p, uint32_t pos) seek_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, seek_cb), sizeof(((lv_fs_drv_t *)0)->seek_cb)})},
    {"tell_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t tell_cb(struct _lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p) tell_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, tell_cb), sizeof(((lv_fs_drv_t *)0)->tell_cb)})},
    {"trunc_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t trunc_cb(struct _lv_fs_drv_t *drv, void *file_p) trunc_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, trunc_cb), sizeof(((lv_fs_drv_t *)0)->trunc_cb)})},
    {"size_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t size_cb(struct _lv_fs_drv_t *drv, void *file_p, uint32_t *size_p) size_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, size_cb), sizeof(((lv_fs_drv_t *)0)->size_cb)})},
    {"rename_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t rename_cb(struct _lv_fs_drv_t *drv, const char *oldname, const char *newname) rename_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, rename_cb), sizeof(((lv_fs_drv_t *)0)->rename_cb)})},
    {"free_space_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t free_space_cb(struct _lv_fs_drv_t *drv, uint32_t *total_p, uint32_t *free_p) free_space_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, free_space_cb), sizeof(((lv_fs_drv_t *)0)->free_space_cb)})},
    {"dir_open_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t dir_open_cb(struct _lv_fs_drv_t *drv, void *rddir_p, const char *path) dir_open_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, dir_open_cb), sizeof(((lv_fs_drv_t *)0)->dir_open_cb)})},
    {"dir_read_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t dir_read_cb(struct _lv_fs_drv_t *drv, void *rddir_p, char *fn) dir_read_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, dir_read_cb), sizeof(((lv_fs_drv_t *)0)->dir_read_cb)})},
    {"dir_close_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t dir_close_cb(struct _lv_fs_drv_t *drv, void *rddir_p) dir_close_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, dir_close_cb), sizeof(((lv_fs_drv_t *)0)->dir_close_cb)})},
    {"user_data", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_drv_user_data_t user_data", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, user_data), sizeof(((lv_fs_drv_t *)0)->user_data)})},
    {NULL}
};


static PyTypeObject pylv_fs_drv_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.fs_drv_t",
    .tp_doc = "lvgl fs_drv_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_fs_drv_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_fs_drv_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_fs_drv_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_fs_drv_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_fs_drv_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_fs_drv_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_fs_file_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_fs_file_t_Type, sizeof(lv_fs_file_t));
}

static PyGetSetDef pylv_fs_file_t_getset[] = {
    {"file_d", (getter) struct_get_struct, (setter) struct_set_struct, "void file_d", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_file_t, file_d), sizeof(((lv_fs_file_t *)0)->file_d)})},
    {"drv", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_drv_t drv", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_file_t, drv), sizeof(((lv_fs_file_t *)0)->drv)})},
    {NULL}
};


static PyTypeObject pylv_fs_file_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.fs_file_t",
    .tp_doc = "lvgl fs_file_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_fs_file_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_fs_file_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_fs_file_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_fs_file_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_fs_file_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_fs_file_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_fs_dir_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_fs_dir_t_Type, sizeof(lv_fs_dir_t));
}

static PyGetSetDef pylv_fs_dir_t_getset[] = {
    {"dir_d", (getter) struct_get_struct, (setter) struct_set_struct, "void dir_d", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_dir_t, dir_d), sizeof(((lv_fs_dir_t *)0)->dir_d)})},
    {"drv", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_drv_t drv", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_dir_t, drv), sizeof(((lv_fs_dir_t *)0)->drv)})},
    {NULL}
};


static PyTypeObject pylv_fs_dir_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.fs_dir_t",
    .tp_doc = "lvgl fs_dir_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_fs_dir_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_fs_dir_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_fs_dir_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_fs_dir_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_fs_dir_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_fs_dir_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_img_decoder_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_img_decoder_t_Type, sizeof(lv_img_decoder_t));
}

static PyGetSetDef pylv_img_decoder_t_getset[] = {
    {"info_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_img_decoder_info_f_t info_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_img_decoder_t, info_cb), sizeof(((lv_img_decoder_t *)0)->info_cb)})},
    {"open_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_img_decoder_open_f_t open_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_img_decoder_t, open_cb), sizeof(((lv_img_decoder_t *)0)->open_cb)})},
    {"read_line_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_img_decoder_read_line_f_t read_line_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_img_decoder_t, read_line_cb), sizeof(((lv_img_decoder_t *)0)->read_line_cb)})},
    {"close_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_img_decoder_close_f_t close_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_img_decoder_t, close_cb), sizeof(((lv_img_decoder_t *)0)->close_cb)})},
    {"user_data", (getter) struct_get_struct, (setter) struct_set_struct, "lv_img_decoder_user_data_t user_data", & ((struct_closure_t){ &Blob_Type, offsetof(lv_img_decoder_t, user_data), sizeof(((lv_img_decoder_t *)0)->user_data)})},
    {NULL}
};


static PyTypeObject pylv_img_decoder_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.img_decoder_t",
    .tp_doc = "lvgl img_decoder_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_img_decoder_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_img_decoder_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_img_decoder_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_img_decoder_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_img_decoder_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_img_decoder_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_img_decoder_dsc_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_img_decoder_dsc_t_Type, sizeof(lv_img_decoder_dsc_t));
}

static PyGetSetDef pylv_img_decoder_dsc_t_getset[] = {
    {"decoder", (getter) struct_get_struct, (setter) struct_set_struct, "lv_img_decoder_t decoder", & ((struct_closure_t){ &Blob_Type, offsetof(lv_img_decoder_dsc_t, decoder), sizeof(((lv_img_decoder_dsc_t *)0)->decoder)})},
    {"src", (getter) struct_get_struct, (setter) struct_set_struct, "void src", & ((struct_closure_t){ &Blob_Type, offsetof(lv_img_decoder_dsc_t, src), sizeof(((lv_img_decoder_dsc_t *)0)->src)})},
    {"color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t color", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_img_decoder_dsc_t, color), sizeof(lv_color16_t)})},
    {"src_type", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_img_src_t src_type", (void*)offsetof(lv_img_decoder_dsc_t, src_type)},
    {"header", (getter) struct_get_struct, (setter) struct_set_struct, "lv_img_header_t header", & ((struct_closure_t){ &pylv_img_header_t_Type, offsetof(lv_img_decoder_dsc_t, header), sizeof(lv_img_header_t)})},
    {"img_data", (getter) struct_get_struct, (setter) struct_set_struct, "uint8_t img_data", & ((struct_closure_t){ &Blob_Type, offsetof(lv_img_decoder_dsc_t, img_data), sizeof(((lv_img_decoder_dsc_t *)0)->img_data)})},
    {"time_to_open", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t time_to_open", (void*)offsetof(lv_img_decoder_dsc_t, time_to_open)},
    {"error_msg", (getter) struct_get_struct, (setter) struct_set_struct, "char error_msg", & ((struct_closure_t){ &Blob_Type, offsetof(lv_img_decoder_dsc_t, error_msg), sizeof(((lv_img_decoder_dsc_t *)0)->error_msg)})},
    {"user_data", (getter) struct_get_struct, (setter) struct_set_struct, "void user_data", & ((struct_closure_t){ &Blob_Type, offsetof(lv_img_decoder_dsc_t, user_data), sizeof(((lv_img_decoder_dsc_t *)0)->user_data)})},
    {NULL}
};


static PyTypeObject pylv_img_decoder_dsc_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.img_decoder_dsc_t",
    .tp_doc = "lvgl img_decoder_dsc_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_img_decoder_dsc_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_img_decoder_dsc_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_img_decoder_dsc_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_img_decoder_dsc_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_img_decoder_dsc_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_img_decoder_dsc_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_draw_img_dsc_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_draw_img_dsc_t_Type, sizeof(lv_draw_img_dsc_t));
}



static PyObject *
get_struct_bitfield_draw_img_dsc_t_antialias(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_draw_img_dsc_t*)(self->data))->antialias );
}

static int
set_struct_bitfield_draw_img_dsc_t_antialias(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_draw_img_dsc_t*)(self->data))->antialias = v;
    return 0;
}

static PyGetSetDef pylv_draw_img_dsc_t_getset[] = {
    {"opa", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t opa", (void*)offsetof(lv_draw_img_dsc_t, opa)},
    {"angle", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t angle", (void*)offsetof(lv_draw_img_dsc_t, angle)},
    {"pivot", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t pivot", & ((struct_closure_t){ &pylv_point_t_Type, offsetof(lv_draw_img_dsc_t, pivot), sizeof(lv_point_t)})},
    {"zoom", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t zoom", (void*)offsetof(lv_draw_img_dsc_t, zoom)},
    {"recolor_opa", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t recolor_opa", (void*)offsetof(lv_draw_img_dsc_t, recolor_opa)},
    {"recolor", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t recolor", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_draw_img_dsc_t, recolor), sizeof(lv_color16_t)})},
    {"blend_mode", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_blend_mode_t blend_mode", (void*)offsetof(lv_draw_img_dsc_t, blend_mode)},
    {"antialias", (getter) get_struct_bitfield_draw_img_dsc_t_antialias, (setter) set_struct_bitfield_draw_img_dsc_t_antialias, "uint8_t:1 antialias", NULL},
    {NULL}
};


static PyTypeObject pylv_draw_img_dsc_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.draw_img_dsc_t",
    .tp_doc = "lvgl draw_img_dsc_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_draw_img_dsc_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_draw_img_dsc_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_draw_img_dsc_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_draw_img_dsc_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_draw_img_dsc_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_draw_img_dsc_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_realign_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_realign_t_Type, sizeof(lv_realign_t));
}



static PyObject *
get_struct_bitfield_realign_t_auto_realign(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_realign_t*)(self->data))->auto_realign );
}

static int
set_struct_bitfield_realign_t_auto_realign(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_realign_t*)(self->data))->auto_realign = v;
    return 0;
}



static PyObject *
get_struct_bitfield_realign_t_mid_align(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_realign_t*)(self->data))->mid_align );
}

static int
set_struct_bitfield_realign_t_mid_align(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_realign_t*)(self->data))->mid_align = v;
    return 0;
}

static PyGetSetDef pylv_realign_t_getset[] = {
    {"base", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_obj_t base", & ((struct_closure_t){ &Blob_Type, offsetof(lv_realign_t, base), sizeof(((lv_realign_t *)0)->base)})},
    {"xofs", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t xofs", (void*)offsetof(lv_realign_t, xofs)},
    {"yofs", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t yofs", (void*)offsetof(lv_realign_t, yofs)},
    {"align", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_align_t align", (void*)offsetof(lv_realign_t, align)},
    {"auto_realign", (getter) get_struct_bitfield_realign_t_auto_realign, (setter) set_struct_bitfield_realign_t_auto_realign, "uint8_t:1 auto_realign", NULL},
    {"mid_align", (getter) get_struct_bitfield_realign_t_mid_align, (setter) set_struct_bitfield_realign_t_mid_align, "uint8_t:1 mid_align", NULL},
    {NULL}
};


static PyTypeObject pylv_realign_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.realign_t",
    .tp_doc = "lvgl realign_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_realign_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_realign_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_realign_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_realign_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_realign_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_realign_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_obj_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_obj_t_Type, sizeof(lv_obj_t));
}



static PyObject *
get_struct_bitfield_obj_t_click(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_obj_t*)(self->data))->click );
}

static int
set_struct_bitfield_obj_t_click(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_obj_t*)(self->data))->click = v;
    return 0;
}



static PyObject *
get_struct_bitfield_obj_t_drag(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_obj_t*)(self->data))->drag );
}

static int
set_struct_bitfield_obj_t_drag(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_obj_t*)(self->data))->drag = v;
    return 0;
}



static PyObject *
get_struct_bitfield_obj_t_drag_throw(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_obj_t*)(self->data))->drag_throw );
}

static int
set_struct_bitfield_obj_t_drag_throw(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_obj_t*)(self->data))->drag_throw = v;
    return 0;
}



static PyObject *
get_struct_bitfield_obj_t_drag_parent(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_obj_t*)(self->data))->drag_parent );
}

static int
set_struct_bitfield_obj_t_drag_parent(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_obj_t*)(self->data))->drag_parent = v;
    return 0;
}



static PyObject *
get_struct_bitfield_obj_t_hidden(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_obj_t*)(self->data))->hidden );
}

static int
set_struct_bitfield_obj_t_hidden(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_obj_t*)(self->data))->hidden = v;
    return 0;
}



static PyObject *
get_struct_bitfield_obj_t_top(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_obj_t*)(self->data))->top );
}

static int
set_struct_bitfield_obj_t_top(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_obj_t*)(self->data))->top = v;
    return 0;
}



static PyObject *
get_struct_bitfield_obj_t_parent_event(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_obj_t*)(self->data))->parent_event );
}

static int
set_struct_bitfield_obj_t_parent_event(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_obj_t*)(self->data))->parent_event = v;
    return 0;
}



static PyObject *
get_struct_bitfield_obj_t_adv_hittest(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_obj_t*)(self->data))->adv_hittest );
}

static int
set_struct_bitfield_obj_t_adv_hittest(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_obj_t*)(self->data))->adv_hittest = v;
    return 0;
}



static PyObject *
get_struct_bitfield_obj_t_gesture_parent(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_obj_t*)(self->data))->gesture_parent );
}

static int
set_struct_bitfield_obj_t_gesture_parent(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_obj_t*)(self->data))->gesture_parent = v;
    return 0;
}



static PyObject *
get_struct_bitfield_obj_t_focus_parent(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_obj_t*)(self->data))->focus_parent );
}

static int
set_struct_bitfield_obj_t_focus_parent(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_obj_t*)(self->data))->focus_parent = v;
    return 0;
}



static PyObject *
get_struct_bitfield_obj_t_drag_dir(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_obj_t*)(self->data))->drag_dir );
}

static int
set_struct_bitfield_obj_t_drag_dir(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 7)) return -1;
    ((lv_obj_t*)(self->data))->drag_dir = v;
    return 0;
}



static PyObject *
get_struct_bitfield_obj_t_base_dir(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_obj_t*)(self->data))->base_dir );
}

static int
set_struct_bitfield_obj_t_base_dir(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_obj_t*)(self->data))->base_dir = v;
    return 0;
}

static PyGetSetDef pylv_obj_t_getset[] = {
    {"parent", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_obj_t parent", & ((struct_closure_t){ &Blob_Type, offsetof(lv_obj_t, parent), sizeof(((lv_obj_t *)0)->parent)})},
    {"child_ll", (getter) struct_get_struct, (setter) struct_set_struct, "lv_ll_t child_ll", & ((struct_closure_t){ &pylv_ll_t_Type, offsetof(lv_obj_t, child_ll), sizeof(lv_ll_t)})},
    {"coords", (getter) struct_get_struct, (setter) struct_set_struct, "lv_area_t coords", & ((struct_closure_t){ &pylv_area_t_Type, offsetof(lv_obj_t, coords), sizeof(lv_area_t)})},
    {"event_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_event_cb_t event_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_obj_t, event_cb), sizeof(((lv_obj_t *)0)->event_cb)})},
    {"signal_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_signal_cb_t signal_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_obj_t, signal_cb), sizeof(((lv_obj_t *)0)->signal_cb)})},
    {"design_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_design_cb_t design_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_obj_t, design_cb), sizeof(((lv_obj_t *)0)->design_cb)})},
    {"ext_attr", (getter) struct_get_struct, (setter) struct_set_struct, "void ext_attr", & ((struct_closure_t){ &Blob_Type, offsetof(lv_obj_t, ext_attr), sizeof(((lv_obj_t *)0)->ext_attr)})},
    {"style_list", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_list", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_obj_t, style_list), sizeof(lv_style_list_t)})},
    {"ext_click_pad_hor", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t ext_click_pad_hor", (void*)offsetof(lv_obj_t, ext_click_pad_hor)},
    {"ext_click_pad_ver", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t ext_click_pad_ver", (void*)offsetof(lv_obj_t, ext_click_pad_ver)},
    {"ext_draw_pad", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t ext_draw_pad", (void*)offsetof(lv_obj_t, ext_draw_pad)},
    {"click", (getter) get_struct_bitfield_obj_t_click, (setter) set_struct_bitfield_obj_t_click, "uint8_t:1 click", NULL},
    {"drag", (getter) get_struct_bitfield_obj_t_drag, (setter) set_struct_bitfield_obj_t_drag, "uint8_t:1 drag", NULL},
    {"drag_throw", (getter) get_struct_bitfield_obj_t_drag_throw, (setter) set_struct_bitfield_obj_t_drag_throw, "uint8_t:1 drag_throw", NULL},
    {"drag_parent", (getter) get_struct_bitfield_obj_t_drag_parent, (setter) set_struct_bitfield_obj_t_drag_parent, "uint8_t:1 drag_parent", NULL},
    {"hidden", (getter) get_struct_bitfield_obj_t_hidden, (setter) set_struct_bitfield_obj_t_hidden, "uint8_t:1 hidden", NULL},
    {"top", (getter) get_struct_bitfield_obj_t_top, (setter) set_struct_bitfield_obj_t_top, "uint8_t:1 top", NULL},
    {"parent_event", (getter) get_struct_bitfield_obj_t_parent_event, (setter) set_struct_bitfield_obj_t_parent_event, "uint8_t:1 parent_event", NULL},
    {"adv_hittest", (getter) get_struct_bitfield_obj_t_adv_hittest, (setter) set_struct_bitfield_obj_t_adv_hittest, "uint8_t:1 adv_hittest", NULL},
    {"gesture_parent", (getter) get_struct_bitfield_obj_t_gesture_parent, (setter) set_struct_bitfield_obj_t_gesture_parent, "uint8_t:1 gesture_parent", NULL},
    {"focus_parent", (getter) get_struct_bitfield_obj_t_focus_parent, (setter) set_struct_bitfield_obj_t_focus_parent, "uint8_t:1 focus_parent", NULL},
    {"drag_dir", (getter) get_struct_bitfield_obj_t_drag_dir, (setter) set_struct_bitfield_obj_t_drag_dir, "lv_drag_dir_t:3 drag_dir", NULL},
    {"base_dir", (getter) get_struct_bitfield_obj_t_base_dir, (setter) set_struct_bitfield_obj_t_base_dir, "lv_bidi_dir_t:2 base_dir", NULL},
    {"group_p", (getter) struct_get_struct, (setter) struct_set_struct, "void group_p", & ((struct_closure_t){ &Blob_Type, offsetof(lv_obj_t, group_p), sizeof(((lv_obj_t *)0)->group_p)})},
    {"protect", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t protect", (void*)offsetof(lv_obj_t, protect)},
    {"state", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_state_t state", (void*)offsetof(lv_obj_t, state)},
    {"realign", (getter) struct_get_struct, (setter) struct_set_struct, "lv_realign_t realign", & ((struct_closure_t){ &pylv_realign_t_Type, offsetof(lv_obj_t, realign), sizeof(lv_realign_t)})},
    {"user_data", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_user_data_t user_data", & ((struct_closure_t){ &Blob_Type, offsetof(lv_obj_t, user_data), sizeof(((lv_obj_t *)0)->user_data)})},
    {NULL}
};


static PyTypeObject pylv_obj_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.obj_t",
    .tp_doc = "lvgl obj_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_obj_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_obj_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_obj_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_obj_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_obj_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_obj_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_obj_type_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_obj_type_t_Type, sizeof(lv_obj_type_t));
}

static PyGetSetDef pylv_obj_type_t_getset[] = {
    {"type", (getter) struct_get_struct, (setter) struct_set_struct, "char8 type", & ((struct_closure_t){ &Blob_Type, offsetof(lv_obj_type_t, type), sizeof(((lv_obj_type_t *)0)->type)})},
    {NULL}
};


static PyTypeObject pylv_obj_type_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.obj_type_t",
    .tp_doc = "lvgl obj_type_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_obj_type_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_obj_type_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_obj_type_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_obj_type_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_obj_type_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_obj_type_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_hit_test_info_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_hit_test_info_t_Type, sizeof(lv_hit_test_info_t));
}

static PyGetSetDef pylv_hit_test_info_t_getset[] = {
    {"point", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t point", & ((struct_closure_t){ &Blob_Type, offsetof(lv_hit_test_info_t, point), sizeof(((lv_hit_test_info_t *)0)->point)})},
    {"result", (getter) struct_get_struct, (setter) struct_set_struct, "bool result", & ((struct_closure_t){ &Blob_Type, offsetof(lv_hit_test_info_t, result), sizeof(((lv_hit_test_info_t *)0)->result)})},
    {NULL}
};


static PyTypeObject pylv_hit_test_info_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.hit_test_info_t",
    .tp_doc = "lvgl hit_test_info_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_hit_test_info_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_hit_test_info_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_hit_test_info_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_hit_test_info_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_hit_test_info_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_hit_test_info_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_get_style_info_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_get_style_info_t_Type, sizeof(lv_get_style_info_t));
}

static PyGetSetDef pylv_get_style_info_t_getset[] = {
    {"part", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t part", (void*)offsetof(lv_get_style_info_t, part)},
    {"result", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t result", & ((struct_closure_t){ &Blob_Type, offsetof(lv_get_style_info_t, result), sizeof(((lv_get_style_info_t *)0)->result)})},
    {NULL}
};


static PyTypeObject pylv_get_style_info_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.get_style_info_t",
    .tp_doc = "lvgl get_style_info_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_get_style_info_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_get_style_info_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_get_style_info_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_get_style_info_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_get_style_info_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_get_style_info_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_get_state_info_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_get_state_info_t_Type, sizeof(lv_get_state_info_t));
}

static PyGetSetDef pylv_get_state_info_t_getset[] = {
    {"part", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t part", (void*)offsetof(lv_get_state_info_t, part)},
    {"result", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_state_t result", (void*)offsetof(lv_get_state_info_t, result)},
    {NULL}
};


static PyTypeObject pylv_get_state_info_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.get_state_info_t",
    .tp_doc = "lvgl get_state_info_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_get_state_info_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_get_state_info_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_get_state_info_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_get_state_info_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_get_state_info_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_get_state_info_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_group_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_group_t_Type, sizeof(lv_group_t));
}



static PyObject *
get_struct_bitfield_group_t_frozen(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_group_t*)(self->data))->frozen );
}

static int
set_struct_bitfield_group_t_frozen(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_group_t*)(self->data))->frozen = v;
    return 0;
}



static PyObject *
get_struct_bitfield_group_t_editing(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_group_t*)(self->data))->editing );
}

static int
set_struct_bitfield_group_t_editing(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_group_t*)(self->data))->editing = v;
    return 0;
}



static PyObject *
get_struct_bitfield_group_t_click_focus(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_group_t*)(self->data))->click_focus );
}

static int
set_struct_bitfield_group_t_click_focus(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_group_t*)(self->data))->click_focus = v;
    return 0;
}



static PyObject *
get_struct_bitfield_group_t_refocus_policy(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_group_t*)(self->data))->refocus_policy );
}

static int
set_struct_bitfield_group_t_refocus_policy(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_group_t*)(self->data))->refocus_policy = v;
    return 0;
}



static PyObject *
get_struct_bitfield_group_t_wrap(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_group_t*)(self->data))->wrap );
}

static int
set_struct_bitfield_group_t_wrap(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_group_t*)(self->data))->wrap = v;
    return 0;
}

static PyGetSetDef pylv_group_t_getset[] = {
    {"obj_ll", (getter) struct_get_struct, (setter) struct_set_struct, "lv_ll_t obj_ll", & ((struct_closure_t){ &pylv_ll_t_Type, offsetof(lv_group_t, obj_ll), sizeof(lv_ll_t)})},
    {"obj_focus", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t obj_focus", & ((struct_closure_t){ &Blob_Type, offsetof(lv_group_t, obj_focus), sizeof(((lv_group_t *)0)->obj_focus)})},
    {"focus_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_group_focus_cb_t focus_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_group_t, focus_cb), sizeof(((lv_group_t *)0)->focus_cb)})},
    {"user_data", (getter) struct_get_struct, (setter) struct_set_struct, "lv_group_user_data_t user_data", & ((struct_closure_t){ &Blob_Type, offsetof(lv_group_t, user_data), sizeof(((lv_group_t *)0)->user_data)})},
    {"frozen", (getter) get_struct_bitfield_group_t_frozen, (setter) set_struct_bitfield_group_t_frozen, "uint8_t:1 frozen", NULL},
    {"editing", (getter) get_struct_bitfield_group_t_editing, (setter) set_struct_bitfield_group_t_editing, "uint8_t:1 editing", NULL},
    {"click_focus", (getter) get_struct_bitfield_group_t_click_focus, (setter) set_struct_bitfield_group_t_click_focus, "uint8_t:1 click_focus", NULL},
    {"refocus_policy", (getter) get_struct_bitfield_group_t_refocus_policy, (setter) set_struct_bitfield_group_t_refocus_policy, "uint8_t:1 refocus_policy", NULL},
    {"wrap", (getter) get_struct_bitfield_group_t_wrap, (setter) set_struct_bitfield_group_t_wrap, "uint8_t:1 wrap", NULL},
    {NULL}
};


static PyTypeObject pylv_group_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.group_t",
    .tp_doc = "lvgl group_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_group_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_group_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_group_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_group_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_group_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_group_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_theme_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_theme_t_Type, sizeof(lv_theme_t));
}

static PyGetSetDef pylv_theme_t_getset[] = {
    {"apply_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_theme_apply_cb_t apply_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_theme_t, apply_cb), sizeof(((lv_theme_t *)0)->apply_cb)})},
    {"apply_xcb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_theme_apply_xcb_t apply_xcb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_theme_t, apply_xcb), sizeof(((lv_theme_t *)0)->apply_xcb)})},
    {"base", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_theme_t base", & ((struct_closure_t){ &Blob_Type, offsetof(lv_theme_t, base), sizeof(((lv_theme_t *)0)->base)})},
    {"color_primary", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t color_primary", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_theme_t, color_primary), sizeof(lv_color16_t)})},
    {"color_secondary", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t color_secondary", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_theme_t, color_secondary), sizeof(lv_color16_t)})},
    {"font_small", (getter) struct_get_struct, (setter) struct_set_struct, "lv_font_t font_small", & ((struct_closure_t){ &Blob_Type, offsetof(lv_theme_t, font_small), sizeof(((lv_theme_t *)0)->font_small)})},
    {"font_normal", (getter) struct_get_struct, (setter) struct_set_struct, "lv_font_t font_normal", & ((struct_closure_t){ &Blob_Type, offsetof(lv_theme_t, font_normal), sizeof(((lv_theme_t *)0)->font_normal)})},
    {"font_subtitle", (getter) struct_get_struct, (setter) struct_set_struct, "lv_font_t font_subtitle", & ((struct_closure_t){ &Blob_Type, offsetof(lv_theme_t, font_subtitle), sizeof(((lv_theme_t *)0)->font_subtitle)})},
    {"font_title", (getter) struct_get_struct, (setter) struct_set_struct, "lv_font_t font_title", & ((struct_closure_t){ &Blob_Type, offsetof(lv_theme_t, font_title), sizeof(((lv_theme_t *)0)->font_title)})},
    {"flags", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t flags", (void*)offsetof(lv_theme_t, flags)},
    {"user_data", (getter) struct_get_struct, (setter) struct_set_struct, "void user_data", & ((struct_closure_t){ &Blob_Type, offsetof(lv_theme_t, user_data), sizeof(((lv_theme_t *)0)->user_data)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t",
    .tp_doc = "lvgl theme_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_theme_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_theme_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_theme_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_theme_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_theme_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_font_fmt_txt_glyph_dsc_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_font_fmt_txt_glyph_dsc_t_Type, sizeof(lv_font_fmt_txt_glyph_dsc_t));
}



static PyObject *
get_struct_bitfield_font_fmt_txt_glyph_dsc_t_bitmap_index(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_font_fmt_txt_glyph_dsc_t*)(self->data))->bitmap_index );
}

static int
set_struct_bitfield_font_fmt_txt_glyph_dsc_t_bitmap_index(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1048575)) return -1;
    ((lv_font_fmt_txt_glyph_dsc_t*)(self->data))->bitmap_index = v;
    return 0;
}



static PyObject *
get_struct_bitfield_font_fmt_txt_glyph_dsc_t_adv_w(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_font_fmt_txt_glyph_dsc_t*)(self->data))->adv_w );
}

static int
set_struct_bitfield_font_fmt_txt_glyph_dsc_t_adv_w(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 4095)) return -1;
    ((lv_font_fmt_txt_glyph_dsc_t*)(self->data))->adv_w = v;
    return 0;
}

static PyGetSetDef pylv_font_fmt_txt_glyph_dsc_t_getset[] = {
    {"bitmap_index", (getter) get_struct_bitfield_font_fmt_txt_glyph_dsc_t_bitmap_index, (setter) set_struct_bitfield_font_fmt_txt_glyph_dsc_t_bitmap_index, "uint32_t:20 bitmap_index", NULL},
    {"adv_w", (getter) get_struct_bitfield_font_fmt_txt_glyph_dsc_t_adv_w, (setter) set_struct_bitfield_font_fmt_txt_glyph_dsc_t_adv_w, "uint32_t:12 adv_w", NULL},
    {"box_w", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t box_w", (void*)offsetof(lv_font_fmt_txt_glyph_dsc_t, box_w)},
    {"box_h", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t box_h", (void*)offsetof(lv_font_fmt_txt_glyph_dsc_t, box_h)},
    {"ofs_x", (getter) struct_get_int8, (setter) struct_set_int8, "int8_t ofs_x", (void*)offsetof(lv_font_fmt_txt_glyph_dsc_t, ofs_x)},
    {"ofs_y", (getter) struct_get_int8, (setter) struct_set_int8, "int8_t ofs_y", (void*)offsetof(lv_font_fmt_txt_glyph_dsc_t, ofs_y)},
    {NULL}
};


static PyTypeObject pylv_font_fmt_txt_glyph_dsc_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.font_fmt_txt_glyph_dsc_t",
    .tp_doc = "lvgl font_fmt_txt_glyph_dsc_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_font_fmt_txt_glyph_dsc_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_font_fmt_txt_glyph_dsc_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_font_fmt_txt_glyph_dsc_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_font_fmt_txt_glyph_dsc_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_font_fmt_txt_glyph_dsc_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_font_fmt_txt_glyph_dsc_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_font_fmt_txt_cmap_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_font_fmt_txt_cmap_t_Type, sizeof(lv_font_fmt_txt_cmap_t));
}

static PyGetSetDef pylv_font_fmt_txt_cmap_t_getset[] = {
    {"range_start", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t range_start", (void*)offsetof(lv_font_fmt_txt_cmap_t, range_start)},
    {"range_length", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t range_length", (void*)offsetof(lv_font_fmt_txt_cmap_t, range_length)},
    {"glyph_id_start", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t glyph_id_start", (void*)offsetof(lv_font_fmt_txt_cmap_t, glyph_id_start)},
    {"unicode_list", (getter) struct_get_struct, (setter) struct_set_struct, "uint16_t unicode_list", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_fmt_txt_cmap_t, unicode_list), sizeof(((lv_font_fmt_txt_cmap_t *)0)->unicode_list)})},
    {"glyph_id_ofs_list", (getter) struct_get_struct, (setter) struct_set_struct, "void glyph_id_ofs_list", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_fmt_txt_cmap_t, glyph_id_ofs_list), sizeof(((lv_font_fmt_txt_cmap_t *)0)->glyph_id_ofs_list)})},
    {"list_length", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t list_length", (void*)offsetof(lv_font_fmt_txt_cmap_t, list_length)},
    {"type", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_font_fmt_txt_cmap_type_t type", (void*)offsetof(lv_font_fmt_txt_cmap_t, type)},
    {NULL}
};


static PyTypeObject pylv_font_fmt_txt_cmap_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.font_fmt_txt_cmap_t",
    .tp_doc = "lvgl font_fmt_txt_cmap_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_font_fmt_txt_cmap_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_font_fmt_txt_cmap_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_font_fmt_txt_cmap_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_font_fmt_txt_cmap_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_font_fmt_txt_cmap_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_font_fmt_txt_cmap_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_font_fmt_txt_kern_pair_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_font_fmt_txt_kern_pair_t_Type, sizeof(lv_font_fmt_txt_kern_pair_t));
}



static PyObject *
get_struct_bitfield_font_fmt_txt_kern_pair_t_pair_cnt(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_font_fmt_txt_kern_pair_t*)(self->data))->pair_cnt );
}

static int
set_struct_bitfield_font_fmt_txt_kern_pair_t_pair_cnt(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1073741823)) return -1;
    ((lv_font_fmt_txt_kern_pair_t*)(self->data))->pair_cnt = v;
    return 0;
}



static PyObject *
get_struct_bitfield_font_fmt_txt_kern_pair_t_glyph_ids_size(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_font_fmt_txt_kern_pair_t*)(self->data))->glyph_ids_size );
}

static int
set_struct_bitfield_font_fmt_txt_kern_pair_t_glyph_ids_size(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_font_fmt_txt_kern_pair_t*)(self->data))->glyph_ids_size = v;
    return 0;
}

static PyGetSetDef pylv_font_fmt_txt_kern_pair_t_getset[] = {
    {"glyph_ids", (getter) struct_get_struct, (setter) struct_set_struct, "void glyph_ids", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_fmt_txt_kern_pair_t, glyph_ids), sizeof(((lv_font_fmt_txt_kern_pair_t *)0)->glyph_ids)})},
    {"values", (getter) struct_get_struct, (setter) struct_set_struct, "int8_t values", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_fmt_txt_kern_pair_t, values), sizeof(((lv_font_fmt_txt_kern_pair_t *)0)->values)})},
    {"pair_cnt", (getter) get_struct_bitfield_font_fmt_txt_kern_pair_t_pair_cnt, (setter) set_struct_bitfield_font_fmt_txt_kern_pair_t_pair_cnt, "uint32_t:30 pair_cnt", NULL},
    {"glyph_ids_size", (getter) get_struct_bitfield_font_fmt_txt_kern_pair_t_glyph_ids_size, (setter) set_struct_bitfield_font_fmt_txt_kern_pair_t_glyph_ids_size, "uint32_t:2 glyph_ids_size", NULL},
    {NULL}
};


static PyTypeObject pylv_font_fmt_txt_kern_pair_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.font_fmt_txt_kern_pair_t",
    .tp_doc = "lvgl font_fmt_txt_kern_pair_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_font_fmt_txt_kern_pair_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_font_fmt_txt_kern_pair_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_font_fmt_txt_kern_pair_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_font_fmt_txt_kern_pair_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_font_fmt_txt_kern_pair_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_font_fmt_txt_kern_pair_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_font_fmt_txt_kern_classes_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_font_fmt_txt_kern_classes_t_Type, sizeof(lv_font_fmt_txt_kern_classes_t));
}

static PyGetSetDef pylv_font_fmt_txt_kern_classes_t_getset[] = {
    {"class_pair_values", (getter) struct_get_struct, (setter) struct_set_struct, "int8_t class_pair_values", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_fmt_txt_kern_classes_t, class_pair_values), sizeof(((lv_font_fmt_txt_kern_classes_t *)0)->class_pair_values)})},
    {"left_class_mapping", (getter) struct_get_struct, (setter) struct_set_struct, "uint8_t left_class_mapping", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_fmt_txt_kern_classes_t, left_class_mapping), sizeof(((lv_font_fmt_txt_kern_classes_t *)0)->left_class_mapping)})},
    {"right_class_mapping", (getter) struct_get_struct, (setter) struct_set_struct, "uint8_t right_class_mapping", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_fmt_txt_kern_classes_t, right_class_mapping), sizeof(((lv_font_fmt_txt_kern_classes_t *)0)->right_class_mapping)})},
    {"left_class_cnt", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t left_class_cnt", (void*)offsetof(lv_font_fmt_txt_kern_classes_t, left_class_cnt)},
    {"right_class_cnt", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t right_class_cnt", (void*)offsetof(lv_font_fmt_txt_kern_classes_t, right_class_cnt)},
    {NULL}
};


static PyTypeObject pylv_font_fmt_txt_kern_classes_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.font_fmt_txt_kern_classes_t",
    .tp_doc = "lvgl font_fmt_txt_kern_classes_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_font_fmt_txt_kern_classes_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_font_fmt_txt_kern_classes_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_font_fmt_txt_kern_classes_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_font_fmt_txt_kern_classes_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_font_fmt_txt_kern_classes_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_font_fmt_txt_kern_classes_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_font_fmt_txt_dsc_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_font_fmt_txt_dsc_t_Type, sizeof(lv_font_fmt_txt_dsc_t));
}



static PyObject *
get_struct_bitfield_font_fmt_txt_dsc_t_cmap_num(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_font_fmt_txt_dsc_t*)(self->data))->cmap_num );
}

static int
set_struct_bitfield_font_fmt_txt_dsc_t_cmap_num(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 511)) return -1;
    ((lv_font_fmt_txt_dsc_t*)(self->data))->cmap_num = v;
    return 0;
}



static PyObject *
get_struct_bitfield_font_fmt_txt_dsc_t_bpp(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_font_fmt_txt_dsc_t*)(self->data))->bpp );
}

static int
set_struct_bitfield_font_fmt_txt_dsc_t_bpp(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 15)) return -1;
    ((lv_font_fmt_txt_dsc_t*)(self->data))->bpp = v;
    return 0;
}



static PyObject *
get_struct_bitfield_font_fmt_txt_dsc_t_kern_classes(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_font_fmt_txt_dsc_t*)(self->data))->kern_classes );
}

static int
set_struct_bitfield_font_fmt_txt_dsc_t_kern_classes(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_font_fmt_txt_dsc_t*)(self->data))->kern_classes = v;
    return 0;
}



static PyObject *
get_struct_bitfield_font_fmt_txt_dsc_t_bitmap_format(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_font_fmt_txt_dsc_t*)(self->data))->bitmap_format );
}

static int
set_struct_bitfield_font_fmt_txt_dsc_t_bitmap_format(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_font_fmt_txt_dsc_t*)(self->data))->bitmap_format = v;
    return 0;
}

static PyGetSetDef pylv_font_fmt_txt_dsc_t_getset[] = {
    {"glyph_bitmap", (getter) struct_get_struct, (setter) struct_set_struct, "uint8_t glyph_bitmap", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_fmt_txt_dsc_t, glyph_bitmap), sizeof(((lv_font_fmt_txt_dsc_t *)0)->glyph_bitmap)})},
    {"glyph_dsc", (getter) struct_get_struct, (setter) struct_set_struct, "lv_font_fmt_txt_glyph_dsc_t glyph_dsc", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_fmt_txt_dsc_t, glyph_dsc), sizeof(((lv_font_fmt_txt_dsc_t *)0)->glyph_dsc)})},
    {"cmaps", (getter) struct_get_struct, (setter) struct_set_struct, "lv_font_fmt_txt_cmap_t cmaps", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_fmt_txt_dsc_t, cmaps), sizeof(((lv_font_fmt_txt_dsc_t *)0)->cmaps)})},
    {"kern_dsc", (getter) struct_get_struct, (setter) struct_set_struct, "void kern_dsc", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_fmt_txt_dsc_t, kern_dsc), sizeof(((lv_font_fmt_txt_dsc_t *)0)->kern_dsc)})},
    {"kern_scale", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t kern_scale", (void*)offsetof(lv_font_fmt_txt_dsc_t, kern_scale)},
    {"cmap_num", (getter) get_struct_bitfield_font_fmt_txt_dsc_t_cmap_num, (setter) set_struct_bitfield_font_fmt_txt_dsc_t_cmap_num, "uint16_t:9 cmap_num", NULL},
    {"bpp", (getter) get_struct_bitfield_font_fmt_txt_dsc_t_bpp, (setter) set_struct_bitfield_font_fmt_txt_dsc_t_bpp, "uint16_t:4 bpp", NULL},
    {"kern_classes", (getter) get_struct_bitfield_font_fmt_txt_dsc_t_kern_classes, (setter) set_struct_bitfield_font_fmt_txt_dsc_t_kern_classes, "uint16_t:1 kern_classes", NULL},
    {"bitmap_format", (getter) get_struct_bitfield_font_fmt_txt_dsc_t_bitmap_format, (setter) set_struct_bitfield_font_fmt_txt_dsc_t_bitmap_format, "uint16_t:2 bitmap_format", NULL},
    {"last_letter", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t last_letter", (void*)offsetof(lv_font_fmt_txt_dsc_t, last_letter)},
    {"last_glyph_id", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t last_glyph_id", (void*)offsetof(lv_font_fmt_txt_dsc_t, last_glyph_id)},
    {NULL}
};


static PyTypeObject pylv_font_fmt_txt_dsc_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.font_fmt_txt_dsc_t",
    .tp_doc = "lvgl font_fmt_txt_dsc_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_font_fmt_txt_dsc_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_font_fmt_txt_dsc_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_font_fmt_txt_dsc_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_font_fmt_txt_dsc_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_font_fmt_txt_dsc_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_font_fmt_txt_dsc_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_cont_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_cont_ext_t_Type, sizeof(lv_cont_ext_t));
}



static PyObject *
get_struct_bitfield_cont_ext_t_layout(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_cont_ext_t*)(self->data))->layout );
}

static int
set_struct_bitfield_cont_ext_t_layout(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 15)) return -1;
    ((lv_cont_ext_t*)(self->data))->layout = v;
    return 0;
}



static PyObject *
get_struct_bitfield_cont_ext_t_fit_left(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_cont_ext_t*)(self->data))->fit_left );
}

static int
set_struct_bitfield_cont_ext_t_fit_left(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_cont_ext_t*)(self->data))->fit_left = v;
    return 0;
}



static PyObject *
get_struct_bitfield_cont_ext_t_fit_right(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_cont_ext_t*)(self->data))->fit_right );
}

static int
set_struct_bitfield_cont_ext_t_fit_right(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_cont_ext_t*)(self->data))->fit_right = v;
    return 0;
}



static PyObject *
get_struct_bitfield_cont_ext_t_fit_top(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_cont_ext_t*)(self->data))->fit_top );
}

static int
set_struct_bitfield_cont_ext_t_fit_top(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_cont_ext_t*)(self->data))->fit_top = v;
    return 0;
}



static PyObject *
get_struct_bitfield_cont_ext_t_fit_bottom(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_cont_ext_t*)(self->data))->fit_bottom );
}

static int
set_struct_bitfield_cont_ext_t_fit_bottom(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_cont_ext_t*)(self->data))->fit_bottom = v;
    return 0;
}

static PyGetSetDef pylv_cont_ext_t_getset[] = {
    {"layout", (getter) get_struct_bitfield_cont_ext_t_layout, (setter) set_struct_bitfield_cont_ext_t_layout, "lv_layout_t:4 layout", NULL},
    {"fit_left", (getter) get_struct_bitfield_cont_ext_t_fit_left, (setter) set_struct_bitfield_cont_ext_t_fit_left, "lv_fit_t:2 fit_left", NULL},
    {"fit_right", (getter) get_struct_bitfield_cont_ext_t_fit_right, (setter) set_struct_bitfield_cont_ext_t_fit_right, "lv_fit_t:2 fit_right", NULL},
    {"fit_top", (getter) get_struct_bitfield_cont_ext_t_fit_top, (setter) set_struct_bitfield_cont_ext_t_fit_top, "lv_fit_t:2 fit_top", NULL},
    {"fit_bottom", (getter) get_struct_bitfield_cont_ext_t_fit_bottom, (setter) set_struct_bitfield_cont_ext_t_fit_bottom, "lv_fit_t:2 fit_bottom", NULL},
    {NULL}
};


static PyTypeObject pylv_cont_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.cont_ext_t",
    .tp_doc = "lvgl cont_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_cont_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_cont_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_cont_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_cont_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_cont_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_cont_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_btn_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_btn_ext_t_Type, sizeof(lv_btn_ext_t));
}



static PyObject *
get_struct_bitfield_btn_ext_t_checkable(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_btn_ext_t*)(self->data))->checkable );
}

static int
set_struct_bitfield_btn_ext_t_checkable(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_btn_ext_t*)(self->data))->checkable = v;
    return 0;
}

static PyGetSetDef pylv_btn_ext_t_getset[] = {
    {"cont", (getter) struct_get_struct, (setter) struct_set_struct, "lv_cont_ext_t cont", & ((struct_closure_t){ &pylv_cont_ext_t_Type, offsetof(lv_btn_ext_t, cont), sizeof(lv_cont_ext_t)})},
    {"checkable", (getter) get_struct_bitfield_btn_ext_t_checkable, (setter) set_struct_bitfield_btn_ext_t_checkable, "uint8_t:1 checkable", NULL},
    {NULL}
};


static PyTypeObject pylv_btn_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.btn_ext_t",
    .tp_doc = "lvgl btn_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_btn_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_btn_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_btn_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_btn_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_btn_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_btn_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_imgbtn_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_imgbtn_ext_t_Type, sizeof(lv_imgbtn_ext_t));
}



static PyObject *
get_struct_bitfield_imgbtn_ext_t_tiled(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_imgbtn_ext_t*)(self->data))->tiled );
}

static int
set_struct_bitfield_imgbtn_ext_t_tiled(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_imgbtn_ext_t*)(self->data))->tiled = v;
    return 0;
}

static PyGetSetDef pylv_imgbtn_ext_t_getset[] = {
    {"btn", (getter) struct_get_struct, (setter) struct_set_struct, "lv_btn_ext_t btn", & ((struct_closure_t){ &pylv_btn_ext_t_Type, offsetof(lv_imgbtn_ext_t, btn), sizeof(lv_btn_ext_t)})},
    {"img_src_mid", (getter) struct_get_struct, (setter) struct_set_struct, "void_LV_BTN_STATE_LAST img_src_mid", & ((struct_closure_t){ &Blob_Type, offsetof(lv_imgbtn_ext_t, img_src_mid), sizeof(((lv_imgbtn_ext_t *)0)->img_src_mid)})},
    {"act_cf", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_img_cf_t act_cf", (void*)offsetof(lv_imgbtn_ext_t, act_cf)},
    {"tiled", (getter) get_struct_bitfield_imgbtn_ext_t_tiled, (setter) set_struct_bitfield_imgbtn_ext_t_tiled, "uint8_t:1 tiled", NULL},
    {NULL}
};


static PyTypeObject pylv_imgbtn_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.imgbtn_ext_t",
    .tp_doc = "lvgl imgbtn_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_imgbtn_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_imgbtn_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_imgbtn_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_imgbtn_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_imgbtn_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_imgbtn_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_label_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_label_ext_t_Type, sizeof(lv_label_ext_t));
}



static PyObject *
get_struct_bitfield_label_ext_t_long_mode(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_label_ext_t*)(self->data))->long_mode );
}

static int
set_struct_bitfield_label_ext_t_long_mode(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 7)) return -1;
    ((lv_label_ext_t*)(self->data))->long_mode = v;
    return 0;
}



static PyObject *
get_struct_bitfield_label_ext_t_static_txt(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_label_ext_t*)(self->data))->static_txt );
}

static int
set_struct_bitfield_label_ext_t_static_txt(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_label_ext_t*)(self->data))->static_txt = v;
    return 0;
}



static PyObject *
get_struct_bitfield_label_ext_t_align(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_label_ext_t*)(self->data))->align );
}

static int
set_struct_bitfield_label_ext_t_align(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_label_ext_t*)(self->data))->align = v;
    return 0;
}



static PyObject *
get_struct_bitfield_label_ext_t_recolor(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_label_ext_t*)(self->data))->recolor );
}

static int
set_struct_bitfield_label_ext_t_recolor(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_label_ext_t*)(self->data))->recolor = v;
    return 0;
}



static PyObject *
get_struct_bitfield_label_ext_t_expand(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_label_ext_t*)(self->data))->expand );
}

static int
set_struct_bitfield_label_ext_t_expand(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_label_ext_t*)(self->data))->expand = v;
    return 0;
}



static PyObject *
get_struct_bitfield_label_ext_t_dot_tmp_alloc(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_label_ext_t*)(self->data))->dot_tmp_alloc );
}

static int
set_struct_bitfield_label_ext_t_dot_tmp_alloc(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_label_ext_t*)(self->data))->dot_tmp_alloc = v;
    return 0;
}

static PyGetSetDef pylv_label_ext_t_getset[] = {
    {"text", (getter) struct_get_struct, (setter) struct_set_struct, "char text", & ((struct_closure_t){ &Blob_Type, offsetof(lv_label_ext_t, text), sizeof(((lv_label_ext_t *)0)->text)})},
    {"dot", (getter) struct_get_struct, (setter) struct_set_struct, "union  {   char *tmp_ptr;   char tmp[3 + 1]; } dot", & ((struct_closure_t){ &pylv_label_ext_t_dot_Type, offsetof(lv_label_ext_t, dot), sizeof(((lv_label_ext_t *)0)->dot)})},
    {"dot_end", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t dot_end", (void*)offsetof(lv_label_ext_t, dot_end)},
    {"anim_speed", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t anim_speed", (void*)offsetof(lv_label_ext_t, anim_speed)},
    {"offset", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t offset", & ((struct_closure_t){ &pylv_point_t_Type, offsetof(lv_label_ext_t, offset), sizeof(lv_point_t)})},
    {"long_mode", (getter) get_struct_bitfield_label_ext_t_long_mode, (setter) set_struct_bitfield_label_ext_t_long_mode, "lv_label_long_mode_t:3 long_mode", NULL},
    {"static_txt", (getter) get_struct_bitfield_label_ext_t_static_txt, (setter) set_struct_bitfield_label_ext_t_static_txt, "uint8_t:1 static_txt", NULL},
    {"align", (getter) get_struct_bitfield_label_ext_t_align, (setter) set_struct_bitfield_label_ext_t_align, "uint8_t:2 align", NULL},
    {"recolor", (getter) get_struct_bitfield_label_ext_t_recolor, (setter) set_struct_bitfield_label_ext_t_recolor, "uint8_t:1 recolor", NULL},
    {"expand", (getter) get_struct_bitfield_label_ext_t_expand, (setter) set_struct_bitfield_label_ext_t_expand, "uint8_t:1 expand", NULL},
    {"dot_tmp_alloc", (getter) get_struct_bitfield_label_ext_t_dot_tmp_alloc, (setter) set_struct_bitfield_label_ext_t_dot_tmp_alloc, "uint8_t:1 dot_tmp_alloc", NULL},
    {NULL}
};


static PyTypeObject pylv_label_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.label_ext_t",
    .tp_doc = "lvgl label_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_label_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_label_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_label_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_label_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_label_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_label_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_img_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_img_ext_t_Type, sizeof(lv_img_ext_t));
}



static PyObject *
get_struct_bitfield_img_ext_t_src_type(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_img_ext_t*)(self->data))->src_type );
}

static int
set_struct_bitfield_img_ext_t_src_type(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_img_ext_t*)(self->data))->src_type = v;
    return 0;
}



static PyObject *
get_struct_bitfield_img_ext_t_auto_size(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_img_ext_t*)(self->data))->auto_size );
}

static int
set_struct_bitfield_img_ext_t_auto_size(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_img_ext_t*)(self->data))->auto_size = v;
    return 0;
}



static PyObject *
get_struct_bitfield_img_ext_t_cf(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_img_ext_t*)(self->data))->cf );
}

static int
set_struct_bitfield_img_ext_t_cf(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 31)) return -1;
    ((lv_img_ext_t*)(self->data))->cf = v;
    return 0;
}



static PyObject *
get_struct_bitfield_img_ext_t_antialias(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_img_ext_t*)(self->data))->antialias );
}

static int
set_struct_bitfield_img_ext_t_antialias(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_img_ext_t*)(self->data))->antialias = v;
    return 0;
}

static PyGetSetDef pylv_img_ext_t_getset[] = {
    {"src", (getter) struct_get_struct, (setter) struct_set_struct, "void src", & ((struct_closure_t){ &Blob_Type, offsetof(lv_img_ext_t, src), sizeof(((lv_img_ext_t *)0)->src)})},
    {"offset", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t offset", & ((struct_closure_t){ &pylv_point_t_Type, offsetof(lv_img_ext_t, offset), sizeof(lv_point_t)})},
    {"w", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t w", (void*)offsetof(lv_img_ext_t, w)},
    {"h", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t h", (void*)offsetof(lv_img_ext_t, h)},
    {"angle", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t angle", (void*)offsetof(lv_img_ext_t, angle)},
    {"pivot", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t pivot", & ((struct_closure_t){ &pylv_point_t_Type, offsetof(lv_img_ext_t, pivot), sizeof(lv_point_t)})},
    {"zoom", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t zoom", (void*)offsetof(lv_img_ext_t, zoom)},
    {"src_type", (getter) get_struct_bitfield_img_ext_t_src_type, (setter) set_struct_bitfield_img_ext_t_src_type, "uint8_t:2 src_type", NULL},
    {"auto_size", (getter) get_struct_bitfield_img_ext_t_auto_size, (setter) set_struct_bitfield_img_ext_t_auto_size, "uint8_t:1 auto_size", NULL},
    {"cf", (getter) get_struct_bitfield_img_ext_t_cf, (setter) set_struct_bitfield_img_ext_t_cf, "uint8_t:5 cf", NULL},
    {"antialias", (getter) get_struct_bitfield_img_ext_t_antialias, (setter) set_struct_bitfield_img_ext_t_antialias, "uint8_t:1 antialias", NULL},
    {NULL}
};


static PyTypeObject pylv_img_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.img_ext_t",
    .tp_doc = "lvgl img_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_img_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_img_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_img_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_img_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_img_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_img_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_line_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_line_ext_t_Type, sizeof(lv_line_ext_t));
}



static PyObject *
get_struct_bitfield_line_ext_t_auto_size(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_line_ext_t*)(self->data))->auto_size );
}

static int
set_struct_bitfield_line_ext_t_auto_size(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_line_ext_t*)(self->data))->auto_size = v;
    return 0;
}



static PyObject *
get_struct_bitfield_line_ext_t_y_inv(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_line_ext_t*)(self->data))->y_inv );
}

static int
set_struct_bitfield_line_ext_t_y_inv(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_line_ext_t*)(self->data))->y_inv = v;
    return 0;
}

static PyGetSetDef pylv_line_ext_t_getset[] = {
    {"point_array", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t point_array", & ((struct_closure_t){ &Blob_Type, offsetof(lv_line_ext_t, point_array), sizeof(((lv_line_ext_t *)0)->point_array)})},
    {"point_num", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t point_num", (void*)offsetof(lv_line_ext_t, point_num)},
    {"auto_size", (getter) get_struct_bitfield_line_ext_t_auto_size, (setter) set_struct_bitfield_line_ext_t_auto_size, "uint8_t:1 auto_size", NULL},
    {"y_inv", (getter) get_struct_bitfield_line_ext_t_y_inv, (setter) set_struct_bitfield_line_ext_t_y_inv, "uint8_t:1 y_inv", NULL},
    {NULL}
};


static PyTypeObject pylv_line_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.line_ext_t",
    .tp_doc = "lvgl line_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_line_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_line_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_line_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_line_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_line_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_line_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_page_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_page_ext_t_Type, sizeof(lv_page_ext_t));
}



static PyObject *
get_struct_bitfield_page_ext_t_scroll_prop(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_page_ext_t*)(self->data))->scroll_prop );
}

static int
set_struct_bitfield_page_ext_t_scroll_prop(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_page_ext_t*)(self->data))->scroll_prop = v;
    return 0;
}

static PyGetSetDef pylv_page_ext_t_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_cont_ext_t bg", & ((struct_closure_t){ &pylv_cont_ext_t_Type, offsetof(lv_page_ext_t, bg), sizeof(lv_cont_ext_t)})},
    {"scrl", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t scrl", & ((struct_closure_t){ &Blob_Type, offsetof(lv_page_ext_t, scrl), sizeof(((lv_page_ext_t *)0)->scrl)})},
    {"scrlbar", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_list_t style;   lv_area_t hor_area;   lv_area_t ver_area;   uint8_t hor_draw : 1;   uint8_t ver_draw : 1;   lv_scrollbar_mode_t mode : 3; } scrlbar", & ((struct_closure_t){ &pylv_page_ext_t_scrlbar_Type, offsetof(lv_page_ext_t, scrlbar), sizeof(((lv_page_ext_t *)0)->scrlbar)})},
    {"edge_flash", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_anim_value_t state;   lv_style_list_t style;   uint8_t enabled : 1;   uint8_t top_ip : 1;   uint8_t bottom_ip : 1;   uint8_t right_ip : 1;   uint8_t left_ip : 1; } edge_flash", & ((struct_closure_t){ &pylv_page_ext_t_edge_flash_Type, offsetof(lv_page_ext_t, edge_flash), sizeof(((lv_page_ext_t *)0)->edge_flash)})},
    {"anim_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t anim_time", (void*)offsetof(lv_page_ext_t, anim_time)},
    {"scroll_prop_obj", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t scroll_prop_obj", & ((struct_closure_t){ &Blob_Type, offsetof(lv_page_ext_t, scroll_prop_obj), sizeof(((lv_page_ext_t *)0)->scroll_prop_obj)})},
    {"scroll_prop", (getter) get_struct_bitfield_page_ext_t_scroll_prop, (setter) set_struct_bitfield_page_ext_t_scroll_prop, "uint8_t:1 scroll_prop", NULL},
    {NULL}
};


static PyTypeObject pylv_page_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.page_ext_t",
    .tp_doc = "lvgl page_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_page_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_page_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_page_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_page_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_page_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_page_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_list_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_list_ext_t_Type, sizeof(lv_list_ext_t));
}

static PyGetSetDef pylv_list_ext_t_getset[] = {
    {"page", (getter) struct_get_struct, (setter) struct_set_struct, "lv_page_ext_t page", & ((struct_closure_t){ &pylv_page_ext_t_Type, offsetof(lv_list_ext_t, page), sizeof(lv_page_ext_t)})},
    {"last_sel_btn", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t last_sel_btn", & ((struct_closure_t){ &Blob_Type, offsetof(lv_list_ext_t, last_sel_btn), sizeof(((lv_list_ext_t *)0)->last_sel_btn)})},
    {"act_sel_btn", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t act_sel_btn", & ((struct_closure_t){ &Blob_Type, offsetof(lv_list_ext_t, act_sel_btn), sizeof(((lv_list_ext_t *)0)->act_sel_btn)})},
    {NULL}
};


static PyTypeObject pylv_list_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.list_ext_t",
    .tp_doc = "lvgl list_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_list_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_list_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_list_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_list_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_list_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_list_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_chart_series_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_chart_series_t_Type, sizeof(lv_chart_series_t));
}



static PyObject *
get_struct_bitfield_chart_series_t_ext_buf_assigned(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_chart_series_t*)(self->data))->ext_buf_assigned );
}

static int
set_struct_bitfield_chart_series_t_ext_buf_assigned(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_chart_series_t*)(self->data))->ext_buf_assigned = v;
    return 0;
}



static PyObject *
get_struct_bitfield_chart_series_t_hidden(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_chart_series_t*)(self->data))->hidden );
}

static int
set_struct_bitfield_chart_series_t_hidden(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_chart_series_t*)(self->data))->hidden = v;
    return 0;
}



static PyObject *
get_struct_bitfield_chart_series_t_y_axis(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_chart_series_t*)(self->data))->y_axis );
}

static int
set_struct_bitfield_chart_series_t_y_axis(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_chart_series_t*)(self->data))->y_axis = v;
    return 0;
}

static PyGetSetDef pylv_chart_series_t_getset[] = {
    {"points", (getter) struct_get_struct, (setter) struct_set_struct, "lv_coord_t points", & ((struct_closure_t){ &Blob_Type, offsetof(lv_chart_series_t, points), sizeof(((lv_chart_series_t *)0)->points)})},
    {"color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t color", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_chart_series_t, color), sizeof(lv_color16_t)})},
    {"start_point", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t start_point", (void*)offsetof(lv_chart_series_t, start_point)},
    {"ext_buf_assigned", (getter) get_struct_bitfield_chart_series_t_ext_buf_assigned, (setter) set_struct_bitfield_chart_series_t_ext_buf_assigned, "uint8_t:1 ext_buf_assigned", NULL},
    {"hidden", (getter) get_struct_bitfield_chart_series_t_hidden, (setter) set_struct_bitfield_chart_series_t_hidden, "uint8_t:1 hidden", NULL},
    {"y_axis", (getter) get_struct_bitfield_chart_series_t_y_axis, (setter) set_struct_bitfield_chart_series_t_y_axis, "lv_chart_axis_t:1 y_axis", NULL},
    {NULL}
};


static PyTypeObject pylv_chart_series_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.chart_series_t",
    .tp_doc = "lvgl chart_series_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_chart_series_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_chart_series_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_chart_series_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_chart_series_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_chart_series_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_chart_series_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_chart_cursor_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_chart_cursor_t_Type, sizeof(lv_chart_cursor_t));
}



static PyObject *
get_struct_bitfield_chart_cursor_t_axes(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_chart_cursor_t*)(self->data))->axes );
}

static int
set_struct_bitfield_chart_cursor_t_axes(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 15)) return -1;
    ((lv_chart_cursor_t*)(self->data))->axes = v;
    return 0;
}

static PyGetSetDef pylv_chart_cursor_t_getset[] = {
    {"point", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t point", & ((struct_closure_t){ &pylv_point_t_Type, offsetof(lv_chart_cursor_t, point), sizeof(lv_point_t)})},
    {"color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t color", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_chart_cursor_t, color), sizeof(lv_color16_t)})},
    {"axes", (getter) get_struct_bitfield_chart_cursor_t_axes, (setter) set_struct_bitfield_chart_cursor_t_axes, "lv_cursor_direction_t:4 axes", NULL},
    {NULL}
};


static PyTypeObject pylv_chart_cursor_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.chart_cursor_t",
    .tp_doc = "lvgl chart_cursor_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_chart_cursor_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_chart_cursor_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_chart_cursor_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_chart_cursor_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_chart_cursor_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_chart_cursor_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_chart_axis_cfg_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_chart_axis_cfg_t_Type, sizeof(lv_chart_axis_cfg_t));
}

static PyGetSetDef pylv_chart_axis_cfg_t_getset[] = {
    {"list_of_values", (getter) struct_get_struct, (setter) struct_set_struct, "char list_of_values", & ((struct_closure_t){ &Blob_Type, offsetof(lv_chart_axis_cfg_t, list_of_values), sizeof(((lv_chart_axis_cfg_t *)0)->list_of_values)})},
    {"options", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_chart_axis_options_t options", (void*)offsetof(lv_chart_axis_cfg_t, options)},
    {"num_tick_marks", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t num_tick_marks", (void*)offsetof(lv_chart_axis_cfg_t, num_tick_marks)},
    {"major_tick_len", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t major_tick_len", (void*)offsetof(lv_chart_axis_cfg_t, major_tick_len)},
    {"minor_tick_len", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t minor_tick_len", (void*)offsetof(lv_chart_axis_cfg_t, minor_tick_len)},
    {NULL}
};


static PyTypeObject pylv_chart_axis_cfg_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.chart_axis_cfg_t",
    .tp_doc = "lvgl chart_axis_cfg_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_chart_axis_cfg_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_chart_axis_cfg_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_chart_axis_cfg_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_chart_axis_cfg_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_chart_axis_cfg_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_chart_axis_cfg_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_chart_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_chart_ext_t_Type, sizeof(lv_chart_ext_t));
}



static PyObject *
get_struct_bitfield_chart_ext_t_update_mode(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_chart_ext_t*)(self->data))->update_mode );
}

static int
set_struct_bitfield_chart_ext_t_update_mode(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_chart_ext_t*)(self->data))->update_mode = v;
    return 0;
}

static PyGetSetDef pylv_chart_ext_t_getset[] = {
    {"series_ll", (getter) struct_get_struct, (setter) struct_set_struct, "lv_ll_t series_ll", & ((struct_closure_t){ &pylv_ll_t_Type, offsetof(lv_chart_ext_t, series_ll), sizeof(lv_ll_t)})},
    {"cursors_ll", (getter) struct_get_struct, (setter) struct_set_struct, "lv_ll_t cursors_ll", & ((struct_closure_t){ &pylv_ll_t_Type, offsetof(lv_chart_ext_t, cursors_ll), sizeof(lv_ll_t)})},
    {"ymin", (getter) struct_get_struct, (setter) struct_set_struct, "lv_coord_t_LV_CHART_AXIS_LAST ymin", & ((struct_closure_t){ &Blob_Type, offsetof(lv_chart_ext_t, ymin), sizeof(((lv_chart_ext_t *)0)->ymin)})},
    {"ymax", (getter) struct_get_struct, (setter) struct_set_struct, "lv_coord_t_LV_CHART_AXIS_LAST ymax", & ((struct_closure_t){ &Blob_Type, offsetof(lv_chart_ext_t, ymax), sizeof(((lv_chart_ext_t *)0)->ymax)})},
    {"hdiv_cnt", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t hdiv_cnt", (void*)offsetof(lv_chart_ext_t, hdiv_cnt)},
    {"vdiv_cnt", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t vdiv_cnt", (void*)offsetof(lv_chart_ext_t, vdiv_cnt)},
    {"point_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t point_cnt", (void*)offsetof(lv_chart_ext_t, point_cnt)},
    {"style_series_bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_series_bg", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_chart_ext_t, style_series_bg), sizeof(lv_style_list_t)})},
    {"style_series", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_series", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_chart_ext_t, style_series), sizeof(lv_style_list_t)})},
    {"style_cursors", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_cursors", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_chart_ext_t, style_cursors), sizeof(lv_style_list_t)})},
    {"type", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_chart_type_t type", (void*)offsetof(lv_chart_ext_t, type)},
    {"y_axis", (getter) struct_get_struct, (setter) struct_set_struct, "lv_chart_axis_cfg_t y_axis", & ((struct_closure_t){ &pylv_chart_axis_cfg_t_Type, offsetof(lv_chart_ext_t, y_axis), sizeof(lv_chart_axis_cfg_t)})},
    {"x_axis", (getter) struct_get_struct, (setter) struct_set_struct, "lv_chart_axis_cfg_t x_axis", & ((struct_closure_t){ &pylv_chart_axis_cfg_t_Type, offsetof(lv_chart_ext_t, x_axis), sizeof(lv_chart_axis_cfg_t)})},
    {"secondary_y_axis", (getter) struct_get_struct, (setter) struct_set_struct, "lv_chart_axis_cfg_t secondary_y_axis", & ((struct_closure_t){ &pylv_chart_axis_cfg_t_Type, offsetof(lv_chart_ext_t, secondary_y_axis), sizeof(lv_chart_axis_cfg_t)})},
    {"update_mode", (getter) get_struct_bitfield_chart_ext_t_update_mode, (setter) set_struct_bitfield_chart_ext_t_update_mode, "uint8_t:1 update_mode", NULL},
    {NULL}
};


static PyTypeObject pylv_chart_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.chart_ext_t",
    .tp_doc = "lvgl chart_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_chart_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_chart_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_chart_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_chart_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_chart_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_chart_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_table_cell_format_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_table_cell_format_t_Type, sizeof(lv_table_cell_format_t));
}

static PyGetSetDef pylv_table_cell_format_t_getset[] = {
    {"s", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   uint8_t align : 2;   uint8_t right_merge : 1;   uint8_t type : 4;   uint8_t crop : 1; } s", & ((struct_closure_t){ &pylv_table_cell_format_t_s_Type, offsetof(lv_table_cell_format_t, s), sizeof(((lv_table_cell_format_t *)0)->s)})},
    {"format_byte", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t format_byte", (void*)offsetof(lv_table_cell_format_t, format_byte)},
    {NULL}
};


static PyTypeObject pylv_table_cell_format_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.table_cell_format_t",
    .tp_doc = "lvgl table_cell_format_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_table_cell_format_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_table_cell_format_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_table_cell_format_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_table_cell_format_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_table_cell_format_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_table_cell_format_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_table_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_table_ext_t_Type, sizeof(lv_table_ext_t));
}



static PyObject *
get_struct_bitfield_table_ext_t_cell_types(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_table_ext_t*)(self->data))->cell_types );
}

static int
set_struct_bitfield_table_ext_t_cell_types(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 15)) return -1;
    ((lv_table_ext_t*)(self->data))->cell_types = v;
    return 0;
}

static PyGetSetDef pylv_table_ext_t_getset[] = {
    {"col_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t col_cnt", (void*)offsetof(lv_table_ext_t, col_cnt)},
    {"row_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t row_cnt", (void*)offsetof(lv_table_ext_t, row_cnt)},
    {"cell_data", (getter) struct_get_struct, (setter) struct_set_struct, "char cell_data", & ((struct_closure_t){ &Blob_Type, offsetof(lv_table_ext_t, cell_data), sizeof(((lv_table_ext_t *)0)->cell_data)})},
    {"row_h", (getter) struct_get_struct, (setter) struct_set_struct, "lv_coord_t row_h", & ((struct_closure_t){ &Blob_Type, offsetof(lv_table_ext_t, row_h), sizeof(((lv_table_ext_t *)0)->row_h)})},
    {"cell_style", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t4 cell_style", & ((struct_closure_t){ &Blob_Type, offsetof(lv_table_ext_t, cell_style), sizeof(((lv_table_ext_t *)0)->cell_style)})},
    {"col_w", (getter) struct_get_struct, (setter) struct_set_struct, "lv_coord_t12 col_w", & ((struct_closure_t){ &Blob_Type, offsetof(lv_table_ext_t, col_w), sizeof(((lv_table_ext_t *)0)->col_w)})},
    {"cell_types", (getter) get_struct_bitfield_table_ext_t_cell_types, (setter) set_struct_bitfield_table_ext_t_cell_types, "uint16_t:4 cell_types", NULL},
    {NULL}
};


static PyTypeObject pylv_table_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.table_ext_t",
    .tp_doc = "lvgl table_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_table_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_table_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_table_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_table_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_table_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_table_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_checkbox_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_checkbox_ext_t_Type, sizeof(lv_checkbox_ext_t));
}

static PyGetSetDef pylv_checkbox_ext_t_getset[] = {
    {"bg_btn", (getter) struct_get_struct, (setter) struct_set_struct, "lv_btn_ext_t bg_btn", & ((struct_closure_t){ &pylv_btn_ext_t_Type, offsetof(lv_checkbox_ext_t, bg_btn), sizeof(lv_btn_ext_t)})},
    {"bullet", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t bullet", & ((struct_closure_t){ &Blob_Type, offsetof(lv_checkbox_ext_t, bullet), sizeof(((lv_checkbox_ext_t *)0)->bullet)})},
    {"label", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t label", & ((struct_closure_t){ &Blob_Type, offsetof(lv_checkbox_ext_t, label), sizeof(((lv_checkbox_ext_t *)0)->label)})},
    {NULL}
};


static PyTypeObject pylv_checkbox_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.checkbox_ext_t",
    .tp_doc = "lvgl checkbox_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_checkbox_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_checkbox_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_checkbox_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_checkbox_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_checkbox_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_checkbox_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_cpicker_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_cpicker_ext_t_Type, sizeof(lv_cpicker_ext_t));
}



static PyObject *
get_struct_bitfield_cpicker_ext_t_color_mode(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_cpicker_ext_t*)(self->data))->color_mode );
}

static int
set_struct_bitfield_cpicker_ext_t_color_mode(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_cpicker_ext_t*)(self->data))->color_mode = v;
    return 0;
}



static PyObject *
get_struct_bitfield_cpicker_ext_t_color_mode_fixed(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_cpicker_ext_t*)(self->data))->color_mode_fixed );
}

static int
set_struct_bitfield_cpicker_ext_t_color_mode_fixed(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_cpicker_ext_t*)(self->data))->color_mode_fixed = v;
    return 0;
}



static PyObject *
get_struct_bitfield_cpicker_ext_t_type(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_cpicker_ext_t*)(self->data))->type );
}

static int
set_struct_bitfield_cpicker_ext_t_type(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_cpicker_ext_t*)(self->data))->type = v;
    return 0;
}

static PyGetSetDef pylv_cpicker_ext_t_getset[] = {
    {"hsv", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_hsv_t hsv", & ((struct_closure_t){ &pylv_color_hsv_t_Type, offsetof(lv_cpicker_ext_t, hsv), sizeof(lv_color_hsv_t)})},
    {"knob", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_list_t style_list;   lv_point_t pos;   uint8_t colored : 1; } knob", & ((struct_closure_t){ &pylv_cpicker_ext_t_knob_Type, offsetof(lv_cpicker_ext_t, knob), sizeof(((lv_cpicker_ext_t *)0)->knob)})},
    {"last_click_time", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t last_click_time", (void*)offsetof(lv_cpicker_ext_t, last_click_time)},
    {"last_change_time", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t last_change_time", (void*)offsetof(lv_cpicker_ext_t, last_change_time)},
    {"last_press_point", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t last_press_point", & ((struct_closure_t){ &pylv_point_t_Type, offsetof(lv_cpicker_ext_t, last_press_point), sizeof(lv_point_t)})},
    {"color_mode", (getter) get_struct_bitfield_cpicker_ext_t_color_mode, (setter) set_struct_bitfield_cpicker_ext_t_color_mode, "lv_cpicker_color_mode_t:2 color_mode", NULL},
    {"color_mode_fixed", (getter) get_struct_bitfield_cpicker_ext_t_color_mode_fixed, (setter) set_struct_bitfield_cpicker_ext_t_color_mode_fixed, "uint8_t:1 color_mode_fixed", NULL},
    {"type", (getter) get_struct_bitfield_cpicker_ext_t_type, (setter) set_struct_bitfield_cpicker_ext_t_type, "lv_cpicker_type_t:1 type", NULL},
    {NULL}
};


static PyTypeObject pylv_cpicker_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.cpicker_ext_t",
    .tp_doc = "lvgl cpicker_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_cpicker_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_cpicker_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_cpicker_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_cpicker_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_cpicker_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_cpicker_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_bar_anim_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_bar_anim_t_Type, sizeof(lv_bar_anim_t));
}

static PyGetSetDef pylv_bar_anim_t_getset[] = {
    {"bar", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t bar", & ((struct_closure_t){ &Blob_Type, offsetof(lv_bar_anim_t, bar), sizeof(((lv_bar_anim_t *)0)->bar)})},
    {"anim_start", (getter) struct_get_int16, (setter) struct_set_int16, "lv_anim_value_t anim_start", (void*)offsetof(lv_bar_anim_t, anim_start)},
    {"anim_end", (getter) struct_get_int16, (setter) struct_set_int16, "lv_anim_value_t anim_end", (void*)offsetof(lv_bar_anim_t, anim_end)},
    {"anim_state", (getter) struct_get_int16, (setter) struct_set_int16, "lv_anim_value_t anim_state", (void*)offsetof(lv_bar_anim_t, anim_state)},
    {NULL}
};


static PyTypeObject pylv_bar_anim_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.bar_anim_t",
    .tp_doc = "lvgl bar_anim_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_bar_anim_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_bar_anim_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_bar_anim_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_bar_anim_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_bar_anim_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_bar_anim_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_bar_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_bar_ext_t_Type, sizeof(lv_bar_ext_t));
}



static PyObject *
get_struct_bitfield_bar_ext_t_type(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_bar_ext_t*)(self->data))->type );
}

static int
set_struct_bitfield_bar_ext_t_type(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_bar_ext_t*)(self->data))->type = v;
    return 0;
}

static PyGetSetDef pylv_bar_ext_t_getset[] = {
    {"cur_value", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t cur_value", (void*)offsetof(lv_bar_ext_t, cur_value)},
    {"min_value", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t min_value", (void*)offsetof(lv_bar_ext_t, min_value)},
    {"max_value", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t max_value", (void*)offsetof(lv_bar_ext_t, max_value)},
    {"start_value", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t start_value", (void*)offsetof(lv_bar_ext_t, start_value)},
    {"indic_area", (getter) struct_get_struct, (setter) struct_set_struct, "lv_area_t indic_area", & ((struct_closure_t){ &pylv_area_t_Type, offsetof(lv_bar_ext_t, indic_area), sizeof(lv_area_t)})},
    {"anim_time", (getter) struct_get_int16, (setter) struct_set_int16, "lv_anim_value_t anim_time", (void*)offsetof(lv_bar_ext_t, anim_time)},
    {"cur_value_anim", (getter) struct_get_struct, (setter) struct_set_struct, "lv_bar_anim_t cur_value_anim", & ((struct_closure_t){ &pylv_bar_anim_t_Type, offsetof(lv_bar_ext_t, cur_value_anim), sizeof(lv_bar_anim_t)})},
    {"start_value_anim", (getter) struct_get_struct, (setter) struct_set_struct, "lv_bar_anim_t start_value_anim", & ((struct_closure_t){ &pylv_bar_anim_t_Type, offsetof(lv_bar_ext_t, start_value_anim), sizeof(lv_bar_anim_t)})},
    {"type", (getter) get_struct_bitfield_bar_ext_t_type, (setter) set_struct_bitfield_bar_ext_t_type, "uint8_t:2 type", NULL},
    {"style_indic", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_indic", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_bar_ext_t, style_indic), sizeof(lv_style_list_t)})},
    {NULL}
};


static PyTypeObject pylv_bar_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.bar_ext_t",
    .tp_doc = "lvgl bar_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_bar_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_bar_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_bar_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_bar_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_bar_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_bar_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_slider_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_slider_ext_t_Type, sizeof(lv_slider_ext_t));
}



static PyObject *
get_struct_bitfield_slider_ext_t_dragging(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_slider_ext_t*)(self->data))->dragging );
}

static int
set_struct_bitfield_slider_ext_t_dragging(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_slider_ext_t*)(self->data))->dragging = v;
    return 0;
}



static PyObject *
get_struct_bitfield_slider_ext_t_left_knob_focus(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_slider_ext_t*)(self->data))->left_knob_focus );
}

static int
set_struct_bitfield_slider_ext_t_left_knob_focus(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_slider_ext_t*)(self->data))->left_knob_focus = v;
    return 0;
}

static PyGetSetDef pylv_slider_ext_t_getset[] = {
    {"bar", (getter) struct_get_struct, (setter) struct_set_struct, "lv_bar_ext_t bar", & ((struct_closure_t){ &pylv_bar_ext_t_Type, offsetof(lv_slider_ext_t, bar), sizeof(lv_bar_ext_t)})},
    {"style_knob", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_knob", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_slider_ext_t, style_knob), sizeof(lv_style_list_t)})},
    {"left_knob_area", (getter) struct_get_struct, (setter) struct_set_struct, "lv_area_t left_knob_area", & ((struct_closure_t){ &pylv_area_t_Type, offsetof(lv_slider_ext_t, left_knob_area), sizeof(lv_area_t)})},
    {"right_knob_area", (getter) struct_get_struct, (setter) struct_set_struct, "lv_area_t right_knob_area", & ((struct_closure_t){ &pylv_area_t_Type, offsetof(lv_slider_ext_t, right_knob_area), sizeof(lv_area_t)})},
    {"value_to_set", (getter) struct_get_struct, (setter) struct_set_struct, "int16_t value_to_set", & ((struct_closure_t){ &Blob_Type, offsetof(lv_slider_ext_t, value_to_set), sizeof(((lv_slider_ext_t *)0)->value_to_set)})},
    {"dragging", (getter) get_struct_bitfield_slider_ext_t_dragging, (setter) set_struct_bitfield_slider_ext_t_dragging, "uint8_t:1 dragging", NULL},
    {"left_knob_focus", (getter) get_struct_bitfield_slider_ext_t_left_knob_focus, (setter) set_struct_bitfield_slider_ext_t_left_knob_focus, "uint8_t:1 left_knob_focus", NULL},
    {NULL}
};


static PyTypeObject pylv_slider_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.slider_ext_t",
    .tp_doc = "lvgl slider_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_slider_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_slider_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_slider_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_slider_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_slider_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_slider_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_led_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_led_ext_t_Type, sizeof(lv_led_ext_t));
}

static PyGetSetDef pylv_led_ext_t_getset[] = {
    {"bright", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t bright", (void*)offsetof(lv_led_ext_t, bright)},
    {NULL}
};


static PyTypeObject pylv_led_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.led_ext_t",
    .tp_doc = "lvgl led_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_led_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_led_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_led_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_led_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_led_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_led_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_btnmatrix_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_btnmatrix_ext_t_Type, sizeof(lv_btnmatrix_ext_t));
}



static PyObject *
get_struct_bitfield_btnmatrix_ext_t_recolor(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_btnmatrix_ext_t*)(self->data))->recolor );
}

static int
set_struct_bitfield_btnmatrix_ext_t_recolor(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_btnmatrix_ext_t*)(self->data))->recolor = v;
    return 0;
}



static PyObject *
get_struct_bitfield_btnmatrix_ext_t_one_check(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_btnmatrix_ext_t*)(self->data))->one_check );
}

static int
set_struct_bitfield_btnmatrix_ext_t_one_check(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_btnmatrix_ext_t*)(self->data))->one_check = v;
    return 0;
}



static PyObject *
get_struct_bitfield_btnmatrix_ext_t_align(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_btnmatrix_ext_t*)(self->data))->align );
}

static int
set_struct_bitfield_btnmatrix_ext_t_align(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_btnmatrix_ext_t*)(self->data))->align = v;
    return 0;
}

static PyGetSetDef pylv_btnmatrix_ext_t_getset[] = {
    {"map_p", (getter) struct_get_struct, (setter) struct_set_struct, "char map_p", & ((struct_closure_t){ &Blob_Type, offsetof(lv_btnmatrix_ext_t, map_p), sizeof(((lv_btnmatrix_ext_t *)0)->map_p)})},
    {"button_areas", (getter) struct_get_struct, (setter) struct_set_struct, "lv_area_t button_areas", & ((struct_closure_t){ &Blob_Type, offsetof(lv_btnmatrix_ext_t, button_areas), sizeof(((lv_btnmatrix_ext_t *)0)->button_areas)})},
    {"ctrl_bits", (getter) struct_get_struct, (setter) struct_set_struct, "lv_btnmatrix_ctrl_t ctrl_bits", & ((struct_closure_t){ &Blob_Type, offsetof(lv_btnmatrix_ext_t, ctrl_bits), sizeof(((lv_btnmatrix_ext_t *)0)->ctrl_bits)})},
    {"style_btn", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_btn", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_btnmatrix_ext_t, style_btn), sizeof(lv_style_list_t)})},
    {"btn_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t btn_cnt", (void*)offsetof(lv_btnmatrix_ext_t, btn_cnt)},
    {"btn_id_pr", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t btn_id_pr", (void*)offsetof(lv_btnmatrix_ext_t, btn_id_pr)},
    {"btn_id_focused", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t btn_id_focused", (void*)offsetof(lv_btnmatrix_ext_t, btn_id_focused)},
    {"btn_id_act", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t btn_id_act", (void*)offsetof(lv_btnmatrix_ext_t, btn_id_act)},
    {"recolor", (getter) get_struct_bitfield_btnmatrix_ext_t_recolor, (setter) set_struct_bitfield_btnmatrix_ext_t_recolor, "uint8_t:1 recolor", NULL},
    {"one_check", (getter) get_struct_bitfield_btnmatrix_ext_t_one_check, (setter) set_struct_bitfield_btnmatrix_ext_t_one_check, "uint8_t:1 one_check", NULL},
    {"align", (getter) get_struct_bitfield_btnmatrix_ext_t_align, (setter) set_struct_bitfield_btnmatrix_ext_t_align, "uint8_t:2 align", NULL},
    {NULL}
};


static PyTypeObject pylv_btnmatrix_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.btnmatrix_ext_t",
    .tp_doc = "lvgl btnmatrix_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_btnmatrix_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_btnmatrix_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_btnmatrix_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_btnmatrix_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_btnmatrix_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_btnmatrix_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_keyboard_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_keyboard_ext_t_Type, sizeof(lv_keyboard_ext_t));
}



static PyObject *
get_struct_bitfield_keyboard_ext_t_cursor_mng(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_keyboard_ext_t*)(self->data))->cursor_mng );
}

static int
set_struct_bitfield_keyboard_ext_t_cursor_mng(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_keyboard_ext_t*)(self->data))->cursor_mng = v;
    return 0;
}

static PyGetSetDef pylv_keyboard_ext_t_getset[] = {
    {"btnm", (getter) struct_get_struct, (setter) struct_set_struct, "lv_btnmatrix_ext_t btnm", & ((struct_closure_t){ &pylv_btnmatrix_ext_t_Type, offsetof(lv_keyboard_ext_t, btnm), sizeof(lv_btnmatrix_ext_t)})},
    {"ta", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t ta", & ((struct_closure_t){ &Blob_Type, offsetof(lv_keyboard_ext_t, ta), sizeof(((lv_keyboard_ext_t *)0)->ta)})},
    {"mode", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_keyboard_mode_t mode", (void*)offsetof(lv_keyboard_ext_t, mode)},
    {"cursor_mng", (getter) get_struct_bitfield_keyboard_ext_t_cursor_mng, (setter) set_struct_bitfield_keyboard_ext_t_cursor_mng, "uint8_t:1 cursor_mng", NULL},
    {NULL}
};


static PyTypeObject pylv_keyboard_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.keyboard_ext_t",
    .tp_doc = "lvgl keyboard_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_keyboard_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_keyboard_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_keyboard_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_keyboard_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_keyboard_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_keyboard_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_dropdown_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_dropdown_ext_t_Type, sizeof(lv_dropdown_ext_t));
}



static PyObject *
get_struct_bitfield_dropdown_ext_t_dir(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_dropdown_ext_t*)(self->data))->dir );
}

static int
set_struct_bitfield_dropdown_ext_t_dir(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_dropdown_ext_t*)(self->data))->dir = v;
    return 0;
}



static PyObject *
get_struct_bitfield_dropdown_ext_t_show_selected(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_dropdown_ext_t*)(self->data))->show_selected );
}

static int
set_struct_bitfield_dropdown_ext_t_show_selected(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_dropdown_ext_t*)(self->data))->show_selected = v;
    return 0;
}



static PyObject *
get_struct_bitfield_dropdown_ext_t_static_txt(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_dropdown_ext_t*)(self->data))->static_txt );
}

static int
set_struct_bitfield_dropdown_ext_t_static_txt(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_dropdown_ext_t*)(self->data))->static_txt = v;
    return 0;
}

static PyGetSetDef pylv_dropdown_ext_t_getset[] = {
    {"page", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t page", & ((struct_closure_t){ &Blob_Type, offsetof(lv_dropdown_ext_t, page), sizeof(((lv_dropdown_ext_t *)0)->page)})},
    {"text", (getter) struct_get_struct, (setter) struct_set_struct, "char text", & ((struct_closure_t){ &Blob_Type, offsetof(lv_dropdown_ext_t, text), sizeof(((lv_dropdown_ext_t *)0)->text)})},
    {"symbol", (getter) struct_get_struct, (setter) struct_set_struct, "char symbol", & ((struct_closure_t){ &Blob_Type, offsetof(lv_dropdown_ext_t, symbol), sizeof(((lv_dropdown_ext_t *)0)->symbol)})},
    {"options", (getter) struct_get_struct, (setter) struct_set_struct, "char options", & ((struct_closure_t){ &Blob_Type, offsetof(lv_dropdown_ext_t, options), sizeof(((lv_dropdown_ext_t *)0)->options)})},
    {"style_selected", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_selected", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_dropdown_ext_t, style_selected), sizeof(lv_style_list_t)})},
    {"style_page", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_page", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_dropdown_ext_t, style_page), sizeof(lv_style_list_t)})},
    {"style_scrlbar", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_scrlbar", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_dropdown_ext_t, style_scrlbar), sizeof(lv_style_list_t)})},
    {"max_height", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t max_height", (void*)offsetof(lv_dropdown_ext_t, max_height)},
    {"option_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t option_cnt", (void*)offsetof(lv_dropdown_ext_t, option_cnt)},
    {"sel_opt_id", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t sel_opt_id", (void*)offsetof(lv_dropdown_ext_t, sel_opt_id)},
    {"sel_opt_id_orig", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t sel_opt_id_orig", (void*)offsetof(lv_dropdown_ext_t, sel_opt_id_orig)},
    {"pr_opt_id", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t pr_opt_id", (void*)offsetof(lv_dropdown_ext_t, pr_opt_id)},
    {"dir", (getter) get_struct_bitfield_dropdown_ext_t_dir, (setter) set_struct_bitfield_dropdown_ext_t_dir, "lv_dropdown_dir_t:2 dir", NULL},
    {"show_selected", (getter) get_struct_bitfield_dropdown_ext_t_show_selected, (setter) set_struct_bitfield_dropdown_ext_t_show_selected, "uint8_t:1 show_selected", NULL},
    {"static_txt", (getter) get_struct_bitfield_dropdown_ext_t_static_txt, (setter) set_struct_bitfield_dropdown_ext_t_static_txt, "uint8_t:1 static_txt", NULL},
    {NULL}
};


static PyTypeObject pylv_dropdown_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.dropdown_ext_t",
    .tp_doc = "lvgl dropdown_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_dropdown_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_dropdown_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_dropdown_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_dropdown_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_dropdown_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_dropdown_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_roller_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_roller_ext_t_Type, sizeof(lv_roller_ext_t));
}



static PyObject *
get_struct_bitfield_roller_ext_t_mode(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_roller_ext_t*)(self->data))->mode );
}

static int
set_struct_bitfield_roller_ext_t_mode(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_roller_ext_t*)(self->data))->mode = v;
    return 0;
}



static PyObject *
get_struct_bitfield_roller_ext_t_auto_fit(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_roller_ext_t*)(self->data))->auto_fit );
}

static int
set_struct_bitfield_roller_ext_t_auto_fit(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_roller_ext_t*)(self->data))->auto_fit = v;
    return 0;
}

static PyGetSetDef pylv_roller_ext_t_getset[] = {
    {"page", (getter) struct_get_struct, (setter) struct_set_struct, "lv_page_ext_t page", & ((struct_closure_t){ &pylv_page_ext_t_Type, offsetof(lv_roller_ext_t, page), sizeof(lv_page_ext_t)})},
    {"style_sel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_sel", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_roller_ext_t, style_sel), sizeof(lv_style_list_t)})},
    {"option_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t option_cnt", (void*)offsetof(lv_roller_ext_t, option_cnt)},
    {"sel_opt_id", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t sel_opt_id", (void*)offsetof(lv_roller_ext_t, sel_opt_id)},
    {"sel_opt_id_ori", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t sel_opt_id_ori", (void*)offsetof(lv_roller_ext_t, sel_opt_id_ori)},
    {"mode", (getter) get_struct_bitfield_roller_ext_t_mode, (setter) set_struct_bitfield_roller_ext_t_mode, "lv_roller_mode_t:1 mode", NULL},
    {"auto_fit", (getter) get_struct_bitfield_roller_ext_t_auto_fit, (setter) set_struct_bitfield_roller_ext_t_auto_fit, "uint8_t:1 auto_fit", NULL},
    {NULL}
};


static PyTypeObject pylv_roller_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.roller_ext_t",
    .tp_doc = "lvgl roller_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_roller_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_roller_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_roller_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_roller_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_roller_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_roller_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_textarea_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_textarea_ext_t_Type, sizeof(lv_textarea_ext_t));
}



static PyObject *
get_struct_bitfield_textarea_ext_t_pwd_mode(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_textarea_ext_t*)(self->data))->pwd_mode );
}

static int
set_struct_bitfield_textarea_ext_t_pwd_mode(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_textarea_ext_t*)(self->data))->pwd_mode = v;
    return 0;
}



static PyObject *
get_struct_bitfield_textarea_ext_t_one_line(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_textarea_ext_t*)(self->data))->one_line );
}

static int
set_struct_bitfield_textarea_ext_t_one_line(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_textarea_ext_t*)(self->data))->one_line = v;
    return 0;
}

static PyGetSetDef pylv_textarea_ext_t_getset[] = {
    {"page", (getter) struct_get_struct, (setter) struct_set_struct, "lv_page_ext_t page", & ((struct_closure_t){ &pylv_page_ext_t_Type, offsetof(lv_textarea_ext_t, page), sizeof(lv_page_ext_t)})},
    {"label", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t label", & ((struct_closure_t){ &Blob_Type, offsetof(lv_textarea_ext_t, label), sizeof(((lv_textarea_ext_t *)0)->label)})},
    {"placeholder_txt", (getter) struct_get_struct, (setter) struct_set_struct, "char placeholder_txt", & ((struct_closure_t){ &Blob_Type, offsetof(lv_textarea_ext_t, placeholder_txt), sizeof(((lv_textarea_ext_t *)0)->placeholder_txt)})},
    {"style_placeholder", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_placeholder", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_textarea_ext_t, style_placeholder), sizeof(lv_style_list_t)})},
    {"pwd_tmp", (getter) struct_get_struct, (setter) struct_set_struct, "char pwd_tmp", & ((struct_closure_t){ &Blob_Type, offsetof(lv_textarea_ext_t, pwd_tmp), sizeof(((lv_textarea_ext_t *)0)->pwd_tmp)})},
    {"accepted_chars", (getter) struct_get_struct, (setter) struct_set_struct, "char accepted_chars", & ((struct_closure_t){ &Blob_Type, offsetof(lv_textarea_ext_t, accepted_chars), sizeof(((lv_textarea_ext_t *)0)->accepted_chars)})},
    {"max_length", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t max_length", (void*)offsetof(lv_textarea_ext_t, max_length)},
    {"pwd_show_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t pwd_show_time", (void*)offsetof(lv_textarea_ext_t, pwd_show_time)},
    {"cursor", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_list_t style;   lv_coord_t valid_x;   uint32_t pos;   uint16_t blink_time;   lv_area_t area;   uint32_t txt_byte_pos;   uint8_t state : 1;   uint8_t hidden : 1;   uint8_t click_pos : 1; } cursor", & ((struct_closure_t){ &pylv_textarea_ext_t_cursor_Type, offsetof(lv_textarea_ext_t, cursor), sizeof(((lv_textarea_ext_t *)0)->cursor)})},
    {"pwd_mode", (getter) get_struct_bitfield_textarea_ext_t_pwd_mode, (setter) set_struct_bitfield_textarea_ext_t_pwd_mode, "uint8_t:1 pwd_mode", NULL},
    {"one_line", (getter) get_struct_bitfield_textarea_ext_t_one_line, (setter) set_struct_bitfield_textarea_ext_t_one_line, "uint8_t:1 one_line", NULL},
    {NULL}
};


static PyTypeObject pylv_textarea_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.textarea_ext_t",
    .tp_doc = "lvgl textarea_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_textarea_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_textarea_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_textarea_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_textarea_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_textarea_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_textarea_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_canvas_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_canvas_ext_t_Type, sizeof(lv_canvas_ext_t));
}

static PyGetSetDef pylv_canvas_ext_t_getset[] = {
    {"img", (getter) struct_get_struct, (setter) struct_set_struct, "lv_img_ext_t img", & ((struct_closure_t){ &pylv_img_ext_t_Type, offsetof(lv_canvas_ext_t, img), sizeof(lv_img_ext_t)})},
    {"dsc", (getter) struct_get_struct, (setter) struct_set_struct, "lv_img_dsc_t dsc", & ((struct_closure_t){ &pylv_img_dsc_t_Type, offsetof(lv_canvas_ext_t, dsc), sizeof(lv_img_dsc_t)})},
    {NULL}
};


static PyTypeObject pylv_canvas_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.canvas_ext_t",
    .tp_doc = "lvgl canvas_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_canvas_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_canvas_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_canvas_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_canvas_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_canvas_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_canvas_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_win_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_win_ext_t_Type, sizeof(lv_win_ext_t));
}

static PyGetSetDef pylv_win_ext_t_getset[] = {
    {"page", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t page", & ((struct_closure_t){ &Blob_Type, offsetof(lv_win_ext_t, page), sizeof(((lv_win_ext_t *)0)->page)})},
    {"header", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t header", & ((struct_closure_t){ &Blob_Type, offsetof(lv_win_ext_t, header), sizeof(((lv_win_ext_t *)0)->header)})},
    {"title_txt", (getter) struct_get_struct, (setter) struct_set_struct, "char title_txt", & ((struct_closure_t){ &Blob_Type, offsetof(lv_win_ext_t, title_txt), sizeof(((lv_win_ext_t *)0)->title_txt)})},
    {"btn_w", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t btn_w", (void*)offsetof(lv_win_ext_t, btn_w)},
    {"title_txt_align", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t title_txt_align", (void*)offsetof(lv_win_ext_t, title_txt_align)},
    {NULL}
};


static PyTypeObject pylv_win_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.win_ext_t",
    .tp_doc = "lvgl win_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_win_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_win_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_win_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_win_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_win_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_win_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_tabview_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_tabview_ext_t_Type, sizeof(lv_tabview_ext_t));
}



static PyObject *
get_struct_bitfield_tabview_ext_t_btns_pos(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_tabview_ext_t*)(self->data))->btns_pos );
}

static int
set_struct_bitfield_tabview_ext_t_btns_pos(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 7)) return -1;
    ((lv_tabview_ext_t*)(self->data))->btns_pos = v;
    return 0;
}

static PyGetSetDef pylv_tabview_ext_t_getset[] = {
    {"btns", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t btns", & ((struct_closure_t){ &Blob_Type, offsetof(lv_tabview_ext_t, btns), sizeof(((lv_tabview_ext_t *)0)->btns)})},
    {"indic", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t indic", & ((struct_closure_t){ &Blob_Type, offsetof(lv_tabview_ext_t, indic), sizeof(((lv_tabview_ext_t *)0)->indic)})},
    {"content", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t content", & ((struct_closure_t){ &Blob_Type, offsetof(lv_tabview_ext_t, content), sizeof(((lv_tabview_ext_t *)0)->content)})},
    {"tab_name_ptr", (getter) struct_get_struct, (setter) struct_set_struct, "char tab_name_ptr", & ((struct_closure_t){ &Blob_Type, offsetof(lv_tabview_ext_t, tab_name_ptr), sizeof(((lv_tabview_ext_t *)0)->tab_name_ptr)})},
    {"point_last", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t point_last", & ((struct_closure_t){ &pylv_point_t_Type, offsetof(lv_tabview_ext_t, point_last), sizeof(lv_point_t)})},
    {"tab_cur", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t tab_cur", (void*)offsetof(lv_tabview_ext_t, tab_cur)},
    {"tab_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t tab_cnt", (void*)offsetof(lv_tabview_ext_t, tab_cnt)},
    {"anim_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t anim_time", (void*)offsetof(lv_tabview_ext_t, anim_time)},
    {"btns_pos", (getter) get_struct_bitfield_tabview_ext_t_btns_pos, (setter) set_struct_bitfield_tabview_ext_t_btns_pos, "lv_tabview_btns_pos_t:3 btns_pos", NULL},
    {NULL}
};


static PyTypeObject pylv_tabview_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.tabview_ext_t",
    .tp_doc = "lvgl tabview_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_tabview_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_tabview_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_tabview_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_tabview_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_tabview_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_tabview_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_tileview_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_tileview_ext_t_Type, sizeof(lv_tileview_ext_t));
}



static PyObject *
get_struct_bitfield_tileview_ext_t_drag_top_en(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_tileview_ext_t*)(self->data))->drag_top_en );
}

static int
set_struct_bitfield_tileview_ext_t_drag_top_en(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_tileview_ext_t*)(self->data))->drag_top_en = v;
    return 0;
}



static PyObject *
get_struct_bitfield_tileview_ext_t_drag_bottom_en(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_tileview_ext_t*)(self->data))->drag_bottom_en );
}

static int
set_struct_bitfield_tileview_ext_t_drag_bottom_en(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_tileview_ext_t*)(self->data))->drag_bottom_en = v;
    return 0;
}



static PyObject *
get_struct_bitfield_tileview_ext_t_drag_left_en(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_tileview_ext_t*)(self->data))->drag_left_en );
}

static int
set_struct_bitfield_tileview_ext_t_drag_left_en(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_tileview_ext_t*)(self->data))->drag_left_en = v;
    return 0;
}



static PyObject *
get_struct_bitfield_tileview_ext_t_drag_right_en(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_tileview_ext_t*)(self->data))->drag_right_en );
}

static int
set_struct_bitfield_tileview_ext_t_drag_right_en(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_tileview_ext_t*)(self->data))->drag_right_en = v;
    return 0;
}

static PyGetSetDef pylv_tileview_ext_t_getset[] = {
    {"page", (getter) struct_get_struct, (setter) struct_set_struct, "lv_page_ext_t page", & ((struct_closure_t){ &pylv_page_ext_t_Type, offsetof(lv_tileview_ext_t, page), sizeof(lv_page_ext_t)})},
    {"valid_pos", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t valid_pos", & ((struct_closure_t){ &Blob_Type, offsetof(lv_tileview_ext_t, valid_pos), sizeof(((lv_tileview_ext_t *)0)->valid_pos)})},
    {"valid_pos_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t valid_pos_cnt", (void*)offsetof(lv_tileview_ext_t, valid_pos_cnt)},
    {"anim_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t anim_time", (void*)offsetof(lv_tileview_ext_t, anim_time)},
    {"act_id", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t act_id", & ((struct_closure_t){ &pylv_point_t_Type, offsetof(lv_tileview_ext_t, act_id), sizeof(lv_point_t)})},
    {"drag_top_en", (getter) get_struct_bitfield_tileview_ext_t_drag_top_en, (setter) set_struct_bitfield_tileview_ext_t_drag_top_en, "uint8_t:1 drag_top_en", NULL},
    {"drag_bottom_en", (getter) get_struct_bitfield_tileview_ext_t_drag_bottom_en, (setter) set_struct_bitfield_tileview_ext_t_drag_bottom_en, "uint8_t:1 drag_bottom_en", NULL},
    {"drag_left_en", (getter) get_struct_bitfield_tileview_ext_t_drag_left_en, (setter) set_struct_bitfield_tileview_ext_t_drag_left_en, "uint8_t:1 drag_left_en", NULL},
    {"drag_right_en", (getter) get_struct_bitfield_tileview_ext_t_drag_right_en, (setter) set_struct_bitfield_tileview_ext_t_drag_right_en, "uint8_t:1 drag_right_en", NULL},
    {NULL}
};


static PyTypeObject pylv_tileview_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.tileview_ext_t",
    .tp_doc = "lvgl tileview_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_tileview_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_tileview_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_tileview_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_tileview_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_tileview_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_tileview_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_msgbox_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_msgbox_ext_t_Type, sizeof(lv_msgbox_ext_t));
}

static PyGetSetDef pylv_msgbox_ext_t_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_cont_ext_t bg", & ((struct_closure_t){ &pylv_cont_ext_t_Type, offsetof(lv_msgbox_ext_t, bg), sizeof(lv_cont_ext_t)})},
    {"text", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t text", & ((struct_closure_t){ &Blob_Type, offsetof(lv_msgbox_ext_t, text), sizeof(((lv_msgbox_ext_t *)0)->text)})},
    {"btnm", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t btnm", & ((struct_closure_t){ &Blob_Type, offsetof(lv_msgbox_ext_t, btnm), sizeof(((lv_msgbox_ext_t *)0)->btnm)})},
    {"anim_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t anim_time", (void*)offsetof(lv_msgbox_ext_t, anim_time)},
    {NULL}
};


static PyTypeObject pylv_msgbox_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.msgbox_ext_t",
    .tp_doc = "lvgl msgbox_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_msgbox_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_msgbox_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_msgbox_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_msgbox_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_msgbox_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_msgbox_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_objmask_mask_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_objmask_mask_t_Type, sizeof(lv_objmask_mask_t));
}

static PyGetSetDef pylv_objmask_mask_t_getset[] = {
    {"param", (getter) struct_get_struct, (setter) struct_set_struct, "void param", & ((struct_closure_t){ &Blob_Type, offsetof(lv_objmask_mask_t, param), sizeof(((lv_objmask_mask_t *)0)->param)})},
    {NULL}
};


static PyTypeObject pylv_objmask_mask_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.objmask_mask_t",
    .tp_doc = "lvgl objmask_mask_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_objmask_mask_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_objmask_mask_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_objmask_mask_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_objmask_mask_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_objmask_mask_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_objmask_mask_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_objmask_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_objmask_ext_t_Type, sizeof(lv_objmask_ext_t));
}

static PyGetSetDef pylv_objmask_ext_t_getset[] = {
    {"cont", (getter) struct_get_struct, (setter) struct_set_struct, "lv_cont_ext_t cont", & ((struct_closure_t){ &pylv_cont_ext_t_Type, offsetof(lv_objmask_ext_t, cont), sizeof(lv_cont_ext_t)})},
    {"mask_ll", (getter) struct_get_struct, (setter) struct_set_struct, "lv_ll_t mask_ll", & ((struct_closure_t){ &pylv_ll_t_Type, offsetof(lv_objmask_ext_t, mask_ll), sizeof(lv_ll_t)})},
    {NULL}
};


static PyTypeObject pylv_objmask_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.objmask_ext_t",
    .tp_doc = "lvgl objmask_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_objmask_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_objmask_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_objmask_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_objmask_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_objmask_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_objmask_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_linemeter_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_linemeter_ext_t_Type, sizeof(lv_linemeter_ext_t));
}



static PyObject *
get_struct_bitfield_linemeter_ext_t_mirrored(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_linemeter_ext_t*)(self->data))->mirrored );
}

static int
set_struct_bitfield_linemeter_ext_t_mirrored(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_linemeter_ext_t*)(self->data))->mirrored = v;
    return 0;
}

static PyGetSetDef pylv_linemeter_ext_t_getset[] = {
    {"scale_angle", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t scale_angle", (void*)offsetof(lv_linemeter_ext_t, scale_angle)},
    {"angle_ofs", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t angle_ofs", (void*)offsetof(lv_linemeter_ext_t, angle_ofs)},
    {"line_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t line_cnt", (void*)offsetof(lv_linemeter_ext_t, line_cnt)},
    {"cur_value", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t cur_value", (void*)offsetof(lv_linemeter_ext_t, cur_value)},
    {"min_value", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t min_value", (void*)offsetof(lv_linemeter_ext_t, min_value)},
    {"max_value", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t max_value", (void*)offsetof(lv_linemeter_ext_t, max_value)},
    {"mirrored", (getter) get_struct_bitfield_linemeter_ext_t_mirrored, (setter) set_struct_bitfield_linemeter_ext_t_mirrored, "uint8_t:1 mirrored", NULL},
    {NULL}
};


static PyTypeObject pylv_linemeter_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.linemeter_ext_t",
    .tp_doc = "lvgl linemeter_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_linemeter_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_linemeter_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_linemeter_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_linemeter_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_linemeter_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_linemeter_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_gauge_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_gauge_ext_t_Type, sizeof(lv_gauge_ext_t));
}

static PyGetSetDef pylv_gauge_ext_t_getset[] = {
    {"lmeter", (getter) struct_get_struct, (setter) struct_set_struct, "lv_linemeter_ext_t lmeter", & ((struct_closure_t){ &pylv_linemeter_ext_t_Type, offsetof(lv_gauge_ext_t, lmeter), sizeof(lv_linemeter_ext_t)})},
    {"values", (getter) struct_get_struct, (setter) struct_set_struct, "int32_t values", & ((struct_closure_t){ &Blob_Type, offsetof(lv_gauge_ext_t, values), sizeof(((lv_gauge_ext_t *)0)->values)})},
    {"needle_colors", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t needle_colors", & ((struct_closure_t){ &Blob_Type, offsetof(lv_gauge_ext_t, needle_colors), sizeof(((lv_gauge_ext_t *)0)->needle_colors)})},
    {"needle_img", (getter) struct_get_struct, (setter) struct_set_struct, "void needle_img", & ((struct_closure_t){ &Blob_Type, offsetof(lv_gauge_ext_t, needle_img), sizeof(((lv_gauge_ext_t *)0)->needle_img)})},
    {"needle_img_pivot", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t needle_img_pivot", & ((struct_closure_t){ &pylv_point_t_Type, offsetof(lv_gauge_ext_t, needle_img_pivot), sizeof(lv_point_t)})},
    {"style_needle", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_needle", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_gauge_ext_t, style_needle), sizeof(lv_style_list_t)})},
    {"style_strong", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_strong", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_gauge_ext_t, style_strong), sizeof(lv_style_list_t)})},
    {"needle_count", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t needle_count", (void*)offsetof(lv_gauge_ext_t, needle_count)},
    {"label_count", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t label_count", (void*)offsetof(lv_gauge_ext_t, label_count)},
    {"format_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_gauge_format_cb_t format_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_gauge_ext_t, format_cb), sizeof(((lv_gauge_ext_t *)0)->format_cb)})},
    {NULL}
};


static PyTypeObject pylv_gauge_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.gauge_ext_t",
    .tp_doc = "lvgl gauge_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_gauge_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_gauge_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_gauge_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_gauge_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_gauge_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_gauge_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_switch_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_switch_ext_t_Type, sizeof(lv_switch_ext_t));
}

static PyGetSetDef pylv_switch_ext_t_getset[] = {
    {"bar", (getter) struct_get_struct, (setter) struct_set_struct, "lv_bar_ext_t bar", & ((struct_closure_t){ &pylv_bar_ext_t_Type, offsetof(lv_switch_ext_t, bar), sizeof(lv_bar_ext_t)})},
    {"style_knob", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_knob", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_switch_ext_t, style_knob), sizeof(lv_style_list_t)})},
    {NULL}
};


static PyTypeObject pylv_switch_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.switch_ext_t",
    .tp_doc = "lvgl switch_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_switch_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_switch_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_switch_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_switch_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_switch_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_switch_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_arc_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_arc_ext_t_Type, sizeof(lv_arc_ext_t));
}



static PyObject *
get_struct_bitfield_arc_ext_t_dragging(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_arc_ext_t*)(self->data))->dragging );
}

static int
set_struct_bitfield_arc_ext_t_dragging(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_arc_ext_t*)(self->data))->dragging = v;
    return 0;
}



static PyObject *
get_struct_bitfield_arc_ext_t_type(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_arc_ext_t*)(self->data))->type );
}

static int
set_struct_bitfield_arc_ext_t_type(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_arc_ext_t*)(self->data))->type = v;
    return 0;
}



static PyObject *
get_struct_bitfield_arc_ext_t_adjustable(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_arc_ext_t*)(self->data))->adjustable );
}

static int
set_struct_bitfield_arc_ext_t_adjustable(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_arc_ext_t*)(self->data))->adjustable = v;
    return 0;
}



static PyObject *
get_struct_bitfield_arc_ext_t_min_close(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_arc_ext_t*)(self->data))->min_close );
}

static int
set_struct_bitfield_arc_ext_t_min_close(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_arc_ext_t*)(self->data))->min_close = v;
    return 0;
}

static PyGetSetDef pylv_arc_ext_t_getset[] = {
    {"rotation_angle", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t rotation_angle", (void*)offsetof(lv_arc_ext_t, rotation_angle)},
    {"arc_angle_start", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t arc_angle_start", (void*)offsetof(lv_arc_ext_t, arc_angle_start)},
    {"arc_angle_end", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t arc_angle_end", (void*)offsetof(lv_arc_ext_t, arc_angle_end)},
    {"bg_angle_start", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t bg_angle_start", (void*)offsetof(lv_arc_ext_t, bg_angle_start)},
    {"bg_angle_end", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t bg_angle_end", (void*)offsetof(lv_arc_ext_t, bg_angle_end)},
    {"style_arc", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_arc", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_arc_ext_t, style_arc), sizeof(lv_style_list_t)})},
    {"style_knob", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_knob", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_arc_ext_t, style_knob), sizeof(lv_style_list_t)})},
    {"cur_value", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t cur_value", (void*)offsetof(lv_arc_ext_t, cur_value)},
    {"min_value", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t min_value", (void*)offsetof(lv_arc_ext_t, min_value)},
    {"max_value", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t max_value", (void*)offsetof(lv_arc_ext_t, max_value)},
    {"dragging", (getter) get_struct_bitfield_arc_ext_t_dragging, (setter) set_struct_bitfield_arc_ext_t_dragging, "uint16_t:1 dragging", NULL},
    {"type", (getter) get_struct_bitfield_arc_ext_t_type, (setter) set_struct_bitfield_arc_ext_t_type, "uint16_t:2 type", NULL},
    {"adjustable", (getter) get_struct_bitfield_arc_ext_t_adjustable, (setter) set_struct_bitfield_arc_ext_t_adjustable, "uint16_t:1 adjustable", NULL},
    {"min_close", (getter) get_struct_bitfield_arc_ext_t_min_close, (setter) set_struct_bitfield_arc_ext_t_min_close, "uint16_t:1 min_close", NULL},
    {"chg_rate", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t chg_rate", (void*)offsetof(lv_arc_ext_t, chg_rate)},
    {"last_tick", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t last_tick", (void*)offsetof(lv_arc_ext_t, last_tick)},
    {"last_angle", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t last_angle", (void*)offsetof(lv_arc_ext_t, last_angle)},
    {NULL}
};


static PyTypeObject pylv_arc_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.arc_ext_t",
    .tp_doc = "lvgl arc_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_arc_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_arc_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_arc_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_arc_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_arc_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_arc_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_spinner_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_spinner_ext_t_Type, sizeof(lv_spinner_ext_t));
}



static PyObject *
get_struct_bitfield_spinner_ext_t_anim_type(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_spinner_ext_t*)(self->data))->anim_type );
}

static int
set_struct_bitfield_spinner_ext_t_anim_type(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_spinner_ext_t*)(self->data))->anim_type = v;
    return 0;
}



static PyObject *
get_struct_bitfield_spinner_ext_t_anim_dir(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_spinner_ext_t*)(self->data))->anim_dir );
}

static int
set_struct_bitfield_spinner_ext_t_anim_dir(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_spinner_ext_t*)(self->data))->anim_dir = v;
    return 0;
}

static PyGetSetDef pylv_spinner_ext_t_getset[] = {
    {"arc", (getter) struct_get_struct, (setter) struct_set_struct, "lv_arc_ext_t arc", & ((struct_closure_t){ &pylv_arc_ext_t_Type, offsetof(lv_spinner_ext_t, arc), sizeof(lv_arc_ext_t)})},
    {"arc_length", (getter) struct_get_int16, (setter) struct_set_int16, "lv_anim_value_t arc_length", (void*)offsetof(lv_spinner_ext_t, arc_length)},
    {"time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t time", (void*)offsetof(lv_spinner_ext_t, time)},
    {"anim_type", (getter) get_struct_bitfield_spinner_ext_t_anim_type, (setter) set_struct_bitfield_spinner_ext_t_anim_type, "lv_spinner_type_t:2 anim_type", NULL},
    {"anim_dir", (getter) get_struct_bitfield_spinner_ext_t_anim_dir, (setter) set_struct_bitfield_spinner_ext_t_anim_dir, "lv_spinner_dir_t:1 anim_dir", NULL},
    {NULL}
};


static PyTypeObject pylv_spinner_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.spinner_ext_t",
    .tp_doc = "lvgl spinner_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_spinner_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_spinner_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_spinner_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_spinner_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_spinner_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_spinner_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_calendar_date_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_calendar_date_t_Type, sizeof(lv_calendar_date_t));
}

static PyGetSetDef pylv_calendar_date_t_getset[] = {
    {"year", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t year", (void*)offsetof(lv_calendar_date_t, year)},
    {"month", (getter) struct_get_int8, (setter) struct_set_int8, "int8_t month", (void*)offsetof(lv_calendar_date_t, month)},
    {"day", (getter) struct_get_int8, (setter) struct_set_int8, "int8_t day", (void*)offsetof(lv_calendar_date_t, day)},
    {NULL}
};


static PyTypeObject pylv_calendar_date_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.calendar_date_t",
    .tp_doc = "lvgl calendar_date_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_calendar_date_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_calendar_date_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_calendar_date_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_calendar_date_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_calendar_date_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_calendar_date_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_calendar_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_calendar_ext_t_Type, sizeof(lv_calendar_ext_t));
}

static PyGetSetDef pylv_calendar_ext_t_getset[] = {
    {"today", (getter) struct_get_struct, (setter) struct_set_struct, "lv_calendar_date_t today", & ((struct_closure_t){ &pylv_calendar_date_t_Type, offsetof(lv_calendar_ext_t, today), sizeof(lv_calendar_date_t)})},
    {"showed_date", (getter) struct_get_struct, (setter) struct_set_struct, "lv_calendar_date_t showed_date", & ((struct_closure_t){ &pylv_calendar_date_t_Type, offsetof(lv_calendar_ext_t, showed_date), sizeof(lv_calendar_date_t)})},
    {"highlighted_dates", (getter) struct_get_struct, (setter) struct_set_struct, "lv_calendar_date_t highlighted_dates", & ((struct_closure_t){ &Blob_Type, offsetof(lv_calendar_ext_t, highlighted_dates), sizeof(((lv_calendar_ext_t *)0)->highlighted_dates)})},
    {"btn_pressing", (getter) struct_get_int8, (setter) struct_set_int8, "int8_t btn_pressing", (void*)offsetof(lv_calendar_ext_t, btn_pressing)},
    {"highlighted_dates_num", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t highlighted_dates_num", (void*)offsetof(lv_calendar_ext_t, highlighted_dates_num)},
    {"pressed_date", (getter) struct_get_struct, (setter) struct_set_struct, "lv_calendar_date_t pressed_date", & ((struct_closure_t){ &pylv_calendar_date_t_Type, offsetof(lv_calendar_ext_t, pressed_date), sizeof(lv_calendar_date_t)})},
    {"day_names", (getter) struct_get_struct, (setter) struct_set_struct, "char day_names", & ((struct_closure_t){ &Blob_Type, offsetof(lv_calendar_ext_t, day_names), sizeof(((lv_calendar_ext_t *)0)->day_names)})},
    {"month_names", (getter) struct_get_struct, (setter) struct_set_struct, "char month_names", & ((struct_closure_t){ &Blob_Type, offsetof(lv_calendar_ext_t, month_names), sizeof(((lv_calendar_ext_t *)0)->month_names)})},
    {"style_header", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_header", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_calendar_ext_t, style_header), sizeof(lv_style_list_t)})},
    {"style_day_names", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_day_names", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_calendar_ext_t, style_day_names), sizeof(lv_style_list_t)})},
    {"style_date_nums", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_date_nums", & ((struct_closure_t){ &pylv_style_list_t_Type, offsetof(lv_calendar_ext_t, style_date_nums), sizeof(lv_style_list_t)})},
    {NULL}
};


static PyTypeObject pylv_calendar_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.calendar_ext_t",
    .tp_doc = "lvgl calendar_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_calendar_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_calendar_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_calendar_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_calendar_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_calendar_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_calendar_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_spinbox_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_spinbox_ext_t_Type, sizeof(lv_spinbox_ext_t));
}



static PyObject *
get_struct_bitfield_spinbox_ext_t_rollover(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_spinbox_ext_t*)(self->data))->rollover );
}

static int
set_struct_bitfield_spinbox_ext_t_rollover(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_spinbox_ext_t*)(self->data))->rollover = v;
    return 0;
}



static PyObject *
get_struct_bitfield_spinbox_ext_t_digit_count(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_spinbox_ext_t*)(self->data))->digit_count );
}

static int
set_struct_bitfield_spinbox_ext_t_digit_count(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 15)) return -1;
    ((lv_spinbox_ext_t*)(self->data))->digit_count = v;
    return 0;
}



static PyObject *
get_struct_bitfield_spinbox_ext_t_dec_point_pos(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_spinbox_ext_t*)(self->data))->dec_point_pos );
}

static int
set_struct_bitfield_spinbox_ext_t_dec_point_pos(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 15)) return -1;
    ((lv_spinbox_ext_t*)(self->data))->dec_point_pos = v;
    return 0;
}



static PyObject *
get_struct_bitfield_spinbox_ext_t_digit_padding_left(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_spinbox_ext_t*)(self->data))->digit_padding_left );
}

static int
set_struct_bitfield_spinbox_ext_t_digit_padding_left(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 15)) return -1;
    ((lv_spinbox_ext_t*)(self->data))->digit_padding_left = v;
    return 0;
}

static PyGetSetDef pylv_spinbox_ext_t_getset[] = {
    {"ta", (getter) struct_get_struct, (setter) struct_set_struct, "lv_textarea_ext_t ta", & ((struct_closure_t){ &pylv_textarea_ext_t_Type, offsetof(lv_spinbox_ext_t, ta), sizeof(lv_textarea_ext_t)})},
    {"value", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t value", (void*)offsetof(lv_spinbox_ext_t, value)},
    {"range_max", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t range_max", (void*)offsetof(lv_spinbox_ext_t, range_max)},
    {"range_min", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t range_min", (void*)offsetof(lv_spinbox_ext_t, range_min)},
    {"step", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t step", (void*)offsetof(lv_spinbox_ext_t, step)},
    {"rollover", (getter) get_struct_bitfield_spinbox_ext_t_rollover, (setter) set_struct_bitfield_spinbox_ext_t_rollover, "uint8_t:1 rollover", NULL},
    {"digit_count", (getter) get_struct_bitfield_spinbox_ext_t_digit_count, (setter) set_struct_bitfield_spinbox_ext_t_digit_count, "uint16_t:4 digit_count", NULL},
    {"dec_point_pos", (getter) get_struct_bitfield_spinbox_ext_t_dec_point_pos, (setter) set_struct_bitfield_spinbox_ext_t_dec_point_pos, "uint16_t:4 dec_point_pos", NULL},
    {"digit_padding_left", (getter) get_struct_bitfield_spinbox_ext_t_digit_padding_left, (setter) set_struct_bitfield_spinbox_ext_t_digit_padding_left, "uint16_t:4 digit_padding_left", NULL},
    {NULL}
};


static PyTypeObject pylv_spinbox_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.spinbox_ext_t",
    .tp_doc = "lvgl spinbox_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_spinbox_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_spinbox_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_spinbox_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_spinbox_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_spinbox_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_spinbox_ext_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_img_cache_entry_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_img_cache_entry_t_Type, sizeof(lv_img_cache_entry_t));
}

static PyGetSetDef pylv_img_cache_entry_t_getset[] = {
    {"dec_dsc", (getter) struct_get_struct, (setter) struct_set_struct, "lv_img_decoder_dsc_t dec_dsc", & ((struct_closure_t){ &pylv_img_decoder_dsc_t_Type, offsetof(lv_img_cache_entry_t, dec_dsc), sizeof(lv_img_decoder_dsc_t)})},
    {"life", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t life", (void*)offsetof(lv_img_cache_entry_t, life)},
    {NULL}
};


static PyTypeObject pylv_img_cache_entry_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.img_cache_entry_t",
    .tp_doc = "lvgl img_cache_entry_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_img_cache_entry_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_img_cache_entry_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_img_cache_entry_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_img_cache_entry_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_img_cache_entry_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_img_cache_entry_t **)target = (void *)((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}








static PyObject *
get_struct_bitfield_color1_t_ch_blue(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_color1_t*)(self->data))->ch.blue );
}

static int
set_struct_bitfield_color1_t_ch_blue(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_color1_t*)(self->data))->ch.blue = v;
    return 0;
}



static PyObject *
get_struct_bitfield_color1_t_ch_green(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_color1_t*)(self->data))->ch.green );
}

static int
set_struct_bitfield_color1_t_ch_green(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_color1_t*)(self->data))->ch.green = v;
    return 0;
}



static PyObject *
get_struct_bitfield_color1_t_ch_red(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_color1_t*)(self->data))->ch.red );
}

static int
set_struct_bitfield_color1_t_ch_red(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_color1_t*)(self->data))->ch.red = v;
    return 0;
}

static PyGetSetDef pylv_color1_t_ch_getset[] = {
    {"blue", (getter) get_struct_bitfield_color1_t_ch_blue, (setter) set_struct_bitfield_color1_t_ch_blue, "uint8_t:1 blue", NULL},
    {"green", (getter) get_struct_bitfield_color1_t_ch_green, (setter) set_struct_bitfield_color1_t_ch_green, "uint8_t:1 green", NULL},
    {"red", (getter) get_struct_bitfield_color1_t_ch_red, (setter) set_struct_bitfield_color1_t_ch_red, "uint8_t:1 red", NULL},
    {NULL}
};


static PyTypeObject pylv_color1_t_ch_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.color1_t_ch",
    .tp_doc = "lvgl color1_t_ch",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_color1_t_ch_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};





static PyObject *
get_struct_bitfield_color8_t_ch_blue(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_color8_t*)(self->data))->ch.blue );
}

static int
set_struct_bitfield_color8_t_ch_blue(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_color8_t*)(self->data))->ch.blue = v;
    return 0;
}



static PyObject *
get_struct_bitfield_color8_t_ch_green(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_color8_t*)(self->data))->ch.green );
}

static int
set_struct_bitfield_color8_t_ch_green(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 7)) return -1;
    ((lv_color8_t*)(self->data))->ch.green = v;
    return 0;
}



static PyObject *
get_struct_bitfield_color8_t_ch_red(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_color8_t*)(self->data))->ch.red );
}

static int
set_struct_bitfield_color8_t_ch_red(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 7)) return -1;
    ((lv_color8_t*)(self->data))->ch.red = v;
    return 0;
}

static PyGetSetDef pylv_color8_t_ch_getset[] = {
    {"blue", (getter) get_struct_bitfield_color8_t_ch_blue, (setter) set_struct_bitfield_color8_t_ch_blue, "uint8_t:2 blue", NULL},
    {"green", (getter) get_struct_bitfield_color8_t_ch_green, (setter) set_struct_bitfield_color8_t_ch_green, "uint8_t:3 green", NULL},
    {"red", (getter) get_struct_bitfield_color8_t_ch_red, (setter) set_struct_bitfield_color8_t_ch_red, "uint8_t:3 red", NULL},
    {NULL}
};


static PyTypeObject pylv_color8_t_ch_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.color8_t_ch",
    .tp_doc = "lvgl color8_t_ch",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_color8_t_ch_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};





static PyObject *
get_struct_bitfield_color16_t_ch_blue(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_color16_t*)(self->data))->ch.blue );
}

static int
set_struct_bitfield_color16_t_ch_blue(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 31)) return -1;
    ((lv_color16_t*)(self->data))->ch.blue = v;
    return 0;
}



static PyObject *
get_struct_bitfield_color16_t_ch_green(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_color16_t*)(self->data))->ch.green );
}

static int
set_struct_bitfield_color16_t_ch_green(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 63)) return -1;
    ((lv_color16_t*)(self->data))->ch.green = v;
    return 0;
}



static PyObject *
get_struct_bitfield_color16_t_ch_red(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_color16_t*)(self->data))->ch.red );
}

static int
set_struct_bitfield_color16_t_ch_red(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 31)) return -1;
    ((lv_color16_t*)(self->data))->ch.red = v;
    return 0;
}

static PyGetSetDef pylv_color16_t_ch_getset[] = {
    {"blue", (getter) get_struct_bitfield_color16_t_ch_blue, (setter) set_struct_bitfield_color16_t_ch_blue, "uint16_t:5 blue", NULL},
    {"green", (getter) get_struct_bitfield_color16_t_ch_green, (setter) set_struct_bitfield_color16_t_ch_green, "uint16_t:6 green", NULL},
    {"red", (getter) get_struct_bitfield_color16_t_ch_red, (setter) set_struct_bitfield_color16_t_ch_red, "uint16_t:5 red", NULL},
    {NULL}
};


static PyTypeObject pylv_color16_t_ch_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.color16_t_ch",
    .tp_doc = "lvgl color16_t_ch",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_color16_t_ch_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_color32_t_ch_getset[] = {
    {"blue", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t blue", (void*)(offsetof(lv_color32_t, ch.blue)-offsetof(lv_color32_t, ch))},
    {"green", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t green", (void*)(offsetof(lv_color32_t, ch.green)-offsetof(lv_color32_t, ch))},
    {"red", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t red", (void*)(offsetof(lv_color32_t, ch.red)-offsetof(lv_color32_t, ch))},
    {"alpha", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t alpha", (void*)(offsetof(lv_color32_t, ch.alpha)-offsetof(lv_color32_t, ch))},
    {NULL}
};


static PyTypeObject pylv_color32_t_ch_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.color32_t_ch",
    .tp_doc = "lvgl color32_t_ch",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_color32_t_ch_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_indev_proc_t_types_getset[] = {
    {"pointer", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_point_t act_point;   lv_point_t last_point;   lv_point_t vect;   lv_point_t drag_sum;   lv_point_t drag_throw_vect;   struct _lv_obj_t *act_obj;   struct _lv_obj_t *last_obj;   struct _lv_obj_t *last_pressed;   lv_gesture_dir_t gesture_dir;   lv_point_t gesture_sum;   uint8_t drag_limit_out : 1;   uint8_t drag_in_prog : 1;   lv_drag_dir_t drag_dir : 3;   uint8_t gesture_sent : 1; } pointer", & ((struct_closure_t){ &pylv_indev_proc_t_types_pointer_Type, (offsetof(lv_indev_proc_t, types.pointer)-offsetof(lv_indev_proc_t, types)), sizeof(((lv_indev_proc_t *)0)->types.pointer)})},
    {"keypad", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_indev_state_t last_state;   uint32_t last_key; } keypad", & ((struct_closure_t){ &pylv_indev_proc_t_types_keypad_Type, (offsetof(lv_indev_proc_t, types.keypad)-offsetof(lv_indev_proc_t, types)), sizeof(((lv_indev_proc_t *)0)->types.keypad)})},
    {NULL}
};


static PyTypeObject pylv_indev_proc_t_types_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.indev_proc_t_types",
    .tp_doc = "lvgl indev_proc_t_types",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_indev_proc_t_types_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};





static PyObject *
get_struct_bitfield_indev_proc_t_types_pointer_drag_limit_out(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_indev_proc_t*)(self->data))->types.pointer.drag_limit_out );
}

static int
set_struct_bitfield_indev_proc_t_types_pointer_drag_limit_out(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_indev_proc_t*)(self->data))->types.pointer.drag_limit_out = v;
    return 0;
}



static PyObject *
get_struct_bitfield_indev_proc_t_types_pointer_drag_in_prog(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_indev_proc_t*)(self->data))->types.pointer.drag_in_prog );
}

static int
set_struct_bitfield_indev_proc_t_types_pointer_drag_in_prog(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_indev_proc_t*)(self->data))->types.pointer.drag_in_prog = v;
    return 0;
}



static PyObject *
get_struct_bitfield_indev_proc_t_types_pointer_drag_dir(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_indev_proc_t*)(self->data))->types.pointer.drag_dir );
}

static int
set_struct_bitfield_indev_proc_t_types_pointer_drag_dir(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 7)) return -1;
    ((lv_indev_proc_t*)(self->data))->types.pointer.drag_dir = v;
    return 0;
}



static PyObject *
get_struct_bitfield_indev_proc_t_types_pointer_gesture_sent(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_indev_proc_t*)(self->data))->types.pointer.gesture_sent );
}

static int
set_struct_bitfield_indev_proc_t_types_pointer_gesture_sent(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_indev_proc_t*)(self->data))->types.pointer.gesture_sent = v;
    return 0;
}

static PyGetSetDef pylv_indev_proc_t_types_pointer_getset[] = {
    {"act_point", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t act_point", & ((struct_closure_t){ &pylv_point_t_Type, (offsetof(lv_indev_proc_t, types.pointer.act_point)-offsetof(lv_indev_proc_t, types.pointer)), sizeof(lv_point_t)})},
    {"last_point", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t last_point", & ((struct_closure_t){ &pylv_point_t_Type, (offsetof(lv_indev_proc_t, types.pointer.last_point)-offsetof(lv_indev_proc_t, types.pointer)), sizeof(lv_point_t)})},
    {"vect", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t vect", & ((struct_closure_t){ &pylv_point_t_Type, (offsetof(lv_indev_proc_t, types.pointer.vect)-offsetof(lv_indev_proc_t, types.pointer)), sizeof(lv_point_t)})},
    {"drag_sum", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t drag_sum", & ((struct_closure_t){ &pylv_point_t_Type, (offsetof(lv_indev_proc_t, types.pointer.drag_sum)-offsetof(lv_indev_proc_t, types.pointer)), sizeof(lv_point_t)})},
    {"drag_throw_vect", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t drag_throw_vect", & ((struct_closure_t){ &pylv_point_t_Type, (offsetof(lv_indev_proc_t, types.pointer.drag_throw_vect)-offsetof(lv_indev_proc_t, types.pointer)), sizeof(lv_point_t)})},
    {"act_obj", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_obj_t act_obj", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_indev_proc_t, types.pointer.act_obj)-offsetof(lv_indev_proc_t, types.pointer)), sizeof(((lv_indev_proc_t *)0)->types.pointer.act_obj)})},
    {"last_obj", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_obj_t last_obj", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_indev_proc_t, types.pointer.last_obj)-offsetof(lv_indev_proc_t, types.pointer)), sizeof(((lv_indev_proc_t *)0)->types.pointer.last_obj)})},
    {"last_pressed", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_obj_t last_pressed", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_indev_proc_t, types.pointer.last_pressed)-offsetof(lv_indev_proc_t, types.pointer)), sizeof(((lv_indev_proc_t *)0)->types.pointer.last_pressed)})},
    {"gesture_dir", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_gesture_dir_t gesture_dir", (void*)(offsetof(lv_indev_proc_t, types.pointer.gesture_dir)-offsetof(lv_indev_proc_t, types.pointer))},
    {"gesture_sum", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t gesture_sum", & ((struct_closure_t){ &pylv_point_t_Type, (offsetof(lv_indev_proc_t, types.pointer.gesture_sum)-offsetof(lv_indev_proc_t, types.pointer)), sizeof(lv_point_t)})},
    {"drag_limit_out", (getter) get_struct_bitfield_indev_proc_t_types_pointer_drag_limit_out, (setter) set_struct_bitfield_indev_proc_t_types_pointer_drag_limit_out, "uint8_t:1 drag_limit_out", NULL},
    {"drag_in_prog", (getter) get_struct_bitfield_indev_proc_t_types_pointer_drag_in_prog, (setter) set_struct_bitfield_indev_proc_t_types_pointer_drag_in_prog, "uint8_t:1 drag_in_prog", NULL},
    {"drag_dir", (getter) get_struct_bitfield_indev_proc_t_types_pointer_drag_dir, (setter) set_struct_bitfield_indev_proc_t_types_pointer_drag_dir, "lv_drag_dir_t:3 drag_dir", NULL},
    {"gesture_sent", (getter) get_struct_bitfield_indev_proc_t_types_pointer_gesture_sent, (setter) set_struct_bitfield_indev_proc_t_types_pointer_gesture_sent, "uint8_t:1 gesture_sent", NULL},
    {NULL}
};


static PyTypeObject pylv_indev_proc_t_types_pointer_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.indev_proc_t_types_pointer",
    .tp_doc = "lvgl indev_proc_t_types_pointer",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_indev_proc_t_types_pointer_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_indev_proc_t_types_keypad_getset[] = {
    {"last_state", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_indev_state_t last_state", (void*)(offsetof(lv_indev_proc_t, types.keypad.last_state)-offsetof(lv_indev_proc_t, types.keypad))},
    {"last_key", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t last_key", (void*)(offsetof(lv_indev_proc_t, types.keypad.last_key)-offsetof(lv_indev_proc_t, types.keypad))},
    {NULL}
};


static PyTypeObject pylv_indev_proc_t_types_keypad_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.indev_proc_t_types_keypad",
    .tp_doc = "lvgl indev_proc_t_types_keypad",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_indev_proc_t_types_keypad_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};





static PyObject *
get_struct_bitfield_draw_mask_line_param_t_cfg_side(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_draw_mask_line_param_t*)(self->data))->cfg.side );
}

static int
set_struct_bitfield_draw_mask_line_param_t_cfg_side(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_draw_mask_line_param_t*)(self->data))->cfg.side = v;
    return 0;
}

static PyGetSetDef pylv_draw_mask_line_param_t_cfg_getset[] = {
    {"p1", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t p1", & ((struct_closure_t){ &pylv_point_t_Type, (offsetof(lv_draw_mask_line_param_t, cfg.p1)-offsetof(lv_draw_mask_line_param_t, cfg)), sizeof(lv_point_t)})},
    {"p2", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t p2", & ((struct_closure_t){ &pylv_point_t_Type, (offsetof(lv_draw_mask_line_param_t, cfg.p2)-offsetof(lv_draw_mask_line_param_t, cfg)), sizeof(lv_point_t)})},
    {"side", (getter) get_struct_bitfield_draw_mask_line_param_t_cfg_side, (setter) set_struct_bitfield_draw_mask_line_param_t_cfg_side, "lv_draw_mask_line_side_t:2 side", NULL},
    {NULL}
};


static PyTypeObject pylv_draw_mask_line_param_t_cfg_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.draw_mask_line_param_t_cfg",
    .tp_doc = "lvgl draw_mask_line_param_t_cfg",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_draw_mask_line_param_t_cfg_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_draw_mask_angle_param_t_cfg_getset[] = {
    {"vertex_p", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t vertex_p", & ((struct_closure_t){ &pylv_point_t_Type, (offsetof(lv_draw_mask_angle_param_t, cfg.vertex_p)-offsetof(lv_draw_mask_angle_param_t, cfg)), sizeof(lv_point_t)})},
    {"start_angle", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t start_angle", (void*)(offsetof(lv_draw_mask_angle_param_t, cfg.start_angle)-offsetof(lv_draw_mask_angle_param_t, cfg))},
    {"end_angle", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t end_angle", (void*)(offsetof(lv_draw_mask_angle_param_t, cfg.end_angle)-offsetof(lv_draw_mask_angle_param_t, cfg))},
    {NULL}
};


static PyTypeObject pylv_draw_mask_angle_param_t_cfg_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.draw_mask_angle_param_t_cfg",
    .tp_doc = "lvgl draw_mask_angle_param_t_cfg",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_draw_mask_angle_param_t_cfg_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};





static PyObject *
get_struct_bitfield_draw_mask_radius_param_t_cfg_outer(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_draw_mask_radius_param_t*)(self->data))->cfg.outer );
}

static int
set_struct_bitfield_draw_mask_radius_param_t_cfg_outer(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_draw_mask_radius_param_t*)(self->data))->cfg.outer = v;
    return 0;
}

static PyGetSetDef pylv_draw_mask_radius_param_t_cfg_getset[] = {
    {"rect", (getter) struct_get_struct, (setter) struct_set_struct, "lv_area_t rect", & ((struct_closure_t){ &pylv_area_t_Type, (offsetof(lv_draw_mask_radius_param_t, cfg.rect)-offsetof(lv_draw_mask_radius_param_t, cfg)), sizeof(lv_area_t)})},
    {"radius", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t radius", (void*)(offsetof(lv_draw_mask_radius_param_t, cfg.radius)-offsetof(lv_draw_mask_radius_param_t, cfg))},
    {"outer", (getter) get_struct_bitfield_draw_mask_radius_param_t_cfg_outer, (setter) set_struct_bitfield_draw_mask_radius_param_t_cfg_outer, "uint8_t:1 outer", NULL},
    {NULL}
};


static PyTypeObject pylv_draw_mask_radius_param_t_cfg_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.draw_mask_radius_param_t_cfg",
    .tp_doc = "lvgl draw_mask_radius_param_t_cfg",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_draw_mask_radius_param_t_cfg_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_draw_mask_fade_param_t_cfg_getset[] = {
    {"coords", (getter) struct_get_struct, (setter) struct_set_struct, "lv_area_t coords", & ((struct_closure_t){ &pylv_area_t_Type, (offsetof(lv_draw_mask_fade_param_t, cfg.coords)-offsetof(lv_draw_mask_fade_param_t, cfg)), sizeof(lv_area_t)})},
    {"y_top", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t y_top", (void*)(offsetof(lv_draw_mask_fade_param_t, cfg.y_top)-offsetof(lv_draw_mask_fade_param_t, cfg))},
    {"y_bottom", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t y_bottom", (void*)(offsetof(lv_draw_mask_fade_param_t, cfg.y_bottom)-offsetof(lv_draw_mask_fade_param_t, cfg))},
    {"opa_top", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t opa_top", (void*)(offsetof(lv_draw_mask_fade_param_t, cfg.opa_top)-offsetof(lv_draw_mask_fade_param_t, cfg))},
    {"opa_bottom", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t opa_bottom", (void*)(offsetof(lv_draw_mask_fade_param_t, cfg.opa_bottom)-offsetof(lv_draw_mask_fade_param_t, cfg))},
    {NULL}
};


static PyTypeObject pylv_draw_mask_fade_param_t_cfg_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.draw_mask_fade_param_t_cfg",
    .tp_doc = "lvgl draw_mask_fade_param_t_cfg",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_draw_mask_fade_param_t_cfg_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_draw_mask_map_param_t_cfg_getset[] = {
    {"coords", (getter) struct_get_struct, (setter) struct_set_struct, "lv_area_t coords", & ((struct_closure_t){ &pylv_area_t_Type, (offsetof(lv_draw_mask_map_param_t, cfg.coords)-offsetof(lv_draw_mask_map_param_t, cfg)), sizeof(lv_area_t)})},
    {"map", (getter) struct_get_struct, (setter) struct_set_struct, "lv_opa_t map", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_draw_mask_map_param_t, cfg.map)-offsetof(lv_draw_mask_map_param_t, cfg)), sizeof(((lv_draw_mask_map_param_t *)0)->cfg.map)})},
    {NULL}
};


static PyTypeObject pylv_draw_mask_map_param_t_cfg_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.draw_mask_map_param_t_cfg",
    .tp_doc = "lvgl draw_mask_map_param_t_cfg",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_draw_mask_map_param_t_cfg_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_img_transform_dsc_t_cfg_getset[] = {
    {"src", (getter) struct_get_struct, (setter) struct_set_struct, "void src", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_img_transform_dsc_t, cfg.src)-offsetof(lv_img_transform_dsc_t, cfg)), sizeof(((lv_img_transform_dsc_t *)0)->cfg.src)})},
    {"src_w", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t src_w", (void*)(offsetof(lv_img_transform_dsc_t, cfg.src_w)-offsetof(lv_img_transform_dsc_t, cfg))},
    {"src_h", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t src_h", (void*)(offsetof(lv_img_transform_dsc_t, cfg.src_h)-offsetof(lv_img_transform_dsc_t, cfg))},
    {"pivot_x", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t pivot_x", (void*)(offsetof(lv_img_transform_dsc_t, cfg.pivot_x)-offsetof(lv_img_transform_dsc_t, cfg))},
    {"pivot_y", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t pivot_y", (void*)(offsetof(lv_img_transform_dsc_t, cfg.pivot_y)-offsetof(lv_img_transform_dsc_t, cfg))},
    {"angle", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t angle", (void*)(offsetof(lv_img_transform_dsc_t, cfg.angle)-offsetof(lv_img_transform_dsc_t, cfg))},
    {"zoom", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t zoom", (void*)(offsetof(lv_img_transform_dsc_t, cfg.zoom)-offsetof(lv_img_transform_dsc_t, cfg))},
    {"color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t color", & ((struct_closure_t){ &pylv_color16_t_Type, (offsetof(lv_img_transform_dsc_t, cfg.color)-offsetof(lv_img_transform_dsc_t, cfg)), sizeof(lv_color16_t)})},
    {"cf", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_img_cf_t cf", (void*)(offsetof(lv_img_transform_dsc_t, cfg.cf)-offsetof(lv_img_transform_dsc_t, cfg))},
    {"antialias", (getter) struct_get_struct, (setter) struct_set_struct, "bool antialias", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_img_transform_dsc_t, cfg.antialias)-offsetof(lv_img_transform_dsc_t, cfg)), sizeof(((lv_img_transform_dsc_t *)0)->cfg.antialias)})},
    {NULL}
};


static PyTypeObject pylv_img_transform_dsc_t_cfg_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.img_transform_dsc_t_cfg",
    .tp_doc = "lvgl img_transform_dsc_t_cfg",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_img_transform_dsc_t_cfg_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_img_transform_dsc_t_res_getset[] = {
    {"color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t color", & ((struct_closure_t){ &pylv_color16_t_Type, (offsetof(lv_img_transform_dsc_t, res.color)-offsetof(lv_img_transform_dsc_t, res)), sizeof(lv_color16_t)})},
    {"opa", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t opa", (void*)(offsetof(lv_img_transform_dsc_t, res.opa)-offsetof(lv_img_transform_dsc_t, res))},
    {NULL}
};


static PyTypeObject pylv_img_transform_dsc_t_res_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.img_transform_dsc_t_res",
    .tp_doc = "lvgl img_transform_dsc_t_res",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_img_transform_dsc_t_res_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};





static PyObject *
get_struct_bitfield_img_transform_dsc_t_tmp_chroma_keyed(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_img_transform_dsc_t*)(self->data))->tmp.chroma_keyed );
}

static int
set_struct_bitfield_img_transform_dsc_t_tmp_chroma_keyed(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_img_transform_dsc_t*)(self->data))->tmp.chroma_keyed = v;
    return 0;
}



static PyObject *
get_struct_bitfield_img_transform_dsc_t_tmp_has_alpha(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_img_transform_dsc_t*)(self->data))->tmp.has_alpha );
}

static int
set_struct_bitfield_img_transform_dsc_t_tmp_has_alpha(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_img_transform_dsc_t*)(self->data))->tmp.has_alpha = v;
    return 0;
}



static PyObject *
get_struct_bitfield_img_transform_dsc_t_tmp_native_color(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_img_transform_dsc_t*)(self->data))->tmp.native_color );
}

static int
set_struct_bitfield_img_transform_dsc_t_tmp_native_color(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_img_transform_dsc_t*)(self->data))->tmp.native_color = v;
    return 0;
}

static PyGetSetDef pylv_img_transform_dsc_t_tmp_getset[] = {
    {"img_dsc", (getter) struct_get_struct, (setter) struct_set_struct, "lv_img_dsc_t img_dsc", & ((struct_closure_t){ &pylv_img_dsc_t_Type, (offsetof(lv_img_transform_dsc_t, tmp.img_dsc)-offsetof(lv_img_transform_dsc_t, tmp)), sizeof(lv_img_dsc_t)})},
    {"pivot_x_256", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t pivot_x_256", (void*)(offsetof(lv_img_transform_dsc_t, tmp.pivot_x_256)-offsetof(lv_img_transform_dsc_t, tmp))},
    {"pivot_y_256", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t pivot_y_256", (void*)(offsetof(lv_img_transform_dsc_t, tmp.pivot_y_256)-offsetof(lv_img_transform_dsc_t, tmp))},
    {"sinma", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t sinma", (void*)(offsetof(lv_img_transform_dsc_t, tmp.sinma)-offsetof(lv_img_transform_dsc_t, tmp))},
    {"cosma", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t cosma", (void*)(offsetof(lv_img_transform_dsc_t, tmp.cosma)-offsetof(lv_img_transform_dsc_t, tmp))},
    {"chroma_keyed", (getter) get_struct_bitfield_img_transform_dsc_t_tmp_chroma_keyed, (setter) set_struct_bitfield_img_transform_dsc_t_tmp_chroma_keyed, "uint8_t:1 chroma_keyed", NULL},
    {"has_alpha", (getter) get_struct_bitfield_img_transform_dsc_t_tmp_has_alpha, (setter) set_struct_bitfield_img_transform_dsc_t_tmp_has_alpha, "uint8_t:1 has_alpha", NULL},
    {"native_color", (getter) get_struct_bitfield_img_transform_dsc_t_tmp_native_color, (setter) set_struct_bitfield_img_transform_dsc_t_tmp_native_color, "uint8_t:1 native_color", NULL},
    {"zoom_inv", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t zoom_inv", (void*)(offsetof(lv_img_transform_dsc_t, tmp.zoom_inv)-offsetof(lv_img_transform_dsc_t, tmp))},
    {"xs", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t xs", (void*)(offsetof(lv_img_transform_dsc_t, tmp.xs)-offsetof(lv_img_transform_dsc_t, tmp))},
    {"ys", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t ys", (void*)(offsetof(lv_img_transform_dsc_t, tmp.ys)-offsetof(lv_img_transform_dsc_t, tmp))},
    {"xs_int", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t xs_int", (void*)(offsetof(lv_img_transform_dsc_t, tmp.xs_int)-offsetof(lv_img_transform_dsc_t, tmp))},
    {"ys_int", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t ys_int", (void*)(offsetof(lv_img_transform_dsc_t, tmp.ys_int)-offsetof(lv_img_transform_dsc_t, tmp))},
    {"pxi", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t pxi", (void*)(offsetof(lv_img_transform_dsc_t, tmp.pxi)-offsetof(lv_img_transform_dsc_t, tmp))},
    {"px_size", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t px_size", (void*)(offsetof(lv_img_transform_dsc_t, tmp.px_size)-offsetof(lv_img_transform_dsc_t, tmp))},
    {NULL}
};


static PyTypeObject pylv_img_transform_dsc_t_tmp_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.img_transform_dsc_t_tmp",
    .tp_doc = "lvgl img_transform_dsc_t_tmp",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_img_transform_dsc_t_tmp_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_label_ext_t_dot_getset[] = {
    {"tmp_ptr", (getter) struct_get_struct, (setter) struct_set_struct, "char tmp_ptr", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_label_ext_t, dot.tmp_ptr)-offsetof(lv_label_ext_t, dot)), sizeof(((lv_label_ext_t *)0)->dot.tmp_ptr)})},
    {"tmp", (getter) struct_get_struct, (setter) struct_set_struct, "char3 + 1 tmp", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_label_ext_t, dot.tmp)-offsetof(lv_label_ext_t, dot)), sizeof(((lv_label_ext_t *)0)->dot.tmp)})},
    {NULL}
};


static PyTypeObject pylv_label_ext_t_dot_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.label_ext_t_dot",
    .tp_doc = "lvgl label_ext_t_dot",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_label_ext_t_dot_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};





static PyObject *
get_struct_bitfield_page_ext_t_scrlbar_hor_draw(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_page_ext_t*)(self->data))->scrlbar.hor_draw );
}

static int
set_struct_bitfield_page_ext_t_scrlbar_hor_draw(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_page_ext_t*)(self->data))->scrlbar.hor_draw = v;
    return 0;
}



static PyObject *
get_struct_bitfield_page_ext_t_scrlbar_ver_draw(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_page_ext_t*)(self->data))->scrlbar.ver_draw );
}

static int
set_struct_bitfield_page_ext_t_scrlbar_ver_draw(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_page_ext_t*)(self->data))->scrlbar.ver_draw = v;
    return 0;
}



static PyObject *
get_struct_bitfield_page_ext_t_scrlbar_mode(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_page_ext_t*)(self->data))->scrlbar.mode );
}

static int
set_struct_bitfield_page_ext_t_scrlbar_mode(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 7)) return -1;
    ((lv_page_ext_t*)(self->data))->scrlbar.mode = v;
    return 0;
}

static PyGetSetDef pylv_page_ext_t_scrlbar_getset[] = {
    {"style", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style", & ((struct_closure_t){ &pylv_style_list_t_Type, (offsetof(lv_page_ext_t, scrlbar.style)-offsetof(lv_page_ext_t, scrlbar)), sizeof(lv_style_list_t)})},
    {"hor_area", (getter) struct_get_struct, (setter) struct_set_struct, "lv_area_t hor_area", & ((struct_closure_t){ &pylv_area_t_Type, (offsetof(lv_page_ext_t, scrlbar.hor_area)-offsetof(lv_page_ext_t, scrlbar)), sizeof(lv_area_t)})},
    {"ver_area", (getter) struct_get_struct, (setter) struct_set_struct, "lv_area_t ver_area", & ((struct_closure_t){ &pylv_area_t_Type, (offsetof(lv_page_ext_t, scrlbar.ver_area)-offsetof(lv_page_ext_t, scrlbar)), sizeof(lv_area_t)})},
    {"hor_draw", (getter) get_struct_bitfield_page_ext_t_scrlbar_hor_draw, (setter) set_struct_bitfield_page_ext_t_scrlbar_hor_draw, "uint8_t:1 hor_draw", NULL},
    {"ver_draw", (getter) get_struct_bitfield_page_ext_t_scrlbar_ver_draw, (setter) set_struct_bitfield_page_ext_t_scrlbar_ver_draw, "uint8_t:1 ver_draw", NULL},
    {"mode", (getter) get_struct_bitfield_page_ext_t_scrlbar_mode, (setter) set_struct_bitfield_page_ext_t_scrlbar_mode, "lv_scrollbar_mode_t:3 mode", NULL},
    {NULL}
};


static PyTypeObject pylv_page_ext_t_scrlbar_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.page_ext_t_scrlbar",
    .tp_doc = "lvgl page_ext_t_scrlbar",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_page_ext_t_scrlbar_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};





static PyObject *
get_struct_bitfield_page_ext_t_edge_flash_enabled(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_page_ext_t*)(self->data))->edge_flash.enabled );
}

static int
set_struct_bitfield_page_ext_t_edge_flash_enabled(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_page_ext_t*)(self->data))->edge_flash.enabled = v;
    return 0;
}



static PyObject *
get_struct_bitfield_page_ext_t_edge_flash_top_ip(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_page_ext_t*)(self->data))->edge_flash.top_ip );
}

static int
set_struct_bitfield_page_ext_t_edge_flash_top_ip(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_page_ext_t*)(self->data))->edge_flash.top_ip = v;
    return 0;
}



static PyObject *
get_struct_bitfield_page_ext_t_edge_flash_bottom_ip(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_page_ext_t*)(self->data))->edge_flash.bottom_ip );
}

static int
set_struct_bitfield_page_ext_t_edge_flash_bottom_ip(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_page_ext_t*)(self->data))->edge_flash.bottom_ip = v;
    return 0;
}



static PyObject *
get_struct_bitfield_page_ext_t_edge_flash_right_ip(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_page_ext_t*)(self->data))->edge_flash.right_ip );
}

static int
set_struct_bitfield_page_ext_t_edge_flash_right_ip(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_page_ext_t*)(self->data))->edge_flash.right_ip = v;
    return 0;
}



static PyObject *
get_struct_bitfield_page_ext_t_edge_flash_left_ip(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_page_ext_t*)(self->data))->edge_flash.left_ip );
}

static int
set_struct_bitfield_page_ext_t_edge_flash_left_ip(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_page_ext_t*)(self->data))->edge_flash.left_ip = v;
    return 0;
}

static PyGetSetDef pylv_page_ext_t_edge_flash_getset[] = {
    {"state", (getter) struct_get_int16, (setter) struct_set_int16, "lv_anim_value_t state", (void*)(offsetof(lv_page_ext_t, edge_flash.state)-offsetof(lv_page_ext_t, edge_flash))},
    {"style", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style", & ((struct_closure_t){ &pylv_style_list_t_Type, (offsetof(lv_page_ext_t, edge_flash.style)-offsetof(lv_page_ext_t, edge_flash)), sizeof(lv_style_list_t)})},
    {"enabled", (getter) get_struct_bitfield_page_ext_t_edge_flash_enabled, (setter) set_struct_bitfield_page_ext_t_edge_flash_enabled, "uint8_t:1 enabled", NULL},
    {"top_ip", (getter) get_struct_bitfield_page_ext_t_edge_flash_top_ip, (setter) set_struct_bitfield_page_ext_t_edge_flash_top_ip, "uint8_t:1 top_ip", NULL},
    {"bottom_ip", (getter) get_struct_bitfield_page_ext_t_edge_flash_bottom_ip, (setter) set_struct_bitfield_page_ext_t_edge_flash_bottom_ip, "uint8_t:1 bottom_ip", NULL},
    {"right_ip", (getter) get_struct_bitfield_page_ext_t_edge_flash_right_ip, (setter) set_struct_bitfield_page_ext_t_edge_flash_right_ip, "uint8_t:1 right_ip", NULL},
    {"left_ip", (getter) get_struct_bitfield_page_ext_t_edge_flash_left_ip, (setter) set_struct_bitfield_page_ext_t_edge_flash_left_ip, "uint8_t:1 left_ip", NULL},
    {NULL}
};


static PyTypeObject pylv_page_ext_t_edge_flash_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.page_ext_t_edge_flash",
    .tp_doc = "lvgl page_ext_t_edge_flash",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_page_ext_t_edge_flash_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};





static PyObject *
get_struct_bitfield_table_cell_format_t_s_align(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_table_cell_format_t*)(self->data))->s.align );
}

static int
set_struct_bitfield_table_cell_format_t_s_align(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_table_cell_format_t*)(self->data))->s.align = v;
    return 0;
}



static PyObject *
get_struct_bitfield_table_cell_format_t_s_right_merge(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_table_cell_format_t*)(self->data))->s.right_merge );
}

static int
set_struct_bitfield_table_cell_format_t_s_right_merge(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_table_cell_format_t*)(self->data))->s.right_merge = v;
    return 0;
}



static PyObject *
get_struct_bitfield_table_cell_format_t_s_type(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_table_cell_format_t*)(self->data))->s.type );
}

static int
set_struct_bitfield_table_cell_format_t_s_type(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 15)) return -1;
    ((lv_table_cell_format_t*)(self->data))->s.type = v;
    return 0;
}



static PyObject *
get_struct_bitfield_table_cell_format_t_s_crop(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_table_cell_format_t*)(self->data))->s.crop );
}

static int
set_struct_bitfield_table_cell_format_t_s_crop(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_table_cell_format_t*)(self->data))->s.crop = v;
    return 0;
}

static PyGetSetDef pylv_table_cell_format_t_s_getset[] = {
    {"align", (getter) get_struct_bitfield_table_cell_format_t_s_align, (setter) set_struct_bitfield_table_cell_format_t_s_align, "uint8_t:2 align", NULL},
    {"right_merge", (getter) get_struct_bitfield_table_cell_format_t_s_right_merge, (setter) set_struct_bitfield_table_cell_format_t_s_right_merge, "uint8_t:1 right_merge", NULL},
    {"type", (getter) get_struct_bitfield_table_cell_format_t_s_type, (setter) set_struct_bitfield_table_cell_format_t_s_type, "uint8_t:4 type", NULL},
    {"crop", (getter) get_struct_bitfield_table_cell_format_t_s_crop, (setter) set_struct_bitfield_table_cell_format_t_s_crop, "uint8_t:1 crop", NULL},
    {NULL}
};


static PyTypeObject pylv_table_cell_format_t_s_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.table_cell_format_t_s",
    .tp_doc = "lvgl table_cell_format_t_s",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_table_cell_format_t_s_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};





static PyObject *
get_struct_bitfield_cpicker_ext_t_knob_colored(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_cpicker_ext_t*)(self->data))->knob.colored );
}

static int
set_struct_bitfield_cpicker_ext_t_knob_colored(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_cpicker_ext_t*)(self->data))->knob.colored = v;
    return 0;
}

static PyGetSetDef pylv_cpicker_ext_t_knob_getset[] = {
    {"style_list", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style_list", & ((struct_closure_t){ &pylv_style_list_t_Type, (offsetof(lv_cpicker_ext_t, knob.style_list)-offsetof(lv_cpicker_ext_t, knob)), sizeof(lv_style_list_t)})},
    {"pos", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t pos", & ((struct_closure_t){ &pylv_point_t_Type, (offsetof(lv_cpicker_ext_t, knob.pos)-offsetof(lv_cpicker_ext_t, knob)), sizeof(lv_point_t)})},
    {"colored", (getter) get_struct_bitfield_cpicker_ext_t_knob_colored, (setter) set_struct_bitfield_cpicker_ext_t_knob_colored, "uint8_t:1 colored", NULL},
    {NULL}
};


static PyTypeObject pylv_cpicker_ext_t_knob_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.cpicker_ext_t_knob",
    .tp_doc = "lvgl cpicker_ext_t_knob",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_cpicker_ext_t_knob_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};





static PyObject *
get_struct_bitfield_textarea_ext_t_cursor_state(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_textarea_ext_t*)(self->data))->cursor.state );
}

static int
set_struct_bitfield_textarea_ext_t_cursor_state(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_textarea_ext_t*)(self->data))->cursor.state = v;
    return 0;
}



static PyObject *
get_struct_bitfield_textarea_ext_t_cursor_hidden(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_textarea_ext_t*)(self->data))->cursor.hidden );
}

static int
set_struct_bitfield_textarea_ext_t_cursor_hidden(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_textarea_ext_t*)(self->data))->cursor.hidden = v;
    return 0;
}



static PyObject *
get_struct_bitfield_textarea_ext_t_cursor_click_pos(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_textarea_ext_t*)(self->data))->cursor.click_pos );
}

static int
set_struct_bitfield_textarea_ext_t_cursor_click_pos(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_textarea_ext_t*)(self->data))->cursor.click_pos = v;
    return 0;
}

static PyGetSetDef pylv_textarea_ext_t_cursor_getset[] = {
    {"style", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_list_t style", & ((struct_closure_t){ &pylv_style_list_t_Type, (offsetof(lv_textarea_ext_t, cursor.style)-offsetof(lv_textarea_ext_t, cursor)), sizeof(lv_style_list_t)})},
    {"valid_x", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t valid_x", (void*)(offsetof(lv_textarea_ext_t, cursor.valid_x)-offsetof(lv_textarea_ext_t, cursor))},
    {"pos", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t pos", (void*)(offsetof(lv_textarea_ext_t, cursor.pos)-offsetof(lv_textarea_ext_t, cursor))},
    {"blink_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t blink_time", (void*)(offsetof(lv_textarea_ext_t, cursor.blink_time)-offsetof(lv_textarea_ext_t, cursor))},
    {"area", (getter) struct_get_struct, (setter) struct_set_struct, "lv_area_t area", & ((struct_closure_t){ &pylv_area_t_Type, (offsetof(lv_textarea_ext_t, cursor.area)-offsetof(lv_textarea_ext_t, cursor)), sizeof(lv_area_t)})},
    {"txt_byte_pos", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t txt_byte_pos", (void*)(offsetof(lv_textarea_ext_t, cursor.txt_byte_pos)-offsetof(lv_textarea_ext_t, cursor))},
    {"state", (getter) get_struct_bitfield_textarea_ext_t_cursor_state, (setter) set_struct_bitfield_textarea_ext_t_cursor_state, "uint8_t:1 state", NULL},
    {"hidden", (getter) get_struct_bitfield_textarea_ext_t_cursor_hidden, (setter) set_struct_bitfield_textarea_ext_t_cursor_hidden, "uint8_t:1 hidden", NULL},
    {"click_pos", (getter) get_struct_bitfield_textarea_ext_t_cursor_click_pos, (setter) set_struct_bitfield_textarea_ext_t_cursor_click_pos, "uint8_t:1 click_pos", NULL},
    {NULL}
};


static PyTypeObject pylv_textarea_ext_t_cursor_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.textarea_ext_t_cursor",
    .tp_doc = "lvgl textarea_ext_t_cursor",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_textarea_ext_t_cursor_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



/****************************************************************
 * Custom types: constclass                                     *  
 ****************************************************************/

static PyType_Slot constclass_slots[] = {
    {0, 0},
};

/* Create a new class which represents a set of constants
 * Used for C enum constants, symbols and colors
 *
 * variadic arguments are char* name, <type> value, ... , NULL
 * representing the enum values
 *
 * dtype: 'd' for integers, 's' for strings, 'C' for colors (lv_color_t)
 *
 */
static PyObject* build_constclass(char dtype, char *name, ...) {

    va_list args;
    va_start(args, name);

    PyType_Spec spec = {
        .name = name,
        .basicsize = sizeof(PyObject),
        .itemsize = 0,
        .flags = Py_TPFLAGS_DEFAULT,
        .slots = constclass_slots /* terminated by slot==0. */
    };
    
    PyObject *constclass_type = PyType_FromSpec(&spec);
    if (!constclass_type) return NULL;
    
    ((PyTypeObject*)constclass_type)->tp_new = NULL; // objects cannot be instantiated
    
    while(1) {
        char *name = va_arg(args, char*);
        if (!name) break;
        
        PyObject *value=NULL;
        lv_color_t color;
        
        switch(dtype) {
            case 'd':
                value = PyLong_FromLong(va_arg(args, int));
                break;
            case 's':
                value = PyUnicode_FromString(va_arg(args, char *));
                break;
            case 'C':
                color = va_arg(args, lv_color_t);
                value = pystruct_from_c(&pylv_color16_t_Type, &color, sizeof(lv_color_t), 1);
                break;
            default:
                assert(0);
        }
        
        if (!value) goto error;
        
        PyObject_SetAttrString(constclass_type, name, value);
        Py_DECREF(value);
    }

    return constclass_type;

error:
    Py_DECREF(constclass_type);
    return NULL;

}


/****************************************************************
 * Custom method implementations                                *
 ****************************************************************/
 
/*
 * lv_obj_get_child and lv_obj_get_child_back are not really Pythonic. This
 * implementation returns a list of children
 */
 
static PyObject*
pylv_obj_get_children(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    lv_obj_t *child = NULL;
    PyObject *pychild;
    PyObject *ret = PyList_New(0);
    if (!ret) return NULL;
    
    LVGL_LOCK
    
    while (1) {
        child = lv_obj_get_child(self->ref, child);
        if (!child) break;
        pychild = pyobj_from_lv(child);
        
        if (PyList_Append(ret, pychild)) { // PyList_Append increases refcount
            Py_DECREF(ret);
            Py_DECREF(pychild);
            ret = NULL;
            break;
        }
        Py_DECREF(pychild);
    }

    LVGL_UNLOCK
    
    return ret;
}

static PyObject*
pylv_obj_get_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    lv_obj_type_t result;
    PyObject *list = NULL;
    PyObject *str = NULL;
    
    LVGL_LOCK
    lv_obj_get_type(self->ref, &result);
    LVGL_UNLOCK
    
    list = PyList_New(0);
    
    for(int i=0; i<LV_MAX_ANCESTOR_NUM;i++) {
        if (!result.type[i]) break;

        str = PyUnicode_FromString(result.type[i]);
        if (!str) goto error;
        
        if (PyList_Append(list, str)) goto error; // PyList_Append increases refcount
    }
  
    return list;
    
error:
    Py_XDECREF(list);
    Py_XDECREF(str);  
    return NULL;
}

void pylv_event_cb(lv_obj_t *obj, lv_event_t event) {
    pylv_Obj *self = (pylv_Obj *)obj->user_data;
    assert(self && self->event_cb);
    
    PyObject *result = PyObject_CallFunction(self->event_cb, "I", event);
    
    if (result) {
        Py_DECREF(result);
    } else {
        PyErr_Print();
        PyErr_Clear();
    }
    
}

static PyObject *
pylv_obj_set_event_cb(pylv_Obj *self, PyObject *args, PyObject *kwds) {
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"event_cb", NULL};
    PyObject *callback, *old_callback;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &callback)) return NULL;
    
    old_callback = self->event_cb;
    self->event_cb = callback;
    Py_INCREF(callback);
    Py_XDECREF(old_callback);
    
    LVGL_LOCK
    lv_obj_set_event_cb(self->ref, pylv_event_cb);
    LVGL_UNLOCK
    
    
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_get_letter_pos(pylv_Label *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"index", NULL};
    int index;
    lv_point_t pos;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &index)) return NULL;   
    
    LVGL_LOCK
    lv_label_get_letter_pos(self->ref, index, &pos);
    LVGL_UNLOCK

    return Py_BuildValue("ii", (int) pos.x, (int) pos.y);
}

static PyObject*
pylv_label_get_letter_on(pylv_Label *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"pos", NULL};
    int x, y, index;
    lv_point_t pos;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "(ii)", kwlist , &x, &y)) return NULL;   
    
    pos.x = x;
    pos.y = y;
    
    LVGL_LOCK
    index = lv_label_get_letter_on(self->ref, &pos);
    LVGL_UNLOCK

    return Py_BuildValue("i", index);
}


/*

static PyObject*
pylv_list_add(pylv_List *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"img_src", "txt", "rel_action", NULL};
    PyObject *img_src;
    const char *txt;
    PyObject *rel_action;
    PyObject *ret;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OsO", kwlist , &img_src, &txt, &rel_action)) return NULL; 
      
    if ( img_src!=Py_None || rel_action!=Py_None) {
        PyErr_SetString(PyExc_ValueError, "only img_src == None and rel_action == None is currently supported");
        return NULL;
    } 

    LVGL_LOCK
    ret = pyobj_from_lv(lv_list_add(self->ref, NULL, txt, NULL));
    LVGL_UNLOCK
    
    return ret;

}
*/

// lv_list_focus takes lv_obj_t* as first argument, but it is not the list itself!
static PyObject*
pylv_list_focus(pylv_List *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"obj", "anim_en", NULL};
    pylv_Btn * obj;
    lv_obj_t *parent;
    int anim_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!p", kwlist , &pylv_btn_Type, &obj, &anim_en)) return NULL;
    
    LVGL_LOCK         
    
    parent = lv_obj_get_parent(obj->ref);
    if (parent) parent = lv_obj_get_parent(parent); // get the obj's parent's parent in a safe way
    
    if (parent != self->ref) {
        if (unlock) unlock(unlock_arg);
        return PyErr_Format(PyExc_RuntimeError, "%R is not a child of %R", obj, self);
    }
    
    lv_list_focus(obj->ref, anim_en);
    
    LVGL_UNLOCK
    Py_RETURN_NONE;
}



/****************************************************************
 * Methods and object definitions                               *
 ****************************************************************/


    
static void
pylv_obj_dealloc(pylv_Obj *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_obj_init(pylv_Obj *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Obj *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_obj_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_obj_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_obj_del(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_res_t result = lv_obj_del(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_clean(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_obj_clean(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_invalidate_area(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_invalidate_area: Parameter type not found >const lv_area_t*< ");
    return NULL;
}

static PyObject*
pylv_obj_invalidate(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_obj_invalidate(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_area_is_visible(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_area_is_visible: Parameter type not found >lv_area_t*< ");
    return NULL;
}

static PyObject*
pylv_obj_is_visible(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_obj_is_visible(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_set_parent(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"parent", NULL};
    pylv_Obj * parent;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &pylv_obj_Type, &parent)) return NULL;

    LVGL_LOCK         
    lv_obj_set_parent(self->ref, parent->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_move_foreground(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_obj_move_foreground(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_move_background(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_obj_move_background(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_pos(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"x", "y", NULL};
    short int x;
    short int y;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &x, &y)) return NULL;

    LVGL_LOCK         
    lv_obj_set_pos(self->ref, x, y);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_x(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"x", NULL};
    short int x;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &x)) return NULL;

    LVGL_LOCK         
    lv_obj_set_x(self->ref, x);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_y(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"y", NULL};
    short int y;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &y)) return NULL;

    LVGL_LOCK         
    lv_obj_set_y(self->ref, y);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"w", "h", NULL};
    short int w;
    short int h;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &w, &h)) return NULL;

    LVGL_LOCK         
    lv_obj_set_size(self->ref, w, h);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"w", NULL};
    short int w;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &w)) return NULL;

    LVGL_LOCK         
    lv_obj_set_width(self->ref, w);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"h", NULL};
    short int h;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &h)) return NULL;

    LVGL_LOCK         
    lv_obj_set_height(self->ref, h);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_width_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"w", NULL};
    short int w;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &w)) return NULL;

    LVGL_LOCK         
    lv_obj_set_width_fit(self->ref, w);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_height_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"h", NULL};
    short int h;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &h)) return NULL;

    LVGL_LOCK         
    lv_obj_set_height_fit(self->ref, h);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_width_margin(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"w", NULL};
    short int w;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &w)) return NULL;

    LVGL_LOCK         
    lv_obj_set_width_margin(self->ref, w);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_height_margin(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"h", NULL};
    short int h;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &h)) return NULL;

    LVGL_LOCK         
    lv_obj_set_height_margin(self->ref, h);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"base", "align", "x_ofs", "y_ofs", NULL};
    pylv_Obj * base;
    unsigned char align;
    short int x_ofs;
    short int y_ofs;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!bhh", kwlist , &pylv_obj_Type, &base, &align, &x_ofs, &y_ofs)) return NULL;

    LVGL_LOCK         
    lv_obj_align(self->ref, base->ref, align, x_ofs, y_ofs);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_align_x(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"base", "align", "x_ofs", NULL};
    pylv_Obj * base;
    unsigned char align;
    short int x_ofs;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!bh", kwlist , &pylv_obj_Type, &base, &align, &x_ofs)) return NULL;

    LVGL_LOCK         
    lv_obj_align_x(self->ref, base->ref, align, x_ofs);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_align_y(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"base", "align", "y_ofs", NULL};
    pylv_Obj * base;
    unsigned char align;
    short int y_ofs;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!bh", kwlist , &pylv_obj_Type, &base, &align, &y_ofs)) return NULL;

    LVGL_LOCK         
    lv_obj_align_y(self->ref, base->ref, align, y_ofs);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_align_mid(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"base", "align", "x_ofs", "y_ofs", NULL};
    pylv_Obj * base;
    unsigned char align;
    short int x_ofs;
    short int y_ofs;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!bhh", kwlist , &pylv_obj_Type, &base, &align, &x_ofs, &y_ofs)) return NULL;

    LVGL_LOCK         
    lv_obj_align_mid(self->ref, base->ref, align, x_ofs, y_ofs);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_align_mid_x(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"base", "align", "x_ofs", NULL};
    pylv_Obj * base;
    unsigned char align;
    short int x_ofs;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!bh", kwlist , &pylv_obj_Type, &base, &align, &x_ofs)) return NULL;

    LVGL_LOCK         
    lv_obj_align_mid_x(self->ref, base->ref, align, x_ofs);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_align_mid_y(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"base", "align", "y_ofs", NULL};
    pylv_Obj * base;
    unsigned char align;
    short int y_ofs;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!bh", kwlist , &pylv_obj_Type, &base, &align, &y_ofs)) return NULL;

    LVGL_LOCK         
    lv_obj_align_mid_y(self->ref, base->ref, align, y_ofs);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_realign(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_obj_realign(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_auto_realign(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_obj_set_auto_realign(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_ext_click_area(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"left", "right", "top", "bottom", NULL};
    short int left;
    short int right;
    short int top;
    short int bottom;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hhhh", kwlist , &left, &right, &top, &bottom)) return NULL;

    LVGL_LOCK         
    lv_obj_set_ext_click_area(self->ref, left, right, top, bottom);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_add_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "style", NULL};
    unsigned char part;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &part, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_obj_add_style(self->ref, part, style);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_remove_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "style", NULL};
    unsigned char part;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &part, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_obj_remove_style(self->ref, part, style);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_clean_style_list(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK         
    lv_obj_clean_style_list(self->ref, part);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_reset_style_list(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK         
    lv_obj_reset_style_list(self->ref, part);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_refresh_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "prop", NULL};
    unsigned char part;
    unsigned short int prop;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bH", kwlist , &part, &prop)) return NULL;

    LVGL_LOCK         
    lv_obj_refresh_style(self->ref, part, prop);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_remove_style_local_prop(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "prop", NULL};
    unsigned char part;
    unsigned short int prop;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bH", kwlist , &part, &prop)) return NULL;

    LVGL_LOCK        
    bool result = lv_obj_remove_style_local_prop(self->ref, part, prop);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_set_hidden(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_obj_set_hidden(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_adv_hittest(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_obj_set_adv_hittest(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_click(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_obj_set_click(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_top(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_obj_set_top(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_drag(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_obj_set_drag(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_drag_dir(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"drag_dir", NULL};
    unsigned char drag_dir;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &drag_dir)) return NULL;

    LVGL_LOCK         
    lv_obj_set_drag_dir(self->ref, drag_dir);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_drag_throw(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_obj_set_drag_throw(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_drag_parent(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_obj_set_drag_parent(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_focus_parent(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_obj_set_focus_parent(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_gesture_parent(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_obj_set_gesture_parent(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_parent_event(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_obj_set_parent_event(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_base_dir(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"dir", NULL};
    unsigned char dir;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &dir)) return NULL;

    LVGL_LOCK         
    lv_obj_set_base_dir(self->ref, dir);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_add_protect(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"prot", NULL};
    unsigned char prot;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &prot)) return NULL;

    LVGL_LOCK         
    lv_obj_add_protect(self->ref, prot);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_clear_protect(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"prot", NULL};
    unsigned char prot;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &prot)) return NULL;

    LVGL_LOCK         
    lv_obj_clear_protect(self->ref, prot);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_state(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"state", NULL};
    unsigned char state;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &state)) return NULL;

    LVGL_LOCK         
    lv_obj_set_state(self->ref, state);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_add_state(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"state", NULL};
    unsigned char state;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &state)) return NULL;

    LVGL_LOCK         
    lv_obj_add_state(self->ref, state);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_clear_state(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"state", NULL};
    unsigned char state;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &state)) return NULL;

    LVGL_LOCK         
    lv_obj_clear_state(self->ref, state);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_finish_transitions(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK         
    lv_obj_finish_transitions(self->ref, part);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_signal_cb(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_signal_cb: Parameter type not found >lv_signal_cb_t< ");
    return NULL;
}

static PyObject*
pylv_obj_set_design_cb(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_design_cb: Parameter type not found >lv_design_cb_t< ");
    return NULL;
}

static PyObject*
pylv_obj_refresh_ext_draw_pad(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_obj_refresh_ext_draw_pad(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_screen(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_obj_get_screen(self->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}

static PyObject*
pylv_obj_get_disp(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_disp: Return type not found >lv_disp_t*< ");
    return NULL;
}

static PyObject*
pylv_obj_get_parent(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_obj_get_parent(self->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}

static PyObject*
pylv_obj_count_children(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_obj_count_children(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_obj_count_children_recursive(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_obj_count_children_recursive(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_obj_get_coords(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_coords: Parameter type not found >lv_area_t*< ");
    return NULL;
}

static PyObject*
pylv_obj_get_inner_coords(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_inner_coords: Parameter type not found >lv_area_t*< ");
    return NULL;
}

static PyObject*
pylv_obj_get_x(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_obj_get_x(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_y(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_obj_get_y(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_obj_get_width(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_obj_get_height(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_width_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_obj_get_width_fit(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_height_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_obj_get_height_fit(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_height_margin(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_obj_get_height_margin(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_width_margin(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_obj_get_width_margin(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_width_grid(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"div", "span", NULL};
    unsigned char div;
    unsigned char span;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bb", kwlist , &div, &span)) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_obj_get_width_grid(self->ref, div, span);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_height_grid(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"div", "span", NULL};
    unsigned char div;
    unsigned char span;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bb", kwlist , &div, &span)) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_obj_get_height_grid(self->ref, div, span);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_auto_realign(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_obj_get_auto_realign(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_ext_click_pad_left(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_obj_get_ext_click_pad_left(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_ext_click_pad_right(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_obj_get_ext_click_pad_right(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_ext_click_pad_top(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_obj_get_ext_click_pad_top(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_ext_click_pad_bottom(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_obj_get_ext_click_pad_bottom(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_ext_draw_pad(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_obj_get_ext_draw_pad(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_style_list(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style_list: Return type not found >lv_style_list_t*< ");
    return NULL;
}

static PyObject*
pylv_obj_get_local_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_t* result = lv_obj_get_local_style(self->ref, part);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}

static PyObject*
pylv_obj_get_style_radius(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_radius(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_radius(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_radius(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_clip_corner(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    bool result = lv_obj_get_style_clip_corner(self->ref, part);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_set_style_local_clip_corner(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbp", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_clip_corner(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_size(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_size(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_transform_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_transform_width(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_transform_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_transform_width(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_transform_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_transform_height(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_transform_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_transform_height(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_transform_angle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_transform_angle(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_transform_angle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_transform_angle(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_transform_zoom(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_transform_zoom(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_transform_zoom(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_transform_zoom(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_opa_scale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_opa_t result = lv_obj_get_style_opa_scale(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_opa_scale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_opa_scale(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_pad_top(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_pad_top(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_pad_top(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_pad_top(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_pad_bottom(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_pad_bottom(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_pad_bottom(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_pad_bottom(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_pad_left(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_pad_left(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_pad_left(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_pad_left(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_pad_right(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_pad_right(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_pad_right(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_pad_right(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_pad_inner(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_pad_inner(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_pad_inner(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_pad_inner(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_margin_top(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_margin_top(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_margin_top(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_margin_top(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_margin_bottom(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_margin_bottom(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_margin_bottom(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_margin_bottom(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_margin_left(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_margin_left(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_margin_left(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_margin_left(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_margin_right(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_margin_right(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_margin_right(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_margin_right(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_bg_blend_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_blend_mode_t result = lv_obj_get_style_bg_blend_mode(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_bg_blend_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_bg_blend_mode(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_bg_main_stop(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_bg_main_stop(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_bg_main_stop(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_bg_main_stop(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_bg_grad_stop(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_bg_grad_stop(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_bg_grad_stop(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_bg_grad_stop(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_bg_grad_dir(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_grad_dir_t result = lv_obj_get_style_bg_grad_dir(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_bg_grad_dir(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_bg_grad_dir(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_bg_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style_bg_color: Return type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_set_style_local_bg_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_style_local_bg_color: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_get_style_bg_grad_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style_bg_grad_color: Return type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_set_style_local_bg_grad_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_style_local_bg_grad_color: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_get_style_bg_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_opa_t result = lv_obj_get_style_bg_opa(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_bg_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_bg_opa(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_border_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_border_width(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_border_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_border_width(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_border_side(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_border_side_t result = lv_obj_get_style_border_side(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_border_side(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_border_side(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_border_blend_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_blend_mode_t result = lv_obj_get_style_border_blend_mode(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_border_blend_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_border_blend_mode(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_border_post(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    bool result = lv_obj_get_style_border_post(self->ref, part);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_set_style_local_border_post(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbp", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_border_post(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_border_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style_border_color: Return type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_set_style_local_border_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_style_local_border_color: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_get_style_border_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_opa_t result = lv_obj_get_style_border_opa(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_border_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_border_opa(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_outline_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_outline_width(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_outline_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_outline_width(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_outline_pad(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_outline_pad(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_outline_pad(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_outline_pad(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_outline_blend_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_blend_mode_t result = lv_obj_get_style_outline_blend_mode(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_outline_blend_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_outline_blend_mode(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_outline_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style_outline_color: Return type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_set_style_local_outline_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_style_local_outline_color: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_get_style_outline_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_opa_t result = lv_obj_get_style_outline_opa(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_outline_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_outline_opa(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_shadow_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_shadow_width(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_shadow_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_shadow_width(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_shadow_ofs_x(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_shadow_ofs_x(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_shadow_ofs_x(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_shadow_ofs_x(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_shadow_ofs_y(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_shadow_ofs_y(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_shadow_ofs_y(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_shadow_ofs_y(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_shadow_spread(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_shadow_spread(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_shadow_spread(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_shadow_spread(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_shadow_blend_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_blend_mode_t result = lv_obj_get_style_shadow_blend_mode(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_shadow_blend_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_shadow_blend_mode(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_shadow_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style_shadow_color: Return type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_set_style_local_shadow_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_style_local_shadow_color: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_get_style_shadow_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_opa_t result = lv_obj_get_style_shadow_opa(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_shadow_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_shadow_opa(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_pattern_repeat(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    bool result = lv_obj_get_style_pattern_repeat(self->ref, part);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_set_style_local_pattern_repeat(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbp", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_pattern_repeat(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_pattern_blend_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_blend_mode_t result = lv_obj_get_style_pattern_blend_mode(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_pattern_blend_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_pattern_blend_mode(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_pattern_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style_pattern_recolor: Return type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_set_style_local_pattern_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_style_local_pattern_recolor: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_get_style_pattern_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_opa_t result = lv_obj_get_style_pattern_opa(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_pattern_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_pattern_opa(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_pattern_recolor_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_opa_t result = lv_obj_get_style_pattern_recolor_opa(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_pattern_recolor_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_pattern_recolor_opa(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_pattern_image(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style_pattern_image: Return type not found >const void*< ");
    return NULL;
}

static PyObject*
pylv_obj_set_style_local_pattern_image(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_style_local_pattern_image: Parameter type not found >const void*< ");
    return NULL;
}

static PyObject*
pylv_obj_get_style_value_letter_space(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_value_letter_space(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_value_letter_space(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_value_letter_space(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_value_line_space(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_value_line_space(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_value_line_space(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_value_line_space(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_value_blend_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_blend_mode_t result = lv_obj_get_style_value_blend_mode(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_value_blend_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_value_blend_mode(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_value_ofs_x(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_value_ofs_x(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_value_ofs_x(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_value_ofs_x(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_value_ofs_y(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_value_ofs_y(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_value_ofs_y(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_value_ofs_y(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_value_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_align_t result = lv_obj_get_style_value_align(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_value_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_value_align(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_value_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style_value_color: Return type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_set_style_local_value_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_style_local_value_color: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_get_style_value_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_opa_t result = lv_obj_get_style_value_opa(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_value_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_value_opa(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_value_font(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style_value_font: Return type not found >const lv_font_t*< ");
    return NULL;
}

static PyObject*
pylv_obj_set_style_local_value_font(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_style_local_value_font: Parameter type not found >const lv_font_t*< ");
    return NULL;
}

static PyObject*
pylv_obj_get_style_value_str(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    const char* result = lv_obj_get_style_value_str(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_obj_set_style_local_value_str(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    const char * value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbs", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_value_str(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_text_letter_space(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_text_letter_space(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_text_letter_space(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_text_letter_space(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_text_line_space(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_text_line_space(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_text_line_space(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_text_line_space(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_text_decor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_text_decor_t result = lv_obj_get_style_text_decor(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_text_decor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_text_decor(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_text_blend_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_blend_mode_t result = lv_obj_get_style_text_blend_mode(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_text_blend_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_text_blend_mode(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_text_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style_text_color: Return type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_set_style_local_text_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_style_local_text_color: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_get_style_text_sel_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style_text_sel_color: Return type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_set_style_local_text_sel_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_style_local_text_sel_color: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_get_style_text_sel_bg_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style_text_sel_bg_color: Return type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_set_style_local_text_sel_bg_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_style_local_text_sel_bg_color: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_get_style_text_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_opa_t result = lv_obj_get_style_text_opa(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_text_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_text_opa(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_text_font(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style_text_font: Return type not found >const lv_font_t*< ");
    return NULL;
}

static PyObject*
pylv_obj_set_style_local_text_font(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_style_local_text_font: Parameter type not found >const lv_font_t*< ");
    return NULL;
}

static PyObject*
pylv_obj_get_style_line_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_line_width(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_line_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_line_width(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_line_blend_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_blend_mode_t result = lv_obj_get_style_line_blend_mode(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_line_blend_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_line_blend_mode(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_line_dash_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_line_dash_width(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_line_dash_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_line_dash_width(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_line_dash_gap(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_line_dash_gap(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_line_dash_gap(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_line_dash_gap(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_line_rounded(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    bool result = lv_obj_get_style_line_rounded(self->ref, part);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_set_style_local_line_rounded(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbp", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_line_rounded(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_line_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style_line_color: Return type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_set_style_local_line_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_style_local_line_color: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_get_style_line_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_opa_t result = lv_obj_get_style_line_opa(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_line_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_line_opa(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_image_blend_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_blend_mode_t result = lv_obj_get_style_image_blend_mode(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_image_blend_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_image_blend_mode(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_image_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style_image_recolor: Return type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_set_style_local_image_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_style_local_image_recolor: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_get_style_image_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_opa_t result = lv_obj_get_style_image_opa(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_image_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_image_opa(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_image_recolor_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_opa_t result = lv_obj_get_style_image_recolor_opa(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_set_style_local_image_recolor_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    unsigned char value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbb", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_image_recolor_opa(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_transition_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_transition_time(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_transition_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_transition_time(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_transition_delay(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_transition_delay(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_transition_delay(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_transition_delay(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_transition_prop_1(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_transition_prop_1(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_transition_prop_1(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_transition_prop_1(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_transition_prop_2(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_transition_prop_2(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_transition_prop_2(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_transition_prop_2(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_transition_prop_3(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_transition_prop_3(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_transition_prop_3(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_transition_prop_3(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_transition_prop_4(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_transition_prop_4(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_transition_prop_4(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_transition_prop_4(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_transition_prop_5(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_transition_prop_5(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_transition_prop_5(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_transition_prop_5(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_transition_prop_6(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_transition_prop_6(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_transition_prop_6(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_transition_prop_6(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_transition_path(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style_transition_path: Return type not found >const lv_anim_path_t*< ");
    return NULL;
}

static PyObject*
pylv_obj_set_style_local_transition_path(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_style_local_transition_path: Parameter type not found >const lv_anim_path_t*< ");
    return NULL;
}

static PyObject*
pylv_obj_get_style_scale_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_scale_width(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_scale_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_scale_width(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_scale_border_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_scale_border_width(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_scale_border_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_scale_border_width(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_scale_end_border_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_scale_end_border_width(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_scale_end_border_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_scale_end_border_width(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_scale_end_line_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_style_int_t result = lv_obj_get_style_scale_end_line_width(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_set_style_local_scale_end_line_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_scale_end_line_width(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_style_scale_grad_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style_scale_grad_color: Return type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_set_style_local_scale_grad_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_style_local_scale_grad_color: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_get_style_scale_end_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style_scale_end_color: Return type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_set_style_local_scale_end_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_style_local_scale_end_color: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_obj_set_style_local_pad_all(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_pad_all(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_style_local_pad_hor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_pad_hor(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_style_local_pad_ver(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_pad_ver(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_style_local_margin_all(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_margin_all(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_style_local_margin_hor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_margin_hor(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_style_local_margin_ver(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", "state", "value", NULL};
    unsigned char part;
    unsigned char state;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbh", kwlist , &part, &state, &value)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style_local_margin_ver(self->ref, part, state, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_hidden(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_obj_get_hidden(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_adv_hittest(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_obj_get_adv_hittest(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_click(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_obj_get_click(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_top(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_obj_get_top(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_drag(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_obj_get_drag(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_drag_dir(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_drag_dir_t result = lv_obj_get_drag_dir(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_get_drag_throw(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_obj_get_drag_throw(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_drag_parent(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_obj_get_drag_parent(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_focus_parent(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_obj_get_focus_parent(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_parent_event(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_obj_get_parent_event(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_gesture_parent(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_obj_get_gesture_parent(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_base_dir(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_bidi_dir_t result = lv_obj_get_base_dir(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_get_protect(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint8_t result = lv_obj_get_protect(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_is_protected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"prot", NULL};
    unsigned char prot;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &prot)) return NULL;

    LVGL_LOCK        
    bool result = lv_obj_is_protected(self->ref, prot);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_state(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_state_t result = lv_obj_get_state(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_get_signal_cb(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_signal_cb: Return type not found >lv_signal_cb_t< ");
    return NULL;
}

static PyObject*
pylv_obj_get_design_cb(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_design_cb: Return type not found >lv_design_cb_t< ");
    return NULL;
}

static PyObject*
pylv_obj_get_event_cb(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_event_cb: Return type not found >lv_event_cb_t< ");
    return NULL;
}

static PyObject*
pylv_obj_is_point_on_coords(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_is_point_on_coords: Parameter type not found >const lv_point_t*< ");
    return NULL;
}

static PyObject*
pylv_obj_hittest(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_hittest: Parameter type not found >lv_point_t*< ");
    return NULL;
}

static PyObject*
pylv_obj_get_user_data(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_user_data: Return type not found >lv_obj_user_data_t< ");
    return NULL;
}

static PyObject*
pylv_obj_get_user_data_ptr(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_user_data_ptr: Return type not found >lv_obj_user_data_t*< ");
    return NULL;
}

static PyObject*
pylv_obj_set_user_data(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_user_data: Parameter type not found >lv_obj_user_data_t< ");
    return NULL;
}

static PyObject*
pylv_obj_get_group(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_group: Return type not found >void*< ");
    return NULL;
}

static PyObject*
pylv_obj_is_focused(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_obj_is_focused(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_focused_obj(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_obj_get_focused_obj(self->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}

static PyObject*
pylv_obj_init_draw_rect_dsc(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_init_draw_rect_dsc: Parameter type not found >lv_draw_rect_dsc_t*< ");
    return NULL;
}

static PyObject*
pylv_obj_init_draw_label_dsc(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_init_draw_label_dsc: Parameter type not found >lv_draw_label_dsc_t*< ");
    return NULL;
}

static PyObject*
pylv_obj_init_draw_img_dsc(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_init_draw_img_dsc: Parameter type not found >lv_draw_img_dsc_t*< ");
    return NULL;
}

static PyObject*
pylv_obj_init_draw_line_dsc(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_init_draw_line_dsc: Parameter type not found >lv_draw_line_dsc_t*< ");
    return NULL;
}

static PyObject*
pylv_obj_get_draw_rect_ext_pad_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"part", NULL};
    unsigned char part;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &part)) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_obj_get_draw_rect_ext_pad_size(self->ref, part);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_fade_in(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"time", "delay", NULL};
    unsigned int time;
    unsigned int delay;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "II", kwlist , &time, &delay)) return NULL;

    LVGL_LOCK         
    lv_obj_fade_in(self->ref, time, delay);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_fade_out(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"time", "delay", NULL};
    unsigned int time;
    unsigned int delay;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "II", kwlist , &time, &delay)) return NULL;

    LVGL_LOCK         
    lv_obj_fade_out(self->ref, time, delay);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_align_origo(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"base", "align", "x_ofs", "y_ofs", NULL};
    pylv_Obj * base;
    unsigned char align;
    short int x_ofs;
    short int y_ofs;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!bhh", kwlist , &pylv_obj_Type, &base, &align, &x_ofs, &y_ofs)) return NULL;

    LVGL_LOCK         
    lv_obj_align_origo(self->ref, base->ref, align, x_ofs, y_ofs);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_align_origo_x(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"base", "align", "x_ofs", NULL};
    pylv_Obj * base;
    unsigned char align;
    short int x_ofs;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!bh", kwlist , &pylv_obj_Type, &base, &align, &x_ofs)) return NULL;

    LVGL_LOCK         
    lv_obj_align_origo_x(self->ref, base->ref, align, x_ofs);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_align_origo_y(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"base", "align", "y_ofs", NULL};
    pylv_Obj * base;
    unsigned char align;
    short int y_ofs;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!bh", kwlist , &pylv_obj_Type, &base, &align, &y_ofs)) return NULL;

    LVGL_LOCK         
    lv_obj_align_origo_y(self->ref, base->ref, align, y_ofs);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}


static PyMethodDef pylv_obj_methods[] = {
    {"del_", (PyCFunction) pylv_obj_del, METH_VARARGS | METH_KEYWORDS, "lv_res_t lv_obj_del(lv_obj_t *obj)"},
    {"clean", (PyCFunction) pylv_obj_clean, METH_VARARGS | METH_KEYWORDS, "void lv_obj_clean(lv_obj_t *obj)"},
    {"invalidate_area", (PyCFunction) pylv_obj_invalidate_area, METH_VARARGS | METH_KEYWORDS, "void lv_obj_invalidate_area(const lv_obj_t *obj, const lv_area_t *area)"},
    {"invalidate", (PyCFunction) pylv_obj_invalidate, METH_VARARGS | METH_KEYWORDS, "void lv_obj_invalidate(const lv_obj_t *obj)"},
    {"area_is_visible", (PyCFunction) pylv_obj_area_is_visible, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_area_is_visible(const lv_obj_t *obj, lv_area_t *area)"},
    {"is_visible", (PyCFunction) pylv_obj_is_visible, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_is_visible(const lv_obj_t *obj)"},
    {"set_parent", (PyCFunction) pylv_obj_set_parent, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_parent(lv_obj_t *obj, lv_obj_t *parent)"},
    {"move_foreground", (PyCFunction) pylv_obj_move_foreground, METH_VARARGS | METH_KEYWORDS, "void lv_obj_move_foreground(lv_obj_t *obj)"},
    {"move_background", (PyCFunction) pylv_obj_move_background, METH_VARARGS | METH_KEYWORDS, "void lv_obj_move_background(lv_obj_t *obj)"},
    {"set_pos", (PyCFunction) pylv_obj_set_pos, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_pos(lv_obj_t *obj, lv_coord_t x, lv_coord_t y)"},
    {"set_x", (PyCFunction) pylv_obj_set_x, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_x(lv_obj_t *obj, lv_coord_t x)"},
    {"set_y", (PyCFunction) pylv_obj_set_y, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_y(lv_obj_t *obj, lv_coord_t y)"},
    {"set_size", (PyCFunction) pylv_obj_set_size, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_size(lv_obj_t *obj, lv_coord_t w, lv_coord_t h)"},
    {"set_width", (PyCFunction) pylv_obj_set_width, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_width(lv_obj_t *obj, lv_coord_t w)"},
    {"set_height", (PyCFunction) pylv_obj_set_height, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_height(lv_obj_t *obj, lv_coord_t h)"},
    {"set_width_fit", (PyCFunction) pylv_obj_set_width_fit, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_width_fit(lv_obj_t *obj, lv_coord_t w)"},
    {"set_height_fit", (PyCFunction) pylv_obj_set_height_fit, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_height_fit(lv_obj_t *obj, lv_coord_t h)"},
    {"set_width_margin", (PyCFunction) pylv_obj_set_width_margin, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_width_margin(lv_obj_t *obj, lv_coord_t w)"},
    {"set_height_margin", (PyCFunction) pylv_obj_set_height_margin, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_height_margin(lv_obj_t *obj, lv_coord_t h)"},
    {"align", (PyCFunction) pylv_obj_align, METH_VARARGS | METH_KEYWORDS, "void lv_obj_align(lv_obj_t *obj, const lv_obj_t *base, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs)"},
    {"align_x", (PyCFunction) pylv_obj_align_x, METH_VARARGS | METH_KEYWORDS, "void lv_obj_align_x(lv_obj_t *obj, const lv_obj_t *base, lv_align_t align, lv_coord_t x_ofs)"},
    {"align_y", (PyCFunction) pylv_obj_align_y, METH_VARARGS | METH_KEYWORDS, "void lv_obj_align_y(lv_obj_t *obj, const lv_obj_t *base, lv_align_t align, lv_coord_t y_ofs)"},
    {"align_mid", (PyCFunction) pylv_obj_align_mid, METH_VARARGS | METH_KEYWORDS, "void lv_obj_align_mid(lv_obj_t *obj, const lv_obj_t *base, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs)"},
    {"align_mid_x", (PyCFunction) pylv_obj_align_mid_x, METH_VARARGS | METH_KEYWORDS, "void lv_obj_align_mid_x(lv_obj_t *obj, const lv_obj_t *base, lv_align_t align, lv_coord_t x_ofs)"},
    {"align_mid_y", (PyCFunction) pylv_obj_align_mid_y, METH_VARARGS | METH_KEYWORDS, "void lv_obj_align_mid_y(lv_obj_t *obj, const lv_obj_t *base, lv_align_t align, lv_coord_t y_ofs)"},
    {"realign", (PyCFunction) pylv_obj_realign, METH_VARARGS | METH_KEYWORDS, "void lv_obj_realign(lv_obj_t *obj)"},
    {"set_auto_realign", (PyCFunction) pylv_obj_set_auto_realign, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_auto_realign(lv_obj_t *obj, bool en)"},
    {"set_ext_click_area", (PyCFunction) pylv_obj_set_ext_click_area, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_ext_click_area(lv_obj_t *obj, lv_coord_t left, lv_coord_t right, lv_coord_t top, lv_coord_t bottom)"},
    {"add_style", (PyCFunction) pylv_obj_add_style, METH_VARARGS | METH_KEYWORDS, "void lv_obj_add_style(lv_obj_t *obj, uint8_t part, lv_style_t *style)"},
    {"remove_style", (PyCFunction) pylv_obj_remove_style, METH_VARARGS | METH_KEYWORDS, "void lv_obj_remove_style(lv_obj_t *obj, uint8_t part, lv_style_t *style)"},
    {"clean_style_list", (PyCFunction) pylv_obj_clean_style_list, METH_VARARGS | METH_KEYWORDS, "void lv_obj_clean_style_list(lv_obj_t *obj, uint8_t part)"},
    {"reset_style_list", (PyCFunction) pylv_obj_reset_style_list, METH_VARARGS | METH_KEYWORDS, "void lv_obj_reset_style_list(lv_obj_t *obj, uint8_t part)"},
    {"refresh_style", (PyCFunction) pylv_obj_refresh_style, METH_VARARGS | METH_KEYWORDS, "void lv_obj_refresh_style(lv_obj_t *obj, uint8_t part, lv_style_property_t prop)"},
    {"remove_style_local_prop", (PyCFunction) pylv_obj_remove_style_local_prop, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_remove_style_local_prop(lv_obj_t *obj, uint8_t part, lv_style_property_t prop)"},
    {"set_hidden", (PyCFunction) pylv_obj_set_hidden, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_hidden(lv_obj_t *obj, bool en)"},
    {"set_adv_hittest", (PyCFunction) pylv_obj_set_adv_hittest, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_adv_hittest(lv_obj_t *obj, bool en)"},
    {"set_click", (PyCFunction) pylv_obj_set_click, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_click(lv_obj_t *obj, bool en)"},
    {"set_top", (PyCFunction) pylv_obj_set_top, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_top(lv_obj_t *obj, bool en)"},
    {"set_drag", (PyCFunction) pylv_obj_set_drag, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_drag(lv_obj_t *obj, bool en)"},
    {"set_drag_dir", (PyCFunction) pylv_obj_set_drag_dir, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_drag_dir(lv_obj_t *obj, lv_drag_dir_t drag_dir)"},
    {"set_drag_throw", (PyCFunction) pylv_obj_set_drag_throw, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_drag_throw(lv_obj_t *obj, bool en)"},
    {"set_drag_parent", (PyCFunction) pylv_obj_set_drag_parent, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_drag_parent(lv_obj_t *obj, bool en)"},
    {"set_focus_parent", (PyCFunction) pylv_obj_set_focus_parent, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_focus_parent(lv_obj_t *obj, bool en)"},
    {"set_gesture_parent", (PyCFunction) pylv_obj_set_gesture_parent, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_gesture_parent(lv_obj_t *obj, bool en)"},
    {"set_parent_event", (PyCFunction) pylv_obj_set_parent_event, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_parent_event(lv_obj_t *obj, bool en)"},
    {"set_base_dir", (PyCFunction) pylv_obj_set_base_dir, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_base_dir(lv_obj_t *obj, lv_bidi_dir_t dir)"},
    {"add_protect", (PyCFunction) pylv_obj_add_protect, METH_VARARGS | METH_KEYWORDS, "void lv_obj_add_protect(lv_obj_t *obj, uint8_t prot)"},
    {"clear_protect", (PyCFunction) pylv_obj_clear_protect, METH_VARARGS | METH_KEYWORDS, "void lv_obj_clear_protect(lv_obj_t *obj, uint8_t prot)"},
    {"set_state", (PyCFunction) pylv_obj_set_state, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_state(lv_obj_t *obj, lv_state_t state)"},
    {"add_state", (PyCFunction) pylv_obj_add_state, METH_VARARGS | METH_KEYWORDS, "void lv_obj_add_state(lv_obj_t *obj, lv_state_t state)"},
    {"clear_state", (PyCFunction) pylv_obj_clear_state, METH_VARARGS | METH_KEYWORDS, "void lv_obj_clear_state(lv_obj_t *obj, lv_state_t state)"},
    {"finish_transitions", (PyCFunction) pylv_obj_finish_transitions, METH_VARARGS | METH_KEYWORDS, "void lv_obj_finish_transitions(lv_obj_t *obj, uint8_t part)"},
    {"set_event_cb", (PyCFunction) pylv_obj_set_event_cb, METH_VARARGS | METH_KEYWORDS, ""},
    {"set_signal_cb", (PyCFunction) pylv_obj_set_signal_cb, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_signal_cb(lv_obj_t *obj, lv_signal_cb_t signal_cb)"},
    {"set_design_cb", (PyCFunction) pylv_obj_set_design_cb, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_design_cb(lv_obj_t *obj, lv_design_cb_t design_cb)"},
    {"refresh_ext_draw_pad", (PyCFunction) pylv_obj_refresh_ext_draw_pad, METH_VARARGS | METH_KEYWORDS, "void lv_obj_refresh_ext_draw_pad(lv_obj_t *obj)"},
    {"get_screen", (PyCFunction) pylv_obj_get_screen, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_obj_get_screen(const lv_obj_t *obj)"},
    {"get_disp", (PyCFunction) pylv_obj_get_disp, METH_VARARGS | METH_KEYWORDS, "lv_disp_t *lv_obj_get_disp(const lv_obj_t *obj)"},
    {"get_parent", (PyCFunction) pylv_obj_get_parent, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_obj_get_parent(const lv_obj_t *obj)"},
    {"count_children", (PyCFunction) pylv_obj_count_children, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_obj_count_children(const lv_obj_t *obj)"},
    {"count_children_recursive", (PyCFunction) pylv_obj_count_children_recursive, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_obj_count_children_recursive(const lv_obj_t *obj)"},
    {"get_coords", (PyCFunction) pylv_obj_get_coords, METH_VARARGS | METH_KEYWORDS, "void lv_obj_get_coords(const lv_obj_t *obj, lv_area_t *cords_p)"},
    {"get_inner_coords", (PyCFunction) pylv_obj_get_inner_coords, METH_VARARGS | METH_KEYWORDS, "void lv_obj_get_inner_coords(const lv_obj_t *obj, lv_area_t *coords_p)"},
    {"get_x", (PyCFunction) pylv_obj_get_x, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_x(const lv_obj_t *obj)"},
    {"get_y", (PyCFunction) pylv_obj_get_y, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_y(const lv_obj_t *obj)"},
    {"get_width", (PyCFunction) pylv_obj_get_width, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_width(const lv_obj_t *obj)"},
    {"get_height", (PyCFunction) pylv_obj_get_height, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_height(const lv_obj_t *obj)"},
    {"get_width_fit", (PyCFunction) pylv_obj_get_width_fit, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_width_fit(const lv_obj_t *obj)"},
    {"get_height_fit", (PyCFunction) pylv_obj_get_height_fit, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_height_fit(const lv_obj_t *obj)"},
    {"get_height_margin", (PyCFunction) pylv_obj_get_height_margin, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_height_margin(lv_obj_t *obj)"},
    {"get_width_margin", (PyCFunction) pylv_obj_get_width_margin, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_width_margin(lv_obj_t *obj)"},
    {"get_width_grid", (PyCFunction) pylv_obj_get_width_grid, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_width_grid(lv_obj_t *obj, uint8_t div, uint8_t span)"},
    {"get_height_grid", (PyCFunction) pylv_obj_get_height_grid, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_height_grid(lv_obj_t *obj, uint8_t div, uint8_t span)"},
    {"get_auto_realign", (PyCFunction) pylv_obj_get_auto_realign, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_auto_realign(const lv_obj_t *obj)"},
    {"get_ext_click_pad_left", (PyCFunction) pylv_obj_get_ext_click_pad_left, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_ext_click_pad_left(const lv_obj_t *obj)"},
    {"get_ext_click_pad_right", (PyCFunction) pylv_obj_get_ext_click_pad_right, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_ext_click_pad_right(const lv_obj_t *obj)"},
    {"get_ext_click_pad_top", (PyCFunction) pylv_obj_get_ext_click_pad_top, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_ext_click_pad_top(const lv_obj_t *obj)"},
    {"get_ext_click_pad_bottom", (PyCFunction) pylv_obj_get_ext_click_pad_bottom, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_ext_click_pad_bottom(const lv_obj_t *obj)"},
    {"get_ext_draw_pad", (PyCFunction) pylv_obj_get_ext_draw_pad, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_ext_draw_pad(const lv_obj_t *obj)"},
    {"get_style_list", (PyCFunction) pylv_obj_get_style_list, METH_VARARGS | METH_KEYWORDS, "lv_style_list_t *lv_obj_get_style_list(const lv_obj_t *obj, uint8_t part)"},
    {"get_local_style", (PyCFunction) pylv_obj_get_local_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_obj_get_local_style(lv_obj_t *obj, uint8_t part)"},
    {"get_style_radius", (PyCFunction) pylv_obj_get_style_radius, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_radius(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_radius", (PyCFunction) pylv_obj_set_style_local_radius, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_radius(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_clip_corner", (PyCFunction) pylv_obj_get_style_clip_corner, METH_VARARGS | METH_KEYWORDS, "inline static bool lv_obj_get_style_clip_corner(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_clip_corner", (PyCFunction) pylv_obj_set_style_local_clip_corner, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_clip_corner(lv_obj_t *obj, uint8_t part, lv_state_t state, bool value)"},
    {"get_style_size", (PyCFunction) pylv_obj_get_style_size, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_size(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_size", (PyCFunction) pylv_obj_set_style_local_size, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_size(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_transform_width", (PyCFunction) pylv_obj_get_style_transform_width, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_transform_width(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_transform_width", (PyCFunction) pylv_obj_set_style_local_transform_width, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_transform_width(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_transform_height", (PyCFunction) pylv_obj_get_style_transform_height, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_transform_height(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_transform_height", (PyCFunction) pylv_obj_set_style_local_transform_height, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_transform_height(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_transform_angle", (PyCFunction) pylv_obj_get_style_transform_angle, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_transform_angle(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_transform_angle", (PyCFunction) pylv_obj_set_style_local_transform_angle, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_transform_angle(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_transform_zoom", (PyCFunction) pylv_obj_get_style_transform_zoom, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_transform_zoom(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_transform_zoom", (PyCFunction) pylv_obj_set_style_local_transform_zoom, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_transform_zoom(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_opa_scale", (PyCFunction) pylv_obj_get_style_opa_scale, METH_VARARGS | METH_KEYWORDS, "inline static lv_opa_t lv_obj_get_style_opa_scale(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_opa_scale", (PyCFunction) pylv_obj_set_style_local_opa_scale, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_opa_scale(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_opa_t value)"},
    {"get_style_pad_top", (PyCFunction) pylv_obj_get_style_pad_top, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_pad_top(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_pad_top", (PyCFunction) pylv_obj_set_style_local_pad_top, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_pad_top(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_pad_bottom", (PyCFunction) pylv_obj_get_style_pad_bottom, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_pad_bottom(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_pad_bottom", (PyCFunction) pylv_obj_set_style_local_pad_bottom, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_pad_bottom(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_pad_left", (PyCFunction) pylv_obj_get_style_pad_left, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_pad_left(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_pad_left", (PyCFunction) pylv_obj_set_style_local_pad_left, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_pad_left(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_pad_right", (PyCFunction) pylv_obj_get_style_pad_right, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_pad_right(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_pad_right", (PyCFunction) pylv_obj_set_style_local_pad_right, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_pad_right(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_pad_inner", (PyCFunction) pylv_obj_get_style_pad_inner, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_pad_inner(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_pad_inner", (PyCFunction) pylv_obj_set_style_local_pad_inner, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_pad_inner(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_margin_top", (PyCFunction) pylv_obj_get_style_margin_top, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_margin_top(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_margin_top", (PyCFunction) pylv_obj_set_style_local_margin_top, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_margin_top(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_margin_bottom", (PyCFunction) pylv_obj_get_style_margin_bottom, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_margin_bottom(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_margin_bottom", (PyCFunction) pylv_obj_set_style_local_margin_bottom, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_margin_bottom(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_margin_left", (PyCFunction) pylv_obj_get_style_margin_left, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_margin_left(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_margin_left", (PyCFunction) pylv_obj_set_style_local_margin_left, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_margin_left(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_margin_right", (PyCFunction) pylv_obj_get_style_margin_right, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_margin_right(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_margin_right", (PyCFunction) pylv_obj_set_style_local_margin_right, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_margin_right(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_bg_blend_mode", (PyCFunction) pylv_obj_get_style_bg_blend_mode, METH_VARARGS | METH_KEYWORDS, "inline static lv_blend_mode_t lv_obj_get_style_bg_blend_mode(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_bg_blend_mode", (PyCFunction) pylv_obj_set_style_local_bg_blend_mode, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_bg_blend_mode(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_blend_mode_t value)"},
    {"get_style_bg_main_stop", (PyCFunction) pylv_obj_get_style_bg_main_stop, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_bg_main_stop(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_bg_main_stop", (PyCFunction) pylv_obj_set_style_local_bg_main_stop, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_bg_main_stop(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_bg_grad_stop", (PyCFunction) pylv_obj_get_style_bg_grad_stop, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_bg_grad_stop(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_bg_grad_stop", (PyCFunction) pylv_obj_set_style_local_bg_grad_stop, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_bg_grad_stop(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_bg_grad_dir", (PyCFunction) pylv_obj_get_style_bg_grad_dir, METH_VARARGS | METH_KEYWORDS, "inline static lv_grad_dir_t lv_obj_get_style_bg_grad_dir(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_bg_grad_dir", (PyCFunction) pylv_obj_set_style_local_bg_grad_dir, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_bg_grad_dir(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_grad_dir_t value)"},
    {"get_style_bg_color", (PyCFunction) pylv_obj_get_style_bg_color, METH_VARARGS | METH_KEYWORDS, "inline static lv_color_t lv_obj_get_style_bg_color(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_bg_color", (PyCFunction) pylv_obj_set_style_local_bg_color, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_bg_color(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_color_t value)"},
    {"get_style_bg_grad_color", (PyCFunction) pylv_obj_get_style_bg_grad_color, METH_VARARGS | METH_KEYWORDS, "inline static lv_color_t lv_obj_get_style_bg_grad_color(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_bg_grad_color", (PyCFunction) pylv_obj_set_style_local_bg_grad_color, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_bg_grad_color(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_color_t value)"},
    {"get_style_bg_opa", (PyCFunction) pylv_obj_get_style_bg_opa, METH_VARARGS | METH_KEYWORDS, "inline static lv_opa_t lv_obj_get_style_bg_opa(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_bg_opa", (PyCFunction) pylv_obj_set_style_local_bg_opa, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_bg_opa(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_opa_t value)"},
    {"get_style_border_width", (PyCFunction) pylv_obj_get_style_border_width, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_border_width(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_border_width", (PyCFunction) pylv_obj_set_style_local_border_width, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_border_width(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_border_side", (PyCFunction) pylv_obj_get_style_border_side, METH_VARARGS | METH_KEYWORDS, "inline static lv_border_side_t lv_obj_get_style_border_side(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_border_side", (PyCFunction) pylv_obj_set_style_local_border_side, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_border_side(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_border_side_t value)"},
    {"get_style_border_blend_mode", (PyCFunction) pylv_obj_get_style_border_blend_mode, METH_VARARGS | METH_KEYWORDS, "inline static lv_blend_mode_t lv_obj_get_style_border_blend_mode(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_border_blend_mode", (PyCFunction) pylv_obj_set_style_local_border_blend_mode, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_border_blend_mode(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_blend_mode_t value)"},
    {"get_style_border_post", (PyCFunction) pylv_obj_get_style_border_post, METH_VARARGS | METH_KEYWORDS, "inline static bool lv_obj_get_style_border_post(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_border_post", (PyCFunction) pylv_obj_set_style_local_border_post, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_border_post(lv_obj_t *obj, uint8_t part, lv_state_t state, bool value)"},
    {"get_style_border_color", (PyCFunction) pylv_obj_get_style_border_color, METH_VARARGS | METH_KEYWORDS, "inline static lv_color_t lv_obj_get_style_border_color(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_border_color", (PyCFunction) pylv_obj_set_style_local_border_color, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_border_color(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_color_t value)"},
    {"get_style_border_opa", (PyCFunction) pylv_obj_get_style_border_opa, METH_VARARGS | METH_KEYWORDS, "inline static lv_opa_t lv_obj_get_style_border_opa(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_border_opa", (PyCFunction) pylv_obj_set_style_local_border_opa, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_border_opa(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_opa_t value)"},
    {"get_style_outline_width", (PyCFunction) pylv_obj_get_style_outline_width, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_outline_width(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_outline_width", (PyCFunction) pylv_obj_set_style_local_outline_width, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_outline_width(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_outline_pad", (PyCFunction) pylv_obj_get_style_outline_pad, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_outline_pad(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_outline_pad", (PyCFunction) pylv_obj_set_style_local_outline_pad, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_outline_pad(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_outline_blend_mode", (PyCFunction) pylv_obj_get_style_outline_blend_mode, METH_VARARGS | METH_KEYWORDS, "inline static lv_blend_mode_t lv_obj_get_style_outline_blend_mode(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_outline_blend_mode", (PyCFunction) pylv_obj_set_style_local_outline_blend_mode, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_outline_blend_mode(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_blend_mode_t value)"},
    {"get_style_outline_color", (PyCFunction) pylv_obj_get_style_outline_color, METH_VARARGS | METH_KEYWORDS, "inline static lv_color_t lv_obj_get_style_outline_color(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_outline_color", (PyCFunction) pylv_obj_set_style_local_outline_color, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_outline_color(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_color_t value)"},
    {"get_style_outline_opa", (PyCFunction) pylv_obj_get_style_outline_opa, METH_VARARGS | METH_KEYWORDS, "inline static lv_opa_t lv_obj_get_style_outline_opa(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_outline_opa", (PyCFunction) pylv_obj_set_style_local_outline_opa, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_outline_opa(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_opa_t value)"},
    {"get_style_shadow_width", (PyCFunction) pylv_obj_get_style_shadow_width, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_shadow_width(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_shadow_width", (PyCFunction) pylv_obj_set_style_local_shadow_width, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_shadow_width(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_shadow_ofs_x", (PyCFunction) pylv_obj_get_style_shadow_ofs_x, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_shadow_ofs_x(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_shadow_ofs_x", (PyCFunction) pylv_obj_set_style_local_shadow_ofs_x, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_shadow_ofs_x(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_shadow_ofs_y", (PyCFunction) pylv_obj_get_style_shadow_ofs_y, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_shadow_ofs_y(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_shadow_ofs_y", (PyCFunction) pylv_obj_set_style_local_shadow_ofs_y, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_shadow_ofs_y(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_shadow_spread", (PyCFunction) pylv_obj_get_style_shadow_spread, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_shadow_spread(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_shadow_spread", (PyCFunction) pylv_obj_set_style_local_shadow_spread, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_shadow_spread(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_shadow_blend_mode", (PyCFunction) pylv_obj_get_style_shadow_blend_mode, METH_VARARGS | METH_KEYWORDS, "inline static lv_blend_mode_t lv_obj_get_style_shadow_blend_mode(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_shadow_blend_mode", (PyCFunction) pylv_obj_set_style_local_shadow_blend_mode, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_shadow_blend_mode(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_blend_mode_t value)"},
    {"get_style_shadow_color", (PyCFunction) pylv_obj_get_style_shadow_color, METH_VARARGS | METH_KEYWORDS, "inline static lv_color_t lv_obj_get_style_shadow_color(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_shadow_color", (PyCFunction) pylv_obj_set_style_local_shadow_color, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_shadow_color(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_color_t value)"},
    {"get_style_shadow_opa", (PyCFunction) pylv_obj_get_style_shadow_opa, METH_VARARGS | METH_KEYWORDS, "inline static lv_opa_t lv_obj_get_style_shadow_opa(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_shadow_opa", (PyCFunction) pylv_obj_set_style_local_shadow_opa, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_shadow_opa(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_opa_t value)"},
    {"get_style_pattern_repeat", (PyCFunction) pylv_obj_get_style_pattern_repeat, METH_VARARGS | METH_KEYWORDS, "inline static bool lv_obj_get_style_pattern_repeat(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_pattern_repeat", (PyCFunction) pylv_obj_set_style_local_pattern_repeat, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_pattern_repeat(lv_obj_t *obj, uint8_t part, lv_state_t state, bool value)"},
    {"get_style_pattern_blend_mode", (PyCFunction) pylv_obj_get_style_pattern_blend_mode, METH_VARARGS | METH_KEYWORDS, "inline static lv_blend_mode_t lv_obj_get_style_pattern_blend_mode(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_pattern_blend_mode", (PyCFunction) pylv_obj_set_style_local_pattern_blend_mode, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_pattern_blend_mode(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_blend_mode_t value)"},
    {"get_style_pattern_recolor", (PyCFunction) pylv_obj_get_style_pattern_recolor, METH_VARARGS | METH_KEYWORDS, "inline static lv_color_t lv_obj_get_style_pattern_recolor(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_pattern_recolor", (PyCFunction) pylv_obj_set_style_local_pattern_recolor, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_pattern_recolor(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_color_t value)"},
    {"get_style_pattern_opa", (PyCFunction) pylv_obj_get_style_pattern_opa, METH_VARARGS | METH_KEYWORDS, "inline static lv_opa_t lv_obj_get_style_pattern_opa(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_pattern_opa", (PyCFunction) pylv_obj_set_style_local_pattern_opa, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_pattern_opa(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_opa_t value)"},
    {"get_style_pattern_recolor_opa", (PyCFunction) pylv_obj_get_style_pattern_recolor_opa, METH_VARARGS | METH_KEYWORDS, "inline static lv_opa_t lv_obj_get_style_pattern_recolor_opa(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_pattern_recolor_opa", (PyCFunction) pylv_obj_set_style_local_pattern_recolor_opa, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_pattern_recolor_opa(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_opa_t value)"},
    {"get_style_pattern_image", (PyCFunction) pylv_obj_get_style_pattern_image, METH_VARARGS | METH_KEYWORDS, "inline static const void *lv_obj_get_style_pattern_image(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_pattern_image", (PyCFunction) pylv_obj_set_style_local_pattern_image, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_pattern_image(lv_obj_t *obj, uint8_t part, lv_state_t state, const void *value)"},
    {"get_style_value_letter_space", (PyCFunction) pylv_obj_get_style_value_letter_space, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_value_letter_space(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_value_letter_space", (PyCFunction) pylv_obj_set_style_local_value_letter_space, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_value_letter_space(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_value_line_space", (PyCFunction) pylv_obj_get_style_value_line_space, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_value_line_space(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_value_line_space", (PyCFunction) pylv_obj_set_style_local_value_line_space, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_value_line_space(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_value_blend_mode", (PyCFunction) pylv_obj_get_style_value_blend_mode, METH_VARARGS | METH_KEYWORDS, "inline static lv_blend_mode_t lv_obj_get_style_value_blend_mode(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_value_blend_mode", (PyCFunction) pylv_obj_set_style_local_value_blend_mode, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_value_blend_mode(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_blend_mode_t value)"},
    {"get_style_value_ofs_x", (PyCFunction) pylv_obj_get_style_value_ofs_x, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_value_ofs_x(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_value_ofs_x", (PyCFunction) pylv_obj_set_style_local_value_ofs_x, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_value_ofs_x(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_value_ofs_y", (PyCFunction) pylv_obj_get_style_value_ofs_y, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_value_ofs_y(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_value_ofs_y", (PyCFunction) pylv_obj_set_style_local_value_ofs_y, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_value_ofs_y(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_value_align", (PyCFunction) pylv_obj_get_style_value_align, METH_VARARGS | METH_KEYWORDS, "inline static lv_align_t lv_obj_get_style_value_align(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_value_align", (PyCFunction) pylv_obj_set_style_local_value_align, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_value_align(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_align_t value)"},
    {"get_style_value_color", (PyCFunction) pylv_obj_get_style_value_color, METH_VARARGS | METH_KEYWORDS, "inline static lv_color_t lv_obj_get_style_value_color(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_value_color", (PyCFunction) pylv_obj_set_style_local_value_color, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_value_color(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_color_t value)"},
    {"get_style_value_opa", (PyCFunction) pylv_obj_get_style_value_opa, METH_VARARGS | METH_KEYWORDS, "inline static lv_opa_t lv_obj_get_style_value_opa(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_value_opa", (PyCFunction) pylv_obj_set_style_local_value_opa, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_value_opa(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_opa_t value)"},
    {"get_style_value_font", (PyCFunction) pylv_obj_get_style_value_font, METH_VARARGS | METH_KEYWORDS, "inline static const lv_font_t *lv_obj_get_style_value_font(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_value_font", (PyCFunction) pylv_obj_set_style_local_value_font, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_value_font(lv_obj_t *obj, uint8_t part, lv_state_t state, const lv_font_t *value)"},
    {"get_style_value_str", (PyCFunction) pylv_obj_get_style_value_str, METH_VARARGS | METH_KEYWORDS, "inline static const char *lv_obj_get_style_value_str(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_value_str", (PyCFunction) pylv_obj_set_style_local_value_str, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_value_str(lv_obj_t *obj, uint8_t part, lv_state_t state, const char *value)"},
    {"get_style_text_letter_space", (PyCFunction) pylv_obj_get_style_text_letter_space, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_text_letter_space(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_text_letter_space", (PyCFunction) pylv_obj_set_style_local_text_letter_space, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_text_letter_space(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_text_line_space", (PyCFunction) pylv_obj_get_style_text_line_space, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_text_line_space(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_text_line_space", (PyCFunction) pylv_obj_set_style_local_text_line_space, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_text_line_space(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_text_decor", (PyCFunction) pylv_obj_get_style_text_decor, METH_VARARGS | METH_KEYWORDS, "inline static lv_text_decor_t lv_obj_get_style_text_decor(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_text_decor", (PyCFunction) pylv_obj_set_style_local_text_decor, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_text_decor(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_text_decor_t value)"},
    {"get_style_text_blend_mode", (PyCFunction) pylv_obj_get_style_text_blend_mode, METH_VARARGS | METH_KEYWORDS, "inline static lv_blend_mode_t lv_obj_get_style_text_blend_mode(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_text_blend_mode", (PyCFunction) pylv_obj_set_style_local_text_blend_mode, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_text_blend_mode(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_blend_mode_t value)"},
    {"get_style_text_color", (PyCFunction) pylv_obj_get_style_text_color, METH_VARARGS | METH_KEYWORDS, "inline static lv_color_t lv_obj_get_style_text_color(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_text_color", (PyCFunction) pylv_obj_set_style_local_text_color, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_text_color(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_color_t value)"},
    {"get_style_text_sel_color", (PyCFunction) pylv_obj_get_style_text_sel_color, METH_VARARGS | METH_KEYWORDS, "inline static lv_color_t lv_obj_get_style_text_sel_color(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_text_sel_color", (PyCFunction) pylv_obj_set_style_local_text_sel_color, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_text_sel_color(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_color_t value)"},
    {"get_style_text_sel_bg_color", (PyCFunction) pylv_obj_get_style_text_sel_bg_color, METH_VARARGS | METH_KEYWORDS, "inline static lv_color_t lv_obj_get_style_text_sel_bg_color(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_text_sel_bg_color", (PyCFunction) pylv_obj_set_style_local_text_sel_bg_color, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_text_sel_bg_color(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_color_t value)"},
    {"get_style_text_opa", (PyCFunction) pylv_obj_get_style_text_opa, METH_VARARGS | METH_KEYWORDS, "inline static lv_opa_t lv_obj_get_style_text_opa(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_text_opa", (PyCFunction) pylv_obj_set_style_local_text_opa, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_text_opa(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_opa_t value)"},
    {"get_style_text_font", (PyCFunction) pylv_obj_get_style_text_font, METH_VARARGS | METH_KEYWORDS, "inline static const lv_font_t *lv_obj_get_style_text_font(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_text_font", (PyCFunction) pylv_obj_set_style_local_text_font, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_text_font(lv_obj_t *obj, uint8_t part, lv_state_t state, const lv_font_t *value)"},
    {"get_style_line_width", (PyCFunction) pylv_obj_get_style_line_width, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_line_width(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_line_width", (PyCFunction) pylv_obj_set_style_local_line_width, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_line_width(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_line_blend_mode", (PyCFunction) pylv_obj_get_style_line_blend_mode, METH_VARARGS | METH_KEYWORDS, "inline static lv_blend_mode_t lv_obj_get_style_line_blend_mode(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_line_blend_mode", (PyCFunction) pylv_obj_set_style_local_line_blend_mode, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_line_blend_mode(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_blend_mode_t value)"},
    {"get_style_line_dash_width", (PyCFunction) pylv_obj_get_style_line_dash_width, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_line_dash_width(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_line_dash_width", (PyCFunction) pylv_obj_set_style_local_line_dash_width, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_line_dash_width(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_line_dash_gap", (PyCFunction) pylv_obj_get_style_line_dash_gap, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_line_dash_gap(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_line_dash_gap", (PyCFunction) pylv_obj_set_style_local_line_dash_gap, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_line_dash_gap(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_line_rounded", (PyCFunction) pylv_obj_get_style_line_rounded, METH_VARARGS | METH_KEYWORDS, "inline static bool lv_obj_get_style_line_rounded(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_line_rounded", (PyCFunction) pylv_obj_set_style_local_line_rounded, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_line_rounded(lv_obj_t *obj, uint8_t part, lv_state_t state, bool value)"},
    {"get_style_line_color", (PyCFunction) pylv_obj_get_style_line_color, METH_VARARGS | METH_KEYWORDS, "inline static lv_color_t lv_obj_get_style_line_color(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_line_color", (PyCFunction) pylv_obj_set_style_local_line_color, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_line_color(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_color_t value)"},
    {"get_style_line_opa", (PyCFunction) pylv_obj_get_style_line_opa, METH_VARARGS | METH_KEYWORDS, "inline static lv_opa_t lv_obj_get_style_line_opa(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_line_opa", (PyCFunction) pylv_obj_set_style_local_line_opa, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_line_opa(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_opa_t value)"},
    {"get_style_image_blend_mode", (PyCFunction) pylv_obj_get_style_image_blend_mode, METH_VARARGS | METH_KEYWORDS, "inline static lv_blend_mode_t lv_obj_get_style_image_blend_mode(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_image_blend_mode", (PyCFunction) pylv_obj_set_style_local_image_blend_mode, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_image_blend_mode(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_blend_mode_t value)"},
    {"get_style_image_recolor", (PyCFunction) pylv_obj_get_style_image_recolor, METH_VARARGS | METH_KEYWORDS, "inline static lv_color_t lv_obj_get_style_image_recolor(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_image_recolor", (PyCFunction) pylv_obj_set_style_local_image_recolor, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_image_recolor(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_color_t value)"},
    {"get_style_image_opa", (PyCFunction) pylv_obj_get_style_image_opa, METH_VARARGS | METH_KEYWORDS, "inline static lv_opa_t lv_obj_get_style_image_opa(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_image_opa", (PyCFunction) pylv_obj_set_style_local_image_opa, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_image_opa(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_opa_t value)"},
    {"get_style_image_recolor_opa", (PyCFunction) pylv_obj_get_style_image_recolor_opa, METH_VARARGS | METH_KEYWORDS, "inline static lv_opa_t lv_obj_get_style_image_recolor_opa(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_image_recolor_opa", (PyCFunction) pylv_obj_set_style_local_image_recolor_opa, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_image_recolor_opa(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_opa_t value)"},
    {"get_style_transition_time", (PyCFunction) pylv_obj_get_style_transition_time, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_transition_time(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_transition_time", (PyCFunction) pylv_obj_set_style_local_transition_time, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_transition_time(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_transition_delay", (PyCFunction) pylv_obj_get_style_transition_delay, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_transition_delay(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_transition_delay", (PyCFunction) pylv_obj_set_style_local_transition_delay, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_transition_delay(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_transition_prop_1", (PyCFunction) pylv_obj_get_style_transition_prop_1, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_transition_prop_1(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_transition_prop_1", (PyCFunction) pylv_obj_set_style_local_transition_prop_1, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_transition_prop_1(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_transition_prop_2", (PyCFunction) pylv_obj_get_style_transition_prop_2, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_transition_prop_2(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_transition_prop_2", (PyCFunction) pylv_obj_set_style_local_transition_prop_2, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_transition_prop_2(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_transition_prop_3", (PyCFunction) pylv_obj_get_style_transition_prop_3, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_transition_prop_3(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_transition_prop_3", (PyCFunction) pylv_obj_set_style_local_transition_prop_3, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_transition_prop_3(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_transition_prop_4", (PyCFunction) pylv_obj_get_style_transition_prop_4, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_transition_prop_4(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_transition_prop_4", (PyCFunction) pylv_obj_set_style_local_transition_prop_4, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_transition_prop_4(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_transition_prop_5", (PyCFunction) pylv_obj_get_style_transition_prop_5, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_transition_prop_5(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_transition_prop_5", (PyCFunction) pylv_obj_set_style_local_transition_prop_5, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_transition_prop_5(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_transition_prop_6", (PyCFunction) pylv_obj_get_style_transition_prop_6, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_transition_prop_6(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_transition_prop_6", (PyCFunction) pylv_obj_set_style_local_transition_prop_6, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_transition_prop_6(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_transition_path", (PyCFunction) pylv_obj_get_style_transition_path, METH_VARARGS | METH_KEYWORDS, "inline static const lv_anim_path_t *lv_obj_get_style_transition_path(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_transition_path", (PyCFunction) pylv_obj_set_style_local_transition_path, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_transition_path(lv_obj_t *obj, uint8_t part, lv_state_t state, const lv_anim_path_t *value)"},
    {"get_style_scale_width", (PyCFunction) pylv_obj_get_style_scale_width, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_scale_width(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_scale_width", (PyCFunction) pylv_obj_set_style_local_scale_width, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_scale_width(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_scale_border_width", (PyCFunction) pylv_obj_get_style_scale_border_width, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_scale_border_width(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_scale_border_width", (PyCFunction) pylv_obj_set_style_local_scale_border_width, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_scale_border_width(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_scale_end_border_width", (PyCFunction) pylv_obj_get_style_scale_end_border_width, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_scale_end_border_width(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_scale_end_border_width", (PyCFunction) pylv_obj_set_style_local_scale_end_border_width, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_scale_end_border_width(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_scale_end_line_width", (PyCFunction) pylv_obj_get_style_scale_end_line_width, METH_VARARGS | METH_KEYWORDS, "inline static lv_style_int_t lv_obj_get_style_scale_end_line_width(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_scale_end_line_width", (PyCFunction) pylv_obj_set_style_local_scale_end_line_width, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_scale_end_line_width(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_style_scale_grad_color", (PyCFunction) pylv_obj_get_style_scale_grad_color, METH_VARARGS | METH_KEYWORDS, "inline static lv_color_t lv_obj_get_style_scale_grad_color(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_scale_grad_color", (PyCFunction) pylv_obj_set_style_local_scale_grad_color, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_scale_grad_color(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_color_t value)"},
    {"get_style_scale_end_color", (PyCFunction) pylv_obj_get_style_scale_end_color, METH_VARARGS | METH_KEYWORDS, "inline static lv_color_t lv_obj_get_style_scale_end_color(const lv_obj_t *obj, uint8_t part)"},
    {"set_style_local_scale_end_color", (PyCFunction) pylv_obj_set_style_local_scale_end_color, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_scale_end_color(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_color_t value)"},
    {"set_style_local_pad_all", (PyCFunction) pylv_obj_set_style_local_pad_all, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_pad_all(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"set_style_local_pad_hor", (PyCFunction) pylv_obj_set_style_local_pad_hor, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_pad_hor(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"set_style_local_pad_ver", (PyCFunction) pylv_obj_set_style_local_pad_ver, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_pad_ver(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"set_style_local_margin_all", (PyCFunction) pylv_obj_set_style_local_margin_all, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_margin_all(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"set_style_local_margin_hor", (PyCFunction) pylv_obj_set_style_local_margin_hor, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_margin_hor(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"set_style_local_margin_ver", (PyCFunction) pylv_obj_set_style_local_margin_ver, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_set_style_local_margin_ver(lv_obj_t *obj, uint8_t part, lv_state_t state, lv_style_int_t value)"},
    {"get_hidden", (PyCFunction) pylv_obj_get_hidden, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_hidden(const lv_obj_t *obj)"},
    {"get_adv_hittest", (PyCFunction) pylv_obj_get_adv_hittest, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_adv_hittest(const lv_obj_t *obj)"},
    {"get_click", (PyCFunction) pylv_obj_get_click, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_click(const lv_obj_t *obj)"},
    {"get_top", (PyCFunction) pylv_obj_get_top, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_top(const lv_obj_t *obj)"},
    {"get_drag", (PyCFunction) pylv_obj_get_drag, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_drag(const lv_obj_t *obj)"},
    {"get_drag_dir", (PyCFunction) pylv_obj_get_drag_dir, METH_VARARGS | METH_KEYWORDS, "lv_drag_dir_t lv_obj_get_drag_dir(const lv_obj_t *obj)"},
    {"get_drag_throw", (PyCFunction) pylv_obj_get_drag_throw, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_drag_throw(const lv_obj_t *obj)"},
    {"get_drag_parent", (PyCFunction) pylv_obj_get_drag_parent, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_drag_parent(const lv_obj_t *obj)"},
    {"get_focus_parent", (PyCFunction) pylv_obj_get_focus_parent, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_focus_parent(const lv_obj_t *obj)"},
    {"get_parent_event", (PyCFunction) pylv_obj_get_parent_event, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_parent_event(const lv_obj_t *obj)"},
    {"get_gesture_parent", (PyCFunction) pylv_obj_get_gesture_parent, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_gesture_parent(const lv_obj_t *obj)"},
    {"get_base_dir", (PyCFunction) pylv_obj_get_base_dir, METH_VARARGS | METH_KEYWORDS, "lv_bidi_dir_t lv_obj_get_base_dir(const lv_obj_t *obj)"},
    {"get_protect", (PyCFunction) pylv_obj_get_protect, METH_VARARGS | METH_KEYWORDS, "uint8_t lv_obj_get_protect(const lv_obj_t *obj)"},
    {"is_protected", (PyCFunction) pylv_obj_is_protected, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_is_protected(const lv_obj_t *obj, uint8_t prot)"},
    {"get_state", (PyCFunction) pylv_obj_get_state, METH_VARARGS | METH_KEYWORDS, "lv_state_t lv_obj_get_state(const lv_obj_t *obj, uint8_t part)"},
    {"get_signal_cb", (PyCFunction) pylv_obj_get_signal_cb, METH_VARARGS | METH_KEYWORDS, "lv_signal_cb_t lv_obj_get_signal_cb(const lv_obj_t *obj)"},
    {"get_design_cb", (PyCFunction) pylv_obj_get_design_cb, METH_VARARGS | METH_KEYWORDS, "lv_design_cb_t lv_obj_get_design_cb(const lv_obj_t *obj)"},
    {"get_event_cb", (PyCFunction) pylv_obj_get_event_cb, METH_VARARGS | METH_KEYWORDS, "lv_event_cb_t lv_obj_get_event_cb(const lv_obj_t *obj)"},
    {"is_point_on_coords", (PyCFunction) pylv_obj_is_point_on_coords, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_is_point_on_coords(lv_obj_t *obj, const lv_point_t *point)"},
    {"hittest", (PyCFunction) pylv_obj_hittest, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_hittest(lv_obj_t *obj, lv_point_t *point)"},
    {"get_type", (PyCFunction) pylv_obj_get_type, METH_VARARGS | METH_KEYWORDS, ""},
    {"get_user_data", (PyCFunction) pylv_obj_get_user_data, METH_VARARGS | METH_KEYWORDS, "lv_obj_user_data_t lv_obj_get_user_data(const lv_obj_t *obj)"},
    {"get_user_data_ptr", (PyCFunction) pylv_obj_get_user_data_ptr, METH_VARARGS | METH_KEYWORDS, "lv_obj_user_data_t *lv_obj_get_user_data_ptr(const lv_obj_t *obj)"},
    {"set_user_data", (PyCFunction) pylv_obj_set_user_data, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_user_data(lv_obj_t *obj, lv_obj_user_data_t data)"},
    {"get_group", (PyCFunction) pylv_obj_get_group, METH_VARARGS | METH_KEYWORDS, "void *lv_obj_get_group(const lv_obj_t *obj)"},
    {"is_focused", (PyCFunction) pylv_obj_is_focused, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_is_focused(const lv_obj_t *obj)"},
    {"get_focused_obj", (PyCFunction) pylv_obj_get_focused_obj, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_obj_get_focused_obj(const lv_obj_t *obj)"},
    {"init_draw_rect_dsc", (PyCFunction) pylv_obj_init_draw_rect_dsc, METH_VARARGS | METH_KEYWORDS, "void lv_obj_init_draw_rect_dsc(lv_obj_t *obj, uint8_t type, lv_draw_rect_dsc_t *draw_dsc)"},
    {"init_draw_label_dsc", (PyCFunction) pylv_obj_init_draw_label_dsc, METH_VARARGS | METH_KEYWORDS, "void lv_obj_init_draw_label_dsc(lv_obj_t *obj, uint8_t type, lv_draw_label_dsc_t *draw_dsc)"},
    {"init_draw_img_dsc", (PyCFunction) pylv_obj_init_draw_img_dsc, METH_VARARGS | METH_KEYWORDS, "void lv_obj_init_draw_img_dsc(lv_obj_t *obj, uint8_t part, lv_draw_img_dsc_t *draw_dsc)"},
    {"init_draw_line_dsc", (PyCFunction) pylv_obj_init_draw_line_dsc, METH_VARARGS | METH_KEYWORDS, "void lv_obj_init_draw_line_dsc(lv_obj_t *obj, uint8_t part, lv_draw_line_dsc_t *draw_dsc)"},
    {"get_draw_rect_ext_pad_size", (PyCFunction) pylv_obj_get_draw_rect_ext_pad_size, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_draw_rect_ext_pad_size(lv_obj_t *obj, uint8_t part)"},
    {"fade_in", (PyCFunction) pylv_obj_fade_in, METH_VARARGS | METH_KEYWORDS, "void lv_obj_fade_in(lv_obj_t *obj, uint32_t time, uint32_t delay)"},
    {"fade_out", (PyCFunction) pylv_obj_fade_out, METH_VARARGS | METH_KEYWORDS, "void lv_obj_fade_out(lv_obj_t *obj, uint32_t time, uint32_t delay)"},
    {"align_origo", (PyCFunction) pylv_obj_align_origo, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_align_origo(lv_obj_t *obj, const lv_obj_t *base, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs)"},
    {"align_origo_x", (PyCFunction) pylv_obj_align_origo_x, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_align_origo_x(lv_obj_t *obj, const lv_obj_t *base, lv_align_t align, lv_coord_t x_ofs)"},
    {"align_origo_y", (PyCFunction) pylv_obj_align_origo_y, METH_VARARGS | METH_KEYWORDS, "inline static void lv_obj_align_origo_y(lv_obj_t *obj, const lv_obj_t *base, lv_align_t align, lv_coord_t y_ofs)"},
    {"get_children", (PyCFunction) pylv_obj_get_children, METH_VARARGS | METH_KEYWORDS, ""},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_obj_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Obj",
    .tp_doc = "lvgl Obj",
    .tp_basicsize = sizeof(pylv_Obj),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_obj_init,
    .tp_dealloc = (destructor) pylv_obj_dealloc,
    .tp_methods = pylv_obj_methods,
    .tp_weaklistoffset = offsetof(pylv_Obj, weakreflist),
};

    
static void
pylv_cont_dealloc(pylv_Cont *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_cont_init(pylv_Cont *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Cont *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_cont_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_cont_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_cont_set_layout(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"layout", NULL};
    unsigned char layout;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &layout)) return NULL;

    LVGL_LOCK         
    lv_cont_set_layout(self->ref, layout);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_cont_set_fit4(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"left", "right", "top", "bottom", NULL};
    unsigned char left;
    unsigned char right;
    unsigned char top;
    unsigned char bottom;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbbb", kwlist , &left, &right, &top, &bottom)) return NULL;

    LVGL_LOCK         
    lv_cont_set_fit4(self->ref, left, right, top, bottom);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_cont_set_fit2(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"hor", "ver", NULL};
    unsigned char hor;
    unsigned char ver;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bb", kwlist , &hor, &ver)) return NULL;

    LVGL_LOCK         
    lv_cont_set_fit2(self->ref, hor, ver);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_cont_set_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"fit", NULL};
    unsigned char fit;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &fit)) return NULL;

    LVGL_LOCK         
    lv_cont_set_fit(self->ref, fit);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_cont_get_layout(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_layout_t result = lv_cont_get_layout(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_cont_get_fit_left(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_fit_t result = lv_cont_get_fit_left(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_cont_get_fit_right(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_fit_t result = lv_cont_get_fit_right(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_cont_get_fit_top(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_fit_t result = lv_cont_get_fit_top(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_cont_get_fit_bottom(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_fit_t result = lv_cont_get_fit_bottom(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}


static PyMethodDef pylv_cont_methods[] = {
    {"set_layout", (PyCFunction) pylv_cont_set_layout, METH_VARARGS | METH_KEYWORDS, "void lv_cont_set_layout(lv_obj_t *cont, lv_layout_t layout)"},
    {"set_fit4", (PyCFunction) pylv_cont_set_fit4, METH_VARARGS | METH_KEYWORDS, "void lv_cont_set_fit4(lv_obj_t *cont, lv_fit_t left, lv_fit_t right, lv_fit_t top, lv_fit_t bottom)"},
    {"set_fit2", (PyCFunction) pylv_cont_set_fit2, METH_VARARGS | METH_KEYWORDS, "inline static void lv_cont_set_fit2(lv_obj_t *cont, lv_fit_t hor, lv_fit_t ver)"},
    {"set_fit", (PyCFunction) pylv_cont_set_fit, METH_VARARGS | METH_KEYWORDS, "inline static void lv_cont_set_fit(lv_obj_t *cont, lv_fit_t fit)"},
    {"get_layout", (PyCFunction) pylv_cont_get_layout, METH_VARARGS | METH_KEYWORDS, "lv_layout_t lv_cont_get_layout(const lv_obj_t *cont)"},
    {"get_fit_left", (PyCFunction) pylv_cont_get_fit_left, METH_VARARGS | METH_KEYWORDS, "lv_fit_t lv_cont_get_fit_left(const lv_obj_t *cont)"},
    {"get_fit_right", (PyCFunction) pylv_cont_get_fit_right, METH_VARARGS | METH_KEYWORDS, "lv_fit_t lv_cont_get_fit_right(const lv_obj_t *cont)"},
    {"get_fit_top", (PyCFunction) pylv_cont_get_fit_top, METH_VARARGS | METH_KEYWORDS, "lv_fit_t lv_cont_get_fit_top(const lv_obj_t *cont)"},
    {"get_fit_bottom", (PyCFunction) pylv_cont_get_fit_bottom, METH_VARARGS | METH_KEYWORDS, "lv_fit_t lv_cont_get_fit_bottom(const lv_obj_t *cont)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_cont_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Cont",
    .tp_doc = "lvgl Cont",
    .tp_basicsize = sizeof(pylv_Cont),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_cont_init,
    .tp_dealloc = (destructor) pylv_cont_dealloc,
    .tp_methods = pylv_cont_methods,
    .tp_weaklistoffset = offsetof(pylv_Cont, weakreflist),
};

    
static void
pylv_btn_dealloc(pylv_Btn *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_btn_init(pylv_Btn *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Btn *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_btn_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_btn_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_btn_set_checkable(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"tgl", NULL};
    int tgl;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &tgl)) return NULL;

    LVGL_LOCK         
    lv_btn_set_checkable(self->ref, tgl);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btn_set_state(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"state", NULL};
    unsigned char state;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &state)) return NULL;

    LVGL_LOCK         
    lv_btn_set_state(self->ref, state);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btn_toggle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_btn_toggle(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btn_get_state(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_btn_state_t result = lv_btn_get_state(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_btn_get_checkable(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_btn_get_checkable(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_btn_methods[] = {
    {"set_checkable", (PyCFunction) pylv_btn_set_checkable, METH_VARARGS | METH_KEYWORDS, "void lv_btn_set_checkable(lv_obj_t *btn, bool tgl)"},
    {"set_state", (PyCFunction) pylv_btn_set_state, METH_VARARGS | METH_KEYWORDS, "void lv_btn_set_state(lv_obj_t *btn, lv_btn_state_t state)"},
    {"toggle", (PyCFunction) pylv_btn_toggle, METH_VARARGS | METH_KEYWORDS, "void lv_btn_toggle(lv_obj_t *btn)"},
    {"get_state", (PyCFunction) pylv_btn_get_state, METH_VARARGS | METH_KEYWORDS, "lv_btn_state_t lv_btn_get_state(const lv_obj_t *btn)"},
    {"get_checkable", (PyCFunction) pylv_btn_get_checkable, METH_VARARGS | METH_KEYWORDS, "bool lv_btn_get_checkable(const lv_obj_t *btn)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_btn_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Btn",
    .tp_doc = "lvgl Btn",
    .tp_basicsize = sizeof(pylv_Btn),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_btn_init,
    .tp_dealloc = (destructor) pylv_btn_dealloc,
    .tp_methods = pylv_btn_methods,
    .tp_weaklistoffset = offsetof(pylv_Btn, weakreflist),
};

    
static void
pylv_imgbtn_dealloc(pylv_Imgbtn *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_imgbtn_init(pylv_Imgbtn *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Imgbtn *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_imgbtn_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_imgbtn_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_imgbtn_set_src(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_imgbtn_set_src: Parameter type not found >const void*< ");
    return NULL;
}

static PyObject*
pylv_imgbtn_set_state(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"state", NULL};
    unsigned char state;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &state)) return NULL;

    LVGL_LOCK         
    lv_imgbtn_set_state(self->ref, state);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_imgbtn_toggle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_imgbtn_toggle(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_imgbtn_get_src(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_imgbtn_get_src: Return type not found >const void*< ");
    return NULL;
}


static PyMethodDef pylv_imgbtn_methods[] = {
    {"set_src", (PyCFunction) pylv_imgbtn_set_src, METH_VARARGS | METH_KEYWORDS, "void lv_imgbtn_set_src(lv_obj_t *imgbtn, lv_btn_state_t state, const void *src)"},
    {"set_state", (PyCFunction) pylv_imgbtn_set_state, METH_VARARGS | METH_KEYWORDS, "void lv_imgbtn_set_state(lv_obj_t *imgbtn, lv_btn_state_t state)"},
    {"toggle", (PyCFunction) pylv_imgbtn_toggle, METH_VARARGS | METH_KEYWORDS, "void lv_imgbtn_toggle(lv_obj_t *imgbtn)"},
    {"get_src", (PyCFunction) pylv_imgbtn_get_src, METH_VARARGS | METH_KEYWORDS, "const void *lv_imgbtn_get_src(lv_obj_t *imgbtn, lv_btn_state_t state)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_imgbtn_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Imgbtn",
    .tp_doc = "lvgl Imgbtn",
    .tp_basicsize = sizeof(pylv_Imgbtn),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_imgbtn_init,
    .tp_dealloc = (destructor) pylv_imgbtn_dealloc,
    .tp_methods = pylv_imgbtn_methods,
    .tp_weaklistoffset = offsetof(pylv_Imgbtn, weakreflist),
};

    
static void
pylv_label_dealloc(pylv_Label *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_label_init(pylv_Label *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Label *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_label_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_label_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_label_set_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"text", NULL};
    const char * text;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &text)) return NULL;

    LVGL_LOCK         
    lv_label_set_text(self->ref, text);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_text_fmt(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_label_set_text_fmt: variable arguments not supported");
    return NULL;
}

static PyObject*
pylv_label_set_text_static(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"text", NULL};
    const char * text;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &text)) return NULL;

    LVGL_LOCK         
    lv_label_set_text_static(self->ref, text);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_long_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"long_mode", NULL};
    unsigned char long_mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &long_mode)) return NULL;

    LVGL_LOCK         
    lv_label_set_long_mode(self->ref, long_mode);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"align", NULL};
    unsigned char align;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &align)) return NULL;

    LVGL_LOCK         
    lv_label_set_align(self->ref, align);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_label_set_recolor(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_anim_speed(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"anim_speed", NULL};
    unsigned short int anim_speed;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_speed)) return NULL;

    LVGL_LOCK         
    lv_label_set_anim_speed(self->ref, anim_speed);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_text_sel_start(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"index", NULL};
    unsigned int index;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "I", kwlist , &index)) return NULL;

    LVGL_LOCK         
    lv_label_set_text_sel_start(self->ref, index);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_text_sel_end(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"index", NULL};
    unsigned int index;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "I", kwlist , &index)) return NULL;

    LVGL_LOCK         
    lv_label_set_text_sel_end(self->ref, index);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_get_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    char* result = lv_label_get_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_label_get_long_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_label_long_mode_t result = lv_label_get_long_mode(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_label_get_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_label_align_t result = lv_label_get_align(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_label_get_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_label_get_recolor(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_label_get_anim_speed(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_label_get_anim_speed(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_label_is_char_under_pos(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_label_is_char_under_pos: Parameter type not found >lv_point_t*< ");
    return NULL;
}

static PyObject*
pylv_label_get_text_sel_start(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint32_t result = lv_label_get_text_sel_start(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("I", result);
}

static PyObject*
pylv_label_get_text_sel_end(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint32_t result = lv_label_get_text_sel_end(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("I", result);
}

static PyObject*
pylv_label_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_label_get_style: Return type not found >lv_style_list_t*< ");
    return NULL;
}

static PyObject*
pylv_label_ins_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"pos", "txt", NULL};
    unsigned int pos;
    const char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Is", kwlist , &pos, &txt)) return NULL;

    LVGL_LOCK         
    lv_label_ins_text(self->ref, pos, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_cut_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"pos", "cnt", NULL};
    unsigned int pos;
    unsigned int cnt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "II", kwlist , &pos, &cnt)) return NULL;

    LVGL_LOCK         
    lv_label_cut_text(self->ref, pos, cnt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_refr_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_label_refr_text(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}


static PyMethodDef pylv_label_methods[] = {
    {"set_text", (PyCFunction) pylv_label_set_text, METH_VARARGS | METH_KEYWORDS, "void lv_label_set_text(lv_obj_t *label, const char *text)"},
    {"set_text_fmt", (PyCFunction) pylv_label_set_text_fmt, METH_VARARGS | METH_KEYWORDS, "void lv_label_set_text_fmt(lv_obj_t *label, const char *fmt, ...)"},
    {"set_text_static", (PyCFunction) pylv_label_set_text_static, METH_VARARGS | METH_KEYWORDS, "void lv_label_set_text_static(lv_obj_t *label, const char *text)"},
    {"set_long_mode", (PyCFunction) pylv_label_set_long_mode, METH_VARARGS | METH_KEYWORDS, "void lv_label_set_long_mode(lv_obj_t *label, lv_label_long_mode_t long_mode)"},
    {"set_align", (PyCFunction) pylv_label_set_align, METH_VARARGS | METH_KEYWORDS, "void lv_label_set_align(lv_obj_t *label, lv_label_align_t align)"},
    {"set_recolor", (PyCFunction) pylv_label_set_recolor, METH_VARARGS | METH_KEYWORDS, "void lv_label_set_recolor(lv_obj_t *label, bool en)"},
    {"set_anim_speed", (PyCFunction) pylv_label_set_anim_speed, METH_VARARGS | METH_KEYWORDS, "void lv_label_set_anim_speed(lv_obj_t *label, uint16_t anim_speed)"},
    {"set_text_sel_start", (PyCFunction) pylv_label_set_text_sel_start, METH_VARARGS | METH_KEYWORDS, "void lv_label_set_text_sel_start(lv_obj_t *label, uint32_t index)"},
    {"set_text_sel_end", (PyCFunction) pylv_label_set_text_sel_end, METH_VARARGS | METH_KEYWORDS, "void lv_label_set_text_sel_end(lv_obj_t *label, uint32_t index)"},
    {"get_text", (PyCFunction) pylv_label_get_text, METH_VARARGS | METH_KEYWORDS, "char *lv_label_get_text(const lv_obj_t *label)"},
    {"get_long_mode", (PyCFunction) pylv_label_get_long_mode, METH_VARARGS | METH_KEYWORDS, "lv_label_long_mode_t lv_label_get_long_mode(const lv_obj_t *label)"},
    {"get_align", (PyCFunction) pylv_label_get_align, METH_VARARGS | METH_KEYWORDS, "lv_label_align_t lv_label_get_align(const lv_obj_t *label)"},
    {"get_recolor", (PyCFunction) pylv_label_get_recolor, METH_VARARGS | METH_KEYWORDS, "bool lv_label_get_recolor(const lv_obj_t *label)"},
    {"get_anim_speed", (PyCFunction) pylv_label_get_anim_speed, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_label_get_anim_speed(const lv_obj_t *label)"},
    {"get_letter_pos", (PyCFunction) pylv_label_get_letter_pos, METH_VARARGS | METH_KEYWORDS, ""},
    {"get_letter_on", (PyCFunction) pylv_label_get_letter_on, METH_VARARGS | METH_KEYWORDS, ""},
    {"is_char_under_pos", (PyCFunction) pylv_label_is_char_under_pos, METH_VARARGS | METH_KEYWORDS, "bool lv_label_is_char_under_pos(const lv_obj_t *label, lv_point_t *pos)"},
    {"get_text_sel_start", (PyCFunction) pylv_label_get_text_sel_start, METH_VARARGS | METH_KEYWORDS, "uint32_t lv_label_get_text_sel_start(const lv_obj_t *label)"},
    {"get_text_sel_end", (PyCFunction) pylv_label_get_text_sel_end, METH_VARARGS | METH_KEYWORDS, "uint32_t lv_label_get_text_sel_end(const lv_obj_t *label)"},
    {"get_style", (PyCFunction) pylv_label_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_list_t *lv_label_get_style(lv_obj_t *label, uint8_t type)"},
    {"ins_text", (PyCFunction) pylv_label_ins_text, METH_VARARGS | METH_KEYWORDS, "void lv_label_ins_text(lv_obj_t *label, uint32_t pos, const char *txt)"},
    {"cut_text", (PyCFunction) pylv_label_cut_text, METH_VARARGS | METH_KEYWORDS, "void lv_label_cut_text(lv_obj_t *label, uint32_t pos, uint32_t cnt)"},
    {"refr_text", (PyCFunction) pylv_label_refr_text, METH_VARARGS | METH_KEYWORDS, "void lv_label_refr_text(lv_obj_t *label)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_label_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Label",
    .tp_doc = "lvgl Label",
    .tp_basicsize = sizeof(pylv_Label),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_label_init,
    .tp_dealloc = (destructor) pylv_label_dealloc,
    .tp_methods = pylv_label_methods,
    .tp_weaklistoffset = offsetof(pylv_Label, weakreflist),
};

    
static void
pylv_img_dealloc(pylv_Img *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_img_init(pylv_Img *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Img *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_img_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_img_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_img_set_src(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_img_set_src: Parameter type not found >const void*< ");
    return NULL;
}

static PyObject*
pylv_img_set_auto_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"autosize_en", NULL};
    int autosize_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &autosize_en)) return NULL;

    LVGL_LOCK         
    lv_img_set_auto_size(self->ref, autosize_en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_img_set_offset_x(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"x", NULL};
    short int x;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &x)) return NULL;

    LVGL_LOCK         
    lv_img_set_offset_x(self->ref, x);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_img_set_offset_y(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"y", NULL};
    short int y;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &y)) return NULL;

    LVGL_LOCK         
    lv_img_set_offset_y(self->ref, y);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_img_set_pivot(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"pivot_x", "pivot_y", NULL};
    short int pivot_x;
    short int pivot_y;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &pivot_x, &pivot_y)) return NULL;

    LVGL_LOCK         
    lv_img_set_pivot(self->ref, pivot_x, pivot_y);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_img_set_angle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"angle", NULL};
    short int angle;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &angle)) return NULL;

    LVGL_LOCK         
    lv_img_set_angle(self->ref, angle);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_img_set_zoom(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"zoom", NULL};
    unsigned short int zoom;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &zoom)) return NULL;

    LVGL_LOCK         
    lv_img_set_zoom(self->ref, zoom);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_img_set_antialias(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"antialias", NULL};
    int antialias;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &antialias)) return NULL;

    LVGL_LOCK         
    lv_img_set_antialias(self->ref, antialias);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_img_get_src(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_img_get_src: Return type not found >const void*< ");
    return NULL;
}

static PyObject*
pylv_img_get_file_name(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_img_get_file_name(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_img_get_auto_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_img_get_auto_size(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_img_get_offset_x(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_img_get_offset_x(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_img_get_offset_y(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_img_get_offset_y(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_img_get_angle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_img_get_angle(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_img_get_pivot(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_img_get_pivot: Parameter type not found >lv_point_t*< ");
    return NULL;
}

static PyObject*
pylv_img_get_zoom(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_img_get_zoom(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_img_get_antialias(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_img_get_antialias(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_img_methods[] = {
    {"set_src", (PyCFunction) pylv_img_set_src, METH_VARARGS | METH_KEYWORDS, "void lv_img_set_src(lv_obj_t *img, const void *src_img)"},
    {"set_auto_size", (PyCFunction) pylv_img_set_auto_size, METH_VARARGS | METH_KEYWORDS, "void lv_img_set_auto_size(lv_obj_t *img, bool autosize_en)"},
    {"set_offset_x", (PyCFunction) pylv_img_set_offset_x, METH_VARARGS | METH_KEYWORDS, "void lv_img_set_offset_x(lv_obj_t *img, lv_coord_t x)"},
    {"set_offset_y", (PyCFunction) pylv_img_set_offset_y, METH_VARARGS | METH_KEYWORDS, "void lv_img_set_offset_y(lv_obj_t *img, lv_coord_t y)"},
    {"set_pivot", (PyCFunction) pylv_img_set_pivot, METH_VARARGS | METH_KEYWORDS, "void lv_img_set_pivot(lv_obj_t *img, lv_coord_t pivot_x, lv_coord_t pivot_y)"},
    {"set_angle", (PyCFunction) pylv_img_set_angle, METH_VARARGS | METH_KEYWORDS, "void lv_img_set_angle(lv_obj_t *img, int16_t angle)"},
    {"set_zoom", (PyCFunction) pylv_img_set_zoom, METH_VARARGS | METH_KEYWORDS, "void lv_img_set_zoom(lv_obj_t *img, uint16_t zoom)"},
    {"set_antialias", (PyCFunction) pylv_img_set_antialias, METH_VARARGS | METH_KEYWORDS, "void lv_img_set_antialias(lv_obj_t *img, bool antialias)"},
    {"get_src", (PyCFunction) pylv_img_get_src, METH_VARARGS | METH_KEYWORDS, "const void *lv_img_get_src(lv_obj_t *img)"},
    {"get_file_name", (PyCFunction) pylv_img_get_file_name, METH_VARARGS | METH_KEYWORDS, "const char *lv_img_get_file_name(const lv_obj_t *img)"},
    {"get_auto_size", (PyCFunction) pylv_img_get_auto_size, METH_VARARGS | METH_KEYWORDS, "bool lv_img_get_auto_size(const lv_obj_t *img)"},
    {"get_offset_x", (PyCFunction) pylv_img_get_offset_x, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_img_get_offset_x(lv_obj_t *img)"},
    {"get_offset_y", (PyCFunction) pylv_img_get_offset_y, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_img_get_offset_y(lv_obj_t *img)"},
    {"get_angle", (PyCFunction) pylv_img_get_angle, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_img_get_angle(lv_obj_t *img)"},
    {"get_pivot", (PyCFunction) pylv_img_get_pivot, METH_VARARGS | METH_KEYWORDS, "void lv_img_get_pivot(lv_obj_t *img, lv_point_t *center)"},
    {"get_zoom", (PyCFunction) pylv_img_get_zoom, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_img_get_zoom(lv_obj_t *img)"},
    {"get_antialias", (PyCFunction) pylv_img_get_antialias, METH_VARARGS | METH_KEYWORDS, "bool lv_img_get_antialias(lv_obj_t *img)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_img_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Img",
    .tp_doc = "lvgl Img",
    .tp_basicsize = sizeof(pylv_Img),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_img_init,
    .tp_dealloc = (destructor) pylv_img_dealloc,
    .tp_methods = pylv_img_methods,
    .tp_weaklistoffset = offsetof(pylv_Img, weakreflist),
};

    
static void
pylv_line_dealloc(pylv_Line *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_line_init(pylv_Line *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Line *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_line_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_line_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_line_set_points(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_line_set_points: Parameter type not found >lv_point_t< ");
    return NULL;
}

static PyObject*
pylv_line_set_auto_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_line_set_auto_size(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_line_set_y_invert(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_line_set_y_invert(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_line_get_auto_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_line_get_auto_size(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_line_get_y_invert(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_line_get_y_invert(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_line_methods[] = {
    {"set_points", (PyCFunction) pylv_line_set_points, METH_VARARGS | METH_KEYWORDS, "void lv_line_set_points(lv_obj_t *line, const lv_point_t point_a[], uint16_t point_num)"},
    {"set_auto_size", (PyCFunction) pylv_line_set_auto_size, METH_VARARGS | METH_KEYWORDS, "void lv_line_set_auto_size(lv_obj_t *line, bool en)"},
    {"set_y_invert", (PyCFunction) pylv_line_set_y_invert, METH_VARARGS | METH_KEYWORDS, "void lv_line_set_y_invert(lv_obj_t *line, bool en)"},
    {"get_auto_size", (PyCFunction) pylv_line_get_auto_size, METH_VARARGS | METH_KEYWORDS, "bool lv_line_get_auto_size(const lv_obj_t *line)"},
    {"get_y_invert", (PyCFunction) pylv_line_get_y_invert, METH_VARARGS | METH_KEYWORDS, "bool lv_line_get_y_invert(const lv_obj_t *line)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_line_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Line",
    .tp_doc = "lvgl Line",
    .tp_basicsize = sizeof(pylv_Line),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_line_init,
    .tp_dealloc = (destructor) pylv_line_dealloc,
    .tp_methods = pylv_line_methods,
    .tp_weaklistoffset = offsetof(pylv_Line, weakreflist),
};

    
static void
pylv_page_dealloc(pylv_Page *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_page_init(pylv_Page *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Page *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_page_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_page_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_page_clean(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_page_clean(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_get_scrollable(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_page_get_scrollable(self->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}

static PyObject*
pylv_page_get_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_page_get_anim_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_page_set_scrollbar_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"sb_mode", NULL};
    unsigned char sb_mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &sb_mode)) return NULL;

    LVGL_LOCK         
    lv_page_set_scrollbar_mode(self->ref, sb_mode);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"anim_time", NULL};
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_time)) return NULL;

    LVGL_LOCK         
    lv_page_set_anim_time(self->ref, anim_time);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scroll_propagation(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_page_set_scroll_propagation(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_edge_flash(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_page_set_edge_flash(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scrollable_fit4(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"left", "right", "top", "bottom", NULL};
    unsigned char left;
    unsigned char right;
    unsigned char top;
    unsigned char bottom;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbbb", kwlist , &left, &right, &top, &bottom)) return NULL;

    LVGL_LOCK         
    lv_page_set_scrollable_fit4(self->ref, left, right, top, bottom);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scrollable_fit2(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"hor", "ver", NULL};
    unsigned char hor;
    unsigned char ver;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bb", kwlist , &hor, &ver)) return NULL;

    LVGL_LOCK         
    lv_page_set_scrollable_fit2(self->ref, hor, ver);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scrollable_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"fit", NULL};
    unsigned char fit;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &fit)) return NULL;

    LVGL_LOCK         
    lv_page_set_scrollable_fit(self->ref, fit);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scrl_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"w", NULL};
    short int w;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &w)) return NULL;

    LVGL_LOCK         
    lv_page_set_scrl_width(self->ref, w);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scrl_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"h", NULL};
    short int h;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &h)) return NULL;

    LVGL_LOCK         
    lv_page_set_scrl_height(self->ref, h);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scrl_layout(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"layout", NULL};
    unsigned char layout;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &layout)) return NULL;

    LVGL_LOCK         
    lv_page_set_scrl_layout(self->ref, layout);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_get_scrollbar_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_scrollbar_mode_t result = lv_page_get_scrollbar_mode(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_page_get_scroll_propagation(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_page_get_scroll_propagation(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_page_get_edge_flash(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_page_get_edge_flash(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_page_get_width_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_page_get_width_fit(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_page_get_height_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_page_get_height_fit(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_page_get_width_grid(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"div", "span", NULL};
    unsigned char div;
    unsigned char span;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bb", kwlist , &div, &span)) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_page_get_width_grid(self->ref, div, span);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_page_get_height_grid(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"div", "span", NULL};
    unsigned char div;
    unsigned char span;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bb", kwlist , &div, &span)) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_page_get_height_grid(self->ref, div, span);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_page_get_scrl_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_page_get_scrl_width(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_page_get_scrl_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_page_get_scrl_height(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_page_get_scrl_layout(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_layout_t result = lv_page_get_scrl_layout(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_page_get_scrl_fit_left(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_fit_t result = lv_page_get_scrl_fit_left(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_page_get_scrl_fit_right(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_fit_t result = lv_page_get_scrl_fit_right(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_page_get_scrl_fit_top(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_fit_t result = lv_page_get_scrl_fit_top(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_page_get_scrl_fit_bottom(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_fit_t result = lv_page_get_scrl_fit_bottom(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_page_on_edge(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"edge", NULL};
    unsigned char edge;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &edge)) return NULL;

    LVGL_LOCK        
    bool result = lv_page_on_edge(self->ref, edge);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_page_glue_obj(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"glue", NULL};
    int glue;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &glue)) return NULL;

    LVGL_LOCK         
    lv_page_glue_obj(self->ref, glue);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_focus(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"obj", "anim_en", NULL};
    pylv_Obj * obj;
    unsigned char anim_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!b", kwlist , &pylv_obj_Type, &obj, &anim_en)) return NULL;

    LVGL_LOCK         
    lv_page_focus(self->ref, obj->ref, anim_en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_scroll_hor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"dist", NULL};
    short int dist;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &dist)) return NULL;

    LVGL_LOCK         
    lv_page_scroll_hor(self->ref, dist);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_scroll_ver(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"dist", NULL};
    short int dist;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &dist)) return NULL;

    LVGL_LOCK         
    lv_page_scroll_ver(self->ref, dist);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_start_edge_flash(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"edge", NULL};
    unsigned char edge;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &edge)) return NULL;

    LVGL_LOCK         
    lv_page_start_edge_flash(self->ref, edge);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scrlbar_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"sb_mode", NULL};
    unsigned char sb_mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &sb_mode)) return NULL;

    LVGL_LOCK         
    lv_page_set_scrlbar_mode(self->ref, sb_mode);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_get_scrlbar_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_scrollbar_mode_t result = lv_page_get_scrlbar_mode(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_page_get_scrl(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_page_get_scrl(self->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}


static PyMethodDef pylv_page_methods[] = {
    {"clean", (PyCFunction) pylv_page_clean, METH_VARARGS | METH_KEYWORDS, "void lv_page_clean(lv_obj_t *page)"},
    {"get_scrollable", (PyCFunction) pylv_page_get_scrollable, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_page_get_scrollable(const lv_obj_t *page)"},
    {"get_anim_time", (PyCFunction) pylv_page_get_anim_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_page_get_anim_time(const lv_obj_t *page)"},
    {"set_scrollbar_mode", (PyCFunction) pylv_page_set_scrollbar_mode, METH_VARARGS | METH_KEYWORDS, "void lv_page_set_scrollbar_mode(lv_obj_t *page, lv_scrollbar_mode_t sb_mode)"},
    {"set_anim_time", (PyCFunction) pylv_page_set_anim_time, METH_VARARGS | METH_KEYWORDS, "void lv_page_set_anim_time(lv_obj_t *page, uint16_t anim_time)"},
    {"set_scroll_propagation", (PyCFunction) pylv_page_set_scroll_propagation, METH_VARARGS | METH_KEYWORDS, "void lv_page_set_scroll_propagation(lv_obj_t *page, bool en)"},
    {"set_edge_flash", (PyCFunction) pylv_page_set_edge_flash, METH_VARARGS | METH_KEYWORDS, "void lv_page_set_edge_flash(lv_obj_t *page, bool en)"},
    {"set_scrollable_fit4", (PyCFunction) pylv_page_set_scrollable_fit4, METH_VARARGS | METH_KEYWORDS, "inline static void lv_page_set_scrollable_fit4(lv_obj_t *page, lv_fit_t left, lv_fit_t right, lv_fit_t top, lv_fit_t bottom)"},
    {"set_scrollable_fit2", (PyCFunction) pylv_page_set_scrollable_fit2, METH_VARARGS | METH_KEYWORDS, "inline static void lv_page_set_scrollable_fit2(lv_obj_t *page, lv_fit_t hor, lv_fit_t ver)"},
    {"set_scrollable_fit", (PyCFunction) pylv_page_set_scrollable_fit, METH_VARARGS | METH_KEYWORDS, "inline static void lv_page_set_scrollable_fit(lv_obj_t *page, lv_fit_t fit)"},
    {"set_scrl_width", (PyCFunction) pylv_page_set_scrl_width, METH_VARARGS | METH_KEYWORDS, "inline static void lv_page_set_scrl_width(lv_obj_t *page, lv_coord_t w)"},
    {"set_scrl_height", (PyCFunction) pylv_page_set_scrl_height, METH_VARARGS | METH_KEYWORDS, "inline static void lv_page_set_scrl_height(lv_obj_t *page, lv_coord_t h)"},
    {"set_scrl_layout", (PyCFunction) pylv_page_set_scrl_layout, METH_VARARGS | METH_KEYWORDS, "inline static void lv_page_set_scrl_layout(lv_obj_t *page, lv_layout_t layout)"},
    {"get_scrollbar_mode", (PyCFunction) pylv_page_get_scrollbar_mode, METH_VARARGS | METH_KEYWORDS, "lv_scrollbar_mode_t lv_page_get_scrollbar_mode(const lv_obj_t *page)"},
    {"get_scroll_propagation", (PyCFunction) pylv_page_get_scroll_propagation, METH_VARARGS | METH_KEYWORDS, "bool lv_page_get_scroll_propagation(lv_obj_t *page)"},
    {"get_edge_flash", (PyCFunction) pylv_page_get_edge_flash, METH_VARARGS | METH_KEYWORDS, "bool lv_page_get_edge_flash(lv_obj_t *page)"},
    {"get_width_fit", (PyCFunction) pylv_page_get_width_fit, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_page_get_width_fit(lv_obj_t *page)"},
    {"get_height_fit", (PyCFunction) pylv_page_get_height_fit, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_page_get_height_fit(lv_obj_t *page)"},
    {"get_width_grid", (PyCFunction) pylv_page_get_width_grid, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_page_get_width_grid(lv_obj_t *page, uint8_t div, uint8_t span)"},
    {"get_height_grid", (PyCFunction) pylv_page_get_height_grid, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_page_get_height_grid(lv_obj_t *page, uint8_t div, uint8_t span)"},
    {"get_scrl_width", (PyCFunction) pylv_page_get_scrl_width, METH_VARARGS | METH_KEYWORDS, "inline static lv_coord_t lv_page_get_scrl_width(const lv_obj_t *page)"},
    {"get_scrl_height", (PyCFunction) pylv_page_get_scrl_height, METH_VARARGS | METH_KEYWORDS, "inline static lv_coord_t lv_page_get_scrl_height(const lv_obj_t *page)"},
    {"get_scrl_layout", (PyCFunction) pylv_page_get_scrl_layout, METH_VARARGS | METH_KEYWORDS, "inline static lv_layout_t lv_page_get_scrl_layout(const lv_obj_t *page)"},
    {"get_scrl_fit_left", (PyCFunction) pylv_page_get_scrl_fit_left, METH_VARARGS | METH_KEYWORDS, "inline static lv_fit_t lv_page_get_scrl_fit_left(const lv_obj_t *page)"},
    {"get_scrl_fit_right", (PyCFunction) pylv_page_get_scrl_fit_right, METH_VARARGS | METH_KEYWORDS, "inline static lv_fit_t lv_page_get_scrl_fit_right(const lv_obj_t *page)"},
    {"get_scrl_fit_top", (PyCFunction) pylv_page_get_scrl_fit_top, METH_VARARGS | METH_KEYWORDS, "inline static lv_fit_t lv_page_get_scrl_fit_top(const lv_obj_t *page)"},
    {"get_scrl_fit_bottom", (PyCFunction) pylv_page_get_scrl_fit_bottom, METH_VARARGS | METH_KEYWORDS, "inline static lv_fit_t lv_page_get_scrl_fit_bottom(const lv_obj_t *page)"},
    {"on_edge", (PyCFunction) pylv_page_on_edge, METH_VARARGS | METH_KEYWORDS, "bool lv_page_on_edge(lv_obj_t *page, lv_page_edge_t edge)"},
    {"glue_obj", (PyCFunction) pylv_page_glue_obj, METH_VARARGS | METH_KEYWORDS, "void lv_page_glue_obj(lv_obj_t *obj, bool glue)"},
    {"focus", (PyCFunction) pylv_page_focus, METH_VARARGS | METH_KEYWORDS, "void lv_page_focus(lv_obj_t *page, const lv_obj_t *obj, lv_anim_enable_t anim_en)"},
    {"scroll_hor", (PyCFunction) pylv_page_scroll_hor, METH_VARARGS | METH_KEYWORDS, "void lv_page_scroll_hor(lv_obj_t *page, lv_coord_t dist)"},
    {"scroll_ver", (PyCFunction) pylv_page_scroll_ver, METH_VARARGS | METH_KEYWORDS, "void lv_page_scroll_ver(lv_obj_t *page, lv_coord_t dist)"},
    {"start_edge_flash", (PyCFunction) pylv_page_start_edge_flash, METH_VARARGS | METH_KEYWORDS, "void lv_page_start_edge_flash(lv_obj_t *page, lv_page_edge_t edge)"},
    {"set_scrlbar_mode", (PyCFunction) pylv_page_set_scrlbar_mode, METH_VARARGS | METH_KEYWORDS, "inline static void lv_page_set_scrlbar_mode(lv_obj_t *page, lv_scrollbar_mode_t sb_mode)"},
    {"get_scrlbar_mode", (PyCFunction) pylv_page_get_scrlbar_mode, METH_VARARGS | METH_KEYWORDS, "inline static lv_scrollbar_mode_t lv_page_get_scrlbar_mode(lv_obj_t *page)"},
    {"get_scrl", (PyCFunction) pylv_page_get_scrl, METH_VARARGS | METH_KEYWORDS, "inline static lv_obj_t *lv_page_get_scrl(lv_obj_t *page)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_page_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Page",
    .tp_doc = "lvgl Page",
    .tp_basicsize = sizeof(pylv_Page),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_page_init,
    .tp_dealloc = (destructor) pylv_page_dealloc,
    .tp_methods = pylv_page_methods,
    .tp_weaklistoffset = offsetof(pylv_Page, weakreflist),
};

    
static void
pylv_list_dealloc(pylv_List *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_list_init(pylv_List *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_List *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_list_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_list_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_list_clean(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_list_clean(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_list_add_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_list_add_btn: Parameter type not found >const void*< ");
    return NULL;
}

static PyObject*
pylv_list_remove(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"index", NULL};
    unsigned short int index;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &index)) return NULL;

    LVGL_LOCK        
    bool result = lv_list_remove(self->ref, index);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_list_focus_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"btn", NULL};
    pylv_Obj * btn;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &pylv_obj_Type, &btn)) return NULL;

    LVGL_LOCK         
    lv_list_focus_btn(self->ref, btn->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_list_set_layout(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"layout", NULL};
    unsigned char layout;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &layout)) return NULL;

    LVGL_LOCK         
    lv_list_set_layout(self->ref, layout);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_list_get_btn_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_list_get_btn_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_list_get_btn_label(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_list_get_btn_label(self->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}

static PyObject*
pylv_list_get_btn_img(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_list_get_btn_img(self->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}

static PyObject*
pylv_list_get_prev_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"prev_btn", NULL};
    pylv_Obj * prev_btn;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &pylv_obj_Type, &prev_btn)) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_list_get_prev_btn(self->ref, prev_btn->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}

static PyObject*
pylv_list_get_next_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"prev_btn", NULL};
    pylv_Obj * prev_btn;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &pylv_obj_Type, &prev_btn)) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_list_get_next_btn(self->ref, prev_btn->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}

static PyObject*
pylv_list_get_btn_index(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"btn", NULL};
    pylv_Obj * btn;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &pylv_obj_Type, &btn)) return NULL;

    LVGL_LOCK        
    int32_t result = lv_list_get_btn_index(self->ref, btn->ref);
    LVGL_UNLOCK
    return Py_BuildValue("I", result);
}

static PyObject*
pylv_list_get_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_list_get_size(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_list_get_btn_selected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_list_get_btn_selected(self->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}

static PyObject*
pylv_list_get_layout(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_layout_t result = lv_list_get_layout(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_list_up(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_list_up(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_list_down(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_list_down(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}


static PyMethodDef pylv_list_methods[] = {
    {"clean", (PyCFunction) pylv_list_clean, METH_VARARGS | METH_KEYWORDS, "void lv_list_clean(lv_obj_t *list)"},
    {"add_btn", (PyCFunction) pylv_list_add_btn, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_list_add_btn(lv_obj_t *list, const void *img_src, const char *txt)"},
    {"remove", (PyCFunction) pylv_list_remove, METH_VARARGS | METH_KEYWORDS, "bool lv_list_remove(const lv_obj_t *list, uint16_t index)"},
    {"focus_btn", (PyCFunction) pylv_list_focus_btn, METH_VARARGS | METH_KEYWORDS, "void lv_list_focus_btn(lv_obj_t *list, lv_obj_t *btn)"},
    {"set_layout", (PyCFunction) pylv_list_set_layout, METH_VARARGS | METH_KEYWORDS, "void lv_list_set_layout(lv_obj_t *list, lv_layout_t layout)"},
    {"get_btn_text", (PyCFunction) pylv_list_get_btn_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_list_get_btn_text(const lv_obj_t *btn)"},
    {"get_btn_label", (PyCFunction) pylv_list_get_btn_label, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_list_get_btn_label(const lv_obj_t *btn)"},
    {"get_btn_img", (PyCFunction) pylv_list_get_btn_img, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_list_get_btn_img(const lv_obj_t *btn)"},
    {"get_prev_btn", (PyCFunction) pylv_list_get_prev_btn, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_list_get_prev_btn(const lv_obj_t *list, lv_obj_t *prev_btn)"},
    {"get_next_btn", (PyCFunction) pylv_list_get_next_btn, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_list_get_next_btn(const lv_obj_t *list, lv_obj_t *prev_btn)"},
    {"get_btn_index", (PyCFunction) pylv_list_get_btn_index, METH_VARARGS | METH_KEYWORDS, "int32_t lv_list_get_btn_index(const lv_obj_t *list, const lv_obj_t *btn)"},
    {"get_size", (PyCFunction) pylv_list_get_size, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_list_get_size(const lv_obj_t *list)"},
    {"get_btn_selected", (PyCFunction) pylv_list_get_btn_selected, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_list_get_btn_selected(const lv_obj_t *list)"},
    {"get_layout", (PyCFunction) pylv_list_get_layout, METH_VARARGS | METH_KEYWORDS, "lv_layout_t lv_list_get_layout(lv_obj_t *list)"},
    {"up", (PyCFunction) pylv_list_up, METH_VARARGS | METH_KEYWORDS, "void lv_list_up(const lv_obj_t *list)"},
    {"down", (PyCFunction) pylv_list_down, METH_VARARGS | METH_KEYWORDS, "void lv_list_down(const lv_obj_t *list)"},
    {"focus", (PyCFunction) pylv_list_focus, METH_VARARGS | METH_KEYWORDS, ""},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_list_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.List",
    .tp_doc = "lvgl List",
    .tp_basicsize = sizeof(pylv_List),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_list_init,
    .tp_dealloc = (destructor) pylv_list_dealloc,
    .tp_methods = pylv_list_methods,
    .tp_weaklistoffset = offsetof(pylv_List, weakreflist),
};

    
static void
pylv_chart_dealloc(pylv_Chart *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_chart_init(pylv_Chart *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Chart *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_chart_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_chart_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_chart_add_series(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_add_series: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_chart_remove_series(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_remove_series: Parameter type not found >lv_chart_series_t*< ");
    return NULL;
}

static PyObject*
pylv_chart_add_cursor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_add_cursor: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_chart_clear_series(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_clear_series: Parameter type not found >lv_chart_series_t*< ");
    return NULL;
}

static PyObject*
pylv_chart_hide_series(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_hide_series: Parameter type not found >lv_chart_series_t*< ");
    return NULL;
}

static PyObject*
pylv_chart_set_div_line_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"hdiv", "vdiv", NULL};
    unsigned char hdiv;
    unsigned char vdiv;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bb", kwlist , &hdiv, &vdiv)) return NULL;

    LVGL_LOCK         
    lv_chart_set_div_line_count(self->ref, hdiv, vdiv);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_y_range(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"axis", "ymin", "ymax", NULL};
    unsigned char axis;
    short int ymin;
    short int ymax;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bhh", kwlist , &axis, &ymin, &ymax)) return NULL;

    LVGL_LOCK         
    lv_chart_set_y_range(self->ref, axis, ymin, ymax);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK         
    lv_chart_set_type(self->ref, type);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_point_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"point_cnt", NULL};
    unsigned short int point_cnt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &point_cnt)) return NULL;

    LVGL_LOCK         
    lv_chart_set_point_count(self->ref, point_cnt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_init_points(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_init_points: Parameter type not found >lv_chart_series_t*< ");
    return NULL;
}

static PyObject*
pylv_chart_set_points(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_set_points: Parameter type not found >lv_chart_series_t*< ");
    return NULL;
}

static PyObject*
pylv_chart_set_next(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_set_next: Parameter type not found >lv_chart_series_t*< ");
    return NULL;
}

static PyObject*
pylv_chart_set_update_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"update_mode", NULL};
    unsigned char update_mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &update_mode)) return NULL;

    LVGL_LOCK         
    lv_chart_set_update_mode(self->ref, update_mode);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_x_tick_length(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"major_tick_len", "minor_tick_len", NULL};
    unsigned char major_tick_len;
    unsigned char minor_tick_len;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bb", kwlist , &major_tick_len, &minor_tick_len)) return NULL;

    LVGL_LOCK         
    lv_chart_set_x_tick_length(self->ref, major_tick_len, minor_tick_len);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_y_tick_length(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"major_tick_len", "minor_tick_len", NULL};
    unsigned char major_tick_len;
    unsigned char minor_tick_len;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bb", kwlist , &major_tick_len, &minor_tick_len)) return NULL;

    LVGL_LOCK         
    lv_chart_set_y_tick_length(self->ref, major_tick_len, minor_tick_len);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_secondary_y_tick_length(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"major_tick_len", "minor_tick_len", NULL};
    unsigned char major_tick_len;
    unsigned char minor_tick_len;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bb", kwlist , &major_tick_len, &minor_tick_len)) return NULL;

    LVGL_LOCK         
    lv_chart_set_secondary_y_tick_length(self->ref, major_tick_len, minor_tick_len);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_x_tick_texts(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"list_of_values", "num_tick_marks", "options", NULL};
    const char * list_of_values;
    unsigned char num_tick_marks;
    unsigned char options;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "sbb", kwlist , &list_of_values, &num_tick_marks, &options)) return NULL;

    LVGL_LOCK         
    lv_chart_set_x_tick_texts(self->ref, list_of_values, num_tick_marks, options);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_secondary_y_tick_texts(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"list_of_values", "num_tick_marks", "options", NULL};
    const char * list_of_values;
    unsigned char num_tick_marks;
    unsigned char options;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "sbb", kwlist , &list_of_values, &num_tick_marks, &options)) return NULL;

    LVGL_LOCK         
    lv_chart_set_secondary_y_tick_texts(self->ref, list_of_values, num_tick_marks, options);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_y_tick_texts(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"list_of_values", "num_tick_marks", "options", NULL};
    const char * list_of_values;
    unsigned char num_tick_marks;
    unsigned char options;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "sbb", kwlist , &list_of_values, &num_tick_marks, &options)) return NULL;

    LVGL_LOCK         
    lv_chart_set_y_tick_texts(self->ref, list_of_values, num_tick_marks, options);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_x_start_point(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_set_x_start_point: Parameter type not found >lv_chart_series_t*< ");
    return NULL;
}

static PyObject*
pylv_chart_set_ext_array(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_set_ext_array: Parameter type not found >lv_chart_series_t*< ");
    return NULL;
}

static PyObject*
pylv_chart_set_point_id(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_set_point_id: Parameter type not found >lv_chart_series_t*< ");
    return NULL;
}

static PyObject*
pylv_chart_set_series_axis(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_set_series_axis: Parameter type not found >lv_chart_series_t*< ");
    return NULL;
}

static PyObject*
pylv_chart_set_cursor_point(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_set_cursor_point: Parameter type not found >lv_chart_cursor_t*< ");
    return NULL;
}

static PyObject*
pylv_chart_get_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_chart_type_t result = lv_chart_get_type(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_chart_get_point_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_chart_get_point_count(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_chart_get_point_id(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_get_point_id: Parameter type not found >lv_chart_series_t*< ");
    return NULL;
}

static PyObject*
pylv_chart_get_series_axis(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_get_series_axis: Parameter type not found >lv_chart_series_t*< ");
    return NULL;
}

static PyObject*
pylv_chart_get_series_area(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_get_series_area: Parameter type not found >lv_area_t*< ");
    return NULL;
}

static PyObject*
pylv_chart_get_cursor_point(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_get_cursor_point: Parameter type not found >lv_chart_cursor_t*< ");
    return NULL;
}

static PyObject*
pylv_chart_get_nearest_index_from_coord(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"x", NULL};
    short int x;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &x)) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_chart_get_nearest_index_from_coord(self->ref, x);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_chart_get_x_from_index(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_get_x_from_index: Parameter type not found >lv_chart_series_t*< ");
    return NULL;
}

static PyObject*
pylv_chart_get_y_from_index(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_get_y_from_index: Parameter type not found >lv_chart_series_t*< ");
    return NULL;
}

static PyObject*
pylv_chart_refresh(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_chart_refresh(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_range(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"ymin", "ymax", NULL};
    short int ymin;
    short int ymax;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &ymin, &ymax)) return NULL;

    LVGL_LOCK         
    lv_chart_set_range(self->ref, ymin, ymax);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_clear_serie(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_clear_serie: Parameter type not found >lv_chart_series_t*< ");
    return NULL;
}


static PyMethodDef pylv_chart_methods[] = {
    {"add_series", (PyCFunction) pylv_chart_add_series, METH_VARARGS | METH_KEYWORDS, "lv_chart_series_t *lv_chart_add_series(lv_obj_t *chart, lv_color_t color)"},
    {"remove_series", (PyCFunction) pylv_chart_remove_series, METH_VARARGS | METH_KEYWORDS, "void lv_chart_remove_series(lv_obj_t *chart, lv_chart_series_t *series)"},
    {"add_cursor", (PyCFunction) pylv_chart_add_cursor, METH_VARARGS | METH_KEYWORDS, "lv_chart_cursor_t *lv_chart_add_cursor(lv_obj_t *chart, lv_color_t color, lv_cursor_direction_t dir)"},
    {"clear_series", (PyCFunction) pylv_chart_clear_series, METH_VARARGS | METH_KEYWORDS, "void lv_chart_clear_series(lv_obj_t *chart, lv_chart_series_t *series)"},
    {"hide_series", (PyCFunction) pylv_chart_hide_series, METH_VARARGS | METH_KEYWORDS, "void lv_chart_hide_series(lv_obj_t *chart, lv_chart_series_t *series, bool hide)"},
    {"set_div_line_count", (PyCFunction) pylv_chart_set_div_line_count, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_div_line_count(lv_obj_t *chart, uint8_t hdiv, uint8_t vdiv)"},
    {"set_y_range", (PyCFunction) pylv_chart_set_y_range, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_y_range(lv_obj_t *chart, lv_chart_axis_t axis, lv_coord_t ymin, lv_coord_t ymax)"},
    {"set_type", (PyCFunction) pylv_chart_set_type, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_type(lv_obj_t *chart, lv_chart_type_t type)"},
    {"set_point_count", (PyCFunction) pylv_chart_set_point_count, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_point_count(lv_obj_t *chart, uint16_t point_cnt)"},
    {"init_points", (PyCFunction) pylv_chart_init_points, METH_VARARGS | METH_KEYWORDS, "void lv_chart_init_points(lv_obj_t *chart, lv_chart_series_t *ser, lv_coord_t y)"},
    {"set_points", (PyCFunction) pylv_chart_set_points, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_points(lv_obj_t *chart, lv_chart_series_t *ser, lv_coord_t y_array[])"},
    {"set_next", (PyCFunction) pylv_chart_set_next, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_next(lv_obj_t *chart, lv_chart_series_t *ser, lv_coord_t y)"},
    {"set_update_mode", (PyCFunction) pylv_chart_set_update_mode, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_update_mode(lv_obj_t *chart, lv_chart_update_mode_t update_mode)"},
    {"set_x_tick_length", (PyCFunction) pylv_chart_set_x_tick_length, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_x_tick_length(lv_obj_t *chart, uint8_t major_tick_len, uint8_t minor_tick_len)"},
    {"set_y_tick_length", (PyCFunction) pylv_chart_set_y_tick_length, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_y_tick_length(lv_obj_t *chart, uint8_t major_tick_len, uint8_t minor_tick_len)"},
    {"set_secondary_y_tick_length", (PyCFunction) pylv_chart_set_secondary_y_tick_length, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_secondary_y_tick_length(lv_obj_t *chart, uint8_t major_tick_len, uint8_t minor_tick_len)"},
    {"set_x_tick_texts", (PyCFunction) pylv_chart_set_x_tick_texts, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_x_tick_texts(lv_obj_t *chart, const char *list_of_values, uint8_t num_tick_marks, lv_chart_axis_options_t options)"},
    {"set_secondary_y_tick_texts", (PyCFunction) pylv_chart_set_secondary_y_tick_texts, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_secondary_y_tick_texts(lv_obj_t *chart, const char *list_of_values, uint8_t num_tick_marks, lv_chart_axis_options_t options)"},
    {"set_y_tick_texts", (PyCFunction) pylv_chart_set_y_tick_texts, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_y_tick_texts(lv_obj_t *chart, const char *list_of_values, uint8_t num_tick_marks, lv_chart_axis_options_t options)"},
    {"set_x_start_point", (PyCFunction) pylv_chart_set_x_start_point, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_x_start_point(lv_obj_t *chart, lv_chart_series_t *ser, uint16_t id)"},
    {"set_ext_array", (PyCFunction) pylv_chart_set_ext_array, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_ext_array(lv_obj_t *chart, lv_chart_series_t *ser, lv_coord_t array[], uint16_t point_cnt)"},
    {"set_point_id", (PyCFunction) pylv_chart_set_point_id, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_point_id(lv_obj_t *chart, lv_chart_series_t *ser, lv_coord_t value, uint16_t id)"},
    {"set_series_axis", (PyCFunction) pylv_chart_set_series_axis, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_series_axis(lv_obj_t *chart, lv_chart_series_t *ser, lv_chart_axis_t axis)"},
    {"set_cursor_point", (PyCFunction) pylv_chart_set_cursor_point, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_cursor_point(lv_obj_t *chart, lv_chart_cursor_t *cursor, lv_point_t *point)"},
    {"get_type", (PyCFunction) pylv_chart_get_type, METH_VARARGS | METH_KEYWORDS, "lv_chart_type_t lv_chart_get_type(const lv_obj_t *chart)"},
    {"get_point_count", (PyCFunction) pylv_chart_get_point_count, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_chart_get_point_count(const lv_obj_t *chart)"},
    {"get_point_id", (PyCFunction) pylv_chart_get_point_id, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_chart_get_point_id(lv_obj_t *chart, lv_chart_series_t *ser, uint16_t id)"},
    {"get_series_axis", (PyCFunction) pylv_chart_get_series_axis, METH_VARARGS | METH_KEYWORDS, "lv_chart_axis_t lv_chart_get_series_axis(lv_obj_t *chart, lv_chart_series_t *ser)"},
    {"get_series_area", (PyCFunction) pylv_chart_get_series_area, METH_VARARGS | METH_KEYWORDS, "void lv_chart_get_series_area(lv_obj_t *chart, lv_area_t *series_area)"},
    {"get_cursor_point", (PyCFunction) pylv_chart_get_cursor_point, METH_VARARGS | METH_KEYWORDS, "lv_point_t lv_chart_get_cursor_point(lv_obj_t *chart, lv_chart_cursor_t *cursor)"},
    {"get_nearest_index_from_coord", (PyCFunction) pylv_chart_get_nearest_index_from_coord, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_chart_get_nearest_index_from_coord(lv_obj_t *chart, lv_coord_t x)"},
    {"get_x_from_index", (PyCFunction) pylv_chart_get_x_from_index, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_chart_get_x_from_index(lv_obj_t *chart, lv_chart_series_t *ser, uint16_t id)"},
    {"get_y_from_index", (PyCFunction) pylv_chart_get_y_from_index, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_chart_get_y_from_index(lv_obj_t *chart, lv_chart_series_t *ser, uint16_t id)"},
    {"refresh", (PyCFunction) pylv_chart_refresh, METH_VARARGS | METH_KEYWORDS, "void lv_chart_refresh(lv_obj_t *chart)"},
    {"set_range", (PyCFunction) pylv_chart_set_range, METH_VARARGS | METH_KEYWORDS, "inline static void lv_chart_set_range(lv_obj_t *chart, lv_coord_t ymin, lv_coord_t ymax)"},
    {"clear_serie", (PyCFunction) pylv_chart_clear_serie, METH_VARARGS | METH_KEYWORDS, "inline static void lv_chart_clear_serie(lv_obj_t *chart, lv_chart_series_t *series)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_chart_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Chart",
    .tp_doc = "lvgl Chart",
    .tp_basicsize = sizeof(pylv_Chart),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_chart_init,
    .tp_dealloc = (destructor) pylv_chart_dealloc,
    .tp_methods = pylv_chart_methods,
    .tp_weaklistoffset = offsetof(pylv_Chart, weakreflist),
};

    
static void
pylv_table_dealloc(pylv_Table *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_table_init(pylv_Table *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Table *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_table_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_table_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_table_set_cell_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"row", "col", "txt", NULL};
    unsigned short int row;
    unsigned short int col;
    const char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HHs", kwlist , &row, &col, &txt)) return NULL;

    LVGL_LOCK         
    lv_table_set_cell_value(self->ref, row, col, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_table_set_cell_value_fmt(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_table_set_cell_value_fmt: variable arguments not supported");
    return NULL;
}

static PyObject*
pylv_table_set_row_cnt(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"row_cnt", NULL};
    unsigned short int row_cnt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &row_cnt)) return NULL;

    LVGL_LOCK         
    lv_table_set_row_cnt(self->ref, row_cnt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_table_set_col_cnt(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"col_cnt", NULL};
    unsigned short int col_cnt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &col_cnt)) return NULL;

    LVGL_LOCK         
    lv_table_set_col_cnt(self->ref, col_cnt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_table_set_col_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"col_id", "w", NULL};
    unsigned short int col_id;
    short int w;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hh", kwlist , &col_id, &w)) return NULL;

    LVGL_LOCK         
    lv_table_set_col_width(self->ref, col_id, w);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_table_set_cell_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"row", "col", "align", NULL};
    unsigned short int row;
    unsigned short int col;
    unsigned char align;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HHb", kwlist , &row, &col, &align)) return NULL;

    LVGL_LOCK         
    lv_table_set_cell_align(self->ref, row, col, align);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_table_set_cell_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"row", "col", "type", NULL};
    unsigned short int row;
    unsigned short int col;
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HHb", kwlist , &row, &col, &type)) return NULL;

    LVGL_LOCK         
    lv_table_set_cell_type(self->ref, row, col, type);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_table_set_cell_crop(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"row", "col", "crop", NULL};
    unsigned short int row;
    unsigned short int col;
    int crop;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HHp", kwlist , &row, &col, &crop)) return NULL;

    LVGL_LOCK         
    lv_table_set_cell_crop(self->ref, row, col, crop);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_table_set_cell_merge_right(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"row", "col", "en", NULL};
    unsigned short int row;
    unsigned short int col;
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HHp", kwlist , &row, &col, &en)) return NULL;

    LVGL_LOCK         
    lv_table_set_cell_merge_right(self->ref, row, col, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_table_get_cell_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"row", "col", NULL};
    unsigned short int row;
    unsigned short int col;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HH", kwlist , &row, &col)) return NULL;

    LVGL_LOCK        
    const char* result = lv_table_get_cell_value(self->ref, row, col);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_table_get_row_cnt(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_table_get_row_cnt(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_table_get_col_cnt(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_table_get_col_cnt(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_table_get_col_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"col_id", NULL};
    unsigned short int col_id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &col_id)) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_table_get_col_width(self->ref, col_id);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_table_get_cell_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"row", "col", NULL};
    unsigned short int row;
    unsigned short int col;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HH", kwlist , &row, &col)) return NULL;

    LVGL_LOCK        
    lv_label_align_t result = lv_table_get_cell_align(self->ref, row, col);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_table_get_cell_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"row", "col", NULL};
    unsigned short int row;
    unsigned short int col;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HH", kwlist , &row, &col)) return NULL;

    LVGL_LOCK        
    lv_label_align_t result = lv_table_get_cell_type(self->ref, row, col);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_table_get_cell_crop(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"row", "col", NULL};
    unsigned short int row;
    unsigned short int col;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HH", kwlist , &row, &col)) return NULL;

    LVGL_LOCK        
    lv_label_align_t result = lv_table_get_cell_crop(self->ref, row, col);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_table_get_cell_merge_right(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"row", "col", NULL};
    unsigned short int row;
    unsigned short int col;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HH", kwlist , &row, &col)) return NULL;

    LVGL_LOCK        
    bool result = lv_table_get_cell_merge_right(self->ref, row, col);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_table_get_pressed_cell(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_table_get_pressed_cell: Parameter type not found >uint16_t*< ");
    return NULL;
}


static PyMethodDef pylv_table_methods[] = {
    {"set_cell_value", (PyCFunction) pylv_table_set_cell_value, METH_VARARGS | METH_KEYWORDS, "void lv_table_set_cell_value(lv_obj_t *table, uint16_t row, uint16_t col, const char *txt)"},
    {"set_cell_value_fmt", (PyCFunction) pylv_table_set_cell_value_fmt, METH_VARARGS | METH_KEYWORDS, "void lv_table_set_cell_value_fmt(lv_obj_t *table, uint16_t row, uint16_t col, const char *fmt, ...)"},
    {"set_row_cnt", (PyCFunction) pylv_table_set_row_cnt, METH_VARARGS | METH_KEYWORDS, "void lv_table_set_row_cnt(lv_obj_t *table, uint16_t row_cnt)"},
    {"set_col_cnt", (PyCFunction) pylv_table_set_col_cnt, METH_VARARGS | METH_KEYWORDS, "void lv_table_set_col_cnt(lv_obj_t *table, uint16_t col_cnt)"},
    {"set_col_width", (PyCFunction) pylv_table_set_col_width, METH_VARARGS | METH_KEYWORDS, "void lv_table_set_col_width(lv_obj_t *table, uint16_t col_id, lv_coord_t w)"},
    {"set_cell_align", (PyCFunction) pylv_table_set_cell_align, METH_VARARGS | METH_KEYWORDS, "void lv_table_set_cell_align(lv_obj_t *table, uint16_t row, uint16_t col, lv_label_align_t align)"},
    {"set_cell_type", (PyCFunction) pylv_table_set_cell_type, METH_VARARGS | METH_KEYWORDS, "void lv_table_set_cell_type(lv_obj_t *table, uint16_t row, uint16_t col, uint8_t type)"},
    {"set_cell_crop", (PyCFunction) pylv_table_set_cell_crop, METH_VARARGS | METH_KEYWORDS, "void lv_table_set_cell_crop(lv_obj_t *table, uint16_t row, uint16_t col, bool crop)"},
    {"set_cell_merge_right", (PyCFunction) pylv_table_set_cell_merge_right, METH_VARARGS | METH_KEYWORDS, "void lv_table_set_cell_merge_right(lv_obj_t *table, uint16_t row, uint16_t col, bool en)"},
    {"get_cell_value", (PyCFunction) pylv_table_get_cell_value, METH_VARARGS | METH_KEYWORDS, "const char *lv_table_get_cell_value(lv_obj_t *table, uint16_t row, uint16_t col)"},
    {"get_row_cnt", (PyCFunction) pylv_table_get_row_cnt, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_table_get_row_cnt(lv_obj_t *table)"},
    {"get_col_cnt", (PyCFunction) pylv_table_get_col_cnt, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_table_get_col_cnt(lv_obj_t *table)"},
    {"get_col_width", (PyCFunction) pylv_table_get_col_width, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_table_get_col_width(lv_obj_t *table, uint16_t col_id)"},
    {"get_cell_align", (PyCFunction) pylv_table_get_cell_align, METH_VARARGS | METH_KEYWORDS, "lv_label_align_t lv_table_get_cell_align(lv_obj_t *table, uint16_t row, uint16_t col)"},
    {"get_cell_type", (PyCFunction) pylv_table_get_cell_type, METH_VARARGS | METH_KEYWORDS, "lv_label_align_t lv_table_get_cell_type(lv_obj_t *table, uint16_t row, uint16_t col)"},
    {"get_cell_crop", (PyCFunction) pylv_table_get_cell_crop, METH_VARARGS | METH_KEYWORDS, "lv_label_align_t lv_table_get_cell_crop(lv_obj_t *table, uint16_t row, uint16_t col)"},
    {"get_cell_merge_right", (PyCFunction) pylv_table_get_cell_merge_right, METH_VARARGS | METH_KEYWORDS, "bool lv_table_get_cell_merge_right(lv_obj_t *table, uint16_t row, uint16_t col)"},
    {"get_pressed_cell", (PyCFunction) pylv_table_get_pressed_cell, METH_VARARGS | METH_KEYWORDS, "lv_res_t lv_table_get_pressed_cell(lv_obj_t *table, uint16_t *row, uint16_t *col)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_table_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Table",
    .tp_doc = "lvgl Table",
    .tp_basicsize = sizeof(pylv_Table),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_table_init,
    .tp_dealloc = (destructor) pylv_table_dealloc,
    .tp_methods = pylv_table_methods,
    .tp_weaklistoffset = offsetof(pylv_Table, weakreflist),
};

    
static void
pylv_checkbox_dealloc(pylv_Checkbox *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_checkbox_init(pylv_Checkbox *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Checkbox *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_checkbox_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_checkbox_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_checkbox_set_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"txt", NULL};
    const char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    LVGL_LOCK         
    lv_checkbox_set_text(self->ref, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_checkbox_set_text_static(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"txt", NULL};
    const char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    LVGL_LOCK         
    lv_checkbox_set_text_static(self->ref, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_checkbox_set_checked(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"checked", NULL};
    int checked;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &checked)) return NULL;

    LVGL_LOCK         
    lv_checkbox_set_checked(self->ref, checked);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_checkbox_set_disabled(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_checkbox_set_disabled(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_checkbox_set_state(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"state", NULL};
    unsigned char state;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &state)) return NULL;

    LVGL_LOCK         
    lv_checkbox_set_state(self->ref, state);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_checkbox_get_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_checkbox_get_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_checkbox_is_checked(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_checkbox_is_checked(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_checkbox_is_inactive(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_checkbox_is_inactive(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_checkbox_methods[] = {
    {"set_text", (PyCFunction) pylv_checkbox_set_text, METH_VARARGS | METH_KEYWORDS, "void lv_checkbox_set_text(lv_obj_t *cb, const char *txt)"},
    {"set_text_static", (PyCFunction) pylv_checkbox_set_text_static, METH_VARARGS | METH_KEYWORDS, "void lv_checkbox_set_text_static(lv_obj_t *cb, const char *txt)"},
    {"set_checked", (PyCFunction) pylv_checkbox_set_checked, METH_VARARGS | METH_KEYWORDS, "void lv_checkbox_set_checked(lv_obj_t *cb, bool checked)"},
    {"set_disabled", (PyCFunction) pylv_checkbox_set_disabled, METH_VARARGS | METH_KEYWORDS, "void lv_checkbox_set_disabled(lv_obj_t *cb)"},
    {"set_state", (PyCFunction) pylv_checkbox_set_state, METH_VARARGS | METH_KEYWORDS, "void lv_checkbox_set_state(lv_obj_t *cb, lv_btn_state_t state)"},
    {"get_text", (PyCFunction) pylv_checkbox_get_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_checkbox_get_text(const lv_obj_t *cb)"},
    {"is_checked", (PyCFunction) pylv_checkbox_is_checked, METH_VARARGS | METH_KEYWORDS, "inline static bool lv_checkbox_is_checked(const lv_obj_t *cb)"},
    {"is_inactive", (PyCFunction) pylv_checkbox_is_inactive, METH_VARARGS | METH_KEYWORDS, "inline static bool lv_checkbox_is_inactive(const lv_obj_t *cb)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_checkbox_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Checkbox",
    .tp_doc = "lvgl Checkbox",
    .tp_basicsize = sizeof(pylv_Checkbox),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_checkbox_init,
    .tp_dealloc = (destructor) pylv_checkbox_dealloc,
    .tp_methods = pylv_checkbox_methods,
    .tp_weaklistoffset = offsetof(pylv_Checkbox, weakreflist),
};

    
static void
pylv_cpicker_dealloc(pylv_Cpicker *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_cpicker_init(pylv_Cpicker *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Cpicker *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_cpicker_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_cpicker_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_cpicker_set_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK         
    lv_cpicker_set_type(self->ref, type);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_cpicker_set_hue(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"hue", NULL};
    unsigned short int hue;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &hue)) return NULL;

    LVGL_LOCK        
    bool result = lv_cpicker_set_hue(self->ref, hue);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_cpicker_set_saturation(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"saturation", NULL};
    unsigned char saturation;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &saturation)) return NULL;

    LVGL_LOCK        
    bool result = lv_cpicker_set_saturation(self->ref, saturation);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_cpicker_set_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"val", NULL};
    unsigned char val;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &val)) return NULL;

    LVGL_LOCK        
    bool result = lv_cpicker_set_value(self->ref, val);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_cpicker_set_hsv(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_cpicker_set_hsv: Parameter type not found >lv_color_hsv_t< ");
    return NULL;
}

static PyObject*
pylv_cpicker_set_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_cpicker_set_color: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_cpicker_set_color_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"mode", NULL};
    unsigned char mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &mode)) return NULL;

    LVGL_LOCK         
    lv_cpicker_set_color_mode(self->ref, mode);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_cpicker_set_color_mode_fixed(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"fixed", NULL};
    int fixed;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &fixed)) return NULL;

    LVGL_LOCK         
    lv_cpicker_set_color_mode_fixed(self->ref, fixed);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_cpicker_set_knob_colored(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_cpicker_set_knob_colored(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_cpicker_get_color_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_cpicker_color_mode_t result = lv_cpicker_get_color_mode(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_cpicker_get_color_mode_fixed(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_cpicker_get_color_mode_fixed(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_cpicker_get_hue(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_cpicker_get_hue(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_cpicker_get_saturation(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint8_t result = lv_cpicker_get_saturation(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_cpicker_get_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint8_t result = lv_cpicker_get_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_cpicker_get_hsv(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_cpicker_get_hsv: Return type not found >lv_color_hsv_t< ");
    return NULL;
}

static PyObject*
pylv_cpicker_get_color(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_cpicker_get_color: Return type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_cpicker_get_knob_colored(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_cpicker_get_knob_colored(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_cpicker_methods[] = {
    {"set_type", (PyCFunction) pylv_cpicker_set_type, METH_VARARGS | METH_KEYWORDS, "void lv_cpicker_set_type(lv_obj_t *cpicker, lv_cpicker_type_t type)"},
    {"set_hue", (PyCFunction) pylv_cpicker_set_hue, METH_VARARGS | METH_KEYWORDS, "bool lv_cpicker_set_hue(lv_obj_t *cpicker, uint16_t hue)"},
    {"set_saturation", (PyCFunction) pylv_cpicker_set_saturation, METH_VARARGS | METH_KEYWORDS, "bool lv_cpicker_set_saturation(lv_obj_t *cpicker, uint8_t saturation)"},
    {"set_value", (PyCFunction) pylv_cpicker_set_value, METH_VARARGS | METH_KEYWORDS, "bool lv_cpicker_set_value(lv_obj_t *cpicker, uint8_t val)"},
    {"set_hsv", (PyCFunction) pylv_cpicker_set_hsv, METH_VARARGS | METH_KEYWORDS, "bool lv_cpicker_set_hsv(lv_obj_t *cpicker, lv_color_hsv_t hsv)"},
    {"set_color", (PyCFunction) pylv_cpicker_set_color, METH_VARARGS | METH_KEYWORDS, "bool lv_cpicker_set_color(lv_obj_t *cpicker, lv_color_t color)"},
    {"set_color_mode", (PyCFunction) pylv_cpicker_set_color_mode, METH_VARARGS | METH_KEYWORDS, "void lv_cpicker_set_color_mode(lv_obj_t *cpicker, lv_cpicker_color_mode_t mode)"},
    {"set_color_mode_fixed", (PyCFunction) pylv_cpicker_set_color_mode_fixed, METH_VARARGS | METH_KEYWORDS, "void lv_cpicker_set_color_mode_fixed(lv_obj_t *cpicker, bool fixed)"},
    {"set_knob_colored", (PyCFunction) pylv_cpicker_set_knob_colored, METH_VARARGS | METH_KEYWORDS, "void lv_cpicker_set_knob_colored(lv_obj_t *cpicker, bool en)"},
    {"get_color_mode", (PyCFunction) pylv_cpicker_get_color_mode, METH_VARARGS | METH_KEYWORDS, "lv_cpicker_color_mode_t lv_cpicker_get_color_mode(lv_obj_t *cpicker)"},
    {"get_color_mode_fixed", (PyCFunction) pylv_cpicker_get_color_mode_fixed, METH_VARARGS | METH_KEYWORDS, "bool lv_cpicker_get_color_mode_fixed(lv_obj_t *cpicker)"},
    {"get_hue", (PyCFunction) pylv_cpicker_get_hue, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_cpicker_get_hue(lv_obj_t *cpicker)"},
    {"get_saturation", (PyCFunction) pylv_cpicker_get_saturation, METH_VARARGS | METH_KEYWORDS, "uint8_t lv_cpicker_get_saturation(lv_obj_t *cpicker)"},
    {"get_value", (PyCFunction) pylv_cpicker_get_value, METH_VARARGS | METH_KEYWORDS, "uint8_t lv_cpicker_get_value(lv_obj_t *cpicker)"},
    {"get_hsv", (PyCFunction) pylv_cpicker_get_hsv, METH_VARARGS | METH_KEYWORDS, "lv_color_hsv_t lv_cpicker_get_hsv(lv_obj_t *cpicker)"},
    {"get_color", (PyCFunction) pylv_cpicker_get_color, METH_VARARGS | METH_KEYWORDS, "lv_color_t lv_cpicker_get_color(lv_obj_t *cpicker)"},
    {"get_knob_colored", (PyCFunction) pylv_cpicker_get_knob_colored, METH_VARARGS | METH_KEYWORDS, "bool lv_cpicker_get_knob_colored(lv_obj_t *cpicker)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_cpicker_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Cpicker",
    .tp_doc = "lvgl Cpicker",
    .tp_basicsize = sizeof(pylv_Cpicker),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_cpicker_init,
    .tp_dealloc = (destructor) pylv_cpicker_dealloc,
    .tp_methods = pylv_cpicker_methods,
    .tp_weaklistoffset = offsetof(pylv_Cpicker, weakreflist),
};

    
static void
pylv_bar_dealloc(pylv_Bar *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_bar_init(pylv_Bar *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Bar *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_bar_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_bar_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_bar_set_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"value", "anim", NULL};
    short int value;
    unsigned char anim;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hb", kwlist , &value, &anim)) return NULL;

    LVGL_LOCK         
    lv_bar_set_value(self->ref, value, anim);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_bar_set_start_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"start_value", "anim", NULL};
    short int start_value;
    unsigned char anim;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hb", kwlist , &start_value, &anim)) return NULL;

    LVGL_LOCK         
    lv_bar_set_start_value(self->ref, start_value, anim);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_bar_set_range(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"min", "max", NULL};
    short int min;
    short int max;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &min, &max)) return NULL;

    LVGL_LOCK         
    lv_bar_set_range(self->ref, min, max);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_bar_set_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK         
    lv_bar_set_type(self->ref, type);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_bar_set_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"anim_time", NULL};
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_time)) return NULL;

    LVGL_LOCK         
    lv_bar_set_anim_time(self->ref, anim_time);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_bar_get_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int16_t result = lv_bar_get_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_bar_get_start_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int16_t result = lv_bar_get_start_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_bar_get_min_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int16_t result = lv_bar_get_min_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_bar_get_max_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int16_t result = lv_bar_get_max_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_bar_get_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_bar_type_t result = lv_bar_get_type(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_bar_get_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_bar_get_anim_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_bar_set_sym(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_bar_set_sym(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_bar_get_sym(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_bar_get_sym(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_bar_methods[] = {
    {"set_value", (PyCFunction) pylv_bar_set_value, METH_VARARGS | METH_KEYWORDS, "void lv_bar_set_value(lv_obj_t *bar, int16_t value, lv_anim_enable_t anim)"},
    {"set_start_value", (PyCFunction) pylv_bar_set_start_value, METH_VARARGS | METH_KEYWORDS, "void lv_bar_set_start_value(lv_obj_t *bar, int16_t start_value, lv_anim_enable_t anim)"},
    {"set_range", (PyCFunction) pylv_bar_set_range, METH_VARARGS | METH_KEYWORDS, "void lv_bar_set_range(lv_obj_t *bar, int16_t min, int16_t max)"},
    {"set_type", (PyCFunction) pylv_bar_set_type, METH_VARARGS | METH_KEYWORDS, "void lv_bar_set_type(lv_obj_t *bar, lv_bar_type_t type)"},
    {"set_anim_time", (PyCFunction) pylv_bar_set_anim_time, METH_VARARGS | METH_KEYWORDS, "void lv_bar_set_anim_time(lv_obj_t *bar, uint16_t anim_time)"},
    {"get_value", (PyCFunction) pylv_bar_get_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_bar_get_value(const lv_obj_t *bar)"},
    {"get_start_value", (PyCFunction) pylv_bar_get_start_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_bar_get_start_value(const lv_obj_t *bar)"},
    {"get_min_value", (PyCFunction) pylv_bar_get_min_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_bar_get_min_value(const lv_obj_t *bar)"},
    {"get_max_value", (PyCFunction) pylv_bar_get_max_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_bar_get_max_value(const lv_obj_t *bar)"},
    {"get_type", (PyCFunction) pylv_bar_get_type, METH_VARARGS | METH_KEYWORDS, "lv_bar_type_t lv_bar_get_type(lv_obj_t *bar)"},
    {"get_anim_time", (PyCFunction) pylv_bar_get_anim_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_bar_get_anim_time(const lv_obj_t *bar)"},
    {"set_sym", (PyCFunction) pylv_bar_set_sym, METH_VARARGS | METH_KEYWORDS, "inline static void lv_bar_set_sym(lv_obj_t *bar, bool en)"},
    {"get_sym", (PyCFunction) pylv_bar_get_sym, METH_VARARGS | METH_KEYWORDS, "inline static bool lv_bar_get_sym(lv_obj_t *bar)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_bar_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Bar",
    .tp_doc = "lvgl Bar",
    .tp_basicsize = sizeof(pylv_Bar),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_bar_init,
    .tp_dealloc = (destructor) pylv_bar_dealloc,
    .tp_methods = pylv_bar_methods,
    .tp_weaklistoffset = offsetof(pylv_Bar, weakreflist),
};

    
static void
pylv_slider_dealloc(pylv_Slider *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_slider_init(pylv_Slider *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Slider *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_slider_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_slider_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_slider_set_left_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"left_value", "anim", NULL};
    short int left_value;
    unsigned char anim;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hb", kwlist , &left_value, &anim)) return NULL;

    LVGL_LOCK         
    lv_slider_set_left_value(self->ref, left_value, anim);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_slider_set_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK         
    lv_slider_set_type(self->ref, type);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_slider_get_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int16_t result = lv_slider_get_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_slider_get_left_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int16_t result = lv_slider_get_left_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_slider_is_dragged(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_slider_is_dragged(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_slider_get_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_slider_type_t result = lv_slider_get_type(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}


static PyMethodDef pylv_slider_methods[] = {
    {"set_left_value", (PyCFunction) pylv_slider_set_left_value, METH_VARARGS | METH_KEYWORDS, "inline static void lv_slider_set_left_value(lv_obj_t *slider, int16_t left_value, lv_anim_enable_t anim)"},
    {"set_type", (PyCFunction) pylv_slider_set_type, METH_VARARGS | METH_KEYWORDS, "inline static void lv_slider_set_type(lv_obj_t *slider, lv_slider_type_t type)"},
    {"get_value", (PyCFunction) pylv_slider_get_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_slider_get_value(const lv_obj_t *slider)"},
    {"get_left_value", (PyCFunction) pylv_slider_get_left_value, METH_VARARGS | METH_KEYWORDS, "inline static int16_t lv_slider_get_left_value(const lv_obj_t *slider)"},
    {"is_dragged", (PyCFunction) pylv_slider_is_dragged, METH_VARARGS | METH_KEYWORDS, "bool lv_slider_is_dragged(const lv_obj_t *slider)"},
    {"get_type", (PyCFunction) pylv_slider_get_type, METH_VARARGS | METH_KEYWORDS, "inline static lv_slider_type_t lv_slider_get_type(lv_obj_t *slider)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_slider_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Slider",
    .tp_doc = "lvgl Slider",
    .tp_basicsize = sizeof(pylv_Slider),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_slider_init,
    .tp_dealloc = (destructor) pylv_slider_dealloc,
    .tp_methods = pylv_slider_methods,
    .tp_weaklistoffset = offsetof(pylv_Slider, weakreflist),
};

    
static void
pylv_led_dealloc(pylv_Led *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_led_init(pylv_Led *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Led *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_led_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_led_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_led_set_bright(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"bright", NULL};
    unsigned char bright;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &bright)) return NULL;

    LVGL_LOCK         
    lv_led_set_bright(self->ref, bright);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_led_on(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_led_on(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_led_off(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_led_off(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_led_toggle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_led_toggle(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_led_get_bright(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint8_t result = lv_led_get_bright(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}


static PyMethodDef pylv_led_methods[] = {
    {"set_bright", (PyCFunction) pylv_led_set_bright, METH_VARARGS | METH_KEYWORDS, "void lv_led_set_bright(lv_obj_t *led, uint8_t bright)"},
    {"on", (PyCFunction) pylv_led_on, METH_VARARGS | METH_KEYWORDS, "void lv_led_on(lv_obj_t *led)"},
    {"off", (PyCFunction) pylv_led_off, METH_VARARGS | METH_KEYWORDS, "void lv_led_off(lv_obj_t *led)"},
    {"toggle", (PyCFunction) pylv_led_toggle, METH_VARARGS | METH_KEYWORDS, "void lv_led_toggle(lv_obj_t *led)"},
    {"get_bright", (PyCFunction) pylv_led_get_bright, METH_VARARGS | METH_KEYWORDS, "uint8_t lv_led_get_bright(const lv_obj_t *led)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_led_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Led",
    .tp_doc = "lvgl Led",
    .tp_basicsize = sizeof(pylv_Led),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_led_init,
    .tp_dealloc = (destructor) pylv_led_dealloc,
    .tp_methods = pylv_led_methods,
    .tp_weaklistoffset = offsetof(pylv_Led, weakreflist),
};

    
static void
pylv_btnmatrix_dealloc(pylv_Btnmatrix *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_btnmatrix_init(pylv_Btnmatrix *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Btnmatrix *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_btnmatrix_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_btnmatrix_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_btnmatrix_set_map(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"map", NULL};
    char map;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "c", kwlist , &map)) return NULL;

    LVGL_LOCK         
    lv_btnmatrix_set_map(self->ref, map);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnmatrix_set_ctrl_map(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"ctrl_map", NULL};
    unsigned short int ctrl_map;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &ctrl_map)) return NULL;

    LVGL_LOCK         
    lv_btnmatrix_set_ctrl_map(self->ref, ctrl_map);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnmatrix_set_focused_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"id", NULL};
    unsigned short int id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &id)) return NULL;

    LVGL_LOCK         
    lv_btnmatrix_set_focused_btn(self->ref, id);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnmatrix_set_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_btnmatrix_set_recolor(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnmatrix_set_btn_ctrl(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"btn_id", "ctrl", NULL};
    unsigned short int btn_id;
    unsigned short int ctrl;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HH", kwlist , &btn_id, &ctrl)) return NULL;

    LVGL_LOCK         
    lv_btnmatrix_set_btn_ctrl(self->ref, btn_id, ctrl);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnmatrix_clear_btn_ctrl(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"btn_id", "ctrl", NULL};
    unsigned short int btn_id;
    unsigned short int ctrl;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HH", kwlist , &btn_id, &ctrl)) return NULL;

    LVGL_LOCK         
    lv_btnmatrix_clear_btn_ctrl(self->ref, btn_id, ctrl);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnmatrix_set_btn_ctrl_all(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"ctrl", NULL};
    unsigned short int ctrl;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &ctrl)) return NULL;

    LVGL_LOCK         
    lv_btnmatrix_set_btn_ctrl_all(self->ref, ctrl);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnmatrix_clear_btn_ctrl_all(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"ctrl", NULL};
    unsigned short int ctrl;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &ctrl)) return NULL;

    LVGL_LOCK         
    lv_btnmatrix_clear_btn_ctrl_all(self->ref, ctrl);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnmatrix_set_btn_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"btn_id", "width", NULL};
    unsigned short int btn_id;
    unsigned char width;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hb", kwlist , &btn_id, &width)) return NULL;

    LVGL_LOCK         
    lv_btnmatrix_set_btn_width(self->ref, btn_id, width);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnmatrix_set_one_check(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"one_chk", NULL};
    int one_chk;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &one_chk)) return NULL;

    LVGL_LOCK         
    lv_btnmatrix_set_one_check(self->ref, one_chk);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnmatrix_set_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"align", NULL};
    unsigned char align;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &align)) return NULL;

    LVGL_LOCK         
    lv_btnmatrix_set_align(self->ref, align);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnmatrix_get_map_array(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_btnmatrix_get_map_array: Return type not found >const char**< ");
    return NULL;
}

static PyObject*
pylv_btnmatrix_get_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_btnmatrix_get_recolor(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_btnmatrix_get_active_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_btnmatrix_get_active_btn(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_btnmatrix_get_active_btn_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_btnmatrix_get_active_btn_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_btnmatrix_get_focused_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_btnmatrix_get_focused_btn(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_btnmatrix_get_btn_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"btn_id", NULL};
    unsigned short int btn_id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &btn_id)) return NULL;

    LVGL_LOCK        
    const char* result = lv_btnmatrix_get_btn_text(self->ref, btn_id);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_btnmatrix_get_btn_ctrl(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"btn_id", "ctrl", NULL};
    unsigned short int btn_id;
    unsigned short int ctrl;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HH", kwlist , &btn_id, &ctrl)) return NULL;

    LVGL_LOCK        
    bool result = lv_btnmatrix_get_btn_ctrl(self->ref, btn_id, ctrl);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_btnmatrix_get_one_check(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_btnmatrix_get_one_check(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_btnmatrix_get_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_label_align_t result = lv_btnmatrix_get_align(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}


static PyMethodDef pylv_btnmatrix_methods[] = {
    {"set_map", (PyCFunction) pylv_btnmatrix_set_map, METH_VARARGS | METH_KEYWORDS, "void lv_btnmatrix_set_map(lv_obj_t *btnm, const char *map[])"},
    {"set_ctrl_map", (PyCFunction) pylv_btnmatrix_set_ctrl_map, METH_VARARGS | METH_KEYWORDS, "void lv_btnmatrix_set_ctrl_map(lv_obj_t *btnm, const lv_btnmatrix_ctrl_t ctrl_map[])"},
    {"set_focused_btn", (PyCFunction) pylv_btnmatrix_set_focused_btn, METH_VARARGS | METH_KEYWORDS, "void lv_btnmatrix_set_focused_btn(lv_obj_t *btnm, uint16_t id)"},
    {"set_recolor", (PyCFunction) pylv_btnmatrix_set_recolor, METH_VARARGS | METH_KEYWORDS, "void lv_btnmatrix_set_recolor(const lv_obj_t *btnm, bool en)"},
    {"set_btn_ctrl", (PyCFunction) pylv_btnmatrix_set_btn_ctrl, METH_VARARGS | METH_KEYWORDS, "void lv_btnmatrix_set_btn_ctrl(lv_obj_t *btnm, uint16_t btn_id, lv_btnmatrix_ctrl_t ctrl)"},
    {"clear_btn_ctrl", (PyCFunction) pylv_btnmatrix_clear_btn_ctrl, METH_VARARGS | METH_KEYWORDS, "void lv_btnmatrix_clear_btn_ctrl(const lv_obj_t *btnm, uint16_t btn_id, lv_btnmatrix_ctrl_t ctrl)"},
    {"set_btn_ctrl_all", (PyCFunction) pylv_btnmatrix_set_btn_ctrl_all, METH_VARARGS | METH_KEYWORDS, "void lv_btnmatrix_set_btn_ctrl_all(lv_obj_t *btnm, lv_btnmatrix_ctrl_t ctrl)"},
    {"clear_btn_ctrl_all", (PyCFunction) pylv_btnmatrix_clear_btn_ctrl_all, METH_VARARGS | METH_KEYWORDS, "void lv_btnmatrix_clear_btn_ctrl_all(lv_obj_t *btnm, lv_btnmatrix_ctrl_t ctrl)"},
    {"set_btn_width", (PyCFunction) pylv_btnmatrix_set_btn_width, METH_VARARGS | METH_KEYWORDS, "void lv_btnmatrix_set_btn_width(lv_obj_t *btnm, uint16_t btn_id, uint8_t width)"},
    {"set_one_check", (PyCFunction) pylv_btnmatrix_set_one_check, METH_VARARGS | METH_KEYWORDS, "void lv_btnmatrix_set_one_check(lv_obj_t *btnm, bool one_chk)"},
    {"set_align", (PyCFunction) pylv_btnmatrix_set_align, METH_VARARGS | METH_KEYWORDS, "void lv_btnmatrix_set_align(lv_obj_t *btnm, lv_label_align_t align)"},
    {"get_map_array", (PyCFunction) pylv_btnmatrix_get_map_array, METH_VARARGS | METH_KEYWORDS, "const char **lv_btnmatrix_get_map_array(const lv_obj_t *btnm)"},
    {"get_recolor", (PyCFunction) pylv_btnmatrix_get_recolor, METH_VARARGS | METH_KEYWORDS, "bool lv_btnmatrix_get_recolor(const lv_obj_t *btnm)"},
    {"get_active_btn", (PyCFunction) pylv_btnmatrix_get_active_btn, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_btnmatrix_get_active_btn(const lv_obj_t *btnm)"},
    {"get_active_btn_text", (PyCFunction) pylv_btnmatrix_get_active_btn_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_btnmatrix_get_active_btn_text(const lv_obj_t *btnm)"},
    {"get_focused_btn", (PyCFunction) pylv_btnmatrix_get_focused_btn, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_btnmatrix_get_focused_btn(const lv_obj_t *btnm)"},
    {"get_btn_text", (PyCFunction) pylv_btnmatrix_get_btn_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_btnmatrix_get_btn_text(const lv_obj_t *btnm, uint16_t btn_id)"},
    {"get_btn_ctrl", (PyCFunction) pylv_btnmatrix_get_btn_ctrl, METH_VARARGS | METH_KEYWORDS, "bool lv_btnmatrix_get_btn_ctrl(lv_obj_t *btnm, uint16_t btn_id, lv_btnmatrix_ctrl_t ctrl)"},
    {"get_one_check", (PyCFunction) pylv_btnmatrix_get_one_check, METH_VARARGS | METH_KEYWORDS, "bool lv_btnmatrix_get_one_check(const lv_obj_t *btnm)"},
    {"get_align", (PyCFunction) pylv_btnmatrix_get_align, METH_VARARGS | METH_KEYWORDS, "lv_label_align_t lv_btnmatrix_get_align(const lv_obj_t *btnm)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_btnmatrix_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Btnmatrix",
    .tp_doc = "lvgl Btnmatrix",
    .tp_basicsize = sizeof(pylv_Btnmatrix),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_btnmatrix_init,
    .tp_dealloc = (destructor) pylv_btnmatrix_dealloc,
    .tp_methods = pylv_btnmatrix_methods,
    .tp_weaklistoffset = offsetof(pylv_Btnmatrix, weakreflist),
};

    
static void
pylv_keyboard_dealloc(pylv_Keyboard *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_keyboard_init(pylv_Keyboard *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Keyboard *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_keyboard_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_keyboard_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_keyboard_set_textarea(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"ta", NULL};
    pylv_Obj * ta;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &pylv_obj_Type, &ta)) return NULL;

    LVGL_LOCK         
    lv_keyboard_set_textarea(self->ref, ta->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_keyboard_set_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"mode", NULL};
    unsigned char mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &mode)) return NULL;

    LVGL_LOCK         
    lv_keyboard_set_mode(self->ref, mode);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_keyboard_set_cursor_manage(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_keyboard_set_cursor_manage(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_keyboard_set_map(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"mode", "map", NULL};
    unsigned char mode;
    char map;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bc", kwlist , &mode, &map)) return NULL;

    LVGL_LOCK         
    lv_keyboard_set_map(self->ref, mode, map);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_keyboard_set_ctrl_map(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"mode", "ctrl_map", NULL};
    unsigned char mode;
    unsigned short int ctrl_map;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bH", kwlist , &mode, &ctrl_map)) return NULL;

    LVGL_LOCK         
    lv_keyboard_set_ctrl_map(self->ref, mode, ctrl_map);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_keyboard_get_textarea(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_keyboard_get_textarea(self->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}

static PyObject*
pylv_keyboard_get_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_keyboard_mode_t result = lv_keyboard_get_mode(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_keyboard_get_cursor_manage(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_keyboard_get_cursor_manage(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_keyboard_def_event_cb(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"event", NULL};
    unsigned char event;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &event)) return NULL;

    LVGL_LOCK         
    lv_keyboard_def_event_cb(self->ref, event);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}


static PyMethodDef pylv_keyboard_methods[] = {
    {"set_textarea", (PyCFunction) pylv_keyboard_set_textarea, METH_VARARGS | METH_KEYWORDS, "void lv_keyboard_set_textarea(lv_obj_t *kb, lv_obj_t *ta)"},
    {"set_mode", (PyCFunction) pylv_keyboard_set_mode, METH_VARARGS | METH_KEYWORDS, "void lv_keyboard_set_mode(lv_obj_t *kb, lv_keyboard_mode_t mode)"},
    {"set_cursor_manage", (PyCFunction) pylv_keyboard_set_cursor_manage, METH_VARARGS | METH_KEYWORDS, "void lv_keyboard_set_cursor_manage(lv_obj_t *kb, bool en)"},
    {"set_map", (PyCFunction) pylv_keyboard_set_map, METH_VARARGS | METH_KEYWORDS, "void lv_keyboard_set_map(lv_obj_t *kb, lv_keyboard_mode_t mode, const char *map[])"},
    {"set_ctrl_map", (PyCFunction) pylv_keyboard_set_ctrl_map, METH_VARARGS | METH_KEYWORDS, "void lv_keyboard_set_ctrl_map(lv_obj_t *kb, lv_keyboard_mode_t mode, const lv_btnmatrix_ctrl_t ctrl_map[])"},
    {"get_textarea", (PyCFunction) pylv_keyboard_get_textarea, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_keyboard_get_textarea(const lv_obj_t *kb)"},
    {"get_mode", (PyCFunction) pylv_keyboard_get_mode, METH_VARARGS | METH_KEYWORDS, "lv_keyboard_mode_t lv_keyboard_get_mode(const lv_obj_t *kb)"},
    {"get_cursor_manage", (PyCFunction) pylv_keyboard_get_cursor_manage, METH_VARARGS | METH_KEYWORDS, "bool lv_keyboard_get_cursor_manage(const lv_obj_t *kb)"},
    {"def_event_cb", (PyCFunction) pylv_keyboard_def_event_cb, METH_VARARGS | METH_KEYWORDS, "void lv_keyboard_def_event_cb(lv_obj_t *kb, lv_event_t event)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_keyboard_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Keyboard",
    .tp_doc = "lvgl Keyboard",
    .tp_basicsize = sizeof(pylv_Keyboard),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_keyboard_init,
    .tp_dealloc = (destructor) pylv_keyboard_dealloc,
    .tp_methods = pylv_keyboard_methods,
    .tp_weaklistoffset = offsetof(pylv_Keyboard, weakreflist),
};

    
static void
pylv_dropdown_dealloc(pylv_Dropdown *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_dropdown_init(pylv_Dropdown *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Dropdown *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_dropdown_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_dropdown_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_dropdown_set_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"txt", NULL};
    const char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    LVGL_LOCK         
    lv_dropdown_set_text(self->ref, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_dropdown_clear_options(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_dropdown_clear_options(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_dropdown_set_options(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"options", NULL};
    const char * options;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &options)) return NULL;

    LVGL_LOCK         
    lv_dropdown_set_options(self->ref, options);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_dropdown_set_options_static(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"options", NULL};
    const char * options;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &options)) return NULL;

    LVGL_LOCK         
    lv_dropdown_set_options_static(self->ref, options);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_dropdown_add_option(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"option", "pos", NULL};
    const char * option;
    unsigned int pos;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "sI", kwlist , &option, &pos)) return NULL;

    LVGL_LOCK         
    lv_dropdown_add_option(self->ref, option, pos);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_dropdown_set_selected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"sel_opt", NULL};
    unsigned short int sel_opt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &sel_opt)) return NULL;

    LVGL_LOCK         
    lv_dropdown_set_selected(self->ref, sel_opt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_dropdown_set_dir(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"dir", NULL};
    unsigned char dir;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &dir)) return NULL;

    LVGL_LOCK         
    lv_dropdown_set_dir(self->ref, dir);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_dropdown_set_max_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"h", NULL};
    short int h;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &h)) return NULL;

    LVGL_LOCK         
    lv_dropdown_set_max_height(self->ref, h);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_dropdown_set_symbol(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"symbol", NULL};
    const char * symbol;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &symbol)) return NULL;

    LVGL_LOCK         
    lv_dropdown_set_symbol(self->ref, symbol);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_dropdown_set_show_selected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"show", NULL};
    int show;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &show)) return NULL;

    LVGL_LOCK         
    lv_dropdown_set_show_selected(self->ref, show);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_dropdown_get_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_dropdown_get_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_dropdown_get_options(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_dropdown_get_options(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_dropdown_get_selected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_dropdown_get_selected(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_dropdown_get_option_cnt(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_dropdown_get_option_cnt(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_dropdown_get_selected_str(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_dropdown_get_selected_str: Parameter type not found >char*< ");
    return NULL;
}

static PyObject*
pylv_dropdown_get_max_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_dropdown_get_max_height(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_dropdown_get_symbol(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_dropdown_get_symbol(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_dropdown_get_dir(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_dropdown_dir_t result = lv_dropdown_get_dir(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_dropdown_get_show_selected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_dropdown_get_show_selected(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_dropdown_open(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_dropdown_open(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_dropdown_close(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_dropdown_close(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_dropdown_set_draw_arrow(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_dropdown_set_draw_arrow(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_dropdown_get_draw_arrow(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_dropdown_get_draw_arrow(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_dropdown_methods[] = {
    {"set_text", (PyCFunction) pylv_dropdown_set_text, METH_VARARGS | METH_KEYWORDS, "void lv_dropdown_set_text(lv_obj_t *ddlist, const char *txt)"},
    {"clear_options", (PyCFunction) pylv_dropdown_clear_options, METH_VARARGS | METH_KEYWORDS, "void lv_dropdown_clear_options(lv_obj_t *ddlist)"},
    {"set_options", (PyCFunction) pylv_dropdown_set_options, METH_VARARGS | METH_KEYWORDS, "void lv_dropdown_set_options(lv_obj_t *ddlist, const char *options)"},
    {"set_options_static", (PyCFunction) pylv_dropdown_set_options_static, METH_VARARGS | METH_KEYWORDS, "void lv_dropdown_set_options_static(lv_obj_t *ddlist, const char *options)"},
    {"add_option", (PyCFunction) pylv_dropdown_add_option, METH_VARARGS | METH_KEYWORDS, "void lv_dropdown_add_option(lv_obj_t *ddlist, const char *option, uint32_t pos)"},
    {"set_selected", (PyCFunction) pylv_dropdown_set_selected, METH_VARARGS | METH_KEYWORDS, "void lv_dropdown_set_selected(lv_obj_t *ddlist, uint16_t sel_opt)"},
    {"set_dir", (PyCFunction) pylv_dropdown_set_dir, METH_VARARGS | METH_KEYWORDS, "void lv_dropdown_set_dir(lv_obj_t *ddlist, lv_dropdown_dir_t dir)"},
    {"set_max_height", (PyCFunction) pylv_dropdown_set_max_height, METH_VARARGS | METH_KEYWORDS, "void lv_dropdown_set_max_height(lv_obj_t *ddlist, lv_coord_t h)"},
    {"set_symbol", (PyCFunction) pylv_dropdown_set_symbol, METH_VARARGS | METH_KEYWORDS, "void lv_dropdown_set_symbol(lv_obj_t *ddlist, const char *symbol)"},
    {"set_show_selected", (PyCFunction) pylv_dropdown_set_show_selected, METH_VARARGS | METH_KEYWORDS, "void lv_dropdown_set_show_selected(lv_obj_t *ddlist, bool show)"},
    {"get_text", (PyCFunction) pylv_dropdown_get_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_dropdown_get_text(lv_obj_t *ddlist)"},
    {"get_options", (PyCFunction) pylv_dropdown_get_options, METH_VARARGS | METH_KEYWORDS, "const char *lv_dropdown_get_options(const lv_obj_t *ddlist)"},
    {"get_selected", (PyCFunction) pylv_dropdown_get_selected, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_dropdown_get_selected(const lv_obj_t *ddlist)"},
    {"get_option_cnt", (PyCFunction) pylv_dropdown_get_option_cnt, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_dropdown_get_option_cnt(const lv_obj_t *ddlist)"},
    {"get_selected_str", (PyCFunction) pylv_dropdown_get_selected_str, METH_VARARGS | METH_KEYWORDS, "void lv_dropdown_get_selected_str(const lv_obj_t *ddlist, char *buf, uint32_t buf_size)"},
    {"get_max_height", (PyCFunction) pylv_dropdown_get_max_height, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_dropdown_get_max_height(const lv_obj_t *ddlist)"},
    {"get_symbol", (PyCFunction) pylv_dropdown_get_symbol, METH_VARARGS | METH_KEYWORDS, "const char *lv_dropdown_get_symbol(lv_obj_t *ddlist)"},
    {"get_dir", (PyCFunction) pylv_dropdown_get_dir, METH_VARARGS | METH_KEYWORDS, "lv_dropdown_dir_t lv_dropdown_get_dir(const lv_obj_t *ddlist)"},
    {"get_show_selected", (PyCFunction) pylv_dropdown_get_show_selected, METH_VARARGS | METH_KEYWORDS, "bool lv_dropdown_get_show_selected(lv_obj_t *ddlist)"},
    {"open", (PyCFunction) pylv_dropdown_open, METH_VARARGS | METH_KEYWORDS, "void lv_dropdown_open(lv_obj_t *ddlist)"},
    {"close", (PyCFunction) pylv_dropdown_close, METH_VARARGS | METH_KEYWORDS, "void lv_dropdown_close(lv_obj_t *ddlist)"},
    {"set_draw_arrow", (PyCFunction) pylv_dropdown_set_draw_arrow, METH_VARARGS | METH_KEYWORDS, "inline static void lv_dropdown_set_draw_arrow(lv_obj_t *ddlist, bool en)"},
    {"get_draw_arrow", (PyCFunction) pylv_dropdown_get_draw_arrow, METH_VARARGS | METH_KEYWORDS, "inline static bool lv_dropdown_get_draw_arrow(lv_obj_t *ddlist)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_dropdown_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Dropdown",
    .tp_doc = "lvgl Dropdown",
    .tp_basicsize = sizeof(pylv_Dropdown),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_dropdown_init,
    .tp_dealloc = (destructor) pylv_dropdown_dealloc,
    .tp_methods = pylv_dropdown_methods,
    .tp_weaklistoffset = offsetof(pylv_Dropdown, weakreflist),
};

    
static void
pylv_roller_dealloc(pylv_Roller *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_roller_init(pylv_Roller *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Roller *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_roller_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_roller_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_roller_set_options(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"options", "mode", NULL};
    const char * options;
    unsigned char mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "sb", kwlist , &options, &mode)) return NULL;

    LVGL_LOCK         
    lv_roller_set_options(self->ref, options, mode);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_roller_set_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"align", NULL};
    unsigned char align;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &align)) return NULL;

    LVGL_LOCK         
    lv_roller_set_align(self->ref, align);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_roller_set_selected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"sel_opt", "anim", NULL};
    unsigned short int sel_opt;
    unsigned char anim;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hb", kwlist , &sel_opt, &anim)) return NULL;

    LVGL_LOCK         
    lv_roller_set_selected(self->ref, sel_opt, anim);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_roller_set_visible_row_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"row_cnt", NULL};
    unsigned char row_cnt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &row_cnt)) return NULL;

    LVGL_LOCK         
    lv_roller_set_visible_row_count(self->ref, row_cnt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_roller_set_auto_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"auto_fit", NULL};
    int auto_fit;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &auto_fit)) return NULL;

    LVGL_LOCK         
    lv_roller_set_auto_fit(self->ref, auto_fit);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_roller_get_selected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_roller_get_selected(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_roller_get_option_cnt(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_roller_get_option_cnt(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_roller_get_selected_str(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_roller_get_selected_str: Parameter type not found >char*< ");
    return NULL;
}

static PyObject*
pylv_roller_get_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_label_align_t result = lv_roller_get_align(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_roller_get_auto_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_roller_get_auto_fit(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_roller_get_options(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_roller_get_options(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_roller_set_fix_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"w", NULL};
    short int w;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &w)) return NULL;

    LVGL_LOCK         
    lv_roller_set_fix_width(self->ref, w);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}


static PyMethodDef pylv_roller_methods[] = {
    {"set_options", (PyCFunction) pylv_roller_set_options, METH_VARARGS | METH_KEYWORDS, "void lv_roller_set_options(lv_obj_t *roller, const char *options, lv_roller_mode_t mode)"},
    {"set_align", (PyCFunction) pylv_roller_set_align, METH_VARARGS | METH_KEYWORDS, "void lv_roller_set_align(lv_obj_t *roller, lv_label_align_t align)"},
    {"set_selected", (PyCFunction) pylv_roller_set_selected, METH_VARARGS | METH_KEYWORDS, "void lv_roller_set_selected(lv_obj_t *roller, uint16_t sel_opt, lv_anim_enable_t anim)"},
    {"set_visible_row_count", (PyCFunction) pylv_roller_set_visible_row_count, METH_VARARGS | METH_KEYWORDS, "void lv_roller_set_visible_row_count(lv_obj_t *roller, uint8_t row_cnt)"},
    {"set_auto_fit", (PyCFunction) pylv_roller_set_auto_fit, METH_VARARGS | METH_KEYWORDS, "void lv_roller_set_auto_fit(lv_obj_t *roller, bool auto_fit)"},
    {"get_selected", (PyCFunction) pylv_roller_get_selected, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_roller_get_selected(const lv_obj_t *roller)"},
    {"get_option_cnt", (PyCFunction) pylv_roller_get_option_cnt, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_roller_get_option_cnt(const lv_obj_t *roller)"},
    {"get_selected_str", (PyCFunction) pylv_roller_get_selected_str, METH_VARARGS | METH_KEYWORDS, "void lv_roller_get_selected_str(const lv_obj_t *roller, char *buf, uint32_t buf_size)"},
    {"get_align", (PyCFunction) pylv_roller_get_align, METH_VARARGS | METH_KEYWORDS, "lv_label_align_t lv_roller_get_align(const lv_obj_t *roller)"},
    {"get_auto_fit", (PyCFunction) pylv_roller_get_auto_fit, METH_VARARGS | METH_KEYWORDS, "bool lv_roller_get_auto_fit(lv_obj_t *roller)"},
    {"get_options", (PyCFunction) pylv_roller_get_options, METH_VARARGS | METH_KEYWORDS, "const char *lv_roller_get_options(const lv_obj_t *roller)"},
    {"set_fix_width", (PyCFunction) pylv_roller_set_fix_width, METH_VARARGS | METH_KEYWORDS, "inline static void lv_roller_set_fix_width(lv_obj_t *roller, lv_coord_t w)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_roller_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Roller",
    .tp_doc = "lvgl Roller",
    .tp_basicsize = sizeof(pylv_Roller),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_roller_init,
    .tp_dealloc = (destructor) pylv_roller_dealloc,
    .tp_methods = pylv_roller_methods,
    .tp_weaklistoffset = offsetof(pylv_Roller, weakreflist),
};

    
static void
pylv_textarea_dealloc(pylv_Textarea *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_textarea_init(pylv_Textarea *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Textarea *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_textarea_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_textarea_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_textarea_add_char(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"c", NULL};
    unsigned int c;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "I", kwlist , &c)) return NULL;

    LVGL_LOCK         
    lv_textarea_add_char(self->ref, c);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_add_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"txt", NULL};
    const char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    LVGL_LOCK         
    lv_textarea_add_text(self->ref, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_del_char(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_textarea_del_char(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_del_char_forward(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_textarea_del_char_forward(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_set_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"txt", NULL};
    const char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    LVGL_LOCK         
    lv_textarea_set_text(self->ref, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_set_placeholder_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"txt", NULL};
    const char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    LVGL_LOCK         
    lv_textarea_set_placeholder_text(self->ref, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_set_cursor_pos(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"pos", NULL};
    int pos;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "I", kwlist , &pos)) return NULL;

    LVGL_LOCK         
    lv_textarea_set_cursor_pos(self->ref, pos);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_set_cursor_hidden(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"hide", NULL};
    int hide;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &hide)) return NULL;

    LVGL_LOCK         
    lv_textarea_set_cursor_hidden(self->ref, hide);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_set_cursor_click_pos(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_textarea_set_cursor_click_pos(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_set_pwd_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_textarea_set_pwd_mode(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_set_one_line(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_textarea_set_one_line(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_set_text_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"align", NULL};
    unsigned char align;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &align)) return NULL;

    LVGL_LOCK         
    lv_textarea_set_text_align(self->ref, align);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_set_accepted_chars(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"list", NULL};
    const char * list;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &list)) return NULL;

    LVGL_LOCK         
    lv_textarea_set_accepted_chars(self->ref, list);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_set_max_length(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"num", NULL};
    unsigned int num;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "I", kwlist , &num)) return NULL;

    LVGL_LOCK         
    lv_textarea_set_max_length(self->ref, num);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_set_insert_replace(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"txt", NULL};
    const char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    LVGL_LOCK         
    lv_textarea_set_insert_replace(self->ref, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_set_text_sel(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_textarea_set_text_sel(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_set_pwd_show_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"time", NULL};
    unsigned short int time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &time)) return NULL;

    LVGL_LOCK         
    lv_textarea_set_pwd_show_time(self->ref, time);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_set_cursor_blink_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"time", NULL};
    unsigned short int time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &time)) return NULL;

    LVGL_LOCK         
    lv_textarea_set_cursor_blink_time(self->ref, time);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_get_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_textarea_get_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_textarea_get_placeholder_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_textarea_get_placeholder_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_textarea_get_label(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_textarea_get_label(self->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}

static PyObject*
pylv_textarea_get_cursor_pos(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint32_t result = lv_textarea_get_cursor_pos(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("I", result);
}

static PyObject*
pylv_textarea_get_cursor_hidden(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_textarea_get_cursor_hidden(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_textarea_get_cursor_click_pos(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_textarea_get_cursor_click_pos(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_textarea_get_pwd_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_textarea_get_pwd_mode(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_textarea_get_one_line(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_textarea_get_one_line(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_textarea_get_accepted_chars(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_textarea_get_accepted_chars(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_textarea_get_max_length(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint32_t result = lv_textarea_get_max_length(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("I", result);
}

static PyObject*
pylv_textarea_text_is_selected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_textarea_text_is_selected(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_textarea_get_text_sel_en(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_textarea_get_text_sel_en(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_textarea_get_pwd_show_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_textarea_get_pwd_show_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_textarea_get_cursor_blink_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_textarea_get_cursor_blink_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_textarea_clear_selection(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_textarea_clear_selection(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_cursor_right(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_textarea_cursor_right(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_cursor_left(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_textarea_cursor_left(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_cursor_down(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_textarea_cursor_down(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_textarea_cursor_up(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_textarea_cursor_up(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}


static PyMethodDef pylv_textarea_methods[] = {
    {"add_char", (PyCFunction) pylv_textarea_add_char, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_add_char(lv_obj_t *ta, uint32_t c)"},
    {"add_text", (PyCFunction) pylv_textarea_add_text, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_add_text(lv_obj_t *ta, const char *txt)"},
    {"del_char", (PyCFunction) pylv_textarea_del_char, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_del_char(lv_obj_t *ta)"},
    {"del_char_forward", (PyCFunction) pylv_textarea_del_char_forward, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_del_char_forward(lv_obj_t *ta)"},
    {"set_text", (PyCFunction) pylv_textarea_set_text, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_set_text(lv_obj_t *ta, const char *txt)"},
    {"set_placeholder_text", (PyCFunction) pylv_textarea_set_placeholder_text, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_set_placeholder_text(lv_obj_t *ta, const char *txt)"},
    {"set_cursor_pos", (PyCFunction) pylv_textarea_set_cursor_pos, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_set_cursor_pos(lv_obj_t *ta, int32_t pos)"},
    {"set_cursor_hidden", (PyCFunction) pylv_textarea_set_cursor_hidden, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_set_cursor_hidden(lv_obj_t *ta, bool hide)"},
    {"set_cursor_click_pos", (PyCFunction) pylv_textarea_set_cursor_click_pos, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_set_cursor_click_pos(lv_obj_t *ta, bool en)"},
    {"set_pwd_mode", (PyCFunction) pylv_textarea_set_pwd_mode, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_set_pwd_mode(lv_obj_t *ta, bool en)"},
    {"set_one_line", (PyCFunction) pylv_textarea_set_one_line, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_set_one_line(lv_obj_t *ta, bool en)"},
    {"set_text_align", (PyCFunction) pylv_textarea_set_text_align, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_set_text_align(lv_obj_t *ta, lv_label_align_t align)"},
    {"set_accepted_chars", (PyCFunction) pylv_textarea_set_accepted_chars, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_set_accepted_chars(lv_obj_t *ta, const char *list)"},
    {"set_max_length", (PyCFunction) pylv_textarea_set_max_length, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_set_max_length(lv_obj_t *ta, uint32_t num)"},
    {"set_insert_replace", (PyCFunction) pylv_textarea_set_insert_replace, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_set_insert_replace(lv_obj_t *ta, const char *txt)"},
    {"set_text_sel", (PyCFunction) pylv_textarea_set_text_sel, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_set_text_sel(lv_obj_t *ta, bool en)"},
    {"set_pwd_show_time", (PyCFunction) pylv_textarea_set_pwd_show_time, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_set_pwd_show_time(lv_obj_t *ta, uint16_t time)"},
    {"set_cursor_blink_time", (PyCFunction) pylv_textarea_set_cursor_blink_time, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_set_cursor_blink_time(lv_obj_t *ta, uint16_t time)"},
    {"get_text", (PyCFunction) pylv_textarea_get_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_textarea_get_text(const lv_obj_t *ta)"},
    {"get_placeholder_text", (PyCFunction) pylv_textarea_get_placeholder_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_textarea_get_placeholder_text(lv_obj_t *ta)"},
    {"get_label", (PyCFunction) pylv_textarea_get_label, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_textarea_get_label(const lv_obj_t *ta)"},
    {"get_cursor_pos", (PyCFunction) pylv_textarea_get_cursor_pos, METH_VARARGS | METH_KEYWORDS, "uint32_t lv_textarea_get_cursor_pos(const lv_obj_t *ta)"},
    {"get_cursor_hidden", (PyCFunction) pylv_textarea_get_cursor_hidden, METH_VARARGS | METH_KEYWORDS, "bool lv_textarea_get_cursor_hidden(const lv_obj_t *ta)"},
    {"get_cursor_click_pos", (PyCFunction) pylv_textarea_get_cursor_click_pos, METH_VARARGS | METH_KEYWORDS, "bool lv_textarea_get_cursor_click_pos(lv_obj_t *ta)"},
    {"get_pwd_mode", (PyCFunction) pylv_textarea_get_pwd_mode, METH_VARARGS | METH_KEYWORDS, "bool lv_textarea_get_pwd_mode(const lv_obj_t *ta)"},
    {"get_one_line", (PyCFunction) pylv_textarea_get_one_line, METH_VARARGS | METH_KEYWORDS, "bool lv_textarea_get_one_line(const lv_obj_t *ta)"},
    {"get_accepted_chars", (PyCFunction) pylv_textarea_get_accepted_chars, METH_VARARGS | METH_KEYWORDS, "const char *lv_textarea_get_accepted_chars(lv_obj_t *ta)"},
    {"get_max_length", (PyCFunction) pylv_textarea_get_max_length, METH_VARARGS | METH_KEYWORDS, "uint32_t lv_textarea_get_max_length(lv_obj_t *ta)"},
    {"text_is_selected", (PyCFunction) pylv_textarea_text_is_selected, METH_VARARGS | METH_KEYWORDS, "bool lv_textarea_text_is_selected(const lv_obj_t *ta)"},
    {"get_text_sel_en", (PyCFunction) pylv_textarea_get_text_sel_en, METH_VARARGS | METH_KEYWORDS, "bool lv_textarea_get_text_sel_en(lv_obj_t *ta)"},
    {"get_pwd_show_time", (PyCFunction) pylv_textarea_get_pwd_show_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_textarea_get_pwd_show_time(lv_obj_t *ta)"},
    {"get_cursor_blink_time", (PyCFunction) pylv_textarea_get_cursor_blink_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_textarea_get_cursor_blink_time(lv_obj_t *ta)"},
    {"clear_selection", (PyCFunction) pylv_textarea_clear_selection, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_clear_selection(lv_obj_t *ta)"},
    {"cursor_right", (PyCFunction) pylv_textarea_cursor_right, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_cursor_right(lv_obj_t *ta)"},
    {"cursor_left", (PyCFunction) pylv_textarea_cursor_left, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_cursor_left(lv_obj_t *ta)"},
    {"cursor_down", (PyCFunction) pylv_textarea_cursor_down, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_cursor_down(lv_obj_t *ta)"},
    {"cursor_up", (PyCFunction) pylv_textarea_cursor_up, METH_VARARGS | METH_KEYWORDS, "void lv_textarea_cursor_up(lv_obj_t *ta)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_textarea_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Textarea",
    .tp_doc = "lvgl Textarea",
    .tp_basicsize = sizeof(pylv_Textarea),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_textarea_init,
    .tp_dealloc = (destructor) pylv_textarea_dealloc,
    .tp_methods = pylv_textarea_methods,
    .tp_weaklistoffset = offsetof(pylv_Textarea, weakreflist),
};

    
static void
pylv_canvas_dealloc(pylv_Canvas *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_canvas_init(pylv_Canvas *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Canvas *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_canvas_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_canvas_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_canvas_set_buffer(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_set_buffer: Parameter type not found >void*< ");
    return NULL;
}

static PyObject*
pylv_canvas_set_px(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_set_px: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_canvas_set_palette(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_set_palette: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_canvas_get_px(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_get_px: Return type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_canvas_get_img(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_get_img: Return type not found >lv_img_dsc_t*< ");
    return NULL;
}

static PyObject*
pylv_canvas_copy_buf(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_copy_buf: Parameter type not found >const void*< ");
    return NULL;
}

static PyObject*
pylv_canvas_transform(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_transform: Parameter type not found >lv_img_dsc_t*< ");
    return NULL;
}

static PyObject*
pylv_canvas_blur_hor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_blur_hor: Parameter type not found >const lv_area_t*< ");
    return NULL;
}

static PyObject*
pylv_canvas_blur_ver(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_blur_ver: Parameter type not found >const lv_area_t*< ");
    return NULL;
}

static PyObject*
pylv_canvas_fill_bg(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_fill_bg: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_canvas_draw_rect(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_draw_rect: Parameter type not found >const lv_draw_rect_dsc_t*< ");
    return NULL;
}

static PyObject*
pylv_canvas_draw_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_draw_text: Parameter type not found >lv_draw_label_dsc_t*< ");
    return NULL;
}

static PyObject*
pylv_canvas_draw_img(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_draw_img: Parameter type not found >const void*< ");
    return NULL;
}

static PyObject*
pylv_canvas_draw_line(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_draw_line: Parameter type not found >lv_point_t< ");
    return NULL;
}

static PyObject*
pylv_canvas_draw_polygon(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_draw_polygon: Parameter type not found >lv_point_t< ");
    return NULL;
}

static PyObject*
pylv_canvas_draw_arc(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_draw_arc: Parameter type not found >const lv_draw_line_dsc_t*< ");
    return NULL;
}


static PyMethodDef pylv_canvas_methods[] = {
    {"set_buffer", (PyCFunction) pylv_canvas_set_buffer, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_set_buffer(lv_obj_t *canvas, void *buf, lv_coord_t w, lv_coord_t h, lv_img_cf_t cf)"},
    {"set_px", (PyCFunction) pylv_canvas_set_px, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_set_px(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_color_t c)"},
    {"set_palette", (PyCFunction) pylv_canvas_set_palette, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_set_palette(lv_obj_t *canvas, uint8_t id, lv_color_t c)"},
    {"get_px", (PyCFunction) pylv_canvas_get_px, METH_VARARGS | METH_KEYWORDS, "lv_color_t lv_canvas_get_px(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y)"},
    {"get_img", (PyCFunction) pylv_canvas_get_img, METH_VARARGS | METH_KEYWORDS, "lv_img_dsc_t *lv_canvas_get_img(lv_obj_t *canvas)"},
    {"copy_buf", (PyCFunction) pylv_canvas_copy_buf, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_copy_buf(lv_obj_t *canvas, const void *to_copy, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h)"},
    {"transform", (PyCFunction) pylv_canvas_transform, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_transform(lv_obj_t *canvas, lv_img_dsc_t *img, int16_t angle, uint16_t zoom, lv_coord_t offset_x, lv_coord_t offset_y, int32_t pivot_x, int32_t pivot_y, bool antialias)"},
    {"blur_hor", (PyCFunction) pylv_canvas_blur_hor, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_blur_hor(lv_obj_t *canvas, const lv_area_t *area, uint16_t r)"},
    {"blur_ver", (PyCFunction) pylv_canvas_blur_ver, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_blur_ver(lv_obj_t *canvas, const lv_area_t *area, uint16_t r)"},
    {"fill_bg", (PyCFunction) pylv_canvas_fill_bg, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_fill_bg(lv_obj_t *canvas, lv_color_t color, lv_opa_t opa)"},
    {"draw_rect", (PyCFunction) pylv_canvas_draw_rect, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_draw_rect(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h, const lv_draw_rect_dsc_t *rect_dsc)"},
    {"draw_text", (PyCFunction) pylv_canvas_draw_text, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_draw_text(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_coord_t max_w, lv_draw_label_dsc_t *label_draw_dsc, const char *txt, lv_label_align_t align)"},
    {"draw_img", (PyCFunction) pylv_canvas_draw_img, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_draw_img(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, const void *src, const lv_draw_img_dsc_t *img_draw_dsc)"},
    {"draw_line", (PyCFunction) pylv_canvas_draw_line, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_draw_line(lv_obj_t *canvas, const lv_point_t points[], uint32_t point_cnt, const lv_draw_line_dsc_t *line_draw_dsc)"},
    {"draw_polygon", (PyCFunction) pylv_canvas_draw_polygon, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_draw_polygon(lv_obj_t *canvas, const lv_point_t points[], uint32_t point_cnt, const lv_draw_rect_dsc_t *poly_draw_dsc)"},
    {"draw_arc", (PyCFunction) pylv_canvas_draw_arc, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_draw_arc(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_coord_t r, int32_t start_angle, int32_t end_angle, const lv_draw_line_dsc_t *arc_draw_dsc)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_canvas_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Canvas",
    .tp_doc = "lvgl Canvas",
    .tp_basicsize = sizeof(pylv_Canvas),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_canvas_init,
    .tp_dealloc = (destructor) pylv_canvas_dealloc,
    .tp_methods = pylv_canvas_methods,
    .tp_weaklistoffset = offsetof(pylv_Canvas, weakreflist),
};

    
static void
pylv_win_dealloc(pylv_Win *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_win_init(pylv_Win *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Win *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_win_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_win_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_win_clean(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_win_clean(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_add_btn_right(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_win_add_btn_right: Parameter type not found >const void*< ");
    return NULL;
}

static PyObject*
pylv_win_add_btn_left(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_win_add_btn_left: Parameter type not found >const void*< ");
    return NULL;
}

static PyObject*
pylv_win_close_event_cb(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"event", NULL};
    unsigned char event;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &event)) return NULL;

    LVGL_LOCK         
    lv_win_close_event_cb(self->ref, event);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_title(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"title", NULL};
    const char * title;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &title)) return NULL;

    LVGL_LOCK         
    lv_win_set_title(self->ref, title);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_header_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"size", NULL};
    short int size;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &size)) return NULL;

    LVGL_LOCK         
    lv_win_set_header_height(self->ref, size);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_btn_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"width", NULL};
    short int width;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &width)) return NULL;

    LVGL_LOCK         
    lv_win_set_btn_width(self->ref, width);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_content_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"w", "h", NULL};
    short int w;
    short int h;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &w, &h)) return NULL;

    LVGL_LOCK         
    lv_win_set_content_size(self->ref, w, h);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_layout(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"layout", NULL};
    unsigned char layout;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &layout)) return NULL;

    LVGL_LOCK         
    lv_win_set_layout(self->ref, layout);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_scrollbar_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"sb_mode", NULL};
    unsigned char sb_mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &sb_mode)) return NULL;

    LVGL_LOCK         
    lv_win_set_scrollbar_mode(self->ref, sb_mode);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"anim_time", NULL};
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_time)) return NULL;

    LVGL_LOCK         
    lv_win_set_anim_time(self->ref, anim_time);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_drag(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_win_set_drag(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_title_set_alignment(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"alignment", NULL};
    unsigned char alignment;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &alignment)) return NULL;

    LVGL_LOCK         
    lv_win_title_set_alignment(self->ref, alignment);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_get_title(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_win_get_title(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_win_get_content(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_win_get_content(self->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}

static PyObject*
pylv_win_get_header_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_win_get_header_height(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_win_get_btn_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_win_get_btn_width(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_win_get_from_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_win_get_from_btn(self->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}

static PyObject*
pylv_win_get_layout(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_layout_t result = lv_win_get_layout(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_win_get_sb_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_scrollbar_mode_t result = lv_win_get_sb_mode(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_win_get_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_win_get_anim_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_win_get_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_win_get_width(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_win_title_get_alignment(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint8_t result = lv_win_title_get_alignment(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_win_focus(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"obj", "anim_en", NULL};
    pylv_Obj * obj;
    unsigned char anim_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!b", kwlist , &pylv_obj_Type, &obj, &anim_en)) return NULL;

    LVGL_LOCK         
    lv_win_focus(self->ref, obj->ref, anim_en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_scroll_hor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"dist", NULL};
    short int dist;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &dist)) return NULL;

    LVGL_LOCK         
    lv_win_scroll_hor(self->ref, dist);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_scroll_ver(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"dist", NULL};
    short int dist;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &dist)) return NULL;

    LVGL_LOCK         
    lv_win_scroll_ver(self->ref, dist);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_add_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_win_add_btn: Parameter type not found >const void*< ");
    return NULL;
}


static PyMethodDef pylv_win_methods[] = {
    {"clean", (PyCFunction) pylv_win_clean, METH_VARARGS | METH_KEYWORDS, "void lv_win_clean(lv_obj_t *win)"},
    {"add_btn_right", (PyCFunction) pylv_win_add_btn_right, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_win_add_btn_right(lv_obj_t *win, const void *img_src)"},
    {"add_btn_left", (PyCFunction) pylv_win_add_btn_left, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_win_add_btn_left(lv_obj_t *win, const void *img_src)"},
    {"close_event_cb", (PyCFunction) pylv_win_close_event_cb, METH_VARARGS | METH_KEYWORDS, "void lv_win_close_event_cb(lv_obj_t *btn, lv_event_t event)"},
    {"set_title", (PyCFunction) pylv_win_set_title, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_title(lv_obj_t *win, const char *title)"},
    {"set_header_height", (PyCFunction) pylv_win_set_header_height, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_header_height(lv_obj_t *win, lv_coord_t size)"},
    {"set_btn_width", (PyCFunction) pylv_win_set_btn_width, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_btn_width(lv_obj_t *win, lv_coord_t width)"},
    {"set_content_size", (PyCFunction) pylv_win_set_content_size, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_content_size(lv_obj_t *win, lv_coord_t w, lv_coord_t h)"},
    {"set_layout", (PyCFunction) pylv_win_set_layout, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_layout(lv_obj_t *win, lv_layout_t layout)"},
    {"set_scrollbar_mode", (PyCFunction) pylv_win_set_scrollbar_mode, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_scrollbar_mode(lv_obj_t *win, lv_scrollbar_mode_t sb_mode)"},
    {"set_anim_time", (PyCFunction) pylv_win_set_anim_time, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_anim_time(lv_obj_t *win, uint16_t anim_time)"},
    {"set_drag", (PyCFunction) pylv_win_set_drag, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_drag(lv_obj_t *win, bool en)"},
    {"title_set_alignment", (PyCFunction) pylv_win_title_set_alignment, METH_VARARGS | METH_KEYWORDS, "void lv_win_title_set_alignment(lv_obj_t *win, uint8_t alignment)"},
    {"get_title", (PyCFunction) pylv_win_get_title, METH_VARARGS | METH_KEYWORDS, "const char *lv_win_get_title(const lv_obj_t *win)"},
    {"get_content", (PyCFunction) pylv_win_get_content, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_win_get_content(const lv_obj_t *win)"},
    {"get_header_height", (PyCFunction) pylv_win_get_header_height, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_win_get_header_height(const lv_obj_t *win)"},
    {"get_btn_width", (PyCFunction) pylv_win_get_btn_width, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_win_get_btn_width(lv_obj_t *win)"},
    {"get_from_btn", (PyCFunction) pylv_win_get_from_btn, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_win_get_from_btn(const lv_obj_t *ctrl_btn)"},
    {"get_layout", (PyCFunction) pylv_win_get_layout, METH_VARARGS | METH_KEYWORDS, "lv_layout_t lv_win_get_layout(lv_obj_t *win)"},
    {"get_sb_mode", (PyCFunction) pylv_win_get_sb_mode, METH_VARARGS | METH_KEYWORDS, "lv_scrollbar_mode_t lv_win_get_sb_mode(lv_obj_t *win)"},
    {"get_anim_time", (PyCFunction) pylv_win_get_anim_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_win_get_anim_time(const lv_obj_t *win)"},
    {"get_width", (PyCFunction) pylv_win_get_width, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_win_get_width(lv_obj_t *win)"},
    {"title_get_alignment", (PyCFunction) pylv_win_title_get_alignment, METH_VARARGS | METH_KEYWORDS, "uint8_t lv_win_title_get_alignment(lv_obj_t *win)"},
    {"focus", (PyCFunction) pylv_win_focus, METH_VARARGS | METH_KEYWORDS, "void lv_win_focus(lv_obj_t *win, lv_obj_t *obj, lv_anim_enable_t anim_en)"},
    {"scroll_hor", (PyCFunction) pylv_win_scroll_hor, METH_VARARGS | METH_KEYWORDS, "inline static void lv_win_scroll_hor(lv_obj_t *win, lv_coord_t dist)"},
    {"scroll_ver", (PyCFunction) pylv_win_scroll_ver, METH_VARARGS | METH_KEYWORDS, "inline static void lv_win_scroll_ver(lv_obj_t *win, lv_coord_t dist)"},
    {"add_btn", (PyCFunction) pylv_win_add_btn, METH_VARARGS | METH_KEYWORDS, "inline static lv_obj_t *lv_win_add_btn(lv_obj_t *win, const void *img_src)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_win_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Win",
    .tp_doc = "lvgl Win",
    .tp_basicsize = sizeof(pylv_Win),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_win_init,
    .tp_dealloc = (destructor) pylv_win_dealloc,
    .tp_methods = pylv_win_methods,
    .tp_weaklistoffset = offsetof(pylv_Win, weakreflist),
};

    
static void
pylv_tabview_dealloc(pylv_Tabview *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_tabview_init(pylv_Tabview *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Tabview *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_tabview_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_tabview_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_tabview_add_tab(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"name", NULL};
    const char * name;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &name)) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_tabview_add_tab(self->ref, name);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}

static PyObject*
pylv_tabview_clean_tab(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_tabview_clean_tab(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_tabview_set_tab_act(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"id", "anim", NULL};
    unsigned short int id;
    unsigned char anim;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hb", kwlist , &id, &anim)) return NULL;

    LVGL_LOCK         
    lv_tabview_set_tab_act(self->ref, id, anim);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_tabview_set_tab_name(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_tabview_set_tab_name: Parameter type not found >char*< ");
    return NULL;
}

static PyObject*
pylv_tabview_set_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"anim_time", NULL};
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_time)) return NULL;

    LVGL_LOCK         
    lv_tabview_set_anim_time(self->ref, anim_time);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_tabview_set_btns_pos(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"btns_pos", NULL};
    unsigned char btns_pos;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &btns_pos)) return NULL;

    LVGL_LOCK         
    lv_tabview_set_btns_pos(self->ref, btns_pos);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_tabview_get_tab_act(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_tabview_get_tab_act(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_tabview_get_tab_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_tabview_get_tab_count(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_tabview_get_tab(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"id", NULL};
    unsigned short int id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &id)) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_tabview_get_tab(self->ref, id);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}

static PyObject*
pylv_tabview_get_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_tabview_get_anim_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_tabview_get_btns_pos(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_tabview_btns_pos_t result = lv_tabview_get_btns_pos(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}


static PyMethodDef pylv_tabview_methods[] = {
    {"add_tab", (PyCFunction) pylv_tabview_add_tab, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_tabview_add_tab(lv_obj_t *tabview, const char *name)"},
    {"clean_tab", (PyCFunction) pylv_tabview_clean_tab, METH_VARARGS | METH_KEYWORDS, "void lv_tabview_clean_tab(lv_obj_t *tab)"},
    {"set_tab_act", (PyCFunction) pylv_tabview_set_tab_act, METH_VARARGS | METH_KEYWORDS, "void lv_tabview_set_tab_act(lv_obj_t *tabview, uint16_t id, lv_anim_enable_t anim)"},
    {"set_tab_name", (PyCFunction) pylv_tabview_set_tab_name, METH_VARARGS | METH_KEYWORDS, "void lv_tabview_set_tab_name(lv_obj_t *tabview, uint16_t id, char *name)"},
    {"set_anim_time", (PyCFunction) pylv_tabview_set_anim_time, METH_VARARGS | METH_KEYWORDS, "void lv_tabview_set_anim_time(lv_obj_t *tabview, uint16_t anim_time)"},
    {"set_btns_pos", (PyCFunction) pylv_tabview_set_btns_pos, METH_VARARGS | METH_KEYWORDS, "void lv_tabview_set_btns_pos(lv_obj_t *tabview, lv_tabview_btns_pos_t btns_pos)"},
    {"get_tab_act", (PyCFunction) pylv_tabview_get_tab_act, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_tabview_get_tab_act(const lv_obj_t *tabview)"},
    {"get_tab_count", (PyCFunction) pylv_tabview_get_tab_count, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_tabview_get_tab_count(const lv_obj_t *tabview)"},
    {"get_tab", (PyCFunction) pylv_tabview_get_tab, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_tabview_get_tab(const lv_obj_t *tabview, uint16_t id)"},
    {"get_anim_time", (PyCFunction) pylv_tabview_get_anim_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_tabview_get_anim_time(const lv_obj_t *tabview)"},
    {"get_btns_pos", (PyCFunction) pylv_tabview_get_btns_pos, METH_VARARGS | METH_KEYWORDS, "lv_tabview_btns_pos_t lv_tabview_get_btns_pos(const lv_obj_t *tabview)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_tabview_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Tabview",
    .tp_doc = "lvgl Tabview",
    .tp_basicsize = sizeof(pylv_Tabview),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_tabview_init,
    .tp_dealloc = (destructor) pylv_tabview_dealloc,
    .tp_methods = pylv_tabview_methods,
    .tp_weaklistoffset = offsetof(pylv_Tabview, weakreflist),
};

    
static void
pylv_tileview_dealloc(pylv_Tileview *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_tileview_init(pylv_Tileview *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Tileview *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_tileview_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_tileview_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_tileview_add_element(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"element", NULL};
    pylv_Obj * element;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &pylv_obj_Type, &element)) return NULL;

    LVGL_LOCK         
    lv_tileview_add_element(self->ref, element->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_tileview_set_valid_positions(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_tileview_set_valid_positions: Parameter type not found >lv_point_t< ");
    return NULL;
}

static PyObject*
pylv_tileview_set_tile_act(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"x", "y", "anim", NULL};
    short int x;
    short int y;
    unsigned char anim;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hhb", kwlist , &x, &y, &anim)) return NULL;

    LVGL_LOCK         
    lv_tileview_set_tile_act(self->ref, x, y, anim);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_tileview_get_tile_act(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_tileview_get_tile_act: Parameter type not found >lv_coord_t*< ");
    return NULL;
}


static PyMethodDef pylv_tileview_methods[] = {
    {"add_element", (PyCFunction) pylv_tileview_add_element, METH_VARARGS | METH_KEYWORDS, "void lv_tileview_add_element(lv_obj_t *tileview, lv_obj_t *element)"},
    {"set_valid_positions", (PyCFunction) pylv_tileview_set_valid_positions, METH_VARARGS | METH_KEYWORDS, "void lv_tileview_set_valid_positions(lv_obj_t *tileview, const lv_point_t valid_pos[], uint16_t valid_pos_cnt)"},
    {"set_tile_act", (PyCFunction) pylv_tileview_set_tile_act, METH_VARARGS | METH_KEYWORDS, "void lv_tileview_set_tile_act(lv_obj_t *tileview, lv_coord_t x, lv_coord_t y, lv_anim_enable_t anim)"},
    {"get_tile_act", (PyCFunction) pylv_tileview_get_tile_act, METH_VARARGS | METH_KEYWORDS, "void lv_tileview_get_tile_act(lv_obj_t *tileview, lv_coord_t *x, lv_coord_t *y)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_tileview_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Tileview",
    .tp_doc = "lvgl Tileview",
    .tp_basicsize = sizeof(pylv_Tileview),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_tileview_init,
    .tp_dealloc = (destructor) pylv_tileview_dealloc,
    .tp_methods = pylv_tileview_methods,
    .tp_weaklistoffset = offsetof(pylv_Tileview, weakreflist),
};

    
static void
pylv_msgbox_dealloc(pylv_Msgbox *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_msgbox_init(pylv_Msgbox *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Msgbox *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_msgbox_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_msgbox_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_msgbox_add_btns(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"btn_mapaction", NULL};
    char btn_mapaction;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "c", kwlist , &btn_mapaction)) return NULL;

    LVGL_LOCK         
    lv_msgbox_add_btns(self->ref, btn_mapaction);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_msgbox_set_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"txt", NULL};
    const char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    LVGL_LOCK         
    lv_msgbox_set_text(self->ref, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_msgbox_set_text_fmt(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_msgbox_set_text_fmt: variable arguments not supported");
    return NULL;
}

static PyObject*
pylv_msgbox_set_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"anim_time", NULL};
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_time)) return NULL;

    LVGL_LOCK         
    lv_msgbox_set_anim_time(self->ref, anim_time);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_msgbox_start_auto_close(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"delay", NULL};
    unsigned short int delay;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &delay)) return NULL;

    LVGL_LOCK         
    lv_msgbox_start_auto_close(self->ref, delay);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_msgbox_stop_auto_close(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_msgbox_stop_auto_close(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_msgbox_set_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_msgbox_set_recolor(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_msgbox_get_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_msgbox_get_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_msgbox_get_active_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_msgbox_get_active_btn(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_msgbox_get_active_btn_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_msgbox_get_active_btn_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_msgbox_get_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_msgbox_get_anim_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_msgbox_get_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_msgbox_get_recolor(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_msgbox_get_btnmatrix(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_msgbox_get_btnmatrix(self->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}


static PyMethodDef pylv_msgbox_methods[] = {
    {"add_btns", (PyCFunction) pylv_msgbox_add_btns, METH_VARARGS | METH_KEYWORDS, "void lv_msgbox_add_btns(lv_obj_t *mbox, const char *btn_mapaction[])"},
    {"set_text", (PyCFunction) pylv_msgbox_set_text, METH_VARARGS | METH_KEYWORDS, "void lv_msgbox_set_text(lv_obj_t *mbox, const char *txt)"},
    {"set_text_fmt", (PyCFunction) pylv_msgbox_set_text_fmt, METH_VARARGS | METH_KEYWORDS, "void lv_msgbox_set_text_fmt(lv_obj_t *mbox, const char *fmt, ...)"},
    {"set_anim_time", (PyCFunction) pylv_msgbox_set_anim_time, METH_VARARGS | METH_KEYWORDS, "void lv_msgbox_set_anim_time(lv_obj_t *mbox, uint16_t anim_time)"},
    {"start_auto_close", (PyCFunction) pylv_msgbox_start_auto_close, METH_VARARGS | METH_KEYWORDS, "void lv_msgbox_start_auto_close(lv_obj_t *mbox, uint16_t delay)"},
    {"stop_auto_close", (PyCFunction) pylv_msgbox_stop_auto_close, METH_VARARGS | METH_KEYWORDS, "void lv_msgbox_stop_auto_close(lv_obj_t *mbox)"},
    {"set_recolor", (PyCFunction) pylv_msgbox_set_recolor, METH_VARARGS | METH_KEYWORDS, "void lv_msgbox_set_recolor(lv_obj_t *mbox, bool en)"},
    {"get_text", (PyCFunction) pylv_msgbox_get_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_msgbox_get_text(const lv_obj_t *mbox)"},
    {"get_active_btn", (PyCFunction) pylv_msgbox_get_active_btn, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_msgbox_get_active_btn(lv_obj_t *mbox)"},
    {"get_active_btn_text", (PyCFunction) pylv_msgbox_get_active_btn_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_msgbox_get_active_btn_text(lv_obj_t *mbox)"},
    {"get_anim_time", (PyCFunction) pylv_msgbox_get_anim_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_msgbox_get_anim_time(const lv_obj_t *mbox)"},
    {"get_recolor", (PyCFunction) pylv_msgbox_get_recolor, METH_VARARGS | METH_KEYWORDS, "bool lv_msgbox_get_recolor(const lv_obj_t *mbox)"},
    {"get_btnmatrix", (PyCFunction) pylv_msgbox_get_btnmatrix, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_msgbox_get_btnmatrix(lv_obj_t *mbox)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_msgbox_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Msgbox",
    .tp_doc = "lvgl Msgbox",
    .tp_basicsize = sizeof(pylv_Msgbox),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_msgbox_init,
    .tp_dealloc = (destructor) pylv_msgbox_dealloc,
    .tp_methods = pylv_msgbox_methods,
    .tp_weaklistoffset = offsetof(pylv_Msgbox, weakreflist),
};

    
static void
pylv_objmask_dealloc(pylv_Objmask *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_objmask_init(pylv_Objmask *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Objmask *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_objmask_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_objmask_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_objmask_add_mask(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_objmask_add_mask: Parameter type not found >void*< ");
    return NULL;
}

static PyObject*
pylv_objmask_update_mask(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_objmask_update_mask: Parameter type not found >lv_objmask_mask_t*< ");
    return NULL;
}

static PyObject*
pylv_objmask_remove_mask(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_objmask_remove_mask: Parameter type not found >lv_objmask_mask_t*< ");
    return NULL;
}


static PyMethodDef pylv_objmask_methods[] = {
    {"add_mask", (PyCFunction) pylv_objmask_add_mask, METH_VARARGS | METH_KEYWORDS, "lv_objmask_mask_t *lv_objmask_add_mask(lv_obj_t *objmask, void *param)"},
    {"update_mask", (PyCFunction) pylv_objmask_update_mask, METH_VARARGS | METH_KEYWORDS, "void lv_objmask_update_mask(lv_obj_t *objmask, lv_objmask_mask_t *mask, void *param)"},
    {"remove_mask", (PyCFunction) pylv_objmask_remove_mask, METH_VARARGS | METH_KEYWORDS, "void lv_objmask_remove_mask(lv_obj_t *objmask, lv_objmask_mask_t *mask)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_objmask_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Objmask",
    .tp_doc = "lvgl Objmask",
    .tp_basicsize = sizeof(pylv_Objmask),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_objmask_init,
    .tp_dealloc = (destructor) pylv_objmask_dealloc,
    .tp_methods = pylv_objmask_methods,
    .tp_weaklistoffset = offsetof(pylv_Objmask, weakreflist),
};

    
static void
pylv_linemeter_dealloc(pylv_Linemeter *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_linemeter_init(pylv_Linemeter *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Linemeter *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_linemeter_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_linemeter_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_linemeter_set_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"value", NULL};
    int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "I", kwlist , &value)) return NULL;

    LVGL_LOCK         
    lv_linemeter_set_value(self->ref, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_linemeter_set_range(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"min", "max", NULL};
    int min;
    int max;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "II", kwlist , &min, &max)) return NULL;

    LVGL_LOCK         
    lv_linemeter_set_range(self->ref, min, max);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_linemeter_set_scale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"angle", "line_cnt", NULL};
    unsigned short int angle;
    unsigned short int line_cnt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HH", kwlist , &angle, &line_cnt)) return NULL;

    LVGL_LOCK         
    lv_linemeter_set_scale(self->ref, angle, line_cnt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_linemeter_set_angle_offset(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"angle", NULL};
    unsigned short int angle;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &angle)) return NULL;

    LVGL_LOCK         
    lv_linemeter_set_angle_offset(self->ref, angle);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_linemeter_set_mirror(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"mirror", NULL};
    int mirror;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &mirror)) return NULL;

    LVGL_LOCK         
    lv_linemeter_set_mirror(self->ref, mirror);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_linemeter_get_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int32_t result = lv_linemeter_get_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("I", result);
}

static PyObject*
pylv_linemeter_get_min_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int32_t result = lv_linemeter_get_min_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("I", result);
}

static PyObject*
pylv_linemeter_get_max_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int32_t result = lv_linemeter_get_max_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("I", result);
}

static PyObject*
pylv_linemeter_get_line_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_linemeter_get_line_count(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_linemeter_get_scale_angle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_linemeter_get_scale_angle(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_linemeter_get_angle_offset(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_linemeter_get_angle_offset(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_linemeter_draw_scale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_linemeter_draw_scale: Parameter type not found >const lv_area_t*< ");
    return NULL;
}

static PyObject*
pylv_linemeter_get_mirror(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_linemeter_get_mirror(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_linemeter_methods[] = {
    {"set_value", (PyCFunction) pylv_linemeter_set_value, METH_VARARGS | METH_KEYWORDS, "void lv_linemeter_set_value(lv_obj_t *lmeter, int32_t value)"},
    {"set_range", (PyCFunction) pylv_linemeter_set_range, METH_VARARGS | METH_KEYWORDS, "void lv_linemeter_set_range(lv_obj_t *lmeter, int32_t min, int32_t max)"},
    {"set_scale", (PyCFunction) pylv_linemeter_set_scale, METH_VARARGS | METH_KEYWORDS, "void lv_linemeter_set_scale(lv_obj_t *lmeter, uint16_t angle, uint16_t line_cnt)"},
    {"set_angle_offset", (PyCFunction) pylv_linemeter_set_angle_offset, METH_VARARGS | METH_KEYWORDS, "void lv_linemeter_set_angle_offset(lv_obj_t *lmeter, uint16_t angle)"},
    {"set_mirror", (PyCFunction) pylv_linemeter_set_mirror, METH_VARARGS | METH_KEYWORDS, "void lv_linemeter_set_mirror(lv_obj_t *lmeter, bool mirror)"},
    {"get_value", (PyCFunction) pylv_linemeter_get_value, METH_VARARGS | METH_KEYWORDS, "int32_t lv_linemeter_get_value(const lv_obj_t *lmeter)"},
    {"get_min_value", (PyCFunction) pylv_linemeter_get_min_value, METH_VARARGS | METH_KEYWORDS, "int32_t lv_linemeter_get_min_value(const lv_obj_t *lmeter)"},
    {"get_max_value", (PyCFunction) pylv_linemeter_get_max_value, METH_VARARGS | METH_KEYWORDS, "int32_t lv_linemeter_get_max_value(const lv_obj_t *lmeter)"},
    {"get_line_count", (PyCFunction) pylv_linemeter_get_line_count, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_linemeter_get_line_count(const lv_obj_t *lmeter)"},
    {"get_scale_angle", (PyCFunction) pylv_linemeter_get_scale_angle, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_linemeter_get_scale_angle(const lv_obj_t *lmeter)"},
    {"get_angle_offset", (PyCFunction) pylv_linemeter_get_angle_offset, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_linemeter_get_angle_offset(lv_obj_t *lmeter)"},
    {"draw_scale", (PyCFunction) pylv_linemeter_draw_scale, METH_VARARGS | METH_KEYWORDS, "void lv_linemeter_draw_scale(lv_obj_t *lmeter, const lv_area_t *clip_area, uint8_t part)"},
    {"get_mirror", (PyCFunction) pylv_linemeter_get_mirror, METH_VARARGS | METH_KEYWORDS, "bool lv_linemeter_get_mirror(lv_obj_t *lmeter)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_linemeter_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Linemeter",
    .tp_doc = "lvgl Linemeter",
    .tp_basicsize = sizeof(pylv_Linemeter),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_linemeter_init,
    .tp_dealloc = (destructor) pylv_linemeter_dealloc,
    .tp_methods = pylv_linemeter_methods,
    .tp_weaklistoffset = offsetof(pylv_Linemeter, weakreflist),
};

    
static void
pylv_gauge_dealloc(pylv_Gauge *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_gauge_init(pylv_Gauge *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Gauge *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_gauge_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_gauge_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_gauge_set_needle_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_gauge_set_needle_count: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_gauge_set_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"needle_id", "value", NULL};
    unsigned char needle_id;
    int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bI", kwlist , &needle_id, &value)) return NULL;

    LVGL_LOCK         
    lv_gauge_set_value(self->ref, needle_id, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_gauge_set_critical_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"value", NULL};
    int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "I", kwlist , &value)) return NULL;

    LVGL_LOCK         
    lv_gauge_set_critical_value(self->ref, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_gauge_set_scale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"angle", "line_cnt", "label_cnt", NULL};
    unsigned short int angle;
    unsigned char line_cnt;
    unsigned char label_cnt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hbb", kwlist , &angle, &line_cnt, &label_cnt)) return NULL;

    LVGL_LOCK         
    lv_gauge_set_scale(self->ref, angle, line_cnt, label_cnt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_gauge_set_needle_img(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_gauge_set_needle_img: Parameter type not found >const void*< ");
    return NULL;
}

static PyObject*
pylv_gauge_set_formatter_cb(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_gauge_set_formatter_cb: Parameter type not found >lv_gauge_format_cb_t< ");
    return NULL;
}

static PyObject*
pylv_gauge_get_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"needle", NULL};
    unsigned char needle;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &needle)) return NULL;

    LVGL_LOCK        
    int32_t result = lv_gauge_get_value(self->ref, needle);
    LVGL_UNLOCK
    return Py_BuildValue("I", result);
}

static PyObject*
pylv_gauge_get_needle_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint8_t result = lv_gauge_get_needle_count(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_gauge_get_critical_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int32_t result = lv_gauge_get_critical_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("I", result);
}

static PyObject*
pylv_gauge_get_label_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint8_t result = lv_gauge_get_label_count(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_gauge_get_needle_img(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_gauge_get_needle_img: Return type not found >const void*< ");
    return NULL;
}

static PyObject*
pylv_gauge_get_needle_img_pivot_x(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_gauge_get_needle_img_pivot_x(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_gauge_get_needle_img_pivot_y(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_gauge_get_needle_img_pivot_y(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}


static PyMethodDef pylv_gauge_methods[] = {
    {"set_needle_count", (PyCFunction) pylv_gauge_set_needle_count, METH_VARARGS | METH_KEYWORDS, "void lv_gauge_set_needle_count(lv_obj_t *gauge, uint8_t needle_cnt, const lv_color_t colors[])"},
    {"set_value", (PyCFunction) pylv_gauge_set_value, METH_VARARGS | METH_KEYWORDS, "void lv_gauge_set_value(lv_obj_t *gauge, uint8_t needle_id, int32_t value)"},
    {"set_critical_value", (PyCFunction) pylv_gauge_set_critical_value, METH_VARARGS | METH_KEYWORDS, "inline static void lv_gauge_set_critical_value(lv_obj_t *gauge, int32_t value)"},
    {"set_scale", (PyCFunction) pylv_gauge_set_scale, METH_VARARGS | METH_KEYWORDS, "void lv_gauge_set_scale(lv_obj_t *gauge, uint16_t angle, uint8_t line_cnt, uint8_t label_cnt)"},
    {"set_needle_img", (PyCFunction) pylv_gauge_set_needle_img, METH_VARARGS | METH_KEYWORDS, "void lv_gauge_set_needle_img(lv_obj_t *gauge, const void *img, lv_coord_t pivot_x, lv_coord_t pivot_y)"},
    {"set_formatter_cb", (PyCFunction) pylv_gauge_set_formatter_cb, METH_VARARGS | METH_KEYWORDS, "void lv_gauge_set_formatter_cb(lv_obj_t *gauge, lv_gauge_format_cb_t format_cb)"},
    {"get_value", (PyCFunction) pylv_gauge_get_value, METH_VARARGS | METH_KEYWORDS, "int32_t lv_gauge_get_value(const lv_obj_t *gauge, uint8_t needle)"},
    {"get_needle_count", (PyCFunction) pylv_gauge_get_needle_count, METH_VARARGS | METH_KEYWORDS, "uint8_t lv_gauge_get_needle_count(const lv_obj_t *gauge)"},
    {"get_critical_value", (PyCFunction) pylv_gauge_get_critical_value, METH_VARARGS | METH_KEYWORDS, "inline static int32_t lv_gauge_get_critical_value(const lv_obj_t *gauge)"},
    {"get_label_count", (PyCFunction) pylv_gauge_get_label_count, METH_VARARGS | METH_KEYWORDS, "uint8_t lv_gauge_get_label_count(const lv_obj_t *gauge)"},
    {"get_needle_img", (PyCFunction) pylv_gauge_get_needle_img, METH_VARARGS | METH_KEYWORDS, "const void *lv_gauge_get_needle_img(lv_obj_t *gauge)"},
    {"get_needle_img_pivot_x", (PyCFunction) pylv_gauge_get_needle_img_pivot_x, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_gauge_get_needle_img_pivot_x(lv_obj_t *gauge)"},
    {"get_needle_img_pivot_y", (PyCFunction) pylv_gauge_get_needle_img_pivot_y, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_gauge_get_needle_img_pivot_y(lv_obj_t *gauge)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_gauge_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Gauge",
    .tp_doc = "lvgl Gauge",
    .tp_basicsize = sizeof(pylv_Gauge),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_gauge_init,
    .tp_dealloc = (destructor) pylv_gauge_dealloc,
    .tp_methods = pylv_gauge_methods,
    .tp_weaklistoffset = offsetof(pylv_Gauge, weakreflist),
};

    
static void
pylv_switch_dealloc(pylv_Switch *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_switch_init(pylv_Switch *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Switch *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_switch_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_switch_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_switch_on(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"anim", NULL};
    unsigned char anim;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &anim)) return NULL;

    LVGL_LOCK         
    lv_switch_on(self->ref, anim);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_switch_off(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"anim", NULL};
    unsigned char anim;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &anim)) return NULL;

    LVGL_LOCK         
    lv_switch_off(self->ref, anim);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_switch_toggle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"anim", NULL};
    unsigned char anim;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &anim)) return NULL;

    LVGL_LOCK        
    bool result = lv_switch_toggle(self->ref, anim);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_switch_get_state(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_switch_get_state(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_switch_methods[] = {
    {"on", (PyCFunction) pylv_switch_on, METH_VARARGS | METH_KEYWORDS, "void lv_switch_on(lv_obj_t *sw, lv_anim_enable_t anim)"},
    {"off", (PyCFunction) pylv_switch_off, METH_VARARGS | METH_KEYWORDS, "void lv_switch_off(lv_obj_t *sw, lv_anim_enable_t anim)"},
    {"toggle", (PyCFunction) pylv_switch_toggle, METH_VARARGS | METH_KEYWORDS, "bool lv_switch_toggle(lv_obj_t *sw, lv_anim_enable_t anim)"},
    {"get_state", (PyCFunction) pylv_switch_get_state, METH_VARARGS | METH_KEYWORDS, "inline static bool lv_switch_get_state(const lv_obj_t *sw)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_switch_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Switch",
    .tp_doc = "lvgl Switch",
    .tp_basicsize = sizeof(pylv_Switch),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_switch_init,
    .tp_dealloc = (destructor) pylv_switch_dealloc,
    .tp_methods = pylv_switch_methods,
    .tp_weaklistoffset = offsetof(pylv_Switch, weakreflist),
};

    
static void
pylv_arc_dealloc(pylv_Arc *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_arc_init(pylv_Arc *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Arc *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_arc_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_arc_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_arc_set_start_angle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"start", NULL};
    unsigned short int start;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &start)) return NULL;

    LVGL_LOCK         
    lv_arc_set_start_angle(self->ref, start);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_arc_set_end_angle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"end", NULL};
    unsigned short int end;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &end)) return NULL;

    LVGL_LOCK         
    lv_arc_set_end_angle(self->ref, end);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_arc_set_angles(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"start", "end", NULL};
    unsigned short int start;
    unsigned short int end;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HH", kwlist , &start, &end)) return NULL;

    LVGL_LOCK         
    lv_arc_set_angles(self->ref, start, end);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_arc_set_bg_start_angle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"start", NULL};
    unsigned short int start;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &start)) return NULL;

    LVGL_LOCK         
    lv_arc_set_bg_start_angle(self->ref, start);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_arc_set_bg_end_angle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"end", NULL};
    unsigned short int end;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &end)) return NULL;

    LVGL_LOCK         
    lv_arc_set_bg_end_angle(self->ref, end);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_arc_set_bg_angles(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"start", "end", NULL};
    unsigned short int start;
    unsigned short int end;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HH", kwlist , &start, &end)) return NULL;

    LVGL_LOCK         
    lv_arc_set_bg_angles(self->ref, start, end);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_arc_set_rotation(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"rotation_angle", NULL};
    unsigned short int rotation_angle;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &rotation_angle)) return NULL;

    LVGL_LOCK         
    lv_arc_set_rotation(self->ref, rotation_angle);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_arc_set_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK         
    lv_arc_set_type(self->ref, type);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_arc_set_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"value", NULL};
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &value)) return NULL;

    LVGL_LOCK         
    lv_arc_set_value(self->ref, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_arc_set_range(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"min", "max", NULL};
    short int min;
    short int max;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &min, &max)) return NULL;

    LVGL_LOCK         
    lv_arc_set_range(self->ref, min, max);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_arc_set_chg_rate(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"threshold", NULL};
    unsigned short int threshold;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &threshold)) return NULL;

    LVGL_LOCK         
    lv_arc_set_chg_rate(self->ref, threshold);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_arc_set_adjustable(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"adjustable", NULL};
    int adjustable;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &adjustable)) return NULL;

    LVGL_LOCK         
    lv_arc_set_adjustable(self->ref, adjustable);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_arc_get_angle_start(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_arc_get_angle_start(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_arc_get_angle_end(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_arc_get_angle_end(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_arc_get_bg_angle_start(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_arc_get_bg_angle_start(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_arc_get_bg_angle_end(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_arc_get_bg_angle_end(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_arc_get_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_arc_type_t result = lv_arc_get_type(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_arc_get_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int16_t result = lv_arc_get_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_arc_get_min_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int16_t result = lv_arc_get_min_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_arc_get_max_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int16_t result = lv_arc_get_max_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_arc_is_dragged(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_arc_is_dragged(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_arc_get_adjustable(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_arc_get_adjustable(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_arc_methods[] = {
    {"set_start_angle", (PyCFunction) pylv_arc_set_start_angle, METH_VARARGS | METH_KEYWORDS, "void lv_arc_set_start_angle(lv_obj_t *arc, uint16_t start)"},
    {"set_end_angle", (PyCFunction) pylv_arc_set_end_angle, METH_VARARGS | METH_KEYWORDS, "void lv_arc_set_end_angle(lv_obj_t *arc, uint16_t end)"},
    {"set_angles", (PyCFunction) pylv_arc_set_angles, METH_VARARGS | METH_KEYWORDS, "void lv_arc_set_angles(lv_obj_t *arc, uint16_t start, uint16_t end)"},
    {"set_bg_start_angle", (PyCFunction) pylv_arc_set_bg_start_angle, METH_VARARGS | METH_KEYWORDS, "void lv_arc_set_bg_start_angle(lv_obj_t *arc, uint16_t start)"},
    {"set_bg_end_angle", (PyCFunction) pylv_arc_set_bg_end_angle, METH_VARARGS | METH_KEYWORDS, "void lv_arc_set_bg_end_angle(lv_obj_t *arc, uint16_t end)"},
    {"set_bg_angles", (PyCFunction) pylv_arc_set_bg_angles, METH_VARARGS | METH_KEYWORDS, "void lv_arc_set_bg_angles(lv_obj_t *arc, uint16_t start, uint16_t end)"},
    {"set_rotation", (PyCFunction) pylv_arc_set_rotation, METH_VARARGS | METH_KEYWORDS, "void lv_arc_set_rotation(lv_obj_t *arc, uint16_t rotation_angle)"},
    {"set_type", (PyCFunction) pylv_arc_set_type, METH_VARARGS | METH_KEYWORDS, "void lv_arc_set_type(lv_obj_t *arc, lv_arc_type_t type)"},
    {"set_value", (PyCFunction) pylv_arc_set_value, METH_VARARGS | METH_KEYWORDS, "void lv_arc_set_value(lv_obj_t *arc, int16_t value)"},
    {"set_range", (PyCFunction) pylv_arc_set_range, METH_VARARGS | METH_KEYWORDS, "void lv_arc_set_range(lv_obj_t *arc, int16_t min, int16_t max)"},
    {"set_chg_rate", (PyCFunction) pylv_arc_set_chg_rate, METH_VARARGS | METH_KEYWORDS, "void lv_arc_set_chg_rate(lv_obj_t *arc, uint16_t threshold)"},
    {"set_adjustable", (PyCFunction) pylv_arc_set_adjustable, METH_VARARGS | METH_KEYWORDS, "void lv_arc_set_adjustable(lv_obj_t *arc, bool adjustable)"},
    {"get_angle_start", (PyCFunction) pylv_arc_get_angle_start, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_arc_get_angle_start(lv_obj_t *arc)"},
    {"get_angle_end", (PyCFunction) pylv_arc_get_angle_end, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_arc_get_angle_end(lv_obj_t *arc)"},
    {"get_bg_angle_start", (PyCFunction) pylv_arc_get_bg_angle_start, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_arc_get_bg_angle_start(lv_obj_t *arc)"},
    {"get_bg_angle_end", (PyCFunction) pylv_arc_get_bg_angle_end, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_arc_get_bg_angle_end(lv_obj_t *arc)"},
    {"get_type", (PyCFunction) pylv_arc_get_type, METH_VARARGS | METH_KEYWORDS, "lv_arc_type_t lv_arc_get_type(const lv_obj_t *arc)"},
    {"get_value", (PyCFunction) pylv_arc_get_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_arc_get_value(const lv_obj_t *arc)"},
    {"get_min_value", (PyCFunction) pylv_arc_get_min_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_arc_get_min_value(const lv_obj_t *arc)"},
    {"get_max_value", (PyCFunction) pylv_arc_get_max_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_arc_get_max_value(const lv_obj_t *arc)"},
    {"is_dragged", (PyCFunction) pylv_arc_is_dragged, METH_VARARGS | METH_KEYWORDS, "bool lv_arc_is_dragged(const lv_obj_t *arc)"},
    {"get_adjustable", (PyCFunction) pylv_arc_get_adjustable, METH_VARARGS | METH_KEYWORDS, "bool lv_arc_get_adjustable(lv_obj_t *arc)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_arc_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Arc",
    .tp_doc = "lvgl Arc",
    .tp_basicsize = sizeof(pylv_Arc),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_arc_init,
    .tp_dealloc = (destructor) pylv_arc_dealloc,
    .tp_methods = pylv_arc_methods,
    .tp_weaklistoffset = offsetof(pylv_Arc, weakreflist),
};

    
static void
pylv_spinner_dealloc(pylv_Spinner *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_spinner_init(pylv_Spinner *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Spinner *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_spinner_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_spinner_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_spinner_set_arc_length(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"deg", NULL};
    short int deg;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &deg)) return NULL;

    LVGL_LOCK         
    lv_spinner_set_arc_length(self->ref, deg);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_spinner_set_spin_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"time", NULL};
    unsigned short int time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &time)) return NULL;

    LVGL_LOCK         
    lv_spinner_set_spin_time(self->ref, time);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_spinner_set_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK         
    lv_spinner_set_type(self->ref, type);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_spinner_set_dir(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"dir", NULL};
    unsigned char dir;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &dir)) return NULL;

    LVGL_LOCK         
    lv_spinner_set_dir(self->ref, dir);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_spinner_get_arc_length(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_anim_value_t result = lv_spinner_get_arc_length(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_spinner_get_spin_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_spinner_get_spin_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_spinner_get_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_spinner_type_t result = lv_spinner_get_type(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_spinner_get_dir(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_spinner_dir_t result = lv_spinner_get_dir(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}


static PyMethodDef pylv_spinner_methods[] = {
    {"set_arc_length", (PyCFunction) pylv_spinner_set_arc_length, METH_VARARGS | METH_KEYWORDS, "void lv_spinner_set_arc_length(lv_obj_t *spinner, lv_anim_value_t deg)"},
    {"set_spin_time", (PyCFunction) pylv_spinner_set_spin_time, METH_VARARGS | METH_KEYWORDS, "void lv_spinner_set_spin_time(lv_obj_t *spinner, uint16_t time)"},
    {"set_type", (PyCFunction) pylv_spinner_set_type, METH_VARARGS | METH_KEYWORDS, "void lv_spinner_set_type(lv_obj_t *spinner, lv_spinner_type_t type)"},
    {"set_dir", (PyCFunction) pylv_spinner_set_dir, METH_VARARGS | METH_KEYWORDS, "void lv_spinner_set_dir(lv_obj_t *spinner, lv_spinner_dir_t dir)"},
    {"get_arc_length", (PyCFunction) pylv_spinner_get_arc_length, METH_VARARGS | METH_KEYWORDS, "lv_anim_value_t lv_spinner_get_arc_length(const lv_obj_t *spinner)"},
    {"get_spin_time", (PyCFunction) pylv_spinner_get_spin_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_spinner_get_spin_time(const lv_obj_t *spinner)"},
    {"get_type", (PyCFunction) pylv_spinner_get_type, METH_VARARGS | METH_KEYWORDS, "lv_spinner_type_t lv_spinner_get_type(lv_obj_t *spinner)"},
    {"get_dir", (PyCFunction) pylv_spinner_get_dir, METH_VARARGS | METH_KEYWORDS, "lv_spinner_dir_t lv_spinner_get_dir(lv_obj_t *spinner)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_spinner_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Spinner",
    .tp_doc = "lvgl Spinner",
    .tp_basicsize = sizeof(pylv_Spinner),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_spinner_init,
    .tp_dealloc = (destructor) pylv_spinner_dealloc,
    .tp_methods = pylv_spinner_methods,
    .tp_weaklistoffset = offsetof(pylv_Spinner, weakreflist),
};

    
static void
pylv_calendar_dealloc(pylv_Calendar *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_calendar_init(pylv_Calendar *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Calendar *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_calendar_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_calendar_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_calendar_set_today_date(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_calendar_set_today_date: Parameter type not found >lv_calendar_date_t*< ");
    return NULL;
}

static PyObject*
pylv_calendar_set_showed_date(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_calendar_set_showed_date: Parameter type not found >lv_calendar_date_t*< ");
    return NULL;
}

static PyObject*
pylv_calendar_set_highlighted_dates(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_calendar_set_highlighted_dates: Parameter type not found >lv_calendar_date_t< ");
    return NULL;
}

static PyObject*
pylv_calendar_set_day_names(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_calendar_set_day_names: Parameter type not found >const char**< ");
    return NULL;
}

static PyObject*
pylv_calendar_set_month_names(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_calendar_set_month_names: Parameter type not found >const char**< ");
    return NULL;
}

static PyObject*
pylv_calendar_get_today_date(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_calendar_get_today_date: Return type not found >lv_calendar_date_t*< ");
    return NULL;
}

static PyObject*
pylv_calendar_get_showed_date(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_calendar_get_showed_date: Return type not found >lv_calendar_date_t*< ");
    return NULL;
}

static PyObject*
pylv_calendar_get_pressed_date(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_calendar_get_pressed_date: Return type not found >lv_calendar_date_t*< ");
    return NULL;
}

static PyObject*
pylv_calendar_get_highlighted_dates(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_calendar_get_highlighted_dates: Return type not found >lv_calendar_date_t*< ");
    return NULL;
}

static PyObject*
pylv_calendar_get_highlighted_dates_num(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_calendar_get_highlighted_dates_num(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_calendar_get_day_names(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_calendar_get_day_names: Return type not found >const char**< ");
    return NULL;
}

static PyObject*
pylv_calendar_get_month_names(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_calendar_get_month_names: Return type not found >const char**< ");
    return NULL;
}


static PyMethodDef pylv_calendar_methods[] = {
    {"set_today_date", (PyCFunction) pylv_calendar_set_today_date, METH_VARARGS | METH_KEYWORDS, "void lv_calendar_set_today_date(lv_obj_t *calendar, lv_calendar_date_t *today)"},
    {"set_showed_date", (PyCFunction) pylv_calendar_set_showed_date, METH_VARARGS | METH_KEYWORDS, "void lv_calendar_set_showed_date(lv_obj_t *calendar, lv_calendar_date_t *showed)"},
    {"set_highlighted_dates", (PyCFunction) pylv_calendar_set_highlighted_dates, METH_VARARGS | METH_KEYWORDS, "void lv_calendar_set_highlighted_dates(lv_obj_t *calendar, lv_calendar_date_t highlighted[], uint16_t date_num)"},
    {"set_day_names", (PyCFunction) pylv_calendar_set_day_names, METH_VARARGS | METH_KEYWORDS, "void lv_calendar_set_day_names(lv_obj_t *calendar, const char **day_names)"},
    {"set_month_names", (PyCFunction) pylv_calendar_set_month_names, METH_VARARGS | METH_KEYWORDS, "void lv_calendar_set_month_names(lv_obj_t *calendar, const char **month_names)"},
    {"get_today_date", (PyCFunction) pylv_calendar_get_today_date, METH_VARARGS | METH_KEYWORDS, "lv_calendar_date_t *lv_calendar_get_today_date(const lv_obj_t *calendar)"},
    {"get_showed_date", (PyCFunction) pylv_calendar_get_showed_date, METH_VARARGS | METH_KEYWORDS, "lv_calendar_date_t *lv_calendar_get_showed_date(const lv_obj_t *calendar)"},
    {"get_pressed_date", (PyCFunction) pylv_calendar_get_pressed_date, METH_VARARGS | METH_KEYWORDS, "lv_calendar_date_t *lv_calendar_get_pressed_date(const lv_obj_t *calendar)"},
    {"get_highlighted_dates", (PyCFunction) pylv_calendar_get_highlighted_dates, METH_VARARGS | METH_KEYWORDS, "lv_calendar_date_t *lv_calendar_get_highlighted_dates(const lv_obj_t *calendar)"},
    {"get_highlighted_dates_num", (PyCFunction) pylv_calendar_get_highlighted_dates_num, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_calendar_get_highlighted_dates_num(const lv_obj_t *calendar)"},
    {"get_day_names", (PyCFunction) pylv_calendar_get_day_names, METH_VARARGS | METH_KEYWORDS, "const char **lv_calendar_get_day_names(const lv_obj_t *calendar)"},
    {"get_month_names", (PyCFunction) pylv_calendar_get_month_names, METH_VARARGS | METH_KEYWORDS, "const char **lv_calendar_get_month_names(const lv_obj_t *calendar)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_calendar_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Calendar",
    .tp_doc = "lvgl Calendar",
    .tp_basicsize = sizeof(pylv_Calendar),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_calendar_init,
    .tp_dealloc = (destructor) pylv_calendar_dealloc,
    .tp_methods = pylv_calendar_methods,
    .tp_weaklistoffset = offsetof(pylv_Calendar, weakreflist),
};

    
static void
pylv_spinbox_dealloc(pylv_Spinbox *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_spinbox_init(pylv_Spinbox *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Spinbox *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_spinbox_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_spinbox_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    self->ref->user_data = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_spinbox_set_rollover(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"b", NULL};
    int b;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &b)) return NULL;

    LVGL_LOCK         
    lv_spinbox_set_rollover(self->ref, b);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_spinbox_set_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"i", NULL};
    int i;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "I", kwlist , &i)) return NULL;

    LVGL_LOCK         
    lv_spinbox_set_value(self->ref, i);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_spinbox_set_digit_format(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"digit_count", "separator_position", NULL};
    unsigned char digit_count;
    unsigned char separator_position;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bb", kwlist , &digit_count, &separator_position)) return NULL;

    LVGL_LOCK         
    lv_spinbox_set_digit_format(self->ref, digit_count, separator_position);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_spinbox_set_step(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"step", NULL};
    unsigned int step;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "I", kwlist , &step)) return NULL;

    LVGL_LOCK         
    lv_spinbox_set_step(self->ref, step);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_spinbox_set_range(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"range_min", "range_max", NULL};
    int range_min;
    int range_max;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "II", kwlist , &range_min, &range_max)) return NULL;

    LVGL_LOCK         
    lv_spinbox_set_range(self->ref, range_min, range_max);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_spinbox_set_padding_left(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"padding", NULL};
    unsigned char padding;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &padding)) return NULL;

    LVGL_LOCK         
    lv_spinbox_set_padding_left(self->ref, padding);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_spinbox_get_rollover(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_spinbox_get_rollover(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_spinbox_get_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int32_t result = lv_spinbox_get_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("I", result);
}

static PyObject*
pylv_spinbox_get_step(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int32_t result = lv_spinbox_get_step(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("I", result);
}

static PyObject*
pylv_spinbox_step_next(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_spinbox_step_next(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_spinbox_step_prev(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_spinbox_step_prev(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_spinbox_increment(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_spinbox_increment(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_spinbox_decrement(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_spinbox_decrement(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}


static PyMethodDef pylv_spinbox_methods[] = {
    {"set_rollover", (PyCFunction) pylv_spinbox_set_rollover, METH_VARARGS | METH_KEYWORDS, "void lv_spinbox_set_rollover(lv_obj_t *spinbox, bool b)"},
    {"set_value", (PyCFunction) pylv_spinbox_set_value, METH_VARARGS | METH_KEYWORDS, "void lv_spinbox_set_value(lv_obj_t *spinbox, int32_t i)"},
    {"set_digit_format", (PyCFunction) pylv_spinbox_set_digit_format, METH_VARARGS | METH_KEYWORDS, "void lv_spinbox_set_digit_format(lv_obj_t *spinbox, uint8_t digit_count, uint8_t separator_position)"},
    {"set_step", (PyCFunction) pylv_spinbox_set_step, METH_VARARGS | METH_KEYWORDS, "void lv_spinbox_set_step(lv_obj_t *spinbox, uint32_t step)"},
    {"set_range", (PyCFunction) pylv_spinbox_set_range, METH_VARARGS | METH_KEYWORDS, "void lv_spinbox_set_range(lv_obj_t *spinbox, int32_t range_min, int32_t range_max)"},
    {"set_padding_left", (PyCFunction) pylv_spinbox_set_padding_left, METH_VARARGS | METH_KEYWORDS, "void lv_spinbox_set_padding_left(lv_obj_t *spinbox, uint8_t padding)"},
    {"get_rollover", (PyCFunction) pylv_spinbox_get_rollover, METH_VARARGS | METH_KEYWORDS, "bool lv_spinbox_get_rollover(lv_obj_t *spinbox)"},
    {"get_value", (PyCFunction) pylv_spinbox_get_value, METH_VARARGS | METH_KEYWORDS, "int32_t lv_spinbox_get_value(lv_obj_t *spinbox)"},
    {"get_step", (PyCFunction) pylv_spinbox_get_step, METH_VARARGS | METH_KEYWORDS, "inline static int32_t lv_spinbox_get_step(lv_obj_t *spinbox)"},
    {"step_next", (PyCFunction) pylv_spinbox_step_next, METH_VARARGS | METH_KEYWORDS, "void lv_spinbox_step_next(lv_obj_t *spinbox)"},
    {"step_prev", (PyCFunction) pylv_spinbox_step_prev, METH_VARARGS | METH_KEYWORDS, "void lv_spinbox_step_prev(lv_obj_t *spinbox)"},
    {"increment", (PyCFunction) pylv_spinbox_increment, METH_VARARGS | METH_KEYWORDS, "void lv_spinbox_increment(lv_obj_t *spinbox)"},
    {"decrement", (PyCFunction) pylv_spinbox_decrement, METH_VARARGS | METH_KEYWORDS, "void lv_spinbox_decrement(lv_obj_t *spinbox)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_spinbox_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Spinbox",
    .tp_doc = "lvgl Spinbox",
    .tp_basicsize = sizeof(pylv_Spinbox),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_spinbox_init,
    .tp_dealloc = (destructor) pylv_spinbox_dealloc,
    .tp_methods = pylv_spinbox_methods,
    .tp_weaklistoffset = offsetof(pylv_Spinbox, weakreflist),
};





/****************************************************************
 * Miscellaneous functions                                      *
 ****************************************************************/

static PyObject *
pylv_scr_act(PyObject *self, PyObject *args) {
    lv_obj_t *scr;
    LVGL_LOCK
    scr = lv_scr_act();
    LVGL_UNLOCK
    return pyobj_from_lv(scr);
}

static PyObject *
pylv_scr_load(PyObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"scr", NULL};
    pylv_Obj *scr;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist, &pylv_obj_Type, &scr)) return NULL;
    LVGL_LOCK
    lv_scr_load(scr->ref);
    LVGL_UNLOCK
    
    Py_RETURN_NONE;
}


static PyObject *
poll(PyObject *self, PyObject *args) {
    LVGL_LOCK
    lv_tick_inc(1);
    lv_task_handler();
    LVGL_UNLOCK
    
    Py_RETURN_NONE;
}


/* TODO: all the framebuffer display driver stuff could be separated (i.e. do not default to it but allow user to register custom frame buffer driver) */

static lv_color_t disp_buf1[1024 * 10];
lv_disp_buf_t disp_buffer;
char framebuffer[LV_HOR_RES_MAX * LV_VER_RES_MAX * 2];


/* disp_flush should copy from the VDB (virtual display buffer to the screen.
 * In our case, we copy to the framebuffer
 */

 
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
    char *dest = framebuffer + ((area->y1)*LV_HOR_RES_MAX + area->x1) * 2;
    char *src = (char *) color_p;

    for(int32_t y = area->y1; y<=area->y2; y++) {
        memcpy(dest, src, 2*(area->x2-area->x1+1));
        src += 2*(area->x2-area->x1+1);
        dest += 2*LV_HOR_RES_MAX;
    }
    
    lv_disp_flush_ready(disp_drv);
}

static lv_disp_drv_t display_driver = {0};
static lv_indev_drv_t indev_driver = {0};
static int indev_driver_registered = 0;
static int indev_x, indev_y, indev_state=0;

static bool indev_read(struct _lv_indev_drv_t * indev_drv, lv_indev_data_t *data) {
    data->point.x = indev_x;
    data->point.y = indev_y;
    data->state = indev_state;

    return false;
}


static PyObject *
send_mouse_event(PyObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"x", "y", "pressed", NULL};
    int x=0, y=0, pressed=0;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iip", kwlist, &x, &y, &pressed)) {
        return NULL;
    }
    
    if (!indev_driver_registered) {
        lv_indev_drv_init(&indev_driver);
        indev_driver.type = LV_INDEV_TYPE_POINTER;
        indev_driver.read_cb = indev_read;
        lv_indev_drv_register(&indev_driver);
        indev_driver_registered = 1;
    }
    indev_x = x;
    indev_y = y;
    indev_state = pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    
    Py_RETURN_NONE;
}


/****************************************************************
 *  Module global stuff                                         *
 ****************************************************************/


static PyMethodDef lvglMethods[] = {
    {"scr_act",  pylv_scr_act, METH_NOARGS, NULL},
    {"scr_load", (PyCFunction)pylv_scr_load, METH_VARARGS | METH_KEYWORDS, NULL},
    {"poll", poll, METH_NOARGS, NULL},
    {"send_mouse_event", (PyCFunction)send_mouse_event, METH_VARARGS | METH_KEYWORDS, NULL},
//    {"report_style_mod", (PyCFunction)report_style_mod, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};



static struct PyModuleDef lvglmodule = {
    PyModuleDef_HEAD_INIT,
    "lvgl",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    lvglMethods
};

static PyObject*
Obj_repr(pylv_Obj *self) {
    return PyUnicode_FromFormat("<%s object at %p referencing %p>", Py_TYPE(self)->tp_name, self, self->ref);
}


PyMODINIT_FUNC
PyInit_lvgl(void) {

    PyObject *module = NULL;
    
    module = PyModule_Create(&lvglmodule);
    if (!module) goto error;
    
    pylv_obj_Type.tp_repr = (reprfunc) Obj_repr;   
    
    if (PyType_Ready(&Ptr_Type) < 0) return NULL;
    if (PyType_Ready(&Style_Type) < 0) return NULL;
    

    pylv_obj_Type.tp_base = NULL;
    if (PyType_Ready(&pylv_obj_Type) < 0) return NULL;

    pylv_cont_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_cont_Type) < 0) return NULL;

    pylv_btn_Type.tp_base = &pylv_cont_Type;
    if (PyType_Ready(&pylv_btn_Type) < 0) return NULL;

    pylv_imgbtn_Type.tp_base = &pylv_btn_Type;
    if (PyType_Ready(&pylv_imgbtn_Type) < 0) return NULL;

    pylv_label_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_label_Type) < 0) return NULL;

    pylv_img_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_img_Type) < 0) return NULL;

    pylv_line_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_line_Type) < 0) return NULL;

    pylv_page_Type.tp_base = &pylv_cont_Type;
    if (PyType_Ready(&pylv_page_Type) < 0) return NULL;

    pylv_list_Type.tp_base = &pylv_page_Type;
    if (PyType_Ready(&pylv_list_Type) < 0) return NULL;

    pylv_chart_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_chart_Type) < 0) return NULL;

    pylv_table_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_table_Type) < 0) return NULL;

    pylv_checkbox_Type.tp_base = &pylv_btn_Type;
    if (PyType_Ready(&pylv_checkbox_Type) < 0) return NULL;

    pylv_cpicker_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_cpicker_Type) < 0) return NULL;

    pylv_bar_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_bar_Type) < 0) return NULL;

    pylv_slider_Type.tp_base = &pylv_bar_Type;
    if (PyType_Ready(&pylv_slider_Type) < 0) return NULL;

    pylv_led_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_led_Type) < 0) return NULL;

    pylv_btnmatrix_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_btnmatrix_Type) < 0) return NULL;

    pylv_keyboard_Type.tp_base = &pylv_btnmatrix_Type;
    if (PyType_Ready(&pylv_keyboard_Type) < 0) return NULL;

    pylv_dropdown_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_dropdown_Type) < 0) return NULL;

    pylv_roller_Type.tp_base = &pylv_page_Type;
    if (PyType_Ready(&pylv_roller_Type) < 0) return NULL;

    pylv_textarea_Type.tp_base = &pylv_page_Type;
    if (PyType_Ready(&pylv_textarea_Type) < 0) return NULL;

    pylv_canvas_Type.tp_base = &pylv_img_Type;
    if (PyType_Ready(&pylv_canvas_Type) < 0) return NULL;

    pylv_win_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_win_Type) < 0) return NULL;

    pylv_tabview_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_tabview_Type) < 0) return NULL;

    pylv_tileview_Type.tp_base = &pylv_page_Type;
    if (PyType_Ready(&pylv_tileview_Type) < 0) return NULL;

    pylv_msgbox_Type.tp_base = &pylv_cont_Type;
    if (PyType_Ready(&pylv_msgbox_Type) < 0) return NULL;

    pylv_objmask_Type.tp_base = &pylv_cont_Type;
    if (PyType_Ready(&pylv_objmask_Type) < 0) return NULL;

    pylv_linemeter_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_linemeter_Type) < 0) return NULL;

    pylv_gauge_Type.tp_base = &pylv_linemeter_Type;
    if (PyType_Ready(&pylv_gauge_Type) < 0) return NULL;

    pylv_switch_Type.tp_base = &pylv_bar_Type;
    if (PyType_Ready(&pylv_switch_Type) < 0) return NULL;

    pylv_arc_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_arc_Type) < 0) return NULL;

    pylv_spinner_Type.tp_base = &pylv_arc_Type;
    if (PyType_Ready(&pylv_spinner_Type) < 0) return NULL;

    pylv_calendar_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_calendar_Type) < 0) return NULL;

    pylv_spinbox_Type.tp_base = &pylv_textarea_Type;
    if (PyType_Ready(&pylv_spinbox_Type) < 0) return NULL;


    if (PyType_Ready(&Blob_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_mem_monitor_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_mem_buf_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_ll_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_task_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_sqrt_res_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_color1_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_color8_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_color16_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_color32_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_color_hsv_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_point_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_area_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_disp_buf_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_disp_drv_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_disp_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_indev_data_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_indev_drv_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_indev_proc_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_indev_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_font_glyph_dsc_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_font_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_anim_path_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_anim_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_draw_mask_common_dsc_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_draw_mask_line_param_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_draw_mask_angle_param_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_draw_mask_radius_param_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_draw_mask_fade_param_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_draw_mask_map_param_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_style_list_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_draw_rect_dsc_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_draw_label_dsc_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_draw_label_hint_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_draw_line_dsc_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_img_header_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_img_dsc_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_img_transform_dsc_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_fs_drv_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_fs_file_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_fs_dir_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_img_decoder_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_img_decoder_dsc_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_draw_img_dsc_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_realign_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_obj_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_obj_type_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_hit_test_info_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_get_style_info_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_get_state_info_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_group_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_font_fmt_txt_glyph_dsc_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_font_fmt_txt_cmap_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_font_fmt_txt_kern_pair_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_font_fmt_txt_kern_classes_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_font_fmt_txt_dsc_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_cont_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_btn_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_imgbtn_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_label_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_img_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_line_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_page_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_list_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_chart_series_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_chart_cursor_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_chart_axis_cfg_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_chart_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_table_cell_format_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_table_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_checkbox_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_cpicker_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_bar_anim_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_bar_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_slider_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_led_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_btnmatrix_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_keyboard_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_dropdown_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_roller_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_textarea_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_canvas_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_win_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_tabview_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_tileview_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_msgbox_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_objmask_mask_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_objmask_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_linemeter_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_gauge_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_switch_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_arc_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_spinner_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_calendar_date_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_calendar_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_spinbox_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_img_cache_entry_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_color1_t_ch_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_color8_t_ch_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_color16_t_ch_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_color32_t_ch_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_indev_proc_t_types_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_indev_proc_t_types_pointer_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_indev_proc_t_types_keypad_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_draw_mask_line_param_t_cfg_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_draw_mask_angle_param_t_cfg_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_draw_mask_radius_param_t_cfg_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_draw_mask_fade_param_t_cfg_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_draw_mask_map_param_t_cfg_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_img_transform_dsc_t_cfg_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_img_transform_dsc_t_res_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_img_transform_dsc_t_tmp_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_label_ext_t_dot_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_page_ext_t_scrlbar_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_page_ext_t_edge_flash_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_table_cell_format_t_s_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_cpicker_ext_t_knob_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_textarea_ext_t_cursor_Type) < 0) return NULL;


    struct_dict = PyDict_New();
    if (!struct_dict) return NULL;
    //TODO: remove
    PyModule_AddObject(module, "_structs_", struct_dict);


    Py_INCREF(&pylv_obj_Type);
    PyModule_AddObject(module, "Obj", (PyObject *) &pylv_obj_Type); 

    Py_INCREF(&pylv_cont_Type);
    PyModule_AddObject(module, "Cont", (PyObject *) &pylv_cont_Type); 

    Py_INCREF(&pylv_btn_Type);
    PyModule_AddObject(module, "Btn", (PyObject *) &pylv_btn_Type); 

    Py_INCREF(&pylv_imgbtn_Type);
    PyModule_AddObject(module, "Imgbtn", (PyObject *) &pylv_imgbtn_Type); 

    Py_INCREF(&pylv_label_Type);
    PyModule_AddObject(module, "Label", (PyObject *) &pylv_label_Type); 

    Py_INCREF(&pylv_img_Type);
    PyModule_AddObject(module, "Img", (PyObject *) &pylv_img_Type); 

    Py_INCREF(&pylv_line_Type);
    PyModule_AddObject(module, "Line", (PyObject *) &pylv_line_Type); 

    Py_INCREF(&pylv_page_Type);
    PyModule_AddObject(module, "Page", (PyObject *) &pylv_page_Type); 

    Py_INCREF(&pylv_list_Type);
    PyModule_AddObject(module, "List", (PyObject *) &pylv_list_Type); 

    Py_INCREF(&pylv_chart_Type);
    PyModule_AddObject(module, "Chart", (PyObject *) &pylv_chart_Type); 

    Py_INCREF(&pylv_table_Type);
    PyModule_AddObject(module, "Table", (PyObject *) &pylv_table_Type); 

    Py_INCREF(&pylv_checkbox_Type);
    PyModule_AddObject(module, "Checkbox", (PyObject *) &pylv_checkbox_Type); 

    Py_INCREF(&pylv_cpicker_Type);
    PyModule_AddObject(module, "Cpicker", (PyObject *) &pylv_cpicker_Type); 

    Py_INCREF(&pylv_bar_Type);
    PyModule_AddObject(module, "Bar", (PyObject *) &pylv_bar_Type); 

    Py_INCREF(&pylv_slider_Type);
    PyModule_AddObject(module, "Slider", (PyObject *) &pylv_slider_Type); 

    Py_INCREF(&pylv_led_Type);
    PyModule_AddObject(module, "Led", (PyObject *) &pylv_led_Type); 

    Py_INCREF(&pylv_btnmatrix_Type);
    PyModule_AddObject(module, "Btnmatrix", (PyObject *) &pylv_btnmatrix_Type); 

    Py_INCREF(&pylv_keyboard_Type);
    PyModule_AddObject(module, "Keyboard", (PyObject *) &pylv_keyboard_Type); 

    Py_INCREF(&pylv_dropdown_Type);
    PyModule_AddObject(module, "Dropdown", (PyObject *) &pylv_dropdown_Type); 

    Py_INCREF(&pylv_roller_Type);
    PyModule_AddObject(module, "Roller", (PyObject *) &pylv_roller_Type); 

    Py_INCREF(&pylv_textarea_Type);
    PyModule_AddObject(module, "Textarea", (PyObject *) &pylv_textarea_Type); 

    Py_INCREF(&pylv_canvas_Type);
    PyModule_AddObject(module, "Canvas", (PyObject *) &pylv_canvas_Type); 

    Py_INCREF(&pylv_win_Type);
    PyModule_AddObject(module, "Win", (PyObject *) &pylv_win_Type); 

    Py_INCREF(&pylv_tabview_Type);
    PyModule_AddObject(module, "Tabview", (PyObject *) &pylv_tabview_Type); 

    Py_INCREF(&pylv_tileview_Type);
    PyModule_AddObject(module, "Tileview", (PyObject *) &pylv_tileview_Type); 

    Py_INCREF(&pylv_msgbox_Type);
    PyModule_AddObject(module, "Msgbox", (PyObject *) &pylv_msgbox_Type); 

    Py_INCREF(&pylv_objmask_Type);
    PyModule_AddObject(module, "Objmask", (PyObject *) &pylv_objmask_Type); 

    Py_INCREF(&pylv_linemeter_Type);
    PyModule_AddObject(module, "Linemeter", (PyObject *) &pylv_linemeter_Type); 

    Py_INCREF(&pylv_gauge_Type);
    PyModule_AddObject(module, "Gauge", (PyObject *) &pylv_gauge_Type); 

    Py_INCREF(&pylv_switch_Type);
    PyModule_AddObject(module, "Switch", (PyObject *) &pylv_switch_Type); 

    Py_INCREF(&pylv_arc_Type);
    PyModule_AddObject(module, "Arc", (PyObject *) &pylv_arc_Type); 

    Py_INCREF(&pylv_spinner_Type);
    PyModule_AddObject(module, "Spinner", (PyObject *) &pylv_spinner_Type); 

    Py_INCREF(&pylv_calendar_Type);
    PyModule_AddObject(module, "Calendar", (PyObject *) &pylv_calendar_Type); 

    Py_INCREF(&pylv_spinbox_Type);
    PyModule_AddObject(module, "Spinbox", (PyObject *) &pylv_spinbox_Type); 



    Py_INCREF(&pylv_mem_monitor_t_Type);
    PyModule_AddObject(module, "mem_monitor_t", (PyObject *) &pylv_mem_monitor_t_Type); 

    Py_INCREF(&pylv_mem_buf_t_Type);
    PyModule_AddObject(module, "mem_buf_t", (PyObject *) &pylv_mem_buf_t_Type); 

    Py_INCREF(&pylv_ll_t_Type);
    PyModule_AddObject(module, "ll_t", (PyObject *) &pylv_ll_t_Type); 

    Py_INCREF(&pylv_task_t_Type);
    PyModule_AddObject(module, "task_t", (PyObject *) &pylv_task_t_Type); 

    Py_INCREF(&pylv_sqrt_res_t_Type);
    PyModule_AddObject(module, "sqrt_res_t", (PyObject *) &pylv_sqrt_res_t_Type); 

    Py_INCREF(&pylv_color1_t_Type);
    PyModule_AddObject(module, "color1_t", (PyObject *) &pylv_color1_t_Type); 

    Py_INCREF(&pylv_color8_t_Type);
    PyModule_AddObject(module, "color8_t", (PyObject *) &pylv_color8_t_Type); 

    Py_INCREF(&pylv_color16_t_Type);
    PyModule_AddObject(module, "color16_t", (PyObject *) &pylv_color16_t_Type); 

    Py_INCREF(&pylv_color32_t_Type);
    PyModule_AddObject(module, "color32_t", (PyObject *) &pylv_color32_t_Type); 

    Py_INCREF(&pylv_color_hsv_t_Type);
    PyModule_AddObject(module, "color_hsv_t", (PyObject *) &pylv_color_hsv_t_Type); 

    Py_INCREF(&pylv_point_t_Type);
    PyModule_AddObject(module, "point_t", (PyObject *) &pylv_point_t_Type); 

    Py_INCREF(&pylv_area_t_Type);
    PyModule_AddObject(module, "area_t", (PyObject *) &pylv_area_t_Type); 

    Py_INCREF(&pylv_disp_buf_t_Type);
    PyModule_AddObject(module, "disp_buf_t", (PyObject *) &pylv_disp_buf_t_Type); 

    Py_INCREF(&pylv_disp_drv_t_Type);
    PyModule_AddObject(module, "disp_drv_t", (PyObject *) &pylv_disp_drv_t_Type); 

    Py_INCREF(&pylv_disp_t_Type);
    PyModule_AddObject(module, "disp_t", (PyObject *) &pylv_disp_t_Type); 

    Py_INCREF(&pylv_indev_data_t_Type);
    PyModule_AddObject(module, "indev_data_t", (PyObject *) &pylv_indev_data_t_Type); 

    Py_INCREF(&pylv_indev_drv_t_Type);
    PyModule_AddObject(module, "indev_drv_t", (PyObject *) &pylv_indev_drv_t_Type); 

    Py_INCREF(&pylv_indev_proc_t_Type);
    PyModule_AddObject(module, "indev_proc_t", (PyObject *) &pylv_indev_proc_t_Type); 

    Py_INCREF(&pylv_indev_t_Type);
    PyModule_AddObject(module, "indev_t", (PyObject *) &pylv_indev_t_Type); 

    Py_INCREF(&pylv_font_glyph_dsc_t_Type);
    PyModule_AddObject(module, "font_glyph_dsc_t", (PyObject *) &pylv_font_glyph_dsc_t_Type); 

    Py_INCREF(&pylv_font_t_Type);
    PyModule_AddObject(module, "font_t", (PyObject *) &pylv_font_t_Type); 

    Py_INCREF(&pylv_anim_path_t_Type);
    PyModule_AddObject(module, "anim_path_t", (PyObject *) &pylv_anim_path_t_Type); 

    Py_INCREF(&pylv_anim_t_Type);
    PyModule_AddObject(module, "anim_t", (PyObject *) &pylv_anim_t_Type); 

    Py_INCREF(&pylv_draw_mask_common_dsc_t_Type);
    PyModule_AddObject(module, "draw_mask_common_dsc_t", (PyObject *) &pylv_draw_mask_common_dsc_t_Type); 

    Py_INCREF(&pylv_draw_mask_line_param_t_Type);
    PyModule_AddObject(module, "draw_mask_line_param_t", (PyObject *) &pylv_draw_mask_line_param_t_Type); 

    Py_INCREF(&pylv_draw_mask_angle_param_t_Type);
    PyModule_AddObject(module, "draw_mask_angle_param_t", (PyObject *) &pylv_draw_mask_angle_param_t_Type); 

    Py_INCREF(&pylv_draw_mask_radius_param_t_Type);
    PyModule_AddObject(module, "draw_mask_radius_param_t", (PyObject *) &pylv_draw_mask_radius_param_t_Type); 

    Py_INCREF(&pylv_draw_mask_fade_param_t_Type);
    PyModule_AddObject(module, "draw_mask_fade_param_t", (PyObject *) &pylv_draw_mask_fade_param_t_Type); 

    Py_INCREF(&pylv_draw_mask_map_param_t_Type);
    PyModule_AddObject(module, "draw_mask_map_param_t", (PyObject *) &pylv_draw_mask_map_param_t_Type); 

    Py_INCREF(&pylv_style_list_t_Type);
    PyModule_AddObject(module, "style_list_t", (PyObject *) &pylv_style_list_t_Type); 

    Py_INCREF(&pylv_draw_rect_dsc_t_Type);
    PyModule_AddObject(module, "draw_rect_dsc_t", (PyObject *) &pylv_draw_rect_dsc_t_Type); 

    Py_INCREF(&pylv_draw_label_dsc_t_Type);
    PyModule_AddObject(module, "draw_label_dsc_t", (PyObject *) &pylv_draw_label_dsc_t_Type); 

    Py_INCREF(&pylv_draw_label_hint_t_Type);
    PyModule_AddObject(module, "draw_label_hint_t", (PyObject *) &pylv_draw_label_hint_t_Type); 

    Py_INCREF(&pylv_draw_line_dsc_t_Type);
    PyModule_AddObject(module, "draw_line_dsc_t", (PyObject *) &pylv_draw_line_dsc_t_Type); 

    Py_INCREF(&pylv_img_header_t_Type);
    PyModule_AddObject(module, "img_header_t", (PyObject *) &pylv_img_header_t_Type); 

    Py_INCREF(&pylv_img_dsc_t_Type);
    PyModule_AddObject(module, "img_dsc_t", (PyObject *) &pylv_img_dsc_t_Type); 

    Py_INCREF(&pylv_img_transform_dsc_t_Type);
    PyModule_AddObject(module, "img_transform_dsc_t", (PyObject *) &pylv_img_transform_dsc_t_Type); 

    Py_INCREF(&pylv_fs_drv_t_Type);
    PyModule_AddObject(module, "fs_drv_t", (PyObject *) &pylv_fs_drv_t_Type); 

    Py_INCREF(&pylv_fs_file_t_Type);
    PyModule_AddObject(module, "fs_file_t", (PyObject *) &pylv_fs_file_t_Type); 

    Py_INCREF(&pylv_fs_dir_t_Type);
    PyModule_AddObject(module, "fs_dir_t", (PyObject *) &pylv_fs_dir_t_Type); 

    Py_INCREF(&pylv_img_decoder_t_Type);
    PyModule_AddObject(module, "img_decoder_t", (PyObject *) &pylv_img_decoder_t_Type); 

    Py_INCREF(&pylv_img_decoder_dsc_t_Type);
    PyModule_AddObject(module, "img_decoder_dsc_t", (PyObject *) &pylv_img_decoder_dsc_t_Type); 

    Py_INCREF(&pylv_draw_img_dsc_t_Type);
    PyModule_AddObject(module, "draw_img_dsc_t", (PyObject *) &pylv_draw_img_dsc_t_Type); 

    Py_INCREF(&pylv_realign_t_Type);
    PyModule_AddObject(module, "realign_t", (PyObject *) &pylv_realign_t_Type); 

    Py_INCREF(&pylv_obj_t_Type);
    PyModule_AddObject(module, "obj_t", (PyObject *) &pylv_obj_t_Type); 

    Py_INCREF(&pylv_obj_type_t_Type);
    PyModule_AddObject(module, "obj_type_t", (PyObject *) &pylv_obj_type_t_Type); 

    Py_INCREF(&pylv_hit_test_info_t_Type);
    PyModule_AddObject(module, "hit_test_info_t", (PyObject *) &pylv_hit_test_info_t_Type); 

    Py_INCREF(&pylv_get_style_info_t_Type);
    PyModule_AddObject(module, "get_style_info_t", (PyObject *) &pylv_get_style_info_t_Type); 

    Py_INCREF(&pylv_get_state_info_t_Type);
    PyModule_AddObject(module, "get_state_info_t", (PyObject *) &pylv_get_state_info_t_Type); 

    Py_INCREF(&pylv_group_t_Type);
    PyModule_AddObject(module, "group_t", (PyObject *) &pylv_group_t_Type); 

    Py_INCREF(&pylv_theme_t_Type);
    PyModule_AddObject(module, "theme_t", (PyObject *) &pylv_theme_t_Type); 

    Py_INCREF(&pylv_font_fmt_txt_glyph_dsc_t_Type);
    PyModule_AddObject(module, "font_fmt_txt_glyph_dsc_t", (PyObject *) &pylv_font_fmt_txt_glyph_dsc_t_Type); 

    Py_INCREF(&pylv_font_fmt_txt_cmap_t_Type);
    PyModule_AddObject(module, "font_fmt_txt_cmap_t", (PyObject *) &pylv_font_fmt_txt_cmap_t_Type); 

    Py_INCREF(&pylv_font_fmt_txt_kern_pair_t_Type);
    PyModule_AddObject(module, "font_fmt_txt_kern_pair_t", (PyObject *) &pylv_font_fmt_txt_kern_pair_t_Type); 

    Py_INCREF(&pylv_font_fmt_txt_kern_classes_t_Type);
    PyModule_AddObject(module, "font_fmt_txt_kern_classes_t", (PyObject *) &pylv_font_fmt_txt_kern_classes_t_Type); 

    Py_INCREF(&pylv_font_fmt_txt_dsc_t_Type);
    PyModule_AddObject(module, "font_fmt_txt_dsc_t", (PyObject *) &pylv_font_fmt_txt_dsc_t_Type); 

    Py_INCREF(&pylv_cont_ext_t_Type);
    PyModule_AddObject(module, "cont_ext_t", (PyObject *) &pylv_cont_ext_t_Type); 

    Py_INCREF(&pylv_btn_ext_t_Type);
    PyModule_AddObject(module, "btn_ext_t", (PyObject *) &pylv_btn_ext_t_Type); 

    Py_INCREF(&pylv_imgbtn_ext_t_Type);
    PyModule_AddObject(module, "imgbtn_ext_t", (PyObject *) &pylv_imgbtn_ext_t_Type); 

    Py_INCREF(&pylv_label_ext_t_Type);
    PyModule_AddObject(module, "label_ext_t", (PyObject *) &pylv_label_ext_t_Type); 

    Py_INCREF(&pylv_img_ext_t_Type);
    PyModule_AddObject(module, "img_ext_t", (PyObject *) &pylv_img_ext_t_Type); 

    Py_INCREF(&pylv_line_ext_t_Type);
    PyModule_AddObject(module, "line_ext_t", (PyObject *) &pylv_line_ext_t_Type); 

    Py_INCREF(&pylv_page_ext_t_Type);
    PyModule_AddObject(module, "page_ext_t", (PyObject *) &pylv_page_ext_t_Type); 

    Py_INCREF(&pylv_list_ext_t_Type);
    PyModule_AddObject(module, "list_ext_t", (PyObject *) &pylv_list_ext_t_Type); 

    Py_INCREF(&pylv_chart_series_t_Type);
    PyModule_AddObject(module, "chart_series_t", (PyObject *) &pylv_chart_series_t_Type); 

    Py_INCREF(&pylv_chart_cursor_t_Type);
    PyModule_AddObject(module, "chart_cursor_t", (PyObject *) &pylv_chart_cursor_t_Type); 

    Py_INCREF(&pylv_chart_axis_cfg_t_Type);
    PyModule_AddObject(module, "chart_axis_cfg_t", (PyObject *) &pylv_chart_axis_cfg_t_Type); 

    Py_INCREF(&pylv_chart_ext_t_Type);
    PyModule_AddObject(module, "chart_ext_t", (PyObject *) &pylv_chart_ext_t_Type); 

    Py_INCREF(&pylv_table_cell_format_t_Type);
    PyModule_AddObject(module, "table_cell_format_t", (PyObject *) &pylv_table_cell_format_t_Type); 

    Py_INCREF(&pylv_table_ext_t_Type);
    PyModule_AddObject(module, "table_ext_t", (PyObject *) &pylv_table_ext_t_Type); 

    Py_INCREF(&pylv_checkbox_ext_t_Type);
    PyModule_AddObject(module, "checkbox_ext_t", (PyObject *) &pylv_checkbox_ext_t_Type); 

    Py_INCREF(&pylv_cpicker_ext_t_Type);
    PyModule_AddObject(module, "cpicker_ext_t", (PyObject *) &pylv_cpicker_ext_t_Type); 

    Py_INCREF(&pylv_bar_anim_t_Type);
    PyModule_AddObject(module, "bar_anim_t", (PyObject *) &pylv_bar_anim_t_Type); 

    Py_INCREF(&pylv_bar_ext_t_Type);
    PyModule_AddObject(module, "bar_ext_t", (PyObject *) &pylv_bar_ext_t_Type); 

    Py_INCREF(&pylv_slider_ext_t_Type);
    PyModule_AddObject(module, "slider_ext_t", (PyObject *) &pylv_slider_ext_t_Type); 

    Py_INCREF(&pylv_led_ext_t_Type);
    PyModule_AddObject(module, "led_ext_t", (PyObject *) &pylv_led_ext_t_Type); 

    Py_INCREF(&pylv_btnmatrix_ext_t_Type);
    PyModule_AddObject(module, "btnmatrix_ext_t", (PyObject *) &pylv_btnmatrix_ext_t_Type); 

    Py_INCREF(&pylv_keyboard_ext_t_Type);
    PyModule_AddObject(module, "keyboard_ext_t", (PyObject *) &pylv_keyboard_ext_t_Type); 

    Py_INCREF(&pylv_dropdown_ext_t_Type);
    PyModule_AddObject(module, "dropdown_ext_t", (PyObject *) &pylv_dropdown_ext_t_Type); 

    Py_INCREF(&pylv_roller_ext_t_Type);
    PyModule_AddObject(module, "roller_ext_t", (PyObject *) &pylv_roller_ext_t_Type); 

    Py_INCREF(&pylv_textarea_ext_t_Type);
    PyModule_AddObject(module, "textarea_ext_t", (PyObject *) &pylv_textarea_ext_t_Type); 

    Py_INCREF(&pylv_canvas_ext_t_Type);
    PyModule_AddObject(module, "canvas_ext_t", (PyObject *) &pylv_canvas_ext_t_Type); 

    Py_INCREF(&pylv_win_ext_t_Type);
    PyModule_AddObject(module, "win_ext_t", (PyObject *) &pylv_win_ext_t_Type); 

    Py_INCREF(&pylv_tabview_ext_t_Type);
    PyModule_AddObject(module, "tabview_ext_t", (PyObject *) &pylv_tabview_ext_t_Type); 

    Py_INCREF(&pylv_tileview_ext_t_Type);
    PyModule_AddObject(module, "tileview_ext_t", (PyObject *) &pylv_tileview_ext_t_Type); 

    Py_INCREF(&pylv_msgbox_ext_t_Type);
    PyModule_AddObject(module, "msgbox_ext_t", (PyObject *) &pylv_msgbox_ext_t_Type); 

    Py_INCREF(&pylv_objmask_mask_t_Type);
    PyModule_AddObject(module, "objmask_mask_t", (PyObject *) &pylv_objmask_mask_t_Type); 

    Py_INCREF(&pylv_objmask_ext_t_Type);
    PyModule_AddObject(module, "objmask_ext_t", (PyObject *) &pylv_objmask_ext_t_Type); 

    Py_INCREF(&pylv_linemeter_ext_t_Type);
    PyModule_AddObject(module, "linemeter_ext_t", (PyObject *) &pylv_linemeter_ext_t_Type); 

    Py_INCREF(&pylv_gauge_ext_t_Type);
    PyModule_AddObject(module, "gauge_ext_t", (PyObject *) &pylv_gauge_ext_t_Type); 

    Py_INCREF(&pylv_switch_ext_t_Type);
    PyModule_AddObject(module, "switch_ext_t", (PyObject *) &pylv_switch_ext_t_Type); 

    Py_INCREF(&pylv_arc_ext_t_Type);
    PyModule_AddObject(module, "arc_ext_t", (PyObject *) &pylv_arc_ext_t_Type); 

    Py_INCREF(&pylv_spinner_ext_t_Type);
    PyModule_AddObject(module, "spinner_ext_t", (PyObject *) &pylv_spinner_ext_t_Type); 

    Py_INCREF(&pylv_calendar_date_t_Type);
    PyModule_AddObject(module, "calendar_date_t", (PyObject *) &pylv_calendar_date_t_Type); 

    Py_INCREF(&pylv_calendar_ext_t_Type);
    PyModule_AddObject(module, "calendar_ext_t", (PyObject *) &pylv_calendar_ext_t_Type); 

    Py_INCREF(&pylv_spinbox_ext_t_Type);
    PyModule_AddObject(module, "spinbox_ext_t", (PyObject *) &pylv_spinbox_ext_t_Type); 

    Py_INCREF(&pylv_img_cache_entry_t_Type);
    PyModule_AddObject(module, "img_cache_entry_t", (PyObject *) &pylv_img_cache_entry_t_Type); 


    
    PyModule_AddObject(module, "RES", build_constclass('d', "RES", "INV", LV_RES_INV, "OK", LV_RES_OK, NULL));
    PyModule_AddObject(module, "TASK_PRIO", build_constclass('d', "TASK_PRIO", "OFF", LV_TASK_PRIO_OFF, "LOWEST", LV_TASK_PRIO_LOWEST, "LOW", LV_TASK_PRIO_LOW, "MID", LV_TASK_PRIO_MID, "HIGH", LV_TASK_PRIO_HIGH, "HIGHEST", LV_TASK_PRIO_HIGHEST, NULL));
    PyModule_AddObject(module, "ALIGN", build_constclass('d', "ALIGN", "CENTER", LV_ALIGN_CENTER, "IN_TOP_LEFT", LV_ALIGN_IN_TOP_LEFT, "IN_TOP_MID", LV_ALIGN_IN_TOP_MID, "IN_TOP_RIGHT", LV_ALIGN_IN_TOP_RIGHT, "IN_BOTTOM_LEFT", LV_ALIGN_IN_BOTTOM_LEFT, "IN_BOTTOM_MID", LV_ALIGN_IN_BOTTOM_MID, "IN_BOTTOM_RIGHT", LV_ALIGN_IN_BOTTOM_RIGHT, "IN_LEFT_MID", LV_ALIGN_IN_LEFT_MID, "IN_RIGHT_MID", LV_ALIGN_IN_RIGHT_MID, "OUT_TOP_LEFT", LV_ALIGN_OUT_TOP_LEFT, "OUT_TOP_MID", LV_ALIGN_OUT_TOP_MID, "OUT_TOP_RIGHT", LV_ALIGN_OUT_TOP_RIGHT, "OUT_BOTTOM_LEFT", LV_ALIGN_OUT_BOTTOM_LEFT, "OUT_BOTTOM_MID", LV_ALIGN_OUT_BOTTOM_MID, "OUT_BOTTOM_RIGHT", LV_ALIGN_OUT_BOTTOM_RIGHT, "OUT_LEFT_TOP", LV_ALIGN_OUT_LEFT_TOP, "OUT_LEFT_MID", LV_ALIGN_OUT_LEFT_MID, "OUT_LEFT_BOTTOM", LV_ALIGN_OUT_LEFT_BOTTOM, "OUT_RIGHT_TOP", LV_ALIGN_OUT_RIGHT_TOP, "OUT_RIGHT_MID", LV_ALIGN_OUT_RIGHT_MID, "OUT_RIGHT_BOTTOM", LV_ALIGN_OUT_RIGHT_BOTTOM, NULL));
    PyModule_AddObject(module, "INDEV_TYPE", build_constclass('d', "INDEV_TYPE", "NONE", LV_INDEV_TYPE_NONE, "POINTER", LV_INDEV_TYPE_POINTER, "KEYPAD", LV_INDEV_TYPE_KEYPAD, "BUTTON", LV_INDEV_TYPE_BUTTON, "ENCODER", LV_INDEV_TYPE_ENCODER, NULL));
    PyModule_AddObject(module, "INDEV_STATE", build_constclass('d', "INDEV_STATE", "REL", LV_INDEV_STATE_REL, "PR", LV_INDEV_STATE_PR, NULL));
    PyModule_AddObject(module, "DRAG_DIR", build_constclass('d', "DRAG_DIR", "HOR", LV_DRAG_DIR_HOR, "VER", LV_DRAG_DIR_VER, "BOTH", LV_DRAG_DIR_BOTH, "ONE", LV_DRAG_DIR_ONE, NULL));
    PyModule_AddObject(module, "GESTURE_DIR", build_constclass('d', "GESTURE_DIR", "TOP", LV_GESTURE_DIR_TOP, "BOTTOM", LV_GESTURE_DIR_BOTTOM, "LEFT", LV_GESTURE_DIR_LEFT, "RIGHT", LV_GESTURE_DIR_RIGHT, NULL));
    PyModule_AddObject(module, "FONT_SUBPX", build_constclass('d', "FONT_SUBPX", "NONE", LV_FONT_SUBPX_NONE, "HOR", LV_FONT_SUBPX_HOR, "VER", LV_FONT_SUBPX_VER, "BOTH", LV_FONT_SUBPX_BOTH, NULL));
    PyModule_AddObject(module, "ANIM", build_constclass('d', "ANIM", "OFF", LV_ANIM_OFF, "ON", LV_ANIM_ON, NULL));
    PyModule_AddObject(module, "DRAW_MASK_RES", build_constclass('d', "DRAW_MASK_RES", "TRANSP", LV_DRAW_MASK_RES_TRANSP, "FULL_COVER", LV_DRAW_MASK_RES_FULL_COVER, "CHANGED", LV_DRAW_MASK_RES_CHANGED, "UNKNOWN", LV_DRAW_MASK_RES_UNKNOWN, NULL));
    PyModule_AddObject(module, "DRAW_MASK_TYPE", build_constclass('d', "DRAW_MASK_TYPE", "LINE", LV_DRAW_MASK_TYPE_LINE, "ANGLE", LV_DRAW_MASK_TYPE_ANGLE, "RADIUS", LV_DRAW_MASK_TYPE_RADIUS, "FADE", LV_DRAW_MASK_TYPE_FADE, "MAP", LV_DRAW_MASK_TYPE_MAP, NULL));
    PyModule_AddObject(module, "BLEND_MODE", build_constclass('d', "BLEND_MODE", "NORMAL", LV_BLEND_MODE_NORMAL, "ADDITIVE", LV_BLEND_MODE_ADDITIVE, "SUBTRACTIVE", LV_BLEND_MODE_SUBTRACTIVE, NULL));
    PyModule_AddObject(module, "BORDER_SIDE", build_constclass('d', "BORDER_SIDE", "NONE", LV_BORDER_SIDE_NONE, "BOTTOM", LV_BORDER_SIDE_BOTTOM, "TOP", LV_BORDER_SIDE_TOP, "LEFT", LV_BORDER_SIDE_LEFT, "RIGHT", LV_BORDER_SIDE_RIGHT, "FULL", LV_BORDER_SIDE_FULL, "INTERNAL", LV_BORDER_SIDE_INTERNAL, NULL));
    PyModule_AddObject(module, "GRAD_DIR", build_constclass('d', "GRAD_DIR", "NONE", LV_GRAD_DIR_NONE, "VER", LV_GRAD_DIR_VER, "HOR", LV_GRAD_DIR_HOR, NULL));
    PyModule_AddObject(module, "TEXT_DECOR", build_constclass('d', "TEXT_DECOR", "NONE", LV_TEXT_DECOR_NONE, "UNDERLINE", LV_TEXT_DECOR_UNDERLINE, "STRIKETHROUGH", LV_TEXT_DECOR_STRIKETHROUGH, NULL));
    PyModule_AddObject(module, "STYLE", build_constclass('d', "STYLE", "RADIUS", LV_STYLE_RADIUS, "CLIP_CORNER", LV_STYLE_CLIP_CORNER, "SIZE", LV_STYLE_SIZE, "TRANSFORM_WIDTH", LV_STYLE_TRANSFORM_WIDTH, "TRANSFORM_HEIGHT", LV_STYLE_TRANSFORM_HEIGHT, "TRANSFORM_ANGLE", LV_STYLE_TRANSFORM_ANGLE, "TRANSFORM_ZOOM", LV_STYLE_TRANSFORM_ZOOM, "OPA_SCALE", LV_STYLE_OPA_SCALE, "PAD_TOP", LV_STYLE_PAD_TOP, "PAD_BOTTOM", LV_STYLE_PAD_BOTTOM, "PAD_LEFT", LV_STYLE_PAD_LEFT, "PAD_RIGHT", LV_STYLE_PAD_RIGHT, "PAD_INNER", LV_STYLE_PAD_INNER, "MARGIN_TOP", LV_STYLE_MARGIN_TOP, "MARGIN_BOTTOM", LV_STYLE_MARGIN_BOTTOM, "MARGIN_LEFT", LV_STYLE_MARGIN_LEFT, "MARGIN_RIGHT", LV_STYLE_MARGIN_RIGHT, "BG_BLEND_MODE", LV_STYLE_BG_BLEND_MODE, "BG_MAIN_STOP", LV_STYLE_BG_MAIN_STOP, "BG_GRAD_STOP", LV_STYLE_BG_GRAD_STOP, "BG_GRAD_DIR", LV_STYLE_BG_GRAD_DIR, "BG_COLOR", LV_STYLE_BG_COLOR, "BG_GRAD_COLOR", LV_STYLE_BG_GRAD_COLOR, "BG_OPA", LV_STYLE_BG_OPA, "BORDER_WIDTH", LV_STYLE_BORDER_WIDTH, "BORDER_SIDE", LV_STYLE_BORDER_SIDE, "BORDER_BLEND_MODE", LV_STYLE_BORDER_BLEND_MODE, "BORDER_POST", LV_STYLE_BORDER_POST, "BORDER_COLOR", LV_STYLE_BORDER_COLOR, "BORDER_OPA", LV_STYLE_BORDER_OPA, "OUTLINE_WIDTH", LV_STYLE_OUTLINE_WIDTH, "OUTLINE_PAD", LV_STYLE_OUTLINE_PAD, "OUTLINE_BLEND_MODE", LV_STYLE_OUTLINE_BLEND_MODE, "OUTLINE_COLOR", LV_STYLE_OUTLINE_COLOR, "OUTLINE_OPA", LV_STYLE_OUTLINE_OPA, "SHADOW_WIDTH", LV_STYLE_SHADOW_WIDTH, "SHADOW_OFS_X", LV_STYLE_SHADOW_OFS_X, "SHADOW_OFS_Y", LV_STYLE_SHADOW_OFS_Y, "SHADOW_SPREAD", LV_STYLE_SHADOW_SPREAD, "SHADOW_BLEND_MODE", LV_STYLE_SHADOW_BLEND_MODE, "SHADOW_COLOR", LV_STYLE_SHADOW_COLOR, "SHADOW_OPA", LV_STYLE_SHADOW_OPA, "PATTERN_BLEND_MODE", LV_STYLE_PATTERN_BLEND_MODE, "PATTERN_REPEAT", LV_STYLE_PATTERN_REPEAT, "PATTERN_RECOLOR", LV_STYLE_PATTERN_RECOLOR, "PATTERN_OPA", LV_STYLE_PATTERN_OPA, "PATTERN_RECOLOR_OPA", LV_STYLE_PATTERN_RECOLOR_OPA, "PATTERN_IMAGE", LV_STYLE_PATTERN_IMAGE, "VALUE_LETTER_SPACE", LV_STYLE_VALUE_LETTER_SPACE, "VALUE_LINE_SPACE", LV_STYLE_VALUE_LINE_SPACE, "VALUE_BLEND_MODE", LV_STYLE_VALUE_BLEND_MODE, "VALUE_OFS_X", LV_STYLE_VALUE_OFS_X, "VALUE_OFS_Y", LV_STYLE_VALUE_OFS_Y, "VALUE_ALIGN", LV_STYLE_VALUE_ALIGN, "VALUE_COLOR", LV_STYLE_VALUE_COLOR, "VALUE_OPA", LV_STYLE_VALUE_OPA, "VALUE_FONT", LV_STYLE_VALUE_FONT, "VALUE_STR", LV_STYLE_VALUE_STR, "TEXT_LETTER_SPACE", LV_STYLE_TEXT_LETTER_SPACE, "TEXT_LINE_SPACE", LV_STYLE_TEXT_LINE_SPACE, "TEXT_DECOR", LV_STYLE_TEXT_DECOR, "TEXT_BLEND_MODE", LV_STYLE_TEXT_BLEND_MODE, "TEXT_COLOR", LV_STYLE_TEXT_COLOR, "TEXT_SEL_COLOR", LV_STYLE_TEXT_SEL_COLOR, "TEXT_SEL_BG_COLOR", LV_STYLE_TEXT_SEL_BG_COLOR, "TEXT_OPA", LV_STYLE_TEXT_OPA, "TEXT_FONT", LV_STYLE_TEXT_FONT, "LINE_WIDTH", LV_STYLE_LINE_WIDTH, "LINE_BLEND_MODE", LV_STYLE_LINE_BLEND_MODE, "LINE_DASH_WIDTH", LV_STYLE_LINE_DASH_WIDTH, "LINE_DASH_GAP", LV_STYLE_LINE_DASH_GAP, "LINE_ROUNDED", LV_STYLE_LINE_ROUNDED, "LINE_COLOR", LV_STYLE_LINE_COLOR, "LINE_OPA", LV_STYLE_LINE_OPA, "IMAGE_BLEND_MODE", LV_STYLE_IMAGE_BLEND_MODE, "IMAGE_RECOLOR", LV_STYLE_IMAGE_RECOLOR, "IMAGE_OPA", LV_STYLE_IMAGE_OPA, "IMAGE_RECOLOR_OPA", LV_STYLE_IMAGE_RECOLOR_OPA, "TRANSITION_TIME", LV_STYLE_TRANSITION_TIME, "TRANSITION_DELAY", LV_STYLE_TRANSITION_DELAY, "TRANSITION_PROP_1", LV_STYLE_TRANSITION_PROP_1, "TRANSITION_PROP_2", LV_STYLE_TRANSITION_PROP_2, "TRANSITION_PROP_3", LV_STYLE_TRANSITION_PROP_3, "TRANSITION_PROP_4", LV_STYLE_TRANSITION_PROP_4, "TRANSITION_PROP_5", LV_STYLE_TRANSITION_PROP_5, "TRANSITION_PROP_6", LV_STYLE_TRANSITION_PROP_6, "TRANSITION_PATH", LV_STYLE_TRANSITION_PATH, "SCALE_WIDTH", LV_STYLE_SCALE_WIDTH, "SCALE_BORDER_WIDTH", LV_STYLE_SCALE_BORDER_WIDTH, "SCALE_END_BORDER_WIDTH", LV_STYLE_SCALE_END_BORDER_WIDTH, "SCALE_END_LINE_WIDTH", LV_STYLE_SCALE_END_LINE_WIDTH, "SCALE_GRAD_COLOR", LV_STYLE_SCALE_GRAD_COLOR, "SCALE_END_COLOR", LV_STYLE_SCALE_END_COLOR, NULL));
    PyModule_AddObject(module, "BIDI_DIR", build_constclass('d', "BIDI_DIR", "LTR", LV_BIDI_DIR_LTR, "RTL", LV_BIDI_DIR_RTL, "AUTO", LV_BIDI_DIR_AUTO, "INHERIT", LV_BIDI_DIR_INHERIT, "NEUTRAL", LV_BIDI_DIR_NEUTRAL, "WEAK", LV_BIDI_DIR_WEAK, NULL));
    PyModule_AddObject(module, "TXT_FLAG", build_constclass('d', "TXT_FLAG", "NONE", LV_TXT_FLAG_NONE, "RECOLOR", LV_TXT_FLAG_RECOLOR, "EXPAND", LV_TXT_FLAG_EXPAND, "CENTER", LV_TXT_FLAG_CENTER, "RIGHT", LV_TXT_FLAG_RIGHT, "FIT", LV_TXT_FLAG_FIT, NULL));
    PyModule_AddObject(module, "TXT_CMD_STATE", build_constclass('d', "TXT_CMD_STATE", "WAIT", LV_TXT_CMD_STATE_WAIT, "PAR", LV_TXT_CMD_STATE_PAR, "IN", LV_TXT_CMD_STATE_IN, NULL));
    PyModule_AddObject(module, "IMG_CF", build_constclass('d', "IMG_CF", "UNKNOWN", LV_IMG_CF_UNKNOWN, "RAW", LV_IMG_CF_RAW, "RAW_ALPHA", LV_IMG_CF_RAW_ALPHA, "RAW_CHROMA_KEYED", LV_IMG_CF_RAW_CHROMA_KEYED, "TRUE_COLOR", LV_IMG_CF_TRUE_COLOR, "TRUE_COLOR_ALPHA", LV_IMG_CF_TRUE_COLOR_ALPHA, "TRUE_COLOR_CHROMA_KEYED", LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED, "INDEXED_1BIT", LV_IMG_CF_INDEXED_1BIT, "INDEXED_2BIT", LV_IMG_CF_INDEXED_2BIT, "INDEXED_4BIT", LV_IMG_CF_INDEXED_4BIT, "INDEXED_8BIT", LV_IMG_CF_INDEXED_8BIT, "ALPHA_1BIT", LV_IMG_CF_ALPHA_1BIT, "ALPHA_2BIT", LV_IMG_CF_ALPHA_2BIT, "ALPHA_4BIT", LV_IMG_CF_ALPHA_4BIT, "ALPHA_8BIT", LV_IMG_CF_ALPHA_8BIT, "RESERVED_15", LV_IMG_CF_RESERVED_15, "RESERVED_16", LV_IMG_CF_RESERVED_16, "RESERVED_17", LV_IMG_CF_RESERVED_17, "RESERVED_18", LV_IMG_CF_RESERVED_18, "RESERVED_19", LV_IMG_CF_RESERVED_19, "RESERVED_20", LV_IMG_CF_RESERVED_20, "RESERVED_21", LV_IMG_CF_RESERVED_21, "RESERVED_22", LV_IMG_CF_RESERVED_22, "RESERVED_23", LV_IMG_CF_RESERVED_23, "USER_ENCODED_0", LV_IMG_CF_USER_ENCODED_0, "USER_ENCODED_1", LV_IMG_CF_USER_ENCODED_1, "USER_ENCODED_2", LV_IMG_CF_USER_ENCODED_2, "USER_ENCODED_3", LV_IMG_CF_USER_ENCODED_3, "USER_ENCODED_4", LV_IMG_CF_USER_ENCODED_4, "USER_ENCODED_5", LV_IMG_CF_USER_ENCODED_5, "USER_ENCODED_6", LV_IMG_CF_USER_ENCODED_6, "USER_ENCODED_7", LV_IMG_CF_USER_ENCODED_7, NULL));
    PyModule_AddObject(module, "FS_RES", build_constclass('d', "FS_RES", "OK", LV_FS_RES_OK, "HW_ERR", LV_FS_RES_HW_ERR, "FS_ERR", LV_FS_RES_FS_ERR, "NOT_EX", LV_FS_RES_NOT_EX, "FULL", LV_FS_RES_FULL, "LOCKED", LV_FS_RES_LOCKED, "DENIED", LV_FS_RES_DENIED, "BUSY", LV_FS_RES_BUSY, "TOUT", LV_FS_RES_TOUT, "NOT_IMP", LV_FS_RES_NOT_IMP, "OUT_OF_MEM", LV_FS_RES_OUT_OF_MEM, "INV_PARAM", LV_FS_RES_INV_PARAM, "UNKNOWN", LV_FS_RES_UNKNOWN, NULL));
    PyModule_AddObject(module, "FS_MODE", build_constclass('d', "FS_MODE", "WR", LV_FS_MODE_WR, "RD", LV_FS_MODE_RD, NULL));
    PyModule_AddObject(module, "IMG_SRC", build_constclass('d', "IMG_SRC", "VARIABLE", LV_IMG_SRC_VARIABLE, "FILE", LV_IMG_SRC_FILE, "SYMBOL", LV_IMG_SRC_SYMBOL, "UNKNOWN", LV_IMG_SRC_UNKNOWN, NULL));
    PyModule_AddObject(module, "DESIGN", build_constclass('d', "DESIGN", "DRAW_MAIN", LV_DESIGN_DRAW_MAIN, "DRAW_POST", LV_DESIGN_DRAW_POST, "COVER_CHK", LV_DESIGN_COVER_CHK, NULL));
    PyModule_AddObject(module, "DESIGN_RES", build_constclass('d', "DESIGN_RES", "OK", LV_DESIGN_RES_OK, "COVER", LV_DESIGN_RES_COVER, "NOT_COVER", LV_DESIGN_RES_NOT_COVER, "MASKED", LV_DESIGN_RES_MASKED, NULL));
    PyModule_AddObject(module, "EVENT", build_constclass('d', "EVENT", "PRESSED", LV_EVENT_PRESSED, "PRESSING", LV_EVENT_PRESSING, "PRESS_LOST", LV_EVENT_PRESS_LOST, "SHORT_CLICKED", LV_EVENT_SHORT_CLICKED, "LONG_PRESSED", LV_EVENT_LONG_PRESSED, "LONG_PRESSED_REPEAT", LV_EVENT_LONG_PRESSED_REPEAT, "CLICKED", LV_EVENT_CLICKED, "RELEASED", LV_EVENT_RELEASED, "DRAG_BEGIN", LV_EVENT_DRAG_BEGIN, "DRAG_END", LV_EVENT_DRAG_END, "DRAG_THROW_BEGIN", LV_EVENT_DRAG_THROW_BEGIN, "GESTURE", LV_EVENT_GESTURE, "KEY", LV_EVENT_KEY, "FOCUSED", LV_EVENT_FOCUSED, "DEFOCUSED", LV_EVENT_DEFOCUSED, "LEAVE", LV_EVENT_LEAVE, "VALUE_CHANGED", LV_EVENT_VALUE_CHANGED, "INSERT", LV_EVENT_INSERT, "REFRESH", LV_EVENT_REFRESH, "APPLY", LV_EVENT_APPLY, "CANCEL", LV_EVENT_CANCEL, "DELETE", LV_EVENT_DELETE, NULL));
    PyModule_AddObject(module, "SIGNAL", build_constclass('d', "SIGNAL", "CLEANUP", LV_SIGNAL_CLEANUP, "CHILD_CHG", LV_SIGNAL_CHILD_CHG, "COORD_CHG", LV_SIGNAL_COORD_CHG, "PARENT_SIZE_CHG", LV_SIGNAL_PARENT_SIZE_CHG, "STYLE_CHG", LV_SIGNAL_STYLE_CHG, "BASE_DIR_CHG", LV_SIGNAL_BASE_DIR_CHG, "REFR_EXT_DRAW_PAD", LV_SIGNAL_REFR_EXT_DRAW_PAD, "GET_TYPE", LV_SIGNAL_GET_TYPE, "GET_STYLE", LV_SIGNAL_GET_STYLE, "GET_STATE_DSC", LV_SIGNAL_GET_STATE_DSC, "HIT_TEST", LV_SIGNAL_HIT_TEST, "PRESSED", LV_SIGNAL_PRESSED, "PRESSING", LV_SIGNAL_PRESSING, "PRESS_LOST", LV_SIGNAL_PRESS_LOST, "RELEASED", LV_SIGNAL_RELEASED, "LONG_PRESS", LV_SIGNAL_LONG_PRESS, "LONG_PRESS_REP", LV_SIGNAL_LONG_PRESS_REP, "DRAG_BEGIN", LV_SIGNAL_DRAG_BEGIN, "DRAG_THROW_BEGIN", LV_SIGNAL_DRAG_THROW_BEGIN, "DRAG_END", LV_SIGNAL_DRAG_END, "GESTURE", LV_SIGNAL_GESTURE, "LEAVE", LV_SIGNAL_LEAVE, "FOCUS", LV_SIGNAL_FOCUS, "DEFOCUS", LV_SIGNAL_DEFOCUS, "CONTROL", LV_SIGNAL_CONTROL, "GET_EDITABLE", LV_SIGNAL_GET_EDITABLE, NULL));
    PyModule_AddObject(module, "PROTECT", build_constclass('d', "PROTECT", "NONE", LV_PROTECT_NONE, "CHILD_CHG", LV_PROTECT_CHILD_CHG, "PARENT", LV_PROTECT_PARENT, "POS", LV_PROTECT_POS, "FOLLOW", LV_PROTECT_FOLLOW, "PRESS_LOST", LV_PROTECT_PRESS_LOST, "CLICK_FOCUS", LV_PROTECT_CLICK_FOCUS, "EVENT_TO_DISABLED", LV_PROTECT_EVENT_TO_DISABLED, NULL));
    PyModule_AddObject(module, "STATE", build_constclass('d', "STATE", "DEFAULT", LV_STATE_DEFAULT, "CHECKED", LV_STATE_CHECKED, "FOCUSED", LV_STATE_FOCUSED, "EDITED", LV_STATE_EDITED, "HOVERED", LV_STATE_HOVERED, "PRESSED", LV_STATE_PRESSED, "DISABLED", LV_STATE_DISABLED, NULL));
    PyModule_AddObject(module, "OBJ_PART", build_constclass('d', "OBJ_PART", "MAIN", LV_OBJ_PART_MAIN, "ALL", LV_OBJ_PART_ALL, NULL));
    PyModule_AddObject(module, "KEY", build_constclass('d', "KEY", "UP", LV_KEY_UP, "DOWN", LV_KEY_DOWN, "RIGHT", LV_KEY_RIGHT, "LEFT", LV_KEY_LEFT, "ESC", LV_KEY_ESC, "DEL", LV_KEY_DEL, "BACKSPACE", LV_KEY_BACKSPACE, "ENTER", LV_KEY_ENTER, "NEXT", LV_KEY_NEXT, "PREV", LV_KEY_PREV, "HOME", LV_KEY_HOME, "END", LV_KEY_END, NULL));
    PyModule_AddObject(module, "GROUP_REFOCUS_POLICY", build_constclass('d', "GROUP_REFOCUS_POLICY", "NEXT", LV_GROUP_REFOCUS_POLICY_NEXT, "PREV", LV_GROUP_REFOCUS_POLICY_PREV, NULL));
    PyModule_AddObject(module, "FONT_FMT_TXT_CMAP", build_constclass('d', "FONT_FMT_TXT_CMAP", "FORMAT0_FULL", LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL, "SPARSE_FULL", LV_FONT_FMT_TXT_CMAP_SPARSE_FULL, "FORMAT0_TINY", LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY, "SPARSE_TINY", LV_FONT_FMT_TXT_CMAP_SPARSE_TINY, NULL));
    PyModule_AddObject(module, "LAYOUT", build_constclass('d', "LAYOUT", "OFF", LV_LAYOUT_OFF, "CENTER", LV_LAYOUT_CENTER, "COLUMN_LEFT", LV_LAYOUT_COLUMN_LEFT, "COLUMN_MID", LV_LAYOUT_COLUMN_MID, "COLUMN_RIGHT", LV_LAYOUT_COLUMN_RIGHT, "ROW_TOP", LV_LAYOUT_ROW_TOP, "ROW_MID", LV_LAYOUT_ROW_MID, "ROW_BOTTOM", LV_LAYOUT_ROW_BOTTOM, "PRETTY_TOP", LV_LAYOUT_PRETTY_TOP, "PRETTY_MID", LV_LAYOUT_PRETTY_MID, "PRETTY_BOTTOM", LV_LAYOUT_PRETTY_BOTTOM, "GRID", LV_LAYOUT_GRID, NULL));
    PyModule_AddObject(module, "FIT", build_constclass('d', "FIT", "NONE", LV_FIT_NONE, "TIGHT", LV_FIT_TIGHT, "PARENT", LV_FIT_PARENT, "MAX", LV_FIT_MAX, NULL));
    PyModule_AddObject(module, "BTN_STATE", build_constclass('d', "BTN_STATE", "RELEASED", LV_BTN_STATE_RELEASED, "PRESSED", LV_BTN_STATE_PRESSED, "DISABLED", LV_BTN_STATE_DISABLED, "CHECKED_RELEASED", LV_BTN_STATE_CHECKED_RELEASED, "CHECKED_PRESSED", LV_BTN_STATE_CHECKED_PRESSED, "CHECKED_DISABLED", LV_BTN_STATE_CHECKED_DISABLED, NULL));
    PyModule_AddObject(module, "BTN_PART", build_constclass('d', "BTN_PART", "MAIN", LV_BTN_PART_MAIN, NULL));
    PyModule_AddObject(module, "IMGBTN_PART", build_constclass('d', "IMGBTN_PART", "MAIN", LV_IMGBTN_PART_MAIN, NULL));
    PyModule_AddObject(module, "LABEL_LONG", build_constclass('d', "LABEL_LONG", "EXPAND", LV_LABEL_LONG_EXPAND, "BREAK", LV_LABEL_LONG_BREAK, "DOT", LV_LABEL_LONG_DOT, "SROLL", LV_LABEL_LONG_SROLL, "SROLL_CIRC", LV_LABEL_LONG_SROLL_CIRC, "CROP", LV_LABEL_LONG_CROP, NULL));
    PyModule_AddObject(module, "LABEL_ALIGN", build_constclass('d', "LABEL_ALIGN", "LEFT", LV_LABEL_ALIGN_LEFT, "CENTER", LV_LABEL_ALIGN_CENTER, "RIGHT", LV_LABEL_ALIGN_RIGHT, "AUTO", LV_LABEL_ALIGN_AUTO, NULL));
    PyModule_AddObject(module, "LABEL_PART", build_constclass('d', "LABEL_PART", "MAIN", LV_LABEL_PART_MAIN, NULL));
    PyModule_AddObject(module, "IMG_PART", build_constclass('d', "IMG_PART", "MAIN", LV_IMG_PART_MAIN, NULL));
    PyModule_AddObject(module, "LINE_PART", build_constclass('d', "LINE_PART", "MAIN", LV_LINE_PART_MAIN, NULL));
    PyModule_AddObject(module, "SCROLLBAR_MODE", build_constclass('d', "SCROLLBAR_MODE", "OFF", LV_SCROLLBAR_MODE_OFF, "ON", LV_SCROLLBAR_MODE_ON, "DRAG", LV_SCROLLBAR_MODE_DRAG, "AUTO", LV_SCROLLBAR_MODE_AUTO, "HIDE", LV_SCROLLBAR_MODE_HIDE, "UNHIDE", LV_SCROLLBAR_MODE_UNHIDE, NULL));
    PyModule_AddObject(module, "PAGE_EDGE", build_constclass('d', "PAGE_EDGE", "LEFT", LV_PAGE_EDGE_LEFT, "TOP", LV_PAGE_EDGE_TOP, "RIGHT", LV_PAGE_EDGE_RIGHT, "BOTTOM", LV_PAGE_EDGE_BOTTOM, NULL));
    PyModule_AddObject(module, "PAGE_PART", build_constclass('d', "PAGE_PART", "BG", LV_PAGE_PART_BG, "SCROLLBAR", LV_PAGE_PART_SCROLLBAR, "EDGE_FLASH", LV_PAGE_PART_EDGE_FLASH, "SCROLLABLE", LV_PAGE_PART_SCROLLABLE, NULL));
    PyModule_AddObject(module, "LIST_PART", build_constclass('d', "LIST_PART", "BG", LV_LIST_PART_BG, "SCROLLBAR", LV_LIST_PART_SCROLLBAR, "EDGE_FLASH", LV_LIST_PART_EDGE_FLASH, "SCROLLABLE", LV_LIST_PART_SCROLLABLE, NULL));
    PyModule_AddObject(module, "CHART_TYPE", build_constclass('d', "CHART_TYPE", "NONE", LV_CHART_TYPE_NONE, "LINE", LV_CHART_TYPE_LINE, "COLUMN", LV_CHART_TYPE_COLUMN, NULL));
    PyModule_AddObject(module, "CHART_UPDATE_MODE", build_constclass('d', "CHART_UPDATE_MODE", "SHIFT", LV_CHART_UPDATE_MODE_SHIFT, "CIRCULAR", LV_CHART_UPDATE_MODE_CIRCULAR, NULL));
    PyModule_AddObject(module, "CHART_AXIS", build_constclass('d', "CHART_AXIS", "SKIP_LAST_TICK", LV_CHART_AXIS_SKIP_LAST_TICK, "DRAW_LAST_TICK", LV_CHART_AXIS_DRAW_LAST_TICK, "INVERSE_LABELS_ORDER", LV_CHART_AXIS_INVERSE_LABELS_ORDER, NULL));
    PyModule_AddObject(module, "CHART_CURSOR", build_constclass('d', "CHART_CURSOR", "NONE", LV_CHART_CURSOR_NONE, "RIGHT", LV_CHART_CURSOR_RIGHT, "UP", LV_CHART_CURSOR_UP, "LEFT", LV_CHART_CURSOR_LEFT, "DOWN", LV_CHART_CURSOR_DOWN, NULL));
    PyModule_AddObject(module, "CHECKBOX_PART", build_constclass('d', "CHECKBOX_PART", "BG", LV_CHECKBOX_PART_BG, "BULLET", LV_CHECKBOX_PART_BULLET, NULL));
    PyModule_AddObject(module, "CPICKER_TYPE", build_constclass('d', "CPICKER_TYPE", "RECT", LV_CPICKER_TYPE_RECT, "DISC", LV_CPICKER_TYPE_DISC, NULL));
    PyModule_AddObject(module, "CPICKER_COLOR_MODE", build_constclass('d', "CPICKER_COLOR_MODE", "HUE", LV_CPICKER_COLOR_MODE_HUE, "SATURATION", LV_CPICKER_COLOR_MODE_SATURATION, "VALUE", LV_CPICKER_COLOR_MODE_VALUE, NULL));
    PyModule_AddObject(module, "BAR_TYPE", build_constclass('d', "BAR_TYPE", "NORMAL", LV_BAR_TYPE_NORMAL, "SYMMETRICAL", LV_BAR_TYPE_SYMMETRICAL, "CUSTOM", LV_BAR_TYPE_CUSTOM, NULL));
    PyModule_AddObject(module, "BAR_PART", build_constclass('d', "BAR_PART", "BG", LV_BAR_PART_BG, "INDIC", LV_BAR_PART_INDIC, NULL));
    PyModule_AddObject(module, "SLIDER_TYPE", build_constclass('d', "SLIDER_TYPE", "NORMAL", LV_SLIDER_TYPE_NORMAL, "SYMMETRICAL", LV_SLIDER_TYPE_SYMMETRICAL, "RANGE", LV_SLIDER_TYPE_RANGE, NULL));
    PyModule_AddObject(module, "LED_PART", build_constclass('d', "LED_PART", "MAIN", LV_LED_PART_MAIN, NULL));
    PyModule_AddObject(module, "BTNMATRIX_CTRL", build_constclass('d', "BTNMATRIX_CTRL", "HIDDEN", LV_BTNMATRIX_CTRL_HIDDEN, "NO_REPEAT", LV_BTNMATRIX_CTRL_NO_REPEAT, "DISABLED", LV_BTNMATRIX_CTRL_DISABLED, "CHECKABLE", LV_BTNMATRIX_CTRL_CHECKABLE, "CHECK_STATE", LV_BTNMATRIX_CTRL_CHECK_STATE, "CLICK_TRIG", LV_BTNMATRIX_CTRL_CLICK_TRIG, NULL));
    PyModule_AddObject(module, "BTNMATRIX_PART", build_constclass('d', "BTNMATRIX_PART", "BG", LV_BTNMATRIX_PART_BG, "BTN", LV_BTNMATRIX_PART_BTN, NULL));
    PyModule_AddObject(module, "KEYBOARD_MODE", build_constclass('d', "KEYBOARD_MODE", "TEXT_LOWER", LV_KEYBOARD_MODE_TEXT_LOWER, "TEXT_UPPER", LV_KEYBOARD_MODE_TEXT_UPPER, "SPECIAL", LV_KEYBOARD_MODE_SPECIAL, "NUM", LV_KEYBOARD_MODE_NUM, NULL));
    PyModule_AddObject(module, "KEYBOARD_PART", build_constclass('d', "KEYBOARD_PART", "BG", LV_KEYBOARD_PART_BG, "BTN", LV_KEYBOARD_PART_BTN, NULL));
    PyModule_AddObject(module, "DROPDOWN_DIR", build_constclass('d', "DROPDOWN_DIR", "DOWN", LV_DROPDOWN_DIR_DOWN, "UP", LV_DROPDOWN_DIR_UP, "LEFT", LV_DROPDOWN_DIR_LEFT, "RIGHT", LV_DROPDOWN_DIR_RIGHT, NULL));
    PyModule_AddObject(module, "DROPDOWN_PART", build_constclass('d', "DROPDOWN_PART", "MAIN", LV_DROPDOWN_PART_MAIN, "LIST", LV_DROPDOWN_PART_LIST, "SCROLLBAR", LV_DROPDOWN_PART_SCROLLBAR, "SELECTED", LV_DROPDOWN_PART_SELECTED, NULL));
    PyModule_AddObject(module, "ROLLER_MODE", build_constclass('d', "ROLLER_MODE", "NORMAL", LV_ROLLER_MODE_NORMAL, "INFINITE", LV_ROLLER_MODE_INFINITE, NULL));
    PyModule_AddObject(module, "ROLLER_PART", build_constclass('d', "ROLLER_PART", "BG", LV_ROLLER_PART_BG, "SELECTED", LV_ROLLER_PART_SELECTED, NULL));
    PyModule_AddObject(module, "TEXTAREA_PART", build_constclass('d', "TEXTAREA_PART", "BG", LV_TEXTAREA_PART_BG, "SCROLLBAR", LV_TEXTAREA_PART_SCROLLBAR, "EDGE_FLASH", LV_TEXTAREA_PART_EDGE_FLASH, "CURSOR", LV_TEXTAREA_PART_CURSOR, "PLACEHOLDER", LV_TEXTAREA_PART_PLACEHOLDER, NULL));
    PyModule_AddObject(module, "CANVAS_PART", build_constclass('d', "CANVAS_PART", "MAIN", LV_CANVAS_PART_MAIN, NULL));
    PyModule_AddObject(module, "TABVIEW_TAB_POS", build_constclass('d', "TABVIEW_TAB_POS", "NONE", LV_TABVIEW_TAB_POS_NONE, "TOP", LV_TABVIEW_TAB_POS_TOP, "BOTTOM", LV_TABVIEW_TAB_POS_BOTTOM, "LEFT", LV_TABVIEW_TAB_POS_LEFT, "RIGHT", LV_TABVIEW_TAB_POS_RIGHT, NULL));
    PyModule_AddObject(module, "TABVIEW_PART", build_constclass('d', "TABVIEW_PART", "BG", LV_TABVIEW_PART_BG, "BG_SCROLLABLE", LV_TABVIEW_PART_BG_SCROLLABLE, "TAB_BG", LV_TABVIEW_PART_TAB_BG, "TAB_BTN", LV_TABVIEW_PART_TAB_BTN, "INDIC", LV_TABVIEW_PART_INDIC, NULL));
    PyModule_AddObject(module, "MSGBOX_PART", build_constclass('d', "MSGBOX_PART", "BG", LV_MSGBOX_PART_BG, "BTN_BG", LV_MSGBOX_PART_BTN_BG, "BTN", LV_MSGBOX_PART_BTN, NULL));
    PyModule_AddObject(module, "OBJMASK_PART", build_constclass('d', "OBJMASK_PART", "MAIN", LV_OBJMASK_PART_MAIN, NULL));
    PyModule_AddObject(module, "LINEMETER_PART", build_constclass('d', "LINEMETER_PART", "MAIN", LV_LINEMETER_PART_MAIN, NULL));
    PyModule_AddObject(module, "GAUGE_PART", build_constclass('d', "GAUGE_PART", "MAIN", LV_GAUGE_PART_MAIN, "MAJOR", LV_GAUGE_PART_MAJOR, "NEEDLE", LV_GAUGE_PART_NEEDLE, NULL));
    PyModule_AddObject(module, "SWITCH_PART", build_constclass('d', "SWITCH_PART", "BG", LV_SWITCH_PART_BG, "INDIC", LV_SWITCH_PART_INDIC, "KNOB", LV_SWITCH_PART_KNOB, NULL));
    PyModule_AddObject(module, "ARC_TYPE", build_constclass('d', "ARC_TYPE", "NORMAL", LV_ARC_TYPE_NORMAL, "SYMMETRIC", LV_ARC_TYPE_SYMMETRIC, "REVERSE", LV_ARC_TYPE_REVERSE, NULL));
    PyModule_AddObject(module, "ARC_PART", build_constclass('d', "ARC_PART", "BG", LV_ARC_PART_BG, "INDIC", LV_ARC_PART_INDIC, "KNOB", LV_ARC_PART_KNOB, NULL));
    PyModule_AddObject(module, "SPINNER_TYPE", build_constclass('d', "SPINNER_TYPE", "SPINNING_ARC", LV_SPINNER_TYPE_SPINNING_ARC, "FILLSPIN_ARC", LV_SPINNER_TYPE_FILLSPIN_ARC, "CONSTANT_ARC", LV_SPINNER_TYPE_CONSTANT_ARC, NULL));
    PyModule_AddObject(module, "SPINNER_DIR", build_constclass('d', "SPINNER_DIR", "FORWARD", LV_SPINNER_DIR_FORWARD, "BACKWARD", LV_SPINNER_DIR_BACKWARD, NULL));
    PyModule_AddObject(module, "SPINNER_PART", build_constclass('d', "SPINNER_PART", "BG", LV_SPINNER_PART_BG, "INDIC", LV_SPINNER_PART_INDIC, NULL));
    PyModule_AddObject(module, "CALENDAR_PART", build_constclass('d', "CALENDAR_PART", "BG", LV_CALENDAR_PART_BG, "HEADER", LV_CALENDAR_PART_HEADER, "DAY_NAMES", LV_CALENDAR_PART_DAY_NAMES, "DATE", LV_CALENDAR_PART_DATE, NULL));
    PyModule_AddObject(module, "SPINBOX_PART", build_constclass('d', "SPINBOX_PART", "BG", LV_SPINBOX_PART_BG, "CURSOR", LV_SPINBOX_PART_CURSOR, NULL));


    PyModule_AddObject(module, "SYMBOL", build_constclass('s', "SYMBOL", "AUDIO", LV_SYMBOL_AUDIO, "VIDEO", LV_SYMBOL_VIDEO, "LIST", LV_SYMBOL_LIST, "OK", LV_SYMBOL_OK, "CLOSE", LV_SYMBOL_CLOSE, "POWER", LV_SYMBOL_POWER, "SETTINGS", LV_SYMBOL_SETTINGS, "HOME", LV_SYMBOL_HOME, "DOWNLOAD", LV_SYMBOL_DOWNLOAD, "DRIVE", LV_SYMBOL_DRIVE, "REFRESH", LV_SYMBOL_REFRESH, "MUTE", LV_SYMBOL_MUTE, "VOLUME_MID", LV_SYMBOL_VOLUME_MID, "VOLUME_MAX", LV_SYMBOL_VOLUME_MAX, "IMAGE", LV_SYMBOL_IMAGE, "EDIT", LV_SYMBOL_EDIT, "PREV", LV_SYMBOL_PREV, "PLAY", LV_SYMBOL_PLAY, "PAUSE", LV_SYMBOL_PAUSE, "STOP", LV_SYMBOL_STOP, "NEXT", LV_SYMBOL_NEXT, "EJECT", LV_SYMBOL_EJECT, "LEFT", LV_SYMBOL_LEFT, "RIGHT", LV_SYMBOL_RIGHT, "PLUS", LV_SYMBOL_PLUS, "MINUS", LV_SYMBOL_MINUS, "EYE_OPEN", LV_SYMBOL_EYE_OPEN, "EYE_CLOSE", LV_SYMBOL_EYE_CLOSE, "WARNING", LV_SYMBOL_WARNING, "SHUFFLE", LV_SYMBOL_SHUFFLE, "UP", LV_SYMBOL_UP, "DOWN", LV_SYMBOL_DOWN, "LOOP", LV_SYMBOL_LOOP, "DIRECTORY", LV_SYMBOL_DIRECTORY, "UPLOAD", LV_SYMBOL_UPLOAD, "CALL", LV_SYMBOL_CALL, "CUT", LV_SYMBOL_CUT, "COPY", LV_SYMBOL_COPY, "SAVE", LV_SYMBOL_SAVE, "CHARGE", LV_SYMBOL_CHARGE, "PASTE", LV_SYMBOL_PASTE, "BELL", LV_SYMBOL_BELL, "KEYBOARD", LV_SYMBOL_KEYBOARD, "GPS", LV_SYMBOL_GPS, "FILE", LV_SYMBOL_FILE, "WIFI", LV_SYMBOL_WIFI, "BATTERY_FULL", LV_SYMBOL_BATTERY_FULL, "BATTERY_3", LV_SYMBOL_BATTERY_3, "BATTERY_2", LV_SYMBOL_BATTERY_2, "BATTERY_1", LV_SYMBOL_BATTERY_1, "BATTERY_EMPTY", LV_SYMBOL_BATTERY_EMPTY, "USB", LV_SYMBOL_USB, "BLUETOOTH", LV_SYMBOL_BLUETOOTH, "TRASH", LV_SYMBOL_TRASH, "BACKSPACE", LV_SYMBOL_BACKSPACE, "SD_CARD", LV_SYMBOL_SD_CARD, "NEW_LINE", LV_SYMBOL_NEW_LINE, "DUMMY", LV_SYMBOL_DUMMY, "BULLET", LV_SYMBOL_BULLET, NULL));


    PyModule_AddObject(module, "COLOR", build_constclass('C', "COLOR", "WHITE", LV_COLOR_WHITE, "SILVER", LV_COLOR_SILVER, "GRAY", LV_COLOR_GRAY, "BLACK", LV_COLOR_BLACK, "RED", LV_COLOR_RED, "MAROON", LV_COLOR_MAROON, "YELLOW", LV_COLOR_YELLOW, "OLIVE", LV_COLOR_OLIVE, "LIME", LV_COLOR_LIME, "GREEN", LV_COLOR_GREEN, "CYAN", LV_COLOR_CYAN, "AQUA", LV_COLOR_AQUA, "TEAL", LV_COLOR_TEAL, "BLUE", LV_COLOR_BLUE, "NAVY", LV_COLOR_NAVY, "MAGENTA", LV_COLOR_MAGENTA, "PURPLE", LV_COLOR_PURPLE, "ORANGE", LV_COLOR_ORANGE, "SIZE", LV_COLOR_SIZE, "MIX_ROUND_OFS", LV_COLOR_MIX_ROUND_OFS, NULL));


   PyModule_AddObject(module, "font_montserrat_14", pystruct_from_c(&pylv_font_t_Type, &lv_font_montserrat_14, sizeof(lv_font_t), 0));
   PyModule_AddObject(module, "anim_path_def", pystruct_from_c(&pylv_anim_path_t_Type, &lv_anim_path_def, sizeof(lv_anim_path_t), 0));


    Py_INCREF(&Style_Type);
    PyModule_AddObject(module, "Style", (PyObject *) &Style_Type);

    // refcount for typesdict is initally 1; it is used by pyobj_from_lv
    // refcounts to py{name}_Type objects are incremented due to "O" format
    typesdict = Py_BuildValue("{sOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsO}",
        "lv_obj", &pylv_obj_Type,
        "lv_cont", &pylv_cont_Type,
        "lv_btn", &pylv_btn_Type,
        "lv_imgbtn", &pylv_imgbtn_Type,
        "lv_label", &pylv_label_Type,
        "lv_img", &pylv_img_Type,
        "lv_line", &pylv_line_Type,
        "lv_page", &pylv_page_Type,
        "lv_list", &pylv_list_Type,
        "lv_chart", &pylv_chart_Type,
        "lv_table", &pylv_table_Type,
        "lv_checkbox", &pylv_checkbox_Type,
        "lv_cpicker", &pylv_cpicker_Type,
        "lv_bar", &pylv_bar_Type,
        "lv_slider", &pylv_slider_Type,
        "lv_led", &pylv_led_Type,
        "lv_btnmatrix", &pylv_btnmatrix_Type,
        "lv_keyboard", &pylv_keyboard_Type,
        "lv_dropdown", &pylv_dropdown_Type,
        "lv_roller", &pylv_roller_Type,
        "lv_textarea", &pylv_textarea_Type,
        "lv_canvas", &pylv_canvas_Type,
        "lv_win", &pylv_win_Type,
        "lv_tabview", &pylv_tabview_Type,
        "lv_tileview", &pylv_tileview_Type,
        "lv_msgbox", &pylv_msgbox_Type,
        "lv_objmask", &pylv_objmask_Type,
        "lv_linemeter", &pylv_linemeter_Type,
        "lv_gauge", &pylv_gauge_Type,
        "lv_switch", &pylv_switch_Type,
        "lv_arc", &pylv_arc_Type,
        "lv_spinner", &pylv_spinner_Type,
        "lv_calendar", &pylv_calendar_Type,
        "lv_spinbox", &pylv_spinbox_Type);
    
    PyModule_AddObject(module, "framebuffer", PyMemoryView_FromMemory(framebuffer, LV_HOR_RES_MAX * LV_VER_RES_MAX * 2, PyBUF_READ));
    PyModule_AddObject(module, "HOR_RES", PyLong_FromLong(LV_HOR_RES_MAX));
    PyModule_AddObject(module, "VER_RES", PyLong_FromLong(LV_VER_RES_MAX));


    lv_disp_drv_init(&display_driver);
    display_driver.hor_res = LV_HOR_RES_MAX;
    display_driver.ver_res = LV_VER_RES_MAX;
    
    display_driver.flush_cb = disp_flush;
    
    lv_disp_buf_init(&disp_buffer,disp_buf1, NULL, sizeof(disp_buf1)/sizeof(lv_color_t));
    display_driver.buffer = &disp_buffer;

    lv_init();
    
    lv_disp_drv_register(&display_driver);

    return module;
    
error:
    Py_XDECREF(module);
    return NULL;
}

