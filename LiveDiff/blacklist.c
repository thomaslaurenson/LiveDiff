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

// Snippet to copy to main if testing required
/*
trieNode_t *root;
printf("Trie Example\n");

//Create a trie
TrieCreate(&root);

TrieAdd(&root, "C:\\");
//TrieAdd(&root, "autoexec.bat\0");
TrieAdd(&root, "C:\\autoexec.bat");
TrieAdd(&root, "C:\\automeow.bat");
TrieAdd(&root, "C:\\config.sys");

printf("DONE, now searching\n");
found = TrieSearch1(root->children, "C:\\fuckyou");
printf("%s\n", "C:\\fuckyou");
printf("%s\n", found ? "true" : "false");
found = TrieSearch1(root->children, "C:\\config.sys");
printf("%s\n", "C:\\config.sys");
printf("%s\n", found ? "true" : "false");
found = TrieSearch1(root->children, "C:\\config.sts");
printf("%s\n", "C:\\config.sts");
printf("%s\n", found ? "true" : "false");
//Destroy the trie
//TrieDestroy(root);

// From here we can try read a file in
printf("\n\n\nNOW DOING BLACKLIST\n\n");
//LPTSTR lpszFileBlacklist = TEXT("blacklist-FILES.txt");
LPTSTR lpszFileBlacklist = TEXT("blacklist-FILES-Copy.txt");
wchar_t line[MAX_PATH];
FILE *hFile;
TrieCreate(&blacklistFILES);

hFile = _wfopen(lpszFileBlacklist, L"rb, ccs=UTF-16LE");
//int i = 0;

while (fgetws(line, MAX_PATH, hFile))
{
// Remove newline character
if (line[_tcslen(line) - 1] == (TCHAR)'\n') {
line[_tcslen(line) - 1] = (TCHAR)'\0';
}
if (line[_tcslen(line) - 1] == (TCHAR)'\r') {
line[_tcslen(line) - 1] = (TCHAR)'\0';
}

size_t lenKey = _tcslen(line) + 1;
TrieAdd(&blacklistFILES, line);
}

// From here is a snapshot test
BOOL found = FALSE;
TrieCreate(&blacklistFILES);
TrieCreate(&blacklistHKLM);
TrieCreate(&blacklistHKU);
lpShot = &Shot1;
printf("  > Scanning Windows Registry...\n");
RegShot(lpShot);
printf("  > Scanning Windows File System...\n");
FileShot(lpShot);

found = TrieSearch1(blacklistFILES->children, TEXT("C:\\Windows\\System32\\DriverStore\\FileRepository\\mdmbr00a.inf_x86_neutral_f6d7a36091b1af37\\BrSerIb.sys"));
printf("%ws\n", TEXT("C:\\Windows\\System32\\DriverStore\\FileRepository\\mdmbr00a.inf_x86_neutral_f6d7a36091b1af37\\BrSerIb.sys"));
printf("%s\n", found ? "true" : "false");
found = TrieSearch1(blacklistFILES->children, TEXT("C:\\Windows\\System32\\DriverStore\\ANAL\\mdmbr00a.inf_x86_neutral_f6d7a36091b1af37\\BrSerIb.sys"));
printf("%ws\n", TEXT("C:\\Windows\\System32\\DriverStore\\ANAL\\mdmbr00a.inf_x86_neutral_f6d7a36091b1af37\\BrSerIb.sys"));
printf("%s\n", found ? "true" : "false");
found = TrieSearch1(blacklistFILES->children, TEXT("C:\\Program Files\\TrueCrypt\\License.txt"));
printf("%ws\n", TEXT("C:\\Program Files\\TrueCrypt\\License.txt"));
printf("%s\n", found ? "true" : "false");
found = TrieSearch1(blacklistFILES->children, TEXT("Program Files\\TrueCrypt\\License.txt"));
printf("%ws\n", TEXT("Program Files\\TrueCrypt\\License.txt"));
printf("%s\n", found ? "true" : "false");

found = TrieSearch1(blacklistHKLM->children, TEXT("HKLM\\SYSTEM\\ControlSet001\\Control\\CriticalDeviceDatabase\\PCI#VEN_8086&DEV_2682&SUBSYS_03321014\\Service"));
printf("%ws\n", TEXT("HKLM\\SYSTEM\\ControlSet001\\Control\\CriticalDeviceDatabase\\PCI#VEN_8086&DEV_2682&SUBSYS_03321014\\Service"));
printf("%s\n", found ? "true" : "false");

found = TrieSearch1(blacklistHKU->children, TEXT("HKU\\S-1-5-18\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Lockdown_Zones\\2\\1400"));
printf("%ws\n", TEXT("HKU\\S-1-5-18\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Lockdown_Zones\\2\\1400"));
printf("%s\n", found ? "true" : "false");

found = TrieSearch1(blacklistHKU->children, TEXT("HKU\\S-1-5-18\\Software\\Microsoft\\Windows\\SHIT\\Internet Settings\\Lockdown_Zones\\2\\1400"));
printf("%ws\n", TEXT("HKU\\S-1-5-18\\Software\\Microsoft\\Windows\\SHIT\\Internet Settings\\Lockdown_Zones\\2\\1400"));
printf("%s\n", found ? "true" : "false");

*/


