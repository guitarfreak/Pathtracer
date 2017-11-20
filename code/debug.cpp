
enum StructType {
	STRUCTTYPE_int = 0,
	STRUCTTYPE_float,
	STRUCTTYPE_char,
	STRUCTTYPE_bool,

	STRUCTTYPE_Vec3,
	STRUCTTYPE_Vec4,
	STRUCTTYPE_AppSettings,
	// STRUCTTYPE_AppColors,
	STRUCTTYPE_RelativeColor,
	STRUCTTYPE_AppColorsRelative,

	STRUCTTYPE_TestStruct,

	STRUCTTYPE_SIZE,
};

#define Get_Struct_Type(name) STRUCTTYPE_##name

enum {
	ARRAYTYPE_CONSTANT, // int test[];
	ARRAYTYPE_DYNAMIC, // int* test;

	ARRAYTYPE_SIZE, 
};

enum {
	ARRAYSIZEMODE_SIZE = 0,
	ARRAYSIZEMODE_OFFSET,
	ARRAYSIZEMODE_NULL_TERMINATED,
};

struct ArrayInfo {
	int type;
	int sizeMode;

	union {
		int size;
		int offset;
	};
};

ArrayInfo arrayInfoString() { return {ARRAYTYPE_DYNAMIC, ARRAYSIZEMODE_NULL_TERMINATED}; }

struct StructMemberInfo {
	char* name;
	int type;
	int offset;
	int arrayCount;
	ArrayInfo arrays[2];
};

StructMemberInfo initMemberInfo(char* name, int type, int offset) {
	StructMemberInfo info;
	info.name = name;
	info.type = type;
	info.offset = offset;
	info.arrayCount = 0;

	return info;
}

StructMemberInfo initMemberInfo(char* name, int type, int offset, ArrayInfo aInfo1, ArrayInfo aInfo2 = {-1}) {
	StructMemberInfo info = initMemberInfo(name, type, offset);
	info.arrayCount = 1;
	if(aInfo2.type != -1) info.arrayCount = 2;

	info.arrays[0] = aInfo1;
	info.arrays[1] = aInfo2;

	return info;
}

struct StructInfo {
	char* name;
	int size;
	int memberCount;
	StructMemberInfo* list;
};

#define Struct_Member_Info_Array(name) name##_StructMemberInfos
#define Init_Member(stype, name, mtype) #name, STRUCTTYPE_##mtype, offsetof(stype, name)
#define Init_Struct_Info(name) {#name, sizeof(name), arrayCount(Struct_Member_Info_Array(name)), Struct_Member_Info_Array(name)}
#define Struct_Member(struct, offset) ((void*)(((char*)struct) + offset))


StructMemberInfo Struct_Member_Info_Array(Vec3)[] = {
	initMemberInfo( Init_Member(Vec3, x, float) ),
	initMemberInfo( Init_Member(Vec3, y, float) ),
	initMemberInfo( Init_Member(Vec3, z, float) ),
};

StructMemberInfo Struct_Member_Info_Array(Vec4)[] = {
	initMemberInfo( Init_Member(Vec4, x, float) ),
	initMemberInfo( Init_Member(Vec4, y, float) ),
	initMemberInfo( Init_Member(Vec4, z, float) ),
	initMemberInfo( Init_Member(Vec4, a, float) ),
};

// StructMemberInfo Struct_Member_Info_Array(AppSettings)[] = {
// 	initMemberInfo( Init_Member(AppSettings, font, char), arrayInfoString()),
// 	initMemberInfo( Init_Member(AppSettings, fontBold, char), arrayInfoString()),
// 	initMemberInfo( Init_Member(AppSettings, fontItalic, char), arrayInfoString()),
// 	initMemberInfo( Init_Member(AppSettings, fontHeight, int)),
// 	initMemberInfo( Init_Member(AppSettings, fontShadow, float)),
// 	initMemberInfo( Init_Member(AppSettings, graphTitleFontHeight, int)),
// 	initMemberInfo( Init_Member(AppSettings, graphFontShadow, float)),
// 	initMemberInfo( Init_Member(AppSettings, windowHeightMod, float)),
// 	initMemberInfo( Init_Member(AppSettings, heightMod, float)),
// 	initMemberInfo( Init_Member(AppSettings, textPaddingMod, float)),
// 	initMemberInfo( Init_Member(AppSettings, rounding, float)),
// 	initMemberInfo( Init_Member(AppSettings, windowBorder, int)),
// 	initMemberInfo( Init_Member(AppSettings, border, int)),
// 	initMemberInfo( Init_Member(AppSettings, padding, int)),
// 	initMemberInfo( Init_Member(AppSettings, darkTheme, bool)),
// };

