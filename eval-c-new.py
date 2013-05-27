#!/usr/bin/env python

import sys
import os
import signal
import time
import socket
import datetime

#first is the designated master
servers_local = ['B 127.0.0.1 11001',
		   'A 127.0.0.1 10001',
		   'C 127.0.0.1 12001']

servers = ['B 192.168.1.102 10001',
		   'A 192.168.1.101 10001',
		   'C 192.168.1.103 10001']

evalservers_local = ['A 127.0.0.1 9000',
			   'B 127.0.0.1 9100',
			   'C 127.0.0.1 9200']

evalservers = ['A 192.168.1.101 9000',
			   'B 192.168.1.102 9000',
			   'C 192.168.1.103 9000']

def evalprepare():
	global resultfile
	cf = open('kvs-c.config', 'w')
	for server in servers:
		cf.write(server)
		cf.write('\n')
	cf.close()
	resultfile = open('eval-result.txt', 'w')

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
			#check data integrity?
			presult = self.mysocket.recv(1000)
			result = result + presult
			if result[len(result)-1] == '#':
				return result[0:len(result)-1]


	def check(self):
		self.mysocket.send('ok#')
		result = ''
		while (1):
			presult = self.mysocket.recv(1000)
			if len(presult) == 0:
				return 'fail'
			result = result + presult
			if result[len(result)-1] == '#':
				break
		if (result[0:2] == 'ok'):
			return 'ok'
		else:
			return 'fail'

	def close(self):
		self.mysocket.close()

snodes = []

def make_eval_network():
	global snodes
	for i in evalservers:
		args = i.split()
		snodes.append(node(args[0], args[1], int(args[2])))
	for i in snodes:
		print 'check node ' + i.name + ' ' + i.check()

def prepare():
	#start servers
	for i in snodes:
		i.work('start')

	time.sleep(3)

	for i in snodes:
		result = i.done()
		if result != 'ok':
			print 'start server ' + str(i.name) + ' fail'
			return 'fail'
	return 'ok'

def clear():
	#kill servers
	for i in snodes:
		i.work('kill')

	time.sleep(3)

	for i in snodes:
		result = i.done()
		#kill fail only when the node has crashed
		if result != 'ok':
			print 'kill server' + str(i.name) + ' fail'

def spawn(cid, bin, args):
	output = open('eval-' + str(cid) +'.out', 'w')
	try:
		pipe_r, pipe_w = os.pipe()
	except OSError as msg:
		print '@spawn: pipe failed'
		output.close()
#		os.remove('eval-' + str(cid) +'.out')
		return 0, 0
	try:
		pid = os.fork()
	except OSError as msg:
		print '@spawn: fork child failed'
		output.close()
#		os.remove('eval-' + str(cid) +'.out')
		os.close(pipe_r)
		os.close(pipe_w)
		return 0, 0
	if pid == 0:
		os.close(pipe_r)
		os.dup2(output.fileno(), 1)
		os.dup2(output.fileno(), 2)
		output.close()
		#not sure whether pipe_w is 3 or 4, but definitly not 15(EVAL_PIPE)
		if pipe_w != 15:
			os.dup2(pipe_w, 15)
			os.close(pipe_w)
		os.execv(bin, args)
	else:
		output.close()
		os.close(pipe_w)
	return pid, pipe_r

def gettime():
	now = datetime.datetime.now()
	return "%d:%d:%d " % (now.hour, now.minute, now.second)

def eval_(evalid, cn, duration, rw_ratio):
	hint = 'eval-'+str(evalid)+' with ' + str(cn) + ' clients '
	hint = hint + 'for ' + str(duration) + 's under ' + str(rw_ratio*10) +'% w load: '

	bin = 'eval-c'
	#use this if do distributed eval
	#cid = 0
	children = []
	childrenpipe = []
	result = []
	failed = False
	for i in range(cn):
		args = (bin, str(i), str(evalid), str(duration), str(rw_ratio))
		child, pipe_r = spawn(i, bin, args)
		if child == 0:
			failed = True
			#let spawned children go to the end, fail-safe but not fail-fast
			break
		else:
			children.append(child)
			childrenpipe.append(pipe_r)

	if failed:
		for i in children:
			os.signal(i, SIGKILL)
		for i in childrenpipe:
			os.close(i)
		print hint
		print 'fail'
		return False

	#sleep duration time
	if evalid == 2:
		#kill server in 10s
		time.sleep(10)
	else:
		time.sleep(duration)

	if evalid == 2:
		#preconfigurated master
		snodes[1].work('kill')
		kresult = snodes[1].done()
		if kresult != 'ok':
			print 'kill master ' + snodes[1].name + ' fail'
			failed = True
		else:
			print 'kill master ' + snodes[1].name + ' ok'
			#sleep 20 seconds
			time.sleep(15)

	#for eval-2 only
	#zombies
	if failed:
		for i in children:
			os.signal(i, SIGKILL)
		for i in childrenpipe:
			os.close(i)
		print hint
		print 'fail'
		return False

	assert(len(children) == len(childrenpipe))

	for i in range(len(children)):
		pid, status = os.waitpid(children[i], 0)
		if status != 0 or pid != children[i]:
			failed = True
			print 'check child %d status error' % (i,)
			os.close(childrenpipe[i])
