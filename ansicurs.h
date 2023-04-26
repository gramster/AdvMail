/*
 * ansicurs.h - a simple set of curses-based screen functions
 *	for Gram's Commander v4.0.
 *
 * These routines provide a simple set of terminal I/O routines.
 * They do not use curses windows. The aim is to provide useable
 * terminal screen I/O in a set of ANSI-C routines. Many curses
 * implementations still have non-ANSI header files, so they cannot
 * be used from C++ directly, while this set of routines can.
 *
 * To support windowing, the origin and clipping region for screen
 * I/O can be set. Note that this is not yet fully implemented!
 *
 * Written by Graham Wheeler, June 1984. gram@aztec.co.za
 * (c) 1994, All Rights Reserved
 *
 */

#ifndef _ANSICURS_H
#define _ANSICURS_H

/* Key constants that GetKey returns. Can't use the curses ones
   as we can't include curses.h from C++ in some implementations */

#define KB_UP		256
#define KB_DOWN		257
#define KB_LEFT		258
#define KB_RIGHT	259
#define KB_HOME		260
#define KB_END		261
#define KB_PGUP		262
#define KB_PGDN		263
#define KB_DELETE	264
#define KB_BACKSPACE	265
#define KB_F0		266
#define KB_FKEY_NUM(n)	((n)-KB_F0)
#define KB_FKEY_CODE(n)	((n)+KB_F0)
#define IS_FKEY(ch)	(ch>=KB_F0 && ch<=KB_FKEY_CODE(7))

#ifdef __cplusplus

extern "C" {
    void StartCurses(void); // call this at startup and after shelling out
    void StopCurses(void); // call this before shelling out
    void QuitCurses(void); // call this the last time
    int GetLines(void);
    int GetColumns(void);
    void ClearScreen(void);
    void Beep(void);
    void RefreshScreen(void);
    void CenterText(int row, char *text);
    void ReverseOn(void);
    void ReverseOff(void);
    void BoldOn(void);
    void BoldOff(void);
    void PutString(int row, int col, char *text);
    void GetCursor(int *r, int *c);
    void SetCursor(int r, int c);
    int GetKey(void);
    void SetOrigin(int r, int c);
    void GetOrigin(int *r, int *c);
    void SetClip(int r, int c);
    void GetClip(int *r, int *c);
    void StartRecord(char *fname);
    void StartReplay(char *fname);
    void StartCapture(char *fname);
}

#else

    void StartCurses(void);
    void StopCurses(void);
    void QuitCurses(void);
    int GetLines(void);
    int GetColumns(void);
    void ClearScreen(void) ;
    void Beep(void);
    void RefreshScreen(void);
    void CenterText(int row, char *text);
    void ReverseOn(void);
    void ReverseOff(void);
    void BoldOn(void);
    void BoldOff(void);
    void PutString(int row, int col, char *text);
    void GetCursor(int *r, int *c);
    void SetCursor(int r, int c);
    int GetKey(void);
    void SetOrigin(int r, int c);
    void GetOrigin(int *r, int *c);
    void SetClip(int r, int c);
    void GetClip(int *r, int *c);
    void StartRecord(char *fname);
    void StartReplay(char *fname);
    void StartCapture(char *fname);
#endif

#endif


