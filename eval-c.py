#!/usr/bin/env python

import sys
import os
import signal
import time
import socket
import datetime

myport = 0
myid = 0

duration = 60
rw_ratio = 3

def spawn(bin, args):
	#mknod also works.
	pipe_r, pipe_w = os.pipe()
	child = os.fork()
	if pid == 0:
		os.dup2(pipe_w, 1)
		os.dup2(pipe_w, 2)
		os.execv(bin, args)
	else:
		return child, pipe_r


def evaluate(eval_id):
	bin = './eval-c'
	args = (bin, myid, eval_id, duration, rw_ratio)
	child, pipe_r = spaw(bin, args)
	pid, status = os.waitpid(child, 0)
	if stauts != 0 or pid != child:
		result = str(myid) + ': error\n'
	else:
		result = str(myid) + ': ' + read(pipe_r, 1000)
	os.close(pipe_r)
	return result

class commander:
	def __init__(self):
		self.mysocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.mysocket.bind(("", myport))
		self.mysocket.listen(5)
		self.commander_socket, self.commander_address = self.mysocket.accept()
	
	def wait_command(self):
		command = self.commander_socket.recv(1000)
		return command

	def reply(self, result):
		self.commander_socket.send(result)

	def close(self):
		self.mysocket.close()
		self.commander_socket.close()

def start():
	cmd = commander()
	while (1):
		command = cmd.wait_command()
		if command == "end":
			cmd.close()
			return
		elif command == "eval-1":
			result = evaluate(1)
			cmd.reply(result)
		elif command == "eval-2":
			result = evalute(2)
			cmd.reply(result)
		elif command == 'eval-3':
			result = evalute(3)
			cmd.reply(result)
		elif command == 'ok':
			cmd.reply('ok')
		else
			bad_command = 'bad command: ' + command
			cmd.reply(bad_command)

if __name__ == '__main__':
	if len(sys.argv) == 3:
		myid = int(sys.argv[1])
		myport = int(sys.argv[2])
		start()
	else:
		print 'too many or too few arguments'



