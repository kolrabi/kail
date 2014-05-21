///////////////////////////////////////////////////////////////////////////////
// Tests for ResIL
// Copyright Björn Ganster
///////////////////////////////////////////////////////////////////////////////

// Prevent stupid MSVC warnings for using basic proven C functions
#ifdef _MSC_VER
#define _SCL_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#endif

#include <Windows.h>
#include <tchar.h>
#include <../../include/IL/il.h>
#include <../../include/IL/ilu.h>
//#include "zlib-1.2.6\\zlib.h"
#include <direct.h>

///////////////////////////////////////////////////////////////////////////////

#define PathCharMod "%S"

#ifdef _WIN32
#define PATH_SEPARATOR L"\\"
#else
#define PATH_SEPARATOR "/"
#endif

int errors;

///////////////////////////////////////////////////////////////////////////////

const TCHAR* sourceDir = L"D:\\testIL\\";
//const TCHAR* sourceDir = L"D:\\testIL\\bad\\";
//const TCHAR* sourceDir = L"D:\\temp\\bugs\\";
const TCHAR* targetDir = L"D:\\testIL\\decoded\\";

void printOffset(void* a, void* b, char* msg)
{
	size_t s = ((BYTE*)b)-((BYTE*)a);
	printf("%s: %i\n", msg, s);
}

inline void testHeap()
{
   #ifdef _DEBUG
	void* p = malloc(1024);
	free(p);
   #endif
}

///////////////////////////////////////////////////////////////////////////////

void test_ilLoad(wchar_t* sourceFN, wchar_t* targetFN)
{
	testHeap();
	ilInit();
	testHeap();
	ILuint handle = ilGenImage();
	testHeap();
	ilBindImage(handle);
	testHeap();
	//printf("Loading " PathCharMod "\n", sourceFN);
	ilResetRead();
	ILenum sourceType = ilDetermineType(sourceFN);
	if (!ilLoad(sourceType, sourceFN)) {
		printf("test_ilLoad: Failed to load %S\n", sourceFN);
		++errors;
		return;
	}
	testHeap();
	//ilConvertImage(IL_BGR, IL_UNSIGNED_BYTE);
	//ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
	//iluScale(150, 150, 1);
	testHeap();
	DeleteFile(targetFN);
	//printf("Saving " PathCharMod "\n", targetFN);
	if (!ilSaveImage(targetFN)) {
		printf("test_ilLoad: Failed to save " PathCharMod "\n", targetFN);
		++errors;
	}
	testHeap();
	ilDeleteImage(handle);
}

///////////////////////////////////////////////////////////////////////////////

void test_ilLoadF(wchar_t* sourceFN, wchar_t* targetFN)
{
	testHeap();
	ilInit();
	testHeap();
	ILuint handle = ilGenImage();
	testHeap();
	ilBindImage(handle);
	testHeap();
	//printf("Loading " PathCharMod "\n", sourceFN);
	bool loaded = false;

	//FILE* f = _wfopen(sourceFN, L"rb");
	FILE * f;
	char buf[10];
	_wfopen_s(&f, sourceFN, L"rb");
	fread(buf, 1, 10, f);
	fseek(f, 0, IL_SEEK_SET);
	if (f != NULL) {
		loaded = ilLoadF(IL_TYPE_UNKNOWN, f);
		if(!loaded) {
			printf("test_ilLoadF: Failed to load %S\n", sourceFN);
			++errors;
		}
	} else {
		printf("test_ilLoadF: Failed to open %S\n", sourceFN);
		++errors;
	}

	fclose(f);
	testHeap();
	//ilConvertImage(IL_BGR, IL_UNSIGNED_BYTE);
	//ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
	//iluScale(150, 150, 1);
	testHeap();
	DeleteFile(targetFN);
	//printf("Saving " PathCharMod "\n", targetFN);

	if (loaded)
		if (!ilSaveImage(targetFN)) {
			printf ("test_ilLoadF: Failed to save %S\n", targetFN);
			++errors;
		}

	testHeap();
	ilDeleteImage(handle);
}

