// BridgeDealer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"


#define MAX_LOADSTRING 100


// Global Variables:
int			coreCount;
int			processorCount;
int			L1CacheCount, L2CacheCount, L3CacheCount;
INITCOMMONCONTROLSEX icex;						// comctl32.lib connection to progress bar class
HWND		hProgressWindow = NULL;				// progress bar window
HWND		hProgressBar = NULL;				// progress bar 
HINSTANCE	hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
HBITMAP		hCards = NULL;
HBITMAP		hCardsOff = NULL;
HBITMAP		hCardMark = NULL;
HBITMAP		hCardPlus = NULL;
HBITMAP		hCardMinus = NULL;
HBITMAP		hCardLock = NULL;
bool		startSet;
bool		writeHands;							// write hands when we are done
bool		savingHands;						// we are saving these hands		
SELECT		selected;							// card selected to be moved
RESTRICTION restriction;						// list of restrictions
HWND		hDoneButton = NULL;					// done button for changing cards and playing hands
HWND		hBackButton = NULL;					// back button for playing hand
HWND		hRestartButton = NULL;				// restart button for playing hand
bool		hideHands[4] = { false, false, false, false };
bool		hideHandCounts = false;
bool		hideContracts = false;
bool		playHand = false;
bool		lockCards = false;
int			playContract, playSuit, playDeclarer;
int			NSWon, EWWon;
bool		addComments = false;				// use comments instead of par results for each hand
U32			seed;
WNDPROC		oldEditWindowProc;					// old edit control procedure
int			results[MAX_HAND_COUNT][CONTRACTS][DIRECTIONS];		// used to redeal some hands to get average results
int			averageHandCount;					// number of hands used so far
bool		moreResults;						// used to get more results
bool		showDeviations;						// show deviations for stats

static	LARGE_INTEGER	ticksPerSecond;
static	LARGE_INTEGER	tickStart;				// start time
static	LARGE_INTEGER	tickEnd;				// end time


// get the high resolution counter's accuracy
void	TimeInit()
{
	QueryPerformanceFrequency(&ticksPerSecond);
}

void	TimeStart()
{
	QueryPerformanceCounter(&tickStart);		// start timer
}

double	TimeEnd()
{
	QueryPerformanceCounter(&tickEnd);			// end timer
	return (double)(tickEnd.QuadPart - tickStart.QuadPart) / (double)ticksPerSecond.QuadPart;
}


void	ProgressBarStart()
{
	RECT rcClient;								// Client area of parent window.

	if (hProgressWindow == NULL)				// if window is not created ignore command
		return;
	// create bar and position it
	GetClientRect(hProgressWindow, &rcClient);
	hProgressBar = CreateWindowEx(0, PROGRESS_CLASS, 0,
		WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
		rcClient.left, (rcClient.bottom - rcClient.bottom) / 2, 250, 20,
		hProgressWindow, (HMENU)0, GetModuleHandle(NULL), NULL);
	// Set the range of the progress bar. 
	SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	ShowWindow(hProgressWindow, SW_SHOW);
	UpdateWindow(hProgressWindow);
}

void	ProgressBarSet(int percentage)
{
	SendMessage(hProgressBar, PBM_SETPOS, percentage, 0);
}

void	ProgressBarStop()
{
	if (hProgressWindow == NULL)				// if window is not created ignore command
		return;
	DestroyWindow(hProgressBar);
	ShowWindow(hProgressWindow, SW_HIDE);
}


void ClearLockedCards()
{
	restriction.lockedCount[0] = restriction.lockedCount[1] = restriction.lockedCount[2] = restriction.lockedCount[3] = 0;
	restriction.lockedCards[0][0] = restriction.lockedCards[0][1] = restriction.lockedCards[0][2] = restriction.lockedCards[0][3] = 0;
	restriction.lockedCards[1][0] = restriction.lockedCards[1][1] = restriction.lockedCards[1][2] = restriction.lockedCards[1][3] = 0;
	restriction.lockedCards[2][0] = restriction.lockedCards[2][1] = restriction.lockedCards[2][2] = restriction.lockedCards[2][3] = 0;
	restriction.lockedCards[3][0] = restriction.lockedCards[3][1] = restriction.lockedCards[3][2] = restriction.lockedCards[3][3] = 0;
}


void ClearRestrictions()
{
	restriction.mask = 0;							// no restrictions
	restriction.dealer = DIR_SYSTEMIC;
	restriction.vul = VUL_SYSTEMIC;
	ClearLockedCards();								// clear locked cards too
}


void EnableRestrictionMenus(HMENU hMenu)
{
	if (restriction.lockedCount[0] == 0
	&& restriction.lockedCount[1] == 0
	&& restriction.lockedCount[2] == 0
	&& restriction.lockedCount[3] == 0)
	{
		EnableMenuItem(hMenu, IDM_CLEARLOCK, MF_BYCOMMAND | MF_GRAYED);
		if (restriction.mask == 0)
			EnableMenuItem(hMenu, IDM_SAVERESTRICTIONS, MF_BYCOMMAND | MF_GRAYED);
		else
			EnableMenuItem(hMenu, IDM_SAVERESTRICTIONS, MF_BYCOMMAND | MF_ENABLED);
	}
	else
	{
		EnableMenuItem(hMenu, IDM_SAVERESTRICTIONS, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_CLEARLOCK, MF_BYCOMMAND | MF_ENABLED);
	}
}


