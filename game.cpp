#include "stdafx.h"

#define RETRIES			100000					// number of shuffles before we give up
#define	WIDTH			72						// size of the cards
#define	HEIGHT			96
#define XOVERLAP		16
#define YOVERLAP		48
#define	CONTRACT_STRING	26


CARD	deck[52];								// deck of shuffled cards
HAND	hands[4];								// hands from N, E, S, W
HAND	savedHands[MAX_HAND_RECORD_COUNT][4];	// save hands for printing later
int		savedVul[MAX_HAND_RECORD_COUNT];		// save vulnerability for printing later
int		savedDealer[MAX_HAND_RECORD_COUNT];		// save dealer for printing later
int		hcPoints[4];							// points for each hand
int		totalPoints[4];							// total points for each hand
int		concentrated[4];						// percentage concentrated for each hand
int		controls[4];							// controls for each hand
int		losers[4];								// losers for each hand
int		quickTricks[4];							// quick tricks for each hand
HAND	handsSaved[2];							
int		hcPointsSaved[2];
int		totalPointsSaved[2];
int		concentratedSaved[2];					
int		controlsSaved[2];						
int		losersSaved[2];							
int		quickTricksSaved[2];					
int		deal;									// which deal is this
int		dealer;									// who is the dealer (N, E, S, W)
int		vulnerable;								// index into vulnerable table
char 	contract1List[CONTRACT_STRING + 4];		// list of contracts that can be made
char 	contract2List[CONTRACT_STRING + 4];		// leave room for ...
char 	parList[CONTRACT_STRING + 4];			// par score string
ddTableDealsPBN DDdealsPBN;						// PBN format for our deal
ddTablesRes tableRes;							// double dummy results
allParResults pres;								// par results


static VULNERABLE Vulnerable[16] = 
{ 
	VUL_NONE, VUL_NS, VUL_EW, VUL_BOTH,
	VUL_NS, VUL_EW, VUL_BOTH, VUL_NONE,
	VUL_EW, VUL_BOTH, VUL_NONE, VUL_NS,
	VUL_BOTH, VUL_NONE, VUL_NS, VUL_EW,
};
char *	DealerString[4] = { "N Deals", "E Deals", "S Deals", "W Deals" };
char *	VulPBNString[4] = { "None", "NS", "EW", "All" };
char *	DealerChar = "NESW";
char *	ContractString[CONTRACTS] = { "C", "D", "H", "S", "NT" };
char *	DeclarerString[DIRECTIONS] = { "North", "East", "South", "West" };
char	LeaderString[DIRECTIONS] = { 'N', 'E', 'S', 'W' };
char *	SuitString = "SHDC";
char *	SpotString = "23456789TJQKA";
char *	VulString[16] =
{
	"None Vul", "N-S Vul", "E-W Vul", "Both Vul",
	"N-S Vul", "E-W Vul", "Both Vul", "None Vul",
	"E-W Vul", "Both Vul", "None Vul", "N-S Vul",
	"Both Vul", "None Vul", "N-S Vul", "E-W Vul",
};
// mode = 0: par calculation, vulnerability None
// mode = 1: par calculation, vulnerability All
// mode = 2: par calculation, vulnerability NS
// mode = 3: par calculation, vulnerability EW
static int	  Mode[4] = { 0, 2, 3, 1 };


bool DoneSavingHands()
{
	return deal == MAX_HAND_RECORD_COUNT;
}


void SaveEWHandInfo()
{
	handsSaved[0] = hands[DIR_EAST];
	handsSaved[1] = hands[DIR_WEST];
	hcPointsSaved[0] = hcPoints[DIR_EAST];
	hcPointsSaved[1] = hcPoints[DIR_WEST];
	totalPointsSaved[0] = totalPoints[DIR_EAST];
	totalPointsSaved[1] = totalPoints[DIR_WEST];
	concentratedSaved[0] = concentrated[DIR_EAST];
	concentratedSaved[1] = concentrated[DIR_WEST];
	controlsSaved[0] = controls[DIR_EAST];
	controlsSaved[1] = controls[DIR_WEST];
	losersSaved[0] = losers[DIR_EAST];
	losersSaved[1] = losers[DIR_WEST];
	quickTricksSaved[0] = quickTricks[DIR_EAST];
	quickTricksSaved[1] = quickTricks[DIR_WEST];
}

void RestoreEWHandInfo()
{
	hands[DIR_EAST] = handsSaved[0];
	hands[DIR_WEST] = handsSaved[1];
	hcPoints[DIR_EAST] = hcPointsSaved[0];
	hcPoints[DIR_WEST] = hcPointsSaved[1];
	totalPoints[DIR_EAST] = totalPointsSaved[0];
	totalPoints[DIR_WEST] = totalPointsSaved[1];
	concentrated[DIR_EAST] = concentratedSaved[0];
	concentrated[DIR_WEST] = concentratedSaved[1];
	controls[DIR_EAST] = controlsSaved[0];
	controls[DIR_WEST] = controlsSaved[1];
	losers[DIR_EAST] = losersSaved[0];
	losers[DIR_WEST] = losersSaved[1];
	quickTricks[DIR_EAST] = quickTricksSaved[0];
	quickTricks[DIR_WEST] = quickTricksSaved[1];
}

int GetDeal()
{
	return deal;
}

int GetDealer()
{
	return dealer;
}

char GetDealerPBNChar()
{
	return DealerChar[dealer];
}

char GetDealerPBNChar(int d)
{
	return DealerChar[d];
}

void SetDealer(int d)
{
	dealer = d;
}

int GetVul()
{
	return vulnerable;
}

char * GetVulPBNString()
{
	return VulPBNString[Vulnerable[vulnerable]];
}

char * GetVulPBNString(int v)
{
	return VulPBNString[v];
}

void SetVul(HWND hWnd, int v, bool create)
{
	int vv = vulnerable;
	vulnerable = v;
	if (v != vv)
		CreateContractList(hWnd);
}


bool	MeetsPointRestriction(int h, RESTRICT * restrict)
{
	if (restrict->HCP)
	{
		if (hcPoints[h] < restrict->minPoints
		|| hcPoints[h] > restrict->maxPoints)
			return false;
	}
	else
	{
		if (totalPoints[h] < restrict->minPoints
		|| totalPoints[h] > restrict->maxPoints)
			return false;
	}
	if (restrict->concentrated)
	{
		if (concentrated[h] < restrict->kRatio)
			return false;
	}
	if (controls[h] < restrict->minControls
	|| controls[h] > restrict->maxControls)
		return false;
	if (losers[h] < restrict->minLosers
	|| losers[h] > restrict->maxLosers)
		return false;
	if (quickTricks[h] < restrict->minQTricks
	|| quickTricks[h] > restrict->maxQTricks)
		return false;
	return true;
}

//
//	input: mask has one to all four bits set (mask != 0)
//	return: pick random bit position (0-3) set in mask
//
static int randomChoices[24][4] = 
{
	{ 0, 1, 2, 3, },
	{ 0, 1, 3, 2, },
	{ 0, 2, 3, 1, },
	{ 0, 2, 1, 3, },
	{ 0, 3, 1, 2, },
	{ 0, 3, 2, 1, },
	{ 1, 0, 2, 3, },
	{ 1, 0, 3, 2, },
	{ 1, 2, 3, 0, },
	{ 1, 2, 0, 3, },
	{ 1, 3, 0, 2, },
	{ 1, 3, 2, 0, },
	{ 2, 1, 0, 3, },
	{ 2, 1, 3, 0, },
	{ 2, 0, 3, 1, },
	{ 2, 0, 1, 3, },
	{ 2, 3, 1, 0, },
	{ 2, 3, 0, 1, },
	{ 3, 1, 2, 0, },
	{ 3, 1, 0, 2, },
	{ 3, 2, 0, 1, },
	{ 3, 2, 1, 0, },
	{ 3, 0, 1, 2, },
	{ 3, 0, 2, 1, },
};
int RandomBitInMask(int mask)
{
	int index = Random(24);
	for (int i = 0; i < 4; i++)
	{
		if ((mask & (1 << randomChoices[index][i])) != 0)
			return randomChoices[index][i];
	}
	return 0;											// we should never use this exit
}


//
//	assignment is an array of 4 which assigns a suit to the 4 requirements of the general shape
//	it is valid only if we return true
//
bool MeetsGeneralShape(int h, RESTRICT * restrict, int * assignment)
{
	int mask1[4], mask = 0;
	for (int s = 0; s < 4; s++)
	{
		mask1[s] = 0;
		for (int i = 0; i < 4; i++)						// see what suits each restrictions matches
		{
			if (restrict->minCards[i] <= hands[h].suits[s].count
			&&  hands[h].suits[s].count <= restrict->maxCards[i])
				mask1[s] |= 1 << i;
		}
		if (mask1[0] == 0)								// this suit does not match any restriction
			return false;
		mask |= mask1[s];
	}
	if (mask != 0xf)									// at least one restriction is not covered by one of the suits
		return false;
	// now make sure we do not have only one suit for two restrictions (like 65xx for 6421 hand)
	// or two suits for three restrictions (like 4441 but the hand is 4432)
	// assign suits as if they were exact shape instead of general shape
	// but make sure we randomly assign the suits else we could get all spades
	for (int s = 0; s < 4; s++)
	{
		if (mask1[s] == 0)								// this requirement was used by another suit
			return false;
		assignment[s] = RandomBitInMask(mask1[s]);
		mask1[0] &= ~(1 << assignment[s]);
		mask1[1] &= ~(1 << assignment[s]);
		mask1[2] &= ~(1 << assignment[s]);
		mask1[3] &= ~(1 << assignment[s]);
	}
	return true;
}


