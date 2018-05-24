#include "Python.h"
#include "structmember.h"
#include "lvgl/lvgl.h"

#if LV_COLOR_DEPTH != 16
#error Only 16 bits color depth is currently supported
#endif

typedef struct {
    PyObject_HEAD
    lv_obj_t *ref;
} pylv_Object;

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
//    {"text_color", (getter) Style_get_uint16, (setter) Style_set_uint16, "text_color", (void*)offsetof(lv_style_t, text.color)},
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
pylv_obj_get_children(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    lv_obj_t *child = NULL;
    pylv_Object *pychild;
    PyObject *ret = PyList_New(0);
    if (!ret) return NULL;
    
    while (1) {
        child = lv_obj_get_child(self->ref, child);
        if (!child) break;
        pychild = lv_obj_get_free_ptr(child);
        
        if (!pychild) {
            // Child is not known to Python, create a new Object instance
            pychild = PyObject_New(pylv_Object, &pylv_obj_Type);
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


static void
pylv_obj_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_obj_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_obj_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_obj_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_win_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_win_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_win_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_win_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_label_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_label_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_label_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_label_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_lmeter_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_lmeter_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_lmeter_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_lmeter_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_btnm_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_btnm_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_btnm_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_btnm_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_chart_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_chart_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_chart_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_chart_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_cont_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_cont_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_cont_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_cont_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_led_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_led_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_led_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_led_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_kb_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_kb_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_kb_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_kb_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_img_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_img_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_img_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_img_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_bar_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_bar_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_bar_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_bar_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_line_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_line_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_line_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_line_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_tabview_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_tabview_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_tabview_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_tabview_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_mbox_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_mbox_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_mbox_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_mbox_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_gauge_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_gauge_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_gauge_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_gauge_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_page_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_page_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_page_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_page_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_ta_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_ta_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_ta_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_ta_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_btn_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_btn_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_btn_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_btn_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_ddlist_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_ddlist_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_ddlist_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_ddlist_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_list_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_list_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_list_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_list_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_slider_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_slider_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_slider_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_slider_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_sw_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_sw_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_sw_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_sw_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_cb_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_cb_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_cb_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_cb_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}

static void
pylv_roller_dealloc(pylv_Object *self) 
{
    // TODO: delete lvgl object? How to manage whether it has references in LittlevGL?

}

static int
pylv_roller_init(pylv_Object *self, PyObject *args, PyObject *kwds) 
{
    static char *kwlist[] = {"parent", "copy", NULL};
    pylv_Object *parent;
    pylv_Object *copy=NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|O!", kwlist, &pylv_obj_Type, &parent, &pylv_roller_Type, &copy)) {
        return -1;
    }   
    self->ref = lv_roller_create(parent->ref, copy ? copy->ref : NULL);
    lv_obj_set_free_ptr(self->ref, self);

    return 0;
}



static PyObject*
pylv_obj_invalidate(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    lv_obj_invalidate(self->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_parent(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"parent", NULL};
    pylv_Object * parent;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &pylv_obj_Type, &parent)) return NULL;
    lv_obj_set_parent(self->ref, parent->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_pos(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"x", "y", NULL};
    short int x;
    short int y;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &x, &y)) return NULL;
    lv_obj_set_pos(self->ref, x, y);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_x(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"x", NULL};
    short int x;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &x)) return NULL;
    lv_obj_set_x(self->ref, x);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_y(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"y", NULL};
    short int y;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &y)) return NULL;
    lv_obj_set_y(self->ref, y);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_size(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"w", "h", NULL};
    short int w;
    short int h;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &w, &h)) return NULL;
    lv_obj_set_size(self->ref, w, h);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_width(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"w", NULL};
    short int w;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &w)) return NULL;
    lv_obj_set_width(self->ref, w);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_height(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"h", NULL};
    short int h;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &h)) return NULL;
    lv_obj_set_height(self->ref, h);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_align(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"base", "align", "x_mod", "y_mod", NULL};
    pylv_Object * base;
    int align;
    short int x_mod;
    short int y_mod;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!ihh", kwlist , &pylv_obj_Type, &base, &align, &x_mod, &y_mod)) return NULL;
    lv_obj_align(self->ref, base->ref, align, x_mod, y_mod);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"style", NULL};
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &Style_Type, &style)) return NULL;
    lv_obj_set_style(self->ref, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_refresh_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    lv_obj_refresh_style(self->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_hidden(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;
    lv_obj_set_hidden(self->ref, en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_click(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;
    lv_obj_set_click(self->ref, en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_top(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;
    lv_obj_set_top(self->ref, en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_drag(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;
    lv_obj_set_drag(self->ref, en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_drag_throw(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;
    lv_obj_set_drag_throw(self->ref, en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_drag_parent(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;
    lv_obj_set_drag_parent(self->ref, en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_protect(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"prot", NULL};
    unsigned char prot;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &prot)) return NULL;
    lv_obj_set_protect(self->ref, prot);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_clear_protect(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"prot", NULL};
    unsigned char prot;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &prot)) return NULL;
    lv_obj_clear_protect(self->ref, prot);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_set_signal_func(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_obj_set_design_func(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_obj_refresh_ext_size(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    lv_obj_refresh_ext_size(self->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_obj_get_screen(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    lv_obj_t *result = lv_obj_get_screen(self->ref);
    
    if (!result) Py_RETURN_NONE;
    
    PyObject *retobj = lv_obj_get_free_ptr(result);
    if (retobj) {
        Py_INCREF(retobj);
        return retobj;
    } else {
        PyErr_SetString(PyExc_RuntimeError, "resulting object not known in Python");
        return NULL;
    }
}

static PyObject*
pylv_obj_get_parent(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    lv_obj_t *result = lv_obj_get_parent(self->ref);
    
    if (!result) Py_RETURN_NONE;
    
    PyObject *retobj = lv_obj_get_free_ptr(result);
    if (retobj) {
        Py_INCREF(retobj);
        return retobj;
    } else {
        PyErr_SetString(PyExc_RuntimeError, "resulting object not known in Python");
        return NULL;
    }
}

static PyObject*
pylv_obj_get_coords(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_obj_get_x(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    short int result = lv_obj_get_x(self->ref);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_y(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    short int result = lv_obj_get_y(self->ref);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_width(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    short int result = lv_obj_get_width(self->ref);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_height(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    short int result = lv_obj_get_height(self->ref);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_ext_size(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    short int result = lv_obj_get_ext_size(self->ref);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_obj_get_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    return Style_From_lv_style(lv_obj_get_style(self->ref));
}

static PyObject*
pylv_obj_get_hidden(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_obj_get_hidden(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_click(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_obj_get_click(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_top(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_obj_get_top(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_drag(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_obj_get_drag(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_drag_throw(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_obj_get_drag_throw(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_drag_parent(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_obj_get_drag_parent(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_is_protected(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"prot", NULL};
    unsigned char prot;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &prot)) return NULL;
    int result = lv_obj_is_protected(self->ref, prot);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_obj_get_signal_func(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_obj_get_design_func(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_obj_get_type(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_obj_get_group(pylv_Object *self, PyObject *args, PyObject *kwds)
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
    .tp_doc = "lvgl obj",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_obj_init,
    .tp_dealloc = (destructor) pylv_obj_dealloc,
    .tp_methods = pylv_obj_methods,
};

static PyObject*
pylv_win_add_btn(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_win_close_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_win_close_action(self->ref);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_win_set_title(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"title", NULL};
    char * title;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &title)) return NULL;
    lv_win_set_title(self->ref, title);
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_btn_size(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"size", NULL};
    short int size;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &size)) return NULL;
    lv_win_set_btn_size(self->ref, size);
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_sb_mode(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"sb_mode", NULL};
    int sb_mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &sb_mode)) return NULL;
    lv_win_set_sb_mode(self->ref, sb_mode);
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_layout(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"layout", NULL};
    int layout;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &layout)) return NULL;
    lv_win_set_layout(self->ref, layout);
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;
    lv_win_set_style(self->ref, type, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_win_get_btn_size(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    short int result = lv_win_get_btn_size(self->ref);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_win_get_layout(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_win_get_layout(self->ref);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_win_get_sb_mode(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_win_get_sb_mode(self->ref);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_win_get_width(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    short int result = lv_win_get_width(self->ref);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_win_get_from_btn(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    lv_obj_t *result = lv_win_get_from_btn(self->ref);
    
    if (!result) Py_RETURN_NONE;
    
    PyObject *retobj = lv_obj_get_free_ptr(result);
    if (retobj) {
        Py_INCREF(retobj);
        return retobj;
    } else {
        PyErr_SetString(PyExc_RuntimeError, "resulting object not known in Python");
        return NULL;
    }
}

static PyObject*
pylv_win_get_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_win_get_style(self->ref, type));
}

static PyObject*
pylv_win_focus(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"obj", "anim_time", NULL};
    pylv_Object * obj;
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!H", kwlist , &pylv_obj_Type, &obj, &anim_time)) return NULL;
    lv_win_focus(self->ref, obj->ref, anim_time);
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
    .tp_doc = "lvgl win",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_win_init,
    .tp_dealloc = (destructor) pylv_win_dealloc,
    .tp_methods = pylv_win_methods,
};

static PyObject*
pylv_label_set_text(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"text", NULL};
    char * text;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &text)) return NULL;
    lv_label_set_text(self->ref, text);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_array_text(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"array", "size", NULL};
    char * array;
    unsigned short int size;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "sH", kwlist , &array, &size)) return NULL;
    lv_label_set_array_text(self->ref, array, size);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_static_text(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"text", NULL};
    char * text;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &text)) return NULL;
    lv_label_set_static_text(self->ref, text);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_long_mode(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"long_mode", NULL};
    int long_mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &long_mode)) return NULL;
    lv_label_set_long_mode(self->ref, long_mode);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_align(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"align", NULL};
    int align;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &align)) return NULL;
    lv_label_set_align(self->ref, align);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_recolor(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"recolor_en", NULL};
    int recolor_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &recolor_en)) return NULL;
    lv_label_set_recolor(self->ref, recolor_en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_no_break(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"no_break_en", NULL};
    int no_break_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &no_break_en)) return NULL;
    lv_label_set_no_break(self->ref, no_break_en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_body_draw(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"body_en", NULL};
    int body_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &body_en)) return NULL;
    lv_label_set_body_draw(self->ref, body_en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_anim_speed(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"anim_speed", NULL};
    unsigned short int anim_speed;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_speed)) return NULL;
    lv_label_set_anim_speed(self->ref, anim_speed);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"style", NULL};
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &Style_Type, &style)) return NULL;
    lv_label_set_style(self->ref, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_get_long_mode(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_label_get_long_mode(self->ref);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_label_get_align(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_label_get_align(self->ref);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_label_get_recolor(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_label_get_recolor(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_label_get_no_break(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_label_get_no_break(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_label_get_body_draw(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_label_get_body_draw(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_label_get_letter_pos(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_label_ins_text(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"pos", "txt", NULL};
    unsigned int pos;
    char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Is", kwlist , &pos, &txt)) return NULL;
    lv_label_ins_text(self->ref, pos, txt);
    Py_RETURN_NONE;
}

static PyObject*
pylv_label_cut_text(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"pos", "cnt", NULL};
    unsigned int pos;
    unsigned int cnt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "II", kwlist , &pos, &cnt)) return NULL;
    lv_label_cut_text(self->ref, pos, cnt);
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
    {"set_style", (PyCFunction) pylv_label_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
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
    .tp_doc = "lvgl label",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_label_init,
    .tp_dealloc = (destructor) pylv_label_dealloc,
    .tp_methods = pylv_label_methods,
};

static PyObject*
pylv_lmeter_set_value(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"value", NULL};
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &value)) return NULL;
    lv_lmeter_set_value(self->ref, value);
    Py_RETURN_NONE;
}

static PyObject*
pylv_lmeter_set_range(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"min", "max", NULL};
    short int min;
    short int max;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &min, &max)) return NULL;
    lv_lmeter_set_range(self->ref, min, max);
    Py_RETURN_NONE;
}

static PyObject*
pylv_lmeter_set_scale(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"angle", "line_cnt", NULL};
    unsigned short int angle;
    unsigned char line_cnt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hb", kwlist , &angle, &line_cnt)) return NULL;
    lv_lmeter_set_scale(self->ref, angle, line_cnt);
    Py_RETURN_NONE;
}

static PyObject*
pylv_lmeter_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"bg", NULL};
    Style_Object * bg;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &Style_Type, &bg)) return NULL;
    lv_lmeter_set_style(self->ref, bg->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_lmeter_get_style_bg(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    return Style_From_lv_style(lv_lmeter_get_style_bg(self->ref));
}
static PyMethodDef pylv_lmeter_methods[] = {
    {"set_value", (PyCFunction) pylv_lmeter_set_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_range", (PyCFunction) pylv_lmeter_set_range, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_scale", (PyCFunction) pylv_lmeter_set_scale, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_lmeter_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style_bg", (PyCFunction) pylv_lmeter_get_style_bg, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};
static PyTypeObject pylv_lmeter_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Lmeter",
    .tp_doc = "lvgl lmeter",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_lmeter_init,
    .tp_dealloc = (destructor) pylv_lmeter_dealloc,
    .tp_methods = pylv_lmeter_methods,
};

static PyObject*
pylv_btnm_set_map(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_btnm_set_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_btnm_set_toggle(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", "id", NULL};
    int en;
    unsigned short int id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "pH", kwlist , &en, &id)) return NULL;
    lv_btnm_set_toggle(self->ref, en, id);
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;
    lv_btnm_set_style(self->ref, type, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_btnm_get_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_btnm_get_style(pylv_Object *self, PyObject *args, PyObject *kwds)
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
    .tp_doc = "lvgl btnm",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_btnm_init,
    .tp_dealloc = (destructor) pylv_btnm_dealloc,
    .tp_methods = pylv_btnm_methods,
};

static PyObject*
pylv_chart_add_series(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_chart_set_div_line_count(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"hdiv", "vdiv", NULL};
    unsigned char hdiv;
    unsigned char vdiv;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bb", kwlist , &hdiv, &vdiv)) return NULL;
    lv_chart_set_div_line_count(self->ref, hdiv, vdiv);
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_range(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"ymin", "ymax", NULL};
    short int ymin;
    short int ymax;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &ymin, &ymax)) return NULL;
    lv_chart_set_range(self->ref, ymin, ymax);
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_type(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    lv_chart_set_type(self->ref, type);
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_point_count(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"point_cnt", NULL};
    unsigned short int point_cnt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &point_cnt)) return NULL;
    lv_chart_set_point_count(self->ref, point_cnt);
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_series_opa(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"opa", NULL};
    unsigned char opa;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &opa)) return NULL;
    lv_chart_set_series_opa(self->ref, opa);
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_series_width(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"width", NULL};
    short int width;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &width)) return NULL;
    lv_chart_set_series_width(self->ref, width);
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_set_series_darking(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"dark_eff", NULL};
    unsigned char dark_eff;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &dark_eff)) return NULL;
    lv_chart_set_series_darking(self->ref, dark_eff);
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_init_points(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_chart_set_points(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_chart_set_next(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_chart_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"style", NULL};
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &Style_Type, &style)) return NULL;
    lv_chart_set_style(self->ref, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_chart_get_type(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_chart_get_type(self->ref);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_chart_get_series_opa(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    unsigned char result = lv_chart_get_series_opa(self->ref);
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_chart_get_series_width(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    short int result = lv_chart_get_series_width(self->ref);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_chart_get_series_darking(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    unsigned char result = lv_chart_get_series_darking(self->ref);
    return Py_BuildValue("b", result);
}

static PyObject*
pylv_chart_refresh(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    lv_chart_refresh(self->ref);
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
    {"set_style", (PyCFunction) pylv_chart_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
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
    .tp_doc = "lvgl chart",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_chart_init,
    .tp_dealloc = (destructor) pylv_chart_dealloc,
    .tp_methods = pylv_chart_methods,
};

static PyObject*
pylv_cont_set_layout(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"layout", NULL};
    int layout;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &layout)) return NULL;
    lv_cont_set_layout(self->ref, layout);
    Py_RETURN_NONE;
}

static PyObject*
pylv_cont_set_fit(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"hor_en", "ver_en", NULL};
    int hor_en;
    int ver_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "pp", kwlist , &hor_en, &ver_en)) return NULL;
    lv_cont_set_fit(self->ref, hor_en, ver_en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_cont_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"style", NULL};
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &Style_Type, &style)) return NULL;
    lv_cont_set_style(self->ref, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_cont_get_layout(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_cont_get_layout(self->ref);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_cont_get_hor_fit(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_cont_get_hor_fit(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_cont_get_ver_fit(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_cont_get_ver_fit(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_cont_get_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    return Style_From_lv_style(lv_cont_get_style(self->ref));
}
static PyMethodDef pylv_cont_methods[] = {
    {"set_layout", (PyCFunction) pylv_cont_set_layout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_fit", (PyCFunction) pylv_cont_set_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_cont_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_layout", (PyCFunction) pylv_cont_get_layout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_hor_fit", (PyCFunction) pylv_cont_get_hor_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_ver_fit", (PyCFunction) pylv_cont_get_ver_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_cont_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};
static PyTypeObject pylv_cont_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Cont",
    .tp_doc = "lvgl cont",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_cont_init,
    .tp_dealloc = (destructor) pylv_cont_dealloc,
    .tp_methods = pylv_cont_methods,
};

static PyObject*
pylv_led_set_bright(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"bright", NULL};
    unsigned char bright;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &bright)) return NULL;
    lv_led_set_bright(self->ref, bright);
    Py_RETURN_NONE;
}

static PyObject*
pylv_led_on(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    lv_led_on(self->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_led_off(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    lv_led_off(self->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_led_toggle(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    lv_led_toggle(self->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_led_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"style", NULL};
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &Style_Type, &style)) return NULL;
    lv_led_set_style(self->ref, style->ref);
    Py_RETURN_NONE;
}
static PyMethodDef pylv_led_methods[] = {
    {"set_bright", (PyCFunction) pylv_led_set_bright, METH_VARARGS | METH_KEYWORDS, NULL},
    {"on", (PyCFunction) pylv_led_on, METH_VARARGS | METH_KEYWORDS, NULL},
    {"off", (PyCFunction) pylv_led_off, METH_VARARGS | METH_KEYWORDS, NULL},
    {"toggle", (PyCFunction) pylv_led_toggle, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_led_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};
static PyTypeObject pylv_led_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Led",
    .tp_doc = "lvgl led",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_led_init,
    .tp_dealloc = (destructor) pylv_led_dealloc,
    .tp_methods = pylv_led_methods,
};

static PyObject*
pylv_kb_set_ta(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"ta", NULL};
    pylv_Object * ta;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &pylv_obj_Type, &ta)) return NULL;
    lv_kb_set_ta(self->ref, ta->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_kb_set_mode(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"mode", NULL};
    int mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &mode)) return NULL;
    lv_kb_set_mode(self->ref, mode);
    Py_RETURN_NONE;
}

