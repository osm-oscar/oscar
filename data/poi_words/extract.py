import math
import argparse
import xml.dom.minidom
import itertools
import codecs
import sys
import enum
#
# {
#	basePart : String
#	relationPart: String
#	spl : "singular" | "plural"
#	rel : "none" | "in" | "near"
# }

class SPL(enum.Enum):
	SINGULAR = "singular"
	PLURAL = "plural"
	def __str__(self):
		return self.value

class SpatialRelation(enum.Enum):
	NONE = "none"
	NEAR = "near"
	IN = "in"
	def __str__(self):
		return self.value

sys.stdout = codecs.getwriter('utf-8')(sys.stdout) 

cmdLineParser = argparse.ArgumentParser(description="Nix")
cmdLineParser.add_argument('filename',  metavar='filename',  help="Name of the file to process",  type=str,  nargs='+')
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

baseQS = dict() #String -> [SPL, tag_key, tag_value]
spatialQS = [] #[String, SPL, SpatialRelation]

for l in filter(lambda x: x.startswith("| "), lines):
	tokens = l[2:].split('||')
	tokens = list(map(lambda x: x.strip(' '), tokens))
	qs = tokens[0]
	tag_key = tokens[1]
	tag_value = tokens[2]
	rel = tokens[3]
	spl = tokens[4]

	if (spl == "Y"):
		spl = SPL.PLURAL
	else:
		spl = SPL.SINGULAR

	if (rel == "-"):
		rel = SpatialRelation.NONE
	elif (rel == "near"):
		rel = SpatialRelation.NEAR
	elif (rel == "in"):
		rel = SpatialRelation.IN
	else:
		rel = SpatialRelation.NONE

	if rel == SpatialRelation.NONE:
		baseQS[qs] = [spl, tag_key, tag_value]
		continue
	
	spatialQS.append([qs, spl, rel, tag_key, tag_value])

print("{")
print("\"base\": [")
for qs, [spl, tag_key, tag_value] in baseQS.items():
	print(str("{"))
	print("\tbasePart: \"" + qs + "\"")
	print("\trelationPart: \"\"")
	print("\tspl: \"" + str(spl) + "\"")
	print("\trel: \"none\"")
	print("\ttag_key: \"" + tag_key + "\"")
	print("\ttag_value: \"" + tag_value + "\"")
	print("},")
print("]")
if False:
	print("\"spatial\": [")
	for qs, spl, rel, tag_key, tag_value in spatialQS:
		basepart = qs
		relpart = ""
		print(str("{"))
		print("\tbasePart: \"" + basepart + "\"")
		print("\trelationPart: \"" + relpart + "\"")
		print("\tspl: \"" + str(spl) + "\"")
		print("\trel: \"" + str(rel) + "\"")
		print("\ttag_key: \"" + tag_key + "\"")
		print("\ttag_value: \"" + tag_value + "\"")
		print("},")
	print("]")
print("}")