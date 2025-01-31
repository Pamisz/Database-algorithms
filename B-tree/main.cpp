#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <fstream>

#define D 2

void readRecordFromFile(int pos, const std::string& filename) {
	std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
	if (!file) {
		std::cerr << "Error: Cannot open file for writing: " << filename << "\n";
		return;
	}

	const size_t recordSize = 6 * sizeof(double) + sizeof(int);

	file.seekg(pos * recordSize, std::ios::beg);
	if (!file) {
		std::cerr << "Error: Invalid position in file.\n";
		file.close();
		return;
	}

	double values[6];
	int intValue;

	for (int i = 0; i < 6; ++i) {
		file.read(reinterpret_cast<char*>(&values[i]), sizeof(double));
		if (!file) {
			std::cerr << "Error: Failed to read double value at index " << i << ".\n";
			file.close();
			return;
		}
	}

	file.read(reinterpret_cast<char*>(&intValue), sizeof(int));
	if (!file) {
		std::cerr << "Error: Failed to read int value.\n";
		file.close();
		return;
	}

	file.close();

	std::cout << "Record at position " << pos << ":\n";
	std::cout << "Values: ";
	for (int i = 0; i < 6; ++i) {
		std::cout << values[i] << " ";
	}
	std::cout << "Key: " << intValue << "\n\n";


}

class BTreeNode {
public:
	bool isLeaf;
	std::vector<int> keys;
	std::vector<int> addresses;
	std::vector<int> children;
	int index = -1;
	int parent = -1;

	BTreeNode(bool leaf) {
		isLeaf = leaf;
	}
};

class BTree {
public:
	BTreeNode* root;
	std::string filename = "tree.bin";
	int save = 0;
	int read = 0;
	int optionalOperations = 0;
	int largestNode = -1;
	std::vector<int> freeIdx;

	void checkpoint() {
		std::cout << "\n Number of saves: " << save;
		std::cout << "\n Number of reads: " << read;
		std::cout << "\n Number of optional saves & reads (e.g. updating child parent, getting node to insert etc.): " << optionalOperations;
		std::cout << "\n\n";
		save = 0;
		read = 0;
		optionalOperations = 0;
	}

	BTreeNode* loadNode(int index) {
		if (index == -1) {
			return root;
		}

		std::fstream file(filename, std::ios::in | std::ios::binary);
		if (!file) {
			std::cerr << "Error: Could not open file for reading.\n";
			return nullptr;
		}

		size_t size = 6 * D * sizeof(int) + 3 * sizeof(int) + sizeof(bool);
		file.seekg(0, std::ios::end);
		size_t fileSize = file.tellg();

		if (index * size >= fileSize) {
			return nullptr;
		}

		file.seekg(index * size, std::ios::beg);
		if (!file) {
			return nullptr;
		}

		BTreeNode* node = new BTreeNode(false);

		//isLeaf
		file.read(reinterpret_cast<char*>(&node->isLeaf), sizeof(node->isLeaf));

		//	index
		file.read(reinterpret_cast<char*>(&node->index), sizeof(node->index));

		// parent
		file.read(reinterpret_cast<char*>(&node->parent), sizeof(node->parent));

		// vectors
		for (int i = 0; i < 2 * D; i++) {
			int child;
			file.read(reinterpret_cast<char*>(&child), sizeof(child));
			if (child != -1) {
				node->children.push_back(child);
			}

			int key;
			file.read(reinterpret_cast<char*>(&key), sizeof(key));
			if (key != -1) {
				node->keys.push_back(key);
			}

			int address;
			file.read(reinterpret_cast<char*>(&address), sizeof(address));
			if (address != -1) {
				node->addresses.push_back(address);
			}
		}

		if (node->children.size() < 2 * D + 1) {
			int extraChild;
			file.read(reinterpret_cast<char*>(&extraChild), sizeof(extraChild));
			if (extraChild != -1) {
				node->children.push_back(extraChild);
			}
		}

		file.close();
		return node;
	}

