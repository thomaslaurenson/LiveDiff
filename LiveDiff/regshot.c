/*
Copyright 2016 Thomas Laurenson
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

// Variable for Snapshot1, Snapshot2, and CompareResults structures
SNAPSHOT Shot1;
SNAPSHOT Shot2;
COMPRESULTS	CompareResult;

// Buffers and size variables
LPTSTR lpStringBuffer;
LPBYTE lpDataBuffer;
size_t nStringBufferSize;
size_t nDataBufferSize;
size_t nSourceSize;

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
							if (dwPrecisionLevel > 1)
							{
								lpVC2->fValueMatch = ISMODI;
								CompareResult.stcChanged.cValues++;
								CompareResult.stcModified.cValues++;
								CreateNewResult(VALMODI, lpVC1, lpVC2);
							}
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
		printf("Different dir chain found. Exiting.\n");
		exit(1);
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
		DWORD cValues;
		DWORD cchMaxValueName;
		DWORD cbMaxValueData;
		FILETIME ftLastWriteTime;

		// Create new key content, put in a separate var for later use
		lpKC = MYALLOC0(sizeof(KEYCONTENT));

		// Set father of current key
		lpKC->lpFatherKC = lpFatherKC;

		// Set key name, key name length
		lpKC->lpszKeyName = lpszRegKeyName;
		lpKC->cchKeyName = _tcslen(lpKC->lpszKeyName);

		// Check if key is to be excluded
		if (performStaticBlacklisting) {
			BOOL found = FALSE;
			LPTSTR lpszFullPath;
			lpszFullPath = GetWholeKeyName(lpKC, FALSE);
			found = TrieSearchPath(blacklistKEYS->children, lpszFullPath);
			if (found) 
			{
				MYFREE(lpszFullPath);
				FreeAllKeyContents(lpKC);

				lpShot->stCounts.cKeysBlacklist++;

				return NULL;
			}
			MYFREE(lpszFullPath);
		}

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
			&ftLastWriteTime
			);
		if (ERROR_SUCCESS != nErrNo) {
			// TODO: process/protocol issue in some way, do not silently ignore it (at least in Debug builds)
			FreeAllKeyContents(lpKC);
			return NULL;
		}

		lpKC->ftLastWriteTime = ftLastWriteTime;

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
			nStringBufferSize = AdjustBuffer(&lpStringBuffer, nStringBufferSize, nSourceSize, BUFFER_BLOCK_BYTES);

			// Get buffer for maximum value data length
			nSourceSize = cbMaxValueData;
			nDataBufferSize = AdjustBuffer(&lpDataBuffer, nDataBufferSize, nSourceSize, BUFFER_BLOCK_BYTES);

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
				if (lpVC->lpszValueName == NULL) 
				{
					LPTSTR lpszDefaultValueName = TEXT("(Default)\0");
					MYFREE(lpVC->lpszValueName);
					lpVC->cchValueName = _tcslen(lpszDefaultValueName);
					lpVC->lpszValueName = MYALLOC(lpVC->cchValueName * sizeof(TCHAR));
					_tcscpy(lpVC->lpszValueName, lpszDefaultValueName);
				}

				// Blacklisting implementation for known Registry values		
				if (dwBlacklist == 1)
				{
					// First snapshot, therefore populate the Trie (Prefix Tree)
					// Get the full file path
					LPTSTR lpszFullPath;
					lpszFullPath = GetWholeValueName(lpVC, FALSE);
					
					// Add full path to Registry blacklist, then free path
					TrieAdd(&blacklistVALUES, lpszFullPath);
					MYFREE(lpszFullPath);
					
					// Increase value count for display purposes
					lpShot->stCounts.cValues++;

					//continue; // ignore this entry and continue with next brother value
				}
				else if (dwBlacklist == 2 && performDynamicBlacklisting)
				{
					// Not the first snapshot, so filter known Registry value paths
					BOOL found = FALSE;
					LPTSTR lpszFullPath;

					// Get the full Registry path
					lpszFullPath = GetWholeValueName(lpVC, FALSE);

					// Search the Registry blacklist prefix tree for path
					found = TrieSearchPath(blacklistVALUES->children, lpszFullPath);

					// If path is found, skip to next entry, otherwise free the path and keep going
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
	if ((dwBlacklist == 2 && performDynamicBlacklisting) || (performStaticBlacklisting)) {
		printf("  > Keys: %d  Values: %d  Blacklisted Keys: %d  Blacklisted Values: %d\r", lpShot->stCounts.cKeys, 
			lpShot->stCounts.cValues, 
			lpShot->stCounts.cKeysBlacklist,
			lpShot->stCounts.cValuesBlacklist);
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
		nStringBufferSize = AdjustBuffer(&lpStringBuffer, nStringBufferSize, nSourceSize, BUFFER_BLOCK_BYTES);

		// Get registry sub keys
		lplpKCPrev = &lpKC->lpFirstSubKC;
		for (i = 0;; i++) {
			// Extra local block to reduce stack usage due to recursive calls
				{
					DWORD cchSubKeyName;
					FILETIME ftLastWriteTime;

					// Enumerate sub key
					cchSubKeyName = (DWORD)nStringBufferSize;
					nErrNo = RegEnumKeyEx(hRegKey, i,
						lpStringBuffer,
						&cchSubKeyName,  // in TCHARs; in *including* and out *excluding* NULL char
						NULL,
						NULL,
						NULL,
						&ftLastWriteTime);

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
	if ((dwBlacklist == 2 && performDynamicBlacklisting) || (performStaticBlacklisting)) {
		printf("  > Keys: %d  Values: %d  Blacklisted Keys: %d  Blacklisted Values: %d\n", lpShot->stCounts.cKeys,
			lpShot->stCounts.cValues,
			lpShot->stCounts.cKeysBlacklist,
			lpShot->stCounts.cValuesBlacklist);
	}
	else {
		printf("  > Keys: %d  Values: %d\n", lpShot->stCounts.cKeys, lpShot->stCounts.cValues);
	}
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