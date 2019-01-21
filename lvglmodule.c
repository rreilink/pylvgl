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
    lv_obj_t *ref;
    PyObject *signal_func;
    lv_signal_func_t orig_c_signal_func;
} pylv_Obj;

typedef pylv_Obj pylv_Win;

typedef pylv_Obj pylv_Label;

typedef pylv_Obj pylv_Lmeter;

typedef struct {
    PyObject_HEAD
    lv_obj_t *ref;
    PyObject *signal_func;
    lv_signal_func_t orig_c_signal_func;
    const char **map;
} pylv_Btnm;

typedef pylv_Obj pylv_Chart;

typedef pylv_Obj pylv_Cont;

typedef pylv_Obj pylv_Led;

typedef struct {
    PyObject_HEAD
    lv_obj_t *ref;
    PyObject *signal_func;
    lv_signal_func_t orig_c_signal_func;
    const char **map;
    PyObject *ok_action;
    PyObject *hide_action;
} pylv_Kb;

typedef pylv_Obj pylv_Img;

typedef pylv_Obj pylv_Bar;

typedef pylv_Obj pylv_Arc;

typedef pylv_Obj pylv_Line;

typedef pylv_Obj pylv_Tabview;

typedef pylv_Cont pylv_Mbox;

typedef pylv_Lmeter pylv_Gauge;

typedef struct {
    PyObject_HEAD
    lv_obj_t *ref;
    PyObject *signal_func;
    lv_signal_func_t orig_c_signal_func;
    PyObject *rel_action;
    PyObject *pr_action;
} pylv_Page;

typedef struct {
    PyObject_HEAD
    lv_obj_t *ref;
    PyObject *signal_func;
    lv_signal_func_t orig_c_signal_func;
    PyObject *rel_action;
    PyObject *pr_action;
    PyObject *action;
} pylv_Ta;

typedef struct {
    PyObject_HEAD
    lv_obj_t *ref;
    PyObject *signal_func;
    lv_signal_func_t orig_c_signal_func;
    PyObject *actions[LV_BTN_ACTION_NUM];
} pylv_Btn;

typedef struct {
    PyObject_HEAD
    lv_obj_t *ref;
    PyObject *signal_func;
    lv_signal_func_t orig_c_signal_func;
    PyObject *rel_action;
    PyObject *pr_action;
    PyObject *action;
} pylv_Ddlist;

typedef pylv_Arc pylv_Preload;

typedef pylv_Page pylv_List;

typedef struct {
    PyObject_HEAD
    lv_obj_t *ref;
    PyObject *signal_func;
    lv_signal_func_t orig_c_signal_func;
    PyObject *action;
} pylv_Slider;

typedef pylv_Slider pylv_Sw;

typedef struct {
    PyObject_HEAD
    lv_obj_t *ref;
    PyObject *signal_func;
    lv_signal_func_t orig_c_signal_func;
    PyObject *actions[LV_BTN_ACTION_NUM];
    PyObject *action;
} pylv_Cb;

typedef pylv_Ddlist pylv_Roller;



/****************************************************************
 * Forward declaration of type objects                          *
 ****************************************************************/

PyObject *typesdict = NULL;


static PyTypeObject pylv_obj_Type;

static PyTypeObject pylv_win_Type;

static PyTypeObject pylv_label_Type;

static PyTypeObject pylv_lmeter_Type;

static PyTypeObject pylv_btnm_Type;

static PyTypeObject pylv_chart_Type;

static PyTypeObject pylv_cont_Type;

static PyTypeObject pylv_led_Type;

static PyTypeObject pylv_kb_Type;

static PyTypeObject pylv_img_Type;

static PyTypeObject pylv_bar_Type;

static PyTypeObject pylv_arc_Type;

static PyTypeObject pylv_line_Type;

static PyTypeObject pylv_tabview_Type;

static PyTypeObject pylv_mbox_Type;

static PyTypeObject pylv_gauge_Type;

static PyTypeObject pylv_page_Type;

static PyTypeObject pylv_ta_Type;

static PyTypeObject pylv_btn_Type;

static PyTypeObject pylv_ddlist_Type;

static PyTypeObject pylv_preload_Type;

static PyTypeObject pylv_list_Type;

static PyTypeObject pylv_slider_Type;

static PyTypeObject pylv_sw_Type;

static PyTypeObject pylv_cb_Type;

static PyTypeObject pylv_roller_Type;


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
    
    pyobj = lv_obj_get_free_ptr(obj);
    
    if (pyobj) {
        Py_INCREF(pyobj); // increase reference count of returned object
    } else {
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
        lv_obj_set_free_ptr(obj, pyobj);
    }
    
    return (PyObject *)pyobj;
}



/* Given an iterable of bytes, return a char** pointer which contains:
 *
 * - A ""-terminated char** table pointing to strings
 * - the data of those strings
 *
 * buildstringmap returns NULL and sets an exception when an error occurs.
 *
 * The returned buffer must be freed using PyMem_Free if it is no longer used
 */


static const char **buildstringmap(PyObject *arg) {

    PyObject *iterator = NULL;
    PyObject *item;
    PyObject *list = NULL;
    char *data;
    char **ptrs = 0;
    char *str;
    Py_ssize_t length;
    
    int nitems = 0, totalbytes = 0;
    int error = 0;
    
    iterator =  PyObject_GetIter(arg);
    if (!iterator) goto error;
    
    list = PyList_New(0);
    if (!list) goto error;

    while (!error && (item = PyIter_Next(iterator))) {


        if (!PyBytes_Check(item)) {
            PyErr_SetString(PyExc_ValueError, "items must be bytes objects");
            error = 1;
        } else {
            totalbytes += PyBytes_GET_SIZE(item) + 1; // +1 for terminating \0
            nitems ++;
            if (PyList_Append(list, item) != 0) {
                error = 1;
            }
        }

        Py_DECREF(item);
    }

    Py_DECREF(iterator);
    iterator = NULL;
    
    if (PyErr_Occurred()) goto error;
    
    // Add final empty string
    item = PyBytes_FromString("");
    if (!item) goto error;
    totalbytes += 1;
    nitems++;
    error = PyList_Append(list, item);
    Py_DECREF(item);
    if (error) goto error;

    // We now have a list of bytes object, and know we need totalbytes of data
    // We additionally need (nitems) * sizeof (char *) for the pointers to the strings
    ptrs = (char**) PyMem_Malloc(totalbytes + (nitems * sizeof(char *)));
    if (!ptrs) goto error;
    
    data = (char*)&ptrs[nitems]; // points to first char after the char** list
    
    for(int i = 0; i<nitems; i++) {
        if (PyBytes_AsStringAndSize(PyList_GetItem(list, i), &str, &length)!=0) goto error;
        length += 1; // length including terminating \0
        if (length > totalbytes) {
            PyErr_SetString(PyExc_AssertionError, "length <= totalbytes");
        }
    
        memcpy(data, str, length);
        ptrs[i] = data;

        data += length;
        totalbytes -= length;
    }

    if (totalbytes) {
        PyErr_SetString(PyExc_AssertionError, "totalbytes==0");
        goto error;
    }

    Py_DECREF(list);
    return (const char **)ptrs;
    
error:
    PyMem_Free(ptrs);
    Py_XDECREF(iterator);
    Py_XDECREF(list);
    
    return NULL;
    
}


/****************************************************************
 * Custom types: enums                                          *  
 ****************************************************************/

static PyType_Slot enum_slots[] = {
    {0, 0},
};

/* Create a new class which represents an enumeration
 * variadic arguments are char* name, int value, ... , NULL
 * representing the enum values
 */
static PyObject* build_enum(char *name, ...) {

    va_list args;
    va_start(args, name);

    PyType_Spec spec = {
        .name = name,
        .basicsize = sizeof(PyObject),
        .itemsize = 0,
        .flags = Py_TPFLAGS_DEFAULT,
        .slots = enum_slots /* terminated by slot==0. */
    };
    
    PyObject *enum_type = PyType_FromSpec(&spec);
    if (!enum_type) return NULL;
    
    ((PyTypeObject*)enum_type)->tp_new = NULL; // enum objects cannot be instantiated
    
    while(1) {
        char *name = va_arg(args, char*);
        if (!name) break;
        
        PyObject *value;
        value = PyLong_FromLong(va_arg(args, int));
        if (!value) goto error;
        
        PyObject_SetAttrString(enum_type, name, value);
        Py_DECREF(value);
    }

    return enum_type;

error:
    Py_DECREF(enum_type);
    return NULL;

}


/****************************************************************
 * Font class                                                  *  
 ****************************************************************/

static PyTypeObject Font_Type; // forward declaration of type

typedef struct {
    PyObject_HEAD
    const lv_font_t *ref;
} Font_Object;

static PyObject*
Font_repr(Font_Object *self) {
    return PyUnicode_FromFormat("<lvgl.Font object at %p referencing %p>", self, self->ref);
}

static PyTypeObject Font_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Font",
    .tp_basicsize = sizeof(Font_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    //no .tp_new to prevent creation of new instaces from Python
    .tp_repr = (reprfunc) Font_repr,
};

static PyObject* 
Font_From_lv_font(const lv_font_t *font) {
    Font_Object *ret;

    ret = PyObject_New(Font_Object, &Font_Type);
    if (ret) ret->ref = font;
    return (PyObject *)ret;
}

/****************************************************************
 * Style class                                                  *  
 ****************************************************************/

static PyTypeObject Style_Type; // forward declaration of type

typedef struct {
    PyObject_HEAD
    lv_style_t *ref;
} Style_Object;


static PyObject *Style_data(Style_Object *self, PyObject *args) {
    return PyMemoryView_FromMemory((void*)(self->ref), sizeof(*self->ref), PyBUF_WRITE);
}

static PyObject *Style_copy(Style_Object *self, PyObject *args) {
    // Copy style
    
    Style_Object *ret = PyObject_New(Style_Object, &Style_Type);
    if (!ret) return NULL;
    
    // This leaks memory (no PyMem_Free) but we have no way of knowing where the
    // style is in use in lvgl. So to avoid crashes, better not free it is it may
    // be in use.
    ret->ref = PyMem_Malloc(sizeof(lv_style_t));
    
    if (!ret->ref) {
        Py_DECREF(ret);
        return NULL;
    }
    
    memcpy(ret->ref, self->ref, sizeof(lv_style_t));
    
    return (PyObject *)ret;
}

static PyMemberDef Style_members[] = {
    {NULL, 0, 0, 0, NULL}
    
};

static PyObject *
Style_get_int16(Style_Object *self, void *closure)
{
    return PyLong_FromLong(*((int16_t*)((char*)self->ref + (int)closure) ));
}

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

static int
Style_set_int16(Style_Object *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, -32768, 32767)) return -1;
    
    *((int16_t*)((char*)self->ref + (int)closure) ) = v;
    return 0;
}

static PyObject *
Style_get_uint16(Style_Object *self, void *closure)
{
    return PyLong_FromLong(*((uint16_t*)((char*)self->ref + (int)closure) ));
}

static int
Style_set_uint16(Style_Object *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 65535)) return -1;
    
    *((uint16_t*)((char*)self->ref + (int)closure) ) = v;
    return 0;
}

static PyObject *
Style_get_uint8(Style_Object *self, void *closure)
{
    return PyLong_FromLong(*((uint8_t*)((char*)self->ref + (int)closure) ));
}

static int
Style_set_uint8(Style_Object *self, PyObject *value, void *closure)
{
    long v;
    if (long_to_int(value, &v, 0, 255)) return -1;
    
    *((uint8_t*)((char*)self->ref + (int)closure) ) = v;
    return 0;
}

static PyObject *
Style_get_font(Style_Object *self, void *closure) {
    return Font_From_lv_font(self->ref->text.font);
}


static int
Style_set_font(Style_Object *self, PyObject *value, void *closure)
{
    if (Py_TYPE(value) != &Font_Type) {
        PyErr_Format(PyExc_TypeError, "lvgl.Style font attribute must be of type 'lvgl.Font', not '%.200s'", Py_TYPE(value)->tp_name);
        return 1;
    }
    self->ref->text.font = ((Font_Object *)value)->ref;

    return 0;
}


static PyGetSetDef Style_getsetters[] = {
   {"body_main_color", (getter) Style_get_uint16, (setter) Style_set_uint16, "body.main_color", (void*)offsetof(lv_style_t, body.main_color)},
   {"body_grad_color", (getter) Style_get_uint16, (setter) Style_set_uint16, "body.grad_color", (void*)offsetof(lv_style_t, body.grad_color)},
   {"body_radius", (getter) Style_get_int16, (setter) Style_set_int16, "body.radius", (void*)offsetof(lv_style_t, body.radius)},
   {"body_opa", (getter) Style_get_uint8, (setter) Style_set_uint8, "body.opa", (void*)offsetof(lv_style_t, body.opa)},
   {"body_border_color", (getter) Style_get_uint16, (setter) Style_set_uint16, "body.border.color", (void*)offsetof(lv_style_t, body.border.color)},
   {"body_border_width", (getter) Style_get_int16, (setter) Style_set_int16, "body.border.width", (void*)offsetof(lv_style_t, body.border.width)},
   {"body_border_part", (getter) Style_get_uint8, (setter) Style_set_uint8, "body.border.part", (void*)offsetof(lv_style_t, body.border.part)},
   {"body_border_opa", (getter) Style_get_uint8, (setter) Style_set_uint8, "body.border.opa", (void*)offsetof(lv_style_t, body.border.opa)},
   {"body_shadow_color", (getter) Style_get_uint16, (setter) Style_set_uint16, "body.shadow.color", (void*)offsetof(lv_style_t, body.shadow.color)},
   {"body_shadow_width", (getter) Style_get_int16, (setter) Style_set_int16, "body.shadow.width", (void*)offsetof(lv_style_t, body.shadow.width)},
   {"body_shadow_type", (getter) Style_get_uint8, (setter) Style_set_uint8, "body.shadow.type", (void*)offsetof(lv_style_t, body.shadow.type)},
   {"body_padding_ver", (getter) Style_get_int16, (setter) Style_set_int16, "body.padding.ver", (void*)offsetof(lv_style_t, body.padding.ver)},
   {"body_padding_hor", (getter) Style_get_int16, (setter) Style_set_int16, "body.padding.hor", (void*)offsetof(lv_style_t, body.padding.hor)},
   {"body_padding_inner", (getter) Style_get_int16, (setter) Style_set_int16, "body.padding.inner", (void*)offsetof(lv_style_t, body.padding.inner)},
   {"text_color", (getter) Style_get_uint16, (setter) Style_set_uint16, "text.color", (void*)offsetof(lv_style_t, text.color)},
   {"text_letter_space", (getter) Style_get_int16, (setter) Style_set_int16, "text.letter_space", (void*)offsetof(lv_style_t, text.letter_space)},
   {"text_line_space", (getter) Style_get_int16, (setter) Style_set_int16, "text.line_space", (void*)offsetof(lv_style_t, text.line_space)},
   {"text_opa", (getter) Style_get_uint8, (setter) Style_set_uint8, "text.opa", (void*)offsetof(lv_style_t, text.opa)},
   {"image_color", (getter) Style_get_uint16, (setter) Style_set_uint16, "image.color", (void*)offsetof(lv_style_t, image.color)},
   {"image_intense", (getter) Style_get_uint8, (setter) Style_set_uint8, "image.intense", (void*)offsetof(lv_style_t, image.intense)},
   {"image_opa", (getter) Style_get_uint8, (setter) Style_set_uint8, "image.opa", (void*)offsetof(lv_style_t, image.opa)},
   {"line_color", (getter) Style_get_uint16, (setter) Style_set_uint16, "line.color", (void*)offsetof(lv_style_t, line.color)},
   {"line_width", (getter) Style_get_int16, (setter) Style_set_int16, "line.width", (void*)offsetof(lv_style_t, line.width)},
   {"line_opa", (getter) Style_get_uint8, (setter) Style_set_uint8, "line.opa", (void*)offsetof(lv_style_t, line.opa)},

    {"text_font", (getter) Style_get_font, (setter) Style_set_font, "text.font", NULL},
    {NULL},
};


