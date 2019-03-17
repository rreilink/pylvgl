#include "Python.h"
#include "structmember.h"
#undef B0 // Workaround for lvgl Issue 941 https://github.com/littlevgl/lvgl/issues/941
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
    PyObject *event;
    lv_event_cb_t orig_c_event_cb;
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

static PyTypeObject pylv_spinbox_Type;


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
    
    pyobj = *lv_obj_get_user_data(obj);
    
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
        *lv_obj_get_user_data(obj) = pyobj;
    }
    
    return (PyObject *)pyobj;
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
 * Custom types: structs                                        *  
 ****************************************************************/
typedef struct {
    PyObject_HEAD
    void *data;
    PyObject *owner; // NULL = reference to global C data, self=allocated @ init, other object=sharing from that object; decref when we are deallocated
} StructObject;

static PyObject*
Struct_repr(StructObject *self) {
    return PyUnicode_FromFormat("<%s struct at %p data = %p owner = %p>", Py_TYPE(self)->tp_name, self, self->data, self->owner);
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
    .tp_dealloc = (destructor) Struct_dealloc
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


static PyObject *
struct_get_uint8(StructObject *self, void *closure)
{
    return PyLong_FromLong(*((uint8_t*)((char*)self->data + (int)closure) ));
}

static int
struct_set_uint8(StructObject *self, PyObject *value, void *closure)
{
    long v;
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
    if (long_to_int(value, &v, -2147483648, 2147483647)) return -1;
    
    *((int32_t*)((char*)self->data + (int)closure) ) = v;
    return 0;
}


static PyObject *
struct_get_blob(StructObject *self, void *closure)
{
    StructObject *ret;
    ret = (StructObject*)PyObject_New(StructObject, &Blob_Type);
    if (ret) {
        ret->owner = self->owner;
        Py_INCREF(self->owner);
        ret->data = self->data + (int)closure; // TODO: stash the size in there, too
    }
    return (PyObject*)ret;
}

static int
struct_set_blob(StructObject *self, PyObject *value, void *closure)
{
    PyErr_SetString(PyExc_NotImplementedError, "setting this data type is not supported");
    return -1;
}






static int
pylv_mem_monitor_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_mem_monitor_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_mem_monitor_t));
    self->owner = (PyObject *)self;

    return 0;
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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_ll_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_ll_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_ll_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_ll_t_getset[] = {
    {"n_size", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t n_size", (void*)offsetof(lv_ll_t, n_size)},
    {"head", (getter) struct_get_blob, (setter) struct_set_blob, "lv_ll_node_t head", (void*)offsetof(lv_ll_t, head)},
    {"tail", (getter) struct_get_blob, (setter) struct_set_blob, "lv_ll_node_t tail", (void*)offsetof(lv_ll_t, tail)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_task_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_task_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_task_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_task_t_getset[] = {
    {"period", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t period", (void*)offsetof(lv_task_t, period)},
    {"last_run", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t last_run", (void*)offsetof(lv_task_t, last_run)},
    {"task", (getter) struct_get_blob, (setter) struct_set_blob, "void task(void *) task", (void*)offsetof(lv_task_t, task)},
    {"param", (getter) struct_get_blob, (setter) struct_set_blob, "void param", (void*)offsetof(lv_task_t, param)},
    {"prio", (getter) NULL, (setter) NULL, "uint8_t prio", NULL},
    {"once", (getter) NULL, (setter) NULL, "uint8_t once", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_color_hsv_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_color_hsv_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_color_hsv_t));
    self->owner = (PyObject *)self;

    return 0;
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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_point_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_point_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_point_t));
    self->owner = (PyObject *)self;

    return 0;
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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_area_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_area_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_area_t));
    self->owner = (PyObject *)self;

    return 0;
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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_disp_buf_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_disp_buf_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_disp_buf_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_disp_buf_t_getset[] = {
    {"buf1", (getter) struct_get_blob, (setter) struct_set_blob, "void buf1", (void*)offsetof(lv_disp_buf_t, buf1)},
    {"buf2", (getter) struct_get_blob, (setter) struct_set_blob, "void buf2", (void*)offsetof(lv_disp_buf_t, buf2)},
    {"buf_act", (getter) struct_get_blob, (setter) struct_set_blob, "void buf_act", (void*)offsetof(lv_disp_buf_t, buf_act)},
    {"size", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t size", (void*)offsetof(lv_disp_buf_t, size)},
    {"area", (getter) struct_get_blob, (setter) struct_set_blob, "lv_area_t area", (void*)offsetof(lv_disp_buf_t, area)},
    {"flushing", (getter) NULL, (setter) NULL, "uint32_t flushing", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_disp_drv_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_disp_drv_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_disp_drv_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_disp_drv_t_getset[] = {
    {"hor_res", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t hor_res", (void*)offsetof(lv_disp_drv_t, hor_res)},
    {"ver_res", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t ver_res", (void*)offsetof(lv_disp_drv_t, ver_res)},
    {"buffer", (getter) struct_get_blob, (setter) struct_set_blob, "lv_disp_buf_t buffer", (void*)offsetof(lv_disp_drv_t, buffer)},
    {"flush_cb", (getter) struct_get_blob, (setter) struct_set_blob, "void flush_cb(struct _disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) flush_cb", (void*)offsetof(lv_disp_drv_t, flush_cb)},
    {"rounder_cb", (getter) struct_get_blob, (setter) struct_set_blob, "void rounder_cb(struct _disp_drv_t *disp_drv, lv_area_t *area) rounder_cb", (void*)offsetof(lv_disp_drv_t, rounder_cb)},
    {"set_px_cb", (getter) struct_get_blob, (setter) struct_set_blob, "void set_px_cb(struct _disp_drv_t *disp_drv, uint8_t *buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa) set_px_cb", (void*)offsetof(lv_disp_drv_t, set_px_cb)},
    {"monitor_cb", (getter) struct_get_blob, (setter) struct_set_blob, "void monitor_cb(struct _disp_drv_t *disp_drv, uint32_t time, uint32_t px) monitor_cb", (void*)offsetof(lv_disp_drv_t, monitor_cb)},
    {"user_data", (getter) struct_get_blob, (setter) struct_set_blob, "lv_disp_drv_user_data_t user_data", (void*)offsetof(lv_disp_drv_t, user_data)},
    {"mem_blend", (getter) struct_get_blob, (setter) struct_set_blob, "void mem_blend(lv_color_t *dest, const lv_color_t *src, uint32_t length, lv_opa_t opa) mem_blend", (void*)offsetof(lv_disp_drv_t, mem_blend)},
    {"mem_fill", (getter) struct_get_blob, (setter) struct_set_blob, "void mem_fill(lv_color_t *dest, uint32_t length, lv_color_t color) mem_fill", (void*)offsetof(lv_disp_drv_t, mem_fill)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_disp_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_disp_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_disp_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_disp_t_getset[] = {
    {"driver", (getter) struct_get_blob, (setter) struct_set_blob, "lv_disp_drv_t driver", (void*)offsetof(lv_disp_t, driver)},
    {"scr_ll", (getter) struct_get_blob, (setter) struct_set_blob, "lv_ll_t scr_ll", (void*)offsetof(lv_disp_t, scr_ll)},
    {"act_scr", (getter) struct_get_blob, (setter) struct_set_blob, "struct _lv_obj_t act_scr", (void*)offsetof(lv_disp_t, act_scr)},
    {"top_layer", (getter) struct_get_blob, (setter) struct_set_blob, "struct _lv_obj_t top_layer", (void*)offsetof(lv_disp_t, top_layer)},
    {"sys_layer", (getter) struct_get_blob, (setter) struct_set_blob, "struct _lv_obj_t sys_layer", (void*)offsetof(lv_disp_t, sys_layer)},
    {"inv_areas", (getter) struct_get_blob, (setter) struct_set_blob, "lv_area_t32 inv_areas", (void*)offsetof(lv_disp_t, inv_areas)},
    {"inv_area_joined", (getter) struct_get_blob, (setter) struct_set_blob, "uint8_t32 inv_area_joined", (void*)offsetof(lv_disp_t, inv_area_joined)},
    {"inv_p", (getter) NULL, (setter) NULL, "uint32_t inv_p", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_indev_data_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_indev_data_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_indev_data_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_indev_data_t_getset[] = {
    {"point", (getter) struct_get_blob, (setter) struct_set_blob, "lv_point_t point", (void*)offsetof(lv_indev_data_t, point)},
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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_indev_drv_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_indev_drv_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_indev_drv_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_indev_drv_t_getset[] = {
    {"type", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_hal_indev_type_t type", (void*)offsetof(lv_indev_drv_t, type)},
    {"read_cb", (getter) struct_get_blob, (setter) struct_set_blob, "bool read_cb(struct _lv_indev_drv_t *indev_drv, lv_indev_data_t *data) read_cb", (void*)offsetof(lv_indev_drv_t, read_cb)},
    {"user_data", (getter) struct_get_blob, (setter) struct_set_blob, "lv_indev_drv_user_data_t user_data", (void*)offsetof(lv_indev_drv_t, user_data)},
    {"disp", (getter) struct_get_blob, (setter) struct_set_blob, "struct _disp_t disp", (void*)offsetof(lv_indev_drv_t, disp)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_indev_proc_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_indev_proc_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_indev_proc_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_indev_proc_t_getset[] = {
    {"state", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_indev_state_t state", (void*)offsetof(lv_indev_proc_t, state)},
    {"types", (getter) struct_get_blob, (setter) struct_set_blob, "union  {   struct    {     lv_point_t act_point;     lv_point_t last_point;     lv_point_t vect;     lv_point_t drag_sum;     lv_point_t drag_throw_vect;     struct _lv_obj_t *act_obj;     struct _lv_obj_t *last_obj;     struct _lv_obj_t *last_pressed;     uint8_t drag_limit_out : 1;     uint8_t drag_in_prog : 1;     uint8_t wait_until_release : 1;   } pointer;   struct    {     lv_indev_state_t last_state;     uint32_t last_key;   } keypad; } types", (void*)offsetof(lv_indev_proc_t, types)},
    {"pr_timestamp", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t pr_timestamp", (void*)offsetof(lv_indev_proc_t, pr_timestamp)},
    {"longpr_rep_timestamp", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t longpr_rep_timestamp", (void*)offsetof(lv_indev_proc_t, longpr_rep_timestamp)},
    {"long_pr_sent", (getter) NULL, (setter) NULL, "uint8_t long_pr_sent", NULL},
    {"reset_query", (getter) NULL, (setter) NULL, "uint8_t reset_query", NULL},
    {"disabled", (getter) NULL, (setter) NULL, "uint8_t disabled", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_indev_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_indev_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_indev_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_indev_t_getset[] = {
    {"driver", (getter) struct_get_blob, (setter) struct_set_blob, "lv_indev_drv_t driver", (void*)offsetof(lv_indev_t, driver)},
    {"proc", (getter) struct_get_blob, (setter) struct_set_blob, "lv_indev_proc_t proc", (void*)offsetof(lv_indev_t, proc)},
    {"feedback", (getter) struct_get_blob, (setter) struct_set_blob, "lv_indev_feedback_t feedback", (void*)offsetof(lv_indev_t, feedback)},
    {"last_activity_time", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t last_activity_time", (void*)offsetof(lv_indev_t, last_activity_time)},
    {"cursor", (getter) struct_get_blob, (setter) struct_set_blob, "struct _lv_obj_t cursor", (void*)offsetof(lv_indev_t, cursor)},
    {"group", (getter) struct_get_blob, (setter) struct_set_blob, "struct _lv_group_t group", (void*)offsetof(lv_indev_t, group)},
    {"btn_points", (getter) struct_get_blob, (setter) struct_set_blob, "lv_point_t btn_points", (void*)offsetof(lv_indev_t, btn_points)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_font_glyph_dsc_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_font_glyph_dsc_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_font_glyph_dsc_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_font_glyph_dsc_t_getset[] = {
    {"w_px", (getter) NULL, (setter) NULL, "uint32_t w_px", NULL},
    {"glyph_index", (getter) NULL, (setter) NULL, "uint32_t glyph_index", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_font_unicode_map_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_font_unicode_map_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_font_unicode_map_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_font_unicode_map_t_getset[] = {
    {"unicode", (getter) NULL, (setter) NULL, "uint32_t unicode", NULL},
    {"glyph_dsc_index", (getter) NULL, (setter) NULL, "uint32_t glyph_dsc_index", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_font_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_font_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_font_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_font_t_getset[] = {
    {"unicode_first", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t unicode_first", (void*)offsetof(lv_font_t, unicode_first)},
    {"unicode_last", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t unicode_last", (void*)offsetof(lv_font_t, unicode_last)},
    {"glyph_bitmap", (getter) struct_get_blob, (setter) struct_set_blob, "uint8_t glyph_bitmap", (void*)offsetof(lv_font_t, glyph_bitmap)},
    {"glyph_dsc", (getter) struct_get_blob, (setter) struct_set_blob, "lv_font_glyph_dsc_t glyph_dsc", (void*)offsetof(lv_font_t, glyph_dsc)},
    {"unicode_list", (getter) struct_get_blob, (setter) struct_set_blob, "uint32_t unicode_list", (void*)offsetof(lv_font_t, unicode_list)},
    {"get_bitmap", (getter) struct_get_blob, (setter) struct_set_blob, "const uint8_t *get_bitmap(const struct _lv_font_struct *, uint32_t) get_bitmap", (void*)offsetof(lv_font_t, get_bitmap)},
    {"get_width", (getter) struct_get_blob, (setter) struct_set_blob, "int16_t get_width(const struct _lv_font_struct *, uint32_t) get_width", (void*)offsetof(lv_font_t, get_width)},
    {"next_page", (getter) struct_get_blob, (setter) struct_set_blob, "struct _lv_font_struct next_page", (void*)offsetof(lv_font_t, next_page)},
    {"h_px", (getter) NULL, (setter) NULL, "uint32_t h_px", NULL},
    {"bpp", (getter) NULL, (setter) NULL, "uint32_t bpp", NULL},
    {"monospace", (getter) NULL, (setter) NULL, "uint32_t monospace", NULL},
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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_anim_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_anim_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_anim_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_anim_t_getset[] = {
    {"var", (getter) struct_get_blob, (setter) struct_set_blob, "void var", (void*)offsetof(lv_anim_t, var)},
    {"fp", (getter) struct_get_blob, (setter) struct_set_blob, "lv_anim_fp_t fp", (void*)offsetof(lv_anim_t, fp)},
    {"end_cb", (getter) struct_get_blob, (setter) struct_set_blob, "lv_anim_cb_t end_cb", (void*)offsetof(lv_anim_t, end_cb)},
    {"path", (getter) struct_get_blob, (setter) struct_set_blob, "lv_anim_path_t path", (void*)offsetof(lv_anim_t, path)},
    {"start", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t start", (void*)offsetof(lv_anim_t, start)},
    {"end", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t end", (void*)offsetof(lv_anim_t, end)},
    {"time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t time", (void*)offsetof(lv_anim_t, time)},
    {"act_time", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t act_time", (void*)offsetof(lv_anim_t, act_time)},
    {"playback_pause", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t playback_pause", (void*)offsetof(lv_anim_t, playback_pause)},
    {"repeat_pause", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t repeat_pause", (void*)offsetof(lv_anim_t, repeat_pause)},
    {"playback", (getter) NULL, (setter) NULL, "uint8_t playback", NULL},
    {"repeat", (getter) NULL, (setter) NULL, "uint8_t repeat", NULL},
    {"playback_now", (getter) NULL, (setter) NULL, "uint8_t playback_now", NULL},
    {"has_run", (getter) NULL, (setter) NULL, "uint32_t has_run", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_style_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_style_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_style_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_style_t_getset[] = {
    {"glass", (getter) NULL, (setter) NULL, "uint8_t glass", NULL},
    {"body", (getter) struct_get_blob, (setter) struct_set_blob, "struct  {   lv_color_t main_color;   lv_color_t grad_color;   lv_coord_t radius;   lv_opa_t opa;   struct    {     lv_color_t color;     lv_coord_t width;     lv_border_part_t part;     lv_opa_t opa;   } border;   struct    {     lv_color_t color;     lv_coord_t width;     lv_shadow_type_t type;   } shadow;   struct    {     lv_coord_t ver;     lv_coord_t hor;     lv_coord_t inner;   } padding; } body", (void*)offsetof(lv_style_t, body)},
    {"text", (getter) struct_get_blob, (setter) struct_set_blob, "struct  {   lv_color_t color;   const lv_font_t *font;   lv_coord_t letter_space;   lv_coord_t line_space;   lv_opa_t opa; } text", (void*)offsetof(lv_style_t, text)},
    {"image", (getter) struct_get_blob, (setter) struct_set_blob, "struct  {   lv_color_t color;   lv_opa_t intense;   lv_opa_t opa; } image", (void*)offsetof(lv_style_t, image)},
    {"line", (getter) struct_get_blob, (setter) struct_set_blob, "struct  {   lv_color_t color;   lv_coord_t width;   lv_opa_t opa;   uint8_t rounded : 1; } line", (void*)offsetof(lv_style_t, line)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_style_anim_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_style_anim_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_style_anim_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_style_anim_t_getset[] = {
    {"style_start", (getter) struct_get_blob, (setter) struct_set_blob, "lv_style_t style_start", (void*)offsetof(lv_style_anim_t, style_start)},
    {"style_end", (getter) struct_get_blob, (setter) struct_set_blob, "lv_style_t style_end", (void*)offsetof(lv_style_anim_t, style_end)},
    {"style_anim", (getter) struct_get_blob, (setter) struct_set_blob, "lv_style_t style_anim", (void*)offsetof(lv_style_anim_t, style_anim)},
    {"end_cb", (getter) struct_get_blob, (setter) struct_set_blob, "lv_anim_cb_t end_cb", (void*)offsetof(lv_style_anim_t, end_cb)},
    {"time", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t time", (void*)offsetof(lv_style_anim_t, time)},
    {"act_time", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t act_time", (void*)offsetof(lv_style_anim_t, act_time)},
    {"playback_pause", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t playback_pause", (void*)offsetof(lv_style_anim_t, playback_pause)},
    {"repeat_pause", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t repeat_pause", (void*)offsetof(lv_style_anim_t, repeat_pause)},
    {"playback", (getter) NULL, (setter) NULL, "uint8_t playback", NULL},
    {"repeat", (getter) NULL, (setter) NULL, "uint8_t repeat", NULL},

    {NULL}
};

static PyTypeObject pylv_style_anim_t_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.style_anim_t",
    .tp_doc = "lvgl style_anim_t",
    .tp_basicsize = sizeof(StructObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_style_anim_t_init,
    .tp_dealloc = (destructor) Struct_dealloc,
    .tp_getset = pylv_style_anim_t_getset,
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_obj_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_obj_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_obj_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_obj_t_getset[] = {
    {"par", (getter) struct_get_blob, (setter) struct_set_blob, "struct _lv_obj_t par", (void*)offsetof(lv_obj_t, par)},
    {"child_ll", (getter) struct_get_blob, (setter) struct_set_blob, "lv_ll_t child_ll", (void*)offsetof(lv_obj_t, child_ll)},
    {"coords", (getter) struct_get_blob, (setter) struct_set_blob, "lv_area_t coords", (void*)offsetof(lv_obj_t, coords)},
    {"event_cb", (getter) struct_get_blob, (setter) struct_set_blob, "lv_event_cb_t event_cb", (void*)offsetof(lv_obj_t, event_cb)},
    {"signal_cb", (getter) struct_get_blob, (setter) struct_set_blob, "lv_signal_cb_t signal_cb", (void*)offsetof(lv_obj_t, signal_cb)},
    {"design_cb", (getter) struct_get_blob, (setter) struct_set_blob, "lv_design_cb_t design_cb", (void*)offsetof(lv_obj_t, design_cb)},
    {"ext_attr", (getter) struct_get_blob, (setter) struct_set_blob, "void ext_attr", (void*)offsetof(lv_obj_t, ext_attr)},
    {"style_p", (getter) struct_get_blob, (setter) struct_set_blob, "lv_style_t style_p", (void*)offsetof(lv_obj_t, style_p)},
    {"group_p", (getter) struct_get_blob, (setter) struct_set_blob, "void group_p", (void*)offsetof(lv_obj_t, group_p)},
    {"click", (getter) NULL, (setter) NULL, "uint8_t click", NULL},
    {"drag", (getter) NULL, (setter) NULL, "uint8_t drag", NULL},
    {"drag_throw", (getter) NULL, (setter) NULL, "uint8_t drag_throw", NULL},
    {"drag_parent", (getter) NULL, (setter) NULL, "uint8_t drag_parent", NULL},
    {"hidden", (getter) NULL, (setter) NULL, "uint8_t hidden", NULL},
    {"top", (getter) NULL, (setter) NULL, "uint8_t top", NULL},
    {"opa_scale_en", (getter) NULL, (setter) NULL, "uint8_t opa_scale_en", NULL},
    {"parent_event", (getter) NULL, (setter) NULL, "uint8_t parent_event", NULL},
    {"protect", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t protect", (void*)offsetof(lv_obj_t, protect)},
    {"opa_scale", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_opa_t opa_scale", (void*)offsetof(lv_obj_t, opa_scale)},
    {"ext_size", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t ext_size", (void*)offsetof(lv_obj_t, ext_size)},
    {"user_data", (getter) struct_get_blob, (setter) struct_set_blob, "lv_obj_user_data_t user_data", (void*)offsetof(lv_obj_t, user_data)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_obj_type_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_obj_type_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_obj_type_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_obj_type_t_getset[] = {
    {"type", (getter) struct_get_blob, (setter) struct_set_blob, "char8 type", (void*)offsetof(lv_obj_type_t, type)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_group_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_group_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_group_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_group_t_getset[] = {
    {"obj_ll", (getter) struct_get_blob, (setter) struct_set_blob, "lv_ll_t obj_ll", (void*)offsetof(lv_group_t, obj_ll)},
    {"obj_focus", (getter) struct_get_blob, (setter) struct_set_blob, "lv_obj_t obj_focus", (void*)offsetof(lv_group_t, obj_focus)},
    {"style_mod", (getter) struct_get_blob, (setter) struct_set_blob, "lv_group_style_mod_func_t style_mod", (void*)offsetof(lv_group_t, style_mod)},
    {"style_mod_edit", (getter) struct_get_blob, (setter) struct_set_blob, "lv_group_style_mod_func_t style_mod_edit", (void*)offsetof(lv_group_t, style_mod_edit)},
    {"focus_cb", (getter) struct_get_blob, (setter) struct_set_blob, "lv_group_focus_cb_t focus_cb", (void*)offsetof(lv_group_t, focus_cb)},
    {"style_tmp", (getter) struct_get_blob, (setter) struct_set_blob, "lv_style_t style_tmp", (void*)offsetof(lv_group_t, style_tmp)},
    {"user_data", (getter) struct_get_blob, (setter) struct_set_blob, "lv_group_user_data_t user_data", (void*)offsetof(lv_group_t, user_data)},
    {"frozen", (getter) NULL, (setter) NULL, "uint8_t frozen", NULL},
    {"editing", (getter) NULL, (setter) NULL, "uint8_t editing", NULL},
    {"click_focus", (getter) NULL, (setter) NULL, "uint8_t click_focus", NULL},
    {"refocus_policy", (getter) NULL, (setter) NULL, "uint8_t refocus_policy", NULL},
    {"wrap", (getter) NULL, (setter) NULL, "uint8_t wrap", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_theme_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_theme_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_theme_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_theme_t_getset[] = {
    {"style", (getter) struct_get_blob, (setter) struct_set_blob, "struct  {   lv_style_t *bg;   lv_style_t *panel;   lv_style_t *cont;   struct    {     lv_style_t *rel;     lv_style_t *pr;     lv_style_t *tgl_rel;     lv_style_t *tgl_pr;     lv_style_t *ina;   } btn;   struct    {     lv_style_t *rel;     lv_style_t *pr;     lv_style_t *tgl_rel;     lv_style_t *tgl_pr;     lv_style_t *ina;   } imgbtn;   struct    {     lv_style_t *prim;     lv_style_t *sec;     lv_style_t *hint;   } label;   struct    {     lv_style_t *light;     lv_style_t *dark;   } img;   struct    {     lv_style_t *decor;   } line;   lv_style_t *led;   struct    {     lv_style_t *bg;     lv_style_t *indic;   } bar;   struct    {     lv_style_t *bg;     lv_style_t *indic;     lv_style_t *knob;   } slider;   lv_style_t *lmeter;   lv_style_t *gauge;   lv_style_t *arc;   lv_style_t *preload;   struct    {     lv_style_t *bg;     lv_style_t *indic;     lv_style_t *knob_off;     lv_style_t *knob_on;   } sw;   lv_style_t *chart;   struct    {     lv_style_t *bg;     struct      {       lv_style_t *rel;       lv_style_t *pr;       lv_style_t *tgl_rel;       lv_style_t *tgl_pr;       lv_style_t *ina;     } box;   } cb;   struct    {     lv_style_t *bg;     struct      {       lv_style_t *rel;       lv_style_t *pr;       lv_style_t *tgl_rel;       lv_style_t *tgl_pr;       lv_style_t *ina;     } btn;   } btnm;   struct    {     lv_style_t *bg;     struct      {       lv_style_t *rel;       lv_style_t *pr;       lv_style_t *tgl_rel;       lv_style_t *tgl_pr;       lv_style_t *ina;     } btn;   } kb;   struct    {     lv_style_t *bg;     struct      {       lv_style_t *bg;       lv_style_t *rel;       lv_style_t *pr;     } btn;   } mbox;   struct    {     lv_style_t *bg;     lv_style_t *scrl;     lv_style_t *sb;   } page;   struct    {     lv_style_t *area;     lv_style_t *oneline;     lv_style_t *cursor;     lv_style_t *sb;   } ta;   struct    {     lv_style_t *bg;     lv_style_t *cursor;     lv_style_t *sb;   } spinbox;   struct    {     lv_style_t *bg;     lv_style_t *scrl;     lv_style_t *sb;     struct      {       lv_style_t *rel;       lv_style_t *pr;       lv_style_t *tgl_rel;       lv_style_t *tgl_pr;       lv_style_t *ina;     } btn;   } list;   struct    {     lv_style_t *bg;     lv_style_t *sel;     lv_style_t *sb;   } ddlist;   struct    {     lv_style_t *bg;     lv_style_t *sel;   } roller;   struct    {     lv_style_t *bg;     lv_style_t *indic;     struct      {       lv_style_t *bg;       lv_style_t *rel;       lv_style_t *pr;       lv_style_t *tgl_rel;       lv_style_t *tgl_pr;     } btn;   } tabview;   struct    {     lv_style_t *bg;     lv_style_t *scrl;     lv_style_t *sb;   } tileview;   struct    {     lv_style_t *bg;     lv_style_t *cell;   } table;   struct    {     lv_style_t *bg;     lv_style_t *sb;     lv_style_t *header;     struct      {       lv_style_t *bg;       lv_style_t *scrl;     } content;     struct      {       lv_style_t *rel;       lv_style_t *pr;     } btn;   } win; } style", (void*)offsetof(lv_theme_t, style)},
    {"group", (getter) struct_get_blob, (setter) struct_set_blob, "struct  {   lv_group_style_mod_func_t style_mod;   lv_group_style_mod_func_t style_mod_edit; } group", (void*)offsetof(lv_theme_t, group)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_cont_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_cont_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_cont_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_cont_ext_t_getset[] = {
    {"layout", (getter) NULL, (setter) NULL, "uint8_t layout", NULL},
    {"fit_left", (getter) NULL, (setter) NULL, "uint8_t fit_left", NULL},
    {"fit_right", (getter) NULL, (setter) NULL, "uint8_t fit_right", NULL},
    {"fit_top", (getter) NULL, (setter) NULL, "uint8_t fit_top", NULL},
    {"fit_bottom", (getter) NULL, (setter) NULL, "uint8_t fit_bottom", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_btn_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_btn_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_btn_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_btn_ext_t_getset[] = {
    {"cont", (getter) struct_get_blob, (setter) struct_set_blob, "lv_cont_ext_t cont", (void*)offsetof(lv_btn_ext_t, cont)},
    {"styles", (getter) struct_get_blob, (setter) struct_set_blob, "lv_style_tLV_BTN_STATE_NUM styles", (void*)offsetof(lv_btn_ext_t, styles)},
    {"state", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_btn_state_t state", (void*)offsetof(lv_btn_ext_t, state)},
    {"ink_in_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t ink_in_time", (void*)offsetof(lv_btn_ext_t, ink_in_time)},
    {"ink_wait_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t ink_wait_time", (void*)offsetof(lv_btn_ext_t, ink_wait_time)},
    {"ink_out_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t ink_out_time", (void*)offsetof(lv_btn_ext_t, ink_out_time)},
    {"toggle", (getter) NULL, (setter) NULL, "uint8_t toggle", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_img_header_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_img_header_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_img_header_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_img_header_t_getset[] = {
    {"cf", (getter) NULL, (setter) NULL, "uint32_t cf", NULL},
    {"always_zero", (getter) NULL, (setter) NULL, "uint32_t always_zero", NULL},
    {"reserved", (getter) NULL, (setter) NULL, "uint32_t reserved", NULL},
    {"w", (getter) NULL, (setter) NULL, "uint32_t w", NULL},
    {"h", (getter) NULL, (setter) NULL, "uint32_t h", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_img_dsc_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_img_dsc_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_img_dsc_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_img_dsc_t_getset[] = {
    {"header", (getter) struct_get_blob, (setter) struct_set_blob, "lv_img_header_t header", (void*)offsetof(lv_img_dsc_t, header)},
    {"data_size", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t data_size", (void*)offsetof(lv_img_dsc_t, data_size)},
    {"data", (getter) struct_get_blob, (setter) struct_set_blob, "uint8_t data", (void*)offsetof(lv_img_dsc_t, data)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_imgbtn_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_imgbtn_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_imgbtn_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_imgbtn_ext_t_getset[] = {
    {"btn", (getter) struct_get_blob, (setter) struct_set_blob, "lv_btn_ext_t btn", (void*)offsetof(lv_imgbtn_ext_t, btn)},
    {"img_src", (getter) struct_get_blob, (setter) struct_set_blob, "voidLV_BTN_STATE_NUM img_src", (void*)offsetof(lv_imgbtn_ext_t, img_src)},
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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_fs_file_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_fs_file_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_fs_file_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_fs_file_t_getset[] = {
    {"file_d", (getter) struct_get_blob, (setter) struct_set_blob, "void file_d", (void*)offsetof(lv_fs_file_t, file_d)},
    {"drv", (getter) struct_get_blob, (setter) struct_set_blob, "struct __lv_fs_drv_t drv", (void*)offsetof(lv_fs_file_t, drv)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_fs_dir_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_fs_dir_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_fs_dir_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_fs_dir_t_getset[] = {
    {"dir_d", (getter) struct_get_blob, (setter) struct_set_blob, "void dir_d", (void*)offsetof(lv_fs_dir_t, dir_d)},
    {"drv", (getter) struct_get_blob, (setter) struct_set_blob, "struct __lv_fs_drv_t drv", (void*)offsetof(lv_fs_dir_t, drv)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_fs_drv_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_fs_drv_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_fs_drv_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_fs_drv_t_getset[] = {
    {"letter", (getter) struct_get_blob, (setter) struct_set_blob, "char letter", (void*)offsetof(lv_fs_drv_t, letter)},
    {"file_size", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t file_size", (void*)offsetof(lv_fs_drv_t, file_size)},
    {"rddir_size", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t rddir_size", (void*)offsetof(lv_fs_drv_t, rddir_size)},
    {"ready", (getter) struct_get_blob, (setter) struct_set_blob, "bool ready(void) ready", (void*)offsetof(lv_fs_drv_t, ready)},
    {"open", (getter) struct_get_blob, (setter) struct_set_blob, "lv_fs_res_t open(void *file_p, const char *path, lv_fs_mode_t mode) open", (void*)offsetof(lv_fs_drv_t, open)},
    {"close", (getter) struct_get_blob, (setter) struct_set_blob, "lv_fs_res_t close(void *file_p) close", (void*)offsetof(lv_fs_drv_t, close)},
    {"remove", (getter) struct_get_blob, (setter) struct_set_blob, "lv_fs_res_t remove(const char *fn) remove", (void*)offsetof(lv_fs_drv_t, remove)},
    {"read", (getter) struct_get_blob, (setter) struct_set_blob, "lv_fs_res_t read(void *file_p, void *buf, uint32_t btr, uint32_t *br) read", (void*)offsetof(lv_fs_drv_t, read)},
    {"write", (getter) struct_get_blob, (setter) struct_set_blob, "lv_fs_res_t write(void *file_p, const void *buf, uint32_t btw, uint32_t *bw) write", (void*)offsetof(lv_fs_drv_t, write)},
    {"seek", (getter) struct_get_blob, (setter) struct_set_blob, "lv_fs_res_t seek(void *file_p, uint32_t pos) seek", (void*)offsetof(lv_fs_drv_t, seek)},
    {"tell", (getter) struct_get_blob, (setter) struct_set_blob, "lv_fs_res_t tell(void *file_p, uint32_t *pos_p) tell", (void*)offsetof(lv_fs_drv_t, tell)},
    {"trunc", (getter) struct_get_blob, (setter) struct_set_blob, "lv_fs_res_t trunc(void *file_p) trunc", (void*)offsetof(lv_fs_drv_t, trunc)},
    {"size", (getter) struct_get_blob, (setter) struct_set_blob, "lv_fs_res_t size(void *file_p, uint32_t *size_p) size", (void*)offsetof(lv_fs_drv_t, size)},
    {"rename", (getter) struct_get_blob, (setter) struct_set_blob, "lv_fs_res_t rename(const char *oldname, const char *newname) rename", (void*)offsetof(lv_fs_drv_t, rename)},
    {"free_space", (getter) struct_get_blob, (setter) struct_set_blob, "lv_fs_res_t free_space(uint32_t *total_p, uint32_t *free_p) free_space", (void*)offsetof(lv_fs_drv_t, free_space)},
    {"dir_open", (getter) struct_get_blob, (setter) struct_set_blob, "lv_fs_res_t dir_open(void *rddir_p, const char *path) dir_open", (void*)offsetof(lv_fs_drv_t, dir_open)},
    {"dir_read", (getter) struct_get_blob, (setter) struct_set_blob, "lv_fs_res_t dir_read(void *rddir_p, char *fn) dir_read", (void*)offsetof(lv_fs_drv_t, dir_read)},
    {"dir_close", (getter) struct_get_blob, (setter) struct_set_blob, "lv_fs_res_t dir_close(void *rddir_p) dir_close", (void*)offsetof(lv_fs_drv_t, dir_close)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_label_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_label_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_label_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_label_ext_t_getset[] = {
    {"text", (getter) struct_get_blob, (setter) struct_set_blob, "char text", (void*)offsetof(lv_label_ext_t, text)},
    {"long_mode", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_label_long_mode_t long_mode", (void*)offsetof(lv_label_ext_t, long_mode)},
    {"dot_tmp", (getter) struct_get_blob, (setter) struct_set_blob, "char(3 * 4) + 1 dot_tmp", (void*)offsetof(lv_label_ext_t, dot_tmp)},
    {"dot_end", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t dot_end", (void*)offsetof(lv_label_ext_t, dot_end)},
    {"anim_speed", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t anim_speed", (void*)offsetof(lv_label_ext_t, anim_speed)},
    {"offset", (getter) struct_get_blob, (setter) struct_set_blob, "lv_point_t offset", (void*)offsetof(lv_label_ext_t, offset)},
    {"static_txt", (getter) NULL, (setter) NULL, "uint8_t static_txt", NULL},
    {"align", (getter) NULL, (setter) NULL, "uint8_t align", NULL},
    {"recolor", (getter) NULL, (setter) NULL, "uint8_t recolor", NULL},
    {"expand", (getter) NULL, (setter) NULL, "uint8_t expand", NULL},
    {"body_draw", (getter) NULL, (setter) NULL, "uint8_t body_draw", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_img_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_img_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_img_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_img_ext_t_getset[] = {
    {"src", (getter) struct_get_blob, (setter) struct_set_blob, "void src", (void*)offsetof(lv_img_ext_t, src)},
    {"offset", (getter) struct_get_blob, (setter) struct_set_blob, "lv_point_t offset", (void*)offsetof(lv_img_ext_t, offset)},
    {"w", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t w", (void*)offsetof(lv_img_ext_t, w)},
    {"h", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t h", (void*)offsetof(lv_img_ext_t, h)},
    {"src_type", (getter) NULL, (setter) NULL, "uint8_t src_type", NULL},
    {"auto_size", (getter) NULL, (setter) NULL, "uint8_t auto_size", NULL},
    {"cf", (getter) NULL, (setter) NULL, "uint8_t cf", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_line_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_line_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_line_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_line_ext_t_getset[] = {
    {"point_array", (getter) struct_get_blob, (setter) struct_set_blob, "lv_point_t point_array", (void*)offsetof(lv_line_ext_t, point_array)},
    {"point_num", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t point_num", (void*)offsetof(lv_line_ext_t, point_num)},
    {"auto_size", (getter) NULL, (setter) NULL, "uint8_t auto_size", NULL},
    {"y_inv", (getter) NULL, (setter) NULL, "uint8_t y_inv", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_page_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_page_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_page_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_page_ext_t_getset[] = {
    {"bg", (getter) struct_get_blob, (setter) struct_set_blob, "lv_cont_ext_t bg", (void*)offsetof(lv_page_ext_t, bg)},
    {"scrl", (getter) struct_get_blob, (setter) struct_set_blob, "lv_obj_t scrl", (void*)offsetof(lv_page_ext_t, scrl)},
    {"sb", (getter) struct_get_blob, (setter) struct_set_blob, "struct  {   lv_style_t *style;   lv_area_t hor_area;   lv_area_t ver_area;   uint8_t hor_draw : 1;   uint8_t ver_draw : 1;   lv_sb_mode_t mode : 3; } sb", (void*)offsetof(lv_page_ext_t, sb)},
    {"edge_flash", (getter) struct_get_blob, (setter) struct_set_blob, "struct  {   uint16_t state;   lv_style_t *style;   uint8_t enabled : 1;   uint8_t top_ip : 1;   uint8_t bottom_ip : 1;   uint8_t right_ip : 1;   uint8_t left_ip : 1; } edge_flash", (void*)offsetof(lv_page_ext_t, edge_flash)},
    {"arrow_scroll", (getter) NULL, (setter) NULL, "uint8_t arrow_scroll", NULL},
    {"scroll_prop", (getter) NULL, (setter) NULL, "uint8_t scroll_prop", NULL},
    {"scroll_prop_ip", (getter) NULL, (setter) NULL, "uint8_t scroll_prop_ip", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_list_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_list_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_list_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_list_ext_t_getset[] = {
    {"page", (getter) struct_get_blob, (setter) struct_set_blob, "lv_page_ext_t page", (void*)offsetof(lv_list_ext_t, page)},
    {"anim_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t anim_time", (void*)offsetof(lv_list_ext_t, anim_time)},
    {"styles_btn", (getter) struct_get_blob, (setter) struct_set_blob, "lv_style_tLV_BTN_STATE_NUM styles_btn", (void*)offsetof(lv_list_ext_t, styles_btn)},
    {"style_img", (getter) struct_get_blob, (setter) struct_set_blob, "lv_style_t style_img", (void*)offsetof(lv_list_ext_t, style_img)},
    {"size", (getter) struct_get_uint32, (setter) struct_set_uint32, "uint32_t size", (void*)offsetof(lv_list_ext_t, size)},
    {"single_mode", (getter) struct_get_blob, (setter) struct_set_blob, "bool single_mode", (void*)offsetof(lv_list_ext_t, single_mode)},
    {"last_sel", (getter) struct_get_blob, (setter) struct_set_blob, "lv_obj_t last_sel", (void*)offsetof(lv_list_ext_t, last_sel)},
    {"selected_btn", (getter) struct_get_blob, (setter) struct_set_blob, "lv_obj_t selected_btn", (void*)offsetof(lv_list_ext_t, selected_btn)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_chart_series_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_chart_series_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_chart_series_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_chart_series_t_getset[] = {
    {"points", (getter) struct_get_blob, (setter) struct_set_blob, "lv_coord_t points", (void*)offsetof(lv_chart_series_t, points)},
    {"color", (getter) struct_get_blob, (setter) struct_set_blob, "lv_color_t color", (void*)offsetof(lv_chart_series_t, color)},
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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_chart_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_chart_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_chart_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_chart_ext_t_getset[] = {
    {"series_ll", (getter) struct_get_blob, (setter) struct_set_blob, "lv_ll_t series_ll", (void*)offsetof(lv_chart_ext_t, series_ll)},
    {"ymin", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t ymin", (void*)offsetof(lv_chart_ext_t, ymin)},
    {"ymax", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t ymax", (void*)offsetof(lv_chart_ext_t, ymax)},
    {"hdiv_cnt", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t hdiv_cnt", (void*)offsetof(lv_chart_ext_t, hdiv_cnt)},
    {"vdiv_cnt", (getter) struct_get_uint8, (setter) struct_set_uint8, "uint8_t vdiv_cnt", (void*)offsetof(lv_chart_ext_t, vdiv_cnt)},
    {"point_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t point_cnt", (void*)offsetof(lv_chart_ext_t, point_cnt)},
    {"type", (getter) NULL, (setter) NULL, "uint8_t type", NULL},
    {"series", (getter) struct_get_blob, (setter) struct_set_blob, "struct  {   lv_coord_t width;   uint8_t num;   lv_opa_t opa;   lv_opa_t dark; } series", (void*)offsetof(lv_chart_ext_t, series)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_table_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_table_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_table_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_table_ext_t_getset[] = {
    {"col_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t col_cnt", (void*)offsetof(lv_table_ext_t, col_cnt)},
    {"row_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t row_cnt", (void*)offsetof(lv_table_ext_t, row_cnt)},
    {"cell_data", (getter) struct_get_blob, (setter) struct_set_blob, "char cell_data", (void*)offsetof(lv_table_ext_t, cell_data)},
    {"cell_style", (getter) struct_get_blob, (setter) struct_set_blob, "lv_style_t4 cell_style", (void*)offsetof(lv_table_ext_t, cell_style)},
    {"col_w", (getter) struct_get_blob, (setter) struct_set_blob, "lv_coord_t12 col_w", (void*)offsetof(lv_table_ext_t, col_w)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_cb_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_cb_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_cb_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_cb_ext_t_getset[] = {
    {"bg_btn", (getter) struct_get_blob, (setter) struct_set_blob, "lv_btn_ext_t bg_btn", (void*)offsetof(lv_cb_ext_t, bg_btn)},
    {"bullet", (getter) struct_get_blob, (setter) struct_set_blob, "lv_obj_t bullet", (void*)offsetof(lv_cb_ext_t, bullet)},
    {"label", (getter) struct_get_blob, (setter) struct_set_blob, "lv_obj_t label", (void*)offsetof(lv_cb_ext_t, label)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_bar_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_bar_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_bar_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_bar_ext_t_getset[] = {
    {"cur_value", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t cur_value", (void*)offsetof(lv_bar_ext_t, cur_value)},
    {"min_value", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t min_value", (void*)offsetof(lv_bar_ext_t, min_value)},
    {"max_value", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t max_value", (void*)offsetof(lv_bar_ext_t, max_value)},
    {"anim_start", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t anim_start", (void*)offsetof(lv_bar_ext_t, anim_start)},
    {"anim_end", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t anim_end", (void*)offsetof(lv_bar_ext_t, anim_end)},
    {"anim_state", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t anim_state", (void*)offsetof(lv_bar_ext_t, anim_state)},
    {"anim_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t anim_time", (void*)offsetof(lv_bar_ext_t, anim_time)},
    {"sym", (getter) NULL, (setter) NULL, "uint8_t sym", NULL},
    {"style_indic", (getter) struct_get_blob, (setter) struct_set_blob, "lv_style_t style_indic", (void*)offsetof(lv_bar_ext_t, style_indic)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_slider_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_slider_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_slider_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_slider_ext_t_getset[] = {
    {"bar", (getter) struct_get_blob, (setter) struct_set_blob, "lv_bar_ext_t bar", (void*)offsetof(lv_slider_ext_t, bar)},
    {"style_knob", (getter) struct_get_blob, (setter) struct_set_blob, "lv_style_t style_knob", (void*)offsetof(lv_slider_ext_t, style_knob)},
    {"drag_value", (getter) struct_get_int16, (setter) struct_set_int16, "int16_t drag_value", (void*)offsetof(lv_slider_ext_t, drag_value)},
    {"knob_in", (getter) NULL, (setter) NULL, "uint8_t knob_in", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_led_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_led_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_led_ext_t));
    self->owner = (PyObject *)self;

    return 0;
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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_btnm_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_btnm_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_btnm_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_btnm_ext_t_getset[] = {
    {"map_p", (getter) struct_get_blob, (setter) struct_set_blob, "char map_p", (void*)offsetof(lv_btnm_ext_t, map_p)},
    {"button_areas", (getter) struct_get_blob, (setter) struct_set_blob, "lv_area_t button_areas", (void*)offsetof(lv_btnm_ext_t, button_areas)},
    {"ctrl_bits", (getter) struct_get_blob, (setter) struct_set_blob, "lv_btnm_ctrl_t ctrl_bits", (void*)offsetof(lv_btnm_ext_t, ctrl_bits)},
    {"styles_btn", (getter) struct_get_blob, (setter) struct_set_blob, "lv_style_tLV_BTN_STATE_NUM styles_btn", (void*)offsetof(lv_btnm_ext_t, styles_btn)},
    {"btn_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t btn_cnt", (void*)offsetof(lv_btnm_ext_t, btn_cnt)},
    {"btn_id_pr", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t btn_id_pr", (void*)offsetof(lv_btnm_ext_t, btn_id_pr)},
    {"btn_id_act", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t btn_id_act", (void*)offsetof(lv_btnm_ext_t, btn_id_act)},
    {"recolor", (getter) NULL, (setter) NULL, "uint8_t recolor", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_kb_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_kb_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_kb_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_kb_ext_t_getset[] = {
    {"btnm", (getter) struct_get_blob, (setter) struct_set_blob, "lv_btnm_ext_t btnm", (void*)offsetof(lv_kb_ext_t, btnm)},
    {"ta", (getter) struct_get_blob, (setter) struct_set_blob, "lv_obj_t ta", (void*)offsetof(lv_kb_ext_t, ta)},
    {"mode", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_kb_mode_t mode", (void*)offsetof(lv_kb_ext_t, mode)},
    {"cursor_mng", (getter) NULL, (setter) NULL, "uint8_t cursor_mng", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_ddlist_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_ddlist_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_ddlist_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_ddlist_ext_t_getset[] = {
    {"page", (getter) struct_get_blob, (setter) struct_set_blob, "lv_page_ext_t page", (void*)offsetof(lv_ddlist_ext_t, page)},
    {"label", (getter) struct_get_blob, (setter) struct_set_blob, "lv_obj_t label", (void*)offsetof(lv_ddlist_ext_t, label)},
    {"sel_style", (getter) struct_get_blob, (setter) struct_set_blob, "lv_style_t sel_style", (void*)offsetof(lv_ddlist_ext_t, sel_style)},
    {"option_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t option_cnt", (void*)offsetof(lv_ddlist_ext_t, option_cnt)},
    {"sel_opt_id", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t sel_opt_id", (void*)offsetof(lv_ddlist_ext_t, sel_opt_id)},
    {"sel_opt_id_ori", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t sel_opt_id_ori", (void*)offsetof(lv_ddlist_ext_t, sel_opt_id_ori)},
    {"anim_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t anim_time", (void*)offsetof(lv_ddlist_ext_t, anim_time)},
    {"opened", (getter) NULL, (setter) NULL, "uint8_t opened", NULL},
    {"draw_arrow", (getter) NULL, (setter) NULL, "uint8_t draw_arrow", NULL},
    {"stay_open", (getter) NULL, (setter) NULL, "uint8_t stay_open", NULL},
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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_roller_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_roller_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_roller_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_roller_ext_t_getset[] = {
    {"ddlist", (getter) struct_get_blob, (setter) struct_set_blob, "lv_ddlist_ext_t ddlist", (void*)offsetof(lv_roller_ext_t, ddlist)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_ta_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_ta_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_ta_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_ta_ext_t_getset[] = {
    {"page", (getter) struct_get_blob, (setter) struct_set_blob, "lv_page_ext_t page", (void*)offsetof(lv_ta_ext_t, page)},
    {"label", (getter) struct_get_blob, (setter) struct_set_blob, "lv_obj_t label", (void*)offsetof(lv_ta_ext_t, label)},
    {"placeholder", (getter) struct_get_blob, (setter) struct_set_blob, "lv_obj_t placeholder", (void*)offsetof(lv_ta_ext_t, placeholder)},
    {"pwd_tmp", (getter) struct_get_blob, (setter) struct_set_blob, "char pwd_tmp", (void*)offsetof(lv_ta_ext_t, pwd_tmp)},
    {"accapted_chars", (getter) struct_get_blob, (setter) struct_set_blob, "char accapted_chars", (void*)offsetof(lv_ta_ext_t, accapted_chars)},
    {"max_length", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t max_length", (void*)offsetof(lv_ta_ext_t, max_length)},
    {"pwd_mode", (getter) NULL, (setter) NULL, "uint8_t pwd_mode", NULL},
    {"one_line", (getter) NULL, (setter) NULL, "uint8_t one_line", NULL},
    {"cursor", (getter) struct_get_blob, (setter) struct_set_blob, "struct  {   lv_style_t *style;   lv_coord_t valid_x;   uint16_t pos;   lv_area_t area;   uint16_t txt_byte_pos;   lv_cursor_type_t type : 4;   uint8_t state : 1; } cursor", (void*)offsetof(lv_ta_ext_t, cursor)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_canvas_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_canvas_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_canvas_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_canvas_ext_t_getset[] = {
    {"img", (getter) struct_get_blob, (setter) struct_set_blob, "lv_img_ext_t img", (void*)offsetof(lv_canvas_ext_t, img)},
    {"dsc", (getter) struct_get_blob, (setter) struct_set_blob, "lv_img_dsc_t dsc", (void*)offsetof(lv_canvas_ext_t, dsc)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_win_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_win_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_win_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_win_ext_t_getset[] = {
    {"page", (getter) struct_get_blob, (setter) struct_set_blob, "lv_obj_t page", (void*)offsetof(lv_win_ext_t, page)},
    {"header", (getter) struct_get_blob, (setter) struct_set_blob, "lv_obj_t header", (void*)offsetof(lv_win_ext_t, header)},
    {"title", (getter) struct_get_blob, (setter) struct_set_blob, "lv_obj_t title", (void*)offsetof(lv_win_ext_t, title)},
    {"style_header", (getter) struct_get_blob, (setter) struct_set_blob, "lv_style_t style_header", (void*)offsetof(lv_win_ext_t, style_header)},
    {"style_btn_rel", (getter) struct_get_blob, (setter) struct_set_blob, "lv_style_t style_btn_rel", (void*)offsetof(lv_win_ext_t, style_btn_rel)},
    {"style_btn_pr", (getter) struct_get_blob, (setter) struct_set_blob, "lv_style_t style_btn_pr", (void*)offsetof(lv_win_ext_t, style_btn_pr)},
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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_tabview_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_tabview_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_tabview_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_tabview_ext_t_getset[] = {
    {"btns", (getter) struct_get_blob, (setter) struct_set_blob, "lv_obj_t btns", (void*)offsetof(lv_tabview_ext_t, btns)},
    {"indic", (getter) struct_get_blob, (setter) struct_set_blob, "lv_obj_t indic", (void*)offsetof(lv_tabview_ext_t, indic)},
    {"content", (getter) struct_get_blob, (setter) struct_set_blob, "lv_obj_t content", (void*)offsetof(lv_tabview_ext_t, content)},
    {"tab_name_ptr", (getter) struct_get_blob, (setter) struct_set_blob, "char tab_name_ptr", (void*)offsetof(lv_tabview_ext_t, tab_name_ptr)},
    {"point_last", (getter) struct_get_blob, (setter) struct_set_blob, "lv_point_t point_last", (void*)offsetof(lv_tabview_ext_t, point_last)},
    {"tab_cur", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t tab_cur", (void*)offsetof(lv_tabview_ext_t, tab_cur)},
    {"tab_cnt", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t tab_cnt", (void*)offsetof(lv_tabview_ext_t, tab_cnt)},
    {"anim_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t anim_time", (void*)offsetof(lv_tabview_ext_t, anim_time)},
    {"slide_enable", (getter) NULL, (setter) NULL, "uint8_t slide_enable", NULL},
    {"draging", (getter) NULL, (setter) NULL, "uint8_t draging", NULL},
    {"drag_hor", (getter) NULL, (setter) NULL, "uint8_t drag_hor", NULL},
    {"btns_hide", (getter) NULL, (setter) NULL, "uint8_t btns_hide", NULL},
    {"btns_pos", (getter) NULL, (setter) NULL, "lv_tabview_btns_pos_t btns_pos", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_tileview_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_tileview_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_tileview_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_tileview_ext_t_getset[] = {
    {"page", (getter) struct_get_blob, (setter) struct_set_blob, "lv_page_ext_t page", (void*)offsetof(lv_tileview_ext_t, page)},
    {"valid_pos", (getter) struct_get_blob, (setter) struct_set_blob, "lv_point_t valid_pos", (void*)offsetof(lv_tileview_ext_t, valid_pos)},
    {"anim_time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t anim_time", (void*)offsetof(lv_tileview_ext_t, anim_time)},
    {"act_id", (getter) struct_get_blob, (setter) struct_set_blob, "lv_point_t act_id", (void*)offsetof(lv_tileview_ext_t, act_id)},
    {"drag_top_en", (getter) NULL, (setter) NULL, "uint8_t drag_top_en", NULL},
    {"drag_bottom_en", (getter) NULL, (setter) NULL, "uint8_t drag_bottom_en", NULL},
    {"drag_left_en", (getter) NULL, (setter) NULL, "uint8_t drag_left_en", NULL},
    {"drag_right_en", (getter) NULL, (setter) NULL, "uint8_t drag_right_en", NULL},
    {"drag_hor", (getter) NULL, (setter) NULL, "uint8_t drag_hor", NULL},
    {"drag_ver", (getter) NULL, (setter) NULL, "uint8_t drag_ver", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_mbox_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_mbox_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_mbox_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_mbox_ext_t_getset[] = {
    {"bg", (getter) struct_get_blob, (setter) struct_set_blob, "lv_cont_ext_t bg", (void*)offsetof(lv_mbox_ext_t, bg)},
    {"text", (getter) struct_get_blob, (setter) struct_set_blob, "lv_obj_t text", (void*)offsetof(lv_mbox_ext_t, text)},
    {"btnm", (getter) struct_get_blob, (setter) struct_set_blob, "lv_obj_t btnm", (void*)offsetof(lv_mbox_ext_t, btnm)},
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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_lmeter_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_lmeter_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_lmeter_ext_t));
    self->owner = (PyObject *)self;

    return 0;
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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_gauge_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_gauge_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_gauge_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_gauge_ext_t_getset[] = {
    {"lmeter", (getter) struct_get_blob, (setter) struct_set_blob, "lv_lmeter_ext_t lmeter", (void*)offsetof(lv_gauge_ext_t, lmeter)},
    {"values", (getter) struct_get_blob, (setter) struct_set_blob, "int16_t values", (void*)offsetof(lv_gauge_ext_t, values)},
    {"needle_colors", (getter) struct_get_blob, (setter) struct_set_blob, "lv_color_t needle_colors", (void*)offsetof(lv_gauge_ext_t, needle_colors)},
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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_sw_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_sw_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_sw_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_sw_ext_t_getset[] = {
    {"slider", (getter) struct_get_blob, (setter) struct_set_blob, "lv_slider_ext_t slider", (void*)offsetof(lv_sw_ext_t, slider)},
    {"style_knob_off", (getter) struct_get_blob, (setter) struct_set_blob, "lv_style_t style_knob_off", (void*)offsetof(lv_sw_ext_t, style_knob_off)},
    {"style_knob_on", (getter) struct_get_blob, (setter) struct_set_blob, "lv_style_t style_knob_on", (void*)offsetof(lv_sw_ext_t, style_knob_on)},
    {"start_x", (getter) struct_get_int16, (setter) struct_set_int16, "lv_coord_t start_x", (void*)offsetof(lv_sw_ext_t, start_x)},
    {"changed", (getter) NULL, (setter) NULL, "uint8_t changed", NULL},
    {"slided", (getter) NULL, (setter) NULL, "uint8_t slided", NULL},
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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_arc_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_arc_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_arc_ext_t));
    self->owner = (PyObject *)self;

    return 0;
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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_preload_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_preload_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_preload_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_preload_ext_t_getset[] = {
    {"arc", (getter) struct_get_blob, (setter) struct_set_blob, "lv_arc_ext_t arc", (void*)offsetof(lv_preload_ext_t, arc)},
    {"arc_length", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t arc_length", (void*)offsetof(lv_preload_ext_t, arc_length)},
    {"time", (getter) struct_get_uint16, (setter) struct_set_uint16, "uint16_t time", (void*)offsetof(lv_preload_ext_t, time)},
    {"anim_type", (getter) struct_get_uint8, (setter) struct_set_uint8, "lv_preloader_type_t anim_type", (void*)offsetof(lv_preload_ext_t, anim_type)},

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
    .tp_repr = (reprfunc) Struct_repr

};



static int
pylv_spinbox_ext_t_init(StructObject *self, PyObject *args, PyObject *kwds) 
{

    self->data = PyMem_Malloc(sizeof(lv_spinbox_ext_t));
    if (!self->data) return -1;
    
    memset(self->data, 0, sizeof(lv_spinbox_ext_t));
    self->owner = (PyObject *)self;

    return 0;
}


static PyGetSetDef pylv_spinbox_ext_t_getset[] = {
    {"ta", (getter) struct_get_blob, (setter) struct_set_blob, "lv_ta_ext_t ta", (void*)offsetof(lv_spinbox_ext_t, ta)},
    {"value", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t value", (void*)offsetof(lv_spinbox_ext_t, value)},
    {"range_max", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t range_max", (void*)offsetof(lv_spinbox_ext_t, range_max)},
    {"range_min", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t range_min", (void*)offsetof(lv_spinbox_ext_t, range_min)},
    {"step", (getter) struct_get_int32, (setter) struct_set_int32, "int32_t step", (void*)offsetof(lv_spinbox_ext_t, step)},
    {"digit_count", (getter) NULL, (setter) NULL, "uint16_t digit_count", NULL},
    {"dec_point_pos", (getter) NULL, (setter) NULL, "uint16_t dec_point_pos", NULL},
    {"digit_padding_left", (getter) NULL, (setter) NULL, "uint16_t digit_padding_left", NULL},

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
    .tp_repr = (reprfunc) Struct_repr

};




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
    *lv_obj_get_user_data(self->ref) = self;
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
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_obj_set_auto_realign(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
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
pylv_obj_set_parent_event(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
pylv_obj_set_event_cb(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_event_cb: Parameter type not found >lv_event_cb_t< ");
    return NULL;
}

static PyObject*
pylv_obj_send_event(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_send_event: Parameter type not found >lv_event_t< ");
    return NULL;
}

static PyObject*
pylv_obj_set_signal_cb(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_signal_cb: Parameter type not found >lv_signal_cb_t< ");
    return NULL;
}

static PyObject*
pylv_obj_send_signal(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_send_signal: Parameter type not found >void*< ");
    return NULL;
}

static PyObject*
pylv_obj_set_design_cb(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_set_design_cb: Parameter type not found >lv_design_cb_t< ");
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_animate: Parameter type not found >void cb(lv_obj_t *)*< ");
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
pylv_obj_get_disp(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_disp: Return type not found >lv_disp_t*< ");
    return NULL;
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_coords: Parameter type not found >lv_area_t*< ");
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
pylv_obj_get_auto_realign(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_obj_get_auto_realign(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_style: Return type not found >lv_style_t*< ");
    return NULL;
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
pylv_obj_get_parent_event(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_obj_get_parent_event(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_opa_scale_enable(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_obj_get_opa_scale_enable(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
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
pylv_obj_get_signal_func(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_signal_func: Return type not found >lv_signal_cb_t< ");
    return NULL;
}

static PyObject*
pylv_obj_get_design_func(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_obj_get_design_func: Return type not found >lv_design_cb_t< ");
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
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_obj_is_focused(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_obj_methods[] = {
    {"invalidate", (PyCFunction) pylv_obj_invalidate, METH_VARARGS | METH_KEYWORDS, "void lv_obj_invalidate(const lv_obj_t *obj)"},
    {"set_parent", (PyCFunction) pylv_obj_set_parent, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_parent(lv_obj_t *obj, lv_obj_t *parent)"},
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
    {"set_style", (PyCFunction) pylv_obj_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_style(lv_obj_t *obj, lv_style_t *style)"},
    {"refresh_style", (PyCFunction) pylv_obj_refresh_style, METH_VARARGS | METH_KEYWORDS, "void lv_obj_refresh_style(lv_obj_t *obj)"},
    {"set_hidden", (PyCFunction) pylv_obj_set_hidden, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_hidden(lv_obj_t *obj, bool en)"},
    {"set_click", (PyCFunction) pylv_obj_set_click, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_click(lv_obj_t *obj, bool en)"},
    {"set_top", (PyCFunction) pylv_obj_set_top, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_top(lv_obj_t *obj, bool en)"},
    {"set_drag", (PyCFunction) pylv_obj_set_drag, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_drag(lv_obj_t *obj, bool en)"},
    {"set_drag_throw", (PyCFunction) pylv_obj_set_drag_throw, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_drag_throw(lv_obj_t *obj, bool en)"},
    {"set_drag_parent", (PyCFunction) pylv_obj_set_drag_parent, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_drag_parent(lv_obj_t *obj, bool en)"},
    {"set_parent_event", (PyCFunction) pylv_obj_set_parent_event, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_parent_event(lv_obj_t *obj, bool en)"},
    {"set_opa_scale_enable", (PyCFunction) pylv_obj_set_opa_scale_enable, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_opa_scale_enable(lv_obj_t *obj, bool en)"},
    {"set_opa_scale", (PyCFunction) pylv_obj_set_opa_scale, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_opa_scale(lv_obj_t *obj, lv_opa_t opa_scale)"},
    {"set_protect", (PyCFunction) pylv_obj_set_protect, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_protect(lv_obj_t *obj, uint8_t prot)"},
    {"clear_protect", (PyCFunction) pylv_obj_clear_protect, METH_VARARGS | METH_KEYWORDS, "void lv_obj_clear_protect(lv_obj_t *obj, uint8_t prot)"},
    {"set_event_cb", (PyCFunction) pylv_obj_set_event_cb, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_event_cb(lv_obj_t *obj, lv_event_cb_t cb)"},
    {"send_event", (PyCFunction) pylv_obj_send_event, METH_VARARGS | METH_KEYWORDS, "lv_res_t lv_obj_send_event(lv_obj_t *obj, lv_event_t event)"},
    {"set_signal_cb", (PyCFunction) pylv_obj_set_signal_cb, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_signal_cb(lv_obj_t *obj, lv_signal_cb_t cb)"},
    {"send_signal", (PyCFunction) pylv_obj_send_signal, METH_VARARGS | METH_KEYWORDS, "void lv_obj_send_signal(lv_obj_t *obj, lv_signal_t signal, void *param)"},
    {"set_design_cb", (PyCFunction) pylv_obj_set_design_cb, METH_VARARGS | METH_KEYWORDS, "void lv_obj_set_design_cb(lv_obj_t *obj, lv_design_cb_t cb)"},
    {"refresh_ext_size", (PyCFunction) pylv_obj_refresh_ext_size, METH_VARARGS | METH_KEYWORDS, "void lv_obj_refresh_ext_size(lv_obj_t *obj)"},
    {"animate", (PyCFunction) pylv_obj_animate, METH_VARARGS | METH_KEYWORDS, "void lv_obj_animate(lv_obj_t *obj, lv_anim_builtin_t type, uint16_t time, uint16_t delay, void (*cb)(lv_obj_t *))"},
    {"get_screen", (PyCFunction) pylv_obj_get_screen, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_obj_get_screen(const lv_obj_t *obj)"},
    {"get_disp", (PyCFunction) pylv_obj_get_disp, METH_VARARGS | METH_KEYWORDS, "lv_disp_t *lv_obj_get_disp(const lv_obj_t *obj)"},
    {"get_parent", (PyCFunction) pylv_obj_get_parent, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_obj_get_parent(const lv_obj_t *obj)"},
    {"count_children", (PyCFunction) pylv_obj_count_children, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_obj_count_children(const lv_obj_t *obj)"},
    {"get_coords", (PyCFunction) pylv_obj_get_coords, METH_VARARGS | METH_KEYWORDS, "void lv_obj_get_coords(const lv_obj_t *obj, lv_area_t *cords_p)"},
    {"get_x", (PyCFunction) pylv_obj_get_x, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_x(const lv_obj_t *obj)"},
    {"get_y", (PyCFunction) pylv_obj_get_y, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_y(const lv_obj_t *obj)"},
    {"get_width", (PyCFunction) pylv_obj_get_width, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_width(const lv_obj_t *obj)"},
    {"get_height", (PyCFunction) pylv_obj_get_height, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_height(const lv_obj_t *obj)"},
    {"get_ext_size", (PyCFunction) pylv_obj_get_ext_size, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_obj_get_ext_size(const lv_obj_t *obj)"},
    {"get_auto_realign", (PyCFunction) pylv_obj_get_auto_realign, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_auto_realign(lv_obj_t *obj)"},
    {"get_style", (PyCFunction) pylv_obj_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_obj_get_style(const lv_obj_t *obj)"},
    {"get_hidden", (PyCFunction) pylv_obj_get_hidden, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_hidden(const lv_obj_t *obj)"},
    {"get_click", (PyCFunction) pylv_obj_get_click, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_click(const lv_obj_t *obj)"},
    {"get_top", (PyCFunction) pylv_obj_get_top, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_top(const lv_obj_t *obj)"},
    {"get_drag", (PyCFunction) pylv_obj_get_drag, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_drag(const lv_obj_t *obj)"},
    {"get_drag_throw", (PyCFunction) pylv_obj_get_drag_throw, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_drag_throw(const lv_obj_t *obj)"},
    {"get_drag_parent", (PyCFunction) pylv_obj_get_drag_parent, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_drag_parent(const lv_obj_t *obj)"},
    {"get_parent_event", (PyCFunction) pylv_obj_get_parent_event, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_get_parent_event(const lv_obj_t *obj)"},
    {"get_opa_scale_enable", (PyCFunction) pylv_obj_get_opa_scale_enable, METH_VARARGS | METH_KEYWORDS, "lv_opa_t lv_obj_get_opa_scale_enable(const lv_obj_t *obj)"},
    {"get_opa_scale", (PyCFunction) pylv_obj_get_opa_scale, METH_VARARGS | METH_KEYWORDS, "lv_opa_t lv_obj_get_opa_scale(const lv_obj_t *obj)"},
    {"get_protect", (PyCFunction) pylv_obj_get_protect, METH_VARARGS | METH_KEYWORDS, "uint8_t lv_obj_get_protect(const lv_obj_t *obj)"},
    {"is_protected", (PyCFunction) pylv_obj_is_protected, METH_VARARGS | METH_KEYWORDS, "bool lv_obj_is_protected(const lv_obj_t *obj, uint8_t prot)"},
    {"get_signal_func", (PyCFunction) pylv_obj_get_signal_func, METH_VARARGS | METH_KEYWORDS, "lv_signal_cb_t lv_obj_get_signal_func(const lv_obj_t *obj)"},
    {"get_design_func", (PyCFunction) pylv_obj_get_design_func, METH_VARARGS | METH_KEYWORDS, "lv_design_cb_t lv_obj_get_design_func(const lv_obj_t *obj)"},
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
    *lv_obj_get_user_data(self->ref) = self;
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_cont_set_layout(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_cont_set_fit4: Parameter type not found >lv_fit_t< ");
    return NULL;
}

static PyObject*
pylv_cont_set_fit2(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_cont_set_fit2: Parameter type not found >lv_fit_t< ");
    return NULL;
}

static PyObject*
pylv_cont_set_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_cont_set_fit: Parameter type not found >lv_fit_t< ");
    return NULL;
}

static PyObject*
pylv_cont_get_layout(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_cont_get_layout(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_cont_get_fit_left(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_cont_get_fit_left: Return type not found >lv_fit_t< ");
    return NULL;
}

static PyObject*
pylv_cont_get_fit_right(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_cont_get_fit_right: Return type not found >lv_fit_t< ");
    return NULL;
}

static PyObject*
pylv_cont_get_fit_top(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_cont_get_fit_top: Return type not found >lv_fit_t< ");
    return NULL;
}

static PyObject*
pylv_cont_get_fit_bottom(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_cont_get_fit_bottom: Return type not found >lv_fit_t< ");
    return NULL;
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
    {"set_layout", (PyCFunction) pylv_cont_set_layout, METH_VARARGS | METH_KEYWORDS, "void lv_cont_set_layout(lv_obj_t *cont, lv_layout_t layout)"},
    {"set_fit4", (PyCFunction) pylv_cont_set_fit4, METH_VARARGS | METH_KEYWORDS, "void lv_cont_set_fit4(lv_obj_t *cont, lv_fit_t left, lv_fit_t right, lv_fit_t top, lv_fit_t bottom)"},
    {"set_fit2", (PyCFunction) pylv_cont_set_fit2, METH_VARARGS | METH_KEYWORDS, "inline static void lv_cont_set_fit2(lv_obj_t *cont, lv_fit_t hor, lv_fit_t ver)"},
    {"set_fit", (PyCFunction) pylv_cont_set_fit, METH_VARARGS | METH_KEYWORDS, "inline static void lv_cont_set_fit(lv_obj_t *cont, lv_fit_t fit)"},
    {"get_layout", (PyCFunction) pylv_cont_get_layout, METH_VARARGS | METH_KEYWORDS, "lv_layout_t lv_cont_get_layout(const lv_obj_t *cont)"},
    {"get_fit_left", (PyCFunction) pylv_cont_get_fit_left, METH_VARARGS | METH_KEYWORDS, "lv_fit_t lv_cont_get_fit_left(const lv_obj_t *cont)"},
    {"get_fit_right", (PyCFunction) pylv_cont_get_fit_right, METH_VARARGS | METH_KEYWORDS, "lv_fit_t lv_cont_get_fit_right(const lv_obj_t *cont)"},
    {"get_fit_top", (PyCFunction) pylv_cont_get_fit_top, METH_VARARGS | METH_KEYWORDS, "lv_fit_t lv_cont_get_fit_top(const lv_obj_t *cont)"},
    {"get_fit_bottom", (PyCFunction) pylv_cont_get_fit_bottom, METH_VARARGS | METH_KEYWORDS, "lv_fit_t lv_cont_get_fit_bottom(const lv_obj_t *cont)"},
    {"get_fit_width", (PyCFunction) pylv_cont_get_fit_width, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_cont_get_fit_width(lv_obj_t *cont)"},
    {"get_fit_height", (PyCFunction) pylv_cont_get_fit_height, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_cont_get_fit_height(lv_obj_t *cont)"},
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
    *lv_obj_get_user_data(self->ref) = self;
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_btn_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_btn_get_state(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_btn_get_state(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_btn_get_style: Return type not found >lv_style_t*< ");
    return NULL;
}


static PyMethodDef pylv_btn_methods[] = {
    {"set_toggle", (PyCFunction) pylv_btn_set_toggle, METH_VARARGS | METH_KEYWORDS, "void lv_btn_set_toggle(lv_obj_t *btn, bool tgl)"},
    {"set_state", (PyCFunction) pylv_btn_set_state, METH_VARARGS | METH_KEYWORDS, "void lv_btn_set_state(lv_obj_t *btn, lv_btn_state_t state)"},
    {"toggle", (PyCFunction) pylv_btn_toggle, METH_VARARGS | METH_KEYWORDS, "void lv_btn_toggle(lv_obj_t *btn)"},
    {"set_ink_in_time", (PyCFunction) pylv_btn_set_ink_in_time, METH_VARARGS | METH_KEYWORDS, "void lv_btn_set_ink_in_time(lv_obj_t *btn, uint16_t time)"},
    {"set_ink_wait_time", (PyCFunction) pylv_btn_set_ink_wait_time, METH_VARARGS | METH_KEYWORDS, "void lv_btn_set_ink_wait_time(lv_obj_t *btn, uint16_t time)"},
    {"set_ink_out_time", (PyCFunction) pylv_btn_set_ink_out_time, METH_VARARGS | METH_KEYWORDS, "void lv_btn_set_ink_out_time(lv_obj_t *btn, uint16_t time)"},
    {"set_style", (PyCFunction) pylv_btn_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_btn_set_style(lv_obj_t *btn, lv_btn_style_t type, lv_style_t *style)"},
    {"get_state", (PyCFunction) pylv_btn_get_state, METH_VARARGS | METH_KEYWORDS, "lv_btn_state_t lv_btn_get_state(const lv_obj_t *btn)"},
    {"get_toggle", (PyCFunction) pylv_btn_get_toggle, METH_VARARGS | METH_KEYWORDS, "bool lv_btn_get_toggle(const lv_obj_t *btn)"},
    {"get_ink_in_time", (PyCFunction) pylv_btn_get_ink_in_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_btn_get_ink_in_time(const lv_obj_t *btn)"},
    {"get_ink_wait_time", (PyCFunction) pylv_btn_get_ink_wait_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_btn_get_ink_wait_time(const lv_obj_t *btn)"},
    {"get_ink_out_time", (PyCFunction) pylv_btn_get_ink_out_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_btn_get_ink_out_time(const lv_obj_t *btn)"},
    {"get_style", (PyCFunction) pylv_btn_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_btn_get_style(const lv_obj_t *btn, lv_btn_style_t type)"},
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
pylv_imgbtn_dealloc(pylv_Imgbtn *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    *lv_obj_get_user_data(self->ref) = self;
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_imgbtn_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_imgbtn_get_style: Return type not found >lv_style_t*< ");
    return NULL;
}


static PyMethodDef pylv_imgbtn_methods[] = {
    {"set_src", (PyCFunction) pylv_imgbtn_set_src, METH_VARARGS | METH_KEYWORDS, "void lv_imgbtn_set_src(lv_obj_t *imgbtn, lv_btn_state_t state, const void *src)"},
    {"set_style", (PyCFunction) pylv_imgbtn_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_imgbtn_set_style(lv_obj_t *imgbtn, lv_imgbtn_style_t type, lv_style_t *style)"},
    {"get_src", (PyCFunction) pylv_imgbtn_get_src, METH_VARARGS | METH_KEYWORDS, "const void *lv_imgbtn_get_src(lv_obj_t *imgbtn, lv_btn_state_t state)"},
    {"get_style", (PyCFunction) pylv_imgbtn_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_imgbtn_get_style(const lv_obj_t *imgbtn, lv_imgbtn_style_t type)"},
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
    *lv_obj_get_user_data(self->ref) = self;
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_label_set_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    unsigned char result = lv_label_get_long_mode(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_label_get_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_label_get_align(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
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
    {"get_text", (PyCFunction) pylv_label_get_text, METH_VARARGS | METH_KEYWORDS, "char *lv_label_get_text(const lv_obj_t *label)"},
    {"get_long_mode", (PyCFunction) pylv_label_get_long_mode, METH_VARARGS | METH_KEYWORDS, "lv_label_long_mode_t lv_label_get_long_mode(const lv_obj_t *label)"},
    {"get_align", (PyCFunction) pylv_label_get_align, METH_VARARGS | METH_KEYWORDS, "lv_label_align_t lv_label_get_align(const lv_obj_t *label)"},
    {"get_recolor", (PyCFunction) pylv_label_get_recolor, METH_VARARGS | METH_KEYWORDS, "bool lv_label_get_recolor(const lv_obj_t *label)"},
    {"get_body_draw", (PyCFunction) pylv_label_get_body_draw, METH_VARARGS | METH_KEYWORDS, "bool lv_label_get_body_draw(const lv_obj_t *label)"},
    {"get_anim_speed", (PyCFunction) pylv_label_get_anim_speed, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_label_get_anim_speed(const lv_obj_t *label)"},
    {"get_letter_pos", (PyCFunction) pylv_label_get_letter_pos, METH_VARARGS | METH_KEYWORDS, ""},
    {"get_letter_on", (PyCFunction) pylv_label_get_letter_on, METH_VARARGS | METH_KEYWORDS, ""},
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
    *lv_obj_get_user_data(self->ref) = self;
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
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char * result = lv_img_get_file_name(self->ref);
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
pylv_img_get_offset_x(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_img_get_offset_x(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_img_get_offset_y(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    short int result = lv_img_get_offset_y(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
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
    *lv_obj_get_user_data(self->ref) = self;
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
pylv_line_get_y_invert(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_line_get_y_invert(self->ref);
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
    *lv_obj_get_user_data(self->ref) = self;
    LVGL_UNLOCK

    return 0;
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_page_set_scrl_fit4: Parameter type not found >lv_fit_t< ");
    return NULL;
}

static PyObject*
pylv_page_set_scrl_fit2(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_page_set_scrl_fit2: Parameter type not found >lv_fit_t< ");
    return NULL;
}

static PyObject*
pylv_page_set_scrl_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_page_set_scrl_fit: Parameter type not found >lv_fit_t< ");
    return NULL;
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_page_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_page_get_sb_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_page_get_sb_mode(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
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
pylv_page_get_scroll_propagation(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_page_get_scroll_propagation(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_page_get_edge_flash(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_page_get_edge_flash(self->ref);
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
    unsigned char result = lv_page_get_scrl_layout(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_page_get_scrl_fit_left(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_page_get_scrl_fit_left: Return type not found >lv_fit_t< ");
    return NULL;
}

static PyObject*
pylv_page_get_scrl_fit_right(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_page_get_scrl_fit_right: Return type not found >lv_fit_t< ");
    return NULL;
}

static PyObject*
pylv_page_get_scrl_get_fit_top(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_page_get_scrl_get_fit_top: Return type not found >lv_fit_t< ");
    return NULL;
}

static PyObject*
pylv_page_get_scrl_fit_bottom(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_page_get_scrl_fit_bottom: Return type not found >lv_fit_t< ");
    return NULL;
}

static PyObject*
pylv_page_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_page_get_style: Return type not found >lv_style_t*< ");
    return NULL;
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

static PyObject*
pylv_page_start_edge_flash(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    {"set_style", (PyCFunction) pylv_page_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_page_set_style(lv_obj_t *page, lv_page_style_t type, lv_style_t *style)"},
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
    {"get_scrl_get_fit_top", (PyCFunction) pylv_page_get_scrl_get_fit_top, METH_VARARGS | METH_KEYWORDS, "inline static lv_fit_t lv_page_get_scrl_get_fit_top(const lv_obj_t *page)"},
    {"get_scrl_fit_bottom", (PyCFunction) pylv_page_get_scrl_fit_bottom, METH_VARARGS | METH_KEYWORDS, "inline static lv_fit_t lv_page_get_scrl_fit_bottom(const lv_obj_t *page)"},
    {"get_style", (PyCFunction) pylv_page_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_page_get_style(const lv_obj_t *page, lv_page_style_t type)"},
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
    *lv_obj_get_user_data(self->ref) = self;
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
pylv_list_remove(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"index", NULL};
    unsigned int index;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "I", kwlist , &index)) return NULL;

    LVGL_LOCK        
    int result = lv_list_remove(self->ref, index);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_list_set_single_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_list_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_list_get_single_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_list_get_single_mode(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_list_get_btn_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char * result = lv_list_get_btn_text(self->ref);
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
pylv_list_get_btn_index(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"btn", NULL};
    pylv_Obj * btn;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &pylv_obj_Type, &btn)) return NULL;

    LVGL_LOCK        
    int result = lv_list_get_btn_index(self->ref, btn->ref);
    LVGL_UNLOCK
    return Py_BuildValue("I", result);
}

static PyObject*
pylv_list_get_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned int result = lv_list_get_size(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("I", result);
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_list_get_style: Return type not found >lv_style_t*< ");
    return NULL;
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
    {"clean", (PyCFunction) pylv_list_clean, METH_VARARGS | METH_KEYWORDS, "void lv_list_clean(lv_obj_t *obj)"},
    {"add", (PyCFunction) pylv_list_add, METH_VARARGS | METH_KEYWORDS, ""},
    {"remove", (PyCFunction) pylv_list_remove, METH_VARARGS | METH_KEYWORDS, "bool lv_list_remove(const lv_obj_t *list, uint32_t index)"},
    {"set_single_mode", (PyCFunction) pylv_list_set_single_mode, METH_VARARGS | METH_KEYWORDS, "void lv_list_set_single_mode(lv_obj_t *list, bool mode)"},
    {"set_btn_selected", (PyCFunction) pylv_list_set_btn_selected, METH_VARARGS | METH_KEYWORDS, "void lv_list_set_btn_selected(lv_obj_t *list, lv_obj_t *btn)"},
    {"set_anim_time", (PyCFunction) pylv_list_set_anim_time, METH_VARARGS | METH_KEYWORDS, "void lv_list_set_anim_time(lv_obj_t *list, uint16_t anim_time)"},
    {"set_style", (PyCFunction) pylv_list_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_list_set_style(lv_obj_t *list, lv_list_style_t type, lv_style_t *style)"},
    {"get_single_mode", (PyCFunction) pylv_list_get_single_mode, METH_VARARGS | METH_KEYWORDS, "bool lv_list_get_single_mode(lv_obj_t *list)"},
    {"get_btn_text", (PyCFunction) pylv_list_get_btn_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_list_get_btn_text(const lv_obj_t *btn)"},
    {"get_btn_label", (PyCFunction) pylv_list_get_btn_label, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_list_get_btn_label(const lv_obj_t *btn)"},
    {"get_btn_img", (PyCFunction) pylv_list_get_btn_img, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_list_get_btn_img(const lv_obj_t *btn)"},
    {"get_prev_btn", (PyCFunction) pylv_list_get_prev_btn, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_list_get_prev_btn(const lv_obj_t *list, lv_obj_t *prev_btn)"},
    {"get_next_btn", (PyCFunction) pylv_list_get_next_btn, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_list_get_next_btn(const lv_obj_t *list, lv_obj_t *prev_btn)"},
    {"get_btn_index", (PyCFunction) pylv_list_get_btn_index, METH_VARARGS | METH_KEYWORDS, "int32_t lv_list_get_btn_index(const lv_obj_t *list, const lv_obj_t *btn)"},
    {"get_size", (PyCFunction) pylv_list_get_size, METH_VARARGS | METH_KEYWORDS, "uint32_t lv_list_get_size(const lv_obj_t *list)"},
    {"get_btn_selected", (PyCFunction) pylv_list_get_btn_selected, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_list_get_btn_selected(const lv_obj_t *list)"},
    {"get_anim_time", (PyCFunction) pylv_list_get_anim_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_list_get_anim_time(const lv_obj_t *list)"},
    {"get_style", (PyCFunction) pylv_list_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_list_get_style(const lv_obj_t *list, lv_list_style_t type)"},
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
    *lv_obj_get_user_data(self->ref) = self;
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
pylv_chart_get_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_chart_get_type(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
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
    {"get_type", (PyCFunction) pylv_chart_get_type, METH_VARARGS | METH_KEYWORDS, "lv_chart_type_t lv_chart_get_type(const lv_obj_t *chart)"},
    {"get_point_cnt", (PyCFunction) pylv_chart_get_point_cnt, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_chart_get_point_cnt(const lv_obj_t *chart)"},
    {"get_series_opa", (PyCFunction) pylv_chart_get_series_opa, METH_VARARGS | METH_KEYWORDS, "lv_opa_t lv_chart_get_series_opa(const lv_obj_t *chart)"},
    {"get_series_width", (PyCFunction) pylv_chart_get_series_width, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_chart_get_series_width(const lv_obj_t *chart)"},
    {"get_series_darking", (PyCFunction) pylv_chart_get_series_darking, METH_VARARGS | METH_KEYWORDS, "lv_opa_t lv_chart_get_series_darking(const lv_obj_t *chart)"},
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
};

static void
pylv_table_dealloc(pylv_Table *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    *lv_obj_get_user_data(self->ref) = self;
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_table_set_cell_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_table_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_table_get_cell_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"row", "col", NULL};
    unsigned short int row;
    unsigned short int col;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HH", kwlist , &row, &col)) return NULL;

    LVGL_LOCK        
    const char * result = lv_table_get_cell_value(self->ref, row, col);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_table_get_row_cnt(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_table_get_row_cnt(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_table_get_col_cnt(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_table_get_col_cnt(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_table_get_col_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"col_id", NULL};
    unsigned short int col_id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &col_id)) return NULL;

    LVGL_LOCK        
    short int result = lv_table_get_col_width(self->ref, col_id);
    LVGL_UNLOCK
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_table_get_cell_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"row", "col", NULL};
    unsigned short int row;
    unsigned short int col;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HH", kwlist , &row, &col)) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_table_get_cell_align(self->ref, row, col);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_table_get_cell_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"row", "col", NULL};
    unsigned short int row;
    unsigned short int col;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HH", kwlist , &row, &col)) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_table_get_cell_type(self->ref, row, col);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_table_get_cell_crop(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"row", "col", NULL};
    unsigned short int row;
    unsigned short int col;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HH", kwlist , &row, &col)) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_table_get_cell_crop(self->ref, row, col);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_table_get_cell_merge_right(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"row", "col", NULL};
    unsigned short int row;
    unsigned short int col;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "HH", kwlist , &row, &col)) return NULL;

    LVGL_LOCK        
    int result = lv_table_get_cell_merge_right(self->ref, row, col);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_table_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_table_get_style: Return type not found >lv_style_t*< ");
    return NULL;
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
    {"set_style", (PyCFunction) pylv_table_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_table_set_style(lv_obj_t *table, lv_table_style_t type, lv_style_t *style)"},
    {"get_cell_value", (PyCFunction) pylv_table_get_cell_value, METH_VARARGS | METH_KEYWORDS, "const char *lv_table_get_cell_value(lv_obj_t *table, uint16_t row, uint16_t col)"},
    {"get_row_cnt", (PyCFunction) pylv_table_get_row_cnt, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_table_get_row_cnt(lv_obj_t *table)"},
    {"get_col_cnt", (PyCFunction) pylv_table_get_col_cnt, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_table_get_col_cnt(lv_obj_t *table)"},
    {"get_col_width", (PyCFunction) pylv_table_get_col_width, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_table_get_col_width(lv_obj_t *table, uint16_t col_id)"},
    {"get_cell_align", (PyCFunction) pylv_table_get_cell_align, METH_VARARGS | METH_KEYWORDS, "lv_label_align_t lv_table_get_cell_align(lv_obj_t *table, uint16_t row, uint16_t col)"},
    {"get_cell_type", (PyCFunction) pylv_table_get_cell_type, METH_VARARGS | METH_KEYWORDS, "lv_label_align_t lv_table_get_cell_type(lv_obj_t *table, uint16_t row, uint16_t col)"},
    {"get_cell_crop", (PyCFunction) pylv_table_get_cell_crop, METH_VARARGS | METH_KEYWORDS, "lv_label_align_t lv_table_get_cell_crop(lv_obj_t *table, uint16_t row, uint16_t col)"},
    {"get_cell_merge_right", (PyCFunction) pylv_table_get_cell_merge_right, METH_VARARGS | METH_KEYWORDS, "bool lv_table_get_cell_merge_right(lv_obj_t *table, uint16_t row, uint16_t col)"},
    {"get_style", (PyCFunction) pylv_table_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_table_get_style(const lv_obj_t *table, lv_table_style_t type)"},
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
    *lv_obj_get_user_data(self->ref) = self;
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_cb_set_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_cb_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_cb_get_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char * result = lv_cb_get_text(self->ref);
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_cb_get_style: Return type not found >lv_style_t*< ");
    return NULL;
}


static PyMethodDef pylv_cb_methods[] = {
    {"set_text", (PyCFunction) pylv_cb_set_text, METH_VARARGS | METH_KEYWORDS, "void lv_cb_set_text(lv_obj_t *cb, const char *txt)"},
    {"set_static_text", (PyCFunction) pylv_cb_set_static_text, METH_VARARGS | METH_KEYWORDS, "void lv_cb_set_static_text(lv_obj_t *cb, const char *txt)"},
    {"set_checked", (PyCFunction) pylv_cb_set_checked, METH_VARARGS | METH_KEYWORDS, "inline static void lv_cb_set_checked(lv_obj_t *cb, bool checked)"},
    {"set_inactive", (PyCFunction) pylv_cb_set_inactive, METH_VARARGS | METH_KEYWORDS, "inline static void lv_cb_set_inactive(lv_obj_t *cb)"},
    {"set_style", (PyCFunction) pylv_cb_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_cb_set_style(lv_obj_t *cb, lv_cb_style_t type, lv_style_t *style)"},
    {"get_text", (PyCFunction) pylv_cb_get_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_cb_get_text(const lv_obj_t *cb)"},
    {"is_checked", (PyCFunction) pylv_cb_is_checked, METH_VARARGS | METH_KEYWORDS, "inline static bool lv_cb_is_checked(const lv_obj_t *cb)"},
    {"get_style", (PyCFunction) pylv_cb_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_cb_get_style(const lv_obj_t *cb, lv_cb_style_t type)"},
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
    *lv_obj_get_user_data(self->ref) = self;
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_bar_set_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_bar_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
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
pylv_bar_get_sym(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_bar_get_sym(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_bar_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_bar_get_style: Return type not found >lv_style_t*< ");
    return NULL;
}


static PyMethodDef pylv_bar_methods[] = {
    {"set_value", (PyCFunction) pylv_bar_set_value, METH_VARARGS | METH_KEYWORDS, "void lv_bar_set_value(lv_obj_t *bar, int16_t value, bool anim)"},
    {"set_range", (PyCFunction) pylv_bar_set_range, METH_VARARGS | METH_KEYWORDS, "void lv_bar_set_range(lv_obj_t *bar, int16_t min, int16_t max)"},
    {"set_sym", (PyCFunction) pylv_bar_set_sym, METH_VARARGS | METH_KEYWORDS, "void lv_bar_set_sym(lv_obj_t *bar, bool en)"},
    {"set_style", (PyCFunction) pylv_bar_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_bar_set_style(lv_obj_t *bar, lv_bar_style_t type, lv_style_t *style)"},
    {"get_value", (PyCFunction) pylv_bar_get_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_bar_get_value(const lv_obj_t *bar)"},
    {"get_min_value", (PyCFunction) pylv_bar_get_min_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_bar_get_min_value(const lv_obj_t *bar)"},
    {"get_max_value", (PyCFunction) pylv_bar_get_max_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_bar_get_max_value(const lv_obj_t *bar)"},
    {"get_sym", (PyCFunction) pylv_bar_get_sym, METH_VARARGS | METH_KEYWORDS, "bool lv_bar_get_sym(lv_obj_t *bar)"},
    {"get_style", (PyCFunction) pylv_bar_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_bar_get_style(const lv_obj_t *bar, lv_bar_style_t type)"},
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
    *lv_obj_get_user_data(self->ref) = self;
    LVGL_UNLOCK

    return 0;
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_slider_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_slider_get_style: Return type not found >lv_style_t*< ");
    return NULL;
}


static PyMethodDef pylv_slider_methods[] = {
    {"set_knob_in", (PyCFunction) pylv_slider_set_knob_in, METH_VARARGS | METH_KEYWORDS, "void lv_slider_set_knob_in(lv_obj_t *slider, bool in)"},
    {"set_style", (PyCFunction) pylv_slider_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_slider_set_style(lv_obj_t *slider, lv_slider_style_t type, lv_style_t *style)"},
    {"get_value", (PyCFunction) pylv_slider_get_value, METH_VARARGS | METH_KEYWORDS, "int16_t lv_slider_get_value(const lv_obj_t *slider)"},
    {"is_dragged", (PyCFunction) pylv_slider_is_dragged, METH_VARARGS | METH_KEYWORDS, "bool lv_slider_is_dragged(const lv_obj_t *slider)"},
    {"get_knob_in", (PyCFunction) pylv_slider_get_knob_in, METH_VARARGS | METH_KEYWORDS, "bool lv_slider_get_knob_in(const lv_obj_t *slider)"},
    {"get_style", (PyCFunction) pylv_slider_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_slider_get_style(const lv_obj_t *slider, lv_slider_style_t type)"},
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
    *lv_obj_get_user_data(self->ref) = self;
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
    *lv_obj_get_user_data(self->ref) = self;
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_btnm_set_map(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_btnm_set_map: Parameter type not found >const char**< ");
    return NULL;
}

static PyObject*
pylv_btnm_set_ctrl_map(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_btnm_set_ctrl_map: Parameter type not found >const lv_btnm_ctrl_t*< ");
    return NULL;
}

static PyObject*
pylv_btnm_set_pressed(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_btnm_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_btnm_set_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    LVGL_LOCK         
    lv_btnm_set_recolor(self->ref, en);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_set_btn_hidden(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"btn_idx", "hidden", NULL};
    unsigned short int btn_idx;
    int hidden;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hp", kwlist , &btn_idx, &hidden)) return NULL;

    LVGL_LOCK         
    lv_btnm_set_btn_hidden(self->ref, btn_idx, hidden);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_set_btn_inactive(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"btn_id", "ina", NULL};
    unsigned short int btn_id;
    int ina;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hp", kwlist , &btn_id, &ina)) return NULL;

    LVGL_LOCK         
    lv_btnm_set_btn_inactive(self->ref, btn_id, ina);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_set_btn_no_repeat(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"btn_id", "no_rep", NULL};
    unsigned short int btn_id;
    int no_rep;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hp", kwlist , &btn_id, &no_rep)) return NULL;

    LVGL_LOCK         
    lv_btnm_set_btn_no_repeat(self->ref, btn_id, no_rep);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_set_btn_toggle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"btn_id", "tgl", NULL};
    unsigned short int btn_id;
    int tgl;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hp", kwlist , &btn_id, &tgl)) return NULL;

    LVGL_LOCK         
    lv_btnm_set_btn_toggle(self->ref, btn_id, tgl);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_set_btn_toggle_state(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"btn_id", "toggle", NULL};
    unsigned short int btn_id;
    int toggle;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hp", kwlist , &btn_id, &toggle)) return NULL;

    LVGL_LOCK         
    lv_btnm_set_btn_toggle_state(self->ref, btn_id, toggle);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_set_btn_flags(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"btn_id", "hidden", "inactive", "no_repeat", "toggle", "toggle_state", NULL};
    unsigned short int btn_id;
    int hidden;
    int inactive;
    int no_repeat;
    int toggle;
    int toggle_state;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hppppp", kwlist , &btn_id, &hidden, &inactive, &no_repeat, &toggle, &toggle_state)) return NULL;

    LVGL_LOCK         
    lv_btnm_set_btn_flags(self->ref, btn_id, hidden, inactive, no_repeat, toggle, toggle_state);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_set_btn_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
pylv_btnm_set_btn_toggle_state_all(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"state", NULL};
    int state;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &state)) return NULL;

    LVGL_LOCK         
    lv_btnm_set_btn_toggle_state_all(self->ref, state);
    LVGL_UNLOCK
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_get_map(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_btnm_get_map: Return type not found >const char**< ");
    return NULL;
}

static PyObject*
pylv_btnm_get_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_btnm_get_recolor(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_btnm_get_active_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_btnm_get_active_btn(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_btnm_get_active_btn_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char * result = lv_btnm_get_active_btn_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_btnm_get_pressed_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_btnm_get_pressed_btn(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_btnm_get_btn_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"btn_id", NULL};
    unsigned short int btn_id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &btn_id)) return NULL;

    LVGL_LOCK        
    const char * result = lv_btnm_get_btn_text(self->ref, btn_id);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_btnm_get_btn_no_repeate(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"btn_id", NULL};
    unsigned short int btn_id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &btn_id)) return NULL;

    LVGL_LOCK        
    int result = lv_btnm_get_btn_no_repeate(self->ref, btn_id);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_btnm_get_btn_hidden(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"btn_id", NULL};
    unsigned short int btn_id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &btn_id)) return NULL;

    LVGL_LOCK        
    int result = lv_btnm_get_btn_hidden(self->ref, btn_id);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_btnm_get_btn_inactive(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"btn_id", NULL};
    unsigned short int btn_id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &btn_id)) return NULL;

    LVGL_LOCK        
    int result = lv_btnm_get_btn_inactive(self->ref, btn_id);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_btnm_get_btn_toggle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"btn_id", NULL};
    short int btn_id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &btn_id)) return NULL;

    LVGL_LOCK        
    int result = lv_btnm_get_btn_toggle(self->ref, btn_id);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_btnm_get_btn_toggle_state(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"btn_id", NULL};
    short int btn_id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &btn_id)) return NULL;

    LVGL_LOCK        
    int result = lv_btnm_get_btn_toggle_state(self->ref, btn_id);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_btnm_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_btnm_get_style: Return type not found >lv_style_t*< ");
    return NULL;
}


static PyMethodDef pylv_btnm_methods[] = {
    {"set_map", (PyCFunction) pylv_btnm_set_map, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_map(const lv_obj_t *btnm, const char **map)"},
    {"set_ctrl_map", (PyCFunction) pylv_btnm_set_ctrl_map, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_ctrl_map(const lv_obj_t *btnm, const lv_btnm_ctrl_t *ctrl_map)"},
    {"set_pressed", (PyCFunction) pylv_btnm_set_pressed, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_pressed(const lv_obj_t *btnm, uint16_t id)"},
    {"set_style", (PyCFunction) pylv_btnm_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_style(lv_obj_t *btnm, lv_btnm_style_t type, lv_style_t *style)"},
    {"set_recolor", (PyCFunction) pylv_btnm_set_recolor, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_recolor(const lv_obj_t *btnm, bool en)"},
    {"set_btn_hidden", (PyCFunction) pylv_btnm_set_btn_hidden, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_btn_hidden(const lv_obj_t *btnm, uint16_t btn_idx, bool hidden)"},
    {"set_btn_inactive", (PyCFunction) pylv_btnm_set_btn_inactive, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_btn_inactive(const lv_obj_t *btnm, uint16_t btn_id, bool ina)"},
    {"set_btn_no_repeat", (PyCFunction) pylv_btnm_set_btn_no_repeat, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_btn_no_repeat(const lv_obj_t *btnm, uint16_t btn_id, bool no_rep)"},
    {"set_btn_toggle", (PyCFunction) pylv_btnm_set_btn_toggle, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_btn_toggle(const lv_obj_t *btnm, uint16_t btn_id, bool tgl)"},
    {"set_btn_toggle_state", (PyCFunction) pylv_btnm_set_btn_toggle_state, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_btn_toggle_state(lv_obj_t *btnm, uint16_t btn_id, bool toggle)"},
    {"set_btn_flags", (PyCFunction) pylv_btnm_set_btn_flags, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_btn_flags(const lv_obj_t *btnm, uint16_t btn_id, bool hidden, bool inactive, bool no_repeat, bool toggle, bool toggle_state)"},
    {"set_btn_width", (PyCFunction) pylv_btnm_set_btn_width, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_btn_width(const lv_obj_t *btnm, uint16_t btn_id, uint8_t width)"},
    {"set_btn_toggle_state_all", (PyCFunction) pylv_btnm_set_btn_toggle_state_all, METH_VARARGS | METH_KEYWORDS, "void lv_btnm_set_btn_toggle_state_all(lv_obj_t *btnm, bool state)"},
    {"get_map", (PyCFunction) pylv_btnm_get_map, METH_VARARGS | METH_KEYWORDS, "const char **lv_btnm_get_map(const lv_obj_t *btnm)"},
    {"get_recolor", (PyCFunction) pylv_btnm_get_recolor, METH_VARARGS | METH_KEYWORDS, "bool lv_btnm_get_recolor(const lv_obj_t *btnm)"},
    {"get_active_btn", (PyCFunction) pylv_btnm_get_active_btn, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_btnm_get_active_btn(const lv_obj_t *btnm)"},
    {"get_active_btn_text", (PyCFunction) pylv_btnm_get_active_btn_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_btnm_get_active_btn_text(const lv_obj_t *btnm)"},
    {"get_pressed_btn", (PyCFunction) pylv_btnm_get_pressed_btn, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_btnm_get_pressed_btn(const lv_obj_t *btnm)"},
    {"get_btn_text", (PyCFunction) pylv_btnm_get_btn_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_btnm_get_btn_text(const lv_obj_t *btnm, uint16_t btn_id)"},
    {"get_btn_no_repeate", (PyCFunction) pylv_btnm_get_btn_no_repeate, METH_VARARGS | METH_KEYWORDS, "bool lv_btnm_get_btn_no_repeate(lv_obj_t *btnm, uint16_t btn_id)"},
    {"get_btn_hidden", (PyCFunction) pylv_btnm_get_btn_hidden, METH_VARARGS | METH_KEYWORDS, "bool lv_btnm_get_btn_hidden(lv_obj_t *btnm, uint16_t btn_id)"},
    {"get_btn_inactive", (PyCFunction) pylv_btnm_get_btn_inactive, METH_VARARGS | METH_KEYWORDS, "bool lv_btnm_get_btn_inactive(lv_obj_t *btnm, uint16_t btn_id)"},
    {"get_btn_toggle", (PyCFunction) pylv_btnm_get_btn_toggle, METH_VARARGS | METH_KEYWORDS, "bool lv_btnm_get_btn_toggle(const lv_obj_t *btnm, int16_t btn_id)"},
    {"get_btn_toggle_state", (PyCFunction) pylv_btnm_get_btn_toggle_state, METH_VARARGS | METH_KEYWORDS, "bool lv_btnm_get_btn_toggle_state(const lv_obj_t *btnm, int16_t btn_id)"},
    {"get_style", (PyCFunction) pylv_btnm_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_btnm_get_style(const lv_obj_t *btnm, lv_btnm_style_t type)"},
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
    *lv_obj_get_user_data(self->ref) = self;
    LVGL_UNLOCK

    return 0;
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_kb_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
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
    unsigned char result = lv_kb_get_mode(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_kb_get_style: Return type not found >lv_style_t*< ");
    return NULL;
}


static PyMethodDef pylv_kb_methods[] = {
    {"set_ta", (PyCFunction) pylv_kb_set_ta, METH_VARARGS | METH_KEYWORDS, "void lv_kb_set_ta(lv_obj_t *kb, lv_obj_t *ta)"},
    {"set_mode", (PyCFunction) pylv_kb_set_mode, METH_VARARGS | METH_KEYWORDS, "void lv_kb_set_mode(lv_obj_t *kb, lv_kb_mode_t mode)"},
    {"set_cursor_manage", (PyCFunction) pylv_kb_set_cursor_manage, METH_VARARGS | METH_KEYWORDS, "void lv_kb_set_cursor_manage(lv_obj_t *kb, bool en)"},
    {"set_style", (PyCFunction) pylv_kb_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_kb_set_style(lv_obj_t *kb, lv_kb_style_t type, lv_style_t *style)"},
    {"get_ta", (PyCFunction) pylv_kb_get_ta, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_kb_get_ta(const lv_obj_t *kb)"},
    {"get_mode", (PyCFunction) pylv_kb_get_mode, METH_VARARGS | METH_KEYWORDS, "lv_kb_mode_t lv_kb_get_mode(const lv_obj_t *kb)"},
    {"get_cursor_manage", (PyCFunction) pylv_kb_get_cursor_manage, METH_VARARGS | METH_KEYWORDS, "bool lv_kb_get_cursor_manage(const lv_obj_t *kb)"},
    {"get_style", (PyCFunction) pylv_kb_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_kb_get_style(const lv_obj_t *kb, lv_kb_style_t type)"},
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
    *lv_obj_get_user_data(self->ref) = self;
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_ddlist_set_options(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
pylv_ddlist_set_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_ddlist_set_fit: Parameter type not found >lv_fit_t< ");
    return NULL;
}

static PyObject*
pylv_ddlist_set_draw_arrow(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_ddlist_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_ddlist_set_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char * result = lv_ddlist_get_options(self->ref);
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_ddlist_get_selected_str: Parameter type not found >char*< ");
    return NULL;
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
pylv_ddlist_get_draw_arrow(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_ddlist_get_draw_arrow(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_ddlist_get_stay_open(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_ddlist_get_stay_open(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_ddlist_get_style: Return type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_ddlist_get_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_ddlist_get_align(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_ddlist_open(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    {"set_fit", (PyCFunction) pylv_ddlist_set_fit, METH_VARARGS | METH_KEYWORDS, "void lv_ddlist_set_fit(lv_obj_t *ddlist, lv_fit_t fit)"},
    {"set_draw_arrow", (PyCFunction) pylv_ddlist_set_draw_arrow, METH_VARARGS | METH_KEYWORDS, "void lv_ddlist_set_draw_arrow(lv_obj_t *ddlist, bool en)"},
    {"set_stay_open", (PyCFunction) pylv_ddlist_set_stay_open, METH_VARARGS | METH_KEYWORDS, "void lv_ddlist_set_stay_open(lv_obj_t *ddlist, bool en)"},
    {"set_anim_time", (PyCFunction) pylv_ddlist_set_anim_time, METH_VARARGS | METH_KEYWORDS, "void lv_ddlist_set_anim_time(lv_obj_t *ddlist, uint16_t anim_time)"},
    {"set_style", (PyCFunction) pylv_ddlist_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_ddlist_set_style(lv_obj_t *ddlist, lv_ddlist_style_t type, lv_style_t *style)"},
    {"set_align", (PyCFunction) pylv_ddlist_set_align, METH_VARARGS | METH_KEYWORDS, "void lv_ddlist_set_align(lv_obj_t *ddlist, lv_label_align_t align)"},
    {"get_options", (PyCFunction) pylv_ddlist_get_options, METH_VARARGS | METH_KEYWORDS, "const char *lv_ddlist_get_options(const lv_obj_t *ddlist)"},
    {"get_selected", (PyCFunction) pylv_ddlist_get_selected, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_ddlist_get_selected(const lv_obj_t *ddlist)"},
    {"get_selected_str", (PyCFunction) pylv_ddlist_get_selected_str, METH_VARARGS | METH_KEYWORDS, "void lv_ddlist_get_selected_str(const lv_obj_t *ddlist, char *buf)"},
    {"get_fix_height", (PyCFunction) pylv_ddlist_get_fix_height, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_ddlist_get_fix_height(const lv_obj_t *ddlist)"},
    {"get_draw_arrow", (PyCFunction) pylv_ddlist_get_draw_arrow, METH_VARARGS | METH_KEYWORDS, "bool lv_ddlist_get_draw_arrow(lv_obj_t *ddlist)"},
    {"get_stay_open", (PyCFunction) pylv_ddlist_get_stay_open, METH_VARARGS | METH_KEYWORDS, "bool lv_ddlist_get_stay_open(lv_obj_t *ddlist)"},
    {"get_anim_time", (PyCFunction) pylv_ddlist_get_anim_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_ddlist_get_anim_time(const lv_obj_t *ddlist)"},
    {"get_style", (PyCFunction) pylv_ddlist_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_ddlist_get_style(const lv_obj_t *ddlist, lv_ddlist_style_t type)"},
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
    *lv_obj_get_user_data(self->ref) = self;
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_roller_set_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
pylv_roller_set_hor_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_roller_set_hor_fit: Parameter type not found >lv_fit_t< ");
    return NULL;
}

static PyObject*
pylv_roller_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_roller_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_roller_get_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_roller_get_align(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_roller_get_style: Return type not found >lv_style_t*< ");
    return NULL;
}


static PyMethodDef pylv_roller_methods[] = {
    {"set_align", (PyCFunction) pylv_roller_set_align, METH_VARARGS | METH_KEYWORDS, "void lv_roller_set_align(lv_obj_t *roller, lv_label_align_t align)"},
    {"set_selected", (PyCFunction) pylv_roller_set_selected, METH_VARARGS | METH_KEYWORDS, "void lv_roller_set_selected(lv_obj_t *roller, uint16_t sel_opt, bool anim_en)"},
    {"set_visible_row_count", (PyCFunction) pylv_roller_set_visible_row_count, METH_VARARGS | METH_KEYWORDS, "void lv_roller_set_visible_row_count(lv_obj_t *roller, uint8_t row_cnt)"},
    {"set_hor_fit", (PyCFunction) pylv_roller_set_hor_fit, METH_VARARGS | METH_KEYWORDS, "inline static void lv_roller_set_hor_fit(lv_obj_t *roller, lv_fit_t fit)"},
    {"set_style", (PyCFunction) pylv_roller_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_roller_set_style(lv_obj_t *roller, lv_roller_style_t type, lv_style_t *style)"},
    {"get_align", (PyCFunction) pylv_roller_get_align, METH_VARARGS | METH_KEYWORDS, "lv_label_align_t lv_roller_get_align(const lv_obj_t *roller)"},
    {"get_hor_fit", (PyCFunction) pylv_roller_get_hor_fit, METH_VARARGS | METH_KEYWORDS, "bool lv_roller_get_hor_fit(const lv_obj_t *roller)"},
    {"get_style", (PyCFunction) pylv_roller_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_roller_get_style(const lv_obj_t *roller, lv_roller_style_t type)"},
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
    *lv_obj_get_user_data(self->ref) = self;
    LVGL_UNLOCK

    return 0;
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_ta_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_ta_get_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char * result = lv_ta_get_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_ta_get_placeholder_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char * result = lv_ta_get_placeholder_text(self->ref);
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
    unsigned char result = lv_ta_get_cursor_type(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
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
    const char * result = lv_ta_get_accepted_chars(self->ref);
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_ta_get_style: Return type not found >lv_style_t*< ");
    return NULL;
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
    {"set_style", (PyCFunction) pylv_ta_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_ta_set_style(lv_obj_t *ta, lv_ta_style_t type, lv_style_t *style)"},
    {"get_text", (PyCFunction) pylv_ta_get_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_ta_get_text(const lv_obj_t *ta)"},
    {"get_placeholder_text", (PyCFunction) pylv_ta_get_placeholder_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_ta_get_placeholder_text(lv_obj_t *ta)"},
    {"get_label", (PyCFunction) pylv_ta_get_label, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_ta_get_label(const lv_obj_t *ta)"},
    {"get_cursor_pos", (PyCFunction) pylv_ta_get_cursor_pos, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_ta_get_cursor_pos(const lv_obj_t *ta)"},
    {"get_cursor_type", (PyCFunction) pylv_ta_get_cursor_type, METH_VARARGS | METH_KEYWORDS, "lv_cursor_type_t lv_ta_get_cursor_type(const lv_obj_t *ta)"},
    {"get_pwd_mode", (PyCFunction) pylv_ta_get_pwd_mode, METH_VARARGS | METH_KEYWORDS, "bool lv_ta_get_pwd_mode(const lv_obj_t *ta)"},
    {"get_one_line", (PyCFunction) pylv_ta_get_one_line, METH_VARARGS | METH_KEYWORDS, "bool lv_ta_get_one_line(const lv_obj_t *ta)"},
    {"get_accepted_chars", (PyCFunction) pylv_ta_get_accepted_chars, METH_VARARGS | METH_KEYWORDS, "const char *lv_ta_get_accepted_chars(lv_obj_t *ta)"},
    {"get_max_length", (PyCFunction) pylv_ta_get_max_length, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_ta_get_max_length(lv_obj_t *ta)"},
    {"get_style", (PyCFunction) pylv_ta_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_ta_get_style(const lv_obj_t *ta, lv_ta_style_t type)"},
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
};

static void
pylv_canvas_dealloc(pylv_Canvas *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    *lv_obj_get_user_data(self->ref) = self;
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_canvas_get_px(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_get_px: Return type not found >lv_color_t< ");
    return NULL;
}

static PyObject*
pylv_canvas_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_canvas_get_style: Return type not found >lv_style_t*< ");
    return NULL;
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
    {"set_style", (PyCFunction) pylv_canvas_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_set_style(lv_obj_t *canvas, lv_canvas_style_t type, lv_style_t *style)"},
    {"get_px", (PyCFunction) pylv_canvas_get_px, METH_VARARGS | METH_KEYWORDS, "lv_color_t lv_canvas_get_px(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y)"},
    {"get_style", (PyCFunction) pylv_canvas_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_canvas_get_style(const lv_obj_t *canvas, lv_canvas_style_t type)"},
    {"copy_buf", (PyCFunction) pylv_canvas_copy_buf, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_copy_buf(lv_obj_t *canvas, const void *to_copy, lv_coord_t w, lv_coord_t h, lv_coord_t x, lv_coord_t y)"},
    {"mult_buf", (PyCFunction) pylv_canvas_mult_buf, METH_VARARGS | METH_KEYWORDS, "void lv_canvas_mult_buf(lv_obj_t *canvas, void *to_copy, lv_coord_t w, lv_coord_t h, lv_coord_t x, lv_coord_t y)"},
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
    *lv_obj_get_user_data(self->ref) = self;
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_win_add_btn: Parameter type not found >const void*< ");
    return NULL;
}

static PyObject*
pylv_win_close_event(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_win_close_event: Parameter type not found >lv_event_t< ");
    return NULL;
}

static PyObject*
pylv_win_set_title(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_win_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_win_set_drag(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char * result = lv_win_get_title(self->ref);
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
    unsigned char result = lv_win_get_layout(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_win_get_sb_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_win_get_sb_mode(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_win_get_style: Return type not found >lv_style_t*< ");
    return NULL;
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
    {"clean", (PyCFunction) pylv_win_clean, METH_VARARGS | METH_KEYWORDS, "void lv_win_clean(lv_obj_t *obj)"},
    {"add_btn", (PyCFunction) pylv_win_add_btn, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_win_add_btn(lv_obj_t *win, const void *img_src, lv_event_cb_t event_cb)"},
    {"close_event", (PyCFunction) pylv_win_close_event, METH_VARARGS | METH_KEYWORDS, "void lv_win_close_event(lv_obj_t *btn, lv_event_t event)"},
    {"set_title", (PyCFunction) pylv_win_set_title, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_title(lv_obj_t *win, const char *title)"},
    {"set_btn_size", (PyCFunction) pylv_win_set_btn_size, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_btn_size(lv_obj_t *win, lv_coord_t size)"},
    {"set_layout", (PyCFunction) pylv_win_set_layout, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_layout(lv_obj_t *win, lv_layout_t layout)"},
    {"set_sb_mode", (PyCFunction) pylv_win_set_sb_mode, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_sb_mode(lv_obj_t *win, lv_sb_mode_t sb_mode)"},
    {"set_style", (PyCFunction) pylv_win_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_style(lv_obj_t *win, lv_win_style_t type, lv_style_t *style)"},
    {"set_drag", (PyCFunction) pylv_win_set_drag, METH_VARARGS | METH_KEYWORDS, "void lv_win_set_drag(lv_obj_t *win, bool en)"},
    {"get_title", (PyCFunction) pylv_win_get_title, METH_VARARGS | METH_KEYWORDS, "const char *lv_win_get_title(const lv_obj_t *win)"},
    {"get_content", (PyCFunction) pylv_win_get_content, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_win_get_content(const lv_obj_t *win)"},
    {"get_btn_size", (PyCFunction) pylv_win_get_btn_size, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_win_get_btn_size(const lv_obj_t *win)"},
    {"get_from_btn", (PyCFunction) pylv_win_get_from_btn, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_win_get_from_btn(const lv_obj_t *ctrl_btn)"},
    {"get_layout", (PyCFunction) pylv_win_get_layout, METH_VARARGS | METH_KEYWORDS, "lv_layout_t lv_win_get_layout(lv_obj_t *win)"},
    {"get_sb_mode", (PyCFunction) pylv_win_get_sb_mode, METH_VARARGS | METH_KEYWORDS, "lv_sb_mode_t lv_win_get_sb_mode(lv_obj_t *win)"},
    {"get_width", (PyCFunction) pylv_win_get_width, METH_VARARGS | METH_KEYWORDS, "lv_coord_t lv_win_get_width(lv_obj_t *win)"},
    {"get_style", (PyCFunction) pylv_win_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_win_get_style(const lv_obj_t *win, lv_win_style_t type)"},
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
    *lv_obj_get_user_data(self->ref) = self;
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_tabview_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_tabview_set_btns_pos(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_tabview_get_style: Return type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_tabview_get_btns_pos(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_tabview_get_btns_pos(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_tabview_get_btns_hidden(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_tabview_get_btns_hidden(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_tabview_methods[] = {
    {"clean", (PyCFunction) pylv_tabview_clean, METH_VARARGS | METH_KEYWORDS, "void lv_tabview_clean(lv_obj_t *obj)"},
    {"add_tab", (PyCFunction) pylv_tabview_add_tab, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_tabview_add_tab(lv_obj_t *tabview, const char *name)"},
    {"set_tab_act", (PyCFunction) pylv_tabview_set_tab_act, METH_VARARGS | METH_KEYWORDS, "void lv_tabview_set_tab_act(lv_obj_t *tabview, uint16_t id, bool anim_en)"},
    {"set_sliding", (PyCFunction) pylv_tabview_set_sliding, METH_VARARGS | METH_KEYWORDS, "void lv_tabview_set_sliding(lv_obj_t *tabview, bool en)"},
    {"set_anim_time", (PyCFunction) pylv_tabview_set_anim_time, METH_VARARGS | METH_KEYWORDS, "void lv_tabview_set_anim_time(lv_obj_t *tabview, uint16_t anim_time)"},
    {"set_style", (PyCFunction) pylv_tabview_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_tabview_set_style(lv_obj_t *tabview, lv_tabview_style_t type, lv_style_t *style)"},
    {"set_btns_pos", (PyCFunction) pylv_tabview_set_btns_pos, METH_VARARGS | METH_KEYWORDS, "void lv_tabview_set_btns_pos(lv_obj_t *tabview, lv_tabview_btns_pos_t btns_pos)"},
    {"set_btns_hidden", (PyCFunction) pylv_tabview_set_btns_hidden, METH_VARARGS | METH_KEYWORDS, "void lv_tabview_set_btns_hidden(lv_obj_t *tabview, bool en)"},
    {"get_tab_act", (PyCFunction) pylv_tabview_get_tab_act, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_tabview_get_tab_act(const lv_obj_t *tabview)"},
    {"get_tab_count", (PyCFunction) pylv_tabview_get_tab_count, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_tabview_get_tab_count(const lv_obj_t *tabview)"},
    {"get_tab", (PyCFunction) pylv_tabview_get_tab, METH_VARARGS | METH_KEYWORDS, "lv_obj_t *lv_tabview_get_tab(const lv_obj_t *tabview, uint16_t id)"},
    {"get_sliding", (PyCFunction) pylv_tabview_get_sliding, METH_VARARGS | METH_KEYWORDS, "bool lv_tabview_get_sliding(const lv_obj_t *tabview)"},
    {"get_anim_time", (PyCFunction) pylv_tabview_get_anim_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_tabview_get_anim_time(const lv_obj_t *tabview)"},
    {"get_style", (PyCFunction) pylv_tabview_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_tabview_get_style(const lv_obj_t *tabview, lv_tabview_style_t type)"},
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
};

static void
pylv_tileview_dealloc(pylv_Tileview *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    *lv_obj_get_user_data(self->ref) = self;
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_tileview_add_element(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_tileview_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_tileview_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_tileview_get_style: Return type not found >lv_style_t*< ");
    return NULL;
}


static PyMethodDef pylv_tileview_methods[] = {
    {"add_element", (PyCFunction) pylv_tileview_add_element, METH_VARARGS | METH_KEYWORDS, "void lv_tileview_add_element(lv_obj_t *tileview, lv_obj_t *element)"},
    {"set_valid_positions", (PyCFunction) pylv_tileview_set_valid_positions, METH_VARARGS | METH_KEYWORDS, "void lv_tileview_set_valid_positions(lv_obj_t *tileview, const lv_point_t *valid_pos)"},
    {"set_tile_act", (PyCFunction) pylv_tileview_set_tile_act, METH_VARARGS | METH_KEYWORDS, "void lv_tileview_set_tile_act(lv_obj_t *tileview, lv_coord_t x, lv_coord_t y, bool anim_en)"},
    {"set_style", (PyCFunction) pylv_tileview_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_tileview_set_style(lv_obj_t *tileview, lv_tileview_style_t type, lv_style_t *style)"},
    {"get_style", (PyCFunction) pylv_tileview_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_tileview_get_style(const lv_obj_t *tileview, lv_tileview_style_t type)"},
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
    *lv_obj_get_user_data(self->ref) = self;
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_mbox_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_mbox_set_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char * result = lv_mbox_get_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
}

static PyObject*
pylv_mbox_get_active_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_mbox_get_active_btn(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}

static PyObject*
pylv_mbox_get_active_btn_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    const char * result = lv_mbox_get_active_btn_text(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("s", result);
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_mbox_get_style: Return type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_mbox_get_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_mbox_get_recolor(self->ref);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_mbox_get_btnm(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    {"set_style", (PyCFunction) pylv_mbox_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_mbox_set_style(lv_obj_t *mbox, lv_mbox_style_t type, lv_style_t *style)"},
    {"set_recolor", (PyCFunction) pylv_mbox_set_recolor, METH_VARARGS | METH_KEYWORDS, "void lv_mbox_set_recolor(lv_obj_t *mbox, bool en)"},
    {"get_text", (PyCFunction) pylv_mbox_get_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_mbox_get_text(const lv_obj_t *mbox)"},
    {"get_active_btn", (PyCFunction) pylv_mbox_get_active_btn, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_mbox_get_active_btn(lv_obj_t *mbox)"},
    {"get_active_btn_text", (PyCFunction) pylv_mbox_get_active_btn_text, METH_VARARGS | METH_KEYWORDS, "const char *lv_mbox_get_active_btn_text(lv_obj_t *mbox)"},
    {"get_anim_time", (PyCFunction) pylv_mbox_get_anim_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_mbox_get_anim_time(const lv_obj_t *mbox)"},
    {"get_style", (PyCFunction) pylv_mbox_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_mbox_get_style(const lv_obj_t *mbox, lv_mbox_style_t type)"},
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
    *lv_obj_get_user_data(self->ref) = self;
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
    *lv_obj_get_user_data(self->ref) = self;
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_gauge_set_needle_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_gauge_set_needle_count: Parameter type not found >const lv_color_t*< ");
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
    {"set_needle_count", (PyCFunction) pylv_gauge_set_needle_count, METH_VARARGS | METH_KEYWORDS, "void lv_gauge_set_needle_count(lv_obj_t *gauge, uint8_t needle_cnt, const lv_color_t *colors)"},
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
    *lv_obj_get_user_data(self->ref) = self;
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_sw_on(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {"anim", NULL};
    int anim;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &anim)) return NULL;

    LVGL_LOCK        
    int result = lv_sw_toggle(self->ref, anim);
    LVGL_UNLOCK
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_sw_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_sw_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_sw_set_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_sw_get_style: Return type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_sw_get_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned short int result = lv_sw_get_anim_time(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("H", result);
}


static PyMethodDef pylv_sw_methods[] = {
    {"on", (PyCFunction) pylv_sw_on, METH_VARARGS | METH_KEYWORDS, "void lv_sw_on(lv_obj_t *sw, bool anim)"},
    {"off", (PyCFunction) pylv_sw_off, METH_VARARGS | METH_KEYWORDS, "void lv_sw_off(lv_obj_t *sw, bool anim)"},
    {"toggle", (PyCFunction) pylv_sw_toggle, METH_VARARGS | METH_KEYWORDS, "bool lv_sw_toggle(lv_obj_t *sw, bool anim)"},
    {"set_style", (PyCFunction) pylv_sw_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_sw_set_style(lv_obj_t *sw, lv_sw_style_t type, lv_style_t *style)"},
    {"set_anim_time", (PyCFunction) pylv_sw_set_anim_time, METH_VARARGS | METH_KEYWORDS, "void lv_sw_set_anim_time(lv_obj_t *sw, uint16_t anim_time)"},
    {"get_state", (PyCFunction) pylv_sw_get_state, METH_VARARGS | METH_KEYWORDS, "inline static bool lv_sw_get_state(const lv_obj_t *sw)"},
    {"get_style", (PyCFunction) pylv_sw_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_sw_get_style(const lv_obj_t *sw, lv_sw_style_t type)"},
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
    *lv_obj_get_user_data(self->ref) = self;
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_arc_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_arc_get_style: Return type not found >lv_style_t*< ");
    return NULL;
}


static PyMethodDef pylv_arc_methods[] = {
    {"set_angles", (PyCFunction) pylv_arc_set_angles, METH_VARARGS | METH_KEYWORDS, "void lv_arc_set_angles(lv_obj_t *arc, uint16_t start, uint16_t end)"},
    {"set_style", (PyCFunction) pylv_arc_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_arc_set_style(lv_obj_t *arc, lv_arc_style_t type, lv_style_t *style)"},
    {"get_angle_start", (PyCFunction) pylv_arc_get_angle_start, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_arc_get_angle_start(lv_obj_t *arc)"},
    {"get_angle_end", (PyCFunction) pylv_arc_get_angle_end, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_arc_get_angle_end(lv_obj_t *arc)"},
    {"get_style", (PyCFunction) pylv_arc_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_arc_get_style(const lv_obj_t *arc, lv_arc_style_t type)"},
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
    *lv_obj_get_user_data(self->ref) = self;
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_preload_set_style: Parameter type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_preload_set_animation_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    unsigned char type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &type)) return NULL;

    LVGL_LOCK         
    lv_preload_set_animation_type(self->ref, type);
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
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_preload_get_style: Return type not found >lv_style_t*< ");
    return NULL;
}

static PyObject*
pylv_preload_get_animation_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    unsigned char result = lv_preload_get_animation_type(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("b", result);
}


static PyMethodDef pylv_preload_methods[] = {
    {"set_arc_length", (PyCFunction) pylv_preload_set_arc_length, METH_VARARGS | METH_KEYWORDS, "void lv_preload_set_arc_length(lv_obj_t *preload, uint16_t deg)"},
    {"set_spin_time", (PyCFunction) pylv_preload_set_spin_time, METH_VARARGS | METH_KEYWORDS, "void lv_preload_set_spin_time(lv_obj_t *preload, uint16_t time)"},
    {"set_style", (PyCFunction) pylv_preload_set_style, METH_VARARGS | METH_KEYWORDS, "void lv_preload_set_style(lv_obj_t *preload, lv_preload_style_t type, lv_style_t *style)"},
    {"set_animation_type", (PyCFunction) pylv_preload_set_animation_type, METH_VARARGS | METH_KEYWORDS, "void lv_preload_set_animation_type(lv_obj_t *preload, lv_preloader_type_t type)"},
    {"get_arc_length", (PyCFunction) pylv_preload_get_arc_length, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_preload_get_arc_length(const lv_obj_t *preload)"},
    {"get_spin_time", (PyCFunction) pylv_preload_get_spin_time, METH_VARARGS | METH_KEYWORDS, "uint16_t lv_preload_get_spin_time(const lv_obj_t *preload)"},
    {"get_style", (PyCFunction) pylv_preload_get_style, METH_VARARGS | METH_KEYWORDS, "lv_style_t *lv_preload_get_style(const lv_obj_t *preload, lv_preload_style_t type)"},
    {"get_animation_type", (PyCFunction) pylv_preload_get_animation_type, METH_VARARGS | METH_KEYWORDS, "lv_preloader_type_t lv_preload_get_animation_type(lv_obj_t *preload)"},
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
pylv_spinbox_dealloc(pylv_Spinbox *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

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
    *lv_obj_get_user_data(self->ref) = self;
    LVGL_UNLOCK

    return 0;
}


static PyObject*
pylv_spinbox_set_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
pylv_spinbox_set_value_changed_cb(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented: lv_spinbox_set_value_changed_cb: Parameter type not found >lv_spinbox_value_changed_cb_t< ");
    return NULL;
}

static PyObject*
pylv_spinbox_set_padding_left(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    LVGL_LOCK        
    int result = lv_spinbox_get_value(self->ref);
    LVGL_UNLOCK
    return Py_BuildValue("I", result);
}

static PyObject*
pylv_spinbox_step_next(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
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
    {"set_value_changed_cb", (PyCFunction) pylv_spinbox_set_value_changed_cb, METH_VARARGS | METH_KEYWORDS, "void lv_spinbox_set_value_changed_cb(lv_obj_t *spinbox, lv_spinbox_value_changed_cb_t cb)"},
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

    pylv_spinbox_Type.tp_base = &pylv_ta_Type;
    if (PyType_Ready(&pylv_spinbox_Type) < 0) return NULL;


    if (PyType_Ready(&Blob_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_mem_monitor_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_ll_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_task_t_Type) < 0) return NULL;

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

    if (PyType_Ready(&pylv_style_anim_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_obj_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_obj_type_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_group_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_theme_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_cont_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_btn_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_img_header_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_img_dsc_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_imgbtn_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_fs_file_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_fs_dir_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_fs_drv_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_label_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_img_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_line_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_page_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_list_ext_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_chart_series_t_Type) < 0) return NULL;

    if (PyType_Ready(&pylv_chart_ext_t_Type) < 0) return NULL;

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

    if (PyType_Ready(&pylv_spinbox_ext_t_Type) < 0) return NULL;




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

    Py_INCREF(&pylv_spinbox_Type);
    PyModule_AddObject(module, "Spinbox", (PyObject *) &pylv_spinbox_Type); 



    Py_INCREF(&pylv_mem_monitor_t_Type);
    PyModule_AddObject(module, "mem_monitor_t", (PyObject *) &pylv_mem_monitor_t_Type); 

    Py_INCREF(&pylv_ll_t_Type);
    PyModule_AddObject(module, "ll_t", (PyObject *) &pylv_ll_t_Type); 

    Py_INCREF(&pylv_task_t_Type);
    PyModule_AddObject(module, "task_t", (PyObject *) &pylv_task_t_Type); 

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

    Py_INCREF(&pylv_style_anim_t_Type);
    PyModule_AddObject(module, "style_anim_t", (PyObject *) &pylv_style_anim_t_Type); 

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

    Py_INCREF(&pylv_img_header_t_Type);
    PyModule_AddObject(module, "img_header_t", (PyObject *) &pylv_img_header_t_Type); 

    Py_INCREF(&pylv_img_dsc_t_Type);
    PyModule_AddObject(module, "img_dsc_t", (PyObject *) &pylv_img_dsc_t_Type); 

    Py_INCREF(&pylv_imgbtn_ext_t_Type);
    PyModule_AddObject(module, "imgbtn_ext_t", (PyObject *) &pylv_imgbtn_ext_t_Type); 

    Py_INCREF(&pylv_fs_file_t_Type);
    PyModule_AddObject(module, "fs_file_t", (PyObject *) &pylv_fs_file_t_Type); 

    Py_INCREF(&pylv_fs_dir_t_Type);
    PyModule_AddObject(module, "fs_dir_t", (PyObject *) &pylv_fs_dir_t_Type); 

    Py_INCREF(&pylv_fs_drv_t_Type);
    PyModule_AddObject(module, "fs_drv_t", (PyObject *) &pylv_fs_drv_t_Type); 

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

    Py_INCREF(&pylv_chart_ext_t_Type);
    PyModule_AddObject(module, "chart_ext_t", (PyObject *) &pylv_chart_ext_t_Type); 

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

    Py_INCREF(&pylv_spinbox_ext_t_Type);
    PyModule_AddObject(module, "spinbox_ext_t", (PyObject *) &pylv_spinbox_ext_t_Type); 


    
    PyModule_AddObject(module, "TASK_PRIO", build_enum("TASK_PRIO", "OFF", LV_TASK_PRIO_OFF, "LOWEST", LV_TASK_PRIO_LOWEST, "LOW", LV_TASK_PRIO_LOW, "MID", LV_TASK_PRIO_MID, "HIGH", LV_TASK_PRIO_HIGH, "HIGHEST", LV_TASK_PRIO_HIGHEST, "NUM", LV_TASK_PRIO_NUM, NULL));
    PyModule_AddObject(module, "INDEV_TYPE", build_enum("INDEV_TYPE", "NONE", LV_INDEV_TYPE_NONE, "POINTER", LV_INDEV_TYPE_POINTER, "KEYPAD", LV_INDEV_TYPE_KEYPAD, "BUTTON", LV_INDEV_TYPE_BUTTON, "ENCODER", LV_INDEV_TYPE_ENCODER, NULL));
    PyModule_AddObject(module, "INDEV_STATE", build_enum("INDEV_STATE", "REL", LV_INDEV_STATE_REL, "PR", LV_INDEV_STATE_PR, NULL));
    PyModule_AddObject(module, "BORDER", build_enum("BORDER", "NONE", LV_BORDER_NONE, "BOTTOM", LV_BORDER_BOTTOM, "TOP", LV_BORDER_TOP, "LEFT", LV_BORDER_LEFT, "RIGHT", LV_BORDER_RIGHT, "FULL", LV_BORDER_FULL, "INTERNAL", LV_BORDER_INTERNAL, NULL));
    PyModule_AddObject(module, "SHADOW", build_enum("SHADOW", "BOTTOM", LV_SHADOW_BOTTOM, "FULL", LV_SHADOW_FULL, NULL));
    PyModule_AddObject(module, "DESIGN", build_enum("DESIGN", "DRAW_MAIN", LV_DESIGN_DRAW_MAIN, "DRAW_POST", LV_DESIGN_DRAW_POST, "COVER_CHK", LV_DESIGN_COVER_CHK, NULL));
    PyModule_AddObject(module, "RES", build_enum("RES", "INV", LV_RES_INV, "OK", LV_RES_OK, NULL));
    PyModule_AddObject(module, "SIGNAL", build_enum("SIGNAL", "CLEANUP", LV_SIGNAL_CLEANUP, "CHILD_CHG", LV_SIGNAL_CHILD_CHG, "CORD_CHG", LV_SIGNAL_CORD_CHG, "PARENT_SIZE_CHG", LV_SIGNAL_PARENT_SIZE_CHG, "STYLE_CHG", LV_SIGNAL_STYLE_CHG, "REFR_EXT_SIZE", LV_SIGNAL_REFR_EXT_SIZE, "GET_TYPE", LV_SIGNAL_GET_TYPE, "PRESSED", LV_SIGNAL_PRESSED, "PRESSING", LV_SIGNAL_PRESSING, "PRESS_LOST", LV_SIGNAL_PRESS_LOST, "RELEASED", LV_SIGNAL_RELEASED, "LONG_PRESS", LV_SIGNAL_LONG_PRESS, "LONG_PRESS_REP", LV_SIGNAL_LONG_PRESS_REP, "DRAG_BEGIN", LV_SIGNAL_DRAG_BEGIN, "DRAG_END", LV_SIGNAL_DRAG_END, "FOCUS", LV_SIGNAL_FOCUS, "DEFOCUS", LV_SIGNAL_DEFOCUS, "CONTROLL", LV_SIGNAL_CONTROLL, "GET_EDITABLE", LV_SIGNAL_GET_EDITABLE, NULL));
    PyModule_AddObject(module, "ALIGN", build_enum("ALIGN", "CENTER", LV_ALIGN_CENTER, "IN_TOP_LEFT", LV_ALIGN_IN_TOP_LEFT, "IN_TOP_MID", LV_ALIGN_IN_TOP_MID, "IN_TOP_RIGHT", LV_ALIGN_IN_TOP_RIGHT, "IN_BOTTOM_LEFT", LV_ALIGN_IN_BOTTOM_LEFT, "IN_BOTTOM_MID", LV_ALIGN_IN_BOTTOM_MID, "IN_BOTTOM_RIGHT", LV_ALIGN_IN_BOTTOM_RIGHT, "IN_LEFT_MID", LV_ALIGN_IN_LEFT_MID, "IN_RIGHT_MID", LV_ALIGN_IN_RIGHT_MID, "OUT_TOP_LEFT", LV_ALIGN_OUT_TOP_LEFT, "OUT_TOP_MID", LV_ALIGN_OUT_TOP_MID, "OUT_TOP_RIGHT", LV_ALIGN_OUT_TOP_RIGHT, "OUT_BOTTOM_LEFT", LV_ALIGN_OUT_BOTTOM_LEFT, "OUT_BOTTOM_MID", LV_ALIGN_OUT_BOTTOM_MID, "OUT_BOTTOM_RIGHT", LV_ALIGN_OUT_BOTTOM_RIGHT, "OUT_LEFT_TOP", LV_ALIGN_OUT_LEFT_TOP, "OUT_LEFT_MID", LV_ALIGN_OUT_LEFT_MID, "OUT_LEFT_BOTTOM", LV_ALIGN_OUT_LEFT_BOTTOM, "OUT_RIGHT_TOP", LV_ALIGN_OUT_RIGHT_TOP, "OUT_RIGHT_MID", LV_ALIGN_OUT_RIGHT_MID, "OUT_RIGHT_BOTTOM", LV_ALIGN_OUT_RIGHT_BOTTOM, NULL));
    PyModule_AddObject(module, "PROTECT", build_enum("PROTECT", "NONE", LV_PROTECT_NONE, "CHILD_CHG", LV_PROTECT_CHILD_CHG, "PARENT", LV_PROTECT_PARENT, "POS", LV_PROTECT_POS, "FOLLOW", LV_PROTECT_FOLLOW, "PRESS_LOST", LV_PROTECT_PRESS_LOST, "CLICK_FOCUS", LV_PROTECT_CLICK_FOCUS, NULL));
    PyModule_AddObject(module, "ANIM", build_enum("ANIM", "NONE", LV_ANIM_NONE, "FLOAT_TOP", LV_ANIM_FLOAT_TOP, "FLOAT_LEFT", LV_ANIM_FLOAT_LEFT, "FLOAT_BOTTOM", LV_ANIM_FLOAT_BOTTOM, "FLOAT_RIGHT", LV_ANIM_FLOAT_RIGHT, "GROW_H", LV_ANIM_GROW_H, "GROW_V", LV_ANIM_GROW_V, NULL));
    PyModule_AddObject(module, "LAYOUT", build_enum("LAYOUT", "OFF", LV_LAYOUT_OFF, "CENTER", LV_LAYOUT_CENTER, "COL_L", LV_LAYOUT_COL_L, "COL_M", LV_LAYOUT_COL_M, "COL_R", LV_LAYOUT_COL_R, "ROW_T", LV_LAYOUT_ROW_T, "ROW_M", LV_LAYOUT_ROW_M, "ROW_B", LV_LAYOUT_ROW_B, "PRETTY", LV_LAYOUT_PRETTY, "GRID", LV_LAYOUT_GRID, NULL));
    PyModule_AddObject(module, "BTN_STATE", build_enum("BTN_STATE", "REL", LV_BTN_STATE_REL, "PR", LV_BTN_STATE_PR, "TGL_REL", LV_BTN_STATE_TGL_REL, "TGL_PR", LV_BTN_STATE_TGL_PR, "INA", LV_BTN_STATE_INA, "NUM", LV_BTN_STATE_NUM, NULL));
    PyModule_AddObject(module, "BTN_STYLE", build_enum("BTN_STYLE", "REL", LV_BTN_STYLE_REL, "PR", LV_BTN_STYLE_PR, "TGL_REL", LV_BTN_STYLE_TGL_REL, "TGL_PR", LV_BTN_STYLE_TGL_PR, "INA", LV_BTN_STYLE_INA, NULL));
    PyModule_AddObject(module, "TXT_FLAG", build_enum("TXT_FLAG", "NONE", LV_TXT_FLAG_NONE, "RECOLOR", LV_TXT_FLAG_RECOLOR, "EXPAND", LV_TXT_FLAG_EXPAND, "CENTER", LV_TXT_FLAG_CENTER, "RIGHT", LV_TXT_FLAG_RIGHT, NULL));
    PyModule_AddObject(module, "TXT_CMD_STATE", build_enum("TXT_CMD_STATE", "WAIT", LV_TXT_CMD_STATE_WAIT, "PAR", LV_TXT_CMD_STATE_PAR, "IN", LV_TXT_CMD_STATE_IN, NULL));
    PyModule_AddObject(module, "IMG_SRC", build_enum("IMG_SRC", "VARIABLE", LV_IMG_SRC_VARIABLE, "FILE", LV_IMG_SRC_FILE, "SYMBOL", LV_IMG_SRC_SYMBOL, "UNKNOWN", LV_IMG_SRC_UNKNOWN, NULL));
    PyModule_AddObject(module, "IMG_CF", build_enum("IMG_CF", "UNKNOWN", LV_IMG_CF_UNKNOWN, "RAW", LV_IMG_CF_RAW, "RAW_ALPHA", LV_IMG_CF_RAW_ALPHA, "RAW_CHROMA_KEYED", LV_IMG_CF_RAW_CHROMA_KEYED, "TRUE_COLOR", LV_IMG_CF_TRUE_COLOR, "TRUE_COLOR_ALPHA", LV_IMG_CF_TRUE_COLOR_ALPHA, "TRUE_COLOR_CHROMA_KEYED", LV_IMG_CF_TRUE_COLOR_CHROMA_KEYED, "INDEXED_1BIT", LV_IMG_CF_INDEXED_1BIT, "INDEXED_2BIT", LV_IMG_CF_INDEXED_2BIT, "INDEXED_4BIT", LV_IMG_CF_INDEXED_4BIT, "INDEXED_8BIT", LV_IMG_CF_INDEXED_8BIT, "ALPHA_1BIT", LV_IMG_CF_ALPHA_1BIT, "ALPHA_2BIT", LV_IMG_CF_ALPHA_2BIT, "ALPHA_4BIT", LV_IMG_CF_ALPHA_4BIT, "ALPHA_8BIT", LV_IMG_CF_ALPHA_8BIT, NULL));
    PyModule_AddObject(module, "IMGBTN_STYLE", build_enum("IMGBTN_STYLE", "REL", LV_IMGBTN_STYLE_REL, "PR", LV_IMGBTN_STYLE_PR, "TGL_REL", LV_IMGBTN_STYLE_TGL_REL, "TGL_PR", LV_IMGBTN_STYLE_TGL_PR, "INA", LV_IMGBTN_STYLE_INA, NULL));
    PyModule_AddObject(module, "FS_RES", build_enum("FS_RES", "OK", LV_FS_RES_OK, "HW_ERR", LV_FS_RES_HW_ERR, "FS_ERR", LV_FS_RES_FS_ERR, "NOT_EX", LV_FS_RES_NOT_EX, "FULL", LV_FS_RES_FULL, "LOCKED", LV_FS_RES_LOCKED, "DENIED", LV_FS_RES_DENIED, "BUSY", LV_FS_RES_BUSY, "TOUT", LV_FS_RES_TOUT, "NOT_IMP", LV_FS_RES_NOT_IMP, "OUT_OF_MEM", LV_FS_RES_OUT_OF_MEM, "INV_PARAM", LV_FS_RES_INV_PARAM, "UNKNOWN", LV_FS_RES_UNKNOWN, NULL));
    PyModule_AddObject(module, "FS_MODE", build_enum("FS_MODE", "WR", LV_FS_MODE_WR, "RD", LV_FS_MODE_RD, NULL));
    PyModule_AddObject(module, "LABEL_LONG", build_enum("LABEL_LONG", "EXPAND", LV_LABEL_LONG_EXPAND, "BREAK", LV_LABEL_LONG_BREAK, "SCROLL", LV_LABEL_LONG_SCROLL, "DOT", LV_LABEL_LONG_DOT, "ROLL", LV_LABEL_LONG_ROLL, "CROP", LV_LABEL_LONG_CROP, NULL));
    PyModule_AddObject(module, "LABEL_ALIGN", build_enum("LABEL_ALIGN", "LEFT", LV_LABEL_ALIGN_LEFT, "CENTER", LV_LABEL_ALIGN_CENTER, "RIGHT", LV_LABEL_ALIGN_RIGHT, NULL));
    PyModule_AddObject(module, "SB_MODE", build_enum("SB_MODE", "OFF", LV_SB_MODE_OFF, "ON", LV_SB_MODE_ON, "DRAG", LV_SB_MODE_DRAG, "AUTO", LV_SB_MODE_AUTO, "HIDE", LV_SB_MODE_HIDE, "UNHIDE", LV_SB_MODE_UNHIDE, NULL));
    PyModule_AddObject(module, "PAGE_STYLE", build_enum("PAGE_STYLE", "BG", LV_PAGE_STYLE_BG, "SCRL", LV_PAGE_STYLE_SCRL, "SB", LV_PAGE_STYLE_SB, "EDGE_FLASH", LV_PAGE_STYLE_EDGE_FLASH, NULL));
    PyModule_AddObject(module, "LIST_STYLE", build_enum("LIST_STYLE", "BG", LV_LIST_STYLE_BG, "SCRL", LV_LIST_STYLE_SCRL, "SB", LV_LIST_STYLE_SB, "EDGE_FLASH", LV_LIST_STYLE_EDGE_FLASH, "BTN_REL", LV_LIST_STYLE_BTN_REL, "BTN_PR", LV_LIST_STYLE_BTN_PR, "BTN_TGL_REL", LV_LIST_STYLE_BTN_TGL_REL, "BTN_TGL_PR", LV_LIST_STYLE_BTN_TGL_PR, "BTN_INA", LV_LIST_STYLE_BTN_INA, NULL));
    PyModule_AddObject(module, "CHART_TYPE", build_enum("CHART_TYPE", "LINE", LV_CHART_TYPE_LINE, "COLUMN", LV_CHART_TYPE_COLUMN, "POINT", LV_CHART_TYPE_POINT, "VERTICAL_LINE", LV_CHART_TYPE_VERTICAL_LINE, NULL));
    PyModule_AddObject(module, "TABLE_STYLE", build_enum("TABLE_STYLE", "BG", LV_TABLE_STYLE_BG, "CELL1", LV_TABLE_STYLE_CELL1, "CELL2", LV_TABLE_STYLE_CELL2, "CELL3", LV_TABLE_STYLE_CELL3, "CELL4", LV_TABLE_STYLE_CELL4, NULL));
    PyModule_AddObject(module, "CB_STYLE", build_enum("CB_STYLE", "BG", LV_CB_STYLE_BG, "BOX_REL", LV_CB_STYLE_BOX_REL, "BOX_PR", LV_CB_STYLE_BOX_PR, "BOX_TGL_REL", LV_CB_STYLE_BOX_TGL_REL, "BOX_TGL_PR", LV_CB_STYLE_BOX_TGL_PR, "BOX_INA", LV_CB_STYLE_BOX_INA, NULL));
    PyModule_AddObject(module, "BAR_STYLE", build_enum("BAR_STYLE", "BG", LV_BAR_STYLE_BG, "INDIC", LV_BAR_STYLE_INDIC, NULL));
    PyModule_AddObject(module, "SLIDER_STYLE", build_enum("SLIDER_STYLE", "BG", LV_SLIDER_STYLE_BG, "INDIC", LV_SLIDER_STYLE_INDIC, "KNOB", LV_SLIDER_STYLE_KNOB, NULL));
    PyModule_AddObject(module, "BTNM_STYLE", build_enum("BTNM_STYLE", "BG", LV_BTNM_STYLE_BG, "BTN_REL", LV_BTNM_STYLE_BTN_REL, "BTN_PR", LV_BTNM_STYLE_BTN_PR, "BTN_TGL_REL", LV_BTNM_STYLE_BTN_TGL_REL, "BTN_TGL_PR", LV_BTNM_STYLE_BTN_TGL_PR, "BTN_INA", LV_BTNM_STYLE_BTN_INA, NULL));
    PyModule_AddObject(module, "KB_MODE", build_enum("KB_MODE", "TEXT", LV_KB_MODE_TEXT, "NUM", LV_KB_MODE_NUM, NULL));
    PyModule_AddObject(module, "KB_STYLE", build_enum("KB_STYLE", "BG", LV_KB_STYLE_BG, "BTN_REL", LV_KB_STYLE_BTN_REL, "BTN_PR", LV_KB_STYLE_BTN_PR, "BTN_TGL_REL", LV_KB_STYLE_BTN_TGL_REL, "BTN_TGL_PR", LV_KB_STYLE_BTN_TGL_PR, "BTN_INA", LV_KB_STYLE_BTN_INA, NULL));
    PyModule_AddObject(module, "DDLIST_STYLE", build_enum("DDLIST_STYLE", "BG", LV_DDLIST_STYLE_BG, "SEL", LV_DDLIST_STYLE_SEL, "SB", LV_DDLIST_STYLE_SB, NULL));
    PyModule_AddObject(module, "ROLLER_STYLE", build_enum("ROLLER_STYLE", "BG", LV_ROLLER_STYLE_BG, "SEL", LV_ROLLER_STYLE_SEL, NULL));
    PyModule_AddObject(module, "CURSOR", build_enum("CURSOR", "NONE", LV_CURSOR_NONE, "LINE", LV_CURSOR_LINE, "BLOCK", LV_CURSOR_BLOCK, "OUTLINE", LV_CURSOR_OUTLINE, "UNDERLINE", LV_CURSOR_UNDERLINE, "HIDDEN", LV_CURSOR_HIDDEN, NULL));
    PyModule_AddObject(module, "TA_STYLE", build_enum("TA_STYLE", "BG", LV_TA_STYLE_BG, "SB", LV_TA_STYLE_SB, "EDGE_FLASH", LV_TA_STYLE_EDGE_FLASH, "CURSOR", LV_TA_STYLE_CURSOR, "PLACEHOLDER", LV_TA_STYLE_PLACEHOLDER, NULL));
    PyModule_AddObject(module, "CANVAS_STYLE", build_enum("CANVAS_STYLE", "MAIN", LV_CANVAS_STYLE_MAIN, NULL));
    PyModule_AddObject(module, "WIN_STYLE", build_enum("WIN_STYLE", "BG", LV_WIN_STYLE_BG, "CONTENT_BG", LV_WIN_STYLE_CONTENT_BG, "CONTENT_SCRL", LV_WIN_STYLE_CONTENT_SCRL, "SB", LV_WIN_STYLE_SB, "HEADER", LV_WIN_STYLE_HEADER, "BTN_REL", LV_WIN_STYLE_BTN_REL, "BTN_PR", LV_WIN_STYLE_BTN_PR, NULL));
    PyModule_AddObject(module, "TABVIEW_BTNS_POS", build_enum("TABVIEW_BTNS_POS", "TOP", LV_TABVIEW_BTNS_POS_TOP, "BOTTOM", LV_TABVIEW_BTNS_POS_BOTTOM, NULL));
    PyModule_AddObject(module, "TABVIEW_STYLE", build_enum("TABVIEW_STYLE", "BG", LV_TABVIEW_STYLE_BG, "INDIC", LV_TABVIEW_STYLE_INDIC, "BTN_BG", LV_TABVIEW_STYLE_BTN_BG, "BTN_REL", LV_TABVIEW_STYLE_BTN_REL, "BTN_PR", LV_TABVIEW_STYLE_BTN_PR, "BTN_TGL_REL", LV_TABVIEW_STYLE_BTN_TGL_REL, "BTN_TGL_PR", LV_TABVIEW_STYLE_BTN_TGL_PR, NULL));
    PyModule_AddObject(module, "TILEVIEW_STYLE", build_enum("TILEVIEW_STYLE", "BG", LV_TILEVIEW_STYLE_BG, NULL));
    PyModule_AddObject(module, "MBOX_STYLE", build_enum("MBOX_STYLE", "BG", LV_MBOX_STYLE_BG, "BTN_BG", LV_MBOX_STYLE_BTN_BG, "BTN_REL", LV_MBOX_STYLE_BTN_REL, "BTN_PR", LV_MBOX_STYLE_BTN_PR, "BTN_TGL_REL", LV_MBOX_STYLE_BTN_TGL_REL, "BTN_TGL_PR", LV_MBOX_STYLE_BTN_TGL_PR, "BTN_INA", LV_MBOX_STYLE_BTN_INA, NULL));
    PyModule_AddObject(module, "SW_STYLE", build_enum("SW_STYLE", "BG", LV_SW_STYLE_BG, "INDIC", LV_SW_STYLE_INDIC, "KNOB_OFF", LV_SW_STYLE_KNOB_OFF, "KNOB_ON", LV_SW_STYLE_KNOB_ON, NULL));
    PyModule_AddObject(module, "ARC_STYLE", build_enum("ARC_STYLE", "MAIN", LV_ARC_STYLE_MAIN, NULL));
    PyModule_AddObject(module, "PRELOAD_TYPE", build_enum("PRELOAD_TYPE", "SPINNING_ARC", LV_PRELOAD_TYPE_SPINNING_ARC, "FILLSPIN_ARC", LV_PRELOAD_TYPE_FILLSPIN_ARC, NULL));
    PyModule_AddObject(module, "PRELOAD_STYLE", build_enum("PRELOAD_STYLE", "MAIN", LV_PRELOAD_STYLE_MAIN, NULL));
    PyModule_AddObject(module, "SPINBOX_STYLE", build_enum("SPINBOX_STYLE", "BG", LV_SPINBOX_STYLE_BG, "SB", LV_SPINBOX_STYLE_SB, "CURSOR", LV_SPINBOX_STYLE_CURSOR, NULL));




    // refcount for typesdict is initally 1; it is used by pyobj_from_lv
    // refcounts to py{name}_Type objects are incremented due to "O" format
    typesdict = Py_BuildValue("{sOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsOsO}",
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

