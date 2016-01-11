/*
Copyright 2015 Thomas Laurenson
thomaslaurenson.com

This file was taken and modified from the Regshot project
See: http://sourceforge.net/projects/regshot/
Copyright 2011-2013 Regshot Team
Copyright 1999-2003,2007,2011 TiANWEi
Copyright 2004 tulipfan

This file is part of LiveDiff.

LiveDiff is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

LiveDiff is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with LiveDiff.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "global.h"

#define MAX_SIGNATURE_LENGTH 12
#define LIVEDIFF_READ_BLOCK_SIZE 8192

// VARIABLES FOR SAVING SNAPSHOTS
// Default file extension when saving/loading snapshots
LPTSTR lpszLiveDiffFileDefExt = TEXT("shot");

// Specify file sugnature (magic number) for saved snapshots
// SBCS/MBCS signature (even in Unicode builds for backwards compatibility)
char szLiveDiffFileSignatureSBCS[] = "LIVEDIFF_SNAPSHOT";
// use a number to define a file format not compatible with older releases (e.g. "3" could be UTF-32 or Big Endian)
char szLiveDoffFileSignatureUTF16[] = "LIVEDIFF_SNAPSHOT2";
#define szLiveDiffFileSignature szLiveDoffFileSignatureUTF16

// Save snapshot variables
FILEHEADER fileheader;
FILEEXTRADATA fileextradata;
SAVEKEYCONTENT sKC;
SAVEVALUECONTENT sVC;

// Variable for snapshot 1 and 2
SNAPSHOT Shot1;
SNAPSHOT Shot2;

// Buffers and size variables
LPBYTE lpFileBuffer;
LPTSTR lpStringBuffer;
LPBYTE lpDataBuffer;
size_t nStringBufferSize;
size_t nDataBufferSize;
size_t nSourceSize;

// Compare result structure
COMPRESULTS	CompareResult;

// Empty string statement
LPTSTR lpszEmpty = TEXT("");

// NumberOfBytesWritten
DWORD NBW;

// Handle of file
HANDLE hFileWholeReg;

// Filetime structure for last write timestampt
FILETIME ftLastWrite;

// Initialise Registry hive naming conventions
LPTSTR lpszHKLMShort = TEXT("HKLM");
LPTSTR lpszHKLMLong = TEXT("HKEY_LOCAL_MACHINE");
LPTSTR lpszHKUShort = TEXT("HKU");
LPTSTR lpszHKULong = TEXT("HKEY_USERS");

// The current Registry hive being process (used for blacklisting)
LPTSTR lpszCurrentHive;

// Long int for error numbering
LONG nErrNo;

// ----------------------------------------------------------------------
// Get whole registry key name from KEYCONTENT
// ----------------------------------------------------------------------
LPTSTR GetWholeKeyName(LPKEYCONTENT lpStartKC, BOOL fUseLongNames)
{
	LPKEYCONTENT lpKC;
	LPTSTR lpszName;
	LPTSTR lpszTail;
	size_t cchName;
	LPTSTR lpszKeyName;

	cchName = 0;
	for (lpKC = lpStartKC; NULL != lpKC; lpKC = lpKC->lpFatherKC) {
		if (NULL != lpKC->lpszKeyName) {
			if ((fUseLongNames) && (lpszHKLMShort == lpKC->lpszKeyName)) {
				cchName += CCH_HKLM_LONG;
			}
			else if ((fUseLongNames) && (lpszHKUShort == lpKC->lpszKeyName)) {
				cchName += CCH_HKU_LONG;
			}
			else {
				cchName += lpKC->cchKeyName;
			}
			cchName++;  // +1 char for backslash or NULL char
		}
	}
	if (0 == cchName) {  // at least create an empty string with NULL char
		cchName++;
	}

	lpszName = MYALLOC(cchName * sizeof(TCHAR));

	lpszTail = &lpszName[cchName - 1];
	lpszTail[0] = (TCHAR)'\0';

	for (lpKC = lpStartKC; NULL != lpKC; lpKC = lpKC->lpFatherKC) {
		if (NULL != lpKC->lpszKeyName) {
			if ((fUseLongNames) && (lpszHKLMShort == lpKC->lpszKeyName)) {
				cchName = CCH_HKLM_LONG;
				lpszKeyName = lpszHKLMLong;
			}
			else if ((fUseLongNames) && (lpszHKUShort == lpKC->lpszKeyName)) {
				cchName = CCH_HKU_LONG;
				lpszKeyName = lpszHKULong;
			}
			else {
				cchName = lpKC->cchKeyName;
				lpszKeyName = lpKC->lpszKeyName;
			}
			_tcsncpy(lpszTail -= cchName, lpszKeyName, cchName);
			if (lpszTail > lpszName) {
				*--lpszTail = (TCHAR)'\\';
			}
		}
	}

	return lpszName;
}

// ----------------------------------------------------------------------
// Get whole value name from VALUECONTENT
// ----------------------------------------------------------------------
LPTSTR GetWholeValueName(LPVALUECONTENT lpVC, BOOL fUseLongNames)
{
	LPKEYCONTENT lpKC;
	LPTSTR lpszName;
	LPTSTR lpszTail;
	size_t cchName;
	size_t cchValueName;
	LPTSTR lpszKeyName;

	cchValueName = 0;
	if (NULL != lpVC->lpszValueName) 
	{
		cchValueName = lpVC->cchValueName;
	}
	/*
	else
	{
		// If a default value, set it so
		lpVC->lpszValueName = TEXT("(Default)");
		lpVC->cchValueName = _tcslen(lpVC->lpszValueName) + 1;
		cchValueName = _tcslen(lpVC->lpszValueName) + 1;
	}*/

	cchName = cchValueName + 1;  // +1 char for NULL char

	for (lpKC = lpVC->lpFatherKC; NULL != lpKC; lpKC = lpKC->lpFatherKC) {
		if (NULL != lpKC->lpszKeyName) {
			if ((fUseLongNames) && (lpszHKLMShort == lpKC->lpszKeyName)) {
				cchName += CCH_HKLM_LONG;
			}
			else if ((fUseLongNames) && (lpszHKUShort == lpKC->lpszKeyName)) {
				cchName += CCH_HKU_LONG;
			}
			else {
				cchName += lpKC->cchKeyName;
			}
			cchName++;  // +1 char for backslash
		}
	}

	lpszName = MYALLOC(cchName * sizeof(TCHAR));

	lpszTail = &lpszName[cchName - 1];
	lpszTail[0] = (TCHAR)'\0';

	if (NULL != lpVC->lpszValueName) {
		_tcsncpy(lpszTail -= cchValueName, lpVC->lpszValueName, cchValueName);
	}

	if (lpszTail > lpszName) {
		*--lpszTail = (TCHAR)'\\';
	}

	for (lpKC = lpVC->lpFatherKC; NULL != lpKC; lpKC = lpKC->lpFatherKC) {
		if (NULL != lpKC->lpszKeyName) {
			if ((fUseLongNames) && (lpszHKLMShort == lpKC->lpszKeyName)) {
				cchName = CCH_HKLM_LONG;
				lpszKeyName = lpszHKLMLong;
			}
			else if ((fUseLongNames) && (lpszHKUShort == lpKC->lpszKeyName)) {
				cchName = CCH_HKU_LONG;
				lpszKeyName = lpszHKULong;
			}
			else {
				cchName = lpKC->cchKeyName;
				lpszKeyName = lpKC->lpszKeyName;
			}
			_tcsncpy(lpszTail -= cchName, lpszKeyName, cchName);
			if (lpszTail > lpszName) {
				*--lpszTail = (TCHAR)'\\';

			}
		}
	}

	return lpszName;
}

// ----------------------------------------------------------------------
// Determine the Registry value type (e.g., REG_SZ, REG_BINARY) and return 
// ----------------------------------------------------------------------
LPTSTR GetValueDataType(DWORD nTypeCode)
{
	LPTSTR lpszDataType;
	lpszDataType = MYALLOC0(40 * sizeof(wchar_t));
	if (nTypeCode == REG_NONE) { lpszDataType = TEXT("REG_NONE"); }
	else if (nTypeCode == REG_SZ) { lpszDataType = TEXT("REG_SZ"); }
	else if (nTypeCode == REG_EXPAND_SZ) { lpszDataType = TEXT("REG_EXPAND_SZ"); }
	else if (nTypeCode == REG_BINARY) { lpszDataType = TEXT("REG_BINARY"); }
	else if (nTypeCode == REG_DWORD) { lpszDataType = TEXT("REG_DWORD"); }
	else if (nTypeCode == REG_DWORD_BIG_ENDIAN) { lpszDataType = TEXT("REG_DWORD_BIG_ENDIAN"); }
	else if (nTypeCode == REG_LINK) { lpszDataType = TEXT("REG_LINK"); }
	else if (nTypeCode == REG_MULTI_SZ) { lpszDataType = TEXT("REG_MULTI_SZ"); }
	else if (nTypeCode == REG_RESOURCE_LIST) { lpszDataType = TEXT("REG_RESOURCE_LIST"); }
	else if (nTypeCode == REG_FULL_RESOURCE_DESCRIPTOR) { lpszDataType = TEXT("REG_FULL_RESOURCE_DESCRIPTOR"); }
	else if (nTypeCode == REG_RESOURCE_REQUIREMENTS_LIST) { lpszDataType = TEXT("REG_RESOURCE_REQUIREMENTS_LIST"); }
	else if (nTypeCode == REG_QWORD) { lpszDataType = TEXT("REG_QWORD"); }
	return lpszDataType;
}

