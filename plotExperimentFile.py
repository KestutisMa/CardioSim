# from scipy.interpolate import interp1d
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

import csv
dt = 1 # experiment dt, ms
print("\n\n**Starting\n")
path1 = r'C:\Users\Kestutis\OneDrive - Lietuvos sveikatos mokslu universitetas\is Dropbox\Programming\gjJulia\cx43_cardio_t_vj.csv'
df1 = pd.read_csv(path1, delimiter="\t")
df1.dropna(axis = 0, how = 'any', inplace = True) 

# fig,ax = plt.subplots(2)
# ax[0].plot(df1["vj"])
# ax[1].plot(df2["gj"])
# plt.show()


df1["t_vj"] *= 1e3
#df1["vj"] *= (Vfi-V0)  * 1e3

fig,ax1 = plt.subplots()
ax1.plot(df1["t_vj"], df1["vj"],'b-')
ax1.set_xlabel("time, ms")
ax1.set_ylabel(r'$V$, mV')
plt.legend([r'$V_j$'])

#ax2 = ax1.twinx()
#ax2.set_ylabel(r'$V_j$')
plt.show()
#plt.ion()