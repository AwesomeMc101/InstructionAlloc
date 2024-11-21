#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <sstream>
#include <string>

class MemoryManager {
private:
	struct lPair {
		int* addr;
		size_t s;
	};

	size_t _s;
	bool* allocated;
	lPair* allocated_p;
	size_t aps=0;
public:
	int* memory;
	MemoryManager(size_t s) {
		_s = s;
		memory = static_cast<int*>(malloc(s));
		allocated = static_cast<bool*>(calloc(s, 1)); //S number of allocated bytes
	}
	~MemoryManager() {
		//std::free(memory);
	//	std::free(allocated);
	}

	int* request(size_t s) { //Call with bytes
		//4 ints = 4*sizeof(int) == s


		//Find non-allocated memory
		bool* begin = allocated;  //Starting ptr.
		bool found = false; //Bool to track.
		int n = 0; //Counter
		while (begin != allocated + _s && found == false) { //Ensure begin does not equal the end of the byte array. Auto break when found == true.
			n = (*begin == 0 ? (n + 1) : 0); //==0->Unallocated, increment; ==1->Allocated, reset counter
			if (n >= s) { found = true;  break; } //Finish.
			begin++; //Inc the pointer
		}
		if (!found) { printf("Unable to allocate %d bytes!\n", s); return nullptr; } //Insufficient memory.


		int* return_pointer = memory + ((begin - allocated - n + 1) / sizeof(int)); //Subtract pointers. Subtract n to get the first but add a magic number cuz memory is fun.

		//Set bool arr to 1s
		for (int i = 0; i < s; i++) {
			*(allocated + (begin - allocated - n + 1) + i) = 1; //Pointer arith. Take the allocated array. Add the distance to the first byte we need to set to 1. Then add i to count up.
		}

		lPair* nlp = static_cast<lPair*>(malloc((aps + 1) * sizeof(lPair))); //Realloc array of datapairs. (Used for free())
		memcpy(nlp, allocated_p, (aps * sizeof(lPair))); //Copy old
		nlp[aps] = {return_pointer, s}; //Set final value to new struct with addr and size. 
		std::free(allocated_p); //Free old array
		allocated_p = nlp; //Set array ptr to new
		++aps; //Increment counter

		return return_pointer; //Return address to allocated memory.
	}

	void free(int* arr) {
		for (int i = 0; i < aps; i++) { //Find the correct address to free.
			if (allocated_p[i].addr == arr) { //Compare
				//Found, dealloc
				size_t dist = arr - memory; //Distance from the current array to initial memory position
				for (int d = dist; d < allocated_p[i].s; d++) { //Set each byte to 0
					allocated[d] = 0;
				}

				lPair* nlp = static_cast<lPair*>(malloc((aps - 1) * sizeof(lPair))); //Realloc with 1 less
				memcpy(nlp, allocated_p, i * sizeof(lPair)); //Copy values in array located BEFORE current value.
				memcpy(nlp + i, allocated_p + i + 1, (aps - i - 1) * sizeof(lPair)); //Copy values in array located AFTER current value.
				std::free(allocated_p); //Free old

				allocated_p = nlp; //Set new ptr
				aps--; //Decrement counter
				return; //No need to finish loop- the job is done.
			}
		}
	}
};

class Executor {
public:
	typedef struct PtrVar { //Store: name of variable, address, and size in BYTES
		const char* name; //var
		int* addr; //0x0
		size_t s; //n*sizeof(int)
	};

	MemoryManager* mm; //Set on construct
	PtrVar* reserved_addresses; //Hold set of PtrVar
	size_t ra_size; //Hold size of reserved_addresses