bool	MeetsRestriction(int h, RESTRICT * restrict, int * assignment)
{
	if (restrict->not)									// skip not restrictions
		return true;
	if (MeetsPointRestriction(h, restrict) == false)
		return false;
	if (restrict->exactShape)
	{
		if (hands[h].suits[0].count < restrict->minCards[0]
			|| hands[h].suits[1].count < restrict->minCards[1]
			|| hands[h].suits[2].count < restrict->minCards[2]
			|| hands[h].suits[3].count < restrict->minCards[3]
			|| hands[h].suits[0].count > restrict->maxCards[0]
			|| hands[h].suits[1].count > restrict->maxCards[1]
			|| hands[h].suits[2].count > restrict->maxCards[2]
			|| hands[h].suits[3].count > restrict->maxCards[3])
			return false;
	}
	else if (MeetsGeneralShape(h, restrict, assignment) == false)
		return false;
	return true;
}


bool	MeetsSouthRestrictions(int h, int * assignment, bool generalShape = false)
{
	RESTRICT  * restrict;
	bool	ret = true;												// in case there are only not conditions
	int		orMask = 0, count = 0;									// get south non-NOT restrictions 
	int		r = 0;

	// we4 do this in case of say the 1NT with 5-2 majors
	// where we want to swap suits but do not want to get all spades
	for (int r = 0; r < 4; r++)										// get non-NOT restrictions								
	{
		if ((restriction.mask & (1 << r)) == 0)						// end of restrictions 
			break;
		restrict = &restriction.restrict[r];
		if (restrict->not)											// skip not restrictions
			continue;
		count++;
		orMask |= 1 << r;
	}
	if (generalShape == true
	&& count > 1)
	{
		r = RandomBitInMask(orMask);								// start with random OR restriction
	}
	for (; r < 4; r++)																					
	{
		if ((restriction.mask & (1 << r)) == 0)						// end of restrictions 
			break;
		restrict = &restriction.restrict[r];
		if (restrict->not)											// skip not restrictions
			continue;
		if (MeetsPointRestriction(h, restrict) == false)
		{
			ret = false;
			continue;
		}
		if (restrict->exactShape && generalShape == false)
		{
			if (hands[h].suits[0].count < restrict->minCards[0]
			|| hands[h].suits[1].count < restrict->minCards[1]
			|| hands[h].suits[2].count < restrict->minCards[2]
			|| hands[h].suits[3].count < restrict->minCards[3]
			|| hands[h].suits[0].count > restrict->maxCards[0]
			|| hands[h].suits[1].count > restrict->maxCards[1]
			|| hands[h].suits[2].count > restrict->maxCards[2]
			|| hands[h].suits[3].count > restrict->maxCards[3])
			{
				ret = false;
				continue;
			}
			return true;
		}
		else if (MeetsGeneralShape(h, restrict, assignment))
			return true;
		else
			ret = false;
	}
	return ret;
}


// Only south has NOT restrictions
bool MeetsNotRestrictions(int h)
{
	RESTRICT * restrict = &restriction.restrict[0];
	int assignment[4];

	for (int r = 0; r < MAX_SOUTH_RESTRICT; r++, restrict++)
	{
		if (restrict->not == false)						// look for NOT restrictions
			continue;
		if (MeetsPointRestriction(h, restrict) == false)
			continue;
		if (restrict->exactShape)
		{
			if (hands[h].suits[0].count < restrict->minCards[0]
			|| hands[h].suits[1].count < restrict->minCards[1]
			|| hands[h].suits[2].count < restrict->minCards[2]
			|| hands[h].suits[3].count < restrict->minCards[3]
			|| hands[h].suits[0].count > restrict->maxCards[0]
			|| hands[h].suits[1].count > restrict->maxCards[1]
			|| hands[h].suits[2].count > restrict->maxCards[2]
			|| hands[h].suits[3].count > restrict->maxCards[3])
				continue;
			return false;
		}
		else if (MeetsGeneralShape(h, restrict, assignment))
			return false;
	}
	return true;
}


void DeckInit()
{
	for (int spot = 0; spot < SPOTS; spot++)
	{
		deck[spot].spot = spot;
		deck[spot].suit = SUIT_SPADES;
		deck[spot + SPOTS].spot = spot;
		deck[spot + SPOTS].suit = SUIT_HEARTS;
		deck[spot + SPOTS * 2].spot = spot;
		deck[spot + SPOTS * 2].suit = SUIT_CLUBS;
		deck[spot + SPOTS * 3].spot = spot;
		deck[spot + SPOTS * 3].suit = SUIT_DIAMONDS;
	}
}


void GameInit()
{
	deal = 0;
	dealer = -1;
	vulnerable = -1;
	DeckInit();
}


void SwapHands(int h1, int h2)
{
	if (h1 == h2)
		return;
	int t;
	for (int s = 0; s < 4; s++)
	{
		int c1 = hands[h1].suits[s].count;
		int c2 = hands[h2].suits[s].count;
		for (int c = 0; c < c1 || c < c2; c++)
		{
			t = hands[h1].suits[s].spots[c];
			hands[h1].suits[s].spots[c] = hands[h2].suits[s].spots[c];
			hands[h2].suits[s].spots[c] = t;
		}
		hands[h1].suits[s].count = c2;
		hands[h2].suits[s].count = c1;
	}
	t = hcPoints[h1];								// swap points too
	hcPoints[h1] = hcPoints[h2];
	hcPoints[h2] = t;
	t = totalPoints[h1];
	totalPoints[h1] = totalPoints[h2];
	totalPoints[h2] = t;
	t = concentrated[h1];
	concentrated[h1] = concentrated[h2];
	concentrated[h2] = t;
	t = controls[h1];
	controls[h1] = controls[h2];
	controls[h2] = t;
	t = losers[h1];
	losers[h1] = losers[h2];
	losers[h2] = t;
	t = quickTricks[h1];
	quickTricks[h1] = quickTricks[h2];
	quickTricks[h2] = t;
}


void SwapSuits(int s1, int s2)
{
	if (s1 == s2)
		return;
	for (int h = 0; h < 4; h++)
	{
		int c1 = hands[h].suits[s1].count;
		int c2 = hands[h].suits[s2].count;
		for (int c = 0; c < c1 || c < c2; c++)
		{
			int t = hands[h].suits[s1].spots[c];
			hands[h].suits[s1].spots[c] = hands[h].suits[s2].spots[c];
			hands[h].suits[s2].spots[c] = t;
		}
		hands[h].suits[s1].count = c2;
		hands[h].suits[s2].count = c1;
	}
}


struct Contract
{
	int8	tricks;			// 7 to 13
	int8	contract;		// see CONTRACTS enum 0=clubs...4=no trump
	int8	resIndex;		// 0=spades, ... 3=clubs, 4=notrump
};
static Contract contracts[35] = 
{
	{ 13, CONTRACT_NOTRUMP, 4 },
	{ 13, CONTRACT_SPADES, 0 },
	{ 13, CONTRACT_HEARTS, 1 },
	{ 13, CONTRACT_DIAMONDS, 2 },
	{ 13, CONTRACT_CLUBS, 3 },
	{ 12, CONTRACT_NOTRUMP, 4 },
	{ 12, CONTRACT_SPADES, 0 },
	{ 12, CONTRACT_HEARTS, 1 },
	{ 12, CONTRACT_DIAMONDS, 2 },
	{ 12, CONTRACT_CLUBS, 3 },
	{ 11, CONTRACT_NOTRUMP, 4 },	// 460
	{ 11, CONTRACT_SPADES, 0 },		// 450
	{ 11, CONTRACT_HEARTS, 1 },
	{ 10, CONTRACT_NOTRUMP, 4 },	// 430
	{ 10, CONTRACT_SPADES, 0 },		// 420
	{ 10, CONTRACT_HEARTS, 1 },
	{ 11, CONTRACT_DIAMONDS, 2 },	// 400
	{ 11, CONTRACT_CLUBS, 3 },
	{ 9, CONTRACT_NOTRUMP, 4 },		// 400	
	{ 9, CONTRACT_SPADES, 0 },		// 140
	{ 9, CONTRACT_HEARTS, 1 },
	{ 10, CONTRACT_DIAMONDS, 2 },	// 130
	{ 10, CONTRACT_CLUBS, 3 },
	{ 8, CONTRACT_NOTRUMP, 4 },		// 120
	{ 9, CONTRACT_DIAMONDS, 2 },	// 110
	{ 9, CONTRACT_CLUBS, 3 },
	{ 8, CONTRACT_SPADES, 0 },		// 110
	{ 8, CONTRACT_HEARTS, 1 },
	{ 8, CONTRACT_DIAMONDS, 2 },	// 90
	{ 8, CONTRACT_CLUBS, 3 },
	{ 7, CONTRACT_NOTRUMP, 4 },		// 90
	{ 7, CONTRACT_SPADES, 0 },		// 80
	{ 7, CONTRACT_HEARTS, 1 },
	{ 7, CONTRACT_DIAMONDS, 2 },	// 70
	{ 7, CONTRACT_CLUBS, 3 },
};

