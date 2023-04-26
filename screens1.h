#ifndef SCREENS1_H
#define SCREENS1_H

#include "screens0.h"

class BBAreaScreen : public OMPageableListScreen
{
protected:
    virtual char *FormatItem(FolderItem *fi);
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int fkeynum, int &done);
    virtual void UpdateCount();
public:
    BBAreaScreen(Screen *parent_in);
    virtual ~BBAreaScreen();
};

class BBoardScreen : public OMPageableListScreen
{
    FKeySet8by2 *emptyfkeys, *msgfkeys, *foldfkeys, *basicfkeys;
protected:
    virtual char *FormatItem(FolderItem *fi);
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int fkeynum, int &done);
    virtual void UpdateCount();
public:
    BBoardScreen(Screen *parent_in, int fnum);
    virtual void Paint();
    virtual ~BBoardScreen();
};

class DistListTrayScreen : public OMPageableListScreen
{
protected:
    virtual char *FormatItem(FolderItem *fi);
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int fkeynum, int &done);
    virtual void UpdateCount();
public:
    DistListTrayScreen(Screen *parent_in);
    virtual ~DistListTrayScreen();
};

class FileTrayScreen : public OMPageableListScreen
{
protected:
    virtual char *FormatItem(FolderItem *fi);
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int fkeynum, int &done);
    virtual void UpdateCount();
public:
    FileTrayScreen(Screen *parent_in);
    virtual ~FileTrayScreen();
};

class FolderScreen : public OMPageableListScreen
{
    FKeySet8by2 *emptyfkeys, *msgfkeys, *foldfkeys, *basicfkeys;
protected:
    virtual char *FormatItem(FolderItem *fi);
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int fkeynum, int &done);
    virtual void UpdateCount();
    void RenameFolder();
public:
    FolderScreen(Screen *parent_in, int fnum);
    virtual void Paint();
    virtual ~FolderScreen();
};

class InTrayScreen : public OMPageableListScreen
{
protected:
    virtual char *FormatItem(FolderItem *fi);
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int fkeynum, int &done);
    virtual void UpdateCount();
public:
    InTrayScreen(Screen *parent_in);
    virtual void Paint();
    virtual ~InTrayScreen();
};

class LoginScreen : public OMFormScreen
{
    int retry, uname, sname, passwd;
protected:
    virtual Screen *HandleFKey(int fkeynum, int &done);
public:
    LoginScreen();
    virtual int Run();
    virtual ~LoginScreen();
};

//------------------------------------------------------------

class OpenMsgScreen : public OMPageableListScreen
{
    int subjfld, cntfld;
protected:
    virtual char *FormatItem(FolderItem *fi);
    int IncludeFile();
    int  EditSubject(char *buf);
    void EditMessageSubject();
    void EditItemSubject();
    Screen *EditItem();
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int fkeynum, int &done) = 0;
    Screen *ReadItem();
public:
    OpenMsgScreen(Screen *parent_in, int mnum, int nlbls, int nflds);
    virtual void Paint();
    virtual int Run();
    virtual ~OpenMsgScreen();
};

class OpenROMsgScreen : public OpenMsgScreen
{
 protected:
    virtual Screen *HandleFKey(int fkeynum, int &done) = 0;
 public:
    OpenROMsgScreen(Screen *parent_in, int inum);
    virtual void Paint();
    virtual ~OpenROMsgScreen();
};

class OpenInMsgScreen : public OpenROMsgScreen
{
 protected:
    virtual Screen *HandleFKey(int fkeynum, int &done);
 public:
    OpenInMsgScreen(Screen *parent_in, int inum);
    virtual void Paint();
    ~OpenInMsgScreen();
};

class OpenFiledMsgScreen : public OpenROMsgScreen
{
 protected:
    FKeySet8by2 *dlfkeys, *msgfkeys, *basicfkeys;
    virtual Screen *HandleFKey(int fkeynum, int &done);
 public:
    OpenFiledMsgScreen(Screen *parent_in, int inum);
    virtual void Paint();
    ~OpenFiledMsgScreen();
};

class OpenWasteMsgScreen : public OpenInMsgScreen
{
 public:
    OpenWasteMsgScreen(Screen *parent_in, int inum);
    ~OpenWasteMsgScreen();
};

