import sys

def A(m, n, i):
	if m == 0:
		return n+1
	elif n == 0:
		x = A(m-1, 1, i+1)
		print str(i) + ": " + str(x)
		return x
	else:
		x = A(m-1, A(m, n-1, i+1), i+1)
		print str(i) + ": " + str(x)
		return x

print "0: " + str(A(int(sys.argv[1]), int(sys.argv[2]), 1))
