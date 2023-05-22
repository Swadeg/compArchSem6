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

#define NOT_DIRTY_ADDR 1
#define NO_DIRTY_OFFSET -1

class way
{
	unsigned wayBlockSize_;
	vector<unsigned> tagVector_;
	vector<bool> dirtyVector_;
	vector<bool> validVector_;

public:
	way(unsigned waySize, unsigned blockSize);
	~way(){};
	unsigned getTag(unsigned set) { return tagVector_[set]; }
	void setTag(unsigned set, int tag) { tagVector_[set] = tag; }
	bool getValid(unsigned set) { return validVector_[set]; }
	void setValid(unsigned set,  bool valid) { validVector_[set] = valid; }
	unsigned getDirty(unsigned set);
	void setDirty(unsigned set, bool dirty) { dirtyVector_[set] = dirty; }
};

way::way(unsigned waySize, unsigned blockSize)
{
	wayBlockSize_ = blockSize;
	tagVector_ = vector<unsigned>(waySize, 0);
	dirtyVector_ = vector<bool>(waySize, false);
	validVector_ = vector<bool>(waySize, false);
}

unsigned way::getDirty(unsigned set){
	return dirtyVector_[set];
}

// unsigned way::getDirty(unsigned set)
// {
// 	unsigned offsetToReturn = NO_DIRTY_OFFSET;
// 	for (int off=0; off<pow(2,wayBlockSize_); off++ )
// 	{
// 		if (dirtyVector_[set][off]==true) 
// 		{
// 			offsetToReturn = off;
// 		}
// 	}
// 	return offsetToReturn;
// }

class L
{
	unsigned waysNum_;
	unsigned setNum_; /*also the size of tagVector_*/
	unsigned blockSize_;
	unsigned missesNum_;
	unsigned hitsNum_;
	vector<way> ways_;
	vector<vector<unsigned>> LRUcount_; /*size of waysNum_*setNum__*/

public:
	L(unsigned BSize, unsigned LSize, unsigned LCyc, unsigned LAssoc); /*fill with parameters*/
	~L(){};
	int lookForTag(uint32_t address);
	void readHit();
	void readMiss();
	void writeHit();
	void writeMiss();
	void write(uint32_t address, int way);
	unsigned makePlaceByLRUpolicy(uint32_t address, int* way);
	void markDirtyBlock(uint32_t address, int way);
	unsigned getMissesNum();
	unsigned getHitsNum();
	void updateLRUCount(uint32_t address, int way);
	void updateLRUCountReplace(uint32_t address, int way);

};

L::L(unsigned BSize, unsigned LSize, unsigned LCyc, unsigned LAssoc)
{
	missesNum_ = 0;
	hitsNum_ = 0;
	blockSize_ = BSize;
	waysNum_ = pow(2, LAssoc);
	setNum_ = pow(2, ((LSize - BSize) - LAssoc)); /* #blocks/waysNum = setsNum */
	way initWay(setNum_, blockSize_);
	ways_ = vector<way>(waysNum_, initWay);
	LRUcount_ = vector<vector<unsigned>>(waysNum_, vector<unsigned>(setNum_,0));
	for (unsigned i = 0; i < waysNum_; i++)
	{
		LRUcount_[i] = vector<unsigned>(setNum_,i);
	}
}

int L::lookForTag(uint32_t address)
{
	uint32_t set = (address >> blockSize_) & (uint32_t)(setNum_ - 1);
	uint32_t tag = address >> (int)(blockSize_ + log2(setNum_));
	for (unsigned i = 0; i < waysNum_; i++)
	{
		if ((ways_[i].getTag(set) == tag) && ways_[i].getValid(set))
			return i;
	}
	return -1;
}

void L::readHit(){hitsNum_++;}

void L::readMiss(){missesNum_++;}

void L::writeHit(){hitsNum_++;}

void L::writeMiss() { missesNum_++; }