class OpenRWMsgScreen : public OpenMsgScreen
{
    int prifld, sensfld, ackfld;
 protected:
    virtual Screen *HandleFKey(int fkeynum, int &done) = 0;
    void EditMessageSettings();
    void DeleteChild();
 public:
    OpenRWMsgScreen(Screen *parent_in, int inum);
    virtual int Run();
    virtual void Paint();
    ~OpenRWMsgScreen();
};

class OpenOutMsgScreen : public OpenRWMsgScreen
{
 protected:
    virtual Screen *HandleFKey(int fkeynum, int &done);
 public:
    OpenOutMsgScreen(Screen *parent_in, int inum);
    virtual void Paint();
    ~OpenOutMsgScreen();
};

class OpenNewMsgScreen : public OpenRWMsgScreen
{
    int result;
 protected:
    virtual Screen *HandleFKey(int fkeynum, int &done);
 public:
    OpenNewMsgScreen(Screen *parent_in, int inum);
    virtual int Run();
    ~OpenNewMsgScreen();
    virtual void Paint();
    int Result();
};

//----------------------------------------------------

class OutTrayScreen : public OMPageableListScreen
{
protected:
    virtual char *FormatItem(FolderItem *fi);
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int fkeynum, int &done);
    virtual void UpdateCount();
public:
    OutTrayScreen(Screen *parent_in);
    virtual ~OutTrayScreen();
};

class PendingTrayScreen : public OMPageableListScreen
{
protected:
    virtual char *FormatItem(FolderItem *fi);
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int fkeynum, int &done);
    virtual void UpdateCount();
public:
    PendingTrayScreen(Screen *parent_in);
    virtual ~PendingTrayScreen();
};

class ReadItemScreen : public OMCompositePageableScreen
{
protected:
    virtual void GetItems();
    virtual Screen *HandleFKey(int fkeynum, int &done);
public:
    ReadItemScreen(Screen *parent_in, int msgnum);
    virtual ~ReadItemScreen();
};

class ReadMsgScreen : public OMPageableScreen
{
    ViewedMessage *msg;
    Message *realmsg;
protected:
    virtual void GetItems();
    virtual int PrevPage();
    virtual int NextPage();
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int fkeynum, int &done);
    void DeleteMessage();
public:
    ReadMsgScreen(Screen *parent_in, Message *msg_in, int can_reply,	
	int can_forward);
    virtual ~ReadMsgScreen();
};

class FilePathsScreen : public OMFormScreen
{
    int hlpfld, typfld, lnkfld;
protected:
    virtual Screen *HandleFKey(int fkeynum, int &done);
    void SaveConfigElt(int fld, char *fldname);
public:
    FilePathsScreen(Screen *parent_in);
    virtual ~FilePathsScreen();
};

class ReconfigScreen : public OMFormScreen
{
    int usrfld, srvfld, cwfld, prtfld, tabfld;
    int ssfld, polfld, rdlfld, pdlfld, xtrfld;
    int afalbl, aralbl;
protected:
    virtual Screen *HandleFKey(int fkeynum, int &done);
    void GetConfigElt(int fld, int &val);
    void SaveConfigElt(int fld, char *fldname);
    void SaveConfig();
    void SetAutoLabels();
public:
    ReconfigScreen(Screen *parent_in);
    virtual ~ReconfigScreen();
};

class TrackMsgScreen : public OMPageableScreen
{
protected:
    void ClearItems(int from);
    virtual void GetItems();
    virtual Screen *HandleFKey(int fkeynum, int &done);
    virtual int PrevPage();
    virtual int NextPage();
public:
    TrackMsgScreen(Screen *parent_in);
    virtual ~TrackMsgScreen();
};

class UserFolderScreen : public OMScreen
{
    Menu menu;
    Screen *NextScreen();
protected:
    int mustrefresh;
    virtual void GetItems(); // NB must call explicitly as not a PageableScreen
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int fkeynum, int &done);
    int Quit();
public:
    UserFolderScreen(Screen *parent_in);
    virtual void Paint();
    virtual void RefreshContents();
    virtual ~UserFolderScreen();
};

#endif 

