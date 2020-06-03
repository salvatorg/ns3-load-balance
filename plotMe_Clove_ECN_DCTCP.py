#!/usr/bin/python


import matplotlib.pyplot as plt
import numpy as np


# fig, axs = plt.subplots(nrows=1, ncols=2)
fig, axs = plt.subplots(nrows=1, ncols=2)

# fig.suptitle('Clove-ECN, Asym. Topology, DCTCP')

# # 10Gbps, FTO(Flowlet Timout)=500us
# # x axis values
# x = [10,20,30,40,50,60,70,80,90]
# # y axis values
# y = [1.7,3.9,6.2,8,10.3,13.2,16.7,31.9,48.8]
# # axs[0].plot(x, y, linestyle='-', linewidth = 1, marker='o', markerfacecolor='black', markersize=8, label='10Gbps')
# # axs[0].set_title('10Gbps') # Axis [0, 0]
# # axs[0].grid()
# axs.plot(x, y, 'tab:blue',linestyle='-', linewidth = 1, marker='o', markersize=5, label='10Gbps, FTO=500us')

# # 1Gbps, FTO(Flowlet Timout)=500us
# # x axis values
# x = [10,20,30,40,50,60,70,80,90]
# # y axis values
# y = [1,2,5,8,12,14,25.9,25.3,31]
# # axs[1].plot(x, y, 'tab:orange', linestyle='-', linewidth = 1, marker='x', markerfacecolor='black', markersize=8, label='100Mbps')
# # axs[1].set_title('100Mbps') # Axis [0, 1]
# # axs[1].grid()
# axs.plot(x, y, 'tab:orange', linestyle='-', linewidth = 1, marker='x',  markersize=5, label='1Gbps, FTO=500us')


# # 10Gbps, FTO(Flowlet Timout)=150us
# # x axis values
# x = [10,20,30,40,50,60,70,80,90]
# # y axis values
# y = [1.2,3.2,5.3,7.3,9.4,12.8,17.9,27.7,34.8] # old log system
# # y = [6.3,7.8,10.6,12.1] # new log system
# # axs[0].plot(x, y, linestyle='-', linewidth = 1, marker='o', markerfacecolor='black', markersize=8, label='10Gbps')
# # axs[0].set_title('10Gbps') # Axis [0, 0]
# # axs[0].grid()
# axs.plot(x, y,'tab:purple', linestyle='-', linewidth = 1, marker='s', markersize=5, label='10Gbps, FTO=150us')

# # 1Gbps, FTO(Flowlet Timout)=150us
# # x axis values
# x = [10,20,30,40,50,60,70,80,90]
# # y axis values
# y = [1,2.6,6.4,9.9,13.6,16.8,21,23.3,26]
# # axs[0].plot(x, y, linestyle='-', linewidth = 1, marker='o', markerfacecolor='black', markersize=8, label='10Gbps')
# # axs[0].set_title('10Gbps') # Axis [0, 0]
# # axs[0].grid()
# axs.plot(x, y,'tab:olive', linestyle='-', linewidth = 1, marker='D', markersize=5, label='1Gbps, FTO=150us')


# ########################################
# #  NEW LOGGER
# # 10Gbps, FTO(Flowlet Timout)=150us
# # x axis values
# x = [10,20,30,40,50,60,70,80,90]
# # y axis values
# y = [5.9,7.2,9.6,10.7,11.9,13.3,15.4,18.2,22] # new log system
# # axs[0].plot(x, y, linestyle='-', linewidth = 1, marker='o', markerfacecolor='black', markersize=8, label='10Gbps')
# # axs[0].set_title('10Gbps') # Axis [0, 0]
# # axs[0].grid()
# axs.plot(x, y,'tab:red', linestyle='dotted', linewidth = 1, marker='2', markersize=5, label='10Gbps, FTO=150us NEW_LOGGER')
# #  NEW LOGGER
# # 1Gbps, FTO(Flowlet Timout)=150us
# # x axis values
# x = [10,20,30,40,50,60,70,80,90]
# # y axis values
# y = [7.7,9,11.6,13.3,15,16.3,17.4,18.3,19.4] 
# # axs[0].plot(x, y, linestyle='-', linewidth = 1, marker='o', markerfacecolor='black', markersize=8, label='10Gbps')
# # axs[0].set_title('10Gbps') # Axis [0, 0]
# # axs[0].grid()
# axs.plot(x, y,'tab:pink', linestyle='dotted', linewidth = 1, marker='3', markersize=5, label='1Gbps, FTO=150us NEW_LOGGER')



