#include <MemorySystem.h>

class some_object
{
	public: 
		void read_complete(uint, uint64_t, uint64_t);
		void write_complete(uint, uint64_t, uint64_t);
		int add_one_and_run();
};