void GetBestContract(int & bid, int & suit, int & declarer)
{
	for (int i = 0; i < 35; i++)
	{
		int index = contracts[i].resIndex;
		if (tableRes.results[0].resTable[index][DIR_SOUTH] == contracts[i].tricks)
		{
			bid = contracts[i].tricks - 6;
			suit = contracts[i].contract;
			declarer = DIR_SOUTH;
			return;
		}
		if (tableRes.results[0].resTable[index][DIR_NORTH] == contracts[i].tricks)
		{
			bid = contracts[i].tricks - 6;
			suit = contracts[i].contract;
			declarer = DIR_NORTH;
			return;
		}
		if (tableRes.results[0].resTable[index][DIR_EAST] == contracts[i].tricks)
		{
			bid = contracts[i].tricks - 6;
			suit = contracts[i].contract;
			declarer = DIR_EAST;
			return;
		}
		if (tableRes.results[0].resTable[index][DIR_WEST] == contracts[i].tricks)
		{
			bid = contracts[i].tricks - 6;
			suit = contracts[i].contract;
			declarer = DIR_WEST;
			return;
		}
	}
}


void ListContracts()
{
	char string[CONTRACT_STRING+1], dir1, dir2, who[20], suit;
	int index, contract;
	char * list;

	strcpy(parList, "Par ");
	// skip over NS or EW
	for (int i = 2; i < (int)strlen(pres.presults[0].parScore[0]); i++)
	{
		if (pres.presults[0].parScore[0][i] != '-'
		||	pres.presults[0].parScore[0][i] != '+')
		{
			strncat(parList, &pres.presults[0].parScore[0][i], 10);
			break;
		}
	}
	// change format for how many down for sacrifices
	index = strlen(pres.presults[0].parContractsString[0]);
	if (pres.presults[0].parContractsString[0][index-1] == 'x')	// this is a sacrifice
	{
		int cnt = sscanf(pres.presults[0].parContractsString[0], "%c%c:%s %d%c", &dir1, &dir2, who, &contract, &suit);
		if (suit == 'S')
			index = 0;
		else if (suit == 'H')
			index = 1;
		else if (suit == 'D')
			index = 2;
		else if (suit == 'C')
			index = 3;
		else 
			index = 4;
		int down;
		if (who[0] == 'N')
			down = 6 + contract - tableRes.results[0].resTable[index][0];
		else if (who[0] == 'S')
			down = 6 + contract - tableRes.results[0].resTable[index][2];
		else if (who[0] == 'E')
			down = 6 + contract - tableRes.results[0].resTable[index][1];
		else
			down = 6 + contract - tableRes.results[0].resTable[index][3];
		sprintf(parList, "%s:%s %d%cx-%d", parList, who, contract, suit, down);
	}
	contract1List[0] = 0;
	contract2List[0] = 0;
	list = contract1List;
	for (int i = 0; i < 35; i++)
	{
		index = contracts[i].resIndex;
		// NS contracts
		if (tableRes.results[0].resTable[index][0] == contracts[i].tricks
		&&	tableRes.results[0].resTable[index][2] == contracts[i].tricks)
		{
			sprintf(string, "NS %d%s; ", contracts[i].tricks - 6, ContractString[contracts[i].contract]);
			if (strlen(string) + strlen(list) > CONTRACT_STRING)
			{
				if (list == contract1List)
					list = contract2List;
				else 
				{
					list[strlen(list) - 1] = 0;			// remove trailing space
					strncat(list, "...", CONTRACT_STRING);
					break;
				}
			}
			strncat(list, string, CONTRACT_STRING);
			string[CONTRACT_STRING] = 0;				// remove trailing blank if necessary
		}
		else if (tableRes.results[0].resTable[index][0] == contracts[i].tricks)
		{
			sprintf(string, "N %d%s; ", contracts[i].tricks - 6, ContractString[contracts[i].contract]);
			if (strlen(string) + strlen(list) > CONTRACT_STRING)
			{
				if (list == contract1List)
					list = contract2List;
				else
				{
					list[strlen(list) - 1] = 0;			// remove trailing space
					strncat(list, "...", CONTRACT_STRING);
					break;
				}
			}
			strncat(list, string, CONTRACT_STRING);
			string[CONTRACT_STRING] = 0;				// remove trailing blank if necessary
		}
		else if (tableRes.results[0].resTable[index][2] == contracts[i].tricks)
		{
			sprintf(string, "S %d%s; ", contracts[i].tricks - 6, ContractString[contracts[i].contract]);
			if (strlen(string) + strlen(list) > CONTRACT_STRING)
			{
				if (list == contract1List)
					list = contract2List;
				else
				{
					list[strlen(list) - 1] = 0;			// remove trailing space
					strncat(list, "...", CONTRACT_STRING);
					break;
				}
			}
			strncat(list, string, CONTRACT_STRING);
			string[CONTRACT_STRING] = 0;				// remove trailing blank if necessary
		}
		// EW contracts
		if (tableRes.results[0].resTable[index][1] == contracts[i].tricks
		&&	tableRes.results[0].resTable[index][3] == contracts[i].tricks)
		{
			sprintf(string, "EW %d%s; ", contracts[i].tricks - 6, ContractString[contracts[i].contract]);
			if (strlen(string) + strlen(list) > CONTRACT_STRING)
			{
				if (list == contract1List)
					list = contract2List;
				else
				{
					list[strlen(list) - 1] = 0;			// remove trailing space
					strncat(list, "...", CONTRACT_STRING);
					break;
				}
			}
			strncat(list, string, CONTRACT_STRING);
			string[CONTRACT_STRING] = 0;				// remove trailing blank if necessary
		}
		else if (tableRes.results[0].resTable[index][1] == contracts[i].tricks)
		{
			sprintf(string, "E %d%s; ", contracts[i].tricks - 6, ContractString[contracts[i].contract]);
			if (strlen(string) + strlen(list) > CONTRACT_STRING)
			{
				if (list == contract1List)
					list = contract2List;
				else
				{
					list[strlen(list)-1] = 0;			// remove trailing space
					strncat(list, "...", CONTRACT_STRING);
					break;
				}
			}
			strncat(list, string, CONTRACT_STRING);
			string[CONTRACT_STRING] = 0;				// remove trailing blank if necessary
		}
		else if (tableRes.results[0].resTable[index][3] == contracts[i].tricks)
		{
			sprintf(string, "W %d%s; ", contracts[i].tricks - 6, ContractString[contracts[i].contract]);
			if (strlen(string) + strlen(list) > CONTRACT_STRING)
			{
				if (list == contract1List)
					list = contract2List;
				else
				{
					list[strlen(list) - 1] = 0;			// remove trailing space
					strncat(list, "...", CONTRACT_STRING);
					break;
				}
			}
			strncat(list, string, CONTRACT_STRING);
			string[CONTRACT_STRING] = 0;				// remove trailing blank if necessary
		}
	}
}


//	
//	keep1 and keep2 can be used to keep specific hands (-1 if no hand is kept)
//
int Shuffle(int keep1, int keep2, struct deal * dl)
{
	int kept = 0;
	if (keep1 >= 0)										// keep this hands cards out of the shuffle
	{
		for (int s = 0; s < 4; s++)
		{
			for (int c = 0; c < hands[keep1].suits[s].count; c++)
			{
				for (int n = kept; n < 52; n++)
				{
					if (deck[n].suit == s
					&& deck[n].spot == hands[keep1].suits[s].spots[c])
					{
						CARD t = deck[kept];
						deck[kept++] = deck[n];
						deck[n] = t;
						break;
					}
				}
			}
		}
	}
	if (keep2 >= 0)
	{
		for (int s = 0; s < 4; s++)
		{
			for (int c = 0; c < hands[keep2].suits[s].count; c++)
			{
				for (int n = kept; n < 52; n++)
				{
					if (deck[n].suit == s
					&& deck[n].spot == hands[keep2].suits[s].spots[c])
					{
						CARD t = deck[kept];
						deck[kept++] = deck[n];
						deck[n] = t;
						break;
					}
				}
			}
		}
	}
	// pull locked cards from the shuffle
	for (int h = 0; h < 4; h++)
	{
		if (h == keep1 || h == keep2)							// any locked cards in these hands are already kept
			continue;
		for (int c = 0; c < restriction.lockedCount[h]; c++)
		{
			for (int n = kept; n < 52; n++)
			{
				if ((restriction.lockedCards[h][deck[n].suit] & (1 << deck[n].spot)) != 0)
				{
					CARD t = deck[kept];
					deck[kept++] = deck[n];
					deck[n] = t;
					break;
				}
			}
		}
	}
	// pull cards already played from the shuffle
	// this assumes both keep1 and keep2 are used
	if (dl != NULL)
	{
		int h = min(keep1, keep2) ^ 1;
		for (int c = 0; c < BackupPlayCount[h]; c++)
		{
			for (int n = kept; n < 52; n++)
			{
				if (deck[n].suit == BackupPlaySuit[h][c]
				&& deck[n].spot == BackupPlaySpot[h][c])
				{
					CARD t = deck[kept];
					deck[kept++] = deck[n];
					deck[n] = t;
					break;
				}
			}
		}
		h += 2;
		for (int c = 0; c < BackupPlayCount[h]; c++)
		{
			for (int n = kept; n < 52; n++)
			{
				if (deck[n].suit == BackupPlaySuit[h][c]
				&& deck[n].spot == BackupPlaySpot[h][c])
				{
					CARD t = deck[kept];
					deck[kept++] = deck[n];
					deck[n] = t;
					break;
				}
			}
		}
	}
	CARD c;
	for (int i = kept; i < 52; i++)
	{
		int r = kept + Random(52-kept);
		c = deck[r];
		deck[r] = deck[i];
		deck[i] = c;
	}
	return kept;
}


