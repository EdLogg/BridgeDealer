// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"
#include "resource.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <commdlg.h>					// required for load and save files
#include <shlobj.h>						// to use SHGetKnownFolderPath
#include <crtdbg.h>						// to check for memory leaks
#include <setjmp.h>						// used by PDF routines

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>						// for sprintf
#include <time.h>						// for time functions
#include <math.h>						// for sqrt

#include "hpdf.h"						// PDF definitions
#include "dll.h"						// double dummy
#include "rand.h"						// my random number generator

typedef	signed char			int8;
typedef	signed short		int16;
typedef	signed int			int32;
typedef	unsigned char		uint8;
typedef	unsigned short		uint16;
typedef	unsigned int		uint32;
#if defined(WIN32)
typedef signed __int64		int64;
typedef unsigned __int64	uint64;
#else
typedef signed long long	int64;
typedef unsigned long long	uint64;
#endif


#define	SAVE_FILE_VERSION		3		// change this when the format of the preference file is changed
#define	RESTRICT_FILE_VERSION	2		// change this when the format of the restriction file is changed
#define	MAX_HAND_RECORD_COUNT	72		// max number of hands for hand records
#define	AVERAGE_HAND_COUNT		20		// number of deals to create average hand results
#define	MAX_HAND_COUNT			100		// max number of deals to create average hand results
#define ANALYZE_HAND_COUNT		20		// max count of deals for analyze play
#define	MAX_ANALYZE_DEPTH		6		// max depth for analyze play
#define MAX_ANALYSIS_TRICKS		10000	// max number of tricks for hand analysis
#define SHOW_ALL_LINES_OF_PLAY	0		// set to 1 to show all lines of play (debug only)


enum DIRECTION
{
	DIR_CANCEL = -2,					// used to cancel a change or lock direction
	DIR_SYSTEMIC = -1,					// used to pick dealer based on hand number
	DIR_NORTH = 0,
	DIR_EAST,
	DIR_SOUTH,
	DIR_WEST,
	DIRECTIONS,
};

enum VULNERABLE
{
	VUL_CANCEL = -2,					// used to cancel a change or lock vul
	VUL_SYSTEMIC = -1,					// used to pick vlnerability based on deal number
	VUL_NONE = 0,
	VUL_NS,
	VUL_EW,
	VUL_BOTH,
};

enum SUITS
{
	SUIT_SPADES = 0,
	SUIT_HEARTS,
	SUIT_DIAMONDS,
	SUIT_CLUBS,
	SUITS,
	SUIT_NOTRUMP = 4,					// same as Double Dummy Solver (DDS_NOTRUMP see dds.h)
};

enum CONTRACTS
{
	CONTRACT_CLUBS = 0,
	CONTRACT_DIAMONDS,
	CONTRACT_HEARTS,
	CONTRACT_SPADES,
	CONTRACT_NOTRUMP,
	CONTRACTS,
};

enum SPOTS
{
	SPOT_NONE = -1,
	SPOT_TWO,
	SPOT_THREE,
	SPOT_FOUR,
	SPOT_FIVE,
	SPOT_SIX,
	SPOT_SEVEN,
	SPOT_EIGHT,
	SPOT_NINE,
	SPOT_TEN,
	SPOT_JACK,
	SPOT_QUEEN,
	SPOT_KING,
	SPOT_ACE,
	SPOTS,
};

struct CARD
{
	int8	suit;				// see enum above
	int8	spot;				// see enum above
};

struct PART
{
	int8	count;
	int8	spots[13];
};

struct HAND
{
	PART	suits[4];
};

struct TRICK
{
	int8	leader;				// direction of leader
	int8	winner;				// direction of winner
	int8	count;				// count of dealMask when we have merged all lines of play
	int8	dealNum;			// deal number for the initial line of play
	uint64	dealMask;			// mask of deals that use this trick
	CARD	cards[4];			// cards played index by direction
//	int		equals[4][4];		// equal cards for opponents ([dir][suit])
	TRICK *	next;				// next trick if any (0 if none)
	TRICK *	alt;				// alternate trick (line of play) if any (0 if none)
};


