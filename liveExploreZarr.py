#%%
%%time
from typing import Dict
import holoviews as hv
from matplotlib.pyplot import scatter
from numpy.lib.shape_base import tile

import panel as pn
from panel.interact import interact, interactive, fixed, interact_manual
from panel import widgets

import numpy as np
from scipy.signal import find_peaks
import z5py

import plotly.graph_objs as go
import plotly.express as px

pn.extension(comms='vscode')

#%%
%%time

gj_plot_cell_x = np.array([22])
gj_plot_cell_y = np.array([0])

gj_min = 10e-9 # map coloerscale
gj_max = 30e-9

# cv_meas_cell_x = [21, 22]
cv_meas_cell_x = [20, 40]
cv_meas_cell_y = [0, 0]

# u_plot_cell_x = np.array([20, 40])
u_plot_cell_x = np.array([21, 22])
u_plot_cell_y = np.array([0, 0])

# u = np.load('reg_u_xy_iFrame.npy')
# gj = np.load('reg_gj_orientation_xy_iFrame.npy')

# f = z5py.File(r'C:\OpenGL\fentonGjOpenGL\build\RelWithDebInfo\data1.zr', use_zarr_format=True)
# f = z5py.File(r'C:\OpenGL\fentonGjOpenGL\build\RelWithDebInfo\data1-2021-12-16 300-46nS 60-120-180-240bpm.zr', use_zarr_format=True) #krenta geras
f = z5py.File(r'C:\OpenGL\fentonGjOpenGL\data1.zr', use_zarr_format=True)

def load_data():
    print(list(f.keys()))
    u_dataset = f['u']
    gj_dataset = f['gj']

    frames_count = u_dataset.attrs['frames_count'] 
    u = u_dataset[:frames_count]     
    gj = gj_dataset[:frames_count]     
    print('frames_count = ', frames_count)
    print(u.shape)
    print(gj.shape)
    # iterSelectSlider
    return u, gj


global u, gj
u,gj = load_data()
frames_count = u.shape[0]

# fig = fig.to_dict()

# plotly_pane = pn.pane.Plotly(fig)
# plotly_pane.object = fig

# iterSelectSlider = pn.widgets.FloatSlider(name='i_frame', start=0, end=frames_count-1, value=frames_count-1, step=1, value_throttled=0.1)
tSelectSlider = pn.widgets.FloatSlider(name='t, ms', start=0, end=(frames_count-1)*0.2, value=(frames_count-1)*0.2, step=0.2, value_throttled=0.1)
t_beginSelectSlider = pn.widgets.FloatSlider(name='t_bein, ms', start=0, end=(frames_count-1)*0.2, value=0, step=0.2, value_throttled=1)
t_endSelectSlider = pn.widgets.FloatSlider(name='t_end, ms', start=0, end=(frames_count-1)*0.2, value=(frames_count-1)*0.2, step=0.2, value_throttled=1)
refreshButton = pn.widgets.Button(name='Refresh')
# p = pn.Pane.Ploty
# param0 = ParamClass()

# refreshButton.on_click(on_button_clicked_fn)

@pn.depends(refreshButton.param.value, watch=True)
def update_range(val):
    global u, gj
    u, gj = load_data()
    frames_count = u.shape[0]
    frames_count = gj.shape[0]
    t_beginSelectSlider.end = frames_count*0.2
    t_endSelectSlider.end = frames_count*0.2
    t_endSelectSlider.value = frames_count*0.2
    tSelectSlider.end = frames_count*0.2
    tSelectSlider.value = frames_count*0.2
    # iterSelectSlider.value = 10

@pn.depends(t_beginSelectSlider.param.value, watch=True)
def setBegin(v):
    tSelectSlider.start = v
@pn.depends(t_endSelectSlider.param.value, watch=True)
def setEnd(v):
    tSelectSlider.end = v

# @pn.depends(tSelectSlider.param.end)
# def voltagePlot(t):
#     last_iter = int(t//0.2)
#     fig = px.scatter(u[:last_iter,reg_y,reg_x],  title='Vm') # cia :u tik del to kad kitaip neupdeitina plot, # pagal plotly doc : with the downside that the resulting traces will need to be manually renamed via fig.data[<n>].name = "name"
#     return fig
@pn.depends(tSelectSlider.param.end)
def voltagePlot(t):
    last_iter = t//0.2
    # fig = px.scatter(u[:v,reg_y,reg_x],  title='Vm') # cia :u tik del to kad kitaip neupdeitina plot, # pagal plotly doc : with the downside that the resulting traces will need to be manually renamed via fig.data[<n>].name = "name"
    fig = go.Figure() # cia :u tik del to kad kitaip neupdeitina plot
    for i in np.arange(len(u_plot_cell_x)): 
        fig.add_trace(go.Scattergl(x=np.multiply(np.arange(last_iter),0.2), y=u[:,u_plot_cell_y[i],u_plot_cell_x[i]], name=f"x {u_plot_cell_x[i]} y {u_plot_cell_y[i]}", mode = "lines+markers", hoverinfo='skip'))   
    fig.update_xaxes(title='t,ms')
    fig.update_yaxes(title='mV')
    fig.layout.title = 'Vm plot'
    fig.layout.legend.title='Reg. locations' # gal geriau fig.update_layout(legend_title_text='Trend') ?
    return fig

