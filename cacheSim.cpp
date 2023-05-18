/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <vector>

using std::cerr;
using std::cout;
using std::endl;
using std::FILE;
using std::ifstream;
using std::string;
using std::stringstream;

using namespace std;

class way
{
	vector<int> tagVector_;
	vector<bool> dirtyVector_;
	vector<bool> validVector_;

public:
	way(unsigned waySize);
	~way(){};
	int getTag(unsigned set) { return tagVector_[set]; }
	void setTag(unsigned set, int tag) { tagVector_[set] = tag; }
	bool getValid(unsigned set) { return validVector_[set]; }
	void setValid(unsigned set, bool valid) { validVector_[set] = valid; }
	bool getDirty(unsigned set) { return dirtyVector_[set]; }
	void setDirty(unsigned set, bool dirty) { dirtyVector_[set] = dirty; }
};

way::way(unsigned waySize)
{
	tagVector_ = vector<int>(waySize, 0);
	dirtyVector_ = vector<bool>(waySize, false);
	validVector_ = vector<bool>(waySize, false);
}

class L
{
	unsigned waysNum_;
	unsigned waySize_; /*also the size of tagVector_*/
	unsigned blockSize_;
	unsigned missesNum_;
	unsigned hitsNum_;
	vector<way> ways_;
	vector<unsigned> LRUcount_; /*size of waysNum_*/

public:
	L(unsigned BSize, unsigned LSize, unsigned LCyc, unsigned LAssoc); /*fill with parameters*/
	~L(){};
	bool lookForTag(uint32_t address);
	void readHit();
	void readMiss();
	void writeHit();
	void writeMiss();
	void write(uint32_t address);
	void makePlaceByLRUpolicy(uint32_t address);
	void markDirtyBlock(uint32_t address);
	unsigned getMissesNum();
	unsigned getHitsNum();
};

L::L(unsigned BSize, unsigned LSize, unsigned LCyc, unsigned LAssoc)
{
	missesNum_ = 0;
	hitsNum_ = 0;
	blockSize_ = BSize;
	waysNum_ = pow(2, LAssoc);
	waySize_ = pow(2, ((LSize - BSize) - LAssoc)); /* #blocks/waysNum = waySize */
	way initWay(waySize_);
	ways_ = vector<way>(waysNum_, initWay);
	LRUcount_ = vector<unsigned>(waysNum_, 0);
	for (int i = 0; i < waysNum_; i++)
	{
		LRUcount_[i] = i;
	}
}

bool L::lookForTag(uint32_t address)
{
	uint32_t set = (address >> blockSize_) & (uint32_t)(waySize_ - 1);
	uint32_t tag = address >> (int)(blockSize_ + log2(waySize_));
	for (int i = 0; i < waysNum_; i++)
	{
		if (ways_[i].getTag(set) == tag)
			return true;
	}
	return false;
}

void L::readHit()
{
	hitsNum_++;
}

void L::readMiss()
{
	missesNum_++;
}

void L::writeHit()
{
	hitsNum_++;
}

void L::writeMiss()
{
	missesNum_++;
}

void L::makePlaceByLRUpolicy(uint32_t address)
{
	uint32_t set = (address >> blockSize_) & (uint32_t)(waySize_ - 1);
	uint32_t tag = address >> (int)(blockSize_ + log2(waySize_));
	unsigned lruWay = 0;
	for (int i = 0; i < waysNum_; i++)
	{
		if (LRUcount_[i] == 0)
		{
			lruWay = i;
			break;
		}
	}
	/*if its a dirty block, should write the old block to one level down*/
	ways_[lruWay].setTag(set, tag);
}

void L::write(uint32_t address)
{
	uint32_t set = (address >> blockSize_) & (uint32_t)(waySize_ - 1);
	uint32_t tag = address >> (int)(blockSize_ + log2(waySize_));
	for (int i = 0; i < waysNum_; i++)
	{
		if (ways_[i].getTag(set) == tag)
		{
			ways_[i].setValid(set, true);
			break;
		}
	}
}

void L::markDirtyBlock(uint32_t address) /*assumes that tag is already exist*/
{
	uint32_t set = (address >> blockSize_) & (uint32_t)(waySize_ - 1);
	uint32_t tag = address >> (int)(blockSize_ + log2(waySize_));
	for (int i = 0; i < waysNum_; i++)
	{
		if (ways_[i].getTag(set) == tag)
		{
			ways_[i].setDirty(set, true);
			break;
		}
	}
}

unsigned L::getHitsNum() { return hitsNum_; }
unsigned L::getMissesNum() { return missesNum_; }