int	CompareSpots(const void * e1, const void * e2)
{
	int8 spot1 = *((int8 *)e1);
	int8 spot2 = *((int8 *)e2);
	if (spot1 < spot2)							// sort by largest first
		return 1;
	return -1;
}


void CreateContractList(HWND hWnd)
{
	// create PBN format
	DDdealsPBN.noOfTables = 1;
	int n = 0;
	DDdealsPBN.deals[0].cards[n++] = DealerChar[dealer];
	DDdealsPBN.deals[0].cards[n++] = ':';
	int h = dealer;
	do
	{
		for (int s = 0; s < 4; s++)
		{
			for (int c = 0; c < hands[h].suits[s].count; c++)
			{
				DDdealsPBN.deals[0].cards[n++] = SpotString[hands[h].suits[s].spots[c]];
			}
			if (s == 3)
				DDdealsPBN.deals[0].cards[n++] = ' ';
			else
				DDdealsPBN.deals[0].cards[n++] = '.';
		}
		if (++h > 3)
			h = 0;
	} while (h != dealer);
	n--;																// remove last space
	DDdealsPBN.deals[0].cards[n] = 0;
	int mode = Mode[Vulnerable[vulnerable]];							// par calculation based on vulnerability
	int trumpFilter[DDS_STRAINS] = { 0, 0, 0, 0, 0 };					// All contracts
	int res;
	res = CalcAllTablesPBN(&DDdealsPBN, mode, trumpFilter, &tableRes, &pres);
	if (res != RETURN_NO_FAULT)
	{
		char string[60];
		sprintf(string, "Double Dummy Solver failed! (%d)", res);
		MessageBox(hWnd, string, "Internal error", MB_ICONSTOP | MB_OK | MB_APPLMODAL);
		contract1List[0] = contract2List[0] = parList[0] = 0;
		return;
	}
	ListContracts();													// list of contracts that can be made
}


void UpdateContractResults(int(&results)[MAX_HAND_COUNT][CONTRACTS][DIRECTIONS], int & count)
{
	for (int h = 0; h < DIRECTIONS; h++)
	{
		results[count][0][h] = tableRes.results[0].resTable[4][h];
		results[count][1][h] = tableRes.results[0].resTable[0][h];
		results[count][2][h] = tableRes.results[0].resTable[1][h];
		results[count][3][h] = tableRes.results[0].resTable[2][h];
		results[count][4][h] = tableRes.results[0].resTable[3][h];
	}
	count++;
}


void ResetDeal(HWND hWnd)
{
	deal = 1;
	if (restriction.dealer != DIR_SYSTEMIC)
		dealer = restriction.dealer;
	else
		dealer = 0;
	if (restriction.vul != VUL_SYSTEMIC
	&&	vulnerable != restriction.vul)
	{
		vulnerable = restriction.vul;
		CreateContractList(hWnd);											// sacrifices may change now
	}
	else if (vulnerable != 0)
	{
		vulnerable = 0;
		CreateContractList(hWnd);											// sacrifices may change now
	}
}


static int	unsupportedHonorSingletonDeduction[4] = { -1, -2, -2, 0 };
static int	unsupportedAceHonorDeduction[3] = { -1, -1, 0 };
static int	unsupportedKingHonorDeduction[2] = { -2, -1 };
static int	unsupportedQueenHonorDeduction = -2;
static int	unsupportedHonorDeduction = -1;

void CountHands()
{
	int h;

	// count points
	hcPoints[0] = hcPoints[1] = hcPoints[2] = hcPoints[3] = 0;
	totalPoints[0] = totalPoints[1] = totalPoints[2] = totalPoints[3] = 0;
	concentrated[0] = concentrated[1] = concentrated[2] = concentrated[3] = 0;
	controls[0] = controls[1] = controls[2] = controls[3] = 0;
	losers[0] = losers[1] = losers[2] = losers[3] = 0;
	quickTricks[0] = quickTricks[1] = quickTricks[2] = quickTricks[3] = 0;
	for (h = 0; h < 4; h++)
	{
		int max1 = 0, max2 = 0;											// find 2 suits with max number of cards
		int cnt1 = -1, cnt2 = -1;
		for (int s = 0; s < 4; s++)
		{
			if (cnt1 < hands[h].suits[s].count)							// find two longest suits (majors in case of ties)
			{
				max2 = max1;
				cnt2 = cnt1;
				max1 = s;
				cnt1 = hands[h].suits[s].count;
			}
			else if (cnt2 < hands[h].suits[s].count)
			{
				max2 = s;
				cnt2 = hands[h].suits[s].count;
			}
			for (int c = 0; c < hands[h].suits[s].count; c++)
			{
				if (hands[h].suits[s].spots[c] <= SPOT_TEN)
					break;
				hcPoints[h] += hands[h].suits[s].spots[c] - SPOT_TEN;
				totalPoints[h] += hands[h].suits[s].spots[c] - SPOT_TEN;
				if (hands[h].suits[s].spots[c] >= SPOT_KING)			// count aces and king controls
					controls[h] += hands[h].suits[s].spots[c] - SPOT_QUEEN;
			}
			// count quick tricks
			if (hands[h].suits[s].count >= 2
				&& hands[h].suits[s].spots[0] == SPOT_ACE
				&& hands[h].suits[s].spots[1] == SPOT_KING)
				quickTricks[h] += 4;
			else if (hands[h].suits[s].count >= 2
				&& hands[h].suits[s].spots[0] == SPOT_ACE
				&& hands[h].suits[s].spots[1] == SPOT_QUEEN)
				quickTricks[h] += 3;
			else if (hands[h].suits[s].count >= 1
				&& hands[h].suits[s].spots[0] == SPOT_ACE)
				quickTricks[h] += 2;
			else if (hands[h].suits[s].count >= 2
				&& hands[h].suits[s].spots[0] == SPOT_KING
				&& hands[h].suits[s].spots[1] == SPOT_QUEEN)
				quickTricks[h] += 2;
			else if (hands[h].suits[s].count >= 1
				&& hands[h].suits[s].spots[0] == SPOT_KING)
				quickTricks[h] += 1;
			// count losers
			switch (hands[h].suits[s].count)
			{
			case 0:													// 0 losers
				break;
			case 1:
				if (hands[h].suits[s].spots[0] != SPOT_ACE)
					losers[h] += 1;
				break;
			case 2:
				if (hands[h].suits[s].spots[0] == SPOT_ACE)
				{
					if (hands[h].suits[s].spots[1] != SPOT_KING)
						losers[h] += 1;
				}
				else if (hands[h].suits[s].spots[0] == SPOT_KING)
				{
					losers[h] += 1;
				}
				else
					losers[h] += 2;
				break;
			default:
				if (hands[h].suits[s].spots[0] == SPOT_ACE)
				{
					if (hands[h].suits[s].spots[1] == SPOT_KING			// AKQ
						&&  hands[h].suits[s].spots[2] == SPOT_QUEEN)
						break;
					if (hands[h].suits[s].spots[1] == SPOT_KING)		// AKx
						losers[h] += 1;
					else if (hands[h].suits[s].spots[1] == SPOT_QUEEN)	// AQx
						losers[h] += 1;
					else if (hands[h].suits[s].spots[1] == SPOT_JACK	// AJT
						&& hands[h].suits[s].spots[2] == SPOT_TEN)
						losers[h] += 1;
					else
						losers[h] += 2;									// Axx
				}
				else if (hands[h].suits[s].spots[0] == SPOT_KING)
				{
					if (hands[h].suits[s].spots[1] == SPOT_QUEEN)		// KQx
						losers[h] += 1;
					else
						losers[h] += 2;									// Kxx
				}
				else if (hands[h].suits[s].spots[0] == SPOT_QUEEN)
				{
					if (hands[h].suits[s].spots[1] == SPOT_QUEEN)		// QJx
						losers[h] += 1;
					else
						losers[h] += 2;									// Qxx
				}
				else
					losers[h] += 3;
				break;
			}
			// count distribution points for total points
			if (hands[h].suits[s].count < 3)
			{
				totalPoints[h] += 3 - hands[h].suits[s].count;
				// unsupported honors adjustment
				if (hands[h].suits[s].count == 1
					&& hands[h].suits[s].spots[0] >= SPOT_JACK)			// stiff JQKA
					totalPoints[h] += unsupportedHonorSingletonDeduction[hands[h].suits[s].spots[0] - SPOT_JACK];
				else if (hands[h].suits[s].count == 2
					&& hands[h].suits[s].spots[0] == SPOT_ACE
					&&  hands[h].suits[s].spots[1] >= SPOT_JACK)
					totalPoints[h] += unsupportedAceHonorDeduction[hands[h].suits[s].spots[1] - SPOT_JACK];
				else if (hands[h].suits[s].count == 2
					&& hands[h].suits[s].spots[0] == SPOT_KING
					&&  hands[h].suits[s].spots[1] >= SPOT_JACK)
					totalPoints[h] += unsupportedKingHonorDeduction[hands[h].suits[s].spots[1] - SPOT_JACK];
				else if (hands[h].suits[s].count == 2
					&& hands[h].suits[s].spots[0] == SPOT_QUEEN
					&&  hands[h].suits[s].spots[1] >= SPOT_JACK)
					totalPoints[h] += unsupportedQueenHonorDeduction;
				else if (hands[h].suits[s].spots[0] >= SPOT_JACK
					&& hands[h].suits[s].spots[0] != SPOT_ACE)
					totalPoints[h] += unsupportedHonorDeduction;
			}
		}
		int points = 0;
		for (int c = 0; c < hands[h].suits[max1].count; c++)
		{
			if (hands[h].suits[max1].spots[c] <= SPOT_TEN)
				break;
			points += hands[h].suits[max1].spots[c] - SPOT_TEN;
		}
		for (int c = 0; c < hands[h].suits[max2].count; c++)
		{
			if (hands[h].suits[max2].spots[c] <= SPOT_TEN)
				break;
			points += hands[h].suits[max2].spots[c] - SPOT_TEN;
		}
		if (hcPoints[h] != 0)
			concentrated[h] = (int)(100 * points / hcPoints[h]);
	}
}

