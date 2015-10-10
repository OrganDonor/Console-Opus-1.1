#!/usr/bin/env python

# Organ Donor Deployment tool
# 2015-06-16 ptw
#
# Takes a CSV file on the command line, which contains the rank name and note number
# of each position in the windchest. No blank lines or labels. Like this:
#4,18
#8,16
#8,6
#8,11
#4,3
#
# Generates Arduino code to create a table of which MTP is used for each note in each rank.
# Like this:
#// MTP Assignments by note in each rank.
#// Converted from file deployment-2015-fair.txt on 2015-06-16T08:20:17.698704 by ./deploy.py
#const byte MTP_table[2][62] PROGMEM = {
#// Rank 4
#{ 0,  0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, },
#// Rank 8
#{ 0,  0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, },
#};
#
import sys
import csv
import datetime

# This function is specific to the Opus 1 windchest.
# Hole positions 1..10 are on MTP 0, 11..20 on MTP 1, and so on.
def MTP_for_position(pos):
	return ((pos-1)/10) % 2

ranks = set()
config = {}

with open(sys.argv[1], 'rb') as csvfile:
	deployment = csv.reader(csvfile)
	position = 1
	for row in deployment:
#		print "Position %d is rank %s, note %d" % (position, row[0], int(row[1]))
		
		ranks.add(row[0])
		config[(row[0],int(row[1]))] = position

		position += 1

#print "Found %d ranks:" % len(ranks)
#print ranks

#for r in ranks:
#	for i in range(1,62):
#		print "%s-%d is %d on MTP %d" % (r, i, config[(r,i)], MTP_for_position(config[(r,i)]))

print "// MTP Assignments by note in each rank."
print "// Converted from file %s on %s by %s" % (sys.argv[1], datetime.datetime.now().isoformat(), sys.argv[0])
print "const byte MTP_table[%d][62] PROGMEM = {" % len(ranks)
for r in sorted(ranks):
	print "// Rank %s" % r
	print "{ 0, ",		# extra 0 to take up array position 0; notes are 1 to 61
	for i in range(1,62):
		print "%d," % MTP_for_position(config[(r,i)]),
	print "},"
print "};"
