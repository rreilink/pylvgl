#include "Python.h"
#include "structmember.h"
#include "lvgl/lvgl.h"

#if LV_COLOR_DEPTH != 16
#error Only 16 bits color depth is currently supported
#endif

/****************************************************************
 * Object struct definitions                                    *
 ****************************************************************/

typedef struct {
    PyObject_HEAD
    lv_obj_t *ref;
} pylv_Obj;

typedef pylv_Obj pylv_Win;

typedef pylv_Obj pylv_Label;

typedef pylv_Obj pylv_Lmeter;

typedef struct {
    PyObject_HEAD
    lv_obj_t *ref;
    const char **map;
} pylv_Btnm;

typedef pylv_Obj pylv_Chart;

typedef pylv_Obj pylv_Cont;

typedef pylv_Obj pylv_Led;

typedef struct {
    PyObject_HEAD
    lv_obj_t *ref;
    const char **map;
    PyObject *ok_action;
    PyObject *hide_action;
} pylv_Kb;

typedef pylv_Obj pylv_Img;

typedef pylv_Obj pylv_Bar;

typedef pylv_Obj pylv_Line;

typedef pylv_Obj pylv_Tabview;

typedef pylv_Cont pylv_Mbox;

typedef pylv_Lmeter pylv_Gauge;

typedef struct {
    PyObject_HEAD
    lv_obj_t *ref;
    PyObject *rel_action;
    PyObject *pr_action;
} pylv_Page;

typedef pylv_Page pylv_Ta;

typedef struct {
    PyObject_HEAD
    lv_obj_t *ref;
    PyObject *actions[LV_BTN_ACTION_NUM];
} pylv_Btn;

typedef struct {
    PyObject_HEAD
    lv_obj_t *ref;
    PyObject *rel_action;
    PyObject *pr_action;
    PyObject *action;
} pylv_Ddlist;

typedef pylv_Page pylv_List;

typedef struct {
    PyObject_HEAD
    lv_obj_t *ref;
    PyObject *action;
} pylv_Slider;

typedef pylv_Slider pylv_Sw;

typedef struct {
    PyObject_HEAD
    lv_obj_t *ref;
    PyObject *actions[LV_BTN_ACTION_NUM];
    PyObject *action;
} pylv_Cb;

typedef pylv_Ddlist pylv_Roller;



/****************************************************************
 * Forward declaration of type objects                          *
 ****************************************************************/


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

static PyTypeObject pylv_line_Type;

static PyTypeObject pylv_tabview_Type;

static PyTypeObject pylv_mbox_Type;

static PyTypeObject pylv_gauge_Type;

static PyTypeObject pylv_page_Type;

static PyTypeObject pylv_ta_Type;

static PyTypeObject pylv_btn_Type;

static PyTypeObject pylv_ddlist_Type;

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

PyObject * pyobj_from_lv(lv_obj_t *obj, PyTypeObject *tp) {
    pylv_Obj *pyobj;
    if (!obj) {
        Py_RETURN_NONE;
    }
    
    pyobj = lv_obj_get_free_ptr(obj);
    
    if (pyobj) {
        Py_INCREF(pyobj); // increase reference count of returned object
    } else {
        // Python object for this lv object does not yet exist. Create a new one
        // Be sure to zero out the memory
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
   {"body_main_color", (getter) Style_get_uint16, (setter) Style_set_uint16, "body.main_color", (void*)offsetof(lv_style_t, body.main_color)},
   {"body_grad_color", (getter) Style_get_uint16, (setter) Style_set_uint16, "body.grad_color", (void*)offsetof(lv_style_t, body.grad_color)},
   {"body_radius", (getter) Style_get_int16, (setter) Style_set_int16, "body.radius", (void*)offsetof(lv_style_t, body.radius)},
   {"body_opa", (getter) Style_get_uint8, (setter) Style_set_uint8, "body.opa", (void*)offsetof(lv_style_t, body.opa)},
   {"body_border_color", (getter) Style_get_uint16, (setter) Style_set_uint16, "body.border.color", (void*)offsetof(lv_style_t, body.border.color)},
   {"body_border_width", (getter) Style_get_int16, (setter) Style_set_int16, "body.border.width", (void*)offsetof(lv_style_t, body.border.width)},
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
    pylv_Obj *pychild;
    PyObject *ret = PyList_New(0);
    if (!ret) return NULL;
    
    if (lock) lock(lock_arg);
    
    while (1) {
        child = lv_obj_get_child(self->ref, child);
        if (!child) break;
        pychild = lv_obj_get_free_ptr(child);
        
        if (!pychild) {
            // Child is not known to Python, create a new Object instance
            pychild = PyObject_New(pylv_Obj, &pylv_obj_Type);
            pychild -> ref = child;
            lv_obj_set_free_ptr(child, pychild);
        }
        
        // Child that is known in Python
        if (PyList_Append(ret, (PyObject *)pychild)) { // PyList_Append increases refcount
            Py_XDECREF(ret);
            ret = NULL;
            break;
        }
    }

    if (unlock) unlock(unlock_arg);
    
    return ret;
}

static PyObject*
pylv_obj_get_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    lv_obj_type_t result;
    PyObject *list = NULL;
    PyObject *str = NULL;
    
    if (lock) lock(lock_arg);
    lv_obj_get_type(self->ref, &result);
    
    list = PyList_New(0);
    
    for(int i=0; i<LV_MAX_ANCESTOR_NUM;i++) {
        if (!result.type[i]) break;

        str = PyUnicode_FromString(result.type[i]);
        if (!str) goto error;
        
        if (PyList_Append(list, str)) goto error; // PyList_Append increases refcount
    }

    if (unlock) unlock(unlock_arg);   
    return list;
    
error:
    Py_XDECREF(list);
    Py_XDECREF(str);
    if (unlock) unlock(unlock_arg);   
    return NULL;
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
    
    if (lock) lock(lock_arg);
    lv_btnm_set_map(self->ref, cmap);
    if (unlock) unlock(unlock_arg);

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

    if (lock) lock(lock_arg);
    ret = pyobj_from_lv(lv_list_add(self->ref, NULL, txt, NULL), &pylv_btn_Type);
    if (unlock) unlock(unlock_arg);
    
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
    
    if (lock) lock(lock_arg);         
    
    parent = lv_obj_get_parent(obj->ref);
    if (parent) parent = lv_obj_get_parent(parent); // get the obj's parent's parent in a safe way
    
    if (parent != self->ref) {
        if (unlock) unlock(unlock_arg);
        return PyErr_Format(PyExc_RuntimeError, "%R is not a child of %R", obj, self);
    }
    
    lv_list_focus(obj->ref, anim_en);
    
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}




lv_res_t pylv_btn_action_click_callback(lv_obj_t* obj) {
    pylv_Btn *pyobj;
    PyObject *handler;
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->actions[0];
        if (handler) PyObject_CallFunctionObjArgs(handler, NULL);
        if (PyErr_Occurred()) PyErr_Print();
    }
    return LV_RES_OK;
}

lv_res_t pylv_btn_action_pr_callback(lv_obj_t* obj) {
    pylv_Btn *pyobj;
    PyObject *handler;
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->actions[1];
        if (handler) PyObject_CallFunctionObjArgs(handler, NULL);
        if (PyErr_Occurred()) PyErr_Print();
    }
    return LV_RES_OK;
}

lv_res_t pylv_btn_action_long_pr_callback(lv_obj_t* obj) {
    pylv_Btn *pyobj;
    PyObject *handler;
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->actions[2];
        if (handler) PyObject_CallFunctionObjArgs(handler, NULL);
        if (PyErr_Occurred()) PyErr_Print();
    }
    return LV_RES_OK;
}

lv_res_t pylv_btn_action_long_pr_repeat_callback(lv_obj_t* obj) {
    pylv_Btn *pyobj;
    PyObject *handler;
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->actions[3];
        if (handler) PyObject_CallFunctionObjArgs(handler, NULL);
        if (PyErr_Occurred()) PyErr_Print();
    }
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_obj_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

    return 0;
}


