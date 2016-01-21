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

HANDLE hFileAPXML;		// Application Profile file handle
DWORD cbCRLF;
DWORD NBW;

//-----------------------------------------------------------------
// DFXML Functions to make life easier
//-----------------------------------------------------------------
// Write out an entire tag
// e.g. <filename>Program Files</filename>
//-----------------------------------------------------------------
VOID xml_out2s(HANDLE hFile, LPTSTR tag, LPTSTR value)
{
	LPTSTR line;
	DWORD maxLen = ((DWORD)_tcslen(tag) * 2) + ((DWORD)_tcslen(value)) + 5 + 2; // 5 is for tags, 2 for newline and /0
	line = MYALLOC0(maxLen * sizeof(TCHAR));

	// Combine the entire line of tag, value, tag, newline
	_sntprintf(line, maxLen, TEXT("<%s>%s</%s>\n"), tag, value, tag);
	WriteFile(hFile, line, (DWORD)(_tcslen(line) * sizeof(TCHAR)), &NBW, NULL);

	// Done, so free the line
	MYFREE(line);
}

//-----------------------------------------------------------------
// Write out an entire tag with attribute
// e.g. <hashdigest type='sha1'>8B38E348E9371894174893A4</hashdigest>\n
//-----------------------------------------------------------------
VOID xml_outa2s(HANDLE hFile, LPTSTR tag, LPTSTR attribute, LPTSTR value)
{
	LPTSTR line;
	DWORD maxLen = ((DWORD)_tcslen(tag) * 2) + (DWORD)_tcslen(attribute) + ((DWORD)_tcslen(value)) + 6 + 2; // 6 is for tags, 2 for newline and /0
	line = MYALLOC0(maxLen * sizeof(TCHAR));

	// Combine the entire line of tag, value, tag, newline
	_sntprintf(line, maxLen, TEXT("<%s %s>%s</%s>\n"), tag, attribute, value, tag);
	WriteFile(hFile, line, (DWORD)(_tcslen(line) * sizeof(TCHAR)), &NBW, NULL);

	// Done, so free the line
	MYFREE(line);
}

//-----------------------------------------------------------------
// Write out a starting tag (e.g. "<fileobject>) and maybe attribute
//-----------------------------------------------------------------
VOID xml_tagout(HANDLE hFile, LPTSTR tag, LPTSTR attribute)
{
	LPTSTR line;
	DWORD maxLen = (DWORD)_tcslen(tag) + (DWORD)_tcslen(attribute) + 3 + 2; // 3 is for tags, 2 for newline and /0
	line = MYALLOC0(maxLen * sizeof(TCHAR));
	if (attribute[0] == '\0') 
	{
		_sntprintf(line, maxLen, TEXT("<%s>\n"), tag);
		WriteFile(hFile, line, (DWORD)(_tcslen(line) * sizeof(TCHAR)), &NBW, NULL);
	}
	else 
	{
		_sntprintf(line, maxLen, TEXT("<%s %s>\n"), tag, attribute);
		WriteFile(hFile, line, (DWORD)(_tcslen(line) * sizeof(TCHAR)), &NBW, NULL);
	}
	MYFREE(line);
}

//-----------------------------------------------------------------
// Write out a closing tag (e.g. "</fileobject>)
//-----------------------------------------------------------------
VOID xml_ctagout(HANDLE hFile, LPTSTR tag)
{
	LPTSTR line;
	DWORD maxLen = ((DWORD)_tcslen(tag)) + 3 + 2; // 3 is for tags, 2 for newline and /0
	line = MYALLOC0(maxLen * sizeof(TCHAR));
	_sntprintf(line, maxLen, TEXT("</%s>\n"), tag);
	WriteFile(hFile, line, (DWORD)(_tcslen(line) * sizeof(TCHAR)), &NBW, NULL);
	MYFREE(line);
}