///////////////////////////////////////////////////////////////////////////////

void test_ilLoadImage(wchar_t* sourceFN, wchar_t* targetFN)
{
	testHeap();
	ilInit();
	testHeap();
	ILuint handle = ilGenImage();
	testHeap();
	ilBindImage(handle);
	testHeap();
	//printf("Loading " PathCharMod "\n", sourceFN);
	if (!ilLoadImage(sourceFN)) {
		printf("test_ilLoadImage: Failed to load " PathCharMod "\n", sourceFN);
		++errors;
		return;
	}
	//ilConvertImage(IL_BGR, IL_UNSIGNED_BYTE);
	//ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
	//iluScale(150, 150, 1);
	testHeap();
	DeleteFile(targetFN);
	//printf("Saving " PathCharMod "\n", targetFN);
	if (!ilSaveImage(targetFN)) {
		printf("test_ilLoadImage: Failed to save " PathCharMod "\n", targetFN);
		++errors;
	}
	testHeap();
	ilDeleteImage(handle);
}

///////////////////////////////////////////////////////////////////////////////

void test_ilLoadL(wchar_t* sourceFN, wchar_t* targetFN)
{
	testHeap();
	ilInit();
	testHeap();
	ILuint handle = ilGenImage();
	testHeap();
	ilBindImage(handle);
	testHeap();
	//printf("Loading " PathCharMod "\n", sourceFN);
	FILE* f = _wfopen(sourceFN, L"rb");
	bool loaded = false;

	if (f != NULL) {
		fseek(f, 0, SEEK_END);
		INT64 size = ftell(f);
		if (size > 0) {
			fseek(f, 0, SEEK_SET);
			char* lump = new char[size];
			size_t read = fread(lump, 1, size, f);
			if (read == size) {
				loaded = ilLoadL(IL_TYPE_UNKNOWN, lump, read);
				if (!loaded) {
					printf("test_ilLoadL: Failed to load " PathCharMod "\n", sourceFN);
					++errors;
				}
			} else {
				printf("test_ilLoadL: Failed to read " PathCharMod "\n", sourceFN);
				++errors;
			}
			delete lump;
		}
		fclose(f);
	} else {
		printf("test_ilLoadL: Failed to load %S\n", sourceFN);
		++errors;
	}

	testHeap();
	//ilConvertImage(IL_BGR, IL_UNSIGNED_BYTE);
	//ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
	//iluScale(150, 150, 1);
	testHeap();
	DeleteFile(targetFN);
	//printf("Saving " PathCharMod "\n", targetFN);
	if (loaded)
		if (!ilSaveImage(targetFN)) {
			printf("test_ilLoadL: Failed to save " PathCharMod "\n", targetFN);
			++errors;
		}
	testHeap();
	ilDeleteImage(handle);
}

///////////////////////////////////////////////////////////////////////////////
// test ilLoadFuncs

BYTE* myData;
ILuint myDataSize, myReadPos;

ILHANDLE  ILAPIENTRY myOpenProc (ILconst_string)
{
	myReadPos = 0;
	return NULL;
}

void ILAPIENTRY myCloseProc(ILHANDLE)
{
}

ILboolean ILAPIENTRY myEofProc(ILHANDLE)
{
	if (myReadPos >= myDataSize)
		return true;
	else
		return false;
}

ILint ILAPIENTRY myGetcProc(ILHANDLE)
{
	if (myReadPos < myDataSize) {
		BYTE retVal = myData[myReadPos];
		++myReadPos;
		return retVal;
	} else 
		return 0;
}

ILuint ILAPIENTRY myReadProc(ILHANDLE, void* buf, ILuint size, ILuint count)
{
	ILuint toRead = size * count;
	if (myReadPos >= myDataSize) {
		count = 0;
		toRead = 0;
	} else {
		if (myReadPos + toRead >= myDataSize) {
			count = ((myDataSize-myReadPos) / size); // round down to size units
			toRead = count * size;
		}
	}

	if (toRead > 0) {
		memcpy(buf, &myData[myReadPos], toRead);
		myReadPos += toRead;
	}

	return count;
}