struct ANALYZEPLAY
{
	int		handToPlay;			// next player to play
	int		played;				// number of cards played to the current trick
	int		ourTricks;			// declarer tricks taken
	int		oppTricks;			// opponents tricks taken
	int		side;				// which declares the contract (0 for NS and 1 for EW)
	int		target;				// target tricks to take
	int		dealNum;			// which deal are we analyzing (see deals[dealNum])
	int		maxDepth;			// depth we are to analyze (1 <= x <= MAX_ANALYZE_DEPTH)
	int		trickNum;			// number of tricks completed so far (index in tricks[])
	int		linesOfPlayNum;		// number of entries in linesOfPlay[] used (next free entry in linesOfPlay[] too)
	int		linesOfPlayCount;	// number of lines of play analyzed counting duplicates and equals
	int		countSolveBoard;	// number of calls to SolveBoard()
	int		nodes;				// nodes processed
	struct deal deal;			// current deal
	TRICK	tricks[13];			// tricks currently being built (tricks[trickNum])
	TRICK	linesOfPlay[MAX_ANALYSIS_TRICKS];	// lines of play saved so far
};


struct SELECT
{
	int		hand;				// which hand it came from (-1 if none)
	int		suit;				// card being moved		
	int		spot;
	int		x, y;				// current screen position of the cards we are moving
	int		xOffset, yOffset;	// offset used to draw card
};


enum RESTRICT_DIRECTION
{
	RESTRICT_SOUTH = 0,
	RESTRICT_WEST = 4,
	RESTRICT_NORTH = 5,
	RESTRICT_EAST = 6,
};


enum RESTRICT_MASK
{
	RESTRICT_MASK_SOUTH = 0x0f,
	RESTRICT_MASK_WEST = 0x10,
	RESTRICT_MASK_NORTH = 0x20,
	RESTRICT_MASK_EAST = 0x40,
};

struct RESTRICT
{
	DIRECTION dir;										// NSEW direction
	bool	exactShape;									// to use the suits as is
	bool	HCP;										// if the point restriction is HCP vs total points
	bool	not;										// if this is a not restriction (vs or restriction)
	bool	concentrated;								// if kRatio is to be used
	int		minCards[4];								// min cards in a suit
	int		maxCards[4];								// max cards in a suit
	int		minPoints;									// point limitations 
	int		maxPoints;
	int		minQTricks;									// quick trick limitations 
	int		maxQTricks;
	int		minControls;								// controls limitations 
	int		maxControls;
	int		minLosers;									// losers limitations 
	int		maxLosers;
	int		kRatio;										// percentage for KRatio if HCP concentrated
};

#define MAX_SOUTH_RESTRICT	4							// 4 for south
#define MAX_RESTRICT	7								// 4 for south and 1 for each NEW

struct RESTRICTION
{
	int			mask;									// mask of which restrictions are enabled 
	DIRECTION	dealer;
	VULNERABLE	vul;
	RESTRICT	restrict[MAX_RESTRICT];
	int			lockedCount[4];							// count of locked cards in each hand
	int			lockedCards[4][4]; 						// mask of locked cards [hand][suit]
};


// game.cpp functions
extern HAND	hands[4];									// hands from N, E, S, W
extern char *	DealerString[DIRECTIONS];
extern char *	VulPBNString[4];
extern char *	ContractString[CONTRACTS];
extern char *	DeclarerString[DIRECTIONS];
extern char		LeaderString[DIRECTIONS];
extern char *	SuitString;
extern char *	SpotString;
extern char *	VulString[16];
extern RECT MiddleRect;
bool DoneSavingHands();
void SaveEWHandInfo();
void RestoreEWHandInfo();
void GetBestContract(int & bid, int & suit, int & declarer);
void DeckInit(void);
void GameInit(void);
int GetDeal(void);
int GetDealer(void);
char GetDealerPBNChar(void);
void SetDealer(int d);
int GetVul(void);
char * GetVulPBNString(void);
void SetVul(HWND hWnd, int v, bool create = true);
void SwapHands(int h1, int h2);
void SwapSuits(int s1, int s2);
void CreateContractList(HWND hWnd);
void UpdateContractResults(int(&results)[MAX_HAND_COUNT][CONTRACTS][DIRECTIONS], int & count);
void ResetDeal(HWND hWnd);
void CountHands();
bool NewDeal(HWND hWnd, bool redeal = false, int keep1 = -1, int keep2 = -1, deal * dl = NULL);
void PrintHand(HWND hWnd);
void WriteHands(HWND hWnd, char * fname);
void DrawCards(HWND hWnd, HDC hdc, PAINTSTRUCT ps);
bool Select(HWND hWnd, int x, int y);
bool SelectRelease(HWND hWnd, int x, int y);
void SelectMove(HWND hWnd, int x, int y);

