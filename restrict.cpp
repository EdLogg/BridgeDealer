#include "stdafx.h"

int			restrictPage;						// current index into restriction.restrict[]
int			restrictPageStart;					// starting index for this direction
int			restrictPages;						// number of active pages for this direction
int			restrictMaxPages;					// max number of pages for this direction


int ConvertString(char * string, bool max = false)
{
	if (string[0] == 0)							// blank string means use min or max
	{
		if (max)
			return 100;
		return -1;
	}
	return atoi(string);
}


int CountMask(int mask)
{
	int count = 0;
	while (mask != 0)
	{
		if ((mask & 1) != 0)
			count++;
		mask >>= 1;
	}
	return count;
}


void	ReadRestrictions(HWND hDlg, RESTRICT  * restrict)
{
	HWND msg;
	char string[32];

	restriction.mask |= (1 << restrictPage);
	restrict->exactShape = IsDlgButtonChecked(hDlg, IDC_SPECIFICSHAPE) == BST_CHECKED;
	restrict->HCP = IsDlgButtonChecked(hDlg, IDC_RESTRICTHCP) == BST_CHECKED;
	restrict->concentrated = IsDlgButtonChecked(hDlg, IDC_RESTRICTCONCEN) == BST_CHECKED;
	msg = GetDlgItem(hDlg, IDC_RESTRICTSMIN);
	GetWindowText(msg, string, sizeof(string));
	restrict->minCards[0] = ConvertString(string);
	msg = GetDlgItem(hDlg, IDC_RESTRICTHMIN);
	GetWindowText(msg, string, sizeof(string));
	restrict->minCards[1] = ConvertString(string);
	msg = GetDlgItem(hDlg, IDC_RESTRICTDMIN);
	GetWindowText(msg, string, sizeof(string));
	restrict->minCards[2] = ConvertString(string);
	msg = GetDlgItem(hDlg, IDC_RESTRICTCMIN);
	GetWindowText(msg, string, sizeof(string));
	restrict->minCards[3] = ConvertString(string);
	msg = GetDlgItem(hDlg, IDC_RESTRICTSMAX);
	GetWindowText(msg, string, sizeof(string));
	restrict->maxCards[0] = ConvertString(string, true);
	msg = GetDlgItem(hDlg, IDC_RESTRICTHMAX);
	GetWindowText(msg, string, sizeof(string));
	restrict->maxCards[1] = ConvertString(string, true);
	msg = GetDlgItem(hDlg, IDC_RESTRICTDMAX);
	GetWindowText(msg, string, sizeof(string));
	restrict->maxCards[2] = ConvertString(string, true);
	msg = GetDlgItem(hDlg, IDC_RESTRICTCMAX);
	GetWindowText(msg, string, sizeof(string));
	restrict->maxCards[3] = ConvertString(string, true);
	msg = GetDlgItem(hDlg, IDC_RESTRICTMIN);
	GetWindowText(msg, string, sizeof(string));
	restrict->minPoints = ConvertString(string);
	msg = GetDlgItem(hDlg, IDC_RESTRICTMAX);
	GetWindowText(msg, string, sizeof(string));
	restrict->maxPoints = ConvertString(string, true);
	msg = GetDlgItem(hDlg, IDC_RESTRICTQTMIN);
	GetWindowText(msg, string, sizeof(string));
	restrict->minQTricks = ConvertString(string);
	msg = GetDlgItem(hDlg, IDC_RESTRICTQTMAX);
	GetWindowText(msg, string, sizeof(string));
	restrict->maxQTricks = ConvertString(string, true);
	msg = GetDlgItem(hDlg, IDC_RESTRICTCONTMIN);
	GetWindowText(msg, string, sizeof(string));
	restrict->minControls = ConvertString(string);
	msg = GetDlgItem(hDlg, IDC_RESTRICTCONTMAX);
	GetWindowText(msg, string, sizeof(string));
	restrict->maxControls = ConvertString(string, true);
	msg = GetDlgItem(hDlg, IDC_RESTRICTLOSEMIN);
	GetWindowText(msg, string, sizeof(string));
	restrict->minLosers = ConvertString(string);
	msg = GetDlgItem(hDlg, IDC_RESTRICTLOSEMAX);
	GetWindowText(msg, string, sizeof(string));
	restrict->maxLosers = ConvertString(string, true);
	msg = GetDlgItem(hDlg, IDC_RESTRICTKRATIO);
	GetWindowText(msg, string, sizeof(string));
	restrict->kRatio = ConvertString(string);
}


