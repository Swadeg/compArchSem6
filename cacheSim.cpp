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
#define NOT_VALID 1
#define NOT_FOUND -1

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
	unsigned getDirty(unsigned set) {return dirtyVector_[set];};
	void setDirty(unsigned set, bool dirty) { dirtyVector_[set] = dirty; }
};

way::way(unsigned waySize, unsigned blockSize)
{
	wayBlockSize_ = blockSize;
	tagVector_ = vector<unsigned>(waySize, 0);
	dirtyVector_ = vector<bool>(waySize, false);
	validVector_ = vector<bool>(waySize, false);
}

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
	//void write(uint32_t address, int way);
	unsigned makePlaceByLRUpolicy(uint32_t address, bool dirty, unsigned lruWay,int* way);
	unsigned checkValidation(uint32_t address, unsigned lruWay);
	void markDirtyBlock(uint32_t address, int way);
	unsigned getMissesNum();
	unsigned getHitsNum();
	unsigned getLRUWay(uint32_t address);
	void updateLRUCountReplace(uint32_t address, int way);
	void invalidate(uint32_t address, unsigned lruWay);

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
		LRUcount_[i] = vector<unsigned>(setNum_,0);
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

unsigned L::getLRUWay(uint32_t address)
{
	uint32_t set = (address >> blockSize_) & (uint32_t)(setNum_ - 1);
	for (unsigned i = 0; i < waysNum_; i++)
	{
		if (!ways_[i].getValid(set)||(ways_[i].getValid(set)&& LRUcount_[i][set] == 0))
			return i;
	}
	return 0;
}

uint32_t L::makePlaceByLRUpolicy(uint32_t address, bool dirty,unsigned lruWay ,int* way)
{
	uint32_t set = (address >> blockSize_) & (uint32_t)(setNum_ - 1);
	uint32_t tag = address >> (int)(blockSize_ + log2(setNum_));
	int oldTag;
	unsigned addrToReturn  = NOT_DIRTY_ADDR;  /*if real adress should have 2 LSB 0*/

	/*if its a dirty block, should write the old block to one level down, so return old tag*/
	if (lruWay >=0){
		if (ways_[lruWay].getValid(set) && ways_[lruWay].getDirty(set))
		{
			oldTag = ways_[lruWay].getTag(set);
			addrToReturn = ( oldTag << (int)(log2(setNum_)) ) | set;
			addrToReturn = addrToReturn << 2;
		}
		ways_[lruWay].setTag(set, tag); /*insert the new tag*/
		ways_[lruWay].setValid(set,true);
		ways_[lruWay].setDirty(set,dirty);
		updateLRUCountReplace(address,lruWay);
		if (way!=NULL)
			*way = lruWay;
	}
	return addrToReturn;
}

unsigned L::checkValidation(uint32_t address, unsigned lruWay){
	uint32_t set = (address >> blockSize_) & (uint32_t)(setNum_ - 1);
	int oldTag;
	unsigned addrToReturn  = NOT_VALID;  /*if real adress should have 2 LSB 0*/
	if(lruWay >=0 && ways_[lruWay].getValid(set)){
		oldTag = ways_[lruWay].getTag(set);
		addrToReturn = ( oldTag << (int)(log2(setNum_)) ) | set;
		addrToReturn = addrToReturn << 2;
	}
	return addrToReturn;
}

void L::markDirtyBlock(uint32_t address, int way) /*assumes that tag is already exist*/
{
	uint32_t set = (address >> blockSize_) & (uint32_t)(setNum_ - 1);
	ways_[way].setDirty(set,true);
}

