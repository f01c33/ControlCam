#!/bin/bash

sudo nmap -F -O --osscan-limit 172.25.3.0/24 > out;
# sudo nmap -n -PN -p U:666,T:666,S:666 -sS -T5 -O -oG out -O --osscan-limit 192.168.1.1/24 > out2;
#cat out | grep -i [0-9].*camera
#cat out | grep -P -i -o '[\d]{1,3}\.[\d]{1,3}\.[\d]{1,3}\.[\d]{1,3}(?=.*open)'
#cat out2 | grep -E -i -oz '[\d]{1,3}\.[\d]{1,3}\.[\d]{1,3}\.[\d]{1,3}(?=.*B\-Link)'
grep -E '(A2:58:57:D5:C0:00)|(00:0C:5D:75:84:22)|(00:0C:5D:75:8C:13)|(00:6E:06:1C:07:38)|(00:AF:30:30:93:F1)|(00:0C:5D:75:89:E9)|(00:0D:C5:DA:A8:4B)|(78:A5:DD:09:B2:2A)' -C 5 out > cameras_found.txt;
cat cameras_found.txt
# Nmap scan report for 172.25.6.25
# Host is up (0.0015s latency).
# Not shown: 99 closed ports
# PORT   STATE SERVICE
# 80/tcp open  http
# MAC Address: 78:A5:DD:09:B2:2A (Shenzhen Smarteye Digital Electronics)
# Device type: specialized|webcam
# Running: AirMagnet embedded, Foscam embedded, Instar embedded, Linux 2.4.X
# OS CPE: cpe:/h:airmagnet:smartedge cpe:/h:foscam:fi8904w cpe:/h:foscam:f18910w cpe:/h:foscam:f18918w cpe:/h:instar:in-3010 cpe:/o:linux:linux_kernel:2.4
# OS details: AirMagnet SmartEdge wireless sensor; or Foscam FI8904W, FI8910W, or FI8918W, or Instar IN-3010 surveillance camera (Linux 2.4)
# Network Distance: 1 hop