// BridgeTimer.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "BridgeTimer.h"

#define MAX_LOADSTRING  100

const RECT	fullRect = { 10, 10, WINDOW_WIDTH - 10, WINDOW_HEIGHT - 10 };
const RECT	timeRect = { 10, 180, WINDOW_WIDTH - 10, 480 };
const RECT  roundRect = { 10, 480, WINDOW_WIDTH - 10, 560 };

// Global Variables:
HINSTANCE hInst;                                // current instance
TCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hStartPauseButton;
HWND hEndRoundButton;
HWND hAddTimeButton;
HWND hSubTimeButton;
HFONT hButtonFont;


// Forward declarations of functions included in this code module:
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);


// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
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


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BRIDGETIMER));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDC_BRIDGETIMER);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_BRIDGETIMER));

    return RegisterClassEx(&wcex);
}


//***********************************************************************
//*** Center the second window in relation to the desktop ***************
//***********************************************************************
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
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd)
    {
        return FALSE;
    }
    CenterWindow(hWnd);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_BRIDGETIMER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BRIDGETIMER));
    
    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int) msg.wParam;
}


// this unchecks all items except the one active
void CheckOptions(HMENU hMenu, int submenu, int index, int count)
{
    HMENU hSubMenu = GetSubMenu(hMenu, submenu);	
    for (int i = 0; i < count; i++)
    {
        if (index != i)
            CheckMenuItem(hSubMenu, i, MF_BYPOSITION | MF_UNCHECKED);
        else
            CheckMenuItem(hSubMenu, i, MF_BYPOSITION | MF_CHECKED);
    }
}


void ChangeTime(HWND hWnd, HMENU hMenu, int inc)
{
    int oldTime = time;
    if (state == STATE_DONE)                // no change when we are done
        return;
    time += inc;
    if (time > 59 * 60)
        time = 59 * 60;
    if (state == STATE_RUNNING
    &&  oldTime > noNewBoardsTime
    &&  time <= noNewBoardsTime)
        PlaySound(TEXT("nonewboards.wav"), NULL, SND_FILENAME | SND_ASYNC);
    if (time <= 0)
    {
        time = 0;
        KillTimer(hWnd, IDT_GAMETIMER);
        if (state == STATE_MOVING
        ||  state == STATE_PAUSED_MOVING)
        {
            round++;
            EnterState(hMenu, STATE_RUNNING);
            SetWindowText(hStartPauseButton, "Pause Timer");    // in case it was paused
        }
        else if (RoundsRemaining())
        {
            PlaySound(TEXT("endround.wav"), NULL, SND_FILENAME | SND_ASYNC);
            SetTimer(hWnd, IDT_GAMETIMER, 1000, (TIMERPROC)NULL);
            EnterState(hMenu, STATE_MOVING);
            SetWindowText(hStartPauseButton, "Pause Timer");    // in case it was paused
            SetTimer(true);
        }
        else
        {
            EnterState(hMenu, STATE_DONE);
            EnableWindow(hStartPauseButton, false); // disable when game over
            EnableWindow(hEndRoundButton, false);   // disable when game over
            EnableWindow(hAddTimeButton, false);    // disable when game over
            EnableWindow(hSubTimeButton, false);    // disable when game over
        }
        RedrawWindow(hWnd, &fullRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ERASENOW);
    }
    if (state == STATE_RUNNING
    ||  state == STATE_MOVING)
        SetTimer(hWnd, IDT_GAMETIMER, 1000, (TIMERPROC)NULL);
}


