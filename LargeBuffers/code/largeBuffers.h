#pragma once
#include "data.h"
#include <queue>


class largeBuffers
{
public:
	std::vector<int> proceedToMerge(disk inputFile, std::string runName, size_t bufferSize, size_t numBuffers, disk finalOutput);

	void validation(disk finalOutput);
private:
	std::vector<disk> createRuns(disk inputFile, std::string runName, size_t recordsPerRun, int& counter);

	int mergeRunes(std::vector<disk> inputRunFiles, disk outputFile);

	std::vector<int> performMerging(std::vector<disk> initialRuns, disk finalOutput, size_t numBuffers);

	void fileCheck(disk file);
};

