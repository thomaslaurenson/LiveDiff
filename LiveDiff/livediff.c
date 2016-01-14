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

HANDLE hHeap;				// LiveDiff heap
LPSNAPSHOT lpShot;			// Pointer to current Shot
LPTSTR lpszCommandline;		// The LiveDiff command line
LPTSTR lpszWindowsVersion;	// The Windows version number
LPTSTR lpszStartDate;		// Program start date

LPTSTR lpsz_app_state;

//-----------------------------------------------------------------
// LiveDiff wmain function
//-----------------------------------------------------------------
int wmain(DWORD argc, TCHAR *argv[])
{
	clock_t startProgramClockTimer;	// Start timer
	clock_t endProgramClockTimer;	// End timer
	int msec;						// Millisecond variable for timer
	hHeap = GetProcessHeap();		// Initialise heap
	SetTextsToDefaultLanguage();	// Initialise common strings

	// Start timing program
	startProgramClockTimer = clock(), endProgramClockTimer;

	// Determine start date and time in ISO 8601 format
	SYSTEMTIME stStartTime;
	GetLocalTime(&stStartTime);
	lpszStartDate = MYALLOC0(21 * sizeof(TCHAR));
	_sntprintf(lpszStartDate, 21, TEXT("%i-%02i-%02iT%02i:%02i:%02iZ"),
		stStartTime.wYear, stStartTime.wMonth, stStartTime.wDay, stStartTime.wHour, stStartTime.wMinute, stStartTime.wSecond);

	// Print a pretty banner...
	printf("\n.____    .__               ________  .__  _____  _____ \n");
	printf("|    |   |__|__  __ ____   \\_____  \\ |__|/ ____\\/ ____\\\n");
	printf("|    |   |  \\  \\/ // __ \\   |   |   \\|  \\   __\\\\   __\\\\ \n");
	printf("|    |___|  |\\   /\\  ___/   |   `    \\  ||  |   |  |   \n");
	printf("|_______ \\__| \\_/  \\___  > /_________/__||__|   |__|   \n");
	printf("        \\/             \\/                              \n");
	printf("                                 By Thomas Laurenson\n");
	printf("                                 thomaslaurenson.com\n\n\n");

	// Set up snapshots and compare results structures
	ZeroMemory(&Shot1, sizeof(Shot1));
	ZeroMemory(&Shot2, sizeof(Shot2));
	ZeroMemory(&CompareResult, sizeof(CompareResult));

	// Initialise main argument variables
	saveSnapShots = FALSE;
	includeBlacklist = FALSE;
	performBlacklistFiltering = FALSE;
	performSHA1Hashing = FALSE;
	performMD5Hashing = FALSE;
	LPTSTR loadFileName1 = NULL;
	LPTSTR loadFileName2 = NULL;
	lpszCommandline = MYALLOC0(100 * sizeof(TCHAR));

	// Determine the Windows version number
	determineWindowsVersion();

	// modeOfOperation dictates what to do:
	// LOAD = load one or two snapshots, then compare (triggered with "--load" argument)
	// PROFILE = looped snapshot comparison procedure (default)
	// PROFILEREBOOT = continue profile mode after reboot
	LPTSTR modeOfOperation = TEXT("PROFILE");

	//-----------------------------------------------------------------
	// If an argument is given (e.g., LiveDiff.exe -s, LiveDiff.exe -h) parse arguments
	//-----------------------------------------------------------------
	if (argc > 1){
		// Print help menu if -h, --help or /?
		if ((_tcscmp(argv[1], _T("-h")) == 0) || (_tcscmp(argv[1], _T("--help")) == 0) || (_tcscmp(argv[1], _T("/?")) == 0)) {
			printf("Description: LiveDiff.exe is a differential analysis tool that captures a\n");
			printf("             snapshot of the file system and Windows Registry before and\n");
			printf("             after a system change (e.g., installing an application). The\n");
			printf("             snapshots are compared and system changes reported using the\n");
			printf("             DFXML forensic data abstraction.\n\n");
			printf("      Usage: LiveDiff.exe [mode] [options]\n\n");
			printf("       Mode: Operational mode for LiveDiff [default --scan]\n");
			printf("             --profile         Profile mode, generate APXML profile\n");
			printf("             --profile-reboot  Profile mode after system reboot\n");
			printf("             --load            Load one, or two snapshot files\n\n");
			printf("    Options: -s Save snapshot files [default FALSE]\n");
			printf("             -b Use dynamic blacklists [default TRUE]\n");
			printf("             -k Use static blacklists [default FALSE]\n");
			printf("             -c Select hash algorithm [default SHA1]\n\n");
			printf("   Examples: LiveDiff.exe\n");
			printf("             LiveDiff.exe --profile\n\n");
			printf("             LiveDiff.exe -s\n");
			printf("             LiveDiff.exe -s -b\n");
			printf("             LiveDiff.exe -c md5,sha1\n");
			printf("             LiveDiff.exe -c md5 -s\n");
			printf("             LiveDiff.exe --load 1.shot 2.shot\n");
			return 0;
		}
		
		// Scan the command line arguments and set booleans
		for (DWORD i = 0; i < argc; i++)
		{
			// Determine if we are loading files
			if (_tcscmp(argv[i], _T("--load")) == 0) {
				modeOfOperation = TEXT("LOAD");
				if (NULL != argv[i + 1]){
					loadFileName1 = argv[i + 1];
				}
				if (NULL != argv[i + 2]){
					loadFileName2 = argv[i + 2];
				}
			}
			// Determine if we are creating an APXML profile
			if (_tcscmp(argv[i], _T("--profile")) == 0){
				modeOfOperation = TEXT("PROFILE");
			}
			// Determine if we are creating an APXML profile (but reboot phase)
			if (_tcscmp(argv[i], _T("--profile-reboot")) == 0){
				modeOfOperation = TEXT("PROFILEREBOOT");
			}
			// Determine if we are saving snapshots
			if (_tcscmp(argv[i], _T("-s")) == 0) {
				saveSnapShots = TRUE;
			}
			// Detemine if we are performing dynamic blacklisting
			if (_tcscmp(argv[i], _T("-b")) == 0) {
				dwBlacklist = 1;
			}
			if (_tcscmp(argv[i], _T("-c")) == 0) {
			    if (_tcscmp(argv[i+1], _T("md5")) == 0) {
			        performMD5Hashing = TRUE;
			    }
                else if (_tcscmp(argv[i+1], _T("sha1")) == 0) {
			        performSHA1Hashing = TRUE;
			    }
                else if (_tcscmp(argv[i+1], _T("md5,sha1")) == 0) {
			        performMD5Hashing = TRUE;
			        performSHA1Hashing = TRUE;
			    }			    
			}			
			// Append command line arguments to string
			_tcscat(lpszCommandline, argv[i]);
			if (i < argc - 1) {
				_tcscat(lpszCommandline, TEXT(" "));
			}
		}
	}

	if (dwBlacklist == 1)
	{
		// Call blacklist preparation functions
		generateBlacklist();
		// Turn on SHA1 file hasing:
		// Only perform SHA1 file hashing is blacklist is included in the scanning process
		// Reason: The performance overhead to hash every file is too computationally intensive
		performSHA1Hashing = TRUE;
		dwBlacklist = 2;
	}

	// From here, call the appropriate function for mode of operation selected by user
	if (modeOfOperation == TEXT("LOAD"))
	{
		// Call load function
		snapshotLoad(loadFileName1, loadFileName2);
		// Print program clock timer
		endProgramClockTimer = clock() - startProgramClockTimer;
		msec = endProgramClockTimer * 1000 / CLOCKS_PER_SEC;
		printf("\n>>> Program run time: %d seconds %d milliseconds\n", msec / 1000, msec % 1000);

		// Done. So exit!
		printf("\n>>> Finished.\n");
	}
	else if (modeOfOperation == TEXT("PROFILE"))
	{
		// Call profile function
		snapshotProfile();
		// Print program clock timer
		endProgramClockTimer = clock() - startProgramClockTimer;
		msec = endProgramClockTimer * 1000 / CLOCKS_PER_SEC;
		printf("\n>>> Program run time: %d seconds %d milliseconds\n", msec / 1000, msec % 1000);
	}
	else if (modeOfOperation == TEXT("PROFILEREBOOT"))
	{
		// Call profile function
		snapshotProfileReboot();
		// Print program clock timer
		endProgramClockTimer = clock() - startProgramClockTimer;
		msec = endProgramClockTimer * 1000 / CLOCKS_PER_SEC;
		printf("\n>>> Program run time: %d seconds %d milliseconds\n", msec / 1000, msec % 1000);
	}
	return 0;
}

