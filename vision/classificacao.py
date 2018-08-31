#!/bin/python3

from math import *
import os

lines = []
with open("canetas.txt") as file:
	lines = file.read().split("\n")
base = []
for l in lines:
	s = l.split(" ")
	# print(s[2:])
	p1 = (int(s[2]),int(s[3]))
	# print(p1)
	p2 = (int(s[4]),int(s[5]))
	dp = (int((p1[0]+p2[0])/2),int((p1[1]+p2[1])/2))

	base.append([int(sqrt(abs(p1[0]-p2[0])*abs(p1[1]-p2[1]))),dp[0],dp[1]])

	# base.append([s[0],p1[0],p1[1],p2[0],p2[1]])
	# os.system("./vision "+s[0]+" >> tmp")

	# print(dp)
	# dp = abs(dp[0]*dp[1])
	# if dp < 2000:
	# 	print(dp,s[0])

with open("tmp") as file:
	result = file.read().split("\n")
result.pop()

def inside(x1,y1,x2,y2,x_c,y_c):
	if x1 <= x_c and x2 >= x_c and y1 <= y_c and y2 >= yc:
		return 1
	else:
		return 0

i = -1
dist = []
hit = 0
# print(len(base),len(result))
for r in result:
	i = i + 1;
	end = [r.split()[0],int(r.split()[1]),int(r.split()[2])]
	# print(end[1:],base[i][1:])
	pit = int(sqrt((end[1]-base[i][1])**2+(end[2]-base[i][2])**2))
	# print(end[1:],base[i],pit)
	if pit < base[i][0]:
		# print("woo")
		hit = hit + 1
	# dp = (end[1]-base[i][1],end[2]-base[i][2])
	# print(end[0],dp[0]+dp[1])
	# print(end[0],",",int(pit))
	# print(end[1:],base[i],inside(base[i][0],base[i][1],base[i][2],base[i][3],end[0],end[1]))
print(hit)