#!/usr/bin/env python

import sys
import os
import signal
import time
import socket

servers = ['A 127.0.0.1 10000',
		   'B 127.0.0.1 10000',
		   'C 127.0.0.1 10000']

def prepare_config():
	cf = open('kvs-c.config', 'w')
	for server in servers:
		cf.write(server)
		cf.write('\n')
	cf.close()

def prepare_children(first_child, children_num, first_port):
	name = 'kvs-' + str(first_child) + '.sh'
	bash_file = open(name, 'w')
	for i in range(children_num):
		bash_file.write('./eval-c.py' + ' ' + str(first_child) + ' ' + str(first_port) + ' &\n')
		first_child += 1
		first_port += 1
	bash_file.close()

def start(first_child)
	name = 'kvs-' + str(first_child) + '.sh'
	os.system('chmod +x ' + name)
	os.system('./' + name)


#shoud be 0-100, and no overlap with other scripts
#example: 0, 20 means 0-19, inclusive
if __name__ == '__main__':
	if len(sys.argv) == 4:
		first_child = int(sys.argv[1])
		children_num = int(sys.argv[2])
		first_port = int(sys.argv[3])
		prepare_config()
		prepare_children(first_child, children_num, first_port)
		start(first_child)
	else:
		print 'too many or too few arguments'

