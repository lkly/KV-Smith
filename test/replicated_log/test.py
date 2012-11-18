#!/usr/bin/env python

import sys
import os
import signal
import time

timeout = 40

def spawn(bin, args):
	#mknod also works.
	#log = open(bin + '.log', 'w')
	pid = os.fork()
	if pid == 0:
		#os.dup2(log.fileno(), 1)
		#os.dup2(log.fileno(), 2)
		os.execv(bin, (bin, args))
	return pid

def run_all_test():
	print 'yet to implement'

def run_test(target, timeout):
	bin = './server_' + str(target)
	server_a = spawn(bin, 'A')
	server_b = spawn(bin, 'B')
	server_c = spawn(bin, 'C')
	server_a_terminated = False
	server_b_terminated = False
	server_c_terminated = False
	server_a_status = 0
	server_b_status = 0
	server_c_status = 0
	time.sleep(2)
	#a somewhat coarse timer.
	#for precision, stick to absolute time instead
	timer = 0
	interval = 1
	while (timer < timeout):
		if (not server_a_terminated):
			(pid, status) = os.waitpid(server_a, os.WNOHANG)
			if pid == server_a:
				server_a_terminated = True
				server_a_status = status
		if (not server_b_terminated):
			(pid, status) = os.waitpid(server_b, os.WNOHANG)
			if pid == server_b:
				server_b_terminated = True
				server_b_status = status
		if (not server_c_terminated):
			(pid, status) = os.waitpid(server_c, os.WNOHANG)
			if pid == server_c:
				server_c_terminated = True
				server_c_status = status
		if (server_a_terminated and server_b_terminated and server_c_terminated):
			break
		timer = timer + interval
		time.sleep(interval)
	if (timer < timeout):
		if server_a_status == 0x0 and server_b_status == 0x0 and server_c_status == 0x0:
			print 'test' + str(target) + ' passed'
#			os.remove(client_bin + '.log')
#			os.remove(server_bin + '.log')
		else:
			print 'test' + str(target) + ' failed'
	else:
		print 'test' + str(target) + ' timed out'
		if not server_a_terminated:
			os.kill(server_a, signal.SIGKILL)
		if not server_b_terminated:
			os.kill(server_b, signal.SIGKILL)
		if not server_b_terminated:
			os.kill(server_b, signal.SIGKILL)

if __name__ == '__main__':
	if len(sys.argv) == 1:
		run_all_test()
	elif len(sys.argv) == 2:
		run_test(sys.argv[1], timeout)
	else:
		print 'too many arguments'