static PyMethodDef Style_methods[] = {
    {"copy", (PyCFunction)Style_copy, METH_NOARGS, ""},
    {"data", (PyCFunction)Style_data, METH_NOARGS, ""},
    {NULL}  /* Sentinel */
};

static PyObject*
Style_repr(Style_Object *self) {
    return PyUnicode_FromFormat("<lvgl.Style object at %p referencing %p>", self, self->ref);
}

static PyTypeObject Style_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Style",
    .tp_basicsize = sizeof(Style_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_members = Style_members,
    .tp_methods = Style_methods,
    .tp_getset = Style_getsetters,
    //no .tp_new to prevent creation of new instaces from Python
    .tp_repr = (reprfunc) Style_repr,
};

static PyObject* 
Style_From_lv_style(lv_style_t *style) {
    Style_Object *ret;
    if (!style) {
        PyErr_SetString(PyExc_RuntimeError, "trying to create lvgl.Style from NULL pointer");
        return NULL;
    }
    
    ret = PyObject_New(Style_Object, &Style_Type);
    if (ret) ret->ref = style;
    return (PyObject *)ret;
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

lv_res_t pylv_obj_signal_callback(lv_obj_t* obj, lv_signal_t signal, void *param) {
    pylv_Obj *pyobj;
    PyObject *handler;
    PyObject *pyarg;
    lv_res_t result;
    lv_point_t point,drag_vect;
    PyObject *dragging;

    PyGILState_STATE gstate;

    gstate = PyGILState_Ensure();
    
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        result = pyobj->orig_c_signal_func(obj, signal, param);
        
        if (result == LV_RES_OK) {
            handler = pyobj->signal_func;
            if (handler) {
                switch(signal) {
                /* All signals that have an lv_indev_t * as argument */
                case LV_SIGNAL_PRESSED:
                case LV_SIGNAL_PRESSING:
                case LV_SIGNAL_PRESS_LOST:
                case LV_SIGNAL_RELEASED:
                case LV_SIGNAL_LONG_PRESS:
                case LV_SIGNAL_LONG_PRESS_REP:
                case LV_SIGNAL_DRAG_BEGIN:
                case LV_SIGNAL_DRAG_END:
                    lv_indev_get_point(param, &point);
                    dragging = lv_indev_is_dragging(param) ? Py_True : Py_False; // ref count is increased by Py_BuildValue "O"
                    lv_indev_get_vect(param, &drag_vect);
                    pyarg = Py_BuildValue("{s(ii)s(ii)sOs(ii)}", 
                        "screenpos", (int) point.x, (int) point.y, 
                        "objpos", (int) (point.x - obj->coords.x1), (int) (point.y - obj->coords.y1), 
                        "dragging", dragging, 
                        "dragvector", (int) drag_vect.x, (int) drag_vect.y);
                    
                    break;
                /* Not implemented (yet), arg = None*/
                case LV_SIGNAL_CLEANUP:
                case LV_SIGNAL_CHILD_CHG:
                case LV_SIGNAL_CORD_CHG:
                case LV_SIGNAL_STYLE_CHG:
                case LV_SIGNAL_REFR_EXT_SIZE:
                case LV_SIGNAL_GET_TYPE:                    
                case LV_SIGNAL_FOCUS:
                case LV_SIGNAL_DEFOCUS:
                case LV_SIGNAL_CONTROLL:
                default:
                    pyarg = Py_None;
                    Py_INCREF(pyarg);
                }
                
                // We are called from within lv_poll which will mean the lock is held
                // Release it since we are allowed to call lv_ functions now
                // (Would otherwise result in deadlock when lvgl calls are made from
                // with the signal handler)
                if (unlock) unlock(unlock_arg);
                PyObject_CallFunction(handler, "iO", (int) signal, pyarg);
                
                Py_DECREF(pyarg);
                if (PyErr_Occurred()) PyErr_Print();
                
                PyGILState_Release(gstate);
                if (lock) lock(lock_arg);

                return LV_RES_OK;

            }

        }
    }
    
    PyGILState_Release(gstate);
    return LV_RES_OK;
}


static PyObject*
pylv_obj_get_signal_func(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist)) return NULL;   
    
    PyObject *signal_func = self->signal_func;
    if (!signal_func) Py_RETURN_NONE;

    Py_INCREF(signal_func);
    return signal_func;
}

static PyObject *
pylv_obj_set_signal_func(pylv_Ddlist *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"signal_func", NULL};
    PyObject *signal_func, *tmp;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist , &signal_func)) return NULL;
    
    tmp = self->signal_func;
    if (signal_func == Py_None) {
        self->signal_func = NULL;
    } else {
        self->signal_func = signal_func;
        Py_INCREF(signal_func);
        
        LVGL_LOCK
        if (!self->orig_c_signal_func) {
            self->orig_c_signal_func = lv_obj_get_signal_func(self->ref);
        }
        lv_obj_set_signal_func(self->ref, pylv_obj_signal_callback);
        LVGL_UNLOCK
    }
    Py_XDECREF(tmp); // Old action (tmp) could be NULL

    Py_RETURN_NONE;
}


static PyObject*
pylv_label_get_letter_pos(pylv_Label *self, PyObject *args, PyObject *kwds)
{
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
pylv_btnm_set_map(pylv_Btnm *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"map", NULL};
    PyObject *map;
    const char **cmap;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist , &map)) return NULL;   
       
    cmap = buildstringmap(map);
    if (!cmap) return NULL;
    
    LVGL_LOCK
    lv_btnm_set_map(self->ref, cmap);
    LVGL_UNLOCK

    // Free the old map (if any) and store the new one to be able to free it later
    if (self->map) PyMem_Free(self->map);
    self->map = cmap;

    Py_RETURN_NONE;
    
}


static PyObject*
pylv_list_add(pylv_List *self, PyObject *args, PyObject *kwds)
{
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




lv_res_t pylv_btn_action_click_callback(lv_obj_t* obj) {
    pylv_Btn *pyobj;
    PyObject *handler;
    PyGILState_STATE gstate;

    gstate = PyGILState_Ensure();
    
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->actions[0];
        if (handler) {
            if (unlock) unlock(unlock_arg); 
            PyObject_CallFunctionObjArgs(handler, NULL);
            if (PyErr_Occurred()) PyErr_Print();
            
            PyGILState_Release(gstate);
            if (lock) lock(lock_arg); 
            return LV_RES_OK;

        }

    }
    PyGILState_Release(gstate);
    return LV_RES_OK;
}

lv_res_t pylv_btn_action_pr_callback(lv_obj_t* obj) {
    pylv_Btn *pyobj;
    PyObject *handler;
    PyGILState_STATE gstate;

    gstate = PyGILState_Ensure();
    
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->actions[1];
        if (handler) {
            if (unlock) unlock(unlock_arg); 
            PyObject_CallFunctionObjArgs(handler, NULL);
            if (PyErr_Occurred()) PyErr_Print();
            
            PyGILState_Release(gstate);
            if (lock) lock(lock_arg); 
            return LV_RES_OK;

        }

    }
    PyGILState_Release(gstate);
    return LV_RES_OK;
}

lv_res_t pylv_btn_action_long_pr_callback(lv_obj_t* obj) {
    pylv_Btn *pyobj;
    PyObject *handler;
    PyGILState_STATE gstate;

    gstate = PyGILState_Ensure();
    
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->actions[2];
        if (handler) {
            if (unlock) unlock(unlock_arg); 
            PyObject_CallFunctionObjArgs(handler, NULL);
            if (PyErr_Occurred()) PyErr_Print();
            
            PyGILState_Release(gstate);
            if (lock) lock(lock_arg); 
            return LV_RES_OK;

        }

    }
    PyGILState_Release(gstate);
    return LV_RES_OK;
}

lv_res_t pylv_btn_action_long_pr_repeat_callback(lv_obj_t* obj) {
    pylv_Btn *pyobj;
    PyObject *handler;
    PyGILState_STATE gstate;

    gstate = PyGILState_Ensure();
    
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->actions[3];
        if (handler) {
            if (unlock) unlock(unlock_arg); 
            PyObject_CallFunctionObjArgs(handler, NULL);
            if (PyErr_Occurred()) PyErr_Print();
            
            PyGILState_Release(gstate);
            if (lock) lock(lock_arg); 
            return LV_RES_OK;

        }

    }
    PyGILState_Release(gstate);
    return LV_RES_OK;
}
static lv_action_t pylv_btn_action_callbacks[LV_BTN_ACTION_NUM] = {pylv_btn_action_click_callback, pylv_btn_action_pr_callback, pylv_btn_action_long_pr_callback, pylv_btn_action_long_pr_repeat_callback};

static PyObject *
pylv_btn_get_action(pylv_Btn *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &type)) return NULL;   
    if (type<0 || type>= LV_BTN_ACTION_NUM) {
        return PyErr_Format(PyExc_ValueError, "action type should be 0<=type<%d, got %d", LV_BTN_ACTION_NUM, type);
    }
    PyObject *action = self->actions[type];
    if (!action) Py_RETURN_NONE;

    Py_INCREF(action);
    return action;
}

static PyObject *
pylv_btn_set_action(pylv_Btn *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "action", NULL};
    PyObject *action, *tmp;
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO", kwlist , &type, &action)) return NULL;
    if (type<0 || type>= LV_BTN_ACTION_NUM) {
        return PyErr_Format(PyExc_ValueError, "action type should be 0<=type<%d, got %d", LV_BTN_ACTION_NUM, type);
    }
        
    tmp = self->actions[type];
    if (action == Py_None) {
        lv_btn_set_action(self->ref, type, NULL);
        self->actions[type] = NULL;
    } else {
        self->actions[type] = action;
        Py_INCREF(action);
        lv_btn_set_action(self->ref, type, pylv_btn_action_callbacks[type]);
    }
    Py_XDECREF(tmp); // Old action (tmp) could be NULL

    Py_RETURN_NONE;
}


/****************************************************************
 * Methods and object definitions                               *
 ****************************************************************/


