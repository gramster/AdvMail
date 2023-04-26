#ifndef SCREEN0_H
#define SCREEN0_H

#include "screen.h"

char *Quote(FolderItem *msg);

//-----------------------------------------------------------------
// Popup list boxes

class OMListBoxPopup : public ListBoxPopup
{
protected:
    char	*helpidx;
    virtual void HandleKey(int ch, int &done, int &rtn);
    virtual void HandleFKey(int ch, int &done, int &rtn);
    virtual int NextPage() = 0;
    virtual int PrevPage() = 0;
    virtual void GetItems() = 0;
public:
    OMListBoxPopup(Screen *owner_in, char *prompt_in, char *hlp_in);
    virtual ~OMListBoxPopup();
};

// Popup for filing items

class FilePopup : public OMListBoxPopup
{
protected:
    int		icnt; // items in folder; whereas `cnt' is folders in folder
    long	*itemnums;
    int 	*posstk, *pgstk, possp; // crude position stack
    int listref_out;
    long itemnum_out;

    virtual void GetItems();
    virtual int NextPage();
    virtual int PrevPage();
    virtual void HandleKey(int ch, int &done, int &rtn);
    virtual void HandleFKey(int ch, int &done, int &rtn);
public:
    FilePopup(Screen *owner_in);
    void GetSelection(int &listref_out, long &itemnum_out);
    virtual ~FilePopup();
};

// The first three screen classes are just so that we can override DrawTitle
// for each of the three corresponding classes in screen.h, without
// having to duplicate it in all the other screen classes.

class OMScreen : public Screen
{
protected:
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int ch, int &done) = 0;
    friend class FilePopup;
public:
    OMScreen(Screen *parent_in, char * title_in, char * prompt_in,
	   int nlabels_in=0);
    virtual void DrawTitle(char *prompt = 0);
    virtual void Paint();
    virtual int Run();
    virtual void RefreshContents();
    virtual ~OMScreen();
};

class OMFormScreen : public FormScreen
{
protected:
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int ch, int &done) = 0;
public:
    OMFormScreen(Screen *parent_in, char * title_in, char * prompt_in,
	   int nlabels_in=0, int nfields_in=0, 
	   int nextchar_in = 0, int prevchar_in = 0, int canedit_in = 1);
    virtual void DrawTitle(char *prompt = 0);
    virtual void Paint();
    virtual int Run();
    virtual ~OMFormScreen();
};

class OMPageableScreen : public PageableScreen
{
protected:
    virtual void GetItems() = 0;
    virtual int PrevPage() = 0;
    virtual int NextPage() = 0;
    virtual int Run();
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int ch, int &done) = 0;
    void Print(int list_ref, long item_num);
    char *GetQuotedPart(FolderItem *msg);
public:
    OMPageableScreen(Screen *parent_in, char *title_in, char *prompt_in,
		   int nlabels_in, int nfields_in, int msglines_in);
    virtual void DrawTitle(char *prompt = 0);
    virtual void Paint();
    virtual void RefreshContents();
    virtual ~OMPageableScreen();    
};

class OMCompositePageableScreen : public OMPageableScreen
{
protected:
    int fnum;		// folder/message item number
    CompositeItem *folder; // composite associated with screen (folder/msg)
    int  cnt;		// number of items/lines in object; 0 if !folderOK
    virtual void GetItems() = 0;
    virtual int PrevPage();
    virtual int NextPage();
    virtual int Run();
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int ch, int &done) = 0;
public:
    OMCompositePageableScreen(Screen *parent_in, char *title_in,
			      char *prompt_in, int nlabels_in, int nfields_in,
			      int msglines_in, int fnum_in);
    virtual void RefreshContents();
    virtual void Paint();
    virtual ~OMCompositePageableScreen();    
};

//----------------------------------------------------------------

class OMPageableListScreen : public OMCompositePageableScreen
{
    char *emptyprompt;	// prompt if page is empty
    int firstline;	// first row of list on screen
protected:
    int mustrefresh;
    int itemnum;
    Menu msgs;		// current page of folder items
    int selected;	// if re-ordering, itemnum of item to be moved
    void GetRange(int &first, int &last);
    virtual void GetItems();
    virtual char *FormatItem(FolderItem *fi) = 0;
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int fkeynum, int &done) = 0;
    void FileItem();
    void CreateArea();
    void CreateFolder();
    void ReorderFolder();
    void DeleteChild();
    void SaveItem(int saveastext);
    void Print(int list_ref = 0, long item_num = 0l);
    int InWaste();
    virtual void UpdateCount();
    void Refresh(int oldpage, int olditemnum);
public:
    OMPageableListScreen(Screen *parent_in, char *title_in, char *prompt_in,
			    int nlabels_in, int nfields_in,
			    int msglines_in, int firstmsgln_in,
			    int fnum_in, char *emptyprompt_in);
    virtual void Paint();
    virtual void RefreshContents();
    virtual ~OMPageableListScreen();    
};

#endif
