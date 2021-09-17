import sys
sys.path.append('..')
import lvgl
from PyQt5 import QtGui, QtWidgets, QtCore

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

class DemoApplication:
    def __init__(self):
        self.app = QtWidgets.QApplication([])
        self.window = LvglWindow();
        self.window.show()
        self.init_gui()
    def run(self):
        self.app.exec_()
        

    
class ButtonDemoApplication(DemoApplication):
    def init_gui(self):

        EVENT_NAMES = {getattr(lvgl.EVENT, name) : name for name in dir(lvgl.EVENT) if not name.startswith('_')}

        self.button = lvgl.Btn(lvgl.scr_act())
        self.button.set_size(150,50)
        self.button.align(self.button.get_parent(), lvgl.ALIGN.IN_TOP_MID, 0, 10)
        
        lvgl.Label(self.button).set_text('Push me')
        
        self.label = lvgl.Label(lvgl.scr_act())
        self.label.set_text('message')
        self.label.set_size(150,50)
        self.label.align(self.button, lvgl.ALIGN.OUT_BOTTOM_LEFT, 0, 0)
        
        self.button.set_event_cb(lambda event: self.label.set_text(EVENT_NAMES.get(event, '?')))

if __name__ == '__main__':
    app = ButtonDemoApplication()
    app.run()
  