def getCV():
    meas_x = cv_meas_cell_x 
    meas_y = cv_meas_cell_y
    # meas_x = [20,40] 
    # meas_y = [0,0]
    pts_begin = (np.diff(u[:,meas_y[0],meas_x[0]]) > 0) & (u[:-1,meas_y[0],meas_x[0]] > -60) & (u[:-1,meas_y[0],meas_x[0]] < -30)
    # pts_begin = (np.diff(df1[meas1]) > 0) & df1[meas1].gt(-60)[:-1] & df1[meas1].lt(-30)[:-1] # butini (), bool[] - kai diff(isvestine) > 0 ir pati reiksme > 0mV (kad nufiltruoti pasvyravimus)
    pts_end =  (np.diff(u[:,meas_y[1],meas_x[1]]) > 0) & (u[:-1,meas_y[1],meas_x[1]] > -60) & (u[:-1,meas_y[1],meas_x[1]] < -30)
    # pts_end = (np.diff(df1[meas2]) > 0) & df1[meas2].gt(-60)[:-1] & df1[meas2].lt(-30)[:-1] # butini ()
    peaks_begin, _ = find_peaks(pts_begin) # grazina index
    peaks_end, _ = find_peaks(pts_end)
    # print(df1["t"][peaks_begin])
    # print(df1["t"][peaks_end].values-df1["t"][peaks_begin].values)
    # cv = (1*100e-6)/((peaks_end - peaks_begin[:len(peaks_end)])/1000) * 100 # *100 m/s->cm/s
    t1 = peaks_begin*0.2
    t2 = peaks_end*0.2
    cells_count = meas_x[1] - meas_x[0]
    cv = (cells_count*100e-6) / ( (t2 - t1[:len(t2)]) / 1000) * 100 # / 1000 s -> ms, *100 m/s->cm/s
    # t_delay = (df1["t"][peaks_end].values - df1["t"][peaks_begin][:len(peaks_end)].values)# *100 m/s->cm/s
    if len(peaks_begin) - len(peaks_end) > 1:
        print('Some waves were blocked and didnt reach end, require manual inspection of CV plot!!')
    # last_iter = t//0.2
    t_all = t2    
    return cv, t_all

@pn.depends(tSelectSlider.param.end)
def cvPlot(t):
    # meas_x = cv_meas_cell_x 
    # meas_y = cv_meas_cell_y
    # # meas_x = [20,40] 
    # # meas_y = [0,0]
    # pts_begin = (np.diff(u[:,meas_y[0],meas_x[0]]) > 0) & (u[:-1,meas_y[0],meas_x[0]] > -60) & (u[:-1,meas_y[0],meas_x[0]] < -30)
    # # pts_begin = (np.diff(df1[meas1]) > 0) & df1[meas1].gt(-60)[:-1] & df1[meas1].lt(-30)[:-1] # butini (), bool[] - kai diff(isvestine) > 0 ir pati reiksme > 0mV (kad nufiltruoti pasvyravimus)
    # pts_end =  (np.diff(u[:,meas_y[1],meas_x[1]]) > 0) & (u[:-1,meas_y[1],meas_x[1]] > -60) & (u[:-1,meas_y[1],meas_x[1]] < -30)
    # # pts_end = (np.diff(df1[meas2]) > 0) & df1[meas2].gt(-60)[:-1] & df1[meas2].lt(-30)[:-1] # butini ()
    # peaks_begin, _ = find_peaks(pts_begin) # grazina index
    # peaks_end, _ = find_peaks(pts_end)
    # # print(df1["t"][peaks_begin])
    # # print(df1["t"][peaks_end].values-df1["t"][peaks_begin].values)
    # # cv = (1*100e-6)/((peaks_end - peaks_begin[:len(peaks_end)])/1000) * 100 # *100 m/s->cm/s
    # t2 = peaks_end*0.2
    # t1 = peaks_begin*0.2
    # cells_count = meas_x[1] - meas_x[0]
    # cv = (cells_count*100e-6) / ( (t2 - t1[:len(t2)]) / 1000) * 100 # / 1000 s -> ms, *100 m/s->cm/s
    # # t_delay = (df1["t"][peaks_end].values - df1["t"][peaks_begin][:len(peaks_end)].values)# *100 m/s->cm/s
    # if len(peaks_begin) - len(peaks_end) > 1:
    #     print('Some waves were blocked and didnt reach end, require manual inspection of CV plot!!')
    # last_iter = t//0.2
    # t_all = t2
    cv, t_all = getCV()
    fig = px.scatter(x=t_all,y=cv, title=f'CV at x: {cv_meas_cell_x}, y: {cv_meas_cell_y}')
    fig.update_xaxes(title='t, ms')
    fig.update_yaxes(title='CV, cm/s')
    return fig

