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

SAVEFILECONTENT sFC;
SAVEHEADFILE sHF;
WIN32_FIND_DATA FindData;

LPTSTR lpszExtDir;
LPTSTR lpszWindowsDirName;

// Set up buffer sizes for file hashing
#define BUFSIZE		1024
#define SHA1LEN		20

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
			_sntprintf(sha1HashString + (i * 2), 2, TEXT("%02X\0"), rgbHash[i]);
		}
	}
	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);
	CloseHandle(hashFile);
	return sha1HashString;
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
	//wprintf(L"%s\n", lpszFullName);
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

		// Blacklisting implementation for files		
		BOOL found = FALSE;
		if (ISFILE(FindData.dwFileAttributes))
		{
			if (dwBlacklist == 1)
			{
				// First snapshot, therefore populate the Trie (Prefix Tree)
				LPTSTR lpszFullPath;
				lpszFullPath = GetWholeFileName(lpFC, 4);
				TrieAdd(&blacklistFILES, lpszFullPath);
				MYFREE(lpszFullPath);
			}
			else if (dwBlacklist == 2)
			{
				// Not the first snapshot, so filter known paths
				LPTSTR lpszFullPath;
				lpszFullPath = GetWholeFileName(lpFC, 4);
				found = TrieSearch1(blacklistFILES->children, lpszFullPath);
				//printf("%s\n", found ? "true" : "false");
				if (found) {
					lpShot->stCounts.cFilesBlacklist++;
					MYFREE(lpszFullPath);
					FreeAllFileContents(lpFC);
					continue;  // ignore this entry and continue with next brother value
				}
				else {
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
		lpFC->nFileSizeLow = FindData.nFileSizeLow;
		lpFC->nFileSizeHigh = FindData.nFileSizeHigh;
		lpFC->nFileAttributes = FindData.dwFileAttributes;

		// Calculate SHA1 of file (computationally intensive!)
		// This should only be included in the following scenarios:
		// 1) The user specified the "-c" command line argument without a blacklist
		// 2) If the file is not found in the BST and blacklist filtering is specified
		if (ISFILE(FindData.dwFileAttributes))
		{
			if (dwBlacklist == 2) {
				lpFC->cchSHA1 = 40;
				lpFC->lpszSHA1 = MYALLOC((lpFC->cchSHA1 + 1) * sizeof(TCHAR));
				_tcscpy(lpFC->lpszSHA1, CalculateSHA1(GetWholeFileName(lpFC, 4)));
			}
		}

		// Print file system shot status
		if (dwBlacklist == 2) {
			printf("  > Dirs: %d  Files: %d  Blacklisted Files: %d\r", lpShot->stCounts.cDirs, lpShot->stCounts.cFiles, lpShot->stCounts.cFilesBlacklist);
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
	//_tcscat(lpszExtDir, TEXT("testdata")); // Temporary test directory, to save scanning the entire file system
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
	lpShot->stCounts.cAll = lpShot->stCounts.cDirs
		+ lpShot->stCounts.cFiles;
	// Print final file system shot count
	printf("  > Dirs:  %d  Files: %d\n", lpShot->stCounts.cDirs, lpShot->stCounts.cFiles);
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

// ----------------------------------------------------------------------
// Save file to HIVE File
//
// This routine is called recursively to store the entries of the dir/file tree
// Therefore temporary vars are put in a local block to reduce stack usage
// ----------------------------------------------------------------------
VOID SaveFiles(LPSNAPSHOT lpShot, LPFILECONTENT lpFC, DWORD nFPFatherFile, DWORD nFPCaller) 
{
	DWORD nFPFile;

	for (; NULL != lpFC; lpFC = lpFC->lpBrotherFC) {
		// Get current file position, put in a separate var for later use
		nFPFile = SetFilePointer(hFileWholeReg, 0, NULL, FILE_CURRENT);
		
		// Write position of current file in caller's field
		if (0 < nFPCaller) {
			SetFilePointer(hFileWholeReg, nFPCaller, NULL, FILE_BEGIN);
			WriteFile(hFileWholeReg, &nFPFile, sizeof(nFPFile), &NBW, NULL);
			SetFilePointer(hFileWholeReg, nFPFile, NULL, FILE_BEGIN);
		}

		// Initialize file content, copy values
		sFC.nWriteDateTimeLow = lpFC->nWriteDateTimeLow;
		sFC.nWriteDateTimeHigh = lpFC->nWriteDateTimeHigh;
		sFC.nFileSizeLow = lpFC->nFileSizeLow;
		sFC.nFileSizeHigh = lpFC->nFileSizeHigh;
		sFC.nFileAttributes = lpFC->nFileAttributes;
		sFC.nChkSum = lpFC->nChkSum;

		// Set file positions of the relatives inside the tree
		sFC.ofsFileName = 0;      // not known yet, may be defined in this call
		sFC.ofsSHA1 = 0;		  // For SHA1 hash sting
		// File name will always be stored behind the structure, so its position is already known
		sFC.ofsFileName = nFPFile + sizeof(sFC);
		// File hash string is behind the filename, so we also know the position (file offset + file name length)
		sFC.ofsSHA1 = sFC.ofsFileName + lpFC->cchFileName; // Is this needed?!
		// Set offsets for related files
		sFC.ofsFirstSubFile = 0;  // not known yet, may be re-written by another recursive call
		sFC.ofsBrotherFile = 0;   // not known yet, may be re-written in another iteration
		sFC.ofsFatherFile = nFPFatherFile;

		// Initialise the file name and SHA1 length
		sFC.nFileNameLen = 0;
		sFC.nFileSHA1Len = 0;

		// Determine file name length
		if (NULL != lpFC->lpszFileName) {
			sFC.nFileNameLen = (DWORD)lpFC->cchFileName;
			if (0 < sFC.nFileNameLen) {  // otherwise leave it all 0
				sFC.nFileNameLen++;  // account for NULL char
				// File name will always be stored behind the structure, so its position is already known
				sFC.ofsFileName = nFPFile + sizeof(sFC);
			}
		}

		// Determine file sha1 length
		if (NULL != lpFC->lpszSHA1) {
			sFC.nFileSHA1Len = (DWORD)lpFC->cchSHA1;
			if (0 < sFC.nFileSHA1Len) {  // otherwise leave it all 0
				sFC.nFileSHA1Len++;  // account for NULL char
				// File hash will always be stored behind the , so its position is already known
				sFC.ofsSHA1 = sFC.ofsFileName + sFC.nFileNameLen * sizeof(TCHAR);
			}
		}

		// Write file content to file
		WriteFile(hFileWholeReg, &sFC, sizeof(sFC), &NBW, NULL);

		// Write file name to file
		if (0 < sFC.nFileNameLen) {
			WriteFile(hFileWholeReg, lpFC->lpszFileName, sFC.nFileNameLen * sizeof(TCHAR), &NBW, NULL);
		}
		else {
			// Write empty string for backward compatibility
			WriteFile(hFileWholeReg, TEXT(""), 1 * sizeof(TCHAR), &NBW, NULL); // used to be lpszEmpty, does this create issues?! (lpszEmpty)
		}

		// Write file SHA1 to file
		if (0 < sFC.nFileSHA1Len) {
			WriteFile(hFileWholeReg, lpFC->lpszSHA1, sFC.nFileSHA1Len * sizeof(TCHAR), &NBW, NULL);
		}
		else {
			// Write empty string for backward compatibility
			WriteFile(hFileWholeReg, TEXT(""), 1 * sizeof(TCHAR), &NBW, NULL);
		}

		// ATTENTION!!! sFC is INVALID from this point on, due to recursive calls
		// If the entry has childs, then do a recursive call for the first child
		// Pass this entry as father and "ofsFirstSubFile" position for storing the first child's position
		if (NULL != lpFC->lpFirstSubFC) {
			SaveFiles(lpShot, lpFC->lpFirstSubFC, nFPFile, nFPFile + offsetof(SAVEFILECONTENT, ofsFirstSubFile));
		}

		// Set "ofsBrotherFile" position for storing the following brother's position
		nFPCaller = nFPFile + offsetof(SAVEFILECONTENT, ofsBrotherFile);
	}
}

//--------------------------------------------------
// Save head file to HIVE file
//--------------------------------------------------
VOID SaveHeadFiles(LPSNAPSHOT lpShot, LPHEADFILE lpHF, DWORD nFPCaller) 
{
	DWORD nFPHF;
	// Write all head files and their file contents
	for (; NULL != lpHF; lpHF = lpHF->lpBrotherHF) {
		// Get current file position
		// put in a separate var for later use
		nFPHF = SetFilePointer(hFileWholeReg, 0, NULL, FILE_CURRENT);

		// Write position of current head file in caller's field
		if (0 < nFPCaller) {
			SetFilePointer(hFileWholeReg, nFPCaller, NULL, FILE_BEGIN);
			WriteFile(hFileWholeReg, &nFPHF, sizeof(nFPHF), &NBW, NULL);

			SetFilePointer(hFileWholeReg, nFPHF, NULL, FILE_BEGIN);
		}

		// Initialize head file
		// Set file positions of the relatives inside the tree
		sHF.ofsBrotherHeadFile = 0;   // not known yet, may be re-written in next iteration
		sHF.ofsFirstFileContent = 0;  // not known yet, may be re-written by SaveFiles call

		// Write head file to file
		// Make sure that ALL fields have been initialized/set
		WriteFile(hFileWholeReg, &sHF, sizeof(sHF), &NBW, NULL);

		// Write all file contents of head file
		if (NULL != lpHF->lpFirstFC) {
			SaveFiles(lpShot, lpHF->lpFirstFC, 0, nFPHF + offsetof(SAVEHEADFILE, ofsFirstFileContent));
		}

		// Set "ofsBrotherHeadFile" position for storing the following brother's position
		nFPCaller = nFPHF + offsetof(SAVEHEADFILE, ofsBrotherHeadFile);
	}
}

// ----------------------------------------------------------------------
// Load file from HIVE file
// ----------------------------------------------------------------------
VOID LoadFiles(LPSNAPSHOT lpShot, DWORD ofsFile, LPFILECONTENT lpFatherFC, LPFILECONTENT *lplpCaller) 
{
	LPFILECONTENT lpFC;
	DWORD ofsBrotherFile;

	// Read all files and their sub file contents
	for (; 0 != ofsFile; ofsFile = ofsBrotherFile) {
		// Copy SAVEFILECONTENT to aligned memory block
		CopyMemory(&sFC, (lpFileBuffer + ofsFile), fileheader.nFCSize);

		// Save offset in local variable
		ofsBrotherFile = sFC.ofsBrotherFile;

		// Create new file content
		// put in a separate var for later use
		lpFC = MYALLOC0(sizeof(FILECONTENT));

		// Set father of current file
		lpFC->lpFatherFC = lpFatherFC;

		// Copy file name
		if (fileextradata.bOldFCVersion) {  // old SBCS/MBCS version
			sFC.nFileNameLen = (DWORD)strlen((const char *)(lpFileBuffer + sFC.ofsFileName));
			if (0 < sFC.nFileNameLen) {
				sFC.nFileNameLen++;  // account for NULL char
			}
		}
		if (0 < sFC.nFileNameLen) {  // otherwise leave it NULL
			// Copy string to an aligned memory block
			nSourceSize = sFC.nFileNameLen * fileheader.nCharSize;
			nStringBufferSize = AdjustBuffer(&lpStringBuffer, nStringBufferSize, nSourceSize, REGSHOT_BUFFER_BLOCK_BYTES);
			ZeroMemory(lpStringBuffer, nStringBufferSize);
			CopyMemory(lpStringBuffer, (lpFileBuffer + sFC.ofsFileName), nSourceSize);

			lpFC->lpszFileName = MYALLOC(sFC.nFileNameLen * sizeof(TCHAR));
			if (fileextradata.bSameCharSize) {
				_tcsncpy(lpFC->lpszFileName, lpStringBuffer, sFC.nFileNameLen);
			}
			else {
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)lpStringBuffer, -1, lpFC->lpszFileName, sFC.nFileNameLen);
			}
			lpFC->lpszFileName[sFC.nFileNameLen - 1] = (TCHAR)'\0';  // safety NULL char

			// Set file name length in chars
			lpFC->cchFileName = _tcslen(lpFC->lpszFileName);
		}

		// Check if file is to be GENERIC excluded
		if ((NULL == lpFC->lpszFileName)
			|| (((TCHAR)'.' == lpFC->lpszFileName[0])  // fast exclusion for 99% of the cases
			&& ((0 == _tcscmp(lpFC->lpszFileName, TEXT(".")))
			|| (0 == _tcscmp(lpFC->lpszFileName, TEXT("..")))))) {
			FreeAllFileContents(lpFC);
			continue;  // ignore this entry and continue with next brother file
		}

		// Check if file is to be BLACKLIST excluded
		/*
		if (includeBlacklist) {
			LPTSTR lpszFullName;
			if (IsInFileBlacklist(lpFC->lpszFileName)) {
				FreeAllFileContents(lpFC);
				continue;  // ignore this entry and continue with next file
			}
			lpszFullName = GetWholeFileName(lpFC, 0);
			if (IsInFileBlacklist(lpszFullName)) {
				MYFREE(lpszFullName);
				FreeAllFileContents(lpFC);
				continue;  // ignore this entry and continue with next brother file
			}
			MYFREE(lpszFullName);
		}
		*/

		// Copy file hash
		if (fileextradata.bOldFCVersion) {  // old SBCS/MBCS version // Looks like this is not used
			sFC.nFileSHA1Len = (DWORD)strlen((const char *)(lpFileBuffer + sFC.ofsSHA1));
			if (0 < sFC.nFileSHA1Len) {
				sFC.nFileSHA1Len++;  // account for NULL char
			}
		}
		if (0 < sFC.nFileSHA1Len) {  // otherwise leave it NULL
			// Copy string to an aligned memory block
			nSourceSize = sFC.nFileSHA1Len * fileheader.nCharSize;
			nStringBufferSize = AdjustBuffer(&lpStringBuffer, nStringBufferSize, nSourceSize, REGSHOT_BUFFER_BLOCK_BYTES);
			ZeroMemory(lpStringBuffer, nStringBufferSize);
			CopyMemory(lpStringBuffer, (lpFileBuffer + sFC.ofsSHA1), nSourceSize);

			lpFC->lpszSHA1 = MYALLOC(sFC.nFileSHA1Len * sizeof(TCHAR));
			if (fileextradata.bSameCharSize) {
				_tcsncpy(lpFC->lpszSHA1, lpStringBuffer, sFC.nFileSHA1Len);
			}
			else {
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (LPCSTR)lpStringBuffer, -1, lpFC->lpszSHA1, sFC.nFileSHA1Len);
			}
			lpFC->lpszSHA1[sFC.nFileSHA1Len - 1] = (TCHAR)'\0';  // safety NULL char

			// Set file name length in chars
			lpFC->cchSHA1 = _tcslen(lpFC->lpszSHA1);
		}

		// Copy pointer to current file into caller's pointer
		if (NULL != lplpCaller) {
			*lplpCaller = lpFC;
		}

		// Increase dir/file count
		if (ISFILE(sFC.nFileAttributes)) {
			lpShot->stCounts.cFiles++;
		}
		else {
			lpShot->stCounts.cDirs++;
		}

		// Copy file data
		lpFC->nWriteDateTimeLow = sFC.nWriteDateTimeLow;
		lpFC->nWriteDateTimeHigh = sFC.nWriteDateTimeHigh;
		lpFC->nFileSizeLow = sFC.nFileSizeLow;
		lpFC->nFileSizeHigh = sFC.nFileSizeHigh;
		lpFC->nFileAttributes = sFC.nFileAttributes;
		lpFC->nChkSum = sFC.nChkSum;
		
		// ATTENTION!!! sFC will be INVALID from this point on, due to recursive calls
		// If the entry has childs, then do a recursive call for the first child
		// Pass this entry as father and "lpFirstSubFC" pointer for storing the first child's pointer
		if (0 != sFC.ofsFirstSubFile) {
			LoadFiles(lpShot, sFC.ofsFirstSubFile, lpFC, &lpFC->lpFirstSubFC);
		}

		// Set "lpBrotherFC" pointer for storing the next brother's pointer
		lplpCaller = &lpFC->lpBrotherFC;
	}
}

//--------------------------------------------------
// Load head file from HIVE file
//--------------------------------------------------
VOID LoadHeadFiles(LPSNAPSHOT lpShot, DWORD ofsHeadFile, LPHEADFILE *lplpCaller) 
{
	LPHEADFILE lpHF;

	// Initialize save structures
	ZeroMemory(&sHF, sizeof(sHF));
	ZeroMemory(&sFC, sizeof(sFC));

	// Read all head files and their file contents
	for (; 0 != ofsHeadFile; ofsHeadFile = sHF.ofsBrotherHeadFile) {
		// Copy SAVEHEADFILE to aligned memory block
		CopyMemory(&sHF, (lpFileBuffer + ofsHeadFile), fileheader.nHFSize);

		// Create new head file
		// put in a separate var for later use
		lpHF = MYALLOC0(sizeof(HEADFILE));

		// Copy pointer to current head file into caller's pointer
		if (NULL != lplpCaller) {
			*lplpCaller = lpHF;
		}

		// If the entry has file contents, then do a call for the first file content
		if (0 != sHF.ofsFirstFileContent) {
			LoadFiles(lpShot, sHF.ofsFirstFileContent, NULL, &lpHF->lpFirstFC);
		}

		// Set "lpBrotherHF" pointer for storing the next brother's pointer
		lplpCaller = &lpHF->lpBrotherHF;
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