// StructMemberInfo Struct_Member_Info_Array(AppColors)[] = {
// 	initMemberInfo( Init_Member(AppColors, font, Vec4)),
// 	initMemberInfo( Init_Member(AppColors, font2, Vec4)),
// 	initMemberInfo( Init_Member(AppColors, graphFont, Vec4)),
// 	initMemberInfo( Init_Member(AppColors, fontShadow, Vec4)),
// 	initMemberInfo( Init_Member(AppColors, windowBorder, Vec4)),
// 	initMemberInfo( Init_Member(AppColors, background, Vec4)),
// 	initMemberInfo( Init_Member(AppColors, button, Vec4)),
// 	initMemberInfo( Init_Member(AppColors, uiBackground, Vec4)),
// 	initMemberInfo( Init_Member(AppColors, edge, Vec4)),
// 	initMemberInfo( Init_Member(AppColors, editCursor, Vec4)),
// 	initMemberInfo( Init_Member(AppColors, editSelection, Vec4)),
// 	initMemberInfo( Init_Member(AppColors, graphData1, Vec4)),
// 	initMemberInfo( Init_Member(AppColors, graphData2, Vec4)),
// 	initMemberInfo( Init_Member(AppColors, graphData3, Vec4)),
// 	initMemberInfo( Init_Member(AppColors, graphBackgroundBottom, Vec4)),
// 	initMemberInfo( Init_Member(AppColors, graphBackgroundTop, Vec4)),
// 	initMemberInfo( Init_Member(AppColors, graphMark, Vec4)),
// 	initMemberInfo( Init_Member(AppColors, graphSubMark, Vec4)),
// };

// StructMemberInfo Struct_Member_Info_Array(RelativeColor)[] = {
// 	initMemberInfo( Init_Member(RelativeColor, c, Vec4)),
// 	initMemberInfo( Init_Member(RelativeColor, i, int)),
// };

// StructMemberInfo Struct_Member_Info_Array(AppColorsRelative)[] = {
// 	initMemberInfo( Init_Member(AppColorsRelative, font, RelativeColor)),
// 	initMemberInfo( Init_Member(AppColorsRelative, font2, RelativeColor)),
// 	initMemberInfo( Init_Member(AppColorsRelative, graphFont, RelativeColor)),
// 	initMemberInfo( Init_Member(AppColorsRelative, fontShadow, RelativeColor)),
// 	initMemberInfo( Init_Member(AppColorsRelative, windowBorder, RelativeColor)),
// 	initMemberInfo( Init_Member(AppColorsRelative, background, RelativeColor)),
// 	initMemberInfo( Init_Member(AppColorsRelative, button, RelativeColor)),
// 	initMemberInfo( Init_Member(AppColorsRelative, uiBackground, RelativeColor)),
// 	initMemberInfo( Init_Member(AppColorsRelative, edge, RelativeColor)),
// 	initMemberInfo( Init_Member(AppColorsRelative, editCursor, RelativeColor)),
// 	initMemberInfo( Init_Member(AppColorsRelative, editSelection, RelativeColor)),
// 	initMemberInfo( Init_Member(AppColorsRelative, graphData1, RelativeColor)),
// 	initMemberInfo( Init_Member(AppColorsRelative, graphData2, RelativeColor)),
// 	initMemberInfo( Init_Member(AppColorsRelative, graphData3, RelativeColor)),
// 	initMemberInfo( Init_Member(AppColorsRelative, graphBackgroundBottom, RelativeColor)),
// 	initMemberInfo( Init_Member(AppColorsRelative, graphBackgroundTop, RelativeColor)),
// 	initMemberInfo( Init_Member(AppColorsRelative, graphMark, RelativeColor)),
// 	initMemberInfo( Init_Member(AppColorsRelative, graphSubMark, RelativeColor)),
// };

MetaParse()
struct TestStruct {
	bool b;
	Vec4 v;

	char* s;

	float fla[3];

	int* ia;
	int iaSize;
};

