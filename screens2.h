// More screens; these are all related to composing and sending mail

#ifndef SCREENS2_H
#define SCREENS2_H

#include "screens0.h"

// File Type Toggle

class FileTypeToggle : public ToggleField
{
    virtual char *GetText();
public:
    FileTypeToggle(int r_in, int c_in, int w_in,
		     int nextchar_in = KB_FKEY_CODE(3),
		     int prevchar_in = KB_FKEY_CODE(2));
    int GetType();
    ~FileTypeToggle();
};

// Popup for distribution list merging

class DListPopup : public OMListBoxPopup
{
protected:
    int listref_out;
    long itemnum_out;
    virtual void HandleKey(int ch, int &done, int &rtn);
    virtual void HandleFKey(int ch, int &done, int &rtn);
    virtual void GetItems();
    virtual int NextPage();
    virtual int PrevPage();
public:
    DListPopup(Screen *owner_in);
    void GetSelection(int &listref_out, long &itemnum_out);
    virtual ~DListPopup();
};

// Popup for choosing the name search directory

class SearchDirPopup : public OMListBoxPopup
{
    int *types;
    virtual void HandleKey(int ch, int &done, int &rtn);
    virtual void HandleFKey(int ch, int &done, int &rtn);
    virtual void GetItems();
    virtual int NextPage();
    virtual int PrevPage();
public:
    SearchDirPopup(Screen *owner_in);
    ~SearchDirPopup();
};

class AddressField : public DataField
{
protected:
    Screen *owner; // kludge; need to paint screen after help
    virtual int  IsValid();
    int Error(char *msg);
public:
    AddressField(Screen *owner_in, int r_in, int c_in, int w_in,
			unsigned cancelkeys_in);
    virtual ~AddressField();
};

class AddressPopup : public OMListBoxPopup
{
    char **names;
    int namecnt;
    virtual void HandleKey(int ch, int &done, int &rtn);
    virtual void HandleFKey(int ch, int &done, int &rtn);
    virtual void GetItems();
    virtual int NextPage();
    virtual int PrevPage();
public:
    AddressPopup(Screen *owner_in, int cnt_in, char **names_in);
    char *GetSelection();
    virtual ~AddressPopup();
};

class AddressPage
{
    OMAddress **addresses;
    int *addrtyp; // TO, CC, BCC or FROM
    int addrcnt;
    AddressPage *next;
    AddressPage *prev;
    friend class EnterDistListScreen;
    friend class NicknameScreen;
public:
    AddressPage(int addrcnt_in);
    ~AddressPage();
    OMAddress *GetAddress(int anum);
    int GetAddrType(int anum);
    void SetAddress(int anum, OMAddress *addr);
    void SetAddrType(int anum, int typ);
    AddressPage *NewPage();
    AddressPage *NextPage();
    AddressPage *PrevPage();
    int IsFull();
    int IsEmpty();
    int PageSize();
};

class EnterDistListScreen : public OMFormScreen
{
protected:
    int pagesize;
    unsigned novalidate;
    int changed;
    AddressPage firstpage, *apage;
    virtual void GetItems();
    virtual Screen *HandleKey(int ch, int &done);
    void SyncAddresses();
    void UpdateTypeLabel(int lnum);
    void ToggleTypes();
    void MergeOpenDListTF(int invert, char *pat = 0);
    void MergeDList(FolderItem* fi, int invert = 0, char *pat = 0);
    void MergeDList(char* fname, int invert = 0, char *pat = 0);
    void MergeList();
    void EmptyDList();
    void ResetChangeFlag();
    int HasDListChanged();
    void AssignNickname();
    int HasAddress(OMAddress *addr);
    void ChangeDirectory();
    void ExtendedAddress();
public:
    EnterDistListScreen(Screen *parent_in, char *title_in, 
		char *prompt_in, int nlabels_in, int nfields_in,
		int offset, int pgsz, unsigned novalidate_in);
    virtual int Run();
    virtual ~EnterDistListScreen();
};

class CreateScreen : public EnterDistListScreen
{
    long msgnum;// list ref of newly created message
    int typfld; // file type field, which seems purposeless
    char *fname;// used to hold compose file filename
    int lref;	// list ref of message being replied to/forwarded, if applic
    long inum;	// item num of message being replied to/forwarded, if applic
    MessageSettings mset; // message settings for new message
    char *quoted; // name of file to use for compose if including original msg
protected:
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int ch, int &done);
    virtual char *GetSubject() = 0;
    virtual int GetType() = 0;
public:
    CreateScreen(Screen *parent_in, char *title_in, 
		char *prompt_in, int nlabels_in, int nfields_in,
		int lref, long inum, int offset, char *quoted = 0);
    virtual ~CreateScreen();
    long MessageNum();
};

class CreateNewScreen : public CreateScreen
{
protected:
    int subjfld;
    virtual char *GetSubject();
    virtual int GetType();
public:
    CreateNewScreen(Screen *parent_in);
    virtual ~CreateNewScreen();
};