//-----------------------------------------------------------------
// Check string for ampersand
//-----------------------------------------------------------------
LPTSTR xml_ampersand_check(LPTSTR value)
{
	size_t maxLen = (DWORD)((_tcslen(value) + 100) * sizeof(TCHAR));
	wchar_t *line = MYALLOC0(maxLen * sizeof(TCHAR));
	memcpy(line, value, maxLen);

	//printf("\n\nSTART: %ws\n", line);
	wchar_t * pwc;
	pwc = _tcschr(line, L'&');
	while (pwc != NULL) 
	{
		int j = pwc - line;
		line = replace_string(line, pwc, j, TEXT("&amp;"));
		pwc = _tcschr(line + j + 1, L'&');
	}
	//printf("FIXED: %ws\n", line);	
	return line;
}

//-----------------------------------------------------------------
// Check string for single quote (apostrophe) character
//-----------------------------------------------------------------
LPTSTR xml_apos_check(LPTSTR value)
{
	size_t maxLen = (DWORD)((_tcslen(value) + 100) * sizeof(TCHAR));
	wchar_t *line = MYALLOC0(maxLen * sizeof(TCHAR));
	memcpy(line, value, maxLen);

	//printf("\n\nSTART: %ws\n", line);
	wchar_t * pwc;
	pwc = _tcschr(line, L'\'');
	while (pwc != NULL) 
	{
		int j = pwc - line;
		line = replace_string(line, pwc, j, TEXT("&apos;"));
		pwc = _tcschr(line + j + 1, L'\'');
	}
	//printf("FIXED: %ws\n", line);	
	return line;
}

//-----------------------------------------------------------------
// Check string for double quote mark character
//-----------------------------------------------------------------
LPTSTR xml_quote_check(LPTSTR value)
{
	size_t maxLen = (DWORD)((_tcslen(value) + 100) * sizeof(TCHAR));
	wchar_t *line = MYALLOC0(maxLen * sizeof(TCHAR));
	memcpy(line, value, maxLen);

	//printf("\n\nSTART: %ws\n", line);
	wchar_t * pwc;
	pwc = _tcschr(line, L'"');
	while (pwc != NULL) 
	{
		int j = pwc - line;
		line = replace_string(line, pwc, j, TEXT("&quot;"));
		pwc = _tcschr(line + j + 1, L'"');
	}
	//printf("FIXED: %ws\n", line);	
	return line;
}

//-----------------------------------------------------------------
// Check string for greater than character
//-----------------------------------------------------------------
LPTSTR xml_gt_check(LPTSTR value)
{
	size_t maxLen = (DWORD)((_tcslen(value) + 100) * sizeof(TCHAR));
	wchar_t *line = MYALLOC0(maxLen * sizeof(TCHAR));
	memcpy(line, value, maxLen);

	//printf("\n\nSTART: %ws\n", line);
	wchar_t * pwc;
	pwc = _tcschr(line, L'>');
	while (pwc != NULL) 
	{
		int j = pwc - line;
		line = replace_string(line, pwc, j, TEXT("&gt;"));
		pwc = _tcschr(line + j + 1, L'>');
	}
	//printf("FIXED: %ws\n", line);	
	return line;
}

//-----------------------------------------------------------------
// Check string for greater than character
//-----------------------------------------------------------------
LPTSTR xml_lt_check(LPTSTR value)
{
	size_t maxLen = (DWORD)((_tcslen(value) + 100) * sizeof(TCHAR));
	wchar_t *line = MYALLOC0(maxLen * sizeof(TCHAR));
	memcpy(line, value, maxLen);

	//printf("\n\nSTART: %ws\n", line);
	wchar_t * pwc;
	pwc = _tcschr(line, L'<');
	while (pwc != NULL) 
	{
		int j = pwc - line;
		line = replace_string(line, pwc, j, TEXT("&lt;"));
		pwc = _tcschr(line + j + 1, L'<');
	}
	//printf("FIXED: %ws\n", line);	
	return line;
}

//-----------------------------------------------------------------
// If a special character is found, replace it with the escaped character
// This includes: & " ' < > characters
//-----------------------------------------------------------------
LPTSTR replace_string(LPTSTR all, LPTSTR end, size_t posd, LPTSTR replace)
{
	size_t maxLen = (DWORD)((_tcslen(all) + 100) * sizeof(TCHAR));
	wchar_t *fixed = MYALLOC0(maxLen * sizeof(TCHAR));
	_tcsncpy(fixed, all, posd);
	fixed[posd] = '\0';
	_tcscat(fixed, replace);
	_tcscat(fixed, end + 1);
	return fixed;
}