StructMemberInfo Struct_Member_Info_Array(TestStruct)[] = {
	initMemberInfo( Init_Member(TestStruct, b, bool)),
	initMemberInfo( Init_Member(TestStruct, v, Vec4)),
	initMemberInfo( Init_Member(TestStruct, s, char), {ARRAYTYPE_DYNAMIC, ARRAYSIZEMODE_NULL_TERMINATED}),
	initMemberInfo( Init_Member(TestStruct, fla, float), {ARRAYTYPE_CONSTANT, ARRAYSIZEMODE_SIZE, 3}),
	initMemberInfo( Init_Member(TestStruct, ia, int), {ARRAYTYPE_DYNAMIC, ARRAYSIZEMODE_OFFSET, offsetof(TestStruct, iaSize)}),
	initMemberInfo( Init_Member(TestStruct, iaSize, int)),
};

StructInfo debugStructInfos[] = {
	{ "int", sizeof(int), 0 },
	{ "float", sizeof(float), 0 },
	{ "char", sizeof(char), 0 },
	{ "bool", sizeof(bool), 0 },
	Init_Struct_Info(Vec3),
	Init_Struct_Info(Vec4),
	// Init_Struct_Info(AppSettings),
	// Init_Struct_Info(AppColors),
	// Init_Struct_Info(RelativeColor),
	// Init_Struct_Info(AppColorsRelative),
	Init_Struct_Info(TestStruct),
};

#define Get_Struct_Info(type) (debugStructInfos + STRUCTTYPE_##type)

// StructInfo* getStructInfo(int type) {

// }

bool typeIsCore(int type) {
	return debugStructInfos[type].memberCount == 0;
}
bool typeIsCore(StructMemberInfo* member) {
	return typeIsCore(member->type);
}

char* coreTypeToString(int type, void* data) {
	switch(type) {
		case STRUCTTYPE_int:   return fillString("%i", *(int*)data);
		case STRUCTTYPE_float: {
			int floatSize = 7;
			char* b = getTString(20);
			sprintf(b, "%1.7f", *(float*)data);
			return b;
		}
		case STRUCTTYPE_char:  return fillString("%c", *(char*)data);
		case STRUCTTYPE_bool:  return fillString((*((bool*)data)) ? "true" : "false");
		default: return "";
	}
}

char* castTypeArray(char* base, ArrayInfo aInfo) {
	char* arrayBase = (aInfo.type == ARRAYTYPE_CONSTANT) ? base :*((char**)(base));
	return arrayBase;
}

int getTypeArraySize(char* structBase, ArrayInfo aInfo, char* arrayBase = 0) {
	int arraySize;
	if(aInfo.sizeMode == ARRAYSIZEMODE_SIZE) arraySize = aInfo.size;
	else if(aInfo.sizeMode == ARRAYSIZEMODE_OFFSET) arraySize = *(int*)(structBase + aInfo.offset);
	else if(aInfo.sizeMode == ARRAYSIZEMODE_NULL_TERMINATED) arraySize = strLen(arrayBase);

	return arraySize;
}

void printType(int structType, void* data, int depth = 0);
void printTypeArray(char* structBase, StructMemberInfo member, char* data, int arrayIndex, int depth) {
	int arrayPrintLimit = 5;

	bool isPrimitive = typeIsCore(member.type);
	ArrayInfo aInfo = member.arrays[arrayIndex];
	int typeSize = debugStructInfos[member.type].size;

	char* arrayBase = castTypeArray(data, aInfo);
	int arraySize = getTypeArraySize(structBase, aInfo, arrayBase);

	// Print dynamic null-terminated char array as string.
	if(member.type == STRUCTTYPE_char && member.arrayCount == 1 && 
	   member.arrays[0].type == ARRAYTYPE_DYNAMIC && member.arrays[0].sizeMode == ARRAYSIZEMODE_NULL_TERMINATED) {
		char* value = arrayBase;
		printf("\"%s\"", value);
		printf(",");
		return;
	}

	printf("[ ");
	
	if(arrayIndex == member.arrayCount-1) {
		int size = min(arraySize, arrayPrintLimit);
		bool isPrimitive = typeIsCore(member.type);

		for(int i = 0; i < size; i++) {
			char* value = arrayBase + (typeSize*i);
			if(!isPrimitive) printf("{ ");
			printType(member.type, value, depth + 1);
			if(!isPrimitive) printf("}, ");
		}
		if(arraySize > arrayPrintLimit) printf("... ");
	} else {
		ArrayInfo aInfo2 = member.arrays[arrayIndex+1];

		int arrayOffset;
		if(aInfo.type == ARRAYTYPE_CONSTANT && aInfo2.type == ARRAYTYPE_CONSTANT) {
			int arraySize2 = getTypeArraySize(data, aInfo2);
			arrayOffset = arraySize2*typeSize;
		} else arrayOffset = sizeof(int*); // Pointers have the same size, so any will suffice.

		for(int i = 0; i < arraySize; i++) {
			printTypeArray(structBase, member, arrayBase + i*arrayOffset, arrayIndex+1, depth+1);
		}
	}

	printf("], ");
}

