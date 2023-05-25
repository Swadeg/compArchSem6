/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <stdint.h>
#include <string>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

using namespace std;

typedef struct parsedAddress {
	uint32_t set;
	uint32_t tag;
} parsedAddress;

typedef struct entry {
	uint32_t tag;
	bool valid, dirty;
	int LRU; // 0 for most recently used, 1 for next one, 2 for next one and so on...
} entry;



// Functions
uint32_t maskBitsFrom32bits(uint32_t input, unsigned lsb, unsigned howManyBits);

parsedAddress parseAddressL1(uint32_t address);
parsedAddress parseAddressL2(uint32_t address);

int findParsedAddressInL1(uint32_t tag, uint32_t set);
int findParsedAddressInL2(uint32_t tag, uint32_t set);

void updateLRU_OnAccessL1(uint32_t set, uint32_t way);
void updateLRU_OnAccessL2(uint32_t set, uint32_t way);

void invalidateWithLRU_L1(uint32_t set, uint32_t way);
void invalidateWithLRU_L2(uint32_t set, uint32_t way);

void processWrite(uint32_t address);
void processRead(uint32_t address);

void printEntry(entry entryToPrint);
void printCache(entry **cache, unsigned howManySets, unsigned howManyWays, string cacheName);
// ----------

// Our data structures:
entry **L1;
entry **L2;

unsigned accessCountL1;
unsigned missCountL1;

unsigned accessCountL2;
unsigned missCountL2;

unsigned totalTime;

unsigned MemCyc = 100, BSize = 3, L1Size = 4, L2Size = 6, L1Assoc = 1,
		L2Assoc = 0, L1Cyc = 1, L2Cyc = 5, WrAlloc = 1;
// ----------


int main(int argc, char **argv) {
/*
	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}
	*/

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = "example1_trace";
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}


/*
	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}
*/

	// Init our data structues:
	unsigned L1_ways = pow(2, L1Assoc);
	unsigned L1_sets = pow(2, L1Size - BSize - L1Assoc);



	L1 = new entry*[L1_sets];
	for (int i = 0; i < L1_sets; i++) {
	    L1[i] = new entry[L1_ways];
	}



	unsigned L2_ways = pow(2, L2Assoc);
	unsigned L2_sets = pow(2, L2Size - BSize - L2Assoc);

	L2 = new entry*[L2_sets];
	for (int i = 0; i < L2_sets; i++) {
		L2[i] = new entry[L2_ways];
	}


	for (int i = 0; i < L1_sets; i++) {
		for (int j = 0; j < L1_ways; j++) {
			L1[i][j].tag = 0;
			L1[i][j].valid = false;
			L1[i][j].dirty = false;
			L1[i][j].LRU = 0;
		}
	}

	for (int i = 0; i < L2_sets; i++) {
		for (int j = 0; j < L2_ways; j++) {
			L2[i][j].tag = 0;
			L2[i][j].valid = false;
			L2[i][j].dirty = false;
			L2[i][j].LRU = 0;
		}
	}


	accessCountL1 = 0;
	missCountL1 = 0;

	accessCountL2 = 0;
	missCountL2 = 0;

	totalTime = 0;
	// ----------

	//cout << "initialCache" << endl;
	//printCache(L1, L1_sets, L1_ways, "L1");
	//printCache(L2, L2_sets, L2_ways, "L2");


	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (r) or write (w)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		//cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		//cout << ", address (hex)" << cutAddress;

		uint32_t num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		//cout << " (dec) " << num << endl;
		//cout << "====================" << endl;


		// Proccess command
		if (operation == 'w') { // Write command
			processWrite(num);
		} else { // Read command
			processRead(num);
		}
		// ----------

		//printCache(L1, L1_sets, L1_ways, "L1");
		//printCache(L2, L2_sets, L2_ways, "L2");
	}

	double L1MissRate = ((double)missCountL1) / accessCountL1;
	double L2MissRate = ((double)missCountL2) / accessCountL2;
	double avgAccTime = ((double)totalTime) / accessCountL1;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);


	// Free memory:
	for (int i = 0; i < L1_sets; i++) {
	    delete[] L1[i];
	}
	delete[] L1;

	for (int i = 0; i < L2_sets; i++) {
	    delete[] L2[i];
	}
	delete[] L2;
	// ----------

	return 0;
}


