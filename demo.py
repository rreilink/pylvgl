import lvgl
from PySide2 import QtGui, QtWidgets, QtCore
app = QtWidgets.QApplication([])

label = QtWidgets.QLabel();
label.setMinimumSize(lvgl.HOR_RES, lvgl.VER_RES)
label.setMaximumSize(lvgl.HOR_RES, lvgl.VER_RES)

label.show()

def update():
    # Poll lvgl and display the framebuffer
    for i in range(10):
        lvgl.poll()

    data = bytes(lvgl.framebuffer)
    img = QtGui.QImage(data, lvgl.HOR_RES, lvgl.VER_RES, QtGui.QImage.Format_RGB16) 
    pm = QtGui.QPixmap.fromImage(img)
    
    label.setPixmap(pm)

t = QtCore.QTimer()
t.timeout.connect(update)
t.start(100)

# Build a GUI
# run this script using -i to try commands interactively
b1 = lvgl.Btn(lvgl.scr_act())
b1.set_size(150,50)
b1.align(b1.get_parent(), lvgl.ALIGN_IN_TOP_MID, 0, 10)

l1 = lvgl.Label(b1)
l1.set_text('Push 1')

s1 = lvgl.Slider(lvgl.scr_act())
s1.align(b1, lvgl.ALIGN_OUT_BOTTOM_MID, 0, 10)

s1 = lvgl.scr_act()
s2 = lvgl.Obj()
lst = lvgl.List(s2)
lst.set_width(320)
lst.set_height(200)
lst.set_sb_mode(lvgl.SB_MODE_AUTO)

# No spacing between items
st1 = lst.get_style(lvgl.LIST_STYLE_SCRL).copy()
st1.body_padding_inner = 0
lst.set_style(lvgl.LIST_STYLE_SCRL, st1)

style = lvgl.style_plain.copy() # copy
style.body_main_color = 0xffff
style.body_grad_color = 0xffff
style.text_color = 0
style.body_padding_ver = 0

for st in [lvgl.LIST_STYLE_BTN_PR, lvgl.LIST_STYLE_BTN_REL, lvgl.LIST_STYLE_BTN_TGL_PR, lvgl.LIST_STYLE_BTN_TGL_REL]:
    lst.set_style(st, style)



items = [lst.add(None, f'Btn {i}', None) for i in range(20)]

lvgl.scr_load(s2)


app.exec_()