void printType(int structType, void* d, int depth) {

	char* data = (char*)d;

	int arrayPrintLimit = 5;
	int indent = 2;

	StructInfo* info = debugStructInfos + structType;

	if(depth == 0) printf("{\n");

	if(typeIsCore(structType)) {
		switch(structType) {
			case STRUCTTYPE_int: printf("%i", *(int*)data); break;
			case STRUCTTYPE_float: printf("%f", *(float*)data); break;
			case STRUCTTYPE_char: printf("%c", *data); break;
			case STRUCTTYPE_bool: printf((*((bool*)data)) ? "true" : "false"); break;
			default: break;
		};

		printf(", ");
		if(depth == 0) printf("\n");
		return;
	}

	for(int i = 0; i < info->memberCount; i++) {
		StructMemberInfo member = info->list[i];

		bool isPrimitive = typeIsCore(member.type);

		if(depth == 0) printf("%*s", indent*(depth+1), "");

		if(member.arrayCount == 0) {
			if(!isPrimitive) printf("{ ");
			printType(member.type, data + member.offset, depth + 1);
			if(!isPrimitive) printf("}, ");
			if(depth == 0) printf("\n");
		} else {
			ArrayInfo aInfo = member.arrays[0];
			printTypeArray(data, member, data + member.offset, 0, depth + 1);

			if(depth == 0) printf("\n");
		}

	}

	if(depth == 0) printf("}\n");
}

// Only structs and strings, no arrays.
void writeTypeSimple(char** b, int structType, void* d, int depth = 0) {
	StructInfo* info = debugStructInfos + structType;

	if(typeIsCore(structType)) {
		
		// Align negative numbers.
		if(structType == STRUCTTYPE_float) {
			float n = *((float*)d);
			if(n == -0.0f) *((float*)d) = 0.0f;
			if(n < 0) (*b)--;
		} else if(structType == STRUCTTYPE_int) {
			int n = *((int*)d);
			if(n == -0) *((int*)d) = 0;
			if(n < 0) (*b)--;
		}

		strCpyInc(b, coreTypeToString(structType, d));
		return;
	}

	if(depth > 0) strCpyInc(b, "{ ");

	int maxStringSize = 0;
	if(depth == 0) {
		for(int i = 0; i < info->memberCount; i++) {
			maxStringSize = max(strLen(info->list[i].name), maxStringSize);
		}
	}

	for(int i = 0; i < info->memberCount; i++) {
		StructMemberInfo member = info->list[i];

		if(depth == 0) {
			char* nameEqualsStr = fillString("%s = ", member.name);
			strCpyInc(b, nameEqualsStr);
			int indent = maxStringSize - strLen(member.name);
			for(int i = 0; i < indent; i++) strCpyInc(b, " ");
		}

		if(member.arrayCount == 0) {
			writeTypeSimple(b, member.type, ((char*)d) + member.offset, depth + 1);
		} else {
			char* arrayBase = castTypeArray(((char*)d) + member.offset, member.arrays[0]);
			strCpyInc(b, fillString("\"%s\"", arrayBase));
		}
		
		if(depth == 0) strCpyInc(b, ";");
		else {
			if(i < info->memberCount-1) strCpyInc(b, ", ");
			else strCpyInc(b, " ");
		}
		if(depth == 0) strCpyInc(b, "\r\n");
	}

	if(depth > 0) strCpyInc(b, "}");
}

enum {
	TYPE_TOKEN_BRACE_OPEN,
	TYPE_TOKEN_BRACE_CLOSE,
	TYPE_TOKEN_COMMA,
	TYPE_TOKEN_EQUALS,
	TYPE_TOKEN_SEMICOLON,
	TYPE_TOKEN_TRUE,
	TYPE_TOKEN_FALSE,
	TYPE_TOKEN_STRING,
	TYPE_TOKEN_INT,
	TYPE_TOKEN_FLOAT,
	TYPE_TOKEN_NAME,
};