########################################
#  NEW LOGGER v2
# 10Gbps, FTO(Flowlet Timout)=150us
# x axis values
x = [60,70,80,90]
# y axis values
y = [13.5,18.8,28.2,34.8] # new log system
# axs[0].plot(x, y, linestyle='-', linewidth = 1, marker='o', markerfacecolor='black', markersize=8, label='10Gbps')
# axs[0].set_title('10Gbps') # Axis [0, 0]
# axs[0].grid()
axs[0].plot(x, y,'tab:green', linestyle='-', linewidth = 1, marker='o', markersize=5, label='Both')
x = [60,70,80,90]
# y axis values
y = [11.2,15.9,24.8,30.3] # new log system
# axs[0].plot(x, y, linestyle='-', linewidth = 1, marker='o', markerfacecolor='black', markersize=8, label='10Gbps')
# axs[0].set_title('10Gbps') # Axis [0, 0]
# axs[0].grid()
axs[0].plot(x, y,'tab:olive', linestyle='-', linewidth = 1, marker='x', markersize=5, label='new_path!=old_path')
x = [60,70,80,90]
# y axis values
y = [2.3,2.9,3.9,4.5] # new log system
# axs[0].plot(x, y, linestyle='-', linewidth = 1, marker='o', markerfacecolor='black', markersize=8, label='10Gbps')
# axs[0].set_title('10Gbps') # Axis [0, 0]
# axs[0].grid()
axs[0].plot(x, y,'tab:gray', linestyle='-', linewidth = 1, marker='s', markersize=5, label='new_path==old_path')
axs[0].set_title('10Gbps Links')

########################################
#  NEW LOGGER v2
# 1Gbps, FTO(Flowlet Timout)=150us
# x axis values
x = [10,20,30,40,50,60,70,80,90]
# y axis values
y = [1.9,3.6,7.7,11.4,14.4,18.2,22.4,24.8,27.8] 
axs[1].plot(x, y,'tab:orange', linestyle='dotted', linewidth = 1, marker='2', markersize=5, label='Both')
# 1Gbps, FTO(Flowlet Timout)=150us
# x axis values
x = [10,20,30,40,50,60,70,80,90]
# y axis values
y = [1.3,2.5,5.9,8.9,11.6,15.4,18.3,20.2,22.5] 
axs[1].plot(x, y,'tab:pink', linestyle='dotted', linewidth = 1, marker='3', markersize=5, label='new_path!=old_path')
# 1Gbps, FTO(Flowlet Timout)=150us
# x axis values
x = [10,20,30,40,50,60,70,80,90]
# y axis values
y = [0.6,1.1,1.8,2.4,2.7,3,4,4.5,5.2] 
axs[1].plot(x, y,'tab:purple', linestyle='dotted', linewidth = 1, marker='4', markersize=5, label='3new_path==old_path')
axs[1].set_title('1Gbps Links')



axs[0].grid()
axs[1].grid()
# axs.set(xlabel='Load %', ylabel='Reroute to Congestion (%)')
# axs.legend()
for ax in axs.flat:
	ax.set(xlabel='Load %', ylabel='Reroute to Congestion (%)')
	ax.legend()
# Hide x labels and tick labels for top plots and y ticks for right plots.
# for ax in axs.flat:
#     ax.label_outer()

fig.suptitle('Clove-ECN, Asym. Topology, Web Search, FTO=150us')

plt.xticks(np.arange(0, 100, 10))
# # giving a title to my graph
# plt.title('Clove-ECN, Asym. Topology, Web Search')
# function to show the plot
# fig.tight_layout()
plt.savefig('Clove-ECN-AsymTopo-DCTCP.pdf', bbox_inches='tight')
plt.show()
