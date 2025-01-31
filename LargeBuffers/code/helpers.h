#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

struct Coordinates {
    std::vector<double> coords;  
    double triangleArea;         

    Coordinates(std::vector<double> coordinates)
        : coords(coordinates),
        triangleArea(calculateTriangleArea(coordinates)) {}

    Coordinates() {};

    double calculateTriangleArea(std::vector<double> coordinates) {
        if (coordinates.size() < 6) {
            std::cerr << "Invalid coordinates size!" << std::endl;
            return 0.0;
        }

        double x1 = coordinates[0], y1 = coordinates[1];
        double x2 = coordinates[2], y2 = coordinates[3];
        double x3 = coordinates[4], y3 = coordinates[5];

        return 0.5 * std::abs((x2 - x1) * (y3 - y1) - (y2 - y1) * (x3 - x1));
    }

    bool operator<(const Coordinates& other) const {
        return triangleArea > other.triangleArea;
    }

    std::vector<double> getCoords() const {
        return coords;
    }

    bool operator==(const Coordinates& other) const {
        return coords == other.coords;
    }

    void printData() const {
        std::cout << "Coords: ";
        for (const auto& coord : coords) {
            std::cout << coord << " ";
        }
        std::cout << ", Area: " << triangleArea << std::endl;
    }
};

struct Block {
	std::vector<Coordinates> coordinates;

	Block() {}

	Block(std::vector<Coordinates> coords) : coordinates(coords) {}

	size_t size() {
		return coordinates.size();
	}

	void addCoordinates(Coordinates coord) {
		coordinates.push_back(coord);
	}

    Coordinates popFirst() {
        Coordinates result = coordinates.front();
        removeByIndex(0);
        return result;
    }

	bool removeByIndex(size_t index) {
		if (index >= coordinates.size()) {
			std::cerr << "Index out of bounds!" << std::endl;
			return false;
		}
		coordinates.erase(coordinates.begin() + index);
		return true;
	}

    std::vector<double> getFlatCoordinates() const {
        std::vector<double> flatData;
        for (const auto& coord : coordinates) {
            auto coords = coord.getCoords();
            flatData.insert(flatData.end(), coords.begin(), coords.end());
        }
        return flatData;
    }

    void printBlock() const {
        for (const auto& coord : coordinates) {
            coord.printData(); 
        }
    }

    void clear() {
        coordinates.clear();
    }
};

template <typename T>
inline void swap(T& a, T& b) {
	T temp = a;
	a = b;
	b = temp;
}

template <typename T>
inline int partition(std::vector<T>& arr, int low, int high) {
	double pivotArea = arr[high].triangleArea;
	int i = low - 1;

	for (int j = low; j < high; j++) {
		if (arr[j].triangleArea < pivotArea) {
			i++;
			swap(arr[i], arr[j]);
		}
	}
	swap(arr[i + 1], arr[high]);
	return i + 1;
}

template <typename T>
inline void quicksort(std::vector<T>& arr, int low, int high) {
	if (low < high) {
		int pi = partition(arr, low, high);

		quicksort(arr, low, pi - 1);
		quicksort(arr, pi + 1, high);
	}
}