// ----------------------------------------------------------------------
// Parse Registry value data
// Return a string (based on data type) that can be printed
// ----------------------------------------------------------------------
LPTSTR ParseValueData(LPBYTE lpData, PDWORD lpcbData, DWORD nTypeCode)
{
	LPTSTR lpszValueData;
	lpszValueData = NULL;

	// Check if the Registry value data is NULL, else process the Registry value data
	if (NULL == lpData)
	{
		lpszValueData = TransformValueData(lpData, lpcbData, REG_BINARY);
	}
	else
	{
		DWORD cbData;
		size_t cchMax;
		size_t cchActual;
		cbData = *lpcbData;
		switch (nTypeCode) 
		{

		// Process the three different string types (normal, expanded and multi)
		case REG_SZ:
		case REG_EXPAND_SZ:
		case REG_MULTI_SZ:
			// We need to do a check for hidden bytes after string[s]
			cchMax = cbData / sizeof(TCHAR);
			if (REG_MULTI_SZ == nTypeCode)
			{
				// Do a search for double NULL chars (REG_MULTI_SZ ends with \0\0)
				for (cchActual = 0; cchActual < cchMax; cchActual++)
				{
					if (0 != ((LPTSTR)lpData)[cchActual]) {
						continue;
					}
					cchActual++;
					// Special case check for incorrectly terminated string
					if (cchActual >= cchMax) {
						break;
					}
					if (0 != ((LPTSTR)lpData)[cchActual]) {
						continue;
					}
					// Found a double NULL terminated string
					cchActual++;
					break;
				}
			}
			else
			{
				// Must be a REG_SZ or REG_EXPAND_SZ
				cchActual = _tcsnlen((LPTSTR)lpData, cchMax);
				if (cchActual < cchMax) {
					cchActual++;  // Account for NULL character
				}
			}

			// Do a size check; actual size VS specified size
			if ((cchActual * sizeof(TCHAR)) == cbData) {
				// If determined size is the same as specified size
				// process using the specified Registry value type
				lpszValueData = TransformValueData(lpData, lpcbData, nTypeCode);
			}
			else
			{
				// Else process the string as REG_BINARY (binary)
				lpszValueData = TransformValueData(lpData, lpcbData, REG_BINARY);
			}
			break;

			// Process DWORD types
		case REG_DWORD_LITTLE_ENDIAN:
		case REG_DWORD_BIG_ENDIAN:
			// Do a size check; DWORD size VS specified size
			if (sizeof(DWORD) == cbData)
			{
				// If the size of the value data is the same as a DWORD size
				// process using the specified Registry value type
				lpszValueData = TransformValueData(lpData, lpcbData, nTypeCode);
			}
			else
			{
				// Else process the string as REG_BINARY (binary)
				lpszValueData = TransformValueData(lpData, lpcbData, REG_BINARY);
			}
			break;

			// Process QWORD types (currently only little endian)
		case REG_QWORD_LITTLE_ENDIAN:
			// If the size if the value data is the same as a QWORD size
			// process using the specified Registry value type
			if (sizeof(QWORD) == cbData)
			{
				// If the size of the value data is the same as a QWORD size
				// process using the specified Registry value type
				lpszValueData = TransformValueData(lpData, lpcbData, nTypeCode);
			}
			else
			{
				// Else process the string as REG_BINARY (binary)
				lpszValueData = TransformValueData(lpData, lpcbData, REG_BINARY);
			}
			break;

		// Process any other Registry value data type as REG_BINARY (binary)
		default:
			lpszValueData = TransformValueData(lpData, lpcbData, REG_BINARY);
		}
	}
	return lpszValueData;
}


// ----------------------------------------------------------------------
// Transform Registry value data based on data_type
// ----------------------------------------------------------------------
LPTSTR TransformValueData(LPBYTE lpData, PDWORD lpcbData, DWORD nConversionType)
{
	LPTSTR lpszValueDataIsNULL = TEXT("NULL");
	LPTSTR lpszValueData;
	LPDWORD lpDword;
	LPQWORD lpQword;
	DWORD nDwordCpu;

	lpszValueData = NULL;
	lpDword = NULL;
	lpQword = NULL;
	lpStringBuffer = NULL;

	if (NULL == lpData)
	{
		lpszValueData = MYALLOC((_tcslen(lpszValueDataIsNULL) + 1) * sizeof(TCHAR));
		//_tcscpy(lpszValueData, lpszValueDataIsNULL);
		_tcscpy_s(lpszValueData, (_tcslen(lpszValueDataIsNULL) + 1) * sizeof(TCHAR), lpszValueDataIsNULL);
	}
	else
	{
		DWORD cbData;
		DWORD ibCurrent;
		size_t cchToGo;
		size_t cchString;
		size_t cchActual;
		LPTSTR lpszSrc;
		LPTSTR lpszDst;

		cbData = *lpcbData;

		switch (nConversionType) {
		case REG_SZ:
			// A normal NULL termination string
			lpszValueData = MYALLOC0(sizeof(TCHAR) + cbData);
			if (NULL != lpData) {
				memcpy(lpszValueData, lpData, cbData);
			}
			break;

		case REG_EXPAND_SZ:
			// A NULL terminated string that contains unexpanded references to environment variables (e.g., "%PATH%")
			// Process and output in the following format: "<string>"\0
			lpszValueData = MYALLOC0(sizeof(TCHAR) + cbData);
			if (NULL != lpData) {
				memcpy(lpszValueData, lpData, cbData);
			}
			break;

		case REG_MULTI_SZ:
			// A sequence of null-terminated strings, terminated by an empty string (\0).
			// Process and output in the following format: "<string>", "<string>", "<string>", ...\0
			nStringBufferSize = AdjustBuffer(&lpStringBuffer, nStringBufferSize, 10 + (2 * cbData), 1024);
			ZeroMemory(lpStringBuffer, nStringBufferSize);
			lpszDst = lpStringBuffer;

			cchActual = 0;
			// Only process if the actual value data is not NULL
			if (NULL != lpData)
			{
				lpszSrc = (LPTSTR)lpData;
				// Convert the byte count to a char count
				cchToGo = cbData / sizeof(TCHAR);
				while ((cchToGo > 0) && (*lpszSrc)) {
					if (0 != cchActual)
					{
						// Add comma (",") to separate strings
						//_tcscpy(lpszDst, TEXT(","));
						_tcscpy_s(lpszDst, 2 * sizeof(TCHAR), TEXT(","));
						// Increase the count by 1 to account for comma
						lpszDst += 1;
						cchActual += 1;
					}
					cchString = _tcsnlen(lpszSrc, cchToGo);
					_tcsncpy(lpszDst, lpszSrc, cchString);
					lpszDst += cchString;
					cchActual += cchString;

					// Decrease count for processed, if count ToGo is 0 then we are done
					cchToGo -= cchString;
					if (cchToGo == 0) {
						break;
					}

					// Account for the NULL character
					lpszSrc += cchString + 1;
					cchToGo -= 1;
				}
			}
			cchActual += 3 + 1;

			// Allocate memory for the constructed string and copy to value data string
			lpszValueData = MYALLOC(cchActual * sizeof(TCHAR));
			_tcscpy(lpszValueData, lpStringBuffer);
			//_tcscpy_s(lpszValueData, cchActual * sizeof(TCHAR), lpStringBuffer);

			break;

		case REG_DWORD_BIG_ENDIAN:
			// convert DWORD big endian
			lpDword = &nDwordCpu;
			for (ibCurrent = 0; ibCurrent < sizeof(DWORD); ibCurrent++) {
				((LPBYTE)&nDwordCpu)[ibCurrent] = lpData[sizeof(DWORD) - 1 - ibCurrent];
			}
			// Output format: "0xXXXXXXXX\0"
			lpszValueData = MYALLOC0((3 + 8 + 1) * sizeof(TCHAR));
			if (NULL != lpData) {
				//_sntprintf(lpszValueData, (3 + 8 + 1), TEXT("0x%08X\0"), *lpDword);
				_sntprintf_s(lpszValueData, (3 + 8 + 1) * sizeof(TCHAR), (3 + 8 + 1), TEXT("0x%08X\0"), *lpDword);
			}
			break;

		case REG_DWORD:
			// A native DWORD that can be displayed as DWORD
			// This includes REG_DWORD_LITTLE_ENDIAN (the same as DWORD)
			if (NULL == lpDword) {
				lpDword = (LPDWORD)lpData;
			}
			// Output format: "0xXXXXXXXX\0"
			lpszValueData = MYALLOC0((2 + 8 + 1) * sizeof(TCHAR));
			if (NULL != lpData) {
				_sntprintf(lpszValueData, (2 + 8 + 1), TEXT("0x%08X\0"), *lpDword);
				//_sntprintf_s(lpszValueData, (2 + 8 + 1) * sizeof(TCHAR), (2 + 8 + 1), TEXT("0x%08X\0"), *lpDword);
			}
			break;

		case REG_QWORD:
			// A native QWORD that can be displayed as QWORD
			// This includes REG_QWORD_LITTLE_ENDIAN (which is the same as QWORD)
			if (NULL == lpQword) {
				lpQword = (LPQWORD)lpData;
			}
			// Output format: "0xXXXXXXXXXXXXXXXX\0"
			lpszValueData = MYALLOC0((3 + 16 + 1) * sizeof(TCHAR));
			if (NULL != lpData) {
				_sntprintf(lpszValueData, (3 + 16 + 1), TEXT("0x%016I64X\0"), *lpQword);
			}
			break;

		default:
			// Default processing and display method: Present value as hex bytes
			// Output format: "[XX][XX]...[XX]\0"
			lpszValueData = MYALLOC0((1 + (cbData * 3) + 1) * sizeof(TCHAR));
			for (ibCurrent = 0; ibCurrent < cbData; ibCurrent++) {
				_sntprintf(lpszValueData + (ibCurrent * 3), 4, TEXT(" %02X\0"), *(lpData + ibCurrent));
			}
			lpszValueData += 1; // Remove the first space character
		}
	}
	return lpszValueData;
}


//-------------------------------------------------------------
// Routine to create new comparison result, distribute to different pointers
//-------------------------------------------------------------
VOID CreateNewResult(DWORD nActionType, LPVOID lpContentOld, LPVOID lpContentNew)
{
	LPCOMPRESULT lpCR;

	lpCR = (LPCOMPRESULT)MYALLOC0(sizeof(COMPRESULT));
	lpCR->lpContentOld = lpContentOld;
	lpCR->lpContentNew = lpContentNew;

	switch (nActionType) 
	{
	case KEYDEL:
		(NULL == CompareResult.stCRHeads.lpCRKeyDeleted) ? (CompareResult.stCRHeads.lpCRKeyDeleted = lpCR) : (CompareResult.stCRCurrent.lpCRKeyDeleted->lpNextCR = lpCR);
		CompareResult.stCRCurrent.lpCRKeyDeleted = lpCR;
		break;
	case KEYADD:
		(NULL == CompareResult.stCRHeads.lpCRKeyAdded) ? (CompareResult.stCRHeads.lpCRKeyAdded = lpCR) : (CompareResult.stCRCurrent.lpCRKeyAdded->lpNextCR = lpCR);
		CompareResult.stCRCurrent.lpCRKeyAdded = lpCR;
		break;
	case VALDEL:
		(NULL == CompareResult.stCRHeads.lpCRValDeleted) ? (CompareResult.stCRHeads.lpCRValDeleted = lpCR) : (CompareResult.stCRCurrent.lpCRValDeleted->lpNextCR = lpCR);
		CompareResult.stCRCurrent.lpCRValDeleted = lpCR;
		break;
	case VALADD:
		(NULL == CompareResult.stCRHeads.lpCRValAdded) ? (CompareResult.stCRHeads.lpCRValAdded = lpCR) : (CompareResult.stCRCurrent.lpCRValAdded->lpNextCR = lpCR);
		CompareResult.stCRCurrent.lpCRValAdded = lpCR;
		break;
	case VALMODI:
		(NULL == CompareResult.stCRHeads.lpCRValModified) ? (CompareResult.stCRHeads.lpCRValModified = lpCR) : (CompareResult.stCRCurrent.lpCRValModified->lpNextCR = lpCR);
		CompareResult.stCRCurrent.lpCRValModified = lpCR;
		break;
	case DIRDEL:
		(NULL == CompareResult.stCRHeads.lpCRDirDeleted) ? (CompareResult.stCRHeads.lpCRDirDeleted = lpCR) : (CompareResult.stCRCurrent.lpCRDirDeleted->lpNextCR = lpCR);
		CompareResult.stCRCurrent.lpCRDirDeleted = lpCR;
		break;
	case DIRADD:
		(NULL == CompareResult.stCRHeads.lpCRDirAdded) ? (CompareResult.stCRHeads.lpCRDirAdded = lpCR) : (CompareResult.stCRCurrent.lpCRDirAdded->lpNextCR = lpCR);
		CompareResult.stCRCurrent.lpCRDirAdded = lpCR;
		break;
	case DIRMODI:
		(NULL == CompareResult.stCRHeads.lpCRDirModified) ? (CompareResult.stCRHeads.lpCRDirModified = lpCR) : (CompareResult.stCRCurrent.lpCRDirModified->lpNextCR = lpCR);
		CompareResult.stCRCurrent.lpCRDirModified = lpCR;
		break;
	case FILEDEL:
		(NULL == CompareResult.stCRHeads.lpCRFileDeleted) ? (CompareResult.stCRHeads.lpCRFileDeleted = lpCR) : (CompareResult.stCRCurrent.lpCRFileDeleted->lpNextCR = lpCR);
		CompareResult.stCRCurrent.lpCRFileDeleted = lpCR;
		break;
	case FILEADD:
		(NULL == CompareResult.stCRHeads.lpCRFileAdded) ? (CompareResult.stCRHeads.lpCRFileAdded = lpCR) : (CompareResult.stCRCurrent.lpCRFileAdded->lpNextCR = lpCR);
		CompareResult.stCRCurrent.lpCRFileAdded = lpCR;
		break;
	case FILEMODI:
		(NULL == CompareResult.stCRHeads.lpCRFileModified) ? (CompareResult.stCRHeads.lpCRFileModified = lpCR) : (CompareResult.stCRCurrent.lpCRFileModified->lpNextCR = lpCR);
		CompareResult.stCRCurrent.lpCRFileModified = lpCR;
		break;
	}
}

