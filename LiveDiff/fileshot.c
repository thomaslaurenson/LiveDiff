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

WIN32_FIND_DATA FindData;
LPTSTR lpszExtDir;
LPTSTR lpszWindowsDirName;

// Set up buffer sizes for file hashing
#define BLOCKSIZE	512
#define BUFSIZE		1024
#define SHA1LEN		20
#define MD5LEN		16

//-----------------------------------------------------------------
// Calculate the MD5 hash value from a previously matched file name
//----------------------------------------------------------------- 
LPTSTR CalculateMD5(LPTSTR FileName)
{
	HANDLE hashFile = NULL;
	BOOL bResult = FALSE;
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	DWORD cbRead = 0;
	DWORD cbHash = MD5LEN;
	BYTE rgbFile[BUFSIZE];
	BYTE rgbHash[MD5LEN];
	CHAR rgbDigits[] = "0123456789abcdef";
	LPTSTR md5HashString;

	// Open the file to perform hash
	hashFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (INVALID_HANDLE_VALUE == hashFile) {
		return TEXT("ERROR_OPENING_FILE\0");
	}

	// Get handle to the crypto provider
	if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
		CloseHandle(hashFile);
		return TEXT("HASHING_FAILED\0");
	}

	if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
		CloseHandle(hashFile);
		return TEXT("HASHING_FAILED\0");
	}

	while (bResult = ReadFile(hashFile, rgbFile, BUFSIZE, &cbRead, NULL)) {
		if (0 == cbRead) {
			break;
		}
		if (!CryptHashData(hHash, rgbFile, cbRead, 0)) {
			CryptReleaseContext(hProv, 0);
			CryptDestroyHash(hHash);
			CloseHandle(hashFile);
			return TEXT("HASHING_FAILED\0");
		}
	}

	if (!bResult) {
		CryptReleaseContext(hProv, 0);
		CryptDestroyHash(hHash);
		CloseHandle(hashFile);
		return TEXT("ERROR_READING_FILE\0");
	}

	// Finally got here with no errors, now calculate the SHA1 hash value
	md5HashString = MYALLOC0((MD5LEN * 2 + 1) * sizeof(TCHAR));
	_tcscpy_s(md5HashString, 1, TEXT(""));
	if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0)) {
		for (DWORD i = 0; i < cbHash; i++) {
			_sntprintf(md5HashString + (i * 2), 2, TEXT("%02x\0"), rgbHash[i]);
		}
	}

	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);
	CloseHandle(hashFile);
	return md5HashString;
}

//-----------------------------------------------------------------
// Calculate the SHA1 hash value from a previously matched file name
//----------------------------------------------------------------- 
LPTSTR CalculateSHA1(LPTSTR FileName)
{
	HANDLE hashFile = NULL;
	BOOL bResult = FALSE;
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	DWORD cbRead = 0;
	DWORD cbHash = SHA1LEN;
	BYTE rgbFile[BUFSIZE];
	BYTE rgbHash[SHA1LEN];
	CHAR rgbDigits[] = "0123456789abcdef";
	LPTSTR sha1HashString;

	// Open the file to perform hash
	hashFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (INVALID_HANDLE_VALUE == hashFile) {
		return TEXT("ERROR_OPENING_FILE\0");
	}

	// Get handle to the crypto provider
	if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
		CloseHandle(hashFile);
		return TEXT("HASHING_FAILED\0");
	}

	if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
		CloseHandle(hashFile);
		return TEXT("HASHING_FAILED\0");
	}

	while (bResult = ReadFile(hashFile, rgbFile, BUFSIZE, &cbRead, NULL)) {
		if (0 == cbRead) {
			break;
		}
		if (!CryptHashData(hHash, rgbFile, cbRead, 0)) {
			CryptReleaseContext(hProv, 0);
			CryptDestroyHash(hHash);
			CloseHandle(hashFile);
			return TEXT("HASHING_FAILED\0");
		}
	}

	if (!bResult) {
		CryptReleaseContext(hProv, 0);
		CryptDestroyHash(hHash);
		CloseHandle(hashFile);
		return TEXT("ERROR_READING_FILE\0");
	}

	// Finally got here with no errors, now calculate the SHA1 hash value
	sha1HashString = MYALLOC0((SHA1LEN * 2 + 1) * sizeof(TCHAR));
	_tcscpy_s(sha1HashString, 1, TEXT(""));
	if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0)) {
		for (DWORD i = 0; i < cbHash; i++) {
			_sntprintf(sha1HashString + (i * 2), 2, TEXT("%02x\0"), rgbHash[i]);
		}
	}
	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);
	CloseHandle(hashFile);
	return sha1HashString;
}