static PyObject*
pylv_kb_set_cursor_manage(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;
    lv_kb_set_cursor_manage(self->ref, en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_kb_set_ok_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_kb_set_hide_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_kb_set_map(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_kb_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;
    lv_kb_set_style(self->ref, type, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_kb_get_ta(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    lv_obj_t *result = lv_kb_get_ta(self->ref);
    
    if (!result) Py_RETURN_NONE;
    
    PyObject *retobj = lv_obj_get_free_ptr(result);
    if (retobj) {
        Py_INCREF(retobj);
        return retobj;
    } else {
        PyErr_SetString(PyExc_RuntimeError, "resulting object not known in Python");
        return NULL;
    }
}

static PyObject*
pylv_kb_get_mode(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_kb_get_mode(self->ref);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_kb_get_cursor_manage(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_kb_get_cursor_manage(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_kb_get_ok_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_kb_get_hide_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_kb_get_style(pylv_Object *self, PyObject *args, PyObject *kwds)
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
    {"set_map", (PyCFunction) pylv_kb_set_map, METH_VARARGS | METH_KEYWORDS, NULL},
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
    .tp_doc = "lvgl kb",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_kb_init,
    .tp_dealloc = (destructor) pylv_kb_dealloc,
    .tp_methods = pylv_kb_methods,
};

static PyObject*
pylv_img_set_src(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_img_set_file(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"fn", NULL};
    char * fn;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &fn)) return NULL;
    lv_img_set_file(self->ref, fn);
    Py_RETURN_NONE;
}

static PyObject*
pylv_img_set_auto_size(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"autosize_en", NULL};
    int autosize_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &autosize_en)) return NULL;
    lv_img_set_auto_size(self->ref, autosize_en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_img_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"style", NULL};
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &Style_Type, &style)) return NULL;
    lv_img_set_style(self->ref, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_img_set_upscale(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"upcale", NULL};
    int upcale;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &upcale)) return NULL;
    lv_img_set_upscale(self->ref, upcale);
    Py_RETURN_NONE;
}