class CreateReplyScreen : public CreateScreen
{
protected:
    Message *msg; // to which we are replying; need for dlists
    int subjlbl;
    virtual char *GetSubject();
    virtual int GetType();
    friend class DListToggleField;
public:
    CreateReplyScreen(Screen *parent_in, Message *msg_in, int in_BBarea = 0,
				char *quoted=0);
    virtual ~CreateReplyScreen();
};

class CreateForwardScreen : public CreateScreen
{
protected:
    int subjlbl;
    virtual char *GetSubject();
    virtual int GetType();
public:
    CreateForwardScreen(Screen *parent_in, Message *msg_in, char *quoted=0);
    virtual ~CreateForwardScreen();
};

class EditDistListScreen : public EnterDistListScreen
{
protected:
    int namefld;
    long itemnum;
    Message *msg;
    virtual Screen *HandleFKey(int ch, int &done);
    void ReadDList(FolderItem *fi);
    void Initialise();
public:
    EditDistListScreen(Screen *parent_in, long itemnum_in = 0l);
    EditDistListScreen(Screen *parent_in, Message *msg);
    virtual ~EditDistListScreen();
};

class AutoForwardScreen : public EnterDistListScreen
{
protected:
    char *dlist, *comments;
    int afa, ara, ic, dfm, koc, fprm, fpsm, fccm, rtam, rtum, rtrra;
    int afafld, icfld, dfmfld, kocfld, fprmfld, fpsmfld, fccmfld;
    virtual Screen *HandleFKey(int ch, int &done);
    int SetAction();
public:
    AutoForwardScreen(Screen *parent_in);
    virtual ~AutoForwardScreen();
};

class AutoReplyScreen : public OMFormScreen
{
protected:
    char *reply;
    int afa, ara, ic, dfm, koc, fprm, fpsm, fccm, rtam, rtum, rtrra;
    int arafld, rtamfld, rtumfld, rtrrafld;
    virtual Screen *HandleFKey(int ch, int &done);
    int SetAction();
public:
    AutoReplyScreen(Screen *parent_in);
    void Sanitise();
    virtual ~AutoReplyScreen();
};

class MsgSettingsScreen : public OMFormScreen
{
    int impfld, prifld, sensfld, ackfld;
    int datefld, timefld;
    int apdlfld, aarfld, acfld, andnfld, rmndfld;
    MessageSettings &mset;
protected:
    virtual Screen *HandleFKey(int ch, int &done);
    void GetSettings();
    void SaveSettings();
public:
    MsgSettingsScreen(Screen *parent_in, MessageSettings &mset_in);
    virtual ~MsgSettingsScreen();
};

class ExtendedAddrScreen : public OMFormScreen
{
    int surfld, forfld, inifld, genfld;
    int ac1fld, ac2fld, ac3fld, ac4fld;
    OMAddress *addr, *newaddr;
protected:
    virtual Screen *HandleFKey(int fkeynum, int &done);
    void LoadFields(OMAddress *a);
    void SaveFields(OMAddress *a);
public:
    ExtendedAddrScreen(Screen *parent, OMAddress *addr_in, int canedit = 1);
    virtual ~ExtendedAddrScreen();
};

class X400AddrScreen : public OMFormScreen
{
    int surfld, forfld, inifld, genfld;
    int ac1fld, ac2fld, ac3fld, ac4fld;
    int orgfld, countryfld, admdfld, prmdfld;
    OMAddress *addr;
protected:
    virtual Screen *HandleFKey(int fkeynum, int &done);
    void LoadFields();
    void SaveFields();
public:
    X400AddrScreen(Screen *parent, OMAddress *addr_in, int canedit = 1);
    virtual ~X400AddrScreen();
};

class X400AddrScreen2 : public OMFormScreen
{
    int x121fld, uauafld, telefld;
    int ddat1fld, dda1fld, ddat2fld, dda2fld;
    int ddat3fld, dda3fld, ddat4fld, dda4fld;
    OMAddress *addr;
protected:
    virtual Screen *HandleFKey(int fkeynum, int &done);
    void LoadFields();
    void SaveFields();
public:
    X400AddrScreen2(Screen *parent, OMAddress *addr_in, int canedit = 1);
    virtual ~X400AddrScreen2();
};

class ForeignAddrScreen : public OMFormScreen
{
    int surfld, forfld, inifld, genfld;
    int ac1fld, ac2fld, ac3fld, ac4fld;
    OMAddress *addr;
protected:
    virtual Screen *HandleFKey(int fkeynum, int &done);
    void LoadFields();
    void SaveFields();
public:
    ForeignAddrScreen(Screen *parent, OMAddress *addr_in, int canedit = 1);
    virtual ~ForeignAddrScreen();
};

//------------------------------------------------------------

class NicknameScreen : public OMFormScreen
{
protected:
    int pagesize;
    int changed;
    AddressPage firstpage, *apage;
    virtual void GetItems();
    virtual Screen *HandleKey(int ch, int &done);
    virtual Screen *HandleFKey(int fkeynum, int &done);
    void SyncAddresses();
public:
    NicknameScreen(Screen *parent_in);
    virtual ~NicknameScreen();
};

int ExternalCompose(char *fname, int type);

#endif