void L::updateLRUCountReplace(uint32_t address, int way)
{
	uint32_t set = (address >> blockSize_) & (uint32_t)(setNum_ - 1);
	for (unsigned i = 0; i < waysNum_; i++)
	{
		bool largerLRU = (ways_[way].getValid(set) && LRUcount_[i][set] > LRUcount_[way][set]) || !ways_[way].getValid(set);
		if (largerLRU && ways_[i].getValid(set) && int(i)!=way ){
			LRUcount_[i][set]--;
		}
	}
	LRUcount_[way][set] = waysNum_-1;
	ways_[way].setValid(set,true);
}
void L::invalidate(uint32_t address, unsigned lruWay){
	uint32_t set = (address >> blockSize_) & (uint32_t)(setNum_ - 1);
	for (unsigned i = 0; i < waysNum_; i++)
	{
		bool smallerLRU = (LRUcount_[i][set] < LRUcount_[lruWay][set]);
		if (smallerLRU && ways_[i].getValid(set) && i!=lruWay ){
			LRUcount_[i][set]++;
		}
	}
	LRUcount_[lruWay][set]=0;
	ways_[lruWay].setValid(set,false);
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
	unsigned L1accessNum = 0;
	unsigned L2accessNum = 0;
	unsigned totTime = 0;
	//unsigned totAccessNum = 0;

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
		//cout << "operation: " << operation;
		string cutAddress = address.substr(2); // Removing the "0x" part of the address
		// DEBUG - remove this line
		//cout << ", address (hex)" << cutAddress;

		/************************ manage the cashe *********************************/

		uint32_t addr = static_cast<uint32_t>(std::stoul(cutAddress, nullptr, 16));
		int oldAddr, oldAddrValid; unsigned lruWay;
		if (operation=='r')
		{
			int wayFound = l1.lookForTag(addr);
			L1accessNum++;
			totTime += L1Cyc;
			if (wayFound != NOT_FOUND) /*lookForTag returns true if tag is in L1*/
			{
				l1.readHit();
				l1.updateLRUCountReplace(addr,wayFound);
			}
			else
			{
				l1.readMiss();
				L2accessNum++;
				totTime +=L2Cyc;
				wayFound = l2.lookForTag(addr);
				if (wayFound != NOT_FOUND) // found in L2
				{
					l2.readHit();
					l2.updateLRUCountReplace(addr,wayFound);
				}
				else
				{
					l2.readMiss();
					memAccNum++;	/*access mem to read data from there*/
					totTime+= MemCyc;
					lruWay = l2.getLRUWay(addr);
					oldAddrValid = l2.checkValidation(addr,lruWay);
					if (oldAddrValid != NOT_VALID){
						wayFound =l1.lookForTag(oldAddrValid);
						if (wayFound != NOT_FOUND)
							l1.invalidate(oldAddrValid,wayFound);
					}
					l2.makePlaceByLRUpolicy(addr,false, lruWay, &wayFound);
				}
				oldAddr = l1.makePlaceByLRUpolicy(addr,false, l1.getLRUWay(addr), &wayFound);
				if (oldAddr != NOT_DIRTY_ADDR){
					lruWay = l2.getLRUWay(oldAddr);
					l2.markDirtyBlock(oldAddr,lruWay);
					l2.updateLRUCountReplace(oldAddr,lruWay);
				}
			}
		}
		
		
		else if (operation=='w')
		{
			int wayFound = l1.lookForTag(addr); 
			L1accessNum++;
			totTime += L1Cyc;
			if (wayFound != NOT_FOUND) //tag is in L1
			{
				l1.writeHit(); //measurment
				//l1.write(addr);
				l1.markDirtyBlock(addr,wayFound); //mark written block in L1 as dirty
				l1.updateLRUCountReplace(addr, wayFound);
			}
			else
			{
				l1.writeMiss();
				L2accessNum++;
				totTime+= L2Cyc;
				wayFound =l2.lookForTag(addr);
				if (wayFound != NOT_FOUND) //tag isnt in L1 but in L2
				{
					l2.writeHit();
					if (WrAlloc)
					{
						l2.updateLRUCountReplace(addr,wayFound);
						oldAddr = l1.makePlaceByLRUpolicy(addr,true,l1.getLRUWay(addr),NULL);
						if (oldAddr!=NOT_DIRTY_ADDR) 				//write old tag that was in L1 to L2
						{
							lruWay = l2.getLRUWay(oldAddr);
							l2.markDirtyBlock(oldAddr,lruWay);
							l2.updateLRUCountReplace(oldAddr,lruWay);
						}
					}
					else
					{
						l2.markDirtyBlock(addr,wayFound);
						l2.updateLRUCountReplace(addr,wayFound);
					}
				}
				else //tag isnt in L1 and isnt in L2
				{
					l2.writeMiss();
					totTime+=MemCyc;
					memAccNum++; //access mem to write new data there
					if (WrAlloc)
					{
						lruWay = l2.getLRUWay(addr);
						oldAddrValid = l2.checkValidation(addr,lruWay);
						if (oldAddrValid != NOT_VALID){
							wayFound =l1.lookForTag(oldAddrValid);
							if (wayFound != NOT_FOUND)
								l1.invalidate(oldAddrValid,wayFound);
						}
							
						
						oldAddr = l2.makePlaceByLRUpolicy(addr,lruWay,false,NULL); //insert new tag to cache
						if (oldAddr!=NOT_DIRTY_ADDR) memAccNum++; //access mem to write old tag
						//l2.write(addr,wayFound);
						lruWay = l1.getLRUWay(addr);
						oldAddr = l1.makePlaceByLRUpolicy(addr,true, lruWay,&wayFound);
						if (oldAddr!=NOT_DIRTY_ADDR) 				//write old tag that was in L1 to L2
						{
							lruWay = l2.getLRUWay(oldAddr);
							l2.markDirtyBlock(oldAddr,lruWay);
							l2.updateLRUCountReplace(oldAddr,lruWay);
						}
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
		//cout << " (dec) " << num << endl;
	}

	double L1MissRate;
	double L2MissRate;
	//double L1HitsRate;
	//double L2HitsRate;
	double avgAccTime;
	unsigned L1MissesNum = l1.getMissesNum();
	//unsigned L1HitsNum = l1.getHitsNum();
	unsigned L2MissesNum = l2.getMissesNum();
	//unsigned L2HitsNum = l2.getHitsNum();

	L1MissRate = (double)L1MissesNum / (L1accessNum);
	L2MissRate = (double)L2MissesNum / (L2accessNum);
	avgAccTime = (double)totTime / (L1accessNum);
	//avgAccTime = (1-L1MissRate) * L1Cyc + L1MissRate * (1-L2MissRate) * (L1Cyc + L2Cyc) + L1MissRate * L2MissRate * (L1Cyc + L2Cyc + MemCyc);
	// L1MissRate = (double)L1MissesNum / (L1MissesNum + L1HitsNum);
	// L1HitsRate = (double)L1HitsNum / (L1MissesNum + L1HitsNum);
	// L2MissRate = (double)L2MissesNum / (L2MissesNum + L2HitsNum);
	// L2HitsRate = (double)L2HitsNum / (L2MissesNum + L2HitsNum);
	// avgAccTime = L1HitsRate * L1Cyc + L1MissRate * L2HitsRate * (L1Cyc + L2Cyc) + L1MissRate * L2MissRate * (L1Cyc + L2Cyc + MemCyc);

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
