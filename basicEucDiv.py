import sys

if len(sys.argv) < 3:
	print "invalid set of parameters"
	exit()

a = int(sys.argv[1])
b = int(sys.argv[2])

q = 0
r = 0

less = False
greater = False

while not (less and greater):
	if a > b:
		greater = True
		a -= a
	elif a < b:
		less = True
		a += a
	elif a == b:
		less = True
		greater = True
	q += 1

r = b - (q * int(sys.argv[1]))

print "q: " + str(q) + " -- r: " + str(r)
