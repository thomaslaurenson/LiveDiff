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

#ifndef LIVEDIFF_GLOBAL_H
#define LIVEDIFF_GLOBAL_H

#include <windows.h>
#include <stdio.h>
#include <shlobj.h>
#include <string.h>
#include <tchar.h>
#include <time.h>
#pragma comment(lib,"Crypt32.lib")

// ----------------------------------------------------------------------
// Definition for QWORD (not yet defined globally in WinDef.h)
// ----------------------------------------------------------------------
#ifndef QWORD
typedef unsigned __int64 QWORD, NEAR *PQWORD, FAR *LPQWORD;
#endif

// ----------------------------------------------------------------------
// Set up program heap
// ----------------------------------------------------------------------
#define USEHEAPALLOC_DANGER
#ifdef USEHEAPALLOC_DANGER
extern HANDLE hHeap;
// MSDN doc say use HEAP_NO_SERIALIZE is not good for process heap :( 
// so change from 1 to 0 which is slower than using 1
#define MYALLOC(x)  HeapAlloc(hHeap,0,x)
#define MYALLOC0(x) HeapAlloc(hHeap,8,x) // (HEAP_NO_SERIALIZE|) HEAP_ZERO_MEMORY ,1|8
#define MYFREE(x)   HeapFree(hHeap,0,x)
#else
#define MYALLOC(x)  GlobalAlloc(GMEM_FIXED,x)
#define MYALLOC0(x) GlobalAlloc(GPTR,x)
#define MYFREE(x)   GlobalFree(x)
#endif

// ----------------------------------------------------------------------
// Registry hive naming conventions (short and long)
// ----------------------------------------------------------------------
LPTSTR lpszHKLMShort;
LPTSTR lpszHKLMLong;
#define CCH_HKLM_LONG  18

LPTSTR lpszHKUShort;
LPTSTR lpszHKULong;
#define CCH_HKU_LONG  10

BOOL fUseLongRegHead;

// ----------------------------------------------------------------------
// Definitions to distinguish file system directories and files
// ----------------------------------------------------------------------
#define ISDIR(x)  ( (x&FILE_ATTRIBUTE_DIRECTORY) != 0 )
#define ISFILE(x) ( (x&FILE_ATTRIBUTE_DIRECTORY) == 0 )

// ----------------------------------------------------------------------
// Definitions for matching status, including modification type
// ----------------------------------------------------------------------
#define NOMATCH         0
#define ISMATCH         1
#define ISDEL           2
#define ISADD           3
#define ISMODI          4
#define ISCHNG			5

// ----------------------------------------------------------------------
// Definitions for digital artifact type and modification type
// ----------------------------------------------------------------------
#define KEYDEL          1
#define KEYADD          2
#define VALDEL          3
#define VALADD          4
#define VALMODI         5
#define DIRDEL          6
#define DIRADD          7
#define DIRMODI         8
#define FILEDEL         9
#define FILEADD         10
#define FILEMODI        11
#define FILECHNG		12

// ----------------------------------------------------------------------
// Definition for output files
// ----------------------------------------------------------------------
#define EXTDIRLEN					MAX_PATH * 4	// Define searching directory length in TCHARs (MAX_PATH includes NULL)
#define SIZEOF_INFOBOX				4096			// Define snapshot summary display size
#define BUFFER_BLOCK_BYTES			1024			// Buffer for various byte objects

// ----------------------------------------------------------------------
// IMPLEMENTED STRUCTURES:
// ----------------------------------------------------------------------
// Structure used for counts of keys, values, directories and files
// ----------------------------------------------------------------------
struct _COUNTS
{
	DWORD cAll;				// Count for all digital artifacts
	DWORD cKeys;			// Count for Registry keys
	DWORD cValues;			// Count for Registry values
	DWORD cDirs;			// Count for file system directories
	DWORD cFiles;			// Count for file system files
	DWORD cFilesBlacklist;	// Count for blacklisted file system files
	DWORD cValuesBlacklist;	// Count for blacklisted Registry values
};
typedef struct _COUNTS COUNTS, FAR *LPCOUNTS;

