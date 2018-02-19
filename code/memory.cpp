
struct MemoryBlock {
	int tempStack[10];
	int tempStackSize;

	ExtendibleMemoryArray* pMemory;
	MemoryArray* tMemory;
};

extern MemoryBlock* globalMemory;

#define getPStruct(type) 		(type*)(getPMemory(sizeof(type)))
#define getPArray(type, count) 	(type*)(getPMemory(sizeof(type) * (count)))
#define getPString(count) 	    (char*)(getPMemory((count)))
#define getTStruct(type) 		(type*)(getTMemory(sizeof(type)))
#define getTArray(type, count) 	(type*)(getTMemory(sizeof(type) * (count)))
#define getTString(size) 		(char*)(getTMemory(size)) 
#define getDStruct(type) 		(type*)(getDMemory(sizeof(type)))
#define getDArray(type, count) 	(type*)(getDMemory(sizeof(type) * (count)))

void *getPMemory(int size, MemoryBlock * memory = 0) {
	if(memory == 0) memory = globalMemory;

	void* location = getExtendibleMemoryArray(size, memory->pMemory);
    return location;
}

void * getTMemory(int size, MemoryBlock * memory = 0) {
	if(memory == 0) memory = globalMemory;

	void* location = getMemoryArray(size, memory->tMemory);
    return location;
}

void pushTMemoryStack(MemoryBlock* memory = 0) {
	if(memory == 0) memory = globalMemory;

	globalMemory->tempStack[globalMemory->tempStackSize++] = globalMemory->tMemory->index;
}

void clearTMemoryToStackIndex(MemoryBlock* memory = 0) {
	if(memory == 0) memory = globalMemory;

	globalMemory->tMemory->index = globalMemory->tempStack[globalMemory->tempStackSize-1];
}

void popTMemoryStack(MemoryBlock* memory = 0) {
	if(memory == 0) memory = globalMemory;

	clearTMemoryToStackIndex(memory);

	globalMemory->tempStackSize--;
}

inline char* getPStringCpy(char* str, int size = -1) {
	char* newStr = getPString((size == -1 ? strLen(str) : size) + 1);
	strCpy(newStr, str, size);
	return newStr;
}

inline char* getTStringCpy(char* str, int size = -1) {
	char* newStr = getTString((size == -1 ? strLen(str) : size) + 1);
	strCpy(newStr, str, size);
	return newStr;
}

inline char* getPStringClr(int size) { 
	char* s = getPString(size);
	s[0] = '\0';
	return s;
}

inline char* getTStringClr(int size) { 
	char* s = getTString(size);
	s[0] = '\0';
	return s;
}

void clearTMemory(MemoryBlock * memory = 0) {
	if(memory == 0) memory = globalMemory;

	clearMemoryArray(memory->tMemory);
}



char** getTStringArray(char** strings, int count) {

	char** array = getTArray(char*, count);
	for(int i = 0; i < count; i++) {
		array[i] = getTString(strLen(strings[i]));
	}

	return array;
}
