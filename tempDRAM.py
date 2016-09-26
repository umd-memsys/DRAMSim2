# Temperature Dependent DRAM Power and Performance Model 
# 
# How to run? 
# -d , -tmp are necessary parameters to run this program
# python tempDRAM.py -d ini/DDR3_micron_32M_8B_x8_sg15.ini -tmp 85 
# -s , -t , -c are optional parameters: (-s system.ini.example -t traces/k6_aoe_02_short.trc -c 1000000)
#
# Ahmet Fatih Inci   8/26/16  --afinci--


import os, sys
from tempfile import mkstemp
from shutil import move

def rPChanger(temp, ini):
	table = {'50': '11700', '55': '9700', '60': '7800', '65': '7800', '70': '7800', '75': '5800', '80': '3900', '85': '2900', '90': '1900', '95': '1400', '100': '970'}
	newRP = table[temp]
	fh, abs_path = mkstemp()
	with open(abs_path,'w') as new_file:
		with open("%s" %ini) as old_file:
			for line in old_file:
				if line[:15] == "REFRESH_PERIOD=":
					new_file.write(line[:15]+str(newRP)+str("\n"))
				else:
					new_file.write(line)
	os.close(fh)
    
	os.remove("%s" %ini)
    
	move(abs_path, "%s" %ini)

def main(argv):
	dc = {}
	for i in range(2,len(argv),2):
		dc[argv[i-1]] = argv[i]

	ini = dc["-d"] # device ini file 
	temp = dc["-tmp"] # temperature 

	try:
		sysini = dc["-s"]
	except: # default system ini file
		sysini = "system.ini.example"
	try:
		trace = dc["-t"]
	except: # default memory trace 
		trace = "traces/k6_aoe_02_short.trc"
	
	try:
		count = dc["-c"]
	except: # 1M cycle
		count = "1000000"
	
	rPChanger(temp, ini) 

	os.system('./DRAMSim -t %s -s %s -d %s -c %s' %(trace, sysini, ini, count)) # DRAMSim is running 
	
if __name__ == "__main__":
    main(sys.argv)