/************************************************************************************/
int main(int argc, char **argv)
{

	if (argc < 19)
	{
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char *fileString = argv[1];
	ifstream file(fileString); // input file stream
	string line;
	if (!file || !file.good())
	{
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			 L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2)
	{
		string s(argv[i]);
		if (s == "--mem-cyc")
		{
			MemCyc = atoi(argv[i + 1]);
		}
		else if (s == "--bsize")
		{
			BSize = atoi(argv[i + 1]);
		}
		else if (s == "--l1-size")
		{
			L1Size = atoi(argv[i + 1]);
		}
		else if (s == "--l2-size")
		{
			L2Size = atoi(argv[i + 1]);
		}
		else if (s == "--l1-cyc")
		{
			L1Cyc = atoi(argv[i + 1]);
		}
		else if (s == "--l2-cyc")
		{
			L2Cyc = atoi(argv[i + 1]);
		}
		else if (s == "--l1-assoc")
		{
			L1Assoc = atoi(argv[i + 1]);
		}
		else if (s == "--l2-assoc")
		{
			L2Assoc = atoi(argv[i + 1]);
		}
		else if (s == "--wr-alloc")
		{
			WrAlloc = atoi(argv[i + 1]);
		}
		else
		{
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

	/* initiate the cashes */
	L l1(BSize, L1Size, L1Cyc, L1Assoc);
	L l2(BSize, L2Size, L2Cyc, L2Assoc);
	unsigned memAccNum = 0;

	while (getline(file, line))
	{

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address))
		{
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		cout << "operation: " << operation;
		string cutAddress = address.substr(2); // Removing the "0x" part of the address
		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;

		/************************ manage the cashe *********************************/
		uint32_t addr = std::stoul(cutAddress); /*address as uint32_t*/
		if (operation=='r')
		{
			if (l1.lookForTag(addr)) /*lookForTag returns true if tag is in L1*/
			{
				l1.readHit();
			}
			else
			{
				l1.readMiss();
				if (l2.lookForTag(addr))
				{
					l2.readHit();
				}
				else
				{
					l2.readMiss();
					memAccNum++;				   /*access mem to read data from there*/
					l2.makePlaceByLRUpolicy(addr); /*insert new tag to cache*/
					l2.write(addr);
					l1.makePlaceByLRUpolicy(addr);
					l1.write(addr);
				}
			}
		}

		else if (operation=='w')
		{
			if (l1.lookForTag(addr)) /*tag is in L1*/
			{
				l1.writeHit();
				l1.write(addr);
				l1.markDirtyBlock(addr); /*mark written block in L1 as dirty*/
			}
			else
			{
				l1.writeMiss();
				if (l2.lookForTag(addr)) /*tag isnt in L1 but in L2*/
				{
					l2.writeHit();
					if (WrAlloc)
					{
						l2.write(addr);
						l2.markDirtyBlock(addr);
						l1.makePlaceByLRUpolicy(addr);
						l1.write(addr);
					}
					else /*no write allocate*/
					{
						l2.write(addr);
						l2.markDirtyBlock(addr);
					}
				}
				else /*tag isnt in L1 and isnt in L2*/
				{
					l2.writeMiss();
					memAccNum++; /*access mem to write new data there*/
					if (WrAlloc)
					{
						l2.makePlaceByLRUpolicy(addr);
						l2.write(addr);
						l1.makePlaceByLRUpolicy(addr);
						l1.write(addr);
					}
					else /*no write allocate*/
					{
						/*nothing*/
					}
				}
			}
		}
		/**********************************************************************/

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);
		// DEBUG - remove this line
		cout << " (dec) " << num << endl;
	}

	double L1MissRate;
	double L2MissRate;
	double L1HitsRate;
	double L2HitsRate;
	double avgAccTime;
	unsigned L1MissesNum = l1.getMissesNum();
	unsigned L1HitsNum = l1.getHitsNum();
	unsigned L2MissesNum = l2.getMissesNum();
	unsigned L2HitsNum = l2.getHitsNum();

	L1MissRate = (double)L1MissesNum / (L1MissesNum + L1HitsNum);
	L1HitsRate = (double)L1HitsNum / (L1MissesNum + L1HitsNum);
	L2MissRate = (double)L2MissesNum / (L2MissesNum + L2HitsNum);
	L2HitsRate = (double)L2HitsNum / (L2MissesNum + L2HitsNum);
	avgAccTime = L1HitsRate * L1Cyc + L1MissRate * L2HitsRate * (L1Cyc + L2Cyc) + L1MissRate * L2MissRate * (L1Cyc + L2Cyc + MemCyc);

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
