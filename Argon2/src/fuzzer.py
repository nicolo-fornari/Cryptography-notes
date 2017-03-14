import os

def params(a,m,p,l,t):
	cmd = "./argon2 -v 0 -a %d -m %d -p %d -l %d -n %d" % (a,m,p,l,t)
	print cmd

	p1 = os.popen(cmd,"r")
	digest1 = p1.readline()[:-1]
	p2 = os.popen(cmd,"r")
	digest2 = p2.readline()[:-1]

	if digest1 == digest2:
		code = " OK"
	else:
		code = " ERR"	
	log.write(cmd+code+" \n")


def fuzz():
	argon = [0,1] # respectively argon2d, argon2i
	memory = [20,30,40,50]
	parallelism = [1,2,3,4,5,6,7,8]
	length = [32,64,80,128,256]
	passes = [1,2,3,4]

	for a in argon:
		for m in memory:
			for p in parallelism:
				for l in length:
					for t in passes:
						params(a,m*8*p,p,l,t)



log = open('log','w')
fuzz()
log.close()

