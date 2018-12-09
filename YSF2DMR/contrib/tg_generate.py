f=open('/tmp/group.txt')
group=f.readlines()
f.close()

def tg_count(tg_num):
  count=0
  global group
  for line in group:
    newlines=line.split('},')
    for item in newlines:
        if (item.find(','+str(tg_num)+',')>0): count+=1
  return count

f = open('/tmp/data.json')
content = f.readlines()
f.close()

content=[x.strip() for x in content]
content = content[1:]
content = content[:-1]
f = open("/tmp/TGList.txt","wb+")

for line in content:
  line=line.replace('"','')
  line=line.replace(',','')
  line=line.split(':')
  i=line[0]
  val=int(i)
  if (val>=4000) and (val<=5000):
    b='1'
  else:
    b='0'
  if val==9990: b=2
  if (val>90) and (val!=4000) and (val!=5000) and (val!=9990):
    c=tg_count(val)
  else:
    c=0
  line=line[0]+';'+ str(b)+';'+str(c)+';'+line[1]+';'+line[1]
  line=line.replace(' ','')
  f.write(line+'\n')

f.close()
