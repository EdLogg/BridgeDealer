#include "stdafx.h"

#define EDIT_LINES			4
#define EDIT_CHARS			256

jmp_buf env;

char titleString[TITLE_LEN + 1];			// one extra character for a new line
char headerString[HEADER_LEN + 1];
char dateString[HEADER_LEN + 1];
char title2String[TITLE_LEN + 1];			// one extra character for a new line
char header2String[HEADER_LEN + 1];
char date2String[HEADER_LEN + 1];
char *	sourceString = "Printed using Ed Logg's Bridge Dealer";
int	handCount;								// number of hands to be printed
int	avgPoints[4] = { 0, 0, 0, 0 };
int balanced[4] = { 0, 0, 0, 0 };
int voids[4] = { 0, 0, 0, 0 };
int singletons[4] = { 0, 0, 0, 0 };
int longSuits[4] = { 0, 0, 0, 0 };
int handsPrinted = 0;
HPDF_Doc  pdf = NULL;
HPDF_Page page = NULL;


// positions of the hands on the page as multiples of w and h
const int Xpos[18] = 
{
	2, 3, 
	0, 1, 2, 3,
	0, 1, 2, 3,
	0, 1, 2, 3,
	0, 1, 2, 3,
};
const int Ypos[18] = 
{
	4, 4,
	3, 3, 3, 3, 
	2, 2, 2, 2,
	1, 1, 1, 1, 
	0, 0, 0, 0,
};


int stringToken(char * string, char * delimit)
{
	int count = 0;
	while (string[count] != 0)
	{
		for (int n = 0; delimit[n] != 0; n++)
		{
			if (string[count] == delimit[n])
				return count;
		}
		count++;
	}
	return count;
}


void CheckForBalancedHands(HAND * hands)
{
	int twos;
	for (int h = 0; h < 4; h++)
	{
		twos = 0;
		for (int s = 0; s < 4; s++)
		{
			switch (hands[h].suits[s].count)
			{
			default:
				goto NEXT;								// unbalanced hand
			case 2:
				twos++;
				break;
			case 3:
			case 4:
			case 5:
				break;
			}
		}
		if (twos < 2)									// 4333, 4432, 5332 are balanced
			balanced[h]++;
NEXT: ;
	}
}

void ParseSpots(PART * part, char suit, char * string, int hand)
{
	string[0] = suit;
	string[1] = ' ';
	if (part->count == 0)
		voids[hand]++;
	else if (part->count == 1)
		singletons[hand]++;
	else if (part->count > 6)
		longSuits[hand]++;
	if (part->count == 0)
	{
		string[2] = '-';
		string[3] = 0;
		return;
	}
	for (int i = 0; i < part->count; i++)
	{
		string[2 + i] = SpotString[part->spots[i]];
	}
	string[2 + part->count] = 0;
}


