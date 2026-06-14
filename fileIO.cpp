#include "stdafx.h"


int	boardNumber;										// board number to load


void InitGameFileData(void)
{
	titleString[0] = headerString[0] = 0;				// strcpy does not add zero terminator so use strncat
	title2String[0] = header2String[0] = 0;	
	strncat(titleString, "Title String", TITLE_LEN);
	strncat(headerString, "Description String", HEADER_LEN);
	strncat(title2String, "Title String", TITLE_LEN);
	strncat(header2String, "Description String", HEADER_LEN);
	handCount = 34;
	analyzeHandCountIndex = 4;							// index for number of hands to play
	analyzeDepthIndex = 2;								// index for depth of analyze play
}


//	Windows does not permit the following characters in file names:
//	<, >, :, ", /, |, \, ?, *
//
void ValidateFileName(char * fname)
{
	while (*fname != 0)
	{
		if (*fname < ' ')
			*fname = ' ';
		else if (*fname > '~')
			*fname = ' ';
		else if (*fname == '<')
			*fname = ' ';
		else if (*fname == '>')
			*fname = ' ';
		else if (*fname == ':')
			*fname = ' ';
		else if (*fname == '"')
			*fname = ' ';
		else if (*fname == '/')
			*fname = ' ';
		else if (*fname == '|')
			*fname = ' ';
		else if (*fname == '\\')
			*fname = ' ';
		else if (*fname == '?')
			*fname = ' ';
		else if (*fname == '*')
			*fname = ' ';
		++fname;
	}
}