static PyObject*
pylv_img_get_src_type(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_img_get_auto_size(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_img_get_auto_size(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_img_get_upscale(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_img_get_upscale(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}
static PyMethodDef pylv_img_methods[] = {
    {"set_src", (PyCFunction) pylv_img_set_src, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_file", (PyCFunction) pylv_img_set_file, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_auto_size", (PyCFunction) pylv_img_set_auto_size, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_img_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_upscale", (PyCFunction) pylv_img_set_upscale, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_src_type", (PyCFunction) pylv_img_get_src_type, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_auto_size", (PyCFunction) pylv_img_get_auto_size, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_upscale", (PyCFunction) pylv_img_get_upscale, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};
static PyTypeObject pylv_img_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Img",
    .tp_doc = "lvgl img",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_img_init,
    .tp_dealloc = (destructor) pylv_img_dealloc,
    .tp_methods = pylv_img_methods,
};

static PyObject*
pylv_bar_set_value(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"value", NULL};
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &value)) return NULL;
    lv_bar_set_value(self->ref, value);
    Py_RETURN_NONE;
}

static PyObject*
pylv_bar_set_value_anim(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"value", "anim_time", NULL};
    short int value;
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hH", kwlist , &value, &anim_time)) return NULL;
    lv_bar_set_value_anim(self->ref, value, anim_time);
    Py_RETURN_NONE;
}