static PyObject*
pylv_obj_invalidate(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    lv_obj_invalidate(self->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_parent(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"parent", NULL};
    pylv_Obj * parent;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &pylv_obj_Type, &parent)) return NULL;

    if (lock) lock(lock_arg);         
    lv_obj_set_parent(self->ref, parent->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_pos(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"x", "y", NULL};
    short int x;
    short int y;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &x, &y)) return NULL;

    if (lock) lock(lock_arg);         
    lv_obj_set_pos(self->ref, x, y);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_x(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"x", NULL};
    short int x;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &x)) return NULL;

    if (lock) lock(lock_arg);         
    lv_obj_set_x(self->ref, x);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_y(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"y", NULL};
    short int y;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &y)) return NULL;

    if (lock) lock(lock_arg);         
    lv_obj_set_y(self->ref, y);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"w", "h", NULL};
    short int w;
    short int h;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &w, &h)) return NULL;

    if (lock) lock(lock_arg);         
    lv_obj_set_size(self->ref, w, h);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"w", NULL};
    short int w;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &w)) return NULL;

    if (lock) lock(lock_arg);         
    lv_obj_set_width(self->ref, w);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"h", NULL};
    short int h;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &h)) return NULL;

    if (lock) lock(lock_arg);         
    lv_obj_set_height(self->ref, h);
    if (unlock) unlock(unlock_arg);
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

    if (lock) lock(lock_arg);         
    lv_obj_align(self->ref, base->ref, align, x_mod, y_mod);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"style", NULL};
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &Style_Type, &style)) return NULL;

    if (lock) lock(lock_arg);         
    lv_obj_set_style(self->ref, style->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_refresh_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    lv_obj_refresh_style(self->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_hidden(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    if (lock) lock(lock_arg);         
    lv_obj_set_hidden(self->ref, en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_click(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    if (lock) lock(lock_arg);         
    lv_obj_set_click(self->ref, en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_top(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    if (lock) lock(lock_arg);         
    lv_obj_set_top(self->ref, en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_drag(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    if (lock) lock(lock_arg);         
    lv_obj_set_drag(self->ref, en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_drag_throw(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    if (lock) lock(lock_arg);         
    lv_obj_set_drag_throw(self->ref, en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_drag_parent(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    if (lock) lock(lock_arg);         
    lv_obj_set_drag_parent(self->ref, en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_protect(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"prot", NULL};
    unsigned char prot;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &prot)) return NULL;

    if (lock) lock(lock_arg);         
    lv_obj_set_protect(self->ref, prot);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_clear_protect(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"prot", NULL};
    unsigned char prot;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &prot)) return NULL;

    if (lock) lock(lock_arg);         
    lv_obj_clear_protect(self->ref, prot);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_signal_func(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
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

    if (lock) lock(lock_arg);         
    lv_obj_refresh_ext_size(self->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_screen(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

        
    if (lock) lock(lock_arg);
    lv_obj_t *result = lv_obj_get_screen(self->ref);
    PyObject *retobj = pyobj_from_lv(result, &pylv_obj_Type);
    if (unlock) unlock(unlock_arg);
    
    return retobj;
}

static PyObject*
pylv_obj_get_parent(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

        
    if (lock) lock(lock_arg);
    lv_obj_t *result = lv_obj_get_parent(self->ref);
    PyObject *retobj = pyobj_from_lv(result, &pylv_obj_Type);
    if (unlock) unlock(unlock_arg);
    
    return retobj;
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

    if (lock) lock(lock_arg);         
    short int result = lv_obj_get_x(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_y(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    short int result = lv_obj_get_y(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    short int result = lv_obj_get_width(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    short int result = lv_obj_get_height(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_ext_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    short int result = lv_obj_get_ext_size(self->ref);
    if (unlock) unlock(unlock_arg);
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

    if (lock) lock(lock_arg);         
    int result = lv_obj_get_hidden(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_click(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_obj_get_click(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_top(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_obj_get_top(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_drag(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_obj_get_drag(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_drag_throw(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_obj_get_drag_throw(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_drag_parent(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_obj_get_drag_parent(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_is_protected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"prot", NULL};
    unsigned char prot;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &prot)) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_obj_is_protected(self->ref, prot);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_signal_func(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
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
    {"set_protect", (PyCFunction) pylv_obj_set_protect, METH_VARARGS | METH_KEYWORDS, NULL},
    {"clear_protect", (PyCFunction) pylv_obj_clear_protect, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_signal_func", (PyCFunction) pylv_obj_set_signal_func, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_design_func", (PyCFunction) pylv_obj_set_design_func, METH_VARARGS | METH_KEYWORDS, NULL},
    {"refresh_ext_size", (PyCFunction) pylv_obj_refresh_ext_size, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_screen", (PyCFunction) pylv_obj_get_screen, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_parent", (PyCFunction) pylv_obj_get_parent, METH_VARARGS | METH_KEYWORDS, NULL},
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
    {"is_protected", (PyCFunction) pylv_obj_is_protected, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_signal_func", (PyCFunction) pylv_obj_get_signal_func, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_design_func", (PyCFunction) pylv_obj_get_design_func, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_type", (PyCFunction) pylv_obj_get_type, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_group", (PyCFunction) pylv_obj_get_group, METH_VARARGS | METH_KEYWORDS, NULL},
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_win_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

    return 0;
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

    if (lock) lock(lock_arg);         
    int result = lv_win_close_action(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_win_set_title(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"title", NULL};
    char * title;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &title)) return NULL;

    if (lock) lock(lock_arg);         
    lv_win_set_title(self->ref, title);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_btn_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"size", NULL};
    short int size;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &size)) return NULL;

    if (lock) lock(lock_arg);         
    lv_win_set_btn_size(self->ref, size);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_sb_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"sb_mode", NULL};
    int sb_mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &sb_mode)) return NULL;

    if (lock) lock(lock_arg);         
    lv_win_set_sb_mode(self->ref, sb_mode);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_layout(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"layout", NULL};
    int layout;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &layout)) return NULL;

    if (lock) lock(lock_arg);         
    lv_win_set_layout(self->ref, layout);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    if (lock) lock(lock_arg);         
    lv_win_set_style(self->ref, type, style->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_get_btn_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    short int result = lv_win_get_btn_size(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_win_get_layout(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_win_get_layout(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_win_get_sb_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_win_get_sb_mode(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_win_get_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    short int result = lv_win_get_width(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_win_get_from_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

        
    if (lock) lock(lock_arg);
    lv_obj_t *result = lv_win_get_from_btn(self->ref);
    PyObject *retobj = pyobj_from_lv(result, &pylv_obj_Type);
    if (unlock) unlock(unlock_arg);
    
    return retobj;
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

    if (lock) lock(lock_arg);         
    lv_win_focus(self->ref, obj->ref, anim_time);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}


static PyMethodDef pylv_win_methods[] = {
    {"add_btn", (PyCFunction) pylv_win_add_btn, METH_VARARGS | METH_KEYWORDS, NULL},
    {"close_action", (PyCFunction) pylv_win_close_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_title", (PyCFunction) pylv_win_set_title, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_btn_size", (PyCFunction) pylv_win_set_btn_size, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_sb_mode", (PyCFunction) pylv_win_set_sb_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_layout", (PyCFunction) pylv_win_set_layout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_win_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_btn_size", (PyCFunction) pylv_win_get_btn_size, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_layout", (PyCFunction) pylv_win_get_layout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_sb_mode", (PyCFunction) pylv_win_get_sb_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_width", (PyCFunction) pylv_win_get_width, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_from_btn", (PyCFunction) pylv_win_get_from_btn, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_win_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"focus", (PyCFunction) pylv_win_focus, METH_VARARGS | METH_KEYWORDS, NULL},
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_label_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

    return 0;
}


static PyObject*
pylv_label_set_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"text", NULL};
    char * text;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &text)) return NULL;

    if (lock) lock(lock_arg);         
    lv_label_set_text(self->ref, text);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_array_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"array", "size", NULL};
    char * array;
    unsigned short int size;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "sH", kwlist , &array, &size)) return NULL;

    if (lock) lock(lock_arg);         
    lv_label_set_array_text(self->ref, array, size);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_static_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"text", NULL};
    char * text;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &text)) return NULL;

    if (lock) lock(lock_arg);         
    lv_label_set_static_text(self->ref, text);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_long_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"long_mode", NULL};
    int long_mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &long_mode)) return NULL;

    if (lock) lock(lock_arg);         
    lv_label_set_long_mode(self->ref, long_mode);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"align", NULL};
    int align;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &align)) return NULL;

    if (lock) lock(lock_arg);         
    lv_label_set_align(self->ref, align);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"recolor_en", NULL};
    int recolor_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &recolor_en)) return NULL;

    if (lock) lock(lock_arg);         
    lv_label_set_recolor(self->ref, recolor_en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_no_break(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"no_break_en", NULL};
    int no_break_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &no_break_en)) return NULL;

    if (lock) lock(lock_arg);         
    lv_label_set_no_break(self->ref, no_break_en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_body_draw(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"body_en", NULL};
    int body_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &body_en)) return NULL;

    if (lock) lock(lock_arg);         
    lv_label_set_body_draw(self->ref, body_en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_anim_speed(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"anim_speed", NULL};
    unsigned short int anim_speed;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_speed)) return NULL;

    if (lock) lock(lock_arg);         
    lv_label_set_anim_speed(self->ref, anim_speed);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_get_long_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_label_get_long_mode(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_label_get_align(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_label_get_align(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_label_get_recolor(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_label_get_recolor(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_label_get_no_break(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_label_get_no_break(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_label_get_body_draw(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_label_get_body_draw(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_label_get_letter_pos(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_label_ins_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"pos", "txt", NULL};
    unsigned int pos;
    char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Is", kwlist , &pos, &txt)) return NULL;

    if (lock) lock(lock_arg);         
    lv_label_ins_text(self->ref, pos, txt);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_cut_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"pos", "cnt", NULL};
    unsigned int pos;
    unsigned int cnt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "II", kwlist , &pos, &cnt)) return NULL;

    if (lock) lock(lock_arg);         
    lv_label_cut_text(self->ref, pos, cnt);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}


static PyMethodDef pylv_label_methods[] = {
    {"set_text", (PyCFunction) pylv_label_set_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_array_text", (PyCFunction) pylv_label_set_array_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_static_text", (PyCFunction) pylv_label_set_static_text, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_long_mode", (PyCFunction) pylv_label_set_long_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_align", (PyCFunction) pylv_label_set_align, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_recolor", (PyCFunction) pylv_label_set_recolor, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_no_break", (PyCFunction) pylv_label_set_no_break, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_body_draw", (PyCFunction) pylv_label_set_body_draw, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_anim_speed", (PyCFunction) pylv_label_set_anim_speed, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_long_mode", (PyCFunction) pylv_label_get_long_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_align", (PyCFunction) pylv_label_get_align, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_recolor", (PyCFunction) pylv_label_get_recolor, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_no_break", (PyCFunction) pylv_label_get_no_break, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_body_draw", (PyCFunction) pylv_label_get_body_draw, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_letter_pos", (PyCFunction) pylv_label_get_letter_pos, METH_VARARGS | METH_KEYWORDS, NULL},
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_lmeter_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

    return 0;
}


static PyObject*
pylv_lmeter_set_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"value", NULL};
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &value)) return NULL;

    if (lock) lock(lock_arg);         
    lv_lmeter_set_value(self->ref, value);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_lmeter_set_range(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"min", "max", NULL};
    short int min;
    short int max;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &min, &max)) return NULL;

    if (lock) lock(lock_arg);         
    lv_lmeter_set_range(self->ref, min, max);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_lmeter_set_scale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"angle", "line_cnt", NULL};
    unsigned short int angle;
    unsigned char line_cnt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hb", kwlist , &angle, &line_cnt)) return NULL;

    if (lock) lock(lock_arg);         
    lv_lmeter_set_scale(self->ref, angle, line_cnt);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_lmeter_get_style_bg(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    return Style_From_lv_style(lv_lmeter_get_style_bg(self->ref));
}


static PyMethodDef pylv_lmeter_methods[] = {
    {"set_value", (PyCFunction) pylv_lmeter_set_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_range", (PyCFunction) pylv_lmeter_set_range, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_scale", (PyCFunction) pylv_lmeter_set_scale, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style_bg", (PyCFunction) pylv_lmeter_get_style_bg, METH_VARARGS | METH_KEYWORDS, NULL},
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_btnm_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

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

    if (lock) lock(lock_arg);         
    lv_btnm_set_toggle(self->ref, en, id);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    if (lock) lock(lock_arg);         
    lv_btnm_set_style(self->ref, type, style->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_get_action(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
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
    {"get_action", (PyCFunction) pylv_btnm_get_action, METH_VARARGS | METH_KEYWORDS, NULL},
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_chart_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

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

    if (lock) lock(lock_arg);         
    lv_chart_set_div_line_count(self->ref, hdiv, vdiv);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_range(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"ymin", "ymax", NULL};
    short int ymin;
    short int ymax;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &ymin, &ymax)) return NULL;

    if (lock) lock(lock_arg);         
    lv_chart_set_range(self->ref, ymin, ymax);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;

    if (lock) lock(lock_arg);         
    lv_chart_set_type(self->ref, type);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_point_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"point_cnt", NULL};
    unsigned short int point_cnt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &point_cnt)) return NULL;

    if (lock) lock(lock_arg);         
    lv_chart_set_point_count(self->ref, point_cnt);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_series_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"opa", NULL};
    unsigned char opa;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &opa)) return NULL;

    if (lock) lock(lock_arg);         
    lv_chart_set_series_opa(self->ref, opa);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_series_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"width", NULL};
    short int width;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &width)) return NULL;

    if (lock) lock(lock_arg);         
    lv_chart_set_series_width(self->ref, width);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_series_darking(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"dark_eff", NULL};
    unsigned char dark_eff;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &dark_eff)) return NULL;

    if (lock) lock(lock_arg);         
    lv_chart_set_series_darking(self->ref, dark_eff);
    if (unlock) unlock(unlock_arg);
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

    if (lock) lock(lock_arg);         
    int result = lv_chart_get_type(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_chart_get_series_opa(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    unsigned char result = lv_chart_get_series_opa(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_chart_get_series_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    short int result = lv_chart_get_series_width(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_chart_get_series_darking(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    unsigned char result = lv_chart_get_series_darking(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_chart_refresh(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    lv_chart_refresh(self->ref);
    if (unlock) unlock(unlock_arg);
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_cont_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

    return 0;
}


static PyObject*
pylv_cont_set_layout(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"layout", NULL};
    int layout;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &layout)) return NULL;

    if (lock) lock(lock_arg);         
    lv_cont_set_layout(self->ref, layout);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_cont_set_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"hor_en", "ver_en", NULL};
    int hor_en;
    int ver_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "pp", kwlist , &hor_en, &ver_en)) return NULL;

    if (lock) lock(lock_arg);         
    lv_cont_set_fit(self->ref, hor_en, ver_en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_cont_get_layout(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_cont_get_layout(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_cont_get_hor_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_cont_get_hor_fit(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_cont_get_ver_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_cont_get_ver_fit(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_cont_methods[] = {
    {"set_layout", (PyCFunction) pylv_cont_set_layout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_fit", (PyCFunction) pylv_cont_set_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_layout", (PyCFunction) pylv_cont_get_layout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_hor_fit", (PyCFunction) pylv_cont_get_hor_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_ver_fit", (PyCFunction) pylv_cont_get_ver_fit, METH_VARARGS | METH_KEYWORDS, NULL},
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_led_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

    return 0;
}


static PyObject*
pylv_led_set_bright(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"bright", NULL};
    unsigned char bright;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &bright)) return NULL;

    if (lock) lock(lock_arg);         
    lv_led_set_bright(self->ref, bright);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_led_on(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    lv_led_on(self->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_led_off(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    lv_led_off(self->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_led_toggle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    lv_led_toggle(self->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}


static PyMethodDef pylv_led_methods[] = {
    {"set_bright", (PyCFunction) pylv_led_set_bright, METH_VARARGS | METH_KEYWORDS, NULL},
    {"on", (PyCFunction) pylv_led_on, METH_VARARGS | METH_KEYWORDS, NULL},
    {"off", (PyCFunction) pylv_led_off, METH_VARARGS | METH_KEYWORDS, NULL},
    {"toggle", (PyCFunction) pylv_led_toggle, METH_VARARGS | METH_KEYWORDS, NULL},
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_kb_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

    return 0;
}


lv_res_t pylv_kb_ok_action_callback(lv_obj_t* obj) {
    pylv_Kb *pyobj;
    PyObject *handler;
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->ok_action;
        if (handler) PyObject_CallFunctionObjArgs(handler, NULL);
        if (PyErr_Occurred()) PyErr_Print();
    }
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
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->hide_action;
        if (handler) PyObject_CallFunctionObjArgs(handler, NULL);
        if (PyErr_Occurred()) PyErr_Print();
    }
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

    if (lock) lock(lock_arg);         
    lv_kb_set_ta(self->ref, ta->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_kb_set_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"mode", NULL};
    int mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &mode)) return NULL;

    if (lock) lock(lock_arg);         
    lv_kb_set_mode(self->ref, mode);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_kb_set_cursor_manage(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    if (lock) lock(lock_arg);         
    lv_kb_set_cursor_manage(self->ref, en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_kb_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    if (lock) lock(lock_arg);         
    lv_kb_set_style(self->ref, type, style->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_kb_get_ta(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

        
    if (lock) lock(lock_arg);
    lv_obj_t *result = lv_kb_get_ta(self->ref);
    PyObject *retobj = pyobj_from_lv(result, &pylv_obj_Type);
    if (unlock) unlock(unlock_arg);
    
    return retobj;
}

static PyObject*
pylv_kb_get_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_kb_get_mode(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_kb_get_cursor_manage(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_kb_get_cursor_manage(self->ref);
    if (unlock) unlock(unlock_arg);
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_img_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

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

    if (lock) lock(lock_arg);         
    lv_img_set_file(self->ref, fn);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_img_set_auto_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"autosize_en", NULL};
    int autosize_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &autosize_en)) return NULL;

    if (lock) lock(lock_arg);         
    lv_img_set_auto_size(self->ref, autosize_en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_img_set_upscale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"upcale", NULL};
    int upcale;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &upcale)) return NULL;

    if (lock) lock(lock_arg);         
    lv_img_set_upscale(self->ref, upcale);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_img_get_src_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_img_get_auto_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_img_get_auto_size(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_img_get_upscale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_img_get_upscale(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}


static PyMethodDef pylv_img_methods[] = {
    {"set_src", (PyCFunction) pylv_img_set_src, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_file", (PyCFunction) pylv_img_set_file, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_auto_size", (PyCFunction) pylv_img_set_auto_size, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_upscale", (PyCFunction) pylv_img_set_upscale, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_src_type", (PyCFunction) pylv_img_get_src_type, METH_VARARGS | METH_KEYWORDS, NULL},
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_bar_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

    return 0;
}


static PyObject*
pylv_bar_set_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"value", NULL};
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &value)) return NULL;

    if (lock) lock(lock_arg);         
    lv_bar_set_value(self->ref, value);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_bar_set_value_anim(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"value", "anim_time", NULL};
    short int value;
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hH", kwlist , &value, &anim_time)) return NULL;

    if (lock) lock(lock_arg);         
    lv_bar_set_value_anim(self->ref, value, anim_time);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_bar_set_range(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"min", "max", NULL};
    short int min;
    short int max;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &min, &max)) return NULL;

    if (lock) lock(lock_arg);         
    lv_bar_set_range(self->ref, min, max);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_bar_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    if (lock) lock(lock_arg);         
    lv_bar_set_style(self->ref, type, style->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_line_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

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

    if (lock) lock(lock_arg);         
    lv_line_set_auto_size(self->ref, autosize_en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_line_set_y_invert(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"yinv_en", NULL};
    int yinv_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &yinv_en)) return NULL;

    if (lock) lock(lock_arg);         
    lv_line_set_y_invert(self->ref, yinv_en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_line_set_upscale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"upcale", NULL};
    int upcale;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &upcale)) return NULL;

    if (lock) lock(lock_arg);         
    lv_line_set_upscale(self->ref, upcale);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_line_get_auto_size(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_line_get_auto_size(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_line_get_y_inv(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_line_get_y_inv(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_line_get_upscale(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_line_get_upscale(self->ref);
    if (unlock) unlock(unlock_arg);
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_tabview_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

    return 0;
}


static PyObject*
pylv_tabview_add_tab(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"name", NULL};
    char * name;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &name)) return NULL;

        
    if (lock) lock(lock_arg);
    lv_obj_t *result = lv_tabview_add_tab(self->ref, name);
    PyObject *retobj = pyobj_from_lv(result, &pylv_obj_Type);
    if (unlock) unlock(unlock_arg);
    
    return retobj;
}

static PyObject*
pylv_tabview_set_tab_act(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", "anim_en", NULL};
    unsigned short int id;
    int anim_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hp", kwlist , &id, &anim_en)) return NULL;

    if (lock) lock(lock_arg);         
    lv_tabview_set_tab_act(self->ref, id, anim_en);
    if (unlock) unlock(unlock_arg);
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

    if (lock) lock(lock_arg);         
    lv_tabview_set_sliding(self->ref, en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_tabview_set_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"anim_time", NULL};
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_time)) return NULL;

    if (lock) lock(lock_arg);         
    lv_tabview_set_anim_time(self->ref, anim_time);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_tabview_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    if (lock) lock(lock_arg);         
    lv_tabview_set_style(self->ref, type, style->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_tabview_get_tab(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", NULL};
    unsigned short int id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &id)) return NULL;

        
    if (lock) lock(lock_arg);
    lv_obj_t *result = lv_tabview_get_tab(self->ref, id);
    PyObject *retobj = pyobj_from_lv(result, &pylv_obj_Type);
    if (unlock) unlock(unlock_arg);
    
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

    if (lock) lock(lock_arg);         
    int result = lv_tabview_get_sliding(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_tabview_get_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_tabview_get_style(self->ref, type));
}


