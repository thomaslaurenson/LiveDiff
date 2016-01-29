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

HANDLE hFileAPXML; // APXML report file handle for ouput
LANGUAGETEXT asLangTexts[cLangStrings]; // Set up language string for display

// ----------------------------------------------------------------------
// Parse LPCOMPRESULT to single file system entry, then pass to PopulateFileObject
// ----------------------------------------------------------------------
VOID ParseLPCompResultsFile(DWORD nActionType, LPCOMPRESULT lpStartCR)
{
	LPCOMPRESULT lpCR;
	// Loop through each item in CompareResults structure
	for (lpCR = lpStartCR; NULL != lpCR; lpCR = lpCR->lpNextCR) 
	{
		if (dwPrecisionLevel >= 1) {
			// Precision level 1: NEW files, directories
			if ((nActionType == DIRADD) || (nActionType == FILEADD) && (NULL != lpCR->lpContentNew)) {
					PopulateFileObject(hFileAPXML, nActionType, lpCR->lpContentNew);
			}
		}
		if (dwPrecisionLevel >= 2) {
			// Precision level 2: MODIFIED files, directories
			if ((nActionType == DIRMODI) || (nActionType == FILEMODI) && (NULL != lpCR->lpContentNew)) {
				PopulateFileObject(hFileAPXML, nActionType, lpCR->lpContentNew);
			}
		}
		if (dwPrecisionLevel >= 3) {
			// Precision level 3: CHANGED (properties) files, directories
			if ((nActionType == FILECHNG) && (NULL != lpCR->lpContentNew)) {
				PopulateFileObject(hFileAPXML, nActionType, lpCR->lpContentNew);
			}
		}

		// We are always going to output deleted content, therefore, populate FileObjects
		if ((nActionType == DIRDEL) || (nActionType == FILEDEL)) {
			if (NULL != lpCR->lpContentOld) {
				PopulateFileObject(hFileAPXML, nActionType, lpCR->lpContentOld);
			}
		}
	}
}

// ----------------------------------------------------------------------
// Parse LPCOMPRESULT to single Registry entry, then pass to PopulateCellObject
// ----------------------------------------------------------------------
VOID ParseLPCompResultsRegistry(DWORD nActionType, LPCOMPRESULT lpStartCR)
{
	LPCOMPRESULT lpCR;
	// Loop through each item in CompareResults structure
	for (lpCR = lpStartCR; NULL != lpCR; lpCR = lpCR->lpNextCR) 
	{
		if (dwPrecisionLevel >= 1) {
			// Precision level 1: NEW keys, values
			if ((nActionType == KEYADD) || (nActionType == VALADD) && (NULL != lpCR->lpContentNew)) {
				PopulateCellObject(hFileAPXML, nActionType, lpCR->lpContentNew);
			}
		}
		if (dwPrecisionLevel >= 2) {
			// Precision level 2: MODIFIED keys, values
			// Need to include Registry key modified (last write) time to support modified key values
			if ((nActionType == VALMODI) && (NULL != lpCR->lpContentNew)) {
				PopulateCellObject(hFileAPXML, nActionType, lpCR->lpContentNew);
			}
		}

		// We are always going to output deleted content, therefore, populate FileObjects
		if ((nActionType == KEYDEL) || (nActionType == VALDEL)) {
			if (NULL != lpCR->lpContentOld) {
				PopulateCellObject(hFileAPXML, nActionType, lpCR->lpContentOld);
			}
		}
	}
}