// ----------------------------------------------------------------------
// Structure used for Windows Registry value entries
// ----------------------------------------------------------------------
struct _VALUECONTENT
{
	DWORD  nTypeCode;                       // Type of value [DWORD,STRING...]
	DWORD  cbData;                          // Value data size in bytes
	LPTSTR lpszValueName;                   // Pointer to value's name
	size_t cchValueName;                    // Length of value's name in chars
	LPBYTE lpValueData;                     // Pointer to value's data
	struct _VALUECONTENT FAR *lpBrotherVC;  // Pointer to value's brother
	struct _KEYCONTENT FAR *lpFatherKC;     // Pointer to value's father key
	DWORD  fValueMatch;                     // Flags used when comparing
};
typedef struct _VALUECONTENT VALUECONTENT, FAR *LPVALUECONTENT;

// ----------------------------------------------------------------------
// Structure used for Windows Registry key entries
// ----------------------------------------------------------------------
struct _KEYCONTENT
{
	LPTSTR lpszKeyName;                     // Pointer to key's name
	size_t cchKeyName;                      // Length of key's name in chars
	FILETIME ftLastWriteTime;				// Registry key last write time
	LPVALUECONTENT lpFirstVC;               // Pointer to key's first value
	struct _KEYCONTENT FAR *lpFirstSubKC;   // Pointer to key's first sub key
	struct _KEYCONTENT FAR *lpBrotherKC;    // Pointer to key's brother
	struct _KEYCONTENT FAR *lpFatherKC;     // Pointer to key's father
	DWORD  fKeyMatch;                       // Flags used when comparing, until 1.8.2 was byte
};
typedef struct _KEYCONTENT KEYCONTENT, FAR *LPKEYCONTENT;

// ----------------------------------------------------------------------
// Structure used for file system content (files and directories)
// ----------------------------------------------------------------------
struct _FILECONTENT
{
	LPTSTR lpszFileName;                    // Pointer to file's name
	size_t cchFileName;                     // Length of file's name in chars
	DWORD  nWriteDateTimeLow;               // File write time [LOW  DWORD]
	DWORD  nWriteDateTimeHigh;              // File write time [HIGH DWORD]
	DWORD  nAccessDateTimeLow;              // File access time [LOW  DWORD]
	DWORD  nAccessDateTimeHigh;             // File access time [HIGH DWORD]
	DWORD  nFileSizeLow;                    // File size [LOW  DWORD]
	DWORD  nFileSizeHigh;                   // File size [HIGH DWORD]
	DWORD  nFileAttributes;                 // File attributes (e.g. directory or file)
	LPTSTR lpszSHA1;						// SHA1 hash of file
	size_t cchSHA1;							// Length of file's SHA1 in chars
	LPTSTR lpszMD5;							// MD5 hash of file
	size_t cchMD5;							// Length of file's MD5 in chars
	struct _FILECONTENT FAR *lpFirstSubFC;  // Pointer to file's first sub file
	struct _FILECONTENT FAR *lpBrotherFC;   // Pointer to file's brother
	struct _FILECONTENT FAR *lpFatherFC;    // Pointer to file's father
	DWORD fFileMatch;                       // Flags used when comparing
};
typedef struct _FILECONTENT FILECONTENT, FAR *LPFILECONTENT;

// ----------------------------------------------------------------------
// Structure used for first file system entry
// ----------------------------------------------------------------------
struct _HEADFILE
{
	LPFILECONTENT lpFirstFC;                // Pointer to head file's first file
	struct _HEADFILE FAR *lpBrotherHF;      // Pointer to head file's brother
	DWORD  fHeadFileMatch;                  // Flags used when comparing
};
typedef struct  _HEADFILE HEADFILE, FAR *LPHEADFILE;