//-----------------------------------------------------------------
// Create the Trie (Prefix Tree)
//-----------------------------------------------------------------
void TrieCreate(trieNode_t **root)
{
	*root = TrieCreateNode('\0');
}


//-----------------------------------------------------------------
// Create a Trie (Prefix Tree) Node
//-----------------------------------------------------------------
trieNode_t *TrieCreateNode(wchar_t key)
{
	trieNode_t *node = NULL;
	node = (trieNode_t *)MYALLOC(sizeof(trieNode_t));

	if (NULL == node) {
		printf("  > Failed to allocated sufficient memory...\n");
		return node;
	}

	node->key = key;
	node->next = NULL;
	node->children = NULL;
	node->parent = NULL;
	node->prev = NULL;
	return node;
}


//-----------------------------------------------------------------
// Add a Trie (Prefix Tree) Node
//-----------------------------------------------------------------
void TrieAdd(trieNode_t **root, wchar_t *key)
{
	trieNode_t *pTrav = NULL;

	if (NULL == *root) {
		//printf("NULL tree\n");
		return;
	}
	//printf("\nInserting key %ws: \n", key);
	pTrav = (*root)->children;

	if (pTrav == NULL)
	{
		// If the node is NULL, this must be the first Node
		for (pTrav = *root; *key; pTrav = pTrav->children) {
			pTrav->children = TrieCreateNode(*key);
			pTrav->children->parent = pTrav;
			//printf("Inserting: [%c]\n", pTrav->children->key);
			key++;
		}

		pTrav->children = TrieCreateNode('\0');
		pTrav->children->parent = pTrav;
		//printf("Inserting: [%c]\n", pTrav->children->key);
		return;
	}

	if (TrieSearch(pTrav, key))
	{
		//printf("Duplicate!\n");
		return;
	}

	while (*key != '\0')
	{
		if (*key == pTrav->key)
		{
			key++;
			//printf("Traversing child: [%c]\n", pTrav->children->key);
			pTrav = pTrav->children;
		}
		else
			break;
	}

	while (pTrav->next)
	{
		if (*key == pTrav->next->key)
		{
			key++;
			TrieAdd(&(pTrav->next), key);
			return;
		}
		pTrav = pTrav->next;
	}

	if (*key) {
		pTrav->next = TrieCreateNode(*key);
	}
	else {
		pTrav->next = TrieCreateNode(*key);
	}

	pTrav->next->parent = pTrav->parent;
	pTrav->next->prev = pTrav;

	//printf("Inserting [%c] as neighbour of [%c] \n", pTrav->next->key, pTrav->key);

	if (!(*key))
		return;

	key++;

	for (pTrav = pTrav->next; *key; pTrav = pTrav->children)
	{
		pTrav->children = TrieCreateNode(*key);
		pTrav->children->parent = pTrav;
		//printf("Inserting: [%c]\n", pTrav->children->key);
		key++;
	}

	pTrav->children = TrieCreateNode('\0');
	pTrav->children->parent = pTrav;
	//printf("Inserting: [%c]\n", pTrav->children->key);
	return;
}