	void saveNode(int index, BTreeNode* node) {
		if (node == root) {
			return;
		}
		std::fstream file(filename, std::ios::in | std::ios::out | std::ios::binary);
		if (!file) {
			std::cerr << "Error: Could not open file for writing.\n";
			return;
		}

		size_t size = 6 * D * sizeof(int) + 3 * sizeof(int) + sizeof(bool);
		file.seekp(index * size);

		//isLeaf
		bool isLeaf = node->isLeaf;
		file.write(reinterpret_cast<char*>(&isLeaf), sizeof(isLeaf));

		//index
		int nodeIndex = node->index;
		file.write(reinterpret_cast<char*>(&nodeIndex), sizeof(nodeIndex));

		//parent
		int parentIndex = node->parent;
		file.write(reinterpret_cast<char*>(&parentIndex), sizeof(parentIndex));

		//vectors
		for (int i = 0; i < 2*D; i++) {
			if (i < node->children.size()) {
				int child = node->children[i];
				file.write(reinterpret_cast<char*>(&child), sizeof(child));
			}
			else {
				int placeholder = -1;
				file.write(reinterpret_cast<char*>(&placeholder), sizeof(placeholder));
			}

			if (i < node->keys.size()) {
				int key = node->keys[i];
				file.write(reinterpret_cast<char*>(&key), sizeof(key));
			}
			else {
				int placeholder = -1; 
				file.write(reinterpret_cast<char*>(&placeholder), sizeof(placeholder));
			}

			if (i < node->addresses.size()) {
				int address = node->addresses[i];
				file.write(reinterpret_cast<char*>(&address), sizeof(address));
			}
			else {
				int placeholder = -1; 
				file.write(reinterpret_cast<char*>(&placeholder), sizeof(placeholder));
			}
		}

		if (node->children.size() == 2 * D + 1) {
			int child = node->children.back();
			file.write(reinterpret_cast<char*>(&child), sizeof(child));
		}
		else {
			int placeholder = -1;
			file.write(reinterpret_cast<char*>(&placeholder), sizeof(placeholder));
		}
		file.close();
	}

	BTree() {
		root = nullptr;
		std::fstream file(filename, std::ios::in | std::ios::out | std::ios::binary);
		if (!file) {
			file.open(filename, std::ios::out | std::ios::binary);
		}
		file.close();
	}

	BTreeNode* search(int key) {
		BTreeNode* s = root;

		if (s == nullptr) {
			return nullptr;
		}

		while (s != nullptr) {
			if (s != root) {
				read++;
			}

			for (int tmp : s->keys) {
				if (tmp == key) {
					return s;
				}
			}

			if (s->keys.empty() && !s->children.empty()) {
				s = loadNode(s->children.front());
			}
			else {
				if (key < s->keys.front()) {
					s = s->children.size() > 0 ? loadNode(s->children[0]) : nullptr;
				}
				else if (key > s->keys.back()) {
					s = s->children.size() > 0 ? loadNode(s->children.back()) : nullptr;
				}
				else {
					size_t i = 0;
					while (i < s->keys.size() - 1 && key > s->keys[i]) {
						i++;
					}
					s = s->children.size() > 0 ? loadNode(s->children[i]) : nullptr;
				}
			}
		}
		return nullptr;
	}

	BTreeNode* findNode(BTreeNode* node, int key) {
		int i = node->keys.size() - 1;

		if (node->isLeaf) {
			return node;
		}
		else {
			while (i >= 0 && key < node->keys[i]) {
				i--;
			}
			if (i != -1 && node->keys[i] == key) {
				return node;
			}
			i++;
			optionalOperations++;
			return findNode(loadNode(node->children[i]), key);
		}
	}

	int insertInPage(BTreeNode* page, int key,int addr = -1) {
		int  i = page->keys.size() - 1;
		page->keys.push_back(-1);
		page->addresses.push_back(-1);

		while (i >= 0 && page->keys[i] > key) {
			page->keys[i + 1] = page->keys[i];
			page->addresses[i + 1] = page->addresses[i];
			i--;
		}
		i++;
		page->keys[i] = key;

		if (addr != -1) {
			page->addresses[i] = addr;
			return page->addresses[i];
		}

		if (page->isLeaf) {
			if (i > 0) {
				page->addresses[i] = page->addresses[i - 1] + 1;
			}
			else if (page->addresses.size() > 1) {
				page->addresses[i] = page->addresses[i + 1];
			}
		}
		else {
			int idx = 0;
			for (idx = 0; idx < page->children.size(); idx++) {
				if (page->children[idx] == page->children[i]) {
					break;
				}
			}

			BTreeNode* left = idx >= i ? loadNode(page->children[idx]) : nullptr;
			if (left) {
				page->addresses[i] = left->addresses.back() + 1;
				optionalOperations++;
			}
		}
		return page->addresses[i];
	}

	void readRecord(int key, const std::string& filename) {
		BTreeNode* page = search(key);
		if (page) {
			for (int i = 0; i < page->keys.size(); i++) {
				if (key == page->keys[i]) {
					readRecordFromFile(page->addresses[i], filename);
					read++;
					return;
				}
			}
		}
		else {
			std::cerr << "Error: Key doesn't exist\n";
		}
	}

	void removeTree() {
		if (remove(filename.c_str() == 0)) {
			std::cout << "File " << filename << " deleted successfully.\n";
		}
		else {
			std::cerr << "Error: Unable to delete file " << filename << "\n";
		}
	}