//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HMENU hMenu = GetMenu(hWnd);
    HMENU hSubMenu;
    int wmId, wmEvent;
    int x;

    switch (message)
    {
    case WM_LBUTTONDOWN:
        break;
    case WM_RBUTTONUP:
        break;
    case WM_CREATE:
        // load game defaults
        LoadGameData();
        StartNewSession(hWnd);
        hSubMenu = GetSubMenu(hMenu, SUBMENU_ROUNDS);
        CheckMenuItem(hSubMenu, roundsIndex, MF_BYPOSITION | MF_CHECKED);
        hSubMenu = GetSubMenu(hMenu, SUBMENU_BOARDS);
        CheckMenuItem(hSubMenu, boardsIndex, MF_BYPOSITION | MF_CHECKED);
        hSubMenu = GetSubMenu(hMenu, SUBMENU_TIME);
        CheckMenuItem(hSubMenu, perBoardIndex, MF_BYPOSITION | MF_CHECKED);
        hSubMenu = GetSubMenu(hMenu, SUBMENU_MOVETIME);
        CheckMenuItem(hSubMenu, moveTimeIndex, MF_BYPOSITION | MF_CHECKED);
        hSubMenu = GetSubMenu(hMenu, SUBMENU_NONEWBOARDS);
        CheckMenuItem(hSubMenu, noNewBoardsIndex, MF_BYPOSITION | MF_CHECKED);
        x = (VISIBLE_WIDTH / 2 - BUTTON_WIDTH) / 2;
        hStartPauseButton = CreateWindow(
            "BUTTON",                                   // Predefined class; Unicode assumed 
            "Start Session",                            // Button text 
            WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  
            x - 50,                                     // x position 
            VISIBLE_HEIGHT - BUTTON_HEIGHT - 10,        // y position 
            BUTTON_WIDTH,                               // Button width
            BUTTON_HEIGHT,                              // Button height
            hWnd,                                       // Parent window
            (HMENU)IDC_START_PAUSE,
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            NULL);   
        hButtonFont = CreateFont(30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_SWISS, "Arial");
        SendMessage(hStartPauseButton, WM_SETFONT, (WPARAM)hButtonFont, TRUE);
        hEndRoundButton = CreateWindow(
            "BUTTON",                                   // Predefined class; Unicode assumed 
            "End Round",                                // Button text 
            WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            VISIBLE_WIDTH / 2 + x + 50,                 // x position 
            VISIBLE_HEIGHT - BUTTON_HEIGHT - 10,        // y position 
            BUTTON_WIDTH,                               // Button width
            BUTTON_HEIGHT,                              // Button height
            hWnd,                                       // Parent window
            (HMENU)IDC_NEXT_ROUND,
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            NULL);
        SendMessage(hEndRoundButton, WM_SETFONT, (WPARAM)hButtonFont, TRUE);
        EnableWindow(hEndRoundButton, false);           // disable until we start
        x = VISIBLE_WIDTH / 2;
        hAddTimeButton = CreateWindow(
            "BUTTON",                                   // Predefined class; Unicode assumed 
            "+30 seconds",                              // Button text 
            WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            x - BUTTON2_WIDTH - 10,                     // x position 
            VISIBLE_HEIGHT - BUTTON2_HEIGHT - 10,       // y position 
            BUTTON2_WIDTH,                              // Button width
            BUTTON2_HEIGHT,                             // Button height
            hWnd,                                       // Parent window
            (HMENU)IDC_ADD_TIME,
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            NULL);
        EnableWindow(hAddTimeButton, false);           // disable until we start
        hSubTimeButton = CreateWindow(
            "BUTTON",                                   // Predefined class; Unicode assumed 
            "-30 seconds",                              // Button text 
            WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            x + 10,                                     // x position 
            VISIBLE_HEIGHT - BUTTON2_HEIGHT - 10,       // y position 
            BUTTON2_WIDTH,                              // Button width
            BUTTON2_HEIGHT,                             // Button height
            hWnd,                                       // Parent window
            (HMENU)IDC_SUB_TIME,
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            NULL);
        EnableWindow(hSubTimeButton, false);           // disable until we start
        break;
    case WM_TIMER:
        switch (wParam)
        {
        case IDT_GAMETIMER:
            if (state == STATE_STARTING)
            {
                SetEndingTime();                        // show end of seesion time
                SetTimer(hWnd, IDT_GAMETIMER, 60000, (TIMERPROC)NULL);
                RedrawWindow(hWnd, &roundRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ERASENOW);
            }
            else
            {
                ChangeTime(hWnd, hMenu, -1);
                RedrawWindow(hWnd, &timeRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ERASENOW);
            }
        }
        break;
    case WM_COMMAND:
        wmId = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDC_SUB_TIME:
            ChangeTime(hWnd, hMenu, -30);
            SetEndingTime();                // reset end time
            SetFocus(hWnd);
            RedrawWindow(hWnd, &fullRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ERASENOW );
            break;
        case IDC_ADD_TIME:
            ChangeTime(hWnd, hMenu, 30);
            SetEndingTime();                // reset end time
            SetFocus(hWnd);
            RedrawWindow(hWnd, &fullRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ERASENOW);
            break;
        case IDC_START_PAUSE:
            if (wmEvent == BN_CLICKED)
            {
                switch (state)
                {
                case STATE_STARTING:
                    EnableWindow(hEndRoundButton, true);        // enable the button now
                    EnableWindow(hAddTimeButton, true);         // enable the button now
                    EnableWindow(hSubTimeButton, true);         // enable the button now
                    SetTimer(hWnd, IDT_GAMETIMER, 1000, (TIMERPROC)NULL);
                    EnterState(hMenu, STATE_RUNNING);
                    SetWindowText(hStartPauseButton, "Pause Timer");
                    RedrawWindow(hWnd, &fullRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ERASENOW);
                    break;
                case STATE_RUNNING:
                    KillTimer(hWnd, IDT_GAMETIMER);
                    EnterState(hMenu, STATE_PAUSED_RUN);
                  	SetWindowText(hStartPauseButton, "Resume Timer");
                    break;
                case STATE_PAUSED_RUN:
                    SetTimer(hWnd, IDT_GAMETIMER, 1000, (TIMERPROC)NULL);
                    EnterState(hMenu, STATE_RUNNING);
                 	SetWindowText(hStartPauseButton, "Pause Timer");
                    break;
                case STATE_MOVING:
                    KillTimer(hWnd, IDT_GAMETIMER);
                    EnterState(hMenu, STATE_PAUSED_MOVING);
                    SetWindowText(hStartPauseButton, "Resume Timer");
                    break;
                case STATE_PAUSED_MOVING:
                    SetTimer(hWnd, IDT_GAMETIMER, 1000, (TIMERPROC)NULL);
                    EnterState(hMenu, STATE_MOVING);
                   	SetWindowText(hStartPauseButton, "Pause Timer");
                    break;
                default:
                case STATE_DONE:
                    break;
                }
                SetFocus(hWnd);
            }
            break;
        case IDC_NEXT_ROUND:
            if (wmEvent == BN_CLICKED)
            {
                KillTimer(hWnd, IDT_GAMETIMER);
                if (RoundsRemaining() == 0)
                {
                    SetEndingTime();                        // set current time as ending time
                    EnterState(hMenu, STATE_DONE);
                    EnableWindow(hStartPauseButton, false); // disable when game over
                    EnableWindow(hEndRoundButton, false);   // disable when game over
                    EnableWindow(hAddTimeButton, false);    // disable when game over
                    EnableWindow(hSubTimeButton, false);    // disable when game over
                }
                else if (state == STATE_RUNNING || state == STATE_PAUSED_RUN)
                {
                    SetTimer(hWnd, IDT_GAMETIMER, 1000, (TIMERPROC)NULL);
                    SetWindowText(hStartPauseButton, "Pause Timer");    // change resume to pause if necessary
                    EnterState(hMenu, STATE_MOVING);       // start timer again
                }
                else  // moving state
                {
                    round++;
                    SetTimer(hWnd, IDT_GAMETIMER, 1000, (TIMERPROC)NULL);
                    SetWindowText(hStartPauseButton, "Pause Timer");    // change resume to pause if necessary
                    state = STATE_MOVING;                   // so that time gets reset to the full value
                    EnterState(hMenu, STATE_RUNNING);       // start timer again
                }
                RedrawWindow(hWnd, &fullRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ERASENOW);
            }
            SetFocus(hWnd);
            break;
        case IDM_ROUNDS4:
        case IDM_ROUNDS5:
        case IDM_ROUNDS6:
        case IDM_ROUNDS7:
        case IDM_ROUNDS8:
        case IDM_ROUNDS9:
        case IDM_ROUNDS10:
        case IDM_ROUNDS11:
        case IDM_ROUNDS12:
        case IDM_ROUNDS13:
        case IDM_ROUNDS14:
        case IDM_ROUNDS15:
        case IDM_ROUNDS16:
            roundsIndex = wmId - IDM_ROUNDS;
            CheckOptions(hMenu, SUBMENU_ROUNDS, roundsIndex, IDM_ROUNDSEND - IDM_ROUNDS);
            SetTimer(true);                             // reset the time to play and last board time 
            RedrawWindow(hWnd, &fullRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ERASENOW);
            break;
        case IDM_BOARDS1:
        case IDM_BOARDS2:
        case IDM_BOARDS3:
        case IDM_BOARDS4:
        case IDM_BOARDS5:
        case IDM_BOARDS6:
            boardsIndex = wmId - IDM_BOARDS;
            CheckOptions(hMenu, SUBMENU_BOARDS, boardsIndex, IDM_BOARDSEND - IDM_BOARDS);
            SetTimer(true);                             // reset the time to play and last board time 
            RedrawWindow(hWnd, &fullRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ERASENOW);
            break;
        case IDM_TIME1:
        case IDM_TIME2:
        case IDM_TIME3:
        case IDM_TIME4:
        case IDM_TIME5:
        case IDM_TIME6:
        case IDM_TIME7:
        case IDM_TIME8:
        case IDM_TIME9:
        case IDM_TIME10:
        case IDM_TIME11:
        case IDM_TIME12:
            if (state == STATE_RUNNING
            || state == STATE_PAUSED_RUN)
                ChangeTime(hWnd, hMenu, ChangeInTime(true, wmId - IDM_TIME));
            perBoardIndex = wmId - IDM_TIME;
            CheckOptions(hMenu, SUBMENU_TIME, perBoardIndex, IDM_TIMEEND - IDM_TIME);
            if (state == STATE_STARTING
            || state == STATE_RUNNING
            || state == STATE_PAUSED_RUN)
                SetTimer(state == STATE_STARTING);  // reset the time to play and last board time 
            RedrawWindow(hWnd, &fullRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ERASENOW);
            break;
        case IDM_MOVETIME1:
        case IDM_MOVETIME2:
        case IDM_MOVETIME3:
        case IDM_MOVETIME4:
        case IDM_MOVETIME5:
        case IDM_MOVETIME6:
        case IDM_MOVETIME7:
        case IDM_MOVETIME8:
        case IDM_MOVETIME9:
            moveTimeIndex = wmId - IDM_MOVETIME;
            CheckOptions(hMenu, SUBMENU_MOVETIME, moveTimeIndex, IDM_MOVETIMEEND - IDM_MOVETIME);
            if (state == STATE_MOVING
            || state == STATE_PAUSED_MOVING)
            {
                ChangeTime(hWnd, hMenu, ChangeInTime(false, wmId - IDM_MOVETIME));
                RedrawWindow(hWnd, &fullRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ERASENOW);
            }
            else
            {
                SetEndingTime();
                RedrawWindow(hWnd, &fullRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ERASENOW);
            }
            break;
        case IDM_NONEWBOARDS1:
        case IDM_NONEWBOARDS2:
        case IDM_NONEWBOARDS3:
        case IDM_NONEWBOARDS4:
        case IDM_NONEWBOARDS5:
        case IDM_NONEWBOARDS6:
            noNewBoardsIndex = wmId - IDM_NONEWBOARDS;
            CheckOptions(hMenu, SUBMENU_NONEWBOARDS, noNewBoardsIndex, IDM_NONEWBOARDSEND - IDM_NONEWBOARDS);
            SetLastBoardTime();                     // reset the last board time 
            RedrawWindow(hWnd, &timeRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ERASENOW);
            break;
        case IDM_NEW_SESSION:
            KillTimer(hWnd, IDT_GAMETIMER);         // kill timer if running
            EnterState(hMenu, STATE_STARTING);
            StartNewSession(hWnd);                     // reset time and last board time
            SetWindowText(hStartPauseButton, "Start Session");
            EnableWindow(hStartPauseButton, true);  // enable at start
            EnableWindow(hEndRoundButton, false);   // disable until we start
            EnableWindow(hAddTimeButton, false);    // disable until we start
            EnableWindow(hSubTimeButton, false);    // disable until we start
            RedrawWindow(hWnd, &fullRect, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_ERASENOW);
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
    case WM_PAINT:
        DrawScreen(hWnd);
        break;
    case WM_DESTROY:
        DeleteObject(hStartPauseButton);
        DeleteObject(hEndRoundButton);
        DeleteObject(hAddTimeButton);
        DeleteObject(hSubTimeButton);
        DeleteObject(hButtonFont);
        SaveGameData();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
