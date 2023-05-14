/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;



class way
{
      unsigned block_size_;
      unsigned num_set_;
      vector<int> tag_vector_;
      vector<vector<bool>> Block_Matrix_;
      
    public:
      way(unsigned block_size, unsigned num_set);
      void write(uint32_t address);
      bool read(uint32_t address);
};



class L
{
  unsigned Bsize_;
  unsigned Lsize_;
  unsigned Lcyc_;
  unsigned Lassoc_;
  unsigned WrAlloc_;
  vector<class way>ways_;
  vector<unsigned> count_;
  
  public:
    L(unsigned Bsize, unsigned Lsize, unsigned Lcyc, unsigned Lassoc, unsigned WrAlloc);
    count_update();
    
};

way::way(unsigned block_size, unsigned num_set)
{
  block_size_ = block_size;
  num_set_ = num_set;
  tag_vector_= vector<int>(pow(2,num_set_), -1);
  Block_Matrix_ = vector<vector<bool>>(pow(2,num_set_), vector<bool>(pow(2,block_size_-2), false));
}

bool way::write(uint32_t address)
{
  uint32_t shifted_add = address>>2;
  unsigned offset = (pow(2,block_size_-2) -1)^ shifted_add;
  shifted_add = address>>block_size_;
  unsigned set = (pow(2,num_set_)-1) ^ shifted_add;
  int tag = shifted_add >> num_set_;
  tag_vector_[set] = tag;
  Block_Matrix_[set][offset] = true;
}

bool way::read(uint32_t address)
{
  uint32_t shifted_add = address>>2;
  unsigned offset = (pow(2,block_size_-2) -1)^ shifted_add;
  shifted_add = address>>block_size_;
  unsigned set = (pow(2,num_set_)-1) ^ shifted_add;
  int tag = shifted_add >> num_set_;
  if (tag_vector_[set] == tag)
    return Block_Matrix_[set][offset];
  return false;  
}


L::L(unsigned Bsize, unsigned Lsize, unsigned Lcyc, unsigned Lassoc, unsigned WrAlloc)
{
  Bsize_ = Bsize;
  Lsize_ = Lsize;
  Lcyc_ = Lcyc;
  Lassoc_ = Lassoc;
  WrAlloc_ = WrAlloc;
  
  unsigned num_blocks = Lsize_ - Bsize_;
  unsigned num_set = num_blocks - Lassoc_;
  way init_way(Bsize_, num_set);
  ways_ = vector<class way>(pow(2,Lassoc_),init_way);
  count_ = vector<unsigned>(pow(2,Lassoc_),0);
  for (int i=0,i<count_.size(); i++)
  {
    count_[i]=i;
  }
  
}

bool L::read(uint32_t address)
{
  uint32_t shifted_add = address>>2;
  unsigned offset = (pow(2,block_size_-2) -1)^ shifted_add;
  shifted_add = address>>block_size_;
  unsigned set = (pow(2,num_set_)-1) ^ shifted_add;
  int tag = shifted_add >> num_set_;
  for(int i=0;i<ways_size();i++)
  {
    if (ways_[i].read(address))
    {
      count_update();
      return true; //fix address
    }
  }
  /*data not found*/
  return false;
}

bool L::write(uint32_t address)
{
  int lru_idx;
  for(int i=0; i<count_size(); i++)
  {
    if(count_[i]==0 && ways_[i].read(address))
    {
      lru_idx=i;
      count_update();
      return true;
    }
  }
  return false;
}

int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1SLize = atoi(argv[i + 1]);
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
        
        L l1(BSize, L1SLize, L1Cyc, L1Assoc, WrAlloc);
        L l2(BSize, L2SLize, L2Cyc, L2Assoc, WrAlloc);
        unsigned read_miss_cyc=0;
        unsigned write_miss_cyc=0;
        
	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address
      
		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;
                uint32_t address = std::stoi(cutAddress, nullptr, 16);
		
		if(operation=="r")
		{
		  if (l1.read(address)) //read hit
		  {
		    read_hit_cyc+=L1Cyc;
		  }
		  else if(l2.read(address))
		  {
		    read_hit_cyc+=L2Cyc;
		  }
		  else
		  {
		    read_hit_cyc+=MemCyc;
		  }
		}
		else if(operation=="w")
		{
		  if (WrAlloc)
		  {
		    if(!l1.read(address)) // not found in l1
		    {
		      write_miss_cyc += L1Cyc;
		      if(!l2.read(address))
		      {
		        write_miss_cyc += L2Cyc;
		      }
		      else
		      {
		        
		      }
		      l1.write(address);
		      l2.write(address);
		    }
		  }
		}
		
		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		cout << " (dec) " << num << endl;

	}

	double L1MissRate;
	double L2MissRate;
	double avgAccTime;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
