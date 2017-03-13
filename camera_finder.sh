#!/bin/bash

sudo nmap -n -PN -sS -F -T4 -O -oG out -O --osscan-limit 192.168.0.1/24
# sudo nmap -n -PN -p U:666,T:666,S:666 -sS -T5 -O -oG out -O --osscan-limit 192.168.1.1/24 > out2;
#cat out | grep -i [0-9].*camera
cat out | grep -P -i -o '[\d]{1,3}\.[\d]{1,3}\.[\d]{1,3}\.[\d]{1,3}(?=.*open)'
cat out2 | grep -E -i -oz '[\d]{1,3}\.[\d]{1,3}\.[\d]{1,3}\.[\d]{1,3}(?=.*B\-Link)'

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