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

# style = l1.get_style()
# style.text_color = 0x003f;
# l1.refresh_style()

kbd =lvgl.Kb(lvgl.scr_act())
s = kbd.get_style(lvgl.BTNM_STYLE_BG)
s.body_padding_inner = 2
kbd.refresh_style()
kbd.set_map([b'test',b'hoi', b'\n', b'test2', b'\203test3'])

app.exec_()
