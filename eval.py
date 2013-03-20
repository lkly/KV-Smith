#!/usr/bin/env python

import sys
import os
import signal
import time
import socket
import datetime


servers = ['A 127.0.0.1 10000',
		   'B 127.0.0.1 10000',
		   'C 127.0.0.1 10000']

clients = ['0 20 127.0.0.1 9000',
		   '20 20 127.0.0.1 9000']


class node:
	def __init__(self, name, ip, port):
		self.name = name
		self.mysocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.mysocket.connect((ip, port))

	#work and done called in pairs
	def work(self, job):
		self.mysocket.send(job)

	def done(self):
		result = self.mysocket.recv(1000)
		return result

	def check(self):
		self.mysocket.send('ok')
		self.mysocket.settimeout('10.0')
		try:
			result = self.mysocket.recv(1000)
		except socket.timeout as msg:
			severs.remove()
			return False
		self.mysocket.settimeout(None)
		if (result == 'ok'):
			return True
		else:
			return False

	def close(self):
		self.mysocket.close()

snodes = []
cnodes = []

def eval_1(number):
	print 'start eval-1 with %d clients:' % (number,)
	workers = select(number)
	print 'use clients: ',
	for i in workers:
		print i.name,
	print '\n'
	for i in workers:
		i.work('eval-1')
	print 'sleep 1 minute'
	time.sleep(60)
	print 'gather results'
	for i in workers:
		result = i.done()
		print str(i.name) + ': '
		print result
		
def eval_2(number):
	print 'start eval-2 with %d clients:' % (number,)
	workers = select(number)
	print 'use clients: ',
	for i in workers:
		print i.name,
	print '\n'
	for i in workers:
		i.work('eval-2')
	print 'sleep 30 seconds'
	time.sleep(30)

	#preconfigurated master
	print 'kill master'
	snodes[1].work('kill')

	print 'sleep 20s'
	print 'gather results'
	for i in workers:
		result = i.done()
		print str(i.name) + ': '
		print result

def eval_3(number):
	print 'start eval-1 with %d clients:' % (number,)
	workers = select(number)
	print 'use clients: ',
	for i in workers:
		print i.name,
	print '\n'
	for i in workers:
		i.work('eval-1')
	print 'sleep 1 minute'
	time.sleep(60)
	print 'gather results'
	for i in workers:
		result = i.done()
		print str(i.name) + ': '
		print result


def start():
	for i in servers:
		args = i.split()
		snode = snodes.append(node(args[0], args[1], int(args[2])))
	for i in clients:
		args = i.split()
		for j in range(args[1]):
			cnodes = cnodes.append(node(args[0]+j, args[2], int(args[3])))

evaluate()


if __name__ == '__main__':
	if len(sys.argv) == 1:
		start()
	else:
		print 'too many or too few arguments'
