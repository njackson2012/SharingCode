import time
import math

def test(start):
	vals = []
	delta = round(time.time() * 1000) - start
	#print delta
	cycleLength = 0
	xMod = 0
	yMod = 0
	zMod = 0
	pitVMod = 0
	rolVMod = 0
	pitPMod = 0
	rolPMod = 0
	delay = 5000
	pause = 2000
	amplitude = 1
	testLength = 20000
	pauseMod = 0
	tdelta = delta - delay - (testLength / 4)
	degRatio = 0.0

	if delta > delay:

		if tdelta > 0:
			pauseMod += 1
			while tdelta > pause:
				#print tdelta
				tdelta -= pause 
				tdelta -= (testLength / 4)
				pauseMod += 1
		if tdelta >= 0:
			#print tdelta
			#print tdelta % pause
			print "Extending"
			degRatio = (360.0 / (testLength + ((tdelta % pause) + (pause * pauseMod))))
		else:
			degRatio = (360.0 / (testLength + (pause * pauseMod)))

		cycleLength = (delay + testLength + ((4 * pause) * (1 + int(pauseMod / 4))))

		if delta < cycleLength:
			xMod = 1
		elif delta < cycleLength * 2:
			yMod = 1
		elif delta < cycleLength * 3:
			zMod = 1
		elif delta < cycleLength * 4:
			pitVMod = 1
		elif delta < cycleLength * 5:
			rolVMod = 1
		elif delta < cycleLength * 6:
			pitPMod = 1
		elif delta < cycleLength * 7:
			rolPMod = 1

	#print str(xMod) + " " + str(yMod) + " " + str(zMod)
	#print degRatio
	vals.append(xMod * math.sin(math.radians((delta - delay) * degRatio)))
	vals.append(yMod * math.sin(math.radians((delta - cycleLength) * degRatio)))
	vals.append(zMod * math.sin(math.radians((delta - (cycleLength * 2)) * degRatio)))
	vals.append(pitVMod * math.sin(math.radians((delta - (cycleLength * 3)) * degRatio)))
	vals.append(rolVMod * math.sin(math.radians((delta - (cycleLength * 4)) * degRatio)))
	vals.append(pitPMod * math.sin(math.radians((delta - (cycleLength * 5)) * degRatio)))
	vals.append(rolPMod * math.sin(math.radians((delta - (cycleLength * 6)) * degRatio)))

	return vals

start = round(time.time() * 1000)

for i in range(17100):
	print test(start)
	#test(start)
	time.sleep(.01)