//-------------------------------------------------------------
// Routine to free all comparison results (release memory)
//-------------------------------------------------------------
VOID FreeAllCompResults(LPCOMPRESULT lpCR) 
{
	LPCOMPRESULT lpNextCR;

	for (; NULL != lpCR; lpCR = lpNextCR) 
	{
		// Save pointer in local variable
		lpNextCR = lpCR->lpNextCR;

		// Free compare result itself
		MYFREE(lpCR);
	}
}

// ----------------------------------------------------------------------
// Free all compare results
// ----------------------------------------------------------------------
VOID FreeCompareResult(void) 
{
	FreeAllCompResults(CompareResult.stCRHeads.lpCRKeyDeleted);
	FreeAllCompResults(CompareResult.stCRHeads.lpCRKeyAdded);
	FreeAllCompResults(CompareResult.stCRHeads.lpCRValDeleted);
	FreeAllCompResults(CompareResult.stCRHeads.lpCRValAdded);
	FreeAllCompResults(CompareResult.stCRHeads.lpCRValModified);
	FreeAllCompResults(CompareResult.stCRHeads.lpCRDirDeleted);
	FreeAllCompResults(CompareResult.stCRHeads.lpCRDirAdded);
	FreeAllCompResults(CompareResult.stCRHeads.lpCRDirModified);
	FreeAllCompResults(CompareResult.stCRHeads.lpCRFileDeleted);
	FreeAllCompResults(CompareResult.stCRHeads.lpCRFileAdded);
	FreeAllCompResults(CompareResult.stCRHeads.lpCRFileModified);
	// Also free the memory
	ZeroMemory(&CompareResult, sizeof(CompareResult));
}

//-------------------------------------------------------------
// Registry comparison engine
//-------------------------------------------------------------
VOID CompareRegKeys(LPKEYCONTENT lpStartKC1, LPKEYCONTENT lpStartKC2) 
{
	LPKEYCONTENT lpKC1;
	LPKEYCONTENT lpKC2;

	// Compare keys
	for (lpKC1 = lpStartKC1; NULL != lpKC1; lpKC1 = lpKC1->lpBrotherKC) 
	{
		CompareResult.stcCompared.cKeys++;
		// Find a matching key for KC1
		for (lpKC2 = lpStartKC2; NULL != lpKC2; lpKC2 = lpKC2->lpBrotherKC) 
		{
			// skip KC2 if already matched
			if (NOMATCH != lpKC2->fKeyMatch) {
				continue;
			}
			// skip KC2 if names do *not* match (ATTENTION: test for match, THEN negate)
			if (!(
				(lpKC1->lpszKeyName == lpKC2->lpszKeyName)
				|| ((NULL != lpKC1->lpszKeyName) && (NULL != lpKC2->lpszKeyName) && (0 == _tcscmp(lpKC1->lpszKeyName, lpKC2->lpszKeyName)))  // TODO: case-insensitive compare?
				)) {
				continue;
			}

			// Same key name of KC1 found in KC2! Mark KC2 as matched to skip it for the next KC1, then compare their values and sub keys!
			lpKC2->fKeyMatch = ISMATCH;

			// Extra local block to reduce stack usage due to recursive calls
			{
				LPVALUECONTENT lpVC1;
				LPVALUECONTENT lpVC2;

				// Compare values
				for (lpVC1 = lpKC1->lpFirstVC; NULL != lpVC1; lpVC1 = lpVC1->lpBrotherVC) 
				{
					CompareResult.stcCompared.cValues++;
					// Find a matching value for VC1
					for (lpVC2 = lpKC2->lpFirstVC; NULL != lpVC2; lpVC2 = lpVC2->lpBrotherVC) 
					{
						// skip VC2 if already matched
						if (NOMATCH != lpVC2->fValueMatch) {
							continue;
						}
						// skip VC2 if types do *not* match (even if same name then interpret as deleted+added)
						if (lpVC1->nTypeCode != lpVC2->nTypeCode) {
							continue;
						}
						// skip VC2 if names do *not* match (ATTENTION: test for match, THEN negate)
						if (!(
							(lpVC1->lpszValueName == lpVC2->lpszValueName)
							|| ((NULL != lpVC1->lpszValueName) && (NULL != lpVC2->lpszValueName) && (0 == _tcscmp(lpVC1->lpszValueName, lpVC2->lpszValueName)))  // TODO: case-insensitive compare?
							)) {
							continue;
						}

						// Same value type and name of VC1 found in VC2, so compare their size and data
						if ((lpVC1->cbData == lpVC2->cbData)
							&& ((lpVC1->lpValueData == lpVC2->lpValueData)
							|| ((NULL != lpVC1->lpValueData) && (NULL != lpVC2->lpValueData) && (0 == memcmp(lpVC1->lpValueData, lpVC2->lpValueData, lpVC1->cbData)))
							)) {
							// Same value data of VC1 found in VC2
							lpVC2->fValueMatch = ISMATCH;
						}
						else {
							// Value data differ, so value is modified
							lpVC2->fValueMatch = ISMODI;
							CompareResult.stcChanged.cValues++;
							CompareResult.stcModified.cValues++;
							CreateNewResult(VALMODI, lpVC1, lpVC2);
						}
						break;
					}
					if (NULL == lpVC2) 
					{
						// VC1 has no match in KC2, so VC1 is a deleted value
						CompareResult.stcChanged.cValues++;
						CompareResult.stcDeleted.cValues++;
						CreateNewResult(VALDEL, lpVC1, NULL);
					}
				}
				// After looping all values of KC1, do an extra loop over all KC2 values and check previously set match flags to determine added values
				for (lpVC2 = lpKC2->lpFirstVC; NULL != lpVC2; lpVC2 = lpVC2->lpBrotherVC) 
				{
					// skip VC2 if already matched
					if (NOMATCH != lpVC2->fValueMatch) {
						continue;
					}

					CompareResult.stcCompared.cValues++;

					// VC2 has no match in KC1, so VC2 is an added value
					CompareResult.stcChanged.cValues++;
					CompareResult.stcAdded.cValues++;
					CreateNewResult(VALADD, NULL, lpVC2);
				}
			}  // End of extra local block

			// Compare sub keys if any
			if ((NULL != lpKC1->lpFirstSubKC) || (NULL != lpKC2->lpFirstSubKC)) {
				CompareRegKeys(lpKC1->lpFirstSubKC, lpKC2->lpFirstSubKC);
			}
			break;
		}
		if (NULL == lpKC2) {
			// KC1 has no matching KC2, so KC1 is a deleted key
			CompareResult.stcChanged.cKeys++;
			CompareResult.stcDeleted.cKeys++;
			CreateNewResult(KEYDEL, lpKC1, NULL);
			// Extra local block to reduce stack usage due to recursive calls
			{
				LPVALUECONTENT lpVC1;

				for (lpVC1 = lpKC1->lpFirstVC; NULL != lpVC1; lpVC1 = lpVC1->lpBrotherVC) 
				{
					CompareResult.stcCompared.cValues++;
					CompareResult.stcChanged.cValues++;
					CompareResult.stcDeleted.cValues++;
					CreateNewResult(VALDEL, lpVC1, NULL);
				}
			}  // End of extra local block

			// "Compare"/Log sub keys if any
			if (NULL != lpKC1->lpFirstSubKC) {
				CompareRegKeys(lpKC1->lpFirstSubKC, NULL);
			}
		}
	}
	// After looping all KC1 keys, do an extra loop over all KC2 keys and check previously set match flags to determine added keys
	for (lpKC2 = lpStartKC2; NULL != lpKC2; lpKC2 = lpKC2->lpBrotherKC) 
	{
		// skip KC2 if already matched
		if (NOMATCH != lpKC2->fKeyMatch) {
			continue;
		}

		// KC2 has no matching KC1, so KC2 is an added key
		CompareResult.stcCompared.cKeys++;
		CompareResult.stcChanged.cKeys++;
		CompareResult.stcAdded.cKeys++;
		CreateNewResult(KEYADD, NULL, lpKC2);
		// Extra local block to reduce stack usage due to recursive calls
		{
			LPVALUECONTENT lpVC2;

			for (lpVC2 = lpKC2->lpFirstVC; NULL != lpVC2; lpVC2 = lpVC2->lpBrotherVC) 
			{
				CompareResult.stcCompared.cValues++;
				CompareResult.stcChanged.cValues++;
				CompareResult.stcAdded.cValues++;
				CreateNewResult(VALADD, NULL, lpVC2);
			}
		}  // End of extra local block

		// "Compare"/Log sub keys if any
		if (NULL != lpKC2->lpFirstSubKC) {
			CompareRegKeys(NULL, lpKC2->lpFirstSubKC);
		}
	}
}

