#pragma once 

#include "rt_types.cpp"
#include <stdio.h>

#define arrayCount(array) (sizeof(array) / sizeof((array)[0]))
#define addPointer(ptr, int) ptr = (char*)ptr + (int)
#define memberSize(type, member) sizeof(((type *)0)->member)
#define memberOffsetSize(type, member) vec2i(offsetof(type, member), memberSize(type, member))
#define mallocArray(type, count) (type*)malloc(sizeof(type)*(count))
#define mallocStruct(type) (type*)malloc(sizeof(type))
#define mallocString(count) (char*)malloc(sizeof(char)*(count))

#define allocaArray(type, count) (type*)alloca(sizeof(type)*(count))
#define allocaStruct(type) (type*)alloca(sizeof(type))
#define allocaString(count) (char*)alloca(sizeof(char)*(count))

#define zeroStruct(s, structType) zeroMemory(s, sizeof(structType));
#define copyArray(dst, src, type, count) memCpy(dst, src, sizeof(type)*count);
#define moveArray(dst, src, type, count) memmove(dst, src, sizeof(type)*count);

#define PVEC2(v) v.x, v.y
#define PVEC3(v) v.x, v.y, v.z
#define PVEC4(v) v.x, v.y, v.z, v.w
#define PRECT(r) r.left, r.bottom, r.right, r.top

#define Void_Dref(type, ptr) (*((type*)ptr))

#define For_Array(array, count, type) \
	for(type* it = array; (it-array) < count; it++)

#define arrayIndex(w, x, y) (y*w + x)
#define arrayIndex3D(w, h, x, y, z) (z*h*w + y*w + x)

#define MetaParse() 

uint16_t swapU16( uint16_t val ) {
    return (val << 8) | (val >> 8 );
}

int16_t swapI16( int16_t val ) {
    return (val << 8) | ((val >> 8) & 0xFF);
}

uint32_t swapU32( uint32_t val ) {
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF ); 
    return (val << 16) | (val >> 16);
}

int32_t swapI32( int32_t val ) {
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF ); 
    return (val << 16) | ((val >> 16) & 0xFFFF);
}


int myAssert(bool check) {
	if(!check) {

		printf("Assert fired!\n");
		if(IsDebuggerPresent()) {
			__debugbreak();
		}
		exit(1);
	}
	return 234;
}

void memSet(void* dest, int value, int numOfBytes) {
	char* destPointer = (char*)dest;
	for(int i = 0; i < numOfBytes; i++) {
		destPointer[i] = value;
	}
}

void memSetLarge(void* dest, int value, i64 numOfBytes) {
	char* destPointer = (char*)dest;
	for(i64 i = 0; i < numOfBytes; i++) {
		destPointer[i] = value;
	}
}

void zeroMemory(void* memory, int size) {
	memSet(memory, 0, size);
}

void memCpy(void* dest, void* source, int numOfBytes) {
	char* destPointer = (char*)dest;
	char* sourcePointer = (char*)source;
	for(int i = 0; i < numOfBytes; i++) {
		destPointer[i] = sourcePointer[i];
	}
}

void freeAndSetNullSave(void* data) {
	if(data) {
		free(data);
		data = 0;
	}
}

#define reallocArraySave(type, ptr, count) \
	freeAndSetNullSave(ptr);               \
	ptr = mallocArray(type, (count));

#define reallocStructSave(type, ptr) \
	freeAndSetNullSave(ptr);         \
	ptr = mallocStruct(type);

#define reallocStringSave(ptr, count) \
	freeAndSetNullSave(ptr);          \
	ptr = mallocString((count));

void freeZero(void* data) {
	if(data) {
		free(data);
		data = 0;
	}
}

//
// Strings
// 

int strLen(char* str) {	
	int len = 0;
	while(str[len] != '\0') len++;

	return len;
}

inline void strClear(char* string) {
	string[0] = '\0';
}

int intDigits(int n) {
	int count = 0;
	if(n == 0) return 1;

	while(n != 0) {
		n /= 10;
		count++;
	}

	return count;
}

inline char * intToStr(char * buffer, int var) {
	int digits = intDigits(var);
	if(var < 0) digits++;
	buffer[digits--] = '\0';

	if(var < 0) {
		buffer[0] = '-';
		var *= -1;
	}

	if(var == 0) {
		buffer[0] = '0';
		return buffer;
	}


	while(var != 0) {
		buffer[digits--] = (char)((var % 10) + '0');
		var /= 10;
	}

	return buffer;
};