ILint ILAPIENTRY mySeekProc(ILHANDLE, ILint64 offset, ILuint mode)
{
	switch(mode) {
	case SEEK_SET:
		myReadPos = offset;
		break;
	case SEEK_CUR:
		myReadPos += offset;
		break;
	case SEEK_END:
		myReadPos = myDataSize + offset;
		break;
	}

	return 0; // success
}

ILuint ILAPIENTRY myTellProc(ILHANDLE)
{
	return myReadPos;
}

void test_ilLoadFuncs(wchar_t* sourceFN, wchar_t* targetFN)
{
	testHeap();
	ilInit();
	testHeap();
	ILuint handle = ilGenImage();
	testHeap();
	ilBindImage(handle);
	testHeap();
	//printf("Loading " PathCharMod "\n", sourceFN);
	FILE* f = _wfopen(sourceFN, L"rb");
	bool loaded = false;

	if (f != NULL) {
		fseek(f, 0, SEEK_END);
		myDataSize = ftell(f);
		if (myDataSize > 0) {
			fseek(f, 0, SEEK_SET);
			myData = new BYTE[myDataSize];
			size_t read = fread(myData, 1, myDataSize, f);
			myReadPos = 0;
			if (read == myDataSize) {
				//loaded = ilLoadL(IL_TYPE_UNKNOWN, lump, read);
				ilSetRead(myOpenProc, myCloseProc, myEofProc, myGetcProc, myReadProc, mySeekProc, myTellProc);
				loaded = ilLoadFuncs(IL_TYPE_UNKNOWN);
				if (!loaded) {
					printf("test_ilLoadFuncs: Failed to load " PathCharMod "\n", sourceFN);
					++errors;
				}
			} else {
				printf("test_ilLoadFuncs: Failed to read " PathCharMod "\n", sourceFN);
				++errors;
			}
			delete myData;
		}
		fclose(f);
	} else {
		printf("test_ilLoadFuncs: Failed to open %S\n", sourceFN);
		++errors;
	}

	testHeap();
	//ilConvertImage(IL_BGR, IL_UNSIGNED_BYTE);
	//ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
	//iluScale(150, 150, 1);
	testHeap();
	DeleteFile(targetFN);
	//printf("Saving " PathCharMod "\n", targetFN);
	if (loaded)
		if(!ilSaveImage(targetFN)) {
			printf("Failed to save " PathCharMod " after ilLoadFuncs\n", targetFN);
			++errors;
		}
	testHeap();
	ilDeleteImage(handle);
}

///////////////////////////////////////////////////////////////////////////////

ILboolean testDetermineTypeFromContent(TCHAR* fn)
{
	testHeap();
	ilInit();
	testHeap();
	ILuint handle = ilGenImage();
	testHeap();
	ilBindImage(handle);
	testHeap();
	//printf("Loading " PathCharMod "\n", sourceFN);
	FILE* f = _wfopen(fn, L"rb");
	bool loaded = false;
	ILenum type = IL_TYPE_UNKNOWN;

	if (f != NULL) {
		fseek(f, 0, SEEK_END);
		myDataSize = ftell(f);
		if (myDataSize > 0) {
			fseek(f, 0, SEEK_SET);
			myData = new BYTE[myDataSize];
			size_t read = fread(myData, 1, myDataSize, f);
			myReadPos = 0;
			if (read == myDataSize) {
				//loaded = ilLoadL(IL_TYPE_UNKNOWN, lump, read);
				ilSetRead(myOpenProc, myCloseProc, myEofProc, myGetcProc, myReadProc, mySeekProc, myTellProc);
				type = ilDetermineTypeFuncs();
				if (type == IL_TYPE_UNKNOWN) {
					printf("testDetermineTypeFromContent: Failed to determine type of " PathCharMod "\n", fn);
					++errors;
				}
			} else {
				printf("testDetermineTypeFromContent: Failed to read " PathCharMod "\n", fn);
				++errors;
			}
			delete myData;
		}
		fclose(f);
	} else {
		printf("testDetermineTypeFromContent: Failed to open %S\n", fn);
		++errors;
	}

	testHeap();
	ilDeleteImage(handle);

	if (type == IL_TYPE_UNKNOWN) {
		++errors;
		return IL_FALSE;
	} else
		return IL_TRUE;
}

