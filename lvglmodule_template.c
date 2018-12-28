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

<<<objects:{structcode}>>>

/****************************************************************
 * Forward declaration of type objects                          *
 ****************************************************************/

PyObject *typesdict = NULL;

<<<objects:
static PyTypeObject pylv_{name}_Type;
>>>

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
<<STYLE_GETSET>>
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



<<BTN_CALLBACKS>>

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

<<<objects:
static void
pylv_{name}_dealloc(pylv_{pyname} *self) 
{{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}}

static int
pylv_{name}_init(pylv_{pyname} *self, PyObject *args, PyObject *kwds) 
{{
    static char *kwlist[] = {{"parent", "copy", NULL}};
    pylv_Obj *parent=NULL;
    pylv_{pyname} *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!O!", kwlist, &pylv_obj_Type, &parent, &pylv_{name}_Type, &copy)) {{
        return -1;
    }}   
    
    LVGL_LOCK
    self->ref = lv_{name}_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    LVGL_UNLOCK

    return 0;
}}

{methodscode}

static PyMethodDef pylv_{name}_methods[] = {{
{methodtablecode}    {{NULL}}  /* Sentinel */
}};

static PyTypeObject pylv_{name}_Type = {{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.{pyname}",
    .tp_doc = "lvgl {pyname}",
    .tp_basicsize = sizeof(pylv_{pyname}),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_{name}_init,
    .tp_dealloc = (destructor) pylv_{name}_dealloc,
    .tp_methods = pylv_{name}_methods,
}};
>>>


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
    
<<<objects:
    pylv_{name}_Type.tp_base = {base};
    if (PyType_Ready(&pylv_{name}_Type) < 0) return NULL;
>>>

    Py_INCREF(&Style_Type);
    PyModule_AddObject(module, "Style", (PyObject *) &Style_Type); 

<<<objects:
    Py_INCREF(&pylv_{name}_Type);
    PyModule_AddObject(module, "{pyname}", (PyObject *) &pylv_{name}_Type); 
>>>

    
<<ENUM_ASSIGNMENTS>>
    
<<STYLE_ASSIGNMENTS>>

<<FONT_ASSIGNMENTS>>

<<SYMBOL_ASSIGNMENTS>>

    // refcount for typesdict is initally 1; it is used by pyobj_from_lv
    // refcounts to py{name}_Type objects are incremented due to "O" format
    typesdict = Py_BuildValue("{<<<objects:sO>>>}"<<<objects:,
        "lv_{name}", &pylv_{name}_Type>>>);
    
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

