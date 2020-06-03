#!/usr/bin/python


import matplotlib.pyplot as plt
import numpy as np


# fig, axs = plt.subplots(nrows=1, ncols=2)
fig, axs = plt.subplots(nrows=1, ncols=1)


# 10Gbps, 500us
# x axis values
x = [10,20,30,40,50,60,70,80,90]
# y axis values
y = [0.9,0.9,2.7,3.3,3.4,4.6,5.6,6.5,5.8]
# axs[0].plot(x, y, linestyle='-', linewidth = 1, marker='o', markerfacecolor='black', markersize=8, label='10Gbps')
# axs[0].set_title('10Gbps') # Axis [0, 0]
# axs[0].grid()
axs.plot(x, y, 'tab:blue',linestyle='-', linewidth = 1, marker='o', markersize=5, label='10Gbps, 500us')

# 1Gbps, 500us
# x axis values
x = [10,20,30,40,50,60,70,80,90]
# y axis values
y = [0,1.6,2.2,3,4.3,4,1.4,2.4,3.9]
# axs[1].plot(x, y, 'tab:orange', linestyle='-', linewidth = 1, marker='x', markerfacecolor='black', markersize=8, label='100Mbps')
# axs[1].set_title('100Mbps') # Axis [0, 1]
# axs[1].grid()
axs.plot(x, y, 'tab:orange', linestyle='-', linewidth = 1, marker='x', markersize=5, label='1Gbps, 500us')


# # axs[1, 0].plot(x, -y, 'tab:green')
# # axs[1, 0].set_title('') # Axis [1, 0]
# # axs[1, 1].plot(x, -y, 'tab:red')
# # axs[1, 1].set_title('') # Axis [1, 1]


# 10Gbps, 150us
# x axis values
x = [10,20,30,40,50,60,70,80,90]
# y axis values
y = [0.3,1.4,2.1,2.4,3.3,3.6,5.7,6,6.2]
# axs[0].plot(x, y, linestyle='-', linewidth = 1, marker='o', markerfacecolor='black', markersize=8, label='10Gbps')
# axs[0].set_title('10Gbps') # Axis [0, 0]
# axs[0].grid()
axs.plot(x, y, 'tab:purple',linestyle='-', linewidth = 1, marker='s', markersize=5, label='10Gbps, 150us')

# 1Gbps
# x axis values
x = [10,20,30,40,50,60,70,80,90]
# y axis values
y = [0,0.8,0.9,1.4,1.5,4.1,3.4,3.1,5]
# axs[1].plot(x, y, 'tab:orange', linestyle='-', linewidth = 1, marker='x', markerfacecolor='black', markersize=8, label='100Mbps')
# axs[1].set_title('100Mbps') # Axis [0, 1]
# axs[1].grid()
axs.plot(x, y, 'tab:olive', linestyle='-', linewidth = 1, marker='D', markersize=5, label='1Gbps, 150us')




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
plt.title('Clove-ECN, Asym. Topology, Data Mining')
# function to show the plot
# fig.tight_layout()
plt.savefig('Clove-ECN-AsymTopo-VL2.pdf', bbox_inches='tight')
plt.show()