uint32_t L::makePlaceByLRUpolicy(uint32_t address, int* way)
{
	uint32_t set = (address >> blockSize_) & (uint32_t)(setNum_ - 1);
	uint32_t tag = address >> (int)(blockSize_ + log2(setNum_));
	unsigned lruWay = 0;
	int oldTag;
	unsigned addrToReturn  = NOT_DIRTY_ADDR;  /*if real adress should have 2 LSB 0
	for (unsigned i = 0; i < waysNum_; i++)
	{
		if (LRUcount_[i][set] == 0)
		{
			lruWay = i;
			break;
		}
	}*/
	/*if its a dirty block, should write the old block to one level down, so return old tag*/
	if ( ways_[lruWay].getDirty(set))
	{
		oldTag = ways_[lruWay].getTag(set);
		addrToReturn = ( oldTag << (int)(log2(setNum_)) ) | set;
		addrToReturn = addrToReturn << 2;
	}
	ways_[lruWay].setTag(set, tag); /*insert the new tag*/
	if (way != NULL)
		*way = int(lruWay);
	return addrToReturn;
}

void L::write(uint32_t address, int way)
{
	//uint32_t offset = (address>>2) & (uint32_t)(pow(2, blockSize_-2) - 1);
	uint32_t set = (address >> blockSize_) & (uint32_t)(setNum_ - 1);
	int tagSize = 32 - blockSize_ - (int)log2(setNum_); 
	uint32_t tag = (address >> (blockSize_ + (int)log2(setNum_))) & (uint32_t)(pow(2,tagSize)-1);	
	if (ways_[way].getTag(set) == tag){
		ways_[way].setValid(set,true);
	}
	// for (int i = 0; i < waysNum_; i++)
	// {
	// 	if (ways_[i].getTag(set) == tag)
	// 	{
	// 		ways_[i].setValid(set, true);
	// 		break;
	// 	}
	// }
}

void L::markDirtyBlock(uint32_t address, int way) /*assumes that tag is already exist*/
{
	uint32_t set = (address >> blockSize_) & (uint32_t)(setNum_ - 1);
	//uint32_t tag = address >> (int)(blockSize_ + log2(setNum_));
	ways_[way].setDirty(set,true);
}

void L::updateLRUCountReplace(uint32_t address, int way)
{
	uint32_t set = (address >> blockSize_) & (uint32_t)(setNum_ - 1);
	//uint32_t tag = address >> (int)(blockSize_ + log2(setNum_));
	for (unsigned i = 0; i < waysNum_; i++)
	{
		bool largerLRU = (LRUcount_[i][set] > LRUcount_[way][set]) || !ways_[way].getValid(set);
		if (largerLRU && ways_[i].getValid(set) && int(i)!=way ){
			LRUcount_[i][set]--;
		}
	}
	LRUcount_[way][set] = waysNum_-1;
	ways_[way].setValid(set,true);
	
}

