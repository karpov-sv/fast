#!/usr/bin/env python
#-*- coding:utf-8 -*-

"""Module for working with the Favor Inter Module Message Protocol
which is described at http://lynx.sao.ru/favor/wiki/InterModuleMessagesFormat
"""

import re


__MESSAGE_FORMAT = re.compile('([a-zA-Z0-9-_]+)=(([a-zA-Z0-9-+,\\._]+)|([\'"])(.*?[^\\\\])\\4|)')


class Message:
        def __init__(self, mtype, kwargs={}):
		self.mtype = mtype
		self.kwargs = kwargs

	def __eq__(self, message):
		if message is None: return False
		elif message.mtype != self.mtype: return False
		elif message.kwargs != self.kwargs: return False
		else: return True

        def name(self):
                return self.mtype

        def get(self, key, value=None):
                return self.kwargs.get(key, value)

        def args(self):
                s = []
		for k in self.kwargs:
			v = self.kwargs[k]
			v = v.replace('\\', '\\\\')
			v = v.replace('\"', '\\\"')
			v = v.replace('\'', '\\\'')
			v = v.replace('\t', '\\t')
			s.append(k + '="' + v + '"')
		return ' '.join(s)

        def __str__(self):
		return self.mtype + ' ' + self.args()

def parse(line):
	"""Performs parsing of the IMMP message."""
	chunks = line.split(None, 1)
	if len(chunks) == 0:
		raise ValueError, 'IMMP Message should have type as a first word'
	mtype = chunks[0]
	if len(mtype) == 0:
		raise ValueError, 'IMMP Message type should be a non-empty string'

        if __MESSAGE_FORMAT.findall(chunks[0]):
                chunks = ["none", line]
                mtype = "none"

        kwargs = {}
	if len(chunks) == 2:
		for k, v, _, _, _ in __MESSAGE_FORMAT.findall(chunks[1]):
                        if len(v) > 1:
			        v = v[0] in '\'\"' and v[1:-1] or v
			v = v.replace('\\\\', '\\')
			v = v.replace('\\\"', '\"')
			v = v.replace('\\\'', '\'')
			v = v.replace('\\t', '\t')
			kwargs[k] = v
	return Message(mtype, kwargs)

if __name__ == '__main__':
        #command = parse("fast_status cap=sky lgt=      mir=EMCCD flt=O pol=2xW blk=BVR ")
        command = parse("Alpha=02_34_45.99 Delta=+57_47_39.7 AzCur=-217_13_08.9 ZdCur=+59_35_59.9 P2=172.079 PA=340.969 Mtime=23_42_53 Stime=16_18_37")
        print command.name(), command.kwargs