static PyObject*
pylv_bar_set_range(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"min", "max", NULL};
    short int min;
    short int max;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &min, &max)) return NULL;
    lv_bar_set_range(self->ref, min, max);
    Py_RETURN_NONE;
}

static PyObject*
pylv_bar_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;
    lv_bar_set_style(self->ref, type, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_bar_get_style(pylv_Object *self, PyObject *args, PyObject *kwds)
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
    .tp_doc = "lvgl bar",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_bar_init,
    .tp_dealloc = (destructor) pylv_bar_dealloc,
    .tp_methods = pylv_bar_methods,
};

static PyObject*
pylv_line_set_points(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_line_set_auto_size(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"autosize_en", NULL};
    int autosize_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &autosize_en)) return NULL;
    lv_line_set_auto_size(self->ref, autosize_en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_line_set_y_invert(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"yinv_en", NULL};
    int yinv_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &yinv_en)) return NULL;
    lv_line_set_y_invert(self->ref, yinv_en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_line_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"style", NULL};
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &Style_Type, &style)) return NULL;
    lv_line_set_style(self->ref, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_line_set_upscale(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"upcale", NULL};
    int upcale;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &upcale)) return NULL;
    lv_line_set_upscale(self->ref, upcale);
    Py_RETURN_NONE;
}

static PyObject*
pylv_line_get_auto_size(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_line_get_auto_size(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_line_get_y_inv(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_line_get_y_inv(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_line_get_upscale(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_line_get_upscale(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}
static PyMethodDef pylv_line_methods[] = {
    {"set_points", (PyCFunction) pylv_line_set_points, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_auto_size", (PyCFunction) pylv_line_set_auto_size, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_y_invert", (PyCFunction) pylv_line_set_y_invert, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_line_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_upscale", (PyCFunction) pylv_line_set_upscale, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_auto_size", (PyCFunction) pylv_line_get_auto_size, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_y_inv", (PyCFunction) pylv_line_get_y_inv, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_upscale", (PyCFunction) pylv_line_get_upscale, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};
static PyTypeObject pylv_line_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Line",
    .tp_doc = "lvgl line",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_line_init,
    .tp_dealloc = (destructor) pylv_line_dealloc,
    .tp_methods = pylv_line_methods,
};

static PyObject*
pylv_tabview_add_tab(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"name", NULL};
    char * name;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &name)) return NULL;

    lv_obj_t *result = lv_tabview_add_tab(self->ref, name);
    
    if (!result) Py_RETURN_NONE;
    
    PyObject *retobj = lv_obj_get_free_ptr(result);
    if (retobj) {
        Py_INCREF(retobj);
        return retobj;
    } else {
        PyErr_SetString(PyExc_RuntimeError, "resulting object not known in Python");
        return NULL;
    }
}

static PyObject*
pylv_tabview_set_tab_act(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", "anim_en", NULL};
    unsigned short int id;
    int anim_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hp", kwlist , &id, &anim_en)) return NULL;
    lv_tabview_set_tab_act(self->ref, id, anim_en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_tabview_set_tab_load_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_tabview_set_sliding(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;
    lv_tabview_set_sliding(self->ref, en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_tabview_set_anim_time(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"anim_time", NULL};
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_time)) return NULL;
    lv_tabview_set_anim_time(self->ref, anim_time);
    Py_RETURN_NONE;
}

static PyObject*
pylv_tabview_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;
    lv_tabview_set_style(self->ref, type, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_tabview_get_tab(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"id", NULL};
    unsigned short int id;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &id)) return NULL;

    lv_obj_t *result = lv_tabview_get_tab(self->ref, id);
    
    if (!result) Py_RETURN_NONE;
    
    PyObject *retobj = lv_obj_get_free_ptr(result);
    if (retobj) {
        Py_INCREF(retobj);
        return retobj;
    } else {
        PyErr_SetString(PyExc_RuntimeError, "resulting object not known in Python");
        return NULL;
    }
}

static PyObject*
pylv_tabview_get_tab_load_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_tabview_get_sliding(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_tabview_get_sliding(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_tabview_get_style(pylv_Object *self, PyObject *args, PyObject *kwds)
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
    .tp_doc = "lvgl tabview",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_tabview_init,
    .tp_dealloc = (destructor) pylv_tabview_dealloc,
    .tp_methods = pylv_tabview_methods,
};

static PyObject*
pylv_mbox_add_btns(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_mbox_set_text(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"txt", NULL};
    char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;
    lv_mbox_set_text(self->ref, txt);
    Py_RETURN_NONE;
}

static PyObject*
pylv_mbox_set_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_mbox_set_anim_time(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"anim_time", NULL};
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_time)) return NULL;
    lv_mbox_set_anim_time(self->ref, anim_time);
    Py_RETURN_NONE;
}

