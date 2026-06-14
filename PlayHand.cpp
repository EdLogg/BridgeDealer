#include "stdafx.h"

// global variables
struct deal dl;										// current deal state
futureTricks optimal;								// list of optimal cards to play 
int analyzeHandCountSaved;							// number of hands to analyze saved so we can look at next failed or good hand
int analyzeHandCountIndex;							// index for number of hands
int analyzeDepthIndex;								// index for depth of analyze play
int analyzeHandCount;								// number of hands to analyze
int analyzeDepth;									// depth of analyze play
int analyzeHandCounts[IDM_ANALYZESAMPLES] = { 2, 3, 4, 5, 10, 20 };
int analyzeDepths[IDM_ANALYZEDEPTHS] = { 1, 2, 3, 4, 5, 6 };
int handToPlay;
int	playedRank[4];
int playedSuit[4];
// local variables
int plays;
int suitLead;
int threadIndex = 0;
int BackupPlayDeal;									// what deal are we using 
int BackupPlayPlays;								// how many plays have been made to start
int BackupPlayTrick;								// what trick should we stop at when we back up
int BackupPlaySpot[4][13];							// used to back up to the previous trick
int BackupPlaySuit[4][13];
int BackupPlayCount[4];								// count of entries above
int BackupLeader[13];
int MapTrump[5] = { 3, 2, 1, 0, 4 };				// map contracts playSuit to suits 


void 	GetDealString(uint64 mask, char * string, int len)
{
	int end = 0;
	int deal = 1;
	while (mask != 0)
	{
		if ((mask & 1) != 0)
			end += sprintf(&string[end], "%d,", deal);
		mask >>= 1;
		deal++;
	}
	if (end > 0)
		--end;
	string[end] = 0;
}


void	PrintTrick(FILE *file, TRICK * trick, int depth, bool alt, int samples)
{
	int p0, p1, p2, p3;
	p0 = trick->leader;
	p1 = p0 + 1;
	if (p1 == 4)
		p1 = 0;
	p2 = p1 + 1;
	if (p2 == 4)
		p2 = 0;
	p3 = p2 + 1;
	if (p3 == 4)
		p3 = 0;
	if (alt)
		for (int i = 1; i < depth; i++)
			if (samples == 0)
				fprintf(file, "               ");	// to replace previous trick
			else
				fprintf(file, "                      ");	// to replace previous trick
	if (samples == 0)
		fprintf(file, "%c:%c%c-%c%c-%c%c-%c%c, ", LeaderString[trick->leader],
		SpotString[trick->cards[p0].spot], SuitString[trick->cards[p0].suit],
		SpotString[trick->cards[p1].spot], SuitString[trick->cards[p1].suit],
		SpotString[trick->cards[p2].spot], SuitString[trick->cards[p2].suit],
		SpotString[trick->cards[p3].spot], SuitString[trick->cards[p3].suit]);
	else
		fprintf(file, "%c:%c%c-%c%c-%c%c-%c%c (%3d%%), ", LeaderString[trick->leader],
		SpotString[trick->cards[p0].spot], SuitString[trick->cards[p0].suit],
		SpotString[trick->cards[p1].spot], SuitString[trick->cards[p1].suit],
		SpotString[trick->cards[p2].spot], SuitString[trick->cards[p2].suit],
		SpotString[trick->cards[p3].spot], SuitString[trick->cards[p3].suit],
		trick->count * 100 / samples);
	if (depth == analyzeDepth)
	{
		if (samples == 0)
			fprintf(file, "\n");
		else
		{
			char string[128];
			GetDealString(trick->dealMask, string, 128);
			fprintf(file, "deals:%s\n", string);
		}
	}
}


void	PrintAllTricks(FILE *file, TRICK * trick, int depth, bool alt, int samples)
{
	PrintTrick(file, trick, depth + 1, alt, samples);
	if (trick->next != NULL)
		PrintAllTricks(file, trick->next, depth + 1, false, samples);
	if (trick->alt != NULL
	&& depth != 0)										// first level contains only solutions
	{
		fprintf(file, "             ");					// to replace "Solution %2d: " 
		PrintAllTricks(file, trick->alt, depth, true, samples);
	}
}


void PrintSolutions(FILE * file, TRICK * tricks, int samples)
{
	for (int i = 1; tricks != NULL; i++)
	{
		fprintf(file, "Solution %2d: ", i);
		PrintAllTricks(file, tricks, 0, false, samples);
		tricks = tricks->alt;
	}
}


void PlayHandInit(HWND hWnd)
{
	int First[DIRECTIONS] = { DIR_EAST, DIR_SOUTH, DIR_WEST, DIR_NORTH };
	NSWon = 0;
	EWWon = 0;
	dl.trump = MapTrump[playSuit];					// map contract to suit
	BackupLeader[0] = handToPlay = dl.first = First[playDeclarer];
	BackupPlayCount[0] = BackupPlayCount[1] = BackupPlayCount[2] = BackupPlayCount[3] = 0;
	BackupPlayTrick = 0;
	BackupPlayDeal = 0;
	BackupPlayPlays = 0;
	plays = 0;
	for (int h = 0; h < DDS_HANDS; h++)
	{
		playedRank[h] = SPOT_NONE;					// clear cards played
		for (int s = 0; s < DDS_SUITS; s++)
		{
			dl.remainCards[h][s] = 0;
			for (int c = 0; c < hands[h].suits[s].count; c++)
				dl.remainCards[h][s] |= 1 << (hands[h].suits[s].spots[c] + 2);	// 2 is bit 2 in deal
		}
	}
	dl.currentTrickSuit[0] = 0;
	dl.currentTrickSuit[1] = 0;
	dl.currentTrickSuit[2] = 0;
	dl.currentTrickRank[0] = 0;
	dl.currentTrickRank[1] = 0;
	dl.currentTrickRank[2] = 0;
#if defined(__linux) || defined(__APPLE__)
	SetMaxThreads(0);
#endif
	int res = SolveBoard(dl, -1, 3, 1, &optimal, threadIndex, NULL);
	if (res != RETURN_NO_FAULT)
	{
		char string[60];
		sprintf(string, "SolveBoard Failed! (%d)", res);
		MessageBox(hWnd, string, "Internal Error", MB_ICONSTOP | MB_OK | MB_APPLMODAL);
		return;
	}
	HMENU hMenu = GetMenu(hWnd);							// turn on/off some menu items
	if (handToPlay == playDeclarer || handToPlay == (playDeclarer ^ 2))
	{
		EnableMenuItem(hMenu, IDM_SAVESOLUTION, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_ANALYZEPLAY, MF_BYCOMMAND | MF_ENABLED);
	}
	else
	{
		EnableMenuItem(hMenu, IDM_SAVESOLUTION, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_ANALYZEPLAY, MF_BYCOMMAND | MF_GRAYED);
	}
	EnableMenuItem(hMenu, IDM_ANALYZEPICKWIN, MF_BYCOMMAND | MF_GRAYED);
	EnableMenuItem(hMenu, IDM_ANALYZEPICKLOSS, MF_BYCOMMAND | MF_GRAYED);
}


