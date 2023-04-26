/*
 * ansicurs.c - a simple set of curses-based screen functions
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

#if __MSDOS__
#include <conio.h>
#include <dos.h>
#else
#include <curses.h>
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "ansicurs.h"

#define curs_set(n) /* need to handle this intelligently for those
			curses libraries that support it. */

#define MYCTRL(ch)	((ch) - 'A'+1) /* CTRL already def'ed on some systems */

#ifndef max
#define max(a,b) 	((a)>(b) ? (a) : (b))
#endif

#if __MSDOS__
  // DOS->curses mapping

#define KEY_UP		0x4800
#define KEY_DOWN	0x5000
#define KEY_LEFT	0x4B00
#define KEY_RIGHT	0x4D00
#define KEY_HOME	0x4700
#define KEY_END		0x4F00
#define KEY_PPAGE	0x4900
#define KEY_NPAGE	0x5100
#define KEY_DC		0x5300
#define KEY_BACKSPACE	0x0008
#define KEY_ENTER	0x000A
#define KEY_F1		0x3B00
#define KEY_F(nm)	(0x3A00 + 256*(nm))
#define LINES		25
#define COLS		80

#define beep()			putch(7)
#define werase(w)		clrscr()
#define refresh()
#define mvaddstr(r, c, s)	do { gotoxy(c+1,r+1); cputs(s); } while(0)
#define move(r,c)		gotoxy(c+1,r+1)
#define keypad(w,n)
#define getch()			rawgetch()
short rawgetch();
#endif

/* auto-test by shell script support */

FILE *replay = NULL, *record = NULL, *capture = NULL;

static int r, c;
static int rorigin, corigin;
static int rclip, cclip; /* column clipping not yet fully done */

void StartCurses(void)
{
    r = c = 0;
#if !__MSDOS__
    if (capture != stdout)
    {
        initscr();
        nonl();
        raw();
        noecho();
        curs_set(0);
    }
#endif
    rclip = LINES;
    cclip = COLS;
    rorigin = corigin = 0;
}

void StopCurses(void)
{
#if __MSDOS__
    textattr(7);
    clrscr();
#else
    if (capture != stdout)
    {
        curs_set(1);
        refresh();
        endwin();
    }
#endif
}

void QuitCurses(void)
{
    StopCurses();
    if (record) fclose(record);
    if (replay) fclose(replay);
    if (capture) fclose(capture);
    record = replay = capture = 0;
}

int GetLines(void)
{
    return LINES;
}

int GetColumns(void)
{
    return COLS;
}

void ClearScreen(void) 
{
    if (capture) fputs("Clear\n", capture);
    if (capture != stdout) werase(stdscr);
    r = c = 0;
}

void Beep(void)
{
    if (capture) fputs("Beep\n", capture);
    if (capture != stdout) beep();
}

void RefreshScreen(void)
{
    if (capture) fputs("Refresh\n", capture);
    if (capture != stdout) refresh();
}

void CenterText(int row, char *text)
{
    int len = strlen(text), col = (COLS-len)/2+1, rr = row-rorigin;
    if (capture) fprintf(capture, "%2d %2d %s\n", row,col,text);
    if (capture != stdout && rr>=0 && rr<rclip && strlen(text)>corigin)
	mvaddstr(rr, max(0, col - corigin), text + corigin);
    r = row;
    c = col+len;
}

void ReverseOn(void)
{
    if (capture) fputs("Reverse on\n", capture);
    if (capture != stdout)
#if __MSDOS__
	textattr(0x71);
#else
	attron(A_REVERSE);
#endif
}

void ReverseOff(void)
{
    if (capture) fputs("Reverse off\n", capture);
    if (capture != stdout)
#if __MSDOS__
	textattr(0x17);
#else
	attroff(A_REVERSE);
#endif
}

void BoldOn(void)
{
    if (capture) fputs("Bold on\n", capture);
    if (capture != stdout)
#if __MSDOS__
	highvideo();
#else
	attron(A_BOLD);
#endif
}

void BoldOff(void)
{
    if (capture) fputs("Bold off\n", capture);
    if (capture != stdout)
#if __MSDOS__
	normvideo();
#else
	attroff(A_BOLD);
#endif
}

void PutString(int row, int col, char *text)
{
    int rr = row-rorigin;
    if (capture) fprintf(capture, "%2d %2d %s\n", row,col,text);
    if (capture != stdout && rr>=0 && rr < rclip && strlen(text)>corigin)
	mvaddstr(rr, max(col-corigin, 0), text+corigin);
    r = row;
    c = col + strlen(text);
}

void SetCursor(int row, int col)
{
    int rr = row-rorigin;
    int cc = col-corigin;
    if (capture) fprintf(capture, "Move %2d %2d\n", row,col);
    if (capture != stdout && rr>=0 && rr<rclip && cc>=0 && cc<COLS)
	move(rr,cc);
    r=row;
    c=col;
}

void GetCursor(int *row, int *col)
{
    *row = r - rorigin;
    *col = c - corigin;
}

typedef struct
{
    char *label;
    int code;
} keymap_t;