int intDigits(i64 n) {
	int count = 0;
	if(n == 0) return 1;

	while(n != 0) {
		n /= 10;
		count++;
	}

	return count;
}

inline char * intToStr(char* buffer, i64 var) {
	int digits = intDigits(var);
	if(var < 0) digits++;
	buffer[digits--] = '\0';

	if(var < 0) {
		buffer[0] = '-';
		var *= -1;
	}

	if(var == 0) {
		buffer[0] = '0';
		return buffer;
	}

	while(var != 0) {
		buffer[digits--] = (char)((var % 10) + '0');
		var /= 10;
	}

	return buffer;
};

char* floatToStr(char * buffer, float f, int precision = 0)
{
	int stringSize = sprintf(buffer, "%1.7f", f); // TODO: Make more robust.
	if (precision > 0) {
		buffer[stringSize - (7-precision)] = '\0';
	} else {
		int stringIndex = stringSize-1;
		while(buffer[stringIndex] == '0') {
			stringIndex--;
		}
		if(buffer[stringIndex] == '.') {
			stringIndex++;
			buffer[stringIndex] = '0';
		}
		buffer[stringIndex + 1] = '\0'; 
	}

	return buffer;
}

void strCpyBackwards(char* dest, char* str, int size) {
	for(int i = size-1; i >= 0; i--) dest[i] = str[i];
}

inline void strInsert(char* string, int index, char ch) {
	int stringLength = strLen(string);
	// memCpy(string+index+1, string+index, stringLength-index+1);
	strCpyBackwards(string+index+1, string+index, stringLength-index+1);
	string[index] = ch;
}

void strInsert(char* destination, int index, char* str, int size = -1) {
	if(size == -1) size = strLen(str);
	int sizeDest = strLen(destination);

	strCpyBackwards(destination + size + index, destination + index, sizeDest - index + 1);
	strCpyBackwards(destination + index, str, size);
}

void strAppend(char* destination, char* str, int size = -1) {
	int destSize = strLen(destination);
	int strSize = size == -1? strLen(str) : size;

	destination[destSize+strSize] = '\0';

	for(int i = 0; i < strSize; i++) {
		destination[destSize+i] = str[i];
	}
}

inline int charToInt(char c) {
	return (int)c - '0';
}

inline int strToInt(char* string) {
	int result = atoi(string);
	return result;
}

inline int strToIntSave(char* string) {
	if(!string) return 0;
	else return strToInt(string);
}

inline int strHexToInt(char* string) {
	int result = (int)strtol(string, 0, 16);
	return result;
}

inline float strToFloat(char* string) {
	float result = atof(string);		
	return result;
}

inline void strErase(char* string, int index, int size) {
	int amount = strLen(string) - index - size;
	memCpy(string+index, string+index+size, amount+1);
}

inline bool strEmpty(char* string) {
	bool result = string[0] == '\0';
	return result;
}

void copySubstring(char * destination, char * source, int start, int end) {
	int size = end - start;
	memCpy(destination, source + start, size + 1);
	destination[size + 1] = '\0';
}

void strCpy(char * destination, char * source, int size = -1) {
	if(size != 0) {
		if(size == -1) size = strLen(source);
		memCpy(destination, source, size);
	}
	destination[size] = '\0';
}
void strCpyInc(char ** destination, char * source, int size = -1) {
	if(size != 0) {
		if(size == -1) size = strLen(source);
		memCpy(*destination, source, size);
	}
	(*destination)[size] = '\0';
	(*destination) += size;
}

bool strCompare(char* str, char* str2, int size = -1) {
	int length = size == -1 ? strLen(str2) : size;

	if(length != strLen(str)) return false;

	bool result = true;
	for(int i = 0; i < length; i++) {
		if(str[i] != str2[i]) {
			result = false;
			break;
		}
	}

	return result;
}

bool strCompare(char* str, int size1, char* str2, int size2) {
	if(size1 != size2) return false;

	bool result = true;
	for(int i = 0; i < size1; i++) {
		if(str[i] != str2[i]) {
			result = false;
			break;
		}
	}

	return result;
}

bool strStartsWith(char* str, char* str2, int size = 0) {
	int length = size ? size : strLen(str2);

	for(int i = 0; i < length; i++) {
		if(str[i] == '\0') return false;
		if(str[i] != str2[i]) return false;
	}

	return true;
}