void PlayHandBack(HWND hWnd, bool restart)
{	
	int trick = NSWon + EWWon;
	if (BackupPlayDeal == 0							// at the start of the original hand
	&& trick == 0
	&& plays == 0)
		return;										// do nothing
	else if (BackupPlayDeal != 0					// at the beginning of another hand
	&& trick == BackupPlayTrick
	&& plays == BackupPlayPlays)
		return;										// do nothing
RESTART:
	if (plays != 0)
	{
		int who;
		for (int i = 0; i < plays; i++)
		{
			who = dl.first + i;
			if (who >= 4)
				who -= 4;
			if (BackupPlayDeal == 0
			|| trick > BackupPlayTrick
			|| i >= BackupPlayPlays)
				dl.remainCards[who][dl.currentTrickSuit[i]] |= 1 << dl.currentTrickRank[i];
		}
		plays = BackupPlayPlays;
	}
	else if (BackupPlayDeal == 0
	|| trick > BackupPlayTrick)
	{
		if (dl.first == 0 || dl.first == 2)
			NSWon -= 1;
		else
			EWWon -= 1;
		trick--;
		dl.first = BackupLeader[trick];
		for (int i = 0; i < 4; i++)
		{
			dl.remainCards[i][BackupPlaySuit[i][trick]] |= 1 << (BackupPlaySpot[i][trick] + 2);
		}
	}
	BackupPlayCount[0] = BackupPlayCount[1] = BackupPlayCount[2] = BackupPlayCount[3] = trick;
	if (BackupPlayDeal == 0
	&& trick < BackupPlayTrick)							// back up past last analysis
	{													// do not allow us to select previous hands from analysis
		HMENU hMenu = GetMenu(hWnd);					
		EnableMenuItem(hMenu, IDM_ANALYZEPICKWIN, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_ANALYZEPICKLOSS, MF_BYCOMMAND | MF_GRAYED);
		BackupPlayPlays = 0;						
		BackupPlayTrick = 0;				
	}
	if (restart)
	{
		if (BackupPlayDeal == 0
		&&  trick != 0)
			goto RESTART;
		if (BackupPlayDeal != 0
		&& trick > BackupPlayTrick)
			goto RESTART;
	}
	handToPlay = dl.first;
	playedRank[0] = playedRank[1] = playedRank[2] = playedRank[3] = SPOT_NONE;
	dl.currentTrickSuit[0] = 0;
	dl.currentTrickSuit[1] = 0;
	dl.currentTrickSuit[2] = 0;
	dl.currentTrickRank[0] = 0;
	dl.currentTrickRank[1] = 0;
	dl.currentTrickRank[2] = 0;
	int res = SolveBoard(dl, -1, 3, 1, &optimal, threadIndex, NULL);
	if (res != RETURN_NO_FAULT)
	{
		char string[60];
		sprintf(string, "SolveBoard Failed! (%d)", res);
		MessageBox(hWnd, string, "Internal Error", MB_ICONSTOP | MB_OK | MB_APPLMODAL);
	}
	HMENU hMenu = GetMenu(hWnd);							// turn on/off some menu items
	if (handToPlay == playDeclarer || handToPlay == (playDeclarer ^ 2))
	{
		EnableMenuItem(hMenu, IDM_SAVESOLUTION, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(hMenu, IDM_ANALYZEPLAY, MF_BYCOMMAND | MF_ENABLED);
	}
	else
	{
		EnableMenuItem(hMenu, IDM_SAVESOLUTION, MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(hMenu, IDM_ANALYZEPLAY, MF_BYCOMMAND | MF_GRAYED);
	}
}


// turn off last trick if we select next card
void PlayHandSelect()
{
	if (plays == 0)
	{
		playedRank[0] = playedRank[1] = playedRank[2] = playedRank[3] = SPOT_NONE;
	}
}


bool PlayHandLegal(int hand, int suit)
{
	if (plays == 0								// any card is legal for the first player
	|| dl.currentTrickSuit[0] == suit			// same suit is legal
	|| dl.remainCards[hand][dl.currentTrickSuit[0]] == 0) // no card in that suit so any card is fine
		return true;
	return false;
}


void PlayHandCard(HWND hWnd, int suit, int rank)
{
	int trick = NSWon + EWWon;
	BackupPlaySuit[handToPlay][trick] = playedSuit[handToPlay] = suit;
	BackupPlaySpot[handToPlay][trick] = playedRank[handToPlay] = rank;
	BackupPlayCount[handToPlay]++;
	dl.remainCards[handToPlay][playedSuit[handToPlay]] &= ~(1 << (playedRank[handToPlay] + 2));
	if (plays == 0)
		suitLead = suit;
	if (plays < 3)
	{
		dl.currentTrickSuit[plays] = playedSuit[handToPlay];
		dl.currentTrickRank[plays] = playedRank[handToPlay] + 2;
	}
	if (++handToPlay == 4)
		handToPlay = 0;
	if (++plays == 4)
	{
		plays = 0;
		int winner;
		bool trumped = false;
		int suitLeadRank = -1;
		for (int i = 0; i < 4; i++)
		{
			if (playedSuit[i] == MapTrump[playSuit])
			{
				if (trumped == false
				||  playedRank[i] > suitLeadRank)
				{
					trumped = true;
					suitLeadRank = playedRank[i];
					winner = i;
				}
			}
			else if (trumped == false
			&&  playedSuit[i] == suitLead
			&&  playedRank[i] > suitLeadRank)
			{
				suitLeadRank = playedRank[i];
				winner = i;
			}
		}
		if (winner == 0 || winner == 2)
			NSWon++;
		else
			EWWon++;
		handToPlay = dl.first = winner;
		dl.currentTrickSuit[0] = 0;
		dl.currentTrickSuit[1] = 0;
		dl.currentTrickSuit[2] = 0;
		dl.currentTrickRank[0] = 0;
		dl.currentTrickRank[1] = 0;
		dl.currentTrickRank[2] = 0;
	}
	HMENU hMenu = GetMenu(hWnd);							// turn on/off some menu items
	if (NSWon + EWWon < 13)
	{
		BackupLeader[NSWon + EWWon] = dl.first;
		int res = SolveBoard(dl, -1, 3, 1, &optimal, threadIndex, NULL);
		if (res != RETURN_NO_FAULT)
		{
			char string[60];
			sprintf(string, "SolveBoard Failed! (%d)", res);
			MessageBox(hWnd, string, "Internal Error", MB_ICONSTOP | MB_OK | MB_APPLMODAL);
			return;
		}
		if (handToPlay == playDeclarer || handToPlay == (playDeclarer ^ 2))
		{
			EnableMenuItem(hMenu, IDM_SAVESOLUTION, MF_BYCOMMAND | MF_ENABLED);
			EnableMenuItem(hMenu, IDM_ANALYZEPLAY, MF_BYCOMMAND | MF_ENABLED);
		}
		else
		{
			EnableMenuItem(hMenu, IDM_SAVESOLUTION, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(hMenu, IDM_ANALYZEPLAY, MF_BYCOMMAND | MF_GRAYED);
		}
	}
}


ANALYZEPLAY analyzePlays[ANALYZE_HAND_COUNT];	// information for all deals
struct deal deals[ANALYZE_HAND_COUNT];			// deals that meet the restrictions
futureTricks optimals[ANALYZE_HAND_COUNT];		// results from deals[] using SolveBoard()
bool	dealsSuceed[ANALYZE_HAND_COUNT];		// if these deals[] succeed making the contract
int		analyzeFail;							// number of deals that failed
int		analyzeCount;							// number of entries in analyzePlays[] that are used
int		good;									// index into next deal that succeeds or fails
int		bad;
double	analyzeTime;							// time taken to analyze entries in analyzePlays
int		allLinesOfPlayCount;					// how many entries in allLinesOfPlay[] are used
TRICK	allLinesOfPlay[MAX_ANALYSIS_TRICKS];	// lines of play for all deals
TRICK 	allLinesOfPlayStart;					// ->next points to first (highest rated) entry to use
int		countSolveBoard;
int		countLinesOfPlay;


TRICK * AddNewLineOfPlay(ANALYZEPLAY & analyzePlay, int depth)
{
	int index = analyzePlay.linesOfPlayNum;
	TRICK * start = &analyzePlay.linesOfPlay[index];
	TRICK * prev = NULL;
	for (int d = depth; d < analyzePlay.maxDepth; d++)
	{
		if (index == MAX_ANALYSIS_TRICKS)						// trick table is full
			return NULL;
		if (prev != NULL)
			prev->next = &analyzePlay.linesOfPlay[index];
		analyzePlay.linesOfPlay[index] = analyzePlay.tricks[d];	// copy trick into lines of play
		analyzePlay.linesOfPlay[index].next = NULL;				// clear pointers 
		analyzePlay.linesOfPlay[index].alt = NULL;
		prev = &analyzePlay.linesOfPlay[index];
		index = ++analyzePlay.linesOfPlayNum;
	}
	return start;												// return pointer to starting list of tricks
}


//
//	Insure that a card used to win a trick is not played later
//	For example if KS wins a trick and a line where the TS wins a trick 
//	and later the KS wins a trick, then these are separate lines of play
//
bool NotUsed(TRICK * lines, ANALYZEPLAY & analyzePlay, int d)
{
	int suit = lines->cards[lines->winner].suit;
	int spot = lines->cards[lines->winner].spot;
	int who = lines->winner;
	for (d++; d < analyzePlay.maxDepth; d++)
	{
		if (analyzePlay.tricks[d].cards[who].suit == suit
		&&  analyzePlay.tricks[d].cards[who].spot == spot)
			return false;
	}
	return true;
}


bool Find(TRICK * lines, int spot, int suit, int who)
{
	if (lines->next != NULL)
	{
		if (lines->next->cards[who].spot == spot
		&&  lines->next->cards[who].suit == suit)
			return true;
		if (Find(lines->next, spot, suit, who))
			return true;
	}
	if (lines->alt != NULL)
	{
		if (lines->alt->cards[who].spot == spot
		&&  lines->alt->cards[who].suit == suit)
			return true;
		if (Find(lines->alt, spot, suit, who))
			return true;
	}
	return false;
}


//
//	Insure that a card used to win a trick is not played later
//	For example if KS wins a trick in lone line and the TS wins a trick 
//	and later the KS wins a trick, then these are separate lines of play
//
bool NotUsed(TRICK * lines, TRICK * list)
{
	int who = lines->winner;
	if (lines->cards[who].spot == list->cards[who].spot)	// same spot is fine
		return true;
	int suit = lines->cards[who].suit;
	int spot = lines->cards[who].spot;
	if (Find(list, spot, suit, who))
		return false;
	who = list->winner;
	suit = list->cards[who].suit;
	spot = list->cards[who].spot;
	if (Find(lines, spot, suit, who))
		return false;
	return true;
}


//
// create a tree for the different lines of play 
//
bool UpdateLinesOfPlay(ANALYZEPLAY & analyzePlay)
{
	int d;

	TRICK * list = analyzePlay.linesOfPlay;
	analyzePlay.linesOfPlayCount++;
	if (analyzePlay.linesOfPlayNum == 0)						// add tricks to our list
	{
		// all the TRICK fields are set 
		return AddNewLineOfPlay(analyzePlay, 0) != NULL;		// create first list of tricks
	}
	for (d = 0; d < analyzePlay.maxDepth; d++)					// find existing line of play (tricks) if any
	{
#if SHOW_ALL_LINES_OF_PLAY		
		while (true)
#else
		while (list->leader != analyzePlay.tricks[d].leader
		|| list->winner != analyzePlay.tricks[d].winner
		|| list->cards[analyzePlay.tricks[d].leader].suit != analyzePlay.tricks[d].cards[analyzePlay.tricks[d].leader].suit
		|| list->cards[analyzePlay.tricks[d].winner].suit != analyzePlay.tricks[d].cards[analyzePlay.tricks[d].winner].suit
		|| NotUsed(list, analyzePlay, d) == false)				// our cards are not be used later in list
#endif
		{
			if (list->alt == NULL)
			{
				list->alt = AddNewLineOfPlay(analyzePlay, d);	// create alternate list of tricks
				return list->alt != NULL;						// return false if table is full
			}
			list = list->alt;
		}
		list = list->next;
	}
	return true;
}


TRICK * AddLineOfPlay(TRICK * lineOfPlay, bool addAlt)
{
	int index = allLinesOfPlayCount++;
	if (index == MAX_ANALYSIS_TRICKS)							// trick table is full
		return NULL;
	allLinesOfPlay[index] = *lineOfPlay;						// copy trick 
	if (lineOfPlay->next != NULL)								// add next too
		allLinesOfPlay[index].next = AddLineOfPlay(lineOfPlay->next, true);
	else
		allLinesOfPlay[index].next = NULL;
	if (addAlt == true
	&& lineOfPlay->alt != NULL)								// add alt too
		allLinesOfPlay[index].alt = AddLineOfPlay(lineOfPlay->alt, true);
	else
		allLinesOfPlay[index].alt = NULL;
	return &allLinesOfPlay[index];								// return pointer to starting list of tricks
}


// if the list does not match one of the top level items of linesOfPlay, add the whole sublist
// if it does match update the count and hand and recurse with the next and alt pointers
void UpdateLineOfPlay(TRICK * lineOfPlay, TRICK * newlist)
{
	if (allLinesOfPlayCount == 0)
	{
		AddLineOfPlay(newlist, true);							// add everything
		return;
	}
#if SHOW_ALL_LINES_OF_PLAY		
	while (lineOfPlay->alt != NULL)								// goto the end of alt list and add new entries
		lineOfPlay = lineOfPlay->alt;
	lineOfPlay->alt = AddLineOfPlay(newlist, true);				// add all the newlist
#else
	TRICK * lines = lineOfPlay;
	TRICK * list = newlist;
	while (list != NULL)										// go through all the new list
	{
		do
		{
			if (list->leader == lines->leader					// update entry
			&& list->winner == lines->winner
			&& list->cards[list->leader].suit == lines->cards[lines->leader].suit
			&& list->cards[list->winner].suit == lines->cards[lines->winner].suit
			&& NotUsed(list, lines))							// our winning cards in either line are not be used later in the other line
			{
				lines->dealMask |= ((uint64)1 << list->dealNum);	// update mask of deals that use this line
				if (lines->next != NULL)
					UpdateLineOfPlay(lines->next, list->next);	// OK update next trick in the line of play
				lines = lineOfPlay;								// restart the update
				list = list->alt;
				break;
			}
			else if (lines->alt == NULL)						// add the new list entry
			{
				lines->alt = AddLineOfPlay(list, false);		// add the current trick as new
				lines = lineOfPlay;								// restart the update
				list = list->alt;
				if (list == NULL)								// no more new entries
					break;
			}
			else
				lines = lines->alt;
		} while (lines != NULL);
	}
#endif
}


//
//	merge all lines of play from the analyzePlays[] into allLinesOfPlay[]
//
bool MergeLinesOfPlay()
{
	countSolveBoard = 0;
	countLinesOfPlay = 0;
	allLinesOfPlayCount = 0;
	for (int i = 0; i < analyzeHandCount; i++)					// go through all deals
	{
		if (dealsSuceed[i] == false)							// skip failed deals
			continue;
		countSolveBoard += analyzePlays[i].countSolveBoard;
		countLinesOfPlay += analyzePlays[i].linesOfPlayCount;
		// find line of play in current list if possible
		UpdateLineOfPlay(allLinesOfPlay, analyzePlays[i].linesOfPlay);
	}
	return true;
}


int CountBits(uint64 mask)
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


//
//	The algorithm is as follows:
//	1. start from the top and progress down to the lowest level
//	2. set count to the number of bits in dealMask
//	3. pass the highest count dealMask of all siblings up to the parent
//	we use max for our side because we pick the best chance for our moves
//	we use max for opponenets because some lines lead to more tricks for our side so are not shown
//	or the card is not available to lead which is OK too I believe
//	5. now sort all lines of play based on highest count
//	we return the best dealMask for this level
uint64 CountResults(TRICK * trick)
{
	uint64 mask = 0;
	int count = 0;
	do
	{
		if (trick->next != NULL)
			trick->dealMask = CountResults(trick->next);
		trick->count = CountBits(trick->dealMask);
		if (count < trick->count)
		{
			mask = trick->dealMask;
			count = trick->count;
		}
		trick = trick->alt;
	} while (trick != NULL);
	return mask;
}


void UpdateTrickCount()
{
	// first get correct count for all entries
	TRICK * trick = allLinesOfPlay;
	while (trick != NULL)
	{
		if (trick->next != NULL)
			trick->dealMask = CountResults(trick->next);
		trick->count = CountBits(trick->dealMask);
		trick = trick->alt;
	}
}


void SortResults(TRICK * start, TRICK * parent)
{
	TRICK * prev;
	TRICK * trick = start;
	if (trick->next != NULL)					// sort lower level first
	{
		do
		{
			SortResults(trick->next, trick);	// sort our children
			trick = trick->alt;
		} while (trick != NULL);
	}
	// now sort at our level
RESTART:
	trick = start;
	prev = NULL;
	while (trick->alt != NULL)
	{
		if (trick->count < trick->alt->count)
		{
			TRICK * alt = trick->alt;
			trick->alt = alt->alt;
			alt->alt = trick;
			if (prev != NULL)
				prev->alt = alt;
			if (parent->next == trick)
				parent->next = alt;
			if (trick == start)
				start = alt;
			goto RESTART;
		}
		prev = trick;
		trick = trick->alt;
	}
}


bool AnalyzeDeepPlay(HWND hWnd, ANALYZEPLAY & analyzePlay, int rank, int suit)
{
	futureTricks optimal;								// list of optimal cards to play 
	bool	ret = true;

	TRICK * trick = &analyzePlay.tricks[analyzePlay.trickNum];
	trick->cards[analyzePlay.handToPlay].spot = rank - 2;
	trick->cards[analyzePlay.handToPlay].suit = suit;
	analyzePlay.deal.remainCards[analyzePlay.handToPlay][suit] &= ~(1 << rank);
	if (analyzePlay.played < 3)
	{
		analyzePlay.deal.currentTrickRank[analyzePlay.played] = rank;
		analyzePlay.deal.currentTrickSuit[analyzePlay.played] = suit;
	}
	if (++analyzePlay.handToPlay == 4)
		analyzePlay.handToPlay = 0;
	if (++analyzePlay.played == 4)
	{
		bool trumped = false;
		int suitLead = trick->cards[analyzePlay.deal.first].suit;
		int suitLeadRank = -1;
		for (int i = 0; i < 4; i++)
		{
			if (trick->cards[i].suit == analyzePlay.deal.trump)
			{
				if (trumped == false
				|| trick->cards[i].spot > suitLeadRank)
				{
					trumped = true;
					suitLeadRank = trick->cards[i].spot;
					trick->winner = i;
				}
			}
			else if (trumped == false
			&& trick->cards[i].suit == suitLead
			&& trick->cards[i].spot > suitLeadRank)
			{
				suitLeadRank = trick->cards[i].spot;
				trick->winner = i;
			}
		}
		if (analyzePlay.side == (trick->winner & 1))
			analyzePlay.ourTricks++;
		else
			analyzePlay.oppTricks++;
		analyzePlay.trickNum++;
		analyzePlay.played = 0;
		analyzePlay.handToPlay = trick->winner;
		analyzePlay.deal.first = analyzePlay.handToPlay;
		analyzePlay.deal.currentTrickSuit[0] = 0;
		analyzePlay.deal.currentTrickSuit[1] = 0;
		analyzePlay.deal.currentTrickSuit[2] = 0;
		analyzePlay.deal.currentTrickRank[0] = 0;
		analyzePlay.deal.currentTrickRank[1] = 0;
		analyzePlay.deal.currentTrickRank[2] = 0;
		if (analyzePlay.trickNum < analyzePlay.maxDepth)	// do not write the next trick if we are stopping
		{
			analyzePlay.tricks[analyzePlay.trickNum].leader = analyzePlay.handToPlay;
			analyzePlay.tricks[analyzePlay.trickNum].dealMask = ((uint64)1 << analyzePlay.dealNum);
			analyzePlay.tricks[analyzePlay.trickNum].dealNum = analyzePlay.dealNum;
			analyzePlay.tricks[analyzePlay.trickNum].next = NULL;
			analyzePlay.tricks[analyzePlay.trickNum].alt = NULL;
		}
	}
	if (analyzePlay.trickNum == analyzePlay.maxDepth)		// stop analysis at the last trick
	{
		if (UpdateLinesOfPlay(analyzePlay) == false)		// we ran out of room
		{
			char string[60];
			sprintf(string, "Too many unique tricks (%d) during analysis using depth of %d", analyzeHandCount, analyzeDepth);
			MessageBox(hWnd, string, "Internal Error", MB_ICONSTOP | MB_OK | MB_APPLMODAL);
			ret = false;
		}
	}
	else
	{
		analyzePlay.countSolveBoard++;
		// any card that meets the target not just the best move for declaring side
		// this is just in case we have some cards that will exceed the target
		// and best move for defending side 
		int target;
		if ((analyzePlay.handToPlay & 1) == (playDeclarer & 1))	
			target = analyzePlay.target - analyzePlay.ourTricks;
		else
			target = -1;									// find best moves only
		int res = SolveBoard(analyzePlay.deal, target, 2, 1, &optimal, threadIndex, NULL);
		if (optimal.score[0] < target)						// in case no tricks can be won
			optimal.cards = 0;
		analyzePlay.nodes += optimal.nodes;
		if (res != RETURN_NO_FAULT)
		{
			char string[60];
			sprintf(string, "SolveBoard Failed! (%d)", res);
			MessageBox(hWnd, string, "Internal Error", MB_ICONSTOP | MB_OK | MB_APPLMODAL);
			return false;
		}
		for (int j = 0; j < optimal.cards; j++)
		{
			if (AnalyzeDeepPlay(hWnd, analyzePlay, optimal.rank[j], optimal.suit[j]) == false)
			{
				ret = false;
				break;
			}
		}
	}
	// restore deal before leaving
	if (analyzePlay.played == 0)
	{
		analyzePlay.trickNum--;
		analyzePlay.played = 3;
		analyzePlay.deal.first = analyzePlay.handToPlay = analyzePlay.tricks[analyzePlay.trickNum].leader;
		analyzePlay.deal.currentTrickSuit[0] = trick->cards[analyzePlay.handToPlay].suit;
		analyzePlay.deal.currentTrickRank[0] = trick->cards[analyzePlay.handToPlay].spot + 2;
		if (++analyzePlay.handToPlay == 4)
			analyzePlay.handToPlay = 0;
		analyzePlay.deal.currentTrickSuit[1] = trick->cards[analyzePlay.handToPlay].suit;
		analyzePlay.deal.currentTrickRank[1] = trick->cards[analyzePlay.handToPlay].spot + 2;
		if (++analyzePlay.handToPlay == 4)
			analyzePlay.handToPlay = 0;
		analyzePlay.deal.currentTrickSuit[2] = trick->cards[analyzePlay.handToPlay].suit;
		analyzePlay.deal.currentTrickRank[2] = trick->cards[analyzePlay.handToPlay].spot + 2;
		if (++analyzePlay.handToPlay == 4)
			analyzePlay.handToPlay = 0;
		if (analyzePlay.side == (trick->winner & 1))
			analyzePlay.ourTricks--;
		else
			analyzePlay.oppTricks--;
	}
	else
	{
		--analyzePlay.played;
		if (--analyzePlay.handToPlay < 0)
			analyzePlay.handToPlay = 3;
		analyzePlay.deal.currentTrickSuit[analyzePlay.played] = 0;
		analyzePlay.deal.currentTrickRank[analyzePlay.played] = 0;
	}
	analyzePlay.deal.remainCards[analyzePlay.handToPlay][suit] |= (1 << rank);
	return ret;
}


void DisplayCardsPlayed(FILE * file)						// show tricks and cards already played
{
	int trick = 0; 
	int leader = BackupLeader[0];
	int toPlay = leader;
	while (BackupPlayCount[leader] > trick)
	{
		fprintf(file, "// Trick %2d %c:", trick+1, LeaderString[leader]);
		for (int k = 0; k < 4; k++)
		{
			if (BackupPlayCount[toPlay] == trick)			// end of cards played
				break;
			if (k == 0)
				fprintf(file, "%c%c", SpotString[BackupPlaySpot[toPlay][trick]], SuitString[BackupPlaySuit[toPlay][trick]]);
			else
				fprintf(file, "-%c%c", SpotString[BackupPlaySpot[toPlay][trick]], SuitString[BackupPlaySuit[toPlay][trick]]);
			if (++toPlay == 4)								// wrap play around to next player
				toPlay = 0;
		}
		fprintf(file, "\n");
		if (BackupPlayCount[leader] == trick - 1)			// there would be no leader set for the next trick
			break;
		toPlay = leader = BackupLeader[++trick];
	}
	fprintf(file, "// %s to play\n", DeclarerString[toPlay]);
}


bool SaveSolutions(HWND hWnd, char * fname, int maxDepth)
{
	FILE * file;

	file = fopen(fname, "w");
	if (file != NULL)
	{
		char hands[4][4][SPOTS + 3];
		for (int h = 0; h < 4; h++)
		{
			for (int s = 0; s < 4; s++)
			{
				sprintf(hands[h][s], "%c ", SuitString[s]);
				int spots = dl.remainCards[h][s];
				int count = 2;						// skip over suit declaration
				for (int i = SPOT_ACE, mask = (1 << (SPOT_ACE + 2)); mask >= 4; i--, mask >>= 1)
				{
					if ((spots & mask) != 0)
						hands[h][s][count++] = SpotString[i];
				}
				for (; count < SPOTS + 2; count++)
					hands[h][s][count] = ' ';
				hands[h][s][SPOTS] = 0;
			}
		}
		fprintf(file, "//         %s\n", hands[DIR_NORTH][SUIT_SPADES]);
		fprintf(file, "//         %s\n", hands[DIR_NORTH][SUIT_HEARTS]);
		fprintf(file, "//         %s\n", hands[DIR_NORTH][SUIT_DIAMONDS]);
		fprintf(file, "//         %s\n", hands[DIR_NORTH][SUIT_CLUBS]);
		fprintf(file, "// %s     %s\n", hands[DIR_WEST][SUIT_SPADES], hands[DIR_EAST][SUIT_SPADES]);
		fprintf(file, "// %s     %s\n", hands[DIR_WEST][SUIT_HEARTS], hands[DIR_EAST][SUIT_HEARTS]);
		fprintf(file, "// %s     %s\n", hands[DIR_WEST][SUIT_DIAMONDS], hands[DIR_EAST][SUIT_DIAMONDS]);
		fprintf(file, "// %s     %s\n", hands[DIR_WEST][SUIT_CLUBS], hands[DIR_EAST][SUIT_CLUBS]);
		fprintf(file, "//         %s\n", hands[DIR_SOUTH][SUIT_SPADES]);
		fprintf(file, "//         %s\n", hands[DIR_SOUTH][SUIT_HEARTS]);
		fprintf(file, "//         %s\n", hands[DIR_SOUTH][SUIT_DIAMONDS]);
		fprintf(file, "//         %s\n", hands[DIR_SOUTH][SUIT_CLUBS]);
		fprintf(file, "// Contract: %d%s-%s\n", playContract, ContractString[playSuit], DeclarerString[playDeclarer]);
		fprintf(file, "// NS Won: %d\n", NSWon);
		fprintf(file, "// EW Won: %d\n", EWWon);
		DisplayCardsPlayed(file);							// show tricks and cards already played

		// print deal struct so it can be used as input to DisplaySolution in DDS library examples
		fprintf(file, "{\n\t%d,\t\t// trump\n", dl.trump);
		fprintf(file, "\t%d,\t\t// first\n", dl.first);
		fprintf(file, "\t{ %d, %d, %d },\t// currentTrickSuit\n",
			dl.currentTrickSuit[0], dl.currentTrickSuit[1], dl.currentTrickSuit[2]);
		fprintf(file, "\t{ %d, %d, %d },\t// currentTrickRank\n",
			dl.currentTrickRank[0], dl.currentTrickRank[1], dl.currentTrickRank[2]);
		fprintf(file, "\t{\t\t// remainCards\n");
		fprintf(file, "\t\t{ 0x%04x, 0x%04x, 0x%04x, 0x%04x },\n",
			dl.remainCards[0][0], dl.remainCards[0][1], dl.remainCards[0][2], dl.remainCards[0][3]);
		fprintf(file, "\t\t{ 0x%04x, 0x%04x, 0x%04x, 0x%04x },\n",
			dl.remainCards[1][0], dl.remainCards[1][1], dl.remainCards[1][2], dl.remainCards[1][3]);
		fprintf(file, "\t\t{ 0x%04x, 0x%04x, 0x%04x, 0x%04x },\n",
			dl.remainCards[2][0], dl.remainCards[2][1], dl.remainCards[2][2], dl.remainCards[2][3]);
		fprintf(file, "\t\t{ 0x%04x, 0x%04x, 0x%04x, 0x%04x },\n",
			dl.remainCards[3][0], dl.remainCards[3][1], dl.remainCards[3][2], dl.remainCards[3][3]);
		fprintf(file, "\t},\n};\n");

		TimeStart();										// start timer
		optimals[0] = optimal;								// save results from original SolveBoard with solutions = 3 
		analyzePlays[0].nodes = optimal.nodes;
		// record any previous cards played
		int trick = NSWon + EWWon;
		if (maxDepth > 13 - trick)							// do not go beyond the last trick
			maxDepth = 13 - trick;
		if (maxDepth > analyzeDepth)						// keep max depth within bounds
			maxDepth = analyzeDepth;
		else if (maxDepth < 1)
			maxDepth = 1;
		analyzePlays[0].handToPlay = dl.first;
		analyzePlays[0].played = 0;
		if ((playDeclarer & 1) == 0)
		{
			analyzePlays[0].ourTricks = NSWon;
			analyzePlays[0].oppTricks = EWWon;
			analyzePlays[0].side = 0;
		}
		else
		{
			analyzePlays[0].ourTricks = EWWon;
			analyzePlays[0].oppTricks = NSWon;
			analyzePlays[0].side = 1;
		}
		analyzePlays[0].target = playContract + 6;
		analyzePlays[0].dealNum = 0;
		analyzePlays[0].maxDepth = maxDepth;
		analyzePlays[0].trickNum = 0;
		analyzePlays[0].linesOfPlayNum = 0;
		analyzePlays[0].linesOfPlayCount = 0;
		analyzePlays[0].countSolveBoard = 1;
		analyzePlays[0].deal = dl;	
		analyzePlays[0].tricks[0].leader = analyzePlays[0].handToPlay;
		analyzePlays[0].tricks[0].dealNum = 0;
		analyzePlays[0].tricks[0].dealMask = 1;
		analyzePlays[0].tricks[0].next = NULL;
		analyzePlays[0].tricks[0].alt = NULL;
		for (int k = 0; k < 3; k++)
		{
			if (dl.currentTrickRank[k] != 0)
			{
				analyzePlays[0].tricks[0].cards[analyzePlays[0].handToPlay].spot = dl.currentTrickRank[k] - 2;
				analyzePlays[0].tricks[0].cards[analyzePlays[0].handToPlay].suit = dl.currentTrickSuit[k];
				analyzePlays[0].played++;
				if (++analyzePlays[0].handToPlay == 4)
					analyzePlays[0].handToPlay = 0;
			}
		}
		ProgressBarStart();									// start progress bar
		for (int j = 0; j < optimals[0].cards; j++)			// set end of cards when we fail to meet target
		{
			if (optimals[0].score[j] < analyzePlays[0].target - analyzePlays[0].ourTricks)
			{
				optimals[0].cards = j;
				break;
			}
		}
		for (int j = 0; j < optimals[0].cards; j++)
		{
			ProgressBarSet(100 * j / optimals[0].cards);	// show progress
			if (AnalyzeDeepPlay(hWnd, analyzePlays[0], optimal.rank[j], optimal.suit[j]) == false)
			{
				fclose(file);
				ProgressBarStop();							// stop progress bar
				return false;
			}
		}
		ProgressBarStop();									// stop progress bar
		PrintSolutions(file, &analyzePlays[0].linesOfPlay[0], 0);
		fprintf(file, "\ntime=%.3f\n", TimeEnd());
		fprintf(file, "SolveBoard calls=%d nodes=%d\n", analyzePlays[0].countSolveBoard, analyzePlays[0].nodes);
		fprintf(file, "Process %d lines of play (%d entries used)\n", analyzePlays[0].linesOfPlayCount, analyzePlays[0].linesOfPlayNum);
		fclose(file);
		return true;
	}
	return false;
}


bool SaveAnalysis(HWND hWnd, char * fname)
{
	FILE * file;
	int nodes = 0;

	file = fopen(fname, "w");
	if (file != NULL)
	{
		char hands[4][4][SPOTS + 3];
		for (int i = 0; i < analyzeHandCount; i++)
		{
			nodes += analyzePlays[i].nodes;
			if (dealsSuceed[i] == false)
				continue;
			for (int h = 0; h < 4; h++)
			{
				for (int s = 0; s < 4; s++)
				{
					sprintf(hands[h][s], "%c ", SuitString[s]);
					int spots = deals[i].remainCards[h][s];
					int count = 2;						// skip over suit declaration
					for (int i = SPOT_ACE, mask = (1 << (SPOT_ACE + 2)); mask >= 4; i--, mask >>= 1)
					{
						if ((spots & mask) != 0)
							hands[h][s][count++] = SpotString[i];
					}
					for (; count < SPOTS + 2; count++)
						hands[h][s][count] = ' ';
					hands[h][s][SPOTS] = 0;
				}
			}
			fprintf(file, "// Deal %d\n", i + 1);
			fprintf(file, "//         %s\n", hands[DIR_NORTH][SUIT_SPADES]);
			fprintf(file, "//         %s\n", hands[DIR_NORTH][SUIT_HEARTS]);
			fprintf(file, "//         %s\n", hands[DIR_NORTH][SUIT_DIAMONDS]);
			fprintf(file, "//         %s\n", hands[DIR_NORTH][SUIT_CLUBS]);
			fprintf(file, "// %s     %s\n", hands[DIR_WEST][SUIT_SPADES], hands[DIR_EAST][SUIT_SPADES]);
			fprintf(file, "// %s     %s\n", hands[DIR_WEST][SUIT_HEARTS], hands[DIR_EAST][SUIT_HEARTS]);
			fprintf(file, "// %s     %s\n", hands[DIR_WEST][SUIT_DIAMONDS], hands[DIR_EAST][SUIT_DIAMONDS]);
			fprintf(file, "// %s     %s\n", hands[DIR_WEST][SUIT_CLUBS], hands[DIR_EAST][SUIT_CLUBS]);
			fprintf(file, "//         %s\n", hands[DIR_SOUTH][SUIT_SPADES]);
			fprintf(file, "//         %s\n", hands[DIR_SOUTH][SUIT_HEARTS]);
			fprintf(file, "//         %s\n", hands[DIR_SOUTH][SUIT_DIAMONDS]);
			fprintf(file, "//         %s\n", hands[DIR_SOUTH][SUIT_CLUBS]);

			// print deal struct so it can be used as input to DisplaySolution in DDS library examples
			fprintf(file, "{\n\t%d,\t\t// trump\n", deals[i].trump);
			fprintf(file, "\t%d,\t\t// first\n", deals[i].first);
			fprintf(file, "\t{ %d, %d, %d },\t// currentTrickSuit\n",
				deals[i].currentTrickSuit[0], deals[i].currentTrickSuit[1], deals[i].currentTrickSuit[2]);
			fprintf(file, "\t{ %d, %d, %d },\t// currentTrickRank\n",
				deals[i].currentTrickRank[0], deals[i].currentTrickRank[1], deals[i].currentTrickRank[2]);
			fprintf(file, "\t{\t\t// remainCards\n");
			fprintf(file, "\t\t{ 0x%04x, 0x%04x, 0x%04x, 0x%04x },\n",
				deals[i].remainCards[0][0], deals[i].remainCards[0][1], deals[i].remainCards[0][2], deals[i].remainCards[0][3]);
			fprintf(file, "\t\t{ 0x%04x, 0x%04x, 0x%04x, 0x%04x },\n",
				deals[i].remainCards[1][0], deals[i].remainCards[1][1], deals[i].remainCards[1][2], deals[i].remainCards[1][3]);
			fprintf(file, "\t\t{ 0x%04x, 0x%04x, 0x%04x, 0x%04x },\n",
				deals[i].remainCards[2][0], deals[i].remainCards[2][1], deals[i].remainCards[2][2], deals[i].remainCards[2][3]);
			fprintf(file, "\t\t{ 0x%04x, 0x%04x, 0x%04x, 0x%04x },\n",
				deals[i].remainCards[3][0], deals[i].remainCards[3][1], deals[i].remainCards[3][2], deals[i].remainCards[3][3]);
			fprintf(file, "\t},\n};\n");
			fprintf(file, "// ------------------------------\n");
		}
		fprintf(file, "// Contract: %d%s-%s\n", playContract, ContractString[playSuit], DeclarerString[playDeclarer]);
		fprintf(file, "// NS Won: %d\n", NSWon);
		fprintf(file, "// EW Won: %d\n", EWWon);
		DisplayCardsPlayed(file);							// display cards already played
		PrintSolutions(file, allLinesOfPlayStart.next, analyzeHandCount - analyzeFail);
		fprintf(file, "\ntime=%.3f\n", analyzeTime);
		fprintf(file, "Deals=%d (%d failed)\n", analyzeHandCount, analyzeFail);
		fprintf(file, "SolveBoard calls=%d nodes=%d\n", countSolveBoard, nodes);
		fprintf(file, "Process %d lines of play (%d entries used)\n", countLinesOfPlay, allLinesOfPlayCount);
		fclose(file);
		return true;
	}
	return false;
}


bool AnalyzePlay(HWND hWnd, int maxDepth)
{
	bool ret = true;

	TimeStart();										// start timer
	good = -1;
	bad = -1;
	analyzeFail = 0;
	analyzeCount = 0;
	int trick = NSWon + EWWon;
	BackupPlayTrick = trick;							// back up to this point for other winning hands
	BackupPlayPlays = plays;
	if (maxDepth > 13 - trick)							// do not go beyond the last trick
		maxDepth = 13 - trick;
	if (maxDepth > analyzeDepth)						// keep max depth within bounds
		maxDepth = analyzeDepth;
	else if (maxDepth < 1)
		maxDepth = 1;
	ProgressBarStart();									// start progress bar
	int target;
	if ((playDeclarer & 1) == 0)
		target = playContract + 6 - NSWon;
	else
		target = playContract + 6 - EWWon;
	for (int i = 0; i < analyzeHandCount && ret == true; i++)
	{
		deals[i] = dl;
		if (i != 0)
		{
			ret = NewDeal(hWnd, true, playDeclarer, playDeclarer ^ 2, &deals[i]);
			if (ret == false)								// restrictions prevent us from finding any suitable deals
			{
				good = -1;
				bad = -1;
				goto EXIT;
			}
			int res = SolveBoard(deals[i], target, 3, 1, &optimals[i], threadIndex, NULL);
			analyzePlays[i].nodes = optimals[i].nodes;
			if (res != RETURN_NO_FAULT)
			{
				char string[60];
				sprintf(string, "SolveBoard Failed! (%d)", res);
				MessageBox(hWnd, string, "Internal Error", MB_ICONSTOP | MB_OK | MB_APPLMODAL);
			}
		}
		else
		{
			optimals[i] = optimal;
			analyzePlays[i].nodes = optimal.nodes;
		}
		for (int j = 0; j < optimals[i].cards; j++)		// ignore cards that do not meet target
		{
			if (optimals[i].score[j] < target)
			{
				optimals[i].cards = j;
				break;
			}
		}
		if (optimals[i].cards == 0)
		{
			dealsSuceed[i] = false;
			analyzeFail++;
			if (bad < 0)
				bad = i;
		}
		else
		{
			if (i != 0 && good < 0)
				good = i;
			dealsSuceed[i] = true;
			// record any previous cards played
			analyzePlays[i].handToPlay = dl.first;
			analyzePlays[i].played = 0;
			if ((playDeclarer & 1) == 0)
			{
				analyzePlays[i].ourTricks = NSWon;
				analyzePlays[i].oppTricks = EWWon;
				analyzePlays[i].side = 0;
			}
			else
			{
				analyzePlays[i].ourTricks = EWWon;
				analyzePlays[i].oppTricks = NSWon;
				analyzePlays[i].side = 1;
			}
			analyzePlays[i].target = playContract + 6;
			analyzePlays[i].dealNum = i;
			analyzePlays[i].maxDepth = maxDepth;
			analyzePlays[i].trickNum = 0;
			analyzePlays[i].linesOfPlayNum = 0;
			analyzePlays[i].linesOfPlayCount = 0;
			analyzePlays[i].countSolveBoard = 1;
			analyzePlays[i].deal = deals[i];
			analyzePlays[i].tricks[0].leader = analyzePlays[i].handToPlay;
			analyzePlays[i].tricks[0].dealNum = i;
			analyzePlays[i].tricks[0].dealMask = ((uint64)1 << i);
			analyzePlays[i].tricks[0].next = NULL;
			analyzePlays[i].tricks[0].alt = NULL;
			for (int k = 0; k < 3; k++)
			{
				if (deals[0].currentTrickRank[k] != 0)
				{
					analyzePlays[i].tricks[0].cards[analyzePlays[i].handToPlay].spot = dl.currentTrickRank[k] - 2;
					analyzePlays[i].tricks[0].cards[analyzePlays[i].handToPlay].suit = dl.currentTrickSuit[k];
					analyzePlays[i].played++;
					if (++analyzePlays[i].handToPlay == 4)
						analyzePlays[i].handToPlay = 0;
				}
			}
			// find lead in current list of AnalyzeCards
			for (int j = 0; j < optimals[i].cards; j++)
			{
				int percentage = 100 * ((i * optimals[i].cards) + j) / (analyzeHandCount * optimals[i].cards);
				ProgressBarSet(percentage);					// set progress
				if (AnalyzeDeepPlay(hWnd, analyzePlays[i], optimals[i].rank[j], optimals[i].suit[j]) == false)
				{
					ret = false;
					goto EXIT;
				}
			}
		}
	}
	analyzeHandCountSaved = analyzeHandCount;
	ProgressBarSet(100);									// set progress
	// merge all lines of play and then sort them
	if (MergeLinesOfPlay() == false)						// merge all lines of play
	{
		char string[60];
		sprintf(string, "Too many unique tricks (%d) during analysis using depth of %d", analyzeHandCount, analyzeDepth);
		MessageBox(hWnd, string, "Internal Error", MB_ICONSTOP | MB_OK | MB_APPLMODAL);
		ret = false;
	}
//	FILE * file;
//	file = fopen("merge.txt", "a");
//	PrintSolutions(file, allLinesOfPlay, 2);
//	fprintf(file, "-----------------------------------------\n");
	UpdateTrickCount();										// get count for each line of play and sort them from most likely to suceed
//	PrintSolutions(file, allLinesOfPlay, 2);
//	fprintf(file, "-----------------------------------------\n");	
	allLinesOfPlayStart.next = allLinesOfPlay;
	SortResults(allLinesOfPlay, &allLinesOfPlayStart);		// now sort by the count for each level
//	PrintSolutions(file, allLinesOfPlay, 2);
//	fclose(file);
EXIT:
	ProgressBarStop();										// stop progress bar
	analyzeTime = TimeEnd();
	HMENU hMenu = GetMenu(hWnd);							// turn on some menu items
	if (good >= 0)
		EnableMenuItem(hMenu, IDM_ANALYZEPICKWIN, MF_BYCOMMAND | MF_ENABLED);
	else
		EnableMenuItem(hMenu, IDM_ANALYZEPICKWIN, MF_BYCOMMAND | MF_GRAYED);
	if (bad >= 0)
		EnableMenuItem(hMenu, IDM_ANALYZEPICKLOSS, MF_BYCOMMAND | MF_ENABLED);
	else
		EnableMenuItem(hMenu, IDM_ANALYZEPICKLOSS, MF_BYCOMMAND | MF_GRAYED);
	return ret;
}


void AnalyzeAnotherEWHand(HWND hWnd)
{
	dl = deals[good];
	BackupPlayDeal = good;
	optimal = optimals[good];
	do
	{
		if (++good >= analyzeHandCountSaved)
			good = 0;
	} while (dealsSuceed[good] == false);
}


void AnalyzeFailedEWHand(HWND hWnd)
{
	dl = deals[bad];
	BackupPlayDeal = bad;
	optimal = optimals[bad];
	do
	{
		if (++bad >= analyzeHandCountSaved)
			bad = 0;
	} while (dealsSuceed[bad]);
}


void GetDealsFailed(char * string, int len)
{
	sprintf(string, "%.1f%%", 100.0 * analyzeFail / analyzeHandCount);
}


int GetLinesOfPlay()
{
	int count = 0;
	TRICK * lines = allLinesOfPlayStart.next;
	while (lines != NULL)
	{
		count++;
		lines = lines->alt;
	}
	return count;
}


void GetLinesString(char * string, int len, int index)
{
	string[0] = 0;
	TRICK * lines = allLinesOfPlayStart.next;
	while (index != 0)								// find the next line of play
	{
		index--;
		lines = lines->alt;
		if (lines == NULL)							// end of list
			return;
	}
	int hand = lines->leader + analyzePlays[0].played;	// get hand that must card
	if (hand >= 4)
		hand -= 4;
	sprintf(string, "%c%c",
		SpotString[lines->cards[hand].spot], SuitString[lines->cards[hand].suit]);
}


void GetLinesPercentage(char * string, int len, int index)
{
	string[0] = 0;
	TRICK * lines = allLinesOfPlayStart.next;
	while (index != 0)								// find the next line of play
	{
		index--;
		lines = lines->alt;
		if (lines == NULL)							// end of list
			return;
	}
	int success = analyzeHandCount - analyzeFail;
	sprintf(string, "%.1f%%", 100.0 * lines->count / success);
}
