#include "largeBuffers.h"
#include <fstream>

#define BUFFER_SIZE 3
#define NUM_OF_BUFFERS 5

int main() {
	disk inputFile("input.bin", BUFFER_SIZE);
	disk outputFile("output.bin", BUFFER_SIZE);

	generatingData data;
	data.startMenu(inputFile);

	inputFile.printAll("Data before sorting:");

	largeBuffers alg;
	std::vector<int> buff = alg.proceedToMerge(inputFile, "run", BUFFER_SIZE, NUM_OF_BUFFERS, outputFile);
	int phaseCounter = buff[0];
	int operations = buff[1];
	inputFile.autoDestruction();

	outputFile.printAll("Data after sorting:");

	alg.validation(outputFile);

	std::cout << "\n\nStatistics: \n\n";
	std::cout << "Disk operations (saves & reads of pages):  " << operations << std::endl;
	std::cout << "Sorting phases:  " << phaseCounter << std::endl;

	outputFile.autoDestruction();

	return 0;
}
