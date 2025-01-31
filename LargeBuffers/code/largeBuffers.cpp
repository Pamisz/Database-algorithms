#include "largeBuffers.h"

std::vector<int> largeBuffers::proceedToMerge(disk inputFile, std::string runName, size_t bufferSize, size_t numBuffers, disk finalOutput) {
	size_t recordPerRun = bufferSize * numBuffers;
	int operationsCounter = 0;
	std::vector<disk> runs = createRuns(inputFile, runName, recordPerRun, operationsCounter);
	int phaseCounter = runs.size();
	std::vector<int> buff = performMerging(runs, finalOutput, numBuffers);
	phaseCounter += buff[0];
	operationsCounter += buff[1];
	for (disk file : runs) {
		file.autoDestruction();
		
	}
	disk tmp("tmp.bin", 20);
	tmp.autoDestruction();

	buff[0] = phaseCounter;
	buff[1] = operationsCounter;
	return buff;
}

std::vector<disk> largeBuffers::createRuns(disk inputFile, std::string runName, size_t recordsPerRun, int& counter) {
	std::vector<disk> runs;
	std::vector<Coordinates> buffer;
	buffer.reserve(recordsPerRun);

	size_t blockSize = inputFile.getBlockSize();
	size_t totalRecordsProcessed = 0;
	size_t runIndex = 0;

	while (true) {
		Block blockData = inputFile.readBlock(totalRecordsProcessed / blockSize);  
		counter++;
		if (blockData.coordinates.empty()) {
			break; 
		}

		for (size_t i = 0; i < blockData.size(); i ++) {  
			buffer.emplace_back(blockData.coordinates[i]);
			if (buffer.size() == recordsPerRun) {
				quicksort(buffer, 0, buffer.size() - 1);

				std::string runFileName = runName + std::to_string(runIndex++) + ".bin";
				disk outFile(runFileName, blockSize);
				runs.push_back(outFile);

				outFile.pushBlock(Block(buffer));
				counter++;

				std::cout << "\nRun: " << runIndex << " has been successfully created with: " << recordsPerRun << " records\n" << std::endl;
				buffer.clear();
				fileCheck(outFile);
			}
		}
		totalRecordsProcessed += blockSize;
	}

	if (!buffer.empty()) {
		quicksort(buffer, 0, buffer.size() - 1);

		std::string runFileName = runName + std::to_string(runIndex++) + ".bin";
		disk outFile(runFileName, blockSize);
		runs.push_back(outFile);

		outFile.pushBlock(Block(buffer));
		counter++;

		std::cout << "\nRun: " << runIndex << " has been successfully created with: " << buffer.size() << " records\n";
		fileCheck(outFile);
	}

	std::cout << std::endl;
	return runs;
}

int largeBuffers::mergeRunes(std::vector<disk> inputRunFiles, disk outputFile) {
	size_t numRuns = inputRunFiles.size();
	if (numRuns < 2) {
		std::cerr << "Not enough runs to merge.\n";
		return 0;
	}

	struct QueueElement {
		Coordinates coord;
		size_t runIndex;

		QueueElement(Coordinates c, size_t rIdx)
			: coord(c), runIndex(rIdx) {}

		bool operator<(const QueueElement& other) const {
			return coord < other.coord;
		}
	};

	std::vector<size_t> blocksPos(numRuns, 0);
	std::priority_queue<QueueElement> pq;
	std::vector<Block> blocks(numRuns);

	//Get block from each run and place first records from blocks i pq (from smallest)
	for (size_t i = 0; i < numRuns; i++) {
		blocks[i] = inputRunFiles[i].readBlock(blocksPos[i]);
		blocksPos[i]++;
		if (!blocks[i].coordinates.empty()) {
			pq.emplace(blocks[i].popFirst(), i);
		}
	}

	int operationsCounter = numRuns;

	Block buffer;
	while (!pq.empty()) {
		QueueElement smallest = pq.top();
		pq.pop();

		buffer.addCoordinates(smallest.coord);

		if (buffer.size() == outputFile.getBlockSize()) {
			outputFile.pushBlock(buffer);
			operationsCounter++;
			buffer.clear();
		}

		if (blocks[smallest.runIndex].size()) {
			QueueElement val(blocks[smallest.runIndex].popFirst(), smallest.runIndex);
			pq.emplace(val);
		}
		else {
			// If block is empty we got to read the next one
			blocks[smallest.runIndex] = inputRunFiles[smallest.runIndex].readBlock(blocksPos[smallest.runIndex]);
			operationsCounter++;
			if (!blocks[smallest.runIndex].coordinates.empty()) {
				blocksPos[smallest.runIndex]++;
				QueueElement val(blocks[smallest.runIndex].popFirst(), smallest.runIndex);
				pq.emplace(val);
			}
		}
	}

	if (buffer.size() > 0) {
		outputFile.pushBlock(buffer);
		operationsCounter++;
	}

	return operationsCounter;
}