// Leaves only the bits from lsb to msb, and shifts them to the lsb side of the int
uint32_t maskBitsFrom32bits(uint32_t input, unsigned lsb, unsigned howManyBits) {
	if (howManyBits == 0) {
		return 0;
	}
	return (input << (32 - (lsb + howManyBits))) >> (32 - (howManyBits));
}

parsedAddress parseAddressL1(uint32_t address) {
	parsedAddress output;
	output.set = maskBitsFrom32bits(address, BSize, L1Size - BSize - L1Assoc);
	output.tag = maskBitsFrom32bits(address, L1Size - L1Assoc, 32 - (L1Size - L1Assoc));

	return output;
}

parsedAddress parseAddressL2(uint32_t address) {
	parsedAddress output;
	output.set = maskBitsFrom32bits(address, BSize, L2Size - BSize - L2Assoc);
	output.tag = maskBitsFrom32bits(address, L2Size - L2Assoc, 32 - (L2Size - L2Assoc));

	return output;
}

// Return way number of input tag if it's valid. if not tag isn't found or isn't valid, return -1;
int findParsedAddressInL1(uint32_t tag, uint32_t set) {
	int output = -1;

	for (int j = 0; j < pow(2, L1Assoc); j++) {
		if (L1[set][j].valid && L1[set][j].tag == tag) {
			output = j;
			break;
		}
	}

	return output;
}

// Return way number of input tag if it's valid. if not tag isn't found or isn't valid, return -1;
int findParsedAddressInL2(uint32_t tag, uint32_t set) {
	int output = -1;

	for (int j = 0; j < pow(2, L2Assoc); j++) {
		if (L2[set][j].valid && L2[set][j].tag == tag) {
			output = j;
			break;
		}
	}

	return output;
}

void updateLRU_OnAccessL1(uint32_t set, uint32_t way) {
	for (int j = 0; j < pow(2, L1Assoc); j++) {
		if ((L1[set][j].valid && j != way) &&
				((!L1[set][way].valid) ||
						(L1[set][way].valid && (L1[set][j].LRU < L1[set][way].LRU)))) {
			L1[set][j].LRU++;
		}
	}

	L1[set][way].LRU = 0;
	L1[set][way].valid = true;
}

void updateLRU_OnAccessL2(uint32_t set, uint32_t way) {
	for (int j = 0; j < pow(2, L2Assoc); j++) {
		if ((L2[set][j].valid && j != way) &&
				((!L2[set][way].valid) ||
						(L2[set][way].valid && (L2[set][j].LRU < L2[set][way].LRU)))) {
			L2[set][j].LRU++;
		}
	}

	L2[set][way].LRU = 0;
	L2[set][way].valid = true;
}

void invalidateWithLRU_L1(uint32_t set, uint32_t way) {
	for (int j = 0; j < pow(2, L1Assoc); j++) {
		if ((L1[set][j].valid && j != way) &&
				(L1[set][j].LRU > L1[set][way].LRU)) {
			L1[set][j].LRU--;
		}
	}

	L1[set][way].valid = false;
}

void invalidateWithLRU_L2(uint32_t set, uint32_t way) {
	for (int j = 0; j < pow(2, L2Assoc); j++) {
		if ((L2[set][j].valid && j != way) &&
				(L2[set][j].LRU > L2[set][way].LRU)) {
			L2[set][j].LRU--;
		}
	}

	L2[set][way].valid = false;
}