static PyObject*
pylv_mbox_start_auto_close(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"delay", NULL};
    unsigned short int delay;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &delay)) return NULL;
    lv_mbox_start_auto_close(self->ref, delay);
    Py_RETURN_NONE;
}

static PyObject*
pylv_mbox_stop_auto_close(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    lv_mbox_stop_auto_close(self->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_mbox_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;
    lv_mbox_set_style(self->ref, type, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_mbox_get_from_btn(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    lv_obj_t *result = lv_mbox_get_from_btn(self->ref);
    
    if (!result) Py_RETURN_NONE;
    
    PyObject *retobj = lv_obj_get_free_ptr(result);
    if (retobj) {
        Py_INCREF(retobj);
        return retobj;
    } else {
        PyErr_SetString(PyExc_RuntimeError, "resulting object not known in Python");
        return NULL;
    }
}

static PyObject*
pylv_mbox_get_style(pylv_Object *self, PyObject *args, PyObject *kwds)
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
    .tp_doc = "lvgl mbox",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_mbox_init,
    .tp_dealloc = (destructor) pylv_mbox_dealloc,
    .tp_methods = pylv_mbox_methods,
};

static PyObject*
pylv_gauge_set_needle_count(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_gauge_set_value(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"needle_id", "value", NULL};
    unsigned char needle_id;
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "bh", kwlist , &needle_id, &value)) return NULL;
    lv_gauge_set_value(self->ref, needle_id, value);
    Py_RETURN_NONE;
}

static PyObject*
pylv_gauge_set_range(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"min", "max", NULL};
    short int min;
    short int max;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &min, &max)) return NULL;
    lv_gauge_set_range(self->ref, min, max);
    Py_RETURN_NONE;
}

static PyObject*
pylv_gauge_set_critical_value(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"value", NULL};
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &value)) return NULL;
    lv_gauge_set_critical_value(self->ref, value);
    Py_RETURN_NONE;
}

static PyObject*
pylv_gauge_set_scale(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"angle", "line_cnt", "label_cnt", NULL};
    unsigned short int angle;
    unsigned char line_cnt;
    unsigned char label_cnt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hbb", kwlist , &angle, &line_cnt, &label_cnt)) return NULL;
    lv_gauge_set_scale(self->ref, angle, line_cnt, label_cnt);
    Py_RETURN_NONE;
}

static PyObject*
pylv_gauge_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"bg", NULL};
    Style_Object * bg;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!", kwlist , &Style_Type, &bg)) return NULL;
    lv_gauge_set_style(self->ref, bg->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_gauge_get_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    return Style_From_lv_style(lv_gauge_get_style(self->ref));
}
static PyMethodDef pylv_gauge_methods[] = {
    {"set_needle_count", (PyCFunction) pylv_gauge_set_needle_count, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_value", (PyCFunction) pylv_gauge_set_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_range", (PyCFunction) pylv_gauge_set_range, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_critical_value", (PyCFunction) pylv_gauge_set_critical_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_scale", (PyCFunction) pylv_gauge_set_scale, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_gauge_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_gauge_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};
static PyTypeObject pylv_gauge_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Gauge",
    .tp_doc = "lvgl gauge",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_gauge_init,
    .tp_dealloc = (destructor) pylv_gauge_dealloc,
    .tp_methods = pylv_gauge_methods,
};

static PyObject*
pylv_page_get_scrl(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    lv_obj_t *result = lv_page_get_scrl(self->ref);
    
    if (!result) Py_RETURN_NONE;
    
    PyObject *retobj = lv_obj_get_free_ptr(result);
    if (retobj) {
        Py_INCREF(retobj);
        return retobj;
    } else {
        PyErr_SetString(PyExc_RuntimeError, "resulting object not known in Python");
        return NULL;
    }
}

