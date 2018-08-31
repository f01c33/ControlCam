#!/usr/bin/python3

import os, time

# 65535
def main():
	for i in range(1,65535):
		os.system("curl 'http://www.canyouseeme.org/' -H 'Referer: http://www.canyouseeme.org/' -H 'DNT: 1' --data 'port="+str(i)+"&IP=177.92.51.204' --compressed > out3")
		os.system("echo "+str(i)+"&& cat out3 | grep -E -o 'Success:</b></font>&nbsp;I can see your service on <b>[0-9]{1-3}\.[0-9]{1-3}\.[0-9]{1-3}\.[0-9]{1-3}</b> on port \(<b>[0-9]*' >> openports")

if __name__ == '__main__':
	main()