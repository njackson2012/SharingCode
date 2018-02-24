import sys

def GCD(a, b):
	if a == 0 or b == 0:
		if a > b:
			return a
		else:
			return b
	if a > b:
		return GCD(a % b, b)
	return GCD(a, b % a)

print GCD(int(sys.argv[1]), int(sys.argv[2]))