void DoneButtonEnable(HWND hWnd)
{
	if (hDoneButton != NULL)
		return;
	hDoneButton = CreateWindow(
		"BUTTON",	// Predefined class
		"Done",		// Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
		10,         // x position 
		70,         // y position 
		60,			// Button width
		30,			// Button height
		hWnd,		// Parent window
		(HMENU)IDC_DONEBUTTON,     // button ID
		(HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
		NULL);      // Pointer not needed.
	if (hDoneButton == NULL)
	{
		MessageBox(hWnd, "Cannot create Done button!", "Internal error", MB_ICONSTOP | MB_OK | MB_APPLMODAL);
		return;
	}
	HMENU hMenu = GetMenu(hWnd);							// turn off some menu items
	EnableMenuItem(hMenu, IDM_CREATERECORDS, MF_BYCOMMAND | MF_GRAYED);
	if (playHand == false)									// allow save during play hand or lock cards
		EnableMenuItem(hMenu, IDM_SAVEHAND, MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(hMenu, IDM_SAVESTART, MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(hMenu, IDM_SAVEEND, MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(hMenu, IDM_SAVECANCEL, MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(hMenu, IDM_DEAL, MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(hMenu, IDM_CREATESTATSNS, MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(hMenu, IDM_CREATESTATSS, MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(hMenu, IDM_CREATESTATSN, MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(hMenu, IDM_REDEALNEW, MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(hMenu, IDM_REDEALNE, MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(hMenu, IDM_REDEALNW, MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(hMenu, IDM_REDEALEW, MF_BYCOMMAND | MF_GRAYED);
	if (playHand)
	{
		hBackButton = CreateWindow(
			"BUTTON",	// Predefined class
			"Back",		// Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
			10,         // x position 
			105,         // y position 
			60,			// Button width
			30,			// Button height
			hWnd,		// Parent window
			(HMENU)IDC_BACKBUTTON,     // button ID
			(HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
			NULL);      // Pointer not needed.
		if (hBackButton == NULL)
		{
			MessageBox(hWnd, "Cannot create Back button!", "Internal error", MB_ICONSTOP | MB_OK | MB_APPLMODAL);
			return;
		}
		hRestartButton = CreateWindow(
			"BUTTON",	// Predefined class
			"Restart",		// Button text 
			WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
			10,         // x position 
			140,         // y position 
			60,			// Button width
			30,			// Button height
			hWnd,		// Parent window
			(HMENU)IDC_RESTARTBUTTON,     // button ID
			(HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
			NULL);      // Pointer not needed.
		if (hRestartButton == NULL)
		{
			MessageBox(hWnd, "Cannot create Restart button!", "Internal error", MB_ICONSTOP | MB_OK | MB_APPLMODAL);
			return;
		}
		SaveEWHandInfo();						// save EW hands in case we need to analyze hand which reshuffles the EW cards
		NSWon = 0;
		EWWon = 0;
		EnableMenuItem(hMenu, IDM_LOADHAND, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_REDEAL, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_CHANGEHANDS, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SWAPNS, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SWAPNE, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SWAPNW, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SWAPSE, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SWAPSW, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SWAPEW, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_ROTATE90, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_ROTATE180, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_ROTATE270, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SWAPSH, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SWAPSD, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SWAPSC, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SWAPHD, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SWAPHC, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SWAPDC, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_DEALERN, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_DEALERE, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_DEALERS, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_DEALERW, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_DEALERDEFAULT, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_VULNONE, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_VULNS, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_VULEW, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_VULBOTH, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_VULDEFAULT, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_LOCKN, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_LOCKE, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_LOCKS, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_LOCKW, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_UNLOCK, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_LOCKVULNONE, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_LOCKVULNS, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_LOCKVULEW, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_LOCKVULBOTH, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_UNLOCKVUL, MF_BYCOMMAND | MF_GRAYED);
		//		Allow restrictions in case they will analzye play later
	}
	EnableMenuItem(hMenu, IDM_LOCKCARDS, MF_BYCOMMAND | MF_GRAYED);
}



void DoneButtonDisable(HWND hWnd)
{
	if (hDoneButton == NULL)
		return;
	DestroyWindow(hDoneButton);
	hDoneButton = NULL;
	HMENU hMenu = GetMenu(hWnd);							// turn on some menu items
	EnableMenuItem(hMenu, IDM_CREATERECORDS, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(hMenu, IDM_SAVEHAND, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(hMenu, IDM_SAVESTART, MF_BYCOMMAND | MF_ENABLED);
	if (savingHands)
	{
		EnableMenuItem(hMenu, IDM_SAVEEND, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_SAVECANCEL, MF_BYCOMMAND | MF_ENABLED);
	}
	EnableMenuItem(hMenu, IDM_DEAL, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(hMenu, IDM_CREATESTATSNS, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(hMenu, IDM_CREATESTATSS, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(hMenu, IDM_CREATESTATSN, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(hMenu, IDM_REDEALNEW, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(hMenu, IDM_REDEALNE, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(hMenu, IDM_REDEALNW, MF_BYCOMMAND | MF_ENABLED);
	EnableMenuItem(hMenu, IDM_REDEALEW, MF_BYCOMMAND | MF_ENABLED);
	if (playHand)
	{
		RestoreEWHandInfo();						// restore EW hands in case we need to analyze hand which reshuffles the EW cards
		DestroyWindow(hBackButton);
		DestroyWindow(hRestartButton);
		EnableMenuItem(hMenu, IDM_LOADHAND, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_REDEAL, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_CHANGEHANDS, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_SWAPNS, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_SWAPNE, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_SWAPNW, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_SWAPSE, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_SWAPSW, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_SWAPEW, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_ROTATE90, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_ROTATE180, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_ROTATE270, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_SWAPSH, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_SWAPSD, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_SWAPSC, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_SWAPHD, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_SWAPHC, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_SWAPDC, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_DEALERN, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_DEALERE, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_DEALERS, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_DEALERW, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_DEALERDEFAULT, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_VULNONE, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_VULNS, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_VULEW, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_VULBOTH, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_VULDEFAULT, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_LOCKN, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_LOCKE, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_LOCKS, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_LOCKW, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_UNLOCK, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_LOCKVULNONE, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_LOCKVULNS, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_LOCKVULEW, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_LOCKVULBOTH, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_UNLOCKVUL, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_RESTRICTIONSOUTH, MF_BYCOMMAND | MF_ENABLED);
		if (restriction.mask != 0)
		{
			EnableMenuItem(hMenu, IDM_RESTRICTIONWEST, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_RESTRICTIONNORTH, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_RESTRICTIONEAST, MF_BYCOMMAND | MF_ENABLED);
		}
	}
	EnableMenuItem(hMenu, IDM_LOCKCARDS, MF_BYCOMMAND | MF_ENABLED);
	playHand = false;
	lockCards = false;
	EnableMenuItem(hMenu, IDM_ANALYZEPLAY, MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(hMenu, IDM_SAVESOLUTION, MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(hMenu, IDM_ANALYZEPICKWIN, MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(hMenu, IDM_ANALYZEPICKLOSS, MF_BYCOMMAND | MF_GRAYED);
}


int SD_GetScrollPos(HWND hwnd, int bar, UINT code)
{
	SCROLLINFO si;

	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
	GetScrollInfo(hwnd, bar, &si);
	int minPos = si.nMin;
	int maxPos = si.nMax - (si.nPage - 1);
	int result = -1;
	switch (code)
	{
	case SB_LINEUP /*SB_LINELEFT*/:
		result = max(si.nPos - 1, minPos);
		break;
	case SB_LINEDOWN /*SB_LINERIGHT*/:
		result = min(si.nPos + 1, maxPos);
		break;
	case SB_PAGEUP /*SB_PAGELEFT*/:
		result = max(si.nPos - (int)si.nPage, minPos);
		break;
	case SB_PAGEDOWN /*SB_PAGERIGHT*/:
		result = min(si.nPos + (int)si.nPage, maxPos);
		break;
	case SB_THUMBPOSITION:
		// do nothing
		break;
	case SB_THUMBTRACK:
		result = si.nTrackPos;
		break;
	case SB_TOP /*SB_LEFT*/:
		result = minPos;
		break;
	case SB_BOTTOM /*SB_RIGHT*/:
		result = maxPos;
		break;
	case SB_ENDSCROLL:
		// do nothing
		break;
	}
	return result;
}


// Message handler for help box
INT_PTR CALLBACK Help(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	HWND hwndOwner;
	RECT rcOwner, rcWindow;
	int width, widthOwner, height;
	SCROLLINFO	si;
	int scrollPos;
	static int scrollPrevY = 1;

	switch (message)
	{
	case WM_INITDIALOG:
		if ((hwndOwner = GetParent(hDlg)) == NULL)
			hwndOwner = GetDesktopWindow();
		GetWindowRect(hwndOwner, &rcOwner);
		widthOwner = rcOwner.right - rcOwner.left;
		GetWindowRect(hDlg, &rcWindow);
		width = rcWindow.right - rcWindow.left;
		height = rcWindow.bottom - rcWindow.top;
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
		si.nMin = 1;
		si.nMax = height;
		si.nPage = 40;
		si.nPos = 1;
		SetScrollInfo(hDlg, SB_VERT, &si, FALSE);
		// resize window to fit the main window
		SetWindowPos(hDlg, HWND_TOP, rcOwner.left + (widthOwner - width) / 2, rcOwner.top + 40, 
			rcOwner.right - rcOwner.left, rcOwner.bottom - rcOwner.top - 40 - 3, 0);
		return (INT_PTR)TRUE;

	case WM_SIZE:
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_PAGE;
		si.nPage = LOWORD(lParam);								// new height
		SetScrollInfo(hDlg, SB_VERT, &si, TRUE);				// will adjust the scroll bar icon height
		return (INT_PTR)TRUE;

	case WM_VSCROLL:
		scrollPos = SD_GetScrollPos(hDlg, SB_VERT, LOWORD(wParam));
		if (scrollPos != -1)
		{
			SetScrollPos(hDlg, SB_VERT, scrollPos, TRUE);		// set position of scroll bar but not the window
			int delta = LOWORD(wParam);
			delta = scrollPrevY - scrollPos;
			scrollPrevY = scrollPos;
			ScrollWindow(hDlg, 0, delta, NULL, NULL);
		}
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


// Message handler for help box
INT_PTR CALLBACK Help2(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	HWND hwndOwner;
	RECT rcOwner, rcWindow;
	int width, widthOwner;

	switch (message)
	{
	case WM_INITDIALOG:
		if ((hwndOwner = GetParent(hDlg)) == NULL)
			hwndOwner = GetDesktopWindow();
		GetWindowRect(hwndOwner, &rcOwner);
		widthOwner = rcOwner.right - rcOwner.left;
		GetWindowRect(hDlg, &rcWindow);
		width = rcWindow.right - rcWindow.left;
		SetWindowPos(hDlg, HWND_TOP, rcOwner.left + (widthOwner - width) / 2, rcOwner.top + 40, 0, 0, SWP_NOSIZE);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


// Message handler for about box
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	HWND hwndOwner;
	RECT rcOwner, rcWindow;
	int width, widthOwner;

	switch (message)
	{
	case WM_INITDIALOG:
		if ((hwndOwner = GetParent(hDlg)) == NULL)
			hwndOwner = GetDesktopWindow();
		GetWindowRect(hwndOwner, &rcOwner);
		widthOwner = rcOwner.right - rcOwner.left;
		GetWindowRect(hDlg, &rcWindow);
		width = rcWindow.right - rcWindow.left;
		SetWindowPos(hDlg, HWND_TOP, rcOwner.left + (widthOwner - width) / 2, rcOwner.top + 40, 0, 0, SWP_NOSIZE);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


// Message handler for creating hand records
INT_PTR CALLBACK CreateHandRecords(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	HWND hwndOwner;
	RECT rcOwner, rcWindow;
	int width, widthOwner;
	HWND msg;
	char string[16];
	struct tm *	date;
	time_t		timer;

	switch (message)
	{
	case WM_INITDIALOG:
		if ((hwndOwner = GetParent(hDlg)) == NULL)
			hwndOwner = GetDesktopWindow();
		GetWindowRect(hwndOwner, &rcOwner);
		widthOwner = rcOwner.right - rcOwner.left;
		GetWindowRect(hDlg, &rcWindow);
		width = rcWindow.right - rcWindow.left;
		SetWindowPos(hDlg, HWND_TOP, rcOwner.left + (widthOwner - width) / 2, rcOwner.top + 40, 0, 0, SWP_NOSIZE);
		msg = GetDlgItem(hDlg, IDC_TITLETEXT);
		SetWindowText(msg, titleString);
		msg = GetDlgItem(hDlg, IDC_HEADERTEXT);
		SetWindowText(msg, headerString);
		msg = GetDlgItem(hDlg, IDC_DATETEXT);
		timer = time(NULL);
		date = localtime(&timer);
		strftime(dateString, sizeof(dateString), "%B %d, %Y", date);
		SetWindowText(msg, dateString);
		msg = GetDlgItem(hDlg, IDC_HANDCOUNT);
		sprintf(string, "%d", handCount);
		SetWindowText(msg, string);
		CheckDlgButton(hDlg, IDC_SAVEHANDS, BST_CHECKED);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			msg = GetDlgItem(hDlg, IDC_TITLETEXT);
			GetWindowText(msg, titleString, TITLE_LEN);
			msg = GetDlgItem(hDlg, IDC_HEADERTEXT);
			GetWindowText(msg, headerString, HEADER_LEN);
			msg = GetDlgItem(hDlg, IDC_DATETEXT);
			GetWindowText(msg, dateString, HEADER_LEN);
			msg = GetDlgItem(hDlg, IDC_HANDCOUNT);
			GetWindowText(msg, string, sizeof(string));
			handCount = atoi(string);
			if (handCount < 1)
				handCount = 1;
			else if (handCount > MAX_HAND_RECORD_COUNT)
				handCount = MAX_HAND_RECORD_COUNT;
			startSet = true;
			writeHands = IsDlgButtonChecked(hDlg, IDC_SAVEHANDS) == BST_CHECKED;
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL)
		{
			startSet = false;
			writeHands = false;
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


// Message handler for creating hand records
INT_PTR CALLBACK SaveHands(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	HWND hwndOwner;
	RECT rcOwner, rcWindow;
	int width, widthOwner;
	HWND msg;
	struct tm *	date;
	time_t		timer;

	switch (message)
	{
	case WM_INITDIALOG:
		if ((hwndOwner = GetParent(hDlg)) == NULL)
			hwndOwner = GetDesktopWindow();
		GetWindowRect(hwndOwner, &rcOwner);
		widthOwner = rcOwner.right - rcOwner.left;
		GetWindowRect(hDlg, &rcWindow);
		width = rcWindow.right - rcWindow.left;
		SetWindowPos(hDlg, HWND_TOP, rcOwner.left + (widthOwner - width) / 2, rcOwner.top + 40, 0, 0, SWP_NOSIZE);
		msg = GetDlgItem(hDlg, IDC_TITLETEXT);
		SetWindowText(msg, title2String);
		msg = GetDlgItem(hDlg, IDC_HEADERTEXT);
		SetWindowText(msg, header2String);
		timer = time(NULL);
		date = localtime(&timer);
		msg = GetDlgItem(hDlg, IDC_DATETEXT);
		strftime(date2String, sizeof(date2String), "%B %d, %Y", date);
		SetWindowText(msg, date2String);
		CheckDlgButton(hDlg, IDC_SAVEHANDS, BST_CHECKED);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			msg = GetDlgItem(hDlg, IDC_TITLETEXT);
			GetWindowText(msg, title2String, TITLE_LEN);
			msg = GetDlgItem(hDlg, IDC_HEADERTEXT);
			GetWindowText(msg, header2String, HEADER_LEN);
			msg = GetDlgItem(hDlg, IDC_DATETEXT);
			GetWindowText(msg, date2String, HEADER_LEN);
			writeHands = IsDlgButtonChecked(hDlg, IDC_SAVEHANDS) == BST_CHECKED;
			addComments = IsDlgButtonChecked(hDlg, IDC_ADDCOMMENT) == BST_CHECKED;
			if (addComments)
			{
				hwndOwner = GetParent(hDlg);
				HWND hEdit = GetDlgItem(hwndOwner, IDC_MAIN_EDIT);
				ShowWindow(hEdit, SW_SHOW);
			}
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL)
		{
			savingHands = false;
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


void ShowDeviations(HWND hDlg)
{
	HWND msg;
	char string[16];

	EnableWindow(GetDlgItem(hDlg, IDRETRY), FALSE);		// disable button 
	EnableWindow(GetDlgItem(hDlg, IDCONTINUE), FALSE);	// disable button 
	for (int c = 0; c < 5; c++)
	{
		for (int h = 0; h < 4; h++)
		{
			double result = 0;
			for (int i = 0; i < averageHandCount; i++)
			{
				result += results[i][c][h];
			}
			result /= averageHandCount;					// mean
			double deviation = 0;
			for (int i = 0; i < averageHandCount; i++)
			{
				double x = results[i][c][h] - result;
				deviation += x * x;
			}
			msg = GetDlgItem(hDlg, IDD_STATS00 + 4 * c + h);
			sprintf(string, "%.2f", sqrt(deviation) / averageHandCount);
			SetWindowText(msg, string);
		}
	}
}


void ShowStats(HWND hDlg)
{
	HWND msg;
	char string[16];

	EnableWindow(GetDlgItem(hDlg, IDRETRY), averageHandCount < MAX_HAND_COUNT);	// disable button when we reach max
	EnableWindow(GetDlgItem(hDlg, IDCONTINUE), TRUE);	// enable button 
	for (int c = 0; c < 5; c++)
	{
		for (int h = 0; h < 4; h++)
		{
			double result = 0;
			for (int i = 0; i < averageHandCount; i++)
			{
				result += results[i][c][h];
			}
			msg = GetDlgItem(hDlg, IDD_STATS00 + 4 * c + h);
			sprintf(string, "%.2f", result / averageHandCount);
			SetWindowText(msg, string);
		}
	}
}


// Message handler for creating hand records
INT_PTR CALLBACK ShowAverageStats(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	HWND hwndOwner;
	RECT rcOwner;

	switch (message)
	{
	case WM_INITDIALOG:
		if ((hwndOwner = GetParent(hDlg)) == NULL)
			hwndOwner = GetDesktopWindow();
		GetWindowRect(hwndOwner, &rcOwner);
		SetWindowPos(hDlg, HWND_TOP, rcOwner.left, rcOwner.top + 80, 0, 0, SWP_NOSIZE);
		ShowStats(hDlg);
		showDeviations = false;
		moreResults = false;
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			if (showDeviations)
			{
				ShowStats(hDlg);
				showDeviations = false;
				return (INT_PTR)FALSE;
			}
			else
			{
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
		}
		if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDRETRY)
		{
			EndDialog(hDlg, LOWORD(wParam));
			moreResults = true;
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDCONTINUE)
		{
			ShowDeviations(hDlg);
			showDeviations = true;
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


void ShowAnalysis(HWND hDlg)
{
	HWND msg;
	char string[16];

	for (int c = 0; c < 5; c++)
	{
		for (int h = 0; h < 4; h++)
		{
			double result = 0;
			for (int i = 0; i < averageHandCount; i++)
			{
				result += results[i][c][h];
			}
			msg = GetDlgItem(hDlg, IDD_STATS00 + 4 * c + h);
			sprintf(string, "%.2f", result / averageHandCount);
			SetWindowText(msg, string);
		}
	}
}


// Message handler for creating hand records
INT_PTR CALLBACK DisplayPlayAnalysis(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	HWND hwndOwner;
	RECT rcOwner;
	RECT rcOurs;
	HWND msg;
	char string[64];
	int lines;

	switch (message)
	{
	case WM_INITDIALOG:
		if ((hwndOwner = GetParent(hDlg)) == NULL)
			hwndOwner = GetDesktopWindow();
		GetWindowRect(hwndOwner, &rcOwner);
		GetWindowRect(hDlg, &rcOurs);
		SetWindowPos(hDlg, HWND_TOP, rcOwner.right - (rcOurs.right - rcOurs.left), rcOwner.top + 40, 0, 0, SWP_NOSIZE);
		msg = GetDlgItem(hDlg, IDD_ANALYZEFAILED);
		GetDealsFailed(string, sizeof(string));
		SetWindowText(msg, string);
		lines = GetLinesOfPlay();
		for (int i = 0; i < lines && i < 8; i++)
		{
			msg = GetDlgItem(hDlg, IDD_ANALYZETEXT1 + i);
			GetLinesString(string, sizeof(string), i);
			SetWindowText(msg, string);
			msg = GetDlgItem(hDlg, IDD_ANALYZE1 + i);
			GetLinesPercentage(string, sizeof(string), i);
			SetWindowText(msg, string);
		}
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCONTINUE)
		{
			char fname[MAX_PATH];
			if ((hwndOwner = GetParent(hDlg)) == NULL)
				hwndOwner = GetDesktopWindow();
			fname[0] = 0;
			if (SaveFileName(hwndOwner, fname, "Text Files (*.txt)", "txt"))
				SaveAnalysis(hwndOwner, fname);
			break;
		}
		else if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


// Message handler for getting the contract and declarer
INT_PTR CALLBACK GetPlayContract(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	HWND hwndOwner;
	RECT rcOwner;
	HWND hwndList;
	int pos;
	int	bid, suit, declarer;
	char * contracts[7] = { "1", "2", "3", "4", "5", "6", "7" };

	switch (message)
	{
	case WM_INITDIALOG:
		if ((hwndOwner = GetParent(hDlg)) == NULL)
			hwndOwner = GetDesktopWindow();
		GetWindowRect(hwndOwner, &rcOwner);
		SetWindowPos(hDlg, HWND_TOP, rcOwner.left + CONTRACT_X, rcOwner.top + CONTRACT_Y, 0, 0, SWP_NOSIZE);
		hwndList = GetDlgItem(hDlg, IDC_CONTRACT1);
		GetBestContract(bid, suit, declarer);
		for (int i = 0; i < 7; i++)
		{
			pos = (int)SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM)contracts[i]);
//			SendMessage(hwndList, CB_SETITEMDATA, pos, (LPARAM)i);	// use this if pos != i
		}
		SendMessage(hwndList, CB_SETCURSEL, (WPARAM)(bid-1), 0);		// set default values
		SetFocus(hwndList);
		hwndList = GetDlgItem(hDlg, IDC_CONTRACT2);
		pos = (int)SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM)"Clubs");	// must match CONTRACTS
		pos = (int)SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM)"Diamonds");
		pos = (int)SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM)"Hearts");
		pos = (int)SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM)"Spades");
		pos = (int)SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM)"No Trump");
		SendMessage(hwndList, CB_SETCURSEL, (WPARAM)suit, 0);		// set default values
		hwndList = GetDlgItem(hDlg, IDC_DECLARER);
		pos = (int)SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM)"North");	// must match DIRECTION
		pos = (int)SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM)"East");
		pos = (int)SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM)"South");
		pos = (int)SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM)"West");
		SendMessage(hwndList, CB_SETCURSEL, (WPARAM)declarer, 0);		// set default values
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			playHand = true;
			hwndList = GetDlgItem(hDlg, IDC_CONTRACT1);
			playContract = SendMessage(hwndList, CB_GETCURSEL, 0, 0) + 1;
			hwndList = GetDlgItem(hDlg, IDC_CONTRACT2);
			playSuit = SendMessage(hwndList, CB_GETCURSEL, 0, 0);
			hwndList = GetDlgItem(hDlg, IDC_DECLARER);
			playDeclarer = SendMessage(hwndList, CB_GETCURSEL, 0, 0);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
	}
	return (INT_PTR)FALSE;
}


// Message handler for getting a random seed
INT_PTR CALLBACK GetRandomSeed(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	HWND hwndOwner;
	RECT rcOwner, rcWindow;
	int width, widthOwner;
	HWND msg;
	char string[16];

	switch (message)
	{
	case WM_INITDIALOG:
		if ((hwndOwner = GetParent(hDlg)) == NULL)
			hwndOwner = GetDesktopWindow();
		GetWindowRect(hwndOwner, &rcOwner);
		widthOwner = rcOwner.right - rcOwner.left;
		GetWindowRect(hDlg, &rcWindow);
		width = rcWindow.right - rcWindow.left;
		SetWindowPos(hDlg, HWND_TOP, rcOwner.left + (widthOwner - width) / 2, rcOwner.top + 40, 0, 0, SWP_NOSIZE);
		msg = GetDlgItem(hDlg, IDC_RANDOMSEED);
		SetWindowText(msg, "");
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			msg = GetDlgItem(hDlg, IDC_RANDOMSEED);
			GetWindowText(msg, string, sizeof(string));
			seed = (U32)atoi(string);
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


LRESULT CALLBACK EditWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId;
	HWND parent = GetParent(hWnd);
	switch (message)
	{
	case WM_SYSKEYDOWN:
		wmId = LOWORD(wParam);
		if (wmId == (int)'D')
			return SendMessage(parent, WM_COMMAND, IDM_DEAL, lParam);
		if (wmId == (int)'R')
			return SendMessage(parent, WM_COMMAND, IDM_REDEAL, lParam);
		if (wmId == (int)'L')
			return SendMessage(parent, WM_COMMAND, IDM_LOADHAND, lParam);
		if (wmId == (int)'S')
			return SendMessage(parent, WM_COMMAND, IDM_SAVEHAND, lParam);
		if (wmId == (int)'C')
			return SendMessage(parent, WM_COMMAND, IDM_CHANGEHANDS, lParam);
		if (wmId == (int)'H')
			return SendMessage(parent, WM_COMMAND, IDM_HELP, lParam);
		if (wmId == VK_OEM_2)			// the ? or / key
			return SendMessage(parent, WM_COMMAND, IDM_ABOUT, lParam);
		// fall through
	default:
		return CallWindowProc(oldEditWindowProc, hWnd, message, wParam, lParam);
	}
	return 0;
}


// Helper function to count set bits in the processor mask.
DWORD CountSetBits(ULONG_PTR bitMask)
{
	DWORD bitSetCount = 0;

	while (bitMask != 0)
	{
		if ((bitMask & 1) != 0)
			bitSetCount++;
		bitMask >>= 1;
	}
	return bitSetCount;
}

typedef BOOL(WINAPI *LPFN_GLPI)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);

BOOL CountCoresAndProcessors(int &coreCount, int &processorCount, int &L1CacheCount, int &L2CacheCount, int &L3CacheCount)
{
	LPFN_GLPI glpi;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = NULL;
	DWORD returnLength = 0;
	DWORD byteOffset = 0;
	PCACHE_DESCRIPTOR Cache;

	glpi = (LPFN_GLPI)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetLogicalProcessorInformation");
	if (NULL == glpi)
	{
		coreCount = processorCount = L1CacheCount = L2CacheCount = L3CacheCount = 1;
		return FALSE;
	}
	while (true)
	{
		BOOL rc = glpi(buffer, &returnLength);
		if (rc == FALSE)
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				if (buffer)
					free(buffer);
				buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(returnLength);
				if (NULL == buffer)
				{
					coreCount = processorCount = L1CacheCount = L2CacheCount = L3CacheCount = 1;
					return FALSE;
				}
			}
			else
			{
				coreCount = processorCount = L1CacheCount = L2CacheCount = L3CacheCount = 1;
				return FALSE;
			}
		}
		else
			break;
	}
	ptr = buffer;
	coreCount = processorCount = L1CacheCount = L2CacheCount = L3CacheCount = 0;
	while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength)
	{
		switch (ptr->Relationship)
		{
		case RelationNumaNode:			// Non-NUMA systems report a single record of this type.
			break;
		case RelationProcessorCore:
			coreCount++;
			// A hyperthreaded core supplies more than one logical processor.
			processorCount += CountSetBits(ptr->ProcessorMask);
			break;
		case RelationCache:
			// Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each cache. 
			Cache = &ptr->Cache;
			if (Cache->Level == 1)
				L1CacheCount++;
			else if (Cache->Level == 2)
				L2CacheCount++;
			else if (Cache->Level == 3)
				L3CacheCount++;
			break;
		case RelationProcessorPackage:	// Logical processors share a physical package.
			break;
		default:						// Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value
			break;
		}
		byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
		ptr++;
	}
	free(buffer);
	return TRUE;
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	HMENU hMenu, hSubMenu, hSubMenu2;
	HWND hEdit;
	char fname[MAX_PATH];
	int	x;

	hMenu = GetMenu(hWnd);									// turn on/off some menu items
	switch (message)
	{
	case WM_CREATE:
		// get number of processors and cores
		//	SYSTEM_INFO sysinfo;
		//	GetSystemInfo(&sysinfo);
		//	sysinfo.dwNumberOfProcessors is the number of logical processors
		CountCoresAndProcessors(coreCount, processorCount, L1CacheCount, L2CacheCount, L3CacheCount);
		// load common control library progress bar class
		icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
		icex.dwICC = ICC_PROGRESS_CLASS;					// load progresss bar class  
		InitCommonControlsEx(&icex);
		// create progress bar window and hide it until it is needed
		hProgressWindow = CreateWindowEx(WS_EX_TOPMOST | WS_EX_CLIENTEDGE | WS_EX_NOPARENTNOTIFY,
			PROGRESS_CLASS,	// Predefined class
			"Progress...",	// header text
			WS_CAPTION | WS_TABSTOP | WS_CHILD,				// Styles 
			PROGRESS_X,		// x position 
			PROGRESS_Y,		// y position 
			PROGRESS_WIDTH,	// width,height	
			PROGRESS_HEIGHT,	
			hWnd,			// Parent window
			(HMENU)0,		// no menu
			GetModuleHandle(NULL),
			NULL);			// Pointer not needed.
		if (hProgressWindow == NULL)
		{
			int err = GetLastError();
			MessageBox(hWnd, "Cannot create Progress Bar!", "Internal error", MB_ICONSTOP | MB_OK | MB_APPLMODAL);
		}
		else
			ShowWindow(hProgressWindow, SW_HIDE);
		//		hBackgnd = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BACKGND));
		hCards = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CARDS));
		if (hCards == NULL)
			MessageBox(hWnd, "Could not load art!", "Error", MB_OK | MB_ICONEXCLAMATION);
		hCardsOff = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CARDS_OFF));
		if (hCardsOff == NULL)
			MessageBox(hWnd, "Could not load art!", "Error", MB_OK | MB_ICONEXCLAMATION);
		hCardMark = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CARD_MARK));
		if (hCardMark == NULL)
			MessageBox(hWnd, "Could not load art!", "Error", MB_OK | MB_ICONEXCLAMATION);
		hCardPlus = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CARD_PLUS));
		if (hCardPlus == NULL)
			MessageBox(hWnd, "Could not load art!", "Error", MB_OK | MB_ICONEXCLAMATION);
		hCardMinus = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CARD_MINUS));
		if (hCardMinus == NULL)
			MessageBox(hWnd, "Could not load art!", "Error", MB_OK | MB_ICONEXCLAMATION);
		hCardLock = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CARD_LOCK));
		if (hCardLock == NULL)
			MessageBox(hWnd, "Could not load art!", "Error", MB_OK | MB_ICONEXCLAMATION);
		hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
			WS_CHILD | WS_VISIBLE| ES_MULTILINE | ES_WANTRETURN,
			0, 0, EDIT_WIDTH, EDIT_HEIGHT, hWnd, (HMENU)IDC_MAIN_EDIT, GetModuleHandle(NULL), NULL);
		if (hEdit == NULL)
			MessageBox(hWnd, "Could not create edit box.", "Error", MB_OK | MB_ICONERROR);
		else
		{
			RECT rcClient;
			GetClientRect(hWnd, &rcClient);
			SetWindowPos(hEdit, NULL, rcClient.left + 500, rcClient.top + 80, EDIT_WIDTH, EDIT_HEIGHT, SWP_NOZORDER);
			ShowWindow(hEdit, SW_HIDE);
			oldEditWindowProc = (WNDPROC)GetWindowLongPtr(hEdit, GWLP_WNDPROC);
			SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)EditWindowProc);
		}
		selected.hand = -1;									// no selected card
		ClearRestrictions();								// clear all restrictions
		TimeInit();											// init for timers
		RandomInit();										// init random numbers
		GameInit();
		if (LoadGameFile(hWnd) == false)					// no saved data
			InitGameFileData();								// use preset data
		analyzeHandCount = analyzeHandCounts[analyzeHandCountIndex];
		analyzeDepth = analyzeDepths[analyzeDepthIndex];
		hSubMenu = GetSubMenu(hMenu, SUBMENU_PLAY);
		hSubMenu2 = GetSubMenu(hSubMenu, SUBSUBMENU_ANALYZEDEPTH);
		CheckMenuItem(hSubMenu2, analyzeDepthIndex, MF_BYPOSITION | MF_CHECKED);
		hSubMenu2 = GetSubMenu(hSubMenu, SUBSUBMENU_ANALYZECOUNT);
		CheckMenuItem(hSubMenu2, analyzeHandCountIndex, MF_BYPOSITION | MF_CHECKED);
		savingHands = false;
		EnableMenuItem(hMenu, IDM_SAVESOLUTION, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_ANALYZEPLAY, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_ANALYZEPICKWIN, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_ANALYZEPICKLOSS, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SAVEEND, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SAVECANCEL, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_DEALERDEFAULT, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_VULDEFAULT, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_UNLOCK, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_UNLOCKVUL, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_SAVERESTRICTIONS, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_CLEARLOCK, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_RESTRICTIONWEST, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_RESTRICTIONNORTH, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_RESTRICTIONEAST, MF_BYCOMMAND | MF_GRAYED);
		NewDeal(hWnd);										// new deal
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_DEAL:
			if (writeHands && DoneSavingHands())
				goto ENDSAVE;
			if (savingHands)								// save current hand
				PrintHand(hWnd);
			NewDeal(hWnd, false);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_REDEAL:
			DoneButtonDisable(hWnd);						// disable done button if enabled
			NewDeal(hWnd, true);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_REDEALNEW:
			NewDeal(hWnd, true, 2, -1);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_REDEALNE:
			NewDeal(hWnd, true, 2, 3);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_REDEALNW:
			NewDeal(hWnd, true, 2, 1);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_REDEALEW:
			NewDeal(hWnd, true, 2, 0);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_CREATERECORDS:
			startSet = false;
			writeHands = false;
			DialogBox(hInst, MAKEINTRESOURCE(IDD_INPUTBOX), hWnd, CreateHandRecords);
			if (startSet)
			{
				ClearRestrictions();						// clear all restrictions
				EnableMenuItem(hMenu, IDM_SAVERESTRICTIONS, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_CLEARLOCK, MF_BYCOMMAND | MF_GRAYED);
				GameInit();									// restart hand numbering
				if (PDFStart(hWnd) == false)				// start PDF process & title page
					break;
				for (int i = 0; i < handCount; i++)
				{
					NewDeal(hWnd, false);					// deal a hand 
					PrintHand(hWnd);						// save to PDF file
					RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
				}
				strncpy(fname, headerString, MAX_PATH);
				PDFEnd(hWnd, fname, SaveFileName(hWnd, fname, "PDF Files (*.pdf)", "pdf") == false);
				if (writeHands)
					WriteHands(hWnd, fname);
				writeHands = false;
			}
			break;
		case IDM_CREATESTATSNS:
			memset(results, 0, sizeof(results));
			averageHandCount = 0;
			while (true)
			{
				for (int i = 0; i < AVERAGE_HAND_COUNT; i++)
				{
					NewDeal(hWnd, true, 2, 0);					// redeal EW cards
					UpdateContractResults(results, averageHandCount);
					RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
				}
				DialogBox(hInst, MAKEINTRESOURCE(IDD_STATSBOX), hWnd, ShowAverageStats);
				if (moreResults == false)
					break;
			}
			break;
		case IDM_CREATESTATSS:
			memset(results, 0, sizeof(results));
			averageHandCount = 0;
			while (true)
			{
				for (int i = 0; i < AVERAGE_HAND_COUNT; i++)
				{
					NewDeal(hWnd, true, 2, -1);					// redeal NEW cards
					UpdateContractResults(results, averageHandCount);
					RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
				}
				DialogBox(hInst, MAKEINTRESOURCE(IDD_STATSBOX), hWnd, ShowAverageStats);
				if (moreResults == false)
					break;
			}
			break;
		case IDM_CREATESTATSN:
			memset(results, 0, sizeof(results));
			averageHandCount = 0;
			while (true)
			{
				for (int i = 0; i < AVERAGE_HAND_COUNT; i++)
				{
					NewDeal(hWnd, true, 0, -1);					// redeal SEW cards
					UpdateContractResults(results, averageHandCount);
					RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
				}
				DialogBox(hInst, MAKEINTRESOURCE(IDD_STATSBOX), hWnd, ShowAverageStats);
				if (moreResults == false)
					break;
			}
			break;
		case IDM_LOADHAND:
			if (LoadFileName(hWnd, fname, "Portable Bridge Notation (*.pbn)", "pbn"))
			{
				if (LoadHand(hWnd, fname))
				{
					DoneButtonDisable(hWnd);				// disable done button if enabled
					CountHands();									// recount hands
					CreateContractList(hWnd);						// everything may change
					RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
				}
				else
					MessageBox(hWnd, "Could not load hand!", "Data Error", MB_OK | MB_ICONEXCLAMATION);
			}
			break;
		case IDM_SAVEHAND:
			fname[0] = 0;
			if (SaveFileName(hWnd, fname, "Portable Bridge Notation (*.pbn)", "pbn"))
			{
				SaveHand(hWnd, fname);
			}
			break;
		case IDM_SAVESTART:
			if (savingHands)								// nothing to do
				break;
			savingHands = true;
			writeHands = false;
			DialogBox(hInst, MAKEINTRESOURCE(IDD_INPUT2BOX), hWnd, SaveHands);
			if (savingHands)								// they did not cancel the operation
			{
				ResetDeal(hWnd);							// reset deal to 1 and make sure dealer and vulnerable is correct
				if (PDFStart(hWnd) == false)				// start PDF process & title page
					savingHands = false;
				else
				{
					EnableMenuItem(hMenu, IDM_CREATERECORDS, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(hMenu, IDM_SAVESTART, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(hMenu, IDM_SAVEEND, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(hMenu, IDM_SAVECANCEL, MF_BYCOMMAND | MF_ENABLED);
				}
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			}
			break;
		case IDM_SAVEEND:
ENDSAVE:	strncpy(fname, header2String, MAX_PATH);
			if (SaveFileName(hWnd, fname, "PDF Files (*.pdf)", "pdf") == false)
				break;										// keep save hands alive
			PrintHand(hWnd);								// save to PDF file
			// fall through to cancel or write the file
		case IDM_SAVECANCEL:
			PDFEnd(hWnd, fname, wmId == IDM_SAVECANCEL);
			if (writeHands)
				WriteHands(hWnd, fname);
			writeHands = false;
			savingHands = false;
			if (addComments)
			{
				addComments = false;
				hEdit = GetDlgItem(hWnd, IDC_MAIN_EDIT);
				ShowWindow(hEdit, SW_HIDE);							// hide comment window
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			}
			EnableMenuItem(hMenu, IDM_CREATERECORDS, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_SAVESTART, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_SAVEEND, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(hMenu, IDM_SAVECANCEL, MF_BYCOMMAND | MF_GRAYED);
			break;
		case IDM_LOADRESTRICTIONS:
			if (LoadFileName(hWnd, fname, "Text Files (*.txt)", "txt"))
			{
				LoadRestrictionFile(hWnd, fname);
				EnableRestrictionMenus(hMenu);						// enable or disable restriction menu items
				if (restriction.mask == 0)							// we may only have locked cards
				{
					EnableMenuItem(hMenu, IDM_RESTRICTIONWEST, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(hMenu, IDM_RESTRICTIONNORTH, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(hMenu, IDM_RESTRICTIONEAST, MF_BYCOMMAND | MF_GRAYED);
				}
				else
				{
					EnableMenuItem(hMenu, IDM_RESTRICTIONWEST, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(hMenu, IDM_RESTRICTIONNORTH, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(hMenu, IDM_RESTRICTIONEAST, MF_BYCOMMAND | MF_ENABLED);
				}
				hSubMenu = GetSubMenu(hMenu, SUBMENU_MODIFY);		// clear checks on change dealer and vul
				CheckMenuItem(hSubMenu, 2, MF_BYPOSITION | MF_UNCHECKED);
				CheckMenuItem(hSubMenu, 3, MF_BYPOSITION | MF_UNCHECKED);
				if (restriction.dealer == DIR_SYSTEMIC)
				{
					int x = (GetDeal() - 1) & 3;
					SetDealer(x);
					EnableMenuItem(hMenu, IDM_DEALERN, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(hMenu, IDM_DEALERE, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(hMenu, IDM_DEALERS, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(hMenu, IDM_DEALERW, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(hMenu, IDM_DEALERDEFAULT, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(hMenu, IDM_UNLOCK, MF_BYCOMMAND | MF_GRAYED);
					hSubMenu = GetSubMenu(hMenu, SUBMENU_RESTRICTIONS);
					CheckMenuItem(hSubMenu, 0, MF_BYPOSITION | MF_UNCHECKED);
				}
				else
				{
					SetDealer(restriction.dealer);
					EnableMenuItem(hMenu, IDM_DEALERN, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(hMenu, IDM_DEALERE, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(hMenu, IDM_DEALERS, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(hMenu, IDM_DEALERW, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(hMenu, IDM_DEALERDEFAULT, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(hMenu, IDM_UNLOCK, MF_BYCOMMAND | MF_ENABLED);
					hSubMenu = GetSubMenu(hMenu, SUBMENU_RESTRICTIONS);
					CheckMenuItem(hSubMenu, 0, MF_BYPOSITION | MF_CHECKED);
				}
				if (restriction.vul == VUL_SYSTEMIC)
				{
					int x = (GetDeal() - 1) & 15;
					SetVul(hWnd, x);
					EnableMenuItem(hMenu, IDM_VULNONE, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(hMenu, IDM_VULNS, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(hMenu, IDM_VULEW, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(hMenu, IDM_VULBOTH, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(hMenu, IDM_VULDEFAULT, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(hMenu, IDM_UNLOCKVUL, MF_BYCOMMAND | MF_GRAYED);
					hSubMenu = GetSubMenu(hMenu, SUBMENU_RESTRICTIONS);
					CheckMenuItem(hSubMenu, 1, MF_BYPOSITION | MF_UNCHECKED);
				}
				else
				{
					SetVul(hWnd, restriction.vul);
					EnableMenuItem(hMenu, IDM_VULNONE, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(hMenu, IDM_VULNS, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(hMenu, IDM_VULEW, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(hMenu, IDM_VULBOTH, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(hMenu, IDM_VULDEFAULT, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(hMenu, IDM_UNLOCKVUL, MF_BYCOMMAND | MF_ENABLED);
					hSubMenu = GetSubMenu(hMenu, SUBMENU_RESTRICTIONS);
					CheckMenuItem(hSubMenu, 1, MF_BYPOSITION | MF_CHECKED);
				}
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			}
			hSubMenu = GetSubMenu(hMenu, SUBMENU_RESTRICTIONS);
			CheckMenuItem(hSubMenu, 3, MF_BYPOSITION | MF_CHECKED);
			if ((restriction.mask & 0x10) == 0)
				CheckMenuItem(hSubMenu, 4, MF_BYPOSITION | MF_UNCHECKED);
			else
				CheckMenuItem(hSubMenu, 4, MF_BYPOSITION | MF_CHECKED);
			if ((restriction.mask & 0x20) == 0)
				CheckMenuItem(hSubMenu, 5, MF_BYPOSITION | MF_UNCHECKED);
			else
				CheckMenuItem(hSubMenu, 5, MF_BYPOSITION | MF_CHECKED);
			if ((restriction.mask & 0x40) == 0)
				CheckMenuItem(hSubMenu, 6, MF_BYPOSITION | MF_UNCHECKED);
			else
				CheckMenuItem(hSubMenu, 6, MF_BYPOSITION | MF_CHECKED);
			break;
		case IDM_SAVERESTRICTIONS:
			fname[0] = 0;
			if (SaveFileName(hWnd, fname, "Text Files (*.txt)", "txt"))
				SaveRestrictionFile(hWnd, fname);
			break;
		case IDM_ANALYZEPICKWIN:
			AnalyzeAnotherEWHand(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_ANALYZEPICKLOSS:
			AnalyzeFailedEWHand(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_ANALYZEDEPTH1:
		case IDM_ANALYZEDEPTH2:
		case IDM_ANALYZEDEPTH3:
		case IDM_ANALYZEDEPTH4:
		case IDM_ANALYZEDEPTH5:
		case IDM_ANALYZEDEPTH6:
			hSubMenu = GetSubMenu(hMenu, SUBMENU_PLAY);
			hSubMenu2 = GetSubMenu(hSubMenu, SUBSUBMENU_ANALYZEDEPTH);
			CheckMenuItem(hSubMenu2, analyzeDepthIndex, MF_BYPOSITION | MF_UNCHECKED);
			analyzeDepthIndex = wmId - IDM_ANALYZEDEPTH1;
			analyzeDepth = analyzeDepths[analyzeDepthIndex];
			CheckMenuItem(hSubMenu2, analyzeDepthIndex, MF_BYPOSITION | MF_CHECKED);
			break;
		case IDM_ANALYZESAMPLES2:
		case IDM_ANALYZESAMPLES3:
		case IDM_ANALYZESAMPLES4:
		case IDM_ANALYZESAMPLES5:
		case IDM_ANALYZESAMPLES10:
		case IDM_ANALYZESAMPLES20:
			hSubMenu = GetSubMenu(hMenu, SUBMENU_PLAY);
			hSubMenu2 = GetSubMenu(hSubMenu, SUBSUBMENU_ANALYZECOUNT);
			CheckMenuItem(hSubMenu2, analyzeHandCountIndex, MF_BYPOSITION | MF_UNCHECKED);
			analyzeHandCountIndex = wmId - IDM_ANALYZESAMPLES2;
			analyzeHandCount = analyzeHandCounts[analyzeHandCountIndex];
			CheckMenuItem(hSubMenu2, analyzeHandCountIndex, MF_BYPOSITION | MF_CHECKED);
			break;
		case IDM_PLAY:
			if (hDoneButton != NULL)						// we already are playing the hand
				break;
			DialogBox(hInst, MAKEINTRESOURCE(IDD_PLAYDIALOG), hWnd, GetPlayContract);
			if (playHand)
			{
				DoneButtonEnable(hWnd);
				PlayHandInit(hWnd);							// make our first move
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			}
			break;
		case IDM_SAVESOLUTION:
			fname[0] = 0;
			if (SaveFileName(hWnd, fname, "Text Files (*.txt)", "txt"))
				SaveSolutions(hWnd, fname, analyzeDepth);
			break;
		case IDM_ANALYZEPLAY:
			if (AnalyzePlay(hWnd, analyzeDepth))
			{
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ANALYZEBOX), hWnd, DisplayPlayAnalysis);
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			}
			break;
		case IDM_CHANGEHANDS:
			if (hDoneButton != NULL)						// we already are modifying the hand
				break;
			DoneButtonEnable(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_SETRANDOM:
			seed = 0;
			DialogBox(hInst, MAKEINTRESOURCE(IDD_RANDOMSEEDBOX), hWnd, GetRandomSeed);
			if (seed != 0)
			{
				RandomSeed(seed);
				DeckInit();									// reset the deck
#if 0			// never auto redeal
				if (hDoneButton == NULL)					// we already are not playing the hand or changing hands
				{
					NewDeal(hWnd, false);
					RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
				}
#endif
			}
			break;
		case IDC_DONEBUTTON:
			// either from modify hand or playing hand or locking cards
			if (playHand)
			{
				BackupPlayCount[0] = BackupPlayCount[1] = BackupPlayCount[2] = BackupPlayCount[3] = 0;
				DoneButtonDisable(hWnd);					// disable button
			}
			else if (lockCards)
			{
				EnableRestrictionMenus(hMenu);				// enable or disable restriction menu items
				DoneButtonDisable(hWnd);					// disable button
			}
			else
			{
				DoneButtonDisable(hWnd);					// disable button
				CountHands();								// recount hands
				CreateContractList(hWnd);					// everything may change when we modify a hand
			}
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDC_BACKBUTTON:
			PlayHandBack(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDC_RESTARTBUTTON:
			PlayHandBack(hWnd, true);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_SWAPNS:
			SwapHands(0, 2);
			if (hDoneButton == NULL)						// we are not modifying the hand
				CreateContractList(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_SWAPNE:
			SwapHands(0, 1);
			if (hDoneButton == NULL)						// we are not modifying the hand
				CreateContractList(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_SWAPNW:
			SwapHands(0, 3);
			if (hDoneButton == NULL)						// we are not modifying the hand
				CreateContractList(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_SWAPSE:
			SwapHands(2, 1);
			if (hDoneButton == NULL)						// we are not modifying the hand
				CreateContractList(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_SWAPSW:
			SwapHands(2, 3);
			if (hDoneButton == NULL)						// we are not modifying the hand
				CreateContractList(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_SWAPEW:
			SwapHands(1, 3);
			if (hDoneButton == NULL)						// we are not modifying the hand
				CreateContractList(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_ROTATE90:
			SwapHands(3, 0);
			SwapHands(3, 2);
			SwapHands(2, 1);
			if (hDoneButton == NULL)						// we are not modifying the hand
				CreateContractList(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_ROTATE180:
			SwapHands(0, 2);
			SwapHands(1, 3);
			if (hDoneButton == NULL)						// we are not modifying the hand
				CreateContractList(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_ROTATE270:
			SwapHands(0, 1);
			SwapHands(1, 2);
			SwapHands(2, 3);
			if (hDoneButton == NULL)						// we are not modifying the hand
				CreateContractList(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_SWAPSH:
			SwapSuits(0, 1);
			if (hDoneButton == NULL)						// we are not modifying the hand
				CreateContractList(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_SWAPSD:
			SwapSuits(0, 2);
			if (hDoneButton == NULL)						// we are not modifying the hand
				CreateContractList(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_SWAPSC:
			SwapSuits(0, 3);
			if (hDoneButton == NULL)						// we are not modifying the hand
				CreateContractList(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_SWAPHD:
			SwapSuits(1, 2);
			if (hDoneButton == NULL)						// we are not modifying the hand
				CreateContractList(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_SWAPHC:
			SwapSuits(1, 3);
			if (hDoneButton == NULL)						// we are not modifying the hand
				CreateContractList(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_SWAPDC:
			SwapSuits(2, 3);
			if (hDoneButton == NULL)						// we are not modifying the hand
				CreateContractList(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_DEALERN:
		case IDM_DEALERE:
		case IDM_DEALERS:
		case IDM_DEALERW:
			x = wmId - IDM_DEALERN;
			if (x != GetDealer())							// dealer was changed
			{
				SetDealer(x);
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			}
			x = (GetDeal() - 1) & 3;
			if (x != GetDealer())							// not default
			{
				hSubMenu = GetSubMenu(hMenu, SUBMENU_MODIFY);
				CheckMenuItem(hSubMenu, 2, MF_BYPOSITION | MF_CHECKED);
				EnableMenuItem(hMenu, IDM_DEALERDEFAULT, MF_BYCOMMAND | MF_ENABLED);
			}
			else
			{
				hSubMenu = GetSubMenu(hMenu, SUBMENU_MODIFY);
				CheckMenuItem(hSubMenu, 2, MF_BYPOSITION | MF_UNCHECKED);
				EnableMenuItem(hMenu, IDM_DEALERDEFAULT, MF_BYCOMMAND | MF_GRAYED);
			}
			break;
		case IDM_DEALERDEFAULT:
			x = (GetDeal() - 1) & 3;
			if (x != GetDealer())							// dealer was changed
			{
				SetDealer(x);
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			}
			hSubMenu = GetSubMenu(hMenu, SUBMENU_MODIFY);
			CheckMenuItem(hSubMenu, 2, MF_BYPOSITION | MF_UNCHECKED);
			EnableMenuItem(hMenu, IDM_DEALERDEFAULT, MF_BYCOMMAND | MF_GRAYED);
			break;
		case IDM_LOCKN:
		case IDM_LOCKE:
		case IDM_LOCKS:
		case IDM_LOCKW:
			hSubMenu = GetSubMenu(hMenu, SUBMENU_RESTRICTIONS);
			CheckMenuItem(hSubMenu, 0, MF_BYPOSITION | MF_CHECKED);
			x = wmId - IDM_LOCKN;
			if (x != GetDealer()							// dealer was changed
			|| restriction.dealer == DIR_SYSTEMIC)			// dealer was not locked
			{
				SetDealer(x);
				restriction.dealer = (DIRECTION)x;
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
				EnableMenuItem(hMenu, IDM_DEALERN, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_DEALERE, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_DEALERS, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_DEALERW, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_DEALERDEFAULT, MF_BYCOMMAND | MF_GRAYED);
				hSubMenu = GetSubMenu(hMenu, SUBMENU_MODIFY);
				CheckMenuItem(hSubMenu, 2, MF_BYPOSITION | MF_UNCHECKED);
				EnableMenuItem(hMenu, IDM_UNLOCK, MF_BYCOMMAND | MF_ENABLED);
			}
			break;
		case IDM_UNLOCK:
			hSubMenu = GetSubMenu(hMenu, SUBMENU_RESTRICTIONS);
			CheckMenuItem(hSubMenu, 0, MF_BYPOSITION | MF_UNCHECKED);
			x = (GetDeal() - 1) & 3;
			SetDealer(x);
			restriction.dealer = DIR_SYSTEMIC;
			EnableMenuItem(hMenu, IDM_DEALERN, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_DEALERE, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_DEALERS, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_DEALERW, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_DEALERDEFAULT, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(hMenu, IDM_UNLOCK, MF_BYCOMMAND | MF_GRAYED);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_VULNONE:
		case IDM_VULNS:
		case IDM_VULEW:
		case IDM_VULBOTH:
			x = wmId - IDM_VULNONE;
			if (x != GetVul())								// vul was changed
			{
				SetVul(hWnd, x);
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			}
			x = (GetDeal() - 1) & 15;
			if (x != GetVul())								// not default
			{
				hSubMenu = GetSubMenu(hMenu, SUBMENU_MODIFY);
				CheckMenuItem(hSubMenu, 3, MF_BYPOSITION | MF_CHECKED);
				EnableMenuItem(hMenu, IDM_VULDEFAULT, MF_BYCOMMAND | MF_ENABLED);
			}
			else
			{
				hSubMenu = GetSubMenu(hMenu, SUBMENU_MODIFY);
				CheckMenuItem(hSubMenu, 3, MF_BYPOSITION | MF_UNCHECKED);
				EnableMenuItem(hMenu, IDM_VULDEFAULT, MF_BYCOMMAND | MF_GRAYED);
			}
			break;
		case IDM_VULDEFAULT:
			x = (GetDeal() - 1) & 15;
			if (x != GetVul())								// was changed
			{
				SetVul(hWnd, x);
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			}
			hSubMenu = GetSubMenu(hMenu, SUBMENU_MODIFY);
			CheckMenuItem(hSubMenu, 3, MF_BYPOSITION | MF_UNCHECKED);
			EnableMenuItem(hMenu, IDM_VULDEFAULT, MF_BYCOMMAND | MF_GRAYED);
			break;
		case IDM_LOCKVULNONE:
		case IDM_LOCKVULNS:
		case IDM_LOCKVULEW:
		case IDM_LOCKVULBOTH:
			hSubMenu = GetSubMenu(hMenu, SUBMENU_RESTRICTIONS);
			CheckMenuItem(hSubMenu, 1, MF_BYPOSITION | MF_CHECKED);
			x = wmId - IDM_LOCKVULNONE;
			if (x != GetVul()								// vul was changed
			|| restriction.vul == VUL_SYSTEMIC)				// vul was not locked
			{
				SetVul(hWnd, x);
				restriction.vul = (VULNERABLE)x;
				EnableMenuItem(hMenu, IDM_VULNONE, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_VULNS, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_VULEW, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_VULBOTH, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_VULDEFAULT, MF_BYCOMMAND | MF_GRAYED);
				hSubMenu = GetSubMenu(hMenu, SUBMENU_MODIFY);
				CheckMenuItem(hSubMenu, 3, MF_BYPOSITION | MF_UNCHECKED);
				EnableMenuItem(hMenu, IDM_UNLOCKVUL, MF_BYCOMMAND | MF_ENABLED);
				RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			}
			break;
		case IDM_UNLOCKVUL:
			hSubMenu = GetSubMenu(hMenu, SUBMENU_RESTRICTIONS);
			CheckMenuItem(hSubMenu, 1, MF_BYPOSITION | MF_UNCHECKED);
			x = (GetDeal() - 1) & 15;
			SetVul(hWnd, x);
			restriction.vul = VUL_SYSTEMIC;
			EnableMenuItem(hMenu, IDM_VULNONE, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_VULNS, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_VULEW, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_VULBOTH, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_VULDEFAULT, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(hMenu, IDM_UNLOCKVUL, MF_BYCOMMAND | MF_GRAYED);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_RESTRICTIONSOUTH:
			restrictPageStart = restrictPage = RESTRICT_SOUTH;
			restrictMaxPages = 4;
			DialogBox(hInst, MAKEINTRESOURCE(IDD_RESTRICTIONBOX), hWnd, Restrictions);
			hSubMenu = GetSubMenu(hMenu, SUBMENU_RESTRICTIONS);
			if (restriction.mask == 0)
			{
				CheckMenuItem(hSubMenu, 3, MF_BYPOSITION | MF_UNCHECKED);
				CheckMenuItem(hSubMenu, 4, MF_BYPOSITION | MF_UNCHECKED);
				CheckMenuItem(hSubMenu, 5, MF_BYPOSITION | MF_UNCHECKED);
				CheckMenuItem(hSubMenu, 6, MF_BYPOSITION | MF_UNCHECKED);
				EnableMenuItem(hMenu, IDM_RESTRICTIONWEST, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_RESTRICTIONNORTH, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(hMenu, IDM_RESTRICTIONEAST, MF_BYCOMMAND | MF_GRAYED);
			}
			else
			{
				CheckMenuItem(hSubMenu, 3, MF_BYPOSITION | MF_CHECKED);
				EnableMenuItem(hMenu, IDM_RESTRICTIONWEST, MF_BYCOMMAND | MF_ENABLED);
				EnableMenuItem(hMenu, IDM_RESTRICTIONNORTH, MF_BYCOMMAND | MF_ENABLED);
				EnableMenuItem(hMenu, IDM_RESTRICTIONEAST, MF_BYCOMMAND | MF_ENABLED);
			}
			EnableRestrictionMenus(hMenu);						// enable or disable restriction menu items
			break;
		case IDM_RESTRICTIONWEST:
			restrictPageStart = restrictPage = RESTRICT_WEST;
			restrictMaxPages = 1;
			DialogBox(hInst, MAKEINTRESOURCE(IDD_RESTRICTIONBOX), hWnd, Restrictions);
			hSubMenu = GetSubMenu(hMenu, SUBMENU_RESTRICTIONS);
			if ((restriction.mask & 0x10) == 0)
				CheckMenuItem(hSubMenu, 4, MF_BYPOSITION | MF_UNCHECKED);
			else
				CheckMenuItem(hSubMenu, 4, MF_BYPOSITION | MF_CHECKED);
			break;
		case IDM_RESTRICTIONNORTH:
			restrictPageStart = restrictPage = RESTRICT_NORTH;
			restrictMaxPages = 1;
			DialogBox(hInst, MAKEINTRESOURCE(IDD_RESTRICTIONBOX), hWnd, Restrictions);
			hSubMenu = GetSubMenu(hMenu, SUBMENU_RESTRICTIONS);
			if ((restriction.mask & 0x20) == 0)
				CheckMenuItem(hSubMenu, 5, MF_BYPOSITION | MF_UNCHECKED);
			else
				CheckMenuItem(hSubMenu, 5, MF_BYPOSITION | MF_CHECKED);
			break;
		case IDM_RESTRICTIONEAST:
			restrictPageStart = restrictPage = RESTRICT_EAST;
			restrictMaxPages = 1;
			DialogBox(hInst, MAKEINTRESOURCE(IDD_RESTRICTIONBOX), hWnd, Restrictions);
			hSubMenu = GetSubMenu(hMenu, SUBMENU_RESTRICTIONS);
			if ((restriction.mask & 0x40) == 0)
				CheckMenuItem(hSubMenu, 6, MF_BYPOSITION | MF_UNCHECKED);
			else
				CheckMenuItem(hSubMenu, 6, MF_BYPOSITION | MF_CHECKED);
			break;
		case IDM_LOCKCARDS:
			if (hDoneButton != NULL)						// we already are playing the hand or modifying the hand
				break;
			lockCards = true;
			DoneButtonEnable(hWnd);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_CLEARRESTRICTIONS:
			restriction.mask = 0;
			hSubMenu = GetSubMenu(hMenu, SUBMENU_RESTRICTIONS);
			restriction.dealer = DIR_SYSTEMIC;
			CheckMenuItem(hSubMenu, 0, MF_BYPOSITION | MF_UNCHECKED);
			x = (GetDeal() - 1) & 3;
			SetDealer(x);
			EnableMenuItem(hMenu, IDM_DEALERN, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_DEALERE, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_DEALERS, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_DEALERW, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_DEALERDEFAULT, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(hMenu, IDM_UNLOCK, MF_BYCOMMAND | MF_GRAYED);
			restriction.vul = VUL_SYSTEMIC;
			CheckMenuItem(hSubMenu, 1, MF_BYPOSITION | MF_UNCHECKED);
			x = (GetDeal() - 1) & 15;
			SetVul(hWnd, x);
			EnableMenuItem(hMenu, IDM_VULNONE, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_VULNS, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_VULEW, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_VULBOTH, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_VULDEFAULT, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(hMenu, IDM_UNLOCKVUL, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(hMenu, IDM_SAVERESTRICTIONS, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(hMenu, IDM_CLEARLOCK, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(hMenu, IDM_RESTRICTIONWEST, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(hMenu, IDM_RESTRICTIONNORTH, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(hMenu, IDM_RESTRICTIONEAST, MF_BYCOMMAND | MF_GRAYED);
			hSubMenu = GetSubMenu(hMenu, SUBMENU_RESTRICTIONS);
			CheckMenuItem(hSubMenu, 3, MF_BYPOSITION | MF_UNCHECKED);
			CheckMenuItem(hSubMenu, 4, MF_BYPOSITION | MF_UNCHECKED);
			CheckMenuItem(hSubMenu, 5, MF_BYPOSITION | MF_UNCHECKED);
			CheckMenuItem(hSubMenu, 6, MF_BYPOSITION | MF_UNCHECKED);
			// fall through to IDM_CLEARLOCK
		case IDM_CLEARLOCK:
			ClearLockedCards();
			EnableRestrictionMenus(hMenu);						// enable or disable restriction menu items
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_HIDENORTH:
			if (hideHands[0] = !hideHands[0])
				CheckMenuItem(hMenu, IDM_HIDENORTH, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_HIDENORTH, MF_BYCOMMAND | MF_UNCHECKED);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_HIDESOUTH:
			if (hideHands[2] = !hideHands[2])
				CheckMenuItem(hMenu, IDM_HIDESOUTH, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_HIDESOUTH, MF_BYCOMMAND | MF_UNCHECKED);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_HIDEEAST:
			if (hideHands[1] = !hideHands[1])
				CheckMenuItem(hMenu, IDM_HIDEEAST, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_HIDEEAST, MF_BYCOMMAND | MF_UNCHECKED);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_HIDEWEST:
			if (hideHands[3] = !hideHands[3])
				CheckMenuItem(hMenu, IDM_HIDEWEST, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_HIDEWEST, MF_BYCOMMAND | MF_UNCHECKED);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_HIDECOUNTS:
			if (hideHandCounts = !hideHandCounts)
				CheckMenuItem(hMenu, IDM_HIDECOUNTS, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_HIDECOUNTS, MF_BYCOMMAND | MF_UNCHECKED);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_HIDEDDR:
			if (hideContracts = !hideContracts)
				CheckMenuItem(hMenu, IDM_HIDEDDR, MF_BYCOMMAND | MF_CHECKED);
			else
				CheckMenuItem(hMenu, IDM_HIDEDDR, MF_BYCOMMAND | MF_UNCHECKED);
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			break;
		case IDM_HELP:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_HELPBOX), hWnd, Help);
			break;
		case IDM_HELPCOUNT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_HELPCOUNTBOX), hWnd, Help2);
			break;
		case IDM_HELPPLAY:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_HELPPLAYBOX), hWnd, Help2);
			break;
		case IDM_HELPDEAL:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_HELPDEALBOX), hWnd, Help2);
			break;
		case IDM_HELPHIDE:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_HELPHIDEBOX), hWnd, Help2);
			break;
		case IDM_HELPMODIFY:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_HELPMODIFYBOX), hWnd, Help2);
			break;
		case IDM_HELPRESTRICT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_HELPRESTRICTBOX), hWnd, Help2);
			break;
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDOWN:
		if (hDoneButton != NULL)
			Select(hWnd, lParam & 0xffff, lParam >> 16);
		else 
			SetFocus(hWnd);
		break;
	case WM_LBUTTONUP:
		if (hDoneButton != NULL)
			EnableWindow(hDoneButton, SelectRelease(hWnd, lParam & 0xffff, lParam >> 16));
		break;
	case WM_LBUTTONDBLCLK:
		if (playHand
		&& hDoneButton != NULL)
		{
			Select(hWnd, lParam & 0xffff, lParam >> 16);
			int x = (MiddleRect.left + MiddleRect.right) >> 1;	// place it in the middle
			int y = (MiddleRect.top + MiddleRect.bottom) >> 1;
			EnableWindow(hDoneButton, SelectRelease(hWnd, x, y));
		}
		break;
	case WM_MOUSEMOVE:
		if (hDoneButton != NULL)
			SelectMove(hWnd, lParam & 0xffff, lParam >> 16);
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		DrawCards(hWnd, hdc, ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PDFEnd(hWnd, "", true);								// release memory in case we are int he process of saving files 
		SaveGameFile(hWnd);									// save the current game data
		DeleteObject(hCards);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


void CenterWindow(HWND CW_h2)
{
	RECT CW_rect1, CW_rect2;
	int CW_midx, CW_midy, CW_wx, CW_wy;
	HWND CW_h1;
	CW_h1 = GetDesktopWindow();
	GetWindowRect(CW_h1, &CW_rect1);
	GetWindowRect(CW_h2, &CW_rect2);
	CW_midx = (CW_rect1.right + CW_rect1.left) >> 1;
	CW_midy = (CW_rect1.bottom + CW_rect1.top) >> 1;
	CW_wx = CW_rect2.right - CW_rect2.left;
	CW_wy = CW_rect2.bottom - CW_rect2.top;
	MoveWindow(CW_h2, CW_midx - (CW_wx >> 1), CW_midy - (CW_wy >> 1), CW_wx, CW_wy, false);
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;			// allow double clicks
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BRIDGEDEALER));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_BRIDGEDEALER);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_BRIDGEDEALER));

	return RegisterClassEx(&wcex);
}


//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
bool InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPED | WS_SYSMENU,	// fixed size window
		0, 0, WINDOW_WIDTH + 6, WINDOW_HEIGHT + 44, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}
	CenterWindow(hWnd);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}


int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_BRIDGEDEALER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BRIDGEDEALER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	// _CrtDumpMemoryLeaks() is unnecessary 
	//	because the dds.dll releases their memory after here
	//	and all we would get is a dump of their memory
	return (int)msg.wParam;
}