static PyMethodDef pylv_tabview_methods[] = {
    {"add_tab", (PyCFunction) pylv_tabview_add_tab, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_tab_act", (PyCFunction) pylv_tabview_set_tab_act, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_tab_load_action", (PyCFunction) pylv_tabview_set_tab_load_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_sliding", (PyCFunction) pylv_tabview_set_sliding, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_anim_time", (PyCFunction) pylv_tabview_set_anim_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_tabview_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_tab", (PyCFunction) pylv_tabview_get_tab, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_tab_load_action", (PyCFunction) pylv_tabview_get_tab_load_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_sliding", (PyCFunction) pylv_tabview_get_sliding, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_tabview_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_mbox_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

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

    if (lock) lock(lock_arg);         
    lv_mbox_set_text(self->ref, txt);
    if (unlock) unlock(unlock_arg);
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

    if (lock) lock(lock_arg);         
    lv_mbox_set_anim_time(self->ref, anim_time);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_mbox_start_auto_close(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"delay", NULL};
    unsigned short int delay;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &delay)) return NULL;

    if (lock) lock(lock_arg);         
    lv_mbox_start_auto_close(self->ref, delay);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_mbox_stop_auto_close(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    lv_mbox_stop_auto_close(self->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_mbox_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    if (lock) lock(lock_arg);         
    lv_mbox_set_style(self->ref, type, style->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_mbox_get_from_btn(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

        
    if (lock) lock(lock_arg);
    lv_obj_t *result = lv_mbox_get_from_btn(self->ref);
    PyObject *retobj = pyobj_from_lv(result, &pylv_obj_Type);
    if (unlock) unlock(unlock_arg);
    
    return retobj;
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
    {"get_from_btn", (PyCFunction) pylv_mbox_get_from_btn, METH_VARARGS | METH_KEYWORDS, NULL},
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_gauge_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

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

    if (lock) lock(lock_arg);         
    lv_gauge_set_value(self->ref, needle_id, value);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_gauge_set_critical_value(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"value", NULL};
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &value)) return NULL;

    if (lock) lock(lock_arg);         
    lv_gauge_set_critical_value(self->ref, value);
    if (unlock) unlock(unlock_arg);
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

    if (lock) lock(lock_arg);         
    lv_gauge_set_scale(self->ref, angle, line_cnt, label_cnt);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}


static PyMethodDef pylv_gauge_methods[] = {
    {"set_needle_count", (PyCFunction) pylv_gauge_set_needle_count, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_value", (PyCFunction) pylv_gauge_set_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_critical_value", (PyCFunction) pylv_gauge_set_critical_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_scale", (PyCFunction) pylv_gauge_set_scale, METH_VARARGS | METH_KEYWORDS, NULL},
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_page_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

    return 0;
}


lv_res_t pylv_page_rel_action_callback(lv_obj_t* obj) {
    pylv_Page *pyobj;
    PyObject *handler;
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->rel_action;
        if (handler) PyObject_CallFunctionObjArgs(handler, NULL);
        if (PyErr_Occurred()) PyErr_Print();
    }
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
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->pr_action;
        if (handler) PyObject_CallFunctionObjArgs(handler, NULL);
        if (PyErr_Occurred()) PyErr_Print();
    }
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
pylv_page_get_scrl(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

        
    if (lock) lock(lock_arg);
    lv_obj_t *result = lv_page_get_scrl(self->ref);
    PyObject *retobj = pyobj_from_lv(result, &pylv_obj_Type);
    if (unlock) unlock(unlock_arg);
    
    return retobj;
}

static PyObject*
pylv_page_set_sb_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"sb_mode", NULL};
    int sb_mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &sb_mode)) return NULL;

    if (lock) lock(lock_arg);         
    lv_page_set_sb_mode(self->ref, sb_mode);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scrl_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"hor_en", "ver_en", NULL};
    int hor_en;
    int ver_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "pp", kwlist , &hor_en, &ver_en)) return NULL;

    if (lock) lock(lock_arg);         
    lv_page_set_scrl_fit(self->ref, hor_en, ver_en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scrl_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"w", NULL};
    short int w;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &w)) return NULL;

    if (lock) lock(lock_arg);         
    lv_page_set_scrl_width(self->ref, w);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scrl_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"h", NULL};
    short int h;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &h)) return NULL;

    if (lock) lock(lock_arg);         
    lv_page_set_scrl_height(self->ref, h);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scrl_layout(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"layout", NULL};
    int layout;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &layout)) return NULL;

    if (lock) lock(lock_arg);         
    lv_page_set_scrl_layout(self->ref, layout);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    if (lock) lock(lock_arg);         
    lv_page_set_style(self->ref, type, style->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_get_sb_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_page_get_sb_mode(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_page_get_scrl_width(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    short int result = lv_page_get_scrl_width(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_page_get_scrl_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    short int result = lv_page_get_scrl_height(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_page_get_scrl_layout(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_page_get_scrl_layout(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_page_get_scrl_hor_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_page_get_scrl_hor_fit(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_page_get_scrl_fit_ver(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_page_get_scrl_fit_ver(self->ref);
    if (unlock) unlock(unlock_arg);
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

    if (lock) lock(lock_arg);         
    lv_page_glue_obj(self->ref, glue);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_focus(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"obj", "anim_time", NULL};
    pylv_Obj * obj;
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!H", kwlist , &pylv_obj_Type, &obj, &anim_time)) return NULL;

    if (lock) lock(lock_arg);         
    lv_page_focus(self->ref, obj->ref, anim_time);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}


static PyMethodDef pylv_page_methods[] = {
    {"get_scrl", (PyCFunction) pylv_page_get_scrl, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_rel_action", (PyCFunction) pylv_page_set_rel_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_pr_action", (PyCFunction) pylv_page_set_pr_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_sb_mode", (PyCFunction) pylv_page_set_sb_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_scrl_fit", (PyCFunction) pylv_page_set_scrl_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_scrl_width", (PyCFunction) pylv_page_set_scrl_width, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_scrl_height", (PyCFunction) pylv_page_set_scrl_height, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_scrl_layout", (PyCFunction) pylv_page_set_scrl_layout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_page_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_sb_mode", (PyCFunction) pylv_page_get_sb_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_scrl_width", (PyCFunction) pylv_page_get_scrl_width, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_scrl_height", (PyCFunction) pylv_page_get_scrl_height, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_scrl_layout", (PyCFunction) pylv_page_get_scrl_layout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_scrl_hor_fit", (PyCFunction) pylv_page_get_scrl_hor_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_scrl_fit_ver", (PyCFunction) pylv_page_get_scrl_fit_ver, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_page_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"glue_obj", (PyCFunction) pylv_page_glue_obj, METH_VARARGS | METH_KEYWORDS, NULL},
    {"focus", (PyCFunction) pylv_page_focus, METH_VARARGS | METH_KEYWORDS, NULL},
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_ta_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

    return 0;
}


static PyObject*
pylv_ta_add_char(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"c", NULL};
    char c;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "c", kwlist , &c)) return NULL;

    if (lock) lock(lock_arg);         
    lv_ta_add_char(self->ref, c);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_add_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"txt", NULL};
    char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    if (lock) lock(lock_arg);         
    lv_ta_add_text(self->ref, txt);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_del_char(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    lv_ta_del_char(self->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_text(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"txt", NULL};
    char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;

    if (lock) lock(lock_arg);         
    lv_ta_set_text(self->ref, txt);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_cursor_pos(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"pos", NULL};
    short int pos;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &pos)) return NULL;

    if (lock) lock(lock_arg);         
    lv_ta_set_cursor_pos(self->ref, pos);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_cursor_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"cur_type", NULL};
    int cur_type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &cur_type)) return NULL;

    if (lock) lock(lock_arg);         
    lv_ta_set_cursor_type(self->ref, cur_type);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_pwd_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"pwd_en", NULL};
    int pwd_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &pwd_en)) return NULL;

    if (lock) lock(lock_arg);         
    lv_ta_set_pwd_mode(self->ref, pwd_en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_one_line(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;

    if (lock) lock(lock_arg);         
    lv_ta_set_one_line(self->ref, en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    if (lock) lock(lock_arg);         
    lv_ta_set_style(self->ref, type, style->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_get_label(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

        
    if (lock) lock(lock_arg);
    lv_obj_t *result = lv_ta_get_label(self->ref);
    PyObject *retobj = pyobj_from_lv(result, &pylv_obj_Type);
    if (unlock) unlock(unlock_arg);
    
    return retobj;
}

static PyObject*
pylv_ta_get_cursor_type(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_ta_get_cursor_type(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_ta_get_pwd_mode(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_ta_get_pwd_mode(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_ta_get_one_line(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_ta_get_one_line(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
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

    if (lock) lock(lock_arg);         
    lv_ta_cursor_right(self->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_cursor_left(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    lv_ta_cursor_left(self->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_cursor_down(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    lv_ta_cursor_down(self->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_cursor_up(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    lv_ta_cursor_up(self->ref);
    if (unlock) unlock(unlock_arg);
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
    {"set_style", (PyCFunction) pylv_ta_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_label", (PyCFunction) pylv_ta_get_label, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_cursor_type", (PyCFunction) pylv_ta_get_cursor_type, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_pwd_mode", (PyCFunction) pylv_ta_get_pwd_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_one_line", (PyCFunction) pylv_ta_get_one_line, METH_VARARGS | METH_KEYWORDS, NULL},
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_btn_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

    return 0;
}


static PyObject*
pylv_btn_set_toggle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"tgl", NULL};
    int tgl;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &tgl)) return NULL;

    if (lock) lock(lock_arg);         
    lv_btn_set_toggle(self->ref, tgl);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_btn_set_state(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"state", NULL};
    int state;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &state)) return NULL;

    if (lock) lock(lock_arg);         
    lv_btn_set_state(self->ref, state);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_btn_toggle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    lv_btn_toggle(self->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_btn_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    if (lock) lock(lock_arg);         
    lv_btn_set_style(self->ref, type, style->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_btn_get_state(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_btn_get_state(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_btn_get_toggle(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_btn_get_toggle(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
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
    {"set_style", (PyCFunction) pylv_btn_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_state", (PyCFunction) pylv_btn_get_state, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_toggle", (PyCFunction) pylv_btn_get_toggle, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_action", (PyCFunction) pylv_btn_get_action, METH_VARARGS | METH_KEYWORDS, NULL},
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_ddlist_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

    return 0;
}


lv_res_t pylv_ddlist_action_callback(lv_obj_t* obj) {
    pylv_Ddlist *pyobj;
    PyObject *handler;
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->action;
        if (handler) PyObject_CallFunctionObjArgs(handler, NULL);
        if (PyErr_Occurred()) PyErr_Print();
    }
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

    if (lock) lock(lock_arg);         
    lv_ddlist_set_options(self->ref, options);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_selected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"sel_opt", NULL};
    unsigned short int sel_opt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &sel_opt)) return NULL;

    if (lock) lock(lock_arg);         
    lv_ddlist_set_selected(self->ref, sel_opt);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_fix_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"h", NULL};
    short int h;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &h)) return NULL;

    if (lock) lock(lock_arg);         
    lv_ddlist_set_fix_height(self->ref, h);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_hor_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"fit_en", NULL};
    int fit_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &fit_en)) return NULL;

    if (lock) lock(lock_arg);         
    lv_ddlist_set_hor_fit(self->ref, fit_en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"anim_time", NULL};
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_time)) return NULL;

    if (lock) lock(lock_arg);         
    lv_ddlist_set_anim_time(self->ref, anim_time);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    if (lock) lock(lock_arg);         
    lv_ddlist_set_style(self->ref, type, style->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_get_selected_str(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"buf", NULL};
    char * buf;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &buf)) return NULL;

    if (lock) lock(lock_arg);         
    lv_ddlist_get_selected_str(self->ref, buf);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_get_fix_height(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    short int result = lv_ddlist_get_fix_height(self->ref);
    if (unlock) unlock(unlock_arg);
    return Py_BuildValue("h", result);
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

    if (lock) lock(lock_arg);         
    lv_ddlist_open(self->ref, anim);
    if (unlock) unlock(unlock_arg);
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
    {"get_selected_str", (PyCFunction) pylv_ddlist_get_selected_str, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_action", (PyCFunction) pylv_ddlist_get_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_fix_height", (PyCFunction) pylv_ddlist_get_fix_height, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_ddlist_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"open", (PyCFunction) pylv_ddlist_open, METH_VARARGS | METH_KEYWORDS, NULL},
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_list_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

    return 0;
}


static PyObject*
pylv_list_set_anim_time(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"anim_time", NULL};
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_time)) return NULL;

    if (lock) lock(lock_arg);         
    lv_list_set_anim_time(self->ref, anim_time);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_list_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    if (lock) lock(lock_arg);         
    lv_list_set_style(self->ref, type, style->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_list_get_btn_label(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

        
    if (lock) lock(lock_arg);
    lv_obj_t *result = lv_list_get_btn_label(self->ref);
    PyObject *retobj = pyobj_from_lv(result, &pylv_obj_Type);
    if (unlock) unlock(unlock_arg);
    
    return retobj;
}

static PyObject*
pylv_list_get_btn_img(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

        
    if (lock) lock(lock_arg);
    lv_obj_t *result = lv_list_get_btn_img(self->ref);
    PyObject *retobj = pyobj_from_lv(result, &pylv_obj_Type);
    if (unlock) unlock(unlock_arg);
    
    return retobj;
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

    if (lock) lock(lock_arg);         
    lv_list_up(self->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_list_down(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    lv_list_down(self->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}


static PyMethodDef pylv_list_methods[] = {
    {"add", (PyCFunction) pylv_list_add, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_anim_time", (PyCFunction) pylv_list_set_anim_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_list_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_btn_label", (PyCFunction) pylv_list_get_btn_label, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_btn_img", (PyCFunction) pylv_list_get_btn_img, METH_VARARGS | METH_KEYWORDS, NULL},
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_slider_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

    return 0;
}


lv_res_t pylv_slider_action_callback(lv_obj_t* obj) {
    pylv_Slider *pyobj;
    PyObject *handler;
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->action;
        if (handler) PyObject_CallFunctionObjArgs(handler, NULL);
        if (PyErr_Occurred()) PyErr_Print();
    }
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

    if (lock) lock(lock_arg);         
    lv_slider_set_knob_in(self->ref, in);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_slider_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    if (lock) lock(lock_arg);         
    lv_slider_set_style(self->ref, type, style->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_slider_is_dragged(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_slider_is_dragged(self->ref);
    if (unlock) unlock(unlock_arg);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_slider_get_knob_in(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_slider_get_knob_in(self->ref);
    if (unlock) unlock(unlock_arg);
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_sw_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

    return 0;
}


static PyObject*
pylv_sw_on(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    lv_sw_on(self->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_sw_off(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    lv_sw_off(self->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_sw_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    if (lock) lock(lock_arg);         
    lv_sw_set_style(self->ref, type, style->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_sw_get_state(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_sw_get_state(self->ref);
    if (unlock) unlock(unlock_arg);
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_cb_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

    return 0;
}


lv_res_t pylv_cb_action_callback(lv_obj_t* obj) {
    pylv_Cb *pyobj;
    PyObject *handler;
    pyobj = lv_obj_get_free_ptr(obj);
    if (pyobj) {
        handler = pyobj->action;
        if (handler) PyObject_CallFunctionObjArgs(handler, NULL);
        if (PyErr_Occurred()) PyErr_Print();
    }
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

    if (lock) lock(lock_arg);         
    lv_cb_set_text(self->ref, txt);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_cb_set_checked(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"checked", NULL};
    int checked;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &checked)) return NULL;

    if (lock) lock(lock_arg);         
    lv_cb_set_checked(self->ref, checked);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_cb_set_inactive(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    lv_cb_set_inactive(self->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_cb_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    if (lock) lock(lock_arg);         
    lv_cb_set_style(self->ref, type, style->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_cb_is_checked(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_cb_is_checked(self->ref);
    if (unlock) unlock(unlock_arg);
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
    
    if (lock) lock(lock_arg);
    self->ref = lv_roller_create(parent ? parent->ref : NULL, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);
    if (unlock) unlock(unlock_arg);

    return 0;
}


static PyObject*
pylv_roller_set_selected(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"sel_opt", "anim_en", NULL};
    unsigned short int sel_opt;
    int anim_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hp", kwlist , &sel_opt, &anim_en)) return NULL;

    if (lock) lock(lock_arg);         
    lv_roller_set_selected(self->ref, sel_opt, anim_en);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_roller_set_visible_row_count(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"row_cnt", NULL};
    unsigned char row_cnt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &row_cnt)) return NULL;

    if (lock) lock(lock_arg);         
    lv_roller_set_visible_row_count(self->ref, row_cnt);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_roller_set_style(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;

    if (lock) lock(lock_arg);         
    lv_roller_set_style(self->ref, type, style->ref);
    if (unlock) unlock(unlock_arg);
    Py_RETURN_NONE;
}

static PyObject*
pylv_roller_get_hor_fit(pylv_Obj *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    if (lock) lock(lock_arg);         
    int result = lv_roller_get_hor_fit(self->ref);
    if (unlock) unlock(unlock_arg);
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
    if (lock) lock(lock_arg);
    scr = lv_scr_act();
    if (unlock) unlock(unlock_arg);
    return pyobj_from_lv(scr, &pylv_obj_Type);
}

static PyObject *
pylv_scr_load(PyObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"scr", NULL};
    pylv_Obj *scr;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist, &pylv_obj_Type, &scr)) return NULL;
    if (lock) lock(lock_arg);
    lv_scr_load(scr->ref);
    if (unlock) unlock(unlock_arg);
    
    Py_RETURN_NONE;
}


static PyObject *
poll(PyObject *self, PyObject *args) {
    if (lock) lock(lock_arg);
    lv_tick_inc(1);
    lv_task_handler();
    if (unlock) unlock(unlock_arg);
    
    Py_RETURN_NONE;
}

static PyObject *
report_style_mod(PyObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"style", NULL};
    Style_Object *style = NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!", kwlist, &Style_Type, &style)) {
        return NULL;
    }
    
    if (lock) unlock(lock_arg);
    lv_obj_report_style_mod(style ? style->ref: NULL);
    if (unlock) unlock(unlock_arg);
    
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


    
    PyModule_AddObject(module, "DESIGN_DRAW_MAIN", PyLong_FromLong(0));
    PyModule_AddObject(module, "DESIGN_DRAW_POST", PyLong_FromLong(1));
    PyModule_AddObject(module, "DESIGN_COVER_CHK", PyLong_FromLong(2));
    PyModule_AddObject(module, "RES_INV", PyLong_FromLong(0));
    PyModule_AddObject(module, "RES_OK", PyLong_FromLong(1));
    PyModule_AddObject(module, "SIGNAL_CLEANUP", PyLong_FromLong(0));
    PyModule_AddObject(module, "SIGNAL_CHILD_CHG", PyLong_FromLong(1));
    PyModule_AddObject(module, "SIGNAL_CORD_CHG", PyLong_FromLong(2));
    PyModule_AddObject(module, "SIGNAL_STYLE_CHG", PyLong_FromLong(3));
    PyModule_AddObject(module, "SIGNAL_REFR_EXT_SIZE", PyLong_FromLong(4));
    PyModule_AddObject(module, "SIGNAL_GET_TYPE", PyLong_FromLong(5));
    PyModule_AddObject(module, "SIGNAL_PRESSED", PyLong_FromLong(6));
    PyModule_AddObject(module, "SIGNAL_PRESSING", PyLong_FromLong(7));
    PyModule_AddObject(module, "SIGNAL_PRESS_LOST", PyLong_FromLong(8));
    PyModule_AddObject(module, "SIGNAL_RELEASED", PyLong_FromLong(9));
    PyModule_AddObject(module, "SIGNAL_LONG_PRESS", PyLong_FromLong(10));
    PyModule_AddObject(module, "SIGNAL_LONG_PRESS_REP", PyLong_FromLong(11));
    PyModule_AddObject(module, "SIGNAL_DRAG_BEGIN", PyLong_FromLong(12));
    PyModule_AddObject(module, "SIGNAL_DRAG_END", PyLong_FromLong(13));
    PyModule_AddObject(module, "SIGNAL_FOCUS", PyLong_FromLong(14));
    PyModule_AddObject(module, "SIGNAL_DEFOCUS", PyLong_FromLong(15));
    PyModule_AddObject(module, "SIGNAL_CONTROLL", PyLong_FromLong(16));
    PyModule_AddObject(module, "PROTECT_NONE", PyLong_FromLong(0));
    PyModule_AddObject(module, "PROTECT_CHILD_CHG", PyLong_FromLong(1));
    PyModule_AddObject(module, "PROTECT_PARENT", PyLong_FromLong(2));
    PyModule_AddObject(module, "PROTECT_POS", PyLong_FromLong(4));
    PyModule_AddObject(module, "PROTECT_FOLLOW", PyLong_FromLong(8));
    PyModule_AddObject(module, "PROTECT_PRESS_LOST", PyLong_FromLong(16));
    PyModule_AddObject(module, "ALIGN_CENTER", PyLong_FromLong(0));
    PyModule_AddObject(module, "ALIGN_IN_TOP_LEFT", PyLong_FromLong(1));
    PyModule_AddObject(module, "ALIGN_IN_TOP_MID", PyLong_FromLong(2));
    PyModule_AddObject(module, "ALIGN_IN_TOP_RIGHT", PyLong_FromLong(3));
    PyModule_AddObject(module, "ALIGN_IN_BOTTOM_LEFT", PyLong_FromLong(4));
    PyModule_AddObject(module, "ALIGN_IN_BOTTOM_MID", PyLong_FromLong(5));
    PyModule_AddObject(module, "ALIGN_IN_BOTTOM_RIGHT", PyLong_FromLong(6));
    PyModule_AddObject(module, "ALIGN_IN_LEFT_MID", PyLong_FromLong(7));
    PyModule_AddObject(module, "ALIGN_IN_RIGHT_MID", PyLong_FromLong(8));
    PyModule_AddObject(module, "ALIGN_OUT_TOP_LEFT", PyLong_FromLong(9));
    PyModule_AddObject(module, "ALIGN_OUT_TOP_MID", PyLong_FromLong(10));
    PyModule_AddObject(module, "ALIGN_OUT_TOP_RIGHT", PyLong_FromLong(11));
    PyModule_AddObject(module, "ALIGN_OUT_BOTTOM_LEFT", PyLong_FromLong(12));
    PyModule_AddObject(module, "ALIGN_OUT_BOTTOM_MID", PyLong_FromLong(13));
    PyModule_AddObject(module, "ALIGN_OUT_BOTTOM_RIGHT", PyLong_FromLong(14));
    PyModule_AddObject(module, "ALIGN_OUT_LEFT_TOP", PyLong_FromLong(15));
    PyModule_AddObject(module, "ALIGN_OUT_LEFT_MID", PyLong_FromLong(16));
    PyModule_AddObject(module, "ALIGN_OUT_LEFT_BOTTOM", PyLong_FromLong(17));
    PyModule_AddObject(module, "ALIGN_OUT_RIGHT_TOP", PyLong_FromLong(18));
    PyModule_AddObject(module, "ALIGN_OUT_RIGHT_MID", PyLong_FromLong(19));
    PyModule_AddObject(module, "ALIGN_OUT_RIGHT_BOTTOM", PyLong_FromLong(20));
    PyModule_AddObject(module, "ANIM_NONE", PyLong_FromLong(0));
    PyModule_AddObject(module, "ANIM_FLOAT_TOP", PyLong_FromLong(1));
    PyModule_AddObject(module, "ANIM_FLOAT_LEFT", PyLong_FromLong(2));
    PyModule_AddObject(module, "ANIM_FLOAT_BOTTOM", PyLong_FromLong(3));
    PyModule_AddObject(module, "ANIM_FLOAT_RIGHT", PyLong_FromLong(4));
    PyModule_AddObject(module, "ANIM_GROW_H", PyLong_FromLong(5));
    PyModule_AddObject(module, "ANIM_GROW_V", PyLong_FromLong(6));
    PyModule_AddObject(module, "SLIDER_STYLE_BG", PyLong_FromLong(0));
    PyModule_AddObject(module, "SLIDER_STYLE_INDIC", PyLong_FromLong(1));
    PyModule_AddObject(module, "SLIDER_STYLE_KNOB", PyLong_FromLong(2));
    PyModule_AddObject(module, "LIST_STYLE_BG", PyLong_FromLong(0));
    PyModule_AddObject(module, "LIST_STYLE_SCRL", PyLong_FromLong(1));
    PyModule_AddObject(module, "LIST_STYLE_SB", PyLong_FromLong(2));
    PyModule_AddObject(module, "LIST_STYLE_BTN_REL", PyLong_FromLong(3));
    PyModule_AddObject(module, "LIST_STYLE_BTN_PR", PyLong_FromLong(4));
    PyModule_AddObject(module, "LIST_STYLE_BTN_TGL_REL", PyLong_FromLong(5));
    PyModule_AddObject(module, "LIST_STYLE_BTN_TGL_PR", PyLong_FromLong(6));
    PyModule_AddObject(module, "LIST_STYLE_BTN_INA", PyLong_FromLong(7));
    PyModule_AddObject(module, "KB_MODE_TEXT", PyLong_FromLong(0));
    PyModule_AddObject(module, "KB_MODE_NUM", PyLong_FromLong(1));
    PyModule_AddObject(module, "KB_STYLE_BG", PyLong_FromLong(0));
    PyModule_AddObject(module, "KB_STYLE_BTN_REL", PyLong_FromLong(1));
    PyModule_AddObject(module, "KB_STYLE_BTN_PR", PyLong_FromLong(2));
    PyModule_AddObject(module, "KB_STYLE_BTN_TGL_REL", PyLong_FromLong(3));
    PyModule_AddObject(module, "KB_STYLE_BTN_TGL_PR", PyLong_FromLong(4));
    PyModule_AddObject(module, "KB_STYLE_BTN_INA", PyLong_FromLong(5));
    PyModule_AddObject(module, "BAR_STYLE_BG", PyLong_FromLong(0));
    PyModule_AddObject(module, "BAR_STYLE_INDIC", PyLong_FromLong(1));
    PyModule_AddObject(module, "TABVIEW_STYLE_BG", PyLong_FromLong(0));
    PyModule_AddObject(module, "TABVIEW_STYLE_INDIC", PyLong_FromLong(1));
    PyModule_AddObject(module, "TABVIEW_STYLE_BTN_BG", PyLong_FromLong(2));
    PyModule_AddObject(module, "TABVIEW_STYLE_BTN_REL", PyLong_FromLong(3));
    PyModule_AddObject(module, "TABVIEW_STYLE_BTN_PR", PyLong_FromLong(4));
    PyModule_AddObject(module, "TABVIEW_STYLE_BTN_TGL_REL", PyLong_FromLong(5));
    PyModule_AddObject(module, "TABVIEW_STYLE_BTN_TGL_PR", PyLong_FromLong(6));
    PyModule_AddObject(module, "SW_STYLE_BG", PyLong_FromLong(0));
    PyModule_AddObject(module, "SW_STYLE_INDIC", PyLong_FromLong(1));
    PyModule_AddObject(module, "SW_STYLE_KNOB_OFF", PyLong_FromLong(2));
    PyModule_AddObject(module, "SW_STYLE_KNOB_ON", PyLong_FromLong(3));
    PyModule_AddObject(module, "MBOX_STYLE_BG", PyLong_FromLong(0));
    PyModule_AddObject(module, "MBOX_STYLE_BTN_BG", PyLong_FromLong(1));
    PyModule_AddObject(module, "MBOX_STYLE_BTN_REL", PyLong_FromLong(2));
    PyModule_AddObject(module, "MBOX_STYLE_BTN_PR", PyLong_FromLong(3));
    PyModule_AddObject(module, "MBOX_STYLE_BTN_TGL_REL", PyLong_FromLong(4));
    PyModule_AddObject(module, "MBOX_STYLE_BTN_TGL_PR", PyLong_FromLong(5));
    PyModule_AddObject(module, "MBOX_STYLE_BTN_INA", PyLong_FromLong(6));
    PyModule_AddObject(module, "LABEL_LONG_EXPAND", PyLong_FromLong(0));
    PyModule_AddObject(module, "LABEL_LONG_BREAK", PyLong_FromLong(1));
    PyModule_AddObject(module, "LABEL_LONG_SCROLL", PyLong_FromLong(2));
    PyModule_AddObject(module, "LABEL_LONG_DOT", PyLong_FromLong(3));
    PyModule_AddObject(module, "LABEL_LONG_ROLL", PyLong_FromLong(4));
    PyModule_AddObject(module, "LABEL_ALIGN_LEFT", PyLong_FromLong(0));
    PyModule_AddObject(module, "LABEL_ALIGN_CENTER", PyLong_FromLong(1));
    PyModule_AddObject(module, "CB_STYLE_BG", PyLong_FromLong(0));
    PyModule_AddObject(module, "CB_STYLE_BOX_REL", PyLong_FromLong(1));
    PyModule_AddObject(module, "CB_STYLE_BOX_PR", PyLong_FromLong(2));
    PyModule_AddObject(module, "CB_STYLE_BOX_TGL_REL", PyLong_FromLong(3));
    PyModule_AddObject(module, "CB_STYLE_BOX_TGL_PR", PyLong_FromLong(4));
    PyModule_AddObject(module, "CB_STYLE_BOX_INA", PyLong_FromLong(5));
    PyModule_AddObject(module, "SB_MODE_OFF", PyLong_FromLong(0));
    PyModule_AddObject(module, "SB_MODE_ON", PyLong_FromLong(1));
    PyModule_AddObject(module, "SB_MODE_DRAG", PyLong_FromLong(2));
    PyModule_AddObject(module, "SB_MODE_AUTO", PyLong_FromLong(3));
    PyModule_AddObject(module, "PAGE_STYLE_BG", PyLong_FromLong(0));
    PyModule_AddObject(module, "PAGE_STYLE_SCRL", PyLong_FromLong(1));
    PyModule_AddObject(module, "PAGE_STYLE_SB", PyLong_FromLong(2));
    PyModule_AddObject(module, "WIN_STYLE_BG", PyLong_FromLong(0));
    PyModule_AddObject(module, "WIN_STYLE_CONTENT_BG", PyLong_FromLong(1));
    PyModule_AddObject(module, "WIN_STYLE_CONTENT_SCRL", PyLong_FromLong(2));
    PyModule_AddObject(module, "WIN_STYLE_SB", PyLong_FromLong(3));
    PyModule_AddObject(module, "WIN_STYLE_HEADER", PyLong_FromLong(4));
    PyModule_AddObject(module, "WIN_STYLE_BTN_REL", PyLong_FromLong(5));
    PyModule_AddObject(module, "WIN_STYLE_BTN_PR", PyLong_FromLong(6));
    PyModule_AddObject(module, "BTN_STATE_REL", PyLong_FromLong(0));
    PyModule_AddObject(module, "BTN_STATE_PR", PyLong_FromLong(1));
    PyModule_AddObject(module, "BTN_STATE_TGL_REL", PyLong_FromLong(2));
    PyModule_AddObject(module, "BTN_STATE_TGL_PR", PyLong_FromLong(3));
    PyModule_AddObject(module, "BTN_STATE_INA", PyLong_FromLong(4));
    PyModule_AddObject(module, "BTN_STATE_NUM", PyLong_FromLong(5));
    PyModule_AddObject(module, "BTN_ACTION_CLICK", PyLong_FromLong(0));
    PyModule_AddObject(module, "BTN_ACTION_PR", PyLong_FromLong(1));
    PyModule_AddObject(module, "BTN_ACTION_LONG_PR", PyLong_FromLong(2));
    PyModule_AddObject(module, "BTN_ACTION_LONG_PR_REPEAT", PyLong_FromLong(3));
    PyModule_AddObject(module, "BTN_ACTION_NUM", PyLong_FromLong(4));
    PyModule_AddObject(module, "BTN_STYLE_REL", PyLong_FromLong(0));
    PyModule_AddObject(module, "BTN_STYLE_PR", PyLong_FromLong(1));
    PyModule_AddObject(module, "BTN_STYLE_TGL_REL", PyLong_FromLong(2));
    PyModule_AddObject(module, "BTN_STYLE_TGL_PR", PyLong_FromLong(3));
    PyModule_AddObject(module, "BTN_STYLE_INA", PyLong_FromLong(4));
    PyModule_AddObject(module, "CURSOR_NONE", PyLong_FromLong(0));
    PyModule_AddObject(module, "CURSOR_LINE", PyLong_FromLong(1));
    PyModule_AddObject(module, "CURSOR_BLOCK", PyLong_FromLong(2));
    PyModule_AddObject(module, "CURSOR_OUTLINE", PyLong_FromLong(3));
    PyModule_AddObject(module, "CURSOR_UNDERLINE", PyLong_FromLong(4));
    PyModule_AddObject(module, "CURSOR_HIDDEN", PyLong_FromLong(16));
    PyModule_AddObject(module, "TA_STYLE_BG", PyLong_FromLong(0));
    PyModule_AddObject(module, "TA_STYLE_SB", PyLong_FromLong(1));
    PyModule_AddObject(module, "TA_STYLE_CURSOR", PyLong_FromLong(2));
    PyModule_AddObject(module, "ROLLER_STYLE_BG", PyLong_FromLong(0));
    PyModule_AddObject(module, "ROLLER_STYLE_SEL", PyLong_FromLong(1));
    PyModule_AddObject(module, "CHART_TYPE_LINE", PyLong_FromLong(1));
    PyModule_AddObject(module, "CHART_TYPE_COLUMN", PyLong_FromLong(2));
    PyModule_AddObject(module, "CHART_TYPE_POINT", PyLong_FromLong(4));
    PyModule_AddObject(module, "BTNM_STYLE_BG", PyLong_FromLong(0));
    PyModule_AddObject(module, "BTNM_STYLE_BTN_REL", PyLong_FromLong(1));
    PyModule_AddObject(module, "BTNM_STYLE_BTN_PR", PyLong_FromLong(2));
    PyModule_AddObject(module, "BTNM_STYLE_BTN_TGL_REL", PyLong_FromLong(3));
    PyModule_AddObject(module, "BTNM_STYLE_BTN_TGL_PR", PyLong_FromLong(4));
    PyModule_AddObject(module, "BTNM_STYLE_BTN_INA", PyLong_FromLong(5));
    PyModule_AddObject(module, "DDLIST_STYLE_BG", PyLong_FromLong(0));
    PyModule_AddObject(module, "DDLIST_STYLE_SEL", PyLong_FromLong(1));
    PyModule_AddObject(module, "DDLIST_STYLE_SB", PyLong_FromLong(2));
    PyModule_AddObject(module, "LAYOUT_OFF", PyLong_FromLong(0));
    PyModule_AddObject(module, "LAYOUT_CENTER", PyLong_FromLong(1));
    PyModule_AddObject(module, "LAYOUT_COL_L", PyLong_FromLong(2));
    PyModule_AddObject(module, "LAYOUT_COL_M", PyLong_FromLong(3));
    PyModule_AddObject(module, "LAYOUT_COL_R", PyLong_FromLong(4));
    PyModule_AddObject(module, "LAYOUT_ROW_T", PyLong_FromLong(5));
    PyModule_AddObject(module, "LAYOUT_ROW_M", PyLong_FromLong(6));
    PyModule_AddObject(module, "LAYOUT_ROW_B", PyLong_FromLong(7));
    PyModule_AddObject(module, "LAYOUT_PRETTY", PyLong_FromLong(8));
    PyModule_AddObject(module, "LAYOUT_GRID", PyLong_FromLong(9));

    
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
    PyModule_AddObject(module, "font_symbol_40", Font_From_lv_font(&lv_font_symbol_40));


    PyModule_AddObject(module, "SYMBOL_AUDIO", PyUnicode_FromString("\xEF\x80\x80"));
    PyModule_AddObject(module, "SYMBOL_VIDEO", PyUnicode_FromString("\xEF\x80\x81"));
    PyModule_AddObject(module, "SYMBOL_LIST", PyUnicode_FromString("\xEF\x80\x82"));
    PyModule_AddObject(module, "SYMBOL_OK", PyUnicode_FromString("\xEF\x80\x83"));
    PyModule_AddObject(module, "SYMBOL_CLOSE", PyUnicode_FromString("\xEF\x80\x84"));
    PyModule_AddObject(module, "SYMBOL_POWER", PyUnicode_FromString("\xEF\x80\x85"));
    PyModule_AddObject(module, "SYMBOL_SETTINGS", PyUnicode_FromString("\xEF\x80\x86"));
    PyModule_AddObject(module, "SYMBOL_TRASH", PyUnicode_FromString("\xEF\x80\x87"));
    PyModule_AddObject(module, "SYMBOL_HOME", PyUnicode_FromString("\xEF\x80\x88"));
    PyModule_AddObject(module, "SYMBOL_DOWNLOAD", PyUnicode_FromString("\xEF\x80\x89"));
    PyModule_AddObject(module, "SYMBOL_DRIVE", PyUnicode_FromString("\xEF\x80\x8A"));
    PyModule_AddObject(module, "SYMBOL_REFRESH", PyUnicode_FromString("\xEF\x80\x8B"));
    PyModule_AddObject(module, "SYMBOL_MUTE", PyUnicode_FromString("\xEF\x80\x8C"));
    PyModule_AddObject(module, "SYMBOL_VOLUME_MID", PyUnicode_FromString("\xEF\x80\x8D"));
    PyModule_AddObject(module, "SYMBOL_VOLUME_MAX", PyUnicode_FromString("\xEF\x80\x8E"));
    PyModule_AddObject(module, "SYMBOL_IMAGE", PyUnicode_FromString("\xEF\x80\x8F"));
    PyModule_AddObject(module, "SYMBOL_EDIT", PyUnicode_FromString("\xEF\x80\x90"));
    PyModule_AddObject(module, "SYMBOL_PREV", PyUnicode_FromString("\xEF\x80\x91"));
    PyModule_AddObject(module, "SYMBOL_PLAY", PyUnicode_FromString("\xEF\x80\x92"));
    PyModule_AddObject(module, "SYMBOL_PAUSE", PyUnicode_FromString("\xEF\x80\x93"));
    PyModule_AddObject(module, "SYMBOL_STOP", PyUnicode_FromString("\xEF\x80\x94"));
    PyModule_AddObject(module, "SYMBOL_NEXT", PyUnicode_FromString("\xEF\x80\x95"));
    PyModule_AddObject(module, "SYMBOL_EJECT", PyUnicode_FromString("\xEF\x80\x96"));
    PyModule_AddObject(module, "SYMBOL_LEFT", PyUnicode_FromString("\xEF\x80\x97"));
    PyModule_AddObject(module, "SYMBOL_RIGHT", PyUnicode_FromString("\xEF\x80\x98"));
    PyModule_AddObject(module, "SYMBOL_PLUS", PyUnicode_FromString("\xEF\x80\x99"));
    PyModule_AddObject(module, "SYMBOL_MINUS", PyUnicode_FromString("\xEF\x80\x9A"));
    PyModule_AddObject(module, "SYMBOL_WARNING", PyUnicode_FromString("\xEF\x80\x9B"));
    PyModule_AddObject(module, "SYMBOL_SHUFFLE", PyUnicode_FromString("\xEF\x80\x9C"));
    PyModule_AddObject(module, "SYMBOL_UP", PyUnicode_FromString("\xEF\x80\x9D"));
    PyModule_AddObject(module, "SYMBOL_DOWN", PyUnicode_FromString("\xEF\x80\x9E"));
    PyModule_AddObject(module, "SYMBOL_LOOP", PyUnicode_FromString("\xEF\x80\x9F"));
    PyModule_AddObject(module, "SYMBOL_DIRECTORY", PyUnicode_FromString("\xEF\x80\xA0"));
    PyModule_AddObject(module, "SYMBOL_UPLOAD", PyUnicode_FromString("\xEF\x80\xA1"));
    PyModule_AddObject(module, "SYMBOL_CALL", PyUnicode_FromString("\xEF\x80\xA2"));
    PyModule_AddObject(module, "SYMBOL_CUT", PyUnicode_FromString("\xEF\x80\xA3"));
    PyModule_AddObject(module, "SYMBOL_COPY", PyUnicode_FromString("\xEF\x80\xA4"));
    PyModule_AddObject(module, "SYMBOL_SAVE", PyUnicode_FromString("\xEF\x80\xA5"));
    PyModule_AddObject(module, "SYMBOL_CHARGE", PyUnicode_FromString("\xEF\x80\xA6"));
    PyModule_AddObject(module, "SYMBOL_BELL", PyUnicode_FromString("\xEF\x80\xA7"));
    PyModule_AddObject(module, "SYMBOL_KEYBOARD", PyUnicode_FromString("\xEF\x80\xA8"));
    PyModule_AddObject(module, "SYMBOL_GPS", PyUnicode_FromString("\xEF\x80\xA9"));
    PyModule_AddObject(module, "SYMBOL_FILE", PyUnicode_FromString("\xEF\x80\xAA"));
    PyModule_AddObject(module, "SYMBOL_WIFI", PyUnicode_FromString("\xEF\x80\xAB"));
    PyModule_AddObject(module, "SYMBOL_BATTERY_FULL", PyUnicode_FromString("\xEF\x80\xAC"));
    PyModule_AddObject(module, "SYMBOL_BATTERY_3", PyUnicode_FromString("\xEF\x80\xAD"));
    PyModule_AddObject(module, "SYMBOL_BATTERY_2", PyUnicode_FromString("\xEF\x80\xAE"));
    PyModule_AddObject(module, "SYMBOL_BATTERY_1", PyUnicode_FromString("\xEF\x80\xAF"));
    PyModule_AddObject(module, "SYMBOL_BATTERY_EMPTY", PyUnicode_FromString("\xEF\x80\xB0"));
    PyModule_AddObject(module, "SYMBOL_BLUETOOTH", PyUnicode_FromString("\xEF\x80\xB1"));


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