void L::updateLRUCount(uint32_t address, int way){
	uint32_t set = (address >> blockSize_) & (uint32_t)(setNum_ - 1);
	//uint32_t tag = address >> (int)(blockSize_ + log2(setNum_));
	for (unsigned i = 0; i < waysNum_; i++)
	{
		bool largerLRU = LRUcount_[i][set] > LRUcount_[way][set];
		if (largerLRU && ways_[i].getValid(set) && int(i)!=way){
			LRUcount_[i][set]--;
		}
	}
	ways_[way].setValid(set, false);
}
// {
// 	uint32_t set = (address >> blockSize_) & (uint32_t)(setNum_ - 1);
// 	int tagSize = 32 - blockSize_ - (int)log2(setNum_); 
// 	uint32_t tag = (address >> (blockSize_ + (int)log2(setNum_))) & (uint32_t)(pow(2,tagSize)-1);	for (int i = 0; i < waysNum_; i++)
// 	{
// 		if (ways_[i].getTag(set) == tag)
// 		{
// 			for(int j=waysNum_-1; j>=0; j--)
// 			{
// 				if(j!=i && LRUcount_[j][set]>0)LRUcount_[j][set]--;
// 			}
// 			LRUcount_[i][set] = waysNum_-1;
// 			return;
// 		}
// 	}
// }

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
	char *fileString = argv[1];           /*replace with argv[1] in the end*/
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
		//uint32_t addr = std::stoul(cutAddress); /*address as uint32_t*/
		uint32_t addr = static_cast<uint32_t>(std::stoul(cutAddress, nullptr, 16));
		int oldAddr;
		if (operation=='r')
		{
			int wayFound = l1.lookForTag(addr);
			if (wayFound != -1) /*lookForTag returns true if tag is in L1*/
			{
				l1.readHit();
				l1.updateLRUCountReplace(addr,wayFound);
			}
			else
			{
				l1.readMiss();
				wayFound = l2.lookForTag(addr);
				if (wayFound != -1)
				{
					l2.readHit();
					l2.updateLRUCountReplace(addr,wayFound);
				}
				else
				{
					l2.readMiss();
					memAccNum++;						    /*access mem to read data from there*/
					oldAddr = l2.makePlaceByLRUpolicy(addr,&wayFound); /*insert new tag to cache*/
					if (oldAddr!=NOT_DIRTY_ADDR) memAccNum++; /*access mem to erite old tag*/
					l2.write(addr,wayFound);
					int L1way;
					oldAddr = l1.makePlaceByLRUpolicy(addr,&L1way);
					if (oldAddr!=NOT_DIRTY_ADDR) 				/*write old tag that was in L1 to L2*/
					{
						if (l2.makePlaceByLRUpolicy(oldAddr,&wayFound)!=NOT_DIRTY_ADDR) memAccNum++;
						l2.write(oldAddr,wayFound);
					}
					l1.write(addr,L1way);
				}
			}
		}
		
		
		else if (operation=='w')
		{
			int wayFound = l1.lookForTag(addr); 
			if (wayFound != -1) //tag is in L1
			{
				l1.writeHit(); //measurment
				//l1.write(addr);
				l1.markDirtyBlock(addr,wayFound); //mark written block in L1 as dirty
				l1.updateLRUCountReplace(addr, wayFound);
			}
			else
			{
				l1.writeMiss();
				wayFound =l2.lookForTag(addr);
				if (wayFound != -1) //tag isnt in L1 but in L2
				{
					l2.writeHit();
					if (WrAlloc)
					{
						//l2.write(addr);
						//l2.markDirtyBlock(addr);
						//l2.updateLRUCountReplace(addr,wayFound);
						oldAddr = l1.makePlaceByLRUpolicy(addr,NULL);
						if (oldAddr!=NOT_DIRTY_ADDR) 				//write old tag that was in L1 to L2
						{
							if (l2.makePlaceByLRUpolicy(oldAddr,NULL)!=NOT_DIRTY_ADDR) memAccNum++;
							//l2.write(oldAddr);
							l2.updateLRUCountReplace(addr,wayFound);
						}
						l1.write(addr,wayFound);
						l1.updateLRUCountReplace(addr,wayFound);
					}
					else //no write allocate
					{
						//l2.write(addr);
						l2.updateLRUCountReplace(addr,wayFound);
					}
					l2.markDirtyBlock(addr,wayFound);
				}
				else //tag isnt in L1 and isnt in L2
				{
					l2.writeMiss();
					memAccNum++; //access mem to write new data there
					if (WrAlloc)
					{
						oldAddr = l2.makePlaceByLRUpolicy(addr,&wayFound); //insert new tag to cache
						if (oldAddr!=NOT_DIRTY_ADDR) memAccNum++; //access mem to erite old tag
						l2.write(addr,wayFound);
						int L1way;
						oldAddr = l1.makePlaceByLRUpolicy(addr,&L1way);
						if (oldAddr!=NOT_DIRTY_ADDR) 				//write old tag that was in L1 to L2
						{
							if (l2.makePlaceByLRUpolicy(oldAddr,&wayFound)!=NOT_DIRTY_ADDR) memAccNum++;
							l2.write(oldAddr,wayFound);
							l1.markDirtyBlock(oldAddr,L1way);
						}
						l1.write(addr, L1way);
					}
					else //no write allocate
					{
						//nothing
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
