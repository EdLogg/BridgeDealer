#include "framework.h"
#include "BridgeTimer.h"


// index values for this session
State state;
int roundsIndex;
int boardsIndex;													
int perBoardIndex;
int moveTimeIndex;
int noNewBoardsIndex;
int time;													// in seconds
int PreviousTime;											// previous start time in seconds
int round;													// 1, 2, ...
int noNewBoardsTime;										// time when to display no new boards
int endingTime;												// convert minutes to hh:mm


// values to be used for the variables above
int roundsTable[IDM_ROUNDSEND - IDM_ROUNDS] =
{ 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
int boardsTable[IDM_BOARDSEND - IDM_BOARDS] =
{ 1, 2, 3, 4, 5, 6 };
int timeTable[IDM_TIMEEND - IDM_TIME] =
{ 240, 270, 300, 330, 360, 390, 420, 450, 480, 510, 540, 570 };
int moveTimeTable[IDM_MOVETIMEEND - IDM_MOVETIME] =
{ 60, 90, 120, 150, 180, 210, 240, 270, 300 };
int noNewBoardTable[IDM_NONEWBOARDSEND - IDM_NONEWBOARDS] =
{ 90, 80, 70, 60, 50, 40 };									// percentage of board time


// gameTime is true if we are changing the time per board else the move time
int ChangeInTime(bool gameTime, int newIndex)
{
	if (gameTime)
		return (timeTable[newIndex] - timeTable[perBoardIndex]) * boardsTable[boardsIndex];
	else
		return moveTimeTable[newIndex] - moveTimeTable[moveTimeIndex];
}


void SetEndingTime()
{
	SYSTEMTIME data;
	GetLocalTime(&data);
	switch (state)
	{
	default:
	case STATE_STARTING:
		endingTime = (roundsTable[roundsIndex] + 1 - round) * timeTable[perBoardIndex] * boardsTable[boardsIndex];
		endingTime += (roundsTable[roundsIndex] - round) * moveTimeTable[moveTimeIndex];
		break;
	case STATE_RUNNING:
	case STATE_PAUSED_RUN:
		endingTime = time;							// add in current timer
		endingTime += (roundsTable[roundsIndex] - round) * timeTable[perBoardIndex] * boardsTable[boardsIndex];
		endingTime += (roundsTable[roundsIndex] - round) * moveTimeTable[moveTimeIndex];
		break;
	case STATE_MOVING:
	case STATE_PAUSED_MOVING:
		endingTime = time;							// add in current timer
		endingTime += (roundsTable[roundsIndex] - round) * timeTable[perBoardIndex] * boardsTable[boardsIndex];
		endingTime += (roundsTable[roundsIndex] - round) * moveTimeTable[moveTimeIndex];
		break;
	case STATE_DONE:
		return;										// do not change time when we are donepause timer

	}
	endingTime = (endingTime + 30) / 60;	  		// now in minutes
	endingTime += data.wHour * 60;
	endingTime += data.wMinute;
	while (endingTime >= 24 * 60)					// if it wraps the day
		endingTime -= 24 * 60;
	if (endingTime >= 13 * 60)						// use 12 hour mode
		endingTime -= 12 * 60;
}


void SetLastBoardTime()
{
	noNewBoardsTime = timeTable[perBoardIndex] * noNewBoardTable[noNewBoardsIndex] / 100;
}


void SetTimer(bool resetTime)
{
	if (state == STATE_MOVING
	||  state == STATE_PAUSED_MOVING)
		time = moveTimeTable[moveTimeIndex];
	else if (resetTime)
		time = timeTable[perBoardIndex] * boardsTable[boardsIndex];
	SetLastBoardTime();
	SetEndingTime();
}


void InitGameData()
{
	roundsIndex = 3;
	boardsIndex = 2;
	perBoardIndex = 2;
	moveTimeIndex = 1;
	noNewBoardsIndex = 2;
}


void StartNewSession(HWND hWnd)
{
	state = STATE_STARTING;
	round = 1;
	SetTimer(hWnd, IDT_GAMETIMER, 60000, (TIMERPROC)NULL);	// so that ending time gets updated
	SetTimer(true);
}


void ConvertTime(int time, int& min, int& sec)
{
	sec = time % 60;
	min = time / 60;
}


// enable or disable Rounds and Boards menu 
void UpdateRoundBoardMenus(HMENU hMenu, bool enabled)
{
	for (int i = 0; i < IDM_ROUNDSEND - IDM_ROUNDS; i++)
	{
		if (enabled)
			EnableMenuItem(hMenu, IDM_ROUNDS + i, MF_BYCOMMAND | MF_ENABLED);
		else
			EnableMenuItem(hMenu, IDM_ROUNDS + i, MF_BYCOMMAND | MF_GRAYED);
	}
	for (int i = 0; i < IDM_BOARDSEND - IDM_BOARDS; i++)
	{
		if (enabled)
			EnableMenuItem(hMenu, IDM_BOARDS + i, MF_BYCOMMAND | MF_ENABLED);
		else
			EnableMenuItem(hMenu, IDM_BOARDS + i, MF_BYCOMMAND | MF_GRAYED);
	}
}


// return number of round left (i.e. 0 is on last round)
int RoundsRemaining()
{
	return roundsTable[roundsIndex] - round;
}


void EnterState(HMENU hMenu, State newState)
{
	State oldState = state;
	state = newState;
	switch (state)
	{
	default:
	case STATE_STARTING:
		SetTimer(true);
		UpdateRoundBoardMenus(hMenu, true);				// turn on menus for rounds and boards per round
		break;
	case STATE_RUNNING:
		if (oldState != STATE_PAUSED_RUN)
			SetTimer(true);
		if (oldState == STATE_STARTING)
			UpdateRoundBoardMenus(hMenu, false);		// turn off menus for rounds and boards per round
		break;
	case STATE_PAUSED_RUN:
		break;
	case STATE_MOVING:
	case STATE_PAUSED_MOVING:
		if (oldState == STATE_RUNNING || oldState == STATE_PAUSED_RUN)
			SetTimer(true);
		break;
	case STATE_DONE:
		break;
	}
}


BOOL LoadDefaultsFile(LPCTSTR pszFileName)
{
	FILE* file;

	file = fopen(pszFileName, "r");
	if (file == NULL)										// open failed so we have no saved defaults
		return false;

	int		version;
	if (fscanf(file, "%d", &version) != 1
	|| version != SAVE_FILE_VERSION)						// version has changed 
		return false;
	int c = fscanf(file, "%d,%d,%d,%d,%d",
		&roundsIndex, &boardsIndex, &perBoardIndex, &moveTimeIndex, &noNewBoardsIndex);
	if (c != 5)
	{
		fclose(file);
		return false;
	}
	fclose(file);
	return true;
}


void LoadGameData()
{
	PWSTR	ppszPath;
	char	path[MAX_PATH];

	// get path to the system app data
	SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &ppszPath);
	for (int i = 0; i < 256; i++)
	{
		path[i] = (char)ppszPath[i];
		if (path[i] == 0)
			break;
	}
	// this should be C:\Users\<user>\AppData\Roaming\Risk\...
	strncat(path, "\\BridgeTimer\\BridgeTimer.txt", MAX_PATH - 1);
	if (LoadDefaultsFile(path) == false)
		InitGameData();
}


