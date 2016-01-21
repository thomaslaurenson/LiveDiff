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
// so change fromm 1 to 0 which is slower than using 1
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
// File system global definitions to distinguish files and directories
// ----------------------------------------------------------------------
#define ISDIR(x)  ( (x&FILE_ATTRIBUTE_DIRECTORY) != 0 )
#define ISFILE(x) ( (x&FILE_ATTRIBUTE_DIRECTORY) == 0 )

// ----------------------------------------------------------------------
// Global variable for dynamic blacklisting and file hashing
// ----------------------------------------------------------------------
DWORD dwBlacklist;
BOOL performDynamicBlacklisting;
BOOL performSHA1Hashing;
BOOL performMD5Hashing;
BOOL saveSnapShots;
LPTSTR lpszAppState;
DWORD dwPrecisionLevel;

// ----------------------------------------------------------------------
// Definitions for matching status, including modification type
// ----------------------------------------------------------------------
#define NOMATCH         0
#define ISMATCH         1
#define ISDEL           2
#define ISADD           3
#define ISMODI          4

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

// ----------------------------------------------------------------------
// Definition for output files
// ----------------------------------------------------------------------
#define EXTDIRLEN					MAX_PATH * 4	// Define searching directory length in TCHARs (MAX_PATH includes NULL)
#define OLD_COMPUTERNAMELEN			64				// Define COMPUTER name length, do not change
#define SIZEOF_INFOBOX				4096			// Define snapshot summary size
#define REGSHOT_BUFFER_BLOCK_BYTES	1024