//-----------------------------------------------------------------
// Check string for control characters
//-----------------------------------------------------------------
BOOL xml_check_control(LPTSTR value)
{
	//printf("%ws\n", value);
	if (value == NULL || 0 == _tcscmp(value, TEXT(""))) {
		return FALSE;
	}
	
	size_t maxLen = (_tcslen(value) * sizeof(TCHAR));
	wchar_t *line = MYALLOC0(maxLen * sizeof(TCHAR));
	memcpy(line, value, maxLen);
	BOOL result = FALSE;
	size_t i = 0;

	while (!_istcntrl(line[i])) {
		result = FALSE;
		i++;
	}
	size_t maxLenOut = (i * sizeof(TCHAR));
	if (maxLen != maxLenOut) {
		result = TRUE;
	}
	return result;
}


//-----------------------------------------------------------------
// XML REPORTING STARTS HERE!
//-----------------------------------------------------------------
// Populate DFXML FileObject from LPFILECONTENT
//----------------------------------------------------------------- 
VOID PopulateFileObject(HANDLE hFile, DWORD nActionType, LPFILECONTENT lpCR)
{
	LPTSTR lpszFileObject;			// The fileobject tag
	LPTSTR lpszFileObjectAttribute; // The fileobject tag delta attribute
	LPTSTR lpszFileName;			// Full path and file name
	//LPTSTR lpsznormFileName;		// Normalised full path
	DWORD FileSize = 0;				// File size
	TCHAR lpszSizeString[64];
	LPTSTR lpszMetaType = TEXT("");
	LPTSTR lpszAlloc;
	LPTSTR lpszsha1Hash;
	LPTSTR lpszmd5Hash;

	lpszFileObject = TEXT("fileobject");
	// Write FileObject starting element with delta, determined by nActionType
	if ((nActionType == DIRDEL) || (nActionType == FILEDEL)) {
		lpszFileObjectAttribute = TEXT("delta:deleted_file='1'");
	}
	else if ((nActionType == DIRADD) || (nActionType == FILEADD)) {
		lpszFileObjectAttribute = TEXT("delta:new_file='1'");
	}
	else if ((nActionType == DIRMODI) || (nActionType == FILEMODI)) {
		lpszFileObjectAttribute = TEXT("delta:changed_file='1'");
	}
	else {
		// If nActionType not available write a normal fileobject tag without attribute
		lpszFileObjectAttribute = TEXT("");
	}
	xml_tagout(hFile, lpszFileObject, lpszFileObjectAttribute);

	// Start processing the FILECONTENT, get full file name
	lpszFileName = GetWholeFileName(lpCR, 4);

	// Normalise the full file path, then write to DFXML
	// Remove first 3 characters (this removes "C:\")
	/*
	lpsznormFileName = MYALLOC(MAX_PATH * sizeof(TCHAR));
	_tcscpy(lpsznormFileName, lpszFileName);
	if (_tcslen(lpszFileName) > 3) {
		lpsznormFileName += 3;
	}
	// Replace "\" with "/", this is how fiwalk outputs file paths
	for (DWORD i = 0; i < _tcslen(lpsznormFileName); i++) {
		if (lpsznormFileName[i] == (TCHAR)('\\')) {
			lpsznormFileName[i] = (TCHAR)('/');
		}
	}
	xml_out2s(hFile, TEXT("filename"), lpsznormFileName);
	*/

	// Write full path (filename) to XML report
	// Need a check here for 
	// 1) & ()
	// 2) ' to &apos; (single quote)
	lpszFileName = xml_ampersand_check(lpszFileName);
	lpszFileName = xml_apos_check(lpszFileName);
	xml_out2s(hFile, TEXT("filename"), lpszFileName);

	// Get file size, if a file, convert file size then write tag
	FileSize = lpCR->nFileSizeLow;
	if ((FILEADD == nActionType) || (FILEDEL == nActionType) || (FILEMODI == nActionType)) {
		_itot_s(FileSize, lpszSizeString, 64, 10);
		xml_out2s(hFile, TEXT("filesize"), lpszSizeString);
	}

	// Determine meta_type then write: files = 1, directories = 2
	if ((FILEADD == nActionType) || (FILEMODI == nActionType) || (FILEDEL == nActionType)) {
		lpszMetaType = TEXT("1");
	}
	if ((DIRADD == nActionType) || (DIRMODI == nActionType) || (DIRDEL == nActionType)) {
		lpszMetaType = TEXT("2");
	}
	xml_out2s(hFile, TEXT("meta_type"), lpszMetaType);

	// Determine allocation then write
	if ((DIRDEL == nActionType) || (FILEDEL == nActionType)) {
		lpszAlloc = TEXT("0");
	}
	else {
		lpszAlloc = TEXT("1");
	}
	xml_out2s(hFile, TEXT("alloc_name"), lpszAlloc);
	xml_out2s(hFile, TEXT("alloc_inode"), lpszAlloc);

	// If a file fetch MD5 hash
	if (performMD5Hashing)
	{
		if ((FILEADD == nActionType) || (FILEDEL == nActionType) || (FILEMODI == nActionType)) {
			lpszmd5Hash = lpCR->lpszMD5;
			if (NULL != lpszmd5Hash) { // NULL check, mainly for saved file issue
				xml_outa2s(hFile, TEXT("hashdigest"), TEXT("type='md5'"), lpszmd5Hash);
			}
		}
	}

	// If a file fetch SHA1 hash
	if (performSHA1Hashing)
	{
		if ((FILEADD == nActionType) || (FILEDEL == nActionType) || (FILEMODI == nActionType)) {
			lpszsha1Hash = lpCR->lpszSHA1;
			if (NULL != lpszsha1Hash) { // NULL check, mainly for saved file issue
				xml_outa2s(hFile, TEXT("hashdigest"), TEXT("type='sha1'"), lpszsha1Hash);
			}
		}
	}

	// Write application life cycle phase (app_state) element
	xml_out2s(hFile, TEXT("app_state"), lpszAppState);

	// Write FileObject end element 
	xml_ctagout(hFile, TEXT("fileobject"));
	//MYFREE(normFileName);
}