int strFind(const char* str, char chr, int startIndex = 0) {
	int index = startIndex;
	int pos = -1;
	while(str[index] != '\0') {
		if(str[index] == chr) {
			pos = index;
			break;
		}
		index++;
	}

	return pos+1;
}

int strFindX(const char* str, char chr, int startIndex = 0) {
	int pos = strFind(str, chr, startIndex);
	if(pos == 0) return -1;
	else return pos;
}

int strFindOrEnd(const char* str, char chr, int startIndex = 0) {
	int pos = strFind(str, chr, startIndex);
	if(pos == 0) return strLen((char*)str);
	else return pos;
}

int strFindBackwards(char* str, char chr, int startIndex = -1) {
	int length = startIndex == -1 ? strLen(str) : startIndex;

	int pos = -1;
	for(int i = length - 1; i >= 0; i--) {
		if(str[i] == chr) {
			pos = i;
			break;
		}
	}

	return pos+1;
}

int strFind(char* source, char* str, int to = 0, int from = 0) {
	int sourceLength = to > 0 ? to : strLen(source);
	int length = strLen(str);

	if(length > sourceLength) return -1;

	bool found = false;
	int pos = -1;
	for(int i = from; i < (sourceLength-length) + 1; i++) {
		if(source[i] == str[0]) {
			bool check = true;
			pos = i;
			for(int subIndex = 1; subIndex < length; subIndex++) {
				if(source[i+subIndex] != str[subIndex]) {
					check = false;
					break;
				}
			}
			if(check) {
				found = true;
				break;
			}
		}
	}

	int result = -1;
	if(found) result = pos;

	return result;
}

int strFindRight(char* source, char* str, int searchDistance = 0) {
	int result = strFind(source, str, searchDistance);
	if(result != -1) result += strLen(str);

	return result;
}

// int strFindBackwarts(const char* source, const char* str, int to = 0, int from = 0) {
// 	int sourceLength = to > 0 ? to : strLen(source);
// 	int length = strLen(str);

// 	if(length > sourceLength) return -1;

// 	bool found = false;
// 	int pos = -1;
// 	for(int i = from; i < (sourceLength-length) + 1; i++) {
// 		if(source[i] == str[0]) {
// 			bool check = true;
// 			pos = i;
// 			for(int subIndex = 1; subIndex < length; subIndex++) {
// 				if(source[i+subIndex] != str[subIndex]) {
// 					check = false;
// 					break;
// 				}
// 			}
// 			if(check) {
// 				found = true;
// 				break;
// 			}
// 		}
// 	}

// 	int result = -1;
// 	if(found) result = pos;

// 	return result;
// }

// void strAttach(char* destination, char* source) {
	// int length = strLen(destination);
// }

bool strSearchEnd(char* str, int searchLength) {
	bool foundEnd = false;
	for(int i = 0; i < searchLength; i++) {
		if(str[i] == '\0') {
			foundEnd = true;
			break;
		}
	}

	return foundEnd;
}

void strRemove(char* str, int index, int size = 0) {
	int length = size == 0 ? strLen(str) : size;

	memCpy(str + index - 1, str + (index), length-index+1);
}

void strRemoveX(char* str, int index, int amount, int size = 0) {
	int length = size == 0 ? strLen(str) : size;

	memCpy(str + index, str + index+amount, length-index+amount);
}

void strRemoveWhitespace(char* str, int size = 0) {
	int length = size == 0 ? strLen(str) : size;

	int i = 0;
	while(i < length) {
		if(str[i] == '\t' || str[i] == ' ') {
			strRemove(str, i+1, length); 
			length--;
		} else {
			i++;
		}
	}
}

void reallocString(char** str, char* newStr) {
	// if(*str == 0) (*str) = mallocString(strLen(newStr) + 1);
	// else realloc((*str), sizeof(char)*(strLen(newStr) + 1));
	if(*str != 0) free(*str);
	(*str) = mallocString(strLen(newStr) + 1);

	strCpy((*str), newStr);
}



#define initString(function, size) init(function(size), size)
struct String {
	char* data;
	int size;
	int maxSize;

	void init(void* data, int maxSize) {
		this->data = (char*)data;
		this->maxSize = maxSize;
		this->size = 0;
		setEndZero();
	}