// ----------------------------------------------------------------------
// IMPLEMENTED STRUCTURES:
// ----------------------------------------------------------------------
// Structure used for counts of keys, values, directories and files
// ----------------------------------------------------------------------
struct _COUNTS
{
	DWORD cAll;			// Count for all digital artifacts
	DWORD cKeys;		// Count for Registry keys
	DWORD cValues;		// Count for Registry values
	DWORD cDirs;		// Count for file system directories
	DWORD cFiles;		// Count for file system files
	DWORD cFilesBlacklist;		// Count for blacklisted file system files
	DWORD cValuesBlacklist;		// Count for blacklisted Registry values
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
	DWORD  nFileSizeLow;                    // File size [LOW  DWORD]
	DWORD  nFileSizeHigh;                   // File size [HIGH DWORD]
	DWORD  nFileAttributes;                 // File attributes (e.g. directory)
	LPTSTR lpszSHA1;						// SHA1 hash of file
	LPTSTR lpszMD5;							// MD5 hash of file
	size_t cchSHA1;							// Length of file's SHA1 in chars
	size_t cchMD5;							// Length of file's MD5 in chars
	struct _FILECONTENT FAR *lpFirstSubFC;  // Pointer to file's first sub file
	struct _FILECONTENT FAR *lpBrotherFC;   // Pointer to file's brother
	struct _FILECONTENT FAR *lpFatherFC;    // Pointer to file's father
	DWORD  fFileMatch;                      // Flags used when comparing, until 1.8.2 it was byte
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
	LPKEYCONTENT  lpHKLM;                   // Pointer to snapshots HKLM registry keys
	LPKEYCONTENT  lpHKU;                    // Pointer to snapshots HKU registry keys
	LPHEADFILE    lpHF;                     // Pointer to snapshots head files
	LPTSTR        lpszComputerName;         // Pointer to snapshots computer name
	LPTSTR        lpszUserName;             // Pointer to snapshots user name
	SYSTEMTIME    systemtime;
	COUNTS        stCounts;
	BOOL          fFilled;                  // Flag if Shot was done/loaded (even if result is empty)
	BOOL          fLoaded;                  // Flag if Shot was loaded from a file
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

// ----------------------------------------------------------------------
// External variables
// ----------------------------------------------------------------------
extern COMPRESULTS CompareResult;	// Compare result
extern DWORD NBW;					// NumberOfBytesWritten (used for output to text files)
extern SNAPSHOT Shot1;				// Snapshot One
extern SNAPSHOT Shot2;				// Snapshot Two
extern LPBYTE lpFileBuffer;
extern LPTSTR lpStringBuffer;
extern size_t nStringBufferSize;
extern size_t nSourceSize;
extern LPTSTR lpszMessage;			//Used in debug? Needed?!
extern LPTSTR lpszExtDir;			// External variable for output file 
extern LPTSTR lpszOutputPath;		// External variable for output file path
extern LPTSTR lpszWindowsDirName;	// External variable for Windows directory name
extern LPTSTR lpszEmpty;

//-------------------------------------------------------------
// Global LiveDiff variables
//-------------------------------------------------------------
extern HWND hWnd;				// The handle of LiveDiff
extern HANDLE hFile;            // Handle of file regshot use
extern HANDLE hFileWholeReg;	// Handle of file regshot use
extern LPSNAPSHOT lpMenuShot;	// Pointer to current Shot for popup menus and alike
extern BOOL fUseLongRegHead;	// Flag for compatibility with Regshot 1.61e5 and undoreg 1.46

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

//-------------------------------------------------------------
// Global LiveDiff function definitions
//-------------------------------------------------------------
// livediff.c global functions
VOID determineWindowsVersion();
BOOL snapshotLoad(LPTSTR loadFileName1, LPTSTR loadFileName2);
BOOL snapshotProfile();
BOOL snapshotProfileReboot();

// regshot.c global functions
VOID RegShot(LPSNAPSHOT lpShot);
BOOL DirChainMatch(LPHEADFILE lpHF1, LPHEADFILE lpHF2);
VOID CreateNewResult(DWORD nActionType, LPVOID lpContentOld, LPVOID lpContentNew);
VOID CompareShots(VOID);
VOID CompareHeadFiles(LPHEADFILE lpStartHF1, LPHEADFILE lpStartHF2);
VOID SaveShot(LPSNAPSHOT lpShot, LPTSTR lpszFileName);
BOOL LoadShot(LPSNAPSHOT lpShot, LPTSTR lpszFileName);
VOID ClearRegKeyMatchFlags(LPKEYCONTENT lpKC);
VOID ClearHeadFileMatchFlags(LPHEADFILE lpHF);
VOID FreeAllHeadFiles(LPHEADFILE lpHF);
VOID FreeShot(LPSNAPSHOT lpShot);
size_t AdjustBuffer(LPVOID *lpBuffer, size_t nCurrentSize, size_t nWantedSize, size_t nAlign);
VOID FreeCompareResult(void);
LPTSTR TransformValueData(LPBYTE lpData, PDWORD lpcbData, DWORD nConversionType);
LPTSTR ParseValueData(LPBYTE lpData, PDWORD lpcbData, DWORD nTypeCode);
LPTSTR GetValueDataType(DWORD nTypeCode);

// fileshot.c global functions
VOID FileShot(LPSNAPSHOT lpShot);
LPTSTR GetWholeFileName(LPFILECONTENT lpStartFC, size_t cchExtra);
VOID SaveHeadFiles(LPSNAPSHOT lpShot, LPHEADFILE lpHF, DWORD nFPCaller);
VOID LoadHeadFiles(LPSNAPSHOT lpShot, DWORD ofsHeadFile, LPHEADFILE *lplpCaller);

// blacklist.c global function
void TrieCreate(trieNode_t **root);
void TrieAdd(trieNode_t **root, wchar_t *key);
void TrieRemove(trieNode_t **root, wchar_t *key);
void TrieDestroy(trieNode_t* root);
trieNode_t *TrieCreateNode(wchar_t key);
trieNode_t* TrieSearch(trieNode_t *root, const wchar_t *key);
BOOL TrieSearchPath(trieNode_t *root, const wchar_t *key);

// output.c global functions
BOOL OutputComparisonResult(VOID);
LPTSTR GetWholeKeyName(LPKEYCONTENT lpStartKC, BOOL fUseLongNames);
LPTSTR GetWholeValueName(LPVALUECONTENT lpVC, BOOL fUseLongNames);
BOOL OpenAPXMLReport(LPTSTR lpszAppName);
BOOL reOpenAPXMLReport(LPTSTR lpszAppName);
BOOL GenerateAPXMLReport(VOID);
VOID DisplayShotInfo(LPSNAPSHOT lpShot);
VOID DisplayResultInfo();
VOID SetTextsToDefaultLanguage(VOID);

// dfxml.c global functions
VOID PopulateFileObject(HANDLE hFile, DWORD nActionType, LPFILECONTENT lpCR);
VOID PopulateCellObject(HANDLE hFile, DWORD nActionType, LPCOMPRESULT lpCR);
LPTSTR CalculateSHA1(LPTSTR FileName);
LPTSTR CalculateMD5(LPTSTR FileName);
VOID StartAPXML(LPTSTR lpszStartDate, LPTSTR lpszAppName, LPTSTR lpszAppVersion, LPTSTR lpszCommandLine, LPTSTR lpszWindowsVersion);
VOID StartAPXMLTag(LPTSTR tag);
VOID EndAPXMLTag(LPTSTR tag);
VOID EndAPXML();

LPTSTR xml_ampersand_check(LPTSTR value);
LPTSTR xml_apos_check(LPTSTR value);
LPTSTR xml_quote_check(LPTSTR value);
LPTSTR xml_gt_check(LPTSTR value);
LPTSTR xml_lt_check(LPTSTR value);
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