//------------------------------------------------------------
// Routine to call registry/file comparison engine
//------------------------------------------------------------
VOID CompareShots(VOID) 
{
	if (!DirChainMatch(Shot1.lpHF, Shot2.lpHF)) {
		//MessageBox(hWnd, TEXT("Found two shots with different DIR chain! (or with different order)\r\nYou can continue, but file comparison result would be abnormal!"), asLangTexts[iszTextWarning].lpszText, MB_ICONWARNING);  //TODO: I18N, create text index and translate
	}

	// Initialize result and markers
	FreeCompareResult();
	CompareResult.lpShot1 = &Shot1;
	CompareResult.lpShot2 = &Shot2;
	ClearRegKeyMatchFlags(Shot1.lpHKLM);
	ClearRegKeyMatchFlags(Shot2.lpHKLM);
	ClearRegKeyMatchFlags(Shot1.lpHKU);
	ClearRegKeyMatchFlags(Shot2.lpHKU);
	ClearHeadFileMatchFlags(Shot1.lpHF);
	ClearHeadFileMatchFlags(Shot2.lpHF);

	// Compare HKLM
	if ((NULL != CompareResult.lpShot1->lpHKLM) || (NULL != CompareResult.lpShot2->lpHKLM)) {
		CompareRegKeys(CompareResult.lpShot1->lpHKLM, CompareResult.lpShot2->lpHKLM);
	}
	// Compare HKU
	if ((NULL != CompareResult.lpShot1->lpHKU) || (NULL != CompareResult.lpShot2->lpHKU)) {
		CompareRegKeys(CompareResult.lpShot1->lpHKU, CompareResult.lpShot2->lpHKU);
	}
	// Compare HEADFILEs v1.8.1
	if ((NULL != CompareResult.lpShot1->lpHF) || (NULL != CompareResult.lpShot2->lpHF)) {
		CompareHeadFiles(CompareResult.lpShot1->lpHF, CompareResult.lpShot2->lpHF);
	}

	// Get total count of all items
	CompareResult.stcCompared.cAll = CompareResult.stcCompared.cKeys
		+ CompareResult.stcCompared.cValues
		+ CompareResult.stcCompared.cDirs
		+ CompareResult.stcCompared.cFiles;
	CompareResult.stcChanged.cAll = CompareResult.stcChanged.cKeys
		+ CompareResult.stcChanged.cValues
		+ CompareResult.stcChanged.cDirs
		+ CompareResult.stcChanged.cFiles;
	CompareResult.stcDeleted.cAll = CompareResult.stcDeleted.cKeys
		+ CompareResult.stcDeleted.cValues
		+ CompareResult.stcDeleted.cDirs
		+ CompareResult.stcDeleted.cFiles;
	CompareResult.stcAdded.cAll = CompareResult.stcAdded.cKeys
		+ CompareResult.stcAdded.cValues
		+ CompareResult.stcAdded.cDirs
		+ CompareResult.stcAdded.cFiles;
	CompareResult.stcModified.cAll = CompareResult.stcModified.cKeys
		+ CompareResult.stcModified.cValues
		+ CompareResult.stcModified.cDirs
		+ CompareResult.stcModified.cFiles;
	CompareResult.fFilled = TRUE;
}

// ----------------------------------------------------------------------
// Clear comparison match flags of registry keys and values
// ----------------------------------------------------------------------
VOID ClearRegKeyMatchFlags(LPKEYCONTENT lpKC) 
{
	LPVALUECONTENT lpVC;

	for (; NULL != lpKC; lpKC = lpKC->lpBrotherKC) {
		lpKC->fKeyMatch = NOMATCH;
		for (lpVC = lpKC->lpFirstVC; NULL != lpVC; lpVC = lpVC->lpBrotherVC) {
			lpVC->fValueMatch = NOMATCH;
		}
		ClearRegKeyMatchFlags(lpKC->lpFirstSubKC);
	}
}

// ----------------------------------------------------------------------
// Free all registry values
// ----------------------------------------------------------------------
VOID FreeAllValueContents(LPVALUECONTENT lpVC) 
{
	LPVALUECONTENT lpBrotherVC;

	for (; NULL != lpVC; lpVC = lpBrotherVC) 
	{
		// Save pointer in local variable
		lpBrotherVC = lpVC->lpBrotherVC;

		// Free value name
		if (NULL != lpVC->lpszValueName) {
			MYFREE(lpVC->lpszValueName);
		}

		// Free value data
		if (NULL != lpVC->lpValueData) {
			MYFREE(lpVC->lpValueData);
		}

		// Free entry itself
		MYFREE(lpVC);
	}
}

// ----------------------------------------------------------------------
// Free all registry keys and values
// ----------------------------------------------------------------------
VOID FreeAllKeyContents(LPKEYCONTENT lpKC) 
{
	LPKEYCONTENT lpBrotherKC;

	for (; NULL != lpKC; lpKC = lpBrotherKC) 
	{
		// Save pointer in local variable
		lpBrotherKC = lpKC->lpBrotherKC;

		// Free key name
		if (NULL != lpKC->lpszKeyName) {
			if ((NULL != lpKC->lpFatherKC)  // only the top KC can have HKLM/HKU, so ignore the sub KCs
				|| ((lpKC->lpszKeyName != lpszHKLMShort)
				&& (lpKC->lpszKeyName != lpszHKLMLong)
				&& (lpKC->lpszKeyName != lpszHKUShort)
				&& (lpKC->lpszKeyName != lpszHKULong))) 
			{
				MYFREE(lpKC->lpszKeyName);
			}
		}

		// If the entry has values, then do a call for the first value
		if (NULL != lpKC->lpFirstVC) {
			FreeAllValueContents(lpKC->lpFirstVC);
		}

		// If the entry has childs, then do a recursive call for the first child
		if (NULL != lpKC->lpFirstSubKC) {
			FreeAllKeyContents(lpKC->lpFirstSubKC);
		}

		// Free entry itself
		MYFREE(lpKC);
	}
}

// ----------------------------------------------------------------------
// Free shot completely and initialize
// ----------------------------------------------------------------------
VOID FreeShot(LPSNAPSHOT lpShot) 
{
	if (NULL != lpShot->lpszComputerName) {
		MYFREE(lpShot->lpszComputerName);
	}
	if (NULL != lpShot->lpszUserName) {
		MYFREE(lpShot->lpszUserName);
	}
	if (NULL != lpShot->lpHKLM) {
		FreeAllKeyContents(lpShot->lpHKLM);
	}
	if (NULL != lpShot->lpHKU) {
		FreeAllKeyContents(lpShot->lpHKU);
	}
	if (NULL != lpShot->lpHF) {
		FreeAllHeadFiles(lpShot->lpHF);
	}
	ZeroMemory(lpShot, sizeof(SNAPSHOT));
}

// ----------------------------------------------------------------------
// Get registry snap shot
// ----------------------------------------------------------------------
LPKEYCONTENT GetRegistrySnap(LPSNAPSHOT lpShot, HKEY hRegKey, LPTSTR lpszRegKeyName, LPKEYCONTENT lpFatherKC, LPKEYCONTENT *lplpCaller) 
{
	LPKEYCONTENT lpKC;
	DWORD cSubKeys;
	DWORD cchMaxSubKeyName;

	// Process registry key itself, then key values with data, 
	// then sub keys (see msdn.microsoft.com/en-us/library/windows/desktop/ms724256.aspx)

	// Extra local block to reduce stack usage due to recursive calls
	{
		//LPTSTR lpszFullName;
		DWORD cValues;
		DWORD cchMaxValueName;
		DWORD cbMaxValueData;

		// Create new key content
		// put in a separate var for later use
		lpKC = MYALLOC0(sizeof(KEYCONTENT));

		// Set father of current key
		lpKC->lpFatherKC = lpFatherKC;

		// Set key name
		lpKC->lpszKeyName = lpszRegKeyName;
		lpKC->cchKeyName = _tcslen(lpKC->lpszKeyName);

		// Check if key is to be excluded
		// Removed this code, because I am not excluding Registry keys, only values!	

		// Examine key for values and sub keys, get counts and also maximum lengths of names plus value data
		nErrNo = RegQueryInfoKey(
			hRegKey,
			NULL,
			NULL,
			NULL,
			&cSubKeys,
			&cchMaxSubKeyName,  // in TCHARs *not* incl. NULL char
			NULL,
			&cValues,
			&cchMaxValueName,   // in TCHARs *not* incl. NULL char
			&cbMaxValueData,
			NULL,
			NULL
			);
		if (ERROR_SUCCESS != nErrNo) {
			// TODO: process/protocol issue in some way, do not silently ignore it (at least in Debug builds)
			FreeAllKeyContents(lpKC);
			return NULL;
		}

		// Copy pointer to current key into caller's pointer
		if (NULL != lplpCaller) {
			*lplpCaller = lpKC;
		}

		// Increase key count
		lpShot->stCounts.cKeys++;

		// Copy the registry values of the current key
		if (0 < cValues) {
			LPVALUECONTENT lpVC;
			LPVALUECONTENT *lplpVCPrev;
			DWORD i;
			DWORD cchValueName;
			DWORD nValueType;
			DWORD cbValueData;

			// Account for NULL char
			if (0 < cchMaxValueName) {
				cchMaxValueName++;
			}

			// Get buffer for maximum value name length
			nSourceSize = cchMaxValueName * sizeof(TCHAR);
			nStringBufferSize = AdjustBuffer(&lpStringBuffer, nStringBufferSize, nSourceSize, REGSHOT_BUFFER_BLOCK_BYTES);

			// Get buffer for maximum value data length
			nSourceSize = cbMaxValueData;
			nDataBufferSize = AdjustBuffer(&lpDataBuffer, nDataBufferSize, nSourceSize, REGSHOT_BUFFER_BLOCK_BYTES);

			// Get registry key values
			lplpVCPrev = &lpKC->lpFirstVC;
			for (i = 0;; i++) 
			{
				// Enumerate value
				cchValueName = (DWORD)nStringBufferSize;
				cbValueData = (DWORD)nDataBufferSize;
				nErrNo = RegEnumValue(hRegKey, i,
					lpStringBuffer,
					&cchValueName,   // in TCHARs; in *including* and out *excluding* NULL char
					NULL,
					&nValueType,
					lpDataBuffer,
					&cbValueData);
				if (ERROR_NO_MORE_ITEMS == nErrNo) {
					break;
				}
				if (ERROR_SUCCESS != nErrNo) {
					// TODO: process/protocol issue in some way, do not silently ignore it (at least in Debug builds)
					continue;
				}
				lpStringBuffer[cchValueName] = (TCHAR)'\0';  // safety NULL char

				// Create new value content
				// put in a separate var for later use
				lpVC = MYALLOC0(sizeof(VALUECONTENT));

				// Set father key of current value to current key
				lpVC->lpFatherKC = lpKC;

				// Copy value name
				if (0 < cchValueName) 
				{
					lpVC->lpszValueName = MYALLOC((cchValueName + 1) * sizeof(TCHAR));
					_tcscpy(lpVC->lpszValueName, lpStringBuffer);
					lpVC->cchValueName = _tcslen(lpVC->lpszValueName);
				}

				// If the Registry value name is NULL, make it "(Default)"
				// This inline with MSDN naming conventions and will ease later processing
				if (lpVC->lpszValueName == NULL) {
					MYFREE(lpVC->lpszValueName);
					LPTSTR lpszDefaultValueName = TEXT("(Default)\0");
					lpVC->cchValueName = _tcslen(lpszDefaultValueName);
					lpVC->lpszValueName = MYALLOC(lpVC->cchValueName * sizeof(TCHAR));
					_tcscpy(lpVC->lpszValueName, lpszDefaultValueName);
				}

				// Blacklisting implementation for Registry values		
				if (dwBlacklist == 1)
				{
					// First snapshot, therefore populate the Trie (Prefix Tree)
					LPTSTR lpszFullPath;
					lpszFullPath = GetWholeValueName(lpVC, FALSE);
					if (_tcscmp(lpszCurrentHive, lpszHKLMShort) == 0) {
						TrieAdd(&blacklistHKLM, lpszFullPath);
					}
					else if (_tcscmp(lpszCurrentHive, lpszHKUShort) == 0) {
						TrieAdd(&blacklistHKU, lpszFullPath);
					}
					MYFREE(lpszFullPath);
				}
				else if (dwBlacklist == 2)
				{
					// Not the first snapshot, so filter known Registry value paths
					BOOL found = FALSE;
					LPTSTR lpszFullPath;
					lpszFullPath = GetWholeValueName(lpVC, FALSE);
					if (_tcscmp(lpszCurrentHive, lpszHKLMShort) == 0) {
						found = TrieSearch1(blacklistHKLM->children, lpszFullPath);
					}
					else if (_tcscmp(lpszCurrentHive, lpszHKUShort) == 0) {
						found = TrieSearch1(blacklistHKU->children, lpszFullPath);
					}
					if (found) {
						lpShot->stCounts.cValuesBlacklist++;
						MYFREE(lpszFullPath);
						FreeAllValueContents(lpVC);
						continue;  // ignore this entry and continue with next brother value
					}
					else {
						MYFREE(lpszFullPath);
					}
				}

				// Copy pointer to current value into previous value's next value pointer
				if (NULL != lplpVCPrev) {
					*lplpVCPrev = lpVC;
				}

				// Increase value count
				lpShot->stCounts.cValues++;

				// Copy value meta data
				lpVC->nTypeCode = nValueType;
				lpVC->cbData = cbValueData;

				// Copy value data
				if (0 < cbValueData) {  // otherwise leave it NULL
					lpVC->lpValueData = MYALLOC(cbValueData);
					CopyMemory(lpVC->lpValueData, lpDataBuffer, cbValueData);
				}

				// Set "lpBrotherVC" pointer for storing the next brother's pointer
				lplpVCPrev = &lpVC->lpBrotherVC;
			}
		}
	}  // End of extra local block

	// Print Registry shot status
	if (dwBlacklist == 2) {
		printf("  > Keys: %d  Values: %d  Blacklisted Values: %d\r", lpShot->stCounts.cKeys, lpShot->stCounts.cValues, lpShot->stCounts.cValuesBlacklist);
	}
	else {
		printf("  > Keys: %d  Values: %d\r", lpShot->stCounts.cKeys, lpShot->stCounts.cValues);
	}

	// Process sub keys
	if (0 < cSubKeys) {
		LPKEYCONTENT lpKCSub;
		LPKEYCONTENT *lplpKCPrev;
		DWORD i;
		LPTSTR lpszRegSubKeyName;
		HKEY hRegSubKey;

		// Account for NULL char
		if (0 < cchMaxSubKeyName) {
			cchMaxSubKeyName++;
		}

		// Get buffer for maximum sub key name length
		nSourceSize = cchMaxSubKeyName * sizeof(TCHAR);
		nStringBufferSize = AdjustBuffer(&lpStringBuffer, nStringBufferSize, nSourceSize, REGSHOT_BUFFER_BLOCK_BYTES);

		// Get registry sub keys
		lplpKCPrev = &lpKC->lpFirstSubKC;
		for (i = 0;; i++) {
			// Extra local block to reduce stack usage due to recursive calls
				{
					DWORD cchSubKeyName;

					// Enumerate sub key
					cchSubKeyName = (DWORD)nStringBufferSize;
					nErrNo = RegEnumKeyEx(hRegKey, i,
						lpStringBuffer,
						&cchSubKeyName,  // in TCHARs; in *including* and out *excluding* NULL char
						NULL,
						NULL,
						NULL,
						NULL);
					if (ERROR_NO_MORE_ITEMS == nErrNo) {
						break;
					}
					if (ERROR_SUCCESS != nErrNo) {
						// TODO: process/protocol issue in some way, do not silently ignore it (at least in Debug builds)
						continue;
					}
					lpStringBuffer[cchSubKeyName] = (TCHAR)'\0';  // safety NULL char

					// Copy sub key name
					lpszRegSubKeyName = NULL;
					if (0 < cchSubKeyName) {
						lpszRegSubKeyName = MYALLOC((cchSubKeyName + 1) * sizeof(TCHAR));
						_tcscpy(lpszRegSubKeyName, lpStringBuffer);
					}
				}  // End of extra local block

			nErrNo = RegOpenKeyEx(hRegKey, lpszRegSubKeyName, 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hRegSubKey);
			if (ERROR_SUCCESS != nErrNo) {
				// TODO: process/protocol issue in some way, do not silently ignore it (at least in Debug builds)
				//       e.g. when ERROR_ACCESS_DENIED then at least add key itself to the list
				MYFREE(lpszRegSubKeyName);
				continue;
			}

			lpKCSub = GetRegistrySnap(lpShot, hRegSubKey, lpszRegSubKeyName, lpKC, lplpKCPrev);
			RegCloseKey(hRegSubKey);

			// Set "lpBrotherKC" pointer for storing the next brother's pointer
			if (NULL != lpKCSub) {
				lplpKCPrev = &lpKCSub->lpBrotherKC;
			}
		}
	}
	return lpKC;
}