//-----------------------------------------------------------------
// LiveDiff: Dynamic blacklist generation
//-----------------------------------------------------------------
BOOL generateBlacklist()
{
	printf("\n>>> CREATING DYNAMIC BLACKLISTS...\n");

	TrieCreate(&blacklistFILES);
	TrieCreate(&blacklistHKLM);
	TrieCreate(&blacklistHKU);

	// SHOT ONE
	lpShot = &Shot1;
	printf("  > Scanning Windows Registry...\n");
	RegShot(lpShot);
	printf("  > Scanning Windows File System...\n");
	FileShot(lpShot);

	// We are done here! Free the shot. Return TRUE.
	FreeShot(&Shot1);
	return TRUE;
}

//-----------------------------------------------------------------
// LiveDiff: Perform system snapshot One
//-----------------------------------------------------------------
BOOL performShotOne()
{
	clock_t startClockTimer, endClockTimer;
	int msec;

	printf("\n\n>>> SHOT 1\n");
	printf("  > Press ENTER to start scanning system ...");
	getchar();
	startClockTimer = clock(), endClockTimer;
	lpShot = &Shot1;
	printf("  > Scanning Windows Registry...\n");
	RegShot(lpShot);
	printf("  > Scanning Windows File System...\n");
	FileShot(lpShot);
	// Determine time and print it
	endClockTimer = clock() - startClockTimer;
	msec = endClockTimer * 1000 / CLOCKS_PER_SEC;
	printf("  > Scanning time: %d seconds %d milliseconds\n", msec / 1000, msec % 1000);
	// If requested, save snapshot 1
	if (saveSnapShots) {
		printf("  > Saving snapshot 1...\n");
		LPTSTR lpszFileName = TEXT("LD1.shot");
		SaveShot(lpShot, lpszFileName);
		wprintf(L"  > Snapshot sucessfully saved as: %s\n", lpszFileName);
	}
	// Display the information from the shot
	DisplayShotInfo(lpShot);

	return TRUE;
}

