#!/bin/python3

from math import *

lines = []
with open("anotations.txt") as file:
	lines = file.read().split("\n")

for l in lines:
	s = l.split(" ")
	p1 = (int(s[2]),int(s[3]))
	p2 = (int(s[4]),int(s[5]))
	dp = (p1[0]-p2[0],p1[1]-p2[1])
	print(dp)
	# dp = abs(dp[0]*dp[1])
	# if dp < 2000:
	# 	print(dp,s[0])