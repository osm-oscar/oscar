import math
import argparse
import xml.dom.minidom
import itertools
import codecs
import sys

sys.stdout = codecs.getwriter('utf-8')(sys.stdout) 

cmdLineParser = argparse.ArgumentParser(description="Nix")
cmdLineParser.add_argument('filename',  metavar='filename',  help="Name of the file to process",  type=str,  nargs='+')
cmdLineParser.add_argument('-u', action='store_true',  dest='unique',  help='Unique output')
cmdLineParser.add_argument('-w', action='store_true',  dest='unique',  help='First word until space')
cmdLineParser.add_argument('-kv', action='store_true',  dest='kvonly',  help='Key=Values only')
parsedCmdLine = cmdLineParser.parse_args()

def getText(nodelist):
	rc = []
	for node in nodelist:
		if node.nodeType == node.TEXT_NODE:
			rc.append(node.data)
	return ''.join(rc)

#print  repr(parsedCmdLine)

dom = xml.dom.minidom.parse(parsedCmdLine.filename[0])
textElement = dom.getElementsByTagName('text')[0]
lines = getText(textElement.childNodes).splitlines()

outLines = []

for l in itertools.ifilter(lambda x: x.startswith("| "), lines):
	tokens = l[2:].split('||')
	tokens = map(lambda x: x.strip(' '), tokens)
	begin = 0
	if (parsedCmdLine.kvonly):
		begin = 1
	outLines.append(tokens[begin:3])
	myLine = []
	for i in range(begin, 3):
		myLinetokens[i]