@pn.depends(tSelectSlider.param.end)
def gjPlot(t):
    last_iter = t//0.2
    sel = 0 # W
    fig = go.Figure() # cia :u tik del to kad kitaip neupdeitina plot
    for i in np.arange(len(gj_plot_cell_x)): 
        fig.add_trace(go.Scattergl(x=np.multiply(np.arange(last_iter),0.2), y=gj[:,gj_plot_cell_y[i],gj_plot_cell_x[i],sel], name=f"x {gj_plot_cell_x[i]} y {gj_plot_cell_y[i]}", mode = "lines+markers",hoverinfo='skip',))   
    fig.update_xaxes(title='t, ms')
    fig.update_yaxes(title='gj, S')
    fig.layout.title = f'gj plot at x: {gj_plot_cell_x}, y: {gj_plot_cell_y}'
    fig.layout.legend.title='Reg. locations' # gal geriau fig.update_layout(legend_title_text='Trend') ?        
    return fig

@pn.depends(tSelectSlider.param.value)
def voltageMap(t):
    last_iter = int(t//0.2)
    fig = px.imshow(u[last_iter], width=1500, zmin=-75, zmax=10, title='Vm map')
    fig.add_trace(go.Scatter(x=gj_plot_cell_x, y=gj_plot_cell_y, mode = "markers", hoverinfo='skip') )
    return fig#.to_dict()

@pn.depends(tSelectSlider.param.value)
def gjMap(t):
    last_iter = int(t//0.2)
    fig = px.imshow(gj[last_iter,:,:,0], width=1500, zmin=gj_min, zmax=gj_max, color_continuous_scale='Turbo' , title='gj map')
    # fig = px.imshow(gj[last_iter,:,:,0], width=1500, title='gj map')
    fig.add_trace(go.Scattergl(x=gj_plot_cell_x, y=gj_plot_cell_y, mode = "markers", hoverinfo='skip') )    
    return fig

@pn.depends(tSelectSlider.param.value)
def uContour(t):
    i = int(t//0.2)
    size_y = u.shape[1] #y
    size_x = u.shape[2] #x
    ar = np.empty((size_y*2-1,size_x*2))
    # ar[:] = 0
    ar[:] = np.NaN
    for y in np.arange(size_y):
        for x in np.arange(size_x):
            if y % 2 == 0:                
                x_arr = x*2
            else:
                x_arr = x*2+1            
            y_arr = y*2
            ar[y_arr,x_arr] = u[i,y,x]

    fig = go.Figure(
        data=go.Contour(
        x=np.arange(0, size_x, 0.5),
        y=np.arange(0, size_y, 0.5),
        z=ar,
        connectgaps=True,
        line_smoothing=0,
        contours_coloring='heatmap',
        contours=dict(
            start=-75,
            end=20,
            size=5,
        ),
    ))
    fig.update_layout(
        # autosize=True,
        title='Vm contour',
        width=1500,
        height=300,
    )

    fig.add_trace(go.Scattergl(x=gj_plot_cell_x, y=gj_plot_cell_y, mode = "markers", hoverinfo='skip') )

    return fig

plotly_pane = pn.Column(voltagePlot, pn.Row(cvPlot, gjPlot), refreshButton,tSelectSlider, t_beginSelectSlider, t_endSelectSlider, gjMap, voltageMap)
# plotly_pane = pn.Column(voltagePlot, cvPlot, gjPlot, refreshButton,tSelectSlider, t_beginSelectSlider, t_endSelectSlider, gjMap, voltageMap)
# plotly_pane = pn.Column(voltagePlot, gjPlot, refreshButton,tSelectSlider, gjMap, voltageMap, uContour)


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
# hd.shade.cmap=["lightblue", "darkblue"]
# hv.extension('bokeh') 
ui = plotly_pane
# row = pn.Row(myshade * myshade2)
# row = pn.Column(myshade, height=200, width=1000)
# row.servable()
bokeh_srv = ui.show(threaded=True)



# %% save cv-t
def save_CV_t():
    meas_x = [20,40] 
    meas_y = [0,0]
    pts_begin = (np.diff(u[:,meas_y[0],meas_x[0]]) > 0) & (u[:-1,meas_y[0],meas_x[0]] > -60) & (u[:-1,meas_y[0],meas_x[0]] < -30)
    # pts_begin = (np.diff(df1[meas1]) > 0) & df1[meas1].gt(-60)[:-1] & df1[meas1].lt(-30)[:-1] # butini (), bool[] - kai diff(isvestine) > 0 ir pati reiksme > 0mV (kad nufiltruoti pasvyravimus)
    pts_end =  (np.diff(u[:,meas_y[1],meas_x[1]]) > 0) & (u[:-1,meas_y[1],meas_x[1]] > -60) & (u[:-1,meas_y[1],meas_x[1]] < -30)
    # pts_end = (np.diff(df1[meas2]) > 0) & df1[meas2].gt(-60)[:-1] & df1[meas2].lt(-30)[:-1] # butini ()
    peaks_begin, _ = find_peaks(pts_begin) # grazina index
    peaks_end, _ = find_peaks(pts_end)
    # print(df1["t"][peaks_begin])
    # print(df1["t"][peaks_end].values-df1["t"][peaks_begin].values)
    # cv = (1*100e-6)/((peaks_end - peaks_begin[:len(peaks_end)])/1000) * 100 # *100 m/s->cm/s
    t1 = peaks_begin*0.2
    t2 = peaks_end*0.2
    cells_count = meas_x[1] - meas_x[0]
    cv = (cells_count*100e-6) / ( (t2 - t1[:len(t2)]) / 1000) * 100 # / 1000 s -> ms, *100 m/s->cm/s
    # t_delay = (df1["t"][peaks_end].values - df1["t"][peaks_begin][:len(peaks_end)].values)# *100 m/s->cm/s
    if len(peaks_begin) - len(peaks_end) > 1:
        print('Some waves were blocked and didnt reach end, require manual inspection of CV plot!!')
    t_all = t2

    %cd C:\OpenGL\fentonGjOpenGL
    ar = np.ones((len(t_all), 2))
    ar[:,0] = t_all
    ar[:,1] = cv
    np.savetxt("trink_cv-t.csv", ar, delimiter=",")
    
save_CV_t()

#%% save gj-t
def save_gj_t():
    %cd C:\OpenGL\fentonGjOpenGL
    frames_count = u.shape[0]
    t_ = np.multiply(np.arange(frames_count),0.2)[::1000]
    gj_ = gj[::1000,0,22,0]
    ar = np.ones((len(t_), 2))
    ar[:,0] = t_
    ar[:,1] = gj_
    np.savetxt("trink_gj-t.csv", ar, delimiter=",")
save_gj_t()

#%% kalibracine kreive cv-gj
def plot_CV_gj():
    dt_reg = 0.2 #ms
    t_period = 250 # ms
    i_period = int(t_period / dt_reg)
    gj_vals = gj[::i_period,0,22,0]
    cv, _ = getCV()
    cv_vals = cv#[::i_period]
    # print(gj_vals.shape)
    # print(gj_vals[0,:])
    # print(cv_vals.shape)
    %cd C:\OpenGL\fentonGjOpenGL
    ar = np.ones((len(cv_vals),2))
    ar[:,0] = gj_vals
    ar[:,1] = cv_vals
    np.savetxt("trink_cv-gj_kalibracine.csv", ar, delimiter=",")

    return px.scatter(x=gj_vals,y=cv_vals).show()
plot_CV_gj()

#%%
# from mpl_toolkits.mplot3d import axes3d
import matplotlib
# matplotlib.use('Qt5Agg')
%matplotlib qt
import matplotlib.pyplot as plt

fig = plt.figure()
# ax = fig.add_subplot(projection='3d')
ax = fig.add_subplot()

# Grab some test data.
X = [[1,2],[1,2]]
Y = [[1,2],[1,2]]
Z = [[0, 0],[0,0]]

# Plot a basic wireframe.
ax.plot_wireframe(X, Y, Z)
# ax.plot_wireframe(X, Y, Z, rstride=10, cstride=10)

plt.show()

#%%
# Import modules
import numpy as np
import matplotlib.pyplot as plt
import plotly.plotly as py

# Create some data arrays
x = np.linspace(-2.0 * np.pi, 2.0 * np.pi, 51)
y = np.sin(x)

# Make a plot
mpl_fig = plt.figure()
plt.plot(x, y, 'ko--')
plt.title('sin(x) from -2*pi to 2*pi')
plt.xlabel('x')
plt.ylabel('sin(x)')
plt.show()

# Export plot to plotly
unique_url = py.plot_mpl(mpl_fig, filename="sin(x) test plot")
unique_url