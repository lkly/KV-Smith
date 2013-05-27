#!/usr/bin/env python

import sys
import os
import signal
import time
import socket
import datetime



#address for in-group communication and for service
peers_local = ['A 127.0.0.1 10000 127.0.0.1 10001',
		 'B 127.0.0.1 11000 127.0.0.1 11001',
		 'C 127.0.0.1 12000 127.0.0.1 12001']

peers = ['A 192.168.1.101 10000 192.168.1.101 10001',
		 'B 192.168.1.102 10000 192.168.1.102 10001',
		 'C 192.168.1.103 10000 192.168.1.103 10001']


#0: master, 1: backup
membership = {'A':1,
			  'B':0,
			  'C':1}

myname = ''

myport = ''

child = None

startcount = 0

killtime = ''

def gettime():
	now = datetime.datetime.now()
	return "%d %d %d " % (now.hour, now.minute, now.second)

def spawn(bin, args):
	output = open(myname + '-' + str(startcount) +'.out', 'w')
	try:
		pid = os.fork()
	except OSError as msg:
		print '@spawn: fork child failed'
		output.close()
		return 0
	if pid == 0:
		os.dup2(output.fileno(), 1)
		os.dup2(output.fileno(), 2)
		output.close()
		os.execv(bin, args)
	else:
		output.close()
	return pid


def prepare():
	fn = 'kvs-s-' + myname + '.config'
	cf = open(fn, 'w')
	for peer in peers:
		cf.write(peer)
		cf.write('\n')
	cf.close()

def start_server():
	global startcount
	startcount = startcount + 1
	log = open(myname+'.log', 'w')
	log.close()
	global child
	bin = './eval-s'
	args = (bin, myname, str(membership[myname]*5))
	child = spawn(bin, args)
	if child == 0:
		return 'fail'

	#try 3 times
	times = 3
	while times > 0:
		time.sleep(1)
		pid, status = os.waitpid(child, os.WNOHANG)
		if pid == 0 and status == 0:
			return 'ok'
		times = times - 1
		time.sleep(1)
	return 'fail'

def clear():
	pass
#	os.remove(myname+'.log')
#	os.remove(myname+'-'+ str(startcount)+'.out')

def kill_server():
	global child
	global killtime
	if child != None:
		status = os.waitpid(child, os.WNOHANG)
		if status != (0,0):
			print 'server crashed'
			child = None
			return 'fail'
		#always succeed
		os.kill(child, signal.SIGKILL)
		os.waitpid(child, 0)
		killtime = time.time()
		child = None
		clear()
		return 'ok'
	return 'ok'

class commander:
	def __init__(self):
		self.mysocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.mysocket.bind(("", myport))
		self.mysocket.listen(5)
		self.commander_socket, self.commander_address = self.mysocket.accept()
	
	def wait_command(self):
		command = ''
		while(1):
			pcommand = self.commander_socket.recv(1000)
			command = command + pcommand
			if command[len(command)-1] == '#':
				return command[0:len(command)-1]

	def reply(self, result):
		self.commander_socket.send(result+"#")

	def close(self):
		self.mysocket.close()
		self.commander_socket.close()

def start():
	prepare()
	cmd = commander()
	while (1):
		command = cmd.wait_command()
		if command == "end":
			print 'end'
			cmd.close()
			return
		elif command == "kill":
			print 'kill: ',
			result = kill_server()
			cmd.reply(result)
			print result
		elif command == "start":
			print 'start: ',
			result = start_server()
			cmd.reply(result)
			print result
		elif command == "ok":
			print 'ok'
			cmd.reply('ok')
		elif command == "getkt":
			print 'getkilltime: ',
			print getkilltime()
			cmd.reply(getkilltime())
		elif command == "getrt":
			print 'getrecoverytime: ',
			print getrecoverytime()
			cmd.reply(getrecoverytime())
		else:
			print 'unknown command: ',
			print command

def getkilltime():
	return str(int(killtime))

def getrecoverytime():
	rf = open("recovery.txt", 'r')
	recoverytime = rf.readline(100)
	rf.close()
	return recoverytime[0:(len(recoverytime)-1)]

if __name__ == '__main__':
	#server name: A, B, C
	#server port: 9000, 9100, 9200
	if len(sys.argv) == 3:
		myname = sys.argv[1]
		myport = int(sys.argv[2])
		start()
	else:
		print 'too many or too few arguments'