bool	ValidRestrictions(HWND hDlg, RESTRICT  * restrict)
{
	if (restrict->minCards[0] > 0
	|| restrict->minCards[1] > 0
	|| restrict->minCards[2] > 0
	|| restrict->minCards[3] > 0
	|| restrict->maxCards[0] < 13
	|| restrict->maxCards[1] < 13
	|| restrict->maxCards[2] < 13
	|| restrict->maxCards[3] < 13
	|| restrict->minPoints > 0
	|| restrict->maxPoints < 37
	|| restrict->minQTricks > 0
	|| restrict->maxQTricks < 24
	|| restrict->minControls > 0
	|| restrict->maxControls < 24
	|| restrict->minPoints > 0
	|| restrict->maxPoints < 37
	|| restrict->minLosers > 0
	|| restrict->maxLosers < 37)
		return true;
	return false;
}


static DIRECTION restrictDir[MAX_RESTRICT] =
{
	DIR_SOUTH, DIR_SOUTH, DIR_SOUTH, DIR_SOUTH,
	DIR_WEST, DIR_NORTH, DIR_EAST,
};
static char * restrictTitles[MAX_RESTRICT] =
{
	"Restrict South",
	"Restrict South",
	"Restrict South",
	"Restrict South",
	"Restrict West",
	"Restrict North",
	"Restrict East",
};