//-----------------------------------------------------------------
// LiveDiff: Perform system snapshot Two
//-----------------------------------------------------------------
BOOL performShotTwo()
{
	clock_t startClockTimer, endClockTimer;
	int msec;

	printf("\n\n>>> SHOT 2\n");
	printf("  > Press ENTER to start scanning system ...");
	getchar();
	startClockTimer = clock(), endClockTimer;
	lpShot = &Shot2;
	printf("  > Scanning Windows Registry...\n");
	RegShot(lpShot);
	printf("  > Scanning Windows File System...\n");
	FileShot(lpShot);
	// Determine time and print it
	endClockTimer = clock() - startClockTimer;
	msec = endClockTimer * 1000 / CLOCKS_PER_SEC;
	printf("  > Scanning time: %d seconds %d milliseconds\n", msec / 1000, msec % 1000);
	// If requested, save snapshot 2
	if (saveSnapShots) {
		printf("  > Saving snapshot 2...\n");
		LPTSTR lpszFileName = TEXT("LD2.shot");
		SaveShot(lpShot, lpszFileName);
		wprintf(L"  > Snapshot sucessfully saved as: %s\n", lpszFileName);
	}
	// Display the information from the shot
	DisplayShotInfo(lpShot);

	return TRUE;
}


//-----------------------------------------------------------------
// LiveDiff Loading mode of operation (load 1 or 2 snapshots)
//-----------------------------------------------------------------
BOOL snapshotLoad(LPTSTR loadFileName1, LPTSTR loadFileName2)
{
	// Print loading banner
	printf("\n>>> LOADING MODE...\n");
	printf("  > Loading one (1) or two (2) snapshots, then compare...\n");
	if (NULL != loadFileName1)
	{
		// SHOT ONE (at least one snapshot is always loaded in LOAD mode)
		printf("\n\n>>> SHOT 1\n");
		wprintf(L"  > Loading snapshot 1: %s\n", loadFileName1);
		lpShot = &Shot1;
		LoadShot(lpShot, loadFileName1); // Fix too many parameters, fix by copying to LPTSTR?!
		DisplayShotInfo(lpShot);
	}
	else
	{
		printf("  > You need to specify at least one snapshot file to load... Exiting.\n");
		return TRUE;
	}

	// If there is another filename, load or take shot2
	if (NULL != loadFileName2)
	{
		// SHOT TWO 
		printf("\n\n>>> SHOT 2\n");
		wprintf(L"  > Loading snapshot 2: %s\n", loadFileName2);
		lpShot = &Shot2;
		LoadShot(lpShot, loadFileName2);
		DisplayShotInfo(lpShot);
	}
	else
	{
		// SHOT TWO
		performShotTwo();
	}

	// Compare Shot 1 and Shot 2 and display result overview
	printf("\n>>> Comparing snapshots...\n");
	CompareShots();
	DisplayResultInfo();

	// Now, produce DFXML and RegXML reports
	printf("\n>>> Generating output...\n");
	// Need to update to output to APXML here
	//OutputComparisonResult();

	// Done. So exit!
	printf("\n>>> Finished.\n");
	return TRUE;
}