static PyObject*
pylv_page_set_rel_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_page_set_pr_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_page_set_sb_mode(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"sb_mode", NULL};
    int sb_mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &sb_mode)) return NULL;
    lv_page_set_sb_mode(self->ref, sb_mode);
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scrl_fit(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"hor_en", "ver_en", NULL};
    int hor_en;
    int ver_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "pp", kwlist , &hor_en, &ver_en)) return NULL;
    lv_page_set_scrl_fit(self->ref, hor_en, ver_en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scrl_width(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"w", NULL};
    short int w;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &w)) return NULL;
    lv_page_set_scrl_width(self->ref, w);
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scrl_height(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"h", NULL};
    short int h;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &h)) return NULL;
    lv_page_set_scrl_height(self->ref, h);
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_scrl_layout(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"layout", NULL};
    int layout;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &layout)) return NULL;
    lv_page_set_scrl_layout(self->ref, layout);
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;
    lv_page_set_style(self->ref, type, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_get_sb_mode(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_page_get_sb_mode(self->ref);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_page_get_scrl_width(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    short int result = lv_page_get_scrl_width(self->ref);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_page_get_scrl_height(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    short int result = lv_page_get_scrl_height(self->ref);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_page_get_scrl_layout(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_page_get_scrl_layout(self->ref);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_page_get_scrl_hor_fit(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_page_get_scrl_hor_fit(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_page_get_scrl_fit_ver(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_page_get_scrl_fit_ver(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_page_get_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_page_get_style(self->ref, type));
}

static PyObject*
pylv_page_glue_obj(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"glue", NULL};
    int glue;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &glue)) return NULL;
    lv_page_glue_obj(self->ref, glue);
    Py_RETURN_NONE;
}

static PyObject*
pylv_page_focus(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"obj", "anim_time", NULL};
    pylv_Object * obj;
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!H", kwlist , &pylv_obj_Type, &obj, &anim_time)) return NULL;
    lv_page_focus(self->ref, obj->ref, anim_time);
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
    .tp_doc = "lvgl page",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_page_init,
    .tp_dealloc = (destructor) pylv_page_dealloc,
    .tp_methods = pylv_page_methods,
};

static PyObject*
pylv_ta_add_char(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"c", NULL};
    char c;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "c", kwlist , &c)) return NULL;
    lv_ta_add_char(self->ref, c);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_add_text(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"txt", NULL};
    char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;
    lv_ta_add_text(self->ref, txt);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_del_char(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    lv_ta_del_char(self->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_text(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"txt", NULL};
    char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;
    lv_ta_set_text(self->ref, txt);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_cursor_pos(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"pos", NULL};
    short int pos;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &pos)) return NULL;
    lv_ta_set_cursor_pos(self->ref, pos);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_cursor_type(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"cur_type", NULL};
    int cur_type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &cur_type)) return NULL;
    lv_ta_set_cursor_type(self->ref, cur_type);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_pwd_mode(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"pwd_en", NULL};
    int pwd_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &pwd_en)) return NULL;
    lv_ta_set_pwd_mode(self->ref, pwd_en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_one_line(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"en", NULL};
    int en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &en)) return NULL;
    lv_ta_set_one_line(self->ref, en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_sb_mode(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"mode", NULL};
    int mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &mode)) return NULL;
    lv_ta_set_sb_mode(self->ref, mode);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;
    lv_ta_set_style(self->ref, type, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_get_label(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    lv_obj_t *result = lv_ta_get_label(self->ref);
    
    if (!result) Py_RETURN_NONE;
    
    PyObject *retobj = lv_obj_get_free_ptr(result);
    if (retobj) {
        Py_INCREF(retobj);
        return retobj;
    } else {
        PyErr_SetString(PyExc_RuntimeError, "resulting object not known in Python");
        return NULL;
    }
}

static PyObject*
pylv_ta_get_cursor_type(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_ta_get_cursor_type(self->ref);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_ta_get_pwd_mode(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_ta_get_pwd_mode(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_ta_get_one_line(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_ta_get_one_line(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_ta_get_sb_mode(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_ta_get_sb_mode(self->ref);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_ta_get_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_ta_get_style(self->ref, type));
}

static PyObject*
pylv_ta_cursor_right(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    lv_ta_cursor_right(self->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_cursor_left(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    lv_ta_cursor_left(self->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_cursor_down(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    lv_ta_cursor_down(self->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ta_cursor_up(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    lv_ta_cursor_up(self->ref);
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
    {"set_sb_mode", (PyCFunction) pylv_ta_set_sb_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_ta_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_label", (PyCFunction) pylv_ta_get_label, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_cursor_type", (PyCFunction) pylv_ta_get_cursor_type, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_pwd_mode", (PyCFunction) pylv_ta_get_pwd_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_one_line", (PyCFunction) pylv_ta_get_one_line, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_sb_mode", (PyCFunction) pylv_ta_get_sb_mode, METH_VARARGS | METH_KEYWORDS, NULL},
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
    .tp_doc = "lvgl ta",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_ta_init,
    .tp_dealloc = (destructor) pylv_ta_dealloc,
    .tp_methods = pylv_ta_methods,
};

static PyObject*
pylv_btn_set_toggle(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"tgl", NULL};
    int tgl;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &tgl)) return NULL;
    lv_btn_set_toggle(self->ref, tgl);
    Py_RETURN_NONE;
}

static PyObject*
pylv_btn_set_state(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"state", NULL};
    int state;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &state)) return NULL;
    lv_btn_set_state(self->ref, state);
    Py_RETURN_NONE;
}

static PyObject*
pylv_btn_toggle(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    lv_btn_toggle(self->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_btn_set_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_btn_set_layout(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"layout", NULL};
    int layout;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &layout)) return NULL;
    lv_btn_set_layout(self->ref, layout);
    Py_RETURN_NONE;
}

static PyObject*
pylv_btn_set_fit(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"hor_en", "ver_en", NULL};
    int hor_en;
    int ver_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "pp", kwlist , &hor_en, &ver_en)) return NULL;
    lv_btn_set_fit(self->ref, hor_en, ver_en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_btn_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;
    lv_btn_set_style(self->ref, type, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_btn_get_state(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_btn_get_state(self->ref);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_btn_get_toggle(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_btn_get_toggle(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_btn_get_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_btn_get_layout(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_btn_get_layout(self->ref);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_btn_get_hor_fit(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_btn_get_hor_fit(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_btn_get_ver_fit(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_btn_get_ver_fit(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_btn_get_style(pylv_Object *self, PyObject *args, PyObject *kwds)
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
    {"set_layout", (PyCFunction) pylv_btn_set_layout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_fit", (PyCFunction) pylv_btn_set_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_btn_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_state", (PyCFunction) pylv_btn_get_state, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_toggle", (PyCFunction) pylv_btn_get_toggle, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_action", (PyCFunction) pylv_btn_get_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_layout", (PyCFunction) pylv_btn_get_layout, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_hor_fit", (PyCFunction) pylv_btn_get_hor_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_ver_fit", (PyCFunction) pylv_btn_get_ver_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_btn_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};
static PyTypeObject pylv_btn_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Btn",
    .tp_doc = "lvgl btn",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_btn_init,
    .tp_dealloc = (destructor) pylv_btn_dealloc,
    .tp_methods = pylv_btn_methods,
};

static PyObject*
pylv_ddlist_set_options(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"options", NULL};
    char * options;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &options)) return NULL;
    lv_ddlist_set_options(self->ref, options);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_selected(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"sel_opt", NULL};
    unsigned short int sel_opt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &sel_opt)) return NULL;
    lv_ddlist_set_selected(self->ref, sel_opt);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_ddlist_set_fix_height(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"h", NULL};
    short int h;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &h)) return NULL;
    lv_ddlist_set_fix_height(self->ref, h);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_hor_fit(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"fit_en", NULL};
    int fit_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &fit_en)) return NULL;
    lv_ddlist_set_hor_fit(self->ref, fit_en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_sb_mode(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"mode", NULL};
    int mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &mode)) return NULL;
    lv_ddlist_set_sb_mode(self->ref, mode);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_anim_time(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"anim_time", NULL};
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_time)) return NULL;
    lv_ddlist_set_anim_time(self->ref, anim_time);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;
    lv_ddlist_set_style(self->ref, type, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_get_selected_str(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"buf", NULL};
    char * buf;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &buf)) return NULL;
    lv_ddlist_get_selected_str(self->ref, buf);
    Py_RETURN_NONE;
}

