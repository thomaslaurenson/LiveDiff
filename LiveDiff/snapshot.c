/*
Copyright 2016 Thomas Laurenson
thomaslaurenson.com

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

#define MAX_SIGNATURE_LENGTH 16
#define LIVEDIFF_READ_BLOCK_SIZE 8192

// Handle of file for saving snapshots
HANDLE hFileSnapshotFile;

// Structure variables for saving snapshots
SAVEKEYCONTENT sKC;
SAVEVALUECONTENT sVC;
LIVEDIFFHEADER livediffheader;

// LiveDiff snapshot header signature (magic number)
char lpszLiveDiffSignature[] = "LIVEDIFF_HEADER";

// ----------------------------------------------------------------------
// Save file system and Registry entries to LiveDiff snapshot (.shot) file
// ----------------------------------------------------------------------
VOID SaveSnapShot(LPSNAPSHOT lpShot, LPTSTR lpszFileName)
{
	DWORD nFPCurrent;

	// Check snapshot for content, return if there is nothing to save
	if ((NULL == lpShot->lpHKLM) && (NULL == lpShot->lpHKU) && (NULL == lpShot->lpHF)) {
		return;
	}

	// Open snapshot file for writing
	hFileSnapshotFile = CreateFile(lpszFileName, 
		GENERIC_WRITE, 
		0, 
		NULL, 
		CREATE_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	// Check file handle
	if (INVALID_HANDLE_VALUE == hFileSnapshotFile) {
		printf(">>> ERROR: Creating snapshot file failed.\n");
		return;
	}

	// Initialize file header
	ZeroMemory(&livediffheader, sizeof(LIVEDIFFHEADER));

	// Copy signature to header
	strncpy(livediffheader.signature, lpszLiveDiffSignature, MAX_SIGNATURE_LENGTH);

	// Copy size of header to the header
	livediffheader.nFHSize = sizeof(livediffheader);

	// Set file positions of hives inside the file
	livediffheader.ofsHKLM = 0;   // not known yet, may be empty
	livediffheader.ofsHKU = 0;    // not known yet, may be empty
	livediffheader.ofsHF = 0;  // not known yet, may be empty

	// Copy system time to header
	CopyMemory(&livediffheader.systemtime, &lpShot->systemtime, sizeof(SYSTEMTIME));

	// Write header to file
	WriteFile(hFileSnapshotFile, &livediffheader, sizeof(livediffheader), &NBW, NULL);

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
	CloseHandle(hFileSnapshotFile);
}

// ----------------------------------------------------------------------
// Load file system and Registry entries to LiveDiff snapshot (.shot) file
// ----------------------------------------------------------------------
BOOL LoadSnapShot(LPSNAPSHOT lpShot, LPTSTR lpszFileName)
{
	DWORD cbFileSize;
	DWORD cbFileRemain;
	DWORD cbReadSize;
	DWORD cbFileRead;

	printf("\n>>> Attempting to load snapshot.\n");

	// Open file for reading
	hFileSnapshotFile = CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hFileSnapshotFile) {
		printf("\n>>> ERROR opening file.\n");
		printf("Error %u", GetLastError());
		return FALSE;
	}

	cbFileSize = GetFileSize(hFileSnapshotFile, NULL);
	if (sizeof(livediffheader) > cbFileSize) {
		CloseHandle(hFileSnapshotFile);
		printf("\n>>> ERROR matching file size.\n");
		return FALSE;
	}

	// Initialize file header
	ZeroMemory(&livediffheader, sizeof(livediffheader));
	//ZeroMemory(&fileextradata, sizeof(fileextradata));

	// Read first part of file header from file (signature, nHeaderSize)
	ReadFile(hFileSnapshotFile, &livediffheader, offsetof(LIVEDIFFHEADER, ofsHKLM), &NBW, NULL);

	printf("HEADER: %s", livediffheader.signature);

	// Check file signature for correct type (SBCS/MBCS or UTF-16)
	if (0 != strncmp(lpszLiveDiffSignature, livediffheader.signature, MAX_SIGNATURE_LENGTH)) {
		CloseHandle(hFileSnapshotFile);
		printf("\n>>> ERROR: This is not a snapshot (.shot) file.\n");
		return FALSE;
	}

	// Clear shot
	FreeShot(lpShot);

	// Allocate memory to hold the complete file
	lpFileBuffer = MYALLOC(cbFileSize);

	// Read file blockwise for progress bar
	SetFilePointer(hFileSnapshotFile, 0, NULL, FILE_BEGIN);
	cbFileRemain = cbFileSize;  // 100% to go
	cbReadSize = LIVEDIFF_READ_BLOCK_SIZE;  // next block length to read
	for (cbFileRead = 0; 0 < cbFileRemain; cbFileRead += cbReadSize) {
		// If the rest is smaller than a block, then use the rest length
		if (LIVEDIFF_READ_BLOCK_SIZE > cbFileRemain) {
			cbReadSize = cbFileRemain;
		}

		// Read the next block
		ReadFile(hFileSnapshotFile, lpFileBuffer + cbFileRead, cbReadSize, &NBW, NULL);
		if (NBW != cbReadSize) {
			CloseHandle(hFileSnapshotFile);
			printf("\n>>> ERROR: Cannot read the snapshot file.\n");
			return FALSE;
		}

		// Determine how much to go, if zero leave the for-loop
		cbFileRemain -= cbReadSize;
		if (0 == cbFileRemain) {
			break;
		}
	}

	// Close file
	CloseHandle(hFileSnapshotFile);

	nSourceSize = sizeof(livediffheader);

	// Copy file header to structure
	CopyMemory(&livediffheader, lpFileBuffer, nSourceSize);

	// Enhance data of old headers to be used with newer code
	if ((0 != livediffheader.ofsHKLM) && (livediffheader.ofsHKU == livediffheader.ofsHKLM)) {
		livediffheader.ofsHKLM = 0;
	}
	if ((0 != livediffheader.ofsHKU) && (livediffheader.ofsHF == livediffheader.ofsHKU)) {
		livediffheader.ofsHKU = 0;
	}

	livediffheader.nKCSize = offsetof(SAVEKEYCONTENT, nKeyNameLen);
	livediffheader.nVCSize = offsetof(SAVEVALUECONTENT, nValueNameLen);
	livediffheader.nHFSize = sizeof(SAVEHEADFILE);  // not changed yet, if it is then adopt to offsetof(SAVEHEADFILE, <first new field>)
	livediffheader.nFCSize = offsetof(SAVEFILECONTENT, nFileNameLen);

	// Check structure boundaries
	if (sizeof(SAVEKEYCONTENT) < livediffheader.nKCSize) {
		livediffheader.nKCSize = sizeof(SAVEKEYCONTENT);
	}
	if (sizeof(SAVEVALUECONTENT) < livediffheader.nVCSize) {
		livediffheader.nVCSize = sizeof(SAVEVALUECONTENT);
	}
	if (sizeof(SAVEHEADFILE) < livediffheader.nHFSize) {
		livediffheader.nHFSize = sizeof(SAVEHEADFILE);
	}
	if (sizeof(SAVEFILECONTENT) < livediffheader.nFCSize) {
		livediffheader.nFCSize = sizeof(SAVEFILECONTENT);
	}

	// New temporary string buffer
	lpStringBuffer = NULL;

	CopyMemory(&lpShot->systemtime, &livediffheader.systemtime, sizeof(SYSTEMTIME));

	// Initialize save structures
	ZeroMemory(&sKC, sizeof(sKC));
	ZeroMemory(&sVC, sizeof(sVC));

	if (0 != livediffheader.ofsHKLM)
	{
		//Load(lpShot, livediffheader.ofsHKLM, NULL, &lpShot->lpHKLM);
		/*LoadRegistryEntries*/
	}

	if (0 != livediffheader.ofsHKU)
	{
		LoadRegKeys(lpShot, livediffheader.ofsHKU, NULL, &lpShot->lpHKU);
	}

	if (0 != livediffheader.ofsHF)
	{
		LoadHeadFiles(lpShot, livediffheader.ofsHF, &lpShot->lpHF);
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