//-----------------------------------------------------------------
// Check the Trie (Prefix Tree) for a dulplicate value
//-----------------------------------------------------------------
trieNode_t* TrieSearch(trieNode_t *root, const wchar_t *key)
{
	trieNode_t *level = root;
	trieNode_t *pPtr = NULL;
	int lvl = 0;
	while (1)
	{
		trieNode_t *found = NULL;
		trieNode_t *curr;

		for (curr = level; curr != NULL; curr = curr->next)
		{
			if (curr->key == *key)
			{
				found = curr;
				lvl++;
				break;
			}
		}

		if (found == NULL)
			return NULL;

		if (*key == '\0')
		{
			pPtr = curr;
			return pPtr;
		}

		level = found->children;
		key++;
	}
}


//-----------------------------------------------------------------
// Search the Trie (Prefix Tree) for a specific entry
//-----------------------------------------------------------------
BOOL TrieSearch1(trieNode_t *root, const wchar_t *key)
{
	trieNode_t *level = root;
	trieNode_t *pPtr = NULL;
	//printf("KEY: %ws\n", key);
	int lvl = 0;
	while (1)
	{
		trieNode_t *found = NULL;
		trieNode_t *curr;

		for (curr = level; curr != NULL; curr = curr->next)
		{
			//printf("CURRENT KEY: %c\n", curr->key);
			//printf("%d\n", lvl);
			if (curr->key == *key)
			{
				found = curr;
				lvl++;
				break;
			}
		}

		if (found == NULL)
			return FALSE;

		if (*key == '\0')
		{
			pPtr = curr;
			//printf("FOUND!\n");
			return TRUE;
		}

		level = found->children;
		key++;
	}
}


//-----------------------------------------------------------------
// Remove a Trie (Prefix Tree) Node
//-----------------------------------------------------------------
void TrieRemove(trieNode_t **root, wchar_t *key)
{
	trieNode_t *tPtr = NULL;
	trieNode_t *tmp = NULL;

	if (NULL == *root || NULL == key)
		return;

	tPtr = TrieSearch((*root)->children, key);

	if (NULL == tPtr)
	{
		//printf("Key [%ws] not found in trie\n", key);
		return;
	}

	//printf("Deleting key [%ws] from trie\n", key);

	while (1)
	{
		if (tPtr->prev && tPtr->next)
		{
			tmp = tPtr;
			tPtr->next->prev = tPtr->prev;
			tPtr->prev->next = tPtr->next;
			//printf("Deleted [%c] \n", tmp->key);
			free(tmp);
			break;
		}
		else if (tPtr->prev && !(tPtr->next))
		{
			tmp = tPtr;
			tPtr->prev->next = NULL;
			//printf("Deleted [%c] \n", tmp->key);
			free(tmp);
			break;
		}
		else if (!(tPtr->prev) && tPtr->next)
		{
			tmp = tPtr;
			tPtr->parent->children = tPtr->next;
			//printf("Deleted [%c] \n", tmp->key);
			free(tmp);
			break;
		}
		else
		{
			tmp = tPtr;
			tPtr = tPtr->parent;
			tPtr->children = NULL;
			//printf("Deleted [%c] \n", tmp->key);
			free(tmp);
		}
	}
	//printf("Deleted key [%ws] from trie\n", key);
}


//-----------------------------------------------------------------
// Destroy the Trie (Prefix Tree)
//-----------------------------------------------------------------
void TrieDestroy(trieNode_t* root)
{
	trieNode_t *tPtr = root;
	trieNode_t *tmp = root;

	while (tPtr)
	{
		while (tPtr->children)
			tPtr = tPtr->children;

		if (tPtr->prev && tPtr->next)
		{
			tmp = tPtr;
			tPtr->next->prev = tPtr->prev;
			tPtr->prev->next = tPtr->next;
			//printf("Deleted [%c] \n", tmp->key);
			free(tmp);
		}
		else if (tPtr->prev && !(tPtr->next))
		{
			tmp = tPtr;
			tPtr->prev->next = NULL;
			//printf("Deleted [%c] \n", tmp->key);
			free(tmp);
		}
		else if (!(tPtr->prev) && tPtr->next)
		{
			tmp = tPtr;
			tPtr->parent->children = tPtr->next;
			tPtr->next->prev = NULL;
			tPtr = tPtr->next;
			//printf("Deleted [%c] \n", tmp->key);
			free(tmp);
		}
		else
		{
			tmp = tPtr;
			if (tPtr->parent == NULL)
			{
				//Root
				free(tmp);
				return;
			}
			tPtr = tPtr->parent;
			tPtr->children = NULL;
			//printf("Deleted [%c] \n", tmp->key);
			free(tmp);
		}
	}

}