void PrintTitle(HPDF_Doc pdf, HPDF_Page page, int pageNum, bool titleStats)
{
	HPDF_Font font = HPDF_GetFont(pdf, "Helvetica", NULL);
	HPDF_REAL height = HPDF_Page_GetHeight(page);
	HPDF_REAL width = HPDF_Page_GetWidth(page);
	HPDF_UINT w, h;
	HPDF_REAL tw;
	char string[256];

	h = (HPDF_UINT)(height / 5);
	w = (HPDF_UINT)(width / 4);

	// print a box around the title
	HPDF_Page_SetFontAndSize(page, font, 5);
	HPDF_Page_SetGrayFill(page, 0.2);
	HPDF_Page_SetGrayStroke(page, 0.2);
	HPDF_Page_SetLineWidth(page, 0.8);
	HPDF_Page_MoveTo(page, 0, height-0.5);			// top edge
	HPDF_Page_LineTo(page, 2 * w, height-0.5);
	HPDF_Page_Stroke(page);
	HPDF_Page_MoveTo(page, 0, 4 * h);				// bottom edge
	HPDF_Page_LineTo(page, 2 * w, 4 * h);
	HPDF_Page_Stroke(page);
	HPDF_Page_MoveTo(page, 0.5, height);			// left edge
	HPDF_Page_LineTo(page, 0.5, 4 * h);
	HPDF_Page_Stroke(page);
	HPDF_Page_MoveTo(page, 2 * w, height);			// right edge
	HPDF_Page_LineTo(page, 2 * w, 4 * h);
	HPDF_Page_Stroke(page);
	HPDF_Page_SetGrayFill(page, 0);
	HPDF_Page_SetGrayStroke(page, 0);

	if (savingHands)
	{
		// print the title of the page centered in the upper left box
		HPDF_Page_SetFontAndSize(page, font, 18);
		tw = HPDF_Page_TextWidth(page, title2String);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, (2 * w - tw) / 2, height - 20, title2String);
		HPDF_Page_EndText(page);
		// print the header
		HPDF_Page_SetFontAndSize(page, font, 15);
		tw = HPDF_Page_TextWidth(page, header2String);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, (2 * w - tw) / 2, height - h / 2 + 10, header2String);
		HPDF_Page_EndText(page);
		// print the date
		HPDF_Page_SetFontAndSize(page, font, 15);
		tw = HPDF_Page_TextWidth(page, date2String);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, (2 * w - tw) / 2, height - h / 2 - 10, date2String);
		HPDF_Page_EndText(page);
	}
	else if (titleStats == false)
	{
		// print the title of the page centered in the upper left box
		HPDF_Page_SetFontAndSize(page, font, 18);
		tw = HPDF_Page_TextWidth(page, titleString);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, (2 * w - tw) / 2, height - 20, titleString);
		HPDF_Page_EndText(page);
		// print the header
		HPDF_Page_SetFontAndSize(page, font, 15);
		tw = HPDF_Page_TextWidth(page, headerString);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, (2 * w - tw) / 2, height - h / 2 + 10, headerString);
		HPDF_Page_EndText(page);
		// print the date
		HPDF_Page_SetFontAndSize(page, font, 15);
		tw = HPDF_Page_TextWidth(page, dateString);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, (2 * w - tw) / 2, height - h / 2 - 10, dateString);
		HPDF_Page_EndText(page);
	}

	// print the source
	HPDF_Page_SetFontAndSize(page, font, 10);
	HPDF_Page_BeginText(page);
	HPDF_Page_TextOut(page, 5, height - h + 5, sourceString);
	HPDF_Page_EndText(page);
	// print the page number
	sprintf(string, "Page %d", pageNum);
	HPDF_Page_SetFontAndSize(page, font, 12);
	HPDF_Page_BeginText(page);
	HPDF_Page_TextOut(page, 2 * w - 40, height - h + 5, string);
	HPDF_Page_EndText(page);
}

#define COL_WITH	40

void PrintColumn(HPDF_Page page, int x, int y, char * header, int hand)
{
	HPDF_REAL tw;
	char string[32];

	tw = HPDF_Page_TextWidth(page, header);
	HPDF_Page_BeginText(page);
	HPDF_Page_TextOut(page, x + COL_WITH - tw, y + 99, header);
	HPDF_Page_EndText(page);

	sprintf(string, "%.2f", (double)avgPoints[hand] / handsPrinted);
	tw = HPDF_Page_TextWidth(page, string);
	HPDF_Page_BeginText(page);
	HPDF_Page_TextOut(page, x + COL_WITH - tw, y + 88, string);
	HPDF_Page_EndText(page);

	sprintf(string, "%d", balanced[hand]);
	tw = HPDF_Page_TextWidth(page, string);
	HPDF_Page_BeginText(page);
	HPDF_Page_TextOut(page, x + COL_WITH - tw, y + 77, string);
	HPDF_Page_EndText(page);

	sprintf(string, "%d", voids[hand]);
	tw = HPDF_Page_TextWidth(page, string);
	HPDF_Page_BeginText(page);
	HPDF_Page_TextOut(page, x + COL_WITH - tw, y + 66, string);
	HPDF_Page_EndText(page);

	sprintf(string, "%d", singletons[hand]);
	tw = HPDF_Page_TextWidth(page, string);
	HPDF_Page_BeginText(page);
	HPDF_Page_TextOut(page, x + COL_WITH - tw, y + 55, string);
	HPDF_Page_EndText(page);

	sprintf(string, "%d", longSuits[hand]);
	tw = HPDF_Page_TextWidth(page, string);
	HPDF_Page_BeginText(page);
	HPDF_Page_TextOut(page, x + COL_WITH - tw, y + 44, string);
	HPDF_Page_EndText(page);
}


