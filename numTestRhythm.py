import time
import math

def test(start):
	vals = []
	delta = round(time.time() * 1000) - start
	#print delta
	xMod = 0
	yMod = 0
	zMod = 0
	pitVMod = 0
	rolVMod = 0
	pitPMod = 0
	rolPMod = 0
	degRatio = 90.0 / 5000.0
	if delta > 10000:
		if delta < 33000:
			xMod = 1
		elif delta < 56000:
			yMod = 1
		elif delta < 79000:
			zMod = 1
		elif delta < 102000:
			pitVMod = 1
		elif delta < 125000:
			rolVMod = 1
		elif delta < 148000:
			pitPMod = 1
		elif delta < 171000:
			rolPMod = 1

	#print str(xMod) + " " + str(yMod) + " " + str(zMod)
	#print degRatio
	vals.append(xMod * math.sin(math.radians((delta - 10000) * degRatio)))
	vals.append(yMod * math.sin(math.radians((delta - 33000) * degRatio)))
	vals.append(zMod * math.sin(math.radians((delta - 79000) * degRatio)))
	vals.append(pitVMod * math.sin(math.radians((delta - 102000) * degRatio)))
	vals.append(rolVMod * math.sin(math.radians((delta - 125000) * degRatio)))
	vals.append(pitPMod * math.sin(math.radians((delta - 148000) * degRatio)))
	vals.append(rolPMod * math.sin(math.radians((delta - 171000) * degRatio)))

	return vals

start = round(time.time() * 1000)

for i in range(17100):
	print test(start)
	#test(start)
	time.sleep(.01)
