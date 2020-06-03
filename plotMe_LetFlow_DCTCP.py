#!/usr/bin/python


import matplotlib.pyplot as plt
import numpy as np


# fig, axs = plt.subplots(nrows=1, ncols=2)
fig, axs = plt.subplots(nrows=1, ncols=1)

# # 10Gbps, FTO(Flowlet Timout)=500us
# # x axis values
# x = [10,20,30,40,50,60,70,80,90]
# # y axis values
# y = [0.7,1.9,1.1,4,5.4,7.9,11.9,17.9,18.3]
# axs.plot(x, y, 'tab:blue',linestyle='-', linewidth = 1, marker='o', markersize=5, label='10Gbps, FTO=50us')

# # 1Gbps, FTO(Flowlet Timout)=500us
# # x axis values
# x = [10,20,30,40,50,60,70,80,90]
# # y axis values
# y = [0.8,1.6,4.4,7.6,9.2,10.7,10,10.3,11.5]
# axs.plot(x, y, 'tab:orange', linestyle='-', linewidth = 1, marker='x',  markersize=5, label='1Gbps, FTO=50us')


# 10Gbps, FTO(Flowlet Timout)=150us
# x axis values
x = [60,70,80,90]
# y axis values
y = [8.3,10.2,18.8,25.3]
# 10Gbps, FTO(Flowlet Timout)=150us, NEW_LOGGER
axs.plot(x, y,'tab:purple', linestyle='-', linewidth = 1, marker='s', markersize=5, label='10Gbps, FTO=150us')

# 1Gbps, FTO(Flowlet Timout)=150us
# x axis values
x = [10,20,30,40,50,60,70,80,90]
# y axis values
y = [0.5,1.2,3,4.8,5.8,9.4,11.9,13.1,14.9]
axs.plot(x, y,'tab:olive', linestyle='-', linewidth = 1, marker='D', markersize=5, label='1Gbps, FTO=150us')


########################################
#  NEW LOGGER
# 10Gbps, FTO(Flowlet Timout)=150us, NEW LOGGER
# x axis values
x = [60,70,80,90]
# y axis values
y = [3.3,4.7,6.2,8.5]
# 10Gbps, FTO(Flowlet Timout)=150us, NEW_LOGGER
axs.plot(x, y,'tab:red', linestyle='dotted', linewidth = 1, marker='2', markersize=5, label='10Gbps, FTO=150us, NEW_LOGGER')

# 1Gbps, FTO(Flowlet Timout)=150us
# x axis values
x = [60,70,80,90]
# y axis values
y = [5.1,5.6,5.8,6.1]
axs.plot(x, y,'tab:pink', linestyle='dotted', linewidth = 1, marker='3', markersize=5, label='1Gbps, FTO=150us, NEW_LOGGER')




axs.grid()
axs.set(xlabel='Load %', ylabel='Reroute to Congestion (%)')
axs.legend()
# for ax in axs.flat:
	# ax.set(xlabel='Load %', ylabel='Reroute to Congestion (%)')
# Hide x labels and tick labels for top plots and y ticks for right plots.
# for ax in axs.flat:
#     ax.label_outer()


# # x axis values
# x = [10,20,30,40,50,60,70,80,90]
# # y axis values
# y = [0,0,0,7,0,13,0,31,0]

# # plotting the points 
# plt.plot(x, y, color='green', linestyle='-', linewidth = 1,
#          marker='o', markerfacecolor='black', markersize=8)
# #Set the y-limits of the current axes.
# plt.ylim(1,max(y)+5)
# #Set the x-limits of the current axes.
# plt.xlim(1,max(x)+10)

# # naming the x axis
# plt.xlabel('Load %')
# # naming the y axis
# plt.ylabel('To congested paths')

plt.xticks(np.arange(0, 100, 10))
# # giving a title to my graph
plt.title('LetFlow, Asym. Topology, Web Search')
# function to show the plot
# fig.tight_layout()
plt.savefig('LetFlow-AsymTopo-DCTCP.pdf', bbox_inches='tight')
plt.show()