static PyObject*
pylv_ddlist_get_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_ddlist_get_fix_height(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    short int result = lv_ddlist_get_fix_height(self->ref);
    return Py_BuildValue("h", result);
}

static PyObject*
pylv_ddlist_get_sb_mode(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_ddlist_get_sb_mode(self->ref);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_ddlist_get_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_ddlist_get_style(self->ref, type));
}

static PyObject*
pylv_ddlist_open(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"anim", NULL};
    int anim;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &anim)) return NULL;
    lv_ddlist_open(self->ref, anim);
    Py_RETURN_NONE;
}
static PyMethodDef pylv_ddlist_methods[] = {
    {"set_options", (PyCFunction) pylv_ddlist_set_options, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_selected", (PyCFunction) pylv_ddlist_set_selected, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_action", (PyCFunction) pylv_ddlist_set_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_fix_height", (PyCFunction) pylv_ddlist_set_fix_height, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_hor_fit", (PyCFunction) pylv_ddlist_set_hor_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_sb_mode", (PyCFunction) pylv_ddlist_set_sb_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_anim_time", (PyCFunction) pylv_ddlist_set_anim_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_ddlist_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_selected_str", (PyCFunction) pylv_ddlist_get_selected_str, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_action", (PyCFunction) pylv_ddlist_get_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_fix_height", (PyCFunction) pylv_ddlist_get_fix_height, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_sb_mode", (PyCFunction) pylv_ddlist_get_sb_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_ddlist_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"open", (PyCFunction) pylv_ddlist_open, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};
static PyTypeObject pylv_ddlist_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Ddlist",
    .tp_doc = "lvgl ddlist",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_ddlist_init,
    .tp_dealloc = (destructor) pylv_ddlist_dealloc,
    .tp_methods = pylv_ddlist_methods,
};

static PyObject*
pylv_list_add(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_list_set_anim_time(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"anim_time", NULL};
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_time)) return NULL;
    lv_list_set_anim_time(self->ref, anim_time);
    Py_RETURN_NONE;
}

static PyObject*
pylv_list_set_sb_mode(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"mode", NULL};
    int mode;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &mode)) return NULL;
    lv_list_set_sb_mode(self->ref, mode);
    Py_RETURN_NONE;
}

static PyObject*
pylv_list_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;
    lv_list_set_style(self->ref, type, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_list_get_btn_label(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    lv_obj_t *result = lv_list_get_btn_label(self->ref);
    
    if (!result) Py_RETURN_NONE;
    
    PyObject *retobj = lv_obj_get_free_ptr(result);
    if (retobj) {
        Py_INCREF(retobj);
        return retobj;
    } else {
        PyErr_SetString(PyExc_RuntimeError, "resulting object not known in Python");
        return NULL;
    }
}

static PyObject*
pylv_list_get_btn_img(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;

    lv_obj_t *result = lv_list_get_btn_img(self->ref);
    
    if (!result) Py_RETURN_NONE;
    
    PyObject *retobj = lv_obj_get_free_ptr(result);
    if (retobj) {
        Py_INCREF(retobj);
        return retobj;
    } else {
        PyErr_SetString(PyExc_RuntimeError, "resulting object not known in Python");
        return NULL;
    }
}

static PyObject*
pylv_list_get_sb_mode(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_list_get_sb_mode(self->ref);
    return Py_BuildValue("i", result);
}

static PyObject*
pylv_list_get_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_list_get_style(self->ref, type));
}

static PyObject*
pylv_list_up(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    lv_list_up(self->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_list_down(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    lv_list_down(self->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_list_focus(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"anim_en", NULL};
    int anim_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &anim_en)) return NULL;
    lv_list_focus(self->ref, anim_en);
    Py_RETURN_NONE;
}
static PyMethodDef pylv_list_methods[] = {
    {"add", (PyCFunction) pylv_list_add, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_anim_time", (PyCFunction) pylv_list_set_anim_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_sb_mode", (PyCFunction) pylv_list_set_sb_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_list_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_btn_label", (PyCFunction) pylv_list_get_btn_label, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_btn_img", (PyCFunction) pylv_list_get_btn_img, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_sb_mode", (PyCFunction) pylv_list_get_sb_mode, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_list_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"up", (PyCFunction) pylv_list_up, METH_VARARGS | METH_KEYWORDS, NULL},
    {"down", (PyCFunction) pylv_list_down, METH_VARARGS | METH_KEYWORDS, NULL},
    {"focus", (PyCFunction) pylv_list_focus, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};
static PyTypeObject pylv_list_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.List",
    .tp_doc = "lvgl list",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_list_init,
    .tp_dealloc = (destructor) pylv_list_dealloc,
    .tp_methods = pylv_list_methods,
};

static PyObject*
pylv_slider_set_value(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"value", NULL};
    short int value;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "h", kwlist , &value)) return NULL;
    lv_slider_set_value(self->ref, value);
    Py_RETURN_NONE;
}

static PyObject*
pylv_slider_set_value_anim(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"value", "anim_time", NULL};
    short int value;
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hH", kwlist , &value, &anim_time)) return NULL;
    lv_slider_set_value_anim(self->ref, value, anim_time);
    Py_RETURN_NONE;
}

static PyObject*
pylv_slider_set_range(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"min", "max", NULL};
    short int min;
    short int max;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "hh", kwlist , &min, &max)) return NULL;
    lv_slider_set_range(self->ref, min, max);
    Py_RETURN_NONE;
}

static PyObject*
pylv_slider_set_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_slider_set_knob_in(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"in", NULL};
    int in;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &in)) return NULL;
    lv_slider_set_knob_in(self->ref, in);
    Py_RETURN_NONE;
}

