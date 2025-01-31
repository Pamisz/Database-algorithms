#pragma once
#include "helpers.h"

class disk {
public:
    disk(const std::string filename, const size_t blockSize);

    Block readBlock(size_t blockNumber);

    Coordinates readRecord(size_t recordNumber);

    void pushBlock(Block block);

    void pushRecord(Coordinates coords);

    std::vector<double> getAllValues();

    void printAll(std::string message);

    std::string getName();

    void autoDestruction();

    std::size_t getBlockSize();

private:
    std::string filename;
    std::size_t blockSize;
};

class generatingData
{
public:
    void startMenu(disk inputFile);

private:
	int getChoice();

	void generateRecords(disk file, int N);

	void generateRecordsFromKeyboard(disk target);

	void generateRecordsFromFile(disk source, disk target);

};