//-----------------------------------------------------------------
// Block hashing: Calculate the MD5 hash value for a 4096 block
//----------------------------------------------------------------- 
LPMD5BLOCK md5Block(BYTE rgbFile[BLOCKSIZE], DWORD dwFileOffset, DWORD dwRunLength)
{
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	BYTE rgbHash[MD5LEN];
	DWORD cbHash = MD5LEN;

	LPMD5BLOCK MD5Block = NULL;
	MD5Block = MYALLOC0(sizeof(MD5BLOCK));

	// Get handle to the crypto provider
	if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
		printf(">>> ERROR: md5Block: CryptAcquireContext");
		return MD5Block;
	}

	if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
		printf(">>> ERROR: md5Block: CryptCreateHash"); 
		return MD5Block;
	}

	DWORD cbRead = dwRunLength;

	if (!CryptHashData(hHash, rgbFile, cbRead, 0)) {
		CryptReleaseContext(hProv, 0);
		CryptDestroyHash(hHash);
		printf(">>> ERROR: md5Block: CryptHashData"); 
		return MD5Block;
	}

	if (!CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0)) {
		printf(">>> ERROR: md5Block: CryptGetHashParam");
	}

	int p = 0;
	while (p < MD5LEN) {
		MD5Block->bMD5Hash[p] = rgbHash[p];
		p++;
	}

	MD5Block->lpNextMD5Block = NULL;
	return MD5Block;
}

//-----------------------------------------------------------------
// Block hashing: From a file, calculate the SHA1 hash value in 4096 blocks
//----------------------------------------------------------------- 
LPMD5BLOCK CalculateMD5Blocks(LPTSTR FileName)
{
	HANDLE hashFile = NULL;
	BOOL bResult = FALSE;
	DWORD cbRead = 0;
	BYTE rgbFile[BLOCKSIZE];
	CHAR rgbDigits[] = "0123456789abcdef";
	DWORD dwFileOffset = 0;

	LPMD5BLOCK firstMD5Block = MYALLOC0(sizeof(LPMD5BLOCK));

	// Open the file to perform hash
	hashFile = CreateFile(FileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN,
		NULL);
	if (INVALID_HANDLE_VALUE == hashFile) {
		printf(">>> ERROR: CalculateMD5Blocks: ERROR_OPENING_FILE");
	}

	// Read input file in 4096 byte blocks, and SHA1 hash each block
	while (bResult = ReadFile(hashFile, rgbFile, BLOCKSIZE, &cbRead, NULL)) {
		// If number of read bytes is 0, break loop due to EOF
		if (0 == cbRead) {
			break;
		}

		LPMD5BLOCK MD5Block = md5Block(rgbFile, dwFileOffset, cbRead);

		if (MD5Block == NULL) {
			printf(">>> ERROR: CalculateMD5Blocks: Error creating MD5BLOCK");
			break;
		}

		if (dwFileOffset == 0) {
			firstMD5Block = MD5Block;
		}
		else {
			pushBlock(firstMD5Block, MD5Block);
		}

		// Increase the file offset based on number of bytes read
		dwFileOffset += cbRead;
	}

	// Close the file handle
	CloseHandle(hashFile);

	// Return the first block (linked to all other blocks)
	return firstMD5Block;
}

//-----------------------------------------------------------------
// Block hashing: Append a MD5BLOCK to the previous MD5BLOCK
//----------------------------------------------------------------- 
VOID pushBlock(LPMD5BLOCK firstMD5Block, LPMD5BLOCK MD5Block)
{
	LPMD5BLOCK current = firstMD5Block;
	while (current->lpNextMD5Block != NULL) {
		current = current->lpNextMD5Block;
	}

	current->lpNextMD5Block = MYALLOC0(sizeof(MD5BLOCK));
	current->lpNextMD5Block = MD5Block;
	current->lpNextMD5Block->lpNextMD5Block = NULL;
}

//-------------------------------------------------------------
// Get whole file name from FILECONTENT
//-------------------------------------------------------------
LPTSTR GetWholeFileName(LPFILECONTENT lpStartFC, size_t cchExtra) 
{
	LPFILECONTENT lpFC;
	LPTSTR lpszName;
	LPTSTR lpszTail;
	size_t cchName;

	cchName = 0;
	for (lpFC = lpStartFC; NULL != lpFC; lpFC = lpFC->lpFatherFC) {
		if (NULL != lpFC->lpszFileName) {
			cchName += lpFC->cchFileName + 1;  // +1 char for backslash or NULL char
		}
	}
	if (0 == cchName) {  // at least create an empty string with NULL char
		cchName++;
	}

	lpszName = MYALLOC((cchName + cchExtra) * sizeof(TCHAR));

	lpszTail = &lpszName[cchName - 1];
	lpszTail[0] = (TCHAR)'\0';

	for (lpFC = lpStartFC; NULL != lpFC; lpFC = lpFC->lpFatherFC) {
		if (NULL != lpFC->lpszFileName) {
			cchName = lpFC->cchFileName;
			_tcsncpy(lpszTail -= cchName, lpFC->lpszFileName, cchName);
			if (lpszTail > lpszName) {
				*--lpszTail = (TCHAR)'\\';
			}
		}
	}
	return lpszName;
}