void PrintStats(HPDF_Doc pdf, HPDF_Page page, bool titleStats)
{
	HPDF_Font font = HPDF_GetFont(pdf, "Helvetica", NULL);
	HPDF_REAL width = HPDF_Page_GetWidth(page);
	HPDF_REAL height = HPDF_Page_GetHeight(page);
	HPDF_UINT x, y;
	HPDF_REAL tw;

	if (titleStats)
	{
		x = 8;
		y = 4 * (HPDF_UINT)(height / 5);
	}
	else
	{
		x = (HPDF_UINT)(width / 2) + 8;
		y = 0;
	}
	HPDF_Page_SetFontAndSize(page, font, 11);
	tw = HPDF_Page_TextWidth(page, "Average HCP");
	HPDF_Page_BeginText(page);
	HPDF_Page_TextOut(page, x + 80 - tw, y + 88, "Average HCP");
	HPDF_Page_EndText(page);
	tw = HPDF_Page_TextWidth(page, "Balanced Hands");
	HPDF_Page_BeginText(page);
	HPDF_Page_TextOut(page, x + 80 - tw, y + 77, "Balanced Hands");
	HPDF_Page_EndText(page);
	tw = HPDF_Page_TextWidth(page, "Voids");
	HPDF_Page_BeginText(page);
	HPDF_Page_TextOut(page, x + 80 - tw, y + 66, "Voids");
	HPDF_Page_EndText(page);
	tw = HPDF_Page_TextWidth(page, "Singletons");
	HPDF_Page_BeginText(page);
	HPDF_Page_TextOut(page, x + 80 - tw, y + 55, "Singletons");
	HPDF_Page_EndText(page);
	tw = HPDF_Page_TextWidth(page, "Long Suits (>6)");
	HPDF_Page_BeginText(page);
	HPDF_Page_TextOut(page, x + 80 - tw, y + 44, "Long Suits (>6)");
	HPDF_Page_EndText(page);
	PrintColumn(page, x + 80, y, "North", 0);
	PrintColumn(page, x + 80 + COL_WITH, y, "South", 2);
	PrintColumn(page, x + 80 + 2 * COL_WITH, y, "West", 3);
	PrintColumn(page, x + 80 + 3 * COL_WITH, y, "East", 1);
}


#ifdef HPDF_DLL
void  __stdcall
#else
void
#endif
error_handler (HPDF_STATUS error_no, HPDF_STATUS detail_no, void * user_data)
{
    printf ("ERROR: error_no=%04X, detail_no=%u\n", (HPDF_UINT)error_no,
                (HPDF_UINT)detail_no);
    longjmp(env, 1);
}


bool PDFStart(HWND hWnd)
{
	pdf = HPDF_New(error_handler, NULL);
	if (!pdf)
	{
		MessageBox(hWnd, "Cannot create PDF Doc object!", "Internal error", MB_ICONSTOP | MB_OK | MB_APPLMODAL);
		return false;
	}
	if (setjmp(env))
	{
		HPDF_Free(pdf);
		MessageBox(hWnd, "Cannot create PDF file!", "File Error", MB_ICONSTOP | MB_OK | MB_APPLMODAL);
		exit(1);									// exit else we get exception in DispatchMessage()
	}
	HPDF_SetCompressionMode(pdf, HPDF_COMP_ALL);	// set compression mode
	handsPrinted = 0;								// init stats
	for (int i = 0; i < 4; i++)
	{
		avgPoints[i] = 0;
		balanced[i] = 0;
		voids[i] = 0;
		singletons[i] = 0;
		longSuits[i] = 0;
	}
	return true;
}