// ----------------------------------------------------------------------
// Structure used for snapshots
// ----------------------------------------------------------------------
struct _SNAPSHOT
{
	LPKEYCONTENT	lpHKLM;                 // Pointer to snapshots HKLM registry keys
	LPKEYCONTENT	lpHKU;                  // Pointer to snapshots HKU registry keys
	LPHEADFILE		lpHF;                   // Pointer to snapshots head files
	LPTSTR			lpszAppName;			// Application name
	LPTSTR			lpszAppState;			// Application state (e.g., install, open, uninstall)
	LPTSTR			lpszWindowsVersion;		// Windows version number
	COUNTS			stCounts;				// Counts structure representing number of entries
	BOOL			fFilled;                // Flag if snapshot was done/loaded (even if result is empty)
	BOOL			fLoaded;                // Flag if snapshot was loaded from a file
};
typedef struct _SNAPSHOT SNAPSHOT, FAR *LPSNAPSHOT;

// ----------------------------------------------------------------------
// Structure for a result list from snapshot comparison
// ----------------------------------------------------------------------
struct _COMPRESULT
{
	LPVOID lpContentOld;                    // Pointer to old content
	LPVOID lpContentNew;                    // Pointer to new content
	struct _COMPRESULT FAR *lpNextCR;       // Pointer to next _COMPRESULT
};
typedef struct _COMPRESULT COMPRESULT, FAR *LPCOMPRESULT;

// ----------------------------------------------------------------------
// Structure for pointers to lists of comapred and detected results
// ----------------------------------------------------------------------
struct _COMPPOINTERS
{
	LPCOMPRESULT  lpCRKeyDeleted;
	LPCOMPRESULT  lpCRKeyAdded;
	LPCOMPRESULT  lpCRValDeleted;
	LPCOMPRESULT  lpCRValAdded;
	LPCOMPRESULT  lpCRValModified;
	LPCOMPRESULT  lpCRDirDeleted;
	LPCOMPRESULT  lpCRDirAdded;
	LPCOMPRESULT  lpCRDirModified;
	LPCOMPRESULT  lpCRFileDeleted;
	LPCOMPRESULT  lpCRFileAdded;
	LPCOMPRESULT  lpCRFileModified;
};
typedef struct _COMPPOINTERS COMPPOINTERS, FAR *LPCOMPPOINTERS;

// ----------------------------------------------------------------------
// Structure for lists of comapred and detected results
// ----------------------------------------------------------------------
struct _COMPRESULTS
{
	LPSNAPSHOT		lpShot1;
	LPSNAPSHOT		lpShot2;
	COUNTS			stcCompared;
	COUNTS          stcChanged;
	COUNTS			stcDeleted;
	COUNTS			stcAdded;
	COUNTS			stcModified;
	COMPPOINTERS	stCRHeads;
	COMPPOINTERS	stCRCurrent;
	BOOL			fFilled;		// Flag if comparison was done (even if result is empty)
};
typedef struct _COMPRESULTS COMPRESULTS, FAR *LPCOMPRESULTS;

//-------------------------------------------------------------
// PREFIX TREE (TRIE) IMPLEMENTATION
//-------------------------------------------------------------
typedef struct trieNode {
	wchar_t key;
	struct trieNode *next;
	struct trieNode *prev;
	struct trieNode *children;
	struct trieNode *parent;
} trieNode_t;

trieNode_t *blacklistFILES;
trieNode_t *blacklistREGISTRY;

BOOL populateStaticBlacklist(LPTSTR lpszFileName, trieNode_t * blacklist);

// ----------------------------------------------------------------------
// External variables
// ----------------------------------------------------------------------
extern SNAPSHOT Shot1;				// Snapshot One
extern SNAPSHOT Shot2;				// Snapshot Two
extern COMPRESULTS CompareResult;	// Compare result
extern HANDLE hFile;				// Handle of file LiveDiff use
extern BOOL fUseLongRegHead;		// Long Registry name

DWORD dwBlacklist;
BOOL performDynamicBlacklisting;
BOOL performSHA1Hashing;
BOOL performMD5Hashing;
BOOL saveSnapShots;
LPTSTR lpszAppState;
DWORD dwPrecisionLevel;

//-------------------------------------------------------------
// Global LiveDiff function definitions
//-------------------------------------------------------------
// livediff.c global functions
VOID determineWindowsVersion();
BOOL snapshotProfile();
BOOL snapshotProfileReboot();
BOOL snapshotLoad(LPTSTR loadFileName1, LPTSTR loadFileName2);

