#!/usr/bin/env python3
import lvgl
import faulthandler
faulthandler.enable()
from PyQt5 import QtGui, QtWidgets, QtCore
app = QtWidgets.QApplication([])

class LvglWindow(QtWidgets.QLabel):
    def __init__(self):
        super().__init__()
        self.setMinimumSize(lvgl.HOR_RES, lvgl.VER_RES)
        self.setMaximumSize(lvgl.HOR_RES, lvgl.VER_RES)
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self.update)
        self.timer.start(10)

    def mousePressEvent(self, evt):
        self.mouseMoveEvent(evt)
    def mouseReleaseEvent(self, evt):
        self.mouseMoveEvent(evt)
    def mouseMoveEvent(self, evt):
        pos = evt.pos()
        lvgl.send_mouse_event(pos.x(), pos.y(), evt.buttons() & QtCore.Qt.LeftButton)
    def update(self):
        # Poll lvgl and display the framebuffer
        for i in range(10):
            lvgl.poll()

        data = bytes(lvgl.framebuffer)
        img = QtGui.QImage(data, lvgl.HOR_RES, lvgl.VER_RES, QtGui.QImage.Format_RGB16)
        pm = QtGui.QPixmap.fromImage(img)

        self.setPixmap(pm)

win = LvglWindow();
win.show()



# Build a GUI
# run this script using -i to try commands interactively
b1 = lvgl.Btn(lvgl.scr_act())
b1.set_size(150,50)
b1.align(b1.get_parent(), lvgl.ALIGN.IN_TOP_MID, 0, 10)

l1 = lvgl.Label(b1)
l1.set_text('Push 1')

s1 = lvgl.Slider(lvgl.scr_act())
s1.align(b1, lvgl.ALIGN.OUT_BOTTOM_MID, 0, 10)


EVENT_NAMES = {getattr(lvgl.EVENT, name) : name for name in dir(lvgl.EVENT) if not name.startswith('_')}
def print_event(id):
    print(EVENT_NAMES.get(id, '?'))

b1.set_event_cb(print_event)
s1.set_event_cb(print_event)

# set_style_local
l1.set_style_local_text_letter_space(lvgl.BTN_PART.MAIN, lvgl.STATE.DEFAULT, 10)

style = lvgl.Style()
style.set_radius(lvgl.STATE.DEFAULT, 10)
style.set_bg_color(lvgl.STATE.DEFAULT, lvgl.COLOR.AQUA.full)
b1.add_style(lvgl.BTN_PART.MAIN, style)

#b1.set_style_local_radius(lvgl.BTN_PART.MAIN, lvgl.STATE.DEFAULT, 10)



s1 = lvgl.scr_act()

s2 = lvgl.Obj()
lst = lvgl.List(s2)
lst.set_width(320)
lst.set_height(200)
lst.set_scrlbar_mode(lvgl.SCROLLBAR_MODE.AUTO)

# No spacing between items
#st1 = lvgl.style_t(lst.get_style(lvgl.LIST_STYLE.SCRL))
#st1.body.padding.inner = 0
#lst.set_style(lvgl.LIST_STYLE.SCRL, st1)

#style = lvgl.style_t(lvgl.style_plain)
#style.body.main_color = {'full' : 0xffff}
#style.body.grad_color = lvgl.COLOR.WHITE
#style.text.color = {'full': 0}
#style.body.padding.top = 0
#style.body.padding.bottom = 0
#style.body.padding.inner = 0


#for st in [lvgl.LIST_STYLE.BTN_PR, lvgl.LIST_STYLE.BTN_REL, lvgl.LIST_STYLE.BTN_TGL_PR, lvgl.LIST_STYLE.BTN_TGL_REL]:
#    lst.set_style(st, lvgl.style_plain)


items = [lst.add_btn(None, f'Btn {i}') for i in range(20)]



#symbolstyle = lvgl.style_t(lvgl.style_plain)
#symbolstyle.text_font = lvgl.font_symbol_40
#symbolstyle.text.color = {'full' : 0xffff}

class SymbolButton(lvgl.Btn):
    def __init__(self, symbol, text, *args, **kwds):
        super().__init__(*args, **kwds)
        self.symbol = lvgl.Label(self)
        self.symbol.set_text(symbol)
        #self.symbol.set_style(symbolstyle)
        self.symbol.align(self, lvgl.ALIGN.CENTER,0,0)

        self.label = lvgl.Label(self)
        self.label.set_text(text)
        self.label.align(self, lvgl.ALIGN.CENTER,20,0)


class MainMenu(lvgl.Obj):
    def __init__(self, *args, **kwds):
        super().__init__(*args, **kwds)
        self.btnPrint = SymbolButton(lvgl.SYMBOL.PLAY, 'Print', self)
        self.btnPrint.set_x(0)
        self.btnPrint.set_y(0)
        self.btnPrint.set_width(160)
        self.btnPrint.set_height(90)
        self.btnChange = SymbolButton(lvgl.SYMBOL.SHUFFLE, 'Change filament', self)
        self.btnChange.set_x(160)
        self.btnChange.set_y(0)
        self.btnChange.set_width(160)
        self.btnChange.set_height(90)
        self.btnPreheat = SymbolButton(lvgl.SYMBOL.CHARGE, 'Preheat', self)
        self.btnPreheat.set_x(0)
        self.btnPreheat.set_y(90)
        self.btnPreheat.set_width(160)
        self.btnPreheat.set_height(90)
        self.btnSettings = SymbolButton(lvgl.SYMBOL.SETTINGS, 'Settings', self)
        self.btnSettings.set_x(160)
        self.btnSettings.set_y(90)
        self.btnSettings.set_width(160)
        self.btnSettings.set_height(90)
        self.lblStatus=lvgl.Label(self)
        self.lblStatus.set_text(lvgl.SYMBOL.CHARGE + ' heating')
        self.lblStatus.align(self, lvgl.ALIGN.IN_BOTTOM_LEFT, 5, -5)


s3 = MainMenu()

#s4 = lvgl.Obj()
#kb = lvgl.Kb(s4)
lvgl.scr_load(s3)

s3.btnPrint.set_event_cb(print)

print(s3.btnPrint.get_type())

app.exec_()

