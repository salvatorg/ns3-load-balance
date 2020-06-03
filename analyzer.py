#!/usr/bin/python


import sys
import csv
import os

thr_red = 91000 # 65 packets marking threshold
#thr_red = 28000 # 20 packet marking Threshold

basepath=''

if(len(sys.argv)==1):
	print "Give a base path and/or Id number"
	print sys.argv[0], " [ID] [base_path (default: ./)]"
	sys.exit()
elif(len(sys.argv)==2):
	file1=sys.argv[1]+"_Queues_Sizes_on_change.txt"
	file2=sys.argv[1]+"_Flows_path_change.txt"
else:
	file1=sys.argv[2]+"/"+sys.argv[1]+"_Queues_Sizes_on_change.txt"
	file2=sys.argv[2]+"/"+sys.argv[1]+"_Flows_path_change.txt"

if not (os.path.exists(file1) and os.path.exists(file2)):
	print file1, file2
	print "FILES NOT FOUND"
	sys.exit()

print "Processing files\t\t\t: ",file1, file2

QeuesExceedThr=0
tot_cnt=0
lines=0
s=''
cong_paths=[]
cong_paths_dict=dict()
queues_timestamps=[]
l_s=[]
s_l=[]

def remove_prefix(text, prefix):
    if text.startswith(prefix):
        return text[len(prefix):]
    return text  # or whatever


# CSV format for the utilisation queues of Leafves, Spines
# Each line decribes the Tx queues as follows
# L0port0-S0port0 L0port1-S1port0 ... L7port0-S7port7
with open(file1) as csvfile:
	logreader = csv.reader(csvfile, delimiter=' ')
	for row in logreader:

		if row[3] not in queues_timestamps:
			queues_timestamps.append(row[3])
		else:
			# We have already check this timestamp with queues utilization file
			continue

		row=row[4:-1] # TODO -1 because there is a space at the end of each line
		row_tmp=""

		# Just checking
		if len(row) != 128:
			print "###\tSomething is fucked up (shit Num.1)\t###"
			sys.exit()

		for el in range(0,len(row)):
			# Just checking
			if not row[el].isdigit():
				print "###\tSomething is fucked up (shit Num.2)\t###"
				sys.exit()
			# overcoming the bad format of tracing logs
			# if el!=0:
			# 	row[el] = remove_prefix(row[el],row_tmp)
			
			row_tmp += row[el]
			# print el,row[el]
			# if row[el].isdigit():
			if(int(row[el])>thr_red):
				if (el%2==0): 	# its leaf
					# print"\tInQueueBytes: "+row[el]+"\tNode: Leaf_"+str(el/16)+"-Spine_"+str(((el-((el/16)*16))/2))
					l_s.append([el/16, ((el-((el/16)*16))/2)])
				else:			# its spine
					# print"\tInQueueBytes: "+row[el]+"\tNode: Spine_"+str(((el-((el/16)*16))/2))+"-Leaf_"+str(el/16)
					s_l.append([((el-((el/16)*16))/2),el/16])
				QeuesExceedThr=QeuesExceedThr+1

		# Combine the paths to a generic form, e.g. [s_tor, spine, d_tor] where s_tor, d_tor can * if a complete path was not found
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

		# Clasify it based on time
		cong_paths_dict[queues_timestamps[-1]] = cong_paths
		cong_paths=[]
		l_s=[]
		s_l=[]

		lines = lines + 1

# print queues_timestamps,cong_paths_dict

print "Total Num. of Timestamps analyzed\t: ",lines
# print "Lines is\t: ",lines
print "Threshold\t\t\t\t: ", thr_red, "Bytes"



# {k: v for k, v in sorted(cong_paths_dict.items(), key=lambda item: item[1])}

# print cong_paths_dict.keys()

alt_tot_paths=0
alt_cong_path=0
alt_path = ""
found_cong=0
flows_alt_cong_paths = dict()

with open(file2) as csvfile:
	logreader = csv.reader(csvfile, delimiter=' ')
	for row in logreader:
		flow_id = int(row[1])
		s_tor = int(row[2][1])
		d_tor = int(row[4][0])
		spine = int(row[7][1:3]) - 17
		# time = float(row[9][1:-2]) 
		alt_path = row[7]
		time_s = row[9]

		if not time_s in cong_paths_dict.keys():
			print ">>>>>>NOT FOUND time: ",time_s," in the ",file1," and ",file2
			break



		# For a given timestapm get all the paths that are congested at the given time
		queue_util = cong_paths_dict[time_s]
		for i in queue_util:
			# Check if new path falls into the list of all congested path at that time
			if(i[0] == s_tor or i[2] == d_tor) and i[1] == spine:
				alt_cong_path=alt_cong_path+1
				found_cong=1
				if flow_id in flows_alt_cong_paths.keys():
					if time_s not in flows_alt_cong_paths[flow_id]:
						flows_alt_cong_paths[flow_id].append(time_s)
				else:
					flows_alt_cong_paths[flow_id] = [time_s]
				# print "FOUND congested path :", i, "\tFlowID: ", flow_id ," ",s_tor,"->",d_tor," with path ",alt_path, " at time ",time_s
				# print flows_alt_cong_paths[flow_id]
				break;
		# if found_cong==0:
		# 	print "FOUND NO-congested path with FlowID: ", flow_id ," ",s_tor,"->",d_tor," with path ",alt_path
		# 	print flows_alt_paths[flow_id]

		alt_tot_paths=alt_tot_paths+1
		found_cong=0


print '-'*10
print "Occurence of path change\t\t\t: ", alt_tot_paths
print "Total number of unique flows\t\t\t: ", len(flows_alt_cong_paths.keys())
print "Occurence of changing to a congested path\t: ", alt_cong_path, " out of ", alt_tot_paths , "( %.1f" %((alt_cong_path*100)/alt_tot_paths) , "% )\n"

flowlets_sum = {1:0,2:0,3:0,4:0,5:0,6:0,7:0,8:0,9:0,10:0}
for it in flows_alt_cong_paths.keys():
	if len(flows_alt_cong_paths[it]) < 10:
		flowlets_sum[len(flows_alt_cong_paths[it])] = flowlets_sum[len(flows_alt_cong_paths[it])] + 1
	else:
		flowlets_sum[10] = flowlets_sum[10] + 1
print "Flows with num.[1-10] flowlets to a congested new path: "
for i in flowlets_sum.keys():
	print "\tFlowlets Num. ",i,"\t",flowlets_sum[i]

	