//-----------------------------------------------------------------
// LiveDiff PROFILE mode of operation (create an APXML document)
//-----------------------------------------------------------------
BOOL snapshotProfile()
{
	clock_t startClockTimer, endClockTimer;
	int msec;
	LPTSTR lpszLifeCycleState;
	LPTSTR lpszContinueScanning;
	BOOL boolContinueScanning = TRUE;
	BOOL boolSaveSnapshot = FALSE;
	LPTSTR lpszAppName;
	LPTSTR lpszAppVersion;
	LPTSTR lpszAPXMLFileName;

	// Enter interactive application life cycle scanning mode
	printf("\n>>> APPLICATION PROFILE MODE...\n");
	printf("  > Perform numerous system scans to create APXML report...\n");

	// Get application name from user
	printf("  > Enter application name: ");
	lpszAppName = MYALLOC0(100 * sizeof(TCHAR));
	fgetws(lpszAppName, 100 * sizeof(lpszAppName), stdin);
	if (lpszAppName[_tcslen(lpszAppName) - 1] == (TCHAR)'\n') {
		lpszAppName[_tcslen(lpszAppName) - 1] = (TCHAR)'\0';
	}

	// Get profiled application version from user
	printf("  > Enter application version: ");
	lpszAppVersion = MYALLOC0(100 * sizeof(TCHAR));
	fgetws(lpszAppVersion, 100 * sizeof(lpszAppVersion), stdin);
	if (lpszAppVersion[_tcslen(lpszAppVersion) - 1] == (TCHAR)'\n') {
		lpszAppVersion[_tcslen(lpszAppVersion) - 1] = (TCHAR)'\0';
	}

	// Specify the APXML file name ("Program-Version-WindowsVersion.apxml")
	lpszAPXMLFileName = MYALLOC0(100 * sizeof(TCHAR));
	_sntprintf(lpszAPXMLFileName, 100, TEXT("%s-%s-%s"), lpszAppName, lpszAppVersion, lpszWindowsVersion);
	
	// Open the APXML report and populate XML header
	OpenAPXMLReport(lpszAPXMLFileName);
	StartAPXML(lpszStartDate, lpszAppName, lpszAppVersion, lpszCommandline, lpszWindowsVersion);

	// Blacklist processing
	if (dwBlacklist = 1) {
		generateBlacklist();				// Generate blacklists
		dwBlacklist = 2;					// Turn on perform blacklisting
		performSHA1Hashing = TRUE;			// Turn on SHA1 file hasing:
	}

	// LOOP SNAPSHOT PROCESS... until exited by user
	size_t loopCount = 0;
	do
	{
		// Get user input to determine life cycle state
		printf("\n>>> Starting life cycle scanning...\n");
		printf("  > Enter application life cycle phase (e.g., install): ");
		lpszLifeCycleState = MYALLOC0(100 * sizeof(TCHAR));
		fgetws(lpszLifeCycleState, 100 * sizeof(lpszLifeCycleState), stdin);
		// Remove newline character
		if (lpszLifeCycleState[_tcslen(lpszLifeCycleState) - 1] == (TCHAR)'\n') {
			lpszLifeCycleState[_tcslen(lpszLifeCycleState) - 1] = (TCHAR)'\0';
		}

		if (loopCount > 0)
		{
			// SHOT ONE (A copy of Snapshot2 from previous round)
			printf("\n\n>>> SHOT 1\n");
			printf("  > Copying previous snapshot...\n");
			
			// Copy snapshots and result comparison results
			SNAPSHOT ShotTemp;
			memcpy(&ShotTemp, &Shot1, sizeof(Shot1));     // backup Shot1 in ShotTemp
			memcpy(&Shot1, &Shot2, sizeof(Shot2));        // copy Shot2 to Shot1
			memcpy(&Shot2, &ShotTemp, sizeof(ShotTemp));  // copy ShotTemp (Shot1) to Shot2
			FreeCompareResult();
			ClearRegKeyMatchFlags(Shot1.lpHKLM);
			ClearRegKeyMatchFlags(Shot2.lpHKLM);
			ClearRegKeyMatchFlags(Shot1.lpHKU);
			ClearRegKeyMatchFlags(Shot2.lpHKU);
			ClearHeadFileMatchFlags(Shot1.lpHF);
			ClearHeadFileMatchFlags(Shot2.lpHF);
			FreeShot(&Shot2);

			// Display the information from the shot
			DisplayShotInfo(&Shot1);

			// Save snapshot1 if we are rebooting
			if ((_tcscmp(lpszLifeCycleState, _T("reboot")) == 0))
			{
				printf("  > Saving snapshot 1...\n");
				LPTSTR lpszFileName = TEXT("LD1.shot");
				SaveShot(&Shot1, lpszFileName);
				wprintf(L"  > Snapshot sucessfully saved as: %s\n", lpszFileName);

				// Done with this phase, user needs to reboot and load
				printf("\n>>> Finished. Reboot system. Then run 'LiveDiff.exe --profile-reboot'\n\n");
				return TRUE;
			}
		}
		else
		{
			// SHOT ONE
			printf("\n\n>>> SHOT 1\n");
			printf("  > Press ENTER to start scanning system ...");
			getchar();
			startClockTimer = clock(), endClockTimer;
			lpShot = &Shot1;
			printf("  > Scanning Windows Registry...\n");
			RegShot(lpShot);
			printf("  > Scanning Windows File System...\n");
			FileShot(lpShot);
			// Determine time and print it
			endClockTimer = clock() - startClockTimer;
			msec = endClockTimer * 1000 / CLOCKS_PER_SEC;
			printf("  > Scanning time: %d seconds %d milliseconds\n", msec / 1000, msec % 1000);
			// Save snapshot one
			if ((_tcscmp(lpszLifeCycleState, _T("reboot")) == 0))
			{
				printf("  > Saving snapshot 1...\n");
				LPTSTR lpszFileName = TEXT("LD1.shot");
				SaveShot(lpShot, lpszFileName);
				wprintf(L"  > Snapshot sucessfully saved as: %s\n", lpszFileName);

				// Done with this phase, user needs to reboot and load
				printf("\n>>> Finished. Reboot system. Then run 'LiveDiff.exe -pr'\n\n");
				return TRUE;
			}
			// Display the information from the shot
			DisplayShotInfo(lpShot);
		}

		// SHOT TWO
		performShotTwo();

		// Compare Shot 1 and Shot 2 and display result overview
		printf("\n>>> Comparing snapshots...\n");
		CompareShots();
		DisplayResultInfo();

		// Insert comparison results to parent tag body
		printf("\n>>> Generating output...\n");
		//StartAPXMLTag(lpszLifeCycleState);
		size_t cchlpszLifeCycleState = _tcslen(lpszLifeCycleState);
		lpsz_app_state = MYALLOC0((cchlpszLifeCycleState + 1) * sizeof(TCHAR));
		_tcscpy(lpsz_app_state, lpszLifeCycleState);
		GenerateAPXMLReport();
		//EndAPXMLTag(lpszLifeCycleState);

		// Done.
		printf("\n>>> Finished.\n");

		// Check if user wants to continue
		printf("  > Perform another scan? Select [Y] or N: ");
		lpszContinueScanning = MYALLOC0(2 * sizeof(TCHAR));
		fgetws(lpszContinueScanning, 2 * sizeof(lpszContinueScanning), stdin);
		if (lpszContinueScanning[_tcslen(lpszContinueScanning) - 1] == (TCHAR)'\n') {
			lpszContinueScanning[_tcslen(lpszContinueScanning) - 1] = (TCHAR)'\0';
		}

		if ((_tcscmp(lpszContinueScanning, _T("N")) == 0) || (_tcscmp(lpszContinueScanning, _T("n")) == 0))	{
			boolContinueScanning = FALSE;
		}
		  
		// Free stuff
		MYFREE(lpszLifeCycleState);
		MYFREE(lpszContinueScanning);

		// Increase the loopCount
		loopCount++;

	} while (boolContinueScanning);

	// Close APXML report tag
	EndAPXML();

	// Done.
	return TRUE;
}