	void append(char c) {
		data[size++] = c;
		setEndZero();
	}

	void append(char* str, int length = 0) {
		if(!length) length = strLen(str);
		memCpy(data + size, str, length);
		size += length;
		setEndZero();
	}

	int find(char* str, int start) {
		int len = strLen(str);
		bool found = false;
		int position = 0;
		for(int index = start; index < size-len; index++) {
			if(data[index] == str[0]) {
				position = index+1;
				found = true;
				for(int i = 0; i < len; i++) {
					if(data[index + i] != str[i]) {
						position = 0;
						found = false;
					}
				}
				if(found) break;
			}
		} 

		return position;
	}

	void setEndZero() { data[size] = '\0'; }
};

int createFileAndOverwrite(char* fileName, int fileSize) {
	FILE* file = fopen(fileName, "wb");
	if(file == 0) return -1;

	fileSize += 1;

	char* tempBuffer = (char*)malloc(fileSize);
	zeroMemory(tempBuffer, fileSize);
	fwrite(tempBuffer, fileSize, 1, file);
	free(tempBuffer);

	fclose(file);

	return 0;
}

int fileSize(char* fileName) {
	FILE* file = fopen(fileName, "rb");

	if(file == 0) return -1;

	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fclose(file);

	return size;
}

inline int fileSize(FILE* file) {
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, 0, SEEK_SET);

	return size;
}

bool fileExists(char* fileName) {
	bool result = false;
	FILE* file = fopen(fileName, "rb");

	if(file) {
		result = true;
		fclose(file);
	}
	
	return result;
}

int readFileToBuffer(char* buffer, char* fileName) {
	FILE* file = fopen(fileName, "rb");
	if(file == 0) return -1;

	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, 0, SEEK_SET);

	fread(buffer, size, 1, file);

	fclose(file);
	return size;
}

int readFileSectionToBuffer(char* buffer, char* fileName, int offsetInBytes, int sizeInBytes) {
	FILE* file = fopen(fileName, "r+b");
	if(file == 0) return -1;

	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, 0, SEEK_SET);

	myAssert(offsetInBytes+sizeInBytes < size);

	fseek(file, offsetInBytes, SEEK_SET);
	fread(buffer, sizeInBytes, 1, file);

	fclose(file);
	return size;
}

void readFileToString(String* str, char* fileName) {
	str->size = readFileToBuffer(str->data, fileName);
	str->setEndZero();

	myAssert(str->size <= str->maxSize);
}

int writeBufferToFile(char* buffer, char* fileName, int size = -1) {
	if(size == -1) size = strLen(buffer);

	FILE* file = fopen(fileName, "wb");
	if(!file) {
		printf("Could not open file!\n");
		return -1;
	} 

	fwrite(buffer, size, 1, file);

	fclose(file);
	return size;
}

int writeBufferSectionToFile(char* buffer, char* fileName, int offsetInBytes, int sizeInBytes) {
	FILE* file = fopen(fileName, "r+b");
	if(!file) {
		printf("Could not open file!\n");
		return -1;
	} 

	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, 0, SEEK_SET);

	myAssert(offsetInBytes+sizeInBytes < size);

	fseek(file, offsetInBytes, SEEK_SET);
	fwrite(buffer, sizeInBytes, 1, file);

	fclose(file);
	return size;
}

void writeStringToFile(String str, char* fileName) {
	writeBufferToFile(str.data, fileName, str.size);
}

void writeDataToFile(char* data, int size, char* fileName) {
	FILE* file = fopen(fileName, "wb");
	fwrite(data, size, 1, file);
	fclose(file);
}

void readDataFile(char* data, char* fileName) {
	FILE* file = fopen(fileName, "rb");

	int size = fileSize(file);
	fread(data, size, 1, file);
	fclose(file);
}

char* getFileNameFromPath(char *filePath) {
    char *name = strrchr(filePath, '\\');
    if(!name) name = filePath;
    else name++;

    return name;
}

//
// ???
//

int timeToSeconds(int year = 0, int month = 0, int day = 0, int hour = 0, int minute = 0, int seconds = 0)
{
	// year is 20XX, only works up to ~2060
	return (year * 31556926 + month * 2629743.83 + day * 86400 +
		hour * 3600 + minute * 60 + seconds);
};

inline i64 terraBytes(i64 count)
{
	i64 result = count * 1024 * 1024 * 1024 * 1024;
	return result;
}