// Message handler for about box
INT_PTR CALLBACK Restrictions(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	RESTRICT  * restrict;						// current restriction
	HWND hwndOwner;
	RECT rcOwner, rcWindow;
	int width, widthOwner;
	HWND msg;
	char string[32];

	restrict = &restriction.restrict[restrictPage];		// prevents compiler error
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
		msg = GetDlgItem(hDlg, IDC_RESTRICTTITLE);
		SetWindowText(msg, restrictTitles[restrictPage]);
NEWPAGE:
		restrict = &restriction.restrict[restrictPage];
		if ((restriction.mask & (1 << restrictPage)) == 0)		// new page defaults
		{
CLEARPAGE:
			restriction.mask |= (1 << restrictPage);
			restrict->exactShape = true;
			restrict->HCP = true;
			if (restrictPage == restrictPageStart)
				restrict->not = false;
			restrict->concentrated = false;
			restrict->minCards[0] = -1;
			restrict->minCards[1] = -1;
			restrict->minCards[2] = -1;
			restrict->minCards[3] = -1;
			restrict->maxCards[0] = 100;
			restrict->maxCards[1] = 100;
			restrict->maxCards[2] = 100;
			restrict->maxCards[3] = 100;
			restrict->minPoints = -1;
			restrict->maxPoints = 100;
			restrict->minQTricks = -1;
			restrict->maxQTricks = 100;
			restrict->minControls = -1;
			restrict->maxControls = 100;
			restrict->minLosers = -1;
			restrict->maxLosers = 100;
			restrict->kRatio = 75;
		}
		msg = GetDlgItem(hDlg, IDC_RESTRICTNOTTEXT);
		if (restrict->not)
			SetWindowText(msg, "NOT");
		else
			SetWindowText(msg, "");
		restrictPages = CountMask((((1 << restrictMaxPages) - 1) << restrictPageStart) & restriction.mask);
		restrict->dir = restrictDir[restrictPage];
		sprintf(string, "Page %d/%d", restrictPage - restrictPageStart + 1, restrictPages);
		SetDlgItemText(hDlg, IDC_RESTRICTPAGE, string);
		CheckDlgButton(hDlg, IDC_SPECIFICSHAPE, (restrict->exactShape ? BST_CHECKED : BST_UNCHECKED));
		CheckDlgButton(hDlg, IDC_GENERALSHAPE, (restrict->exactShape == false ? BST_CHECKED : BST_UNCHECKED));
		CheckDlgButton(hDlg, IDC_RESTRICTHCP, (restrict->HCP ? BST_CHECKED : BST_UNCHECKED));
		CheckDlgButton(hDlg, IDC_RESTRICTTPOINTS, (restrict->HCP == false ? BST_CHECKED : BST_UNCHECKED));
		CheckDlgButton(hDlg, IDC_RESTRICTCONCEN, (restrict->concentrated ? BST_CHECKED : BST_UNCHECKED));
		// left hand side
		msg = GetDlgItem(hDlg, IDC_RESTRICTSMIN);
		if (restrict->minCards[0] < 0)
			string[0] = 0;
		else
			sprintf(string, "%d", restrict->minCards[0]);
		SetWindowText(msg, string);
		msg = GetDlgItem(hDlg, IDC_RESTRICTHMIN);
		if (restrict->minCards[1] < 0)
			string[0] = 0;
		else
			sprintf(string, "%d", restrict->minCards[1]);
		SetWindowText(msg, string);
		msg = GetDlgItem(hDlg, IDC_RESTRICTDMIN);
		if (restrict->minCards[2] < 0)
			string[0] = 0;
		else
			sprintf(string, "%d", restrict->minCards[2]);
		SetWindowText(msg, string);
		msg = GetDlgItem(hDlg, IDC_RESTRICTCMIN);
		if (restrict->minCards[3] < 0)
			string[0] = 0;
		else
			sprintf(string, "%d", restrict->minCards[3]);
		SetWindowText(msg, string);
		msg = GetDlgItem(hDlg, IDC_RESTRICTSMAX);
		if (restrict->maxCards[0] > 99)
			string[0] = 0;
		else
			sprintf(string, "%d", restrict->maxCards[0]);
		SetWindowText(msg, string);
		msg = GetDlgItem(hDlg, IDC_RESTRICTHMAX);
		if (restrict->maxCards[1] > 99)
			string[0] = 0;
		else
			sprintf(string, "%d", restrict->maxCards[1]);
		SetWindowText(msg, string);
		msg = GetDlgItem(hDlg, IDC_RESTRICTDMAX);
		if (restrict->maxCards[2] > 99)
			string[0] = 0;
		else
			sprintf(string, "%d", restrict->maxCards[2]);
		SetWindowText(msg, string);
		msg = GetDlgItem(hDlg, IDC_RESTRICTCMAX);
		if (restrict->maxCards[2] > 99)
			string[0] = 0;
		else
			sprintf(string, "%d", restrict->maxCards[3]);
		SetWindowText(msg, string);
		// right hand side
		msg = GetDlgItem(hDlg, IDC_RESTRICTMIN);
		if (restrict->minPoints < 0)
			string[0] = 0;
		else
			sprintf(string, "%d", restrict->minPoints);
		SetWindowText(msg, string);
		msg = GetDlgItem(hDlg, IDC_RESTRICTMAX);
		if (restrict->maxPoints > 99)
			string[0] = 0;
		else
			sprintf(string, "%d", restrict->maxPoints);
		SetWindowText(msg, string);
		msg = GetDlgItem(hDlg, IDC_RESTRICTQTMIN);
		if (restrict->minQTricks < 0)
			string[0] = 0;
		else
			sprintf(string, "%d", restrict->minQTricks);
		SetWindowText(msg, string);
		msg = GetDlgItem(hDlg, IDC_RESTRICTQTMAX);
		if (restrict->maxQTricks > 99)
			string[0] = 0;
		else
			sprintf(string, "%d", restrict->maxQTricks);
		SetWindowText(msg, string);
		msg = GetDlgItem(hDlg, IDC_RESTRICTCONTMIN);
		if (restrict->minControls < 0)
			string[0] = 0;
		else
			sprintf(string, "%d", restrict->minControls);
		SetWindowText(msg, string);
		msg = GetDlgItem(hDlg, IDC_RESTRICTCONTMAX);
		if (restrict->maxControls > 99)
			string[0] = 0;
		else
			sprintf(string, "%d", restrict->maxControls);
		SetWindowText(msg, string);
		msg = GetDlgItem(hDlg, IDC_RESTRICTLOSEMIN);
		if (restrict->minLosers < 0)
			string[0] = 0;
		else
			sprintf(string, "%d", restrict->minLosers);
		SetWindowText(msg, string);
		msg = GetDlgItem(hDlg, IDC_RESTRICTLOSEMAX);
		if (restrict->maxLosers > 99)
			string[0] = 0;
		else
			sprintf(string, "%d", restrict->maxLosers);
		SetWindowText(msg, string);
		msg = GetDlgItem(hDlg, IDC_RESTRICTKRATIO);
		sprintf(string, "%d", restrict->kRatio);
		SetWindowText(msg, string);
		// deal with enabling buttons
		msg = GetDlgItem(hDlg, IDC_RESTRICTOR);
		if (restrictMaxPages == 1
		|| restrictPage + 1 >= restrictPageStart + restrictMaxPages)
			EnableWindow(msg, false);
		else
			EnableWindow(msg, true);
		msg = GetDlgItem(hDlg, IDC_RESTRICTNOT);
		if (restrictMaxPages == 1
		|| restrictPage + 1 >= restrictPageStart + restrictMaxPages)
			EnableWindow(msg, false);
		else
			EnableWindow(msg, true);
		msg = GetDlgItem(hDlg, IDC_RESTRICTPREV);
		if (restrictMaxPages == 1
			|| restrictPage == restrictPageStart)
			EnableWindow(msg, false);
		else
			EnableWindow(msg, true);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			ReadRestrictions(hDlg, restrict);			// read input from dialog
			if (ValidRestrictions(hDlg, restrict))		// something was actually entered
			{
				if (restrictPage + 1 < restrictPages)	// advance to next restriction if there is one
				{
					restrictPage++;
					goto NEWPAGE;
				}
			}
			else
				restriction.mask &= ~(1 << restrictPage);	// clear empty restriction
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL)
		{
			if (ValidRestrictions(hDlg, restrict) == false)	// nothing was there
				restriction.mask &= ~(1 << restrictPage);	// clear empty restriction
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_RESTRICTOR)
		{
			ReadRestrictions(hDlg, restrict);			// read input from dialog
			if (ValidRestrictions(hDlg, restrict))		// something was actually entered
			{
				restrictPage++;							// goto next page or create a new one
				restriction.restrict[restrictPage].not = false;
				goto NEWPAGE;
			}
			return (INT_PTR)TRUE;						// if there are no changes do nothing
		}
		else if (LOWORD(wParam) == IDC_RESTRICTNOT)
		{
			ReadRestrictions(hDlg, restrict);			// read input from dialog
			if (ValidRestrictions(hDlg, restrict))		// something was actually entered
			{
				restrictPage++;							// goto next page or create a new one
				restriction.restrict[restrictPage].not = true;
				goto NEWPAGE;
			}
			return (INT_PTR)TRUE;						// if there are no changes do nothing
		}
		else if (LOWORD(wParam) == IDC_RESTRICTPREV)
		{
			ReadRestrictions(hDlg, restrict);			// read input from dialog
			if (ValidRestrictions(hDlg, restrict) == false)	// nothing entered
			{
				restriction.mask &= ~(1 << restrictPage);	// remove restriction from the active mask
			}
			restrictPage--;
			goto NEWPAGE;
		}
		else if (LOWORD(wParam) == IDC_RESTRICTREMOVE)
		{
			// move higher pages down if there are higher ones
			restriction.mask &= ~(1 << restrictPage);		// remove restriction from the active mask
			for (int i = restrictPage; i + 1 < restrictPages; i++)
			{
				restriction.restrict[i] = restriction.restrict[i+1];
				restriction.mask |= (1 << i);				// move mask down too
				restriction.mask &= ~(1 << (i+1));
			}
			restrictPages--;
			if (restrictPage > restrictPageStart)
			{
				restrictPage--;								// goto previous page
				goto NEWPAGE;
			}
			if (restrictPages > 0)							// use the new current page
				goto NEWPAGE;
			if ((restriction.mask & 1) == 0)				// no south restrictions so clear them all
				restriction.mask = 0;
			EndDialog(hDlg, LOWORD(wParam));				// else exit
			return (INT_PTR)TRUE;
		}
		else if (LOWORD(wParam) == IDC_RESTRICTCLEAR)
		{
			goto CLEARPAGE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
