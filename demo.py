import lvgl
from PySide2 import QtGui, QtWidgets, QtCore

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
b = lvgl.Btn(lvgl.scr_act())
l = lvgl.Label(b)
l.set_text('Push')


