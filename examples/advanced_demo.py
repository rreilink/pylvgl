from basicdemo_qt import DemoApplication, lvgl



symbolstyle = lvgl.style_plain.copy()
symbolstyle.text_font = lvgl.font_symbol_40
symbolstyle.text_color = 0xffff

class SymbolButton(lvgl.Btn):
    def __init__(self, symbol, text, *args, **kwds):
        super().__init__(*args, **kwds)
        self.symbol = lvgl.Label(self)
        self.symbol.set_text(symbol)
        self.symbol.set_style(symbolstyle)
        self.symbol.align(self, lvgl.ALIGN.CENTER,0,0)
        
        self.label = lvgl.Label(self)
        self.label.set_text(text)
        self.label.align(self, lvgl.ALIGN.CENTER,20,0)
        
class Page_Buttons:
    def __init__(self, app, page):
        self.app = app
        self.page = page
        self.btn1 = SymbolButton(lvgl.SYMBOL_PLAY, "Play", page)
        self.btn1.set_size(140,100)
        self.btn2 = SymbolButton(lvgl.SYMBOL_PAUSE, "Pause", page)
        self.btn2.set_size(140,100)
        self.btn2.align(self.btn1, lvgl.ALIGN.OUT_RIGHT_TOP, 10, 0)
    
        self.label = lvgl.Label(page)
        self.label.align(self.btn1, lvgl.ALIGN.OUT_BOTTOM_LEFT, 0, 10)

        for btn, name in [(self.btn1, 'Play'), (self.btn2, 'Pause')]:
            btn.set_action(lvgl.BTN_ACTION.CLICK, lambda name=name: self.label.set_text(name + ' click'))
            btn.set_action(lvgl.BTN_ACTION.PR, lambda name=name: self.label.set_text(name + ' press'))
            btn.set_action(lvgl.BTN_ACTION.LONG_PR, lambda name=name: self.label.set_text(name + ' long press'))
            btn.set_action(lvgl.BTN_ACTION.LONG_PR_REPEAT, lambda name=name: self.label.set_text(name + ' long press repeat'))


class Page_Simple:
    def __init__(self, app, page):
        self.app = app
        self.page = page
        
        # slider 
        self.slider = lvgl.Slider(page)
        self.slider.align(page, lvgl.ALIGN.IN_TOP_LEFT, 20, 0)
        self.slider_label = lvgl.Label(page)
        self.slider_label.align(self.slider, lvgl.ALIGN.OUT_RIGHT_MID, 15, 0)
        self.slider.set_action(self.on_slider_changed)
        self.on_slider_changed()
        
        # style selector
        self.styles = [('Plain', lvgl.style_plain), ('Plain color', lvgl.style_plain_color), ('Pretty', lvgl.style_pretty), ('Pretty color', lvgl.style_pretty_color)]
    
        self.style_selector = lvgl.Ddlist(page)
        self.style_selector.align(self.slider, lvgl.ALIGN.IN_BOTTOM_LEFT, 0, 40)
        self.style_selector.set_options('\n'.join(x[0] for x in self.styles))
        self.style_selector.set_action(self.on_style_selector_changed)
    
    def on_slider_changed(self):
        self.slider_label.set_text(str(self.slider.get_value()))

    def on_style_selector_changed(self):
        selected = self.style_selector.get_selected()
        self.app.screen_main.tabview.set_style(lvgl.TABVIEW_STYLE.BG, self.styles[selected][1])   





class Screen_Main(lvgl.Obj):
    def __init__(self, app, *args, **kwds):
        self.app = app
        super().__init__(*args, **kwds)
        
        self.tabview = lvgl.Tabview(self)
        self.page_simple = Page_Simple(self.app, self.tabview.add_tab('Simple'))
        self.page_buttons = Page_Buttons(self.app, self.tabview.add_tab('Buttons'))


class AdvancedDemoApplication(DemoApplication):
    def init_gui(self):
        self.screen_main = Screen_Main(self)
        lvgl.scr_load(self.screen_main)


if __name__ == '__main__':
    app = AdvancedDemoApplication()
    app.run()