#			os.remove('eval-' + str(i) +'.out')
		else:
			result.append(os.read(childrenpipe[i], 100))
			assert(os.read(childrenpipe[i], 100) == '')
			os.close(childrenpipe[i])
#			os.remove('eval-' + str(i) +'.out')

	if (not failed) and (evalid == 2):
		snodes[1].work('getkt')
		killtime = snodes[1].done()
		#anyone is ok
		snodes[0].work('getrt')
		recoverytime = snodes[0].done()
		failovertime = int(recoverytime)-int(killtime)

	print hint,
	if failed:
		print "fail"
		return False
	else:
		print 'ok'
		print result
		finalr = compute(evalid, result, duration)
		print finalr
		opt = ''
		if evalid == 2:
			opt = 'failover time(server-side): '
			opt = opt + str(failovertime*1000) + 'ms'
			print opt
		logresult(hint, result, finalr, opt)
		return True

def compute(eid, result, duration):
	finalr = ''
	if eid == 1 or eid == 3:
		n1 = 0
		n2 = 0
		for i in result:
			n1 += int(i)
			n2 += duration*1000/int(i)
		n1 = n1/duration
		n2 = n2/len(result)
		finalr = 'average throughput: ' + str(n1) + 'ops/s\n'
		finalr += 'average delay: ' + str(n2) + 'ms/op'
		return finalr
	elif eid == 2:
		n = 0
		for i in result:
			n += int(i)
		n = n/len(result)
		finalr = 'average failover time(client-side): ' + str(n) + 'ms'
		return finalr
	else:
		print 'unknown evalid'
		return finalr

resultfile = None

def logresult(hint, result, finalr, opt):
	resultfile.write(hint)
	resultfile.write('\n')
	resultfile.write(str(result))
	resultfile.write('\n')
	resultfile.write(finalr)
	resultfile.write('\n')
	if opt != '':
		resultfile.write(opt)
		resultfile.write('\n')

def end():
	for i in snodes:
		i.work('end')
		i.close()
	resultfile.write('#all eval done#')
	resultfile.close()

def evaluate_i():
	#eval-id: 1, 2, 3, 4(end)
	#client-num: 10, 20, 30, 40, 50...
	#duration: 60
	#rw_ratio: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
	while 1:
		next_eval = raw_input('eval(eid, cn, duration, ratio):\n')
		ok = raw_input('is '+next_eval + '?\n')
		if ok == 'yes':
			doeval = next_eval.split()
		else:
			continue
		if doeval[0] == '1' or doeval[0] == '2' or doeval[0] == '3':
			if prepare() == 'fail':
				clear()
				continue
			eval_(int(doeval[0]), int(doeval[1]), int(doeval[2]), int(doeval[3]))
			clear()
		elif doeval[0] == '4':
			end()
			return
		else:
			print 'unknown eval'

def start(interactive):
	evalprepare()
	make_eval_network()
	if interactive:
		evaluate_i()
	else:
		evaluate()

def evaluate():
	#eval-1: 5 15 25 35
	print "eval-1 start"
	step = 10
	redo = False
	nc = 5
	while nc <= 15:
		if prepare() == 'fail':
			clear()
			if redo:
				continue
			else:
				print "eval-1 stopped at " + str(nc) + ' nodes'
				#return or break?
				return
		result = eval_(1, nc, 10, 3)
		clear()
		if not result:
			if redo:
				continue
			else:
				print "eval-1 stopped at " + str(nc) + ' nodes'
				#return or break?
				return
		else:
			nc += step
	print "eval-1 end"

	#eval-2: 5 15 25 35
	print "eval-2 start"
	step = 10
	redo = False
	nc = 5
	while nc <= 15:
		if prepare() == 'fail':
			clear()
			if redo:
				continue
			else:
				print "eval-2 stopped at " + str(nc) + ' nodes'
				#return or break?
				return
		result = eval_(2, nc, 10, 3)
		clear()
		if not result:
			if redo:
				continue
			else:
				print "eval-2 stopped at " + str(nc) + ' nodes'
				#return or break?
				return
		else:
			nc += step
	print "eval-2 end"

	#eval-3: 0% 20% 40% 60% 80% 100%
	print "eval-3 start"
	step = 5
	redo = False
	ratio = 0
	while ratio <= 10:
		if prepare() == 'fail':
			clear()
			if redo:
				continue
			else:
				print "eval-3 stopped at " + str(nc) + ' nodes'
				#return or break?
				return
		result = eval_(3, 10, 10, ratio)
		clear()
		if not result:
			if redo:
				continue
			else:
				print "eval-3 stopped at " + str(ratio*10) + "%"
				break;
		else:
			ratio += step
	print "eval-3 end"


if __name__ == '__main__':
#i: interactive-mode
	if len(sys.argv) == 1:
		start(False)
	elif len(sys.argv) == 2:
		if sys.argv[1] == 'i':
			start(True)
		else:
			start(False)
			end()
	else:
		print 'too many or too few arguments'

