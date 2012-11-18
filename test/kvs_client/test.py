#!/usr/bin/env python

import sys
import os
import signal
import time

timeout = 40

def spawn(bin):
	#mknod also works.
	log = open(bin + '.log', 'w')
	pid = os.fork()
	if pid == 0:
		os.dup2(log.fileno(), 1)
		os.dup2(log.fileno(), 2)
		os.execv(bin, (bin, ))
	else:
		log.close()
	return pid

def run_all_test():
	print 'yet to implement'

def run_test(target, timeout):
	server_bin = './server_' + str(target)
	client_bin = './client_' + str(target)
	server = spawn(server_bin)
	time.sleep(2)
	client = spawn(client_bin)
	client_terminated = False
	server_terminated = False
	client_status = 0
	server_status = 0
	time.sleep(2)
	#a somewhat coarse timer.
	#for precision, stick to absolute time instead
	timer = 0
	interval = 1
	while (timer < timeout):
		if (not client_terminated):
			(pid, status) = os.waitpid(client, os.WNOHANG)
			if pid == client:
				client_terminated = True
				client_status = status
		if (not server_terminated):
			(pid, status) = os.waitpid(server, os.WNOHANG)
			if pid == server:
				server_terminated = True
				server_status = status
		if (client_terminated and server_terminated):
			break
		timer = timer + interval
		time.sleep(interval)
	if (timer < timeout):
		if server_status == 0x0 and client_status == 0x0:
			print 'test' + str(target) + ' passed'
			os.remove(client_bin + '.log')
			os.remove(server_bin + '.log')
		else:
			print 'test' + str(target) + ' failed'
			if client_status != 0x0:
				print 'see ' + client_bin + '.log'
			if server_status != 0x0:
				print 'see ' + server_bin + '.log'
	else:
		print 'test' + str(target) + 'timed out'
		if not client_terminated:
			os.kill(client, signal.SIGKILL)
			print 'see ' + client_bin + '.log'
		if not server_terminated:
			os.kill(server, signal.SIGKILL)
			print 'see ' + server_bin + '.log'

if __name__ == '__main__':
	if len(sys.argv) == 1:
		run_all_test()
	elif len(sys.argv) == 2:
		run_test(sys.argv[1], timeout)
	else:
		print 'too many arguments'