//-----------------------------------------------------------------
// Populate RegXML CellObject from a previously matched key/value
//----------------------------------------------------------------- 
VOID PopulateCellObject(HANDLE hFile, DWORD nActionType, LPCOMPRESULT lpCR) 
{
	LPTSTR lpszCellObject;
	LPTSTR lpszCellObjectAttribute;
	LPTSTR lpszCellPath;
	LPTSTR lpszAlloc;
	LPTSTR lpszDataType = TEXT(""); // Need to figure how to alloc memory to an unknown size
	LPTSTR lpszData = TEXT("");
	LPTSTR lpszRawData = TEXT("");
	
	lpszCellObject = TEXT("cellobject");
	// Write CellObject starting element with delta
	if ((nActionType == KEYDEL) || (nActionType == VALDEL)) {
		lpszCellObjectAttribute = TEXT("delta:deleted_cell='1'");
	}
	else if ((nActionType == KEYADD) || (nActionType == VALADD)) {
		lpszCellObjectAttribute = TEXT("delta:new_cell='1'");
	}
	else if (nActionType == VALMODI) {
		lpszCellObjectAttribute = TEXT("delta:changed_cell='1'");
	}
	else {
		lpszCellObjectAttribute = TEXT("");
	}
	xml_tagout(hFile, lpszCellObject, lpszCellObjectAttribute);

	// Allocate some memory for CellObject properties
	//lpszCellPath = MYALLOC0((MAX_PATH * 4) * sizeof(TCHAR));
	lpszCellPath = MYALLOC0(MAX_PATH * sizeof(TCHAR));
	// Aparently value names can be long!
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ms724872%28v=vs.85%29.aspx
	// Key name	  = 255 characters
	// Value name = 16,383 characters

	// Process Registry Keys
	if ((KEYDEL == nActionType) || (KEYADD == nActionType)) 
	{
		LPKEYCONTENT lpKC = lpCR;
		lpszCellPath = GetWholeKeyName(lpKC, fUseLongRegHead);

		// Check the cellpath sting for special characters that need to be escaped
		lpszCellPath = xml_ampersand_check(lpszCellPath);
		lpszCellPath = xml_apos_check(lpszCellPath);
		lpszCellPath = xml_quote_check(lpszCellPath);
		lpszCellPath = xml_gt_check(lpszCellPath);
		lpszCellPath = xml_lt_check(lpszCellPath);

		// Write Registry key entry to RegXML report
		xml_out2s(hFile, TEXT("cellpath"), lpszCellPath);
		xml_out2s(hFile, TEXT("name_type"), TEXT("k"));
	}

	// Process Registry Values
	if ((VALDEL == nActionType) || (VALADD == nActionType) || (VALMODI == nActionType)) 
	{
		// Variables to temporarily store Registry value information 
		LPVALUECONTENT lpVC = lpCR;
		LPBYTE lpData = lpVC->lpValueData;
		DWORD nTypeCode = lpVC->nTypeCode;
		PDWORD cbData = &lpVC->cbData;
		LPTSTR lpszBasename = lpVC->lpszValueName;

		// Fetch the print the CellObject cell path
		lpszCellPath = GetWholeValueName(lpVC, fUseLongRegHead);

		// Check the cellpath sting for special characters that need to be escaped
		lpszCellPath = xml_ampersand_check(lpszCellPath);
		lpszCellPath = xml_apos_check(lpszCellPath);
		lpszCellPath = xml_quote_check(lpszCellPath);
		lpszCellPath = xml_gt_check(lpszCellPath);
		lpszCellPath = xml_lt_check(lpszCellPath);

		// Write out the cellpath for Registry value
		xml_out2s(hFile, TEXT("cellpath"), lpszCellPath);
		
		// Print the CellObject name_type (always "v" for Registry values)
		xml_out2s(hFile, TEXT("name_type"), TEXT("v"));

		// Print the value basename
		if (lpszBasename == NULL) {
			lpszBasename = TEXT("(Default)");
		}
		xml_out2s(hFile, TEXT("basename"), lpszBasename);

		// Print the value data type
		lpszDataType = GetValueDataType(nTypeCode);
		xml_out2s(hFile, TEXT("data_type"), lpszDataType);

		// Get the actual value data, human readable based on type code
		lpszData = ParseValueData(lpData, cbData, nTypeCode);

		// Check the cellpath sting for special characters that need to be escaped
		lpszData = xml_ampersand_check(lpszData);
		lpszData = xml_apos_check(lpszData);
		lpszData = xml_quote_check(lpszData);
		lpszData = xml_gt_check(lpszData);
		lpszData = xml_lt_check(lpszData);

		// Check for control characters, if present, encode using base64
		if (xml_check_control(lpszData)) 
		{
			DWORD pcchString = 0;

			// First, determine length requires
			CryptBinaryToString(lpVC->lpValueData,
				lpVC->cbData,
				CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
				NULL,
				&pcchString);

			// Allocate size for base64 encoded string
			LPTSTR pszString = MYALLOC(pcchString * sizeof(TCHAR));

			// Now, encode value using base64
			CryptBinaryToString(lpVC->lpValueData,
				lpVC->cbData,
				CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
				pszString,
				&pcchString);

			// Write out the data (which is base64 encoded)
			xml_out2s(hFile, TEXT("data_encoding"), TEXT("base64"));
			xml_out2s(hFile, TEXT("data"), pszString);
		}
		else
		{
			// Write out the converted data
			xml_out2s(hFile, TEXT("data"), lpszData);
		}

		// Finally, add a new RegXML element type: raw_data (this stored the Registry value data in hexadecimal)
		// This is for matching purposes, because most Registry parsing tools encode value data differently
		lpszRawData = ParseValueData(lpData, cbData, REG_BINARY);
		xml_out2s(hFile, TEXT("data_raw"), lpszRawData);
	}

	// Determine allocation then write
	if ((KEYDEL == nActionType) || (VALDEL == nActionType)) {
		lpszAlloc = TEXT("0");
	}
	else {
		lpszAlloc = TEXT("1");
	}
	xml_out2s(hFile, TEXT("alloc"), lpszAlloc);

	// Write application life cycle phase (app_state) element
	xml_out2s(hFile, TEXT("app_state"), lpszAppState);

	// Close the cellobject tag
	xml_ctagout(hFile, TEXT("cellobject"));

	// Free the allocated memory
	MYFREE(lpszCellPath);
}