// Message handler for creating hand records
INT_PTR CALLBACK BoardNumber(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	HWND hwndOwner;
	RECT rcOwner, rcWindow;
	int width, widthOwner;
	int height, heightOwner;
	HWND msg;
	char data[256];

	switch (message)
	{
	case WM_INITDIALOG:
		if ((hwndOwner = GetParent(hDlg)) == NULL)
			hwndOwner = GetDesktopWindow();
		GetWindowRect(hwndOwner, &rcOwner);
		widthOwner = rcOwner.right - rcOwner.left;
		heightOwner = rcOwner.bottom - rcOwner.top;
		GetWindowRect(hDlg, &rcWindow);
		width = rcWindow.right - rcWindow.left;
		height = rcWindow.bottom - rcWindow.top;
		SetWindowPos(hDlg, HWND_TOP, rcOwner.left + (widthOwner - width) / 2, rcOwner.top + (heightOwner - height) / 2, 0, 0, SWP_NOSIZE);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			msg = GetDlgItem(hDlg, IDC_BOARDNUMBER);
			GetWindowText(msg, data, sizeof(data)-1);
			boardNumber = atoi(data);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL)
		{
			boardNumber = 0;
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


//
//	Load file in PBN format
//	It uses only the data from the following records
//		[Dealer "N"]
//		[Vulnerable "None"]
//		[Deal "N:985.QT86.Q86.Q84 AJ3.K....."]
//
bool LoadHand(HWND hWnd, char * fname)
{
	FILE * file;
	char command[256], data[256];
	int got = 0;
	int ret;
	bool skipBoard = false;

	boardNumber = 0;
	file = fopen(fname, "r");
	if (file != NULL)
	{
		while ((ret = fscanf(file, "[%s \"%s\"]\n", command, data)) != EOF)
		{
			if (ret == 0)										// skip until we get a line we can read 
				getc(file);
			else if (strcmp(command, "Board") == 0)
			{
				if (boardNumber <= 0)
				{
					DialogBox(hInst, MAKEINTRESOURCE(IDD_BOARDNUMBERBOX), hWnd, BoardNumber);
				}
				skipBoard = false;
				if (boardNumber > 0)							// board number specified
				{
					int num = atoi(data);
					if (num != boardNumber)
						skipBoard = true;
				}
			}
			else if (strcmp(command, "Dealer") == 0
			&& skipBoard == false)
			{
				if (restriction.dealer == DIR_SYSTEMIC)			// do not change dealer if dealer is locked
				{
					if (data[0] == 'N')
						SetDealer(0);
					else if (data[0] == 'E')
						SetDealer(1);
					else if (data[0] == 'S')
						SetDealer(2);
					else if (data[0] == 'W')
						SetDealer(3);
					else
					{
						fclose(file);
						return false;
					}
				}
			}
			else if (strcmp(command, "Vulnerable") == 0
			&& skipBoard == false)
			{
				if (restriction.vul == VUL_SYSTEMIC)			// do not change vul if vul is locked
				{
					if ((data[0] == 'N' && data[1] == 'o')		// None
					|| (data[0] == 'L' && data[1] == 'o')		// Love
					|| data[0] == '-')							// -
						SetVul(hWnd, 0, false);
					else if (data[0] == 'N' && data[1] == 'S')
						SetVul(hWnd, 1, false);
					else if (data[0] == 'E' && data[1] == 'W')
						SetVul(hWnd, 2, false);
					else if ((data[0] == 'B' && data[1] == 'o')	// Both
					|| (data[0] == 'A' && data[1] == 'l'))		// All
						SetVul(hWnd, 3, false);
					else
					{
						fclose(file);
						return false;
					}
				}
			}
			else if (strcmp(command, "Deal") == 0
			&& skipBoard == false)
			{
				int h, s, c;
				if (data[0] == 'N')
					h = 0;
				else if (data[0] == 'E')
					h = 1;
				else if (data[0] == 'S')
					h = 2;
				else if (data[0] == 'W')
					h = 3;
				else
				{
					fclose(file);
					return false;
				}
				int n = 2;
				for (got = 0; got < 4; got++)
				{
					if (got != 0)								// fetch new data
					{
						if (fscanf(file, "%s", data) != 1)	
							break;
					}
					s = 0;
					c = 0;
					hands[h].suits[s].count = 0;
					while (data[n] != 0)
					{
						if (data[n] == '\"')
							break;
						if (data[n] == '.')
						{
							hands[h].suits[s].count = c;
							c = 0;
							if (++s >= 4)
							{
								fclose(file);
								return false;
							}
						}
						else if ('2' <= data[n] && data[n] <= '9')
							hands[h].suits[s].spots[c++] = data[n] - '2';
						else if ('T' == data[n])
							hands[h].suits[s].spots[c++] = SPOT_TEN;
						else if ('J' == data[n])
							hands[h].suits[s].spots[c++] = SPOT_JACK;
						else if ('Q' == data[n])
							hands[h].suits[s].spots[c++] = SPOT_QUEEN;
						else if ('K' == data[n])
							hands[h].suits[s].spots[c++] = SPOT_KING;
						else if ('A' == data[n])
							hands[h].suits[s].spots[c++] = SPOT_ACE;
						else
						{
							fclose(file);
							return false;
						}
						n++;
					}
					hands[h].suits[s].count = c;			// last suit
					if (s >= 4)
					{
						fclose(file);
						return false;
					}
					if (++h == 4)							// wrap around for direction
						h = 0;
					n = 0;
				}
				goto DONE;
			}
		}
DONE:	fclose(file);
		if (got != 4)
			return false;
		return true;
	}
	return false;
}


//
//	Save to PBN format to allow programs like DoubleDummySolver to play the hands
//	We use only the data from the following records
//		[Dealer "N"]
//		[Vulnerable "None"]
//		[Deal "N:985.QT86.Q86.Q84 AJ3.K....."]
//
bool SaveHand(HWND hWnd, char * fname)
{
	FILE * file;
	HAND hands2[4];

	file = fopen(fname, "w");
	if (file != NULL)
	{
		fprintf(file, "[Dealer \"%c\"]\n", GetDealerPBNChar());
		fprintf(file, "[Vulnerable \"%s\"]\n", GetVulPBNString());
		fprintf(file, "[Deal \"%c:", GetDealerPBNChar());
		int hh = GetDealer();
		if (playHand)								// display hand being played and maybe not in hands array
		{
			for (int h = 0; h < 4; h++)
			{
				for (int s = 0; s < 4; s++)
				{
					hands2[h].suits[s].count = 0;
					int mask = 1 << 14;
					for (int c = 12; c >= 0; c--, mask >>= 1)
					{
						if ((dl.remainCards[h][s] & mask) != 0)
							hands2[h].suits[s].spots[hands2[h].suits[s].count++] = c;
						// check to see if card has already been played
						for (int i = 0; i < BackupPlayCount[h]; i++)
						{
							if (BackupPlaySuit[h][i] == s
							&&  BackupPlaySpot[h][i] == c)
								hands2[h].suits[s].spots[hands2[h].suits[s].count++] = c;
						}
					}
				}
			}
		}
		else
		{
			hands2[0] = hands[0];
			hands2[1] = hands[1];
			hands2[2] = hands[2];
			hands2[3] = hands[3];
		}
		for (int h = 0; h < 4; h++)
		{
			for (int s = 0; s < 4; s++)
			{
				for (int c = 0; c < hands2[hh].suits[s].count; c++)
				{
					fprintf(file, "%c", SpotString[hands2[hh].suits[s].spots[c]]);
				}
				if (s < 3)
					fprintf(file, ".");				// between suits
				else if (h < 3)
					fprintf(file, " ");				// between hands
			}
			if (++hh == DIRECTIONS)
				hh = 0;
		}
		fprintf(file, "\"]\n");
		fclose(file);
		return true;
	}
	return false;
}

bool LoadGameFile(HWND hWnd)
{
	PWSTR	ppszPath;
	char	path[256];
	int		version;

	// get path to the system app data
	SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &ppszPath);
	for (int i = 0; i < 256; i++)
	{
		path[i] = (char)ppszPath[i];
		if (path[i] == 0)
			break;
	}
	strncat(path, "\\BridgeDealer\\BridgeDealer.txt", 256);
	FILE * file;
	file = fopen(path, "r");
	if (file != NULL)
	{
		if (fscanf(file, "%d", &version) != 1
		|| version != SAVE_FILE_VERSION)			// version has changed 
			return false;
		fgetc(file);								// skip newline
		titleString[TITLE_LEN] = 0;					// be sure strings are zero terminated
		headerString[HEADER_LEN] = 0;
		title2String[TITLE_LEN] = 0;				// be sure strings are zero terminated
		header2String[HEADER_LEN] = 0;
		if (fgets(title2String, TITLE_LEN + 1, file) == NULL)	// add 1 for the newline character
			return false;
		if (fgets(header2String, HEADER_LEN + 1, file) == NULL)
			return false;
		if (fgets(titleString, TITLE_LEN + 1, file) == NULL)	// add 1 for the newline character
			return false;
		if (fgets(headerString, HEADER_LEN + 1, file) == NULL)
			return false;
		if (fscanf(file, "%d", &handCount) != 1)
			return false;
		if (fscanf(file, "%d", &analyzeHandCountIndex) != 1)
			return false;
		if (analyzeHandCountIndex >= IDM_ANALYZESAMPLES)
			return false;
		if (fscanf(file, "%d", &analyzeDepthIndex) != 1)
			return false;
		if (analyzeDepthIndex >= IDM_ANALYZEDEPTHS)
			return false;
		titleString[strlen(title2String) - 1] = 0;	// remove new line character
		headerString[strlen(headerString) - 1] = 0;
		title2String[strlen(title2String) - 1] = 0;	// remove new line character
		header2String[strlen(header2String) - 1] = 0;
		fclose(file);
		return true;
	}
	return false;
}


bool SaveGameFile(HWND hWnd)
{
	PWSTR	ppszPath;
	char	path[256];
	// get path to the system app data
	SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &ppszPath);
	for (int i = 0; i < 256; i++)
	{
		path[i] = (char)ppszPath[i];
		if (path[i] == 0)
			break;
	}
	// create directory if it does not exist
	strncat(path, "\\BridgeDealer", 256);
	CreateDirectory(path, NULL);
	FILE * file;
	strncat(path, "\\BridgeDealer.txt", 256);
	file = fopen(path, "w");
	if (file != NULL)
	{
		fprintf(file, "%d\n%s\n%s\n", SAVE_FILE_VERSION, title2String, header2String);
		fprintf(file, "%s\n%s\n%d\n%d\n%d\n", titleString, headerString, handCount, analyzeHandCountIndex, analyzeDepthIndex);
		fclose(file);
		return true;
	}
	return false;
}


