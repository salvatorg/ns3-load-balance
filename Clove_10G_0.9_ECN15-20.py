import numpy as np
import matplotlib.pyplot as plt
from numpy import genfromtxt
import sys

fig, (ax1, ax2) = plt.subplots(1, 2)

my_data = genfromtxt('Clove_10G_0.9_ECN15.csv', delimiter=' ', skip_header=1)
data=np.array(my_data).transpose()

# A = np.array([5., 30., 45., 22.])
# B = np.array([5., 25., 50., 20.])
# C = np.array([1.,  2.,  1.,  1.])
X = np.arange(128)

proportion_good	= np.true_divide(data[1], data[0]) * 100
proportion_bad	= np.true_divide(data[2], data[0]) * 100
proportion_demi	= np.true_divide(data[3], data[0]) * 100

# plt.bar(X, data[1], color = 'green', label='Good')
# plt.bar(X, data[2], color = 'red', label='Bad')
# plt.bar(X, data[3], color = 'orange', label='Demi')

ax1.bar(X, proportion_good, color = 'green', label='Good',bottom=proportion_bad+proportion_demi, width=1.0)
ax1.bar(X, proportion_bad, color = 'red', label='Bad',bottom=proportion_demi, width=1.0)
ax1.bar(X, proportion_demi, color = 'orange', label='Demi', width=1.0)
ax1.set_title('ECN15')
ax1.set_xlabel("Server IDs")
ax1.set_ylabel("Decisions %")
ax1.legend(loc="upper right")
ax1.grid(axis='y')
# plt.gca().yaxis.grid(True)



my_data = genfromtxt('Clove_10G_0.9_ECN20.csv', delimiter=' ', skip_header=1)
data=np.array(my_data).transpose()

X = np.arange(128)

proportion_good	= np.true_divide(data[1], data[0]) * 100
proportion_bad	= np.true_divide(data[2], data[0]) * 100
proportion_demi	= np.true_divide(data[3], data[0]) * 100

# plt.bar(X, data[1], color = 'green', label='Good')
# plt.bar(X, data[2], color = 'red', label='Bad')
# plt.bar(X, data[3], color = 'orange', label='Demi')

ax2.bar(X, proportion_good, color = 'green', label='Good',bottom=proportion_bad+proportion_demi, width=1.0)
ax2.bar(X, proportion_bad, color = 'red', label='Bad',bottom=proportion_demi, width=1.0)
ax2.bar(X, proportion_demi, color = 'orange', label='Demi', width=1.0)
ax2.set_title('ECN20')
ax2.set_xlabel("Server IDs")
# ax2.set_ylabel("Decisions %")
ax2.legend(loc="upper right")
ax2.grid(axis='y')

# ax2.gca().yaxis.grid(True)
# plt.xticks(X, X)
# plt.gca().yaxis.grid(True)


fig.suptitle("Clove, 10Gbps, L90%, Asym20%, RTT120, B100")
plt.savefig("Clove_10G_0.9_ECN15-20.pdf")

# plt.show()