std::vector<int> largeBuffers::performMerging(std::vector<disk> initialRuns, disk finalOutput, size_t numBuffers) {
	if (numBuffers < 2) {
		std::cerr << "Not enough buffers to perform merging!" << std::endl;
		return {};
	}

	int mergingCounter = 0, operations = 0;
	std::vector<disk> currentRuns;
	currentRuns.reserve(numBuffers - 1);

	size_t numInitialRuns = initialRuns.size();
	size_t runCount = std::min(numBuffers - 1, numInitialRuns);

	for (size_t i = 0; i < runCount; ++i) {
		currentRuns.push_back(initialRuns[i]);
	}

	while (initialRuns.size() > 1) {
		disk output("tmp.bin", finalOutput.getBlockSize());

		operations += mergeRunes(currentRuns, output);
		mergingCounter++;
		

		initialRuns.erase(initialRuns.begin(), initialRuns.begin() + currentRuns.size());  // Erase merged runs
		initialRuns.insert(initialRuns.begin(), output);  // Insert the new merged run at the beginning

		if (initialRuns.size() < numBuffers){
			initialRuns.push_back(finalOutput);
			operations += mergeRunes(initialRuns, finalOutput);
			mergingCounter++;
			break;
		}
		else {
			currentRuns.clear();
			runCount = std::min(numBuffers - 1, initialRuns.size());
			for (size_t i = 0; i < runCount; ++i) {
				currentRuns.push_back(initialRuns[i]);
			}
		}
	}
	return std::vector<int>{mergingCounter, operations};
}

void largeBuffers::validation(disk finalOutput) {
	char choice;
	std::cout << "\nWould you like to check if the data is sorted well?\n";
	std::cout << "1.Yes\n";
	std::cout << "2.No\n";

	std::cin >> choice;
	if (choice != '1') {
		return;
	}

	std::cout << "\nValidation of sorting by triangle area: \n";

	size_t blockSize = finalOutput.getBlockSize();  
	size_t totalRecordsProcessed = 0;  
	Block blockData;  

	size_t line = 0;  

	while (true) {
		blockData = finalOutput.readBlock(totalRecordsProcessed / blockSize);
		if (blockData.size() == 0) {
			break; 
		}

		for (size_t i = 0; i < blockData.size(); i++) {
			Coordinates tmp(blockData.coordinates[i]);
			std::cout << line << ". ";
			tmp.printData();  
			line++;  
	
		}
		totalRecordsProcessed += blockSize;  
	}

	std::cout << "\nValidation complete.\n";
}

void largeBuffers::fileCheck(disk file) {
	int choice;
	std::cout << "Would you like to print the file: " << file.getName() << std::endl;
	std::cout << "1. Yes\n2. No\n";

	try {
		std::cin >> choice;
		if (std::cin.fail()) {
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		}
	}
	catch (const std::exception& e) {
		return fileCheck(file);
	}

	if (choice < 1 || choice > 2) {
		std::cerr << "Wrong number, try again.\n";
		return fileCheck(file);
	}
	else if (choice == 1) {
		file.printAll("\n" + file.getName() + ":");
	}
}