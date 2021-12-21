# from scipy.interpolate import interp1d
import csv
import numpy as np
from numpy.lib.function_base import disp
import pandas as pd
import matplotlib.pyplot as plt
from scipy.signal import find_peaks
# import mpld3

# plt.style.use('dark_background')

# dt = 1  # experiment dt, ms
print("\n\n**Starting\n")
path1 = r"reg.csv"
df1 = pd.read_csv(path1, delimiter=",")
# df1.dropna(axis = 0, how = 'any', inplace = True)

# fig,ax = plt.subplots(2)
# ax[0].plot(df1["vj"])
# ax[1].plot(df2["gj"])
# plt.show()

# # kai u - normalizuotas
# Vfi = 15e-3  # nernst potential of fast gate
# V0 = -85e-3
# df1["v1"] = (df1["v1"] * (Vfi-V0) + V0) * 1e3
# df1["v2"] = (df1["v2"] * (Vfi-V0) + V0) * 1e3
# df1["v_end"] = (df1["v_end"] * (Vfi-V0) + V0) * 1e3
# df1["vj"] = df1["v2"] - df1["v1"]

# kai u - ne normalizuotas
df1["vj"] = df1["v3"] - df1["v4"]

# df1 = df1[df1["t"] < 3000]
# t = df1["t"][df1["t"] < 10000]

# gj_vj_t = df1[["t", "vj", "gj"]]

# gj_vj_t.to_csv(r"C:\OpenGL\simGJ\vjFiles\vjTest1.csv", index=False)

# df1["vj"] = vj
# dff = df1[df1['t'].between(0, 7500)] #select time
# dff[["t","vj"]].to_csv("vj.csv", index=False) #select colums
# exit()

# // !!!!!!!!! Visa Registracija vyksta kai kvieciam Draw, t.y. sampling rate 10x leciau nei compute !!!!!
fig, (axVjGj,axPeaks, axCV) = plt.subplots(3,1, sharex=True)
axVjGj.plot(df1["t"], df1["v3"], 'b--o')
axVjGj.plot(df1["t"], df1["v4"], 'r--o')
axVjGj.plot(df1["t"], df1["vj"], 'g.--', linewidth=2, markersize=8)
# axVjGj.plot(df1["t"], df1["v1"], 'c--') #for CV
# axVjGj.plot(df1["t"], df1["v4"], 'y--')
axVjGj.set_xlabel("time, ms")
axVjGj.set_ylabel(r'$V$, mV')
axVjGj.legend([r'$V_1$', r'$V_2$', r'$V_j$', r'$V_{begin}$', r'$V_{end}$', r'$gj$'], loc='center left')

ax2 = axVjGj.twinx()
ax2.set_ylabel(r'$g_j$')
# ax2.plot(df1["t"], df1["gj1"], 'k.--', markersize=8)
# ax2.plot(df1["t"], df1["gj2"], 'b.--', markersize=8)
ax2.plot(df1["t"], df1["gj1"], 'b.--', markersize=8)
# ax2.plot(df1["t"], df1["gj_reg2"], 'kv--', markersize=2)
ax2.legend([r'$gj1$',r'$gj2$'], loc='right')
# ax2.legend([r'$gj_{reg2}$'], loc='lower right')

#calculate CV, from v_begin and v_end peaks: (veikia tik kai nera "kas antro" impulso perdavimo)
# meas1 = 'v_begin'
# meas2 = 'v_end'
meas1 = 'v2'
meas2 = 'v6'
# meas1 = 'v1'
# meas2 = 'v2'
pts_begin = (np.diff(df1[meas1]) > 0) & df1[meas1].gt(-60)[:-1] & df1[meas1].lt(-30)[:-1] # butini (), bool[] - kai diff(isvestine) > 0 ir pati reiksme > 0mV (kad nufiltruoti pasvyravimus)
pts_end = (np.diff(df1[meas2]) > 0) & df1[meas2].gt(-60)[:-1] & df1[meas2].lt(-30)[:-1] # butini ()
peaks_begin, _ = find_peaks(pts_begin) # randam visas t reiksmes bool[]
peaks_end, _ = find_peaks(pts_end)
# print(df1["t"][peaks_begin])
# print(df1["t"][peaks_end].values-df1["t"][peaks_begin].values)
# cv = (1*100e-6)/((peaks_end - peaks_begin[:len(peaks_end)])/1000) * 100 # *100 m/s->cm/s
cv = (25*100e-6) / ( (df1["t"][peaks_end].values - df1["t"][peaks_begin][:len(peaks_end)].values) / 1000) * 100 # / 1000 s -> ms, *100 m/s->cm/s
# t_delay = (df1["t"][peaks_end].values - df1["t"][peaks_begin][:len(peaks_end)].values)# *100 m/s->cm/s
if len(peaks_begin) - len(peaks_end) > 1:
    print('Some waves were blocked and didnt reach end, require manual inspection of CV plot!!')

# periods = np.diff(df1["t"][:-1][pts_begin]) #get periods
# print(periods)

axCV.plot(df1["t"][peaks_end].values, cv, 'o--')
axCV.set_ylabel(r'$CV$, cm/s')
# axCV.set_xlim(0, df1.t.max())
# axCV.plot(periods, 'o--')


axPeaks.plot(df1['t'].values, df1[meas1], "--")
axPeaks_1 = axPeaks.twinx()
# axPeaks_1.plot(df1['t'][:-1][pts_begin].values,pts_begin)
# axPeaks_1.plot(df1['t'][:-1][pts_end].values,pts_begin)
axPeaks.plot(df1['t'][peaks_begin].values, df1[meas1][peaks_begin], "o")
axPeaks.plot(df1['t'], df1[meas2], "--")
axPeaks.plot(df1['t'][peaks_end].values, df1[meas2][peaks_end], "x")


# mpld3.display(fig)
plt.show()
# plt.ion()