Token typeGetToken(char** buffer, bool advance = true) {
	char* b = *buffer;
	// b = eatWhiteSpaces(b);

	// Skip whitespace and comments.
	{
		for(;;) {
			if(charIsWhiteSpace(*b)) {
				b = eatWhiteSpaces(b);
				continue;
			}
			if(b[0] == '/' && b[1] == '/') {
				b += 2;
				while(*b != '\n') *b++;
				continue;
			}
			break;
		}
	}

	char* startAddress = b;

	Token token;
	token.data = b;
	token.size = 1;

	char c = b[0];
	b++;
	switch(c) {
		case '{': token.type = TYPE_TOKEN_BRACE_OPEN; break;
		case '}': token.type = TYPE_TOKEN_BRACE_CLOSE; break;
		case ',': token.type = TYPE_TOKEN_COMMA; break;
		case '=': token.type = TYPE_TOKEN_EQUALS; break;
		case ';': token.type = TYPE_TOKEN_SEMICOLON; break;

		case '\"': {
			token.data = b;
			while(*b != '\"') {
				if(*b == '\\') {
					b += 2;
				} else b++;
			}
			b++;

			token.type = TYPE_TOKEN_STRING;
			token.size = b - startAddress - 2;
		} break;

		default: {
			if(charIsLetter(c)) {
				b--;
				if(strStartsWith(b, "true")) {
					token.type = TYPE_TOKEN_TRUE;
					token.size = 4;
					b += 4;
				} else if(strStartsWith(b, "false")) {
					token.type = TYPE_TOKEN_FALSE;
					token.size = 5;
					b += 5;
				} else {
					// Must be name.
					while(charIsAlphaNumeric(*b)) b++;
					token.type = TYPE_TOKEN_NAME;
					token.size = b - startAddress;
				}
			} else if(charIsDigit(c) || c == '-') {
				bool isFloat = false;

				b--;
				if(*b == '-') b++;
				if(*b == '0') b++;
				else while(charIsDigit(*b)) b++;
				if(*b == '.') {
					isFloat = true;
					b++;
					while(charIsDigit(*b)) b++;
				}
				if(*b == 'e' || *b == 'E') {
					b++;
					if(*b == '+' || *b == '-') b++;
					while(charIsDigit(*b)) b++;
				}

				token.type = isFloat ? TYPE_TOKEN_FLOAT : TYPE_TOKEN_INT;
				token.size = b - startAddress;
			} else {
				// Error.
				token.type = -1;
				token.size = 0;
			}
		}
	}

	if(advance) *buffer = b;

	return token;
}

bool expectToken(char** buffer, int tokenType) {
	Token token = typeGetToken(buffer);
	bool result = token.type == tokenType;
	return result;
}

void parseTypeSimple(char** buffer, int structType, void* d, int depth = 0) {
	char* data = (char*)d;
	StructInfo* info = debugStructInfos + structType;

	if(typeIsCore(structType)) {
		// strAppend(b, coreTypeToString(structType, d));
		Token token = typeGetToken(buffer);

		if(structType == STRUCTTYPE_int) *((int*)d) = strToInt(getTStringCpy(token.data, token.size));
		else if(structType == STRUCTTYPE_float) *((float*)d) = strToFloat(getTStringCpy(token.data, token.size));
		else if(structType == STRUCTTYPE_char) *((char*)d) = 'x';
		else if(structType == STRUCTTYPE_bool) *((bool*)d) = token.type == TYPE_TOKEN_TRUE ? true : false;

		return;
	}

	if(depth > 0) expectToken(buffer, TYPE_TOKEN_BRACE_OPEN);

	for(int i = 0; i < info->memberCount; i++) {
		StructMemberInfo member = info->list[i];

		if(depth == 0) {
			expectToken(buffer, TYPE_TOKEN_NAME);
			expectToken(buffer, TYPE_TOKEN_EQUALS);
		}

		if(member.arrayCount == 0) {
			parseTypeSimple(buffer, member.type, data + member.offset, depth + 1);
		} else {
			Token t = typeGetToken(buffer);
			char** memberData = (char**)(data + member.offset);
			// char* arrayBase = castTypeArray(data + member.offset, member.arrays[0]);
			// arrayBase = getPStringCpy(t.data, t.size);
			char* s = getPStringCpy(t.data, t.size);
			*memberData = s;
		}
		
		if(depth == 0) expectToken(buffer, TYPE_TOKEN_SEMICOLON);
		else {
			if(i < info->memberCount-1) expectToken(buffer, TYPE_TOKEN_COMMA);
		}
	}

	if(depth > 0) expectToken(buffer, TYPE_TOKEN_BRACE_CLOSE);
}