///////////////////////////////////////////////////////////////////////////////

void testLoaders(TCHAR* fn)
{
	TCHAR sourceFN[MAX_PATH];
	TCHAR targetFN[MAX_PATH];

	_tcscpy(sourceFN, sourceDir);
	_tcscat(sourceFN, fn);

	_tcscpy(targetFN, targetDir);
	_tcscat(targetFN, fn);
	_tcscat(targetFN, L".ilLoadImage.bmp");
	test_ilLoadImage(sourceFN, targetFN);

	_tcscpy(targetFN, targetDir);
	_tcscat(targetFN, fn);
	_tcscat(targetFN, L".ilLoad.bmp");
	test_ilLoad(sourceFN, targetFN);

	// ilLoadF, ilLoadL and ilLoadFuncs can work only if the type of the file can be determined
	// correctly from the contents without knowing the extension, so test that before running
	// other checks
	if (testDetermineTypeFromContent(sourceFN)) {
		_tcscpy(targetFN, targetDir);
		_tcscat(targetFN, fn);
		_tcscat(targetFN, L".ilLoadF.bmp");
		test_ilLoadF(sourceFN, targetFN);

		_tcscpy(targetFN, targetDir);
		_tcscat(targetFN, fn);
		_tcscat(targetFN, L".ilLoadL.bmp");
		test_ilLoadL(sourceFN, targetFN);

		_tcscpy(targetFN, targetDir);
		_tcscat(targetFN, fn);
		_tcscat(targetFN, L".ilLoadFuncs.bmp");
		test_ilLoadFuncs(sourceFN, targetFN);
	} else {
		// Error message printed in testDetermineTypeFromContent
		//printf("Type check failed for " PathCharMod "\n", fn);
	}
}

///////////////////////////////////////////////////////////////////////////////

FILE * writeFile;

ILint ILAPIENTRY myPutc  (ILubyte b, ILHANDLE)
{
	return fputc(b, writeFile);
}

ILint ILAPIENTRY myWrite (const void* data, ILuint count, ILuint size, ILHANDLE)
{
	return fwrite(data, count, size, writeFile);
}

ILint  ILAPIENTRY mySeek (ILHANDLE, ILint64 offset, ILuint origin)
{
	return fseek(writeFile, offset, origin);
}

ILuint ILAPIENTRY myTell (ILHANDLE)
{
	return ftell(writeFile);
}