// ----------------------------------------------------------------------
// Shot Engine
// ----------------------------------------------------------------------
VOID RegShot(LPSNAPSHOT lpShot) 
{
	//DWORD cchString;

	// Clear shot
	FreeShot(lpShot);

	// New temporary buffers
	lpStringBuffer = NULL;
	lpDataBuffer = NULL;

	lpszCurrentHive = MYALLOC0(MAX_PATH * sizeof(TCHAR));
	_tcscpy(lpszCurrentHive, lpszHKLMShort);

	//wprintf(L"%s\n", lpszCurrentHive);

	// Take HKLM registry shot
	GetRegistrySnap(lpShot, HKEY_LOCAL_MACHINE, lpszHKLMShort, NULL, &lpShot->lpHKLM);

	MYFREE(lpszCurrentHive);
	lpszCurrentHive = MYALLOC0(MAX_PATH * sizeof(TCHAR));
	_tcscpy(lpszCurrentHive, lpszHKUShort);

	//wprintf(L"%s\n", lpszCurrentHive);

	// Take HKU registry shot
	GetRegistrySnap(lpShot, HKEY_USERS, lpszHKUShort, NULL, &lpShot->lpHKU);

	// Print final Registry shot count
	printf("  > Keys:  %d  Values: %d\n", lpShot->stCounts.cKeys, lpShot->stCounts.cValues);

	// Get total count of all items
	lpShot->stCounts.cAll = lpShot->stCounts.cKeys + lpShot->stCounts.cValues;

	// Set flags
	lpShot->fFilled = TRUE;
	lpShot->fLoaded = FALSE;

	if (NULL != lpStringBuffer) {
		MYFREE(lpStringBuffer);
		lpStringBuffer = NULL;
	}
	if (NULL != lpDataBuffer) {
		MYFREE(lpDataBuffer);
		lpDataBuffer = NULL;
	}
}

// ----------------------------------------------------------------------
// Save registry key with values to HIVE file
//
// This routine is called recursively to store the keys of the Registry tree
// Therefore temporary vars are put in a local block to reduce stack usage
// ----------------------------------------------------------------------
VOID SaveRegKeys(LPSNAPSHOT lpShot, LPKEYCONTENT lpKC, DWORD nFPFatherKey, DWORD nFPCaller) 
{
	DWORD nFPKey;

	for (; NULL != lpKC; lpKC = lpKC->lpBrotherKC) {
		// Get current file position
		// put in a separate var for later use
		nFPKey = SetFilePointer(hFileWholeReg, 0, NULL, FILE_CURRENT);

		// Write position of current reg key in caller's field
		if (0 < nFPCaller) {
			SetFilePointer(hFileWholeReg, nFPCaller, NULL, FILE_BEGIN);
			WriteFile(hFileWholeReg, &nFPKey, sizeof(nFPKey), &NBW, NULL);

			SetFilePointer(hFileWholeReg, nFPKey, NULL, FILE_BEGIN);
		}

		// Initialize key content

		// Set file positions of the relatives inside the tree
		sKC.ofsKeyName = 0;      // not known yet, may be defined in this call
		// Key name will always be stored behind the structure, so its position is already known
		sKC.ofsKeyName = nFPKey + sizeof(sKC);

		sKC.ofsFirstValue = 0;   // not known yet, may be re-written in this iteration
		sKC.ofsFirstSubKey = 0;  // not known yet, may be re-written by another recursive call
		sKC.ofsBrotherKey = 0;   // not known yet, may be re-written in another iteration
		sKC.ofsFatherKey = nFPFatherKey;

		// New since key content version 2
		sKC.nKeyNameLen = 0;

		// Extra local block to reduce stack usage due to recursive calls
		{
			LPTSTR lpszKeyName;

			lpszKeyName = lpKC->lpszKeyName;

			// Determine correct key name and length plus file offset
			if (NULL != lpszKeyName) {
				sKC.nKeyNameLen = (DWORD)lpKC->cchKeyName;

				if ((fUseLongRegHead) && (0 == nFPFatherKey)) {
					// Adopt to long HKLM/HKU
					if (lpszHKLMShort == lpszKeyName) {
						lpszKeyName = lpszHKLMLong;
						sKC.nKeyNameLen = (DWORD)_tcslen(lpszKeyName);;
					}
					else if (lpszHKUShort == lpszKeyName) {
						lpszKeyName = lpszHKULong;
						sKC.nKeyNameLen = (DWORD)_tcslen(lpszKeyName);;
					}
				}

				// Determine key name length and file offset
				if (0 < sKC.nKeyNameLen) {  // otherwise leave it all 0
					sKC.nKeyNameLen++;  // account for NULL char

					// Key name will always be stored behind the structure, so its position is already known
					sKC.ofsKeyName = nFPKey + sizeof(sKC);
				}
			}

			// Write key content to file
			// Make sure that ALL fields have been initialized/set
			WriteFile(hFileWholeReg, &sKC, sizeof(sKC), &NBW, NULL);

			// Write key name to file
			if (0 < sKC.nKeyNameLen) {
				WriteFile(hFileWholeReg, lpszKeyName, sKC.nKeyNameLen * sizeof(TCHAR), &NBW, NULL);
			}
			else {
				// Write empty string for backward compatibility
				WriteFile(hFileWholeReg, lpszEmpty, 1 * sizeof(TCHAR), &NBW, NULL);
			}
		}  // End of extra local block

		// Save the values of the current key
		if (NULL != lpKC->lpFirstVC) {
			LPVALUECONTENT lpVC;
			DWORD nFPValue;

			// Write all values of key
			nFPCaller = nFPKey + offsetof(SAVEKEYCONTENT, ofsFirstValue);  // Write position of first value into key
			for (lpVC = lpKC->lpFirstVC; NULL != lpVC; lpVC = lpVC->lpBrotherVC) {
				// Get current file position
				// put in a separate var for later use
				nFPValue = SetFilePointer(hFileWholeReg, 0, NULL, FILE_CURRENT);

				// Write position of current reg value in caller's field
				if (0 < nFPCaller) {
					SetFilePointer(hFileWholeReg, nFPCaller, NULL, FILE_BEGIN);
					WriteFile(hFileWholeReg, &nFPValue, sizeof(nFPValue), &NBW, NULL);

					SetFilePointer(hFileWholeReg, nFPValue, NULL, FILE_BEGIN);
				}

				// Initialize value content

				// Copy values
				sVC.nTypeCode = lpVC->nTypeCode;
				sVC.cbData = lpVC->cbData;

				// Set file positions of the relatives inside the tree
				sVC.ofsValueName = 0;       // not known yet, may be defined in this iteration
				
				// Value name will always be stored behind the structure, so its position is already known
				sVC.ofsValueName = nFPValue + sizeof(sVC);

				sVC.ofsValueData = 0;       // not known yet, may be re-written in this iteration
				sVC.ofsBrotherValue = 0;    // not known yet, may be re-written in next iteration
				sVC.ofsFatherKey = nFPKey;

				// New since value content version 2
				sVC.nValueNameLen = 0;

				// Determine value name length
				if (NULL != lpVC->lpszValueName) {
					sVC.nValueNameLen = (DWORD)lpVC->cchValueName;
					if (0 < sVC.nValueNameLen) {  // otherwise leave it all 0
						sVC.nValueNameLen++;  // account for NULL char
						// Value name will always be stored behind the structure, so its position is already known
						sVC.ofsValueName = nFPValue + sizeof(sVC);
					}
				}

				// Write value content to file
				// Make sure that ALL fields have been initialized/set
				WriteFile(hFileWholeReg, &sVC, sizeof(sVC), &NBW, NULL);

				// Write value name to file
				if (0 < sVC.nValueNameLen) {
					WriteFile(hFileWholeReg, lpVC->lpszValueName, sVC.nValueNameLen * sizeof(TCHAR), &NBW, NULL);
				}
				else {
					// Write empty string for backward compatibility
					WriteFile(hFileWholeReg, lpszEmpty, 1 * sizeof(TCHAR), &NBW, NULL);
				}

				// Write value data to file
				if (0 < sVC.cbData) {
					DWORD nFPValueData;

					// Write position of value data in value content field ofsValueData
					nFPValueData = SetFilePointer(hFileWholeReg, 0, NULL, FILE_CURRENT);

					SetFilePointer(hFileWholeReg, nFPValue + offsetof(SAVEVALUECONTENT, ofsValueData), NULL, FILE_BEGIN);
					WriteFile(hFileWholeReg, &nFPValueData, sizeof(nFPValueData), &NBW, NULL);

					SetFilePointer(hFileWholeReg, nFPValueData, NULL, FILE_BEGIN);

					// Write value data
					WriteFile(hFileWholeReg, lpVC->lpValueData, sVC.cbData, &NBW, NULL);
				}

				// Set "ofsBrotherValue" position for storing the following brother's position
				nFPCaller = nFPValue + offsetof(SAVEVALUECONTENT, ofsBrotherValue);
			}
		}

		// ATTENTION!!! sKC will be INVALID from this point on, due to recursive calls
		// If the entry has childs, then do a recursive call for the first child
		// Pass this entry as father and "ofsFirstSubKey" position for storing the first child's position
		if (NULL != lpKC->lpFirstSubKC) {
			SaveRegKeys(lpShot, lpKC->lpFirstSubKC, nFPKey, nFPKey + offsetof(SAVEKEYCONTENT, ofsFirstSubKey));
		}
		// Set "ofsBrotherKey" position for storing the following brother's position
		nFPCaller = nFPKey + offsetof(SAVEKEYCONTENT, ofsBrotherKey);
	}
}

