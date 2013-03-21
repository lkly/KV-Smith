#!/usr/bin/env python

import sys
import os
import signal
import time
import socket
import datetime



#address for in-group communication and for service
peers = ['A 127.0.0.1 10000 127.0.0.1 10000',
		 'B 127.0.0.1 10000 127.0.0.1 10000',
		 'C 127.0.0.1 10000 127.0.0.1 10000']

#0: master, 1: backup
membership = {'A':1,
			  'B':0,
			  'C':1}

myname = ''

child = None

def gettime():
	now = datetime.datetime.now()
	return "%d:%d:%d " % (now.hour, now.minute, now.second)

def spawn(bin, args):
	output = open(myname+'.out', 'w')
	pid = os.fork()
	if pid == 0:
		os.dup2(output.fileno(), 1)
		os.dup2(output.fileno(), 2)
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
	log = open(myname+'.log', 'w')
	log.close()
	global child
	bin = './eval-s'
	args = (bin, myname, membership[myname]*5)
	child = spawn(bin, args)

def checkchild():
	#try 3 times
	times = 3
	while times > 0:
		time.sleep(3)
		pid, status = os.waitpid(child, os.WNOHANG)
		if pid == 0 and status == 0:
			return 'ok'
		times = times - 1
	return 'fail'

def clear():
	os.remove(myname+'.log')
	os.remove(myname+'.out')

def kill_server():
	global child
	if child != None:
		status = os.waitpid(child, os.WNOHANG)
		if status != (0,0):
			print 'server crashed'
			child = None
			return 'fail'
		os.kill(child, signal.SIGKILL)
		child = None
		clear()
		return 'ok'
	return 'ok'

class commander:
	def __init__(self):
		self.mysocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.mysocket.bind(("", 8000))
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
			cmd.close()
			return
		elif command == "kill":
			result = kill_server()
			cmd.reply(result)
		elif command == "start":
			start_server()
			result = checkchild()
			cmd.reply(result)
		elif command == "ok":
			cmd.reply('ok')
		else
			print 'bad command: ',
			print command


if __name__ == '__main__':
	if len(sys.argv) == 2:
		myname = sys.argv[1]
		start()
	else:
		print 'too many or too few arguments'