	int splitPage(BTreeNode* page, int key, int add = -1) {
		int i = page->keys.size() - 1;
		page->keys.push_back(-1);
		page->addresses.push_back(-1);

		while (i >= 0 && key < page->keys[i]) {
			page->keys[i + 1] = page->keys[i];
			page->addresses[i + 1] = page->addresses[i];
			i--;
		}
		i++;
		page->keys[i] = key;

		if (page->isLeaf) {
			if (i > 0) {
				page->addresses[i] = page->addresses[i - 1] + 1;
			}
			else if (page->addresses.size() > 1) {
				page->addresses[i] = page->addresses[i + 1];
			}
		}
		else {
			int idx = 0;
			for (idx = 0; idx < page->children.size(); idx++) {
				if (page->children[idx] == page->children[i]) {
					break;
				}
			}

			BTreeNode* left = idx >= i ? loadNode(page->children[idx]) : nullptr;
			optionalOperations++;
			if (left) {
				page->addresses[i] = left->addresses.back() + 1;
			}
		}

		if (add != -1) {
			page->addresses[i] = add;
		}

		int addr = page->addresses[i];

		if (page == root) {
			largestNode++;
			page->index = largestNode;
		}

		int middle = page->keys[D];
		int middleAddr = page->addresses[D];

		BTreeNode* newPage = new BTreeNode(page->isLeaf);
		newPage->parent = page->parent;
		largestNode++;
		if (!freeIdx.empty()) {
			newPage->index = freeIdx.back();
			freeIdx.pop_back();
		}
		else {
			newPage->index = largestNode;
		}
		newPage->keys.insert(newPage->keys.begin(), page->keys.begin() + D + 1, page->keys.end());
		newPage->addresses.insert(newPage->addresses.begin(), page->addresses.begin() + D + 1, page->addresses.end());
		page->keys.resize(D);
		page->addresses.resize(D);

		if (!page->isLeaf) {
			newPage->children.insert(newPage->children.begin(), page->children.begin() + D + 1, page->children.end());
			for (int i = 0; i < newPage->children.size(); i++) {
				BTreeNode* chil = loadNode(newPage->children[i]);
				chil->parent = newPage->index;
				saveNode(chil->index, chil);
				optionalOperations += 2;
			}
			page->children.resize(D + 1);
			for (int i = 0; i < page->children.size(); i++) {
				BTreeNode* chil = loadNode(page->children[i]);
				chil->parent = page->index;
				saveNode(chil->index, chil);
				optionalOperations += 2;
			}
		}

		if (page != root) {
			BTreeNode* parent = loadNode(newPage->parent);
			read++;
			if (parent && parent->keys.size() < 2 * D) {
				saveNode(page->index, page);
				saveNode(newPage->index, newPage);
				save += 2;
				insertInPage(parent, middle, middleAddr);
				int idx = 0;
				for (idx; idx < parent->keys.size(); idx++) {
					if (parent->keys[idx] == middle) {
						break;
					}
				}
				parent->children.insert(parent->children.begin() + idx + 1, newPage->index);
				saveNode(parent->index, parent);
				save++;
			}
			else if (parent){
				saveNode(page->index, page);
				saveNode(newPage->index, newPage);
				save += 2;
				int idx = 0;
				for (idx; idx < parent->keys.size(); idx++) {
					if (parent->keys[idx] > middle) {
						break;
					}
				}
				parent->children.insert(parent->children.begin() + idx + 1, newPage->index);
				saveNode(parent->index, parent);
				save++;
				splitPage(parent, middle, middleAddr);
			}
		}
		else {
			BTreeNode* newRoot = new BTreeNode(false);
			newRoot->keys.push_back(middle);
			newRoot->addresses.push_back(middleAddr);
			newRoot->children.push_back(page->index);
			newRoot->children.push_back(newPage->index);
			page->parent = newRoot->index;
			newPage->parent = newRoot->index;
			root = newRoot;

			saveNode(page->index, page);
			saveNode(newPage->index, newPage);

			save += 2;
		}
		return addr;
	}

