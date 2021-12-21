#%%
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
 
%matplotlib qt 
# matplotlib.use('Qt5Agg')  

plt.rc('font', family='serif')
plt.rc('xtick', labelsize='medium')
plt.rc('ytick', labelsize='medium')
plt.rc('ytick', labelsize='medium')
plt.rc('ytick', labelsize='medium')
plt.rc('lines', marker='o')
plt.rc('lines', linestyle='--')
# mpl.rcParams['lines.linestyle'] = '--'

cm = 1/2.54  # centimeters in inches
fig = plt.figure(figsize=(15*cm, 15*cm))
ax = fig.add_subplot(1, 1, 1)

pulse_rate = np.array([80, 120, 240])
cv_cx4345 = np.array([2.5, 2.3, 1.8]) # gj_max = 22nS initial (statosi i modeli)
cv_cx4345_const = np.array([2.95, 2.95, 2.95]) # gj_max = 22nS initial (statosi i modeli), cia gj paskaiciuotas steady-state @ t=0
cv_cx43 = np.array([6.25, 6.25, 6.25]) # su modeliu beveik nesiskiria nuo be (vj gating beveik nera)
legend_labels = ['Cx43-45', 'Cx43-45-non-gated', 'Cx43']
# ax.plot(pulse_rate, cv_cx4345, color='k', ls='solid')
# ax.plot(pulse_rate, cv_cx4345_const, color='0.50', ls='dashed')
# ax.plot(pulse_rate, cv_cx43, color='0.50', ls='solid')
ax.plot(pulse_rate, cv_cx4345, )
ax.plot(pulse_rate, cv_cx4345_const)
ax.plot(pulse_rate, cv_cx43)
ax.legend(bbox_to_anchor=(1, 1), loc=1, frameon=False, fontsize=16, labels=legend_labels, 
        prop={'size':'medium', 'family':'serif'})
ax.set_xlabel('Pulse rate, beats per minute')
ax.set_ylabel('Conduction velocity, cm/s')
plt.show()
# %%