// ----------------------------------------------------------------------
// Save registry and files to HIVE file
// ----------------------------------------------------------------------
VOID SaveShot(LPSNAPSHOT lpShot, LPTSTR lpszFileName) 
{
	TCHAR filepath[MAX_PATH];
	DWORD nFPCurrent;

	// Check if there's anything to save
	if ((NULL == lpShot->lpHKLM) && (NULL == lpShot->lpHKU) && (NULL == lpShot->lpHF)) {
		return;  // leave silently
	}

	// Clear Save File Name result buffer
	ZeroMemory(filepath, sizeof(filepath));

	// Open file for writing
	hFileWholeReg = CreateFile(lpszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hFileWholeReg) {
//		ErrMsg(asLangTexts[iszTextErrorCreateFile].lpszText);
		return;
	}

	// Initialize file header
	ZeroMemory(&fileheader, sizeof(fileheader));

	// Copy SBCS/MBCS signature to header (even in Unicode builds for backwards compatibility)
	strncpy(fileheader.signature, szLiveDiffFileSignature, MAX_SIGNATURE_LENGTH);

	// Set file positions of hives inside the file
	fileheader.ofsHKLM = 0;   // not known yet, may be empty
	fileheader.ofsHKU = 0;    // not known yet, may be empty
	fileheader.ofsHF = 0;  // not known yet, may be empty

	// Copy SBCS/MBCS strings to header (even in Unicode builds for backwards compatibility)
	if (NULL != lpShot->lpszComputerName) {
		WideCharToMultiByte(CP_ACP, 
			WC_COMPOSITECHECK | WC_DEFAULTCHAR, 
			lpShot->lpszComputerName, 
			-1, 
			fileheader.computername, 
			OLD_COMPUTERNAMELEN, 
			NULL, 
			NULL);
	}
	if (NULL != lpShot->lpszUserName) {
		WideCharToMultiByte(CP_ACP, 
			WC_COMPOSITECHECK | WC_DEFAULTCHAR, 
			lpShot->lpszUserName, 
			-1, 
			fileheader.username, 
			OLD_COMPUTERNAMELEN,
			NULL, 
			NULL);
	}

	// Copy system time to header
	CopyMemory(&fileheader.systemtime, &lpShot->systemtime, sizeof(SYSTEMTIME));

	// new since header version 2
	fileheader.nFHSize = sizeof(fileheader);

	fileheader.nFHVersion = FILEHEADER_VERSION_CURRENT;
	fileheader.nCharSize = sizeof(TCHAR);

	fileheader.ofsComputerName = 0;  // not known yet, may be empty
	fileheader.nComputerNameLen = 0;
	if (NULL != lpShot->lpszComputerName) {
		fileheader.nComputerNameLen = (DWORD)_tcslen(lpShot->lpszComputerName);
		if (0 < fileheader.nComputerNameLen) {
			fileheader.nComputerNameLen++;  // account for NULL char
		}
	}

	fileheader.ofsUserName = 0;      // not known yet, may be empty
	fileheader.nUserNameLen = 0;
	if (NULL != lpShot->lpszUserName) {
		fileheader.nUserNameLen = (DWORD)_tcslen(lpShot->lpszUserName);
		if (0 < fileheader.nUserNameLen) {
			fileheader.nUserNameLen++;  // account for NULL char
		}
	}

	fileheader.nKCVersion = KEYCONTENT_VERSION_CURRENT;
	fileheader.nKCSize = sizeof(SAVEKEYCONTENT);

	fileheader.nVCVersion = VALUECONTENT_VERSION_CURRENT;
	fileheader.nVCSize = sizeof(SAVEVALUECONTENT);

	fileheader.nHFVersion = HEADFILE_VERSION_CURRENT;
	fileheader.nHFSize = sizeof(SAVEHEADFILE);

	fileheader.nFCVersion = FILECONTENT_VERSION_CURRENT;
	fileheader.nFCSize = sizeof(SAVEFILECONTENT);

	fileheader.nEndianness = FILEHEADER_ENDIANNESS_VALUE;

	// Write header to file
	WriteFile(hFileWholeReg, &fileheader, sizeof(fileheader), &NBW, NULL);

	// new since header version 2
	// (v2) full computername
	if (0 < fileheader.nComputerNameLen) {
		// Write position in file header
		nFPCurrent = SetFilePointer(hFileWholeReg, 0, NULL, FILE_CURRENT);

		SetFilePointer(hFileWholeReg, offsetof(FILEHEADER, ofsComputerName), NULL, FILE_BEGIN);
		WriteFile(hFileWholeReg, &nFPCurrent, sizeof(nFPCurrent), &NBW, NULL);
		fileheader.ofsComputerName = nFPCurrent;  // keep track in memory too

		SetFilePointer(hFileWholeReg, nFPCurrent, NULL, FILE_BEGIN);

		// Write computername
		WriteFile(hFileWholeReg, lpShot->lpszComputerName, fileheader.nComputerNameLen * sizeof(TCHAR), &NBW, NULL);
	}

	// (v2) full username
	if (0 < fileheader.nUserNameLen) {
		// Write position in file header
		nFPCurrent = SetFilePointer(hFileWholeReg, 0, NULL, FILE_CURRENT);

		SetFilePointer(hFileWholeReg, offsetof(FILEHEADER, ofsUserName), NULL, FILE_BEGIN);
		WriteFile(hFileWholeReg, &nFPCurrent, sizeof(nFPCurrent), &NBW, NULL);
		fileheader.ofsUserName = nFPCurrent;  // keep track in memory too

		SetFilePointer(hFileWholeReg, nFPCurrent, NULL, FILE_BEGIN);

		// Write username
		WriteFile(hFileWholeReg, lpShot->lpszUserName, fileheader.nUserNameLen * sizeof(TCHAR), &NBW, NULL);
	}

	// Save HKLM
	if (NULL != lpShot->lpHKLM) 
	{
		SaveRegKeys(lpShot, lpShot->lpHKLM, 0, offsetof(FILEHEADER, ofsHKLM));
	}

	// Save HKU
	if (NULL != lpShot->lpHKU) 
	{
		SaveRegKeys(lpShot, lpShot->lpHKU, 0, offsetof(FILEHEADER, ofsHKU));
	}

	// Save HEADFILEs
	if (NULL != lpShot->lpHF) 
	{
		SaveHeadFiles(lpShot, lpShot->lpHF, offsetof(FILEHEADER, ofsHF));
	}

	// Close file
	CloseHandle(hFileWholeReg);
}

// ----------------------------------------------------------------------
// Adjust the size of a buffer
// ----------------------------------------------------------------------
size_t AdjustBuffer(LPVOID *lpBuffer, size_t nCurrentSize, size_t nWantedSize, size_t nAlign) 
{
	if (NULL == *lpBuffer) {
		nCurrentSize = 0;
	}

	if (nWantedSize > nCurrentSize) {
		if (NULL != *lpBuffer) {
			MYFREE(*lpBuffer);
			*lpBuffer = NULL;
		}

		if (1 >= nAlign) {
			nCurrentSize = nWantedSize;
		}
		else {
			nCurrentSize = nWantedSize / nAlign;
			nCurrentSize *= nAlign;
			if (nWantedSize > nCurrentSize) {
				nCurrentSize += nAlign;
			}
		}

		*lpBuffer = MYALLOC(nCurrentSize);
	}
	return nCurrentSize;
}


