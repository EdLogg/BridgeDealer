========================================================================
    Ed Logg's BridgeDealer Project 
========================================================================

This software is available for anyone to use.  I just ask that you give me credit if you
use any potion of my code or algorithms.  If you do make any fixes or updates I would
appreciate it if you would forard them to me at elogg@sbcglobal.net.

It was written using Visual Studio 2013 and updated to Visual Studio 2026.  
I have not created any makefile for this project.

There is an extensive help section for this app.  It also goes through a long list
of possible uses.  For example:
1. Dealing random hands for many possible uses
2. You can add restrictions to deals to meet any specific conditions
3. You can create pdf recap sheets with any detail added or omitted
4. You can create pbn files for use with an automated dealing machine
5. With a remote partner you can deal the same hands and bid them together
6. Double dummy results are determined for each hand
7. If you want to compare the results of 3NT vs 4 of a major the program
will deal 20 sets of hands for each contract and give you the results
8. You can play the hand like Deep Finesse or any number of other double
dummy programs
9. I am currently working on a single dummy solver.  It currently is not time
efficient but enabled.

This project uses LibHaru to create PDF documents.  It also uses Double Dummy Solver
by Bo Haglund and Soren Hein.  Please see their license agreements.

Changes to the PDF library:
* change float to double for HPDF_REAL
* fix compiler errors due to Windows platform

Changes to DoubleDummySolver:
* fix compiler errors due to Windows platform
* remove memory leak checks because memory is cleaned up when the dll is released
* change Par to use +score or -score 
* add getc(stdin) to the sample code so we can see the results in the console window
* update examples to show the deal and make them easier to understand
* add BridgeDealer and DisplaySolution to sample code 
* Modify SolveBoard to add a callback so we can create solution for the deal.  
  See DoubleDummySolver/doc/EdLoggReadme.txt.

Internal Issues (not necessary for a working program):
* if pdf file is open and we try to write to it we get an exception error which brings up a dialog box.
  I currently use exit(1) to leave BridgeDealer!  I could not find a way to gracefully continue.  
  Contact libharu.org to see if there is a way around this.
* Windows MessageBox dialogs are centered on the whole screen instead of in the cneter of the app window.
  This is a known Windows deficiecy.  I must write my own MessageBox class and use it instead.

New Feature List:
1. Complete a more time effective single dummy analysis - see SingleDummyAnalysis.txt

Bugs:
None that I am aware of!