BOOL SaveDefaultsFile(LPCTSTR pszFileName)
{
	FILE* file;
	file = fopen(pszFileName, "w");
	if (file != NULL)
	{
		fprintf(file, "%d\n", SAVE_FILE_VERSION);
		fprintf(file, "%d,%d,%d,%d,%d\n",
			roundsIndex, boardsIndex, perBoardIndex, moveTimeIndex, noNewBoardsIndex);
		fclose(file);
		return true;
	}
	return false;
}


bool SaveGameData()
{
	PWSTR	ppszPath;
	char	path[MAX_PATH];
	// get path to the system app data
	SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &ppszPath);
	for (int i = 0; i < 256; i++)
	{
		path[i] = (char)ppszPath[i];
		if (path[i] == 0)
			break;
	}
	// create directory if it does not exist
	strncat(path, "\\BridgeTimer", MAX_PATH - 1);
	CreateDirectory(path, NULL);
	strncat(path, "\\BridgeTimer.txt", MAX_PATH - 1);
	return SaveDefaultsFile(path);
}


void DrawScreen(HWND hWnd)
{
	PAINTSTRUCT ps;
	char string[32];
	HFONT hFont1, hFont2;
	HFONT hOldFont;
	int min, sec;

	HDC hdc = BeginPaint(hWnd, &ps);

	//--------------------- Write header ---------------------------------
	// Create font
	hFont1 = CreateFont(80, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		VARIABLE_PITCH, "Arial");
	hFont2 = CreateFont(160, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		VARIABLE_PITCH, "Arial");
	SetTextAlign(hdc, TA_CENTER);				// align text to center
	switch (state)
	{
	case STATE_STARTING:
		hOldFont = (HFONT)SelectObject(hdc, hFont1);
		SetTextColor(hdc, RGB(0, 0, 0));
		TextOut(hdc, WINDOW_WIDTH/2, 10, "Press Start Session", sizeof("Press Start Session"));
		TextOut(hdc, WINDOW_WIDTH/2, 90, "to begin", sizeof("to begin"));
		break;
	case STATE_RUNNING:
	case STATE_PAUSED_RUN:
		if (time > noNewBoardsTime)
		{
			hOldFont = (HFONT)SelectObject(hdc, hFont2);
			SetTextColor(hdc, RGB(0, 0, 0));
			TextOut(hdc, WINDOW_WIDTH/2, 10, "Play", sizeof("Play"));
		}
		else
		{
			hOldFont = (HFONT)SelectObject(hdc, hFont1);
			SetTextColor(hdc, RGB(255, 0, 0));
			TextOut(hdc, WINDOW_WIDTH/2, 10, "Do not start any", sizeof("Do not start any"));
			TextOut(hdc, WINDOW_WIDTH/2, 90, "new boards", sizeof("new boards"));
		}
		break;
	case STATE_MOVING:
	case STATE_PAUSED_MOVING:
		hOldFont = (HFONT)SelectObject(hdc, hFont2);
		SetTextColor(hdc, RGB(0, 255, 0));
		TextOut(hdc, WINDOW_WIDTH/2, 10, "Move", sizeof("Move"));
		SelectObject(hdc, hFont1);
		TextOut(hdc, WINDOW_WIDTH/2, 210, "for the next round", sizeof("for the next round"));
		SetTextColor(hdc, RGB(0, 0, 0));		// restore color
		break;
	default:
	case STATE_DONE:
		hOldFont = (HFONT)SelectObject(hdc, hFont1);
		SetTextColor(hdc, RGB(0, 0, 0));
		TextOut(hdc, WINDOW_WIDTH/2, 10, "Game Over", sizeof("Game Over"));
		break;
	}
	SelectObject(hdc, hOldFont);				// restore old font
	DeleteObject(hFont1);
	DeleteObject(hFont2);

	//--------------------- Write Time ---------------------------------
	// Create font
	hFont1 = CreateFont(300, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		VARIABLE_PITCH, "Arial");
	// Select font into DC
	hOldFont = (HFONT)SelectObject(hdc, hFont1);
	ConvertTime(time, min, sec);
	snprintf(string, sizeof(string), "%2d:%02d", min, sec);
	switch (state)
	{
	default:
	case STATE_DONE:
	case STATE_STARTING:
		TextOut(hdc, WINDOW_WIDTH / 2, 180, (LPCSTR)string, 5);
		break;
	case STATE_RUNNING:
	case STATE_PAUSED_RUN:
		if (time > noNewBoardsIndex)
		{
			TextOut(hdc, WINDOW_WIDTH/2, 180, (LPCSTR)string, 5);
		}
		else
		{
			SetTextColor(hdc, RGB(255, 0, 0));
			TextOut(hdc, WINDOW_WIDTH/2, 180, (LPCSTR)string, 5);
			SetTextColor(hdc, RGB(0, 0, 0));	// restore color
		}
		break;
	case STATE_MOVING:
	case STATE_PAUSED_MOVING:
		SetTextColor(hdc, RGB(0, 255, 0));
		TextOut(hdc, WINDOW_WIDTH/2, 180, (LPCSTR)string, 5);
		SetTextColor(hdc, RGB(0, 0, 0));		// restore color
		break;
	}
	SelectObject(hdc, hOldFont);				// restore old font
	DeleteObject(hFont1);

	//--------------------- Write last board time ---------------------------------
	switch (state)
	{
	case STATE_STARTING:
	case STATE_RUNNING:
	case STATE_PAUSED_RUN:
		hFont1 = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
			CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
			VARIABLE_PITCH, "Arial");
		hOldFont = (HFONT)SelectObject(hdc, hFont1);
		ConvertTime(noNewBoardsTime, min, sec);
		snprintf(string, sizeof(string), "Start last board before %2d:%02d", min, sec);
		SetTextColor(hdc, RGB(255, 0, 0));
		TextOut(hdc, WINDOW_WIDTH/2, VISIBLE_HEIGHT - 200, (LPCSTR)string, (int)strlen(string));
		SetTextColor(hdc, RGB(0, 0, 0));		// restore color
		SelectObject(hdc, hOldFont);			// restore old font
		DeleteObject(hFont1);
		break;
	case STATE_MOVING:
	case STATE_PAUSED_MOVING:
	default:
	case STATE_DONE:
		break;
	}
	SetTextAlign(hdc, TA_LEFT);					// align text to left

	//--------------------- Write round and expected end time ---------------------------------
	hFont1 = CreateFont(40, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		VARIABLE_PITCH, "Arial");
	hOldFont = (HFONT)SelectObject(hdc, hFont1);
	switch (state)
	{
	default:
	case STATE_STARTING:
	case STATE_RUNNING:
	case STATE_PAUSED_RUN:
	case STATE_MOVING:
	case STATE_PAUSED_MOVING:
	case STATE_DONE:
		snprintf(string, sizeof(string), "Round %2d", round);
		TextOut(hdc, 50, VISIBLE_HEIGHT - 160, (LPCSTR)string, (int)strlen(string));
		ConvertTime(endingTime, min, sec);	
		snprintf(string, sizeof(string), "Expected End Time %2d:%02d", min, sec);
		TextOut(hdc, 50, VISIBLE_HEIGHT - 120, (LPCSTR)string, (int)strlen(string));
		break;
		break;
	}
	SelectObject(hdc, hOldFont);				// restore old font
	DeleteObject(hFont1);

	// Cleanup
	EndPaint(hWnd, &ps);
}
