#pragma once

#include "resource.h"

#define WIN32_LEAN_AND_MEAN							// Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <mmsystem.h>								// for sounds
#include <Shlobj.h>									// for SHGetKnownFolderPath()
#include <stdio.h>									// required for load and save files

// Link the library for sounds
#pragma comment(lib, "winmm.lib")

#define SAVE_FILE_VERSION	0						// change this when the save file format changes
#define WINDOW_WIDTH		900
#define WINDOW_HEIGHT		700
#define VISIBLE_WIDTH		900
#define VISIBLE_HEIGHT		640						// the menu header takes room
#define BUTTON_WIDTH		200
#define BUTTON_HEIGHT		60
#define BUTTON2_WIDTH		100
#define BUTTON2_HEIGHT		40

enum State
{
	STATE_STARTING = 0,
	STATE_RUNNING = 1,
	STATE_PAUSED_RUN = 2,
	STATE_MOVING = 3,
	STATE_PAUSED_MOVING = 4,		
	STATE_DONE = 5,
};

// timer.cpp data
extern State state;
extern int roundsIndex;
extern int boardsIndex;
extern int perBoardIndex;
extern int moveTimeIndex;
extern int noNewBoardsIndex;
extern int time;
extern int PreviousTime;		
extern int round;
extern int noNewBoardsTime;
extern int endingTime;

// timer.cpp functions
void StartNewSession(HWND hWnd);
void SetLastBoardTime();
void SetEndingTime();
void SetTimer(bool resetTime);
int RoundsRemaining();
int ChangeInTime(bool gameTime, int newIndex);
void EnterState(HMENU hMenu, State newState);
void DrawScreen(HWND hWnd);
void LoadGameData();
bool SaveGameData();