// ----------------------------------------------------------------------
// Load registry key with values from HIVE file
// ----------------------------------------------------------------------
VOID LoadRegKeys(LPSNAPSHOT lpShot, DWORD ofsKey, LPKEYCONTENT lpFatherKC, LPKEYCONTENT *lplpCaller) 
{
	LPKEYCONTENT lpKC;
	DWORD ofsBrotherKey;

	// Read all reg keys and their value and sub key contents
	for (; 0 != ofsKey; ofsKey = ofsBrotherKey) {
		// Copy SAVEKEYCONTENT to aligned memory block
		CopyMemory(&sKC, (lpFileBuffer + ofsKey), fileheader.nKCSize);

		// Save offsets in local variables due to recursive calls
		ofsBrotherKey = sKC.ofsBrotherKey;

		// Create new key content
		// put in a separate var for later use
		lpKC = MYALLOC0(sizeof(KEYCONTENT));

		// Set father of current key
		lpKC->lpFatherKC = lpFatherKC;

		// Copy key name
		if (fileextradata.bOldKCVersion) {  // old SBCS/MBCS version
			sKC.nKeyNameLen = (DWORD)strlen((const char *)(lpFileBuffer + sKC.ofsKeyName));
			if (0 < sKC.nKeyNameLen) {
				sKC.nKeyNameLen++;  // account for NULL char
			}
		}
		if (0 < sKC.nKeyNameLen) {  // otherwise leave it NULL
			// Copy string to an aligned memory block
			nSourceSize = sKC.nKeyNameLen * fileheader.nCharSize;
			nStringBufferSize = AdjustBuffer(&lpStringBuffer, nStringBufferSize, nSourceSize, REGSHOT_BUFFER_BLOCK_BYTES);
			ZeroMemory(lpStringBuffer, nStringBufferSize);
			CopyMemory(lpStringBuffer, (lpFileBuffer + sKC.ofsKeyName), nSourceSize);

			lpKC->lpszKeyName = MYALLOC(sKC.nKeyNameLen * sizeof(TCHAR));
			if (fileextradata.bSameCharSize) {
				_tcsncpy(lpKC->lpszKeyName, lpStringBuffer, sKC.nKeyNameLen);
			}
			else {
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)lpStringBuffer, -1, lpKC->lpszKeyName, sKC.nKeyNameLen);
			}
			lpKC->lpszKeyName[sKC.nKeyNameLen - 1] = (TCHAR)'\0';  // safety NULL char

			// Set key name length in chars
			lpKC->cchKeyName = _tcslen(lpKC->lpszKeyName);
		}

		// Adopt to short HKLM/HKU
		if (NULL == lpKC->lpFatherKC) {
			if ((0 == _tcscmp(lpKC->lpszKeyName, lpszHKLMShort))
				|| (0 == _tcscmp(lpKC->lpszKeyName, lpszHKLMLong))) {
				MYFREE(lpKC->lpszKeyName);
				lpKC->lpszKeyName = lpszHKLMShort;
				lpKC->cchKeyName = _tcslen(lpKC->lpszKeyName);
			}
			else if ((0 == _tcscmp(lpKC->lpszKeyName, lpszHKUShort))
				|| (0 == _tcscmp(lpKC->lpszKeyName, lpszHKULong))) {
				MYFREE(lpKC->lpszKeyName);
				lpKC->lpszKeyName = lpszHKUShort;
				lpKC->cchKeyName = _tcslen(lpKC->lpszKeyName);
			}
		}

		// Check if key is to be excluded
		// I am not excluding Registry keys, only values, so comment out for now...
		/*
		if (includeBlacklist) {
			LPTSTR lpszFullName;
			if ((NULL != lpKC->lpszKeyName) && IsInRegistryBlacklist(lpKC->lpszKeyName)) {
				FreeAllKeyContents(lpKC);
				continue;  // ignore this entry and continue with next brother key
			}
			lpszFullName = GetWholeKeyName(lpKC, FALSE);
			if (IsInRegistryBlacklist(lpszFullName)) {
				MYFREE(lpszFullName);
				FreeAllKeyContents(lpKC);
				continue;  // ignore this entry and continue with next brother key
			}
			MYFREE(lpszFullName);
		}
		*/

		// Copy pointer to current key into caller's pointer
		if (NULL != lplpCaller) {
			*lplpCaller = lpKC;
		}

		// Increase key count
		lpShot->stCounts.cKeys++;

		// Copy the value contents of the current key
		if (0 != sKC.ofsFirstValue) {
			LPVALUECONTENT lpVC;
			LPVALUECONTENT *lplpCallerVC;
			DWORD ofsValue;
			//LPTSTR lpszFullName;

			// Read all values of key
			lplpCallerVC = &lpKC->lpFirstVC;
			for (ofsValue = sKC.ofsFirstValue; 0 != ofsValue; ofsValue = sVC.ofsBrotherValue) {
				// Copy SAVEVALUECONTENT to aligned memory block
				CopyMemory(&sVC, (lpFileBuffer + ofsValue), fileheader.nVCSize);

				// Create new value content
				lpVC = MYALLOC0(sizeof(VALUECONTENT));

				// Set father key of current value to current key
				lpVC->lpFatherKC = lpKC;

				// Copy value name
				if (fileextradata.bOldVCVersion) {  // old SBCS/MBCS version
					sVC.nValueNameLen = (DWORD)strlen((const char *)(lpFileBuffer + sVC.ofsValueName));
					if (0 < sVC.nValueNameLen) {
						sVC.nValueNameLen++;  // account for NULL char
					}
				}
				if (0 < sVC.nValueNameLen) {  // otherwise leave it NULL
					// Copy string to an aligned memory block
					nSourceSize = sVC.nValueNameLen * fileheader.nCharSize;
					nStringBufferSize = AdjustBuffer(&lpStringBuffer, nStringBufferSize, nSourceSize, REGSHOT_BUFFER_BLOCK_BYTES);
					ZeroMemory(lpStringBuffer, nStringBufferSize);
					CopyMemory(lpStringBuffer, (lpFileBuffer + sVC.ofsValueName), nSourceSize);

					lpVC->lpszValueName = MYALLOC(sVC.nValueNameLen * sizeof(TCHAR));
					if (fileextradata.bSameCharSize) {
						_tcsncpy(lpVC->lpszValueName, lpStringBuffer, sVC.nValueNameLen);
					}
					else {
						MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)lpStringBuffer, -1, lpVC->lpszValueName, sVC.nValueNameLen);
					}
					lpVC->lpszValueName[sVC.nValueNameLen - 1] = (TCHAR)'\0';  // safety NULL char

					// Set value name length in chars
					lpVC->cchValueName = _tcslen(lpVC->lpszValueName);
				}
				
				/*
				// Check if value is to be excluded
				if (includeBlacklist) {
					LPTSTR lpszFullName;
					if ((NULL != lpVC->lpszValueName) && IsInRegistryBlacklist(lpVC->lpszValueName)) {
						FreeAllValueContents(lpVC);
						continue;  // ignore this entry and continue with next brother value
					}
					lpszFullName = GetWholeValueName(lpVC, FALSE);
					if (IsInRegistryBlacklist(lpszFullName)) {
						MYFREE(lpszFullName);
						FreeAllValueContents(lpVC);
						continue;   // ignore this entry and continue with next brother value
					}
					MYFREE(lpszFullName);
				}
				*/

				// Copy pointer to current value into previous value's next value pointer
				if (NULL != lplpCallerVC) {
					*lplpCallerVC = lpVC;
				}

				// Increase value count
				lpShot->stCounts.cValues++;

				// Copy value meta data
				lpVC->nTypeCode = sVC.nTypeCode;
				lpVC->cbData = sVC.cbData;

				// Copy value data
				if (0 < sVC.cbData) {  // otherwise leave it NULL
					lpVC->lpValueData = MYALLOC(sVC.cbData);
					CopyMemory(lpVC->lpValueData, (lpFileBuffer + sVC.ofsValueData), sVC.cbData);
				}

				// Set "lpBrotherVC" pointer for storing the next brother's pointer
				lplpCallerVC = &lpVC->lpBrotherVC;
			}
		}

		// ATTENTION!!! sKC will be INVALID from this point on, due to recursive calls
		// If the entry has childs, then do a recursive call for the first child
		// Pass this entry as father and "lpFirstSubKC" pointer for storing the first child's pointer
		if (0 != sKC.ofsFirstSubKey) {
			LoadRegKeys(lpShot, sKC.ofsFirstSubKey, lpKC, &lpKC->lpFirstSubKC);
		}

		// Set "lpBrotherKC" pointer for storing the next brother's pointer
		lplpCaller = &lpKC->lpBrotherKC;
	}
}


