class Set:
    def __init__(self, content=None):
        self.members = content
    def noDup(self):
        m = []
        for x in self.members:
            if not x in m:
                m.append(x)
        return Set(m)
    def order(self):
        return Set(insertSort(self.members))
    def output(self):
        print(self.members)

def insertSort(s):
    t = []
    for x in s:
        i = 0
        while i < len(t) and x < t[i]:
            i += 1
        if i == len(s):
            t.append(x)
        else:
            t.insert(i, x)
    return t

s1 = Set([1, 2, 3, 4])
s2 = Set([2, 2, 2, 2])
s3 = Set([3, 2, 4, 5])
s1.output()
s2.output()
s3.output()
s3.order().output()
s2.noDup().output()