inline i64 gigaBytes(i64 count)
{
	i64 result = count * 1024 * 1024 * 1024;
	return result;
}

inline int megaBytes(int count)
{
	int result = count * 1024 * 1024;
	return result;
}

inline int kiloBytes(int count)
{
	int result = count * 1024;
	return result;
}

inline int roundDown(int i, int size) {
	int val = (i/size) * size;
	return val;
}

inline int roundUp(int i, int size) {
	int val = roundDown(i, size);
	if(val != 0) val += size;
	return val;	
}

//
//
//

struct MemoryArray {
	bool initialized;

	char * data;
	int index;
	int size;
};

void initMemoryArray(MemoryArray * memory, int slotSize, void* baseAddress = 0) {
    if(baseAddress) {
	    memory->data = (char*)VirtualAlloc(baseAddress, slotSize, MEM_COMMIT, PAGE_READWRITE);
	    // memory->data = (char*)malloc(slotSize);
	    int errorCode = GetLastError();
    } else memory->data = (char*)malloc(slotSize);

    myAssert(memory->data);

    memory->index = 0;
    memory->size = slotSize;
    memory->initialized = true;
}



MemoryArray* globalMemoryArray;

void* getMemoryArray(int size, MemoryArray * memory = 0) {
    if(!memory) memory = globalMemoryArray;
    myAssert(memory->index + size <= memory->size);

    void * location = memory->data + memory->index;
    memory->index += size;

    return location;
}

void freeMemoryArray(int size, MemoryArray * memory = 0) {
    if(!memory) memory = globalMemoryArray;
    myAssert(memory->size >= memory->index);

    memory->index -= size;
}

void clearMemoryArray(MemoryArray* memory = 0) {
    if(!memory) memory = globalMemoryArray;
	memory->index = 0;
}

void* getBaseMemoryArray(MemoryArray* ma) {
	void* base = ma->data;
	return base;
}



struct ExtendibleMemoryArray {
	void* startAddress;
	int slotSize;
	int allocGranularity;
	MemoryArray arrays[32];
	int index;
};

void initExtendibleMemoryArray(ExtendibleMemoryArray* memory, int slotSize, int allocGranularity, void* baseAddress = 0) {
	memory->startAddress = baseAddress;
	memory->index = 0;
	memory->allocGranularity = allocGranularity;
	memory->slotSize = roundUp(slotSize, memory->allocGranularity);

	initMemoryArray(memory->arrays, memory->slotSize, memory->startAddress);
}

ExtendibleMemoryArray* globalExtendibleMemoryArray;

void* getExtendibleMemoryArray(int size, ExtendibleMemoryArray* memory = 0) {
	if(!memory) memory = globalExtendibleMemoryArray;
	myAssert(size <= memory->slotSize);

	MemoryArray* currentArray = memory->arrays + memory->index;
	if(currentArray->index + size > currentArray->size) {
		memory->index++;
		myAssert(memory->index < arrayCount(memory->arrays));
		i64 baseOffset = (i64)memory->index*(i64)memory->slotSize;

		MemoryArray* mArray = memory->arrays + memory->index;
		if(!mArray->initialized)
			initMemoryArray(&memory->arrays[memory->index], memory->slotSize, (char*)memory->startAddress + baseOffset);
		else
			mArray->index = 0;

		currentArray = memory->arrays + memory->index;
	}

	void* location = getMemoryArray(size, currentArray);
	return location;
}

void* getBaseExtendibleMemoryArray(ExtendibleMemoryArray* ema) {
	void* base = ema->arrays[0].data;
	return base;
}



struct BucketMemory {
	int pageSize;
	int count;
	int useCount;
	char* data;
	bool* used;
};

// slotSize has to be dividable by pageSize
void initBucketMemory(BucketMemory* memory, int pageSize, int slotSize, void* baseAddress = 0) {
	memory->pageSize = pageSize;
	memory->count = slotSize / pageSize;
	memory->useCount = 0;

	if(baseAddress) {
		memory->data = (char*)VirtualAlloc(baseAddress, slotSize + memory->count, MEM_COMMIT, PAGE_READWRITE);
		// memory->data = (char*)malloc(slotSize + memory->count);
	}
	else memory->data = (char*)malloc(slotSize + memory->count);
	myAssert(memory->data);

	memory->used = (bool*)memory->data + slotSize;
	memSet(memory->used, 0, memory->count);
}