void processWrite(uint32_t address) {
	parsedAddress parsedL1 = parseAddressL1(address);
	parsedAddress parsedL2 = parseAddressL2(address);

	accessCountL1++;
	totalTime += L1Cyc;

	int wayNumberL1 = findParsedAddressInL1(parsedL1.tag, parsedL1.set);
	if (wayNumberL1 != -1) { // If found in L1
		L1[parsedL1.set][wayNumberL1].dirty = true;
		updateLRU_OnAccessL1(parsedL1.set, wayNumberL1);
		return;
	}

	// If not found in L1:
	missCountL1++;
	accessCountL2++;
	totalTime += L2Cyc;

	int wayNumberL2 = findParsedAddressInL2(parsedL2.tag, parsedL2.set);

	if (WrAlloc == 0) { // No Write Allocate
		if (wayNumberL2 != -1) { // If found in L2
			L2[parsedL2.set][wayNumberL2].dirty = true;
			updateLRU_OnAccessL2(parsedL2.set, wayNumberL2);
		} else { // If not found in L2
			totalTime += MemCyc;
			missCountL2++;
		}
	} else { // Write Allocate
		if (wayNumberL2 == -1) { // If not found in L2
			totalTime += MemCyc;
			missCountL2++;

			for (wayNumberL2 = 0;
					!((!L2[parsedL2.set][wayNumberL2].valid) || (L2[parsedL2.set][wayNumberL2].valid && (L2[parsedL2.set][wayNumberL2].LRU == (pow(2, L2Assoc) - 1))));
					wayNumberL2++) {
			} // This for loop updates wayNumberL2 to the way number that we'll write to in L2

			if (L2[parsedL2.set][wayNumberL2].valid) { // If replacing valid block in L2, snoop L1 for that block
				uint32_t toEvictAddress = (L2[parsedL2.set][wayNumberL2].tag << (L2Size - L2Assoc)) + (parsedL2.set << BSize);
				parsedAddress evictAddressParsedL1 = parseAddressL1(toEvictAddress);

				int toEvictWayNumberL1 = findParsedAddressInL1(evictAddressParsedL1.tag, evictAddressParsedL1.set);
				if (toEvictWayNumberL1 != -1) { // If found in L1
					invalidateWithLRU_L1(evictAddressParsedL1.set, toEvictWayNumberL1);
				}
			}

			L2[parsedL2.set][wayNumberL2].tag = parsedL2.tag;
			L2[parsedL2.set][wayNumberL2].dirty = false;
		}
		updateLRU_OnAccessL2(parsedL2.set, wayNumberL2);

		for (wayNumberL1 = 0;
				!((!L1[parsedL1.set][wayNumberL1].valid) || (L1[parsedL1.set][wayNumberL1].valid && (L1[parsedL1.set][wayNumberL1].LRU == (pow(2, L1Assoc) - 1))));
				wayNumberL1++) {
		} // This for loop updates wayNumberL1 to the way number that we'll write to in L1

		if (L1[parsedL1.set][wayNumberL1].valid && L1[parsedL1.set][wayNumberL1].dirty) { // If L1 block is dirty and needs to be copied to L2
			uint32_t toCopyAddress = (L1[parsedL1.set][wayNumberL1].tag << (L1Size - L1Assoc)) + (parsedL1.set << BSize);
			parsedAddress toCopyAddressParsedL2 = parseAddressL2(toCopyAddress);

			int toCopyWayNumberL2 = findParsedAddressInL2(toCopyAddressParsedL2.tag, toCopyAddressParsedL2.set);
			L2[toCopyAddressParsedL2.set][toCopyWayNumberL2].dirty = true;
			updateLRU_OnAccessL2(toCopyAddressParsedL2.set, toCopyWayNumberL2);
		}

		L1[parsedL1.set][wayNumberL1].tag = parsedL1.tag;
		L1[parsedL1.set][wayNumberL1].dirty = true;
		updateLRU_OnAccessL1(parsedL1.set, wayNumberL1);
	}
}