//-----------------------------------------------------------------
// LiveDiff PROFILE mode of operation (REBOOT function)
//-----------------------------------------------------------------
BOOL snapshotProfileReboot()
{
	LPTSTR lpszLifeCycleState = TEXT("reboot");

	// CREATE AND LOAD BLACKLIST
	// This is broken now. Because we no longer use a text blacklist.
	// Alas, how to perform blacklisting when blacklist is stored in volatile memory??
	// SOLUTION: Populate blacklist to text file, then reboot
	
	includeBlacklist = TRUE;
	if (includeBlacklist) {
		//loadBlacklist();			// Load the previously created blacklist
		performSHA1Hashing = TRUE;	// Turn on SHA1 file hasing
		performBlacklistFiltering = TRUE;	// Turn on perform blacklisting
	}

	// Enter interactive application life cycle scanning mode
	printf("\n>>> APPLICATION PROFILE MODE (REBOOT)...\n");
	printf("  > Load pre-reboot Shot1, capture Shot2, then finish APXML report...\n");

	// Find the APXML file
	WIN32_FIND_DATA fileData;
	FindFirstFile(TEXT("*.apxml"), &fileData);
	wprintf(L"  > Opening APXML Report: %s\n", fileData.cFileName);

	// Open the APXML report and populate XML header
	reOpenAPXMLReport(fileData.cFileName);

	LPTSTR loadFileName1 = TEXT("LD1.shot");
	// SHOT ONE (shot1 is always loaded in profileReboot mode)
	printf("\n\n>>> SHOT 1\n");
	wprintf(L"  > Loading snapshot 1: %s\n", loadFileName1);
	lpShot = &Shot1;
	LoadShot(lpShot, loadFileName1);
	DisplayShotInfo(lpShot);

	// SHOT TWO
	performShotTwo();

	// Compare Shot 1 and Shot 2 and display result overview
	printf("\n>>> Comparing snapshots...\n");
	CompareShots();
	DisplayResultInfo();

	// Insert life cycle report body
	printf("\n>>> Generating output...\n");
	StartAPXMLTag(lpszLifeCycleState);
	GenerateAPXMLReport();
	EndAPXMLTag(lpszLifeCycleState);

	// Close APXML report tag
	EndAPXML();

	// Done.
	printf("\n>>> Finished.\n");

	return TRUE;
}