void deleteBucketMemory(BucketMemory* memory) {
	VirtualFree(memory->data, 0, MEM_RELEASE);
	// free(memory->data);
}

BucketMemory* globalBucketMemory;

void* getBucketMemory(BucketMemory* memory = 0) {
	if(memory == 0) memory = globalBucketMemory;
	myAssert(memory);

	if(memory->useCount == memory->count) return 0;

	char* address = 0;
	int index;
	for(int i = 0; i < memory->count; i++) {
		if(memory->used[i] == 0) {
			address = memory->data + i*memory->pageSize;
			index = i;
			break;
		}
	}

	myAssert(address);

	if(address) {
		memory->used[index] = true;

		memory->useCount++;
		myAssert(memory->useCount <= memory->count);

		return address;
	}

	return 0;
}

void freeBucketMemory(void* address, BucketMemory* memory = 0) {
	if(memory == 0) memory = globalBucketMemory;
	myAssert(memory);

	memory->useCount--;
	myAssert(memory->useCount >= 0);

	int dataOffset = ((char*)address - memory->data) / memory->pageSize;
	memory->used[dataOffset] = false;
}



struct ExtendibleBucketMemory {
	void* startAddress;
	int slotSize;
	int fullSize;
	int allocGranularity;
	BucketMemory arrays[32];
	bool allocated[32];

	int pageSize;
};

ExtendibleBucketMemory* globalExtendibleBucketMemory;

void initExtendibleBucketMemory(ExtendibleBucketMemory* memory, int pageSize, int slotSize, int allocGranularity, void* baseAddress = 0) {
	memory->startAddress = baseAddress;
	memory->allocGranularity = allocGranularity;
	memory->slotSize = slotSize;
	memory->fullSize = roundUp(slotSize + (slotSize / pageSize), memory->allocGranularity);
	memory->pageSize = pageSize;

	memSet(memory->allocated, 0, arrayCount(memory->arrays));
}

void* getExtendibleBucketMemory(ExtendibleBucketMemory* memory = 0) {
	if(!memory) memory = globalExtendibleBucketMemory;

	// check all allocated arrays for a free slot
	BucketMemory* availableBucket = 0;
	for(int i = 0; i < arrayCount(memory->arrays); i++) {
		if(memory->allocated[i] && (memory->arrays[i].useCount < memory->arrays[i].count)) {
			availableBucket = memory->arrays + i;
			break;
		}
	}

	// allocate array
	if(!availableBucket) {
		// get first array that is not allocated
		int arrayIndex = -1;
		for(int i = 0; i < arrayCount(memory->allocated); i++) {
			if(!memory->allocated[i]) {
				availableBucket = memory->arrays + i;
				arrayIndex = i;
				break;
			}
		}

		myAssert(availableBucket);

		int slotSize = memory->slotSize;
		i64 baseOffset = arrayIndex * memory->fullSize;
		initBucketMemory(availableBucket, memory->pageSize, memory->slotSize, (char*)memory->startAddress + baseOffset);

		memory->allocated[arrayIndex] = true;
	}

	void* location = getBucketMemory(availableBucket);
	return location;
}

void freeExtendibleBucketMemory(void* address, ExtendibleBucketMemory* memory = 0) {
	if(!memory) memory = globalExtendibleBucketMemory;

	// calculate array index with address
	int arrayIndex = ((char*)address - (char*)memory->startAddress) / memory->fullSize;
	BucketMemory* bMemory = memory->arrays + arrayIndex;
	freeBucketMemory(address, bMemory);

	if(bMemory->useCount == 0) {
		deleteBucketMemory(bMemory);
		memory->allocated[arrayIndex] = false;
	}
}

struct AppMemory {
	MemoryArray memoryArrays[4];
	int memoryArrayCount;
	
	ExtendibleMemoryArray extendibleMemoryArrays[4];
	int extendibleMemoryArrayCount;

	BucketMemory bucketMemories[4];
	int bucketMemoryCount;

	ExtendibleBucketMemory extendibleBucketMemories[4];
	int extendibleBucketMemoryCount;
};

//
//
//

inline int flagSet(int flags, int flagType) { return flags | flagType; }
inline int flagRemove(int flags, int flagType) { return flags &= ~flagType; }