//--------------------------------------------------
// Walkthrough lpHF chain and find lpszName matches
//--------------------------------------------------
LPHEADFILE SearchDirChain(LPTSTR lpszName, LPHEADFILE lpHF) 
{
	if (NULL != lpszName) {
		for (; NULL != lpHF; lpHF = lpHF->lpBrotherHF) {
			if ((NULL != lpHF->lpFirstFC)
				&& (NULL != lpHF->lpFirstFC->lpszFileName)
				&& (0 == _tcsicmp(lpszName, lpHF->lpFirstFC->lpszFileName))) {
				return lpHF;
			}
		}
	}
	return NULL;
}

// ----------------------------------------------------------------------
// Free all files
// ----------------------------------------------------------------------
VOID FreeAllFileContents(LPFILECONTENT lpFC) 
{
	LPFILECONTENT lpBrotherFC;

	for (; NULL != lpFC; lpFC = lpBrotherFC) {
		// Save pointer in local variable
		lpBrotherFC = lpFC->lpBrotherFC;

		// Free file name
		if (NULL != lpFC->lpszFileName) {
			MYFREE(lpFC->lpszFileName);
		}

		// If the entry has childs, then do a recursive call for the first child
		if (NULL != lpFC->lpFirstSubFC) {
			FreeAllFileContents(lpFC->lpFirstSubFC);
		}

		// Free entry itself
		MYFREE(lpFC);
	}
}

// ----------------------------------------------------------------------
// Free all head files
// ----------------------------------------------------------------------
VOID FreeAllHeadFiles(LPHEADFILE lpHF)
{
	LPHEADFILE lpBrotherHF;

	for (; NULL != lpHF; lpHF = lpBrotherHF) {
		// Save pointer in local variable
		lpBrotherHF = lpHF->lpBrotherHF;

		// If the entry has childs, then do a recursive call for the first child
		if (NULL != lpHF->lpFirstFC) {
			FreeAllFileContents(lpHF->lpFirstFC);
		}

		// Free entry itself
		MYFREE(lpHF);
	}
}

//-------------------------------------------------------------
// File comparison engine
//-------------------------------------------------------------
VOID CompareFiles(LPFILECONTENT lpStartFC1, LPFILECONTENT lpStartFC2)
{
	LPFILECONTENT lpFC1;
	LPFILECONTENT lpFC2;

	// Compare dirs/files
	for (lpFC1 = lpStartFC1; NULL != lpFC1; lpFC1 = lpFC1->lpBrotherFC) {
		if (ISFILE(lpFC1->nFileAttributes)) {
			CompareResult.stcCompared.cFiles++;
		}
		else {
			CompareResult.stcCompared.cDirs++;
		}

		// Find a matching dir/file for FC1
		for (lpFC2 = lpStartFC2; NULL != lpFC2; lpFC2 = lpFC2->lpBrotherFC) {
			// skip FC2 if already matched
			if (NOMATCH != lpFC2->fFileMatch) {
				continue;
			}

			// skip FC2 if types do *not* match (even if same name then interpret as deleted+added)
			if (ISFILE(lpFC1->nFileAttributes) != ISFILE(lpFC2->nFileAttributes)) {
				continue;
			}

			// skip FC2 if names do *not* match
			if ((NULL == lpFC1->lpszFileName) || (NULL == lpFC2->lpszFileName) || (0 != _tcsicmp(lpFC1->lpszFileName, lpFC2->lpszFileName))) { // 1.8.2 from lstrcmp to strcmp  // 1.9.0 to _tcsicmp
				continue;
			}

			// Same file type and (case-insensitive) name of FC1 found in FC2, so compare their attributes and if applicable their dates and sizes
			if (ISFILE(lpFC1->nFileAttributes)) 
			{
				// Both are files
				if ((lpFC1->nWriteDateTimeLow == lpFC2->nWriteDateTimeLow)
					&& (lpFC1->nWriteDateTimeHigh == lpFC2->nWriteDateTimeHigh)
					&& (lpFC1->nFileSizeLow == lpFC2->nFileSizeLow)
					&& (lpFC1->nFileSizeHigh == lpFC2->nFileSizeHigh)
					&& (lpFC1->nFileAttributes == lpFC2->nFileAttributes)) 
				{
					// Same file of FC1 found in FC2
					lpFC2->fFileMatch = ISMATCH;
				}
				else 
				{
					// File data differ, so file is modified
					lpFC2->fFileMatch = ISMODI;
					CompareResult.stcChanged.cFiles++;
					CompareResult.stcModified.cFiles++;
					CreateNewResult(FILEMODI, lpFC1, lpFC2);
				}
			}
			else
			{
				// Both are dirs
				if (lpFC1->nFileAttributes == lpFC2->nFileAttributes) {
					// Same dir of FC1 found in FC2
					lpFC2->fFileMatch = ISMATCH;
				}
				else 
				{
					// Dir data differ, so dir is modified
					lpFC2->fFileMatch = ISMODI;
					CompareResult.stcChanged.cDirs++;
					CompareResult.stcModified.cDirs++;
					CreateNewResult(DIRMODI, lpFC1, lpFC2);
				}
				// Compare sub files if any
				if ((NULL != lpFC1->lpFirstSubFC) || (NULL != lpFC2->lpFirstSubFC)) {
					CompareFiles(lpFC1->lpFirstSubFC, lpFC2->lpFirstSubFC);
				}
			}

			break;
		}
		if (NULL == lpFC2) 
		{
			// FC1 has no matching FC2, so FC1 is a deleted dir/file
			if (ISFILE(lpFC1->nFileAttributes)) 
			{
				CompareResult.stcChanged.cFiles++;
				CompareResult.stcDeleted.cFiles++;
				CreateNewResult(FILEDEL, lpFC1, NULL);
			}
			else 
			{
				CompareResult.stcChanged.cDirs++;
				CompareResult.stcDeleted.cDirs++;
				CreateNewResult(DIRDEL, lpFC1, NULL);

				// "Compare"/Log sub files if any
				if (NULL != lpFC1->lpFirstSubFC) {
					CompareFiles(lpFC1->lpFirstSubFC, NULL);
				}
			}
		}
	}

	// After looping all FC1 files, do an extra loop over all FC2 files and check previously set match flags to determine added dirs/files
	for (lpFC2 = lpStartFC2; NULL != lpFC2; lpFC2 = lpFC2->lpBrotherFC) 
	{
		// skip FC2 if already matched
		if (NOMATCH != lpFC2->fFileMatch) {
			continue;
		}

		// FC2 has no matching FC1, so FC2 is an added dir/file
		if (ISFILE(lpFC2->nFileAttributes)) 
		{
			CompareResult.stcCompared.cFiles++;
			CompareResult.stcChanged.cFiles++;
			CompareResult.stcAdded.cFiles++;
			CreateNewResult(FILEADD, NULL, lpFC2);
		}
		else 
		{
			CompareResult.stcCompared.cDirs++;
			CompareResult.stcChanged.cDirs++;
			CompareResult.stcAdded.cDirs++;
			CreateNewResult(DIRADD, NULL, lpFC2);

			// "Compare"/Log sub files if any
			if (NULL != lpFC2->lpFirstSubFC) {
				CompareFiles(NULL, lpFC2->lpFirstSubFC);
			}
		}
	}
}