// ----------------------------------------------------------------------
// Create DFXML and RegXML reports of system changes between 2 snapshots
// ----------------------------------------------------------------------
BOOL OpenAPXMLReport(LPTSTR lpszReportBaseName)
{
	LPTSTR lpszOutputPath;
	LPTSTR lpszAPXMLExtension = TEXT(".apxml");
	size_t cchString;

	// Get the current working directory for the APXML report
	lpszOutputPath = MYALLOC0(MAX_PATH * sizeof(TCHAR));
	GetCurrentDirectory(MAX_PATH + 1, lpszOutputPath);

	// Add a backslash to the current working directory
	cchString = _tcslen(lpszOutputPath);
	if ((0 < cchString) && ((TCHAR)'\\' != *(lpszOutputPath + cchString - 1))) {
		*(lpszOutputPath + cchString) = (TCHAR)'\\';
		*(lpszOutputPath + cchString + 1) = (TCHAR)'\0';
		cchString++;
	}

	// Append APXML report base name to output path
	_tcscat(lpszOutputPath, lpszReportBaseName);

	// Append APXML file extension
	_tcscat(lpszOutputPath, lpszAPXMLExtension);

	// Inform user of the output file
	printf("  > APXML output: %ws\n", lpszOutputPath);

	hFileAPXML = CreateFile(lpszOutputPath,
		GENERIC_READ | GENERIC_WRITE, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, 
		NULL,
		CREATE_NEW, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	// All done. Free stuff. Return True.
	MYFREE(lpszOutputPath);
	return TRUE;
}

// ----------------------------------------------------------------------
// Create DFXML and RegXML reports of system changes between 2 snapshots
// ----------------------------------------------------------------------
BOOL reOpenAPXMLReport(LPTSTR lpszAPXMLDestFileName)
{
	hFileAPXML = CreateFile(lpszAPXMLDestFileName, 
		FILE_APPEND_DATA, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, 
		NULL, 
		OPEN_ALWAYS, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	// All done. Free stuff. Return True.
	return TRUE;
}

// ----------------------------------------------------------------------
// Generate APXML report entries based on content from CompareResults structure
// ----------------------------------------------------------------------
BOOL GenerateAPXMLReport(VOID)
{
	// TL: Start making APXML
	// FILE SYSTEM (DIRECTORY) REPORTING! (new, modified, and deleted file system directories
	if (0 != CompareResult.stcAdded.cDirs) { ParseLPCompResultsFile(DIRADD, CompareResult.stCRHeads.lpCRDirAdded); }
	if (0 != CompareResult.stcModified.cDirs) { ParseLPCompResultsFile(DIRMODI, CompareResult.stCRHeads.lpCRDirModified); }
	if (0 != CompareResult.stcDeleted.cDirs) { ParseLPCompResultsFile(DIRDEL, CompareResult.stCRHeads.lpCRDirDeleted); }
	// FILE SYSTEM (DATA FILES) REPORTING! (new, modified, and deleted data files
	if (0 != CompareResult.stcAdded.cFiles) { ParseLPCompResultsFile(FILEADD, CompareResult.stCRHeads.lpCRFileAdded); }
	if (0 != CompareResult.stcModified.cFiles) { ParseLPCompResultsFile(FILEMODI, CompareResult.stCRHeads.lpCRFileModified); }
	if (0 != CompareResult.stcDeleted.cFiles) { ParseLPCompResultsFile(FILEDEL, CompareResult.stCRHeads.lpCRFileDeleted); }

	// REGISTRY KEY REPORTING! (new and deleted Registry keys)
	if (0 != CompareResult.stcAdded.cKeys) { ParseLPCompResultsRegistry(KEYADD, CompareResult.stCRHeads.lpCRKeyAdded); }
	if (0 != CompareResult.stcDeleted.cKeys) { ParseLPCompResultsRegistry(KEYDEL, CompareResult.stCRHeads.lpCRKeyDeleted); }
	// REGISTRY VALUE REPORTING! (new, modified and deleted Registry values)
	if (0 != CompareResult.stcAdded.cValues) { ParseLPCompResultsRegistry(VALADD, CompareResult.stCRHeads.lpCRValAdded); }
	if (0 != CompareResult.stcModified.cValues) { ParseLPCompResultsRegistry(VALMODI, CompareResult.stCRHeads.lpCRValModified); }
	if (0 != CompareResult.stcDeleted.cValues) { ParseLPCompResultsRegistry(VALDEL, CompareResult.stCRHeads.lpCRValDeleted); }
	
	// All done. Return True.
	return TRUE;
}

// ----------------------------------------------------------------------
// Initialize text strings with English defaults
// ----------------------------------------------------------------------
VOID SetTextsToDefaultLanguage(VOID)
{
	// Clear all structures (pointers, IDs, etc.)
	ZeroMemory(asLangTexts, sizeof(asLangTexts));

	// Set English default language strings
	asLangTexts[iszTextKey].lpszText = TEXT("Keys:");
	asLangTexts[iszTextValue].lpszText = TEXT("Values:");
	asLangTexts[iszTextDir].lpszText = TEXT("Dirs:");
	asLangTexts[iszTextFile].lpszText = TEXT("Files:");
	asLangTexts[iszTextTime].lpszText = TEXT("Time:");
	asLangTexts[iszTextKeyAdd].lpszText = TEXT("Keys added:");
	asLangTexts[iszTextKeyDel].lpszText = TEXT("Keys deleted:");
	asLangTexts[iszTextValAdd].lpszText = TEXT("Values added:");
	asLangTexts[iszTextValDel].lpszText = TEXT("Values deleted:");
	asLangTexts[iszTextValModi].lpszText = TEXT("Values modified:");
	asLangTexts[iszTextFileAdd].lpszText = TEXT("Files added:");
	asLangTexts[iszTextFileDel].lpszText = TEXT("Files deleted:");
	asLangTexts[iszTextFileModi].lpszText = TEXT("Files modified:");
	asLangTexts[iszTextDirAdd].lpszText = TEXT("Folders added:");
	asLangTexts[iszTextDirDel].lpszText = TEXT("Folders deleted:");
	asLangTexts[iszTextDirModi].lpszText = TEXT("Folders modified:");
	asLangTexts[iszTextTotal].lpszText = TEXT("Total changes:");
}

// ----------------------------------------------------------------------
// Display snapshot overview to standard output
// ----------------------------------------------------------------------
VOID DisplayShotInfo(LPSNAPSHOT lpShot) 
{
	LPTSTR lpszInfoBox;
	lpszInfoBox = MYALLOC0(SIZEOF_INFOBOX * sizeof(TCHAR));
	_sntprintf(lpszInfoBox, SIZEOF_INFOBOX, TEXT("    %s %u\n    %s %u\n    %s %u\n    %s %u\n\0"),
		asLangTexts[iszTextKey].lpszText, lpShot->stCounts.cKeys,
		asLangTexts[iszTextValue].lpszText, lpShot->stCounts.cValues,
		asLangTexts[iszTextDir].lpszText, lpShot->stCounts.cDirs,
		asLangTexts[iszTextFile].lpszText, lpShot->stCounts.cFiles
		);
	lpszInfoBox[SIZEOF_INFOBOX - 1] = (TCHAR)'\0';  // safety NULL char
	printf("  > %s\n", "Snapshot overview:");
	wprintf(L"%s", lpszInfoBox);
	MYFREE(lpszInfoBox);
}

// ----------------------------------------------------------------------
// Display snapshot comaprison results to standard output
// ----------------------------------------------------------------------
VOID DisplayResultInfo() 
{
	LPTSTR lpszInfoBox;
	lpszInfoBox = MYALLOC0(SIZEOF_INFOBOX * sizeof(TCHAR));
	_sntprintf(lpszInfoBox, SIZEOF_INFOBOX, TEXT("    %s %u\n    %s %u\n    %s %u\n    %s %u\n    %s %u\n    %s %u\n    %s %u\n    %s %u\n    %s %u\n    %s %u\n    %s %u\n    %s %u\n\0"),
		asLangTexts[iszTextKeyAdd].lpszText, CompareResult.stcAdded.cKeys,
		asLangTexts[iszTextKeyDel].lpszText, CompareResult.stcDeleted.cKeys,
		asLangTexts[iszTextValAdd].lpszText, CompareResult.stcAdded.cValues,
		asLangTexts[iszTextValModi].lpszText, CompareResult.stcModified.cValues,
		asLangTexts[iszTextValDel].lpszText, CompareResult.stcDeleted.cValues,
		asLangTexts[iszTextDirAdd].lpszText, CompareResult.stcAdded.cDirs,
		asLangTexts[iszTextDirModi].lpszText, CompareResult.stcModified.cDirs,
		asLangTexts[iszTextDirDel].lpszText, CompareResult.stcDeleted.cDirs,
		asLangTexts[iszTextFileAdd].lpszText, CompareResult.stcAdded.cFiles,
		asLangTexts[iszTextFileModi].lpszText, CompareResult.stcModified.cFiles,
		asLangTexts[iszTextFileDel].lpszText, CompareResult.stcDeleted.cFiles,
		asLangTexts[iszTextTotal].lpszText, CompareResult.stcChanged.cAll
		);
	lpszInfoBox[SIZEOF_INFOBOX - 1] = (TCHAR)'\0';  // safety NULL char
	printf("  > %s\n", "Results overview:");
	wprintf(L"%s", lpszInfoBox);
	MYFREE(lpszInfoBox);
}