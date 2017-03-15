#!/usr/bin/env python
# *-* coding: utf-8 *-*
"""
Generate recommendations using association-rule confidence matrix
Inputs:
 -- List of owned items per user. Format: user item1 ... itemK
 -- Length 2 association rules (confidence matrix). Format: item item confidence
 -- Target users list. Format: user
 -- Number of items to recommend K
Output: list of [K] recommendations for each user

DB 2014-04
"""

__author__ =  'Daniel Bernardes, Mamadou Diaby, Raphael Fournier, Francoise Fogelman Soulie and Emmanuel Viennet'
__version__=  '1.0'

import sys, argparse, datetime
import operator
from scipy import sparse
from pandas import read_csv

DEFAULT_CONF = 0.0 # default confidence value for unspecified association rules

def recommend(owneditems, arules, limit_rec=0, includeconf=False):
	"""
	given a set of owned items and an association rule confidence matrix, recommend items
	"""
	suggestions = {}
	get_conf = suggestions.get
	for owneditem in owneditems:
		newitems = set(arules[owneditem].indices) - owneditems # do not recommend owned items
		for newitem in newitems:
			suggestions[newitem] = max(get_conf(newitem, DEFAULT_CONF),arules[owneditem,newitem])
			# print 'rec to user {0}: {1} -> {2} with confidence {3}'.format(
			# 	user, owneditem, newitem, arules[owneditem,newitem])

	suggestions_sorted = sorted(suggestions.iteritems(),key=operator.itemgetter(1), reverse=True)

	if limit_rec == 0: # no limit
		limit_rec = len(suggestions_sorted)
	if includeconf == True:
		return suggestions_sorted[0:limit_rec]
	else:
		return map(operator.itemgetter(0), suggestions_sorted[0:limit_rec])

def loadSparseMatrix(infile, dtype=int, shape=None):
	"""
	load sparse from path using pandas, return scipy coo_matrix
	"""
	vallist  = read_csv(infile,sep=' ',header=None,names=['row','col','val'])
	if shape == None:
		shape = (vallist['row'].max()+1, vallist['col'].max()+1)
	return sparse.coo_matrix((vallist['val'].values,(vallist['row'].values,vallist['col'].values)),
							 dtype=dtype,shape=shape)

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument('--arules',type=argparse.FileType('r'),required=True,
						help='path of the association rules list')
#	parser.add_argument('--targetusers',type=argparse.FileType('r'),required=True,
#						help='path of the target user list')
	parser.add_argument('--num_items',type=int,default=0,required=True,
						help='total number of items')
	parser.add_argument('--limit_rec',type=int,default=0, # no limit
						help='number of recommended items limit')
	parser.add_argument('user_itemlist',type=argparse.FileType('r'),default=sys.stdin,
						nargs='?',help='User itemlist file path or stdin stream')
	parser.add_argument('--outfile',type=argparse.FileType('w'),default=sys.stdout,
						nargs='?',help='Output file')
	params = parser.parse_args()

	def tsprint(msg): print >> sys.stderr, datetime.datetime.utcnow(), msg

#	tsprint('Reading target user list...')
#	targetuserlist = read_csv(params.targetusers,sep=' ',header=None,names=['user'])
#	targetusers = targetuserlist['user'].values
#	tsprint('Done. :%s users' %(len(targetusers)))

	tsprint('Reading association rules matrix...') ## test sparse formats
	arules = loadSparseMatrix(params.arules,dtype=float,
							  shape=(params.num_items,params.num_items)).tocsr() 
	tsprint('Done.')

	tsprint('Generating recommendations...')
	# Assert: 'targetusers', 'user_itemlist' sorted by user in ascending order
	#current = 0
	for line in params.user_itemlist:
		tokens = map(int,line.split())
		user = tokens[0];
#		print "user", user
#		print "targetusers[current]", targetusers[current]
#		if user != targetusers[current]:
#			continue
		owneditems = set(tokens[1:])
		recommendations = recommend(owneditems, arules, params.limit_rec, includeconf=True)
		#print >>params.outfile, user, ' '.join(map(str,recommendations))
		for (item, score) in recommendations:
			print >>params.outfile, user, item, score
		#current += 1 # move on to the next targeted user
	tsprint('Done.')