void testSavers2(ILenum type, const TCHAR* targetName, const TCHAR* targetExt)
{
	TCHAR targetFN[MAX_PATH];

	// Test ilSave
	_tcscpy(targetFN, targetName);
	_tcscat(targetFN, L".ilSave.");
	_tcscat(targetFN, targetExt);
	if (!ilSave(type, targetFN)) {
		printf("testSavers2: Failed to save " PathCharMod " using ilSave\n", targetFN);
		++errors;
	}

	// Test ilSaveF
	_tcscpy(targetFN, targetName);
	_tcscat(targetFN, L".ilSaveF.");
	_tcscat(targetFN, targetExt);
	FILE* file = _wfopen(targetFN, L"wb");
	if (!ilSaveF(type, file)) {
		printf("testSavers2: Failed to save " PathCharMod " using ilSaveF\n", targetFN);
		++errors;
	}
	fclose(file);

	// Test ilSaveL
	_tcscpy(targetFN, targetName);
	_tcscat(targetFN, L"ilSaveL.");
	_tcscat(targetFN, targetExt);
	ILuint lumpSize = ilDetermineSize(type);
	BYTE* lump = new BYTE[lumpSize];
	ILuint writtenToLump = ilSaveL(type, lump, lumpSize);
	if (writtenToLump > 0) {
		FILE* file = _wfopen(targetFN, L"wb");
		size_t writtenToFile = fwrite(lump, 1, lumpSize, file);
		if (writtenToLump != writtenToFile) {
			printf("testSavers2: Failed to write " PathCharMod " after ilSaveL\n", targetFN);
			++errors;
		}
		fclose(file);
	} else {
		printf("testSavers2: Failed to save " PathCharMod " using ilSaveL\n", targetFN);
		++errors;
	}
	delete lump;

	// Test ilSaveFuncs
	wcscpy(targetFN, targetName);
	wcscat(targetFN, L".ilSaveFuncs.");
	wcscat(targetFN, targetExt);
	writeFile = _wfopen(targetFN, L"wb");
	if (writeFile != NULL) {
		ilSetWrite(NULL, NULL, myPutc, mySeek, myTell, myWrite);
		if (!ilSaveFuncs(type))
			printf("testSavers2: Failed to save " PathCharMod " using ilSave\n", targetFN);
		fclose(writeFile);
	} else
		printf("testSavers2: Failed to open " PathCharMod " for writing\n", targetFN);
}

///////////////////////////////////////////////////////////////////////////////

void testSavers(const TCHAR* sourceFN, const TCHAR* targetFN)
{
	testHeap();
	ilInit();
	testHeap();
	ILuint handle = ilGenImage();
	testHeap();
	ilBindImage(handle);
	testHeap();
	if (!ilLoadImage(sourceFN)) {
		printf("Failed to load %S using ilLoadImage\n", sourceFN);
		++errors;
		return;
	}
	testHeap();

	// gif, ico: no save support...
	// todo: psd, pcx, tga, tif
	testSavers2(IL_BMP, targetFN, L"bmp");
	testSavers2(IL_JPG, targetFN, L"jpg");
	testSavers2(IL_PNG, targetFN, L"png");
	testSavers2(IL_PSD, targetFN, L"psd");
	testSavers2(IL_PCX, targetFN, L"pcx");
	testSavers2(IL_TGA, targetFN, L"tga");
	testSavers2(IL_TIF, targetFN, L"tif");
	testSavers2(IL_TGA, targetFN, L"tga");
	testSavers2(IL_PCX, targetFN, L"pcx");
	testSavers2(IL_PNM, targetFN, L"pnm");
	testSavers2(IL_SGI, targetFN, L"sgi");
	testSavers2(IL_WBMP, targetFN, L"wbmp");
	testSavers2(IL_MNG, targetFN, L"mng");
	testSavers2(IL_VTF, targetFN, L"vtf");

	testHeap();
	ilDeleteImage(handle);
}

///////////////////////////////////////////////////////////////////////////////

int wmain (int argc, TCHAR** argv)
{
	errors = 0;
	_wmkdir(targetDir);

	//testLoaders(L"IMG_5502.mng"); // memory leak

	// Test ilLoad* function family
	// Each file in a given folder is loaded using the ilLoad* functions and saved as a bmp
	TCHAR searchMask[MAX_PATH];
	_tcscpy(searchMask, sourceDir);
	_tcscat(searchMask, L"*");
	WIN32_FIND_DATAW data;
	HANDLE h = FindFirstFile(searchMask, &data);
	if (h != INVALID_HANDLE_VALUE) {
		do {
			if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				testLoaders(data.cFileName);
		} while (FindNextFile(h, &data));
		FindClose(h);
	}

	// Test ilSave* function family
	// A single image is loaded and saved using the ilSave* function family
	_wmkdir(L"D:\\TestIL\\encoded");
	testSavers(L"D:\\TestIL\\Don't Panic 24.bmp", L"D:\\TestIL\\encoded\\Don't Panic 24");

 	printf("%i errors\n", errors);
}
