#include "Python.h"
#include "structmember.h"
#include "lvgl/lvgl.h"

#if LV_COLOR_DEPTH != 16
#error Only 16 bits color depth is currently supported
#endif

<<OBJECT_STRUCTS>>

/****************************************************************
 * Forward declaration of type objects                          *
 ****************************************************************/

<<<
static PyTypeObject py{obj}_Type;
>>>

/****************************************************************
 * Style class                                                  *  
 ****************************************************************/

typedef struct {
    PyObject_HEAD
    lv_style_t *ref;
} Style_Object;


static PyObject *Style_data(Style_Object *self, PyObject *args) {

    return PyMemoryView_FromMemory((void*)(self->ref), sizeof(*self->ref), PyBUF_WRITE);
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


static PyGetSetDef Style_getsetters[] = {
<<STYLE_GETSET>>
    {NULL},
};


static PyMethodDef Style_methods[] = {
    {"data", (PyCFunction) Style_data, METH_NOARGS, ""},
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
    // Do not define tp_new to prevent creation of new instances from within Python
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
            // If PyList_Append fails, how to clean up?
            return NULL;
        }
    }
    
    return ret;
}



/****************************************************************
 * Methods and object definitions                               *
 ****************************************************************/

<<<
static void
py{obj}_dealloc({pylv_Obj} *self) 
{{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}}

static int
py{obj}_init({pylv_Obj} *self, PyObject *args, PyObject *kwds) 
{{
    static char *kwlist[] = {{"parent", "copy", NULL}};
    pylv_Obj *parent;
    {pylv_Obj} *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &py{obj}_Type, &copy)) {{
        return -1;
    }}   
    self->ref = {obj}_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}}
>>>

<<OBJECT_DEFINITIONS>>




/****************************************************************
 * Miscellaneous functions                                      *
 ****************************************************************/

static PyObject *
pylv_scr_act(PyObject *self, PyObject *args) {
    return lv_obj_get_free_ptr(lv_scr_act());
}

static PyObject *
poll(PyObject *self, PyObject *args) {
    lv_tick_inc(1);
    lv_task_handler();
    Py_RETURN_NONE;
}

static PyObject *
report_style_mod(PyObject *self, PyObject *args, PyObject *kwds) {
    static char *kwlist[] = {"style", NULL};
    Style_Object *style = NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O!", kwlist, &Style_Type, &style)) {
        return NULL;
    }
    
    lv_obj_report_style_mod(style ? style->ref: NULL);
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

static lv_disp_drv_t driver = {0};


/****************************************************************
 *  Module global stuff                                         *
 ****************************************************************/


static PyMethodDef lvglMethods[] = {
    {"scr_act",  pylv_scr_act, METH_NOARGS, NULL},
    {"poll", poll, METH_NOARGS, NULL},
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




PyMODINIT_FUNC
PyInit_lvgl(void) {

    PyObject *module = NULL;
    
    module = PyModule_Create(&lvglmodule);
    if (!module) goto error;
    
    if (PyType_Ready(&Style_Type) < 0) return NULL;
    
<<<
    py{obj}_Type.tp_base = {base};
    if (PyType_Ready(&py{obj}_Type) < 0) return NULL;
>>>

<<<
    Py_INCREF(&py{obj}_Type);
    PyModule_AddObject(module, "{typename}", (PyObject *) &py{obj}_Type); 
>>>

    
<<ENUM_ASSIGNMENTS>>
    
<<STYLE_ASSIGNMENTS>>

    PyModule_AddObject(module, "framebuffer", PyMemoryView_FromMemory(framebuffer, LV_HOR_RES * LV_VER_RES * 2, PyBUF_READ));
    PyModule_AddObject(module, "HOR_RES", PyLong_FromLong(LV_HOR_RES));
    PyModule_AddObject(module, "VER_RES", PyLong_FromLong(LV_VER_RES));

    driver.disp_flush = disp_flush;

    lv_init();
    
    lv_disp_drv_register(&driver);



    // Create a Python object for the active screen lv_obj and register it
    pylv_Obj * pyact = PyObject_New(pylv_Obj, &pylv_obj_Type);
    lv_obj_t *act = lv_scr_act();
    pyact -> ref = act;
    lv_obj_set_free_ptr(act, pyact);


    return module;
    
error:
    Py_XDECREF(module);
    return NULL;
}