// ----------------------------------------------------------------------
// Load registry and files from HIVE file
// ----------------------------------------------------------------------
BOOL LoadShot(LPSNAPSHOT lpShot, LPTSTR lpszFileName) 
{
	DWORD cbFileSize;
	DWORD cbFileRemain;
	DWORD cbReadSize;
	DWORD cbFileRead;

	// Open file for reading
	hFileWholeReg = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hFileWholeReg) {
		printf("\n>>> ERROR opening file.\n");
		printf("Error %u", GetLastError());
		return FALSE;
	}

	cbFileSize = GetFileSize(hFileWholeReg, NULL);
	if (sizeof(fileheader) > cbFileSize) {
		CloseHandle(hFileWholeReg);
		printf("\n>>> ERROR matching file size.\n");
		return FALSE;
	}

	// Initialize file header
	ZeroMemory(&fileheader, sizeof(fileheader));
	ZeroMemory(&fileextradata, sizeof(fileextradata));

	// Read first part of file header from file (signature, nHeaderSize)
	ReadFile(hFileWholeReg, &fileheader, offsetof(FILEHEADER, ofsHKLM), &NBW, NULL);

	// Check for valid file signatures (SBCS/MBCS and UTF-16 signature)
	if ((0 != strncmp(szLiveDiffFileSignatureSBCS, fileheader.signature, MAX_SIGNATURE_LENGTH)) && (0 != strncmp(szLiveDoffFileSignatureUTF16, fileheader.signature, MAX_SIGNATURE_LENGTH))) {
		CloseHandle(hFileWholeReg);
		printf("\n>>> ERROR: This is not a LiveDiff file.\n");
		return FALSE;
	}

	// Check file signature for correct type (SBCS/MBCS or UTF-16)
	if (0 != strncmp(szLiveDiffFileSignature, fileheader.signature, MAX_SIGNATURE_LENGTH)) {
		CloseHandle(hFileWholeReg);
		printf("\n>>> ERROR: This is not a LiveDiff file (or it is not unicode).\n");
		return FALSE;
	}

	// Clear shot
	FreeShot(lpShot);

	// Allocate memory to hold the complete file
	lpFileBuffer = MYALLOC(cbFileSize);

	// Read file blockwise for progress bar
	SetFilePointer(hFileWholeReg, 0, NULL, FILE_BEGIN);
	cbFileRemain = cbFileSize;  // 100% to go
	cbReadSize = LIVEDIFF_READ_BLOCK_SIZE;  // next block length to read
	for (cbFileRead = 0; 0 < cbFileRemain; cbFileRead += cbReadSize) {
		// If the rest is smaller than a block, then use the rest length
		if (LIVEDIFF_READ_BLOCK_SIZE > cbFileRemain) {
			cbReadSize = cbFileRemain;
		}

		// Read the next block
		ReadFile(hFileWholeReg, lpFileBuffer + cbFileRead, cbReadSize, &NBW, NULL);
		if (NBW != cbReadSize) {
			CloseHandle(hFileWholeReg);
			printf("\n>>> ERROR: Cannot read the LiveDiff file.\n");
			return FALSE;
		}

		// Determine how much to go, if zero leave the for-loop
		cbFileRemain -= cbReadSize;
		if (0 == cbFileRemain) {
			break;
		}
	}

	// Close file
	CloseHandle(hFileWholeReg);

	// Check size for copying file header
	nSourceSize = fileheader.nFHSize;
	if (0 == nSourceSize) {
		nSourceSize = offsetof(FILEHEADER, nFHVersion);
	}
	else if (sizeof(fileheader) < nSourceSize) {
		nSourceSize = sizeof(fileheader);
	}

	// Copy file header to structure
	CopyMemory(&fileheader, lpFileBuffer, nSourceSize);

	// Enhance data of old headers to be used with newer code
	if (FILEHEADER_VERSION_EMPTY == fileheader.nFHVersion) {
		if ((0 != fileheader.ofsHKLM) && (fileheader.ofsHKU == fileheader.ofsHKLM)) {
			fileheader.ofsHKLM = 0;
		}
		if ((0 != fileheader.ofsHKU) && (fileheader.ofsHF == fileheader.ofsHKU)) {
			fileheader.ofsHKU = 0;
		}

		fileheader.nFHVersion = FILEHEADER_VERSION_1;
		fileheader.nCharSize = 1;
		fileheader.nKCVersion = KEYCONTENT_VERSION_1;
		fileheader.nKCSize = offsetof(SAVEKEYCONTENT, nKeyNameLen);
		fileheader.nVCVersion = VALUECONTENT_VERSION_1;
		fileheader.nVCSize = offsetof(SAVEVALUECONTENT, nValueNameLen);
		fileheader.nHFVersion = HEADFILE_VERSION_1;
		fileheader.nHFSize = sizeof(SAVEHEADFILE);  // not changed yet, if it is then adopt to offsetof(SAVEHEADFILE, <first new field>)
		fileheader.nFCVersion = FILECONTENT_VERSION_1;
		fileheader.nFCSize = offsetof(SAVEFILECONTENT, nFileNameLen);
	}
	if (FILEHEADER_VERSION_2 >= fileheader.nFHVersion) {
		if (0 == fileheader.nEndianness) {
			*((unsigned char *)&fileheader.nEndianness) = 0x78;  // old existing versions were all little endian
			*((unsigned char *)&fileheader.nEndianness + 1) = 0x56;
			*((unsigned char *)&fileheader.nEndianness + 2) = 0x34;
			*((unsigned char *)&fileheader.nEndianness + 3) = 0x12;
		}
	}

	// Check structure boundaries
	if (sizeof(SAVEKEYCONTENT) < fileheader.nKCSize) {
		fileheader.nKCSize = sizeof(SAVEKEYCONTENT);
	}
	if (sizeof(SAVEVALUECONTENT) < fileheader.nVCSize) {
		fileheader.nVCSize = sizeof(SAVEVALUECONTENT);
	}
	if (sizeof(SAVEHEADFILE) < fileheader.nHFSize) {
		fileheader.nHFSize = sizeof(SAVEHEADFILE);
	}
	if (sizeof(SAVEFILECONTENT) < fileheader.nFCSize) {
		fileheader.nFCSize = sizeof(SAVEFILECONTENT);
	}

	// Check for incompatible char size (known: 1 = SBCS/MBCS, 2 = UTF-16)
	if (sizeof(TCHAR) == fileheader.nCharSize) {
		fileextradata.bSameCharSize = TRUE;
	}
	else {
		fileextradata.bSameCharSize = FALSE;
		if (1 == fileheader.nCharSize) {
			printf("\n>>> ERROR: This is not a LiveDiff file.\n");
		}
		else if (2 != fileheader.nCharSize) {
			printf("\n>>> ERROR: Unknown character size.\n");
		}
		else {
			printf("\n>>> ERROR: Unsupported character size.\n");
		}

		if (NULL != lpFileBuffer) {
			MYFREE(lpFileBuffer);
			lpFileBuffer = NULL;
		}
		return FALSE;
	}

	// Check for incompatible endianness (known: Intel = Little Endian)
	if (FILEHEADER_ENDIANNESS_VALUE == fileheader.nEndianness) {
		fileextradata.bSameEndianness = TRUE;
	}
	else {
		fileextradata.bSameEndianness = FALSE;
#ifdef __LITTLE_ENDIAN__
		printf("\n>>> ERROR: This is not a Little Endian LiveDiff hive file.\n");
#else
		printf("\n>>> ERROR: This is not a Big Endian LiveDiff hive file.\n");
#endif
		if (NULL != lpFileBuffer) {
			MYFREE(lpFileBuffer);
			lpFileBuffer = NULL;
		}
		return FALSE;
	}

	if (KEYCONTENT_VERSION_2 > fileheader.nKCVersion) {  // old SBCS/MBCS version
		fileextradata.bOldKCVersion = TRUE;
	}
	else {
		fileextradata.bOldKCVersion = FALSE;
	}

	if (VALUECONTENT_VERSION_2 > fileheader.nVCVersion) {  // old SBCS/MBCS version
		fileextradata.bOldVCVersion = TRUE;
	}
	else {
		fileextradata.bOldVCVersion = FALSE;
	}

	if (FILECONTENT_VERSION_2 > fileheader.nFCVersion) {  // old SBCS/MBCS version
		fileextradata.bOldFCVersion = TRUE;
	}
	else {
		fileextradata.bOldFCVersion = FALSE;
	}

	// ^^^ here the file header can be checked for additional extended content
	// * remember that files from older versions do not provide these additional data

	// New temporary string buffer
	lpStringBuffer = NULL;

	// Copy computer name from file header to shot data
	if (FILEHEADER_VERSION_1 < fileheader.nFHVersion) {  // newer Unicode/SBCS/MBCS version
		if (0 < fileheader.nComputerNameLen) {  // otherwise leave it NULL
			// Copy string to an aligned memory block
			nSourceSize = fileheader.nComputerNameLen * fileheader.nCharSize;
			nStringBufferSize = AdjustBuffer(&lpStringBuffer, nStringBufferSize, nSourceSize, REGSHOT_BUFFER_BLOCK_BYTES);
			ZeroMemory(lpStringBuffer, nStringBufferSize);
			CopyMemory(lpStringBuffer, (lpFileBuffer + fileheader.ofsComputerName), nSourceSize);

			lpShot->lpszComputerName = MYALLOC(fileheader.nComputerNameLen * sizeof(TCHAR));
			if (fileextradata.bSameCharSize) {
				_tcsncpy(lpShot->lpszComputerName, lpStringBuffer, fileheader.nComputerNameLen);
			}
			else {
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)lpStringBuffer, -1, lpShot->lpszComputerName, fileheader.nComputerNameLen);
			}
			lpShot->lpszComputerName[fileheader.nComputerNameLen - 1] = (TCHAR)'\0';  // safety NULL char
		}
	}
	else {  // old SBCS/MBCS version
		// Copy string to an aligned memory block
		nSourceSize = strnlen((const char *)&fileheader.computername, OLD_COMPUTERNAMELEN);
		if (0 < nSourceSize) {  // otherwise leave it NULL
			nStringBufferSize = AdjustBuffer(&lpStringBuffer, nStringBufferSize, (nSourceSize + 1), REGSHOT_BUFFER_BLOCK_BYTES);
			ZeroMemory(lpStringBuffer, nStringBufferSize);
			CopyMemory(lpStringBuffer, &fileheader.computername, nSourceSize);

			lpShot->lpszComputerName = MYALLOC((nSourceSize + 1) * sizeof(TCHAR));
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)lpStringBuffer, (int)(nSourceSize + 1), lpShot->lpszComputerName, (int)(nSourceSize + 1));
			lpShot->lpszComputerName[nSourceSize] = (TCHAR)'\0';  // safety NULL char
		}
	}

	// Copy user name from file header to shot data
	if (FILEHEADER_VERSION_1 < fileheader.nFHVersion) {  // newer Unicode/SBCS/MBCS version
		if (0 < fileheader.nUserNameLen) {  // otherwise leave it NULL
			// Copy string to an aligned memory block
			nSourceSize = fileheader.nUserNameLen * fileheader.nCharSize;
			nStringBufferSize = AdjustBuffer(&lpStringBuffer, nStringBufferSize, nSourceSize, REGSHOT_BUFFER_BLOCK_BYTES);
			ZeroMemory(lpStringBuffer, nStringBufferSize);
			CopyMemory(lpStringBuffer, (lpFileBuffer + fileheader.ofsUserName), nSourceSize);

			lpShot->lpszUserName = MYALLOC(fileheader.nUserNameLen * sizeof(TCHAR));
			if (fileextradata.bSameCharSize) {
				_tcsncpy(lpShot->lpszUserName, lpStringBuffer, fileheader.nUserNameLen);
			}
			else {
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)lpStringBuffer, -1, lpShot->lpszUserName, fileheader.nUserNameLen);
			}
			lpShot->lpszUserName[fileheader.nUserNameLen - 1] = (TCHAR)'\0';  // safety NULL char
		}
	}
	else {  // old SBCS/MBCS version
		// Copy string to an aligned memory block
		nSourceSize = strnlen((const char *)&fileheader.username, OLD_COMPUTERNAMELEN);
		if (0 < nSourceSize) {  // otherwise leave it NULL
			nStringBufferSize = AdjustBuffer(&lpStringBuffer, nStringBufferSize, (nSourceSize + 1), REGSHOT_BUFFER_BLOCK_BYTES);
			ZeroMemory(lpStringBuffer, nStringBufferSize);
			CopyMemory(lpStringBuffer, &fileheader.username, nSourceSize);

			lpShot->lpszUserName = MYALLOC((nSourceSize + 1) * sizeof(TCHAR));
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)lpStringBuffer, (int)(nSourceSize + 1), lpShot->lpszUserName, (int)(nSourceSize + 1));
			lpShot->lpszUserName[nSourceSize] = (TCHAR)'\0';  // safety NULL char
		}
	}

	CopyMemory(&lpShot->systemtime, &fileheader.systemtime, sizeof(SYSTEMTIME));

	// Initialize save structures
	ZeroMemory(&sKC, sizeof(sKC));
	ZeroMemory(&sVC, sizeof(sVC));

	if (0 != fileheader.ofsHKLM) 
	{
		LoadRegKeys(lpShot, fileheader.ofsHKLM, NULL, &lpShot->lpHKLM);
	}

	if (0 != fileheader.ofsHKU) 
	{
		LoadRegKeys(lpShot, fileheader.ofsHKU, NULL, &lpShot->lpHKU);
	}

	if (0 != fileheader.ofsHF) 
	{
		LoadHeadFiles(lpShot, fileheader.ofsHF, &lpShot->lpHF);
	}

	// Get total count of all items
	lpShot->stCounts.cAll = lpShot->stCounts.cKeys
		+ lpShot->stCounts.cValues
		+ lpShot->stCounts.cDirs
		+ lpShot->stCounts.cFiles;

	if (NULL != lpStringBuffer) {
		MYFREE(lpStringBuffer);
		lpStringBuffer = NULL;
	}

	if (NULL != lpFileBuffer) {
		MYFREE(lpFileBuffer);
		lpFileBuffer = NULL;
	}

	// Set flags
	lpShot->fFilled = TRUE;
	lpShot->fLoaded = TRUE;

	return TRUE;
}