//-------------------------------------------------------------
// Directory and file comparison engine
//-------------------------------------------------------------
VOID CompareHeadFiles(LPHEADFILE lpStartHF1, LPHEADFILE lpStartHF2) 
{
	LPHEADFILE lpHF1;
	LPHEADFILE lpHF2;
	LPFILECONTENT lpFC;

	// first loop
	for (lpHF1 = lpStartHF1; NULL != lpHF1; lpHF1 = lpHF1->lpBrotherHF) {
		// Check that first file name present
		if (NULL == lpHF1->lpFirstFC) {
			continue;
		}
		if (NULL == lpHF1->lpFirstFC->lpszFileName) {
			continue;
		}

		// Get corresponding headfile in second shot
		lpHF2 = SearchDirChain(lpHF1->lpFirstFC->lpszFileName, lpStartHF2);
		lpFC = NULL;
		if (NULL != lpHF2) {
			lpHF2->fHeadFileMatch = ISMATCH;
			lpFC = lpHF2->lpFirstFC;
		}
		CompareFiles(lpHF1->lpFirstFC, lpFC);
	}

	// second loop, only those that did not match before
	for (lpHF2 = lpStartHF2; NULL != lpHF2; lpHF2 = lpHF2->lpBrotherHF) {
		// Check that not compared in first loop
		if (NOMATCH != lpHF2->fHeadFileMatch) {
			continue;
		}
		// Check that first file name present
		if (NULL == lpHF2->lpFirstFC) {
			continue;
		}
		if (NULL == lpHF2->lpFirstFC->lpszFileName) {
			continue;
		}

		// Check that there's no corresponding headfile in first shot
		lpHF1 = SearchDirChain(lpHF2->lpFirstFC->lpszFileName, lpStartHF1);
		if (NULL == lpHF1) {
			CompareFiles(NULL, lpHF2->lpFirstFC);
		}
	}
}

