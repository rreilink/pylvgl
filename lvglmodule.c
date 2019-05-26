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

typedef pylv_Btn pylv_Cb;

typedef pylv_Obj pylv_Bar;

typedef pylv_Bar pylv_Slider;

typedef pylv_Obj pylv_Led;

typedef pylv_Obj pylv_Btnm;

typedef pylv_Btnm pylv_Kb;

typedef pylv_Page pylv_Ddlist;

typedef pylv_Ddlist pylv_Roller;

typedef pylv_Page pylv_Ta;

typedef pylv_Img pylv_Canvas;

typedef pylv_Obj pylv_Win;

typedef pylv_Obj pylv_Tabview;

typedef pylv_Page pylv_Tileview;

typedef pylv_Cont pylv_Mbox;

typedef pylv_Obj pylv_Lmeter;

typedef pylv_Lmeter pylv_Gauge;

typedef pylv_Slider pylv_Sw;

typedef pylv_Obj pylv_Arc;

typedef pylv_Arc pylv_Preload;

typedef pylv_Obj pylv_Calendar;

typedef pylv_Ta pylv_Spinbox;



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

static PyTypeObject pylv_cb_Type;

static PyTypeObject pylv_bar_Type;

static PyTypeObject pylv_slider_Type;

static PyTypeObject pylv_led_Type;

static PyTypeObject pylv_btnm_Type;

static PyTypeObject pylv_kb_Type;

static PyTypeObject pylv_ddlist_Type;

static PyTypeObject pylv_roller_Type;

static PyTypeObject pylv_ta_Type;

static PyTypeObject pylv_canvas_Type;

static PyTypeObject pylv_win_Type;

static PyTypeObject pylv_tabview_Type;

static PyTypeObject pylv_tileview_Type;

static PyTypeObject pylv_mbox_Type;

static PyTypeObject pylv_lmeter_Type;

static PyTypeObject pylv_gauge_Type;

static PyTypeObject pylv_sw_Type;

static PyTypeObject pylv_arc_Type;

static PyTypeObject pylv_preload_Type;

static PyTypeObject pylv_calendar_Type;

static PyTypeObject pylv_spinbox_Type;



static PyTypeObject pylv_mem_monitor_t_Type;

static PyTypeObject pylv_ll_t_Type;

static PyTypeObject pylv_task_t_Type;

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

static PyTypeObject pylv_font_unicode_map_t_Type;

static PyTypeObject pylv_font_t_Type;

static PyTypeObject pylv_anim_t_Type;

static PyTypeObject pylv_style_t_Type;

static PyTypeObject pylv_style_anim_dsc_t_Type;

static PyTypeObject pylv_reailgn_t_Type;

static PyTypeObject pylv_obj_t_Type;

static PyTypeObject pylv_obj_type_t_Type;

static PyTypeObject pylv_group_t_Type;

static PyTypeObject pylv_theme_t_Type;

static PyTypeObject pylv_cont_ext_t_Type;

static PyTypeObject pylv_btn_ext_t_Type;

static PyTypeObject pylv_fs_file_t_Type;

static PyTypeObject pylv_fs_dir_t_Type;

static PyTypeObject pylv_fs_drv_t_Type;

static PyTypeObject pylv_img_header_t_Type;

static PyTypeObject pylv_img_dsc_t_Type;

static PyTypeObject pylv_img_decoder_t_Type;

static PyTypeObject pylv_img_decoder_dsc_t_Type;

static PyTypeObject pylv_imgbtn_ext_t_Type;

static PyTypeObject pylv_label_ext_t_Type;

static PyTypeObject pylv_img_ext_t_Type;

static PyTypeObject pylv_line_ext_t_Type;

static PyTypeObject pylv_page_ext_t_Type;

static PyTypeObject pylv_list_ext_t_Type;

static PyTypeObject pylv_chart_series_t_Type;

static PyTypeObject pylv_chart_axis_cfg_t_Type;

static PyTypeObject pylv_chart_ext_t_Type;

static PyTypeObject pylv_table_cell_format_t_Type;

static PyTypeObject pylv_table_ext_t_Type;

static PyTypeObject pylv_cb_ext_t_Type;

static PyTypeObject pylv_bar_ext_t_Type;

static PyTypeObject pylv_slider_ext_t_Type;

static PyTypeObject pylv_led_ext_t_Type;

static PyTypeObject pylv_btnm_ext_t_Type;

static PyTypeObject pylv_kb_ext_t_Type;

static PyTypeObject pylv_ddlist_ext_t_Type;

static PyTypeObject pylv_roller_ext_t_Type;

static PyTypeObject pylv_ta_ext_t_Type;

static PyTypeObject pylv_canvas_ext_t_Type;

static PyTypeObject pylv_win_ext_t_Type;

static PyTypeObject pylv_tabview_ext_t_Type;

static PyTypeObject pylv_tileview_ext_t_Type;

static PyTypeObject pylv_mbox_ext_t_Type;

static PyTypeObject pylv_lmeter_ext_t_Type;

static PyTypeObject pylv_gauge_ext_t_Type;

static PyTypeObject pylv_sw_ext_t_Type;

static PyTypeObject pylv_arc_ext_t_Type;

static PyTypeObject pylv_preload_ext_t_Type;

static PyTypeObject pylv_calendar_date_t_Type;

static PyTypeObject pylv_calendar_ext_t_Type;

static PyTypeObject pylv_spinbox_ext_t_Type;

static PyTypeObject pylv_color8_t_ch_Type;

static PyTypeObject pylv_color16_t_ch_Type;

static PyTypeObject pylv_color32_t_ch_Type;

static PyTypeObject pylv_indev_proc_t_types_Type;

static PyTypeObject pylv_indev_proc_t_types_pointer_Type;

static PyTypeObject pylv_indev_proc_t_types_keypad_Type;

static PyTypeObject pylv_style_t_body_Type;

static PyTypeObject pylv_style_t_body_border_Type;

static PyTypeObject pylv_style_t_body_shadow_Type;

static PyTypeObject pylv_style_t_body_padding_Type;

static PyTypeObject pylv_style_t_text_Type;

static PyTypeObject pylv_style_t_image_Type;

static PyTypeObject pylv_style_t_line_Type;

static PyTypeObject pylv_theme_t_style_Type;

static PyTypeObject pylv_theme_t_style_btn_Type;

static PyTypeObject pylv_theme_t_style_imgbtn_Type;

static PyTypeObject pylv_theme_t_style_label_Type;

static PyTypeObject pylv_theme_t_style_img_Type;

static PyTypeObject pylv_theme_t_style_line_Type;

static PyTypeObject pylv_theme_t_style_bar_Type;

static PyTypeObject pylv_theme_t_style_slider_Type;

static PyTypeObject pylv_theme_t_style_sw_Type;

static PyTypeObject pylv_theme_t_style_calendar_Type;

static PyTypeObject pylv_theme_t_style_cb_Type;

static PyTypeObject pylv_theme_t_style_cb_box_Type;

static PyTypeObject pylv_theme_t_style_btnm_Type;

static PyTypeObject pylv_theme_t_style_btnm_btn_Type;

static PyTypeObject pylv_theme_t_style_kb_Type;

static PyTypeObject pylv_theme_t_style_kb_btn_Type;

static PyTypeObject pylv_theme_t_style_mbox_Type;

static PyTypeObject pylv_theme_t_style_mbox_btn_Type;

static PyTypeObject pylv_theme_t_style_page_Type;

static PyTypeObject pylv_theme_t_style_ta_Type;

static PyTypeObject pylv_theme_t_style_spinbox_Type;

static PyTypeObject pylv_theme_t_style_list_Type;

static PyTypeObject pylv_theme_t_style_list_btn_Type;

static PyTypeObject pylv_theme_t_style_ddlist_Type;

static PyTypeObject pylv_theme_t_style_roller_Type;

static PyTypeObject pylv_theme_t_style_tabview_Type;

static PyTypeObject pylv_theme_t_style_tabview_btn_Type;

static PyTypeObject pylv_theme_t_style_tileview_Type;

static PyTypeObject pylv_theme_t_style_table_Type;

static PyTypeObject pylv_theme_t_style_win_Type;

static PyTypeObject pylv_theme_t_style_win_content_Type;

static PyTypeObject pylv_theme_t_style_win_btn_Type;

static PyTypeObject pylv_theme_t_group_Type;

static PyTypeObject pylv_label_ext_t_dot_Type;

static PyTypeObject pylv_page_ext_t_sb_Type;

static PyTypeObject pylv_page_ext_t_edge_flash_Type;

static PyTypeObject pylv_chart_ext_t_series_Type;

static PyTypeObject pylv_table_cell_format_t_s_Type;

static PyTypeObject pylv_ta_ext_t_cursor_Type;


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
    pylv_Obj* py_obj = (pylv_Obj*)(*lv_obj_get_user_data_ptr(obj));
    
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
            (*lv_obj_get_user_data_ptr(obj)) = NULL;
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
    
    pyobj = *lv_obj_get_user_data_ptr(obj);
    
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
        *lv_obj_get_user_data_ptr(obj) = pyobj;
        install_signal_cb(pyobj);
        // reference count for pyobj is 1 -- the reference stored in the lvgl object user_data
    }

    Py_INCREF(pyobj); // increase reference count of returned object
    
    return (PyObject *)pyobj;
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
    void *data;
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
    *(lv_mem_monitor_t **)target = ((StructObject*)obj) -> data;
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
    *(lv_ll_t **)target = ((StructObject*)obj) -> data;
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



static PyObject *
get_struct_bitfield_task_t_once(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_task_t*)(self->data))->once );
}

static int
set_struct_bitfield_task_t_once(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_task_t*)(self->data))->once = v;
    return 0;
}

static PyGetSetDef pylv_task_t_getset[] = {
    {"period", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t period", (void*)offsetof(lv_task_t, period)},
    {"last_run", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t last_run", (void*)offsetof(lv_task_t, last_run)},
    {"task_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_task_cb_t task_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_task_t, task_cb), sizeof(((lv_task_t *)0)->task_cb)})},
    {"user_data", (getter) struct_get_struct, (setter) struct_set_struct, "void user_data", & ((struct_closure_t){ &Blob_Type, offsetof(lv_task_t, user_data), sizeof(((lv_task_t *)0)->user_data)})},
    {"prio", (getter) get_struct_bitfield_task_t_prio, (setter) set_struct_bitfield_task_t_prio, "uint8_t:3 prio", NULL},
    {"once", (getter) get_struct_bitfield_task_t_once, (setter) set_struct_bitfield_task_t_once, "uint8_t:1 once", NULL},
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
    *(lv_task_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_color1_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_color1_t_Type, sizeof(lv_color1_t));
}



static PyObject *
get_struct_bitfield_color1_t_blue(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_color1_t*)(self->data))->blue );
}

static int
set_struct_bitfield_color1_t_blue(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_color1_t*)(self->data))->blue = v;
    return 0;
}



static PyObject *
get_struct_bitfield_color1_t_green(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_color1_t*)(self->data))->green );
}

static int
set_struct_bitfield_color1_t_green(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_color1_t*)(self->data))->green = v;
    return 0;
}



static PyObject *
get_struct_bitfield_color1_t_red(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_color1_t*)(self->data))->red );
}

static int
set_struct_bitfield_color1_t_red(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_color1_t*)(self->data))->red = v;
    return 0;
}



static PyObject *
get_struct_bitfield_color1_t_full(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_color1_t*)(self->data))->full );
}

static int
set_struct_bitfield_color1_t_full(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_color1_t*)(self->data))->full = v;
    return 0;
}

static PyGetSetDef pylv_color1_t_getset[] = {
    {"blue", (getter) get_struct_bitfield_color1_t_blue, (setter) set_struct_bitfield_color1_t_blue, "uint8_t:1 blue", NULL},
    {"green", (getter) get_struct_bitfield_color1_t_green, (setter) set_struct_bitfield_color1_t_green, "uint8_t:1 green", NULL},
    {"red", (getter) get_struct_bitfield_color1_t_red, (setter) set_struct_bitfield_color1_t_red, "uint8_t:1 red", NULL},
    {"full", (getter) get_struct_bitfield_color1_t_full, (setter) set_struct_bitfield_color1_t_full, "uint8_t:1 full", NULL},
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
    *(lv_color1_t **)target = ((StructObject*)obj) -> data;
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
    *(lv_color8_t **)target = ((StructObject*)obj) -> data;
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
    *(lv_color16_t **)target = ((StructObject*)obj) -> data;
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
    *(lv_color32_t **)target = ((StructObject*)obj) -> data;
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
    *(lv_color_hsv_t **)target = ((StructObject*)obj) -> data;
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
    *(lv_point_t **)target = ((StructObject*)obj) -> data;
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
    *(lv_area_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_disp_buf_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_disp_buf_t_Type, sizeof(lv_disp_buf_t));
}



static PyObject *
get_struct_bitfield_disp_buf_t_flushing(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_disp_buf_t*)(self->data))->flushing );
}

static int
set_struct_bitfield_disp_buf_t_flushing(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_disp_buf_t*)(self->data))->flushing = v;
    return 0;
}

static PyGetSetDef pylv_disp_buf_t_getset[] = {
    {"buf1", (getter) struct_get_struct, (setter) struct_set_struct, "void buf1", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_buf_t, buf1), sizeof(((lv_disp_buf_t *)0)->buf1)})},
    {"buf2", (getter) struct_get_struct, (setter) struct_set_struct, "void buf2", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_buf_t, buf2), sizeof(((lv_disp_buf_t *)0)->buf2)})},
    {"buf_act", (getter) struct_get_struct, (setter) struct_set_struct, "void buf_act", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_buf_t, buf_act), sizeof(((lv_disp_buf_t *)0)->buf_act)})},
    {"size", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t size", (void*)offsetof(lv_disp_buf_t, size)},
    {"area", (getter) struct_get_struct, (setter) struct_set_struct, "lv_area_t area", & ((struct_closure_t){ &pylv_area_t_Type, offsetof(lv_disp_buf_t, area), sizeof(lv_area_t)})},
    {"flushing", (getter) get_struct_bitfield_disp_buf_t_flushing, (setter) set_struct_bitfield_disp_buf_t_flushing, "uint32_t:1 flushing", NULL},
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
    *(lv_disp_buf_t **)target = ((StructObject*)obj) -> data;
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
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_disp_drv_t*)(self->data))->rotated = v;
    return 0;
}

static PyGetSetDef pylv_disp_drv_t_getset[] = {
    {"hor_res", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t hor_res", (void*)offsetof(lv_disp_drv_t, hor_res)},
    {"ver_res", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t ver_res", (void*)offsetof(lv_disp_drv_t, ver_res)},
    {"buffer", (getter) struct_get_struct, (setter) struct_set_struct, "lv_disp_buf_t buffer", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_drv_t, buffer), sizeof(((lv_disp_drv_t *)0)->buffer)})},
    {"antialiasing", (getter) get_struct_bitfield_disp_drv_t_antialiasing, (setter) set_struct_bitfield_disp_drv_t_antialiasing, "uint32_t:1 antialiasing", NULL},
    {"rotated", (getter) get_struct_bitfield_disp_drv_t_rotated, (setter) set_struct_bitfield_disp_drv_t_rotated, "uint32_t:1 rotated", NULL},
    {"flush_cb", (getter) struct_get_struct, (setter) struct_set_struct, "void flush_cb(struct _disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) flush_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_drv_t, flush_cb), sizeof(((lv_disp_drv_t *)0)->flush_cb)})},
    {"rounder_cb", (getter) struct_get_struct, (setter) struct_set_struct, "void rounder_cb(struct _disp_drv_t *disp_drv, lv_area_t *area) rounder_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_drv_t, rounder_cb), sizeof(((lv_disp_drv_t *)0)->rounder_cb)})},
    {"set_px_cb", (getter) struct_get_struct, (setter) struct_set_struct, "void set_px_cb(struct _disp_drv_t *disp_drv, uint8_t *buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa) set_px_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_drv_t, set_px_cb), sizeof(((lv_disp_drv_t *)0)->set_px_cb)})},
    {"monitor_cb", (getter) struct_get_struct, (setter) struct_set_struct, "void monitor_cb(struct _disp_drv_t *disp_drv, uint32_t time, uint32_t px) monitor_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_drv_t, monitor_cb), sizeof(((lv_disp_drv_t *)0)->monitor_cb)})},
    {"mem_blend_cb", (getter) struct_get_struct, (setter) struct_set_struct, "void mem_blend_cb(struct _disp_drv_t *disp_drv, lv_color_t *dest, const lv_color_t *src, uint32_t length, lv_opa_t opa) mem_blend_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_drv_t, mem_blend_cb), sizeof(((lv_disp_drv_t *)0)->mem_blend_cb)})},
    {"mem_fill_cb", (getter) struct_get_struct, (setter) struct_set_struct, "void mem_fill_cb(struct _disp_drv_t *disp_drv, lv_color_t *dest_buf, const lv_area_t *dest_area, const lv_area_t *fill_area, lv_color_t color) mem_fill_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_drv_t, mem_fill_cb), sizeof(((lv_disp_drv_t *)0)->mem_fill_cb)})},
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
    *(lv_disp_drv_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_disp_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_disp_t_Type, sizeof(lv_disp_t));
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
    {"top_layer", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_obj_t top_layer", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_t, top_layer), sizeof(((lv_disp_t *)0)->top_layer)})},
    {"sys_layer", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_obj_t sys_layer", & ((struct_closure_t){ &Blob_Type, offsetof(lv_disp_t, sys_layer), sizeof(((lv_disp_t *)0)->sys_layer)})},
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
    *(lv_disp_t **)target = ((StructObject*)obj) -> data;
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
    *(lv_indev_data_t **)target = ((StructObject*)obj) -> data;
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
    *(lv_indev_drv_t **)target = ((StructObject*)obj) -> data;
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
    {"types", (getter) struct_get_struct, (setter) struct_set_struct, "union  {   struct    {     lv_point_t act_point;     lv_point_t last_point;     lv_point_t vect;     lv_point_t drag_sum;     lv_point_t drag_throw_vect;     struct _lv_obj_t *act_obj;     struct _lv_obj_t *last_obj;     struct _lv_obj_t *last_pressed;     uint8_t drag_limit_out : 1;     uint8_t drag_in_prog : 1;   } pointer;   struct    {     lv_indev_state_t last_state;     uint32_t last_key;   } keypad; } types", & ((struct_closure_t){ &pylv_indev_proc_t_types_Type, offsetof(lv_indev_proc_t, types), sizeof(((lv_indev_proc_t *)0)->types)})},
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
    *(lv_indev_proc_t **)target = ((StructObject*)obj) -> data;
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
    *(lv_indev_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_font_glyph_dsc_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_font_glyph_dsc_t_Type, sizeof(lv_font_glyph_dsc_t));
}



static PyObject *
get_struct_bitfield_font_glyph_dsc_t_w_px(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_font_glyph_dsc_t*)(self->data))->w_px );
}

static int
set_struct_bitfield_font_glyph_dsc_t_w_px(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 255)) return -1;
    ((lv_font_glyph_dsc_t*)(self->data))->w_px = v;
    return 0;
}



static PyObject *
get_struct_bitfield_font_glyph_dsc_t_glyph_index(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_font_glyph_dsc_t*)(self->data))->glyph_index );
}

static int
set_struct_bitfield_font_glyph_dsc_t_glyph_index(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 16777215)) return -1;
    ((lv_font_glyph_dsc_t*)(self->data))->glyph_index = v;
    return 0;
}

static PyGetSetDef pylv_font_glyph_dsc_t_getset[] = {
    {"w_px", (getter) get_struct_bitfield_font_glyph_dsc_t_w_px, (setter) set_struct_bitfield_font_glyph_dsc_t_w_px, "uint32_t:8 w_px", NULL},
    {"glyph_index", (getter) get_struct_bitfield_font_glyph_dsc_t_glyph_index, (setter) set_struct_bitfield_font_glyph_dsc_t_glyph_index, "uint32_t:24 glyph_index", NULL},
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
    *(lv_font_glyph_dsc_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_font_unicode_map_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_font_unicode_map_t_Type, sizeof(lv_font_unicode_map_t));
}



static PyObject *
get_struct_bitfield_font_unicode_map_t_unicode(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_font_unicode_map_t*)(self->data))->unicode );
}

static int
set_struct_bitfield_font_unicode_map_t_unicode(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 2097151)) return -1;
    ((lv_font_unicode_map_t*)(self->data))->unicode = v;
    return 0;
}



static PyObject *
get_struct_bitfield_font_unicode_map_t_glyph_dsc_index(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_font_unicode_map_t*)(self->data))->glyph_dsc_index );
}

static int
set_struct_bitfield_font_unicode_map_t_glyph_dsc_index(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 2047)) return -1;
    ((lv_font_unicode_map_t*)(self->data))->glyph_dsc_index = v;
    return 0;
}

static PyGetSetDef pylv_font_unicode_map_t_getset[] = {
    {"unicode", (getter) get_struct_bitfield_font_unicode_map_t_unicode, (setter) set_struct_bitfield_font_unicode_map_t_unicode, "uint32_t:21 unicode", NULL},
    {"glyph_dsc_index", (getter) get_struct_bitfield_font_unicode_map_t_glyph_dsc_index, (setter) set_struct_bitfield_font_unicode_map_t_glyph_dsc_index, "uint32_t:11 glyph_dsc_index", NULL},
    {NULL}
};


static PyTypeObject pylv_font_unicode_map_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.font_unicode_map_t",
    .tp_doc = "lvgl font_unicode_map_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_font_unicode_map_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_font_unicode_map_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_font_unicode_map_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_font_unicode_map_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_font_unicode_map_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_font_unicode_map_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_font_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_font_t_Type, sizeof(lv_font_t));
}



static PyObject *
get_struct_bitfield_font_t_h_px(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_font_t*)(self->data))->h_px );
}

static int
set_struct_bitfield_font_t_h_px(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 255)) return -1;
    ((lv_font_t*)(self->data))->h_px = v;
    return 0;
}



static PyObject *
get_struct_bitfield_font_t_bpp(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_font_t*)(self->data))->bpp );
}

static int
set_struct_bitfield_font_t_bpp(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 15)) return -1;
    ((lv_font_t*)(self->data))->bpp = v;
    return 0;
}



static PyObject *
get_struct_bitfield_font_t_monospace(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_font_t*)(self->data))->monospace );
}

static int
set_struct_bitfield_font_t_monospace(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 255)) return -1;
    ((lv_font_t*)(self->data))->monospace = v;
    return 0;
}

static PyGetSetDef pylv_font_t_getset[] = {
    {"unicode_first", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t unicode_first", (void*)offsetof(lv_font_t, unicode_first)},
    {"unicode_last", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t unicode_last", (void*)offsetof(lv_font_t, unicode_last)},
    {"glyph_bitmap", (getter) struct_get_struct, (setter) struct_set_struct, "uint8_t glyph_bitmap", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_t, glyph_bitmap), sizeof(((lv_font_t *)0)->glyph_bitmap)})},
    {"glyph_dsc", (getter) struct_get_struct, (setter) struct_set_struct, "lv_font_glyph_dsc_t glyph_dsc", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_t, glyph_dsc), sizeof(((lv_font_t *)0)->glyph_dsc)})},
    {"unicode_list", (getter) struct_get_struct, (setter) struct_set_struct, "uint32_t unicode_list", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_t, unicode_list), sizeof(((lv_font_t *)0)->unicode_list)})},
    {"get_bitmap", (getter) struct_get_struct, (setter) struct_set_struct, "const uint8_t *get_bitmap(const struct _lv_font_struct *, uint32_t) get_bitmap", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_t, get_bitmap), sizeof(((lv_font_t *)0)->get_bitmap)})},
    {"get_width", (getter) struct_get_struct, (setter) struct_set_struct, "int16_t get_width(const struct _lv_font_struct *, uint32_t) get_width", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_t, get_width), sizeof(((lv_font_t *)0)->get_width)})},
    {"next_page", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_font_struct next_page", & ((struct_closure_t){ &Blob_Type, offsetof(lv_font_t, next_page), sizeof(((lv_font_t *)0)->next_page)})},
    {"h_px", (getter) get_struct_bitfield_font_t_h_px, (setter) set_struct_bitfield_font_t_h_px, "uint32_t:8 h_px", NULL},
    {"bpp", (getter) get_struct_bitfield_font_t_bpp, (setter) set_struct_bitfield_font_t_bpp, "uint32_t:4 bpp", NULL},
    {"monospace", (getter) get_struct_bitfield_font_t_monospace, (setter) set_struct_bitfield_font_t_monospace, "uint32_t:8 monospace", NULL},
    {"glyph_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t glyph_cnt", (void*)offsetof(lv_font_t, glyph_cnt)},
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
    *(lv_font_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_anim_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_anim_t_Type, sizeof(lv_anim_t));
}



static PyObject *
get_struct_bitfield_anim_t_playback(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_anim_t*)(self->data))->playback );
}

static int
set_struct_bitfield_anim_t_playback(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_anim_t*)(self->data))->playback = v;
    return 0;
}



static PyObject *
get_struct_bitfield_anim_t_repeat(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_anim_t*)(self->data))->repeat );
}

static int
set_struct_bitfield_anim_t_repeat(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_anim_t*)(self->data))->repeat = v;
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
get_struct_bitfield_anim_t_has_run(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_anim_t*)(self->data))->has_run );
}

static int
set_struct_bitfield_anim_t_has_run(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_anim_t*)(self->data))->has_run = v;
    return 0;
}

static PyGetSetDef pylv_anim_t_getset[] = {
    {"var", (getter) struct_get_struct, (setter) struct_set_struct, "void var", & ((struct_closure_t){ &Blob_Type, offsetof(lv_anim_t, var), sizeof(((lv_anim_t *)0)->var)})},
    {"exec_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_anim_exec_cb_t exec_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_anim_t, exec_cb), sizeof(((lv_anim_t *)0)->exec_cb)})},
    {"path_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_anim_path_cb_t path_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_anim_t, path_cb), sizeof(((lv_anim_t *)0)->path_cb)})},
    {"ready_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_anim_ready_cb_t ready_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_anim_t, ready_cb), sizeof(((lv_anim_t *)0)->ready_cb)})},
    {"start", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t start", (void*)offsetof(lv_anim_t, start)},
    {"end", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t end", (void*)offsetof(lv_anim_t, end)},
    {"time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t time", (void*)offsetof(lv_anim_t, time)},
    {"act_time", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t act_time", (void*)offsetof(lv_anim_t, act_time)},
    {"playback_pause", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t playback_pause", (void*)offsetof(lv_anim_t, playback_pause)},
    {"repeat_pause", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t repeat_pause", (void*)offsetof(lv_anim_t, repeat_pause)},
    {"user_data", (getter) struct_get_struct, (setter) struct_set_struct, "lv_anim_user_data_t user_data", & ((struct_closure_t){ &Blob_Type, offsetof(lv_anim_t, user_data), sizeof(((lv_anim_t *)0)->user_data)})},
    {"playback", (getter) get_struct_bitfield_anim_t_playback, (setter) set_struct_bitfield_anim_t_playback, "uint8_t:1 playback", NULL},
    {"repeat", (getter) get_struct_bitfield_anim_t_repeat, (setter) set_struct_bitfield_anim_t_repeat, "uint8_t:1 repeat", NULL},
    {"playback_now", (getter) get_struct_bitfield_anim_t_playback_now, (setter) set_struct_bitfield_anim_t_playback_now, "uint8_t:1 playback_now", NULL},
    {"has_run", (getter) get_struct_bitfield_anim_t_has_run, (setter) set_struct_bitfield_anim_t_has_run, "uint32_t:1 has_run", NULL},
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
    *(lv_anim_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_style_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_style_t_Type, sizeof(lv_style_t));
}



static PyObject *
get_struct_bitfield_style_t_glass(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_t*)(self->data))->glass );
}

static int
set_struct_bitfield_style_t_glass(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_t*)(self->data))->glass = v;
    return 0;
}

static PyGetSetDef pylv_style_t_getset[] = {
    {"glass", (getter) get_struct_bitfield_style_t_glass, (setter) set_struct_bitfield_style_t_glass, "uint8_t:1 glass", NULL},
    {"body", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_color_t main_color;   lv_color_t grad_color;   lv_coord_t radius;   lv_opa_t opa;   struct    {     lv_color_t color;     lv_coord_t width;     lv_border_part_t part;     lv_opa_t opa;   } border;   struct    {     lv_color_t color;     lv_coord_t width;     lv_shadow_type_t type;   } shadow;   struct    {     lv_coord_t top;     lv_coord_t bottom;     lv_coord_t left;     lv_coord_t right;     lv_coord_t inner;   } padding; } body", & ((struct_closure_t){ &pylv_style_t_body_Type, offsetof(lv_style_t, body), sizeof(((lv_style_t *)0)->body)})},
    {"text", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_color_t color;   lv_color_t sel_color;   const lv_font_t *font;   lv_coord_t letter_space;   lv_coord_t line_space;   lv_opa_t opa; } text", & ((struct_closure_t){ &pylv_style_t_text_Type, offsetof(lv_style_t, text), sizeof(((lv_style_t *)0)->text)})},
    {"image", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_color_t color;   lv_opa_t intense;   lv_opa_t opa; } image", & ((struct_closure_t){ &pylv_style_t_image_Type, offsetof(lv_style_t, image), sizeof(((lv_style_t *)0)->image)})},
    {"line", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_color_t color;   lv_coord_t width;   lv_opa_t opa;   uint8_t rounded : 1; } line", & ((struct_closure_t){ &pylv_style_t_line_Type, offsetof(lv_style_t, line), sizeof(((lv_style_t *)0)->line)})},
    {NULL}
};


static PyTypeObject pylv_style_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.style_t",
    .tp_doc = "lvgl style_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_style_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_style_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_style_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_style_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_style_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_style_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_style_anim_dsc_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_style_anim_dsc_t_Type, sizeof(lv_style_anim_dsc_t));
}

static PyGetSetDef pylv_style_anim_dsc_t_getset[] = {
    {"style_start", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_start", & ((struct_closure_t){ &pylv_style_t_Type, offsetof(lv_style_anim_dsc_t, style_start), sizeof(lv_style_t)})},
    {"style_end", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_end", & ((struct_closure_t){ &pylv_style_t_Type, offsetof(lv_style_anim_dsc_t, style_end), sizeof(lv_style_t)})},
    {"style_anim", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_anim", & ((struct_closure_t){ &Blob_Type, offsetof(lv_style_anim_dsc_t, style_anim), sizeof(((lv_style_anim_dsc_t *)0)->style_anim)})},
    {"ready_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_anim_ready_cb_t ready_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_style_anim_dsc_t, ready_cb), sizeof(((lv_style_anim_dsc_t *)0)->ready_cb)})},
    {NULL}
};


static PyTypeObject pylv_style_anim_dsc_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.style_anim_dsc_t",
    .tp_doc = "lvgl style_anim_dsc_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_style_anim_dsc_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_style_anim_dsc_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_style_anim_dsc_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_style_anim_dsc_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_style_anim_dsc_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_style_anim_dsc_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_reailgn_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_reailgn_t_Type, sizeof(lv_reailgn_t));
}



static PyObject *
get_struct_bitfield_reailgn_t_auto_realign(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_reailgn_t*)(self->data))->auto_realign );
}

static int
set_struct_bitfield_reailgn_t_auto_realign(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_reailgn_t*)(self->data))->auto_realign = v;
    return 0;
}



static PyObject *
get_struct_bitfield_reailgn_t_origo_align(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_reailgn_t*)(self->data))->origo_align );
}

static int
set_struct_bitfield_reailgn_t_origo_align(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_reailgn_t*)(self->data))->origo_align = v;
    return 0;
}

static PyGetSetDef pylv_reailgn_t_getset[] = {
    {"base", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_obj_t base", & ((struct_closure_t){ &Blob_Type, offsetof(lv_reailgn_t, base), sizeof(((lv_reailgn_t *)0)->base)})},
    {"xofs", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t xofs", (void*)offsetof(lv_reailgn_t, xofs)},
    {"yofs", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t yofs", (void*)offsetof(lv_reailgn_t, yofs)},
    {"align", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_align_t align", (void*)offsetof(lv_reailgn_t, align)},
    {"auto_realign", (getter) get_struct_bitfield_reailgn_t_auto_realign, (setter) set_struct_bitfield_reailgn_t_auto_realign, "uint8_t:1 auto_realign", NULL},
    {"origo_align", (getter) get_struct_bitfield_reailgn_t_origo_align, (setter) set_struct_bitfield_reailgn_t_origo_align, "uint8_t:1 origo_align", NULL},
    {NULL}
};


static PyTypeObject pylv_reailgn_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.reailgn_t",
    .tp_doc = "lvgl reailgn_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_reailgn_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_reailgn_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_reailgn_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_reailgn_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_reailgn_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_reailgn_t **)target = ((StructObject*)obj) -> data;
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
get_struct_bitfield_obj_t_opa_scale_en(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_obj_t*)(self->data))->opa_scale_en );
}

static int
set_struct_bitfield_obj_t_opa_scale_en(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_obj_t*)(self->data))->opa_scale_en = v;
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
get_struct_bitfield_obj_t_drag_dir(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_obj_t*)(self->data))->drag_dir );
}

static int
set_struct_bitfield_obj_t_drag_dir(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 3)) return -1;
    ((lv_obj_t*)(self->data))->drag_dir = v;
    return 0;
}



static PyObject *
get_struct_bitfield_obj_t_reserved(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_obj_t*)(self->data))->reserved );
}

static int
set_struct_bitfield_obj_t_reserved(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 63)) return -1;
    ((lv_obj_t*)(self->data))->reserved = v;
    return 0;
}

static PyGetSetDef pylv_obj_t_getset[] = {
    {"par", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_obj_t par", & ((struct_closure_t){ &Blob_Type, offsetof(lv_obj_t, par), sizeof(((lv_obj_t *)0)->par)})},
    {"child_ll", (getter) struct_get_struct, (setter) struct_set_struct, "lv_ll_t child_ll", & ((struct_closure_t){ &pylv_ll_t_Type, offsetof(lv_obj_t, child_ll), sizeof(lv_ll_t)})},
    {"coords", (getter) struct_get_struct, (setter) struct_set_struct, "lv_area_t coords", & ((struct_closure_t){ &pylv_area_t_Type, offsetof(lv_obj_t, coords), sizeof(lv_area_t)})},
    {"event_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_event_cb_t event_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_obj_t, event_cb), sizeof(((lv_obj_t *)0)->event_cb)})},
    {"signal_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_signal_cb_t signal_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_obj_t, signal_cb), sizeof(((lv_obj_t *)0)->signal_cb)})},
    {"design_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_design_cb_t design_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_obj_t, design_cb), sizeof(((lv_obj_t *)0)->design_cb)})},
    {"ext_attr", (getter) struct_get_struct, (setter) struct_set_struct, "void ext_attr", & ((struct_closure_t){ &Blob_Type, offsetof(lv_obj_t, ext_attr), sizeof(((lv_obj_t *)0)->ext_attr)})},
    {"style_p", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_p", & ((struct_closure_t){ &Blob_Type, offsetof(lv_obj_t, style_p), sizeof(((lv_obj_t *)0)->style_p)})},
    {"group_p", (getter) struct_get_struct, (setter) struct_set_struct, "void group_p", & ((struct_closure_t){ &Blob_Type, offsetof(lv_obj_t, group_p), sizeof(((lv_obj_t *)0)->group_p)})},
    {"click", (getter) get_struct_bitfield_obj_t_click, (setter) set_struct_bitfield_obj_t_click, "uint8_t:1 click", NULL},
    {"drag", (getter) get_struct_bitfield_obj_t_drag, (setter) set_struct_bitfield_obj_t_drag, "uint8_t:1 drag", NULL},
    {"drag_throw", (getter) get_struct_bitfield_obj_t_drag_throw, (setter) set_struct_bitfield_obj_t_drag_throw, "uint8_t:1 drag_throw", NULL},
    {"drag_parent", (getter) get_struct_bitfield_obj_t_drag_parent, (setter) set_struct_bitfield_obj_t_drag_parent, "uint8_t:1 drag_parent", NULL},
    {"hidden", (getter) get_struct_bitfield_obj_t_hidden, (setter) set_struct_bitfield_obj_t_hidden, "uint8_t:1 hidden", NULL},
    {"top", (getter) get_struct_bitfield_obj_t_top, (setter) set_struct_bitfield_obj_t_top, "uint8_t:1 top", NULL},
    {"opa_scale_en", (getter) get_struct_bitfield_obj_t_opa_scale_en, (setter) set_struct_bitfield_obj_t_opa_scale_en, "uint8_t:1 opa_scale_en", NULL},
    {"parent_event", (getter) get_struct_bitfield_obj_t_parent_event, (setter) set_struct_bitfield_obj_t_parent_event, "uint8_t:1 parent_event", NULL},
    {"drag_dir", (getter) get_struct_bitfield_obj_t_drag_dir, (setter) set_struct_bitfield_obj_t_drag_dir, "lv_drag_dir_t:2 drag_dir", NULL},
    {"reserved", (getter) get_struct_bitfield_obj_t_reserved, (setter) set_struct_bitfield_obj_t_reserved, "uint8_t:6 reserved", NULL},
    {"protect", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t protect", (void*)offsetof(lv_obj_t, protect)},
    {"opa_scale", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t opa_scale", (void*)offsetof(lv_obj_t, opa_scale)},
    {"ext_draw_pad", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t ext_draw_pad", (void*)offsetof(lv_obj_t, ext_draw_pad)},
    {"realign", (getter) struct_get_struct, (setter) struct_set_struct, "lv_reailgn_t realign", & ((struct_closure_t){ &pylv_reailgn_t_Type, offsetof(lv_obj_t, realign), sizeof(lv_reailgn_t)})},
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
    *(lv_obj_t **)target = ((StructObject*)obj) -> data;
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
    *(lv_obj_type_t **)target = ((StructObject*)obj) -> data;
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
    {"style_mod_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_group_style_mod_cb_t style_mod_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_group_t, style_mod_cb), sizeof(((lv_group_t *)0)->style_mod_cb)})},
    {"style_mod_edit_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_group_style_mod_cb_t style_mod_edit_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_group_t, style_mod_edit_cb), sizeof(((lv_group_t *)0)->style_mod_edit_cb)})},
    {"focus_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_group_focus_cb_t focus_cb", & ((struct_closure_t){ &Blob_Type, offsetof(lv_group_t, focus_cb), sizeof(((lv_group_t *)0)->focus_cb)})},
    {"style_tmp", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_tmp", & ((struct_closure_t){ &pylv_style_t_Type, offsetof(lv_group_t, style_tmp), sizeof(lv_style_t)})},
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
    *(lv_group_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_theme_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_theme_t_Type, sizeof(lv_theme_t));
}

static PyGetSetDef pylv_theme_t_getset[] = {
    {"style", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   lv_style_t *panel;   lv_style_t *cont;   struct    {     lv_style_t *rel;     lv_style_t *pr;     lv_style_t *tgl_rel;     lv_style_t *tgl_pr;     lv_style_t *ina;   } btn;   struct    {     lv_style_t *rel;     lv_style_t *pr;     lv_style_t *tgl_rel;     lv_style_t *tgl_pr;     lv_style_t *ina;   } imgbtn;   struct    {     lv_style_t *prim;     lv_style_t *sec;     lv_style_t *hint;   } label;   struct    {     lv_style_t *light;     lv_style_t *dark;   } img;   struct    {     lv_style_t *decor;   } line;   lv_style_t *led;   struct    {     lv_style_t *bg;     lv_style_t *indic;   } bar;   struct    {     lv_style_t *bg;     lv_style_t *indic;     lv_style_t *knob;   } slider;   lv_style_t *lmeter;   lv_style_t *gauge;   lv_style_t *arc;   lv_style_t *preload;   struct    {     lv_style_t *bg;     lv_style_t *indic;     lv_style_t *knob_off;     lv_style_t *knob_on;   } sw;   lv_style_t *chart;   struct    {     lv_style_t *bg;     lv_style_t *header;     lv_style_t *header_pr;     lv_style_t *day_names;     lv_style_t *highlighted_days;     lv_style_t *inactive_days;     lv_style_t *week_box;     lv_style_t *today_box;   } calendar;   struct    {     lv_style_t *bg;     struct      {       lv_style_t *rel;       lv_style_t *pr;       lv_style_t *tgl_rel;       lv_style_t *tgl_pr;       lv_style_t *ina;     } box;   } cb;   struct    {     lv_style_t *bg;     struct      {       lv_style_t *rel;       lv_style_t *pr;       lv_style_t *tgl_rel;       lv_style_t *tgl_pr;       lv_style_t *ina;     } btn;   } btnm;   struct    {     lv_style_t *bg;     struct      {       lv_style_t *rel;       lv_style_t *pr;       lv_style_t *tgl_rel;       lv_style_t *tgl_pr;       lv_style_t *ina;     } btn;   } kb;   struct    {     lv_style_t *bg;     struct      {       lv_style_t *bg;       lv_style_t *rel;       lv_style_t *pr;     } btn;   } mbox;   struct    {     lv_style_t *bg;     lv_style_t *scrl;     lv_style_t *sb;   } page;   struct    {     lv_style_t *area;     lv_style_t *oneline;     lv_style_t *cursor;     lv_style_t *sb;   } ta;   struct    {     lv_style_t *bg;     lv_style_t *cursor;     lv_style_t *sb;   } spinbox;   struct    {     lv_style_t *bg;     lv_style_t *scrl;     lv_style_t *sb;     struct      {       lv_style_t *rel;       lv_style_t *pr;       lv_style_t *tgl_rel;       lv_style_t *tgl_pr;       lv_style_t *ina;     } btn;   } list;   struct    {     lv_style_t *bg;     lv_style_t *sel;     lv_style_t *sb;   } ddlist;   struct    {     lv_style_t *bg;     lv_style_t *sel;   } roller;   struct    {     lv_style_t *bg;     lv_style_t *indic;     struct      {       lv_style_t *bg;       lv_style_t *rel;       lv_style_t *pr;       lv_style_t *tgl_rel;       lv_style_t *tgl_pr;     } btn;   } tabview;   struct    {     lv_style_t *bg;     lv_style_t *scrl;     lv_style_t *sb;   } tileview;   struct    {     lv_style_t *bg;     lv_style_t *cell;   } table;   struct    {     lv_style_t *bg;     lv_style_t *sb;     lv_style_t *header;     struct      {       lv_style_t *bg;       lv_style_t *scrl;     } content;     struct      {       lv_style_t *rel;       lv_style_t *pr;     } btn;   } win; } style", & ((struct_closure_t){ &pylv_theme_t_style_Type, offsetof(lv_theme_t, style), sizeof(((lv_theme_t *)0)->style)})},
    {"group", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_group_style_mod_cb_t style_mod_cb;   lv_group_style_mod_cb_t style_mod_edit_cb; } group", & ((struct_closure_t){ &pylv_theme_t_group_Type, offsetof(lv_theme_t, group), sizeof(((lv_theme_t *)0)->group)})},
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
    *(lv_theme_t **)target = ((StructObject*)obj) -> data;
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
    {"layout", (getter) get_struct_bitfield_cont_ext_t_layout, (setter) set_struct_bitfield_cont_ext_t_layout, "uint8_t:4 layout", NULL},
    {"fit_left", (getter) get_struct_bitfield_cont_ext_t_fit_left, (setter) set_struct_bitfield_cont_ext_t_fit_left, "uint8_t:2 fit_left", NULL},
    {"fit_right", (getter) get_struct_bitfield_cont_ext_t_fit_right, (setter) set_struct_bitfield_cont_ext_t_fit_right, "uint8_t:2 fit_right", NULL},
    {"fit_top", (getter) get_struct_bitfield_cont_ext_t_fit_top, (setter) set_struct_bitfield_cont_ext_t_fit_top, "uint8_t:2 fit_top", NULL},
    {"fit_bottom", (getter) get_struct_bitfield_cont_ext_t_fit_bottom, (setter) set_struct_bitfield_cont_ext_t_fit_bottom, "uint8_t:2 fit_bottom", NULL},
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
    *(lv_cont_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_btn_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_btn_ext_t_Type, sizeof(lv_btn_ext_t));
}



static PyObject *
get_struct_bitfield_btn_ext_t_state(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_btn_ext_t*)(self->data))->state );
}

static int
set_struct_bitfield_btn_ext_t_state(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 7)) return -1;
    ((lv_btn_ext_t*)(self->data))->state = v;
    return 0;
}



static PyObject *
get_struct_bitfield_btn_ext_t_toggle(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_btn_ext_t*)(self->data))->toggle );
}

static int
set_struct_bitfield_btn_ext_t_toggle(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_btn_ext_t*)(self->data))->toggle = v;
    return 0;
}

static PyGetSetDef pylv_btn_ext_t_getset[] = {
    {"cont", (getter) struct_get_struct, (setter) struct_set_struct, "lv_cont_ext_t cont", & ((struct_closure_t){ &pylv_cont_ext_t_Type, offsetof(lv_btn_ext_t, cont), sizeof(lv_cont_ext_t)})},
    {"styles", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_tLV_BTN_STATE_NUM styles", & ((struct_closure_t){ &Blob_Type, offsetof(lv_btn_ext_t, styles), sizeof(((lv_btn_ext_t *)0)->styles)})},
    {"ink_in_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t ink_in_time", (void*)offsetof(lv_btn_ext_t, ink_in_time)},
    {"ink_wait_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t ink_wait_time", (void*)offsetof(lv_btn_ext_t, ink_wait_time)},
    {"ink_out_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t ink_out_time", (void*)offsetof(lv_btn_ext_t, ink_out_time)},
    {"state", (getter) get_struct_bitfield_btn_ext_t_state, (setter) set_struct_bitfield_btn_ext_t_state, "lv_btn_state_t:3 state", NULL},
    {"toggle", (getter) get_struct_bitfield_btn_ext_t_toggle, (setter) set_struct_bitfield_btn_ext_t_toggle, "uint8_t:1 toggle", NULL},
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
    *(lv_btn_ext_t **)target = ((StructObject*)obj) -> data;
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
    {"drv", (getter) struct_get_struct, (setter) struct_set_struct, "struct __lv_fs_drv_t drv", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_file_t, drv), sizeof(((lv_fs_file_t *)0)->drv)})},
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
    *(lv_fs_file_t **)target = ((StructObject*)obj) -> data;
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
    {"drv", (getter) struct_get_struct, (setter) struct_set_struct, "struct __lv_fs_drv_t drv", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_dir_t, drv), sizeof(((lv_fs_dir_t *)0)->drv)})},
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
    *(lv_fs_dir_t **)target = ((StructObject*)obj) -> data;
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
    {"ready", (getter) struct_get_struct, (setter) struct_set_struct, "bool ready(void) ready", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, ready), sizeof(((lv_fs_drv_t *)0)->ready)})},
    {"open", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t open(void *file_p, const char *path, lv_fs_mode_t mode) open", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, open), sizeof(((lv_fs_drv_t *)0)->open)})},
    {"close", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t close(void *file_p) close", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, close), sizeof(((lv_fs_drv_t *)0)->close)})},
    {"remove", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t remove(const char *fn) remove", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, remove), sizeof(((lv_fs_drv_t *)0)->remove)})},
    {"read", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t read(void *file_p, void *buf, uint32_t btr, uint32_t *br) read", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, read), sizeof(((lv_fs_drv_t *)0)->read)})},
    {"write", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t write(void *file_p, const void *buf, uint32_t btw, uint32_t *bw) write", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, write), sizeof(((lv_fs_drv_t *)0)->write)})},
    {"seek", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t seek(void *file_p, uint32_t pos) seek", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, seek), sizeof(((lv_fs_drv_t *)0)->seek)})},
    {"tell", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t tell(void *file_p, uint32_t *pos_p) tell", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, tell), sizeof(((lv_fs_drv_t *)0)->tell)})},
    {"trunc", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t trunc(void *file_p) trunc", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, trunc), sizeof(((lv_fs_drv_t *)0)->trunc)})},
    {"size", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t size(void *file_p, uint32_t *size_p) size", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, size), sizeof(((lv_fs_drv_t *)0)->size)})},
    {"rename", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t rename(const char *oldname, const char *newname) rename", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, rename), sizeof(((lv_fs_drv_t *)0)->rename)})},
    {"free_space", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t free_space(uint32_t *total_p, uint32_t *free_p) free_space", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, free_space), sizeof(((lv_fs_drv_t *)0)->free_space)})},
    {"dir_open", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t dir_open(void *rddir_p, const char *path) dir_open", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, dir_open), sizeof(((lv_fs_drv_t *)0)->dir_open)})},
    {"dir_read", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t dir_read(void *rddir_p, char *fn) dir_read", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, dir_read), sizeof(((lv_fs_drv_t *)0)->dir_read)})},
    {"dir_close", (getter) struct_get_struct, (setter) struct_set_struct, "lv_fs_res_t dir_close(void *rddir_p) dir_close", & ((struct_closure_t){ &Blob_Type, offsetof(lv_fs_drv_t, dir_close), sizeof(((lv_fs_drv_t *)0)->dir_close)})},
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
    *(lv_fs_drv_t **)target = ((StructObject*)obj) -> data;
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
    *(lv_img_header_t **)target = ((StructObject*)obj) -> data;
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
    *(lv_img_dsc_t **)target = ((StructObject*)obj) -> data;
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
    *(lv_img_decoder_t **)target = ((StructObject*)obj) -> data;
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
    {"style", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style", & ((struct_closure_t){ &Blob_Type, offsetof(lv_img_decoder_dsc_t, style), sizeof(((lv_img_decoder_dsc_t *)0)->style)})},
    {"src", (getter) struct_get_struct, (setter) struct_set_struct, "void src", & ((struct_closure_t){ &Blob_Type, offsetof(lv_img_decoder_dsc_t, src), sizeof(((lv_img_decoder_dsc_t *)0)->src)})},
    {"src_type", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_img_src_t src_type", (void*)offsetof(lv_img_decoder_dsc_t, src_type)},
    {"header", (getter) struct_get_struct, (setter) struct_set_struct, "lv_img_header_t header", & ((struct_closure_t){ &pylv_img_header_t_Type, offsetof(lv_img_decoder_dsc_t, header), sizeof(lv_img_header_t)})},
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
    *(lv_img_decoder_dsc_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_imgbtn_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_imgbtn_ext_t_Type, sizeof(lv_imgbtn_ext_t));
}

static PyGetSetDef pylv_imgbtn_ext_t_getset[] = {
    {"btn", (getter) struct_get_struct, (setter) struct_set_struct, "lv_btn_ext_t btn", & ((struct_closure_t){ &pylv_btn_ext_t_Type, offsetof(lv_imgbtn_ext_t, btn), sizeof(lv_btn_ext_t)})},
    {"img_src", (getter) struct_get_struct, (setter) struct_set_struct, "voidLV_BTN_STATE_NUM img_src", & ((struct_closure_t){ &Blob_Type, offsetof(lv_imgbtn_ext_t, img_src), sizeof(((lv_imgbtn_ext_t *)0)->img_src)})},
    {"act_cf", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_img_cf_t act_cf", (void*)offsetof(lv_imgbtn_ext_t, act_cf)},
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
    *(lv_imgbtn_ext_t **)target = ((StructObject*)obj) -> data;
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
get_struct_bitfield_label_ext_t_body_draw(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_label_ext_t*)(self->data))->body_draw );
}

static int
set_struct_bitfield_label_ext_t_body_draw(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_label_ext_t*)(self->data))->body_draw = v;
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
    {"dot", (getter) struct_get_struct, (setter) struct_set_struct, "union  {   char *tmp_ptr;   char tmp[sizeof(char *)]; } dot", & ((struct_closure_t){ &pylv_label_ext_t_dot_Type, offsetof(lv_label_ext_t, dot), sizeof(((lv_label_ext_t *)0)->dot)})},
    {"dot_end", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t dot_end", (void*)offsetof(lv_label_ext_t, dot_end)},
    {"offset", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t offset", & ((struct_closure_t){ &pylv_point_t_Type, offsetof(lv_label_ext_t, offset), sizeof(lv_point_t)})},
    {"anim_speed", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t anim_speed", (void*)offsetof(lv_label_ext_t, anim_speed)},
    {"txt_sel_start", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t txt_sel_start", (void*)offsetof(lv_label_ext_t, txt_sel_start)},
    {"txt_sel_end", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t txt_sel_end", (void*)offsetof(lv_label_ext_t, txt_sel_end)},
    {"long_mode", (getter) get_struct_bitfield_label_ext_t_long_mode, (setter) set_struct_bitfield_label_ext_t_long_mode, "lv_label_long_mode_t:3 long_mode", NULL},
    {"static_txt", (getter) get_struct_bitfield_label_ext_t_static_txt, (setter) set_struct_bitfield_label_ext_t_static_txt, "uint8_t:1 static_txt", NULL},
    {"align", (getter) get_struct_bitfield_label_ext_t_align, (setter) set_struct_bitfield_label_ext_t_align, "uint8_t:2 align", NULL},
    {"recolor", (getter) get_struct_bitfield_label_ext_t_recolor, (setter) set_struct_bitfield_label_ext_t_recolor, "uint8_t:1 recolor", NULL},
    {"expand", (getter) get_struct_bitfield_label_ext_t_expand, (setter) set_struct_bitfield_label_ext_t_expand, "uint8_t:1 expand", NULL},
    {"body_draw", (getter) get_struct_bitfield_label_ext_t_body_draw, (setter) set_struct_bitfield_label_ext_t_body_draw, "uint8_t:1 body_draw", NULL},
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
    *(lv_label_ext_t **)target = ((StructObject*)obj) -> data;
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

static PyGetSetDef pylv_img_ext_t_getset[] = {
    {"src", (getter) struct_get_struct, (setter) struct_set_struct, "void src", & ((struct_closure_t){ &Blob_Type, offsetof(lv_img_ext_t, src), sizeof(((lv_img_ext_t *)0)->src)})},
    {"offset", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t offset", & ((struct_closure_t){ &pylv_point_t_Type, offsetof(lv_img_ext_t, offset), sizeof(lv_point_t)})},
    {"w", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t w", (void*)offsetof(lv_img_ext_t, w)},
    {"h", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t h", (void*)offsetof(lv_img_ext_t, h)},
    {"src_type", (getter) get_struct_bitfield_img_ext_t_src_type, (setter) set_struct_bitfield_img_ext_t_src_type, "uint8_t:2 src_type", NULL},
    {"auto_size", (getter) get_struct_bitfield_img_ext_t_auto_size, (setter) set_struct_bitfield_img_ext_t_auto_size, "uint8_t:1 auto_size", NULL},
    {"cf", (getter) get_struct_bitfield_img_ext_t_cf, (setter) set_struct_bitfield_img_ext_t_cf, "uint8_t:5 cf", NULL},
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
    *(lv_img_ext_t **)target = ((StructObject*)obj) -> data;
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
    *(lv_line_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_page_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_page_ext_t_Type, sizeof(lv_page_ext_t));
}



static PyObject *
get_struct_bitfield_page_ext_t_arrow_scroll(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_page_ext_t*)(self->data))->arrow_scroll );
}

static int
set_struct_bitfield_page_ext_t_arrow_scroll(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_page_ext_t*)(self->data))->arrow_scroll = v;
    return 0;
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



static PyObject *
get_struct_bitfield_page_ext_t_scroll_prop_ip(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_page_ext_t*)(self->data))->scroll_prop_ip );
}

static int
set_struct_bitfield_page_ext_t_scroll_prop_ip(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_page_ext_t*)(self->data))->scroll_prop_ip = v;
    return 0;
}

static PyGetSetDef pylv_page_ext_t_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_cont_ext_t bg", & ((struct_closure_t){ &pylv_cont_ext_t_Type, offsetof(lv_page_ext_t, bg), sizeof(lv_cont_ext_t)})},
    {"scrl", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t scrl", & ((struct_closure_t){ &Blob_Type, offsetof(lv_page_ext_t, scrl), sizeof(((lv_page_ext_t *)0)->scrl)})},
    {"sb", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   const lv_style_t *style;   lv_area_t hor_area;   lv_area_t ver_area;   uint8_t hor_draw : 1;   uint8_t ver_draw : 1;   lv_sb_mode_t mode : 3; } sb", & ((struct_closure_t){ &pylv_page_ext_t_sb_Type, offsetof(lv_page_ext_t, sb), sizeof(((lv_page_ext_t *)0)->sb)})},
    {"edge_flash", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_anim_value_t state;   const lv_style_t *style;   uint8_t enabled : 1;   uint8_t top_ip : 1;   uint8_t bottom_ip : 1;   uint8_t right_ip : 1;   uint8_t left_ip : 1; } edge_flash", & ((struct_closure_t){ &pylv_page_ext_t_edge_flash_Type, offsetof(lv_page_ext_t, edge_flash), sizeof(((lv_page_ext_t *)0)->edge_flash)})},
    {"arrow_scroll", (getter) get_struct_bitfield_page_ext_t_arrow_scroll, (setter) set_struct_bitfield_page_ext_t_arrow_scroll, "uint8_t:1 arrow_scroll", NULL},
    {"scroll_prop", (getter) get_struct_bitfield_page_ext_t_scroll_prop, (setter) set_struct_bitfield_page_ext_t_scroll_prop, "uint8_t:1 scroll_prop", NULL},
    {"scroll_prop_ip", (getter) get_struct_bitfield_page_ext_t_scroll_prop_ip, (setter) set_struct_bitfield_page_ext_t_scroll_prop_ip, "uint8_t:1 scroll_prop_ip", NULL},
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
    *(lv_page_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_list_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_list_ext_t_Type, sizeof(lv_list_ext_t));
}



static PyObject *
get_struct_bitfield_list_ext_t_single_mode(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_list_ext_t*)(self->data))->single_mode );
}

static int
set_struct_bitfield_list_ext_t_single_mode(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_list_ext_t*)(self->data))->single_mode = v;
    return 0;
}

static PyGetSetDef pylv_list_ext_t_getset[] = {
    {"page", (getter) struct_get_struct, (setter) struct_set_struct, "lv_page_ext_t page", & ((struct_closure_t){ &pylv_page_ext_t_Type, offsetof(lv_list_ext_t, page), sizeof(lv_page_ext_t)})},
    {"styles_btn", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_tLV_BTN_STATE_NUM styles_btn", & ((struct_closure_t){ &Blob_Type, offsetof(lv_list_ext_t, styles_btn), sizeof(((lv_list_ext_t *)0)->styles_btn)})},
    {"style_img", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_img", & ((struct_closure_t){ &Blob_Type, offsetof(lv_list_ext_t, style_img), sizeof(((lv_list_ext_t *)0)->style_img)})},
    {"size", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t size", (void*)offsetof(lv_list_ext_t, size)},
    {"anim_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t anim_time", (void*)offsetof(lv_list_ext_t, anim_time)},
    {"single_mode", (getter) get_struct_bitfield_list_ext_t_single_mode, (setter) set_struct_bitfield_list_ext_t_single_mode, "uint8_t:1 single_mode", NULL},
    {"last_sel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t last_sel", & ((struct_closure_t){ &Blob_Type, offsetof(lv_list_ext_t, last_sel), sizeof(((lv_list_ext_t *)0)->last_sel)})},
    {"selected_btn", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t selected_btn", & ((struct_closure_t){ &Blob_Type, offsetof(lv_list_ext_t, selected_btn), sizeof(((lv_list_ext_t *)0)->selected_btn)})},
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
    *(lv_list_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_chart_series_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_chart_series_t_Type, sizeof(lv_chart_series_t));
}

static PyGetSetDef pylv_chart_series_t_getset[] = {
    {"points", (getter) struct_get_struct, (setter) struct_set_struct, "lv_coord_t points", & ((struct_closure_t){ &Blob_Type, offsetof(lv_chart_series_t, points), sizeof(((lv_chart_series_t *)0)->points)})},
    {"color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t color", & ((struct_closure_t){ &pylv_color16_t_Type, offsetof(lv_chart_series_t, color), sizeof(lv_color16_t)})},
    {"start_point", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t start_point", (void*)offsetof(lv_chart_series_t, start_point)},
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
    *(lv_chart_series_t **)target = ((StructObject*)obj) -> data;
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
    {"num_tick_marks", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t num_tick_marks", (void*)offsetof(lv_chart_axis_cfg_t, num_tick_marks)},
    {"options", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_chart_axis_options_t options", (void*)offsetof(lv_chart_axis_cfg_t, options)},
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
    *(lv_chart_axis_cfg_t **)target = ((StructObject*)obj) -> data;
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
    {"ymin", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t ymin", (void*)offsetof(lv_chart_ext_t, ymin)},
    {"ymax", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t ymax", (void*)offsetof(lv_chart_ext_t, ymax)},
    {"hdiv_cnt", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t hdiv_cnt", (void*)offsetof(lv_chart_ext_t, hdiv_cnt)},
    {"vdiv_cnt", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t vdiv_cnt", (void*)offsetof(lv_chart_ext_t, vdiv_cnt)},
    {"point_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t point_cnt", (void*)offsetof(lv_chart_ext_t, point_cnt)},
    {"type", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_chart_type_t type", (void*)offsetof(lv_chart_ext_t, type)},
    {"y_axis", (getter) struct_get_struct, (setter) struct_set_struct, "lv_chart_axis_cfg_t y_axis", & ((struct_closure_t){ &pylv_chart_axis_cfg_t_Type, offsetof(lv_chart_ext_t, y_axis), sizeof(lv_chart_axis_cfg_t)})},
    {"x_axis", (getter) struct_get_struct, (setter) struct_set_struct, "lv_chart_axis_cfg_t x_axis", & ((struct_closure_t){ &pylv_chart_axis_cfg_t_Type, offsetof(lv_chart_ext_t, x_axis), sizeof(lv_chart_axis_cfg_t)})},
    {"margin", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t margin", (void*)offsetof(lv_chart_ext_t, margin)},
    {"update_mode", (getter) get_struct_bitfield_chart_ext_t_update_mode, (setter) set_struct_bitfield_chart_ext_t_update_mode, "uint8_t:1 update_mode", NULL},
    {"series", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_coord_t width;   uint8_t num;   lv_opa_t opa;   lv_opa_t dark; } series", & ((struct_closure_t){ &pylv_chart_ext_t_series_Type, offsetof(lv_chart_ext_t, series), sizeof(((lv_chart_ext_t *)0)->series)})},
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
    *(lv_chart_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_table_cell_format_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_table_cell_format_t_Type, sizeof(lv_table_cell_format_t));
}

static PyGetSetDef pylv_table_cell_format_t_getset[] = {
    {"s", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   uint8_t align : 2;   uint8_t right_merge : 1;   uint8_t type : 2;   uint8_t crop : 1; } s", & ((struct_closure_t){ &pylv_table_cell_format_t_s_Type, offsetof(lv_table_cell_format_t, s), sizeof(((lv_table_cell_format_t *)0)->s)})},
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
    *(lv_table_cell_format_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_table_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_table_ext_t_Type, sizeof(lv_table_ext_t));
}

static PyGetSetDef pylv_table_ext_t_getset[] = {
    {"col_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t col_cnt", (void*)offsetof(lv_table_ext_t, col_cnt)},
    {"row_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t row_cnt", (void*)offsetof(lv_table_ext_t, row_cnt)},
    {"cell_data", (getter) struct_get_struct, (setter) struct_set_struct, "char cell_data", & ((struct_closure_t){ &Blob_Type, offsetof(lv_table_ext_t, cell_data), sizeof(((lv_table_ext_t *)0)->cell_data)})},
    {"cell_style", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t4 cell_style", & ((struct_closure_t){ &Blob_Type, offsetof(lv_table_ext_t, cell_style), sizeof(((lv_table_ext_t *)0)->cell_style)})},
    {"col_w", (getter) struct_get_struct, (setter) struct_set_struct, "lv_coord_t12 col_w", & ((struct_closure_t){ &Blob_Type, offsetof(lv_table_ext_t, col_w), sizeof(((lv_table_ext_t *)0)->col_w)})},
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
    *(lv_table_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_cb_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_cb_ext_t_Type, sizeof(lv_cb_ext_t));
}

static PyGetSetDef pylv_cb_ext_t_getset[] = {
    {"bg_btn", (getter) struct_get_struct, (setter) struct_set_struct, "lv_btn_ext_t bg_btn", & ((struct_closure_t){ &pylv_btn_ext_t_Type, offsetof(lv_cb_ext_t, bg_btn), sizeof(lv_btn_ext_t)})},
    {"bullet", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t bullet", & ((struct_closure_t){ &Blob_Type, offsetof(lv_cb_ext_t, bullet), sizeof(((lv_cb_ext_t *)0)->bullet)})},
    {"label", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t label", & ((struct_closure_t){ &Blob_Type, offsetof(lv_cb_ext_t, label), sizeof(((lv_cb_ext_t *)0)->label)})},
    {NULL}
};


static PyTypeObject pylv_cb_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.cb_ext_t",
    .tp_doc = "lvgl cb_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_cb_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_cb_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_cb_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_cb_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_cb_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_cb_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_bar_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_bar_ext_t_Type, sizeof(lv_bar_ext_t));
}



static PyObject *
get_struct_bitfield_bar_ext_t_sym(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_bar_ext_t*)(self->data))->sym );
}

static int
set_struct_bitfield_bar_ext_t_sym(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_bar_ext_t*)(self->data))->sym = v;
    return 0;
}

static PyGetSetDef pylv_bar_ext_t_getset[] = {
    {"cur_value", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t cur_value", (void*)offsetof(lv_bar_ext_t, cur_value)},
    {"min_value", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t min_value", (void*)offsetof(lv_bar_ext_t, min_value)},
    {"max_value", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t max_value", (void*)offsetof(lv_bar_ext_t, max_value)},
    {"anim_start", (getter) struct_get_int16, (setter) struct_set_int16, "lv_anim_value_t anim_start", (void*)offsetof(lv_bar_ext_t, anim_start)},
    {"anim_end", (getter) struct_get_int16, (setter) struct_set_int16, "lv_anim_value_t anim_end", (void*)offsetof(lv_bar_ext_t, anim_end)},
    {"anim_state", (getter) struct_get_int16, (setter) struct_set_int16, "lv_anim_value_t anim_state", (void*)offsetof(lv_bar_ext_t, anim_state)},
    {"anim_time", (getter) struct_get_int16, (setter) struct_set_int16, "lv_anim_value_t anim_time", (void*)offsetof(lv_bar_ext_t, anim_time)},
    {"sym", (getter) get_struct_bitfield_bar_ext_t_sym, (setter) set_struct_bitfield_bar_ext_t_sym, "uint8_t:1 sym", NULL},
    {"style_indic", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_indic", & ((struct_closure_t){ &Blob_Type, offsetof(lv_bar_ext_t, style_indic), sizeof(((lv_bar_ext_t *)0)->style_indic)})},
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
    *(lv_bar_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_slider_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_slider_ext_t_Type, sizeof(lv_slider_ext_t));
}



static PyObject *
get_struct_bitfield_slider_ext_t_knob_in(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_slider_ext_t*)(self->data))->knob_in );
}

static int
set_struct_bitfield_slider_ext_t_knob_in(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_slider_ext_t*)(self->data))->knob_in = v;
    return 0;
}

static PyGetSetDef pylv_slider_ext_t_getset[] = {
    {"bar", (getter) struct_get_struct, (setter) struct_set_struct, "lv_bar_ext_t bar", & ((struct_closure_t){ &pylv_bar_ext_t_Type, offsetof(lv_slider_ext_t, bar), sizeof(lv_bar_ext_t)})},
    {"style_knob", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_knob", & ((struct_closure_t){ &Blob_Type, offsetof(lv_slider_ext_t, style_knob), sizeof(((lv_slider_ext_t *)0)->style_knob)})},
    {"drag_value", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t drag_value", (void*)offsetof(lv_slider_ext_t, drag_value)},
    {"knob_in", (getter) get_struct_bitfield_slider_ext_t_knob_in, (setter) set_struct_bitfield_slider_ext_t_knob_in, "uint8_t:1 knob_in", NULL},
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
    *(lv_slider_ext_t **)target = ((StructObject*)obj) -> data;
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
    *(lv_led_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_btnm_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_btnm_ext_t_Type, sizeof(lv_btnm_ext_t));
}



static PyObject *
get_struct_bitfield_btnm_ext_t_recolor(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_btnm_ext_t*)(self->data))->recolor );
}

static int
set_struct_bitfield_btnm_ext_t_recolor(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_btnm_ext_t*)(self->data))->recolor = v;
    return 0;
}



static PyObject *
get_struct_bitfield_btnm_ext_t_one_toggle(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_btnm_ext_t*)(self->data))->one_toggle );
}

static int
set_struct_bitfield_btnm_ext_t_one_toggle(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_btnm_ext_t*)(self->data))->one_toggle = v;
    return 0;
}

static PyGetSetDef pylv_btnm_ext_t_getset[] = {
    {"map_p", (getter) struct_get_struct, (setter) struct_set_struct, "char map_p", & ((struct_closure_t){ &Blob_Type, offsetof(lv_btnm_ext_t, map_p), sizeof(((lv_btnm_ext_t *)0)->map_p)})},
    {"button_areas", (getter) struct_get_struct, (setter) struct_set_struct, "lv_area_t button_areas", & ((struct_closure_t){ &Blob_Type, offsetof(lv_btnm_ext_t, button_areas), sizeof(((lv_btnm_ext_t *)0)->button_areas)})},
    {"ctrl_bits", (getter) struct_get_struct, (setter) struct_set_struct, "lv_btnm_ctrl_t ctrl_bits", & ((struct_closure_t){ &Blob_Type, offsetof(lv_btnm_ext_t, ctrl_bits), sizeof(((lv_btnm_ext_t *)0)->ctrl_bits)})},
    {"styles_btn", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_tLV_BTN_STATE_NUM styles_btn", & ((struct_closure_t){ &Blob_Type, offsetof(lv_btnm_ext_t, styles_btn), sizeof(((lv_btnm_ext_t *)0)->styles_btn)})},
    {"btn_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t btn_cnt", (void*)offsetof(lv_btnm_ext_t, btn_cnt)},
    {"btn_id_pr", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t btn_id_pr", (void*)offsetof(lv_btnm_ext_t, btn_id_pr)},
    {"btn_id_act", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t btn_id_act", (void*)offsetof(lv_btnm_ext_t, btn_id_act)},
    {"recolor", (getter) get_struct_bitfield_btnm_ext_t_recolor, (setter) set_struct_bitfield_btnm_ext_t_recolor, "uint8_t:1 recolor", NULL},
    {"one_toggle", (getter) get_struct_bitfield_btnm_ext_t_one_toggle, (setter) set_struct_bitfield_btnm_ext_t_one_toggle, "uint8_t:1 one_toggle", NULL},
    {NULL}
};


static PyTypeObject pylv_btnm_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.btnm_ext_t",
    .tp_doc = "lvgl btnm_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_btnm_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_btnm_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_btnm_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_btnm_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_btnm_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_btnm_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_kb_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_kb_ext_t_Type, sizeof(lv_kb_ext_t));
}



static PyObject *
get_struct_bitfield_kb_ext_t_cursor_mng(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_kb_ext_t*)(self->data))->cursor_mng );
}

static int
set_struct_bitfield_kb_ext_t_cursor_mng(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_kb_ext_t*)(self->data))->cursor_mng = v;
    return 0;
}

static PyGetSetDef pylv_kb_ext_t_getset[] = {
    {"btnm", (getter) struct_get_struct, (setter) struct_set_struct, "lv_btnm_ext_t btnm", & ((struct_closure_t){ &pylv_btnm_ext_t_Type, offsetof(lv_kb_ext_t, btnm), sizeof(lv_btnm_ext_t)})},
    {"ta", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t ta", & ((struct_closure_t){ &Blob_Type, offsetof(lv_kb_ext_t, ta), sizeof(((lv_kb_ext_t *)0)->ta)})},
    {"mode", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_kb_mode_t mode", (void*)offsetof(lv_kb_ext_t, mode)},
    {"cursor_mng", (getter) get_struct_bitfield_kb_ext_t_cursor_mng, (setter) set_struct_bitfield_kb_ext_t_cursor_mng, "uint8_t:1 cursor_mng", NULL},
    {NULL}
};


static PyTypeObject pylv_kb_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.kb_ext_t",
    .tp_doc = "lvgl kb_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_kb_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_kb_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_kb_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_kb_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_kb_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_kb_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_ddlist_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_ddlist_ext_t_Type, sizeof(lv_ddlist_ext_t));
}



static PyObject *
get_struct_bitfield_ddlist_ext_t_opened(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_ddlist_ext_t*)(self->data))->opened );
}

static int
set_struct_bitfield_ddlist_ext_t_opened(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_ddlist_ext_t*)(self->data))->opened = v;
    return 0;
}



static PyObject *
get_struct_bitfield_ddlist_ext_t_force_sel(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_ddlist_ext_t*)(self->data))->force_sel );
}

static int
set_struct_bitfield_ddlist_ext_t_force_sel(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_ddlist_ext_t*)(self->data))->force_sel = v;
    return 0;
}



static PyObject *
get_struct_bitfield_ddlist_ext_t_draw_arrow(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_ddlist_ext_t*)(self->data))->draw_arrow );
}

static int
set_struct_bitfield_ddlist_ext_t_draw_arrow(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_ddlist_ext_t*)(self->data))->draw_arrow = v;
    return 0;
}



static PyObject *
get_struct_bitfield_ddlist_ext_t_stay_open(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_ddlist_ext_t*)(self->data))->stay_open );
}

static int
set_struct_bitfield_ddlist_ext_t_stay_open(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_ddlist_ext_t*)(self->data))->stay_open = v;
    return 0;
}

static PyGetSetDef pylv_ddlist_ext_t_getset[] = {
    {"page", (getter) struct_get_struct, (setter) struct_set_struct, "lv_page_ext_t page", & ((struct_closure_t){ &pylv_page_ext_t_Type, offsetof(lv_ddlist_ext_t, page), sizeof(lv_page_ext_t)})},
    {"label", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t label", & ((struct_closure_t){ &Blob_Type, offsetof(lv_ddlist_ext_t, label), sizeof(((lv_ddlist_ext_t *)0)->label)})},
    {"sel_style", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t sel_style", & ((struct_closure_t){ &Blob_Type, offsetof(lv_ddlist_ext_t, sel_style), sizeof(((lv_ddlist_ext_t *)0)->sel_style)})},
    {"option_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t option_cnt", (void*)offsetof(lv_ddlist_ext_t, option_cnt)},
    {"sel_opt_id", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t sel_opt_id", (void*)offsetof(lv_ddlist_ext_t, sel_opt_id)},
    {"sel_opt_id_ori", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t sel_opt_id_ori", (void*)offsetof(lv_ddlist_ext_t, sel_opt_id_ori)},
    {"anim_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t anim_time", (void*)offsetof(lv_ddlist_ext_t, anim_time)},
    {"opened", (getter) get_struct_bitfield_ddlist_ext_t_opened, (setter) set_struct_bitfield_ddlist_ext_t_opened, "uint8_t:1 opened", NULL},
    {"force_sel", (getter) get_struct_bitfield_ddlist_ext_t_force_sel, (setter) set_struct_bitfield_ddlist_ext_t_force_sel, "uint8_t:1 force_sel", NULL},
    {"draw_arrow", (getter) get_struct_bitfield_ddlist_ext_t_draw_arrow, (setter) set_struct_bitfield_ddlist_ext_t_draw_arrow, "uint8_t:1 draw_arrow", NULL},
    {"stay_open", (getter) get_struct_bitfield_ddlist_ext_t_stay_open, (setter) set_struct_bitfield_ddlist_ext_t_stay_open, "uint8_t:1 stay_open", NULL},
    {"fix_height", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t fix_height", (void*)offsetof(lv_ddlist_ext_t, fix_height)},
    {NULL}
};


static PyTypeObject pylv_ddlist_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.ddlist_ext_t",
    .tp_doc = "lvgl ddlist_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_ddlist_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_ddlist_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_ddlist_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_ddlist_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_ddlist_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_ddlist_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_roller_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_roller_ext_t_Type, sizeof(lv_roller_ext_t));
}



static PyObject *
get_struct_bitfield_roller_ext_t_inf(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_roller_ext_t*)(self->data))->inf );
}

static int
set_struct_bitfield_roller_ext_t_inf(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_roller_ext_t*)(self->data))->inf = v;
    return 0;
}

static PyGetSetDef pylv_roller_ext_t_getset[] = {
    {"ddlist", (getter) struct_get_struct, (setter) struct_set_struct, "lv_ddlist_ext_t ddlist", & ((struct_closure_t){ &pylv_ddlist_ext_t_Type, offsetof(lv_roller_ext_t, ddlist), sizeof(lv_ddlist_ext_t)})},
    {"inf", (getter) get_struct_bitfield_roller_ext_t_inf, (setter) set_struct_bitfield_roller_ext_t_inf, "uint8_t:1 inf", NULL},
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
    *(lv_roller_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_ta_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_ta_ext_t_Type, sizeof(lv_ta_ext_t));
}



static PyObject *
get_struct_bitfield_ta_ext_t_pwd_mode(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_ta_ext_t*)(self->data))->pwd_mode );
}

static int
set_struct_bitfield_ta_ext_t_pwd_mode(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_ta_ext_t*)(self->data))->pwd_mode = v;
    return 0;
}



static PyObject *
get_struct_bitfield_ta_ext_t_one_line(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_ta_ext_t*)(self->data))->one_line );
}

static int
set_struct_bitfield_ta_ext_t_one_line(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_ta_ext_t*)(self->data))->one_line = v;
    return 0;
}



static PyObject *
get_struct_bitfield_ta_ext_t_text_sel_in_prog(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_ta_ext_t*)(self->data))->text_sel_in_prog );
}

static int
set_struct_bitfield_ta_ext_t_text_sel_in_prog(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_ta_ext_t*)(self->data))->text_sel_in_prog = v;
    return 0;
}



static PyObject *
get_struct_bitfield_ta_ext_t_text_sel_en(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_ta_ext_t*)(self->data))->text_sel_en );
}

static int
set_struct_bitfield_ta_ext_t_text_sel_en(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_ta_ext_t*)(self->data))->text_sel_en = v;
    return 0;
}

static PyGetSetDef pylv_ta_ext_t_getset[] = {
    {"page", (getter) struct_get_struct, (setter) struct_set_struct, "lv_page_ext_t page", & ((struct_closure_t){ &pylv_page_ext_t_Type, offsetof(lv_ta_ext_t, page), sizeof(lv_page_ext_t)})},
    {"label", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t label", & ((struct_closure_t){ &Blob_Type, offsetof(lv_ta_ext_t, label), sizeof(((lv_ta_ext_t *)0)->label)})},
    {"placeholder", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t placeholder", & ((struct_closure_t){ &Blob_Type, offsetof(lv_ta_ext_t, placeholder), sizeof(((lv_ta_ext_t *)0)->placeholder)})},
    {"pwd_tmp", (getter) struct_get_struct, (setter) struct_set_struct, "char pwd_tmp", & ((struct_closure_t){ &Blob_Type, offsetof(lv_ta_ext_t, pwd_tmp), sizeof(((lv_ta_ext_t *)0)->pwd_tmp)})},
    {"accapted_chars", (getter) struct_get_struct, (setter) struct_set_struct, "char accapted_chars", & ((struct_closure_t){ &Blob_Type, offsetof(lv_ta_ext_t, accapted_chars), sizeof(((lv_ta_ext_t *)0)->accapted_chars)})},
    {"max_length", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t max_length", (void*)offsetof(lv_ta_ext_t, max_length)},
    {"pwd_mode", (getter) get_struct_bitfield_ta_ext_t_pwd_mode, (setter) set_struct_bitfield_ta_ext_t_pwd_mode, "uint8_t:1 pwd_mode", NULL},
    {"one_line", (getter) get_struct_bitfield_ta_ext_t_one_line, (setter) set_struct_bitfield_ta_ext_t_one_line, "uint8_t:1 one_line", NULL},
    {"cursor", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   const lv_style_t *style;   lv_coord_t valid_x;   uint16_t pos;   lv_area_t area;   uint16_t txt_byte_pos;   lv_cursor_type_t type : 4;   uint8_t state : 1; } cursor", & ((struct_closure_t){ &pylv_ta_ext_t_cursor_Type, offsetof(lv_ta_ext_t, cursor), sizeof(((lv_ta_ext_t *)0)->cursor)})},
    {"tmp_sel_start", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t tmp_sel_start", (void*)offsetof(lv_ta_ext_t, tmp_sel_start)},
    {"tmp_sel_end", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t tmp_sel_end", (void*)offsetof(lv_ta_ext_t, tmp_sel_end)},
    {"text_sel_in_prog", (getter) get_struct_bitfield_ta_ext_t_text_sel_in_prog, (setter) set_struct_bitfield_ta_ext_t_text_sel_in_prog, "uint8_t:1 text_sel_in_prog", NULL},
    {"text_sel_en", (getter) get_struct_bitfield_ta_ext_t_text_sel_en, (setter) set_struct_bitfield_ta_ext_t_text_sel_en, "uint8_t:1 text_sel_en", NULL},
    {NULL}
};


static PyTypeObject pylv_ta_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.ta_ext_t",
    .tp_doc = "lvgl ta_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_ta_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_ta_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_ta_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_ta_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_ta_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_ta_ext_t **)target = ((StructObject*)obj) -> data;
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
    *(lv_canvas_ext_t **)target = ((StructObject*)obj) -> data;
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
    {"title", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t title", & ((struct_closure_t){ &Blob_Type, offsetof(lv_win_ext_t, title), sizeof(((lv_win_ext_t *)0)->title)})},
    {"style_btn_rel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_btn_rel", & ((struct_closure_t){ &Blob_Type, offsetof(lv_win_ext_t, style_btn_rel), sizeof(((lv_win_ext_t *)0)->style_btn_rel)})},
    {"style_btn_pr", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_btn_pr", & ((struct_closure_t){ &Blob_Type, offsetof(lv_win_ext_t, style_btn_pr), sizeof(((lv_win_ext_t *)0)->style_btn_pr)})},
    {"btn_size", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t btn_size", (void*)offsetof(lv_win_ext_t, btn_size)},
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
    *(lv_win_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_tabview_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_tabview_ext_t_Type, sizeof(lv_tabview_ext_t));
}



static PyObject *
get_struct_bitfield_tabview_ext_t_slide_enable(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_tabview_ext_t*)(self->data))->slide_enable );
}

static int
set_struct_bitfield_tabview_ext_t_slide_enable(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_tabview_ext_t*)(self->data))->slide_enable = v;
    return 0;
}



static PyObject *
get_struct_bitfield_tabview_ext_t_draging(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_tabview_ext_t*)(self->data))->draging );
}

static int
set_struct_bitfield_tabview_ext_t_draging(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_tabview_ext_t*)(self->data))->draging = v;
    return 0;
}



static PyObject *
get_struct_bitfield_tabview_ext_t_drag_hor(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_tabview_ext_t*)(self->data))->drag_hor );
}

static int
set_struct_bitfield_tabview_ext_t_drag_hor(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_tabview_ext_t*)(self->data))->drag_hor = v;
    return 0;
}



static PyObject *
get_struct_bitfield_tabview_ext_t_scroll_ver(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_tabview_ext_t*)(self->data))->scroll_ver );
}

static int
set_struct_bitfield_tabview_ext_t_scroll_ver(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_tabview_ext_t*)(self->data))->scroll_ver = v;
    return 0;
}



static PyObject *
get_struct_bitfield_tabview_ext_t_btns_hide(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_tabview_ext_t*)(self->data))->btns_hide );
}

static int
set_struct_bitfield_tabview_ext_t_btns_hide(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_tabview_ext_t*)(self->data))->btns_hide = v;
    return 0;
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
    if (long_to_int(value, &v, 0, 3)) return -1;
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
    {"slide_enable", (getter) get_struct_bitfield_tabview_ext_t_slide_enable, (setter) set_struct_bitfield_tabview_ext_t_slide_enable, "uint8_t:1 slide_enable", NULL},
    {"draging", (getter) get_struct_bitfield_tabview_ext_t_draging, (setter) set_struct_bitfield_tabview_ext_t_draging, "uint8_t:1 draging", NULL},
    {"drag_hor", (getter) get_struct_bitfield_tabview_ext_t_drag_hor, (setter) set_struct_bitfield_tabview_ext_t_drag_hor, "uint8_t:1 drag_hor", NULL},
    {"scroll_ver", (getter) get_struct_bitfield_tabview_ext_t_scroll_ver, (setter) set_struct_bitfield_tabview_ext_t_scroll_ver, "uint8_t:1 scroll_ver", NULL},
    {"btns_hide", (getter) get_struct_bitfield_tabview_ext_t_btns_hide, (setter) set_struct_bitfield_tabview_ext_t_btns_hide, "uint8_t:1 btns_hide", NULL},
    {"btns_pos", (getter) get_struct_bitfield_tabview_ext_t_btns_pos, (setter) set_struct_bitfield_tabview_ext_t_btns_pos, "lv_tabview_btns_pos_t:2 btns_pos", NULL},
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
    *(lv_tabview_ext_t **)target = ((StructObject*)obj) -> data;
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



static PyObject *
get_struct_bitfield_tileview_ext_t_drag_hor(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_tileview_ext_t*)(self->data))->drag_hor );
}

static int
set_struct_bitfield_tileview_ext_t_drag_hor(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_tileview_ext_t*)(self->data))->drag_hor = v;
    return 0;
}



static PyObject *
get_struct_bitfield_tileview_ext_t_drag_ver(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_tileview_ext_t*)(self->data))->drag_ver );
}

static int
set_struct_bitfield_tileview_ext_t_drag_ver(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_tileview_ext_t*)(self->data))->drag_ver = v;
    return 0;
}

static PyGetSetDef pylv_tileview_ext_t_getset[] = {
    {"page", (getter) struct_get_struct, (setter) struct_set_struct, "lv_page_ext_t page", & ((struct_closure_t){ &pylv_page_ext_t_Type, offsetof(lv_tileview_ext_t, page), sizeof(lv_page_ext_t)})},
    {"valid_pos", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t valid_pos", & ((struct_closure_t){ &Blob_Type, offsetof(lv_tileview_ext_t, valid_pos), sizeof(((lv_tileview_ext_t *)0)->valid_pos)})},
    {"anim_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t anim_time", (void*)offsetof(lv_tileview_ext_t, anim_time)},
    {"act_id", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t act_id", & ((struct_closure_t){ &pylv_point_t_Type, offsetof(lv_tileview_ext_t, act_id), sizeof(lv_point_t)})},
    {"drag_top_en", (getter) get_struct_bitfield_tileview_ext_t_drag_top_en, (setter) set_struct_bitfield_tileview_ext_t_drag_top_en, "uint8_t:1 drag_top_en", NULL},
    {"drag_bottom_en", (getter) get_struct_bitfield_tileview_ext_t_drag_bottom_en, (setter) set_struct_bitfield_tileview_ext_t_drag_bottom_en, "uint8_t:1 drag_bottom_en", NULL},
    {"drag_left_en", (getter) get_struct_bitfield_tileview_ext_t_drag_left_en, (setter) set_struct_bitfield_tileview_ext_t_drag_left_en, "uint8_t:1 drag_left_en", NULL},
    {"drag_right_en", (getter) get_struct_bitfield_tileview_ext_t_drag_right_en, (setter) set_struct_bitfield_tileview_ext_t_drag_right_en, "uint8_t:1 drag_right_en", NULL},
    {"drag_hor", (getter) get_struct_bitfield_tileview_ext_t_drag_hor, (setter) set_struct_bitfield_tileview_ext_t_drag_hor, "uint8_t:1 drag_hor", NULL},
    {"drag_ver", (getter) get_struct_bitfield_tileview_ext_t_drag_ver, (setter) set_struct_bitfield_tileview_ext_t_drag_ver, "uint8_t:1 drag_ver", NULL},
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
    *(lv_tileview_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_mbox_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_mbox_ext_t_Type, sizeof(lv_mbox_ext_t));
}

static PyGetSetDef pylv_mbox_ext_t_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_cont_ext_t bg", & ((struct_closure_t){ &pylv_cont_ext_t_Type, offsetof(lv_mbox_ext_t, bg), sizeof(lv_cont_ext_t)})},
    {"text", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t text", & ((struct_closure_t){ &Blob_Type, offsetof(lv_mbox_ext_t, text), sizeof(((lv_mbox_ext_t *)0)->text)})},
    {"btnm", (getter) struct_get_struct, (setter) struct_set_struct, "lv_obj_t btnm", & ((struct_closure_t){ &Blob_Type, offsetof(lv_mbox_ext_t, btnm), sizeof(((lv_mbox_ext_t *)0)->btnm)})},
    {"anim_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t anim_time", (void*)offsetof(lv_mbox_ext_t, anim_time)},
    {NULL}
};


static PyTypeObject pylv_mbox_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.mbox_ext_t",
    .tp_doc = "lvgl mbox_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_mbox_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_mbox_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_mbox_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_mbox_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_mbox_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_mbox_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_lmeter_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_lmeter_ext_t_Type, sizeof(lv_lmeter_ext_t));
}

static PyGetSetDef pylv_lmeter_ext_t_getset[] = {
    {"scale_angle", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t scale_angle", (void*)offsetof(lv_lmeter_ext_t, scale_angle)},
    {"line_cnt", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t line_cnt", (void*)offsetof(lv_lmeter_ext_t, line_cnt)},
    {"cur_value", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t cur_value", (void*)offsetof(lv_lmeter_ext_t, cur_value)},
    {"min_value", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t min_value", (void*)offsetof(lv_lmeter_ext_t, min_value)},
    {"max_value", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t max_value", (void*)offsetof(lv_lmeter_ext_t, max_value)},
    {NULL}
};


static PyTypeObject pylv_lmeter_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.lmeter_ext_t",
    .tp_doc = "lvgl lmeter_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_lmeter_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_lmeter_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_lmeter_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_lmeter_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_lmeter_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_lmeter_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_gauge_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_gauge_ext_t_Type, sizeof(lv_gauge_ext_t));
}

static PyGetSetDef pylv_gauge_ext_t_getset[] = {
    {"lmeter", (getter) struct_get_struct, (setter) struct_set_struct, "lv_lmeter_ext_t lmeter", & ((struct_closure_t){ &pylv_lmeter_ext_t_Type, offsetof(lv_gauge_ext_t, lmeter), sizeof(lv_lmeter_ext_t)})},
    {"values", (getter) struct_get_struct, (setter) struct_set_struct, "int16_t values", & ((struct_closure_t){ &Blob_Type, offsetof(lv_gauge_ext_t, values), sizeof(((lv_gauge_ext_t *)0)->values)})},
    {"needle_colors", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t needle_colors", & ((struct_closure_t){ &Blob_Type, offsetof(lv_gauge_ext_t, needle_colors), sizeof(((lv_gauge_ext_t *)0)->needle_colors)})},
    {"needle_count", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t needle_count", (void*)offsetof(lv_gauge_ext_t, needle_count)},
    {"label_count", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t label_count", (void*)offsetof(lv_gauge_ext_t, label_count)},
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
    *(lv_gauge_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_sw_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_sw_ext_t_Type, sizeof(lv_sw_ext_t));
}



static PyObject *
get_struct_bitfield_sw_ext_t_changed(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_sw_ext_t*)(self->data))->changed );
}

static int
set_struct_bitfield_sw_ext_t_changed(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_sw_ext_t*)(self->data))->changed = v;
    return 0;
}



static PyObject *
get_struct_bitfield_sw_ext_t_slided(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_sw_ext_t*)(self->data))->slided );
}

static int
set_struct_bitfield_sw_ext_t_slided(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_sw_ext_t*)(self->data))->slided = v;
    return 0;
}

static PyGetSetDef pylv_sw_ext_t_getset[] = {
    {"slider", (getter) struct_get_struct, (setter) struct_set_struct, "lv_slider_ext_t slider", & ((struct_closure_t){ &pylv_slider_ext_t_Type, offsetof(lv_sw_ext_t, slider), sizeof(lv_slider_ext_t)})},
    {"style_knob_off", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_knob_off", & ((struct_closure_t){ &Blob_Type, offsetof(lv_sw_ext_t, style_knob_off), sizeof(((lv_sw_ext_t *)0)->style_knob_off)})},
    {"style_knob_on", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_knob_on", & ((struct_closure_t){ &Blob_Type, offsetof(lv_sw_ext_t, style_knob_on), sizeof(((lv_sw_ext_t *)0)->style_knob_on)})},
    {"start_x", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t start_x", (void*)offsetof(lv_sw_ext_t, start_x)},
    {"changed", (getter) get_struct_bitfield_sw_ext_t_changed, (setter) set_struct_bitfield_sw_ext_t_changed, "uint8_t:1 changed", NULL},
    {"slided", (getter) get_struct_bitfield_sw_ext_t_slided, (setter) set_struct_bitfield_sw_ext_t_slided, "uint8_t:1 slided", NULL},
    {"anim_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t anim_time", (void*)offsetof(lv_sw_ext_t, anim_time)},
    {NULL}
};


static PyTypeObject pylv_sw_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.sw_ext_t",
    .tp_doc = "lvgl sw_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_sw_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_sw_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_sw_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_sw_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_sw_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_sw_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_arc_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_arc_ext_t_Type, sizeof(lv_arc_ext_t));
}

static PyGetSetDef pylv_arc_ext_t_getset[] = {
    {"angle_start", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t angle_start", (void*)offsetof(lv_arc_ext_t, angle_start)},
    {"angle_end", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t angle_end", (void*)offsetof(lv_arc_ext_t, angle_end)},
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
    *(lv_arc_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_preload_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_preload_ext_t_Type, sizeof(lv_preload_ext_t));
}



static PyObject *
get_struct_bitfield_preload_ext_t_anim_type(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_preload_ext_t*)(self->data))->anim_type );
}

static int
set_struct_bitfield_preload_ext_t_anim_type(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_preload_ext_t*)(self->data))->anim_type = v;
    return 0;
}



static PyObject *
get_struct_bitfield_preload_ext_t_anim_dir(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_preload_ext_t*)(self->data))->anim_dir );
}

static int
set_struct_bitfield_preload_ext_t_anim_dir(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_preload_ext_t*)(self->data))->anim_dir = v;
    return 0;
}

static PyGetSetDef pylv_preload_ext_t_getset[] = {
    {"arc", (getter) struct_get_struct, (setter) struct_set_struct, "lv_arc_ext_t arc", & ((struct_closure_t){ &pylv_arc_ext_t_Type, offsetof(lv_preload_ext_t, arc), sizeof(lv_arc_ext_t)})},
    {"arc_length", (getter) struct_get_int16, (setter) struct_set_int16, "lv_anim_value_t arc_length", (void*)offsetof(lv_preload_ext_t, arc_length)},
    {"time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t time", (void*)offsetof(lv_preload_ext_t, time)},
    {"anim_type", (getter) get_struct_bitfield_preload_ext_t_anim_type, (setter) set_struct_bitfield_preload_ext_t_anim_type, "lv_preload_type_t:1 anim_type", NULL},
    {"anim_dir", (getter) get_struct_bitfield_preload_ext_t_anim_dir, (setter) set_struct_bitfield_preload_ext_t_anim_dir, "lv_preload_dir_t:1 anim_dir", NULL},
    {NULL}
};


static PyTypeObject pylv_preload_ext_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.preload_ext_t",
    .tp_doc = "lvgl preload_ext_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_preload_ext_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_preload_ext_t_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};

static int pylv_preload_ext_t_arg_converter(PyObject *obj, void* target) {
    int isinst;
    // TODO: support dictionary as argument; create a new struct object in that case
    isinst = PyObject_IsInstance(obj, (PyObject*)&pylv_preload_ext_t_Type);
    if (isinst == 0) {
        PyErr_Format(PyExc_TypeError, "argument should be of type lv_preload_ext_t");
    }
    if (isinst != 1) {
        return 0;
    }
    *(lv_preload_ext_t **)target = ((StructObject*)obj) -> data;
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
    *(lv_calendar_date_t **)target = ((StructObject*)obj) -> data;
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
    {"highlighted_dates_num", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t highlighted_dates_num", (void*)offsetof(lv_calendar_ext_t, highlighted_dates_num)},
    {"btn_pressing", (getter) struct_get_int8, (setter) struct_set_int8, "int8_t btn_pressing", (void*)offsetof(lv_calendar_ext_t, btn_pressing)},
    {"pressed_date", (getter) struct_get_struct, (setter) struct_set_struct, "lv_calendar_date_t pressed_date", & ((struct_closure_t){ &pylv_calendar_date_t_Type, offsetof(lv_calendar_ext_t, pressed_date), sizeof(lv_calendar_date_t)})},
    {"day_names", (getter) struct_get_struct, (setter) struct_set_struct, "char day_names", & ((struct_closure_t){ &Blob_Type, offsetof(lv_calendar_ext_t, day_names), sizeof(((lv_calendar_ext_t *)0)->day_names)})},
    {"month_names", (getter) struct_get_struct, (setter) struct_set_struct, "char month_names", & ((struct_closure_t){ &Blob_Type, offsetof(lv_calendar_ext_t, month_names), sizeof(((lv_calendar_ext_t *)0)->month_names)})},
    {"style_header", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_header", & ((struct_closure_t){ &Blob_Type, offsetof(lv_calendar_ext_t, style_header), sizeof(((lv_calendar_ext_t *)0)->style_header)})},
    {"style_header_pr", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_header_pr", & ((struct_closure_t){ &Blob_Type, offsetof(lv_calendar_ext_t, style_header_pr), sizeof(((lv_calendar_ext_t *)0)->style_header_pr)})},
    {"style_day_names", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_day_names", & ((struct_closure_t){ &Blob_Type, offsetof(lv_calendar_ext_t, style_day_names), sizeof(((lv_calendar_ext_t *)0)->style_day_names)})},
    {"style_highlighted_days", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_highlighted_days", & ((struct_closure_t){ &Blob_Type, offsetof(lv_calendar_ext_t, style_highlighted_days), sizeof(((lv_calendar_ext_t *)0)->style_highlighted_days)})},
    {"style_inactive_days", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_inactive_days", & ((struct_closure_t){ &Blob_Type, offsetof(lv_calendar_ext_t, style_inactive_days), sizeof(((lv_calendar_ext_t *)0)->style_inactive_days)})},
    {"style_week_box", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_week_box", & ((struct_closure_t){ &Blob_Type, offsetof(lv_calendar_ext_t, style_week_box), sizeof(((lv_calendar_ext_t *)0)->style_week_box)})},
    {"style_today_box", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style_today_box", & ((struct_closure_t){ &Blob_Type, offsetof(lv_calendar_ext_t, style_today_box), sizeof(((lv_calendar_ext_t *)0)->style_today_box)})},
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
    *(lv_calendar_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}




static int
pylv_spinbox_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{
    return struct_init(self, args, kwds, &pylv_spinbox_ext_t_Type, sizeof(lv_spinbox_ext_t));
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
    {"ta", (getter) struct_get_struct, (setter) struct_set_struct, "lv_ta_ext_t ta", & ((struct_closure_t){ &pylv_ta_ext_t_Type, offsetof(lv_spinbox_ext_t, ta), sizeof(lv_ta_ext_t)})},
    {"value", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t value", (void*)offsetof(lv_spinbox_ext_t, value)},
    {"range_max", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t range_max", (void*)offsetof(lv_spinbox_ext_t, range_max)},
    {"range_min", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t range_min", (void*)offsetof(lv_spinbox_ext_t, range_min)},
    {"step", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t step", (void*)offsetof(lv_spinbox_ext_t, step)},
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
    *(lv_spinbox_ext_t **)target = ((StructObject*)obj) -> data;
    Py_INCREF(obj); // Required since **target now uses the data. TODO: this leaks a reference; also support Py_CLEANUP_SUPPORTED
    return 1;

}








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
    {"pointer", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_point_t act_point;   lv_point_t last_point;   lv_point_t vect;   lv_point_t drag_sum;   lv_point_t drag_throw_vect;   struct _lv_obj_t *act_obj;   struct _lv_obj_t *last_obj;   struct _lv_obj_t *last_pressed;   uint8_t drag_limit_out : 1;   uint8_t drag_in_prog : 1; } pointer", & ((struct_closure_t){ &pylv_indev_proc_t_types_pointer_Type, (offsetof(lv_indev_proc_t, types.pointer)-offsetof(lv_indev_proc_t, types)), sizeof(((lv_indev_proc_t *)0)->types.pointer)})},
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

static PyGetSetDef pylv_indev_proc_t_types_pointer_getset[] = {
    {"act_point", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t act_point", & ((struct_closure_t){ &pylv_point_t_Type, (offsetof(lv_indev_proc_t, types.pointer.act_point)-offsetof(lv_indev_proc_t, types.pointer)), sizeof(lv_point_t)})},
    {"last_point", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t last_point", & ((struct_closure_t){ &pylv_point_t_Type, (offsetof(lv_indev_proc_t, types.pointer.last_point)-offsetof(lv_indev_proc_t, types.pointer)), sizeof(lv_point_t)})},
    {"vect", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t vect", & ((struct_closure_t){ &pylv_point_t_Type, (offsetof(lv_indev_proc_t, types.pointer.vect)-offsetof(lv_indev_proc_t, types.pointer)), sizeof(lv_point_t)})},
    {"drag_sum", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t drag_sum", & ((struct_closure_t){ &pylv_point_t_Type, (offsetof(lv_indev_proc_t, types.pointer.drag_sum)-offsetof(lv_indev_proc_t, types.pointer)), sizeof(lv_point_t)})},
    {"drag_throw_vect", (getter) struct_get_struct, (setter) struct_set_struct, "lv_point_t drag_throw_vect", & ((struct_closure_t){ &pylv_point_t_Type, (offsetof(lv_indev_proc_t, types.pointer.drag_throw_vect)-offsetof(lv_indev_proc_t, types.pointer)), sizeof(lv_point_t)})},
    {"act_obj", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_obj_t act_obj", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_indev_proc_t, types.pointer.act_obj)-offsetof(lv_indev_proc_t, types.pointer)), sizeof(((lv_indev_proc_t *)0)->types.pointer.act_obj)})},
    {"last_obj", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_obj_t last_obj", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_indev_proc_t, types.pointer.last_obj)-offsetof(lv_indev_proc_t, types.pointer)), sizeof(((lv_indev_proc_t *)0)->types.pointer.last_obj)})},
    {"last_pressed", (getter) struct_get_struct, (setter) struct_set_struct, "struct _lv_obj_t last_pressed", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_indev_proc_t, types.pointer.last_pressed)-offsetof(lv_indev_proc_t, types.pointer)), sizeof(((lv_indev_proc_t *)0)->types.pointer.last_pressed)})},
    {"drag_limit_out", (getter) get_struct_bitfield_indev_proc_t_types_pointer_drag_limit_out, (setter) set_struct_bitfield_indev_proc_t_types_pointer_drag_limit_out, "uint8_t:1 drag_limit_out", NULL},
    {"drag_in_prog", (getter) get_struct_bitfield_indev_proc_t_types_pointer_drag_in_prog, (setter) set_struct_bitfield_indev_proc_t_types_pointer_drag_in_prog, "uint8_t:1 drag_in_prog", NULL},
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



static PyGetSetDef pylv_style_t_body_getset[] = {
    {"main_color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t main_color", & ((struct_closure_t){ &pylv_color16_t_Type, (offsetof(lv_style_t, body.main_color)-offsetof(lv_style_t, body)), sizeof(lv_color16_t)})},
    {"grad_color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t grad_color", & ((struct_closure_t){ &pylv_color16_t_Type, (offsetof(lv_style_t, body.grad_color)-offsetof(lv_style_t, body)), sizeof(lv_color16_t)})},
    {"radius", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t radius", (void*)(offsetof(lv_style_t, body.radius)-offsetof(lv_style_t, body))},
    {"opa", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t opa", (void*)(offsetof(lv_style_t, body.opa)-offsetof(lv_style_t, body))},
    {"border", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_color_t color;   lv_coord_t width;   lv_border_part_t part;   lv_opa_t opa; } border", & ((struct_closure_t){ &pylv_style_t_body_border_Type, (offsetof(lv_style_t, body.border)-offsetof(lv_style_t, body)), sizeof(((lv_style_t *)0)->body.border)})},
    {"shadow", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_color_t color;   lv_coord_t width;   lv_shadow_type_t type; } shadow", & ((struct_closure_t){ &pylv_style_t_body_shadow_Type, (offsetof(lv_style_t, body.shadow)-offsetof(lv_style_t, body)), sizeof(((lv_style_t *)0)->body.shadow)})},
    {"padding", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_coord_t top;   lv_coord_t bottom;   lv_coord_t left;   lv_coord_t right;   lv_coord_t inner; } padding", & ((struct_closure_t){ &pylv_style_t_body_padding_Type, (offsetof(lv_style_t, body.padding)-offsetof(lv_style_t, body)), sizeof(((lv_style_t *)0)->body.padding)})},
    {NULL}
};


static PyTypeObject pylv_style_t_body_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.style_t_body",
    .tp_doc = "lvgl style_t_body",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_style_t_body_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_style_t_body_border_getset[] = {
    {"color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t color", & ((struct_closure_t){ &pylv_color16_t_Type, (offsetof(lv_style_t, body.border.color)-offsetof(lv_style_t, body.border)), sizeof(lv_color16_t)})},
    {"width", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t width", (void*)(offsetof(lv_style_t, body.border.width)-offsetof(lv_style_t, body.border))},
    {"part", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_border_part_t part", (void*)(offsetof(lv_style_t, body.border.part)-offsetof(lv_style_t, body.border))},
    {"opa", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t opa", (void*)(offsetof(lv_style_t, body.border.opa)-offsetof(lv_style_t, body.border))},
    {NULL}
};


static PyTypeObject pylv_style_t_body_border_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.style_t_body_border",
    .tp_doc = "lvgl style_t_body_border",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_style_t_body_border_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_style_t_body_shadow_getset[] = {
    {"color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t color", & ((struct_closure_t){ &pylv_color16_t_Type, (offsetof(lv_style_t, body.shadow.color)-offsetof(lv_style_t, body.shadow)), sizeof(lv_color16_t)})},
    {"width", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t width", (void*)(offsetof(lv_style_t, body.shadow.width)-offsetof(lv_style_t, body.shadow))},
    {"type", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_shadow_type_t type", (void*)(offsetof(lv_style_t, body.shadow.type)-offsetof(lv_style_t, body.shadow))},
    {NULL}
};


static PyTypeObject pylv_style_t_body_shadow_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.style_t_body_shadow",
    .tp_doc = "lvgl style_t_body_shadow",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_style_t_body_shadow_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_style_t_body_padding_getset[] = {
    {"top", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t top", (void*)(offsetof(lv_style_t, body.padding.top)-offsetof(lv_style_t, body.padding))},
    {"bottom", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t bottom", (void*)(offsetof(lv_style_t, body.padding.bottom)-offsetof(lv_style_t, body.padding))},
    {"left", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t left", (void*)(offsetof(lv_style_t, body.padding.left)-offsetof(lv_style_t, body.padding))},
    {"right", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t right", (void*)(offsetof(lv_style_t, body.padding.right)-offsetof(lv_style_t, body.padding))},
    {"inner", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t inner", (void*)(offsetof(lv_style_t, body.padding.inner)-offsetof(lv_style_t, body.padding))},
    {NULL}
};


static PyTypeObject pylv_style_t_body_padding_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.style_t_body_padding",
    .tp_doc = "lvgl style_t_body_padding",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_style_t_body_padding_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_style_t_text_getset[] = {
    {"color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t color", & ((struct_closure_t){ &pylv_color16_t_Type, (offsetof(lv_style_t, text.color)-offsetof(lv_style_t, text)), sizeof(lv_color16_t)})},
    {"sel_color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t sel_color", & ((struct_closure_t){ &pylv_color16_t_Type, (offsetof(lv_style_t, text.sel_color)-offsetof(lv_style_t, text)), sizeof(lv_color16_t)})},
    {"font", (getter) struct_get_struct, (setter) struct_set_struct, "lv_font_t font", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_style_t, text.font)-offsetof(lv_style_t, text)), sizeof(((lv_style_t *)0)->text.font)})},
    {"letter_space", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t letter_space", (void*)(offsetof(lv_style_t, text.letter_space)-offsetof(lv_style_t, text))},
    {"line_space", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t line_space", (void*)(offsetof(lv_style_t, text.line_space)-offsetof(lv_style_t, text))},
    {"opa", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t opa", (void*)(offsetof(lv_style_t, text.opa)-offsetof(lv_style_t, text))},
    {NULL}
};


static PyTypeObject pylv_style_t_text_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.style_t_text",
    .tp_doc = "lvgl style_t_text",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_style_t_text_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_style_t_image_getset[] = {
    {"color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t color", & ((struct_closure_t){ &pylv_color16_t_Type, (offsetof(lv_style_t, image.color)-offsetof(lv_style_t, image)), sizeof(lv_color16_t)})},
    {"intense", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t intense", (void*)(offsetof(lv_style_t, image.intense)-offsetof(lv_style_t, image))},
    {"opa", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t opa", (void*)(offsetof(lv_style_t, image.opa)-offsetof(lv_style_t, image))},
    {NULL}
};


static PyTypeObject pylv_style_t_image_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.style_t_image",
    .tp_doc = "lvgl style_t_image",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_style_t_image_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};





static PyObject *
get_struct_bitfield_style_t_line_rounded(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_style_t*)(self->data))->line.rounded );
}

static int
set_struct_bitfield_style_t_line_rounded(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_style_t*)(self->data))->line.rounded = v;
    return 0;
}

static PyGetSetDef pylv_style_t_line_getset[] = {
    {"color", (getter) struct_get_struct, (setter) struct_set_struct, "lv_color_t color", & ((struct_closure_t){ &pylv_color16_t_Type, (offsetof(lv_style_t, line.color)-offsetof(lv_style_t, line)), sizeof(lv_color16_t)})},
    {"width", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t width", (void*)(offsetof(lv_style_t, line.width)-offsetof(lv_style_t, line))},
    {"opa", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t opa", (void*)(offsetof(lv_style_t, line.opa)-offsetof(lv_style_t, line))},
    {"rounded", (getter) get_struct_bitfield_style_t_line_rounded, (setter) set_struct_bitfield_style_t_line_rounded, "uint8_t:1 rounded", NULL},
    {NULL}
};


static PyTypeObject pylv_style_t_line_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.style_t_line",
    .tp_doc = "lvgl style_t_line",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_style_t_line_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.bg)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.bg)})},
    {"panel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t panel", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.panel)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.panel)})},
    {"cont", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t cont", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.cont)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.cont)})},
    {"btn", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *rel;   lv_style_t *pr;   lv_style_t *tgl_rel;   lv_style_t *tgl_pr;   lv_style_t *ina; } btn", & ((struct_closure_t){ &pylv_theme_t_style_btn_Type, (offsetof(lv_theme_t, style.btn)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.btn)})},
    {"imgbtn", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *rel;   lv_style_t *pr;   lv_style_t *tgl_rel;   lv_style_t *tgl_pr;   lv_style_t *ina; } imgbtn", & ((struct_closure_t){ &pylv_theme_t_style_imgbtn_Type, (offsetof(lv_theme_t, style.imgbtn)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.imgbtn)})},
    {"label", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *prim;   lv_style_t *sec;   lv_style_t *hint; } label", & ((struct_closure_t){ &pylv_theme_t_style_label_Type, (offsetof(lv_theme_t, style.label)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.label)})},
    {"img", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *light;   lv_style_t *dark; } img", & ((struct_closure_t){ &pylv_theme_t_style_img_Type, (offsetof(lv_theme_t, style.img)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.img)})},
    {"line", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *decor; } line", & ((struct_closure_t){ &pylv_theme_t_style_line_Type, (offsetof(lv_theme_t, style.line)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.line)})},
    {"led", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t led", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.led)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.led)})},
    {"bar", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   lv_style_t *indic; } bar", & ((struct_closure_t){ &pylv_theme_t_style_bar_Type, (offsetof(lv_theme_t, style.bar)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.bar)})},
    {"slider", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   lv_style_t *indic;   lv_style_t *knob; } slider", & ((struct_closure_t){ &pylv_theme_t_style_slider_Type, (offsetof(lv_theme_t, style.slider)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.slider)})},
    {"lmeter", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t lmeter", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.lmeter)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.lmeter)})},
    {"gauge", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t gauge", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.gauge)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.gauge)})},
    {"arc", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t arc", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.arc)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.arc)})},
    {"preload", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t preload", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.preload)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.preload)})},
    {"sw", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   lv_style_t *indic;   lv_style_t *knob_off;   lv_style_t *knob_on; } sw", & ((struct_closure_t){ &pylv_theme_t_style_sw_Type, (offsetof(lv_theme_t, style.sw)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.sw)})},
    {"chart", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t chart", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.chart)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.chart)})},
    {"calendar", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   lv_style_t *header;   lv_style_t *header_pr;   lv_style_t *day_names;   lv_style_t *highlighted_days;   lv_style_t *inactive_days;   lv_style_t *week_box;   lv_style_t *today_box; } calendar", & ((struct_closure_t){ &pylv_theme_t_style_calendar_Type, (offsetof(lv_theme_t, style.calendar)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.calendar)})},
    {"cb", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   struct    {     lv_style_t *rel;     lv_style_t *pr;     lv_style_t *tgl_rel;     lv_style_t *tgl_pr;     lv_style_t *ina;   } box; } cb", & ((struct_closure_t){ &pylv_theme_t_style_cb_Type, (offsetof(lv_theme_t, style.cb)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.cb)})},
    {"btnm", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   struct    {     lv_style_t *rel;     lv_style_t *pr;     lv_style_t *tgl_rel;     lv_style_t *tgl_pr;     lv_style_t *ina;   } btn; } btnm", & ((struct_closure_t){ &pylv_theme_t_style_btnm_Type, (offsetof(lv_theme_t, style.btnm)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.btnm)})},
    {"kb", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   struct    {     lv_style_t *rel;     lv_style_t *pr;     lv_style_t *tgl_rel;     lv_style_t *tgl_pr;     lv_style_t *ina;   } btn; } kb", & ((struct_closure_t){ &pylv_theme_t_style_kb_Type, (offsetof(lv_theme_t, style.kb)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.kb)})},
    {"mbox", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   struct    {     lv_style_t *bg;     lv_style_t *rel;     lv_style_t *pr;   } btn; } mbox", & ((struct_closure_t){ &pylv_theme_t_style_mbox_Type, (offsetof(lv_theme_t, style.mbox)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.mbox)})},
    {"page", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   lv_style_t *scrl;   lv_style_t *sb; } page", & ((struct_closure_t){ &pylv_theme_t_style_page_Type, (offsetof(lv_theme_t, style.page)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.page)})},
    {"ta", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *area;   lv_style_t *oneline;   lv_style_t *cursor;   lv_style_t *sb; } ta", & ((struct_closure_t){ &pylv_theme_t_style_ta_Type, (offsetof(lv_theme_t, style.ta)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.ta)})},
    {"spinbox", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   lv_style_t *cursor;   lv_style_t *sb; } spinbox", & ((struct_closure_t){ &pylv_theme_t_style_spinbox_Type, (offsetof(lv_theme_t, style.spinbox)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.spinbox)})},
    {"list", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   lv_style_t *scrl;   lv_style_t *sb;   struct    {     lv_style_t *rel;     lv_style_t *pr;     lv_style_t *tgl_rel;     lv_style_t *tgl_pr;     lv_style_t *ina;   } btn; } list", & ((struct_closure_t){ &pylv_theme_t_style_list_Type, (offsetof(lv_theme_t, style.list)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.list)})},
    {"ddlist", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   lv_style_t *sel;   lv_style_t *sb; } ddlist", & ((struct_closure_t){ &pylv_theme_t_style_ddlist_Type, (offsetof(lv_theme_t, style.ddlist)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.ddlist)})},
    {"roller", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   lv_style_t *sel; } roller", & ((struct_closure_t){ &pylv_theme_t_style_roller_Type, (offsetof(lv_theme_t, style.roller)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.roller)})},
    {"tabview", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   lv_style_t *indic;   struct    {     lv_style_t *bg;     lv_style_t *rel;     lv_style_t *pr;     lv_style_t *tgl_rel;     lv_style_t *tgl_pr;   } btn; } tabview", & ((struct_closure_t){ &pylv_theme_t_style_tabview_Type, (offsetof(lv_theme_t, style.tabview)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.tabview)})},
    {"tileview", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   lv_style_t *scrl;   lv_style_t *sb; } tileview", & ((struct_closure_t){ &pylv_theme_t_style_tileview_Type, (offsetof(lv_theme_t, style.tileview)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.tileview)})},
    {"table", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   lv_style_t *cell; } table", & ((struct_closure_t){ &pylv_theme_t_style_table_Type, (offsetof(lv_theme_t, style.table)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.table)})},
    {"win", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   lv_style_t *sb;   lv_style_t *header;   struct    {     lv_style_t *bg;     lv_style_t *scrl;   } content;   struct    {     lv_style_t *rel;     lv_style_t *pr;   } btn; } win", & ((struct_closure_t){ &pylv_theme_t_style_win_Type, (offsetof(lv_theme_t, style.win)-offsetof(lv_theme_t, style)), sizeof(((lv_theme_t *)0)->style.win)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style",
    .tp_doc = "lvgl theme_t_style",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_btn_getset[] = {
    {"rel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t rel", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.btn.rel)-offsetof(lv_theme_t, style.btn)), sizeof(((lv_theme_t *)0)->style.btn.rel)})},
    {"pr", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t pr", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.btn.pr)-offsetof(lv_theme_t, style.btn)), sizeof(((lv_theme_t *)0)->style.btn.pr)})},
    {"tgl_rel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t tgl_rel", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.btn.tgl_rel)-offsetof(lv_theme_t, style.btn)), sizeof(((lv_theme_t *)0)->style.btn.tgl_rel)})},
    {"tgl_pr", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t tgl_pr", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.btn.tgl_pr)-offsetof(lv_theme_t, style.btn)), sizeof(((lv_theme_t *)0)->style.btn.tgl_pr)})},
    {"ina", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t ina", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.btn.ina)-offsetof(lv_theme_t, style.btn)), sizeof(((lv_theme_t *)0)->style.btn.ina)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_btn_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_btn",
    .tp_doc = "lvgl theme_t_style_btn",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_btn_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_imgbtn_getset[] = {
    {"rel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t rel", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.imgbtn.rel)-offsetof(lv_theme_t, style.imgbtn)), sizeof(((lv_theme_t *)0)->style.imgbtn.rel)})},
    {"pr", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t pr", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.imgbtn.pr)-offsetof(lv_theme_t, style.imgbtn)), sizeof(((lv_theme_t *)0)->style.imgbtn.pr)})},
    {"tgl_rel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t tgl_rel", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.imgbtn.tgl_rel)-offsetof(lv_theme_t, style.imgbtn)), sizeof(((lv_theme_t *)0)->style.imgbtn.tgl_rel)})},
    {"tgl_pr", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t tgl_pr", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.imgbtn.tgl_pr)-offsetof(lv_theme_t, style.imgbtn)), sizeof(((lv_theme_t *)0)->style.imgbtn.tgl_pr)})},
    {"ina", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t ina", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.imgbtn.ina)-offsetof(lv_theme_t, style.imgbtn)), sizeof(((lv_theme_t *)0)->style.imgbtn.ina)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_imgbtn_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_imgbtn",
    .tp_doc = "lvgl theme_t_style_imgbtn",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_imgbtn_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_label_getset[] = {
    {"prim", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t prim", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.label.prim)-offsetof(lv_theme_t, style.label)), sizeof(((lv_theme_t *)0)->style.label.prim)})},
    {"sec", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t sec", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.label.sec)-offsetof(lv_theme_t, style.label)), sizeof(((lv_theme_t *)0)->style.label.sec)})},
    {"hint", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t hint", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.label.hint)-offsetof(lv_theme_t, style.label)), sizeof(((lv_theme_t *)0)->style.label.hint)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_label_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_label",
    .tp_doc = "lvgl theme_t_style_label",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_label_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_img_getset[] = {
    {"light", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t light", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.img.light)-offsetof(lv_theme_t, style.img)), sizeof(((lv_theme_t *)0)->style.img.light)})},
    {"dark", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t dark", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.img.dark)-offsetof(lv_theme_t, style.img)), sizeof(((lv_theme_t *)0)->style.img.dark)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_img_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_img",
    .tp_doc = "lvgl theme_t_style_img",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_img_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_line_getset[] = {
    {"decor", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t decor", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.line.decor)-offsetof(lv_theme_t, style.line)), sizeof(((lv_theme_t *)0)->style.line.decor)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_line_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_line",
    .tp_doc = "lvgl theme_t_style_line",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_line_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_bar_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.bar.bg)-offsetof(lv_theme_t, style.bar)), sizeof(((lv_theme_t *)0)->style.bar.bg)})},
    {"indic", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t indic", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.bar.indic)-offsetof(lv_theme_t, style.bar)), sizeof(((lv_theme_t *)0)->style.bar.indic)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_bar_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_bar",
    .tp_doc = "lvgl theme_t_style_bar",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_bar_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_slider_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.slider.bg)-offsetof(lv_theme_t, style.slider)), sizeof(((lv_theme_t *)0)->style.slider.bg)})},
    {"indic", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t indic", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.slider.indic)-offsetof(lv_theme_t, style.slider)), sizeof(((lv_theme_t *)0)->style.slider.indic)})},
    {"knob", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t knob", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.slider.knob)-offsetof(lv_theme_t, style.slider)), sizeof(((lv_theme_t *)0)->style.slider.knob)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_slider_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_slider",
    .tp_doc = "lvgl theme_t_style_slider",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_slider_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_sw_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.sw.bg)-offsetof(lv_theme_t, style.sw)), sizeof(((lv_theme_t *)0)->style.sw.bg)})},
    {"indic", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t indic", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.sw.indic)-offsetof(lv_theme_t, style.sw)), sizeof(((lv_theme_t *)0)->style.sw.indic)})},
    {"knob_off", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t knob_off", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.sw.knob_off)-offsetof(lv_theme_t, style.sw)), sizeof(((lv_theme_t *)0)->style.sw.knob_off)})},
    {"knob_on", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t knob_on", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.sw.knob_on)-offsetof(lv_theme_t, style.sw)), sizeof(((lv_theme_t *)0)->style.sw.knob_on)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_sw_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_sw",
    .tp_doc = "lvgl theme_t_style_sw",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_sw_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_calendar_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.calendar.bg)-offsetof(lv_theme_t, style.calendar)), sizeof(((lv_theme_t *)0)->style.calendar.bg)})},
    {"header", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t header", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.calendar.header)-offsetof(lv_theme_t, style.calendar)), sizeof(((lv_theme_t *)0)->style.calendar.header)})},
    {"header_pr", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t header_pr", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.calendar.header_pr)-offsetof(lv_theme_t, style.calendar)), sizeof(((lv_theme_t *)0)->style.calendar.header_pr)})},
    {"day_names", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t day_names", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.calendar.day_names)-offsetof(lv_theme_t, style.calendar)), sizeof(((lv_theme_t *)0)->style.calendar.day_names)})},
    {"highlighted_days", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t highlighted_days", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.calendar.highlighted_days)-offsetof(lv_theme_t, style.calendar)), sizeof(((lv_theme_t *)0)->style.calendar.highlighted_days)})},
    {"inactive_days", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t inactive_days", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.calendar.inactive_days)-offsetof(lv_theme_t, style.calendar)), sizeof(((lv_theme_t *)0)->style.calendar.inactive_days)})},
    {"week_box", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t week_box", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.calendar.week_box)-offsetof(lv_theme_t, style.calendar)), sizeof(((lv_theme_t *)0)->style.calendar.week_box)})},
    {"today_box", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t today_box", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.calendar.today_box)-offsetof(lv_theme_t, style.calendar)), sizeof(((lv_theme_t *)0)->style.calendar.today_box)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_calendar_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_calendar",
    .tp_doc = "lvgl theme_t_style_calendar",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_calendar_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_cb_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.cb.bg)-offsetof(lv_theme_t, style.cb)), sizeof(((lv_theme_t *)0)->style.cb.bg)})},
    {"box", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *rel;   lv_style_t *pr;   lv_style_t *tgl_rel;   lv_style_t *tgl_pr;   lv_style_t *ina; } box", & ((struct_closure_t){ &pylv_theme_t_style_cb_box_Type, (offsetof(lv_theme_t, style.cb.box)-offsetof(lv_theme_t, style.cb)), sizeof(((lv_theme_t *)0)->style.cb.box)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_cb_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_cb",
    .tp_doc = "lvgl theme_t_style_cb",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_cb_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_cb_box_getset[] = {
    {"rel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t rel", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.cb.box.rel)-offsetof(lv_theme_t, style.cb.box)), sizeof(((lv_theme_t *)0)->style.cb.box.rel)})},
    {"pr", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t pr", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.cb.box.pr)-offsetof(lv_theme_t, style.cb.box)), sizeof(((lv_theme_t *)0)->style.cb.box.pr)})},
    {"tgl_rel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t tgl_rel", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.cb.box.tgl_rel)-offsetof(lv_theme_t, style.cb.box)), sizeof(((lv_theme_t *)0)->style.cb.box.tgl_rel)})},
    {"tgl_pr", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t tgl_pr", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.cb.box.tgl_pr)-offsetof(lv_theme_t, style.cb.box)), sizeof(((lv_theme_t *)0)->style.cb.box.tgl_pr)})},
    {"ina", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t ina", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.cb.box.ina)-offsetof(lv_theme_t, style.cb.box)), sizeof(((lv_theme_t *)0)->style.cb.box.ina)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_cb_box_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_cb_box",
    .tp_doc = "lvgl theme_t_style_cb_box",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_cb_box_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_btnm_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.btnm.bg)-offsetof(lv_theme_t, style.btnm)), sizeof(((lv_theme_t *)0)->style.btnm.bg)})},
    {"btn", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *rel;   lv_style_t *pr;   lv_style_t *tgl_rel;   lv_style_t *tgl_pr;   lv_style_t *ina; } btn", & ((struct_closure_t){ &pylv_theme_t_style_btnm_btn_Type, (offsetof(lv_theme_t, style.btnm.btn)-offsetof(lv_theme_t, style.btnm)), sizeof(((lv_theme_t *)0)->style.btnm.btn)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_btnm_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_btnm",
    .tp_doc = "lvgl theme_t_style_btnm",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_btnm_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_btnm_btn_getset[] = {
    {"rel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t rel", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.btnm.btn.rel)-offsetof(lv_theme_t, style.btnm.btn)), sizeof(((lv_theme_t *)0)->style.btnm.btn.rel)})},
    {"pr", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t pr", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.btnm.btn.pr)-offsetof(lv_theme_t, style.btnm.btn)), sizeof(((lv_theme_t *)0)->style.btnm.btn.pr)})},
    {"tgl_rel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t tgl_rel", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.btnm.btn.tgl_rel)-offsetof(lv_theme_t, style.btnm.btn)), sizeof(((lv_theme_t *)0)->style.btnm.btn.tgl_rel)})},
    {"tgl_pr", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t tgl_pr", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.btnm.btn.tgl_pr)-offsetof(lv_theme_t, style.btnm.btn)), sizeof(((lv_theme_t *)0)->style.btnm.btn.tgl_pr)})},
    {"ina", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t ina", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.btnm.btn.ina)-offsetof(lv_theme_t, style.btnm.btn)), sizeof(((lv_theme_t *)0)->style.btnm.btn.ina)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_btnm_btn_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_btnm_btn",
    .tp_doc = "lvgl theme_t_style_btnm_btn",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_btnm_btn_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_kb_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.kb.bg)-offsetof(lv_theme_t, style.kb)), sizeof(((lv_theme_t *)0)->style.kb.bg)})},
    {"btn", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *rel;   lv_style_t *pr;   lv_style_t *tgl_rel;   lv_style_t *tgl_pr;   lv_style_t *ina; } btn", & ((struct_closure_t){ &pylv_theme_t_style_kb_btn_Type, (offsetof(lv_theme_t, style.kb.btn)-offsetof(lv_theme_t, style.kb)), sizeof(((lv_theme_t *)0)->style.kb.btn)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_kb_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_kb",
    .tp_doc = "lvgl theme_t_style_kb",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_kb_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_kb_btn_getset[] = {
    {"rel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t rel", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.kb.btn.rel)-offsetof(lv_theme_t, style.kb.btn)), sizeof(((lv_theme_t *)0)->style.kb.btn.rel)})},
    {"pr", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t pr", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.kb.btn.pr)-offsetof(lv_theme_t, style.kb.btn)), sizeof(((lv_theme_t *)0)->style.kb.btn.pr)})},
    {"tgl_rel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t tgl_rel", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.kb.btn.tgl_rel)-offsetof(lv_theme_t, style.kb.btn)), sizeof(((lv_theme_t *)0)->style.kb.btn.tgl_rel)})},
    {"tgl_pr", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t tgl_pr", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.kb.btn.tgl_pr)-offsetof(lv_theme_t, style.kb.btn)), sizeof(((lv_theme_t *)0)->style.kb.btn.tgl_pr)})},
    {"ina", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t ina", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.kb.btn.ina)-offsetof(lv_theme_t, style.kb.btn)), sizeof(((lv_theme_t *)0)->style.kb.btn.ina)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_kb_btn_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_kb_btn",
    .tp_doc = "lvgl theme_t_style_kb_btn",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_kb_btn_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_mbox_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.mbox.bg)-offsetof(lv_theme_t, style.mbox)), sizeof(((lv_theme_t *)0)->style.mbox.bg)})},
    {"btn", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   lv_style_t *rel;   lv_style_t *pr; } btn", & ((struct_closure_t){ &pylv_theme_t_style_mbox_btn_Type, (offsetof(lv_theme_t, style.mbox.btn)-offsetof(lv_theme_t, style.mbox)), sizeof(((lv_theme_t *)0)->style.mbox.btn)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_mbox_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_mbox",
    .tp_doc = "lvgl theme_t_style_mbox",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_mbox_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_mbox_btn_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.mbox.btn.bg)-offsetof(lv_theme_t, style.mbox.btn)), sizeof(((lv_theme_t *)0)->style.mbox.btn.bg)})},
    {"rel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t rel", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.mbox.btn.rel)-offsetof(lv_theme_t, style.mbox.btn)), sizeof(((lv_theme_t *)0)->style.mbox.btn.rel)})},
    {"pr", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t pr", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.mbox.btn.pr)-offsetof(lv_theme_t, style.mbox.btn)), sizeof(((lv_theme_t *)0)->style.mbox.btn.pr)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_mbox_btn_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_mbox_btn",
    .tp_doc = "lvgl theme_t_style_mbox_btn",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_mbox_btn_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_page_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.page.bg)-offsetof(lv_theme_t, style.page)), sizeof(((lv_theme_t *)0)->style.page.bg)})},
    {"scrl", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t scrl", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.page.scrl)-offsetof(lv_theme_t, style.page)), sizeof(((lv_theme_t *)0)->style.page.scrl)})},
    {"sb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t sb", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.page.sb)-offsetof(lv_theme_t, style.page)), sizeof(((lv_theme_t *)0)->style.page.sb)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_page_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_page",
    .tp_doc = "lvgl theme_t_style_page",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_page_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_ta_getset[] = {
    {"area", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t area", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.ta.area)-offsetof(lv_theme_t, style.ta)), sizeof(((lv_theme_t *)0)->style.ta.area)})},
    {"oneline", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t oneline", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.ta.oneline)-offsetof(lv_theme_t, style.ta)), sizeof(((lv_theme_t *)0)->style.ta.oneline)})},
    {"cursor", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t cursor", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.ta.cursor)-offsetof(lv_theme_t, style.ta)), sizeof(((lv_theme_t *)0)->style.ta.cursor)})},
    {"sb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t sb", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.ta.sb)-offsetof(lv_theme_t, style.ta)), sizeof(((lv_theme_t *)0)->style.ta.sb)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_ta_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_ta",
    .tp_doc = "lvgl theme_t_style_ta",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_ta_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_spinbox_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.spinbox.bg)-offsetof(lv_theme_t, style.spinbox)), sizeof(((lv_theme_t *)0)->style.spinbox.bg)})},
    {"cursor", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t cursor", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.spinbox.cursor)-offsetof(lv_theme_t, style.spinbox)), sizeof(((lv_theme_t *)0)->style.spinbox.cursor)})},
    {"sb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t sb", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.spinbox.sb)-offsetof(lv_theme_t, style.spinbox)), sizeof(((lv_theme_t *)0)->style.spinbox.sb)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_spinbox_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_spinbox",
    .tp_doc = "lvgl theme_t_style_spinbox",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_spinbox_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_list_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.list.bg)-offsetof(lv_theme_t, style.list)), sizeof(((lv_theme_t *)0)->style.list.bg)})},
    {"scrl", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t scrl", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.list.scrl)-offsetof(lv_theme_t, style.list)), sizeof(((lv_theme_t *)0)->style.list.scrl)})},
    {"sb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t sb", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.list.sb)-offsetof(lv_theme_t, style.list)), sizeof(((lv_theme_t *)0)->style.list.sb)})},
    {"btn", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *rel;   lv_style_t *pr;   lv_style_t *tgl_rel;   lv_style_t *tgl_pr;   lv_style_t *ina; } btn", & ((struct_closure_t){ &pylv_theme_t_style_list_btn_Type, (offsetof(lv_theme_t, style.list.btn)-offsetof(lv_theme_t, style.list)), sizeof(((lv_theme_t *)0)->style.list.btn)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_list_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_list",
    .tp_doc = "lvgl theme_t_style_list",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_list_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_list_btn_getset[] = {
    {"rel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t rel", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.list.btn.rel)-offsetof(lv_theme_t, style.list.btn)), sizeof(((lv_theme_t *)0)->style.list.btn.rel)})},
    {"pr", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t pr", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.list.btn.pr)-offsetof(lv_theme_t, style.list.btn)), sizeof(((lv_theme_t *)0)->style.list.btn.pr)})},
    {"tgl_rel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t tgl_rel", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.list.btn.tgl_rel)-offsetof(lv_theme_t, style.list.btn)), sizeof(((lv_theme_t *)0)->style.list.btn.tgl_rel)})},
    {"tgl_pr", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t tgl_pr", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.list.btn.tgl_pr)-offsetof(lv_theme_t, style.list.btn)), sizeof(((lv_theme_t *)0)->style.list.btn.tgl_pr)})},
    {"ina", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t ina", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.list.btn.ina)-offsetof(lv_theme_t, style.list.btn)), sizeof(((lv_theme_t *)0)->style.list.btn.ina)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_list_btn_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_list_btn",
    .tp_doc = "lvgl theme_t_style_list_btn",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_list_btn_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_ddlist_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.ddlist.bg)-offsetof(lv_theme_t, style.ddlist)), sizeof(((lv_theme_t *)0)->style.ddlist.bg)})},
    {"sel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t sel", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.ddlist.sel)-offsetof(lv_theme_t, style.ddlist)), sizeof(((lv_theme_t *)0)->style.ddlist.sel)})},
    {"sb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t sb", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.ddlist.sb)-offsetof(lv_theme_t, style.ddlist)), sizeof(((lv_theme_t *)0)->style.ddlist.sb)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_ddlist_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_ddlist",
    .tp_doc = "lvgl theme_t_style_ddlist",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_ddlist_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_roller_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.roller.bg)-offsetof(lv_theme_t, style.roller)), sizeof(((lv_theme_t *)0)->style.roller.bg)})},
    {"sel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t sel", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.roller.sel)-offsetof(lv_theme_t, style.roller)), sizeof(((lv_theme_t *)0)->style.roller.sel)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_roller_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_roller",
    .tp_doc = "lvgl theme_t_style_roller",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_roller_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_tabview_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.tabview.bg)-offsetof(lv_theme_t, style.tabview)), sizeof(((lv_theme_t *)0)->style.tabview.bg)})},
    {"indic", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t indic", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.tabview.indic)-offsetof(lv_theme_t, style.tabview)), sizeof(((lv_theme_t *)0)->style.tabview.indic)})},
    {"btn", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   lv_style_t *rel;   lv_style_t *pr;   lv_style_t *tgl_rel;   lv_style_t *tgl_pr; } btn", & ((struct_closure_t){ &pylv_theme_t_style_tabview_btn_Type, (offsetof(lv_theme_t, style.tabview.btn)-offsetof(lv_theme_t, style.tabview)), sizeof(((lv_theme_t *)0)->style.tabview.btn)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_tabview_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_tabview",
    .tp_doc = "lvgl theme_t_style_tabview",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_tabview_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_tabview_btn_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.tabview.btn.bg)-offsetof(lv_theme_t, style.tabview.btn)), sizeof(((lv_theme_t *)0)->style.tabview.btn.bg)})},
    {"rel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t rel", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.tabview.btn.rel)-offsetof(lv_theme_t, style.tabview.btn)), sizeof(((lv_theme_t *)0)->style.tabview.btn.rel)})},
    {"pr", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t pr", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.tabview.btn.pr)-offsetof(lv_theme_t, style.tabview.btn)), sizeof(((lv_theme_t *)0)->style.tabview.btn.pr)})},
    {"tgl_rel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t tgl_rel", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.tabview.btn.tgl_rel)-offsetof(lv_theme_t, style.tabview.btn)), sizeof(((lv_theme_t *)0)->style.tabview.btn.tgl_rel)})},
    {"tgl_pr", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t tgl_pr", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.tabview.btn.tgl_pr)-offsetof(lv_theme_t, style.tabview.btn)), sizeof(((lv_theme_t *)0)->style.tabview.btn.tgl_pr)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_tabview_btn_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_tabview_btn",
    .tp_doc = "lvgl theme_t_style_tabview_btn",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_tabview_btn_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_tileview_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.tileview.bg)-offsetof(lv_theme_t, style.tileview)), sizeof(((lv_theme_t *)0)->style.tileview.bg)})},
    {"scrl", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t scrl", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.tileview.scrl)-offsetof(lv_theme_t, style.tileview)), sizeof(((lv_theme_t *)0)->style.tileview.scrl)})},
    {"sb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t sb", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.tileview.sb)-offsetof(lv_theme_t, style.tileview)), sizeof(((lv_theme_t *)0)->style.tileview.sb)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_tileview_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_tileview",
    .tp_doc = "lvgl theme_t_style_tileview",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_tileview_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_table_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.table.bg)-offsetof(lv_theme_t, style.table)), sizeof(((lv_theme_t *)0)->style.table.bg)})},
    {"cell", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t cell", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.table.cell)-offsetof(lv_theme_t, style.table)), sizeof(((lv_theme_t *)0)->style.table.cell)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_table_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_table",
    .tp_doc = "lvgl theme_t_style_table",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_table_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_win_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.win.bg)-offsetof(lv_theme_t, style.win)), sizeof(((lv_theme_t *)0)->style.win.bg)})},
    {"sb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t sb", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.win.sb)-offsetof(lv_theme_t, style.win)), sizeof(((lv_theme_t *)0)->style.win.sb)})},
    {"header", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t header", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.win.header)-offsetof(lv_theme_t, style.win)), sizeof(((lv_theme_t *)0)->style.win.header)})},
    {"content", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *bg;   lv_style_t *scrl; } content", & ((struct_closure_t){ &pylv_theme_t_style_win_content_Type, (offsetof(lv_theme_t, style.win.content)-offsetof(lv_theme_t, style.win)), sizeof(((lv_theme_t *)0)->style.win.content)})},
    {"btn", (getter) struct_get_struct, (setter) struct_set_struct, "struct  {   lv_style_t *rel;   lv_style_t *pr; } btn", & ((struct_closure_t){ &pylv_theme_t_style_win_btn_Type, (offsetof(lv_theme_t, style.win.btn)-offsetof(lv_theme_t, style.win)), sizeof(((lv_theme_t *)0)->style.win.btn)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_win_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_win",
    .tp_doc = "lvgl theme_t_style_win",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_win_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_win_content_getset[] = {
    {"bg", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t bg", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.win.content.bg)-offsetof(lv_theme_t, style.win.content)), sizeof(((lv_theme_t *)0)->style.win.content.bg)})},
    {"scrl", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t scrl", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.win.content.scrl)-offsetof(lv_theme_t, style.win.content)), sizeof(((lv_theme_t *)0)->style.win.content.scrl)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_win_content_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_win_content",
    .tp_doc = "lvgl theme_t_style_win_content",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_win_content_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_style_win_btn_getset[] = {
    {"rel", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t rel", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.win.btn.rel)-offsetof(lv_theme_t, style.win.btn)), sizeof(((lv_theme_t *)0)->style.win.btn.rel)})},
    {"pr", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t pr", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, style.win.btn.pr)-offsetof(lv_theme_t, style.win.btn)), sizeof(((lv_theme_t *)0)->style.win.btn.pr)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_style_win_btn_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_style_win_btn",
    .tp_doc = "lvgl theme_t_style_win_btn",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_style_win_btn_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_theme_t_group_getset[] = {
    {"style_mod_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_group_style_mod_cb_t style_mod_cb", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, group.style_mod_cb)-offsetof(lv_theme_t, group)), sizeof(((lv_theme_t *)0)->group.style_mod_cb)})},
    {"style_mod_edit_cb", (getter) struct_get_struct, (setter) struct_set_struct, "lv_group_style_mod_cb_t style_mod_edit_cb", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_theme_t, group.style_mod_edit_cb)-offsetof(lv_theme_t, group)), sizeof(((lv_theme_t *)0)->group.style_mod_edit_cb)})},
    {NULL}
};


static PyTypeObject pylv_theme_t_group_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.theme_t_group",
    .tp_doc = "lvgl theme_t_group",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_theme_t_group_getset,
    .tp_repr = (reprfunc) Struct_repr,
    .tp_as_buffer = &Struct_bufferprocs
};



static PyGetSetDef pylv_label_ext_t_dot_getset[] = {
    {"tmp_ptr", (getter) struct_get_struct, (setter) struct_set_struct, "char tmp_ptr", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_label_ext_t, dot.tmp_ptr)-offsetof(lv_label_ext_t, dot)), sizeof(((lv_label_ext_t *)0)->dot.tmp_ptr)})},
    {"tmp", (getter) struct_get_struct, (setter) struct_set_struct, "charsizeof(char *) tmp", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_label_ext_t, dot.tmp)-offsetof(lv_label_ext_t, dot)), sizeof(((lv_label_ext_t *)0)->dot.tmp)})},
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
get_struct_bitfield_page_ext_t_sb_hor_draw(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_page_ext_t*)(self->data))->sb.hor_draw );
}

static int
set_struct_bitfield_page_ext_t_sb_hor_draw(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_page_ext_t*)(self->data))->sb.hor_draw = v;
    return 0;
}



static PyObject *
get_struct_bitfield_page_ext_t_sb_ver_draw(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_page_ext_t*)(self->data))->sb.ver_draw );
}

static int
set_struct_bitfield_page_ext_t_sb_ver_draw(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_page_ext_t*)(self->data))->sb.ver_draw = v;
    return 0;
}



static PyObject *
get_struct_bitfield_page_ext_t_sb_mode(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_page_ext_t*)(self->data))->sb.mode );
}

static int
set_struct_bitfield_page_ext_t_sb_mode(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 7)) return -1;
    ((lv_page_ext_t*)(self->data))->sb.mode = v;
    return 0;
}

static PyGetSetDef pylv_page_ext_t_sb_getset[] = {
    {"style", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_page_ext_t, sb.style)-offsetof(lv_page_ext_t, sb)), sizeof(((lv_page_ext_t *)0)->sb.style)})},
    {"hor_area", (getter) struct_get_struct, (setter) struct_set_struct, "lv_area_t hor_area", & ((struct_closure_t){ &pylv_area_t_Type, (offsetof(lv_page_ext_t, sb.hor_area)-offsetof(lv_page_ext_t, sb)), sizeof(lv_area_t)})},
    {"ver_area", (getter) struct_get_struct, (setter) struct_set_struct, "lv_area_t ver_area", & ((struct_closure_t){ &pylv_area_t_Type, (offsetof(lv_page_ext_t, sb.ver_area)-offsetof(lv_page_ext_t, sb)), sizeof(lv_area_t)})},
    {"hor_draw", (getter) get_struct_bitfield_page_ext_t_sb_hor_draw, (setter) set_struct_bitfield_page_ext_t_sb_hor_draw, "uint8_t:1 hor_draw", NULL},
    {"ver_draw", (getter) get_struct_bitfield_page_ext_t_sb_ver_draw, (setter) set_struct_bitfield_page_ext_t_sb_ver_draw, "uint8_t:1 ver_draw", NULL},
    {"mode", (getter) get_struct_bitfield_page_ext_t_sb_mode, (setter) set_struct_bitfield_page_ext_t_sb_mode, "lv_sb_mode_t:3 mode", NULL},
    {NULL}
};


static PyTypeObject pylv_page_ext_t_sb_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.page_ext_t_sb",
    .tp_doc = "lvgl page_ext_t_sb",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_page_ext_t_sb_getset,
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
    {"style", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_page_ext_t, edge_flash.style)-offsetof(lv_page_ext_t, edge_flash)), sizeof(((lv_page_ext_t *)0)->edge_flash.style)})},
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



static PyGetSetDef pylv_chart_ext_t_series_getset[] = {
    {"width", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t width", (void*)(offsetof(lv_chart_ext_t, series.width)-offsetof(lv_chart_ext_t, series))},
    {"num", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t num", (void*)(offsetof(lv_chart_ext_t, series.num)-offsetof(lv_chart_ext_t, series))},
    {"opa", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t opa", (void*)(offsetof(lv_chart_ext_t, series.opa)-offsetof(lv_chart_ext_t, series))},
    {"dark", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t dark", (void*)(offsetof(lv_chart_ext_t, series.dark)-offsetof(lv_chart_ext_t, series))},
    {NULL}
};


static PyTypeObject pylv_chart_ext_t_series_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.chart_ext_t_series",
    .tp_doc = "lvgl chart_ext_t_series",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_chart_ext_t_series_getset,
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
    if (long_to_int(value, &v, 0, 3)) return -1;
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
    {"type", (getter) get_struct_bitfield_table_cell_format_t_s_type, (setter) set_struct_bitfield_table_cell_format_t_s_type, "uint8_t:2 type", NULL},
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
get_struct_bitfield_ta_ext_t_cursor_type(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_ta_ext_t*)(self->data))->cursor.type );
}

static int
set_struct_bitfield_ta_ext_t_cursor_type(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 15)) return -1;
    ((lv_ta_ext_t*)(self->data))->cursor.type = v;
    return 0;
}



static PyObject *
get_struct_bitfield_ta_ext_t_cursor_state(StructObject *self, void *closure)
{
    return PyLong_FromLong(((lv_ta_ext_t*)(self->data))->cursor.state );
}

static int
set_struct_bitfield_ta_ext_t_cursor_state(StructObject *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 1)) return -1;
    ((lv_ta_ext_t*)(self->data))->cursor.state = v;
    return 0;
}

static PyGetSetDef pylv_ta_ext_t_cursor_getset[] = {
    {"style", (getter) struct_get_struct, (setter) struct_set_struct, "lv_style_t style", & ((struct_closure_t){ &Blob_Type, (offsetof(lv_ta_ext_t, cursor.style)-offsetof(lv_ta_ext_t, cursor)), sizeof(((lv_ta_ext_t *)0)->cursor.style)})},
    {"valid_x", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t valid_x", (void*)(offsetof(lv_ta_ext_t, cursor.valid_x)-offsetof(lv_ta_ext_t, cursor))},
    {"pos", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t pos", (void*)(offsetof(lv_ta_ext_t, cursor.pos)-offsetof(lv_ta_ext_t, cursor))},
    {"area", (getter) struct_get_struct, (setter) struct_set_struct, "lv_area_t area", & ((struct_closure_t){ &pylv_area_t_Type, (offsetof(lv_ta_ext_t, cursor.area)-offsetof(lv_ta_ext_t, cursor)), sizeof(lv_area_t)})},
    {"txt_byte_pos", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t txt_byte_pos", (void*)(offsetof(lv_ta_ext_t, cursor.txt_byte_pos)-offsetof(lv_ta_ext_t, cursor))},
    {"type", (getter) get_struct_bitfield_ta_ext_t_cursor_type, (setter) set_struct_bitfield_ta_ext_t_cursor_type, "lv_cursor_type_t:4 type", NULL},
    {"state", (getter) get_struct_bitfield_ta_ext_t_cursor_state, (setter) set_struct_bitfield_ta_ext_t_cursor_state, "uint8_t:1 state", NULL},
    {NULL}
};


static PyTypeObject pylv_ta_ext_t_cursor_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.ta_ext_t_cursor",
    .tp_doc = "lvgl ta_ext_t_cursor",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = NULL, // sub structs cannot be instantiated
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_ta_ext_t_cursor_getset,
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
    pylv_Obj *self = (pylv_Obj *)*lv_obj_get_user_data_ptr(obj);
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
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
pylv_obj_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"base", "align", "x_mod", "y_mod", NULL};
    pylv_Obj * base;
    unsigned char align;
    short int x_mod;
    short int y_mod;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!bhh", kwlist , &pylv_obj_Type, &base, &align, &x_mod, &y_mod)) return NULL;

    LVGL_LOCK         
    lv_obj_align(self->ref, base->ref, align, x_mod, y_mod);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_align_origo(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"base", "align", "x_mod", "y_mod", NULL};
    pylv_Obj * base;
    unsigned char align;
    short int x_mod;
    short int y_mod;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!bhh", kwlist , &pylv_obj_Type, &base, &align, &x_mod, &y_mod)) return NULL;

    LVGL_LOCK         
    lv_obj_align_origo(self->ref, base->ref, align, x_mod, y_mod);
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
pylv_obj_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"style", NULL};
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&", kwlist , pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style(self->ref, style);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_refresh_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_obj_refresh_style(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
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
pylv_obj_set_opa_scale_enable(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_obj_set_opa_scale_enable(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_opa_scale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"opa_scale", NULL};
    unsigned char opa_scale;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &opa_scale)) return NULL;

    LVGL_LOCK         
    lv_obj_set_opa_scale(self->ref, opa_scale);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_protect(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"prot", NULL};
    unsigned char prot;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &prot)) return NULL;

    LVGL_LOCK         
    lv_obj_set_protect(self->ref, prot);
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
pylv_obj_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_obj_get_style(self->ref);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
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
pylv_obj_get_opa_scale_enable(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_opa_t result = lv_obj_get_opa_scale_enable(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_get_opa_scale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_opa_t result = lv_obj_get_opa_scale(self->ref);
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


static PyMethodDef pylv_obj_methods[] = {
    {"del_", (PyCFunction) pylv_obj_del, METH_VARARGS | METH_KEYWORDS, "lv_res_t lv_obj_del(lv_obj_t *obj)"},
    {"clean", (PyCFunction) pylv_obj_clean, METH_VARARGS | METH_KEYWORDS, "void lv_obj_clean(lv_obj_t *obj)"},
    {"invalidate", (PyCFunction) pylv_obj_invalidate, METH_VARARGS | METH_KEYWORDS, "void lv_obj_invalidate(const lv_obj_t *obj)"},
    {"set_parent", (PyCFunction) pylv_obj_set_parent, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_parent(lv_obj_t *obj, lv_obj_t *parent)"},
    {"move_foreground", (PyCFunction) pylv_obj_move_foreground, METH_VARARGS | METH_KEYWORDS, "void lv_obj_move_foreground(lv_obj_t *obj)"},
    {"move_background", (PyCFunction) pylv_obj_move_background, METH_VARARGS | METH_KEYWORDS, "void lv_obj_move_background(lv_obj_t *obj)"},
    {"set_pos", (PyCFunction) pylv_obj_set_pos, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_pos(lv_obj_t *obj, lv_coord_t x, lv_coord_t y)"},
    {"set_x", (PyCFunction) pylv_obj_set_x, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_x(lv_obj_t *obj, lv_coord_t x)"},
    {"set_y", (PyCFunction) pylv_obj_set_y, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_y(lv_obj_t *obj, lv_coord_t y)"},
    {"set_size", (PyCFunction) pylv_obj_set_size, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_size(lv_obj_t *obj, lv_coord_t w, lv_coord_t h)"},
    {"set_width", (PyCFunction) pylv_obj_set_width, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_width(lv_obj_t *obj, lv_coord_t w)"},
    {"set_height", (PyCFunction) pylv_obj_set_height, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_height(lv_obj_t *obj, lv_coord_t h)"},
    {"align", (PyCFunction) pylv_obj_align, METH_VARARGS | METH_KEYWORDS, "void lv_obj_align(lv_obj_t *obj, const lv_obj_t *base, lv_align_t align, lv_coord_t x_mod, lv_coord_t y_mod)"},
    {"align_origo", (PyCFunction) pylv_obj_align_origo, METH_VARARGS | METH_KEYWORDS, "void lv_obj_align_origo(lv_obj_t *obj, const lv_obj_t *base, lv_align_t align, lv_coord_t x_mod, lv_coord_t y_mod)"},
    {"realign", (PyCFunction) pylv_obj_realign, METH_VARARGS | METH_KEYWORDS, "void lv_obj_realign(lv_obj_t *obj)"},
    {"set_auto_realign", (PyCFunction) pylv_obj_set_auto_realign, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_auto_realign(lv_obj_t *obj, bool en)"},
    {"set_ext_click_area", (PyCFunction) pylv_obj_set_ext_click_area, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_ext_click_area(lv_obj_t *obj, lv_coord_t left, lv_coord_t right, lv_coord_t top, lv_coord_t bottom)"},
    {"set_style", (PyCFunction) pylv_obj_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_style(lv_obj_t *obj, const lv_style_t *style)"},
    {"refresh_style", (PyCFunction) pylv_obj_refresh_style, METH_VARARGS | METH_KEYWORDS, "void lv_obj_refresh_style(lv_obj_t *obj)"},
    {"set_hidden", (PyCFunction) pylv_obj_set_hidden, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_hidden(lv_obj_t *obj, bool en)"},
    {"set_click", (PyCFunction) pylv_obj_set_click, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_click(lv_obj_t *obj, bool en)"},
    {"set_top", (PyCFunction) pylv_obj_set_top, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_top(lv_obj_t *obj, bool en)"},
    {"set_drag", (PyCFunction) pylv_obj_set_drag, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_drag(lv_obj_t *obj, bool en)"},
    {"set_drag_dir", (PyCFunction) pylv_obj_set_drag_dir, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_drag_dir(lv_obj_t *obj, lv_drag_dir_t drag_dir)"},
    {"set_drag_throw", (PyCFunction) pylv_obj_set_drag_throw, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_drag_throw(lv_obj_t *obj, bool en)"},
    {"set_drag_parent", (PyCFunction) pylv_obj_set_drag_parent, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_drag_parent(lv_obj_t *obj, bool en)"},
    {"set_parent_event", (PyCFunction) pylv_obj_set_parent_event, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_parent_event(lv_obj_t *obj, bool en)"},
    {"set_opa_scale_enable", (PyCFunction) pylv_obj_set_opa_scale_enable, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_opa_scale_enable(lv_obj_t *obj, bool en)"},
    {"set_opa_scale", (PyCFunction) pylv_obj_set_opa_scale, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_opa_scale(lv_obj_t *obj, lv_opa_t opa_scale)"},
    {"set_protect", (PyCFunction) pylv_obj_set_protect, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_protect(lv_obj_t *obj, uint8_t prot)"},
    {"clear_protect", (PyCFunction) pylv_obj_clear_protect, METH_VARARGS | METH_KEYWORDS, "void lv_obj_clear_protect(lv_obj_t *obj, uint8_t prot)"},
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
    {"get_x", (PyCFunction) pylv_obj_get_x, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_x(const lv_obj_t *obj)"},
    {"get_y", (PyCFunction) pylv_obj_get_y, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_y(const lv_obj_t *obj)"},
    {"get_width", (PyCFunction) pylv_obj_get_width, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_width(const lv_obj_t *obj)"},
    {"get_height", (PyCFunction) pylv_obj_get_height, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_height(const lv_obj_t *obj)"},
    {"get_width_fit", (PyCFunction) pylv_obj_get_width_fit, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_width_fit(lv_obj_t *obj)"},
    {"get_height_fit", (PyCFunction) pylv_obj_get_height_fit, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_height_fit(lv_obj_t *obj)"},
    {"get_auto_realign", (PyCFunction) pylv_obj_get_auto_realign, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_auto_realign(lv_obj_t *obj)"},
    {"get_ext_click_pad_left", (PyCFunction) pylv_obj_get_ext_click_pad_left, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_ext_click_pad_left(const lv_obj_t *obj)"},
    {"get_ext_click_pad_right", (PyCFunction) pylv_obj_get_ext_click_pad_right, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_ext_click_pad_right(const lv_obj_t *obj)"},
    {"get_ext_click_pad_top", (PyCFunction) pylv_obj_get_ext_click_pad_top, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_ext_click_pad_top(const lv_obj_t *obj)"},
    {"get_ext_click_pad_bottom", (PyCFunction) pylv_obj_get_ext_click_pad_bottom, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_ext_click_pad_bottom(const lv_obj_t *obj)"},
    {"get_ext_draw_pad", (PyCFunction) pylv_obj_get_ext_draw_pad, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_ext_draw_pad(const lv_obj_t *obj)"},
    {"get_style", (PyCFunction) pylv_obj_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_obj_get_style(const lv_obj_t *obj)"},
    {"get_hidden", (PyCFunction) pylv_obj_get_hidden, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_hidden(const lv_obj_t *obj)"},
    {"get_click", (PyCFunction) pylv_obj_get_click, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_click(const lv_obj_t *obj)"},
    {"get_top", (PyCFunction) pylv_obj_get_top, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_top(const lv_obj_t *obj)"},
    {"get_drag", (PyCFunction) pylv_obj_get_drag, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_drag(const lv_obj_t *obj)"},
    {"get_drag_dir", (PyCFunction) pylv_obj_get_drag_dir, METH_VARARGS | METH_KEYWORDS, "lv_drag_dir_t lv_obj_get_drag_dir(const lv_obj_t *obj)"},
    {"get_drag_throw", (PyCFunction) pylv_obj_get_drag_throw, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_drag_throw(const lv_obj_t *obj)"},
    {"get_drag_parent", (PyCFunction) pylv_obj_get_drag_parent, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_drag_parent(const lv_obj_t *obj)"},
    {"get_parent_event", (PyCFunction) pylv_obj_get_parent_event, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_parent_event(const lv_obj_t *obj)"},
    {"get_opa_scale_enable", (PyCFunction) pylv_obj_get_opa_scale_enable, METH_VARARGS | METH_KEYWORDS, "lv_opa_t lv_obj_get_opa_scale_enable(const lv_obj_t *obj)"},
    {"get_opa_scale", (PyCFunction) pylv_obj_get_opa_scale, METH_VARARGS | METH_KEYWORDS, "lv_opa_t lv_obj_get_opa_scale(const lv_obj_t *obj)"},
    {"get_protect", (PyCFunction) pylv_obj_get_protect, METH_VARARGS | METH_KEYWORDS, "uint8_t lv_obj_get_protect(const lv_obj_t *obj)"},
    {"is_protected", (PyCFunction) pylv_obj_is_protected, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_is_protected(const lv_obj_t *obj, uint8_t prot)"},
    {"get_signal_cb", (PyCFunction) pylv_obj_get_signal_cb, METH_VARARGS | METH_KEYWORDS, "lv_signal_cb_t lv_obj_get_signal_cb(const lv_obj_t *obj)"},
    {"get_design_cb", (PyCFunction) pylv_obj_get_design_cb, METH_VARARGS | METH_KEYWORDS, "lv_design_cb_t lv_obj_get_design_cb(const lv_obj_t *obj)"},
    {"get_event_cb", (PyCFunction) pylv_obj_get_event_cb, METH_VARARGS | METH_KEYWORDS, "lv_event_cb_t lv_obj_get_event_cb(const lv_obj_t *obj)"},
    {"get_type", (PyCFunction) pylv_obj_get_type, METH_VARARGS | METH_KEYWORDS, ""},
    {"get_group", (PyCFunction) pylv_obj_get_group, METH_VARARGS | METH_KEYWORDS, "void *lv_obj_get_group(const lv_obj_t *obj)"},
    {"is_focused", (PyCFunction) pylv_obj_is_focused, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_is_focused(const lv_obj_t *obj)"},
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_btn_set_toggle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"tgl", NULL};
    int tgl;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &tgl)) return NULL;

    LVGL_LOCK         
    lv_btn_set_toggle(self->ref, tgl);
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
pylv_btn_set_ink_in_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"time", NULL};
    unsigned short int time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &time)) return NULL;

    LVGL_LOCK         
    lv_btn_set_ink_in_time(self->ref, time);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btn_set_ink_wait_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"time", NULL};
    unsigned short int time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &time)) return NULL;

    LVGL_LOCK         
    lv_btn_set_ink_wait_time(self->ref, time);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btn_set_ink_out_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"time", NULL};
    unsigned short int time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &time)) return NULL;

    LVGL_LOCK         
    lv_btn_set_ink_out_time(self->ref, time);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btn_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_btn_set_style(self->ref, type, style);
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
pylv_btn_get_toggle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_btn_get_toggle(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_btn_get_ink_in_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_btn_get_ink_in_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_btn_get_ink_wait_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_btn_get_ink_wait_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_btn_get_ink_out_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_btn_get_ink_out_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_btn_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_btn_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}


static PyMethodDef pylv_btn_methods[] = {
    {"set_toggle", (PyCFunction) pylv_btn_set_toggle, METH_VARARGS | METH_KEYWORDS, "void lv_btn_set_toggle(lv_obj_t *btn, bool tgl)"},
    {"set_state", (PyCFunction) pylv_btn_set_state, METH_VARARGS | METH_KEYWORDS, "void lv_btn_set_state(lv_obj_t *btn, lv_btn_state_t state)"},
    {"toggle", (PyCFunction) pylv_btn_toggle, METH_VARARGS | METH_KEYWORDS, "void lv_btn_toggle(lv_obj_t *btn)"},
    {"set_ink_in_time", (PyCFunction) pylv_btn_set_ink_in_time, METH_VARARGS | METH_KEYWORDS, "void lv_btn_set_ink_in_time(lv_obj_t *btn, uint16_t time)"},
    {"set_ink_wait_time", (PyCFunction) pylv_btn_set_ink_wait_time, METH_VARARGS | METH_KEYWORDS, "void lv_btn_set_ink_wait_time(lv_obj_t *btn, uint16_t time)"},
    {"set_ink_out_time", (PyCFunction) pylv_btn_set_ink_out_time, METH_VARARGS | METH_KEYWORDS, "void lv_btn_set_ink_out_time(lv_obj_t *btn, uint16_t time)"},
    {"set_style", (PyCFunction) pylv_btn_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_btn_set_style(lv_obj_t *btn, lv_btn_style_t type, const lv_style_t *style)"},
    {"get_state", (PyCFunction) pylv_btn_get_state, METH_VARARGS | METH_KEYWORDS, "lv_btn_state_t lv_btn_get_state(const lv_obj_t *btn)"},
    {"get_toggle", (PyCFunction) pylv_btn_get_toggle, METH_VARARGS | METH_KEYWORDS, "bool lv_btn_get_toggle(const lv_obj_t *btn)"},
    {"get_ink_in_time", (PyCFunction) pylv_btn_get_ink_in_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_btn_get_ink_in_time(const lv_obj_t *btn)"},
    {"get_ink_wait_time", (PyCFunction) pylv_btn_get_ink_wait_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_btn_get_ink_wait_time(const lv_obj_t *btn)"},
    {"get_ink_out_time", (PyCFunction) pylv_btn_get_ink_out_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_btn_get_ink_out_time(const lv_obj_t *btn)"},
    {"get_style", (PyCFunction) pylv_btn_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_btn_get_style(const lv_obj_t *btn, lv_btn_style_t type)"},
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
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
pylv_imgbtn_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_imgbtn_set_style(self->ref, type, style);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_imgbtn_get_src(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_imgbtn_get_src: Return type not found >const void*< ");
    return NULL;
}

static PyObject*
pylv_imgbtn_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_imgbtn_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}


static PyMethodDef pylv_imgbtn_methods[] = {
    {"set_src", (PyCFunction) pylv_imgbtn_set_src, METH_VARARGS | METH_KEYWORDS, "void lv_imgbtn_set_src(lv_obj_t *imgbtn, lv_btn_state_t state, const void *src)"},
    {"set_style", (PyCFunction) pylv_imgbtn_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_imgbtn_set_style(lv_obj_t *imgbtn, lv_imgbtn_style_t type, const lv_style_t *style)"},
    {"get_src", (PyCFunction) pylv_imgbtn_get_src, METH_VARARGS | METH_KEYWORDS, "const void *lv_imgbtn_get_src(lv_obj_t *imgbtn, lv_btn_state_t state)"},
    {"get_style", (PyCFunction) pylv_imgbtn_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_imgbtn_get_style(const lv_obj_t *imgbtn, lv_imgbtn_style_t type)"},
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
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
pylv_label_set_array_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"array", "size", NULL};
    const char * array;
    unsigned short int size;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "sH", kwlist , &array, &size)) return NULL;

    LVGL_LOCK         
    lv_label_set_array_text(self->ref, array, size);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_static_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"text", NULL};
    const char * text;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &text)) return NULL;

    LVGL_LOCK         
    lv_label_set_static_text(self->ref, text);
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
pylv_label_set_body_draw(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_label_set_body_draw(self->ref, en);
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
    unsigned short int index;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &index)) return NULL;

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
    unsigned short int index;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &index)) return NULL;

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
pylv_label_get_body_draw(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_label_get_body_draw(self->ref);
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
    uint16_t result = lv_label_get_text_sel_start(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_label_get_text_sel_end(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_label_get_text_sel_end(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
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


static PyMethodDef pylv_label_methods[] = {
    {"set_text", (PyCFunction) pylv_label_set_text, METH_VARARGS | METH_KEYWORDS, "void lv_label_set_text(lv_obj_t *label, const char *text)"},
    {"set_array_text", (PyCFunction) pylv_label_set_array_text, METH_VARARGS | METH_KEYWORDS, "void lv_label_set_array_text(lv_obj_t *label, const char *array, uint16_t size)"},
    {"set_static_text", (PyCFunction) pylv_label_set_static_text, METH_VARARGS | METH_KEYWORDS, "void lv_label_set_static_text(lv_obj_t *label, const char *text)"},
    {"set_long_mode", (PyCFunction) pylv_label_set_long_mode, METH_VARARGS | METH_KEYWORDS, "void lv_label_set_long_mode(lv_obj_t *label, lv_label_long_mode_t long_mode)"},
    {"set_align", (PyCFunction) pylv_label_set_align, METH_VARARGS | METH_KEYWORDS, "void lv_label_set_align(lv_obj_t *label, lv_label_align_t align)"},
    {"set_recolor", (PyCFunction) pylv_label_set_recolor, METH_VARARGS | METH_KEYWORDS, "void lv_label_set_recolor(lv_obj_t *label, bool en)"},
    {"set_body_draw", (PyCFunction) pylv_label_set_body_draw, METH_VARARGS | METH_KEYWORDS, "void lv_label_set_body_draw(lv_obj_t *label, bool en)"},
    {"set_anim_speed", (PyCFunction) pylv_label_set_anim_speed, METH_VARARGS | METH_KEYWORDS, "void lv_label_set_anim_speed(lv_obj_t *label, uint16_t anim_speed)"},
    {"set_text_sel_start", (PyCFunction) pylv_label_set_text_sel_start, METH_VARARGS | METH_KEYWORDS, "void lv_label_set_text_sel_start(lv_obj_t *label, uint16_t index)"},
    {"set_text_sel_end", (PyCFunction) pylv_label_set_text_sel_end, METH_VARARGS | METH_KEYWORDS, "void lv_label_set_text_sel_end(lv_obj_t *label, uint16_t index)"},
    {"get_text", (PyCFunction) pylv_label_get_text, METH_VARARGS | METH_KEYWORDS, "char *lv_label_get_text(const lv_obj_t *label)"},
    {"get_long_mode", (PyCFunction) pylv_label_get_long_mode, METH_VARARGS | METH_KEYWORDS, "lv_label_long_mode_t lv_label_get_long_mode(const lv_obj_t *label)"},
    {"get_align", (PyCFunction) pylv_label_get_align, METH_VARARGS | METH_KEYWORDS, "lv_label_align_t lv_label_get_align(const lv_obj_t *label)"},
    {"get_recolor", (PyCFunction) pylv_label_get_recolor, METH_VARARGS | METH_KEYWORDS, "bool lv_label_get_recolor(const lv_obj_t *label)"},
    {"get_body_draw", (PyCFunction) pylv_label_get_body_draw, METH_VARARGS | METH_KEYWORDS, "bool lv_label_get_body_draw(const lv_obj_t *label)"},
    {"get_anim_speed", (PyCFunction) pylv_label_get_anim_speed, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_label_get_anim_speed(const lv_obj_t *label)"},
    {"get_letter_pos", (PyCFunction) pylv_label_get_letter_pos, METH_VARARGS | METH_KEYWORDS, ""},
    {"get_letter_on", (PyCFunction) pylv_label_get_letter_on, METH_VARARGS | METH_KEYWORDS, ""},
    {"is_char_under_pos", (PyCFunction) pylv_label_is_char_under_pos, METH_VARARGS | METH_KEYWORDS, "bool lv_label_is_char_under_pos(const lv_obj_t *label, lv_point_t *pos)"},
    {"get_text_sel_start", (PyCFunction) pylv_label_get_text_sel_start, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_label_get_text_sel_start(const lv_obj_t *label)"},
    {"get_text_sel_end", (PyCFunction) pylv_label_get_text_sel_end, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_label_get_text_sel_end(const lv_obj_t *label)"},
    {"ins_text", (PyCFunction) pylv_label_ins_text, METH_VARARGS | METH_KEYWORDS, "void lv_label_ins_text(lv_obj_t *label, uint32_t pos, const char *txt)"},
    {"cut_text", (PyCFunction) pylv_label_cut_text, METH_VARARGS | METH_KEYWORDS, "void lv_label_cut_text(lv_obj_t *label, uint32_t pos, uint32_t cnt)"},
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
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
pylv_img_set_file(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"fn", NULL};
    const char * fn;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &fn)) return NULL;

    LVGL_LOCK         
    lv_img_set_file(self->ref, fn);
    LVGL_UNLOCK
    Py_RETURN_NONE;
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
pylv_img_set_offset(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"x", "y", NULL};
    short int x;
    short int y;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &x, &y)) return NULL;

    LVGL_LOCK         
    lv_img_set_offset(self->ref, x, y);
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
pylv_img_set_upscale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"upcale", NULL};
    int upcale;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &upcale)) return NULL;

    LVGL_LOCK         
    lv_img_set_upscale(self->ref, upcale);
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
pylv_img_get_upscale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_img_get_upscale(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_img_methods[] = {
    {"set_src", (PyCFunction) pylv_img_set_src, METH_VARARGS | METH_KEYWORDS, "void lv_img_set_src(lv_obj_t *img, const void *src_img)"},
    {"set_file", (PyCFunction) pylv_img_set_file, METH_VARARGS | METH_KEYWORDS, "inline static void lv_img_set_file(lv_obj_t *img, const char *fn)"},
    {"set_auto_size", (PyCFunction) pylv_img_set_auto_size, METH_VARARGS | METH_KEYWORDS, "void lv_img_set_auto_size(lv_obj_t *img, bool autosize_en)"},
    {"set_offset", (PyCFunction) pylv_img_set_offset, METH_VARARGS | METH_KEYWORDS, "void lv_img_set_offset(lv_obj_t *img, lv_coord_t x, lv_coord_t y)"},
    {"set_offset_x", (PyCFunction) pylv_img_set_offset_x, METH_VARARGS | METH_KEYWORDS, "void lv_img_set_offset_x(lv_obj_t *img, lv_coord_t x)"},
    {"set_offset_y", (PyCFunction) pylv_img_set_offset_y, METH_VARARGS | METH_KEYWORDS, "void lv_img_set_offset_y(lv_obj_t *img, lv_coord_t y)"},
    {"set_upscale", (PyCFunction) pylv_img_set_upscale, METH_VARARGS | METH_KEYWORDS, "inline static void lv_img_set_upscale(lv_obj_t *img, bool upcale)"},
    {"get_src", (PyCFunction) pylv_img_get_src, METH_VARARGS | METH_KEYWORDS, "const void *lv_img_get_src(lv_obj_t *img)"},
    {"get_file_name", (PyCFunction) pylv_img_get_file_name, METH_VARARGS | METH_KEYWORDS, "const char *lv_img_get_file_name(const lv_obj_t *img)"},
    {"get_auto_size", (PyCFunction) pylv_img_get_auto_size, METH_VARARGS | METH_KEYWORDS, "bool lv_img_get_auto_size(const lv_obj_t *img)"},
    {"get_offset_x", (PyCFunction) pylv_img_get_offset_x, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_img_get_offset_x(lv_obj_t *img)"},
    {"get_offset_y", (PyCFunction) pylv_img_get_offset_y, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_img_get_offset_y(lv_obj_t *img)"},
    {"get_upscale", (PyCFunction) pylv_img_get_upscale, METH_VARARGS | METH_KEYWORDS, "inline static bool lv_img_get_upscale(const lv_obj_t *img)"},
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_line_set_points(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_line_set_points: Parameter type not found >const lv_point_t*< ");
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
pylv_line_set_upscale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"upcale", NULL};
    int upcale;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &upcale)) return NULL;

    LVGL_LOCK         
    lv_line_set_upscale(self->ref, upcale);
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

static PyObject*
pylv_line_get_upscale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_line_get_upscale(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_line_methods[] = {
    {"set_points", (PyCFunction) pylv_line_set_points, METH_VARARGS | METH_KEYWORDS, "void lv_line_set_points(lv_obj_t *line, const lv_point_t *point_a, uint16_t point_num)"},
    {"set_auto_size", (PyCFunction) pylv_line_set_auto_size, METH_VARARGS | METH_KEYWORDS, "void lv_line_set_auto_size(lv_obj_t *line, bool en)"},
    {"set_y_invert", (PyCFunction) pylv_line_set_y_invert, METH_VARARGS | METH_KEYWORDS, "void lv_line_set_y_invert(lv_obj_t *line, bool en)"},
    {"set_upscale", (PyCFunction) pylv_line_set_upscale, METH_VARARGS | METH_KEYWORDS, "inline static void lv_line_set_upscale(lv_obj_t *line, bool upcale)"},
    {"get_auto_size", (PyCFunction) pylv_line_get_auto_size, METH_VARARGS | METH_KEYWORDS, "bool lv_line_get_auto_size(const lv_obj_t *line)"},
    {"get_y_invert", (PyCFunction) pylv_line_get_y_invert, METH_VARARGS | METH_KEYWORDS, "bool lv_line_get_y_invert(const lv_obj_t *line)"},
    {"get_upscale", (PyCFunction) pylv_line_get_upscale, METH_VARARGS | METH_KEYWORDS, "inline static bool lv_line_get_upscale(const lv_obj_t *line)"},
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
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

static PyObject*
pylv_page_set_sb_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"sb_mode", NULL};
    unsigned char sb_mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &sb_mode)) return NULL;

    LVGL_LOCK         
    lv_page_set_sb_mode(self->ref, sb_mode);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_arrow_scroll(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_page_set_arrow_scroll(self->ref, en);
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
pylv_page_set_scrl_fit4(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"left", "right", "top", "bottom", NULL};
    unsigned char left;
    unsigned char right;
    unsigned char top;
    unsigned char bottom;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bbbb", kwlist , &left, &right, &top, &bottom)) return NULL;

    LVGL_LOCK         
    lv_page_set_scrl_fit4(self->ref, left, right, top, bottom);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scrl_fit2(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"hor", "ver", NULL};
    unsigned char hor;
    unsigned char ver;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bb", kwlist , &hor, &ver)) return NULL;

    LVGL_LOCK         
    lv_page_set_scrl_fit2(self->ref, hor, ver);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scrl_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"fit", NULL};
    unsigned char fit;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &fit)) return NULL;

    LVGL_LOCK         
    lv_page_set_scrl_fit(self->ref, fit);
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
pylv_page_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_page_set_style(self->ref, type, style);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_get_sb_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_sb_mode_t result = lv_page_get_sb_mode(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_page_get_arrow_scroll(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_page_get_arrow_scroll(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
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
pylv_page_get_fit_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_page_get_fit_width(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_page_get_fit_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_page_get_fit_height(self->ref);
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
pylv_page_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_page_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
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
    static char *kwlist[] = {"obj", "anim_time", NULL};
    pylv_Obj * obj;
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!H", kwlist , &pylv_obj_Type, &obj, &anim_time)) return NULL;

    LVGL_LOCK         
    lv_page_focus(self->ref, obj->ref, anim_time);
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
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_page_start_edge_flash(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}


static PyMethodDef pylv_page_methods[] = {
    {"clean", (PyCFunction) pylv_page_clean, METH_VARARGS | METH_KEYWORDS, "void lv_page_clean(lv_obj_t *obj)"},
    {"get_scrl", (PyCFunction) pylv_page_get_scrl, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_page_get_scrl(const lv_obj_t *page)"},
    {"set_sb_mode", (PyCFunction) pylv_page_set_sb_mode, METH_VARARGS | METH_KEYWORDS, "void lv_page_set_sb_mode(lv_obj_t *page, lv_sb_mode_t sb_mode)"},
    {"set_arrow_scroll", (PyCFunction) pylv_page_set_arrow_scroll, METH_VARARGS | METH_KEYWORDS, "void lv_page_set_arrow_scroll(lv_obj_t *page, bool en)"},
    {"set_scroll_propagation", (PyCFunction) pylv_page_set_scroll_propagation, METH_VARARGS | METH_KEYWORDS, "void lv_page_set_scroll_propagation(lv_obj_t *page, bool en)"},
    {"set_edge_flash", (PyCFunction) pylv_page_set_edge_flash, METH_VARARGS | METH_KEYWORDS, "void lv_page_set_edge_flash(lv_obj_t *page, bool en)"},
    {"set_scrl_fit4", (PyCFunction) pylv_page_set_scrl_fit4, METH_VARARGS | METH_KEYWORDS, "inline static void lv_page_set_scrl_fit4(lv_obj_t *page, lv_fit_t left, lv_fit_t right, lv_fit_t top, lv_fit_t bottom)"},
    {"set_scrl_fit2", (PyCFunction) pylv_page_set_scrl_fit2, METH_VARARGS | METH_KEYWORDS, "inline static void lv_page_set_scrl_fit2(lv_obj_t *page, lv_fit_t hor, lv_fit_t ver)"},
    {"set_scrl_fit", (PyCFunction) pylv_page_set_scrl_fit, METH_VARARGS | METH_KEYWORDS, "inline static void lv_page_set_scrl_fit(lv_obj_t *page, lv_fit_t fit)"},
    {"set_scrl_width", (PyCFunction) pylv_page_set_scrl_width, METH_VARARGS | METH_KEYWORDS, "inline static void lv_page_set_scrl_width(lv_obj_t *page, lv_coord_t w)"},
    {"set_scrl_height", (PyCFunction) pylv_page_set_scrl_height, METH_VARARGS | METH_KEYWORDS, "inline static void lv_page_set_scrl_height(lv_obj_t *page, lv_coord_t h)"},
    {"set_scrl_layout", (PyCFunction) pylv_page_set_scrl_layout, METH_VARARGS | METH_KEYWORDS, "inline static void lv_page_set_scrl_layout(lv_obj_t *page, lv_layout_t layout)"},
    {"set_style", (PyCFunction) pylv_page_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_page_set_style(lv_obj_t *page, lv_page_style_t type, const lv_style_t *style)"},
    {"get_sb_mode", (PyCFunction) pylv_page_get_sb_mode, METH_VARARGS | METH_KEYWORDS, "lv_sb_mode_t lv_page_get_sb_mode(const lv_obj_t *page)"},
    {"get_arrow_scroll", (PyCFunction) pylv_page_get_arrow_scroll, METH_VARARGS | METH_KEYWORDS, "bool lv_page_get_arrow_scroll(const lv_obj_t *page)"},
    {"get_scroll_propagation", (PyCFunction) pylv_page_get_scroll_propagation, METH_VARARGS | METH_KEYWORDS, "bool lv_page_get_scroll_propagation(lv_obj_t *page)"},
    {"get_edge_flash", (PyCFunction) pylv_page_get_edge_flash, METH_VARARGS | METH_KEYWORDS, "bool lv_page_get_edge_flash(lv_obj_t *page)"},
    {"get_fit_width", (PyCFunction) pylv_page_get_fit_width, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_page_get_fit_width(lv_obj_t *page)"},
    {"get_fit_height", (PyCFunction) pylv_page_get_fit_height, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_page_get_fit_height(lv_obj_t *page)"},
    {"get_scrl_width", (PyCFunction) pylv_page_get_scrl_width, METH_VARARGS | METH_KEYWORDS, "inline static lv_coord_t lv_page_get_scrl_width(const lv_obj_t *page)"},
    {"get_scrl_height", (PyCFunction) pylv_page_get_scrl_height, METH_VARARGS | METH_KEYWORDS, "inline static lv_coord_t lv_page_get_scrl_height(const lv_obj_t *page)"},
    {"get_scrl_layout", (PyCFunction) pylv_page_get_scrl_layout, METH_VARARGS | METH_KEYWORDS, "inline static lv_layout_t lv_page_get_scrl_layout(const lv_obj_t *page)"},
    {"get_scrl_fit_left", (PyCFunction) pylv_page_get_scrl_fit_left, METH_VARARGS | METH_KEYWORDS, "inline static lv_fit_t lv_page_get_scrl_fit_left(const lv_obj_t *page)"},
    {"get_scrl_fit_right", (PyCFunction) pylv_page_get_scrl_fit_right, METH_VARARGS | METH_KEYWORDS, "inline static lv_fit_t lv_page_get_scrl_fit_right(const lv_obj_t *page)"},
    {"get_scrl_fit_top", (PyCFunction) pylv_page_get_scrl_fit_top, METH_VARARGS | METH_KEYWORDS, "inline static lv_fit_t lv_page_get_scrl_fit_top(const lv_obj_t *page)"},
    {"get_scrl_fit_bottom", (PyCFunction) pylv_page_get_scrl_fit_bottom, METH_VARARGS | METH_KEYWORDS, "inline static lv_fit_t lv_page_get_scrl_fit_bottom(const lv_obj_t *page)"},
    {"get_style", (PyCFunction) pylv_page_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_page_get_style(const lv_obj_t *page, lv_page_style_t type)"},
    {"on_edge", (PyCFunction) pylv_page_on_edge, METH_VARARGS | METH_KEYWORDS, "bool lv_page_on_edge(lv_obj_t *page, lv_page_edge_t edge)"},
    {"glue_obj", (PyCFunction) pylv_page_glue_obj, METH_VARARGS | METH_KEYWORDS, "void lv_page_glue_obj(lv_obj_t *obj, bool glue)"},
    {"focus", (PyCFunction) pylv_page_focus, METH_VARARGS | METH_KEYWORDS, "void lv_page_focus(lv_obj_t *page, const lv_obj_t *obj, uint16_t anim_time)"},
    {"scroll_hor", (PyCFunction) pylv_page_scroll_hor, METH_VARARGS | METH_KEYWORDS, "void lv_page_scroll_hor(lv_obj_t *page, lv_coord_t dist)"},
    {"scroll_ver", (PyCFunction) pylv_page_scroll_ver, METH_VARARGS | METH_KEYWORDS, "void lv_page_scroll_ver(lv_obj_t *page, lv_coord_t dist)"},
    {"start_edge_flash", (PyCFunction) pylv_page_start_edge_flash, METH_VARARGS | METH_KEYWORDS, "void lv_page_start_edge_flash(lv_obj_t *page)"},
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
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
pylv_list_set_single_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"mode", NULL};
    int mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &mode)) return NULL;

    LVGL_LOCK         
    lv_list_set_single_mode(self->ref, mode);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_list_set_btn_selected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"btn", NULL};
    pylv_Obj * btn;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &pylv_obj_Type, &btn)) return NULL;

    LVGL_LOCK         
    lv_list_set_btn_selected(self->ref, btn->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_list_set_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"anim_time", NULL};
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_time)) return NULL;

    LVGL_LOCK         
    lv_list_set_anim_time(self->ref, anim_time);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_list_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_list_set_style(self->ref, type, style);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_list_get_single_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_list_get_single_mode(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
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
pylv_list_get_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_list_get_anim_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_list_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_list_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
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
    {"clean", (PyCFunction) pylv_list_clean, METH_VARARGS | METH_KEYWORDS, "void lv_list_clean(lv_obj_t *obj)"},
    {"add", (PyCFunction) pylv_list_add, METH_VARARGS | METH_KEYWORDS, ""},
    {"remove", (PyCFunction) pylv_list_remove, METH_VARARGS | METH_KEYWORDS, "bool lv_list_remove(const lv_obj_t *list, uint16_t index)"},
    {"set_single_mode", (PyCFunction) pylv_list_set_single_mode, METH_VARARGS | METH_KEYWORDS, "void lv_list_set_single_mode(lv_obj_t *list, bool mode)"},
    {"set_btn_selected", (PyCFunction) pylv_list_set_btn_selected, METH_VARARGS | METH_KEYWORDS, "void lv_list_set_btn_selected(lv_obj_t *list, lv_obj_t *btn)"},
    {"set_anim_time", (PyCFunction) pylv_list_set_anim_time, METH_VARARGS | METH_KEYWORDS, "void lv_list_set_anim_time(lv_obj_t *list, uint16_t anim_time)"},
    {"set_style", (PyCFunction) pylv_list_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_list_set_style(lv_obj_t *list, lv_list_style_t type, const lv_style_t *style)"},
    {"get_single_mode", (PyCFunction) pylv_list_get_single_mode, METH_VARARGS | METH_KEYWORDS, "bool lv_list_get_single_mode(lv_obj_t *list)"},
    {"get_btn_text", (PyCFunction) pylv_list_get_btn_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_list_get_btn_text(const lv_obj_t *btn)"},
    {"get_btn_label", (PyCFunction) pylv_list_get_btn_label, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_list_get_btn_label(const lv_obj_t *btn)"},
    {"get_btn_img", (PyCFunction) pylv_list_get_btn_img, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_list_get_btn_img(const lv_obj_t *btn)"},
    {"get_prev_btn", (PyCFunction) pylv_list_get_prev_btn, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_list_get_prev_btn(const lv_obj_t *list, lv_obj_t *prev_btn)"},
    {"get_next_btn", (PyCFunction) pylv_list_get_next_btn, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_list_get_next_btn(const lv_obj_t *list, lv_obj_t *prev_btn)"},
    {"get_btn_index", (PyCFunction) pylv_list_get_btn_index, METH_VARARGS | METH_KEYWORDS, "int32_t lv_list_get_btn_index(const lv_obj_t *list, const lv_obj_t *btn)"},
    {"get_size", (PyCFunction) pylv_list_get_size, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_list_get_size(const lv_obj_t *list)"},
    {"get_btn_selected", (PyCFunction) pylv_list_get_btn_selected, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_list_get_btn_selected(const lv_obj_t *list)"},
    {"get_anim_time", (PyCFunction) pylv_list_get_anim_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_list_get_anim_time(const lv_obj_t *list)"},
    {"get_style", (PyCFunction) pylv_list_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_list_get_style(const lv_obj_t *list, lv_list_style_t type)"},
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
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
pylv_chart_clear_serie(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_chart_clear_serie: Parameter type not found >lv_chart_series_t*< ");
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
pylv_chart_set_series_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"opa", NULL};
    unsigned char opa;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &opa)) return NULL;

    LVGL_LOCK         
    lv_chart_set_series_opa(self->ref, opa);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_series_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"width", NULL};
    short int width;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &width)) return NULL;

    LVGL_LOCK         
    lv_chart_set_series_width(self->ref, width);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_series_darking(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"dark_eff", NULL};
    unsigned char dark_eff;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &dark_eff)) return NULL;

    LVGL_LOCK         
    lv_chart_set_series_darking(self->ref, dark_eff);
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
pylv_chart_set_margin(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"margin", NULL};
    unsigned short int margin;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &margin)) return NULL;

    LVGL_LOCK         
    lv_chart_set_margin(self->ref, margin);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_x_ticks(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"list_of_values", "num_tick_marks", "major_tick_len", "minor_tick_len", "options", NULL};
    const char * list_of_values;
    unsigned char num_tick_marks;
    unsigned char major_tick_len;
    unsigned char minor_tick_len;
    unsigned char options;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "sbbbb", kwlist , &list_of_values, &num_tick_marks, &major_tick_len, &minor_tick_len, &options)) return NULL;

    LVGL_LOCK         
    lv_chart_set_x_ticks(self->ref, list_of_values, num_tick_marks, major_tick_len, minor_tick_len, options);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_y_ticks(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"list_of_values", "num_tick_marks", "major_tick_len", "minor_tick_len", "options", NULL};
    const char * list_of_values;
    unsigned char num_tick_marks;
    unsigned char major_tick_len;
    unsigned char minor_tick_len;
    unsigned char options;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "sbbbb", kwlist , &list_of_values, &num_tick_marks, &major_tick_len, &minor_tick_len, &options)) return NULL;

    LVGL_LOCK         
    lv_chart_set_y_ticks(self->ref, list_of_values, num_tick_marks, major_tick_len, minor_tick_len, options);
    LVGL_UNLOCK
    Py_RETURN_NONE;
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
pylv_chart_get_point_cnt(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_chart_get_point_cnt(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_chart_get_series_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_opa_t result = lv_chart_get_series_opa(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_chart_get_series_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_chart_get_series_width(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_chart_get_series_darking(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_opa_t result = lv_chart_get_series_darking(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_chart_get_margin(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_chart_get_margin(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
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


static PyMethodDef pylv_chart_methods[] = {
    {"add_series", (PyCFunction) pylv_chart_add_series, METH_VARARGS | METH_KEYWORDS, "lv_chart_series_t *lv_chart_add_series(lv_obj_t *chart, lv_color_t color)"},
    {"clear_serie", (PyCFunction) pylv_chart_clear_serie, METH_VARARGS | METH_KEYWORDS, "void lv_chart_clear_serie(lv_obj_t *chart, lv_chart_series_t *serie)"},
    {"set_div_line_count", (PyCFunction) pylv_chart_set_div_line_count, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_div_line_count(lv_obj_t *chart, uint8_t hdiv, uint8_t vdiv)"},
    {"set_range", (PyCFunction) pylv_chart_set_range, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_range(lv_obj_t *chart, lv_coord_t ymin, lv_coord_t ymax)"},
    {"set_type", (PyCFunction) pylv_chart_set_type, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_type(lv_obj_t *chart, lv_chart_type_t type)"},
    {"set_point_count", (PyCFunction) pylv_chart_set_point_count, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_point_count(lv_obj_t *chart, uint16_t point_cnt)"},
    {"set_series_opa", (PyCFunction) pylv_chart_set_series_opa, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_series_opa(lv_obj_t *chart, lv_opa_t opa)"},
    {"set_series_width", (PyCFunction) pylv_chart_set_series_width, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_series_width(lv_obj_t *chart, lv_coord_t width)"},
    {"set_series_darking", (PyCFunction) pylv_chart_set_series_darking, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_series_darking(lv_obj_t *chart, lv_opa_t dark_eff)"},
    {"init_points", (PyCFunction) pylv_chart_init_points, METH_VARARGS | METH_KEYWORDS, "void lv_chart_init_points(lv_obj_t *chart, lv_chart_series_t *ser, lv_coord_t y)"},
    {"set_points", (PyCFunction) pylv_chart_set_points, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_points(lv_obj_t *chart, lv_chart_series_t *ser, lv_coord_t *y_array)"},
    {"set_next", (PyCFunction) pylv_chart_set_next, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_next(lv_obj_t *chart, lv_chart_series_t *ser, lv_coord_t y)"},
    {"set_update_mode", (PyCFunction) pylv_chart_set_update_mode, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_update_mode(lv_obj_t *chart, lv_chart_update_mode_t update_mode)"},
    {"set_margin", (PyCFunction) pylv_chart_set_margin, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_margin(lv_obj_t *chart, uint16_t margin)"},
    {"set_x_ticks", (PyCFunction) pylv_chart_set_x_ticks, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_x_ticks(lv_obj_t *chart, const char *list_of_values, uint8_t num_tick_marks, uint8_t major_tick_len, uint8_t minor_tick_len, lv_chart_axis_options_t options)"},
    {"set_y_ticks", (PyCFunction) pylv_chart_set_y_ticks, METH_VARARGS | METH_KEYWORDS, "void lv_chart_set_y_ticks(lv_obj_t *chart, const char *list_of_values, uint8_t num_tick_marks, uint8_t major_tick_len, uint8_t minor_tick_len, lv_chart_axis_options_t options)"},
    {"get_type", (PyCFunction) pylv_chart_get_type, METH_VARARGS | METH_KEYWORDS, "lv_chart_type_t lv_chart_get_type(const lv_obj_t *chart)"},
    {"get_point_cnt", (PyCFunction) pylv_chart_get_point_cnt, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_chart_get_point_cnt(const lv_obj_t *chart)"},
    {"get_series_opa", (PyCFunction) pylv_chart_get_series_opa, METH_VARARGS | METH_KEYWORDS, "lv_opa_t lv_chart_get_series_opa(const lv_obj_t *chart)"},
    {"get_series_width", (PyCFunction) pylv_chart_get_series_width, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_chart_get_series_width(const lv_obj_t *chart)"},
    {"get_series_darking", (PyCFunction) pylv_chart_get_series_darking, METH_VARARGS | METH_KEYWORDS, "lv_opa_t lv_chart_get_series_darking(const lv_obj_t *chart)"},
    {"get_margin", (PyCFunction) pylv_chart_get_margin, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_chart_get_margin(lv_obj_t *chart)"},
    {"refresh", (PyCFunction) pylv_chart_refresh, METH_VARARGS | METH_KEYWORDS, "void lv_chart_refresh(lv_obj_t *chart)"},
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
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
pylv_table_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_table_set_style(self->ref, type, style);
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
pylv_table_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_table_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}


static PyMethodDef pylv_table_methods[] = {
    {"set_cell_value", (PyCFunction) pylv_table_set_cell_value, METH_VARARGS | METH_KEYWORDS, "void lv_table_set_cell_value(lv_obj_t *table, uint16_t row, uint16_t col, const char *txt)"},
    {"set_row_cnt", (PyCFunction) pylv_table_set_row_cnt, METH_VARARGS | METH_KEYWORDS, "void lv_table_set_row_cnt(lv_obj_t *table, uint16_t row_cnt)"},
    {"set_col_cnt", (PyCFunction) pylv_table_set_col_cnt, METH_VARARGS | METH_KEYWORDS, "void lv_table_set_col_cnt(lv_obj_t *table, uint16_t col_cnt)"},
    {"set_col_width", (PyCFunction) pylv_table_set_col_width, METH_VARARGS | METH_KEYWORDS, "void lv_table_set_col_width(lv_obj_t *table, uint16_t col_id, lv_coord_t w)"},
    {"set_cell_align", (PyCFunction) pylv_table_set_cell_align, METH_VARARGS | METH_KEYWORDS, "void lv_table_set_cell_align(lv_obj_t *table, uint16_t row, uint16_t col, lv_label_align_t align)"},
    {"set_cell_type", (PyCFunction) pylv_table_set_cell_type, METH_VARARGS | METH_KEYWORDS, "void lv_table_set_cell_type(lv_obj_t *table, uint16_t row, uint16_t col, uint8_t type)"},
    {"set_cell_crop", (PyCFunction) pylv_table_set_cell_crop, METH_VARARGS | METH_KEYWORDS, "void lv_table_set_cell_crop(lv_obj_t *table, uint16_t row, uint16_t col, bool crop)"},
    {"set_cell_merge_right", (PyCFunction) pylv_table_set_cell_merge_right, METH_VARARGS | METH_KEYWORDS, "void lv_table_set_cell_merge_right(lv_obj_t *table, uint16_t row, uint16_t col, bool en)"},
    {"set_style", (PyCFunction) pylv_table_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_table_set_style(lv_obj_t *table, lv_table_style_t type, const lv_style_t *style)"},
    {"get_cell_value", (PyCFunction) pylv_table_get_cell_value, METH_VARARGS | METH_KEYWORDS, "const char *lv_table_get_cell_value(lv_obj_t *table, uint16_t row, uint16_t col)"},
    {"get_row_cnt", (PyCFunction) pylv_table_get_row_cnt, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_table_get_row_cnt(lv_obj_t *table)"},
    {"get_col_cnt", (PyCFunction) pylv_table_get_col_cnt, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_table_get_col_cnt(lv_obj_t *table)"},
    {"get_col_width", (PyCFunction) pylv_table_get_col_width, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_table_get_col_width(lv_obj_t *table, uint16_t col_id)"},
    {"get_cell_align", (PyCFunction) pylv_table_get_cell_align, METH_VARARGS | METH_KEYWORDS, "lv_label_align_t lv_table_get_cell_align(lv_obj_t *table, uint16_t row, uint16_t col)"},
    {"get_cell_type", (PyCFunction) pylv_table_get_cell_type, METH_VARARGS | METH_KEYWORDS, "lv_label_align_t lv_table_get_cell_type(lv_obj_t *table, uint16_t row, uint16_t col)"},
    {"get_cell_crop", (PyCFunction) pylv_table_get_cell_crop, METH_VARARGS | METH_KEYWORDS, "lv_label_align_t lv_table_get_cell_crop(lv_obj_t *table, uint16_t row, uint16_t col)"},
    {"get_cell_merge_right", (PyCFunction) pylv_table_get_cell_merge_right, METH_VARARGS | METH_KEYWORDS, "bool lv_table_get_cell_merge_right(lv_obj_t *table, uint16_t row, uint16_t col)"},
    {"get_style", (PyCFunction) pylv_table_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_table_get_style(const lv_obj_t *table, lv_table_style_t type)"},
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
pylv_cb_dealloc(pylv_Cb *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_cb_init(pylv_Cb *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Cb *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_cb_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_cb_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    *lv_obj_get_user_data_ptr(self->ref) = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_cb_set_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"txt", NULL};
    const char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    LVGL_LOCK         
    lv_cb_set_text(self->ref, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_cb_set_static_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"txt", NULL};
    const char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    LVGL_LOCK         
    lv_cb_set_static_text(self->ref, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_cb_set_checked(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"checked", NULL};
    int checked;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &checked)) return NULL;

    LVGL_LOCK         
    lv_cb_set_checked(self->ref, checked);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_cb_set_inactive(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_cb_set_inactive(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_cb_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_cb_set_style(self->ref, type, style);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_cb_get_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_cb_get_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_cb_is_checked(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_cb_is_checked(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_cb_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_cb_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}


static PyMethodDef pylv_cb_methods[] = {
    {"set_text", (PyCFunction) pylv_cb_set_text, METH_VARARGS | METH_KEYWORDS, "void lv_cb_set_text(lv_obj_t *cb, const char *txt)"},
    {"set_static_text", (PyCFunction) pylv_cb_set_static_text, METH_VARARGS | METH_KEYWORDS, "void lv_cb_set_static_text(lv_obj_t *cb, const char *txt)"},
    {"set_checked", (PyCFunction) pylv_cb_set_checked, METH_VARARGS | METH_KEYWORDS, "inline static void lv_cb_set_checked(lv_obj_t *cb, bool checked)"},
    {"set_inactive", (PyCFunction) pylv_cb_set_inactive, METH_VARARGS | METH_KEYWORDS, "inline static void lv_cb_set_inactive(lv_obj_t *cb)"},
    {"set_style", (PyCFunction) pylv_cb_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_cb_set_style(lv_obj_t *cb, lv_cb_style_t type, const lv_style_t *style)"},
    {"get_text", (PyCFunction) pylv_cb_get_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_cb_get_text(const lv_obj_t *cb)"},
    {"is_checked", (PyCFunction) pylv_cb_is_checked, METH_VARARGS | METH_KEYWORDS, "inline static bool lv_cb_is_checked(const lv_obj_t *cb)"},
    {"get_style", (PyCFunction) pylv_cb_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_cb_get_style(const lv_obj_t *cb, lv_cb_style_t type)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_cb_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Cb",
    .tp_doc = "lvgl Cb",
    .tp_basicsize = sizeof(pylv_Cb),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_cb_init,
    .tp_dealloc = (destructor) pylv_cb_dealloc,
    .tp_methods = pylv_cb_methods,
    .tp_weaklistoffset = offsetof(pylv_Cb, weakreflist),
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
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
    int anim;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hp", kwlist , &value, &anim)) return NULL;

    LVGL_LOCK         
    lv_bar_set_value(self->ref, value, anim);
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
pylv_bar_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_bar_set_style(self->ref, type, style);
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

static PyObject*
pylv_bar_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_bar_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}


static PyMethodDef pylv_bar_methods[] = {
    {"set_value", (PyCFunction) pylv_bar_set_value, METH_VARARGS | METH_KEYWORDS, "void lv_bar_set_value(lv_obj_t *bar, int16_t value, bool anim)"},
    {"set_range", (PyCFunction) pylv_bar_set_range, METH_VARARGS | METH_KEYWORDS, "void lv_bar_set_range(lv_obj_t *bar, int16_t min, int16_t max)"},
    {"set_sym", (PyCFunction) pylv_bar_set_sym, METH_VARARGS | METH_KEYWORDS, "void lv_bar_set_sym(lv_obj_t *bar, bool en)"},
    {"set_style", (PyCFunction) pylv_bar_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_bar_set_style(lv_obj_t *bar, lv_bar_style_t type, const lv_style_t *style)"},
    {"get_value", (PyCFunction) pylv_bar_get_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_bar_get_value(const lv_obj_t *bar)"},
    {"get_min_value", (PyCFunction) pylv_bar_get_min_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_bar_get_min_value(const lv_obj_t *bar)"},
    {"get_max_value", (PyCFunction) pylv_bar_get_max_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_bar_get_max_value(const lv_obj_t *bar)"},
    {"get_sym", (PyCFunction) pylv_bar_get_sym, METH_VARARGS | METH_KEYWORDS, "bool lv_bar_get_sym(lv_obj_t *bar)"},
    {"get_style", (PyCFunction) pylv_bar_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_bar_get_style(const lv_obj_t *bar, lv_bar_style_t type)"},
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_slider_set_knob_in(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"in", NULL};
    int in;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &in)) return NULL;

    LVGL_LOCK         
    lv_slider_set_knob_in(self->ref, in);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_slider_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_slider_set_style(self->ref, type, style);
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
pylv_slider_get_knob_in(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_slider_get_knob_in(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_slider_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_slider_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}


static PyMethodDef pylv_slider_methods[] = {
    {"set_knob_in", (PyCFunction) pylv_slider_set_knob_in, METH_VARARGS | METH_KEYWORDS, "void lv_slider_set_knob_in(lv_obj_t *slider, bool in)"},
    {"set_style", (PyCFunction) pylv_slider_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_slider_set_style(lv_obj_t *slider, lv_slider_style_t type, const lv_style_t *style)"},
    {"get_value", (PyCFunction) pylv_slider_get_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_slider_get_value(const lv_obj_t *slider)"},
    {"is_dragged", (PyCFunction) pylv_slider_is_dragged, METH_VARARGS | METH_KEYWORDS, "bool lv_slider_is_dragged(const lv_obj_t *slider)"},
    {"get_knob_in", (PyCFunction) pylv_slider_get_knob_in, METH_VARARGS | METH_KEYWORDS, "bool lv_slider_get_knob_in(const lv_obj_t *slider)"},
    {"get_style", (PyCFunction) pylv_slider_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_slider_get_style(const lv_obj_t *slider, lv_slider_style_t type)"},
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
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
pylv_btnm_dealloc(pylv_Btnm *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_btnm_init(pylv_Btnm *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Btnm *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_btnm_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_btnm_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    *lv_obj_get_user_data_ptr(self->ref) = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_btnm_set_map(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"map", NULL};
    char map;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "c", kwlist , &map)) return NULL;

    LVGL_LOCK         
    lv_btnm_set_map(self->ref, map);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_set_ctrl_map(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"ctrl_map", NULL};
    unsigned short int ctrl_map;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &ctrl_map)) return NULL;

    LVGL_LOCK         
    lv_btnm_set_ctrl_map(self->ref, ctrl_map);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_set_pressed(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"id", NULL};
    unsigned short int id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &id)) return NULL;

    LVGL_LOCK         
    lv_btnm_set_pressed(self->ref, id);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_btnm_set_style(self->ref, type, style);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_set_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_btnm_set_recolor(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_set_btn_ctrl(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"btn_id", "ctrl", "en", NULL};
    unsigned short int btn_id;
    unsigned short int ctrl;
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HHp", kwlist , &btn_id, &ctrl, &en)) return NULL;

    LVGL_LOCK         
    lv_btnm_set_btn_ctrl(self->ref, btn_id, ctrl, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_set_btn_ctrl_all(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"ctrl", "en", NULL};
    unsigned short int ctrl;
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hp", kwlist , &ctrl, &en)) return NULL;

    LVGL_LOCK         
    lv_btnm_set_btn_ctrl_all(self->ref, ctrl, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_set_btn_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"btn_id", "width", NULL};
    unsigned short int btn_id;
    unsigned char width;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hb", kwlist , &btn_id, &width)) return NULL;

    LVGL_LOCK         
    lv_btnm_set_btn_width(self->ref, btn_id, width);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_set_one_toggle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"one_toggle", NULL};
    int one_toggle;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &one_toggle)) return NULL;

    LVGL_LOCK         
    lv_btnm_set_one_toggle(self->ref, one_toggle);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_get_map_array(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_btnm_get_map_array: Return type not found >const char**< ");
    return NULL;
}

static PyObject*
pylv_btnm_get_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_btnm_get_recolor(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_btnm_get_active_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_btnm_get_active_btn(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_btnm_get_active_btn_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_btnm_get_active_btn_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_btnm_get_pressed_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_btnm_get_pressed_btn(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_btnm_get_btn_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"btn_id", NULL};
    unsigned short int btn_id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &btn_id)) return NULL;

    LVGL_LOCK        
    const char* result = lv_btnm_get_btn_text(self->ref, btn_id);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_btnm_get_btn_ctrl(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"btn_id", "ctrl", NULL};
    unsigned short int btn_id;
    unsigned short int ctrl;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HH", kwlist , &btn_id, &ctrl)) return NULL;

    LVGL_LOCK        
    bool result = lv_btnm_get_btn_ctrl(self->ref, btn_id, ctrl);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_btnm_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_btnm_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}

static PyObject*
pylv_btnm_get_one_toggle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_btnm_get_one_toggle(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_btnm_methods[] = {
    {"set_map", (PyCFunction) pylv_btnm_set_map, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_map(const lv_obj_t *btnm, const char *map[])"},
    {"set_ctrl_map", (PyCFunction) pylv_btnm_set_ctrl_map, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_ctrl_map(const lv_obj_t *btnm, const lv_btnm_ctrl_t ctrl_map[])"},
    {"set_pressed", (PyCFunction) pylv_btnm_set_pressed, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_pressed(const lv_obj_t *btnm, uint16_t id)"},
    {"set_style", (PyCFunction) pylv_btnm_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_style(lv_obj_t *btnm, lv_btnm_style_t type, const lv_style_t *style)"},
    {"set_recolor", (PyCFunction) pylv_btnm_set_recolor, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_recolor(const lv_obj_t *btnm, bool en)"},
    {"set_btn_ctrl", (PyCFunction) pylv_btnm_set_btn_ctrl, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_btn_ctrl(const lv_obj_t *btnm, uint16_t btn_id, lv_btnm_ctrl_t ctrl, bool en)"},
    {"set_btn_ctrl_all", (PyCFunction) pylv_btnm_set_btn_ctrl_all, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_btn_ctrl_all(lv_obj_t *btnm, lv_btnm_ctrl_t ctrl, bool en)"},
    {"set_btn_width", (PyCFunction) pylv_btnm_set_btn_width, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_btn_width(const lv_obj_t *btnm, uint16_t btn_id, uint8_t width)"},
    {"set_one_toggle", (PyCFunction) pylv_btnm_set_one_toggle, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_one_toggle(lv_obj_t *btnm, bool one_toggle)"},
    {"get_map_array", (PyCFunction) pylv_btnm_get_map_array, METH_VARARGS | METH_KEYWORDS, "const char **lv_btnm_get_map_array(const lv_obj_t *btnm)"},
    {"get_recolor", (PyCFunction) pylv_btnm_get_recolor, METH_VARARGS | METH_KEYWORDS, "bool lv_btnm_get_recolor(const lv_obj_t *btnm)"},
    {"get_active_btn", (PyCFunction) pylv_btnm_get_active_btn, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_btnm_get_active_btn(const lv_obj_t *btnm)"},
    {"get_active_btn_text", (PyCFunction) pylv_btnm_get_active_btn_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_btnm_get_active_btn_text(const lv_obj_t *btnm)"},
    {"get_pressed_btn", (PyCFunction) pylv_btnm_get_pressed_btn, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_btnm_get_pressed_btn(const lv_obj_t *btnm)"},
    {"get_btn_text", (PyCFunction) pylv_btnm_get_btn_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_btnm_get_btn_text(const lv_obj_t *btnm, uint16_t btn_id)"},
    {"get_btn_ctrl", (PyCFunction) pylv_btnm_get_btn_ctrl, METH_VARARGS | METH_KEYWORDS, "bool lv_btnm_get_btn_ctrl(lv_obj_t *btnm, uint16_t btn_id, lv_btnm_ctrl_t ctrl)"},
    {"get_style", (PyCFunction) pylv_btnm_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_btnm_get_style(const lv_obj_t *btnm, lv_btnm_style_t type)"},
    {"get_one_toggle", (PyCFunction) pylv_btnm_get_one_toggle, METH_VARARGS | METH_KEYWORDS, "bool lv_btnm_get_one_toggle(const lv_obj_t *btnm)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_btnm_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Btnm",
    .tp_doc = "lvgl Btnm",
    .tp_basicsize = sizeof(pylv_Btnm),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_btnm_init,
    .tp_dealloc = (destructor) pylv_btnm_dealloc,
    .tp_methods = pylv_btnm_methods,
    .tp_weaklistoffset = offsetof(pylv_Btnm, weakreflist),
};

    
static void
pylv_kb_dealloc(pylv_Kb *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_kb_init(pylv_Kb *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Kb *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_kb_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_kb_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    *lv_obj_get_user_data_ptr(self->ref) = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_kb_set_ta(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"ta", NULL};
    pylv_Obj * ta;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &pylv_obj_Type, &ta)) return NULL;

    LVGL_LOCK         
    lv_kb_set_ta(self->ref, ta->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_kb_set_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"mode", NULL};
    unsigned char mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &mode)) return NULL;

    LVGL_LOCK         
    lv_kb_set_mode(self->ref, mode);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_kb_set_cursor_manage(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_kb_set_cursor_manage(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_kb_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_kb_set_style(self->ref, type, style);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_kb_get_ta(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_kb_get_ta(self->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}

static PyObject*
pylv_kb_get_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_kb_mode_t result = lv_kb_get_mode(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_kb_get_cursor_manage(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_kb_get_cursor_manage(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_kb_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_kb_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}

static PyObject*
pylv_kb_def_event_cb(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"event", NULL};
    unsigned char event;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &event)) return NULL;

    LVGL_LOCK         
    lv_kb_def_event_cb(self->ref, event);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}


static PyMethodDef pylv_kb_methods[] = {
    {"set_ta", (PyCFunction) pylv_kb_set_ta, METH_VARARGS | METH_KEYWORDS, "void lv_kb_set_ta(lv_obj_t *kb, lv_obj_t *ta)"},
    {"set_mode", (PyCFunction) pylv_kb_set_mode, METH_VARARGS | METH_KEYWORDS, "void lv_kb_set_mode(lv_obj_t *kb, lv_kb_mode_t mode)"},
    {"set_cursor_manage", (PyCFunction) pylv_kb_set_cursor_manage, METH_VARARGS | METH_KEYWORDS, "void lv_kb_set_cursor_manage(lv_obj_t *kb, bool en)"},
    {"set_style", (PyCFunction) pylv_kb_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_kb_set_style(lv_obj_t *kb, lv_kb_style_t type, const lv_style_t *style)"},
    {"get_ta", (PyCFunction) pylv_kb_get_ta, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_kb_get_ta(const lv_obj_t *kb)"},
    {"get_mode", (PyCFunction) pylv_kb_get_mode, METH_VARARGS | METH_KEYWORDS, "lv_kb_mode_t lv_kb_get_mode(const lv_obj_t *kb)"},
    {"get_cursor_manage", (PyCFunction) pylv_kb_get_cursor_manage, METH_VARARGS | METH_KEYWORDS, "bool lv_kb_get_cursor_manage(const lv_obj_t *kb)"},
    {"get_style", (PyCFunction) pylv_kb_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_kb_get_style(const lv_obj_t *kb, lv_kb_style_t type)"},
    {"def_event_cb", (PyCFunction) pylv_kb_def_event_cb, METH_VARARGS | METH_KEYWORDS, "void lv_kb_def_event_cb(lv_obj_t *kb, lv_event_t event)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_kb_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Kb",
    .tp_doc = "lvgl Kb",
    .tp_basicsize = sizeof(pylv_Kb),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_kb_init,
    .tp_dealloc = (destructor) pylv_kb_dealloc,
    .tp_methods = pylv_kb_methods,
    .tp_weaklistoffset = offsetof(pylv_Kb, weakreflist),
};

    
static void
pylv_ddlist_dealloc(pylv_Ddlist *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_ddlist_init(pylv_Ddlist *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Ddlist *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_ddlist_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_ddlist_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    *lv_obj_get_user_data_ptr(self->ref) = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_ddlist_set_options(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"options", NULL};
    const char * options;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &options)) return NULL;

    LVGL_LOCK         
    lv_ddlist_set_options(self->ref, options);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_selected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"sel_opt", NULL};
    unsigned short int sel_opt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &sel_opt)) return NULL;

    LVGL_LOCK         
    lv_ddlist_set_selected(self->ref, sel_opt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_fix_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"h", NULL};
    short int h;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &h)) return NULL;

    LVGL_LOCK         
    lv_ddlist_set_fix_height(self->ref, h);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_hor_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"fit", NULL};
    unsigned char fit;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &fit)) return NULL;

    LVGL_LOCK         
    lv_ddlist_set_hor_fit(self->ref, fit);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_draw_arrow(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_ddlist_set_draw_arrow(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_stay_open(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_ddlist_set_stay_open(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"anim_time", NULL};
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_time)) return NULL;

    LVGL_LOCK         
    lv_ddlist_set_anim_time(self->ref, anim_time);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_ddlist_set_style(self->ref, type, style);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"align", NULL};
    unsigned char align;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &align)) return NULL;

    LVGL_LOCK         
    lv_ddlist_set_align(self->ref, align);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_get_options(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_ddlist_get_options(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_ddlist_get_selected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_ddlist_get_selected(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_ddlist_get_selected_str(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_ddlist_get_selected_str: Parameter type not found >char*< ");
    return NULL;
}

static PyObject*
pylv_ddlist_get_fix_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_ddlist_get_fix_height(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_ddlist_get_draw_arrow(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_ddlist_get_draw_arrow(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_ddlist_get_stay_open(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_ddlist_get_stay_open(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_ddlist_get_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_ddlist_get_anim_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_ddlist_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_ddlist_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}

static PyObject*
pylv_ddlist_get_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_label_align_t result = lv_ddlist_get_align(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_ddlist_open(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"anim_en", NULL};
    int anim_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &anim_en)) return NULL;

    LVGL_LOCK         
    lv_ddlist_open(self->ref, anim_en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_close(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"anim_en", NULL};
    int anim_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &anim_en)) return NULL;

    LVGL_LOCK         
    lv_ddlist_close(self->ref, anim_en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}


static PyMethodDef pylv_ddlist_methods[] = {
    {"set_options", (PyCFunction) pylv_ddlist_set_options, METH_VARARGS | METH_KEYWORDS, "void lv_ddlist_set_options(lv_obj_t *ddlist, const char *options)"},
    {"set_selected", (PyCFunction) pylv_ddlist_set_selected, METH_VARARGS | METH_KEYWORDS, "void lv_ddlist_set_selected(lv_obj_t *ddlist, uint16_t sel_opt)"},
    {"set_fix_height", (PyCFunction) pylv_ddlist_set_fix_height, METH_VARARGS | METH_KEYWORDS, "void lv_ddlist_set_fix_height(lv_obj_t *ddlist, lv_coord_t h)"},
    {"set_hor_fit", (PyCFunction) pylv_ddlist_set_hor_fit, METH_VARARGS | METH_KEYWORDS, "void lv_ddlist_set_hor_fit(lv_obj_t *ddlist, lv_fit_t fit)"},
    {"set_draw_arrow", (PyCFunction) pylv_ddlist_set_draw_arrow, METH_VARARGS | METH_KEYWORDS, "void lv_ddlist_set_draw_arrow(lv_obj_t *ddlist, bool en)"},
    {"set_stay_open", (PyCFunction) pylv_ddlist_set_stay_open, METH_VARARGS | METH_KEYWORDS, "void lv_ddlist_set_stay_open(lv_obj_t *ddlist, bool en)"},
    {"set_anim_time", (PyCFunction) pylv_ddlist_set_anim_time, METH_VARARGS | METH_KEYWORDS, "void lv_ddlist_set_anim_time(lv_obj_t *ddlist, uint16_t anim_time)"},
    {"set_style", (PyCFunction) pylv_ddlist_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_ddlist_set_style(lv_obj_t *ddlist, lv_ddlist_style_t type, const lv_style_t *style)"},
    {"set_align", (PyCFunction) pylv_ddlist_set_align, METH_VARARGS | METH_KEYWORDS, "void lv_ddlist_set_align(lv_obj_t *ddlist, lv_label_align_t align)"},
    {"get_options", (PyCFunction) pylv_ddlist_get_options, METH_VARARGS | METH_KEYWORDS, "const char *lv_ddlist_get_options(const lv_obj_t *ddlist)"},
    {"get_selected", (PyCFunction) pylv_ddlist_get_selected, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_ddlist_get_selected(const lv_obj_t *ddlist)"},
    {"get_selected_str", (PyCFunction) pylv_ddlist_get_selected_str, METH_VARARGS | METH_KEYWORDS, "void lv_ddlist_get_selected_str(const lv_obj_t *ddlist, char *buf, uint16_t buf_size)"},
    {"get_fix_height", (PyCFunction) pylv_ddlist_get_fix_height, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_ddlist_get_fix_height(const lv_obj_t *ddlist)"},
    {"get_draw_arrow", (PyCFunction) pylv_ddlist_get_draw_arrow, METH_VARARGS | METH_KEYWORDS, "bool lv_ddlist_get_draw_arrow(lv_obj_t *ddlist)"},
    {"get_stay_open", (PyCFunction) pylv_ddlist_get_stay_open, METH_VARARGS | METH_KEYWORDS, "bool lv_ddlist_get_stay_open(lv_obj_t *ddlist)"},
    {"get_anim_time", (PyCFunction) pylv_ddlist_get_anim_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_ddlist_get_anim_time(const lv_obj_t *ddlist)"},
    {"get_style", (PyCFunction) pylv_ddlist_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_ddlist_get_style(const lv_obj_t *ddlist, lv_ddlist_style_t type)"},
    {"get_align", (PyCFunction) pylv_ddlist_get_align, METH_VARARGS | METH_KEYWORDS, "lv_label_align_t lv_ddlist_get_align(const lv_obj_t *ddlist)"},
    {"open", (PyCFunction) pylv_ddlist_open, METH_VARARGS | METH_KEYWORDS, "void lv_ddlist_open(lv_obj_t *ddlist, bool anim_en)"},
    {"close", (PyCFunction) pylv_ddlist_close, METH_VARARGS | METH_KEYWORDS, "void lv_ddlist_close(lv_obj_t *ddlist, bool anim_en)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_ddlist_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Ddlist",
    .tp_doc = "lvgl Ddlist",
    .tp_basicsize = sizeof(pylv_Ddlist),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_ddlist_init,
    .tp_dealloc = (destructor) pylv_ddlist_dealloc,
    .tp_methods = pylv_ddlist_methods,
    .tp_weaklistoffset = offsetof(pylv_Ddlist, weakreflist),
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_roller_set_options(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"options", "inf", NULL};
    const char * options;
    int inf;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "sp", kwlist , &options, &inf)) return NULL;

    LVGL_LOCK         
    lv_roller_set_options(self->ref, options, inf);
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
    static char *kwlist[] = {"sel_opt", "anim_en", NULL};
    unsigned short int sel_opt;
    int anim_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hp", kwlist , &sel_opt, &anim_en)) return NULL;

    LVGL_LOCK         
    lv_roller_set_selected(self->ref, sel_opt, anim_en);
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
pylv_roller_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_roller_set_style(self->ref, type, style);
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
pylv_roller_get_hor_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_roller_get_hor_fit(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_roller_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_roller_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}


static PyMethodDef pylv_roller_methods[] = {
    {"set_options", (PyCFunction) pylv_roller_set_options, METH_VARARGS | METH_KEYWORDS, "void lv_roller_set_options(lv_obj_t *roller, const char *options, bool inf)"},
    {"set_align", (PyCFunction) pylv_roller_set_align, METH_VARARGS | METH_KEYWORDS, "void lv_roller_set_align(lv_obj_t *roller, lv_label_align_t align)"},
    {"set_selected", (PyCFunction) pylv_roller_set_selected, METH_VARARGS | METH_KEYWORDS, "void lv_roller_set_selected(lv_obj_t *roller, uint16_t sel_opt, bool anim_en)"},
    {"set_visible_row_count", (PyCFunction) pylv_roller_set_visible_row_count, METH_VARARGS | METH_KEYWORDS, "void lv_roller_set_visible_row_count(lv_obj_t *roller, uint8_t row_cnt)"},
    {"set_style", (PyCFunction) pylv_roller_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_roller_set_style(lv_obj_t *roller, lv_roller_style_t type, const lv_style_t *style)"},
    {"get_selected", (PyCFunction) pylv_roller_get_selected, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_roller_get_selected(const lv_obj_t *roller)"},
    {"get_align", (PyCFunction) pylv_roller_get_align, METH_VARARGS | METH_KEYWORDS, "lv_label_align_t lv_roller_get_align(const lv_obj_t *roller)"},
    {"get_hor_fit", (PyCFunction) pylv_roller_get_hor_fit, METH_VARARGS | METH_KEYWORDS, "bool lv_roller_get_hor_fit(const lv_obj_t *roller)"},
    {"get_style", (PyCFunction) pylv_roller_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_roller_get_style(const lv_obj_t *roller, lv_roller_style_t type)"},
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
pylv_ta_dealloc(pylv_Ta *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_ta_init(pylv_Ta *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Ta *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_ta_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_ta_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    *lv_obj_get_user_data_ptr(self->ref) = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_ta_add_char(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"c", NULL};
    unsigned int c;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "I", kwlist , &c)) return NULL;

    LVGL_LOCK         
    lv_ta_add_char(self->ref, c);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_add_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"txt", NULL};
    const char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    LVGL_LOCK         
    lv_ta_add_text(self->ref, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_del_char(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_ta_del_char(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_del_char_forward(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_ta_del_char_forward(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"txt", NULL};
    const char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    LVGL_LOCK         
    lv_ta_set_text(self->ref, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_placeholder_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"txt", NULL};
    const char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    LVGL_LOCK         
    lv_ta_set_placeholder_text(self->ref, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_cursor_pos(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"pos", NULL};
    short int pos;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &pos)) return NULL;

    LVGL_LOCK         
    lv_ta_set_cursor_pos(self->ref, pos);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_cursor_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"cur_type", NULL};
    unsigned char cur_type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &cur_type)) return NULL;

    LVGL_LOCK         
    lv_ta_set_cursor_type(self->ref, cur_type);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_pwd_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_ta_set_pwd_mode(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_one_line(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_ta_set_one_line(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_text_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"align", NULL};
    unsigned char align;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &align)) return NULL;

    LVGL_LOCK         
    lv_ta_set_text_align(self->ref, align);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_accepted_chars(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"list", NULL};
    const char * list;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &list)) return NULL;

    LVGL_LOCK         
    lv_ta_set_accepted_chars(self->ref, list);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_max_length(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"num", NULL};
    unsigned short int num;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &num)) return NULL;

    LVGL_LOCK         
    lv_ta_set_max_length(self->ref, num);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_insert_replace(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"txt", NULL};
    const char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    LVGL_LOCK         
    lv_ta_set_insert_replace(self->ref, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_ta_set_style(self->ref, type, style);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_text_sel(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_ta_set_text_sel(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_get_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_ta_get_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_ta_get_placeholder_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_ta_get_placeholder_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_ta_get_label(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_ta_get_label(self->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}

static PyObject*
pylv_ta_get_cursor_pos(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_ta_get_cursor_pos(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_ta_get_cursor_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_cursor_type_t result = lv_ta_get_cursor_type(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_ta_get_pwd_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_ta_get_pwd_mode(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_ta_get_one_line(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_ta_get_one_line(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_ta_get_accepted_chars(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_ta_get_accepted_chars(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_ta_get_max_length(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_ta_get_max_length(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_ta_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_ta_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}

static PyObject*
pylv_ta_text_is_selected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_ta_text_is_selected(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_ta_get_text_sel_en(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_ta_get_text_sel_en(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_ta_clear_selection(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_ta_clear_selection(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_cursor_right(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_ta_cursor_right(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_cursor_left(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_ta_cursor_left(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_cursor_down(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_ta_cursor_down(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_cursor_up(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_ta_cursor_up(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}


static PyMethodDef pylv_ta_methods[] = {
    {"add_char", (PyCFunction) pylv_ta_add_char, METH_VARARGS | METH_KEYWORDS, "void lv_ta_add_char(lv_obj_t *ta, uint32_t c)"},
    {"add_text", (PyCFunction) pylv_ta_add_text, METH_VARARGS | METH_KEYWORDS, "void lv_ta_add_text(lv_obj_t *ta, const char *txt)"},
    {"del_char", (PyCFunction) pylv_ta_del_char, METH_VARARGS | METH_KEYWORDS, "void lv_ta_del_char(lv_obj_t *ta)"},
    {"del_char_forward", (PyCFunction) pylv_ta_del_char_forward, METH_VARARGS | METH_KEYWORDS, "void lv_ta_del_char_forward(lv_obj_t *ta)"},
    {"set_text", (PyCFunction) pylv_ta_set_text, METH_VARARGS | METH_KEYWORDS, "void lv_ta_set_text(lv_obj_t *ta, const char *txt)"},
    {"set_placeholder_text", (PyCFunction) pylv_ta_set_placeholder_text, METH_VARARGS | METH_KEYWORDS, "void lv_ta_set_placeholder_text(lv_obj_t *ta, const char *txt)"},
    {"set_cursor_pos", (PyCFunction) pylv_ta_set_cursor_pos, METH_VARARGS | METH_KEYWORDS, "void lv_ta_set_cursor_pos(lv_obj_t *ta, int16_t pos)"},
    {"set_cursor_type", (PyCFunction) pylv_ta_set_cursor_type, METH_VARARGS | METH_KEYWORDS, "void lv_ta_set_cursor_type(lv_obj_t *ta, lv_cursor_type_t cur_type)"},
    {"set_pwd_mode", (PyCFunction) pylv_ta_set_pwd_mode, METH_VARARGS | METH_KEYWORDS, "void lv_ta_set_pwd_mode(lv_obj_t *ta, bool en)"},
    {"set_one_line", (PyCFunction) pylv_ta_set_one_line, METH_VARARGS | METH_KEYWORDS, "void lv_ta_set_one_line(lv_obj_t *ta, bool en)"},
    {"set_text_align", (PyCFunction) pylv_ta_set_text_align, METH_VARARGS | METH_KEYWORDS, "void lv_ta_set_text_align(lv_obj_t *ta, lv_label_align_t align)"},
    {"set_accepted_chars", (PyCFunction) pylv_ta_set_accepted_chars, METH_VARARGS | METH_KEYWORDS, "void lv_ta_set_accepted_chars(lv_obj_t *ta, const char *list)"},
    {"set_max_length", (PyCFunction) pylv_ta_set_max_length, METH_VARARGS | METH_KEYWORDS, "void lv_ta_set_max_length(lv_obj_t *ta, uint16_t num)"},
    {"set_insert_replace", (PyCFunction) pylv_ta_set_insert_replace, METH_VARARGS | METH_KEYWORDS, "void lv_ta_set_insert_replace(lv_obj_t *ta, const char *txt)"},
    {"set_style", (PyCFunction) pylv_ta_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_ta_set_style(lv_obj_t *ta, lv_ta_style_t type, const lv_style_t *style)"},
    {"set_text_sel", (PyCFunction) pylv_ta_set_text_sel, METH_VARARGS | METH_KEYWORDS, "void lv_ta_set_text_sel(lv_obj_t *ta, bool en)"},
    {"get_text", (PyCFunction) pylv_ta_get_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_ta_get_text(const lv_obj_t *ta)"},
    {"get_placeholder_text", (PyCFunction) pylv_ta_get_placeholder_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_ta_get_placeholder_text(lv_obj_t *ta)"},
    {"get_label", (PyCFunction) pylv_ta_get_label, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_ta_get_label(const lv_obj_t *ta)"},
    {"get_cursor_pos", (PyCFunction) pylv_ta_get_cursor_pos, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_ta_get_cursor_pos(const lv_obj_t *ta)"},
    {"get_cursor_type", (PyCFunction) pylv_ta_get_cursor_type, METH_VARARGS | METH_KEYWORDS, "lv_cursor_type_t lv_ta_get_cursor_type(const lv_obj_t *ta)"},
    {"get_pwd_mode", (PyCFunction) pylv_ta_get_pwd_mode, METH_VARARGS | METH_KEYWORDS, "bool lv_ta_get_pwd_mode(const lv_obj_t *ta)"},
    {"get_one_line", (PyCFunction) pylv_ta_get_one_line, METH_VARARGS | METH_KEYWORDS, "bool lv_ta_get_one_line(const lv_obj_t *ta)"},
    {"get_accepted_chars", (PyCFunction) pylv_ta_get_accepted_chars, METH_VARARGS | METH_KEYWORDS, "const char *lv_ta_get_accepted_chars(lv_obj_t *ta)"},
    {"get_max_length", (PyCFunction) pylv_ta_get_max_length, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_ta_get_max_length(lv_obj_t *ta)"},
    {"get_style", (PyCFunction) pylv_ta_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_ta_get_style(const lv_obj_t *ta, lv_ta_style_t type)"},
    {"text_is_selected", (PyCFunction) pylv_ta_text_is_selected, METH_VARARGS | METH_KEYWORDS, "bool lv_ta_text_is_selected(const lv_obj_t *ta)"},
    {"get_text_sel_en", (PyCFunction) pylv_ta_get_text_sel_en, METH_VARARGS | METH_KEYWORDS, "bool lv_ta_get_text_sel_en(lv_obj_t *ta)"},
    {"clear_selection", (PyCFunction) pylv_ta_clear_selection, METH_VARARGS | METH_KEYWORDS, "void lv_ta_clear_selection(lv_obj_t *ta)"},
    {"cursor_right", (PyCFunction) pylv_ta_cursor_right, METH_VARARGS | METH_KEYWORDS, "void lv_ta_cursor_right(lv_obj_t *ta)"},
    {"cursor_left", (PyCFunction) pylv_ta_cursor_left, METH_VARARGS | METH_KEYWORDS, "void lv_ta_cursor_left(lv_obj_t *ta)"},
    {"cursor_down", (PyCFunction) pylv_ta_cursor_down, METH_VARARGS | METH_KEYWORDS, "void lv_ta_cursor_down(lv_obj_t *ta)"},
    {"cursor_up", (PyCFunction) pylv_ta_cursor_up, METH_VARARGS | METH_KEYWORDS, "void lv_ta_cursor_up(lv_obj_t *ta)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_ta_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Ta",
    .tp_doc = "lvgl Ta",
    .tp_basicsize = sizeof(pylv_Ta),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_ta_init,
    .tp_dealloc = (destructor) pylv_ta_dealloc,
    .tp_methods = pylv_ta_methods,
    .tp_weaklistoffset = offsetof(pylv_Ta, weakreflist),
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
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
pylv_canvas_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_canvas_set_style(self->ref, type, style);
    LVGL_UNLOCK
    Py_RETURN_NONE;
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
pylv_canvas_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_canvas_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}

static PyObject*
pylv_canvas_copy_buf(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_copy_buf: Parameter type not found >const void*< ");
    return NULL;
}

static PyObject*
pylv_canvas_mult_buf(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_mult_buf: Parameter type not found >void*< ");
    return NULL;
}

static PyObject*
pylv_canvas_rotate(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_rotate: Parameter type not found >lv_img_dsc_t*< ");
    return NULL;
}

static PyObject*
pylv_canvas_draw_circle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_draw_circle: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_canvas_draw_line(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_draw_line: Parameter type not found >lv_point_t< ");
    return NULL;
}

static PyObject*
pylv_canvas_draw_triangle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_draw_triangle: Parameter type not found >lv_point_t*< ");
    return NULL;
}

static PyObject*
pylv_canvas_draw_rect(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_draw_rect: Parameter type not found >lv_point_t*< ");
    return NULL;
}

static PyObject*
pylv_canvas_draw_polygon(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_draw_polygon: Parameter type not found >lv_point_t*< ");
    return NULL;
}

static PyObject*
pylv_canvas_fill_polygon(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_fill_polygon: Parameter type not found >lv_point_t*< ");
    return NULL;
}

static PyObject*
pylv_canvas_boundary_fill4(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_boundary_fill4: Parameter type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_canvas_flood_fill(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_flood_fill: Parameter type not found >lv_color_t< ");
    return NULL;
}


static PyMethodDef pylv_canvas_methods[] = {
    {"set_buffer", (PyCFunction) pylv_canvas_set_buffer, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_set_buffer(lv_obj_t *canvas, void *buf, lv_coord_t w, lv_coord_t h, lv_img_cf_t cf)"},
    {"set_px", (PyCFunction) pylv_canvas_set_px, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_set_px(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_color_t c)"},
    {"set_style", (PyCFunction) pylv_canvas_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_set_style(lv_obj_t *canvas, lv_canvas_style_t type, const lv_style_t *style)"},
    {"get_px", (PyCFunction) pylv_canvas_get_px, METH_VARARGS | METH_KEYWORDS, "lv_color_t lv_canvas_get_px(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y)"},
    {"get_img", (PyCFunction) pylv_canvas_get_img, METH_VARARGS | METH_KEYWORDS, "lv_img_dsc_t *lv_canvas_get_img(lv_obj_t *canvas)"},
    {"get_style", (PyCFunction) pylv_canvas_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_canvas_get_style(const lv_obj_t *canvas, lv_canvas_style_t type)"},
    {"copy_buf", (PyCFunction) pylv_canvas_copy_buf, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_copy_buf(lv_obj_t *canvas, const void *to_copy, lv_coord_t w, lv_coord_t h, lv_coord_t x, lv_coord_t y)"},
    {"mult_buf", (PyCFunction) pylv_canvas_mult_buf, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_mult_buf(lv_obj_t *canvas, void *to_copy, lv_coord_t w, lv_coord_t h, lv_coord_t x, lv_coord_t y)"},
    {"rotate", (PyCFunction) pylv_canvas_rotate, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_rotate(lv_obj_t *canvas, lv_img_dsc_t *img, int16_t angle, lv_coord_t offset_x, lv_coord_t offset_y, int32_t pivot_x, int32_t pivot_y)"},
    {"draw_circle", (PyCFunction) pylv_canvas_draw_circle, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_draw_circle(lv_obj_t *canvas, lv_coord_t x0, lv_coord_t y0, lv_coord_t radius, lv_color_t color)"},
    {"draw_line", (PyCFunction) pylv_canvas_draw_line, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_draw_line(lv_obj_t *canvas, lv_point_t point1, lv_point_t point2, lv_color_t color)"},
    {"draw_triangle", (PyCFunction) pylv_canvas_draw_triangle, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_draw_triangle(lv_obj_t *canvas, lv_point_t *points, lv_color_t color)"},
    {"draw_rect", (PyCFunction) pylv_canvas_draw_rect, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_draw_rect(lv_obj_t *canvas, lv_point_t *points, lv_color_t color)"},
    {"draw_polygon", (PyCFunction) pylv_canvas_draw_polygon, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_draw_polygon(lv_obj_t *canvas, lv_point_t *points, size_t size, lv_color_t color)"},
    {"fill_polygon", (PyCFunction) pylv_canvas_fill_polygon, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_fill_polygon(lv_obj_t *canvas, lv_point_t *points, size_t size, lv_color_t boundary_color, lv_color_t fill_color)"},
    {"boundary_fill4", (PyCFunction) pylv_canvas_boundary_fill4, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_boundary_fill4(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_color_t boundary_color, lv_color_t fill_color)"},
    {"flood_fill", (PyCFunction) pylv_canvas_flood_fill, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_flood_fill(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, lv_color_t fill_color, lv_color_t bg_color)"},
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
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
pylv_win_add_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_win_add_btn: Parameter type not found >const void*< ");
    return NULL;
}

static PyObject*
pylv_win_close_event(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"event", NULL};
    unsigned char event;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &event)) return NULL;

    LVGL_LOCK         
    lv_win_close_event(self->ref, event);
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
pylv_win_set_btn_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"size", NULL};
    short int size;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &size)) return NULL;

    LVGL_LOCK         
    lv_win_set_btn_size(self->ref, size);
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
pylv_win_set_sb_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"sb_mode", NULL};
    unsigned char sb_mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &sb_mode)) return NULL;

    LVGL_LOCK         
    lv_win_set_sb_mode(self->ref, sb_mode);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_win_set_style(self->ref, type, style);
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
pylv_win_get_btn_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_coord_t result = lv_win_get_btn_size(self->ref);
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
    lv_sb_mode_t result = lv_win_get_sb_mode(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
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
pylv_win_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_win_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}

static PyObject*
pylv_win_focus(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"obj", "anim_time", NULL};
    pylv_Obj * obj;
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!H", kwlist , &pylv_obj_Type, &obj, &anim_time)) return NULL;

    LVGL_LOCK         
    lv_win_focus(self->ref, obj->ref, anim_time);
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


static PyMethodDef pylv_win_methods[] = {
    {"clean", (PyCFunction) pylv_win_clean, METH_VARARGS | METH_KEYWORDS, "void lv_win_clean(lv_obj_t *obj)"},
    {"add_btn", (PyCFunction) pylv_win_add_btn, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_win_add_btn(lv_obj_t *win, const void *img_src, lv_event_cb_t event_cb)"},
    {"close_event", (PyCFunction) pylv_win_close_event, METH_VARARGS | METH_KEYWORDS, "void lv_win_close_event(lv_obj_t *btn, lv_event_t event)"},
    {"set_title", (PyCFunction) pylv_win_set_title, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_title(lv_obj_t *win, const char *title)"},
    {"set_btn_size", (PyCFunction) pylv_win_set_btn_size, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_btn_size(lv_obj_t *win, lv_coord_t size)"},
    {"set_layout", (PyCFunction) pylv_win_set_layout, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_layout(lv_obj_t *win, lv_layout_t layout)"},
    {"set_sb_mode", (PyCFunction) pylv_win_set_sb_mode, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_sb_mode(lv_obj_t *win, lv_sb_mode_t sb_mode)"},
    {"set_style", (PyCFunction) pylv_win_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_style(lv_obj_t *win, lv_win_style_t type, const lv_style_t *style)"},
    {"set_drag", (PyCFunction) pylv_win_set_drag, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_drag(lv_obj_t *win, bool en)"},
    {"get_title", (PyCFunction) pylv_win_get_title, METH_VARARGS | METH_KEYWORDS, "const char *lv_win_get_title(const lv_obj_t *win)"},
    {"get_content", (PyCFunction) pylv_win_get_content, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_win_get_content(const lv_obj_t *win)"},
    {"get_btn_size", (PyCFunction) pylv_win_get_btn_size, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_win_get_btn_size(const lv_obj_t *win)"},
    {"get_from_btn", (PyCFunction) pylv_win_get_from_btn, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_win_get_from_btn(const lv_obj_t *ctrl_btn)"},
    {"get_layout", (PyCFunction) pylv_win_get_layout, METH_VARARGS | METH_KEYWORDS, "lv_layout_t lv_win_get_layout(lv_obj_t *win)"},
    {"get_sb_mode", (PyCFunction) pylv_win_get_sb_mode, METH_VARARGS | METH_KEYWORDS, "lv_sb_mode_t lv_win_get_sb_mode(lv_obj_t *win)"},
    {"get_width", (PyCFunction) pylv_win_get_width, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_win_get_width(lv_obj_t *win)"},
    {"get_style", (PyCFunction) pylv_win_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_win_get_style(const lv_obj_t *win, lv_win_style_t type)"},
    {"focus", (PyCFunction) pylv_win_focus, METH_VARARGS | METH_KEYWORDS, "void lv_win_focus(lv_obj_t *win, lv_obj_t *obj, uint16_t anim_time)"},
    {"scroll_hor", (PyCFunction) pylv_win_scroll_hor, METH_VARARGS | METH_KEYWORDS, "inline static void lv_win_scroll_hor(lv_obj_t *win, lv_coord_t dist)"},
    {"scroll_ver", (PyCFunction) pylv_win_scroll_ver, METH_VARARGS | METH_KEYWORDS, "inline static void lv_win_scroll_ver(lv_obj_t *win, lv_coord_t dist)"},
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_tabview_clean(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_tabview_clean(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
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
pylv_tabview_set_tab_act(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"id", "anim_en", NULL};
    unsigned short int id;
    int anim_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hp", kwlist , &id, &anim_en)) return NULL;

    LVGL_LOCK         
    lv_tabview_set_tab_act(self->ref, id, anim_en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_tabview_set_sliding(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_tabview_set_sliding(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
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
pylv_tabview_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_tabview_set_style(self->ref, type, style);
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
pylv_tabview_set_btns_hidden(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_tabview_set_btns_hidden(self->ref, en);
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
pylv_tabview_get_sliding(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_tabview_get_sliding(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
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
pylv_tabview_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_tabview_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
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

static PyObject*
pylv_tabview_get_btns_hidden(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_tabview_get_btns_hidden(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_tabview_methods[] = {
    {"clean", (PyCFunction) pylv_tabview_clean, METH_VARARGS | METH_KEYWORDS, "void lv_tabview_clean(lv_obj_t *obj)"},
    {"add_tab", (PyCFunction) pylv_tabview_add_tab, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_tabview_add_tab(lv_obj_t *tabview, const char *name)"},
    {"set_tab_act", (PyCFunction) pylv_tabview_set_tab_act, METH_VARARGS | METH_KEYWORDS, "void lv_tabview_set_tab_act(lv_obj_t *tabview, uint16_t id, bool anim_en)"},
    {"set_sliding", (PyCFunction) pylv_tabview_set_sliding, METH_VARARGS | METH_KEYWORDS, "void lv_tabview_set_sliding(lv_obj_t *tabview, bool en)"},
    {"set_anim_time", (PyCFunction) pylv_tabview_set_anim_time, METH_VARARGS | METH_KEYWORDS, "void lv_tabview_set_anim_time(lv_obj_t *tabview, uint16_t anim_time)"},
    {"set_style", (PyCFunction) pylv_tabview_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_tabview_set_style(lv_obj_t *tabview, lv_tabview_style_t type, const lv_style_t *style)"},
    {"set_btns_pos", (PyCFunction) pylv_tabview_set_btns_pos, METH_VARARGS | METH_KEYWORDS, "void lv_tabview_set_btns_pos(lv_obj_t *tabview, lv_tabview_btns_pos_t btns_pos)"},
    {"set_btns_hidden", (PyCFunction) pylv_tabview_set_btns_hidden, METH_VARARGS | METH_KEYWORDS, "void lv_tabview_set_btns_hidden(lv_obj_t *tabview, bool en)"},
    {"get_tab_act", (PyCFunction) pylv_tabview_get_tab_act, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_tabview_get_tab_act(const lv_obj_t *tabview)"},
    {"get_tab_count", (PyCFunction) pylv_tabview_get_tab_count, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_tabview_get_tab_count(const lv_obj_t *tabview)"},
    {"get_tab", (PyCFunction) pylv_tabview_get_tab, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_tabview_get_tab(const lv_obj_t *tabview, uint16_t id)"},
    {"get_sliding", (PyCFunction) pylv_tabview_get_sliding, METH_VARARGS | METH_KEYWORDS, "bool lv_tabview_get_sliding(const lv_obj_t *tabview)"},
    {"get_anim_time", (PyCFunction) pylv_tabview_get_anim_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_tabview_get_anim_time(const lv_obj_t *tabview)"},
    {"get_style", (PyCFunction) pylv_tabview_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_tabview_get_style(const lv_obj_t *tabview, lv_tabview_style_t type)"},
    {"get_btns_pos", (PyCFunction) pylv_tabview_get_btns_pos, METH_VARARGS | METH_KEYWORDS, "lv_tabview_btns_pos_t lv_tabview_get_btns_pos(const lv_obj_t *tabview)"},
    {"get_btns_hidden", (PyCFunction) pylv_tabview_get_btns_hidden, METH_VARARGS | METH_KEYWORDS, "bool lv_tabview_get_btns_hidden(const lv_obj_t *tabview)"},
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_tileview_set_valid_positions: Parameter type not found >const lv_point_t*< ");
    return NULL;
}

static PyObject*
pylv_tileview_set_tile_act(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"x", "y", "anim_en", NULL};
    short int x;
    short int y;
    int anim_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hhp", kwlist , &x, &y, &anim_en)) return NULL;

    LVGL_LOCK         
    lv_tileview_set_tile_act(self->ref, x, y, anim_en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_tileview_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_tileview_set_style(self->ref, type, style);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_tileview_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_tileview_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}


static PyMethodDef pylv_tileview_methods[] = {
    {"add_element", (PyCFunction) pylv_tileview_add_element, METH_VARARGS | METH_KEYWORDS, "void lv_tileview_add_element(lv_obj_t *tileview, lv_obj_t *element)"},
    {"set_valid_positions", (PyCFunction) pylv_tileview_set_valid_positions, METH_VARARGS | METH_KEYWORDS, "void lv_tileview_set_valid_positions(lv_obj_t *tileview, const lv_point_t *valid_pos)"},
    {"set_tile_act", (PyCFunction) pylv_tileview_set_tile_act, METH_VARARGS | METH_KEYWORDS, "void lv_tileview_set_tile_act(lv_obj_t *tileview, lv_coord_t x, lv_coord_t y, bool anim_en)"},
    {"set_style", (PyCFunction) pylv_tileview_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_tileview_set_style(lv_obj_t *tileview, lv_tileview_style_t type, const lv_style_t *style)"},
    {"get_style", (PyCFunction) pylv_tileview_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_tileview_get_style(const lv_obj_t *tileview, lv_tileview_style_t type)"},
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
pylv_mbox_dealloc(pylv_Mbox *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_mbox_init(pylv_Mbox *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Mbox *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_mbox_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_mbox_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    *lv_obj_get_user_data_ptr(self->ref) = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_mbox_add_btns(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_mbox_add_btns: Parameter type not found >const char**< ");
    return NULL;
}

static PyObject*
pylv_mbox_set_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"txt", NULL};
    const char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    LVGL_LOCK         
    lv_mbox_set_text(self->ref, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_mbox_set_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"anim_time", NULL};
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_time)) return NULL;

    LVGL_LOCK         
    lv_mbox_set_anim_time(self->ref, anim_time);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_mbox_start_auto_close(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"delay", NULL};
    unsigned short int delay;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &delay)) return NULL;

    LVGL_LOCK         
    lv_mbox_start_auto_close(self->ref, delay);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_mbox_stop_auto_close(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_mbox_stop_auto_close(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_mbox_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_mbox_set_style(self->ref, type, style);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_mbox_set_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_mbox_set_recolor(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_mbox_get_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_mbox_get_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_mbox_get_active_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_mbox_get_active_btn(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_mbox_get_active_btn_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char* result = lv_mbox_get_active_btn_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_mbox_get_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_mbox_get_anim_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_mbox_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_mbox_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}

static PyObject*
pylv_mbox_get_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_mbox_get_recolor(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_mbox_get_btnm(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_mbox_get_btnm(self->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}


static PyMethodDef pylv_mbox_methods[] = {
    {"add_btns", (PyCFunction) pylv_mbox_add_btns, METH_VARARGS | METH_KEYWORDS, "void lv_mbox_add_btns(lv_obj_t *mbox, const char **btn_mapaction)"},
    {"set_text", (PyCFunction) pylv_mbox_set_text, METH_VARARGS | METH_KEYWORDS, "void lv_mbox_set_text(lv_obj_t *mbox, const char *txt)"},
    {"set_anim_time", (PyCFunction) pylv_mbox_set_anim_time, METH_VARARGS | METH_KEYWORDS, "void lv_mbox_set_anim_time(lv_obj_t *mbox, uint16_t anim_time)"},
    {"start_auto_close", (PyCFunction) pylv_mbox_start_auto_close, METH_VARARGS | METH_KEYWORDS, "void lv_mbox_start_auto_close(lv_obj_t *mbox, uint16_t delay)"},
    {"stop_auto_close", (PyCFunction) pylv_mbox_stop_auto_close, METH_VARARGS | METH_KEYWORDS, "void lv_mbox_stop_auto_close(lv_obj_t *mbox)"},
    {"set_style", (PyCFunction) pylv_mbox_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_mbox_set_style(lv_obj_t *mbox, lv_mbox_style_t type, const lv_style_t *style)"},
    {"set_recolor", (PyCFunction) pylv_mbox_set_recolor, METH_VARARGS | METH_KEYWORDS, "void lv_mbox_set_recolor(lv_obj_t *mbox, bool en)"},
    {"get_text", (PyCFunction) pylv_mbox_get_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_mbox_get_text(const lv_obj_t *mbox)"},
    {"get_active_btn", (PyCFunction) pylv_mbox_get_active_btn, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_mbox_get_active_btn(lv_obj_t *mbox)"},
    {"get_active_btn_text", (PyCFunction) pylv_mbox_get_active_btn_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_mbox_get_active_btn_text(lv_obj_t *mbox)"},
    {"get_anim_time", (PyCFunction) pylv_mbox_get_anim_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_mbox_get_anim_time(const lv_obj_t *mbox)"},
    {"get_style", (PyCFunction) pylv_mbox_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_mbox_get_style(const lv_obj_t *mbox, lv_mbox_style_t type)"},
    {"get_recolor", (PyCFunction) pylv_mbox_get_recolor, METH_VARARGS | METH_KEYWORDS, "bool lv_mbox_get_recolor(const lv_obj_t *mbox)"},
    {"get_btnm", (PyCFunction) pylv_mbox_get_btnm, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_mbox_get_btnm(lv_obj_t *mbox)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_mbox_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Mbox",
    .tp_doc = "lvgl Mbox",
    .tp_basicsize = sizeof(pylv_Mbox),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_mbox_init,
    .tp_dealloc = (destructor) pylv_mbox_dealloc,
    .tp_methods = pylv_mbox_methods,
    .tp_weaklistoffset = offsetof(pylv_Mbox, weakreflist),
};

    
static void
pylv_lmeter_dealloc(pylv_Lmeter *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_lmeter_init(pylv_Lmeter *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Lmeter *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_lmeter_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_lmeter_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    *lv_obj_get_user_data_ptr(self->ref) = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_lmeter_set_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"value", NULL};
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &value)) return NULL;

    LVGL_LOCK         
    lv_lmeter_set_value(self->ref, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_lmeter_set_range(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"min", "max", NULL};
    short int min;
    short int max;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &min, &max)) return NULL;

    LVGL_LOCK         
    lv_lmeter_set_range(self->ref, min, max);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_lmeter_set_scale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"angle", "line_cnt", NULL};
    unsigned short int angle;
    unsigned char line_cnt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hb", kwlist , &angle, &line_cnt)) return NULL;

    LVGL_LOCK         
    lv_lmeter_set_scale(self->ref, angle, line_cnt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_lmeter_get_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int16_t result = lv_lmeter_get_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_lmeter_get_min_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int16_t result = lv_lmeter_get_min_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_lmeter_get_max_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int16_t result = lv_lmeter_get_max_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_lmeter_get_line_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint8_t result = lv_lmeter_get_line_count(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_lmeter_get_scale_angle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_lmeter_get_scale_angle(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}


static PyMethodDef pylv_lmeter_methods[] = {
    {"set_value", (PyCFunction) pylv_lmeter_set_value, METH_VARARGS | METH_KEYWORDS, "void lv_lmeter_set_value(lv_obj_t *lmeter, int16_t value)"},
    {"set_range", (PyCFunction) pylv_lmeter_set_range, METH_VARARGS | METH_KEYWORDS, "void lv_lmeter_set_range(lv_obj_t *lmeter, int16_t min, int16_t max)"},
    {"set_scale", (PyCFunction) pylv_lmeter_set_scale, METH_VARARGS | METH_KEYWORDS, "void lv_lmeter_set_scale(lv_obj_t *lmeter, uint16_t angle, uint8_t line_cnt)"},
    {"get_value", (PyCFunction) pylv_lmeter_get_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_lmeter_get_value(const lv_obj_t *lmeter)"},
    {"get_min_value", (PyCFunction) pylv_lmeter_get_min_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_lmeter_get_min_value(const lv_obj_t *lmeter)"},
    {"get_max_value", (PyCFunction) pylv_lmeter_get_max_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_lmeter_get_max_value(const lv_obj_t *lmeter)"},
    {"get_line_count", (PyCFunction) pylv_lmeter_get_line_count, METH_VARARGS | METH_KEYWORDS, "uint8_t lv_lmeter_get_line_count(const lv_obj_t *lmeter)"},
    {"get_scale_angle", (PyCFunction) pylv_lmeter_get_scale_angle, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_lmeter_get_scale_angle(const lv_obj_t *lmeter)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_lmeter_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Lmeter",
    .tp_doc = "lvgl Lmeter",
    .tp_basicsize = sizeof(pylv_Lmeter),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_lmeter_init,
    .tp_dealloc = (destructor) pylv_lmeter_dealloc,
    .tp_methods = pylv_lmeter_methods,
    .tp_weaklistoffset = offsetof(pylv_Lmeter, weakreflist),
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
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
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bh", kwlist , &needle_id, &value)) return NULL;

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
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &value)) return NULL;

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
pylv_gauge_get_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"needle", NULL};
    unsigned char needle;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &needle)) return NULL;

    LVGL_LOCK        
    int16_t result = lv_gauge_get_value(self->ref, needle);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
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
    int16_t result = lv_gauge_get_critical_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
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


static PyMethodDef pylv_gauge_methods[] = {
    {"set_needle_count", (PyCFunction) pylv_gauge_set_needle_count, METH_VARARGS | METH_KEYWORDS, "void lv_gauge_set_needle_count(lv_obj_t *gauge, uint8_t needle_cnt, const lv_color_t colors[])"},
    {"set_value", (PyCFunction) pylv_gauge_set_value, METH_VARARGS | METH_KEYWORDS, "void lv_gauge_set_value(lv_obj_t *gauge, uint8_t needle_id, int16_t value)"},
    {"set_critical_value", (PyCFunction) pylv_gauge_set_critical_value, METH_VARARGS | METH_KEYWORDS, "inline static void lv_gauge_set_critical_value(lv_obj_t *gauge, int16_t value)"},
    {"set_scale", (PyCFunction) pylv_gauge_set_scale, METH_VARARGS | METH_KEYWORDS, "void lv_gauge_set_scale(lv_obj_t *gauge, uint16_t angle, uint8_t line_cnt, uint8_t label_cnt)"},
    {"get_value", (PyCFunction) pylv_gauge_get_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_gauge_get_value(const lv_obj_t *gauge, uint8_t needle)"},
    {"get_needle_count", (PyCFunction) pylv_gauge_get_needle_count, METH_VARARGS | METH_KEYWORDS, "uint8_t lv_gauge_get_needle_count(const lv_obj_t *gauge)"},
    {"get_critical_value", (PyCFunction) pylv_gauge_get_critical_value, METH_VARARGS | METH_KEYWORDS, "inline static int16_t lv_gauge_get_critical_value(const lv_obj_t *gauge)"},
    {"get_label_count", (PyCFunction) pylv_gauge_get_label_count, METH_VARARGS | METH_KEYWORDS, "uint8_t lv_gauge_get_label_count(const lv_obj_t *gauge)"},
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
pylv_sw_dealloc(pylv_Sw *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_sw_init(pylv_Sw *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Sw *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_sw_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_sw_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    *lv_obj_get_user_data_ptr(self->ref) = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_sw_on(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"anim", NULL};
    int anim;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &anim)) return NULL;

    LVGL_LOCK         
    lv_sw_on(self->ref, anim);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_sw_off(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"anim", NULL};
    int anim;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &anim)) return NULL;

    LVGL_LOCK         
    lv_sw_off(self->ref, anim);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_sw_toggle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"anim", NULL};
    int anim;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &anim)) return NULL;

    LVGL_LOCK        
    bool result = lv_sw_toggle(self->ref, anim);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_sw_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_sw_set_style(self->ref, type, style);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_sw_set_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"anim_time", NULL};
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_time)) return NULL;

    LVGL_LOCK         
    lv_sw_set_anim_time(self->ref, anim_time);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_sw_get_state(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    bool result = lv_sw_get_state(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_sw_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_sw_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}

static PyObject*
pylv_sw_get_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_sw_get_anim_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}


static PyMethodDef pylv_sw_methods[] = {
    {"on", (PyCFunction) pylv_sw_on, METH_VARARGS | METH_KEYWORDS, "void lv_sw_on(lv_obj_t *sw, bool anim)"},
    {"off", (PyCFunction) pylv_sw_off, METH_VARARGS | METH_KEYWORDS, "void lv_sw_off(lv_obj_t *sw, bool anim)"},
    {"toggle", (PyCFunction) pylv_sw_toggle, METH_VARARGS | METH_KEYWORDS, "bool lv_sw_toggle(lv_obj_t *sw, bool anim)"},
    {"set_style", (PyCFunction) pylv_sw_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_sw_set_style(lv_obj_t *sw, lv_sw_style_t type, const lv_style_t *style)"},
    {"set_anim_time", (PyCFunction) pylv_sw_set_anim_time, METH_VARARGS | METH_KEYWORDS, "void lv_sw_set_anim_time(lv_obj_t *sw, uint16_t anim_time)"},
    {"get_state", (PyCFunction) pylv_sw_get_state, METH_VARARGS | METH_KEYWORDS, "inline static bool lv_sw_get_state(const lv_obj_t *sw)"},
    {"get_style", (PyCFunction) pylv_sw_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_sw_get_style(const lv_obj_t *sw, lv_sw_style_t type)"},
    {"get_anim_time", (PyCFunction) pylv_sw_get_anim_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_sw_get_anim_time(const lv_obj_t *sw)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_sw_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Sw",
    .tp_doc = "lvgl Sw",
    .tp_basicsize = sizeof(pylv_Sw),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_sw_init,
    .tp_dealloc = (destructor) pylv_sw_dealloc,
    .tp_methods = pylv_sw_methods,
    .tp_weaklistoffset = offsetof(pylv_Sw, weakreflist),
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
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
pylv_arc_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_arc_set_style(self->ref, type, style);
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
pylv_arc_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_arc_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}


static PyMethodDef pylv_arc_methods[] = {
    {"set_angles", (PyCFunction) pylv_arc_set_angles, METH_VARARGS | METH_KEYWORDS, "void lv_arc_set_angles(lv_obj_t *arc, uint16_t start, uint16_t end)"},
    {"set_style", (PyCFunction) pylv_arc_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_arc_set_style(lv_obj_t *arc, lv_arc_style_t type, const lv_style_t *style)"},
    {"get_angle_start", (PyCFunction) pylv_arc_get_angle_start, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_arc_get_angle_start(lv_obj_t *arc)"},
    {"get_angle_end", (PyCFunction) pylv_arc_get_angle_end, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_arc_get_angle_end(lv_obj_t *arc)"},
    {"get_style", (PyCFunction) pylv_arc_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_arc_get_style(const lv_obj_t *arc, lv_arc_style_t type)"},
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
pylv_preload_dealloc(pylv_Preload *self) 
{

    // the accompanying lv_obj holds a reference to the Python object, so
    // dealloc can only take place if the lv_obj has already been deleted using
    // Obj.del_() or .clean() on ints parents. 
    
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);

    Py_TYPE(self)->tp_free((PyObject *) self);

}

static int
pylv_preload_init(pylv_Preload *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Obj *parent=NULL;
    pylv_Preload *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_preload_Type, &copy)) {
        return -1;
    }   
    
    LVGL_LOCK
    self->ref = lv_preload_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    *lv_obj_get_user_data_ptr(self->ref) = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_preload_set_arc_length(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"deg", NULL};
    short int deg;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &deg)) return NULL;

    LVGL_LOCK         
    lv_preload_set_arc_length(self->ref, deg);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_preload_set_spin_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"time", NULL};
    unsigned short int time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &time)) return NULL;

    LVGL_LOCK         
    lv_preload_set_spin_time(self->ref, time);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_preload_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_preload_set_style(self->ref, type, style);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_preload_set_anim_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK         
    lv_preload_set_anim_type(self->ref, type);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_preload_set_anim_dir(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"dir", NULL};
    unsigned char dir;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &dir)) return NULL;

    LVGL_LOCK         
    lv_preload_set_anim_dir(self->ref, dir);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_preload_get_arc_length(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_anim_value_t result = lv_preload_get_arc_length(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_preload_get_spin_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    uint16_t result = lv_preload_get_spin_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_preload_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_preload_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}

static PyObject*
pylv_preload_get_anim_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_preload_type_t result = lv_preload_get_anim_type(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_preload_get_anim_dir(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    lv_preload_dir_t result = lv_preload_get_anim_dir(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}


static PyMethodDef pylv_preload_methods[] = {
    {"set_arc_length", (PyCFunction) pylv_preload_set_arc_length, METH_VARARGS | METH_KEYWORDS, "void lv_preload_set_arc_length(lv_obj_t *preload, lv_anim_value_t deg)"},
    {"set_spin_time", (PyCFunction) pylv_preload_set_spin_time, METH_VARARGS | METH_KEYWORDS, "void lv_preload_set_spin_time(lv_obj_t *preload, uint16_t time)"},
    {"set_style", (PyCFunction) pylv_preload_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_preload_set_style(lv_obj_t *preload, lv_preload_style_t type, const lv_style_t *style)"},
    {"set_anim_type", (PyCFunction) pylv_preload_set_anim_type, METH_VARARGS | METH_KEYWORDS, "void lv_preload_set_anim_type(lv_obj_t *preload, lv_preload_type_t type)"},
    {"set_anim_dir", (PyCFunction) pylv_preload_set_anim_dir, METH_VARARGS | METH_KEYWORDS, "void lv_preload_set_anim_dir(lv_obj_t *preload, lv_preload_dir_t dir)"},
    {"get_arc_length", (PyCFunction) pylv_preload_get_arc_length, METH_VARARGS | METH_KEYWORDS, "lv_anim_value_t lv_preload_get_arc_length(const lv_obj_t *preload)"},
    {"get_spin_time", (PyCFunction) pylv_preload_get_spin_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_preload_get_spin_time(const lv_obj_t *preload)"},
    {"get_style", (PyCFunction) pylv_preload_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_preload_get_style(const lv_obj_t *preload, lv_preload_style_t type)"},
    {"get_anim_type", (PyCFunction) pylv_preload_get_anim_type, METH_VARARGS | METH_KEYWORDS, "lv_preload_type_t lv_preload_get_anim_type(lv_obj_t *preload)"},
    {"get_anim_dir", (PyCFunction) pylv_preload_get_anim_dir, METH_VARARGS | METH_KEYWORDS, "lv_preload_dir_t lv_preload_get_anim_dir(lv_obj_t *preload)"},
    {NULL}  /* Sentinel */
};

static PyTypeObject pylv_preload_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Preload",
    .tp_doc = "lvgl Preload",
    .tp_basicsize = sizeof(pylv_Preload),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_preload_init,
    .tp_dealloc = (destructor) pylv_preload_dealloc,
    .tp_methods = pylv_preload_methods,
    .tp_weaklistoffset = offsetof(pylv_Preload, weakreflist),
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_calendar_set_highlighted_dates: Parameter type not found >lv_calendar_date_t*< ");
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
pylv_calendar_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", "style", NULL};
    unsigned char type;
    lv_style_t * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bO&", kwlist , &type, pylv_style_t_arg_converter, &style)) return NULL;

    LVGL_LOCK         
    lv_calendar_set_style(self->ref, type, style);
    LVGL_UNLOCK
    Py_RETURN_NONE;
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

static PyObject*
pylv_calendar_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK        
    const lv_style_t* result = lv_calendar_get_style(self->ref, type);
    LVGL_UNLOCK
    return pystruct_from_lv(result);            
}


static PyMethodDef pylv_calendar_methods[] = {
    {"set_today_date", (PyCFunction) pylv_calendar_set_today_date, METH_VARARGS | METH_KEYWORDS, "void lv_calendar_set_today_date(lv_obj_t *calendar, lv_calendar_date_t *today)"},
    {"set_showed_date", (PyCFunction) pylv_calendar_set_showed_date, METH_VARARGS | METH_KEYWORDS, "void lv_calendar_set_showed_date(lv_obj_t *calendar, lv_calendar_date_t *showed)"},
    {"set_highlighted_dates", (PyCFunction) pylv_calendar_set_highlighted_dates, METH_VARARGS | METH_KEYWORDS, "void lv_calendar_set_highlighted_dates(lv_obj_t *calendar, lv_calendar_date_t *highlighted, uint16_t date_num)"},
    {"set_day_names", (PyCFunction) pylv_calendar_set_day_names, METH_VARARGS | METH_KEYWORDS, "void lv_calendar_set_day_names(lv_obj_t *calendar, const char **day_names)"},
    {"set_month_names", (PyCFunction) pylv_calendar_set_month_names, METH_VARARGS | METH_KEYWORDS, "void lv_calendar_set_month_names(lv_obj_t *calendar, const char **day_names)"},
    {"set_style", (PyCFunction) pylv_calendar_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_calendar_set_style(lv_obj_t *calendar, lv_calendar_style_t type, const lv_style_t *style)"},
    {"get_today_date", (PyCFunction) pylv_calendar_get_today_date, METH_VARARGS | METH_KEYWORDS, "lv_calendar_date_t *lv_calendar_get_today_date(const lv_obj_t *calendar)"},
    {"get_showed_date", (PyCFunction) pylv_calendar_get_showed_date, METH_VARARGS | METH_KEYWORDS, "lv_calendar_date_t *lv_calendar_get_showed_date(const lv_obj_t *calendar)"},
    {"get_pressed_date", (PyCFunction) pylv_calendar_get_pressed_date, METH_VARARGS | METH_KEYWORDS, "lv_calendar_date_t *lv_calendar_get_pressed_date(const lv_obj_t *calendar)"},
    {"get_highlighted_dates", (PyCFunction) pylv_calendar_get_highlighted_dates, METH_VARARGS | METH_KEYWORDS, "lv_calendar_date_t *lv_calendar_get_highlighted_dates(const lv_obj_t *calendar)"},
    {"get_highlighted_dates_num", (PyCFunction) pylv_calendar_get_highlighted_dates_num, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_calendar_get_highlighted_dates_num(const lv_obj_t *calendar)"},
    {"get_day_names", (PyCFunction) pylv_calendar_get_day_names, METH_VARARGS | METH_KEYWORDS, "const char **lv_calendar_get_day_names(const lv_obj_t *calendar)"},
    {"get_month_names", (PyCFunction) pylv_calendar_get_month_names, METH_VARARGS | METH_KEYWORDS, "const char **lv_calendar_get_month_names(const lv_obj_t *calendar)"},
    {"get_style", (PyCFunction) pylv_calendar_get_style, METH_VARARGS | METH_KEYWORDS, "const lv_style_t *lv_calendar_get_style(const lv_obj_t *calendar, lv_calendar_style_t type)"},
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
    *lv_obj_get_user_data_ptr(self->ref) = self;
    Py_INCREF(self); // since reference is stored in lv_obj user data
    install_signal_cb(self);
    LVGL_UNLOCK

    return 0;
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
pylv_spinbox_step_previous(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    if (check_alive(self)) return NULL;
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_spinbox_step_previous(self->ref);
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
    {"set_value", (PyCFunction) pylv_spinbox_set_value, METH_VARARGS | METH_KEYWORDS, "void lv_spinbox_set_value(lv_obj_t *spinbox, int32_t i)"},
    {"set_digit_format", (PyCFunction) pylv_spinbox_set_digit_format, METH_VARARGS | METH_KEYWORDS, "void lv_spinbox_set_digit_format(lv_obj_t *spinbox, uint8_t digit_count, uint8_t separator_position)"},
    {"set_step", (PyCFunction) pylv_spinbox_set_step, METH_VARARGS | METH_KEYWORDS, "void lv_spinbox_set_step(lv_obj_t *spinbox, uint32_t step)"},
    {"set_range", (PyCFunction) pylv_spinbox_set_range, METH_VARARGS | METH_KEYWORDS, "void lv_spinbox_set_range(lv_obj_t *spinbox, int32_t range_min, int32_t range_max)"},
    {"set_padding_left", (PyCFunction) pylv_spinbox_set_padding_left, METH_VARARGS | METH_KEYWORDS, "void lv_spinbox_set_padding_left(lv_obj_t *spinbox, uint8_t padding)"},
    {"get_value", (PyCFunction) pylv_spinbox_get_value, METH_VARARGS | METH_KEYWORDS, "int32_t lv_spinbox_get_value(lv_obj_t *spinbox)"},
    {"step_next", (PyCFunction) pylv_spinbox_step_next, METH_VARARGS | METH_KEYWORDS, "void lv_spinbox_step_next(lv_obj_t *spinbox)"},
    {"step_previous", (PyCFunction) pylv_spinbox_step_previous, METH_VARARGS | METH_KEYWORDS, "void lv_spinbox_step_previous(lv_obj_t *spinbox)"},
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
    PyModule_AddObject(module, "ptr1", PtrObject_fromptr((void*) 4592));
    PyModule_AddObject(module, "ptr2", PtrObject_fromptr((void*) 4592));
    

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

    pylv_cb_Type.tp_base = &pylv_btn_Type;
    if (PyType_Ready(&pylv_cb_Type) < 0) return NULL;

    pylv_bar_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_bar_Type) < 0) return NULL;

    pylv_slider_Type.tp_base = &pylv_bar_Type;
    if (PyType_Ready(&pylv_slider_Type) < 0) return NULL;

    pylv_led_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_led_Type) < 0) return NULL;

    pylv_btnm_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_btnm_Type) < 0) return NULL;

    pylv_kb_Type.tp_base = &pylv_btnm_Type;
    if (PyType_Ready(&pylv_kb_Type) < 0) return NULL;

    pylv_ddlist_Type.tp_base = &pylv_page_Type;
    if (PyType_Ready(&pylv_ddlist_Type) < 0) return NULL;

    pylv_roller_Type.tp_base = &pylv_ddlist_Type;
    if (PyType_Ready(&pylv_roller_Type) < 0) return NULL;

    pylv_ta_Type.tp_base = &pylv_page_Type;
    if (PyType_Ready(&pylv_ta_Type) < 0) return NULL;

    pylv_canvas_Type.tp_base = &pylv_img_Type;
    if (PyType_Ready(&pylv_canvas_Type) < 0) return NULL;

    pylv_win_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_win_Type) < 0) return NULL;

    pylv_tabview_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_tabview_Type) < 0) return NULL;

    pylv_tileview_Type.tp_base = &pylv_page_Type;
    if (PyType_Ready(&pylv_tileview_Type) < 0) return NULL;

    pylv_mbox_Type.tp_base = &pylv_cont_Type;
    if (PyType_Ready(&pylv_mbox_Type) < 0) return NULL;

    pylv_lmeter_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_lmeter_Type) < 0) return NULL;

    pylv_gauge_Type.tp_base = &pylv_lmeter_Type;
    if (PyType_Ready(&pylv_gauge_Type) < 0) return NULL;

    pylv_sw_Type.tp_base = &pylv_slider_Type;
    if (PyType_Ready(&pylv_sw_Type) < 0) return NULL;

    pylv_arc_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_arc_Type) < 0) return NULL;

    pylv_preload_Type.tp_base = &pylv_arc_Type;
    if (PyType_Ready(&pylv_preload_Type) < 0) return NULL;

    pylv_calendar_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_calendar_Type) < 0) return NULL;

    pylv_spinbox_Type.tp_base = &pylv_ta_Type;
    if (PyType_Ready(&pylv_spinbox_Type) < 0) return NULL;


    if (PyType_Ready(&Blob_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_mem_monitor_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_ll_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_task_t_Type) < 0) return NULL;

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

    if (PyType_Ready(&pylv_font_unicode_map_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_font_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_anim_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_style_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_style_anim_dsc_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_reailgn_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_obj_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_obj_type_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_group_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_cont_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_btn_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_fs_file_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_fs_dir_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_fs_drv_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_img_header_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_img_dsc_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_img_decoder_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_img_decoder_dsc_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_imgbtn_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_label_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_img_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_line_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_page_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_list_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_chart_series_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_chart_axis_cfg_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_chart_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_table_cell_format_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_table_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_cb_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_bar_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_slider_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_led_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_btnm_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_kb_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_ddlist_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_roller_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_ta_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_canvas_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_win_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_tabview_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_tileview_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_mbox_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_lmeter_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_gauge_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_sw_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_arc_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_preload_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_calendar_date_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_calendar_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_spinbox_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_color8_t_ch_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_color16_t_ch_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_color32_t_ch_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_indev_proc_t_types_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_indev_proc_t_types_pointer_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_indev_proc_t_types_keypad_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_style_t_body_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_style_t_body_border_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_style_t_body_shadow_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_style_t_body_padding_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_style_t_text_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_style_t_image_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_style_t_line_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_btn_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_imgbtn_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_label_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_img_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_line_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_bar_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_slider_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_sw_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_calendar_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_cb_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_cb_box_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_btnm_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_btnm_btn_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_kb_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_kb_btn_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_mbox_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_mbox_btn_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_page_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_ta_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_spinbox_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_list_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_list_btn_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_ddlist_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_roller_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_tabview_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_tabview_btn_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_tileview_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_table_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_win_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_win_content_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_style_win_btn_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_group_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_label_ext_t_dot_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_page_ext_t_sb_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_page_ext_t_edge_flash_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_chart_ext_t_series_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_table_cell_format_t_s_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_ta_ext_t_cursor_Type) < 0) return NULL;


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

    Py_INCREF(&pylv_cb_Type);
    PyModule_AddObject(module, "Cb", (PyObject *) &pylv_cb_Type); 

    Py_INCREF(&pylv_bar_Type);
    PyModule_AddObject(module, "Bar", (PyObject *) &pylv_bar_Type); 

    Py_INCREF(&pylv_slider_Type);
    PyModule_AddObject(module, "Slider", (PyObject *) &pylv_slider_Type); 

    Py_INCREF(&pylv_led_Type);
    PyModule_AddObject(module, "Led", (PyObject *) &pylv_led_Type); 

    Py_INCREF(&pylv_btnm_Type);
    PyModule_AddObject(module, "Btnm", (PyObject *) &pylv_btnm_Type); 

    Py_INCREF(&pylv_kb_Type);
    PyModule_AddObject(module, "Kb", (PyObject *) &pylv_kb_Type); 

    Py_INCREF(&pylv_ddlist_Type);
    PyModule_AddObject(module, "Ddlist", (PyObject *) &pylv_ddlist_Type); 

    Py_INCREF(&pylv_roller_Type);
    PyModule_AddObject(module, "Roller", (PyObject *) &pylv_roller_Type); 

    Py_INCREF(&pylv_ta_Type);
    PyModule_AddObject(module, "Ta", (PyObject *) &pylv_ta_Type); 

    Py_INCREF(&pylv_canvas_Type);
    PyModule_AddObject(module, "Canvas", (PyObject *) &pylv_canvas_Type); 

    Py_INCREF(&pylv_win_Type);
    PyModule_AddObject(module, "Win", (PyObject *) &pylv_win_Type); 

    Py_INCREF(&pylv_tabview_Type);
    PyModule_AddObject(module, "Tabview", (PyObject *) &pylv_tabview_Type); 

    Py_INCREF(&pylv_tileview_Type);
    PyModule_AddObject(module, "Tileview", (PyObject *) &pylv_tileview_Type); 

    Py_INCREF(&pylv_mbox_Type);
    PyModule_AddObject(module, "Mbox", (PyObject *) &pylv_mbox_Type); 

    Py_INCREF(&pylv_lmeter_Type);
    PyModule_AddObject(module, "Lmeter", (PyObject *) &pylv_lmeter_Type); 

    Py_INCREF(&pylv_gauge_Type);
    PyModule_AddObject(module, "Gauge", (PyObject *) &pylv_gauge_Type); 

    Py_INCREF(&pylv_sw_Type);
    PyModule_AddObject(module, "Sw", (PyObject *) &pylv_sw_Type); 

    Py_INCREF(&pylv_arc_Type);
    PyModule_AddObject(module, "Arc", (PyObject *) &pylv_arc_Type); 

    Py_INCREF(&pylv_preload_Type);
    PyModule_AddObject(module, "Preload", (PyObject *) &pylv_preload_Type); 

    Py_INCREF(&pylv_calendar_Type);
    PyModule_AddObject(module, "Calendar", (PyObject *) &pylv_calendar_Type); 

    Py_INCREF(&pylv_spinbox_Type);
    PyModule_AddObject(module, "Spinbox", (PyObject *) &pylv_spinbox_Type); 



    Py_INCREF(&pylv_mem_monitor_t_Type);
    PyModule_AddObject(module, "mem_monitor_t", (PyObject *) &pylv_mem_monitor_t_Type); 

    Py_INCREF(&pylv_ll_t_Type);
    PyModule_AddObject(module, "ll_t", (PyObject *) &pylv_ll_t_Type); 

    Py_INCREF(&pylv_task_t_Type);
    PyModule_AddObject(module, "task_t", (PyObject *) &pylv_task_t_Type); 

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

    Py_INCREF(&pylv_font_unicode_map_t_Type);
    PyModule_AddObject(module, "font_unicode_map_t", (PyObject *) &pylv_font_unicode_map_t_Type); 

    Py_INCREF(&pylv_font_t_Type);
    PyModule_AddObject(module, "font_t", (PyObject *) &pylv_font_t_Type); 

    Py_INCREF(&pylv_anim_t_Type);
    PyModule_AddObject(module, "anim_t", (PyObject *) &pylv_anim_t_Type); 

    Py_INCREF(&pylv_style_t_Type);
    PyModule_AddObject(module, "style_t", (PyObject *) &pylv_style_t_Type); 

    Py_INCREF(&pylv_style_anim_dsc_t_Type);
    PyModule_AddObject(module, "style_anim_dsc_t", (PyObject *) &pylv_style_anim_dsc_t_Type); 

    Py_INCREF(&pylv_reailgn_t_Type);
    PyModule_AddObject(module, "reailgn_t", (PyObject *) &pylv_reailgn_t_Type); 

    Py_INCREF(&pylv_obj_t_Type);
    PyModule_AddObject(module, "obj_t", (PyObject *) &pylv_obj_t_Type); 

    Py_INCREF(&pylv_obj_type_t_Type);
    PyModule_AddObject(module, "obj_type_t", (PyObject *) &pylv_obj_type_t_Type); 

    Py_INCREF(&pylv_group_t_Type);
    PyModule_AddObject(module, "group_t", (PyObject *) &pylv_group_t_Type); 

    Py_INCREF(&pylv_theme_t_Type);
    PyModule_AddObject(module, "theme_t", (PyObject *) &pylv_theme_t_Type); 

    Py_INCREF(&pylv_cont_ext_t_Type);
    PyModule_AddObject(module, "cont_ext_t", (PyObject *) &pylv_cont_ext_t_Type); 

    Py_INCREF(&pylv_btn_ext_t_Type);
    PyModule_AddObject(module, "btn_ext_t", (PyObject *) &pylv_btn_ext_t_Type); 

    Py_INCREF(&pylv_fs_file_t_Type);
    PyModule_AddObject(module, "fs_file_t", (PyObject *) &pylv_fs_file_t_Type); 

    Py_INCREF(&pylv_fs_dir_t_Type);
    PyModule_AddObject(module, "fs_dir_t", (PyObject *) &pylv_fs_dir_t_Type); 

    Py_INCREF(&pylv_fs_drv_t_Type);
    PyModule_AddObject(module, "fs_drv_t", (PyObject *) &pylv_fs_drv_t_Type); 

    Py_INCREF(&pylv_img_header_t_Type);
    PyModule_AddObject(module, "img_header_t", (PyObject *) &pylv_img_header_t_Type); 

    Py_INCREF(&pylv_img_dsc_t_Type);
    PyModule_AddObject(module, "img_dsc_t", (PyObject *) &pylv_img_dsc_t_Type); 

    Py_INCREF(&pylv_img_decoder_t_Type);
    PyModule_AddObject(module, "img_decoder_t", (PyObject *) &pylv_img_decoder_t_Type); 

    Py_INCREF(&pylv_img_decoder_dsc_t_Type);
    PyModule_AddObject(module, "img_decoder_dsc_t", (PyObject *) &pylv_img_decoder_dsc_t_Type); 

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

    Py_INCREF(&pylv_chart_axis_cfg_t_Type);
    PyModule_AddObject(module, "chart_axis_cfg_t", (PyObject *) &pylv_chart_axis_cfg_t_Type); 

    Py_INCREF(&pylv_chart_ext_t_Type);
    PyModule_AddObject(module, "chart_ext_t", (PyObject *) &pylv_chart_ext_t_Type); 

    Py_INCREF(&pylv_table_cell_format_t_Type);
    PyModule_AddObject(module, "table_cell_format_t", (PyObject *) &pylv_table_cell_format_t_Type); 

    Py_INCREF(&pylv_table_ext_t_Type);
    PyModule_AddObject(module, "table_ext_t", (PyObject *) &pylv_table_ext_t_Type); 

    Py_INCREF(&pylv_cb_ext_t_Type);
    PyModule_AddObject(module, "cb_ext_t", (PyObject *) &pylv_cb_ext_t_Type); 

    Py_INCREF(&pylv_bar_ext_t_Type);
    PyModule_AddObject(module, "bar_ext_t", (PyObject *) &pylv_bar_ext_t_Type); 

    Py_INCREF(&pylv_slider_ext_t_Type);
    PyModule_AddObject(module, "slider_ext_t", (PyObject *) &pylv_slider_ext_t_Type); 

    Py_INCREF(&pylv_led_ext_t_Type);
    PyModule_AddObject(module, "led_ext_t", (PyObject *) &pylv_led_ext_t_Type); 

    Py_INCREF(&pylv_btnm_ext_t_Type);
    PyModule_AddObject(module, "btnm_ext_t", (PyObject *) &pylv_btnm_ext_t_Type); 

    Py_INCREF(&pylv_kb_ext_t_Type);
    PyModule_AddObject(module, "kb_ext_t", (PyObject *) &pylv_kb_ext_t_Type); 

    Py_INCREF(&pylv_ddlist_ext_t_Type);
    PyModule_AddObject(module, "ddlist_ext_t", (PyObject *) &pylv_ddlist_ext_t_Type); 

    Py_INCREF(&pylv_roller_ext_t_Type);
    PyModule_AddObject(module, "roller_ext_t", (PyObject *) &pylv_roller_ext_t_Type); 

    Py_INCREF(&pylv_ta_ext_t_Type);
    PyModule_AddObject(module, "ta_ext_t", (PyObject *) &pylv_ta_ext_t_Type); 

    Py_INCREF(&pylv_canvas_ext_t_Type);
    PyModule_AddObject(module, "canvas_ext_t", (PyObject *) &pylv_canvas_ext_t_Type); 

    Py_INCREF(&pylv_win_ext_t_Type);
    PyModule_AddObject(module, "win_ext_t", (PyObject *) &pylv_win_ext_t_Type); 

    Py_INCREF(&pylv_tabview_ext_t_Type);
    PyModule_AddObject(module, "tabview_ext_t", (PyObject *) &pylv_tabview_ext_t_Type); 

    Py_INCREF(&pylv_tileview_ext_t_Type);
    PyModule_AddObject(module, "tileview_ext_t", (PyObject *) &pylv_tileview_ext_t_Type); 

    Py_INCREF(&pylv_mbox_ext_t_Type);
    PyModule_AddObject(module, "mbox_ext_t", (PyObject *) &pylv_mbox_ext_t_Type); 

    Py_INCREF(&pylv_lmeter_ext_t_Type);
    PyModule_AddObject(module, "lmeter_ext_t", (PyObject *) &pylv_lmeter_ext_t_Type); 

    Py_INCREF(&pylv_gauge_ext_t_Type);
    PyModule_AddObject(module, "gauge_ext_t", (PyObject *) &pylv_gauge_ext_t_Type); 

    Py_INCREF(&pylv_sw_ext_t_Type);
    PyModule_AddObject(module, "sw_ext_t", (PyObject *) &pylv_sw_ext_t_Type); 

    Py_INCREF(&pylv_arc_ext_t_Type);
    PyModule_AddObject(module, "arc_ext_t", (PyObject *) &pylv_arc_ext_t_Type); 

    Py_INCREF(&pylv_preload_ext_t_Type);
    PyModule_AddObject(module, "preload_ext_t", (PyObject *) &pylv_preload_ext_t_Type); 

    Py_INCREF(&pylv_calendar_date_t_Type);
    PyModule_AddObject(module, "calendar_date_t", (PyObject *) &pylv_calendar_date_t_Type); 

    Py_INCREF(&pylv_calendar_ext_t_Type);
    PyModule_AddObject(module, "calendar_ext_t", (PyObject *) &pylv_calendar_ext_t_Type); 

    Py_INCREF(&pylv_spinbox_ext_t_Type);
    PyModule_AddObject(module, "spinbox_ext_t", (PyObject *) &pylv_spinbox_ext_t_Type); 


    
    PyModule_AddObject(module, "TASK_PRIO", build_constclass('d', "TASK_PRIO", "OFF", LV_TASK_PRIO_OFF, "LOWEST", LV_TASK_PRIO_LOWEST, "LOW", LV_TASK_PRIO_LOW, "MID", LV_TASK_PRIO_MID, "HIGH", LV_TASK_PRIO_HIGH, "HIGHEST", LV_TASK_PRIO_HIGHEST, "NUM", LV_TASK_PRIO_NUM, NULL));
    PyModule_AddObject(module, "INDEV_TYPE", build_constclass('d', "INDEV_TYPE", "NONE", LV_INDEV_TYPE_NONE, "POINTER", LV_INDEV_TYPE_POINTER, "KEYPAD", LV_INDEV_TYPE_KEYPAD, "BUTTON", LV_INDEV_TYPE_BUTTON, "ENCODER", LV_INDEV_TYPE_ENCODER, NULL));
    PyModule_AddObject(module, "INDEV_STATE", build_constclass('d', "INDEV_STATE", "REL", LV_INDEV_STATE_REL, "PR", LV_INDEV_STATE_PR, NULL));
    PyModule_AddObject(module, "BORDER", build_constclass('d', "BORDER", "NONE", LV_BORDER_NONE, "BOTTOM", LV_BORDER_BOTTOM, "TOP", LV_BORDER_TOP, "LEFT", LV_BORDER_LEFT, "RIGHT", LV_BORDER_RIGHT, "FULL", LV_BORDER_FULL, "INTERNAL", LV_BORDER_INTERNAL, NULL));
    PyModule_AddObject(module, "SHADOW", build_constclass('d', "SHADOW", "BOTTOM", LV_SHADOW_BOTTOM, "FULL", LV_SHADOW_FULL, NULL));
    PyModule_AddObject(module, "RES", build_constclass('d', "RES", "INV", LV_RES_INV, "OK", LV_RES_OK, NULL));
    PyModule_AddObject(module, "DESIGN", build_constclass('d', "DESIGN", "DRAW_MAIN", LV_DESIGN_DRAW_MAIN, "DRAW_POST", LV_DESIGN_DRAW_POST, "COVER_CHK", LV_DESIGN_COVER_CHK, NULL));
    PyModule_AddObject(module, "EVENT", build_constclass('d', "EVENT", "PRESSED", LV_EVENT_PRESSED, "PRESSING", LV_EVENT_PRESSING, "PRESS_LOST", LV_EVENT_PRESS_LOST, "SHORT_CLICKED", LV_EVENT_SHORT_CLICKED, "LONG_PRESSED", LV_EVENT_LONG_PRESSED, "LONG_PRESSED_REPEAT", LV_EVENT_LONG_PRESSED_REPEAT, "CLICKED", LV_EVENT_CLICKED, "RELEASED", LV_EVENT_RELEASED, "DRAG_BEGIN", LV_EVENT_DRAG_BEGIN, "DRAG_END", LV_EVENT_DRAG_END, "DRAG_THROW_BEGIN", LV_EVENT_DRAG_THROW_BEGIN, "KEY", LV_EVENT_KEY, "FOCUSED", LV_EVENT_FOCUSED, "DEFOCUSED", LV_EVENT_DEFOCUSED, "VALUE_CHANGED", LV_EVENT_VALUE_CHANGED, "INSERT", LV_EVENT_INSERT, "SELECTED", LV_EVENT_SELECTED, "REFRESH", LV_EVENT_REFRESH, "APPLY", LV_EVENT_APPLY, "CANCEL", LV_EVENT_CANCEL, "DELETE", LV_EVENT_DELETE, NULL));
    PyModule_AddObject(module, "SIGNAL", build_constclass('d', "SIGNAL", "CLEANUP", LV_SIGNAL_CLEANUP, "CHILD_CHG", LV_SIGNAL_CHILD_CHG, "CORD_CHG", LV_SIGNAL_CORD_CHG, "PARENT_SIZE_CHG", LV_SIGNAL_PARENT_SIZE_CHG, "STYLE_CHG", LV_SIGNAL_STYLE_CHG, "REFR_EXT_DRAW_PAD", LV_SIGNAL_REFR_EXT_DRAW_PAD, "GET_TYPE", LV_SIGNAL_GET_TYPE, "PRESSED", LV_SIGNAL_PRESSED, "PRESSING", LV_SIGNAL_PRESSING, "PRESS_LOST", LV_SIGNAL_PRESS_LOST, "RELEASED", LV_SIGNAL_RELEASED, "LONG_PRESS", LV_SIGNAL_LONG_PRESS, "LONG_PRESS_REP", LV_SIGNAL_LONG_PRESS_REP, "DRAG_BEGIN", LV_SIGNAL_DRAG_BEGIN, "DRAG_END", LV_SIGNAL_DRAG_END, "FOCUS", LV_SIGNAL_FOCUS, "DEFOCUS", LV_SIGNAL_DEFOCUS, "CONTROL", LV_SIGNAL_CONTROL, "GET_EDITABLE", LV_SIGNAL_GET_EDITABLE, NULL));
    PyModule_AddObject(module, "ALIGN", build_constclass('d', "ALIGN", "CENTER", LV_ALIGN_CENTER, "IN_TOP_LEFT", LV_ALIGN_IN_TOP_LEFT, "IN_TOP_MID", LV_ALIGN_IN_TOP_MID, "IN_TOP_RIGHT", LV_ALIGN_IN_TOP_RIGHT, "IN_BOTTOM_LEFT", LV_ALIGN_IN_BOTTOM_LEFT, "IN_BOTTOM_MID", LV_ALIGN_IN_BOTTOM_MID, "IN_BOTTOM_RIGHT", LV_ALIGN_IN_BOTTOM_RIGHT, "IN_LEFT_MID", LV_ALIGN_IN_LEFT_MID, "IN_RIGHT_MID", LV_ALIGN_IN_RIGHT_MID, "OUT_TOP_LEFT", LV_ALIGN_OUT_TOP_LEFT, "OUT_TOP_MID", LV_ALIGN_OUT_TOP_MID, "OUT_TOP_RIGHT", LV_ALIGN_OUT_TOP_RIGHT, "OUT_BOTTOM_LEFT", LV_ALIGN_OUT_BOTTOM_LEFT, "OUT_BOTTOM_MID", LV_ALIGN_OUT_BOTTOM_MID, "OUT_BOTTOM_RIGHT", LV_ALIGN_OUT_BOTTOM_RIGHT, "OUT_LEFT_TOP", LV_ALIGN_OUT_LEFT_TOP, "OUT_LEFT_MID", LV_ALIGN_OUT_LEFT_MID, "OUT_LEFT_BOTTOM", LV_ALIGN_OUT_LEFT_BOTTOM, "OUT_RIGHT_TOP", LV_ALIGN_OUT_RIGHT_TOP, "OUT_RIGHT_MID", LV_ALIGN_OUT_RIGHT_MID, "OUT_RIGHT_BOTTOM", LV_ALIGN_OUT_RIGHT_BOTTOM, NULL));
    PyModule_AddObject(module, "DRAG_DIR", build_constclass('d', "DRAG_DIR", "HOR", LV_DRAG_DIR_HOR, "VER", LV_DRAG_DIR_VER, "ALL", LV_DRAG_DIR_ALL, NULL));
    PyModule_AddObject(module, "PROTECT", build_constclass('d', "PROTECT", "NONE", LV_PROTECT_NONE, "CHILD_CHG", LV_PROTECT_CHILD_CHG, "PARENT", LV_PROTECT_PARENT, "POS", LV_PROTECT_POS, "FOLLOW", LV_PROTECT_FOLLOW, "PRESS_LOST", LV_PROTECT_PRESS_LOST, "CLICK_FOCUS", LV_PROTECT_CLICK_FOCUS, NULL));
    PyModule_AddObject(module, "KEY", build_constclass('d', "KEY", "UP", LV_KEY_UP, "DOWN", LV_KEY_DOWN, "RIGHT", LV_KEY_RIGHT, "LEFT", LV_KEY_LEFT, "ESC", LV_KEY_ESC, "DEL", LV_KEY_DEL, "BACKSPACE", LV_KEY_BACKSPACE, "ENTER", LV_KEY_ENTER, "NEXT", LV_KEY_NEXT, "PREV", LV_KEY_PREV, "HOME", LV_KEY_HOME, "END", LV_KEY_END, NULL));
    PyModule_AddObject(module, "GROUP_REFOCUS_POLICY", build_constclass('d', "GROUP_REFOCUS_POLICY", "NEXT", LV_GROUP_REFOCUS_POLICY_NEXT, "PREV", LV_GROUP_REFOCUS_POLICY_PREV, NULL));
    PyModule_AddObject(module, "LAYOUT", build_constclass('d', "LAYOUT", "OFF", LV_LAYOUT_OFF, "CENTER", LV_LAYOUT_CENTER, "COL_L", LV_LAYOUT_COL_L, "COL_M", LV_LAYOUT_COL_M, "COL_R", LV_LAYOUT_COL_R, "ROW_T", LV_LAYOUT_ROW_T, "ROW_M", LV_LAYOUT_ROW_M, "ROW_B", LV_LAYOUT_ROW_B, "PRETTY", LV_LAYOUT_PRETTY, "GRID", LV_LAYOUT_GRID, NULL));
    PyModule_AddObject(module, "FIT", build_constclass('d', "FIT", "NONE", LV_FIT_NONE, "TIGHT", LV_FIT_TIGHT, "FLOOD", LV_FIT_FLOOD, "FILL", LV_FIT_FILL, NULL));
    PyModule_AddObject(module, "BTN_STATE", build_constclass('d', "BTN_STATE", "REL", LV_BTN_STATE_REL, "PR", LV_BTN_STATE_PR, "TGL_REL", LV_BTN_STATE_TGL_REL, "TGL_PR", LV_BTN_STATE_TGL_PR, "INA", LV_BTN_STATE_INA, "NUM", LV_BTN_STATE_NUM, NULL));
    PyModule_AddObject(module, "BTN_STYLE", build_constclass('d', "BTN_STYLE", "REL", LV_BTN_STYLE_REL, "PR", LV_BTN_STYLE_PR, "TGL_REL", LV_BTN_STYLE_TGL_REL, "TGL_PR", LV_BTN_STYLE_TGL_PR, "INA", LV_BTN_STYLE_INA, NULL));
    PyModule_AddObject(module, "TXT_FLAG", build_constclass('d', "TXT_FLAG", "NONE", LV_TXT_FLAG_NONE, "RECOLOR", LV_TXT_FLAG_RECOLOR, "EXPAND", LV_TXT_FLAG_EXPAND, "CENTER", LV_TXT_FLAG_CENTER, "RIGHT", LV_TXT_FLAG_RIGHT, NULL));
    PyModule_AddObject(module, "TXT_CMD_STATE", build_constclass('d', "TXT_CMD_STATE", "WAIT", LV_TXT_CMD_STATE_WAIT, "PAR", LV_TXT_CMD_STATE_PAR, "IN", LV_TXT_CMD_STATE_IN, NULL));
    PyModule_AddObject(module, "FS_RES", build_constclass('d', "FS_RES", "OK", LV_FS_RES_OK, "HW_ERR", LV_FS_RES_HW_ERR, "FS_ERR", LV_FS_RES_FS_ERR, "NOT_EX", LV_FS_RES_NOT_EX, "FULL", LV_FS_RES_FULL, "LOCKED", LV_FS_RES_LOCKED, "DENIED", LV_FS_RES_DENIED, "BUSY", LV_FS_RES_BUSY, "TOUT", LV_FS_RES_TOUT, "NOT_IMP", LV_FS_RES_NOT_IMP, "OUT_OF_MEM", LV_FS_RES_OUT_OF_MEM, "INV_PARAM", LV_FS_RES_INV_PARAM, "UNKNOWN", LV_FS_RES_UNKNOWN, NULL));
    PyModule_AddObject(module, "FS_MODE", build_constclass('d', "FS_MODE", "WR", LV_FS_MODE_WR, "RD", LV_FS_MODE_RD, NULL));
    PyModule_AddObject(module, "IMG_SRC", build_constclass('d', "IMG_SRC", "VARIABLE", LV_IMG_SRC_VARIABLE, "FILE", LV_IMG_SRC_FILE, "SYMBOL", LV_IMG_SRC_SYMBOL, "UNKNOWN", LV_IMG_SRC_UNKNOWN, NULL));
    PyModule_AddObject(module, "IMG_CF", build_constclass('d', "IMG_CF", "UNKNOWN", LV_IMG_CF_UNKNOWN, "RAW", LV_IMG_CF_RAW, "RAW_ALPHA", LV_IMG_CF_RAW_ALPHA, "RAW_CHROMA_KEYED", LV_IMG_CF_RAW_CHROMA_KEYED, "TRUE_COLOR", LV_IMG_CF_TRUE_COLOR, "TRUE_COLOR_ALPHA", LV_IMG_CF_TRUE_COLOR_ALPHA, "TRUE_COLOR_CHROMA_KEYED", LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED, "INDEXED_1BIT", LV_IMG_CF_INDEXED_1BIT, "INDEXED_2BIT", LV_IMG_CF_INDEXED_2BIT, "INDEXED_4BIT", LV_IMG_CF_INDEXED_4BIT, "INDEXED_8BIT", LV_IMG_CF_INDEXED_8BIT, "ALPHA_1BIT", LV_IMG_CF_ALPHA_1BIT, "ALPHA_2BIT", LV_IMG_CF_ALPHA_2BIT, "ALPHA_4BIT", LV_IMG_CF_ALPHA_4BIT, "ALPHA_8BIT", LV_IMG_CF_ALPHA_8BIT, NULL));
    PyModule_AddObject(module, "IMGBTN_STYLE", build_constclass('d', "IMGBTN_STYLE", "REL", LV_IMGBTN_STYLE_REL, "PR", LV_IMGBTN_STYLE_PR, "TGL_REL", LV_IMGBTN_STYLE_TGL_REL, "TGL_PR", LV_IMGBTN_STYLE_TGL_PR, "INA", LV_IMGBTN_STYLE_INA, NULL));
    PyModule_AddObject(module, "LABEL_LONG", build_constclass('d', "LABEL_LONG", "EXPAND", LV_LABEL_LONG_EXPAND, "BREAK", LV_LABEL_LONG_BREAK, "DOT", LV_LABEL_LONG_DOT, "ROLL", LV_LABEL_LONG_ROLL, "ROLL_CIRC", LV_LABEL_LONG_ROLL_CIRC, "CROP", LV_LABEL_LONG_CROP, NULL));
    PyModule_AddObject(module, "LABEL_ALIGN", build_constclass('d', "LABEL_ALIGN", "LEFT", LV_LABEL_ALIGN_LEFT, "CENTER", LV_LABEL_ALIGN_CENTER, "RIGHT", LV_LABEL_ALIGN_RIGHT, NULL));
    PyModule_AddObject(module, "SB_MODE", build_constclass('d', "SB_MODE", "OFF", LV_SB_MODE_OFF, "ON", LV_SB_MODE_ON, "DRAG", LV_SB_MODE_DRAG, "AUTO", LV_SB_MODE_AUTO, "HIDE", LV_SB_MODE_HIDE, "UNHIDE", LV_SB_MODE_UNHIDE, NULL));
    PyModule_AddObject(module, "PAGE_EDGE", build_constclass('d', "PAGE_EDGE", "LEFT", LV_PAGE_EDGE_LEFT, "TOP", LV_PAGE_EDGE_TOP, "RIGHT", LV_PAGE_EDGE_RIGHT, "BOTTOM", LV_PAGE_EDGE_BOTTOM, NULL));
    PyModule_AddObject(module, "PAGE_STYLE", build_constclass('d', "PAGE_STYLE", "BG", LV_PAGE_STYLE_BG, "SCRL", LV_PAGE_STYLE_SCRL, "SB", LV_PAGE_STYLE_SB, "EDGE_FLASH", LV_PAGE_STYLE_EDGE_FLASH, NULL));
    PyModule_AddObject(module, "LIST_STYLE", build_constclass('d', "LIST_STYLE", "BG", LV_LIST_STYLE_BG, "SCRL", LV_LIST_STYLE_SCRL, "SB", LV_LIST_STYLE_SB, "EDGE_FLASH", LV_LIST_STYLE_EDGE_FLASH, "BTN_REL", LV_LIST_STYLE_BTN_REL, "BTN_PR", LV_LIST_STYLE_BTN_PR, "BTN_TGL_REL", LV_LIST_STYLE_BTN_TGL_REL, "BTN_TGL_PR", LV_LIST_STYLE_BTN_TGL_PR, "BTN_INA", LV_LIST_STYLE_BTN_INA, NULL));
    PyModule_AddObject(module, "CHART_TYPE", build_constclass('d', "CHART_TYPE", "LINE", LV_CHART_TYPE_LINE, "COLUMN", LV_CHART_TYPE_COLUMN, "POINT", LV_CHART_TYPE_POINT, "VERTICAL_LINE", LV_CHART_TYPE_VERTICAL_LINE, "AREA", LV_CHART_TYPE_AREA, NULL));
    PyModule_AddObject(module, "CHART_UPDATE_MODE", build_constclass('d', "CHART_UPDATE_MODE", "SHIFT", LV_CHART_UPDATE_MODE_SHIFT, "CIRCULAR", LV_CHART_UPDATE_MODE_CIRCULAR, NULL));
    PyModule_AddObject(module, "CHART_AXIS_DRAW_LAST", build_constclass('d', "CHART_AXIS_DRAW_LAST", "TICK", LV_CHART_AXIS_DRAW_LAST_TICK, NULL));
    PyModule_AddObject(module, "TABLE_STYLE", build_constclass('d', "TABLE_STYLE", "BG", LV_TABLE_STYLE_BG, "CELL1", LV_TABLE_STYLE_CELL1, "CELL2", LV_TABLE_STYLE_CELL2, "CELL3", LV_TABLE_STYLE_CELL3, "CELL4", LV_TABLE_STYLE_CELL4, NULL));
    PyModule_AddObject(module, "CB_STYLE", build_constclass('d', "CB_STYLE", "BG", LV_CB_STYLE_BG, "BOX_REL", LV_CB_STYLE_BOX_REL, "BOX_PR", LV_CB_STYLE_BOX_PR, "BOX_TGL_REL", LV_CB_STYLE_BOX_TGL_REL, "BOX_TGL_PR", LV_CB_STYLE_BOX_TGL_PR, "BOX_INA", LV_CB_STYLE_BOX_INA, NULL));
    PyModule_AddObject(module, "BAR_STYLE", build_constclass('d', "BAR_STYLE", "BG", LV_BAR_STYLE_BG, "INDIC", LV_BAR_STYLE_INDIC, NULL));
    PyModule_AddObject(module, "SLIDER_STYLE", build_constclass('d', "SLIDER_STYLE", "BG", LV_SLIDER_STYLE_BG, "INDIC", LV_SLIDER_STYLE_INDIC, "KNOB", LV_SLIDER_STYLE_KNOB, NULL));
    PyModule_AddObject(module, "BTNM_CTRL", build_constclass('d', "BTNM_CTRL", "HIDDEN", LV_BTNM_CTRL_HIDDEN, "NO_REPEAT", LV_BTNM_CTRL_NO_REPEAT, "INACTIVE", LV_BTNM_CTRL_INACTIVE, "TGL_ENABLE", LV_BTNM_CTRL_TGL_ENABLE, "TGL_STATE", LV_BTNM_CTRL_TGL_STATE, "CLICK_TRIG", LV_BTNM_CTRL_CLICK_TRIG, NULL));
    PyModule_AddObject(module, "BTNM_STYLE", build_constclass('d', "BTNM_STYLE", "BG", LV_BTNM_STYLE_BG, "BTN_REL", LV_BTNM_STYLE_BTN_REL, "BTN_PR", LV_BTNM_STYLE_BTN_PR, "BTN_TGL_REL", LV_BTNM_STYLE_BTN_TGL_REL, "BTN_TGL_PR", LV_BTNM_STYLE_BTN_TGL_PR, "BTN_INA", LV_BTNM_STYLE_BTN_INA, NULL));
    PyModule_AddObject(module, "KB_MODE", build_constclass('d', "KB_MODE", "TEXT", LV_KB_MODE_TEXT, "NUM", LV_KB_MODE_NUM, NULL));
    PyModule_AddObject(module, "KB_STYLE", build_constclass('d', "KB_STYLE", "BG", LV_KB_STYLE_BG, "BTN_REL", LV_KB_STYLE_BTN_REL, "BTN_PR", LV_KB_STYLE_BTN_PR, "BTN_TGL_REL", LV_KB_STYLE_BTN_TGL_REL, "BTN_TGL_PR", LV_KB_STYLE_BTN_TGL_PR, "BTN_INA", LV_KB_STYLE_BTN_INA, NULL));
    PyModule_AddObject(module, "DDLIST_STYLE", build_constclass('d', "DDLIST_STYLE", "BG", LV_DDLIST_STYLE_BG, "SEL", LV_DDLIST_STYLE_SEL, "SB", LV_DDLIST_STYLE_SB, NULL));
    PyModule_AddObject(module, "ROLLER_STYLE", build_constclass('d', "ROLLER_STYLE", "BG", LV_ROLLER_STYLE_BG, "SEL", LV_ROLLER_STYLE_SEL, NULL));
    PyModule_AddObject(module, "CURSOR", build_constclass('d', "CURSOR", "NONE", LV_CURSOR_NONE, "LINE", LV_CURSOR_LINE, "BLOCK", LV_CURSOR_BLOCK, "OUTLINE", LV_CURSOR_OUTLINE, "UNDERLINE", LV_CURSOR_UNDERLINE, "HIDDEN", LV_CURSOR_HIDDEN, NULL));
    PyModule_AddObject(module, "TA_STYLE", build_constclass('d', "TA_STYLE", "BG", LV_TA_STYLE_BG, "SB", LV_TA_STYLE_SB, "EDGE_FLASH", LV_TA_STYLE_EDGE_FLASH, "CURSOR", LV_TA_STYLE_CURSOR, "PLACEHOLDER", LV_TA_STYLE_PLACEHOLDER, NULL));
    PyModule_AddObject(module, "CANVAS_STYLE", build_constclass('d', "CANVAS_STYLE", "MAIN", LV_CANVAS_STYLE_MAIN, NULL));
    PyModule_AddObject(module, "WIN_STYLE", build_constclass('d', "WIN_STYLE", "BG", LV_WIN_STYLE_BG, "CONTENT_BG", LV_WIN_STYLE_CONTENT_BG, "CONTENT_SCRL", LV_WIN_STYLE_CONTENT_SCRL, "SB", LV_WIN_STYLE_SB, "HEADER", LV_WIN_STYLE_HEADER, "BTN_REL", LV_WIN_STYLE_BTN_REL, "BTN_PR", LV_WIN_STYLE_BTN_PR, NULL));
    PyModule_AddObject(module, "TABVIEW_BTNS_POS", build_constclass('d', "TABVIEW_BTNS_POS", "TOP", LV_TABVIEW_BTNS_POS_TOP, "BOTTOM", LV_TABVIEW_BTNS_POS_BOTTOM, "LEFT", LV_TABVIEW_BTNS_POS_LEFT, "RIGHT", LV_TABVIEW_BTNS_POS_RIGHT, NULL));
    PyModule_AddObject(module, "TABVIEW_STYLE", build_constclass('d', "TABVIEW_STYLE", "BG", LV_TABVIEW_STYLE_BG, "INDIC", LV_TABVIEW_STYLE_INDIC, "BTN_BG", LV_TABVIEW_STYLE_BTN_BG, "BTN_REL", LV_TABVIEW_STYLE_BTN_REL, "BTN_PR", LV_TABVIEW_STYLE_BTN_PR, "BTN_TGL_REL", LV_TABVIEW_STYLE_BTN_TGL_REL, "BTN_TGL_PR", LV_TABVIEW_STYLE_BTN_TGL_PR, NULL));
    PyModule_AddObject(module, "TILEVIEW_STYLE", build_constclass('d', "TILEVIEW_STYLE", "BG", LV_TILEVIEW_STYLE_BG, NULL));
    PyModule_AddObject(module, "MBOX_STYLE", build_constclass('d', "MBOX_STYLE", "BG", LV_MBOX_STYLE_BG, "BTN_BG", LV_MBOX_STYLE_BTN_BG, "BTN_REL", LV_MBOX_STYLE_BTN_REL, "BTN_PR", LV_MBOX_STYLE_BTN_PR, "BTN_TGL_REL", LV_MBOX_STYLE_BTN_TGL_REL, "BTN_TGL_PR", LV_MBOX_STYLE_BTN_TGL_PR, "BTN_INA", LV_MBOX_STYLE_BTN_INA, NULL));
    PyModule_AddObject(module, "SW_STYLE", build_constclass('d', "SW_STYLE", "BG", LV_SW_STYLE_BG, "INDIC", LV_SW_STYLE_INDIC, "KNOB_OFF", LV_SW_STYLE_KNOB_OFF, "KNOB_ON", LV_SW_STYLE_KNOB_ON, NULL));
    PyModule_AddObject(module, "ARC_STYLE", build_constclass('d', "ARC_STYLE", "MAIN", LV_ARC_STYLE_MAIN, NULL));
    PyModule_AddObject(module, "PRELOAD_TYPE", build_constclass('d', "PRELOAD_TYPE", "SPINNING_ARC", LV_PRELOAD_TYPE_SPINNING_ARC, "FILLSPIN_ARC", LV_PRELOAD_TYPE_FILLSPIN_ARC, NULL));
    PyModule_AddObject(module, "PRELOAD_DIR", build_constclass('d', "PRELOAD_DIR", "FORWARD", LV_PRELOAD_DIR_FORWARD, "BACKWARD", LV_PRELOAD_DIR_BACKWARD, NULL));
    PyModule_AddObject(module, "PRELOAD_STYLE", build_constclass('d', "PRELOAD_STYLE", "MAIN", LV_PRELOAD_STYLE_MAIN, NULL));
    PyModule_AddObject(module, "CALENDAR_STYLE", build_constclass('d', "CALENDAR_STYLE", "BG", LV_CALENDAR_STYLE_BG, "HEADER", LV_CALENDAR_STYLE_HEADER, "HEADER_PR", LV_CALENDAR_STYLE_HEADER_PR, "DAY_NAMES", LV_CALENDAR_STYLE_DAY_NAMES, "HIGHLIGHTED_DAYS", LV_CALENDAR_STYLE_HIGHLIGHTED_DAYS, "INACTIVE_DAYS", LV_CALENDAR_STYLE_INACTIVE_DAYS, "WEEK_BOX", LV_CALENDAR_STYLE_WEEK_BOX, "TODAY_BOX", LV_CALENDAR_STYLE_TODAY_BOX, NULL));
    PyModule_AddObject(module, "SPINBOX_STYLE", build_constclass('d', "SPINBOX_STYLE", "BG", LV_SPINBOX_STYLE_BG, "SB", LV_SPINBOX_STYLE_SB, "CURSOR", LV_SPINBOX_STYLE_CURSOR, NULL));


    PyModule_AddObject(module, "SYMBOL", build_constclass('s', "SYMBOL", "AUDIO", LV_SYMBOL_AUDIO, "VIDEO", LV_SYMBOL_VIDEO, "LIST", LV_SYMBOL_LIST, "OK", LV_SYMBOL_OK, "CLOSE", LV_SYMBOL_CLOSE, "POWER", LV_SYMBOL_POWER, "SETTINGS", LV_SYMBOL_SETTINGS, "TRASH", LV_SYMBOL_TRASH, "HOME", LV_SYMBOL_HOME, "DOWNLOAD", LV_SYMBOL_DOWNLOAD, "DRIVE", LV_SYMBOL_DRIVE, "REFRESH", LV_SYMBOL_REFRESH, "MUTE", LV_SYMBOL_MUTE, "VOLUME_MID", LV_SYMBOL_VOLUME_MID, "VOLUME_MAX", LV_SYMBOL_VOLUME_MAX, "IMAGE", LV_SYMBOL_IMAGE, "EDIT", LV_SYMBOL_EDIT, "PREV", LV_SYMBOL_PREV, "PLAY", LV_SYMBOL_PLAY, "PAUSE", LV_SYMBOL_PAUSE, "STOP", LV_SYMBOL_STOP, "NEXT", LV_SYMBOL_NEXT, "EJECT", LV_SYMBOL_EJECT, "LEFT", LV_SYMBOL_LEFT, "RIGHT", LV_SYMBOL_RIGHT, "PLUS", LV_SYMBOL_PLUS, "MINUS", LV_SYMBOL_MINUS, "WARNING", LV_SYMBOL_WARNING, "SHUFFLE", LV_SYMBOL_SHUFFLE, "UP", LV_SYMBOL_UP, "DOWN", LV_SYMBOL_DOWN, "LOOP", LV_SYMBOL_LOOP, "DIRECTORY", LV_SYMBOL_DIRECTORY, "UPLOAD", LV_SYMBOL_UPLOAD, "CALL", LV_SYMBOL_CALL, "CUT", LV_SYMBOL_CUT, "COPY", LV_SYMBOL_COPY, "SAVE", LV_SYMBOL_SAVE, "CHARGE", LV_SYMBOL_CHARGE, "BELL", LV_SYMBOL_BELL, "KEYBOARD", LV_SYMBOL_KEYBOARD, "GPS", LV_SYMBOL_GPS, "FILE", LV_SYMBOL_FILE, "WIFI", LV_SYMBOL_WIFI, "BATTERY_FULL", LV_SYMBOL_BATTERY_FULL, "BATTERY_3", LV_SYMBOL_BATTERY_3, "BATTERY_2", LV_SYMBOL_BATTERY_2, "BATTERY_1", LV_SYMBOL_BATTERY_1, "BATTERY_EMPTY", LV_SYMBOL_BATTERY_EMPTY, "BLUETOOTH", LV_SYMBOL_BLUETOOTH, "DUMMY", LV_SYMBOL_DUMMY, NULL));


    PyModule_AddObject(module, "COLOR", build_constclass('C', "COLOR", "WHITE", LV_COLOR_WHITE, "SILVER", LV_COLOR_SILVER, "GRAY", LV_COLOR_GRAY, "BLACK", LV_COLOR_BLACK, "RED", LV_COLOR_RED, "MAROON", LV_COLOR_MAROON, "YELLOW", LV_COLOR_YELLOW, "OLIVE", LV_COLOR_OLIVE, "LIME", LV_COLOR_LIME, "GREEN", LV_COLOR_GREEN, "CYAN", LV_COLOR_CYAN, "AQUA", LV_COLOR_AQUA, "TEAL", LV_COLOR_TEAL, "BLUE", LV_COLOR_BLUE, "NAVY", LV_COLOR_NAVY, "MAGENTA", LV_COLOR_MAGENTA, "PURPLE", LV_COLOR_PURPLE, "ORANGE", LV_COLOR_ORANGE, "SIZE", LV_COLOR_SIZE, NULL));


   PyModule_AddObject(module, "font_dejavu_10", pystruct_from_c(&pylv_font_t_Type, &lv_font_dejavu_10, sizeof(lv_font_t), 0));
   PyModule_AddObject(module, "font_dejavu_10_latin_sup", pystruct_from_c(&pylv_font_t_Type, &lv_font_dejavu_10_latin_sup, sizeof(lv_font_t), 0));
   PyModule_AddObject(module, "font_dejavu_10_cyrillic", pystruct_from_c(&pylv_font_t_Type, &lv_font_dejavu_10_cyrillic, sizeof(lv_font_t), 0));
   PyModule_AddObject(module, "font_symbol_10", pystruct_from_c(&pylv_font_t_Type, &lv_font_symbol_10, sizeof(lv_font_t), 0));
   PyModule_AddObject(module, "font_dejavu_20", pystruct_from_c(&pylv_font_t_Type, &lv_font_dejavu_20, sizeof(lv_font_t), 0));
   PyModule_AddObject(module, "font_dejavu_20_latin_sup", pystruct_from_c(&pylv_font_t_Type, &lv_font_dejavu_20_latin_sup, sizeof(lv_font_t), 0));
   PyModule_AddObject(module, "font_dejavu_20_cyrillic", pystruct_from_c(&pylv_font_t_Type, &lv_font_dejavu_20_cyrillic, sizeof(lv_font_t), 0));
   PyModule_AddObject(module, "font_symbol_20", pystruct_from_c(&pylv_font_t_Type, &lv_font_symbol_20, sizeof(lv_font_t), 0));
   PyModule_AddObject(module, "font_dejavu_30", pystruct_from_c(&pylv_font_t_Type, &lv_font_dejavu_30, sizeof(lv_font_t), 0));
   PyModule_AddObject(module, "font_dejavu_30_latin_sup", pystruct_from_c(&pylv_font_t_Type, &lv_font_dejavu_30_latin_sup, sizeof(lv_font_t), 0));
   PyModule_AddObject(module, "font_dejavu_30_cyrillic", pystruct_from_c(&pylv_font_t_Type, &lv_font_dejavu_30_cyrillic, sizeof(lv_font_t), 0));
   PyModule_AddObject(module, "font_symbol_30", pystruct_from_c(&pylv_font_t_Type, &lv_font_symbol_30, sizeof(lv_font_t), 0));
   PyModule_AddObject(module, "font_dejavu_40", pystruct_from_c(&pylv_font_t_Type, &lv_font_dejavu_40, sizeof(lv_font_t), 0));
   PyModule_AddObject(module, "font_dejavu_40_latin_sup", pystruct_from_c(&pylv_font_t_Type, &lv_font_dejavu_40_latin_sup, sizeof(lv_font_t), 0));
   PyModule_AddObject(module, "font_dejavu_40_cyrillic", pystruct_from_c(&pylv_font_t_Type, &lv_font_dejavu_40_cyrillic, sizeof(lv_font_t), 0));
   PyModule_AddObject(module, "font_symbol_40", pystruct_from_c(&pylv_font_t_Type, &lv_font_symbol_40, sizeof(lv_font_t), 0));
   PyModule_AddObject(module, "font_monospace_8", pystruct_from_c(&pylv_font_t_Type, &lv_font_monospace_8, sizeof(lv_font_t), 0));
   PyModule_AddObject(module, "style_scr", pystruct_from_c(&pylv_style_t_Type, &lv_style_scr, sizeof(lv_style_t), 0));
   PyModule_AddObject(module, "style_transp", pystruct_from_c(&pylv_style_t_Type, &lv_style_transp, sizeof(lv_style_t), 0));
   PyModule_AddObject(module, "style_transp_fit", pystruct_from_c(&pylv_style_t_Type, &lv_style_transp_fit, sizeof(lv_style_t), 0));
   PyModule_AddObject(module, "style_transp_tight", pystruct_from_c(&pylv_style_t_Type, &lv_style_transp_tight, sizeof(lv_style_t), 0));
   PyModule_AddObject(module, "style_plain", pystruct_from_c(&pylv_style_t_Type, &lv_style_plain, sizeof(lv_style_t), 0));
   PyModule_AddObject(module, "style_plain_color", pystruct_from_c(&pylv_style_t_Type, &lv_style_plain_color, sizeof(lv_style_t), 0));
   PyModule_AddObject(module, "style_pretty", pystruct_from_c(&pylv_style_t_Type, &lv_style_pretty, sizeof(lv_style_t), 0));
   PyModule_AddObject(module, "style_pretty_color", pystruct_from_c(&pylv_style_t_Type, &lv_style_pretty_color, sizeof(lv_style_t), 0));
   PyModule_AddObject(module, "style_btn_rel", pystruct_from_c(&pylv_style_t_Type, &lv_style_btn_rel, sizeof(lv_style_t), 0));
   PyModule_AddObject(module, "style_btn_pr", pystruct_from_c(&pylv_style_t_Type, &lv_style_btn_pr, sizeof(lv_style_t), 0));
   PyModule_AddObject(module, "style_btn_tgl_rel", pystruct_from_c(&pylv_style_t_Type, &lv_style_btn_tgl_rel, sizeof(lv_style_t), 0));
   PyModule_AddObject(module, "style_btn_tgl_pr", pystruct_from_c(&pylv_style_t_Type, &lv_style_btn_tgl_pr, sizeof(lv_style_t), 0));
   PyModule_AddObject(module, "style_btn_ina", pystruct_from_c(&pylv_style_t_Type, &lv_style_btn_ina, sizeof(lv_style_t), 0));


    // refcount for typesdict is initally 1; it is used by pyobj_from_lv
    // refcounts to py{name}_Type objects are incremented due to "O" format
    typesdict = Py_BuildValue("{sOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsO}",
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
        "lv_cb", &pylv_cb_Type,
        "lv_bar", &pylv_bar_Type,
        "lv_slider", &pylv_slider_Type,
        "lv_led", &pylv_led_Type,
        "lv_btnm", &pylv_btnm_Type,
        "lv_kb", &pylv_kb_Type,
        "lv_ddlist", &pylv_ddlist_Type,
        "lv_roller", &pylv_roller_Type,
        "lv_ta", &pylv_ta_Type,
        "lv_canvas", &pylv_canvas_Type,
        "lv_win", &pylv_win_Type,
        "lv_tabview", &pylv_tabview_Type,
        "lv_tileview", &pylv_tileview_Type,
        "lv_mbox", &pylv_mbox_Type,
        "lv_lmeter", &pylv_lmeter_Type,
        "lv_gauge", &pylv_gauge_Type,
        "lv_sw", &pylv_sw_Type,
        "lv_arc", &pylv_arc_Type,
        "lv_preload", &pylv_preload_Type,
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

