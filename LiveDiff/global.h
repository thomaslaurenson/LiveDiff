/*
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
BOOL includeBlacklist;
BOOL performBlacklistFiltering;
BOOL performSHA1Hashing;
BOOL performMD5Hashing;
BOOL saveSnapShots;

LPTSTR lpsz_app_state;

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
#define MAXAMOUNTOFFILE				10000			// Max output file counts (e.g., report_0001.dfxml, report_0002.dfxml)
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
// Structure for saving or loading snapshots
// ----------------------------------------------------------------------
struct _FILEHEADER
{
	char signature[12]; // ofs 0 len 12: never convert to Unicode, always use char type and SBCS/MBCS
	DWORD nFHSize;      // (v2) ofs 12 len 4: size of file header incl. signature and header size field

	DWORD ofsHKLM;      // ofs 16 len 4
	DWORD ofsHKU;       // ofs 20 len 4
	DWORD ofsHF;        // ofs 24 len 4: added in 1.8
	DWORD reserved;     // ofs 28 len 4

	// next two fields are kept and filled for <= 1.8.2 compatibility, see substituting fields in file header format version 2
	char computername[OLD_COMPUTERNAMELEN];  // ofs 32 len 64: DO NOT CHANGE, keep as SBCS/MBCS, name maybe truncated or missing NULL char
	char username[OLD_COMPUTERNAMELEN];      // ofs 96 len 64: DO NOT CHANGE, keep as SBCS/MBCS, name maybe truncated or missing NULL char

	SYSTEMTIME systemtime;  // ofs 160 len 16

	// extended values exist only since structure version 2
	// new since file header version 2
	DWORD nFHVersion;        // (v2) ofs 176 len 4: File header structure version
	DWORD nCharSize;         // (v2) ofs 180 len 4: sizeof(TCHAR), allows to separate between SBCS/MBCS (1 for char) and Unicode (2 for UTF-16 WCHAR), may become 4 for UTF-32 in the future
	DWORD nKCVersion;        // (v2) ofs 184 len 4: Key content structure version
	DWORD nKCSize;           // (v2) ofs 188 len 4: Size of key content
	DWORD nVCVersion;        // (v2) ofs 192 len 4: Value content structure version
	DWORD nVCSize;           // (v2) ofs 196 len 4: Size of value content
	DWORD nHFVersion;        // (v2) ofs 200 len 4: Head File structure version
	DWORD nHFSize;           // (v2) ofs 204 len 4: Size of file content
	DWORD nFCVersion;        // (v2) ofs 208 len 4: File content structure version
	DWORD nFCSize;           // (v2) ofs 212 len 4: Size of file content
	DWORD ofsComputerName;   // (v2) ofs 216 len 4
	DWORD nComputerNameLen;  // (v2) ofs 220 len 4: length in chars incl. NULL char
	DWORD ofsUserName;       // (v2) ofs 224 len 4
	DWORD nUserNameLen;      // (v2) ofs 228 len 4: length in chars incl. NULL char
	DWORD nEndianness;       // (v2) ofs 232 len 4: 0x12345678

	// ^^^ here the file header structure can be extended
	// * increase the version number for the new file header format
	// * place a comment with a reference to the new version before the new fields
	// * check all usages and version checks
	// * remember that older versions cannot use these additional data
};
typedef struct _FILEHEADER FILEHEADER, FAR *LPFILEHEADER;

// Definitions for saving or loading file header versions
#define FILEHEADER_VERSION_EMPTY 0
#define FILEHEADER_VERSION_1 1
#define FILEHEADER_VERSION_2 2
#define FILEHEADER_VERSION_CURRENT FILEHEADER_VERSION_2
#define FILEHEADER_ENDIANNESS_VALUE 0x12345678

// ----------------------------------------------------------------------
// Structure for extra data when saving or loading snapshots
// ----------------------------------------------------------------------
struct _FILEEXTRADATA
{
	BOOL bSameCharSize;
	BOOL bSameEndianness;
	BOOL bOldKCVersion;
	BOOL bOldVCVersion;
	BOOL bOldFCVersion;
};
typedef struct _FILEEXTRADATA FILEEXTRADATA, FAR *LPFILEEXTRADATA;

// ----------------------------------------------------------------------
// Structure for saving or loading Registry keys from snapshot
// ----------------------------------------------------------------------
struct _SAVEKEYCONTENT
{
	DWORD ofsKeyName;      // Position of key's name
	DWORD ofsFirstValue;   // Position of key's first value
	DWORD ofsFirstSubKey;  // Position of key's first sub key
	DWORD ofsBrotherKey;   // Position of key's brother key
	DWORD ofsFatherKey;    // Position of key's father key
	// extended values exist only since structure version 2
	// new since key content version 2
	DWORD nKeyNameLen;  // (v2) Length of key's name in chars incl. NULL char (up to 1.8.2 there a was single byte for internal fKeyMatch field that was always zero)
	// ^^^ here the key content structure can be extended
	// * increase the version number for the new key content format
	// * place a comment with a reference to the new version before the new fields
	// * check all usages and version checks
	// * remember that older versions cannot use these additional data
};
typedef struct _SAVEKEYCONTENT SAVEKEYCONTENT, FAR *LPSAVEKEYCONTENT;

// Definitions for saving or loading Registry key content versions
#define KEYCONTENT_VERSION_EMPTY 0
#define KEYCONTENT_VERSION_1 1
#define KEYCONTENT_VERSION_2 2
#define KEYCONTENT_VERSION_CURRENT KEYCONTENT_VERSION_2

// ----------------------------------------------------------------------
// Structure for saving or loading Registry values from snapshot
// ----------------------------------------------------------------------
struct _SAVEVALUECONTENT
{
	DWORD nTypeCode;        // Type of value [DWORD,STRING...]
	DWORD cbData;           // Value data size in bytes
	DWORD ofsValueName;     // Position of value's name
	DWORD ofsValueData;     // Position of value's data
	DWORD ofsBrotherValue;  // Position of value's brother value
	DWORD ofsFatherKey;     // Position of value's father key
	// extended values exist only since structure version 2
	// new since value content version 2
	DWORD nValueNameLen;  // (v2) Length of value's name in chars incl. NULL char (up to 1.8.2 there a was single byte for internal fValueMatch field that was always zero)
	// ^^^ here the value content structure can be extended
	// * increase the version number for the new value content format
	// * place a comment with a reference to the new version before the new fields
	// * check all usages and version checks
	// * remember that older versions cannot use these additional data
};
typedef struct _SAVEVALUECONTENT SAVEVALUECONTENT, FAR *LPSAVEVALUECONTENT;

// Definitions for saving or loading Registry value content versions
#define VALUECONTENT_VERSION_EMPTY 0
#define VALUECONTENT_VERSION_1 1
#define VALUECONTENT_VERSION_2 2
#define VALUECONTENT_VERSION_CURRENT KEYCONTENT_VERSION_2

// ----------------------------------------------------------------------
// Structure for saving or loading directories or files from snapshot
// ----------------------------------------------------------------------
struct _SAVEFILECONTENT
{
	DWORD ofsFileName;         // Position of file name
	DWORD nWriteDateTimeLow;   // File write time [LOW  DWORD]
	DWORD nWriteDateTimeHigh;  // File write time [HIGH DWORD]
	DWORD nFileSizeLow;        // File size [LOW  DWORD]
	DWORD nFileSizeHigh;       // File size [HIGH DWORD]
	DWORD nFileAttributes;     // File attributes
	DWORD nChkSum;             // File checksum (planned for the future, currently not used)
	DWORD ofsSHA1;	           // Position of file SHA1 value
	DWORD ofsFirstSubFile;     // Position of file's first sub file
	DWORD ofsBrotherFile;      // Position of file's brother file
	DWORD ofsFatherFile;       // Position of file's father file
	// extended values exist only since structure version 2
	// new since file content version 2
	DWORD nFileNameLen;  // (v2) Length of file's name in chars incl. NULL char (up to 1.8.2 there a was single byte for internal fFileMatch field that was always zero)
	DWORD nFileSHA1Len;
	// ^^^ here the file content structure can be extended
	// * increase the version number for the new file content format
	// * place a comment with a reference to the new version before the new fields
	// * check all usages and version checks
	// * remember that older versions cannot use these additional data
};
typedef struct _SAVEFILECONTENT SAVEFILECONTENT, FAR *LPSAVEFILECONTENT;

// Definitions for saving or loading file or directories versions
#define FILECONTENT_VERSION_EMPTY 0
#define FILECONTENT_VERSION_1 1
#define FILECONTENT_VERSION_2 2
#define FILECONTENT_VERSION_CURRENT KEYCONTENT_VERSION_2

// ----------------------------------------------------------------------
// Structure for saving or loading headfiles from snapshot
// ----------------------------------------------------------------------
struct _SAVEHEADFILE
{
	DWORD ofsBrotherHeadFile;   // Position of head file's brother head file
	DWORD ofsFirstFileContent;  // Position of head file's first file content
	// extended values exist only since structure version 2
	// ^^^ here the head file structure can be extended
	// * increase the version number for the new head file format
	// * place a comment with a reference to the new version before the new fields
	// * check all usages and version checks
	// * remember that older versions cannot use these additional data
};
typedef struct  _SAVEHEADFILE SAVEHEADFILE, FAR *LPSAVEHEADFILE;

// Definitions for saving or loading headfile versions
#define HEADFILE_VERSION_EMPTY 0
#define HEADFILE_VERSION_1 1
#define HEADFILE_VERSION_CURRENT HEADFILE_VERSION_1

// ----------------------------------------------------------------------
// External variables
// ----------------------------------------------------------------------
extern COMPRESULTS CompareResult;	// Compare result
extern DWORD NBW;					// NumberOfBytesWritten (used for output to text files)
extern SNAPSHOT Shot1;				// Snapshot One
extern SNAPSHOT Shot2;				// Snapshot Two
extern FILEHEADER fileheader;
extern FILEEXTRADATA fileextradata;
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
// BINARY TREE IMPLEMENTATION FOR BLACKLISTING FILE SYSTEM PATHS
//-------------------------------------------------------------
// treeNode structure for file system paths
// The tree splits file system paths at the backslash character
// and creates a tree in the following structure:
//              C:
//             /   \
//          Boot  Users
//          /        \
//      BCD.LOG    Administrator
//-------------------------------------------------------------
typedef struct treeNode_
{
	wchar_t *lpszFilePath;
	struct treeNode_ *left;
	struct treeNode_ *right;
} treeNode;

treeNode* rootHKLM;
treeNode* rootHKU;
treeNode* rootFILES;

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
trieNode_t *blacklistHKLM;
trieNode_t *blacklistHKU;

//-------------------------------------------------------------
// Global LiveDiff function definitions
//-------------------------------------------------------------
// livediff.c global functions
VOID determineWindowsVersion();
BOOL generateBlacklist();
BOOL populateTextBlacklist(LPTSTR lpszFileName, trieNode_t * blacklist);
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
BOOL TrieSearch1(trieNode_t *root, const wchar_t *key);

// output.c global functions
BOOL OutputComparisonResult(VOID);
VOID DisplayShotInfo(LPSNAPSHOT lpShot);
VOID DisplayResultInfo();
LPTSTR GetWholeKeyName(LPKEYCONTENT lpStartKC, BOOL fUseLongNames);
LPTSTR GetWholeValueName(LPVALUECONTENT lpVC, BOOL fUseLongNames);
BOOL OpenAPXMLReport(LPTSTR lpszAppName);
BOOL reOpenAPXMLReport(LPTSTR lpszAppName);
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