void processRead(uint32_t address) {
	parsedAddress parsedL1 = parseAddressL1(address);
	parsedAddress parsedL2 = parseAddressL2(address);

	accessCountL1++;
	totalTime += L1Cyc;

	int wayNumberL1 = findParsedAddressInL1(parsedL1.tag, parsedL1.set);
	if (wayNumberL1 != -1) { // If found in L1
		updateLRU_OnAccessL1(parsedL1.set, wayNumberL1);
		return;
	}

	// If not found in L1:
	missCountL1++;
	accessCountL2++;
	totalTime += L2Cyc;

	int wayNumberL2 = findParsedAddressInL2(parsedL2.tag, parsedL2.set);

	// -------------------------------------------------------
	if (wayNumberL2 == -1) { // If not found in L2
		totalTime += MemCyc;
		missCountL2++;

		for (wayNumberL2 = 0;
				!((!L2[parsedL2.set][wayNumberL2].valid) || (L2[parsedL2.set][wayNumberL2].valid && (L2[parsedL2.set][wayNumberL2].LRU == (pow(2, L2Assoc) - 1))));
				wayNumberL2++) {
		} // This for loop updates wayNumberL2 to the way number that we'll write to in L2

		if (L2[parsedL2.set][wayNumberL2].valid) { // If replacing valid block in L2, snoop L1 for that block
			uint32_t toEvictAddress = (L2[parsedL2.set][wayNumberL2].tag << (L2Size - L2Assoc)) + (parsedL2.set << BSize);
			parsedAddress evictAddressParsedL1 = parseAddressL1(toEvictAddress);

			int toEvictWayNumberL1 = findParsedAddressInL1(evictAddressParsedL1.tag, evictAddressParsedL1.set);
			if (toEvictWayNumberL1 != -1) { // If found in L1
				invalidateWithLRU_L1(evictAddressParsedL1.set, toEvictWayNumberL1);
			}
		}

		L2[parsedL2.set][wayNumberL2].tag = parsedL2.tag;
		L2[parsedL2.set][wayNumberL2].dirty = false;
	}
	updateLRU_OnAccessL2(parsedL2.set, wayNumberL2);

	for (wayNumberL1 = 0;
			!((!L1[parsedL1.set][wayNumberL1].valid) || (L1[parsedL1.set][wayNumberL1].valid && (L1[parsedL1.set][wayNumberL1].LRU == (pow(2, L1Assoc) - 1))));
			wayNumberL1++) {
	} // This for loop updates wayNumberL1 to the way number that we'll write to in L1

	if (L1[parsedL1.set][wayNumberL1].valid && L1[parsedL1.set][wayNumberL1].dirty) { // If L1 block is dirty and needs to be copied to L2
		uint32_t toCopyAddress = (L1[parsedL1.set][wayNumberL1].tag << (L1Size - L1Assoc)) + (parsedL1.set << BSize);
		parsedAddress toCopyAddressParsedL2 = parseAddressL2(toCopyAddress);

		int toCopyWayNumberL2 = findParsedAddressInL2(toCopyAddressParsedL2.tag, toCopyAddressParsedL2.set);
		L2[toCopyAddressParsedL2.set][toCopyWayNumberL2].dirty = true;
		updateLRU_OnAccessL2(toCopyAddressParsedL2.set, toCopyWayNumberL2);
	}

	L1[parsedL1.set][wayNumberL1].tag = parsedL1.tag;
	L1[parsedL1.set][wayNumberL1].dirty = false;
	updateLRU_OnAccessL1(parsedL1.set, wayNumberL1);
}

void printEntry(entry entryToPrint) {
	cout << "tag: " << entryToPrint.tag << ", valid: " << entryToPrint.valid << ", dirty: " << entryToPrint.dirty << ", LRU: " << entryToPrint.LRU << endl;
}

void printCache(entry **cache, unsigned howManySets, unsigned howManyWays, string cacheName) {
	cout << cacheName << ":" << endl;
	for (int i = 0; i < howManySets; i++) {
		for (int j = 0; j < howManyWays; j++) {
			cout << "Set " << i << ", way " << j << endl;
			printEntry(cache[i][j]);
			cout << "-----\n" << endl;
		}
	}
}