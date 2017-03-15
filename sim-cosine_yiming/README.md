Social Filtering Recommendation Framework
=========================================


Version
-------

0.1, February 7th, 2015


Installation
------------

### Requirements:

The use of this framework requires the following libraries:

1. GCC 4.7 or above (or equivalent C++11 compiler)
2. Eigen (http://eigen.tuxfamily.org/)
3. Boost / Program Options (http://www.boost.org/)
4. Python with the packages Numpy, Scipy, Pandas


### Installation:

First verify if all the requirements are met. If not follow the installation procedure for each one.

Secondly, edit the file *setup.mk* in the root folder to specify the correct path for the include files (Eigen and Boost) and libraries (Boost). Suppose the Eigen and Boost include files are found in the folder *path/to/include* and their libraries to *path/to/lib*. In this case you should change the following lines accordingly:

	INCLUDE_FLAGS = -Ipath/to/include 
	LIB_FLAGS     = -Lpath/to/lib

Then launch Makefile in the root folder:

	make



Usage
-----

Given a list of target individuals (to whom one wishes to recommend) and a list of recommendable items the social filtering method will compute a score for each couple (user,item) and rank them.

First, compute the similarity graph between users (or items) from the ratings matrix using one of the the following metrics:

* Standard or asymmetric cosine: `bin/sim-cosine`
* Jaccard: `bin/sim-jaccard`
* Bigrams (association rules of length 2): `apriori/apriori-simple.py` 

Then, use the similarity graph and the ratings matrix to generate the recommendations ranked by score with the program `bin/sf`. Program usage and parameters available with the option `-?`.


Authors
-------

   * Daniel BERNARDES (daniel.bernardes@univ-paris13.fr)
   * Mamadou DIABY (mamadou.diaby@univ-paris13.fr)
   * Raphaël FOURNIER-S'NIEOTTA (fournier@cnam.fr)
   * Françoise FOGELMAN-SOULIÉ (soulie@univ-paris13.fr)
   * Emmanuel VIENNET (emmanuel.viennet@univ-paris13.fr)

This work has been done at L2TI (http://www-l2ti.univ-paris13.fr/site/index.php/en/)


License
-------
Copyright (c) 2014, the authors.

Copyrights licensed under the New BSD License. See the accompanying COPYRIGHT file for terms.
