	
    
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


    /*way class functions*/


    /*L class functions*/
    /*return true if valid tag is exist in cache*/
    bool lookForValidTag(uint32_t address);
    /*for specific set in cache, update the lru count of each way*/
    void lruUpdate(uint32_t adderss);
    /*return the way for write data to it*/
    unsigned getVictimWay(uint32_t addresss);
    /*return true if the block in this way is dirty*/
    bool isDirtyWay(unsigned way, uint32_t address);



	unsigned getTag(unsigned set) { };//return tagVector_[set]; }
	void setTag(unsigned set, int tag) {};// tagVector_[set] = tag; }
	bool getValid(unsigned set) { };//return validVector_[set]; }
	void setValid(unsigned set,  bool valid) {};// validVector_[set] = valid; }
	unsigned getDirty(unsigned set) {};//return dirtyVector_[set];};
	void setDirty(unsigned set, bool dirty) { };//dirtyVector_[set] = dirty; }

	
	void readHit();
	void readMiss();
	void writeHit();
	void writeMiss();
	unsigned makePlaceByLRUpolicy(uint32_t address, bool dirty, unsigned lruWay,int* way);
	unsigned checkValidation(uint32_t address, unsigned lruWay);
	void markDirtyBlock(uint32_t address, int way);
	unsigned getMissesNum();
	unsigned getHitsNum();
	unsigned getLRUWay(uint32_t address);
	void updateLRUCountReplace(uint32_t address, int way);
	void invalidate(uint32_t address, unsigned lruWay);