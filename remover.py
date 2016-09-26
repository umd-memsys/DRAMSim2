# Memory Trace Remover 
#
# It randomly removes x percentage of lines of the given memory trace (cycle = 1M)
#
# Ahmet Fatih Inci 	8/26/16  --afinci--

import random, sys, math

def removeLine(inputfile, percentage, outputfile="out"):
	with open(inputfile, 'r') as f:
		lines = f.readlines()
		cn = 0
		for l in lines:
			if int(l.split(' ')[2][:-1]) < 1000001:
				cn+=1

		num = math.floor(cn*(int(percentage)/100.0))
		i=0
		while i<num:
			n = random.randint(0,len(lines)-1)
			#print n
			if int(lines[n].split(' ')[2][:-1]) < 1000001:
				del lines[n]
				i+=1
		f2 = open(outputfile, "a+")
		for line in lines:
			f2.write(line)
		f2.close()

def main(argv):
	if len(argv) > 3:
		removeLine(argv[1], argv[2], argv[3])
	elif len(argv) > 2:
		removeLine(argv[1], argv[2])
	else:
		raise ValueError('Arguments are not provided.')


if __name__ == "__main__":
    main(sys.argv)
