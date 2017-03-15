# !/bin/bash
kValues=(130)
alphaList=(0.2)

result_dir="result_16_12_22_q1_top130_5cv_90%sp_1/"
origin_dir="/home/workspace/codes/recommender_system/social_filtering/90%_spliting_data"
mkdir ${result_dir}

for k in ${kValues[*]}
do
{
	for a in ${alphaList[*]}
	do 
	{
		echo "k = top${k}, a = ${a}"

		nk="top${k}_"

		na="90%sp"

		target_dir=${result_dir}${nk}${na}
		# origin_dir="result_16_11_03_1/"${nk}${na}
		mkdir ${target_dir}

		for((i=1;i<=1;i++))
		do
		{
		echo "$i times..."
		# python split_data.py ${target_dir} ${i}
		final_dir=${target_dir}"/result_"${i}
		mkdir ${final_dir}

		train_iuv=${origin_dir}"/ratings_train_iuv.dat_"${i}
		train=${origin_dir}"/ratings_train.dat_"${i}
		test_eval=${origin_dir}"/ratings_test_eval.dat_"${i}
		test_input=${origin_dir}"/ratings_test_input.dat_"${i}
		target_users=${origin_dir}"/target_users.dat_"${i}

		beha=${final_dir}"/beha_1st.cos_base"
		simg=${final_dir}"/simg.gph_base"

 		echo "before generate similarity, mem used is:"
                # free -m|grep -i mem | awk '{print $3/$2*100,"%"}'
                free -m|grep -i mem | awk '{print $3}'
		./sim-cosine_yiming/bin/sim-cosine -u 3953 -i 6041 -k ${k} -a $a -s 4 -r ${train_iuv} -t ${train_iuv} 1>${beha}
		echo "After generate similarity, mem used is:" 
                free -m|grep -i mem | awk '{print $3}'

		alpha=1

		# beha2=${target_dir}"/beha_1st.cos_base_"${i}".tmp"
		# python sim_q.py ${beha} ${p} 5

		python complete_graph.py ${beha} ${simg}
		echo "After complete_graph, mem used is:"
                free -m|grep -i mem | awk '{print $3}'
		recw=${final_dir}"/recwname"
		reca=${final_dir}"/recaname"

		./socialfiltering -u 6041 -i 3953 -r ${train} -t ${target_users} -l ${test_input} -g ${simg} -k 5  -b 1 -a ${recw} 1>${reca}
		echo "After socialfiltering, mem used is:"
                free -m|grep -i mem | awk '{print $3}'
		fn=${final_dir}"/SF_"${nk}${na}"_"$sim_value"_result.txt"
		python evaluate_with_orgin.py ${recw} ${reca} $alpha ${origin_dir} ${i} >> ${fn}
		
		}&
		done

	}&
	done
}&
done