//-----------------------------------------------------------------
// Create a APXML header with metadata and Dublic Core (DC)
//----------------------------------------------------------------- 
VOID StartAPXML(LPTSTR lpszStartDate, LPTSTR lpszAppName, LPTSTR lpszAppVersion, LPTSTR lpszCommandLine, LPTSTR lpszWindowsVersion)
{
	LPTSTR lpszXMLHeader	= TEXT("?xml version='1.0' encoding='UTF-16' ?");
	LPTSTR lpszAPXML		= TEXT("<apxml version='1.0.0'\n");
	LPTSTR lpszAPXMLNS		= TEXT("    xmlns='https://github.com/thomaslaurenson/apxml_schema'\n");
	LPTSTR lpszDFXMLDelta	= TEXT("    xmlns:delta='http://www.forensicswiki.org/wiki/Forensic_Disk_Differencing'\n");
	LPTSTR lpszDFXMLXSI		= TEXT("    xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'\n");
	LPTSTR lpszDFXMLDC		= TEXT("    xmlns:dc='http://purl.org/dc/elements/1.1/' >\n");

	// Write APXML header with requires XML namespaces
	xml_tagout(hFileAPXML, lpszXMLHeader, TEXT(""));
	WriteFile(hFileAPXML, lpszAPXML, (DWORD)(_tcslen(lpszAPXML) * sizeof(TCHAR)), &NBW, NULL);
	WriteFile(hFileAPXML, lpszAPXMLNS, (DWORD)(_tcslen(lpszAPXMLNS) * sizeof(TCHAR)), &NBW, NULL);
	WriteFile(hFileAPXML, lpszDFXMLDelta, (DWORD)(_tcslen(lpszDFXMLDelta) * sizeof(TCHAR)), &NBW, NULL);
	WriteFile(hFileAPXML, lpszDFXMLXSI, (DWORD)(_tcslen(lpszDFXMLXSI) * sizeof(TCHAR)), &NBW, NULL);
	WriteFile(hFileAPXML, lpszDFXMLDC, (DWORD)(_tcslen(lpszDFXMLDC) * sizeof(TCHAR)), &NBW, NULL);

	// Write metadata element
	xml_tagout(hFileAPXML, TEXT("metadata"), TEXT(""));
	xml_out2s(hFileAPXML, TEXT("dc:type"), TEXT("Application Profile XML (APXML)"));
	xml_out2s(hFileAPXML, TEXT("dc:publisher"), TEXT("thomaslaurenson.com"));
	xml_out2s(hFileAPXML, TEXT("app_name"), lpszAppName);
	xml_out2s(hFileAPXML, TEXT("app_version"), lpszAppVersion);
	xml_ctagout(hFileAPXML, TEXT("metadata"));

	// Write creator element
	xml_tagout(hFileAPXML, TEXT("creator"), TEXT("version='1.0'"));
	xml_out2s(hFileAPXML, TEXT("program"), TEXT("LiveDiff.exe"));
	xml_out2s(hFileAPXML, TEXT("version"), TEXT("1.0.0"));

	// Write build_environment element
	//xml_tagout(hFileAPXML, TEXT("build_environment"), TEXT(""));
	//xml_out2s(hFileAPXML, TEXT("compiler"), TEXT("Microsoft (R) C/C++ Optimizing Compiler Version 18.00.30501 for x86"));
	//xml_out2s(hFileAPXML, TEXT("compilation_date"), lpszCompileDate);
	//library?
	//xml_ctagout(hFileAPXML, TEXT("build_environment"));

	// Write execution_environment element
	xml_tagout(hFileAPXML, TEXT("execution_environment"), TEXT(""));
	//xml_out2s(hFileAPXML, TEXT("os_sysname"), TEXT(""));
	//xml_out2s(hFileAPXML, TEXT("os_release"), TEXT(""));
	//xml_out2s(hFileAPXML, TEXT("os_version"), TEXT(""));
	//xml_out2s(hFileAPXML, TEXT("host"), TEXT(""));
	//xml_out2s(hFileAPXML, TEXT("arch"), TEXT(""));
	xml_out2s(hFileAPXML, TEXT("windows_version"), lpszWindowsVersion);
	xml_out2s(hFileAPXML, TEXT("command_line"), lpszCommandLine);
	xml_out2s(hFileAPXML, TEXT("start_date"), lpszStartDate);
	xml_ctagout(hFileAPXML, TEXT("execution_environment"));
	xml_ctagout(hFileAPXML, TEXT("creator"));
}

