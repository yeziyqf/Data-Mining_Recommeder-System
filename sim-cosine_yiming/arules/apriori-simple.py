#!/usr/bin/env python
# *-* coding: utf-8 *-*
"""
Generates association rules with confidence with a naif algorithm
Inputs:
 -- User-product matrix path (lines: user product_1 ... product_n)
 -- Support threshold \in [0,1] (optional)
 -- Rule confidence threshold \in [0,1] (optional)
Output: 2-rule matrix (lines: product_src product_dest confiance)

DB 2014-04
"""

__author__ =  'Daniel Bernardes, Mamadou Diaby, Raphael Fournier, Francoise Fogelman Soulie and Emmanuel Viennet'
__version__=  '1.0'

import sys, pandas
import numpy as np
from scipy import sparse

# setup
thresholdsupp = 0 # default: all associations
thresholdconf = 0 # default: all rules

if __name__ == "__main__":

    if len(sys.argv) > 1:
        uivpath = sys.argv[1]
    else:
        print "Not enough arguments"
        exit(-1);

    if len(sys.argv) > 2:
        thresholdsupp = float(sys.argv[2])

    if len(sys.argv) > 3:
        thresholdconf = float(sys.argv[3])

    # calculation
    uivlist  = pandas.read_csv(uivpath,sep=' ',header=None,
                               names=['user','item','value'])
    num_users = uivlist['user'].max()+1
    num_items = uivlist['item'].max()+1
    itemcount = uivlist['item'].value_counts().sort_index(1)
    
    uivmatrix = sparse.csc_matrix((uivlist['value'].values > 0, # replace rating by '1'
                                   (uivlist['user'].values,uivlist['item'].values)),
                                  shape=(num_users,num_items), dtype=int)

    # focus on the items having frequency equal or higher than thresholdsupp
    ### NB: one could construct the uivmatrix with the popular items, instead of
    ### contructing it generally and selecting the popular items.
    popularitems = np.flatnonzero(itemcount.values >= num_users*thresholdsupp)
    support = uivmatrix[:,popularitems].T * uivmatrix[:,popularitems]
    support = support - sparse.diags(itemcount.values,0)

    diaginv = sparse.csc_matrix((1.0/itemcount.values.astype(float),
                                 (np.arange(num_items),np.arange(num_items))),
                                shape=(num_items,num_items))

    confidence = sparse.coo_matrix(diaginv*support)

    # output results
    for user,item,conf in zip(confidence.row, confidence.col, confidence.data):
        print '{0:d} {1:d} {2:.5f}'.format(user,item,conf)
