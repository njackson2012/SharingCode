import sys

fields = [['speed'], ['posx'], ['posy'], ['posz'], ['gforcex'], ['gforcey'], ['gforcez']]
moremoreContent = []

with open(sys.argv[1]) as fin:
	content = fin.read()
	moreContent = content.splitlines()
	for n in moreContent:
		moremoreContent.append(n.split(':'))

for i in moremoreContent:
	for n in range(len(fields)):
		if i[0] == fields[n][0]:
			fields[n].append(i[1])

for i in range(len(fields[0])):
	for j in range(len(fields)):
		sys.stdout.write(fields[j][i] + " ");
	sys.stdout.write("\n")
