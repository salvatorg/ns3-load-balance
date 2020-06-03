#!/usr/bin/python


import sys
import csv

thr_red = 91000

file=sys.argv[1]
if(len(sys.argv)==1):
	print "::ERROR::Give a input file"
	exit

QeuesExceedThr=0
tot_cnt=0
lines=0
s=''
cong_paths=[]
cong_paths_dict=dict()

l_s=[]
s_l=[]



# CSV format for the utilisation queues of Leafves, Spines
# for i,j
# Q-Leaf[i]-Spine[j], Q-Spine[j]-Leaf[i]
with open(file) as csvfile:
	spamreader = csv.reader(csvfile, delimiter='\t')
	for row in spamreader:
		time_s = row[0]
		# print len(time_s)
		if len(time_s)==3:
			time_s = time_s+'0'
		if len(time_s)==1:
			time_s = time_s+'.00'

		time = float(time_s[0:4])

		row=row[1:-1]
		for el in range(0,len(row)):
			if row[el].isdigit():
				if(int(row[el])>thr_red):
					if (el%2==0): 	# its leaf
						# print"\tInQueueBytes: "+row[el]+"\tNode: Leaf_"+str(el/16)+"-Spine_"+str(((el-((el/16)*16))/2))
						l_s.append([el/16, ((el-((el/16)*16))/2)])
					else:			# its spine
						# print"\tInQueueBytes: "+row[el]+"\tNode: Spine_"+str(((el-((el/16)*16))/2))+"-Leaf_"+str(el/16)
						s_l.append([((el-((el/16)*16))/2),el/16])
					QeuesExceedThr=QeuesExceedThr+1

		# Combine the paths to a generic form
		complet = 0
		if len(l_s) > 0 and len(s_l) > 0 :

			for ls in range(len(l_s)):
				for sl in range(len(s_l)):
					# common spine
					if s_l[sl][0]==l_s[ls][1]:
						cong_paths.append([l_s[ls][0],l_s[ls][1],s_l[sl][1]])
						complet = 1
				if complet==0:
					cong_paths.append([l_s[ls][0],l_s[ls][1],'*'])
				complet = 0
	
			complet = 0
			for sl in range(len(s_l)):
				for ls in range(len(l_s)):
					# common spine
					if s_l[sl][0]==l_s[ls][1]:
						complet = 1

				if complet==0:
					cong_paths.append(['*',s_l[sl][0],s_l[sl][1]])
				complet = 0

		if len(l_s) == 0:
			for sl in range(len(s_l)):
				cong_paths.append(['*',s_l[sl][0],s_l[sl][1]])

		if len(s_l) == 0:
			for ls in range(len(l_s)):
					cong_paths.append([l_s[ls][0],l_s[ls][1],'*'])

		#print str(time)[0:4]
		cong_paths_dict[time_s] = cong_paths
		# print "ADD cong_paths_dict", time_s
		cong_paths=[]
		l_s=[]
		s_l=[]


		# if(cnt==0): print("\t-")
		# print("Time\t: %.2f,\toccurences\t%d" %(time,cnt))
		tot_cnt=tot_cnt+QeuesExceedThr
		cnt=0
		if (lines==-1): 
			break
		lines=lines+1

print "Total Num of Queues exceed THR\t: ",tot_cnt
# print "Lines is\t: ",lines
print "Threshold\t: ",thr_red

{k: v for k, v in sorted(cong_paths_dict.items(), key=lambda item: item[1])}

altered_paths=0
con_path_0ms=0
con_path_1ms=0
con_path_2ms=0

with open("alt-paths.txt") as csvfile:
	spamreader = csv.reader(csvfile, delimiter=' ')
	for row in spamreader:
		s_tor = int(row[2][1])
		d_tor = int(row[4][0])
		spine = int(row[7][1:3]) - 17
		time = float(row[9][1:-2]) / 1000000000
		time_s = str(time)



		queue_util = cong_paths_dict[str(time)[0:4]]
		for i in queue_util:
			if(i[0] == s_tor or i[2] == d_tor) and i[1] == spine:
				# print "FOOUND +0ms"
				con_path_0ms=con_path_0ms+1
				break

		queue_util = cong_paths_dict[str(time+0.01)[0:4]]
		for i in queue_util:
			if(i[0] == s_tor or i[2] == d_tor) and i[1] == spine:
				# print "FOOUND +1ms"
				con_path_1ms=con_path_1ms+1
				break

		queue_util = cong_paths_dict[str(time+0.02)[0:4]]
		for i in queue_util:
			if(i[0] == s_tor or i[2] == d_tor) and i[1] == spine:
				# print "FOOUND +2ms"
				con_path_2ms=con_path_2ms+1
				break

		# print '-'*10		
		# print s_tor,spine,d_tor,time,queue_util
		altered_paths = altered_paths + 1
	
	print "Num. altered paths : ", altered_paths
	print "Num. altered paths follow congestion path +0ms : ",con_path_0ms
	print "Num. altered paths follow congestion path +1ms : ",con_path_1ms
	print "Num. altered paths follow congestion path +2ms : ",con_path_2ms