// fileIO.cpp functions
bool LoadHand(HWND hWnd, char * fname);
bool SaveHand(HWND hWnd, char * fname);
bool LoadGameFile(HWND hWnd);
bool SaveGameFile(HWND hWnd);
bool LoadRestrictionFile(HWND hWnd, char * fname);
bool SaveRestrictionFile(HWND hWnd, char * fname);
bool SaveFileName(HWND hwnd, char * fname, char * filterString, char * defaultExt);
bool LoadFileName(HWND hwnd, char * fname, char * filterString, char * defaultExt);

// bridgeSheet.c definitions
#define	TITLE_LEN	33
#define HEADER_LEN	40
extern	char	titleString[TITLE_LEN + 1];
extern	char	headerString[HEADER_LEN + 1];
extern	char	dateString[HEADER_LEN + 1];
extern	int		handCount;
extern	char	title2String[TITLE_LEN + 1];
extern	char	header2String[HEADER_LEN + 1];
extern	char	date2String[HEADER_LEN + 1];
extern	int		handsPrinted;
void InitGameFileData(void);
bool PDFStart(HWND hWnd);
void PDFPrintHand(HWND hWnd, int handNum, int dealer, int vulnerable, HAND * hands, int * points, char * contracts1, char * contracts2, char * par);
void PDFEnd(HWND hWnd, char * fname, bool abort = false);

// PlayHand.cpp definitions
extern struct deal dl;								// current deal state for double dummy solver and display
extern futureTricks optimal;						// list of optimal cards to play 
extern int analyzeHandCountSaved;					// number of hands to analyze saved so we can look at next failed or good hand
extern int analyzeHandCount;						// number of hands to analyze
extern int analyzeDepth;							// depth of analyze play
extern int analyzeHandCountIndex;					// index for number of hands
extern int analyzeDepthIndex;						// index for depth of analyze play
extern int analyzeHandCounts[IDM_ANALYZESAMPLES];
extern int analyzeDepths[IDM_ANALYZEDEPTHS];
extern int handToPlay;
extern int playedRank[4];							// cards played for each hand position (rank = SPOT_NONE if none)
extern int playedSuit[4];
extern int BackupPlaySpot[4][13];					// used to back up to the previous trick
extern int BackupPlaySuit[4][13];					// also used when analyzing hands when we redeal EW hands
extern int BackupPlayCount[4];						// count of entries above
extern int analyzeFail, analyzeCount;
void PlayHandInit(HWND hWnd);
void PlayHandBack(HWND hWnd, bool restart = false);
void PlayHandSelect();
bool PlayHandLegal(int hand, int suit);
void PlayHandCard(HWND hWnd, int suit, int rank);
bool SaveAnalysis(HWND hWnd, char * fname);
bool SaveSolutions(HWND hWnd, char * fname, int maxDepth);
bool AnalyzePlay(HWND hWnd, int maxDepth);
void AnalyzeAnotherEWHand(HWND hWnd);
void AnalyzeFailedEWHand(HWND hWnd);
void GetDealsFailed(char * string, int len);
int  GetLinesOfPlay();
void GetLinesString(char * string, int len, int index);
void GetLinesPercentage(char * string, int len, int index);

// restrict.cpp definitions
extern int		restrictPage;						// current index into restriction.restrict[]
extern int		restrictPageStart;					// starting index for this direction
extern int		restrictMaxPages;					// max number of pages for this direction
INT_PTR CALLBACK Restrictions(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

// BridgeDealer definitions
extern HINSTANCE	hInst;							// current instance
extern int		coreCount;
extern int		processorCount;
extern int		L1CacheCount, L2CacheCount, L3CacheCount;
extern HWND		hProgressWindow;					// progress bar window
extern HWND		hProgressBar;						// progress bar 
extern SELECT	selected;
extern RESTRICTION restriction;						// list of restrictions
extern bool		writeHands;							// write hands when we are done
extern bool		savingHands;						// we are saving these hands
extern HBITMAP	hCards;								// handle to card image
extern HBITMAP	hCardsOff;
extern HBITMAP	hCardMark;
extern HBITMAP	hCardPlus;
extern HBITMAP	hCardMinus;
extern HBITMAP	hCardLock;
extern HWND		hDoneButton;						// for changing cards
extern bool		hideHands[4];
extern bool		hideHandCounts;
extern bool		hideContracts;
extern bool		playHand;
extern bool		lockCards;
extern int		playContract, playSuit, playDeclarer;
extern int		NSWon, EWWon;
extern bool		addComments;
void			TimeStart();
double			TimeEnd();
void			ProgressBarStart();
void			ProgressBarSet(int percentage);
void			ProgressBarStop();
extern void		ClearLockedCards();
extern void		ClearRestrictions();

