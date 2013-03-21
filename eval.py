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
		self.mysocket.send(job+'#')

	def done(self):
		result = ''
		while(1):
			presult = self.mysocket.recv(1000)
			result = result + presult
			if result[len(result)-1] == '#':
				return result[0:len(result)-1]


	def check(self):
		self.mysocket.send('ok#')
		self.mysocket.settimeout('10.0')
		try:
			result = ''
			while (1):
				presult = self.mysocket.recv(1000)
				result = result + presult
				if result[len(result)-1] == '#':
					break
		except socket.timeout as msg:
			return 'fail'
		self.mysocket.settimeout(None)
		if (result[0:2] == 'ok'):
			return 'oK'
		else:
			return 'fail'

	def close(self):
		self.mysocket.close()

snodes = []
cnodes = []

def select(number):
	return cnodes[0:number]

def eval_1(number):
	print 'start eval-1 with %d clients:' % (number,)
	workers = select(number)
	print 'use clients: ',
	for i in workers:
		print str(i.name) + ' '
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
		print str(i.name) + ' '
	print '\n'
	for i in workers:
		i.work('eval-2')

	print 'sleep 30 seconds'
	time.sleep(30)

	#preconfigurated master
	print 'kill master'
	snodes[1].work('kill')

	print 'sleep 20 seconds'
	time.sleep(20)

	print 'gather results'
	for i in workers:
		result = i.done()
		print str(i.name) + ': '
		print result


def eval_3(number):
	print 'start eval-3 with %d clients:' % (number,)
	workers = select(number)
	print 'use clients: ',
	for i in workers:
		print i.name,
	print '\n'
	for i in workers:
		i.work('eval-3')

	print 'sleep 11 minutes'
	time.sleep(660)

	print 'gather results'
	for i in workers:
		result = i.done()
		print str(i.name) + ': '
		print result

def prepare():
	#start servers
	for i in snodes:
		i.work('start')

	time.sleep(3)
	for i in snodes:
		result = i.done()
		print 'start ' + str(i.name) + ': '
		print result

def clear():
	#kill servers
	for i in snodes:
		i.work('kill')

	time.sleep(3)
	for i in snodes:
		result = i.done()
		print 'kill ' + str(i.name) + ': '
		print result

def end():
	for i in snodes:
		i.work('end')
		i.close()
	for i in cnodes:
		i.work('end')
		i.close()


def evaluate():
	while 1:
		next_eval = raw_input('eval')
		ok = raw_input('second '+next_eval)
		if ok == 'yes':
			doeval = next_eval.split()
		else:
			continue
		if doeval[0] == '1':
			prepare()
			eval_1(doeval[1])
			clear()
		elif doeval[0] == '2':
			prepare()
			eval_2(doeval[1])
			clear()
		elif doeval[0] == '3':
			prepare()
			eval_3(doeval[1])
			clear()
		elif doeval[0] == 'e':
			end()
			return
		else:
			print 'unknown eval'

def start():
	for i in servers:
		args = i.split()
		snode = snodes.append(node(args[0], args[1], int(args[2])))
	for i in clients:
		args = i.split()
		for j in range(args[1]):
			cnodes = cnodes.append(node(args[0]+j, args[2], int(args[3])+j))
	for i in cnodes:
		result = i.check()
		print 'check client %d: %s\n' % (i.name, result)
		if result != 'OK':
			end()
			return
	for i in snodes:
		result = i.check()
		print 'check server %s: %s\n' % (i.name, result)
		if reuslt != 'OK':
			end()
			return
	evaluate()


if __name__ == '__main__':
	if len(sys.argv) == 1:
		start()
	else:
		print 'too many or too few arguments'