//
//	Total points add distribution
//	Unsupported honors total points
//	A - 6 points (no deduction)
//	K - 3 points (-2 deduction)
//	Q - 2 points (-2 deduction)
//	J - 2 points (-1 deduction)
//	AK - 8 points (no deduction)
//	AQ - 6 points (-1 deduction)
//	AJ - 5 points (-1 deduction)
//	KQ - 5 points (-1 deduction)
//	KJ - 3 points (-2 deduction)
//	QJ - 2 points (-2 deduction)
//	Ax - 5 points (no deduction)
//	Kx - 3 points (-1 deduction)
//	Qx - 2 points (-1 deduction)
//	Jx - 1 point (-1 deduction)
//
//	redeal means we do not increase the deal number not save the current hand
//	keep1 and keep2 can be used to keep specific hands (-1 if no hand is kept)
//	if dl is used we do not redo the contract list but place the reshuffled cards into the dl struct
bool NewDeal(HWND hWnd, bool redeal, int keep1, int keep2, struct deal * dl)
{
	int h;
	int declarer = min(keep1, keep2);
	int retries = 0;

	if (redeal == false)
	{
		deal++;
		if (restriction.dealer != DIR_SYSTEMIC)
			dealer = restriction.dealer;
		else if (++dealer > 3)
			dealer = 0;
		if (restriction.vul != VUL_SYSTEMIC)
			vulnerable = restriction.vul;
		else if (++vulnerable > 15)
			vulnerable = 0;
	}
	if (addComments)
	{
		HWND hEdit = GetDlgItem(hWnd, IDC_MAIN_EDIT);
		SetWindowText(hEdit, "");
	}
RETRY:
	if (++retries >= RETRIES)
	{
		MessageBox(hWnd, "100,000 shuffles failed to meet restrictions.", "Restriction Warning", MB_ICONSTOP | MB_OK | MB_APPLMODAL);
		contract1List[0] = contract2List[0] = parList[0] = 0;
		return false;
	}
	int n = Shuffle(keep1, keep2, dl);
	// place cards into each hand and sort the suits
	if (keep1 != 0 && keep2 != 0)
		hands[0].suits[0].count = hands[0].suits[1].count = hands[0].suits[2].count = hands[0].suits[3].count = 0;
	if (keep1 != 1 && keep2 != 1)
		hands[1].suits[0].count = hands[1].suits[1].count = hands[1].suits[2].count = hands[1].suits[3].count = 0;
	if (keep1 != 2 && keep2 != 2)
		hands[2].suits[0].count = hands[2].suits[1].count = hands[2].suits[2].count = hands[2].suits[3].count = 0;
	if (keep1 != 3 && keep2 != 3)
		hands[3].suits[0].count = hands[3].suits[1].count = hands[3].suits[2].count = hands[3].suits[3].count = 0;
	for (h = 0; h < 4; h++)
	{
		if (h == keep1 || h == keep2)
			continue;
		int c = 0;
		// put cards already played back into their hand
		for (int cc = 0; cc < BackupPlayCount[h]; cc++)					
		{
			int suit = BackupPlaySuit[h][cc];
			int count = hands[h].suits[suit].count;
			hands[h].suits[suit].spots[count] = BackupPlaySpot[h][cc];
			hands[h].suits[suit].count++;
			c++;
		}
		// put locked cards back into their hand
		for (int s = 0; s < 4; s++)
		{
			int cards = restriction.lockedCards[h][s];
			int mask = (1 << SPOT_ACE);
			for (int spot = SPOT_ACE; cards != 0; mask >>= 1, spot--)
			{
				if ((cards & mask) != 0)
				{
					// be sure we have not already placed it int the hand
					for (int cc = 0; cc < BackupPlayCount[h]; cc++)
					{
						if (BackupPlaySuit[h][cc] == s
						&&  BackupPlaySpot[h][cc] == spot)
							goto SKIPCARD;				// already saved away
					}
					int count = hands[h].suits[s].count;
					hands[h].suits[s].spots[count] = spot;
					hands[h].suits[s].count++;
					c++;
SKIPCARD:			cards &= ~mask;
				}
			}
		}
		// fill the hand with remaining cards
		for (; c < 13; c++, n++)
		{
			int suit = deck[n].suit;
			int count = hands[h].suits[suit].count;
			hands[h].suits[suit].spots[count] = deck[n].spot;
			hands[h].suits[suit].count++;
		}
		for (int s = 0; s < 4; s++)
		{
			qsort(hands[h].suits[s].spots, hands[h].suits[s].count, sizeof(hands[h].suits[s].spots[0]), CompareSpots);
		}
	}
	CountHands();											// count points for the hands
	if (dl != NULL && declarer == DIR_NORTH)				// do not test for any north or south restrictions for NS play
		goto NEXT2;
	if (restriction.mask != 0)
	{
		int assignment[4];
		if (MeetsSouthRestrictions(2, assignment)
		&& MeetsNotRestrictions(2))
		{
			goto NEXT;
		}
		if (dl != NULL)										// do not swap any hands when analyzing the EW play
			goto RETRY;
		if (MeetsSouthRestrictions(0, assignment)
		&& MeetsNotRestrictions(0))
		{
			SwapHands(2, 0);
			goto NEXT;
		}
		if (MeetsSouthRestrictions(1, assignment)
		&& MeetsNotRestrictions(1))
		{
			SwapHands(2, 1);
			goto NEXT;
		}
		if (MeetsSouthRestrictions(3, assignment)
		&& MeetsNotRestrictions(3))
		{
			SwapHands(2, 3);
			goto NEXT;
		}
#if 0 
		// if there are NOT or OR restrictions it makes it even more difficult
		// if the general shape fits then swap suits
		// it is possible the NOT condition will fail but we tried
		if (MeetsSouthRestrictions(2, assignment, true))
		{
			for (int h = 0; h < 4; h++)
			{
				HAND old = hands[h];
				hands[h].suits[assignment[0]] = old.suits[0];
				hands[h].suits[assignment[1]] = old.suits[1];
				hands[h].suits[assignment[2]] = old.suits[2];
				hands[h].suits[assignment[3]] = old.suits[3];
			}
			if (MeetsNotRestrictions(2))
				goto NEXT;
		}
		if (MeetsSouthRestrictions(0, assignment, true))
		{
			for (int h = 0; h < 4; h++)
			{
				HAND old = hands[h];
				hands[h].suits[assignment[0]] = old.suits[0];
				hands[h].suits[assignment[1]] = old.suits[1];
				hands[h].suits[assignment[2]] = old.suits[2];
				hands[h].suits[assignment[3]] = old.suits[3];
			}
			if (MeetsNotRestrictions(0))
			{
				SwapHands(0, 2);
				goto NEXT;
			}
		}
		if (MeetsSouthRestrictions(1, assignment, true))
		{
			for (int h = 0; h < 4; h++)
			{
				HAND old = hands[h];
				hands[h].suits[assignment[0]] = old.suits[0];
				hands[h].suits[assignment[1]] = old.suits[1];
				hands[h].suits[assignment[2]] = old.suits[2];
				hands[h].suits[assignment[3]] = old.suits[3];
			}
			if (MeetsNotRestrictions(1))
			{
				SwapHands(1, 2);
				goto NEXT;
			}
		}
		if (MeetsSouthRestrictions(3, assignment, true))
		{
			for (int h = 0; h < 4; h++)
			{
				HAND old = hands[h];
				hands[h].suits[assignment[0]] = old.suits[0];
				hands[h].suits[assignment[1]] = old.suits[1];
				hands[h].suits[assignment[2]] = old.suits[2];
				hands[h].suits[assignment[3]] = old.suits[3];
			}
			if (MeetsNotRestrictions(3))
			{
				SwapHands(3, 2);
				goto NEXT;
			}
		}
#endif
		goto RETRY;
NEXT:	if ((restriction.mask & RESTRICT_MASK_NORTH) != 0)				// North has a restriction
		{
			if (MeetsRestriction(0, &restriction.restrict[RESTRICT_NORTH], assignment) == false)
			{
				if (dl != NULL)											// do not swap any hands when analyzing the play
					goto RETRY;
				if (MeetsRestriction(1, &restriction.restrict[RESTRICT_NORTH], assignment))
				{
					SwapHands(0, 1);
				}
				else if (MeetsRestriction(3, &restriction.restrict[RESTRICT_NORTH], assignment))
				{
					SwapHands(0, 3);
				}
				else
					goto RETRY;
			}
		}
NEXT2:	if (dl != NULL && declarer == DIR_EAST)				// do not test for any east or west restrictions for EW play
			goto NEXT3;
		if ((restriction.mask & RESTRICT_MASK_WEST) != 0)				// West has a restriction
		{
			if (MeetsRestriction(3, &restriction.restrict[RESTRICT_WEST], assignment) == false)
			{
				if (dl != NULL)											// do not swap any hands when analyzing the play
					goto RETRY;
				if ((restriction.mask & RESTRICT_MASK_NORTH) == 0		// no North restriction
				&& MeetsRestriction(0, &restriction.restrict[RESTRICT_WEST], assignment))
				{
					SwapHands(3, 0);
				}
				else if (MeetsRestriction(1, &restriction.restrict[RESTRICT_WEST], assignment))
				{
					SwapHands(3, 1);
				}
				else
					goto RETRY;
			}
		}
		if ((restriction.mask & RESTRICT_MASK_EAST) != 0)	// East has a restriction
		{
			if (MeetsRestriction(1, &restriction.restrict[RESTRICT_EAST], assignment) == false)
			{
				if (dl != NULL)											// do not swap any hands when analyzing the play
					goto RETRY;
				if ((restriction.mask & RESTRICT_MASK_NORTH) == 0		// no north restriction
				&& MeetsRestriction(0, &restriction.restrict[RESTRICT_EAST], assignment))
				{
					SwapHands(1, 0);
				}
				else if ((restriction.mask & RESTRICT_MASK_WEST) == 0	// no west restriction
				&& MeetsRestriction(3, &restriction.restrict[RESTRICT_EAST], assignment))
				{
					SwapHands(1, 3);
				}
				else
					goto RETRY;
			}
		}
	}
NEXT3:
	// place cards into deal struct if we are analyzing the hand
	if (dl != NULL)
	{
		h = min(keep1, keep2);
		for (h ^= 1; h < 4; h += 2)								// use other players cards
		{
			dl->remainCards[h][0] = dl->remainCards[h][1] = dl->remainCards[h][2] = dl->remainCards[h][3] = 0;
			for (int s = 0; s < 4; s++)
			{
				for (int c = 0; c < hands[h].suits[s].count; c++)
				{
					int spot = hands[h].suits[s].spots[c];
					for (int i = 0; i < BackupPlayCount[h]; i++)
					{
						if (BackupPlaySuit[h][i] == s
						&&  BackupPlaySpot[h][i] == spot)		// do not add saved cards
							goto SKIP;
					}
					dl->remainCards[h][s] |= (1 << (spot + 2));
				SKIP:;
				}
			}
		}
	}
	else
		CreateContractList(hWnd);
	return true;
}


