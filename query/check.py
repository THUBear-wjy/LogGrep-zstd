fo = open("log.out", 'r', encoding='ISO-8859-1')
lines = fo.readlines()
fo.close()
nset = []
for l in lines:
    try:
        nset.append(l.strip())
    except:
        continue

gnset = []
fo1 = open("glog.out", 'r')
lo1 = fo1.readlines()
fo1.close()
for l in lo1:
    try:
        gnset.append(l.strip())
    except:
        continue

print(len(set(nset)))
print(len(set(gnset)))

ss = set(nset) - set(gnset)
print(len(ss))
for n in ss:
    print(n)