bool LoadRestrictionFile(HWND hWnd, char * fname)
{
	FILE * file;
	int version;

	file = fopen(fname, "r");
	if (file != NULL)
	{
		fscanf(file, "%d,%d,%d,%d\n", &version, &restriction.mask, &restriction.dealer, &restriction.vul);
		for (int i = 0; i < MAX_RESTRICT; i++)
		{
			int i1, i2, i3, i4;
			fscanf(file, "%d,%d,%d,%d,%d\n",
				&restriction.restrict[i].dir,
				&i1, &i2, &i3, &i4);
			restriction.restrict[i].exactShape = (i1 != 0);
			restriction.restrict[i].HCP = (i2 != 0);
			restriction.restrict[i].not = (i3 != 0);
			restriction.restrict[i].concentrated = (i4 != 0);
			fscanf(file, "%d,%d,%d,%d,%d,%d,%d,%d\n",
				&restriction.restrict[i].minCards[0],
				&restriction.restrict[i].minCards[1],
				&restriction.restrict[i].minCards[2],
				&restriction.restrict[i].minCards[3],
				&restriction.restrict[i].maxCards[0],
				&restriction.restrict[i].maxCards[1],
				&restriction.restrict[i].maxCards[2],
				&restriction.restrict[i].maxCards[3]);
			fscanf(file, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
				&restriction.restrict[i].minPoints,
				&restriction.restrict[i].maxPoints,
				&restriction.restrict[i].minQTricks,
				&restriction.restrict[i].maxQTricks,
				&restriction.restrict[i].minControls,
				&restriction.restrict[i].maxControls,
				&restriction.restrict[i].minLosers,
				&restriction.restrict[i].maxLosers,
				&restriction.restrict[i].kRatio);
		}
		ClearLockedCards();
		if (RESTRICT_FILE_VERSION >= 2)
		{
			for (int i = 0; i < 4; i++)
			{
				fscanf(file, "%d,0x%x,0x%x,0x%x,0x%x\n",
					&restriction.lockedCount[i],
					&restriction.lockedCards[i][0],
					&restriction.lockedCards[i][1],
					&restriction.lockedCards[i][2],
					&restriction.lockedCards[i][3]);
			}
		}
		fclose(file);
		// check locked cards to be sure they are in all the respective hands
		for (int h = 0; h < 4; h++)
		{
			for (int s = 0; s < 4; s++)
			{
				int mask0 = 1 << SPOT_ACE;
				int mask = restriction.lockedCards[h][s];
				for (int c = SPOT_ACE; c >= SPOT_TWO && mask != 0; c--, mask0 >>= 1)
				{
					if ((mask & mask0) == 0)						// find a locked card
						continue;
					for (int i = 0; i < hands[h].suits[s].count; i++)
					{
						if (hands[h].suits[s].spots[i] == c)
							goto CONT;
					}
					ClearLockedCards();
					MessageBox(hWnd, "Locked cards do not match this deal!\nAll locked cards have been removed.", "User error", MB_ICONSTOP | MB_OK | MB_APPLMODAL);
					return true;

CONT:				mask &= ~mask0;									// remove card from locked list
				}
			}
		}
		return true;
	}
	return false;
}