//-----------------------------------------------------------------
// Start/open applciation profile life cycle tag
//----------------------------------------------------------------- 
VOID StartAPXMLTag(LPTSTR tag)
{
	xml_tagout(hFileAPXML, tag, TEXT(""));
	/*
	TCHAR deltaCount[64];
	xml_tagout(hFileAPXML, TEXT("delta_count"), TEXT(""));
	_itot_s(CompareResult.stcAdded.cFiles, deltaCount, 64, 10);
	xml_out2s(hFileAPXML, TEXT("new_files"), deltaCount);
	_itot_s(CompareResult.stcModified.cFiles, deltaCount, 64, 10);
	xml_out2s(hFileAPXML, TEXT("mod_files"), deltaCount);
	_itot_s(CompareResult.stcDeleted.cFiles, deltaCount, 64, 10);
	xml_out2s(hFileAPXML, TEXT("del_files"), deltaCount);
	_itot_s(CompareResult.stcAdded.cDirs, deltaCount, 64, 10);
	xml_out2s(hFileAPXML, TEXT("new_dirs"), deltaCount);
	_itot_s(CompareResult.stcModified.cDirs, deltaCount, 64, 10);
	xml_out2s(hFileAPXML, TEXT("mod_dirs"), deltaCount);
	_itot_s(CompareResult.stcDeleted.cDirs, deltaCount, 64, 10);
	xml_out2s(hFileAPXML, TEXT("del_dirs"), deltaCount);
	_itot_s(CompareResult.stcAdded.cKeys, deltaCount, 64, 10);
	xml_out2s(hFileAPXML, TEXT("new_keys"), deltaCount);
	_itot_s(CompareResult.stcDeleted.cKeys, deltaCount, 64, 10);
	xml_out2s(hFileAPXML, TEXT("del_keys"), deltaCount);
	_itot_s(CompareResult.stcAdded.cValues, deltaCount, 64, 10);
	xml_out2s(hFileAPXML, TEXT("new_values"), deltaCount);
	_itot_s(CompareResult.stcModified.cValues, deltaCount, 64, 10);
	xml_out2s(hFileAPXML, TEXT("mod_values"), deltaCount);
	_itot_s(CompareResult.stcDeleted.cValues, deltaCount, 64, 10);
	xml_out2s(hFileAPXML, TEXT("del_values"), deltaCount);
	xml_ctagout(hFileAPXML, TEXT("delta_count"));
	*/
}

//-----------------------------------------------------------------
// End/close applciation profile life cycle tag
//----------------------------------------------------------------- 
VOID EndAPXMLTag(LPTSTR tag)
{
	xml_ctagout(hFileAPXML, tag);
}

//-----------------------------------------------------------------
// End/close APXML tag
//----------------------------------------------------------------- 
VOID EndAPXML()
{
	// Determine end date and time in ISO 8601 format
	SYSTEMTIME stEndTime;
	LPTSTR lpszEndDate;
	GetLocalTime(&stEndTime);
	lpszEndDate = MYALLOC0(21 * sizeof(TCHAR));
	_sntprintf(lpszEndDate, 21, TEXT("%i-%02i-%02iT%02i:%02i:%02iZ"),
		stEndTime.wYear, stEndTime.wMonth, stEndTime.wDay, stEndTime.wHour, stEndTime.wMinute, stEndTime.wSecond);

	// Populate the rusage element
	xml_tagout(hFileAPXML, TEXT("rusage"), TEXT(""));
	xml_out2s(hFileAPXML, TEXT("end_date"), lpszEndDate);
	xml_ctagout(hFileAPXML, TEXT("rusage"));

	// Close APXML tag
	xml_ctagout(hFileAPXML, TEXT("apxml"));
}