	BTreeNode* compensation(BTreeNode* page, int key) {
		if (page == root) {
			return nullptr;
		}
		BTreeNode* parent = loadNode(page->parent);
		read++;
		if (parent) {
			int idx = 0;
			for (idx = 0; idx < parent->children.size(); idx++) {
				if (parent->children[idx] == page->index) {
					break;
				}
			}

			BTreeNode* left = idx > 0 ? loadNode(parent->children[idx - 1]) : nullptr;
			BTreeNode* right = parent->children.size() - 1 > idx ? loadNode(parent->children[idx + 1]) : nullptr;
			read++;
			read++;


			if (left && left->keys.size() < 2 * D) {
				if (key > page->keys.front()) {
					int smallest = page->keys.front();
					int smallestAddr = page->addresses.front();
					page->keys.erase(page->keys.begin());
					page->addresses.erase(page->addresses.begin());

					int middle = parent->keys[idx - 1];
					int middleAddr = parent->addresses[idx - 1];
					parent->keys[idx - 1] = smallest;
					parent->addresses[idx - 1] = smallestAddr;

					left->keys.push_back(middle);
					left->addresses.push_back(middleAddr);

					saveNode(left->index, left);
					saveNode(parent->index, parent);
					save += 2;
				}
				else {
					int middle = parent->keys[idx - 1];
					int middleAddr = parent->addresses[idx - 1];
					parent->keys.erase(parent->keys.begin() + idx - 1);
					parent->addresses.erase(parent->addresses.begin() + idx - 1);

					left->keys.push_back(middle);
					left->addresses.push_back(middleAddr);

					page = parent;
					saveNode(left->index, left);
					saveNode(parent->index, parent);
					save += 2;
				}
				return page;
			}
			else if (right && right->keys.size() < 2 * D){
				if (key > page->keys.back()) {
					int middle = parent->keys[idx];
					int middleAddr = parent->addresses[idx];
					parent->keys.erase(parent->keys.begin() + idx);
					parent->addresses.erase(parent->addresses.begin() + idx);

					right->keys.insert(right->keys.begin(), middle);
					right->addresses.insert(right->addresses.begin(), middleAddr);

					page = parent;
					saveNode(right->index, right);
					saveNode(parent->index, parent);
					save += 2;
				}
				else {
					int largest = page->keys.back();
					int largestAddr = page->addresses.back();
					page->keys.pop_back();
					page->addresses.pop_back();

					int middle = parent->keys[idx];
					int middleAddr = parent->addresses[idx];
					parent->keys[idx] = largest;
					parent->addresses[idx] = largestAddr;

					right->keys.insert(right->keys.begin(), middle);
					right->addresses.insert(right->addresses.begin(), middleAddr);

					saveNode(right->index, right);
					saveNode(parent->index, parent);
					save += 2;
				}
				return page;
			}
		}
		return nullptr;
	}

	int insert(int key) {
		//	check if key already exists
		if (search(key)) {
			std::cerr << "Error: Key already exits\n";
			return -1;
		}

		//	no root
		if (root == nullptr) {
			root = new BTreeNode(true);
			root->keys.push_back(key);
			root->addresses.push_back(0);
			return 0;
		}
		else {
			BTreeNode* page = findNode(root, key);
			if (page->keys.size() < 2 * D) {
				int pos = insertInPage(page, key);
				saveNode(page->index, page);
				save++;
				return pos;
			}
			else {
				BTreeNode* copy = compensation(page, key);
				if (copy) {
					int pos = insertInPage(copy, key);
					saveNode(copy->index, copy);
					save++;
					return pos;
				}
				else {
					int pos = splitPage(page, key);
					return pos;
				}
			}
		}
	}

	void displayNode(BTreeNode* node, int level = 0, const std::string& prefix = "") {
		if (!node) return;
		
		if (node != root) {
			read++;
		}

		std::cout << prefix;
		std::cout << (level == 0 ? "Root: " : "|-- ");
		for (int i = 0; i < node->keys.size(); i++) {
			std::cout << "(" << node->keys[i] << "," << node->addresses[i] << ") ";
		}
		std::cout << "\n";

		if (!node->isLeaf) {
			std::string childPrefix = prefix + (level == 0 ? "   " : "|  ");
			for (int i = 0; i < node->children.size(); i++) {
				bool isLast = (i == node->children.size() - 1);
				BTreeNode* child = loadNode(node->children[i]);
				displayNode(child, level + 1, childPrefix + (isLast ? "   " : "|  "));
			}
		}
	}

	void display() {
		if (root != nullptr) {
			displayNode(root, 0);
		}
	}

	void incrementAddr(int index, const std::string& filename) {
		std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
		if (!file) {
			std::cerr << "Error: Cannot open file for writing: " << filename << "\n";
			return;
		}

		const size_t recordSize = 6 * sizeof(double) + sizeof(int);

		file.seekg(index * recordSize, std::ios::beg);
		if (!file) {
			file.close();
			return;
		}

		double values[6];
		int intValue;

		for (int i = 0; i < 6; ++i) {
			file.read(reinterpret_cast<char*>(&values[i]), sizeof(double));
			if (!file) {
				file.close();
				return;
			}
		}

		file.read(reinterpret_cast<char*>(&intValue), sizeof(int));
		if (!file) {
			file.close();
			return;
		}

		file.close();
		optionalOperations++;

		BTreeNode* page = search(intValue);
		if (page) {
			for (int i = 0; i < page->keys.size(); i++) {
				if (intValue == page->keys[i]) {
					page->addresses[i]++;
					saveNode(page->index, page);
					optionalOperations++;
					incrementAddr(index + 1, filename);
					return;
				}
			}
		}

	}

