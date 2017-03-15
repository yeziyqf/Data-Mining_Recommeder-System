/*
	Similarity measures
	DB 2014-04-20
*/
#include <functional>
#include "similarity.hpp"

using namespace std;

rating_t cosine_sim(innerVec targetv, innerVec ratingv, const float alpha,
 const int locality, const rating_t suppthreshold) {
	// cout << targetv << ratingv << "\n\n";
	rating_t sim = targetv.dot(ratingv);
	if (sim < suppthreshold) {
		return 0.0;
	} else {
		sim /= (pow(targetv.squaredNorm(),alpha)*pow(ratingv.squaredNorm(),1.0-alpha));
		return pow(sim, locality);
	}
}

/// implementations valid for binary matrices only ///

// inline rating_t AR_sim(innerVec targetv, innerVec ratingv, const float alpha=1.0,
//  const rating_t suppthreshold = UNDEFTH, const rating_t confthreshold = UNDEFTH) {
// 	assert(targetv.nonZeros() == targetv.squaredNorm());
// 	assert(ratingv.nonZeros() == ratingv.squaredNorm());
// 	rating_t sim = targetv.dot(ratingv);
// 	if (sim < suppthreshold) {
// 		return 0.0;
// 	} else {
// 		sim /= (pow(targetv.nonZeros(),alpha)*pow(ratingv.nonZeros(),1.0-alpha));
// 		return sim < confthreshold? 0.0 : sim;
// 	}
// }

rating_t jaccard_sim(innerVec targetv, innerVec ratingv,const rating_t suppthreshold) {
	assert(targetv.nonZeros() == targetv.squaredNorm());
	assert(ratingv.nonZeros() == ratingv.squaredNorm());
	rating_t sim = targetv.dot(ratingv);
	if (sim < suppthreshold) {
		return 0.0;
	} else {
		sim /= (targetv.nonZeros()+ratingv.nonZeros()-sim);
		return sim;
	}
}
