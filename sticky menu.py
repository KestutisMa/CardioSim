#%%
import panel as pn, numpy as np
import param, uuid
import holoviews as hv

from panel.reactive import ReactiveHTML
from panel.layout.base import ListLike

pn.config.css_files.append("https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css")

css = """
.dropbtn{
  margin: 0 0 15 0 px;
  border-radius: 60%;
  position: relative;
  top: 0px;
  color: white;
  background-color: #00aa41;
  border: none
}

.dropbtn:hover {
    background: #b6abab;
    transition:all 0.3s ease;
    
}
.dropdown-content{
  visibility:hidden;
  position:fixed; 
  box-shadow: 0 4px 8px 0 rgba(0, 0, 0, 0.2), 0 6px 20px 0 rgba(0, 0, 0, 0.19);
  border-radius: 5px;
  z-index: 9999999 !important;
}

.bk-root{
    z-index:inherit;    
    
}
"""
pn.extension(raw_css=[css]) 

class DragMenu(ListLike, ReactiveHTML):
    clicks = param.Integer(default=0)
    icon = param.String(default='home')
    menu = param.String(default='abc')
    js_dummy = pn.pane.HTML('')

    _template =  """    <div class="dropdown" id=${menu}>
                            <button class="dropbtn" id="btn" onclick='${btn_click}'> <i class="fa fa-${icon} fa-2x"></i> </button>
                            <div class="dropdown-content"> 
                                  {% for obj in objects %}
                                    <div id="dragmenu-{{ loop.index0 }}">
                                      ${objects[{{ loop.index0 }}]}
                                    </div>
                                  {% endfor %}
                            </div>
                        </div>
    """ 

    def __init__(self, *objects, **params):
        self.js_dummy = pn.pane.HTML('')
        self.myuuid = str(uuid.uuid4()).replace('-','')
        print ('before super', self.myuuid)
        self.menu = self.myuuid
        super().__init__(objects=list((*objects, self.js_dummy)), **params)
        print ('after super', self.myuuid)
        self.menu = self.myuuid
        self.js_dummy.object = f""" 
       <script> 
          console.log('before time out :   ' + '{str(self.menu)}');
  
window.setTimeout(() => 
    {{
console.log('uuid para definir in timeout :  ' + '{str(self.menu)}');
          dropdown_{self.menu} = document.getElementById('{self.menu}');
          dropdown_{self.menu}_childNodes = dropdown_{self.menu}.childNodes;
          //dropdown_{self.menu}_childNodes[1].style.visibility = 'visible';
          dropdown_{self.menu}_ClientRect = dropdown_{self.menu}_childNodes[0].getBoundingClientRect();
              //dropdown_{self.menu}_childNodes[1].style.right = "200px";


      dropdown_{self.menu}_ClientRect = dropdown_{self.menu}_childNodes[0].getBoundingClientRect();
        dropdown_{self.menu}_childNodes[1].style.right = String(window.innerWidth-dropdown_{self.menu}_ClientRect.right - 20) + 'px';
     
          console.log( String(window.innerWidth) );
          console.log(  dropdown_{self.menu}_ClientRect.right + 'px');
  
    }}, 250);

        </script> 
        """ 
        self.js_dummy.param.trigger('object')
        
    def btn_click(self, event):
        self.clicks += 1
        print (self.clicks, self.menu)
        self.js_dummy.object = f""" 
        <script>
          console.log('log from btn   -->' + '{self.menu}');
           if (dropdown_{self.menu}_childNodes[1].style.visibility != 'visible') 
            {{dropdown_{self.menu}_childNodes[1].style.visibility='visible'}}
          else  {{dropdown_{self.menu}_childNodes[1].style.visibility='hidden'}}
          console.log({self.clicks})
         </script>
        """
        self.js_dummy.param.trigger('object')


def get_tmpl():
  tmpl = pn.template.VanillaTemplate(title='Icon Menu')

  dd0  = DragMenu(pn.WidgetBox(pn.pane.Markdown('# H1aaaa'),
                                  pn.widgets.FloatSlider(value=2,end=10)))
  dd1 = DragMenu(pn.WidgetBox(pn.pane.Markdown('# H2aaaa'),
                                  pn.widgets.FloatSlider(value=2,end=10),
        pn.pane.PNG('https://holoviz.org/assets/earthquakes.png', width= 500, height=300)
        ), icon='bicycle')

  grid = pn.GridBox('', 'light', 'dark', ncols=3)

  for color in pn.indicators.LoadingSpinner.param.color.objects:
      dark = pn.indicators.LoadingSpinner(width=50, height=50, value=True, color=color, bgcolor='dark')
      light = pn.indicators.LoadingSpinner(width=50, height=50, value=True, color=color, bgcolor='light')
      grid.extend((color, light, dark))


  xs = np.linspace(0, np.pi)
  freq = pn.widgets.FloatSlider(name="Frequency", start=0, end=10, value=2)
  phase = pn.widgets.FloatSlider(name="Phase", start=0, end=np.pi)

  @pn.depends(freq=freq, phase=phase)
  def sine(freq, phase):
      return hv.Curve((xs, np.sin(xs*freq+phase))).opts(
          responsive=True, min_height=500)

  @pn.depends(freq=freq, phase=phase)
  def cosine(freq, phase):
      return hv.Curve((xs, np.cos(xs*freq+phase))).opts(
          responsive=True, min_height=500)

  dd2 = DragMenu(pn.WidgetBox(grid))

  dd3 = DragMenu(pn.WidgetBox('# Trig control', freq, phase))

  row = pn.Row(pn.Spacer(sizing_mode='stretch_width'),
              dd0, dd1, dd2, dd3,
              margin=(0,0,0,0),
              sizing_mode='stretch_width')

  tmpl.header.append(row)

  tmpl.main.append(pn.Row(cosine,sizing_mode='stretch_both'))


  return tmpl

pn.extension(comms='vscode')
# get_tmpl().servable()

# BOKEH server
try:
    bokeh_srv.stop()
    bokeh_srv.join()
except:
    print('bokeh not running')
else:
    print('stopped bokeh server succsfully')

print('starting bokeh server')

hv.extension('plotly')
ui = get_tmpl()
bokeh_srv = ui.show(threaded=True)