	void decrementAddr(int index, const std::string& filename) {
		std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
		if (!file) {
			std::cerr << "Error: Cannot open file for writing: " << filename << "\n";
			return;
		}

		const size_t recordSize = 6 * sizeof(double) + sizeof(int);

		file.seekg(index * recordSize, std::ios::beg);
		if (!file) {
			file.close();
			return;
		}

		double values[6];
		int intValue;

		for (int i = 0; i < 6; ++i) {
			file.read(reinterpret_cast<char*>(&values[i]), sizeof(double));
			if (!file) {
				file.close();
				return;
			}
		}

		file.read(reinterpret_cast<char*>(&intValue), sizeof(int));
		if (!file) {
			file.close();
			return;
		}

		file.close();
		optionalOperations++;

		BTreeNode* page = search(intValue);
		if (page) {
			for (int i = 0; i < page->keys.size(); i++) {
				if (intValue == page->keys[i]) {
					page->addresses[i]--;
					saveNode(page->index, page);
					optionalOperations++;
					decrementAddr(index + 1, filename);
					return;
				}
			}
		}
	}

	int removeInPage(BTreeNode* page, int key) {
		for (int i = 0; i < page->keys.size(); i++) {
			if (page->keys[i] == key) {
				page->keys.erase(page->keys.begin() + i);
				int addr = page->addresses[i];
				page->addresses.erase(page->addresses.begin() + i);
				return addr;
			}
		}
		return -1;
	}

	BTreeNode* reverseCompensation(BTreeNode* page) {
		if (page->index == -1) {
			return nullptr;
		}
		BTreeNode* parent = loadNode(page->parent);
		read++;
		if (parent) {
			int idx = 0;
			for (idx = 0; idx < parent->children.size(); idx++) {
				if (parent->children[idx] == page->index) {
					break;
				}
			}

			BTreeNode* left = idx > 0 ? loadNode(parent->children[idx - 1]) : nullptr;
			BTreeNode* right = parent->children.size() - 1 > idx ? loadNode(parent->children[idx + 1]) : nullptr;
			read++;
			read++;
			if (left && left->keys.size() > D) {
				int largest = left->keys.back();
				int largestAddr = left->addresses.back();
				left->keys.pop_back();
				left->addresses.pop_back();

				int middle = parent->keys[idx - 1];
				int middleAddr = parent->addresses[idx - 1];
				parent->keys[idx - 1] = largest;
				parent->addresses[idx - 1] = largestAddr;

				page->keys.insert(page->keys.begin(), middle);
				page->addresses.insert(page->addresses.begin(), middleAddr);

				if (!left->isLeaf) {
					page->children.insert(page->children.begin(), left->children.back());
					left->children.pop_back();
				}

				saveNode(page->index, page);
				saveNode(left->index, left);
				saveNode(parent->index, parent);
				save += 3;
				return page;
			}
			else if (right && right->keys.size() > D) {
				int smallest = right->keys.front();
				int smallestAddr = right->addresses.front();
				right->keys.erase(right->keys.begin());
				right->addresses.erase(right->addresses.begin());

				int middle = parent->keys[idx];
				int middleAddr = parent->addresses[idx];
				parent->keys[idx] = smallest;
				parent->addresses[idx] = smallestAddr;

				page->keys.push_back(middle);
				page->addresses.push_back(middleAddr);

				if (!right->isLeaf) {
					page->children.push_back(right->children.front());
					right->children.erase(right->children.begin());
				}

				saveNode(page->index, page);
				saveNode(right->index, right);
				saveNode(parent->index, parent);
				save += 3;
				return page;
			}
		}
		return nullptr;
	}

