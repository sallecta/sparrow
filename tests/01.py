import sys
import math
print("Welcome to ", sys.version)
print("sys.flags:",sys.flags)
print("5+3/2*4:",5+3/2*4)
str="string"
print("string length:",len(str))
print("min(5, 10):",min(5, 10))
print("max(5, 10):",max(5, 10))
print("abs(-7.25):",abs(-7.25))
print("bool(1)):",bool(1))
print("bool(0)):",bool(0))
print("bool(''''):",bool(""))
print("bool(''abc''):",bool("abc"))
print("round(5.76543, 2):",round(5.76543, 2))
print("int(3.5):",int(3.5))

for n in range(6):
  print("for n in range(6):",n) 
for n in range(3, 6):
  print("for n in range(3, 6):",n) 

print("float(3):",float(3))