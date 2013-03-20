#!/usr/bin/env python

import sys
import os
import signal
import time
import socket
import datetime



#note the trailing blank.
peers = ['A 127.0.0.1 10000',
		 'B 127.0.0.1 10000',
		 'C 127.0.0.1 10000']

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
	cf = open('kvs-s.config', 'w')
	for peer in peers:
		cf.write(peer)
		cf.write('\n')
	cf.close()
	log = open(myname+'.log', 'w')
	log.close()

def start_server():
	global child
	bin = './eval-s'
	args = (bin, myname, membership[myname]*5)
	child = spawn(bin, args)

def clear():
	os.remove('kvs-s.config')
	os.remove(myname+'.log')
	os.remove(myname+'.out')

def kill_server():
	status = os.waitpid(child, os.WNOHANG)
	if status != (0,0):
		print 'server crashed'
		return
	os.kill(child, signal.SIGKILL)
	clear()

class commander:
	def __init__(self):
		self.mysocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.mysocket.bind(("", 8000))
		self.mysocket.listen(5)
		self.commander_socket, self.commander_address = self.mysocket.accept()
	
	def wait_command(self):
		command = self.commander_socket.recv(1000)
		return command
	

def start():
	prepare()
	cmd = commander()
	while (1):
		command = cmd.wait_command()
		if command == "end":
			kill_server()
			return
		elif command == "kill":
			kill_server()
		elif command == "start":
			start_server()
		else
			print 'bad command: ',
			print command

if __name__ == '__main__':
	if len(sys.argv) == 2:
		myname = sys.argv[1]
		start()
	else:
		print 'too many or too few arguments'



