#!/usr/bin/env python
# *-* coding: utf-8 *-*
"""
Generates association rules with confidence using the Apriori algorithm
(FIM package wrapper)
Input: User-product matrix (lines: user product_1 ... product_n)
Output: 2-rule matrix (lines: product_src product_dest confidence)
Command line options: Support&Conf threshold

DB 2014-04
"""

__author__ =  'Daniel Bernardes, Mamadou Diaby, Raphael Fournier, Francoise Fogelman Soulie and Emmanuel Viennet'
__version__=  '1.0'

import sys, fim

if __name__ == "__main__":

    if len(sys.argv) > 1:
        threshold = 100*float(sys.argv[1]);
    else:
        threshold = 1 # default: 1%

    fin = sys.stdin;
    profiles = []
    for line in fin:
        tokens = line.split()
        profiles.append(map(int,tokens[1:]))
    fin.close()

    confidence = fim.apriori(profiles, max=2, supp=threshold,
                             report='e', eval='c', thresh=threshold)

    for triplet in confidence:
        print '{0:d} {1:d} {2:.5f}'.format(triplet[0][0],triplet[0][1],triplet[1][0])