	void merge(BTreeNode* page) {
		if (page->index == -1) {
			return;
		}
		BTreeNode* parent = loadNode(page->parent);
		read++;
		if (parent && !parent->keys.empty()) {
			int idx = 0;
			for (idx = 0; idx < parent->children.size(); idx++) {
				if (parent->children[idx] == page->index) {
					break;
				}
			}

			BTreeNode* left = idx > 0 ? loadNode(parent->children[idx - 1]) : nullptr;
			BTreeNode* right = parent->children.size() - 1 > idx ? loadNode(parent->children[idx + 1]) : nullptr;
			read++;
			read++;
			if (left) {
				int middle = parent->keys[idx - 1];
				int middleAddr = parent->addresses[idx - 1];

				parent->keys.erase(parent->keys.begin() + idx - 1);
				parent->addresses.erase(parent->addresses.begin() + idx - 1);

				left->keys.push_back(middle);
				left->addresses.push_back(middleAddr);

				left->keys.push_back(page->keys.front());
				left->addresses.push_back(page->addresses.front());

				if (!page->isLeaf) {
					left->children.insert(left->children.begin() + 3, page->children.begin(), page->children.end());
				}
				parent->children.erase(parent->children.begin() + idx);

				saveNode(parent->index, parent);
				saveNode(left->index, left);
				save += 2;
				freeIdx.push_back(page->index);
			}
			else if (right) {
				int middle = parent->keys[idx];
				int middleAddr = parent->addresses[idx];

				parent->keys.erase(parent->keys.begin() + idx);
				parent->addresses.erase(parent->addresses.begin() + idx);

				page->keys.push_back(middle);
				page->addresses.push_back(middleAddr);
				page->keys.push_back(right->keys.front());
				page->addresses.push_back(right->addresses.front());
				page->keys.push_back(right->keys[1]);
				page->addresses.push_back(right->addresses[1]);

				if (!right->isLeaf) {
					page->children.insert(page->children.begin() + 2, right->children.begin(), right->children.end());
				}
				parent->children.erase(parent->children.begin() + idx + 1);

				saveNode(parent->index, parent);
				saveNode(page->index, page);
				save += 2;
				freeIdx.push_back(right->index);
			}
		}
	}

	BTreeNode* getSmallest(BTreeNode* page) {
		while (!page->isLeaf) {
			page = loadNode(page->children.front());
			read++;
		}
		return page;
	}

	BTreeNode* getLargest(BTreeNode* page) {
		while (!page->isLeaf) {
			page = loadNode(page->children.back());
			read++;
		}
		return page;
	}

	int remove(int key) {
		//	check if key already exists
		if (!search(key)) {
			std::cerr << "Error: Key doesn't exist\n";
			return -1;
		}

		int addr = -1;

		BTreeNode* page = findNode(root, key);
		if (page->isLeaf) {
			//removeFromPage
			if (page->index == -1 && page->keys.size() == 1) {
				root = nullptr;
				return 0;
			}

			addr = removeInPage(page, key);
			saveNode(page->index, page);
			save++;
		}
		else {
			int idx = 0;
			for (idx; idx < page->keys.size(); idx++) {
				if (page->keys[idx] == key) {
					break;
				}
			}

			int keyToSwap = -1, addToSwap = -1;
			BTreeNode* leaf;
			BTreeNode* left = loadNode(page->children[idx]);
			BTreeNode* right = loadNode(page->children[idx + 1]);
			read++;
			read++;
			if (idx + 1 < page->children.size() - 1) {
				if (left->keys.size() > right->keys.size()) {
					leaf = getLargest(left);
					keyToSwap = leaf->keys.back();
					addToSwap = leaf->addresses.back();
				}
				else {
					leaf = getSmallest(right);
					keyToSwap = leaf->keys.front();
					addToSwap = leaf->addresses.front();
				}
			}
			else {
				leaf = getLargest(left);
				keyToSwap = leaf->keys.back();
				addToSwap = leaf->addresses.back();
			}


			removeInPage(leaf, keyToSwap);
			saveNode(leaf->index, leaf);
			save++;

			addr = removeInPage(page, key);
			insertInPage(page, keyToSwap, addToSwap);
			saveNode(page->index, page);
			save++;

			page = leaf;
		}

		fixTree(page);
		
		return addr;
	}

	void fixTree(BTreeNode* page) {
		if (page->index == -1) {
			if (page == root && page->keys.empty()) {
				root = loadNode(root->children.front());
				root->index = -1;
				read++;
				return;
			}
			else {
				return;
			}
		}

		if (!(page->keys.size() >= D) && page != root) {
			if (!reverseCompensation(page)) {
				merge(page);
				page = loadNode(page->parent);
				read++;
				fixTree(page);
			}
		}
	}
};

struct DataRow {
	std::vector<double> doubles;
	int singleInt;
};

void insertRecordAt(const std::string& filename, const DataRow& newRow, size_t recordIndex) {
	std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
	if (!file) {
		std::cerr << "Error: Cannot open file for writing: " << filename << "\n";
		return;
	}

	size_t recordSize = 6 * sizeof(double) + sizeof(int);

	size_t insertPosition = recordIndex * recordSize;

	file.seekg(0, std::ios::end);
	size_t fileSize = file.tellg();

	if (insertPosition >= fileSize || insertPosition < 0) {
		file.close();
		std::ofstream outFile(filename, std::ios::binary | std::ios::app);
		outFile.write(reinterpret_cast<const char*>(newRow.doubles.data()), 6 * sizeof(double));
		outFile.write(reinterpret_cast<const char*>(&newRow.singleInt), sizeof(newRow.singleInt));
		outFile.close();
		return;
	}

	std::vector<char> buffer(fileSize - insertPosition);
	file.seekg(insertPosition, std::ios::beg);
	file.read(buffer.data(), buffer.size());

	file.seekp(insertPosition, std::ios::beg);
	file.write(reinterpret_cast<const char*>(newRow.doubles.data()), 6 * sizeof(double));
	file.write(reinterpret_cast<const char*>(&newRow.singleInt), sizeof(newRow.singleInt));

	file.write(buffer.data(), buffer.size());

	file.close();
}