void	PrintHand(HWND hWnd)
{
	PDFPrintHand(hWnd, deal, dealer, vulnerable, hands, hcPoints, contract1List, contract2List, parList);
	if (writeHands
	&&	deal <= MAX_HAND_RECORD_COUNT)
	{
		savedVul[deal - 1] = Vulnerable[vulnerable];
		savedDealer[deal-1] = dealer;
		savedHands[deal-1][0] = hands[0];
		savedHands[deal-1][1] = hands[1];
		savedHands[deal-1][2] = hands[2];
		savedHands[deal-1][3] = hands[3];
	}
}


void	WriteHands(HWND hWnd, char * fname)
{
	FILE * file;
	// remove .pdf or other extension and replace with .pbn
	int l = strlen(fname);
	while (l >= 0 && fname[l] != '.')
	{
		l--;
	}
	if (l == 0)								// no suffix to the file so use the whole name minus 4 characters
		l = strlen(fname) - 4;
	fname[l++] = '.';						// add .pbn
	fname[l++] = 'p';						
	fname[l++] = 'b';
	fname[l++] = 'n';
	fname[l] = 0;
	// write all hands to file
	file = fopen(fname, "w");
	if (file != NULL)
	{
		for (int d = 0; d < deal; d++)
		{
			fprintf(file, "[Board \"%d\"]\n", d+1);
			fprintf(file, "[Dealer \"%c\"]\n", GetDealerPBNChar(savedDealer[d]));
			fprintf(file, "[Vulnerable \"%s\"]\n", GetVulPBNString(savedVul[d]));
			fprintf(file, "[Deal \"%c:", GetDealerPBNChar(savedDealer[d]));
			int hh = savedDealer[d];
			for (int h = 0; h < 4; h++)
			{ 
				for (int s = 0; s < 4; s++)
				{
					for (int c = 0; c < savedHands[d][hh].suits[s].count; c++)
					{
						fprintf(file, "%c", SpotString[savedHands[d][hh].suits[s].spots[c]]);
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
		}
		fclose(file);
		return;
	}
	// error dialog
	MessageBox(hWnd, "Cannot open pbn file!", "File error", MB_ICONSTOP | MB_OK | MB_APPLMODAL);
}


int xStart[4] = { 268, 484, 268, 4, };					// N, E, S, W
int yStart[4] = { 4, 248, 500, 248, };
static	RECT HandRects[4] = 
{
	{ 100, 4, 700, 246, },		// north hand area
	{ 484, 248, 750, 498, },	// east hand area
	{ 100, 500, 700, 742, },	// south hand area
	{ 4, 248, 332, 498, },		// west hand area
};
static	RECT DealRect =			{ 650, 4, 740, 60 };
static	RECT CommentsRect =		{ 500, 64, 740, 80 };
static	RECT DealerRect =		{ 4, 4, 70, 20 };
static	RECT DealerLockedRect = { 70, 4, 150, 20 };
static	RECT VulnerableRect	=	{ 4, 24, 70, 40 };
static	RECT VulLockedRect =	{ 70, 24, 150, 40 };
static	RECT RestrictionsRect =	{ 4, 44, 180, 60 };

static	RECT ContractRect =		{ 10, 510, 200, 530 };
static	RECT TrickRect =		{ 10, 530, 200, 550 };
static	RECT NSWonRect =		{ 10, 550, 200, 570 };
static	RECT EWWonRect =		{ 10, 570, 200, 590 };

static	RECT TitlePointRect =	{ 0, 70, 70, 90 };
static	RECT NorthPointRect =	{ 24, 90, 44, 110 };
static	RECT EastPointRect =	{ 44, 110, 64, 130 };
static	RECT SouthPointRect =	{ 24, 130, 44, 150 };
static	RECT WestPointRect =	{ 4, 110, 24, 130 };

static	RECT TitleTPointRect =	{ 90, 70, 160, 90 };
static	RECT NorthTPointRect =	{ 115, 90, 135, 110 };
static	RECT EastTPointRect =	{ 135, 110, 155, 130 };
static	RECT SouthTPointRect =	{ 115, 130, 135, 150 };
static	RECT WestTPointRect =	{ 95, 110, 115, 130 };

static	RECT TitleConcenRect =	{ 180, 70, 250, 90 };
static	RECT NorthConcenRect =	{ 205, 90, 235, 110 };
static	RECT EastConcenRect =	{ 225, 110, 255, 130 };
static	RECT SouthConcenRect =	{ 205, 130, 235, 150 };
static	RECT WestConcenRect =	{ 185, 110, 215, 130 };

static	RECT TitleControlsRect = { 0, 160, 70, 180 };
static	RECT NorthControlsRect = { 24, 180, 44, 200 };
static	RECT EastControlsRect = { 44, 200, 64, 220 };
static	RECT SouthControlsRect = { 24, 220, 44, 240 };
static	RECT WestControlsRect = { 4, 200, 24, 220 };

static	RECT TitleLosersRect =	{ 90, 160, 160, 180 };
static	RECT NorthLosersRect =	{ 115, 180, 135, 200 };
static	RECT EastLosersRect =	{ 135, 200, 155, 220 };
static	RECT SouthLosersRect =	{ 115, 220, 135, 240 };
static	RECT WestLosersRect =	{ 95, 200, 115, 220 };

static	RECT TitleQTricksRect = { 180, 160, 250, 180 };
static	RECT NorthQTricksRect = { 205, 180, 225, 200 };
static	RECT EastQTricksRect =	{ 225, 200, 245, 220 };
static	RECT SouthQTricksRect = { 205, 220, 225, 240 };
static	RECT WestQTricksRect =	{ 185, 200, 205, 220 };

static	RECT HeaderTrickRect =	{ 10, 504, 120, 520 };
static	RECT NoTrumpTrickRect = { 10, 524, 120, 540 };
static	RECT SpadeTrickRect =	{ 10, 544, 120, 560 };
static	RECT HeartTrickRect =	{ 10, 564, 120, 580 };
static	RECT DiamondTrickRect = { 10, 584, 120, 600 };
static	RECT ClubTrickRect =	{ 10, 604, 120, 620 };
static	RECT Contract1Rect =	{ 10, 644, 260, 660 };
static	RECT Contract2Rect =	{ 10, 664, 260, 680 };
static	RECT ParRect =			{ 10, 704, 260, 720 };

RECT		InstructionRect =	{ 199, 280, 482, 500, };		// used for instructions in MiddleRect
RECT		MiddleRect =		{ 199, 245, 482, 498, };		// used to determine if cards are being played
static	int PlayCardX[4] =		{ 305, 410, 305, 200, };		// where played cards go
static	int PlayCardY[4] =		{ 246, 324, 402, 324, };

void DrawCards(HWND hWnd, HDC hdc, PAINTSTRUCT ps)
{
	HDC hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hCards);				// select new object and save the old

	MoveToEx(hdc, MiddleRect.left, MiddleRect.top, NULL);
	LineTo(hdc, MiddleRect.right, MiddleRect.top);
	LineTo(hdc, MiddleRect.right, MiddleRect.bottom);
	LineTo(hdc, MiddleRect.left, MiddleRect.bottom);
	LineTo(hdc, MiddleRect.left, MiddleRect.top);

	// draw text for instructions
	if (hDoneButton != NULL)
	{
		HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		LOGFONT logfont;
		GetObject(hFont, sizeof(LOGFONT), &logfont);
		logfont.lfHeight = 20;
		logfont.lfWeight = 500;		// 400=normal 700=bold 1000=max
		HFONT hNewFont = CreateFontIndirect(&logfont);
		HFONT hOldFont = (HFONT)SelectObject(hdc, hNewFont);
		if (playHand)
			DrawText(hdc, "To play a card use double click\nor drag and drop it into this box.", -1, &InstructionRect, DT_CENTER);
		else if (lockCards)
			DrawText(hdc, "To lock or unlock a card\nclick on the card.", -1, &InstructionRect, DT_CENTER);
		else
			DrawText(hdc, "Move any card from one hand\nto another by dragging and dropping \nit into the other hand.\n\nDone button will be enabled\nwhen all hands have 13 cards.\n\nYou cannot move locked cards.", -1, &InstructionRect, DT_CENTER);
		SelectObject(hdc, hOldFont);									// reset font
	}

	// draw each hand
	for (int h = 0; h < 4; h++)
	{
		if (hideHands[h]
		&&	hDoneButton == NULL)										// do not hide cards when modifying hands
			continue;
		if (playHand)
		{
			if (h == handToPlay)
				SelectObject(hdcMem, hCards);
			else
				SelectObject(hdcMem, hCardsOff);
			for (int s = 0; s < 4; s++)
			{
				int marked = 0;
				int plus = 0;
				int minus = 0;
				if (h == handToPlay)
				{
					int normal;											// normal number of tricks for contract
					if ((playDeclarer & 1) == (h & 1))					// declaring side is playing
						normal = playContract + 6 - ((playDeclarer & 1) == 0 ? NSWon : EWWon);
					else
						normal = 7 - playContract - ((playDeclarer & 1) != 0 ? NSWon : EWWon);
					for (int i = 0; i < optimal.cards; i++)
					{
						if (optimal.suit[i] == s)
						{
							if ((playDeclarer & 1) == (h & 1))			// declaring side is playing
							{
								if (optimal.score[i] > normal)
									plus |= (1 << optimal.rank[i]) | optimal.equals[i];
								else if (optimal.score[i] == normal)
									marked |= (1 << optimal.rank[i]) | optimal.equals[i];
							}
							else
							{
								if (optimal.score[i] > normal)
									minus |= (1 << optimal.rank[i]) | optimal.equals[i];
								else if (optimal.score[i] == normal)
									marked |= (1 << optimal.rank[i]) | optimal.equals[i];
							}
						}
					}
				}
				int pos = 0;
				int mask = 1 << 14;
				for (int c = 12; c >= 0; c--, mask >>= 1)
				{
					if ((dl.remainCards[h][s] & mask) != 0)
					{
						BitBlt(hdc, xStart[h] + pos * XOVERLAP, yStart[h] + s * YOVERLAP, WIDTH, HEIGHT, hdcMem, c * WIDTH, s * HEIGHT, SRCCOPY);
						if ((plus & mask) != 0)
						{
							SelectObject(hdcMem, hCardPlus);
							BitBlt(hdc, xStart[h] + pos * XOVERLAP, yStart[h] + s * YOVERLAP + 28, 16, 16, hdcMem, 0, 0, SRCCOPY);
							if (h == handToPlay)
								SelectObject(hdcMem, hCards);
							else
								SelectObject(hdcMem, hCardsOff);
						}
						else if ((marked & mask) != 0)
						{
							SelectObject(hdcMem, hCardMark);
							BitBlt(hdc, xStart[h] + pos * XOVERLAP, yStart[h] + s * YOVERLAP + 28, 16, 16, hdcMem, 0, 0, SRCCOPY);
							if (h == handToPlay)
								SelectObject(hdcMem, hCards);
							else
								SelectObject(hdcMem, hCardsOff);
						}
						else if ((minus & mask) != 0)
						{
							SelectObject(hdcMem, hCardMinus);
							BitBlt(hdc, xStart[h] + pos * XOVERLAP, yStart[h] + s * YOVERLAP + 28, 16, 16, hdcMem, 0, 0, SRCCOPY);
							if (h == handToPlay)
								SelectObject(hdcMem, hCards);
							else
								SelectObject(hdcMem, hCardsOff);
						}
						pos++;
					}
				}
			}
			SelectObject(hdcMem, hCards);
			if (playedRank[h] >= 0)
				BitBlt(hdc, PlayCardX[h], PlayCardY[h], WIDTH, HEIGHT, hdcMem, playedRank[h] * WIDTH, playedSuit[h] * HEIGHT, SRCCOPY);
		}
		else for (int s = 0; s < 4; s++)
		{
			for (int c = 0; c < hands[h].suits[s].count; c++)
			{
				BitBlt(hdc, xStart[h] + c * XOVERLAP, yStart[h] + s * YOVERLAP, WIDTH, HEIGHT, hdcMem, hands[h].suits[s].spots[c] * WIDTH, s * HEIGHT, SRCCOPY);
				// draw locked cards
				if (hDoneButton != NULL									// draw locked mark only if modifyig or locking cards
				&& (restriction.lockedCards[h][s] & (1 << hands[h].suits[s].spots[c])) != 0)
				{
					SelectObject(hdcMem, hCardLock);
					BitBlt(hdc, xStart[h] + c * XOVERLAP, yStart[h] + s * YOVERLAP + 28, 16, 16, hdcMem, 0, 0, SRCCOPY);
						SelectObject(hdcMem, hCards);
				}
			}
		}
	}
	if (selected.hand >= 0)												// draw selected card
	{
		int h = selected.hand;
		int s = selected.suit;
		int c = selected.spot;
		BitBlt(hdc, selected.x - selected.xOffset, selected.y - selected.yOffset, WIDTH, HEIGHT, hdcMem, c * WIDTH, s * HEIGHT, SRCCOPY);
	}

	char string[120];
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	LOGFONT logfont;
	GetObject(hFont, sizeof(LOGFONT), &logfont);
	logfont.lfHeight = 48;
	logfont.lfWeight = 1000;		// 400=normal 700=bold 1000=max
	HFONT hNewFont = CreateFontIndirect(&logfont);
	HFONT hOldFont = (HFONT)SelectObject(hdc, hNewFont);
	sprintf(string, "%d", deal);
	DrawText(hdc, string, -1, &DealRect, DT_RIGHT | DT_TOP);
	SelectObject(hdc, hOldFont);										// reset font

	DrawText(hdc, DealerString[dealer], -1, &DealerRect, DT_LEFT | DT_TOP);
	if (restriction.dealer != DIR_SYSTEMIC)
		DrawText(hdc, "(locked)", -1, &DealerLockedRect, DT_LEFT | DT_TOP);
	DrawText(hdc, VulString[Vulnerable[vulnerable]], -1, &VulnerableRect, DT_LEFT | DT_TOP);
	if (restriction.vul != VUL_SYSTEMIC)
		DrawText(hdc, "(locked)", -1, &VulLockedRect, DT_LEFT | DT_TOP);
	if (restriction.mask != 0)
		DrawText(hdc, "(hands restrictioned)", -1, &RestrictionsRect, DT_LEFT | DT_TOP);

	if (playHand)
	{
		sprintf(string, "Contract: %d%s-%s", playContract, ContractString[playSuit], DeclarerString[playDeclarer]);
		DrawText(hdc, string, -1, &ContractRect, DT_LEFT | DT_TOP);
		if (NSWon + EWWon < 13)
		{
			sprintf(string, "Trick: %d", 1 + NSWon + EWWon);
			DrawText(hdc, string, -1, &TrickRect, DT_LEFT | DT_TOP);
		}
		sprintf(string, "NS Won: %d", NSWon);
		DrawText(hdc, string, -1, &NSWonRect, DT_LEFT | DT_TOP);
		sprintf(string, "EW Won: %d", EWWon);
		DrawText(hdc, string, -1, &EWWonRect, DT_LEFT | DT_TOP);
	}
	else if (hDoneButton == NULL)
	{
		if (hideHandCounts == false)
		{
			DrawText(hdc, "HCP", -1, &TitlePointRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", hcPoints[0]);
			DrawText(hdc, string, -1, &NorthPointRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", hcPoints[1]);
			DrawText(hdc, string, -1, &EastPointRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", hcPoints[2]);
			DrawText(hdc, string, -1, &SouthPointRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", hcPoints[3]);
			DrawText(hdc, string, -1, &WestPointRect, DT_CENTER | DT_TOP);

			DrawText(hdc, "T. Points", -1, &TitleTPointRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", totalPoints[0]);
			DrawText(hdc, string, -1, &NorthTPointRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", totalPoints[1]);
			DrawText(hdc, string, -1, &EastTPointRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", totalPoints[2]);
			DrawText(hdc, string, -1, &SouthTPointRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", totalPoints[3]);
			DrawText(hdc, string, -1, &WestTPointRect, DT_CENTER | DT_TOP);

			DrawText(hdc, "% Concen.", -1, &TitleConcenRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", concentrated[0]);
			DrawText(hdc, string, -1, &NorthConcenRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", concentrated[1]);
			DrawText(hdc, string, -1, &EastConcenRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", concentrated[2]);
			DrawText(hdc, string, -1, &SouthConcenRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", concentrated[3]);
			DrawText(hdc, string, -1, &WestConcenRect, DT_CENTER | DT_TOP);

			DrawText(hdc, "Controls", -1, &TitleControlsRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", controls[0]);
			DrawText(hdc, string, -1, &NorthControlsRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", controls[1]);
			DrawText(hdc, string, -1, &EastControlsRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", controls[2]);
			DrawText(hdc, string, -1, &SouthControlsRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", controls[3]);
			DrawText(hdc, string, -1, &WestControlsRect, DT_CENTER | DT_TOP);

			DrawText(hdc, "Losers", -1, &TitleLosersRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", losers[0]);
			DrawText(hdc, string, -1, &NorthLosersRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", losers[1]);
			DrawText(hdc, string, -1, &EastLosersRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", losers[2]);
			DrawText(hdc, string, -1, &SouthLosersRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", losers[3]);
			DrawText(hdc, string, -1, &WestLosersRect, DT_CENTER | DT_TOP);

			DrawText(hdc, "Q Tricksx2", -1, &TitleQTricksRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", quickTricks[0]);
			DrawText(hdc, string, -1, &NorthQTricksRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", quickTricks[1]);
			DrawText(hdc, string, -1, &EastQTricksRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", quickTricks[2]);
			DrawText(hdc, string, -1, &SouthQTricksRect, DT_CENTER | DT_TOP);
			sprintf(string, "%d", quickTricks[3]);
			DrawText(hdc, string, -1, &WestQTricksRect, DT_CENTER | DT_TOP);
		}
		if (hideContracts == false
		&&	contract1List[0] != 0)								// if double dummy solver failed then skip these
		{
			DrawText(hdc, "       N  S  E  W", -1, &HeaderTrickRect, DT_LEFT | DT_TOP);
			sprintf(string, "NT: %2d %2d  %2d %2d",
				tableRes.results[0].resTable[4][0], tableRes.results[0].resTable[4][2],
				tableRes.results[0].resTable[4][1], tableRes.results[0].resTable[4][3]);
			DrawText(hdc, string, -1, &NoTrumpTrickRect, DT_LEFT | DT_TOP);
			sprintf(string, "  S: %2d %2d  %2d %2d",
				tableRes.results[0].resTable[0][0], tableRes.results[0].resTable[0][2],
				tableRes.results[0].resTable[0][1], tableRes.results[0].resTable[0][3]);
			DrawText(hdc, string, -1, &SpadeTrickRect, DT_LEFT | DT_TOP);
			sprintf(string, "  H: %2d %2d  %2d %2d",
				tableRes.results[0].resTable[1][0], tableRes.results[0].resTable[1][2],
				tableRes.results[0].resTable[1][1], tableRes.results[0].resTable[1][3]);
			DrawText(hdc, string, -1, &HeartTrickRect, DT_LEFT | DT_TOP);
			sprintf(string, "  D: %2d %2d  %2d %2d",
				tableRes.results[0].resTable[2][0], tableRes.results[0].resTable[2][2],
				tableRes.results[0].resTable[2][1], tableRes.results[0].resTable[2][3]);
			DrawText(hdc, string, -1, &DiamondTrickRect, DT_LEFT | DT_TOP);
			sprintf(string, "  C: %2d %2d  %2d %2d",
				tableRes.results[0].resTable[3][0], tableRes.results[0].resTable[3][2],
				tableRes.results[0].resTable[3][1], tableRes.results[0].resTable[3][3]);
			DrawText(hdc, string, -1, &ClubTrickRect, DT_LEFT | DT_TOP);

			DrawText(hdc, contract1List, -1, &Contract1Rect, DT_LEFT | DT_TOP);
			DrawText(hdc, contract2List, -1, &Contract2Rect, DT_LEFT | DT_TOP);
			DrawText(hdc, parList, -1, &ParRect, DT_LEFT | DT_TOP);
		}
	}
	if (addComments)
	{
		DrawText(hdc, "Comments", -1, &CommentsRect, DT_CENTER | DT_TOP);
	}

	// restore defaults
	SelectObject(hdcMem, hbmOld);										// restore the old object
	DeleteDC(hdcMem);													// delete 												
	DeleteObject(hNewFont);	
}


bool ValidHands()
{
	for (int h = 0; h < 4; h++)
	{
		if (hands[h].suits[0].count 
			+ hands[h].suits[1].count 
			+ hands[h].suits[2].count 
			+ hands[h].suits[3].count != 13)
			return false;
	}
	return true;
}


//
//	find card selected
//	return true if we have a card selected
//
bool	Select(HWND hWnd, int x, int y)
{
	int h, s, c;
	if (selected.hand >= 0)									// card already selected
		return true;
	for (h = 0; h < 4; h++)
	{
		if (playHand && h != handToPlay)					// only accept the active hand
			continue;
		for (s = 3; s >= 0; s--)
		{
			if (playHand)
			{
				int mask = dl.remainCards[h][s];
				for (c = 0; mask != 0; mask >>= 1)			// count number of spots
				{
					if ((mask & 1) != 0)
						c++;								
				}
				for (int spot = 0, mask = 4; c > 0; spot++, mask <<= 1)
				{
					if ((mask & dl.remainCards[h][s]) == 0)
						continue;
					c--;
					int xx = xStart[h] + c * XOVERLAP;
					int yy = yStart[h] + s * YOVERLAP;
					if (xx <= x && x < xx + WIDTH			// we found the card selected
					&&  yy <= y && y < yy + HEIGHT)
					{
						if (PlayHandLegal(h, s) == false)	// we cannot play this suit
						{
							return false;
						}
						selected.hand = h;
						selected.suit = s;
						selected.spot = spot;
						selected.x = x;
						selected.y = y;
						selected.xOffset = x - xx;
						selected.yOffset = y - yy;
						dl.remainCards[h][s] &= ~mask;		// remove from the remaining cards
						PlayHandSelect();
						return true;
					}
				}
			}
			else if (lockCards)
			{
				for (c = hands[h].suits[s].count - 1; c >= 0; c--)
				{
					int xx = xStart[h] + c * XOVERLAP;
					int yy = yStart[h] + s * YOVERLAP;
					if (xx <= x && x < xx + WIDTH			// we found the card selected
					&&  yy <= y && y < yy + HEIGHT)
					{
						int mask = 1 << hands[h].suits[s].spots[c];
						if ((restriction.lockedCards[h][s] & mask) != 0)
							restriction.lockedCount[h]--;
						else
							restriction.lockedCount[h]++;
						restriction.lockedCards[h][s] ^= mask;
						RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
						return true;
					}
				}
			}
			else
			{
				for (c = hands[h].suits[s].count - 1; c >= 0; c--)
				{
					int xx = xStart[h] + c * XOVERLAP;
					int yy = yStart[h] + s * YOVERLAP;
					if (xx <= x && x < xx + WIDTH			// we found the card selected
					&&  yy <= y && y < yy + HEIGHT)
					{
						if ((restriction.lockedCards[h][s] & (1 << hands[h].suits[s].spots[c])) != 0)
							return false;					// we cannot move locked cards
						selected.hand = h;
						selected.suit = s;
						selected.spot = hands[h].suits[s].spots[c];
						selected.x = x;
						selected.y = y;
						selected.xOffset = x - xx;
						selected.yOffset = y - yy;
						while (c < hands[h].suits[s].count - 1)	// move spots down
							hands[h].suits[s].spots[c++] = hands[h].suits[s].spots[c + 1];
						hands[h].suits[s].count--;			// reduce spot count
						return true;
					}
				}
			}
		}
	}
	return false;										// no card selected
}


//
//	drop selected cards into the new location
//	return true if hands have 13 cards each or we are just playing the hand
//
bool	SelectRelease(HWND hWnd, int x, int y)
{
	if (selected.hand >= 0)								// a card is active
	{
		if (playHand)									// playing a hand
		{
			dl.remainCards[selected.hand][selected.suit] |= 1 << (selected.spot + 2);	// put card back into the deal
			if (MiddleRect.left < x && x < MiddleRect.right		// we are in the middle
			&& MiddleRect.top < y && y < MiddleRect.bottom)
			{
				PlayHandCard(hWnd, selected.suit, selected.spot);
			}
			selected.hand = -1;							// clear selected card
			RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
			return true;
		}
		int h;
		for (h = 0; h < 4; h++)
		{
			if (HandRects[h].left <= x && x <= HandRects[h].right
			&&  HandRects[h].top <= y && y <= HandRects[h].bottom)
				break;
		}
		if (h == 4)
			h = selected.hand;							// put it back where it was
		selected.hand = -1;
		int s = selected.suit;
		int c = hands[h].suits[s].count;
		hands[h].suits[s].spots[c] = selected.spot;
		hands[h].suits[s].count++;
		qsort(hands[h].suits[s].spots, hands[h].suits[s].count, sizeof(hands[h].suits[s].spots[0]), CompareSpots);
		RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
	}
	return ValidHands();
}


void	SelectMove(HWND hWnd, int x, int y)
{
	if (selected.hand < 0)
		return;
	selected.x = x;
	selected.y = y;
	RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
}