static keymap_t keymap[] = 
{
    { "up",	    KB_UP },
    { "down",	    KB_DOWN },
    { "left",	    KB_LEFT },
    { "right",	    KB_RIGHT },
    { "pageup",	    KB_PGUP },
    { "pagedown",   KB_PGDN },
    { "home",	    KB_HOME },
    { "end",	    KB_END },
    { "del",	    KB_DELETE },
    { "delete",	    KB_DELETE },
    { "backspace",  KB_BACKSPACE },
    { "enter",	    '\n' },
    { "tab",	    '\t' },
    { "esc",	    27 },
};

#if !__MSDOS__
static void strlwr(char *s)
{
    while (*s) 
    {
	if (isupper(*s)) *s = tolower(*s);
	s++;
    }
}
#endif

static int BatchKey()
{
    if (!feof(replay))
    {
	int i;
	char buf[82];
	if (fgets(buf,80,replay) == NULL) assert(0);
	buf[strlen(buf)-1] = '\0'; /* strip newline */
	if (buf[1]=='\0') return buf[0];
	strlwr(buf);
	if (buf[0]=='f' && isdigit(buf[1]) && buf[2]=='\0')
	    return KB_FKEY_CODE(buf[1]-'1');
	for (i = 0; i < sizeof(keymap)/sizeof(keymap[0]); i++) 
	{
	    if (strcmp(buf, keymap[i].label)==0)
		return keymap[i].code;
	}
    }
    assert(0);
}

static void RecordKey(int ch)
{
    switch(ch)
    {
    case 9:
	fputs("tab\n", record);
	break;
    case 27:
	fputs("esc\n", record);
	break;
    case '\n':
	fputs("enter\n", record);
	break;
    case KB_UP:
	fputs("up\n", record);
	break;
    case KB_DOWN:
	fputs("down\n", record);
	break;
    case KB_LEFT:
	fputs("left\n", record);
	break;
    case KB_RIGHT:
	fputs("right\n", record);
	break;
    case KB_HOME:
	fputs("home\n", record);
	break;
    case KB_END:
	fputs("end\n", record);
	break;
    case KB_PGUP:
	fputs("pageup\n", record);
	break;
    case KB_PGDN:
	fputs("pagedown\n", record);
	break;
    case KB_DELETE:
	fputs("del\n", record);
	break;
    case KB_BACKSPACE:
	fputs("backspace\n", record);
	break;
    default:
	if (ch >= KB_FKEY_CODE(0) && ch <= KB_FKEY_CODE(7))
	    fprintf(record, "F%d\n", ch - KB_F0 + 1);
	else 
	    fprintf(record,"%c\n", ch);
	break;
    }
}

int GetKey(void)
{
    int ch, fk;
    RefreshScreen();
    if (replay) return BatchKey();
    keypad(stdscr,1);
    ch = getch();
retry:
    switch (ch)
    {
    case 27:
	ch = getch();
	if (ch>='1' && ch<='8')
	    ch = KB_FKEY_CODE(ch-'1');
	else if (ch!=27)
	    goto retry;
	break;
    case KEY_UP:
	ch = KB_UP;
	break;
    case KEY_DOWN:
	ch = KB_DOWN;
	break;
    case KEY_LEFT:
	ch = KB_LEFT;
	break;
    case KEY_RIGHT:
	ch = KB_RIGHT;
	break;
    case KEY_HOME:
	ch = KB_HOME;
	break;
    case KEY_END:
	ch = KB_END;
	break;
    case KEY_PPAGE:
    case MYCTRL('U'):
	ch = KB_PGUP;
	break;
    case KEY_NPAGE:
    case MYCTRL('D'):
	ch = KB_PGDN;
	break;
    case 127:
    case KEY_DC:
	ch = KB_DELETE;
	break;
    case 8:
#if !__MSDOS__
    case KEY_BACKSPACE:
#endif
	ch = KB_BACKSPACE;
	break;
    case '\r':
    case KEY_ENTER:
	ch = '\n';
	break;
    default:
	for (fk = 0; fk < 8; fk++)
	{
	    if (ch == KEY_F(fk+1))
	    {
		ch = KB_FKEY_CODE(fk);
		break;
	    }
	}
	if (fk>=8 && ch > 127) /* discard other non-ANSI keys */
	{
	    ch = getch();
	    goto retry;
	}
    }
    if (record) RecordKey(ch);
    return ch;
}

void SetOrigin(int r, int c)
{
    rorigin = r;
    corigin = c;
}

void GetOrigin(int *r, int *c)
{
    *r = rorigin;
    *c = corigin;
}

void SetClip(int r, int c)
{
    if (r<=0) rclip = LINES + r;
    else rclip = r;
    if (c<=0) cclip = COLS + c;
    else cclip = c;
}

void GetClip(int *r, int *c)
{
    *r = rclip;
    *c = cclip;
}

void StartRecord(char *fname)
{
    record = fopen(fname, "w");
}

void StartReplay(char *fname)
{
    replay = fopen(fname, "r");
}

void StartCapture(char *fname)
{
    capture = fopen(fname, "w");
}

#if __MSDOS__
short rawgetch()
{
    short rtn;
    _AH=0;
    geninterrupt(0x16);
    rtn = _AX;
    if (rtn & 0x007F) return (rtn & 0x007F);
    else return rtn;
}
#endif