void insertRecord(DataRow row, const std::string& filename, BTree& tree) {
	int index = tree.insert(row.singleInt);
	if (index != -1) {
		insertRecordAt(filename, row, index);
		tree.save++;
		tree.incrementAddr(index + 1, filename);
		std::cout << "Record inserted succesfully.\n";
	}
}

void insertFromFile(const std::string& filename, BTree& tree) {
	std::string txt;
	std::cout << "Enter filename with data(txt):\n";
	std::cin >> txt;

	std::ifstream file(txt);
	if (!file.is_open()) {
		std::cerr << "\nError: Could not open file " << txt << "\n";
		return;
	}

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream lineStream(line);
		DataRow row;
		double value;
		for (int i = 0; i < 6; ++i) {
			if (!(lineStream >> value)) {
				std::cerr << "Error: Malformed line in file.\n";
				return;
			}
			row.doubles.push_back(value);
		}
		if (!(lineStream >> row.singleInt)) {
			std::cerr << "Error: Malformed line in file.\n";
			return;
		}
		insertRecord(row, filename, tree);
	}
	file.close();
}

void insertFromKeyboard(const std::string& filename, BTree& tree) {
	DataRow row;
	std::string line;
	std::cout << "Enter 6 doubles followed by 1 integer (separated by spaces): \n";
	std::getline(std::cin, line);
	while (line.empty()) {
		std::getline(std::cin, line);
	}

	std::istringstream lineStream(line);
	double value;

	for (int i = 0; i < 6; ++i) {
		if (!(lineStream >> value)) {
			std::cerr << "Error: Invalid input for doubles.\n\n";
			return;
		}
		row.doubles.push_back(value);
	}

	if (!(lineStream >> row.singleInt)) {
		std::cerr << "Error: Invalid input for integer.\n\n";
		return;
	}

	insertRecord(row, filename, tree);
}

void removeRecordAt(const std::string& filename, size_t recordIndex) {
	std::ifstream inFile(filename, std::ios::binary);
	if (!inFile) {
		std::cerr << "\nError: Cannot open file for reading: " << filename << "\n";
		return;
	}

	std::vector<DataRow> values;
	DataRow value;

	while (inFile.peek() != EOF) {
		std::vector<double> doubles(6);
		inFile.read(reinterpret_cast<char*>(doubles.data()), 6 * sizeof(double));

		int singleInt;
		inFile.read(reinterpret_cast<char*>(&singleInt), sizeof(singleInt));


		value.doubles = doubles;
		value.singleInt = singleInt;
		values.push_back(value);
	}
	inFile.close();

	if (recordIndex >= values.size()) {
		std::cerr << "\nError: Record index out of range.\n";
		return;
	}

	values.erase(values.begin() + recordIndex);

	std::ofstream outputFile(filename, std::ios::binary);
	if (!outputFile.is_open()) {
		std::cerr << "\nError: Unable to open file for writing.\n";
		return;
	}

	for (int i = 0; i < values.size(); i++) {
		outputFile.write(reinterpret_cast<const char*>(values[i].doubles.data()), 6 * sizeof(double));
		outputFile.write(reinterpret_cast<const char*>(&values[i].singleInt), sizeof(int));
	}
	outputFile.close();
}

void removeRecord(int key, const std::string& filename, BTree& tree) {
	int index = tree.remove(key);
	if (index != -1) {
		removeRecordAt(filename, index);
		tree.read++;
		tree.decrementAddr(index, filename);
		std::cout << "Records removed succesfully.\n";
	}
}

void removeFromFile(const std::string& filename, BTree& tree) {
	std::string txt;
	std::cout << "Enter filename with data(txt):\n";
	std::cin >> txt;

	std::ifstream file(txt);
	if (!file.is_open()) {
		std::cerr << "\nError: Could not open file " << txt << "\n";
		return;
	}

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream lineStream(line);
		int singleInt = 0;
		if (!(lineStream >> singleInt)) {
			std::cerr << "Error: Malformed line in file.\n";
			return;
		}
		removeRecord(singleInt, filename, tree);
	}
	file.close();
}