inline void flagSet(int* flags, int flagType) { (*flags) |= flagType; }
inline void flagRemove(int* flags, int flagType) { (*flags) &= ~flagType; }

inline bool flagGet(int flags, int flagType) { return (flags | flagType) == flags; }

//
//
//

struct Statistic {
	f64 min;
	f64 max;
	f64 avg;
	int count;
};

void beginStatistic(Statistic* stat) {
	stat->min = DBL_MAX; 
	stat->max = -DBL_MAX; 
	stat->avg = 0; 
	stat->count = 0; 
}

void updateStatistic(Statistic* stat, f64 value) {
	if(value < stat->min) stat->min = value;
	if(value > stat->max) stat->max = value;
	stat->avg += value;
	++stat->count;
}

void endStatistic(Statistic* stat) {
	stat->avg /= stat->count;
}

//
//
//

struct Token {
	int type;
	char* data;
	int size;
};

bool charIsSign(char c) { return c == '-' || c == '+'; }
bool charIsDigit(char c) { return (c >= '0') && (c <= '9'); }
bool charIsLetter(char c) { return ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')); }
bool charIsUppercaseLetter(char c) { return c >= 'A' && c <= 'Z'; }
bool charIsLowercaseLetter(char c) { return c >= 'a' && c <= 'z'; }
bool charIsAlphaNumeric(char c) { return charIsDigit(c) || charIsLetter(c); }
bool charIsWhiteSpace(char c) { return c==' ' || c=='\n' || c=='\t' || c=='\v' || c=='\f' || c=='\r'; }

char* eatWhiteSpaces(char* str) {
	int index = 0;
	while(charIsWhiteSpace(str[index])) index++;
	return str + index;
}

char* eatSpaces(char* str) {
	int index = 0;
	while(str[index] == ' ') index++;
	return str + index;
}

int parseNumber(char* s) {
	int i = 0;
	if(charIsSign(s[i])) i++;

	if(!charIsDigit(s[i++])) return 0;
	while(charIsDigit(s[i])) i++;

	if(s[i] == '.') i++;

	while(charIsDigit(s[i])) i++;

	return i;
}

int parseString(char* s) {
	int i = 0;
	if(!(s[i++] == '\"')) return 0;

	for(;;) {
		int pos = strFind(s + i, '\"');
		if(!pos) return 0;

		i += pos;
		if(s[i-1] != '\\') return i;
	}

}


//

/*
 * PCG Random Number Generation for C.
 *
 * Copyright 2014 Melissa O'Neill <oneill@pcg-random.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For additional information about the PCG random number generation scheme,
 * including its license and other licensing options, visit
 *
 *       http://www.pcg-random.org
 */

#include <inttypes.h>

struct pcg_state_setseq_64 {    // Internals are *Private*.
    uint64_t state;             // RNG state.  All values are possible.
    uint64_t inc;               // Controls which RNG sequence (stream) is
                                // selected. Must *always* be odd.
};
typedef struct pcg_state_setseq_64 pcg32_random_t;

#define PCG32_INITIALIZER   { 0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL }

// state for global RNGs

__declspec(thread) static pcg32_random_t pcg32_global = PCG32_INITIALIZER;


// pcg32_srandom(initstate, initseq)
// pcg32_srandom_r(rng, initstate, initseq):
//     Seed the rng.  Specified in two parts, state initializer and a
//     sequence selection constant (a.k.a. stream id)

uint32_t pcg32_random_r(pcg32_random_t* rng);
void pcg32_srandom_r(pcg32_random_t* rng, uint64_t initstate, uint64_t initseq)
{
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    pcg32_random_r(rng);
    rng->state += initstate;
    pcg32_random_r(rng);
}

void pcg32_srandom(uint64_t seed, uint64_t seq)
{
    pcg32_srandom_r(&pcg32_global, seed, seq);
}

// pcg32_random()
// pcg32_random_r(rng)
//     Generate a uniformly distributed 32-bit random number

uint32_t pcg32_random_r(pcg32_random_t* rng)
{
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

uint32_t pcg32_random()
{
    return pcg32_random_r(&pcg32_global);
}

// pcg32_boundedrand(bound):
// pcg32_boundedrand_r(rng, bound):
//     Generate a uniformly distributed number, r, where 0 <= r < bound

uint32_t pcg32_boundedrand_r(pcg32_random_t* rng, uint32_t bound)
{
    // To avoid bias, we need to make the range of the RNG a multiple of
    // bound, which we do by dropping output less than a threshold.
    // A naive scheme to calculate the threshold would be to do
    //
    //     uint32_t threshold = 0x100000000ull % bound;
    //
    // but 64-bit div/mod is slower than 32-bit div/mod (especially on
    // 32-bit platforms).  In essence, we do
    //
    //     uint32_t threshold = (0x100000000ull-bound) % bound;
    //
    // because this version will calculate the same modulus, but the LHS
    // value is less than 2^32.

    uint32_t threshold = -bound % bound;

    // Uniformity guarantees that this loop will terminate.  In practice, it
    // should usually terminate quickly; on average (assuming all bounds are
    // equally likely), 82.25% of the time, we can expect it to require just
    // one iteration.  In the worst case, someone passes a bound of 2^31 + 1
    // (i.e., 2147483649), which invalidates almost 50% of the range.  In 
    // practice, bounds are typically small and only a tiny amount of the range
    // is eliminated.
    for (;;) {
        uint32_t r = pcg32_random_r(rng);
        if (r >= threshold)
            return r % bound;
    }
}

uint32_t pcg32_boundedrand(uint32_t bound)
{
    return pcg32_boundedrand_r(&pcg32_global, bound);
}

inline int randomIntPCG(int from, int to) {
	return pcg32_boundedrand(to - from + 1) + from;
}

inline float randomFloatPCG(float from, float to, float precision /* eg.: 0.01f*/) {
	return randomIntPCG(from/precision, to/precision) * precision;
}

//

template <typename T>
struct DArray {
	T* data = 0;
	int count = 0;
	int reserved = 0;
	int startSize = 100;

	void init() {
		*this = {};
		startSize = 100;
	}

	int getReservedCount(int newCount) {
		int reservedCount = max(startSize, reserved);
		while(reservedCount < newCount) reservedCount *= 2;

		return reservedCount;
	}

	void resize(int newCount) {
		int reservedCount = getReservedCount(newCount);

		T* newData = mallocArray(T, reservedCount);
		copyArray(newData, data, T, count);

		if(data) free(data);
		data = newData;
		reserved = reservedCount;
	}

	void copy(T* d, int n) {
		count = 0;
		push(d, n);
	}

	void copy(DArray<T> array) {
		return copy(array.data, array.count);
	}
	void copy(DArray<T>* array) {
		return copy(*array);
	}

	void dealloc() {
		if(data) {
			free(data);
			count = 0;
			data = 0;
			reserved = 0;
		}
	}

	void freeResize(int n) {
		dealloc();
		resize(n);
	}

	void push(T element) {
		if(count == reserved) resize(count+1);

		data[count++] = element;
	}

	void push(T* elements, int n) {
		if(count+n-1 >= reserved) resize(count+n);

		copyArray(data+count, elements, T, n);
		count += n;
	}

	void push(DArray* array) {
		push(array->data, array->count);
	}

	void insertMove(T element, int index) {
		if(index > count-1) return push(element);

		if(count == reserved) resize(count+1);

		moveArray(data+index+1, data+index, T, count-(index+1));
		data[index] = element;
		count++;
	}

	void insert(T element, int index) {
		assert(index <= count);
		
		if(index == count) return push(element);
		push(data[index]);
		data[index] = element;
	}

	int find(T value) {
		for(int i = 0; i < count; i++) {
			if(value == data[i]) return i+1;
		}

		return 0;
	}

	T* retrieve(int addedCount) {
		if(count+addedCount-1 >= reserved) resize(count+addedCount);

		T* p = data + count;
		count += addedCount;

		return p;
	}

	bool operator==(DArray<T> array) {
		if(count != array.count) return false;
		for(int i = 0; i < count; i++) {
			if(data[i] != array.data[i]) return false;
		}
		return true;
	}
	bool operator!=(DArray<T> array) { return !(*this == array); }

	void clear()           { count = 0; }
	T    first()           { return data[0]; }
	T    last()            { return data[count-1]; }
	bool empty()           { return count == 0; };
	T    pop()             { return data[--count]; }
	void pop(int n)        { count -= n; }
	void remove(int i)     { data[i] = data[--count]; }
	T&   operator[](int i) { return data[i]; }
	T&   at(int i)         { return data[i]; }
	T*   operator+(int i)  { return data + i; }
	T*   atr(int i)        { return data + i; }
};