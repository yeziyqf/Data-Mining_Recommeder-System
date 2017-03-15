#!/bin/python
import pandas as pd
import sys
def compare(ltype,drec,devam,target_user):
	recall = 0
	precesion = 0
	rec_user = drec.keys()
	for i in rec_user:
	    t = set(deva[i])
	    r = set(drec[i])
	    k = len(r)
	    c_num = len(t.intersection(r))
	    recall +=  c_num * 1.0 / len(t)
	    precesion += c_num * 1.0 / k
	recall = recall / len(rec_user)
	precesion = precesion / len(rec_user)
	user_coverage = len(rec_user) * 1.0/ len(target_user)
	print mark, ltype,round(precesion,3), round(recall,3),round( user_coverage,3)

recwpath = sys.argv[1]
recapath = sys.argv[2]

mark = sys.argv[3]

read_path = sys.argv[4]
times = sys.argv[5]

recw = pd.read_csv(recwpath,sep=' ',names=list('uiv'))
reca = pd.read_csv(recapath,sep=' ',names=list('uiv'))
target_user = pd.read_csv(read_path+'/target_users.dat_' +times,sep=' ',header=None)
# print target_user
target_user = target_user[0]

drecw ={k: g["i"].tolist() for k,g in recw.groupby("u")}
dreca ={k: g["i"].tolist() for k,g in reca.groupby("u")}
eva = pd.read_csv(read_path+'/ratings_test_eval.dat_'+times,sep=' ',names=list('uiv'))
deva ={k: g["i"].tolist() for k,g in eva.groupby("u")}

compare('w',drecw,deva,target_user)
compare('a',dreca,deva,target_user)