void PDFPrintHand(HWND hWnd, int handNum, int dealer, int vulnerable, HAND * hands, int * points, char * contracts1, char * contracts2, char * par)
{
	HPDF_Font font = HPDF_GetFont(pdf, "Helvetica", NULL);
	HPDF_REAL height, width;
	HPDF_REAL tw, max;
	HPDF_UINT x, y, w, h;
	char string[32];
	char string1[32];
	char string2[32];
	char string3[32];
	char string4[32];
	char string5[32];
	char string6[32];
	char string7[32];

	handsPrinted++;
	avgPoints[0] += points[0];
	avgPoints[1] += points[1];
	avgPoints[2] += points[2];
	avgPoints[3] += points[3];
	if (handNum != handsPrinted)						// wrong hand number
	{
		MessageBox(hWnd, "Hand number is not correct!", "Internal error", MB_ICONSTOP | MB_OK | MB_APPLMODAL);
		return;
	}

	if ((handNum % 18) == 1)							// add another page 
	{
		page = HPDF_AddPage(pdf);
		bool titleStats = false;
		if (savingHands == false						// not when saving hands
		&&  handCount > 18								// not on the first page
		&& (handCount - 1) / 18 == (handNum - 1) / 18	// on the last page
		&& ((handCount - 1) % 18) >= 16)				// put stats in title area if it does not fit on the bottom
			titleStats = true;
		PrintTitle(pdf, page, (handNum / 18) + 1, titleStats);	// draw title 
	}

	height = HPDF_Page_GetHeight(page);
	width = HPDF_Page_GetWidth(page);
	h = (HPDF_UINT)(height / 5);
	w = (HPDF_UINT)(width / 4);
	x = Xpos[(handNum-1) % 18] * w;
	y = Ypos[(handNum-1) % 18] * h;

	// print a box around hand
	HPDF_Page_SetFontAndSize(page, font, 5);
	HPDF_Page_SetGrayFill(page, 0.2);
	HPDF_Page_SetGrayStroke(page, 0.2);
	HPDF_Page_SetLineWidth(page, 0.8);
	if (y == 4 * h)										// add top edge if necessary
	{
		HPDF_Page_MoveTo(page, x, height-0.5);
		HPDF_Page_LineTo(page, x + w, height-0.5);
		HPDF_Page_Stroke(page);
	}
	if (x == 0)											// add left edge if necessary
	{
		HPDF_Page_MoveTo(page, 0.5, y);
		HPDF_Page_LineTo(page, 0.5, y + h);
		HPDF_Page_Stroke(page);
	}
	HPDF_Page_MoveTo(page, x + w, y);					// right edge
	HPDF_Page_LineTo(page, x + w, y + h);
	HPDF_Page_Stroke(page);
	HPDF_Page_MoveTo(page, x, y);						// bottom edge
	HPDF_Page_LineTo(page, x + w, y);
	HPDF_Page_Stroke(page);
	HPDF_Page_SetGrayFill(page, 0);
	HPDF_Page_SetGrayStroke(page, 0);

	// show hand number
	sprintf(string, "%2d", handNum);
	HPDF_Page_SetFontAndSize(page, font, 20);
	HPDF_Page_BeginText(page);
	HPDF_Page_TextOut(page, x + w - 24, y + h - 20, string);
	HPDF_Page_EndText(page);

	// show dealer
	HPDF_Page_SetFontAndSize(page, font, 9);
	HPDF_Page_BeginText(page);
	HPDF_Page_TextOut(page, x + 4, y + h - 10, DealerString[dealer]);
	HPDF_Page_EndText(page);

	// show vulnerability
	HPDF_Page_SetFontAndSize(page, font, 9);
	HPDF_Page_BeginText(page);
	HPDF_Page_TextOut(page, x + 4, y + h - 18, VulString[vulnerable]);
	HPDF_Page_EndText(page);

	CheckForBalancedHands(hands);

	HPDF_Page_SetFontAndSize(page, font, 11);
	// show N hand
	if (hideHands[DIR_NORTH] == false)
	{
		ParseSpots(&hands[0].suits[0], 'S', string, 0);
		max = HPDF_Page_TextWidth(page, string);
		ParseSpots(&hands[0].suits[1], 'H', string1, 0);
		tw = HPDF_Page_TextWidth(page, string1);
		if (max < tw) max = tw;
		ParseSpots(&hands[0].suits[2], 'D', string2, 0);
		tw = HPDF_Page_TextWidth(page, string2);
		if (max < tw) max = tw;
		ParseSpots(&hands[0].suits[3], 'C', string3, 0);
		tw = HPDF_Page_TextWidth(page, string3);
		if (max < tw) max = tw;
		ParseSpots(&hands[2].suits[0], 'S', string4, 2);
		tw = HPDF_Page_TextWidth(page, string4);
		if (max < tw) max = tw;
		ParseSpots(&hands[2].suits[1], 'H', string5, 2);
		tw = HPDF_Page_TextWidth(page, string5);
		if (max < tw) max = tw;
		ParseSpots(&hands[2].suits[2], 'D', string6, 2);
		tw = HPDF_Page_TextWidth(page, string6);
		if (max < tw) max = tw;
		ParseSpots(&hands[2].suits[3], 'C', string7, 2);
		tw = HPDF_Page_TextWidth(page, string7);
		if (max < tw) max = tw;
		tw = (w - max) / 2 + 4;
		if (tw < 44)									// do not overwrite the deal and vulnerable
			tw = 44;
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + tw, y + h - 11, string);
		HPDF_Page_EndText(page);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + tw, y + h - 22, string1);
		HPDF_Page_EndText(page);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + tw, y + h - 33, string2);
		HPDF_Page_EndText(page);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + tw, y + h - 44, string3);
		HPDF_Page_EndText(page);
	}

	// show S hand
	if (hideHands[DIR_SOUTH] == false)
	{
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + tw, y + h - 99, string4);
		HPDF_Page_EndText(page);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + tw, y + h - 110, string5);
		HPDF_Page_EndText(page);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + tw, y + h - 121, string6);
		HPDF_Page_EndText(page);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + tw, y + h - 132, string7);
		HPDF_Page_EndText(page);
	}

	// show E hand
	if (hideHands[DIR_EAST] == false)
	{
		ParseSpots(&hands[1].suits[0], 'S', string, 1);
		max = HPDF_Page_TextWidth(page, string);
		ParseSpots(&hands[1].suits[1], 'H', string1, 1);
		tw = HPDF_Page_TextWidth(page, string1);
		if (max < tw) max = tw;
		ParseSpots(&hands[1].suits[2], 'D', string2, 1);
		tw = HPDF_Page_TextWidth(page, string2);
		if (max < tw) max = tw;
		ParseSpots(&hands[1].suits[3], 'C', string3, 1);
		tw = HPDF_Page_TextWidth(page, string3);
		if (max < tw) max = tw;
		tw = w - max - 2;
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + tw, y + h - 55, string);
		HPDF_Page_EndText(page);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + tw, y + h - 66, string1);
		HPDF_Page_EndText(page);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + tw, y + h - 77, string2);
		HPDF_Page_EndText(page);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + tw, y + h - 88, string3);
		HPDF_Page_EndText(page);
	}

	// show W hand
	if (hideHands[DIR_WEST] == false)
	{
		ParseSpots(&hands[3].suits[0], 'S', string, 3);
		ParseSpots(&hands[3].suits[1], 'H', string1, 3);
		ParseSpots(&hands[3].suits[2], 'D', string2, 3);
		ParseSpots(&hands[3].suits[3], 'C', string3, 3);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + 2, y + h - 55, string);
		HPDF_Page_EndText(page);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + 2, y + h - 66, string1);
		HPDF_Page_EndText(page);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + 2, y + h - 77, string2);
		HPDF_Page_EndText(page);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + 2, y + h - 88, string3);
		HPDF_Page_EndText(page);
	}

	// show points
	if (hideHandCounts == false)
	{
		HPDF_Page_SetFontAndSize(page, font, 9);
		sprintf(string, "%2d", points[0]);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + 4 + 8, y + h - 105, string);
		HPDF_Page_EndText(page);
		sprintf(string, "%2d   %2d", points[3], points[1]);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + 4, y + h - 116, string);
		HPDF_Page_EndText(page);
		sprintf(string, "%2d", points[2]);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + 4 + 8, y + h - 127, string);
		HPDF_Page_EndText(page);
	}

	// show comments or contracts and par score 
	if (addComments)
	{
		char text[EDIT_CHARS];
		char lines[EDIT_LINES][EDIT_CHARS];
		int	line = 0, pos = 0;
		HPDF_REAL commentWidth = width / 4 - 3;
		HWND hEdit = GetDlgItem(hWnd, IDC_MAIN_EDIT);
		DWORD dwTextLength = GetWindowTextLength(hEdit);
		GetWindowText(hEdit, text, EDIT_CHARS);
		lines[0][0] = lines[1][0] = lines[2][0] = lines[3][0] = 0;
		for (int n = 0; n < EDIT_CHARS;)
		{
			if (text[n] == 0)								// end of comments
			{
				lines[line][pos] = 0;
				break;
			}
			if (text[n] == '\r'								// \r\n or \n is new line
			|| text[n] == '\n')
			{
				n++;
				if (text[n] == '\n')						// skip \n
					n++;
				if (pos > 0)								// we may be at the start of a new line anyway
				{
					lines[line][pos] = 0;
					pos = 0;
					if (++line >= EDIT_LINES)				// we are done
						break;
				}
				continue;
			}
			int count = stringToken(&text[n], " \r\n");		// find end of next token
RETRY:		strncpy(&lines[line][pos], &text[n], count);	// copy token
			lines[line][pos + count] = 0;
			tw = HPDF_Page_TextWidth(page, &lines[line][0]);
			if (tw > commentWidth)
			{
				if (pos > 0)								// end current line before token
				{
					lines[line][pos] = 0;
					pos = 0;
					if (++line >= EDIT_LINES)				// we are done
						break;
					goto RETRY;
				}
				else 										// break token up
				{
					int len = count;
					do
					{
						len--;
						lines[line][pos + len] = 0;			// remove one character
						tw = HPDF_Page_TextWidth(page, &lines[line][0]);
					} while (tw > commentWidth);
					if (++line >= EDIT_LINES)				// we are done
						break;
					count = count - len;					// copy remaining token to the line buffer
					n += len;
					pos = 0;
					goto RETRY;
				}
			}
			if (text[n + count] == ' ')						// copy ending space too if possible
			{
				lines[line][pos+count] = ' ';
				lines[line][pos+count+1] = 0;
				count++;
			}
			pos += count;
			n += count;
		}
		HPDF_Page_SetFontAndSize(page, font, 9);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + 2, y + 28, lines[0]);
		HPDF_Page_EndText(page);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + 2, y + 20, lines[1]);
		HPDF_Page_EndText(page);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + 2, y + 11, lines[2]);
		HPDF_Page_EndText(page);
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + 2, y + 2, lines[3]);
		HPDF_Page_EndText(page);
	}
	else if (hideContracts == false)
	{
		HPDF_Page_SetFontAndSize(page, font, 11);
		if (strlen(contracts2) == 0)
		{
			HPDF_Page_BeginText(page);
			HPDF_Page_TextOut(page, x + 2, y + 2 + 11, contracts1);
			HPDF_Page_EndText(page);
		}
		else
		{
			HPDF_Page_BeginText(page);
			HPDF_Page_TextOut(page, x + 2, y + 2 + 22, contracts1);
			HPDF_Page_EndText(page);
			HPDF_Page_BeginText(page);
			HPDF_Page_TextOut(page, x + 2, y + 2 + 11, contracts2);
			HPDF_Page_EndText(page);
		}
		HPDF_Page_BeginText(page);
		HPDF_Page_TextOut(page, x + 2, y + 2, par);
		HPDF_Page_EndText(page);
	}
}


void PDFEnd(HWND hWnd, char * fname, bool abort)
{
	if (abort)
		writeHands = false;
	if (abort == false
	&& handsPrinted > 0)								// no page started yet
	{
		bool titleStats = false;
		if (handsPrinted > 18
		&& ((handsPrinted - 1) % 18) >= 16)				// put stats in title area
			titleStats = true;
		if (savingHands == false						// not when saving hands
		&& (handsPrinted <= 16							// we have room for stats
		  || handsPrinted > 18))
			PrintStats(pdf, page, titleStats);
		HPDF_SaveToFile(pdf, fname);					// save the document to a file 
	}
	if (pdf != NULL)
		HPDF_Free(pdf);									// release object
	page = NULL;
	pdf = NULL;
}