static void
pylv_obj_dealloc(pylv_Obj *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_obj_invalidate(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"parent", NULL};
    pylv_Obj * parent;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &pylv_obj_Type, &parent)) return NULL;

    LVGL_LOCK         
    lv_obj_set_parent(self->ref, parent->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_pos(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"base", "align", "x_mod", "y_mod", NULL};
    pylv_Obj * base;
    int align;
    short int x_mod;
    short int y_mod;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!ihh", kwlist , &pylv_obj_Type, &base, &align, &x_mod, &y_mod)) return NULL;

    LVGL_LOCK         
    lv_obj_align(self->ref, base->ref, align, x_mod, y_mod);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"style", NULL};
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &Style_Type, &style)) return NULL;

    LVGL_LOCK         
    lv_obj_set_style(self->ref, style->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_refresh_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_obj_set_drag(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_drag_throw(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_obj_set_drag_parent(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_opa_scale_enable(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"prot", NULL};
    unsigned char prot;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &prot)) return NULL;

    LVGL_LOCK         
    lv_obj_clear_protect(self->ref, prot);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_design_func(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_obj_refresh_ext_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_obj_refresh_ext_size(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_animate(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_obj_get_screen(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_obj_get_screen(self->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}

static PyObject*
pylv_obj_get_parent(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_obj_count_children(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_obj_get_coords(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_obj_get_x(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_obj_get_x(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_y(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_obj_get_y(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_obj_get_width(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_obj_get_height(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_ext_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_obj_get_ext_size(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    return Style_From_lv_style(lv_obj_get_style(self->ref));
}

static PyObject*
pylv_obj_get_hidden(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_obj_get_hidden(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_click(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_obj_get_click(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_top(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_obj_get_top(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_drag(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_obj_get_drag(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_drag_throw(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_obj_get_drag_throw(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_drag_parent(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_obj_get_drag_parent(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_opa_scale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_obj_get_opa_scale(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_get_protect(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_obj_get_protect(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_obj_is_protected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"prot", NULL};
    unsigned char prot;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &prot)) return NULL;

    LVGL_LOCK        
    int result = lv_obj_is_protected(self->ref, prot);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_design_func(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_obj_get_group(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_obj_is_focused(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_obj_is_focused(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_obj_methods[] = {
    {"invalidate", (PyCFunction) pylv_obj_invalidate, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_parent", (PyCFunction) pylv_obj_set_parent, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_pos", (PyCFunction) pylv_obj_set_pos, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_x", (PyCFunction) pylv_obj_set_x, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_y", (PyCFunction) pylv_obj_set_y, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_size", (PyCFunction) pylv_obj_set_size, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_width", (PyCFunction) pylv_obj_set_width, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_height", (PyCFunction) pylv_obj_set_height, METH_VARARGS | METH_KEYWORDS, NULL},
    {"align", (PyCFunction) pylv_obj_align, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_obj_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"refresh_style", (PyCFunction) pylv_obj_refresh_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_hidden", (PyCFunction) pylv_obj_set_hidden, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_click", (PyCFunction) pylv_obj_set_click, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_top", (PyCFunction) pylv_obj_set_top, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_drag", (PyCFunction) pylv_obj_set_drag, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_drag_throw", (PyCFunction) pylv_obj_set_drag_throw, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_drag_parent", (PyCFunction) pylv_obj_set_drag_parent, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_opa_scale_enable", (PyCFunction) pylv_obj_set_opa_scale_enable, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_opa_scale", (PyCFunction) pylv_obj_set_opa_scale, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_protect", (PyCFunction) pylv_obj_set_protect, METH_VARARGS | METH_KEYWORDS, NULL},
    {"clear_protect", (PyCFunction) pylv_obj_clear_protect, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_signal_func", (PyCFunction) pylv_obj_set_signal_func, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_design_func", (PyCFunction) pylv_obj_set_design_func, METH_VARARGS | METH_KEYWORDS, NULL},
    {"refresh_ext_size", (PyCFunction) pylv_obj_refresh_ext_size, METH_VARARGS | METH_KEYWORDS, NULL},
    {"animate", (PyCFunction) pylv_obj_animate, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_screen", (PyCFunction) pylv_obj_get_screen, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_parent", (PyCFunction) pylv_obj_get_parent, METH_VARARGS | METH_KEYWORDS, NULL},
    {"count_children", (PyCFunction) pylv_obj_count_children, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_coords", (PyCFunction) pylv_obj_get_coords, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_x", (PyCFunction) pylv_obj_get_x, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_y", (PyCFunction) pylv_obj_get_y, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_width", (PyCFunction) pylv_obj_get_width, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_height", (PyCFunction) pylv_obj_get_height, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_ext_size", (PyCFunction) pylv_obj_get_ext_size, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_obj_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_hidden", (PyCFunction) pylv_obj_get_hidden, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_click", (PyCFunction) pylv_obj_get_click, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_top", (PyCFunction) pylv_obj_get_top, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_drag", (PyCFunction) pylv_obj_get_drag, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_drag_throw", (PyCFunction) pylv_obj_get_drag_throw, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_drag_parent", (PyCFunction) pylv_obj_get_drag_parent, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_opa_scale", (PyCFunction) pylv_obj_get_opa_scale, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_protect", (PyCFunction) pylv_obj_get_protect, METH_VARARGS | METH_KEYWORDS, NULL},
    {"is_protected", (PyCFunction) pylv_obj_is_protected, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_signal_func", (PyCFunction) pylv_obj_get_signal_func, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_design_func", (PyCFunction) pylv_obj_get_design_func, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_type", (PyCFunction) pylv_obj_get_type, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_group", (PyCFunction) pylv_obj_get_group, METH_VARARGS | METH_KEYWORDS, NULL},
    {"is_focused", (PyCFunction) pylv_obj_is_focused, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_children", (PyCFunction) pylv_obj_get_children, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_win_dealloc(pylv_Win *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_win_clean(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_win_close_action(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_win_close_action(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_win_set_title(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"title", NULL};
    char * title;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &title)) return NULL;

    LVGL_LOCK         
    lv_win_set_title(self->ref, title);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_btn_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"layout", NULL};
    int layout;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &layout)) return NULL;

    LVGL_LOCK         
    lv_win_set_layout(self->ref, layout);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_sb_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"sb_mode", NULL};
    int sb_mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &sb_mode)) return NULL;

    LVGL_LOCK         
    lv_win_set_sb_mode(self->ref, sb_mode);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    LVGL_LOCK         
    lv_win_set_style(self->ref, type, style->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_get_title(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    char * result = lv_win_get_title(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_win_get_content(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_win_get_btn_size(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_win_get_from_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_win_get_layout(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_win_get_sb_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_win_get_sb_mode(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_win_get_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_win_get_width(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_win_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_win_get_style(self->ref, type));
}

static PyObject*
pylv_win_focus(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"dist", NULL};
    short int dist;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &dist)) return NULL;

    LVGL_LOCK         
    lv_win_scroll_ver(self->ref, dist);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}


static PyMethodDef pylv_win_methods[] = {
    {"clean", (PyCFunction) pylv_win_clean, METH_VARARGS | METH_KEYWORDS, NULL},
    {"add_btn", (PyCFunction) pylv_win_add_btn, METH_VARARGS | METH_KEYWORDS, NULL},
    {"close_action", (PyCFunction) pylv_win_close_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_title", (PyCFunction) pylv_win_set_title, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_btn_size", (PyCFunction) pylv_win_set_btn_size, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_layout", (PyCFunction) pylv_win_set_layout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_sb_mode", (PyCFunction) pylv_win_set_sb_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_win_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_title", (PyCFunction) pylv_win_get_title, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_content", (PyCFunction) pylv_win_get_content, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_btn_size", (PyCFunction) pylv_win_get_btn_size, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_from_btn", (PyCFunction) pylv_win_get_from_btn, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_layout", (PyCFunction) pylv_win_get_layout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_sb_mode", (PyCFunction) pylv_win_get_sb_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_width", (PyCFunction) pylv_win_get_width, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_win_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"focus", (PyCFunction) pylv_win_focus, METH_VARARGS | METH_KEYWORDS, NULL},
    {"scroll_hor", (PyCFunction) pylv_win_scroll_hor, METH_VARARGS | METH_KEYWORDS, NULL},
    {"scroll_ver", (PyCFunction) pylv_win_scroll_ver, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_label_dealloc(pylv_Label *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_label_set_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"text", NULL};
    char * text;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &text)) return NULL;

    LVGL_LOCK         
    lv_label_set_text(self->ref, text);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_array_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"array", "size", NULL};
    char * array;
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
    static char *kwlist[] = {"text", NULL};
    char * text;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &text)) return NULL;

    LVGL_LOCK         
    lv_label_set_static_text(self->ref, text);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_long_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"long_mode", NULL};
    int long_mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &long_mode)) return NULL;

    LVGL_LOCK         
    lv_label_set_long_mode(self->ref, long_mode);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"align", NULL};
    int align;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &align)) return NULL;

    LVGL_LOCK         
    lv_label_set_align(self->ref, align);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"recolor_en", NULL};
    int recolor_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &recolor_en)) return NULL;

    LVGL_LOCK         
    lv_label_set_recolor(self->ref, recolor_en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_body_draw(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"body_en", NULL};
    int body_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &body_en)) return NULL;

    LVGL_LOCK         
    lv_label_set_body_draw(self->ref, body_en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_anim_speed(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"anim_speed", NULL};
    unsigned short int anim_speed;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_speed)) return NULL;

    LVGL_LOCK         
    lv_label_set_anim_speed(self->ref, anim_speed);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_get_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    char * result = lv_label_get_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_label_get_long_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_label_get_long_mode(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_label_get_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_label_get_align(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_label_get_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_label_get_recolor(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_label_get_body_draw(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_label_get_body_draw(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_label_get_anim_speed(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_label_get_anim_speed(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_label_ins_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"pos", "txt", NULL};
    unsigned int pos;
    char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Is", kwlist , &pos, &txt)) return NULL;

    LVGL_LOCK         
    lv_label_ins_text(self->ref, pos, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_cut_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    {"set_text", (PyCFunction) pylv_label_set_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_array_text", (PyCFunction) pylv_label_set_array_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_static_text", (PyCFunction) pylv_label_set_static_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_long_mode", (PyCFunction) pylv_label_set_long_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_align", (PyCFunction) pylv_label_set_align, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_recolor", (PyCFunction) pylv_label_set_recolor, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_body_draw", (PyCFunction) pylv_label_set_body_draw, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_anim_speed", (PyCFunction) pylv_label_set_anim_speed, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_text", (PyCFunction) pylv_label_get_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_long_mode", (PyCFunction) pylv_label_get_long_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_align", (PyCFunction) pylv_label_get_align, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_recolor", (PyCFunction) pylv_label_get_recolor, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_body_draw", (PyCFunction) pylv_label_get_body_draw, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_anim_speed", (PyCFunction) pylv_label_get_anim_speed, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_letter_pos", (PyCFunction) pylv_label_get_letter_pos, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_letter_on", (PyCFunction) pylv_label_get_letter_on, METH_VARARGS | METH_KEYWORDS, NULL},
    {"ins_text", (PyCFunction) pylv_label_ins_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"cut_text", (PyCFunction) pylv_label_cut_text, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_lmeter_dealloc(pylv_Lmeter *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_lmeter_set_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_lmeter_get_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_lmeter_get_min_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_lmeter_get_min_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_lmeter_get_max_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_lmeter_get_max_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_lmeter_get_line_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_lmeter_get_line_count(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_lmeter_get_scale_angle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_lmeter_get_scale_angle(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}


static PyMethodDef pylv_lmeter_methods[] = {
    {"set_value", (PyCFunction) pylv_lmeter_set_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_range", (PyCFunction) pylv_lmeter_set_range, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_scale", (PyCFunction) pylv_lmeter_set_scale, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_value", (PyCFunction) pylv_lmeter_get_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_min_value", (PyCFunction) pylv_lmeter_get_min_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_max_value", (PyCFunction) pylv_lmeter_get_max_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_line_count", (PyCFunction) pylv_lmeter_get_line_count, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_scale_angle", (PyCFunction) pylv_lmeter_get_scale_angle, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_btnm_dealloc(pylv_Btnm *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_btnm_set_action(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_btnm_set_toggle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", "id", NULL};
    int en;
    unsigned short int id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "pH", kwlist , &en, &id)) return NULL;

    LVGL_LOCK         
    lv_btnm_set_toggle(self->ref, en, id);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    LVGL_LOCK         
    lv_btnm_set_style(self->ref, type, style->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_get_map(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_btnm_get_action(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_btnm_get_toggled(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_btnm_get_toggled(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_btnm_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_btnm_get_style(self->ref, type));
}


static PyMethodDef pylv_btnm_methods[] = {
    {"set_map", (PyCFunction) pylv_btnm_set_map, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_action", (PyCFunction) pylv_btnm_set_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_toggle", (PyCFunction) pylv_btnm_set_toggle, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_btnm_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_map", (PyCFunction) pylv_btnm_get_map, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_action", (PyCFunction) pylv_btnm_get_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_toggled", (PyCFunction) pylv_btnm_get_toggled, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_btnm_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_chart_dealloc(pylv_Chart *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_chart_add_series(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_chart_set_div_line_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;

    LVGL_LOCK         
    lv_chart_set_type(self->ref, type);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_point_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_chart_set_points(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_chart_set_next(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_chart_get_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_chart_get_type(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_chart_get_point_cnt(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_chart_get_point_cnt(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_chart_get_series_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_chart_get_series_opa(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_chart_get_series_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_chart_get_series_width(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_chart_get_series_darking(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_chart_get_series_darking(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_chart_refresh(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_chart_refresh(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}


static PyMethodDef pylv_chart_methods[] = {
    {"add_series", (PyCFunction) pylv_chart_add_series, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_div_line_count", (PyCFunction) pylv_chart_set_div_line_count, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_range", (PyCFunction) pylv_chart_set_range, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_type", (PyCFunction) pylv_chart_set_type, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_point_count", (PyCFunction) pylv_chart_set_point_count, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_series_opa", (PyCFunction) pylv_chart_set_series_opa, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_series_width", (PyCFunction) pylv_chart_set_series_width, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_series_darking", (PyCFunction) pylv_chart_set_series_darking, METH_VARARGS | METH_KEYWORDS, NULL},
    {"init_points", (PyCFunction) pylv_chart_init_points, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_points", (PyCFunction) pylv_chart_set_points, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_next", (PyCFunction) pylv_chart_set_next, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_type", (PyCFunction) pylv_chart_get_type, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_point_cnt", (PyCFunction) pylv_chart_get_point_cnt, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_series_opa", (PyCFunction) pylv_chart_get_series_opa, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_series_width", (PyCFunction) pylv_chart_get_series_width, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_series_darking", (PyCFunction) pylv_chart_get_series_darking, METH_VARARGS | METH_KEYWORDS, NULL},
    {"refresh", (PyCFunction) pylv_chart_refresh, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_cont_dealloc(pylv_Cont *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_cont_set_layout(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"layout", NULL};
    int layout;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &layout)) return NULL;

    LVGL_LOCK         
    lv_cont_set_layout(self->ref, layout);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_cont_set_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"hor_en", "ver_en", NULL};
    int hor_en;
    int ver_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "pp", kwlist , &hor_en, &ver_en)) return NULL;

    LVGL_LOCK         
    lv_cont_set_fit(self->ref, hor_en, ver_en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_cont_get_layout(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_cont_get_layout(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_cont_get_hor_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_cont_get_hor_fit(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_cont_get_ver_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_cont_get_ver_fit(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_cont_get_fit_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_cont_get_fit_width(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_cont_get_fit_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_cont_get_fit_height(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}


static PyMethodDef pylv_cont_methods[] = {
    {"set_layout", (PyCFunction) pylv_cont_set_layout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_fit", (PyCFunction) pylv_cont_set_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_layout", (PyCFunction) pylv_cont_get_layout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_hor_fit", (PyCFunction) pylv_cont_get_hor_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_ver_fit", (PyCFunction) pylv_cont_get_ver_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_fit_width", (PyCFunction) pylv_cont_get_fit_width, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_fit_height", (PyCFunction) pylv_cont_get_fit_height, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_led_dealloc(pylv_Led *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_led_set_bright(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_led_get_bright(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}


static PyMethodDef pylv_led_methods[] = {
    {"set_bright", (PyCFunction) pylv_led_set_bright, METH_VARARGS | METH_KEYWORDS, NULL},
    {"on", (PyCFunction) pylv_led_on, METH_VARARGS | METH_KEYWORDS, NULL},
    {"off", (PyCFunction) pylv_led_off, METH_VARARGS | METH_KEYWORDS, NULL},
    {"toggle", (PyCFunction) pylv_led_toggle, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_bright", (PyCFunction) pylv_led_get_bright, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_kb_dealloc(pylv_Kb *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


lv_res_t pylv_kb_ok_action_callback(lv_obj_t* obj) {
    pylv_Kb *pyobj;
    PyObject *handler;
    PyGILState_STATE gstate;

    gstate = PyGILState_Ensure();
    
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->ok_action;
        if (handler) {
            if (unlock) unlock(unlock_arg); 
            PyObject_CallFunctionObjArgs(handler, NULL);
            if (PyErr_Occurred()) PyErr_Print();
            
            PyGILState_Release(gstate);
            if (lock) lock(lock_arg); 
            return LV_RES_OK;

        }

    }
    PyGILState_Release(gstate);
    return LV_RES_OK;
}


static PyObject *
pylv_kb_get_ok_action(pylv_Kb *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist)) return NULL;   
    
    PyObject *action = self->ok_action;
    if (!action) Py_RETURN_NONE;

    Py_INCREF(action);
    return action;
}

static PyObject *
pylv_kb_set_ok_action(pylv_Kb *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"action", NULL};
    PyObject *action, *tmp;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist , &action)) return NULL;
    
    tmp = self->ok_action;
    if (action == Py_None) {
        self->ok_action = NULL;
    } else {
        self->ok_action = action;
        Py_INCREF(action);
        lv_kb_set_ok_action(self->ref, pylv_kb_ok_action_callback);
    }
    Py_XDECREF(tmp); // Old action (tmp) could be NULL

    Py_RETURN_NONE;
}

            

lv_res_t pylv_kb_hide_action_callback(lv_obj_t* obj) {
    pylv_Kb *pyobj;
    PyObject *handler;
    PyGILState_STATE gstate;

    gstate = PyGILState_Ensure();
    
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->hide_action;
        if (handler) {
            if (unlock) unlock(unlock_arg); 
            PyObject_CallFunctionObjArgs(handler, NULL);
            if (PyErr_Occurred()) PyErr_Print();
            
            PyGILState_Release(gstate);
            if (lock) lock(lock_arg); 
            return LV_RES_OK;

        }

    }
    PyGILState_Release(gstate);
    return LV_RES_OK;
}


static PyObject *
pylv_kb_get_hide_action(pylv_Kb *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist)) return NULL;   
    
    PyObject *action = self->hide_action;
    if (!action) Py_RETURN_NONE;

    Py_INCREF(action);
    return action;
}

static PyObject *
pylv_kb_set_hide_action(pylv_Kb *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"action", NULL};
    PyObject *action, *tmp;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist , &action)) return NULL;
    
    tmp = self->hide_action;
    if (action == Py_None) {
        self->hide_action = NULL;
    } else {
        self->hide_action = action;
        Py_INCREF(action);
        lv_kb_set_hide_action(self->ref, pylv_kb_hide_action_callback);
    }
    Py_XDECREF(tmp); // Old action (tmp) could be NULL

    Py_RETURN_NONE;
}

            

static PyObject*
pylv_kb_set_ta(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"mode", NULL};
    int mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &mode)) return NULL;

    LVGL_LOCK         
    lv_kb_set_mode(self->ref, mode);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_kb_set_cursor_manage(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    LVGL_LOCK         
    lv_kb_set_style(self->ref, type, style->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_kb_get_ta(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_kb_get_mode(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_kb_get_cursor_manage(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_kb_get_cursor_manage(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_kb_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_kb_get_style(self->ref, type));
}


static PyMethodDef pylv_kb_methods[] = {
    {"set_ta", (PyCFunction) pylv_kb_set_ta, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_mode", (PyCFunction) pylv_kb_set_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_cursor_manage", (PyCFunction) pylv_kb_set_cursor_manage, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_ok_action", (PyCFunction) pylv_kb_set_ok_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_hide_action", (PyCFunction) pylv_kb_set_hide_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_kb_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_ta", (PyCFunction) pylv_kb_get_ta, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_mode", (PyCFunction) pylv_kb_get_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_cursor_manage", (PyCFunction) pylv_kb_get_cursor_manage, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_ok_action", (PyCFunction) pylv_kb_get_ok_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_hide_action", (PyCFunction) pylv_kb_get_hide_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_kb_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_img_dealloc(pylv_Img *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_img_set_src(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_img_set_file(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"fn", NULL};
    char * fn;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &fn)) return NULL;

    LVGL_LOCK         
    lv_img_set_file(self->ref, fn);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_img_set_auto_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"autosize_en", NULL};
    int autosize_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &autosize_en)) return NULL;

    LVGL_LOCK         
    lv_img_set_auto_size(self->ref, autosize_en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_img_set_upscale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_img_get_file_name(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    char * result = lv_img_get_file_name(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_img_get_auto_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_img_get_auto_size(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_img_get_upscale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_img_get_upscale(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_img_methods[] = {
    {"set_src", (PyCFunction) pylv_img_set_src, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_file", (PyCFunction) pylv_img_set_file, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_auto_size", (PyCFunction) pylv_img_set_auto_size, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_upscale", (PyCFunction) pylv_img_set_upscale, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_src", (PyCFunction) pylv_img_get_src, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_file_name", (PyCFunction) pylv_img_get_file_name, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_auto_size", (PyCFunction) pylv_img_get_auto_size, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_upscale", (PyCFunction) pylv_img_get_upscale, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_bar_dealloc(pylv_Bar *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_bar_set_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"value", NULL};
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &value)) return NULL;

    LVGL_LOCK         
    lv_bar_set_value(self->ref, value);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_bar_set_value_anim(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"value", "anim_time", NULL};
    short int value;
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hH", kwlist , &value, &anim_time)) return NULL;

    LVGL_LOCK         
    lv_bar_set_value_anim(self->ref, value, anim_time);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_bar_set_range(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
pylv_bar_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    LVGL_LOCK         
    lv_bar_set_style(self->ref, type, style->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_bar_get_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_bar_get_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_bar_get_min_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_bar_get_min_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_bar_get_max_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_bar_get_max_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_bar_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_bar_get_style(self->ref, type));
}


static PyMethodDef pylv_bar_methods[] = {
    {"set_value", (PyCFunction) pylv_bar_set_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_value_anim", (PyCFunction) pylv_bar_set_value_anim, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_range", (PyCFunction) pylv_bar_set_range, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_bar_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_value", (PyCFunction) pylv_bar_get_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_min_value", (PyCFunction) pylv_bar_get_min_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_max_value", (PyCFunction) pylv_bar_get_max_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_bar_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_arc_dealloc(pylv_Arc *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_arc_set_angles(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    LVGL_LOCK         
    lv_arc_set_style(self->ref, type, style->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_arc_get_angle_start(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_arc_get_angle_start(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_arc_get_angle_end(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_arc_get_angle_end(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_arc_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_arc_get_style(self->ref, type));
}


static PyMethodDef pylv_arc_methods[] = {
    {"set_angles", (PyCFunction) pylv_arc_set_angles, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_arc_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_angle_start", (PyCFunction) pylv_arc_get_angle_start, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_angle_end", (PyCFunction) pylv_arc_get_angle_end, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_arc_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_line_dealloc(pylv_Line *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_line_set_points(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_line_set_auto_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"autosize_en", NULL};
    int autosize_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &autosize_en)) return NULL;

    LVGL_LOCK         
    lv_line_set_auto_size(self->ref, autosize_en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_line_set_y_invert(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"yinv_en", NULL};
    int yinv_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &yinv_en)) return NULL;

    LVGL_LOCK         
    lv_line_set_y_invert(self->ref, yinv_en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_line_set_upscale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_line_get_auto_size(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_line_get_y_inv(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_line_get_y_inv(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_line_get_upscale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_line_get_upscale(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_line_methods[] = {
    {"set_points", (PyCFunction) pylv_line_set_points, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_auto_size", (PyCFunction) pylv_line_set_auto_size, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_y_invert", (PyCFunction) pylv_line_set_y_invert, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_upscale", (PyCFunction) pylv_line_set_upscale, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_auto_size", (PyCFunction) pylv_line_get_auto_size, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_y_inv", (PyCFunction) pylv_line_get_y_inv, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_upscale", (PyCFunction) pylv_line_get_upscale, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_tabview_dealloc(pylv_Tabview *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_tabview_clean(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"name", NULL};
    char * name;
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
pylv_tabview_set_tab_load_action(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_tabview_set_sliding(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    LVGL_LOCK         
    lv_tabview_set_style(self->ref, type, style->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_tabview_set_btns_pos(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"btns_pos", NULL};
    int btns_pos;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &btns_pos)) return NULL;

    LVGL_LOCK         
    lv_tabview_set_btns_pos(self->ref, btns_pos);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_tabview_get_tab_act(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_tabview_get_tab_act(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_tabview_get_tab_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_tabview_get_tab_count(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_tabview_get_tab(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
pylv_tabview_get_tab_load_action(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_tabview_get_sliding(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_tabview_get_sliding(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_tabview_get_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_tabview_get_anim_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_tabview_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_tabview_get_style(self->ref, type));
}

static PyObject*
pylv_tabview_get_btns_pos(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_tabview_get_btns_pos(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("i", result);
}


static PyMethodDef pylv_tabview_methods[] = {
    {"clean", (PyCFunction) pylv_tabview_clean, METH_VARARGS | METH_KEYWORDS, NULL},
    {"add_tab", (PyCFunction) pylv_tabview_add_tab, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_tab_act", (PyCFunction) pylv_tabview_set_tab_act, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_tab_load_action", (PyCFunction) pylv_tabview_set_tab_load_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_sliding", (PyCFunction) pylv_tabview_set_sliding, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_anim_time", (PyCFunction) pylv_tabview_set_anim_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_tabview_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_btns_pos", (PyCFunction) pylv_tabview_set_btns_pos, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_tab_act", (PyCFunction) pylv_tabview_get_tab_act, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_tab_count", (PyCFunction) pylv_tabview_get_tab_count, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_tab", (PyCFunction) pylv_tabview_get_tab, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_tab_load_action", (PyCFunction) pylv_tabview_get_tab_load_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_sliding", (PyCFunction) pylv_tabview_get_sliding, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_anim_time", (PyCFunction) pylv_tabview_get_anim_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_tabview_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_btns_pos", (PyCFunction) pylv_tabview_get_btns_pos, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_mbox_dealloc(pylv_Mbox *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_mbox_add_btns(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_mbox_set_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"txt", NULL};
    char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    LVGL_LOCK         
    lv_mbox_set_text(self->ref, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_mbox_set_action(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_mbox_set_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    LVGL_LOCK         
    lv_mbox_set_style(self->ref, type, style->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_mbox_get_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    char * result = lv_mbox_get_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_mbox_get_from_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK
    lv_obj_t *result = lv_mbox_get_from_btn(self->ref);
    LVGL_UNLOCK
    PyObject *retobj = pyobj_from_lv(result);
    
    return retobj;
}

static PyObject*
pylv_mbox_get_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_mbox_get_anim_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_mbox_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_mbox_get_style(self->ref, type));
}


static PyMethodDef pylv_mbox_methods[] = {
    {"add_btns", (PyCFunction) pylv_mbox_add_btns, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_text", (PyCFunction) pylv_mbox_set_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_action", (PyCFunction) pylv_mbox_set_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_anim_time", (PyCFunction) pylv_mbox_set_anim_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"start_auto_close", (PyCFunction) pylv_mbox_start_auto_close, METH_VARARGS | METH_KEYWORDS, NULL},
    {"stop_auto_close", (PyCFunction) pylv_mbox_stop_auto_close, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_mbox_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_text", (PyCFunction) pylv_mbox_get_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_from_btn", (PyCFunction) pylv_mbox_get_from_btn, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_anim_time", (PyCFunction) pylv_mbox_get_anim_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_mbox_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_gauge_dealloc(pylv_Gauge *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_gauge_set_needle_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_gauge_set_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"needle", NULL};
    unsigned char needle;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &needle)) return NULL;

    LVGL_LOCK        
    short int result = lv_gauge_get_value(self->ref, needle);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_gauge_get_needle_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_gauge_get_needle_count(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_gauge_get_critical_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_gauge_get_critical_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_gauge_get_label_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_gauge_get_label_count(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}


static PyMethodDef pylv_gauge_methods[] = {
    {"set_needle_count", (PyCFunction) pylv_gauge_set_needle_count, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_value", (PyCFunction) pylv_gauge_set_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_critical_value", (PyCFunction) pylv_gauge_set_critical_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_scale", (PyCFunction) pylv_gauge_set_scale, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_value", (PyCFunction) pylv_gauge_get_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_needle_count", (PyCFunction) pylv_gauge_get_needle_count, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_critical_value", (PyCFunction) pylv_gauge_get_critical_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_label_count", (PyCFunction) pylv_gauge_get_label_count, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_page_dealloc(pylv_Page *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


lv_res_t pylv_page_rel_action_callback(lv_obj_t* obj) {
    pylv_Page *pyobj;
    PyObject *handler;
    PyGILState_STATE gstate;

    gstate = PyGILState_Ensure();
    
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->rel_action;
        if (handler) {
            if (unlock) unlock(unlock_arg); 
            PyObject_CallFunctionObjArgs(handler, NULL);
            if (PyErr_Occurred()) PyErr_Print();
            
            PyGILState_Release(gstate);
            if (lock) lock(lock_arg); 
            return LV_RES_OK;

        }

    }
    PyGILState_Release(gstate);
    return LV_RES_OK;
}


static PyObject *
pylv_page_get_rel_action(pylv_Page *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist)) return NULL;   
    
    PyObject *action = self->rel_action;
    if (!action) Py_RETURN_NONE;

    Py_INCREF(action);
    return action;
}

static PyObject *
pylv_page_set_rel_action(pylv_Page *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"action", NULL};
    PyObject *action, *tmp;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist , &action)) return NULL;
    
    tmp = self->rel_action;
    if (action == Py_None) {
        self->rel_action = NULL;
    } else {
        self->rel_action = action;
        Py_INCREF(action);
        lv_page_set_rel_action(self->ref, pylv_page_rel_action_callback);
    }
    Py_XDECREF(tmp); // Old action (tmp) could be NULL

    Py_RETURN_NONE;
}

            

lv_res_t pylv_page_pr_action_callback(lv_obj_t* obj) {
    pylv_Page *pyobj;
    PyObject *handler;
    PyGILState_STATE gstate;

    gstate = PyGILState_Ensure();
    
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->pr_action;
        if (handler) {
            if (unlock) unlock(unlock_arg); 
            PyObject_CallFunctionObjArgs(handler, NULL);
            if (PyErr_Occurred()) PyErr_Print();
            
            PyGILState_Release(gstate);
            if (lock) lock(lock_arg); 
            return LV_RES_OK;

        }

    }
    PyGILState_Release(gstate);
    return LV_RES_OK;
}


static PyObject *
pylv_page_get_pr_action(pylv_Page *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist)) return NULL;   
    
    PyObject *action = self->pr_action;
    if (!action) Py_RETURN_NONE;

    Py_INCREF(action);
    return action;
}

static PyObject *
pylv_page_set_pr_action(pylv_Page *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"action", NULL};
    PyObject *action, *tmp;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist , &action)) return NULL;
    
    tmp = self->pr_action;
    if (action == Py_None) {
        self->pr_action = NULL;
    } else {
        self->pr_action = action;
        Py_INCREF(action);
        lv_page_set_pr_action(self->ref, pylv_page_pr_action_callback);
    }
    Py_XDECREF(tmp); // Old action (tmp) could be NULL

    Py_RETURN_NONE;
}

            

static PyObject*
pylv_page_clean(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"sb_mode", NULL};
    int sb_mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &sb_mode)) return NULL;

    LVGL_LOCK         
    lv_page_set_sb_mode(self->ref, sb_mode);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_arrow_scroll(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_page_set_arrow_scroll(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scrl_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"hor_en", "ver_en", NULL};
    int hor_en;
    int ver_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "pp", kwlist , &hor_en, &ver_en)) return NULL;

    LVGL_LOCK         
    lv_page_set_scrl_fit(self->ref, hor_en, ver_en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scrl_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"layout", NULL};
    int layout;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &layout)) return NULL;

    LVGL_LOCK         
    lv_page_set_scrl_layout(self->ref, layout);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    LVGL_LOCK         
    lv_page_set_style(self->ref, type, style->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_get_sb_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_page_get_sb_mode(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_page_get_arrow_scroll(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_page_get_arrow_scroll(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_page_get_fit_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_page_get_fit_width(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_page_get_fit_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_page_get_fit_height(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_page_get_scrl_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_page_get_scrl_width(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_page_get_scrl_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_page_get_scrl_height(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_page_get_scrl_layout(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_page_get_scrl_layout(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_page_get_scrl_hor_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_page_get_scrl_hor_fit(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_page_get_scrl_fit_ver(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_page_get_scrl_fit_ver(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_page_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_page_get_style(self->ref, type));
}

static PyObject*
pylv_page_glue_obj(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"dist", NULL};
    short int dist;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &dist)) return NULL;

    LVGL_LOCK         
    lv_page_scroll_ver(self->ref, dist);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}


static PyMethodDef pylv_page_methods[] = {
    {"clean", (PyCFunction) pylv_page_clean, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_pr_action", (PyCFunction) pylv_page_get_pr_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_rel_action", (PyCFunction) pylv_page_get_rel_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_scrl", (PyCFunction) pylv_page_get_scrl, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_rel_action", (PyCFunction) pylv_page_set_rel_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_pr_action", (PyCFunction) pylv_page_set_pr_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_sb_mode", (PyCFunction) pylv_page_set_sb_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_arrow_scroll", (PyCFunction) pylv_page_set_arrow_scroll, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_scrl_fit", (PyCFunction) pylv_page_set_scrl_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_scrl_width", (PyCFunction) pylv_page_set_scrl_width, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_scrl_height", (PyCFunction) pylv_page_set_scrl_height, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_scrl_layout", (PyCFunction) pylv_page_set_scrl_layout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_page_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_sb_mode", (PyCFunction) pylv_page_get_sb_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_arrow_scroll", (PyCFunction) pylv_page_get_arrow_scroll, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_fit_width", (PyCFunction) pylv_page_get_fit_width, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_fit_height", (PyCFunction) pylv_page_get_fit_height, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_scrl_width", (PyCFunction) pylv_page_get_scrl_width, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_scrl_height", (PyCFunction) pylv_page_get_scrl_height, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_scrl_layout", (PyCFunction) pylv_page_get_scrl_layout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_scrl_hor_fit", (PyCFunction) pylv_page_get_scrl_hor_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_scrl_fit_ver", (PyCFunction) pylv_page_get_scrl_fit_ver, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_page_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"glue_obj", (PyCFunction) pylv_page_glue_obj, METH_VARARGS | METH_KEYWORDS, NULL},
    {"focus", (PyCFunction) pylv_page_focus, METH_VARARGS | METH_KEYWORDS, NULL},
    {"scroll_hor", (PyCFunction) pylv_page_scroll_hor, METH_VARARGS | METH_KEYWORDS, NULL},
    {"scroll_ver", (PyCFunction) pylv_page_scroll_ver, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_ta_dealloc(pylv_Ta *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


lv_res_t pylv_ta_action_callback(lv_obj_t* obj) {
    pylv_Ta *pyobj;
    PyObject *handler;
    PyGILState_STATE gstate;

    gstate = PyGILState_Ensure();
    
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->action;
        if (handler) {
            if (unlock) unlock(unlock_arg); 
            PyObject_CallFunctionObjArgs(handler, NULL);
            if (PyErr_Occurred()) PyErr_Print();
            
            PyGILState_Release(gstate);
            if (lock) lock(lock_arg); 
            return LV_RES_OK;

        }

    }
    PyGILState_Release(gstate);
    return LV_RES_OK;
}


static PyObject *
pylv_ta_get_action(pylv_Ta *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist)) return NULL;   
    
    PyObject *action = self->action;
    if (!action) Py_RETURN_NONE;

    Py_INCREF(action);
    return action;
}

static PyObject *
pylv_ta_set_action(pylv_Ta *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"action", NULL};
    PyObject *action, *tmp;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist , &action)) return NULL;
    
    tmp = self->action;
    if (action == Py_None) {
        self->action = NULL;
    } else {
        self->action = action;
        Py_INCREF(action);
        lv_ta_set_action(self->ref, pylv_ta_action_callback);
    }
    Py_XDECREF(tmp); // Old action (tmp) could be NULL

    Py_RETURN_NONE;
}

            

static PyObject*
pylv_ta_add_char(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"txt", NULL};
    char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    LVGL_LOCK         
    lv_ta_add_text(self->ref, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_del_char(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_ta_del_char(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"txt", NULL};
    char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    LVGL_LOCK         
    lv_ta_set_text(self->ref, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_cursor_pos(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"cur_type", NULL};
    int cur_type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &cur_type)) return NULL;

    LVGL_LOCK         
    lv_ta_set_cursor_type(self->ref, cur_type);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_pwd_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"pwd_en", NULL};
    int pwd_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &pwd_en)) return NULL;

    LVGL_LOCK         
    lv_ta_set_pwd_mode(self->ref, pwd_en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_one_line(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"align", NULL};
    int align;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &align)) return NULL;

    LVGL_LOCK         
    lv_ta_set_text_align(self->ref, align);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_accepted_chars(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"list", NULL};
    char * list;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &list)) return NULL;

    LVGL_LOCK         
    lv_ta_set_accepted_chars(self->ref, list);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_max_length(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"num", NULL};
    unsigned short int num;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &num)) return NULL;

    LVGL_LOCK         
    lv_ta_set_max_length(self->ref, num);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    LVGL_LOCK         
    lv_ta_set_style(self->ref, type, style->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_get_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    char * result = lv_ta_get_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_ta_get_label(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_ta_get_cursor_pos(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_ta_get_cursor_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_ta_get_cursor_type(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_ta_get_pwd_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_ta_get_pwd_mode(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_ta_get_one_line(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_ta_get_one_line(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_ta_get_accepted_chars(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    char * result = lv_ta_get_accepted_chars(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_ta_get_max_length(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_ta_get_max_length(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_ta_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_ta_get_style(self->ref, type));
}

static PyObject*
pylv_ta_cursor_right(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_ta_cursor_up(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}


static PyMethodDef pylv_ta_methods[] = {
    {"add_char", (PyCFunction) pylv_ta_add_char, METH_VARARGS | METH_KEYWORDS, NULL},
    {"add_text", (PyCFunction) pylv_ta_add_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"del_char", (PyCFunction) pylv_ta_del_char, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_text", (PyCFunction) pylv_ta_set_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_cursor_pos", (PyCFunction) pylv_ta_set_cursor_pos, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_cursor_type", (PyCFunction) pylv_ta_set_cursor_type, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_pwd_mode", (PyCFunction) pylv_ta_set_pwd_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_one_line", (PyCFunction) pylv_ta_set_one_line, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_text_align", (PyCFunction) pylv_ta_set_text_align, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_accepted_chars", (PyCFunction) pylv_ta_set_accepted_chars, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_max_length", (PyCFunction) pylv_ta_set_max_length, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_action", (PyCFunction) pylv_ta_set_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_ta_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_text", (PyCFunction) pylv_ta_get_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_label", (PyCFunction) pylv_ta_get_label, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_cursor_pos", (PyCFunction) pylv_ta_get_cursor_pos, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_cursor_type", (PyCFunction) pylv_ta_get_cursor_type, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_pwd_mode", (PyCFunction) pylv_ta_get_pwd_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_one_line", (PyCFunction) pylv_ta_get_one_line, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_accepted_chars", (PyCFunction) pylv_ta_get_accepted_chars, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_max_length", (PyCFunction) pylv_ta_get_max_length, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_action", (PyCFunction) pylv_ta_get_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_ta_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"cursor_right", (PyCFunction) pylv_ta_cursor_right, METH_VARARGS | METH_KEYWORDS, NULL},
    {"cursor_left", (PyCFunction) pylv_ta_cursor_left, METH_VARARGS | METH_KEYWORDS, NULL},
    {"cursor_down", (PyCFunction) pylv_ta_cursor_down, METH_VARARGS | METH_KEYWORDS, NULL},
    {"cursor_up", (PyCFunction) pylv_ta_cursor_up, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_btn_dealloc(pylv_Btn *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_btn_set_toggle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"state", NULL};
    int state;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &state)) return NULL;

    LVGL_LOCK         
    lv_btn_set_state(self->ref, state);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btn_toggle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    LVGL_LOCK         
    lv_btn_set_style(self->ref, type, style->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btn_get_state(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_btn_get_state(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_btn_get_toggle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_btn_get_toggle(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_btn_get_ink_in_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_btn_get_ink_in_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_btn_get_ink_wait_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_btn_get_ink_wait_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_btn_get_ink_out_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_btn_get_ink_out_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_btn_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_btn_get_style(self->ref, type));
}


static PyMethodDef pylv_btn_methods[] = {
    {"set_toggle", (PyCFunction) pylv_btn_set_toggle, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_state", (PyCFunction) pylv_btn_set_state, METH_VARARGS | METH_KEYWORDS, NULL},
    {"toggle", (PyCFunction) pylv_btn_toggle, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_action", (PyCFunction) pylv_btn_set_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_ink_in_time", (PyCFunction) pylv_btn_set_ink_in_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_ink_wait_time", (PyCFunction) pylv_btn_set_ink_wait_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_ink_out_time", (PyCFunction) pylv_btn_set_ink_out_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_btn_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_state", (PyCFunction) pylv_btn_get_state, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_toggle", (PyCFunction) pylv_btn_get_toggle, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_action", (PyCFunction) pylv_btn_get_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_ink_in_time", (PyCFunction) pylv_btn_get_ink_in_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_ink_wait_time", (PyCFunction) pylv_btn_get_ink_wait_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_ink_out_time", (PyCFunction) pylv_btn_get_ink_out_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_btn_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_ddlist_dealloc(pylv_Ddlist *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


lv_res_t pylv_ddlist_action_callback(lv_obj_t* obj) {
    pylv_Ddlist *pyobj;
    PyObject *handler;
    PyGILState_STATE gstate;

    gstate = PyGILState_Ensure();
    
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->action;
        if (handler) {
            if (unlock) unlock(unlock_arg); 
            PyObject_CallFunctionObjArgs(handler, NULL);
            if (PyErr_Occurred()) PyErr_Print();
            
            PyGILState_Release(gstate);
            if (lock) lock(lock_arg); 
            return LV_RES_OK;

        }

    }
    PyGILState_Release(gstate);
    return LV_RES_OK;
}


static PyObject *
pylv_ddlist_get_action(pylv_Ddlist *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist)) return NULL;   
    
    PyObject *action = self->action;
    if (!action) Py_RETURN_NONE;

    Py_INCREF(action);
    return action;
}

static PyObject *
pylv_ddlist_set_action(pylv_Ddlist *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"action", NULL};
    PyObject *action, *tmp;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist , &action)) return NULL;
    
    tmp = self->action;
    if (action == Py_None) {
        self->action = NULL;
    } else {
        self->action = action;
        Py_INCREF(action);
        lv_ddlist_set_action(self->ref, pylv_ddlist_action_callback);
    }
    Py_XDECREF(tmp); // Old action (tmp) could be NULL

    Py_RETURN_NONE;
}

            

static PyObject*
pylv_ddlist_set_options(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"options", NULL};
    char * options;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &options)) return NULL;

    LVGL_LOCK         
    lv_ddlist_set_options(self->ref, options);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_selected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"fit_en", NULL};
    int fit_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &fit_en)) return NULL;

    LVGL_LOCK         
    lv_ddlist_set_hor_fit(self->ref, fit_en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    LVGL_LOCK         
    lv_ddlist_set_style(self->ref, type, style->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_get_options(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    char * result = lv_ddlist_get_options(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_ddlist_get_selected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_ddlist_get_selected(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_ddlist_get_selected_str(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"buf", NULL};
    char * buf;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &buf)) return NULL;

    LVGL_LOCK         
    lv_ddlist_get_selected_str(self->ref, buf);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_get_fix_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_ddlist_get_fix_height(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_ddlist_get_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_ddlist_get_anim_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_ddlist_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_ddlist_get_style(self->ref, type));
}

static PyObject*
pylv_ddlist_open(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"anim", NULL};
    int anim;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &anim)) return NULL;

    LVGL_LOCK         
    lv_ddlist_open(self->ref, anim);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_close(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"anim", NULL};
    int anim;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &anim)) return NULL;

    LVGL_LOCK         
    lv_ddlist_close(self->ref, anim);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}


static PyMethodDef pylv_ddlist_methods[] = {
    {"set_options", (PyCFunction) pylv_ddlist_set_options, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_selected", (PyCFunction) pylv_ddlist_set_selected, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_action", (PyCFunction) pylv_ddlist_set_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_fix_height", (PyCFunction) pylv_ddlist_set_fix_height, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_hor_fit", (PyCFunction) pylv_ddlist_set_hor_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_anim_time", (PyCFunction) pylv_ddlist_set_anim_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_ddlist_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_options", (PyCFunction) pylv_ddlist_get_options, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_selected", (PyCFunction) pylv_ddlist_get_selected, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_selected_str", (PyCFunction) pylv_ddlist_get_selected_str, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_action", (PyCFunction) pylv_ddlist_get_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_fix_height", (PyCFunction) pylv_ddlist_get_fix_height, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_anim_time", (PyCFunction) pylv_ddlist_get_anim_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_ddlist_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"open", (PyCFunction) pylv_ddlist_open, METH_VARARGS | METH_KEYWORDS, NULL},
    {"close", (PyCFunction) pylv_ddlist_close, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_preload_dealloc(pylv_Preload *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_preload_set_arc_length(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"deg", NULL};
    unsigned short int deg;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &deg)) return NULL;

    LVGL_LOCK         
    lv_preload_set_arc_length(self->ref, deg);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_preload_set_spin_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    LVGL_LOCK         
    lv_preload_set_style(self->ref, type, style->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_preload_get_arc_length(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_preload_get_arc_length(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_preload_get_spin_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_preload_get_spin_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_preload_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_preload_get_style(self->ref, type));
}


static PyMethodDef pylv_preload_methods[] = {
    {"set_arc_length", (PyCFunction) pylv_preload_set_arc_length, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_spin_time", (PyCFunction) pylv_preload_set_spin_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_preload_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_arc_length", (PyCFunction) pylv_preload_get_arc_length, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_spin_time", (PyCFunction) pylv_preload_get_spin_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_preload_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_list_dealloc(pylv_List *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_list_clean(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_list_clean(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_list_set_btn_selected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    LVGL_LOCK         
    lv_list_set_style(self->ref, type, style->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_list_get_btn_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    char * result = lv_list_get_btn_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_list_get_btn_label(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
pylv_list_get_btn_selected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_list_get_anim_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_list_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_list_get_style(self->ref, type));
}

static PyObject*
pylv_list_up(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_list_down(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}


static PyMethodDef pylv_list_methods[] = {
    {"clean", (PyCFunction) pylv_list_clean, METH_VARARGS | METH_KEYWORDS, NULL},
    {"add", (PyCFunction) pylv_list_add, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_btn_selected", (PyCFunction) pylv_list_set_btn_selected, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_anim_time", (PyCFunction) pylv_list_set_anim_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_list_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_btn_text", (PyCFunction) pylv_list_get_btn_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_btn_label", (PyCFunction) pylv_list_get_btn_label, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_btn_img", (PyCFunction) pylv_list_get_btn_img, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_prev_btn", (PyCFunction) pylv_list_get_prev_btn, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_next_btn", (PyCFunction) pylv_list_get_next_btn, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_btn_selected", (PyCFunction) pylv_list_get_btn_selected, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_anim_time", (PyCFunction) pylv_list_get_anim_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_list_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"up", (PyCFunction) pylv_list_up, METH_VARARGS | METH_KEYWORDS, NULL},
    {"down", (PyCFunction) pylv_list_down, METH_VARARGS | METH_KEYWORDS, NULL},
    {"focus", (PyCFunction) pylv_list_focus, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_slider_dealloc(pylv_Slider *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


lv_res_t pylv_slider_action_callback(lv_obj_t* obj) {
    pylv_Slider *pyobj;
    PyObject *handler;
    PyGILState_STATE gstate;

    gstate = PyGILState_Ensure();
    
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->action;
        if (handler) {
            if (unlock) unlock(unlock_arg); 
            PyObject_CallFunctionObjArgs(handler, NULL);
            if (PyErr_Occurred()) PyErr_Print();
            
            PyGILState_Release(gstate);
            if (lock) lock(lock_arg); 
            return LV_RES_OK;

        }

    }
    PyGILState_Release(gstate);
    return LV_RES_OK;
}


static PyObject *
pylv_slider_get_action(pylv_Slider *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist)) return NULL;   
    
    PyObject *action = self->action;
    if (!action) Py_RETURN_NONE;

    Py_INCREF(action);
    return action;
}

static PyObject *
pylv_slider_set_action(pylv_Slider *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"action", NULL};
    PyObject *action, *tmp;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist , &action)) return NULL;
    
    tmp = self->action;
    if (action == Py_None) {
        self->action = NULL;
    } else {
        self->action = action;
        Py_INCREF(action);
        lv_slider_set_action(self->ref, pylv_slider_action_callback);
    }
    Py_XDECREF(tmp); // Old action (tmp) could be NULL

    Py_RETURN_NONE;
}

            

static PyObject*
pylv_slider_set_knob_in(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    LVGL_LOCK         
    lv_slider_set_style(self->ref, type, style->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_slider_get_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_slider_get_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_slider_is_dragged(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_slider_is_dragged(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_slider_get_knob_in(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_slider_get_knob_in(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_slider_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_slider_get_style(self->ref, type));
}


static PyMethodDef pylv_slider_methods[] = {
    {"set_action", (PyCFunction) pylv_slider_set_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_knob_in", (PyCFunction) pylv_slider_set_knob_in, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_slider_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_value", (PyCFunction) pylv_slider_get_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_action", (PyCFunction) pylv_slider_get_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"is_dragged", (PyCFunction) pylv_slider_is_dragged, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_knob_in", (PyCFunction) pylv_slider_get_knob_in, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_slider_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_sw_dealloc(pylv_Sw *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_sw_on(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_sw_on(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_sw_off(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK         
    lv_sw_off(self->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_sw_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    LVGL_LOCK         
    lv_sw_set_style(self->ref, type, style->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_sw_get_state(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_sw_get_state(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_sw_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_sw_get_style(self->ref, type));
}


static PyMethodDef pylv_sw_methods[] = {
    {"on", (PyCFunction) pylv_sw_on, METH_VARARGS | METH_KEYWORDS, NULL},
    {"off", (PyCFunction) pylv_sw_off, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_sw_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_state", (PyCFunction) pylv_sw_get_state, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_sw_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_cb_dealloc(pylv_Cb *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


lv_res_t pylv_cb_action_callback(lv_obj_t* obj) {
    pylv_Cb *pyobj;
    PyObject *handler;
    PyGILState_STATE gstate;

    gstate = PyGILState_Ensure();
    
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->action;
        if (handler) {
            if (unlock) unlock(unlock_arg); 
            PyObject_CallFunctionObjArgs(handler, NULL);
            if (PyErr_Occurred()) PyErr_Print();
            
            PyGILState_Release(gstate);
            if (lock) lock(lock_arg); 
            return LV_RES_OK;

        }

    }
    PyGILState_Release(gstate);
    return LV_RES_OK;
}


static PyObject *
pylv_cb_get_action(pylv_Cb *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist)) return NULL;   
    
    PyObject *action = self->action;
    if (!action) Py_RETURN_NONE;

    Py_INCREF(action);
    return action;
}

static PyObject *
pylv_cb_set_action(pylv_Cb *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"action", NULL};
    PyObject *action, *tmp;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist , &action)) return NULL;
    
    tmp = self->action;
    if (action == Py_None) {
        self->action = NULL;
    } else {
        self->action = action;
        Py_INCREF(action);
        lv_cb_set_action(self->ref, pylv_cb_action_callback);
    }
    Py_XDECREF(tmp); // Old action (tmp) could be NULL

    Py_RETURN_NONE;
}

            

static PyObject*
pylv_cb_set_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"txt", NULL};
    char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    LVGL_LOCK         
    lv_cb_set_text(self->ref, txt);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_cb_set_checked(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    LVGL_LOCK         
    lv_cb_set_style(self->ref, type, style->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_cb_get_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    char * result = lv_cb_get_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_cb_is_checked(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_cb_is_checked(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_cb_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_cb_get_style(self->ref, type));
}


static PyMethodDef pylv_cb_methods[] = {
    {"set_text", (PyCFunction) pylv_cb_set_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_checked", (PyCFunction) pylv_cb_set_checked, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_inactive", (PyCFunction) pylv_cb_set_inactive, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_action", (PyCFunction) pylv_cb_set_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_cb_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_text", (PyCFunction) pylv_cb_get_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"is_checked", (PyCFunction) pylv_cb_is_checked, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_action", (PyCFunction) pylv_cb_get_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_cb_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
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
};

static void
pylv_roller_dealloc(pylv_Roller *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_roller_set_selected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    LVGL_LOCK         
    lv_roller_set_style(self->ref, type, style->ref);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_roller_get_hor_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_roller_get_hor_fit(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_roller_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_roller_get_style(self->ref, type));
}


static PyMethodDef pylv_roller_methods[] = {
    {"set_selected", (PyCFunction) pylv_roller_set_selected, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_visible_row_count", (PyCFunction) pylv_roller_set_visible_row_count, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_roller_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_hor_fit", (PyCFunction) pylv_roller_get_hor_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_roller_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
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

static PyObject *
report_style_mod(PyObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"style", NULL};
    Style_Object *style = NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!", kwlist, &Style_Type, &style)) {
        return NULL;
    }
    
    LVGL_LOCK
    lv_obj_report_style_mod(style ? style->ref: NULL);
    LVGL_UNLOCK
    
    Py_RETURN_NONE;
}


char framebuffer[LV_HOR_RES * LV_VER_RES * 2];
int redraw = 0;


/* disp_flush should copy from the VDB (virtual display buffer to the screen.
 * In our case, we copy to the framebuffer
 */
static void disp_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_p) {

    char *dest = framebuffer + ((y1)*LV_HOR_RES + x1) * 2;
    char *src = (char *) color_p;
    
    for(int32_t y = y1; y<=y2; y++) {
        memcpy(dest, src, 2*(x2-x1+1));
        src += 2*(x2-x1+1);
        dest += 2*LV_HOR_RES;
    }
    redraw++;
    
    lv_flush_ready();
}

static lv_disp_drv_t diplay_driver = {0};
static lv_indev_drv_t indev_driver = {0};
static int indev_driver_registered = 0;
static int indev_x, indev_y, indev_state=0;

static bool indev_read(lv_indev_data_t *data) {
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
        indev_driver.read = indev_read;
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
    {"report_style_mod", (PyCFunction)report_style_mod, METH_VARARGS | METH_KEYWORDS, NULL},
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
    
    if (PyType_Ready(&Font_Type) < 0) return NULL;

    if (PyType_Ready(&Style_Type) < 0) return NULL;
    
    pylv_obj_Type.tp_repr = (reprfunc) Obj_repr;
    

    pylv_obj_Type.tp_base = NULL;
    if (PyType_Ready(&pylv_obj_Type) < 0) return NULL;

    pylv_win_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_win_Type) < 0) return NULL;

    pylv_label_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_label_Type) < 0) return NULL;

    pylv_lmeter_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_lmeter_Type) < 0) return NULL;

    pylv_btnm_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_btnm_Type) < 0) return NULL;

    pylv_chart_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_chart_Type) < 0) return NULL;

    pylv_cont_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_cont_Type) < 0) return NULL;

    pylv_led_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_led_Type) < 0) return NULL;

    pylv_kb_Type.tp_base = &pylv_btnm_Type;
    if (PyType_Ready(&pylv_kb_Type) < 0) return NULL;

    pylv_img_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_img_Type) < 0) return NULL;

    pylv_bar_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_bar_Type) < 0) return NULL;

    pylv_arc_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_arc_Type) < 0) return NULL;

    pylv_line_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_line_Type) < 0) return NULL;

    pylv_tabview_Type.tp_base = &pylv_obj_Type;
    if (PyType_Ready(&pylv_tabview_Type) < 0) return NULL;

    pylv_mbox_Type.tp_base = &pylv_cont_Type;
    if (PyType_Ready(&pylv_mbox_Type) < 0) return NULL;

    pylv_gauge_Type.tp_base = &pylv_lmeter_Type;
    if (PyType_Ready(&pylv_gauge_Type) < 0) return NULL;

    pylv_page_Type.tp_base = &pylv_cont_Type;
    if (PyType_Ready(&pylv_page_Type) < 0) return NULL;

    pylv_ta_Type.tp_base = &pylv_page_Type;
    if (PyType_Ready(&pylv_ta_Type) < 0) return NULL;

    pylv_btn_Type.tp_base = &pylv_cont_Type;
    if (PyType_Ready(&pylv_btn_Type) < 0) return NULL;

    pylv_ddlist_Type.tp_base = &pylv_page_Type;
    if (PyType_Ready(&pylv_ddlist_Type) < 0) return NULL;

    pylv_preload_Type.tp_base = &pylv_arc_Type;
    if (PyType_Ready(&pylv_preload_Type) < 0) return NULL;

    pylv_list_Type.tp_base = &pylv_page_Type;
    if (PyType_Ready(&pylv_list_Type) < 0) return NULL;

    pylv_slider_Type.tp_base = &pylv_bar_Type;
    if (PyType_Ready(&pylv_slider_Type) < 0) return NULL;

    pylv_sw_Type.tp_base = &pylv_slider_Type;
    if (PyType_Ready(&pylv_sw_Type) < 0) return NULL;

    pylv_cb_Type.tp_base = &pylv_btn_Type;
    if (PyType_Ready(&pylv_cb_Type) < 0) return NULL;

    pylv_roller_Type.tp_base = &pylv_ddlist_Type;
    if (PyType_Ready(&pylv_roller_Type) < 0) return NULL;


    Py_INCREF(&Style_Type);
    PyModule_AddObject(module, "Style", (PyObject *) &Style_Type); 


    Py_INCREF(&pylv_obj_Type);
    PyModule_AddObject(module, "Obj", (PyObject *) &pylv_obj_Type); 

    Py_INCREF(&pylv_win_Type);
    PyModule_AddObject(module, "Win", (PyObject *) &pylv_win_Type); 

    Py_INCREF(&pylv_label_Type);
    PyModule_AddObject(module, "Label", (PyObject *) &pylv_label_Type); 

    Py_INCREF(&pylv_lmeter_Type);
    PyModule_AddObject(module, "Lmeter", (PyObject *) &pylv_lmeter_Type); 

    Py_INCREF(&pylv_btnm_Type);
    PyModule_AddObject(module, "Btnm", (PyObject *) &pylv_btnm_Type); 

    Py_INCREF(&pylv_chart_Type);
    PyModule_AddObject(module, "Chart", (PyObject *) &pylv_chart_Type); 

    Py_INCREF(&pylv_cont_Type);
    PyModule_AddObject(module, "Cont", (PyObject *) &pylv_cont_Type); 

    Py_INCREF(&pylv_led_Type);
    PyModule_AddObject(module, "Led", (PyObject *) &pylv_led_Type); 

    Py_INCREF(&pylv_kb_Type);
    PyModule_AddObject(module, "Kb", (PyObject *) &pylv_kb_Type); 

    Py_INCREF(&pylv_img_Type);
    PyModule_AddObject(module, "Img", (PyObject *) &pylv_img_Type); 

    Py_INCREF(&pylv_bar_Type);
    PyModule_AddObject(module, "Bar", (PyObject *) &pylv_bar_Type); 

    Py_INCREF(&pylv_arc_Type);
    PyModule_AddObject(module, "Arc", (PyObject *) &pylv_arc_Type); 

    Py_INCREF(&pylv_line_Type);
    PyModule_AddObject(module, "Line", (PyObject *) &pylv_line_Type); 

    Py_INCREF(&pylv_tabview_Type);
    PyModule_AddObject(module, "Tabview", (PyObject *) &pylv_tabview_Type); 

    Py_INCREF(&pylv_mbox_Type);
    PyModule_AddObject(module, "Mbox", (PyObject *) &pylv_mbox_Type); 

    Py_INCREF(&pylv_gauge_Type);
    PyModule_AddObject(module, "Gauge", (PyObject *) &pylv_gauge_Type); 

    Py_INCREF(&pylv_page_Type);
    PyModule_AddObject(module, "Page", (PyObject *) &pylv_page_Type); 

    Py_INCREF(&pylv_ta_Type);
    PyModule_AddObject(module, "Ta", (PyObject *) &pylv_ta_Type); 

    Py_INCREF(&pylv_btn_Type);
    PyModule_AddObject(module, "Btn", (PyObject *) &pylv_btn_Type); 

    Py_INCREF(&pylv_ddlist_Type);
    PyModule_AddObject(module, "Ddlist", (PyObject *) &pylv_ddlist_Type); 

    Py_INCREF(&pylv_preload_Type);
    PyModule_AddObject(module, "Preload", (PyObject *) &pylv_preload_Type); 

    Py_INCREF(&pylv_list_Type);
    PyModule_AddObject(module, "List", (PyObject *) &pylv_list_Type); 

    Py_INCREF(&pylv_slider_Type);
    PyModule_AddObject(module, "Slider", (PyObject *) &pylv_slider_Type); 

    Py_INCREF(&pylv_sw_Type);
    PyModule_AddObject(module, "Sw", (PyObject *) &pylv_sw_Type); 

    Py_INCREF(&pylv_cb_Type);
    PyModule_AddObject(module, "Cb", (PyObject *) &pylv_cb_Type); 

    Py_INCREF(&pylv_roller_Type);
    PyModule_AddObject(module, "Roller", (PyObject *) &pylv_roller_Type); 


    
    PyModule_AddObject(module, "BORDER", build_enum("BORDER", "NONE", LV_BORDER_NONE, "BOTTOM", LV_BORDER_BOTTOM, "TOP", LV_BORDER_TOP, "LEFT", LV_BORDER_LEFT, "RIGHT", LV_BORDER_RIGHT, "FULL", LV_BORDER_FULL, NULL));
    PyModule_AddObject(module, "SHADOW", build_enum("SHADOW", "BOTTOM", LV_SHADOW_BOTTOM, "FULL", LV_SHADOW_FULL, NULL));
    PyModule_AddObject(module, "DESIGN", build_enum("DESIGN", "DRAW_MAIN", LV_DESIGN_DRAW_MAIN, "DRAW_POST", LV_DESIGN_DRAW_POST, "COVER_CHK", LV_DESIGN_COVER_CHK, NULL));
    PyModule_AddObject(module, "RES", build_enum("RES", "INV", LV_RES_INV, "OK", LV_RES_OK, NULL));
    PyModule_AddObject(module, "SIGNAL", build_enum("SIGNAL", "CLEANUP", LV_SIGNAL_CLEANUP, "CHILD_CHG", LV_SIGNAL_CHILD_CHG, "CORD_CHG", LV_SIGNAL_CORD_CHG, "STYLE_CHG", LV_SIGNAL_STYLE_CHG, "REFR_EXT_SIZE", LV_SIGNAL_REFR_EXT_SIZE, "GET_TYPE", LV_SIGNAL_GET_TYPE, "PRESSED", LV_SIGNAL_PRESSED, "PRESSING", LV_SIGNAL_PRESSING, "PRESS_LOST", LV_SIGNAL_PRESS_LOST, "RELEASED", LV_SIGNAL_RELEASED, "LONG_PRESS", LV_SIGNAL_LONG_PRESS, "LONG_PRESS_REP", LV_SIGNAL_LONG_PRESS_REP, "DRAG_BEGIN", LV_SIGNAL_DRAG_BEGIN, "DRAG_END", LV_SIGNAL_DRAG_END, "FOCUS", LV_SIGNAL_FOCUS, "DEFOCUS", LV_SIGNAL_DEFOCUS, "CONTROLL", LV_SIGNAL_CONTROLL, "GET_EDITABLE", LV_SIGNAL_GET_EDITABLE, NULL));
    PyModule_AddObject(module, "PROTECT", build_enum("PROTECT", "NONE", LV_PROTECT_NONE, "CHILD_CHG", LV_PROTECT_CHILD_CHG, "PARENT", LV_PROTECT_PARENT, "POS", LV_PROTECT_POS, "FOLLOW", LV_PROTECT_FOLLOW, "PRESS_LOST", LV_PROTECT_PRESS_LOST, "CLICK_FOCUS", LV_PROTECT_CLICK_FOCUS, NULL));
    PyModule_AddObject(module, "ALIGN", build_enum("ALIGN", "CENTER", LV_ALIGN_CENTER, "IN_TOP_LEFT", LV_ALIGN_IN_TOP_LEFT, "IN_TOP_MID", LV_ALIGN_IN_TOP_MID, "IN_TOP_RIGHT", LV_ALIGN_IN_TOP_RIGHT, "IN_BOTTOM_LEFT", LV_ALIGN_IN_BOTTOM_LEFT, "IN_BOTTOM_MID", LV_ALIGN_IN_BOTTOM_MID, "IN_BOTTOM_RIGHT", LV_ALIGN_IN_BOTTOM_RIGHT, "IN_LEFT_MID", LV_ALIGN_IN_LEFT_MID, "IN_RIGHT_MID", LV_ALIGN_IN_RIGHT_MID, "OUT_TOP_LEFT", LV_ALIGN_OUT_TOP_LEFT, "OUT_TOP_MID", LV_ALIGN_OUT_TOP_MID, "OUT_TOP_RIGHT", LV_ALIGN_OUT_TOP_RIGHT, "OUT_BOTTOM_LEFT", LV_ALIGN_OUT_BOTTOM_LEFT, "OUT_BOTTOM_MID", LV_ALIGN_OUT_BOTTOM_MID, "OUT_BOTTOM_RIGHT", LV_ALIGN_OUT_BOTTOM_RIGHT, "OUT_LEFT_TOP", LV_ALIGN_OUT_LEFT_TOP, "OUT_LEFT_MID", LV_ALIGN_OUT_LEFT_MID, "OUT_LEFT_BOTTOM", LV_ALIGN_OUT_LEFT_BOTTOM, "OUT_RIGHT_TOP", LV_ALIGN_OUT_RIGHT_TOP, "OUT_RIGHT_MID", LV_ALIGN_OUT_RIGHT_MID, "OUT_RIGHT_BOTTOM", LV_ALIGN_OUT_RIGHT_BOTTOM, NULL));
    PyModule_AddObject(module, "ANIM", build_enum("ANIM", "NONE", LV_ANIM_NONE, "FLOAT_TOP", LV_ANIM_FLOAT_TOP, "FLOAT_LEFT", LV_ANIM_FLOAT_LEFT, "FLOAT_BOTTOM", LV_ANIM_FLOAT_BOTTOM, "FLOAT_RIGHT", LV_ANIM_FLOAT_RIGHT, "GROW_H", LV_ANIM_GROW_H, "GROW_V", LV_ANIM_GROW_V, NULL));
    PyModule_AddObject(module, "LAYOUT", build_enum("LAYOUT", "OFF", LV_LAYOUT_OFF, "CENTER", LV_LAYOUT_CENTER, "COL_L", LV_LAYOUT_COL_L, "COL_M", LV_LAYOUT_COL_M, "COL_R", LV_LAYOUT_COL_R, "ROW_T", LV_LAYOUT_ROW_T, "ROW_M", LV_LAYOUT_ROW_M, "ROW_B", LV_LAYOUT_ROW_B, "PRETTY", LV_LAYOUT_PRETTY, "GRID", LV_LAYOUT_GRID, NULL));
    PyModule_AddObject(module, "INDEV_TYPE", build_enum("INDEV_TYPE", "NONE", LV_INDEV_TYPE_NONE, "POINTER", LV_INDEV_TYPE_POINTER, "KEYPAD", LV_INDEV_TYPE_KEYPAD, "BUTTON", LV_INDEV_TYPE_BUTTON, "ENCODER", LV_INDEV_TYPE_ENCODER, NULL));
    PyModule_AddObject(module, "INDEV_STATE", build_enum("INDEV_STATE", "REL", LV_INDEV_STATE_REL, "PR", LV_INDEV_STATE_PR, NULL));
    PyModule_AddObject(module, "BTN_STATE", build_enum("BTN_STATE", "REL", LV_BTN_STATE_REL, "PR", LV_BTN_STATE_PR, "TGL_REL", LV_BTN_STATE_TGL_REL, "TGL_PR", LV_BTN_STATE_TGL_PR, "INA", LV_BTN_STATE_INA, "NUM", LV_BTN_STATE_NUM, NULL));
    PyModule_AddObject(module, "BTN_ACTION", build_enum("BTN_ACTION", "CLICK", LV_BTN_ACTION_CLICK, "PR", LV_BTN_ACTION_PR, "LONG_PR", LV_BTN_ACTION_LONG_PR, "LONG_PR_REPEAT", LV_BTN_ACTION_LONG_PR_REPEAT, "NUM", LV_BTN_ACTION_NUM, NULL));
    PyModule_AddObject(module, "BTN_STYLE", build_enum("BTN_STYLE", "REL", LV_BTN_STYLE_REL, "PR", LV_BTN_STYLE_PR, "TGL_REL", LV_BTN_STYLE_TGL_REL, "TGL_PR", LV_BTN_STYLE_TGL_PR, "INA", LV_BTN_STYLE_INA, NULL));
    PyModule_AddObject(module, "TXT_FLAG", build_enum("TXT_FLAG", "NONE", LV_TXT_FLAG_NONE, "RECOLOR", LV_TXT_FLAG_RECOLOR, "EXPAND", LV_TXT_FLAG_EXPAND, "CENTER", LV_TXT_FLAG_CENTER, "RIGHT", LV_TXT_FLAG_RIGHT, NULL));
    PyModule_AddObject(module, "TXT_CMD_STATE", build_enum("TXT_CMD_STATE", "WAIT", LV_TXT_CMD_STATE_WAIT, "PAR", LV_TXT_CMD_STATE_PAR, "IN", LV_TXT_CMD_STATE_IN, NULL));
    PyModule_AddObject(module, "LABEL_LONG", build_enum("LABEL_LONG", "EXPAND", LV_LABEL_LONG_EXPAND, "BREAK", LV_LABEL_LONG_BREAK, "SCROLL", LV_LABEL_LONG_SCROLL, "DOT", LV_LABEL_LONG_DOT, "ROLL", LV_LABEL_LONG_ROLL, "CROP", LV_LABEL_LONG_CROP, NULL));
    PyModule_AddObject(module, "LABEL_ALIGN", build_enum("LABEL_ALIGN", "LEFT", LV_LABEL_ALIGN_LEFT, "CENTER", LV_LABEL_ALIGN_CENTER, "RIGHT", LV_LABEL_ALIGN_RIGHT, NULL));
    PyModule_AddObject(module, "BAR_STYLE", build_enum("BAR_STYLE", "BG", LV_BAR_STYLE_BG, "INDIC", LV_BAR_STYLE_INDIC, NULL));
    PyModule_AddObject(module, "SLIDER_STYLE", build_enum("SLIDER_STYLE", "BG", LV_SLIDER_STYLE_BG, "INDIC", LV_SLIDER_STYLE_INDIC, "KNOB", LV_SLIDER_STYLE_KNOB, NULL));
    PyModule_AddObject(module, "FS_RES", build_enum("FS_RES", "OK", LV_FS_RES_OK, "HW_ERR", LV_FS_RES_HW_ERR, "FS_ERR", LV_FS_RES_FS_ERR, "NOT_EX", LV_FS_RES_NOT_EX, "FULL", LV_FS_RES_FULL, "LOCKED", LV_FS_RES_LOCKED, "DENIED", LV_FS_RES_DENIED, "BUSY", LV_FS_RES_BUSY, "TOUT", LV_FS_RES_TOUT, "NOT_IMP", LV_FS_RES_NOT_IMP, "OUT_OF_MEM", LV_FS_RES_OUT_OF_MEM, "INV_PARAM", LV_FS_RES_INV_PARAM, "UNKNOWN", LV_FS_RES_UNKNOWN, NULL));
    PyModule_AddObject(module, "FS_MODE", build_enum("FS_MODE", "WR", LV_FS_MODE_WR, "RD", LV_FS_MODE_RD, NULL));
    PyModule_AddObject(module, "IMG_SRC", build_enum("IMG_SRC", "VARIABLE", LV_IMG_SRC_VARIABLE, "FILE", LV_IMG_SRC_FILE, "SYMBOL", LV_IMG_SRC_SYMBOL, "UNKNOWN", LV_IMG_SRC_UNKNOWN, NULL));
    PyModule_AddObject(module, "IMG_CF", build_enum("IMG_CF", "UNKOWN", LV_IMG_CF_UNKOWN, "RAW", LV_IMG_CF_RAW, "RAW_ALPHA", LV_IMG_CF_RAW_ALPHA, "RAW_CHROMA_KEYED", LV_IMG_CF_RAW_CHROMA_KEYED, "TRUE_COLOR", LV_IMG_CF_TRUE_COLOR, "TRUE_COLOR_ALPHA", LV_IMG_CF_TRUE_COLOR_ALPHA, "TRUE_COLOR_CHROMA_KEYED", LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED, "INDEXED_1BIT", LV_IMG_CF_INDEXED_1BIT, "INDEXED_2BIT", LV_IMG_CF_INDEXED_2BIT, "INDEXED_4BIT", LV_IMG_CF_INDEXED_4BIT, "INDEXED_8BIT", LV_IMG_CF_INDEXED_8BIT, "ALPHA_1BIT", LV_IMG_CF_ALPHA_1BIT, "ALPHA_2BIT", LV_IMG_CF_ALPHA_2BIT, "ALPHA_4BIT", LV_IMG_CF_ALPHA_4BIT, "ALPHA_8BIT", LV_IMG_CF_ALPHA_8BIT, NULL));
    PyModule_AddObject(module, "SB_MODE", build_enum("SB_MODE", "OFF", LV_SB_MODE_OFF, "ON", LV_SB_MODE_ON, "DRAG", LV_SB_MODE_DRAG, "AUTO", LV_SB_MODE_AUTO, "HIDE", LV_SB_MODE_HIDE, "UNHIDE", LV_SB_MODE_UNHIDE, NULL));
    PyModule_AddObject(module, "PAGE_STYLE", build_enum("PAGE_STYLE", "BG", LV_PAGE_STYLE_BG, "SCRL", LV_PAGE_STYLE_SCRL, "SB", LV_PAGE_STYLE_SB, NULL));
    PyModule_AddObject(module, "LIST_STYLE", build_enum("LIST_STYLE", "BG", LV_LIST_STYLE_BG, "SCRL", LV_LIST_STYLE_SCRL, "SB", LV_LIST_STYLE_SB, "BTN_REL", LV_LIST_STYLE_BTN_REL, "BTN_PR", LV_LIST_STYLE_BTN_PR, "BTN_TGL_REL", LV_LIST_STYLE_BTN_TGL_REL, "BTN_TGL_PR", LV_LIST_STYLE_BTN_TGL_PR, "BTN_INA", LV_LIST_STYLE_BTN_INA, NULL));
    PyModule_AddObject(module, "ARC_STYLE", build_enum("ARC_STYLE", "MAIN", LV_ARC_STYLE_MAIN, NULL));
    PyModule_AddObject(module, "PRELOAD_TYPE_SPINNING", build_enum("PRELOAD_TYPE_SPINNING", "ARC", LV_PRELOAD_TYPE_SPINNING_ARC, NULL));
    PyModule_AddObject(module, "PRELOAD_STYLE", build_enum("PRELOAD_STYLE", "MAIN", LV_PRELOAD_STYLE_MAIN, NULL));
    PyModule_AddObject(module, "BTNM_STYLE", build_enum("BTNM_STYLE", "BG", LV_BTNM_STYLE_BG, "BTN_REL", LV_BTNM_STYLE_BTN_REL, "BTN_PR", LV_BTNM_STYLE_BTN_PR, "BTN_TGL_REL", LV_BTNM_STYLE_BTN_TGL_REL, "BTN_TGL_PR", LV_BTNM_STYLE_BTN_TGL_PR, "BTN_INA", LV_BTNM_STYLE_BTN_INA, NULL));
    PyModule_AddObject(module, "KB_MODE", build_enum("KB_MODE", "TEXT", LV_KB_MODE_TEXT, "NUM", LV_KB_MODE_NUM, NULL));
    PyModule_AddObject(module, "KB_STYLE", build_enum("KB_STYLE", "BG", LV_KB_STYLE_BG, "BTN_REL", LV_KB_STYLE_BTN_REL, "BTN_PR", LV_KB_STYLE_BTN_PR, "BTN_TGL_REL", LV_KB_STYLE_BTN_TGL_REL, "BTN_TGL_PR", LV_KB_STYLE_BTN_TGL_PR, "BTN_INA", LV_KB_STYLE_BTN_INA, NULL));
    PyModule_AddObject(module, "WIN_STYLE", build_enum("WIN_STYLE", "BG", LV_WIN_STYLE_BG, "CONTENT_BG", LV_WIN_STYLE_CONTENT_BG, "CONTENT_SCRL", LV_WIN_STYLE_CONTENT_SCRL, "SB", LV_WIN_STYLE_SB, "HEADER", LV_WIN_STYLE_HEADER, "BTN_REL", LV_WIN_STYLE_BTN_REL, "BTN_PR", LV_WIN_STYLE_BTN_PR, NULL));
    PyModule_AddObject(module, "TABVIEW_BTNS_POS", build_enum("TABVIEW_BTNS_POS", "TOP", LV_TABVIEW_BTNS_POS_TOP, "BOTTOM", LV_TABVIEW_BTNS_POS_BOTTOM, NULL));
    PyModule_AddObject(module, "TABVIEW_STYLE", build_enum("TABVIEW_STYLE", "BG", LV_TABVIEW_STYLE_BG, "INDIC", LV_TABVIEW_STYLE_INDIC, "BTN_BG", LV_TABVIEW_STYLE_BTN_BG, "BTN_REL", LV_TABVIEW_STYLE_BTN_REL, "BTN_PR", LV_TABVIEW_STYLE_BTN_PR, "BTN_TGL_REL", LV_TABVIEW_STYLE_BTN_TGL_REL, "BTN_TGL_PR", LV_TABVIEW_STYLE_BTN_TGL_PR, NULL));
    PyModule_AddObject(module, "SW_STYLE", build_enum("SW_STYLE", "BG", LV_SW_STYLE_BG, "INDIC", LV_SW_STYLE_INDIC, "KNOB_OFF", LV_SW_STYLE_KNOB_OFF, "KNOB_ON", LV_SW_STYLE_KNOB_ON, NULL));
    PyModule_AddObject(module, "MBOX_STYLE", build_enum("MBOX_STYLE", "BG", LV_MBOX_STYLE_BG, "BTN_BG", LV_MBOX_STYLE_BTN_BG, "BTN_REL", LV_MBOX_STYLE_BTN_REL, "BTN_PR", LV_MBOX_STYLE_BTN_PR, "BTN_TGL_REL", LV_MBOX_STYLE_BTN_TGL_REL, "BTN_TGL_PR", LV_MBOX_STYLE_BTN_TGL_PR, "BTN_INA", LV_MBOX_STYLE_BTN_INA, NULL));
    PyModule_AddObject(module, "CB_STYLE", build_enum("CB_STYLE", "BG", LV_CB_STYLE_BG, "BOX_REL", LV_CB_STYLE_BOX_REL, "BOX_PR", LV_CB_STYLE_BOX_PR, "BOX_TGL_REL", LV_CB_STYLE_BOX_TGL_REL, "BOX_TGL_PR", LV_CB_STYLE_BOX_TGL_PR, "BOX_INA", LV_CB_STYLE_BOX_INA, NULL));
    PyModule_AddObject(module, "CURSOR", build_enum("CURSOR", "NONE", LV_CURSOR_NONE, "LINE", LV_CURSOR_LINE, "BLOCK", LV_CURSOR_BLOCK, "OUTLINE", LV_CURSOR_OUTLINE, "UNDERLINE", LV_CURSOR_UNDERLINE, "HIDDEN", LV_CURSOR_HIDDEN, NULL));
    PyModule_AddObject(module, "TA_STYLE", build_enum("TA_STYLE", "BG", LV_TA_STYLE_BG, "SB", LV_TA_STYLE_SB, "CURSOR", LV_TA_STYLE_CURSOR, NULL));
    PyModule_AddObject(module, "DDLIST_STYLE", build_enum("DDLIST_STYLE", "BG", LV_DDLIST_STYLE_BG, "SEL", LV_DDLIST_STYLE_SEL, "SB", LV_DDLIST_STYLE_SB, NULL));
    PyModule_AddObject(module, "ROLLER_STYLE", build_enum("ROLLER_STYLE", "BG", LV_ROLLER_STYLE_BG, "SEL", LV_ROLLER_STYLE_SEL, NULL));
    PyModule_AddObject(module, "CHART_TYPE", build_enum("CHART_TYPE", "LINE", LV_CHART_TYPE_LINE, "COLUMN", LV_CHART_TYPE_COLUMN, "POINT", LV_CHART_TYPE_POINT, NULL));

    
    PyModule_AddObject(module, "style_scr",Style_From_lv_style(&lv_style_scr));
    PyModule_AddObject(module, "style_transp",Style_From_lv_style(&lv_style_transp));
    PyModule_AddObject(module, "style_transp_fit",Style_From_lv_style(&lv_style_transp_fit));
    PyModule_AddObject(module, "style_transp_tight",Style_From_lv_style(&lv_style_transp_tight));
    PyModule_AddObject(module, "style_plain",Style_From_lv_style(&lv_style_plain));
    PyModule_AddObject(module, "style_plain_color",Style_From_lv_style(&lv_style_plain_color));
    PyModule_AddObject(module, "style_pretty",Style_From_lv_style(&lv_style_pretty));
    PyModule_AddObject(module, "style_pretty_color",Style_From_lv_style(&lv_style_pretty_color));
    PyModule_AddObject(module, "style_btn_rel",Style_From_lv_style(&lv_style_btn_rel));
    PyModule_AddObject(module, "style_btn_pr",Style_From_lv_style(&lv_style_btn_pr));
    PyModule_AddObject(module, "style_btn_tgl_rel",Style_From_lv_style(&lv_style_btn_tgl_rel));
    PyModule_AddObject(module, "style_btn_tgl_pr",Style_From_lv_style(&lv_style_btn_tgl_pr));
    PyModule_AddObject(module, "style_btn_ina",Style_From_lv_style(&lv_style_btn_ina));


    PyModule_AddObject(module, "font_dejavu_20", Font_From_lv_font(&lv_font_dejavu_20));
    PyModule_AddObject(module, "font_symbol_20", Font_From_lv_font(&lv_font_symbol_20));
    PyModule_AddObject(module, "font_dejavu_30", Font_From_lv_font(&lv_font_dejavu_30));
    PyModule_AddObject(module, "font_symbol_30", Font_From_lv_font(&lv_font_symbol_30));
    PyModule_AddObject(module, "font_symbol_40", Font_From_lv_font(&lv_font_symbol_40));


    PyModule_AddStringConstant(module, "SYMBOL_AUDIO", "\xEF\xA0\x80");
    PyModule_AddStringConstant(module, "SYMBOL_VIDEO", "\xEF\xA0\x81");
    PyModule_AddStringConstant(module, "SYMBOL_LIST", "\xEF\xA0\x82");
    PyModule_AddStringConstant(module, "SYMBOL_OK", "\xEF\xA0\x83");
    PyModule_AddStringConstant(module, "SYMBOL_CLOSE", "\xEF\xA0\x84");
    PyModule_AddStringConstant(module, "SYMBOL_POWER", "\xEF\xA0\x85");
    PyModule_AddStringConstant(module, "SYMBOL_SETTINGS", "\xEF\xA0\x86");
    PyModule_AddStringConstant(module, "SYMBOL_TRASH", "\xEF\xA0\x87");
    PyModule_AddStringConstant(module, "SYMBOL_HOME", "\xEF\xA0\x88");
    PyModule_AddStringConstant(module, "SYMBOL_DOWNLOAD", "\xEF\xA0\x89");
    PyModule_AddStringConstant(module, "SYMBOL_DRIVE", "\xEF\xA0\x8A");
    PyModule_AddStringConstant(module, "SYMBOL_REFRESH", "\xEF\xA0\x8B");
    PyModule_AddStringConstant(module, "SYMBOL_MUTE", "\xEF\xA0\x8C");
    PyModule_AddStringConstant(module, "SYMBOL_VOLUME_MID", "\xEF\xA0\x8D");
    PyModule_AddStringConstant(module, "SYMBOL_VOLUME_MAX", "\xEF\xA0\x8E");
    PyModule_AddStringConstant(module, "SYMBOL_IMAGE", "\xEF\xA0\x8F");
    PyModule_AddStringConstant(module, "SYMBOL_EDIT", "\xEF\xA0\x90");
    PyModule_AddStringConstant(module, "SYMBOL_PREV", "\xEF\xA0\x91");
    PyModule_AddStringConstant(module, "SYMBOL_PLAY", "\xEF\xA0\x92");
    PyModule_AddStringConstant(module, "SYMBOL_PAUSE", "\xEF\xA0\x93");
    PyModule_AddStringConstant(module, "SYMBOL_STOP", "\xEF\xA0\x94");
    PyModule_AddStringConstant(module, "SYMBOL_NEXT", "\xEF\xA0\x95");
    PyModule_AddStringConstant(module, "SYMBOL_EJECT", "\xEF\xA0\x96");
    PyModule_AddStringConstant(module, "SYMBOL_LEFT", "\xEF\xA0\x97");
    PyModule_AddStringConstant(module, "SYMBOL_RIGHT", "\xEF\xA0\x98");
    PyModule_AddStringConstant(module, "SYMBOL_PLUS", "\xEF\xA0\x99");
    PyModule_AddStringConstant(module, "SYMBOL_MINUS", "\xEF\xA0\x9A");
    PyModule_AddStringConstant(module, "SYMBOL_WARNING", "\xEF\xA0\x9B");
    PyModule_AddStringConstant(module, "SYMBOL_SHUFFLE", "\xEF\xA0\x9C");
    PyModule_AddStringConstant(module, "SYMBOL_UP", "\xEF\xA0\x9D");
    PyModule_AddStringConstant(module, "SYMBOL_DOWN", "\xEF\xA0\x9E");
    PyModule_AddStringConstant(module, "SYMBOL_LOOP", "\xEF\xA0\x9F");
    PyModule_AddStringConstant(module, "SYMBOL_DIRECTORY", "\xEF\xA0\xA0");
    PyModule_AddStringConstant(module, "SYMBOL_UPLOAD", "\xEF\xA0\xA1");
    PyModule_AddStringConstant(module, "SYMBOL_CALL", "\xEF\xA0\xA2");
    PyModule_AddStringConstant(module, "SYMBOL_CUT", "\xEF\xA0\xA3");
    PyModule_AddStringConstant(module, "SYMBOL_COPY", "\xEF\xA0\xA4");
    PyModule_AddStringConstant(module, "SYMBOL_SAVE", "\xEF\xA0\xA5");
    PyModule_AddStringConstant(module, "SYMBOL_CHARGE", "\xEF\xA0\xA6");
    PyModule_AddStringConstant(module, "SYMBOL_BELL", "\xEF\xA0\xA7");
    PyModule_AddStringConstant(module, "SYMBOL_KEYBOARD", "\xEF\xA0\xA8");
    PyModule_AddStringConstant(module, "SYMBOL_GPS", "\xEF\xA0\xA9");
    PyModule_AddStringConstant(module, "SYMBOL_FILE", "\xEF\xA0\xAA");
    PyModule_AddStringConstant(module, "SYMBOL_WIFI", "\xEF\xA0\xAB");
    PyModule_AddStringConstant(module, "SYMBOL_BATTERY_FULL", "\xEF\xA0\xAC");
    PyModule_AddStringConstant(module, "SYMBOL_BATTERY_3", "\xEF\xA0\xAD");
    PyModule_AddStringConstant(module, "SYMBOL_BATTERY_2", "\xEF\xA0\xAE");
    PyModule_AddStringConstant(module, "SYMBOL_BATTERY_1", "\xEF\xA0\xAF");
    PyModule_AddStringConstant(module, "SYMBOL_BATTERY_EMPTY", "\xEF\xA0\xB0");
    PyModule_AddStringConstant(module, "SYMBOL_BLUETOOTH", "\xEF\xA0\xB1");
    PyModule_AddStringConstant(module, "SYMBOL_DUMMY", "\xEF\xA3\xBF");


    // refcount for typesdict is initally 1; it is used by pyobj_from_lv
    // refcounts to py{name}_Type objects are incremented due to "O" format
    typesdict = Py_BuildValue("{sOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsO}",
        "lv_obj", &pylv_obj_Type,
        "lv_win", &pylv_win_Type,
        "lv_label", &pylv_label_Type,
        "lv_lmeter", &pylv_lmeter_Type,
        "lv_btnm", &pylv_btnm_Type,
        "lv_chart", &pylv_chart_Type,
        "lv_cont", &pylv_cont_Type,
        "lv_led", &pylv_led_Type,
        "lv_kb", &pylv_kb_Type,
        "lv_img", &pylv_img_Type,
        "lv_bar", &pylv_bar_Type,
        "lv_arc", &pylv_arc_Type,
        "lv_line", &pylv_line_Type,
        "lv_tabview", &pylv_tabview_Type,
        "lv_mbox", &pylv_mbox_Type,
        "lv_gauge", &pylv_gauge_Type,
        "lv_page", &pylv_page_Type,
        "lv_ta", &pylv_ta_Type,
        "lv_btn", &pylv_btn_Type,
        "lv_ddlist", &pylv_ddlist_Type,
        "lv_preload", &pylv_preload_Type,
        "lv_list", &pylv_list_Type,
        "lv_slider", &pylv_slider_Type,
        "lv_sw", &pylv_sw_Type,
        "lv_cb", &pylv_cb_Type,
        "lv_roller", &pylv_roller_Type);
    
    PyModule_AddObject(module, "framebuffer", PyMemoryView_FromMemory(framebuffer, LV_HOR_RES * LV_VER_RES * 2, PyBUF_READ));
    PyModule_AddObject(module, "HOR_RES", PyLong_FromLong(LV_HOR_RES));
    PyModule_AddObject(module, "VER_RES", PyLong_FromLong(LV_VER_RES));

    diplay_driver.disp_flush = disp_flush;

    lv_init();
    
    lv_disp_drv_register(&diplay_driver);

    return module;
    
error:
    Py_XDECREF(module);
    return NULL;
}

