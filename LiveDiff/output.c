/*
Copyright 2015 Thomas Laurenson
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

// Set up file handles for output
HANDLE hFileDFXML;		// DFXML report file handle
HANDLE hFileREGXML;		// RegXML report file handle
HANDLE hFileAPXML;		// APXML report file handle

LANGUAGETEXT asLangTexts[cLangStrings];

// ----------------------------------------------------------------------
// Parse LPCOMPRESULT to single file system entry, then pass to PopulateFileObject
// ----------------------------------------------------------------------
VOID ParseLPCompResultsFile(DWORD nActionType, LPCOMPRESULT lpStartCR)
{
	LPCOMPRESULT lpCR;
	// Loop through each item in CompareResults structure
	for (lpCR = lpStartCR; NULL != lpCR; lpCR = lpCR->lpNextCR) {
		if ((nActionType == DIRADD) || (nActionType == DIRMODI) || (nActionType == FILEADD) || (nActionType == FILEMODI)) {
			if (NULL != lpCR->lpContentNew) {
				PopulateFileObject(hFileAPXML, nActionType, lpCR->lpContentNew);
			}
		}
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
	for (lpCR = lpStartCR; NULL != lpCR; lpCR = lpCR->lpNextCR) {
		if ((nActionType == KEYADD) || (nActionType == VALADD) || (nActionType == VALMODI)) {
			if (NULL != lpCR->lpContentNew) {
				PopulateCellObject(hFileAPXML, nActionType, lpCR->lpContentNew);
			}
		}
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
BOOL OpenAPXMLReport(LPTSTR lpszAppName)
{
	LPTSTR lpszOutputPath;
	LPTSTR lpszReportBaseName;
	LPTSTR lpszAPXMLExtension = TEXT(".apxml");
	LPTSTR lpszAPXMLDestFileName;
	DWORD  nBufferSize = 2048;
	size_t cchString;

	size_t lenReportBaseName = _tcslen(lpszAppName);
	lpszReportBaseName = MYALLOC0(lenReportBaseName * sizeof(TCHAR));
	_tcscpy(lpszReportBaseName, lpszAppName);

	// Allocate space for file names
	lpszOutputPath = MYALLOC0(EXTDIRLEN * sizeof(TCHAR));
	lpszAPXMLDestFileName = MYALLOC0(EXTDIRLEN * sizeof(TCHAR));

	// Get the current directory to save report files in
	GetCurrentDirectory(_tcslen(lpszOutputPath), lpszOutputPath);

	cchString = _tcslen(lpszOutputPath);
	if ((0 < cchString) && ((TCHAR)'\\' != *(lpszOutputPath + cchString - 1))) {
		*(lpszOutputPath + cchString) = (TCHAR)'\\';
		*(lpszOutputPath + cchString + 1) = (TCHAR)'\0';
		cchString++;
	}
	// Create DFXML report file handle
	_tcscpy(lpszAPXMLDestFileName, lpszOutputPath);
	_tcscpy(lpszAPXMLDestFileName, lpszReportBaseName);
	cchString = _tcslen(lpszAPXMLDestFileName);
	_tcscat(lpszAPXMLDestFileName, lpszAPXMLExtension);
	hFileAPXML = CreateFile(lpszAPXMLDestFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

	// All done. Free stuff. Return True.
	//MYFREE(lpszAPXMLDestFileName);
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