	void request(std::string s) {
		//REQUEST f 10 // pointer f 10 ints / 40 bytes
		std::string name; //Hold ptr name
		int size_in_ints = 0; //Hold ptr in int size (40bytes to alloc -> 10)

		//String ops
		//Syntax: REQUEST arrayName 20 -> new arrayName with 20 ints / 80 bytes
		size_t s1 = s.find_first_of(' ');
		size_t s2 = s.find(' ', s1 + 1);
		if (s1 == std::string::npos || s2 == std::string::npos) {
			printf("Invalid REQUEST call! %s, %d, %d\n", s.c_str(), s1, s2);
			return;
		}
		name = s.substr(s1+1, s2 - s1 - 1); //Skip whitespace
		size_in_ints = std::stoul(s.substr(s2, s.size() - s2));

		//Ensure that we have no name overlaps. Memory leaks would be pretty bad!
		for (int i = 0; i < ra_size; i++) {
			if (!strcmp(reserved_addresses[i].name, name.c_str())) {
				//Cannot overwrite vars!
				printf("Pointer with name %s already exists!\n", name);
				return;
			}
		}

		PtrVar* npv = static_cast<PtrVar*>(malloc((ra_size + 1) * sizeof(PtrVar))); //Realloc with +1 size
		memcpy(npv, reserved_addresses, (ra_size * sizeof(PtrVar))); //Copy old data

		int* ptr = mm->request(size_in_ints * sizeof(int)); //Request memory from management. Store in pointer to do null check.
		if (ptr == nullptr) { //Perform null check. Do not push back if ptr doesn't exist.
			printf("Invalid REQUEST call! Unable to allocate sufficient memory!\n");
			std::free(npv); //Free new array! Avoid memory leaks.
			return;
		}

		//Set final value with struct constructor {Name, Addr, Bytes}
		npv[ra_size] = { _strdup(name.c_str()), ptr, static_cast<unsigned int>(size_in_ints) * sizeof(int) }; //strdup to duplicate c_str since it gets destroyed.
		std::free(reserved_addresses); //Free old
		reserved_addresses = npv; //Set
		ra_size++; //Increment
	}
	void free(std::string s) {
		size_t s1 = s.find_first_of(' ');
		if (s1 == std::string::npos) {
			printf("Invalid FREE call! %s, %d\n", s.c_str(), s1);
			return;
		}

		std::string ptr_name = s.substr(s1 + 1, s.size() - s1 - 1); //Find name
		for (int i = 0; i < ra_size; i++) { //Loop to find
			if (!(strcmp(reserved_addresses[i].name, ptr_name.c_str()))) { //Strcmp names
				mm->free(reserved_addresses[i].addr); //Easiest part is actually freeing the memory because manager does it all.

				//Del from reserved addresses
				PtrVar* npv = static_cast<PtrVar*>(malloc((ra_size - 1) * sizeof(PtrVar))); //Realloc -1 size
				memcpy(npv, reserved_addresses, i * sizeof(PtrVar)); //Old data BEFORE i
				memcpy(npv + i, reserved_addresses + i + 1, (ra_size - i - 1) * sizeof(PtrVar));  //Old data AFTER i
				std::free(reserved_addresses); //free reserved addresses
				reserved_addresses = npv; //set new
				ra_size--; //decrement
				return;
			}
		}
		printf("Free failed!\n"); //inform that it didnt work lol
	}
	void print(std::string s) {
		size_t s1 = s.find_first_of(' ');
		if (s1 == std::string::npos) {
			printf("Invalid PRINT call! %s, %d\n", s.c_str(), s1);
			return;
		}
		std::string ptr_name = s.substr(s1 + 1, s.size() - s1 - 1); //find
		for (int i = 0; i < ra_size; i++) {//find pt2
			if (!strcmp(reserved_addresses[i].name, ptr_name.c_str())) { //comp
				uint32_t amt = reserved_addresses[i].s / sizeof(int); //Find the amount of values.
				for (int j = 0; j < amt; j++) {
					std::cout << *(reserved_addresses[i].addr + j) << (!(j % 16) && j != 0 ? "\n" : " "); //newline every 16, print each value
				}
				return;
			}
		}
		printf("Could not find! %s\n", ptr_name.c_str());
	}
	bool write(std::string s) {
		//WRITE n 0 50
		std::string pointer; //store name
		unsigned int _index; //store index
		signed int _value; //store value to write

		//Self explanatory string ops
		uint16_t s1 = s.find_first_of(' ');
		uint16_t s2 = s.find(' ', s1 + 1);
		uint16_t s3 = s.find(' ', s2 + 1);

		pointer = s.substr(s1+1, s2-s1-1);
		_index = std::stoul(s.substr(s2+1, s3-s2-1));
		_value = std::stol(s.substr(s3 + 1, s.size() - s3 - 1));

		for (int i = 0; i < ra_size; i++) { //FIND!
			if (!(strcmp(reserved_addresses[i].name, pointer.c_str()))) { //Compare names
				//Found.
				if (_index > reserved_addresses[i].s) { //Ensure our write is in range
					printf("Invalid WRITE call! (Out of range!) %d > %d", _index, reserved_addresses[i].s); //If not, we inform and return
					return 0;
				}
				reserved_addresses[i].addr[_index] = _value; //write
				return 1;
			}
		}
		return 0;
	}
	int read(std::string s) {
		std::string pointer;
		unsigned int _index;

		uint16_t s1 = s.find_first_of(' ');
		uint16_t s2 = s.find(' ', s1 + 1);

		pointer = s.substr(s1 + 1, s2 - s1 - 1);
		_index = std::stoul(s.substr(s2 + 1, s.size() - s2 - 1));

		for (int i = 0; i < ra_size; i++) { //Find value
			if (!(strcmp(reserved_addresses[i].name, pointer.c_str()))) { //Comp
				//Found.
				if (_index > reserved_addresses[i].s) {
					printf("Invalid READ call! (Out of range!) %d > %d", _index, reserved_addresses[i].s); //Range check
					return 0;
				}
				//Print and return value
				std::cout << "Read Value: " << reserved_addresses[i].addr[_index] << "\n";
				return  reserved_addresses[i].addr[_index];
			}
		}
		return 0;
		
	}
public:
	Executor(MemoryManager* l) {
		mm = l;
		ra_size = 0;
	}

	void parse(std::string s) {
		/*
		Operations:

		REQUEST arrayName intCount -> new arrayName with intCount*sizeof(int) bytes
		FREE arrayName -> free arrayName
		PRINT arrayName -> print every value in arrayName
		WRITE arrayName 0 500 -> set 0th idx of arrayName to 500
		READ arrayName 0 -> read the 0th idx of arrayName

		*/
		
		std::istringstream iss(s);
		std::string line;
		while (std::getline(iss, line)) {
			switch (line[0]) {
			case 'R':
				switch (line[2]) {
				case 'Q':
					request(line);
					break;
				case 'A':
					read(line);
					break;
				}
				break;
			case 'F':
				free(line);
				break;
			case 'P':
				print(line);
				break;
			case 'W':
				write(line);
				break;
			}
		}
	}
};

int main()
{
	//With no contacts in!
	MemoryManager mm = MemoryManager(100 * sizeof(int));
	Executor e = Executor(&mm);
	e.parse("REQUEST f 10\nREQUEST g 10\nFREE f");
}