//-----------------------------------------------------------------
// LiveDiff: Determine the version of Windows we are running on
//-----------------------------------------------------------------
// Taken from:
// http://stackoverflow.com/questions/25986331/how-to-determine-windows-version-in-future-proof-way/25986612#25986612
VOID determineWindowsVersion()
{
	static const wchar_t kernel32[] = L"\\kernel32.dll";
	wchar_t *path = NULL;
	void *ver = NULL, *block;
	UINT n;
	BOOL r;
	DWORD versz, blocksz;
	VS_FIXEDFILEINFO *vinfo;

	path = malloc(sizeof(*path) * MAX_PATH);
	if (!path)
		abort();

	n = GetSystemDirectory(path, MAX_PATH);
	if (n >= MAX_PATH || n == 0 ||
		n > MAX_PATH - sizeof(kernel32) / sizeof(*kernel32))
		abort();
	memcpy(path + n, kernel32, sizeof(kernel32));

	versz = GetFileVersionInfoSize(path, NULL);
	if (versz == 0)
		abort();
	ver = malloc(versz);
	if (!ver)
		abort();
	r = GetFileVersionInfo(path, 0, versz, ver);
	if (!r)
		abort();
	r = VerQueryValue(ver, L"\\", &block, &blocksz);
	if (!r || blocksz < sizeof(VS_FIXEDFILEINFO))
		abort();
	vinfo = (VS_FIXEDFILEINFO *)block;

	lpszWindowsVersion = MYALLOC0(30 * sizeof(TCHAR));
	_sntprintf(lpszWindowsVersion, 30, TEXT("%d.%d.%d"),
		(int)HIWORD(vinfo->dwProductVersionMS),
		(int)LOWORD(vinfo->dwProductVersionMS),
		(int)HIWORD(vinfo->dwProductVersionLS));

	free(path);
	free(ver);
}