// ----------------------------------------------------------------------
// Get file snap shot
// ----------------------------------------------------------------------
VOID GetFilesSnap(LPSNAPSHOT lpShot, LPTSTR lpszFullName, LPFILECONTENT lpFatherFC, LPFILECONTENT *lplpCaller) 
{
	LPFILECONTENT lpFC;
	HANDLE hFile;
	// Full dir/file name is already given
	// Extra local block to reduce stack usage due to recursive calls
	{
		LPTSTR lpszFindFileName;
		lpszFindFileName = NULL;
		// Get father file data if not already provided (=called from FileShot)
		// lpFatherFC only equals "C:\" and is called once at start
		if (NULL == lpFatherFC) 
		{
			
			// Check if file is to be GENERIC excluded
			if ((NULL == lpszFullName)
				|| (((TCHAR)'.' == lpszFullName[0])  // fast exclusion for 99% of the cases
				&& ((0 == _tcscmp(lpszFullName, TEXT(".")))
				|| (0 == _tcscmp(lpszFullName, TEXT("..")))))) {
				return;
			}

			// Create new file content
			lpFatherFC = MYALLOC0(sizeof(FILECONTENT));

			// Set file name length
			lpFatherFC->cchFileName = _tcslen(lpszFullName);

			// Copy file name to new buffer for directory search and more
			lpszFindFileName = MYALLOC((lpFatherFC->cchFileName + 4 + 1) * sizeof(TCHAR));  // +4 for "\*.*" search when directory (later in routine)
			_tcscpy(lpszFindFileName, lpszFullName);
			// Special case if root dir of a drive was specified, needs trailing backslash otherwise current dir of that drive is used
			if ((TCHAR)':' == lpszFindFileName[lpFatherFC->cchFileName - 1]) {
				lpszFindFileName[lpFatherFC->cchFileName] = (TCHAR)'\\';
				lpszFindFileName[lpFatherFC->cchFileName + 1] = (TCHAR)'\0';
			}

			hFile = FindFirstFile(lpszFullName, &FindData);
			if (INVALID_HANDLE_VALUE != hFile) {
				FindClose(hFile);
			}
			else {
				// Workaround for some cases in Windows Vista and later
				ZeroMemory(&FindData, sizeof(FindData));
				
				hFile = CreateFile(lpszFullName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
				if (INVALID_HANDLE_VALUE != hFile) {
					BY_HANDLE_FILE_INFORMATION FileInformation;
					BOOL bResult;

					bResult = GetFileInformationByHandle(hFile, &FileInformation);
					if (bResult) {
						FindData.dwFileAttributes = FileInformation.dwFileAttributes;
						FindData.ftCreationTime = FileInformation.ftCreationTime;
						FindData.ftLastAccessTime = FileInformation.ftLastAccessTime;
						FindData.ftLastWriteTime = FileInformation.ftLastWriteTime;
						FindData.nFileSizeHigh = FileInformation.nFileSizeHigh;
						FindData.nFileSizeLow = FileInformation.nFileSizeLow;
					}
					else {
						FindData.dwFileAttributes = GetFileAttributes(lpszFullName);
						if (INVALID_FILE_ATTRIBUTES == FindData.dwFileAttributes) {
							FindData.dwFileAttributes = 0;
						}
						bResult = GetFileTime(hFile, &FindData.ftCreationTime, &FindData.ftLastAccessTime, &FindData.ftLastWriteTime);
						if (!bResult) {
							FindData.ftCreationTime.dwLowDateTime = 0;
							FindData.ftCreationTime.dwHighDateTime = 0;
							FindData.ftLastAccessTime.dwLowDateTime = 0;
							FindData.ftLastAccessTime.dwHighDateTime = 0;
							FindData.ftLastWriteTime.dwLowDateTime = 0;
							FindData.ftLastWriteTime.dwHighDateTime = 0;
						}
						FindData.nFileSizeLow = GetFileSize(hFile, &FindData.nFileSizeHigh);
						if (INVALID_FILE_SIZE == FindData.nFileSizeLow) {
							FindData.nFileSizeHigh = 0;
							FindData.nFileSizeLow = 0;
						}
					}
					CloseHandle(hFile);
				}
			}

			// Remove previously added backslash (if any)
			lpszFindFileName[lpFatherFC->cchFileName] = (TCHAR)'\0';

			// Copy pointer to current file into caller's pointer
			if (NULL != lplpCaller) {
				*lplpCaller = lpFatherFC;
			}
			          
			// Increase dir/file count
			if (ISFILE(FindData.dwFileAttributes)) {
				lpShot->stCounts.cFiles++;
			}
			else {
				lpShot->stCounts.cDirs++;
			}

			// Copy file name
			lpFatherFC->lpszFileName = MYALLOC((lpFatherFC->cchFileName + 1) * sizeof(TCHAR));
			_tcscpy(lpFatherFC->lpszFileName, lpszFullName);

			// Copy file data
			lpFatherFC->nWriteDateTimeLow = FindData.ftLastWriteTime.dwLowDateTime;
			lpFatherFC->nWriteDateTimeHigh = FindData.ftLastWriteTime.dwHighDateTime;
			lpFatherFC->nAccessDateTimeLow = FindData.ftLastWriteTime.dwLowDateTime;
			lpFatherFC->nAccessDateTimeHigh = FindData.ftLastWriteTime.dwHighDateTime;
			lpFatherFC->nFileSizeLow = FindData.nFileSizeLow;
			lpFatherFC->nFileSizeHigh = FindData.nFileSizeHigh;
			lpFatherFC->nFileAttributes = FindData.dwFileAttributes;

			// Set "lpFirstSubFC" pointer for storing the first child's pointer
			lplpCaller = &lpFatherFC->lpFirstSubFC;
		}

		// If father is a file, then leave (=special case when called from FileShot)
		if (ISFILE(lpFatherFC->nFileAttributes)) {
			if (NULL != lpszFindFileName) {
				MYFREE(lpszFindFileName);
			}
			return;
		}

		// Process all entries of directory
		// a) Create search pattern and start search
		if (NULL == lpszFindFileName) {
			lpszFindFileName = lpszFullName;
		}
		_tcscat(lpszFindFileName, TEXT("\\*.*"));
		hFile = FindFirstFile(lpszFindFileName, &FindData);
		if (lpszFindFileName != lpszFullName) {
			MYFREE(lpszFindFileName);
		}
	}  // End of extra local block
	if (INVALID_HANDLE_VALUE == hFile) {  
		// error: nothing in dir, no access, etc.
		return;
	}
	// b) process entry then find next
	do {
		lpszFullName = NULL;
		BOOL calculateHash = TRUE;
		BOOL found = FALSE;

		// Check if file is to be GENERIC excluded (dot dirs)
		if ((NULL == FindData.cFileName)
			|| (((TCHAR)'.' == FindData.cFileName[0])  // fast exclusion for 99% of the cases
			&& ((0 == _tcscmp(FindData.cFileName, TEXT(".")))
			|| (0 == _tcscmp(FindData.cFileName, TEXT("..")))))) {
			continue;  // ignore this entry and continue with next file
		}

		// Create new file content
		lpFC = MYALLOC0(sizeof(FILECONTENT));
		// Set father of current key
		lpFC->lpFatherFC = lpFatherFC;
		// Set file name length
		lpFC->cchFileName = _tcslen(FindData.cFileName);

		// Allocate memory copy file name to FILECONTENT
		lpFC->lpszFileName = MYALLOC0((lpFC->cchFileName + 1) * sizeof(TCHAR));
		_tcscpy(lpFC->lpszFileName, FindData.cFileName);

		// Static blacklist for directories
		if (ISDIR(FindData.dwFileAttributes))
		{
			if (performStaticBlacklisting)
			{
				LPTSTR lpszFullPath;
				lpszFullPath = GetWholeFileName(lpFC, 4);

				found = TrieSearchPath(blacklistDIRS->children, lpszFullPath);
				if (found)
				{
					lpShot->stCounts.cFilesBlacklist++;
					MYFREE(lpszFullPath);
					FreeAllFileContents(lpFC);

					// Increase value count for display purposes
					lpShot->stCounts.cDirsBlacklist++;
					
					// Ignore this entry and continue with next brother value
					continue;  
				}
				else
				{
					MYFREE(lpszFullPath);
				}
			}
		}

		// Check if the file system entry is a symbolic link
		// If so, skip as it actually resides somewhere else on the file system!
		if (ISSYM(FindData.dwFileAttributes)) {
			if (ISFILE(FindData.dwFileAttributes)) {
				lpShot->stCounts.cFilesBlacklist++;
			}
			else {
				lpShot->stCounts.cDirsBlacklist++;
			}


			continue;
		}

		// Blacklisting implementation for files		
		if (ISFILE(FindData.dwFileAttributes))
		{
			if (dwBlacklist == 1)
			{
				// First snapshot, therefore populate the Trie (Prefix Tree)
				// Get the full file path
				LPTSTR lpszFullPath;
				lpszFullPath = GetWholeFileName(lpFC, 4);

				// Add full path to file blacklist prefix tree, then free path
				TrieAdd(&blacklistFILES, lpszFullPath);
				MYFREE(lpszFullPath);

				// Increase value count for display purposes
				lpShot->stCounts.cFiles++;

				// Ignore this entry and continue with next brother value
				//continue;  
			}
			else if (dwBlacklist == 2)
			{
				// Not the first snapshot, so either:
				// 1) If blacklisting enabled: Ignore file if its in the blacklist
				// 2) If hashing enabled (and not blacklisting): Mark file as not to be hashed
				LPTSTR lpszFullPath;
				lpszFullPath = GetWholeFileName(lpFC, 4);
				found = TrieSearchPath(blacklistFILES->children, lpszFullPath);
				if (found)
				{
					if (performDynamicBlacklisting)
					{
						MYFREE(lpszFullPath);
						FreeAllFileContents(lpFC);

						// Increase value count for display purposes
						lpShot->stCounts.cFilesBlacklist++;

						// Ignore this entry and continue with next brother value
						continue;  
					}
					if (performSHA1Hashing || performMD5Hashing)
					{
						lpShot->stCounts.cFiles++;
						MYFREE(lpszFullPath);
						calculateHash = FALSE;
					}
				}
				else
				{
					MYFREE(lpszFullPath);
				}
			}
		}
					
		// Copy pointer to current file into caller's pointer
		if (NULL != lplpCaller) {
			*lplpCaller = lpFC;
		}

		// Increase dir/file count
		if (ISFILE(FindData.dwFileAttributes)) {
			lpShot->stCounts.cFiles++;
		}
		else {
			lpShot->stCounts.cDirs++;
		}

		// Copy file data
		lpFC->nWriteDateTimeLow = FindData.ftLastWriteTime.dwLowDateTime;
		lpFC->nWriteDateTimeHigh = FindData.ftLastWriteTime.dwHighDateTime;
		lpFC->nAccessDateTimeLow = FindData.ftLastWriteTime.dwLowDateTime;
		lpFC->nAccessDateTimeHigh = FindData.ftLastWriteTime.dwHighDateTime;
		lpFC->nFileSizeLow = FindData.nFileSizeLow;
		lpFC->nFileSizeHigh = FindData.nFileSizeHigh;
		lpFC->nFileAttributes = FindData.dwFileAttributes;

		// Calculate file hash (computationally intensive!)
		// This should only be executed in the following scenarios:
		// 1) If the file system entry is a data file 
		// 2) If the data file is not blacklisted (previously known)
		if (ISFILE(FindData.dwFileAttributes))
		{
			if (dwBlacklist == 2 && calculateHash) {
			    if (performSHA1Hashing)
			    {
				    lpFC->cchSHA1 = 40;
				    lpFC->lpszSHA1 = MYALLOC((lpFC->cchSHA1 + 1) * sizeof(TCHAR));
				    _tcscpy(lpFC->lpszSHA1, CalculateSHA1(GetWholeFileName(lpFC, 4)));
                }
                if (performMD5Hashing)
                {
				    lpFC->cchMD5 = 32;
				    lpFC->lpszMD5 = MYALLOC((lpFC->cchMD5 + 1) * sizeof(TCHAR));
				    _tcscpy(lpFC->lpszMD5, CalculateMD5(GetWholeFileName(lpFC, 4)));
				}
				if (performMD5BlockHashing)
				{
					LPMD5BLOCK theMD5Block = CalculateMD5Blocks(GetWholeFileName(lpFC, 4));
					lpFC->lpMD5Block = MYALLOC0(sizeof(MD5BLOCK));
					lpFC->lpMD5Block = theMD5Block;
				}
			}
		}

		// Print file system shot status
		if ((dwBlacklist == 2 && performDynamicBlacklisting) || (performStaticBlacklisting)) {
			printf("  > Dirs: %d  Files: %d  Blacklisted Dirs: %d  Blacklisted Files: %d\r", lpShot->stCounts.cDirs, 
				lpShot->stCounts.cFiles,
				lpShot->stCounts.cDirsBlacklist,
				lpShot->stCounts.cFilesBlacklist);
		}
		else {
			printf("  > Dirs: %d  Files: %d\r", lpShot->stCounts.cDirs, lpShot->stCounts.cFiles);
		}

		// ATTENTION!!! FindData will be INVALID from this point on, due to recursive calls
		// If the entry is a directory, then do a recursive call for it
		// Pass this entry as father and "lpFirstSubFC" pointer for storing the first child's pointer
		if (ISDIR(lpFC->nFileAttributes)) {
			if (NULL == lpszFullName) {
				lpszFullName = GetWholeFileName(lpFC, 4);  // +4 for "\*.*" search (in recursive call)
			}
			GetFilesSnap(lpShot, lpszFullName, lpFC, &lpFC->lpFirstSubFC);
		}

		if (NULL != lpszFullName) {
			MYFREE(lpszFullName);
		}

		// Set "lpBrotherFC" pointer for storing the next brother's pointer
		lplpCaller = &lpFC->lpBrotherFC;
	} while (FindNextFile(hFile, &FindData));
	FindClose(hFile);
}

// ----------------------------------------------------------------------
// File Shot Engine
// ----------------------------------------------------------------------
VOID FileShot(LPSNAPSHOT lpShot) 
{
	UINT cchExtDir;
	// Determine the Windows directory for file system scanning
	lpszExtDir = MYALLOC0(MAX_PATH * sizeof(TCHAR));
	lpszWindowsDirName = MYALLOC0(MAX_PATH * sizeof(TCHAR));
	GetSystemWindowsDirectory(lpszWindowsDirName, MAX_PATH);  // length incl. NULL character
	GetVolumePathName(lpszWindowsDirName, lpszExtDir, MAX_PATH);
	lpszExtDir[MAX_PATH] = (TCHAR)'\0';
	// Set the length of the Windows directory
	cchExtDir = _tcslen(lpszExtDir);

	if (0 < cchExtDir) {
		LPHEADFILE *lplpHFPrev;
		LPTSTR lpszSubExtDir;
		size_t i;

		lplpHFPrev = &lpShot->lpHF;
		lpszSubExtDir = lpszExtDir;
		for (i = 0; i <= cchExtDir; i++) {
			// Split each directory in string
			if (((TCHAR)';' == lpszExtDir[i]) || ((TCHAR)'\0' == lpszExtDir[i])) {
				LPHEADFILE lpHF;
				size_t j;

				lpszExtDir[i] = (TCHAR)'\0';
				j = i;

				// remove all trailing spaces and backslashes
				while (0 < j) {
					--j;
					if (((TCHAR)'\\' == lpszExtDir[j]) || ((TCHAR)' ' == lpszExtDir[j])) {
						lpszExtDir[j] = (TCHAR)'\0';
					}
					else {
						break;
					}
				}

				// if anything is left then process this directory
				if ((0 < j) && ((TCHAR)'\0' != lpszExtDir[j])) {
					lpHF = MYALLOC0(sizeof(HEADFILE));

					*lplpHFPrev = lpHF;
					lplpHFPrev = &lpHF->lpBrotherHF;

					GetFilesSnap(lpShot, lpszSubExtDir, NULL, &lpHF->lpFirstFC);
				}

				lpszSubExtDir = &lpszExtDir[i + 1];
			}
		}
	}

	// Update total count of all items
	lpShot->stCounts.cAll = lpShot->stCounts.cDirs + lpShot->stCounts.cFiles;
	// Print final file system shot count
	if ((dwBlacklist == 2 && performDynamicBlacklisting) || (performStaticBlacklisting)) {
		printf("  > Dirs: %d  Files: %d  Blacklisted Dirs: %d  Blacklisted Files: %d\n", lpShot->stCounts.cDirs,
			lpShot->stCounts.cFiles,
			lpShot->stCounts.cDirsBlacklist,
			lpShot->stCounts.cFilesBlacklist);
	}
	else {
		printf("  > Dirs: %d  Files: %d\n", lpShot->stCounts.cDirs, lpShot->stCounts.cFiles);
	}
}

// ----------------------------------------------------------------------
// Clear comparison match flags of directories and files
// ----------------------------------------------------------------------
VOID ClearFileMatchFlags(LPFILECONTENT lpFC)
{
	for (; NULL != lpFC; lpFC = lpFC->lpBrotherFC) {
		lpFC->fFileMatch = NOMATCH;
		if (NULL != lpFC->lpFirstSubFC) {
			ClearFileMatchFlags(lpFC->lpFirstSubFC);
		}
	}
}

// ----------------------------------------------------------------------
// Clear comparison match flags in all head files
// ----------------------------------------------------------------------
VOID ClearHeadFileMatchFlags(LPHEADFILE lpHF) 
{
	for (; NULL != lpHF; lpHF = lpHF->lpBrotherHF) {
		lpHF->fHeadFileMatch = NOMATCH;
		if (NULL != lpHF->lpFirstFC) {
			ClearFileMatchFlags(lpHF->lpFirstFC);
		}
	}
}

//--------------------------------------------------
// Walkthrough head file chain and collect it's first dirname to lpszDir
//--------------------------------------------------
BOOL FindDirChain(LPHEADFILE lpHF, LPTSTR lpszDir, size_t nBufferLen) 
{
	size_t      nLen;
	size_t      nWholeLen;
	BOOL        fAddSeparator;
	BOOL        fAddBackslash;

	lpszDir[0] = (TCHAR)'\0';
	nWholeLen = 0;
	for (; NULL != lpHF; lpHF = lpHF->lpBrotherHF) {
		if ((NULL != lpHF->lpFirstFC)
			&& (NULL != lpHF->lpFirstFC->lpszFileName)) {
			nLen = lpHF->lpFirstFC->cchFileName;
			if (0 < nLen) {
				fAddSeparator = FALSE;
				if (0 < nWholeLen) {
					nLen++;
					fAddSeparator = TRUE;
				}
				fAddBackslash = FALSE;
				if ((TCHAR)':' == lpHF->lpFirstFC->lpszFileName[lpHF->lpFirstFC->cchFileName - 1]) {
					nLen++;
					fAddBackslash = TRUE;
				}
				nWholeLen += nLen;
				if (nWholeLen < nBufferLen) {
					if (fAddSeparator) {
						_tcscat(lpszDir, TEXT(";"));
					}
					_tcscat(lpszDir, lpHF->lpFirstFC->lpszFileName);
					if (fAddBackslash) {
						_tcscat(lpszDir, TEXT("\\"));
					}
				}
				else {
					//nWholeLen -= nLen;
					return FALSE;  // buffer too small, exit for-loop and routine
				}
			}
		}
	}
	return TRUE;
}

//--------------------------------------------------
// if two dir chains are the same
//--------------------------------------------------
BOOL DirChainMatch(LPHEADFILE lpHF1, LPHEADFILE lpHF2)
{
	TCHAR lpszDir1[EXTDIRLEN];
	TCHAR lpszDir2[EXTDIRLEN];

	FindDirChain(lpHF1, lpszDir1, EXTDIRLEN);  // Length in TCHARs incl. NULL char
	FindDirChain(lpHF2, lpszDir2, EXTDIRLEN);  // Length in TCHARs incl. NULL char

	if (0 != _tcsicmp(lpszDir1, lpszDir2)) {
		return FALSE;
	}

	return TRUE;
}
