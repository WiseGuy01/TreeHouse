#include "matrix.h"
#include "vec.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <utility>

using namespace std;

vector< pair<double,size_t> > calcDistance(Vec& testPoint, Vec& stDevs, Matrix& mat)
{
	vector< pair<double,size_t> > points;
	for (size_t i = 0; i < mat.rows(); i++)
	{
		double distSum = 0.0;
		for (size_t j = 0; j < mat.cols(); j++)
		{
			double val1 = testPoint[j];
			double val2 = mat[i][j];
			double distDimj = 0.0;

			// Calculate Hamming distance
			if (mat.attrTypes[j] != 0)
			{
				if (val1 == val2)
					distDimj = 0.0;
				else
					distDimj = 1.0;
			}
			// Calculate Mahalanobis distance
			else
			{
				double diff = val1 - val2;
				distDimj = (diff*diff)/(stDevs[j]*stDevs[j]);
			}
			distSum += distDimj;
		}

		double distance = sqrt(distSum);
		pair <double,size_t> tmpPair;
		tmpPair = make_pair(distance,i);
		points.push_back(tmpPair);
		cout << tmpPair.first << " " << tmpPair.second << endl;
	}
	return points;
}

int main()
{
	// Some important input stuff
	string inTrainFeats;
	string inTrainLabs;
	string inTestFeats;
	Matrix mat;
	Matrix trainLabs;
	Matrix testFeats;

	// Actually getting filenames
	size_t k = 0;
	while (k < 1)
	{
		cout << "Feed me number of neighbors(integer >= 1, please): ";
		cin >> k;
	}
	cout << "Feed me training features: ";
	cin >> inTrainFeats;
	cout << "Feed me training labels: ";
	cin >> inTrainLabs;
	cout << "Feed me test features: ";
	cin >> inTestFeats;

	// Actually importing data from files
	mat.loadARFF(inTrainFeats);
	trainLabs.loadARFF(inTrainLabs);
	testFeats.loadARFF(inTestFeats);

	// Print data matrix because reasons
	mat.print(cout);
	cout << endl;
	trainLabs.print(cout);
	cout << endl;
	testFeats.print(cout);
	cout << endl;

	// Calculate some standard deviations/expected error
	size_t numcols = mat.cols();
	Vec stDevs(numcols);
	for (size_t i = 0; i < numcols; i++)
	{
		double stdev = mat.columnStDev(i);
		stDevs[i] = stdev;
		cout << "Column " << i << " standard deviation is " << stDevs[i] << endl;
	}

	// Actual nearest neighbor calculations
	for (size_t outer = 0; outer < testFeats.rows(); outer++)
	{
		// Calculate Distances
		Vec testPoint;
		testPoint.copy(testFeats[outer]);
		vector< pair<double,size_t> > points;
		points = calcDistance(testPoint, stDevs, mat);
		
		// Sort distances and find k shortest
		sort(points.begin(),points.end());
		vector< pair <double,double> > kLsDs;
		for (size_t i = 0; i < trainLabs[0].size(); i++)
		{
			cout << endl;
			for (size_t j = 0; j < k; j++)
			{
				pair <double,double> tmpPair;
				tmpPair = make_pair(trainLabs[points[j].second][i],points[j].first);
				kLsDs.push_back(tmpPair);
				cout << kLsDs[j + 3*i].first << " " << kLsDs[j + 3*i].second << endl;
			}
		}
	}

	return 0;
}