// regshot.c global functions
VOID RegShot(LPSNAPSHOT lpShot);
LPTSTR TransformValueData(LPBYTE lpData, PDWORD lpcbData, DWORD nConversionType);
LPTSTR GetWholeKeyName(LPKEYCONTENT lpStartKC, BOOL fUseLongNames);
LPTSTR GetWholeValueName(LPVALUECONTENT lpVC, BOOL fUseLongNames);
LPTSTR GetValueDataType(DWORD nTypeCode);
LPTSTR ParseValueData(LPBYTE lpData, PDWORD lpcbData, DWORD nTypeCode);
VOID FreeShot(LPSNAPSHOT lpShot);
VOID ClearRegKeyMatchFlags(LPKEYCONTENT lpKC);
VOID CompareShots(VOID);
VOID CreateNewResult(DWORD nActionType, LPVOID lpContentOld, LPVOID lpContentNew);
VOID FreeCompareResult(void);
size_t AdjustBuffer(LPVOID *lpBuffer, size_t nCurrentSize, size_t nWantedSize, size_t nAlign);

// fileshot.c global functions
VOID FileShot(LPSNAPSHOT lpShot);
LPTSTR GetWholeFileName(LPFILECONTENT lpStartFC, size_t cchExtra);
VOID CompareHeadFiles(LPHEADFILE lpStartHF1, LPHEADFILE lpStartHF2);
VOID ClearHeadFileMatchFlags(LPHEADFILE lpHF);
VOID FreeAllHeadFiles(LPHEADFILE lpHF);
BOOL DirChainMatch(LPHEADFILE lpHF1, LPHEADFILE lpHF2);

// blacklist.c global function
void TrieCreate(trieNode_t **root);
void TrieAdd(trieNode_t **root, wchar_t *key);
trieNode_t *TrieCreateNode(wchar_t key);
trieNode_t* TrieSearch(trieNode_t *root, const wchar_t *key);
BOOL TrieSearchPath(trieNode_t *root, const wchar_t *key);

// output.c global functions
VOID SetTextsToDefaultLanguage(VOID);
VOID DisplayResultInfo();
VOID DisplayShotInfo(LPSNAPSHOT lpShot);
BOOL OpenAPXMLReport(LPTSTR lpszAppName);
BOOL GenerateAPXMLReport(VOID);
BOOL reOpenAPXMLReport(LPTSTR lpszAppName);

// dfxml.c global functions
VOID StartAPXML(LPTSTR lpszStartDate, LPTSTR lpszAppName, LPTSTR lpszAppVersion, LPTSTR lpszCommandLine, LPTSTR lpszWindowsVersion);
VOID EndAPXML();
VOID PopulateFileObject(HANDLE hFile, DWORD nActionType, LPFILECONTENT lpCR);
VOID PopulateCellObject(HANDLE hFile, DWORD nActionType, LPCOMPRESULT lpCR);
LPTSTR replace_string(LPTSTR all, LPTSTR end, size_t posd, LPTSTR replace);

//-------------------------------------------------------------
// Language strings
//-------------------------------------------------------------
enum eLangTexts {
	iszTextKey = 0,
	iszTextValue,
	iszTextDir,
	iszTextFile,
	iszTextTime,
	iszTextKeyAdd,
	iszTextKeyDel,
	iszTextValAdd,
	iszTextValDel,
	iszTextValModi,
	iszTextFileAdd,
	iszTextFileDel,
	iszTextFileModi,
	iszTextDirAdd,
	iszTextDirDel,
	iszTextDirModi,
	iszTextTotal,
	cLangStrings
};

struct _LANGUAGETEXT {
	LPTSTR lpszText;
	int nIDDlgItem;
};
typedef struct _LANGUAGETEXT LANGUAGETEXT, FAR *LPLANGUAGETEXT;

extern LANGUAGETEXT asLangTexts[];

#endif  // LIVEDIFF_GLOBAL_H