static PyObject*
pylv_slider_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;
    lv_slider_set_style(self->ref, type, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_slider_get_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_slider_is_dragged(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_slider_is_dragged(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_slider_get_knob_in(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_slider_get_knob_in(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_slider_get_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_slider_get_style(self->ref, type));
}
static PyMethodDef pylv_slider_methods[] = {
    {"set_value", (PyCFunction) pylv_slider_set_value, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_value_anim", (PyCFunction) pylv_slider_set_value_anim, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_range", (PyCFunction) pylv_slider_set_range, METH_VARARGS | METH_KEYWORDS, NULL},
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
    .tp_doc = "lvgl slider",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_slider_init,
    .tp_dealloc = (destructor) pylv_slider_dealloc,
    .tp_methods = pylv_slider_methods,
};

static PyObject*
pylv_sw_on(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    lv_sw_on(self->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_sw_off(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    lv_sw_off(self->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_sw_set_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_sw_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;
    lv_sw_set_style(self->ref, type, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_sw_get_state(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_sw_get_state(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_sw_get_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_sw_get_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_sw_get_style(self->ref, type));
}
static PyMethodDef pylv_sw_methods[] = {
    {"on", (PyCFunction) pylv_sw_on, METH_VARARGS | METH_KEYWORDS, NULL},
    {"off", (PyCFunction) pylv_sw_off, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_action", (PyCFunction) pylv_sw_set_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_sw_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_state", (PyCFunction) pylv_sw_get_state, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_action", (PyCFunction) pylv_sw_get_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_sw_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};
static PyTypeObject pylv_sw_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Sw",
    .tp_doc = "lvgl sw",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_sw_init,
    .tp_dealloc = (destructor) pylv_sw_dealloc,
    .tp_methods = pylv_sw_methods,
};

static PyObject*
pylv_cb_set_text(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"txt", NULL};
    char * txt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &txt)) return NULL;
    lv_cb_set_text(self->ref, txt);
    Py_RETURN_NONE;
}

static PyObject*
pylv_cb_set_checked(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"checked", NULL};
    int checked;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &checked)) return NULL;
    lv_cb_set_checked(self->ref, checked);
    Py_RETURN_NONE;
}

static PyObject*
pylv_cb_set_inactive(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    lv_cb_set_inactive(self->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_cb_set_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_cb_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;
    lv_cb_set_style(self->ref, type, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_cb_is_checked(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_cb_is_checked(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_cb_get_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_cb_get_style(pylv_Object *self, PyObject *args, PyObject *kwds)
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
    .tp_doc = "lvgl cb",
    .tp_basicsize = sizeof(pylv_Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc) pylv_cb_init,
    .tp_dealloc = (destructor) pylv_cb_dealloc,
    .tp_methods = pylv_cb_methods,
};

static PyObject*
pylv_roller_set_options(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"options", NULL};
    char * options;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &options)) return NULL;
    lv_roller_set_options(self->ref, options);
    Py_RETURN_NONE;
}

static PyObject*
pylv_roller_set_selected(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"sel_opt", "anim_en", NULL};
    unsigned short int sel_opt;
    int anim_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Hp", kwlist , &sel_opt, &anim_en)) return NULL;
    lv_roller_set_selected(self->ref, sel_opt, anim_en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_roller_set_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_roller_set_visible_row_count(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"row_cnt", NULL};
    unsigned char row_cnt;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b", kwlist , &row_cnt)) return NULL;
    lv_roller_set_visible_row_count(self->ref, row_cnt);
    Py_RETURN_NONE;
}

static PyObject*
pylv_roller_set_hor_fit(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"fit_en", NULL};
    int fit_en;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "p", kwlist , &fit_en)) return NULL;
    lv_roller_set_hor_fit(self->ref, fit_en);
    Py_RETURN_NONE;
}

static PyObject*
pylv_roller_set_anim_time(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"anim_time", NULL};
    unsigned short int anim_time;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "H", kwlist , &anim_time)) return NULL;
    lv_roller_set_anim_time(self->ref, anim_time);
    Py_RETURN_NONE;
}

static PyObject*
pylv_roller_set_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", "style", NULL};
    int type;
    Style_Object * style;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO!", kwlist , &type, &Style_Type, &style)) return NULL;
    lv_roller_set_style(self->ref, type, style->ref);
    Py_RETURN_NONE;
}

static PyObject*
pylv_roller_get_selected_str(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"buf", NULL};
    char * buf;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist , &buf)) return NULL;
    lv_roller_get_selected_str(self->ref, buf);
    Py_RETURN_NONE;
}

static PyObject*
pylv_roller_get_action(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    PyErr_SetString(PyExc_NotImplementedError, "not implemented");
    return NULL;
}

static PyObject*
pylv_roller_get_hor_fit(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "", kwlist )) return NULL;
    int result = lv_roller_get_hor_fit(self->ref);
    if (result) {Py_RETURN_TRUE;} else {Py_RETURN_FALSE;}
}

static PyObject*
pylv_roller_get_style(pylv_Object *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"type", NULL};
    int type;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist , &type)) return NULL;
    return Style_From_lv_style(lv_roller_get_style(self->ref, type));
}
static PyMethodDef pylv_roller_methods[] = {
    {"set_options", (PyCFunction) pylv_roller_set_options, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_selected", (PyCFunction) pylv_roller_set_selected, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_action", (PyCFunction) pylv_roller_set_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_visible_row_count", (PyCFunction) pylv_roller_set_visible_row_count, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_hor_fit", (PyCFunction) pylv_roller_set_hor_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_anim_time", (PyCFunction) pylv_roller_set_anim_time, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_style", (PyCFunction) pylv_roller_set_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_selected_str", (PyCFunction) pylv_roller_get_selected_str, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_action", (PyCFunction) pylv_roller_get_action, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_hor_fit", (PyCFunction) pylv_roller_get_hor_fit, METH_VARARGS | METH_KEYWORDS, NULL},
    {"get_style", (PyCFunction) pylv_roller_get_style, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}  /* Sentinel */
};
static PyTypeObject pylv_roller_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "lvgl.Roller",
    .tp_doc = "lvgl roller",
    .tp_basicsize = sizeof(pylv_Object),
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


    PyModule_AddObject(module, "framebuffer", PyMemoryView_FromMemory(framebuffer, LV_HOR_RES * LV_VER_RES * 2, PyBUF_READ));
    PyModule_AddObject(module, "HOR_RES", PyLong_FromLong(LV_HOR_RES));
    PyModule_AddObject(module, "VER_RES", PyLong_FromLong(LV_VER_RES));

    driver.disp_flush = disp_flush;

    lv_init();
    
    lv_disp_drv_register(&driver);



    // Create a Python object for the active screen lv_obj and register it
    pylv_Object * pyact = PyObject_New(pylv_Object, &pylv_obj_Type);
    lv_obj_t *act = lv_scr_act();
    pyact -> ref = act;
    lv_obj_set_free_ptr(act, pyact);


    return module;
    
error:
    Py_XDECREF(module);
    return NULL;
}

