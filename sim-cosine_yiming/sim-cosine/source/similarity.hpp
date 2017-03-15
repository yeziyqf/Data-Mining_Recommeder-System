/*
	Similarity measures
	DB 2014-10
*/
#ifndef SIMILARITY_HPP
#define SIMILARITY_HPP
#include <limits>
#include <Eigen/Core> 
#include <Eigen/SparseCore> 

typedef float rating_t;
const float UNDEFTH=std::numeric_limits<rating_t>::min();

using Eigen::Triplet;
using Eigen::VectorXf;
typedef Eigen::SparseMatrix<float, Eigen::RowMajor, std::ptrdiff_t> SparseXf;
typedef Eigen::SparseMatrix<bool,  Eigen::RowMajor, std::ptrdiff_t> SparseXb;
typedef Eigen::SparseMatrixBase<
		Eigen::SparseMatrix<float, 1, std::ptrdiff_t> >::InnerVectorReturnType innerVec;

rating_t cosine_sim(innerVec targetv, innerVec ratingv, const float alpha,
	const int locality, const rating_t suppthreshold);

#endif