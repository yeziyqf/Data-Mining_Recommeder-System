import pandas as pd
import sys


sim_infile_path = sys.argv[1]
sim_outfile_path = sys.argv[2]

sim = pd.read_csv(sim_infile_path, delimiter=" ", names=['i', 'j', 'v'])
# max_user1 = sim['user1'].max()
# max_user2 = sim['user2'].max()
# max_num_user = max_user1 if max_user1 > max_user2 else max_user2
#sim = pd.nan_to_num(sim)
total = 3953

degrees = sim['i'].value_counts().sort_index(0)
dict_degrees = degrees.to_dict()
for i in xrange(total):
    if not dict_degrees.has_key(i):
        dict_degrees[i] = 0

f = open(sim_outfile_path, "w")
sim_input = open(sim_infile_path,'r')
print >> f, total
for key, value in dict_degrees.iteritems():
    print >> f, str(key) + ' ' + str(value)
print >> f, (sim_input.read()).strip()
f.close()
sim_input.close()
