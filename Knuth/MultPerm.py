import sys
import time

full = []
result = []
tagged = []

for i in range(len(sys.argv)):
	if i > 0:
		temp = sys.argv[i].split()
		temp2 = []
		for i in temp:
			if i[0] == '(':
				temp2 = []
				temp2.append(i[1])
			elif i[-1] == ')':
				temp3 = ''
				for j in range(len(i)-1):
					temp3 += i[j]
				temp2.append(temp3)
				full.append(temp2)
			else:
				temp2.append(i)

quant = 0

for i in range(len(full)):
	full[i].append(full[i][0])
	tagged.append([i, len(full[i]) - 1])
for i in full:
	for j in i:
		quant += 1

current = ''

while len(tagged) != quant:
	for i in range(len(full)):
		for j in range(len(full[i])):
			if not ([i, j] in tagged):
				tagged.append([i, j])
				tj = j+1
				while [i, tj%len(full[i])] in tagged:
					tj += 1
				if [i, j+1] == [i, tj%len(full[i])]:
					
print tagged