void removeFromKeyboard(const std::string& filename, BTree& tree) {
	std::string line;
	std::cout << "Enter key to remove: \n";
	std::getline(std::cin, line);
	while (line.empty()) {
		std::getline(std::cin, line);
	}

	int value;
	std::istringstream lineStream(line);
	if (!(lineStream >> value)) {
		std::cerr << "Error: Invalid input for integer.\n\n";
		return;
	}

	removeRecord(value, filename, tree);
}

void printRecords(const std::string& filename) {
	std::ifstream inFile(filename, std::ios::binary);
	if (!inFile) {
		std::cerr << "Error: Cannot open file for reading: " << filename << "\n";
		return;
	}

	std::cout << "Records from output file: \n";
	while (inFile.peek() != EOF) { 
		std::vector<double> doubles(6);
		inFile.read(reinterpret_cast<char*>(doubles.data()), 6 * sizeof(double));

		int singleInt;
		inFile.read(reinterpret_cast<char*>(&singleInt), sizeof(singleInt));

		std::cout << "Values: ";
		for (const auto& d : doubles) {
			std::cout << d << " ";
		}
		std::cout << " Key: " << singleInt << "\n";
	}
	std::cout << "\n";
	inFile.close();
}

void changeRecord(const std::string& filename, BTree& tree) {
	std::string line;
	std::cout << "Enter key to change: \n";
	std::getline(std::cin, line);
	while (line.empty()) {
		std::getline(std::cin, line);
	}

	int value;
	std::istringstream lineStream(line);
	if (!(lineStream >> value)) {
		std::cerr << "Error: Invalid input for integer.\n\n";
		return;
	}

	DataRow row;
	std::cout << "Enter 6 doubles followed by 1 integer (separated by spaces): \n";
	std::getline(std::cin, line);
	while (line.empty()) {
		std::getline(std::cin, line);
	}

	std::istringstream lineStream1(line);
	double doub;
	for (int i = 0; i < 6; ++i) {
		if (!(lineStream1 >> doub)) {
			std::cerr << "Error: Invalid input for doubles.\n\n";
			return;
		}
		row.doubles.push_back(doub);
	}

	if (!(lineStream1 >> row.singleInt)) {
		std::cerr << "Error: Invalid input for integer.\n\n";
		return;
	}

	removeRecord(value, filename, tree);
	insertRecord(row, filename, tree);
}

void menu() {
	char choice = '0';
	const std::string filename = "output.bin";

	std::ofstream outFile(filename, std::ios::binary);
	if (!outFile) {
		std::cerr << "Error: Cannot create file " << filename << "\n";
		return;
	}
	outFile.close();
	BTree tree;
	
	while (choice != 'q') {
		std::cout << "1. Insert records from file\n";
		std::cout << "2. Insert record from keyboard\n";
		std::cout << "3. Read record\n";
		std::cout << "4. Print tree\n";
		std::cout << "5. Print records\n";
		std::cout << "6. Remove records from file\n";
		std::cout << "7. Remove record from keyboard\n";
		std::cout << "8. Change rekord\n";
		std::cout << "Exit - q\n";

		std::cin >> choice;
		std::cout << "\n";

		if (choice == '1') {
			insertFromFile(filename, tree);
			tree.read++;
			tree.checkpoint();
		}
		else if (choice == '2') {
			insertFromKeyboard(filename, tree);
			tree.checkpoint();
		}
		else if (choice == '3') {
			std::cout << "Enter the key: \n";
			int key;
			std::cin >> key;
			tree.readRecord(key, filename);
			tree.checkpoint();
		}
		else if (choice == '4') {
			tree.display();
			tree.checkpoint();
		}
		else if (choice == '5') {
			printRecords(filename);
			tree.read++;
			tree.checkpoint();
		}
		else if (choice == '6') {
			removeFromFile(filename, tree);
			tree.read++;
			tree.checkpoint();
		}
		else if (choice == '7') {
			removeFromKeyboard(filename, tree);
			tree.checkpoint();
		}
		else if (choice == '8') {
			changeRecord(filename, tree);
			tree.checkpoint();
		}
		else if (choice != 'q') {
			std::cerr << "\nNumber out of range. Try again.\n";
		}
	}

	//Remove output.bin
	if (remove(filename.c_str()) == 0) {
		std::cout << "File " << filename << " deleted successfully.\n";
	}
	else {
		std::cerr << "Error: Unable to delete file " << filename << "\n";
	}

	tree.removeTree();
}

int main() {
	
	menu();

	return 0;
}


//experymnet: 
//ustalenie stopnia drzewa
//ilosci rekordow
//wearunek reorganizacji: kiedy musze zmieniac pliki np. przy aktualizajci dzieci czy zmianie kolejnosci
//wspolczynnik alfa?
//jak to zwiêksza ilosc operacji wstawiania i usuwania
//czy istnieje optymalna metoda na dostosowanie parametrow

