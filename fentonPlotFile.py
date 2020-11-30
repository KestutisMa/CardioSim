# from scipy.interpolate import interp1d
import csv
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

# plt.style.use('dark_background')

dt = 1  # experiment dt, ms
print("\n\n**Starting\n")
path1 = r"reg.csv"
df1 = pd.read_csv(path1, delimiter=",")
# df1.dropna(axis = 0, how = 'any', inplace = True)

# fig,ax = plt.subplots(2)
# ax[0].plot(df1["vj"])
# ax[1].plot(df2["gj"])
# plt.show()

Vfi = 15e-3  # nernst potential of fast gate
V0 = -85e-3

df1["v1"] = (df1["v1"] * (Vfi-V0) + V0) * 1e3
df1["v2"] = (df1["v2"] * (Vfi-V0) + V0) * 1e3
df1["v_end"] = (df1["v_end"] * (Vfi-V0) + V0) * 1e3
df1["vj"] = df1["v2"] - df1["v1"]
# df1 = df1[df1["t"] < 3000]
# t = df1["t"][df1["t"] < 500]
vj_t = df1[["t", "vj"]]

# vj_t.to_csv(r"C:\OpenGL\simGJ\vjFiles\vjTest.csv", index=False)

# df1["vj"] = vj
# dff = df1[df1['t'].between(0, 7500)] #select time
# dff[["t","vj"]].to_csv("vj.csv", index=False) #select colums
# exit()

# // !!!!!!!!! Visa Registracija vyksta kai kvieciam Draw, t.y. sampling rate 10x leciau nei compute !!!!!
fig, ax1 = plt.subplots()
ax1.plot(df1["t"], df1["v1"], 'b--o')
ax1.plot(df1["t"], df1["v2"], 'r--o')
ax1.plot(df1["t"], df1["vj"], 'g.--', linewidth=2, markersize=8)
ax1.plot(df1["t"], df1["v_end"], 'y--')
ax1.set_xlabel("time, ms")
ax1.set_ylabel(r'$V$, mV')
ax1.legend([r'$V_1$', r'$V_2$', r'$V_j$',
            r'$V_{end}$', r'$gj_{reg}$'], loc='upper right')

ax2 = ax1.twinx()
ax2.set_ylabel(r'$g_j$')
ax2.plot(df1["t"], df1["gj_reg1"], 'gv--', markersize=8)
ax2.plot(df1["t"], df1["gj_reg2"], 'bv--', markersize=2)
ax2.legend([r'$gj_{reg1}$', r'$gj_{reg2}$'], loc='lower right')
# ax2.legend([r'$gj_{reg2}$'], loc='lower right')

plt.show()
# plt.ion()
