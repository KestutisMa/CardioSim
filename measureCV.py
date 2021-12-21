#%% imports
%%time
from os import name
import holoviews
from holoviews import opts
import numpy as np
import pandas as pd
import pyarrow

import panel as pn

from bokeh.plotting import figure
import plotly.graph_objs as go

import holoviews as hv

# import glob
import os, fnmatch

#%%
Vfi = 15e-3  # nernst potential of fast gate
V0 = -85e-3
path = r"C:\OpenGL\fentonGjOpenGL\CV many impulses" + '\\'
file_names = fnmatch.filter(os.listdir(path), 'vj@gj*.csv')
gj_all = []
for file_name in file_names:
    gj_all.append(int(file_name.split('vj@gj',1)[1].split('.csv',1)[0]))
gj_all.sort()
# gj_all = range(30, 131, 10)
# file_names = glob.glob(path+r'gj@vj*.csv')
# df = vaex.from_arrays(gj=[0,1])
df_first = pd.DataFrame()
df_end = pd.DataFrame()
for gj_iter in gj_all:
    fileName = r'vj@gj' + str(gj_iter) + r'.csv'
    # print(fileName)
    # print(path + fileName)
    df_tmp = pd.read_csv( path + fileName)
    # print(df_tmp)
    # df_tmp = vaex.from_arrow_table( csv.read_csv(path + fileName))
    df_first['vj@gj'+str(gj_iter)] = df_tmp['v1']
    df_end['vj@gj'+str(gj_iter)] = df_tmp['v_end']
    # df['gj'+str(gj_iter)] = (df_tmp['v1']* (Vfi-V0) + V0) * 1e3
cols = df_end.columns
#%%

dt = 0.0002 #s
df_first['t'] = np.arange(0,df_first.count().max()*dt,dt)
df_end['t'] = np.arange(0,df_end.count().max()*dt,dt)


# dataset = hv.Dataset(df)
# myplot =  hv.Scatter(dataset, label=c, kdims=["t"], vdims=[cols[0]])
# for c in cols[1:-1]:
#     myplot *= hv.Scatter(dataset, label=c, kdims=["t"], vdims=[c])
# myplot = myplot.opts(bgcolor='white', width=1000, height=600)

# delays plot
delays_plots = []
for colName in cols[:]:
    delays_plots.append( go.Scattergl(x=df_end.t, y=df_end[colName], name=colName, mode='markers+lines') )

delays_layout = go.Layout(
    title="delays @ gjs",
    autosize=False,    
    width=1200,
    height=800,
    margin=dict(t=50, b=50, r=50, l=50),
    xaxis_title='t, s',
    yaxis_title='V<sub>j</sub>',
    # showlegend=False,    
)
delays_fig = go.Figure(data=delays_plots, layout=delays_layout)
# delays_fig = go.Figure(data=[go.Scattergl(x=[1,2,3], y=[1,2,3], name='hi', mode='markers')] )
# delays_plotly_pane = pn.pane.Plotly(go.Scattergl(x=[1,2,3], y=[1,2,3], name='hi', mode='markers'))
delays_plotly_pane = pn.pane.Plotly(delays_fig)

# CV plot
cvs = [] # CVs
for col_name in cols:
    df_first_signed = np.sign(df_first[col_name] - -60)
    t_crossings_first = df_first['t'][ df_first_signed.diff().gt(0)] # t values at which vj crosses -60 (wave fronts)
    t_crossings_first = np.array(t_crossings_first)
    df_end_signed = np.sign(df_end[col_name] - -60)
    t_crossings_end = df_end['t'][ df_end_signed.diff().gt(0)] # t values at which vj crosses -60 (wave fronts)
    t_crossings_end = np.array(t_crossings_end)
    # cv = df.loc[ df[gj] > -40]['t'].iloc[0]
    # t1 = # v_1 TODO
    # t2 = df_end.loc[ (df_end[col_name] - -60).diff().ne() ]['t'] #v_end
    # t2 = df_end.loc[ (df_end[col_name] - -60).diff().ne() > -60]['t'] #v_end
    time = t_crossings_end[3] - t_crossings_first[3]
    print(col_name, 'first: ', t_crossings_first[3], ' end: ', t_crossings_end[3], ' time', time )
    cv = (32-4) * 100e-6 / time * 100 # m -> cm, -4 nes matuojam toliau nuo krasto
    cvs.append(cv)
df_cv = pd.DataFrame(dict(gj=gj_all, CV=cvs))
# for colName in cols[0:-1]:
cv_plot = go.Scattergl(x=df_cv.gj, y=df_cv['CV'], name='CV', mode='markers+lines')
cv_layout = go.Layout(
    title="CV @ g<sub>j</sub>'s",
    autosize=False,    
    width=800,
    height=600,
    margin=dict(t=50, b=50, r=50, l=50),
    xaxis_title='g<sub>j</sub>, nS',
    yaxis_title='CV, cm/s',
    showlegend=False,    
)

cv_fig = go.Figure(data=cv_plot, layout=cv_layout)
cv_plotly_pane = pn.pane.Plotly(cv_fig)

#testing
#test callback
# def callback(target, event):
#     print('aaaaaaaaaa')
#     target.object = 'event'
# delays_plotly_pane.link(m, callbacks={'click_data': callback})

# BOKEH server
try:
    bokeh_srv.stop()
    bokeh_srv.join()
except:
    print('bokeh not running')
else:
    print('stopped bokeh server succsfully')

print('starting bokeh server')

# pn.extension(comms='vscode') #reikalingas tik jupyter inline,holoviz
hv.extension('plotly')
# hv.extension('bokeh') 
m = pn.pane.Markdown("hi")
col = pn.Column(delays_plotly_pane, cv_plotly_pane, m) 
# row = pn.Row(myplot)
bokeh_srv = col.show(threaded=True)