bool SaveRestrictionFile(HWND hWnd, char * fname)
{
	FILE * file;

	file = fopen(fname, "w");
	if (file != NULL)
	{
		fprintf(file, "%d,%d,%d,%d\n", RESTRICT_FILE_VERSION, restriction.mask, restriction.dealer, restriction.vul);
		for (int i = 0; i < MAX_RESTRICT; i++)
		{
			fprintf(file, "%d,%d,%d,%d,%d\n",
				restriction.restrict[i].dir,
				restriction.restrict[i].exactShape,
				restriction.restrict[i].HCP,
				restriction.restrict[i].not,
				restriction.restrict[i].concentrated);
			fprintf(file, "%d,%d,%d,%d,%d,%d,%d,%d\n",
				restriction.restrict[i].minCards[0],
				restriction.restrict[i].minCards[1],
				restriction.restrict[i].minCards[2],
				restriction.restrict[i].minCards[3],
				restriction.restrict[i].maxCards[0],
				restriction.restrict[i].maxCards[1],
				restriction.restrict[i].maxCards[2],
				restriction.restrict[i].maxCards[3]);
			fprintf(file, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
				restriction.restrict[i].minPoints,
				restriction.restrict[i].maxPoints,
				restriction.restrict[i].minQTricks,
				restriction.restrict[i].maxQTricks,
				restriction.restrict[i].minControls,
				restriction.restrict[i].maxControls,
				restriction.restrict[i].minLosers,
				restriction.restrict[i].maxLosers,
				restriction.restrict[i].kRatio);
		}
		for (int i = 0; i < 4; i++)
		{
			fprintf(file, "%d,0x%04x,0x%04x,0x%04x,0x%04x\n",
				restriction.lockedCount[i],
				restriction.lockedCards[i][0],
				restriction.lockedCards[i][1],
				restriction.lockedCards[i][2],
				restriction.lockedCards[i][3]);
		}
		fclose(file);
		return true;
	}
	return false;
}


bool SaveFileName(HWND hwnd, char * fname, char * filterString, char * defaultExt)
{
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(ofn));
	ValidateFileName(fname);							// insure the file is valid

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = filterString;
	ofn.lpstrFile = fname;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrDefExt = defaultExt;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

	if (GetSaveFileName(&ofn))
		return true;
	return false;
}


bool LoadFileName(HWND hwnd, char * fname, char * filterString, char * defaultExt)
{
	OPENFILENAME ofn;
	fname[0] = 0;

	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFilter = filterString;
	ofn.lpstrFile = fname;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrDefExt = defaultExt;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

	if (GetOpenFileName(&ofn))
		return true;
	return false;
}
