#include "data.h"
#include <fstream>
#include <iomanip> //to set precision


disk::disk(const std::string filename, const size_t blockSize) : filename(filename), blockSize(blockSize) {
	std::ofstream outFile(filename, std::ios::binary | std::ios::app);
	if (!outFile) {
		throw std::runtime_error("Failed to open file for binary appending.");
	}
	outFile.close();
}

Block disk::readBlock(size_t blockNumber) {
	std::vector<double> allValues = getAllValues();

	size_t start = blockNumber * blockSize * 6;
	size_t end = std::min(start + blockSize * 6, allValues.size());

	if (start >= allValues.size()) {
		return {};
	}

	std::vector<Coordinates> coordinates;
	for (size_t i = start; i + 5 < end; i += 6) {
		coordinates.emplace_back(std::vector<double>{
				allValues[i],     // x1
				allValues[i + 1], // y1
				allValues[i + 2], // x2
				allValues[i + 3], // y2
				allValues[i + 4], // x3
				allValues[i + 5]  // y3
		});
	}

	Block block(coordinates);

	return block;
}

Coordinates disk::readRecord(size_t recordNumber) {
	std::vector<double> allValues = getAllValues();

	size_t start = recordNumber * 6;  
	size_t end = start + 6;

	if (start >= allValues.size()) {
		return Coordinates();  
	}

	std::vector<double> values(allValues.begin() + start, allValues.begin() + end);
	
	Coordinates coords(values);

	return coords;
}

void disk::pushBlock(Block block) {
	std::ofstream outFile(filename, std::ios::binary | std::ios::app);
	if (!outFile) {
		throw std::runtime_error("Failed to open file for binary appending.");
	}

	std::vector<double> data = block.getFlatCoordinates();

	size_t totalSize = data.size();
	size_t start = 0;

	while (start < totalSize) {
		size_t currentBlockSize = std::min(blockSize * 6, totalSize - start);
		outFile.write(reinterpret_cast<const char*>(&data[start]), currentBlockSize * sizeof(double));
		start += currentBlockSize;
	}

	outFile.close();
}

void disk::pushRecord(Coordinates coords) {
	std::ofstream outFile(filename, std::ios::binary | std::ios::app);
	if (!outFile) {
		throw std::runtime_error("Failed to open file for binary appending.");
	}

	std::vector<double> values = coords.getCoords();

	for (double value : values) {
		outFile.write(reinterpret_cast<const char*>(&value), sizeof(double));
	}

	outFile.close();
}

std::vector<double> disk::getAllValues() {
	std::ifstream inFile(filename, std::ios::binary);
	if (!inFile) {
		throw std::runtime_error("Failed to open file for reading.");
	}

	inFile.seekg(0, std::ios::end);
	std::streamsize fileSize = inFile.tellg();
	inFile.seekg(0, std::ios::beg);

	if (fileSize % sizeof(double) != 0) {
		throw std::runtime_error("File size is not a multiple of double size.");
	}

	size_t numDoubles = fileSize / sizeof(double);

	std::vector<double> data(numDoubles);
	inFile.read(reinterpret_cast<char*>(data.data()), fileSize);
	inFile.close();

	data.erase(std::remove(data.begin(), data.end(), 0.0), data.end());
	return data;
}

void disk::printAll(std::string message){
	std::cout << "\n" << message << "\n";
	std::vector<double> values = getAllValues();
	size_t len = values.size();
    for (int i = 0; i < len; i++) {
		if (i % 6 == 0 && i != 0) {
			std::cout << std::endl;
		}
        std::cout << values[i] << " ";
    }
        std::cout << std::endl;
}

std::string disk::getName() {
    return filename;
}

void disk::autoDestruction() {
    if (std::remove(filename.c_str()) != 0) {
        throw std::runtime_error("Failed to remove file");
    }
}

std::size_t disk::getBlockSize() {
	return blockSize;
}



int generatingData::getChoice() {
	int choice;
	std::cout << "\nChoose generating records option:\n";
	std::cout << "1. Generating random records\n";
	std::cout << "2. Enter records from keyboard\n";
	std::cout << "3. Enter records from file\n";
	std::cout << "4. Finish generating records\n";

    try {
        std::cin >> choice;
        if (std::cin.fail()) {
            std::cin.clear(); 
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
        }
    }
    catch (const std::exception& e){
        return getChoice();
    }

	if (choice < 1 || choice > 4) {
		std::cerr << "Wrong number, try again.\n";
		return getChoice();
	}

	return choice;
}

void generatingData::generateRecords(disk file, int N) {
	std::srand(std::time(0));
	Block block;

	for (int i = 0; i < N; i++) {
		// -10.00 to 10.00
		std::vector<double> values = {
		(std::rand() % 2001 - 1000) / 100.0, // x1
		(std::rand() % 2001 - 1000) / 100.0, // y1
		(std::rand() % 2001 - 1000) / 100.0, // x2
		(std::rand() % 2001 - 1000) / 100.0, // y2
		(std::rand() % 2001 - 1000) / 100.0, // x3
		(std::rand() % 2001 - 1000) / 100.0  // y3
		};

		block.addCoordinates(values);
	}

	file.pushBlock(block);

	std::cout << "Saved: " << N << " records to file: " << file.getName() << std::endl;
}

void generatingData::generateRecordsFromKeyboard(disk target) {
	double x1, y1, x2, y2, x3, y3;

	while (true) {
		std::cout << "\nEnter 3 pairs of floating-point coordinates (x1 y1 x2 y2 x3 y3): ";
		if (std::cin >> x1 >> y1 >> x2 >> y2 >> x3 >> y3) {
			break;
		}
		else {
			std::cerr << "Invalid input, try again.\n";
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		}
	}

	std::vector<double> values = { x1,y1,x2,y2,x3,y3 };
    target.pushRecord(values);
	std::cout << "Saved record to file: " << target.getName() << std::endl;
}

void generatingData::generateRecordsFromFile(disk source, disk target) {
	std::vector<double> values = source.getAllValues();

	Block block;

	for (size_t i = 0; i + 6 <= values.size(); i += 6) {
		double x1 = values[i];
		double y1 = values[i + 1];
		double x2 = values[i + 2];
		double y2 = values[i + 3];
		double x3 = values[i + 4];
		double y3 = values[i + 5];

		std::vector<double> values = { x1, y1, x2, y2, x3, y3 };
		Coordinates coord(values);
		block.addCoordinates(coord);
	}

	target.pushBlock(block);

	std::cout << "Records successfully added from "
		<< source.getName() << " to " << target.getName() << std::endl;
}

void generatingData::startMenu(disk inputFile) {
	int choice = getChoice();
	while (choice != 4) {
		if (choice == 1) {
			int N;
			std::cout << "Enter the number of records to generate: ";
			std::cin >> N;
			generateRecords(inputFile, N);
		}
		else if (choice == 2) {
			generateRecordsFromKeyboard(inputFile);
		}
		else if (choice == 3) {
			std::string sourceFile;
			std::cout << "Enter source file name: ";
			std::cin >> sourceFile;
			disk tmp(sourceFile, 0);
			generateRecordsFromFile(tmp, inputFile);
		}
		choice = getChoice();
	}
}



