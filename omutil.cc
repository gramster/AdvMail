//-------------------------------------------------------------------
// omutil.cc - Useful routines for dealing with common OpenMail API
//	requests. Necessarily ugly...

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <assert.h>

extern "C" {
#include "omapi.h"
}

#include "debug.h"
#include "omutil.h"

char *ServerError = 0;

typedef enum
{
    IN_TRAY_AREA	= -10,
    OUT_TRAY_AREA	= -20,
    PENDING_TRAY_AREA	= -30,
    FILE_CABINET_AREA	= -40,
    WORK_AREA		= -50,
    DIST_LIST_AREA	= -60,
    BULLETIN_AREA	= -70,
    MESSAGE_AREA	= -100,
    REPLY_AREA		= -300,
    NO_DELIV_AREA	= -400
} AREA_TYPE;

static char host_work_file[256];
static OM_ERROR_STATUS errorstatus;
static char buf[UAL_MAX_BUFFER_SIZE]; // working buffer for commands/responses
static char PARM_PTR toks[UAL_MAX_FIELDS]; // working space for reply tokens

void SetServerError(char *msg)
{
    delete [] ServerError;
    ServerError = new char [strlen(msg)+1];
    if (ServerError) strcpy(ServerError, msg);
    Debug1("SetServerError: %s", ServerError);
}

static void FatalError(char *action)
{
    switch (errorstatus.ErrorGrp)
    {
    case UAL_IF_NETWORK:
	printf("Fatal Error: %s\n",action);
	printf("Error Group: %ld  (Network error)\n",
	       (long)(errorstatus.ErrorGrp));
	printf("Error Reason: %ld (%s)\n", (long)(errorstatus.ErrorReason),
		strerror(errorstatus.ErrorReason));
	printf("Error Reason1: %ld\n",(long)(errorstatus.ErrorReason1));
	break;
    case UAL_IF_PROTOCOL:
	printf("Fatal Error: %s\n",action);
	printf("Error Group: %ld  (Protocol error)\n",
	       (long)(errorstatus.ErrorGrp));
	printf("Error Reason: %ld (%s)\n",(long)(errorstatus.ErrorReason),
		strerror(errorstatus.ErrorReason));
	break;
    default:
	printf("Fatal Error: %s\n",action);
	printf("Error Group: %ld\n",(long)(errorstatus.ErrorGrp));
	printf("Error Reason: %ld\n",(long)(errorstatus.ErrorReason));
	break;
    }
    fflush(stdout);
    exit(1);
}

void (*FatalErrorHandler)(char *) = FatalError;

//-------------------------------------------------------------------

static void SendRequest()
{
    if (ual_sendcommand (&errorstatus, buf) != 0)
        (*FatalErrorHandler)("ual_sendcommand failed");
}

static int GetResponse()
{
    if (ual_recvreply(&errorstatus, buf) != 0)
	(*FatalErrorHandler)("ual_recvreply failed");
    return ual_unpackreply(&errorstatus, buf, toks, UAL_MAX_FIELDS);
}

static int Transact()
{
    SendRequest();
    return GetResponse();
}

//-------------------------------------------------------------------

static void PackATTITM(int folder_ref, int list_ref, int to_item = 0)
{
    Debug3("UAL_ATTITM(%d,%d,%d)", folder_ref, list_ref, to_item);
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_ATTITM),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_i16(UAL_ATTITM_C_FROMLIST,	folder_ref),
		    ual_u32(UAL_ATTITM_C_FROMITEM,	(long)0),
		    ual_i16(UAL_ATTITM_C_TOLIST,	list_ref),
		    ual_u32(UAL_ATTITM_C_TOITEM,	to_item),
		    ual_u32(UAL_ATTITM_C_FLAGS,		(long)0),
		    UAL_LAST_ARG);
}

static void PackCANCEL(int list_ref)
{
    Debug1("UAL_CANCEL(%d)", list_ref);
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,	UAL_CANCEL),
		    ual_i16(UAL_COMM_UAREF,     PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,     0),
		    ual_i16(UAL_CANCEL_C_REF,   list_ref),
		    UAL_LAST_ARG);
}

static void PackCHGPWD(char *old_password, char *new_password)
{
    Debug2("UAL_CHGPWD(%s,%s)", old_password, new_password);
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,	UAL_CHGPWD),
		    ual_i16(UAL_COMM_UAREF,     PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,     0),
		    ual_u32(UAL_CHGPWD_C_FLAGS, 0),
		    ual_str(UAL_CHGPWD_C_OLDPWD,old_password),
		    ual_str(UAL_CHGPWD_C_NEWPWD,new_password),
		    UAL_LAST_ARG);
}

static void PackCHKNAM(char *name, int maxmatch, char *fname, long flags,
	int namefd = 0)
{
    Debug4("UAL_CHKNAM(%s,%d,%s,%ld)", name,maxmatch,fname,flags);
    if (namefd < 0) namefd = 0;
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_CHKNAM),
		    ual_i16(UAL_COMM_UAREF,     	PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,     	0),
		    ual_u32(UAL_CHKNAM_C_FLAGS, 	flags),
		    ual_u32(UAL_CHKNAM_C_LIMIT, 	(long)maxmatch),
		    ual_str(UAL_CHKNAM_C_NAME,  	name),
		    ual_str(UAL_CHKNAM_C_ALTSFNAME,	fname),
		    ual_i16(UAL_CHKNAM_C_DIR_REF,	namefd),
		    UAL_LAST_ARG);
}

static void PackCHKLST(char *remotefile)
{
    Debug1("UAL_CHKLST(%s)", remotefile);
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_CHKLST),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_CHKLST_C_FLAGS,         (long)0),
		    ual_str(UAL_CHKLST_C_FILENAME,	remotefile),
		    UAL_LAST_ARG);
}

static void PackCPYITM(int from_list, long from_item, int to_list, long to_item)
{
    Debug4("UAL_CPYITM(%d,%ld,%d,%ld)",from_list,from_item,to_list,to_item);
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_CPYITM),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_i16(UAL_CPYITM_C_FROMLIST,	from_list),
		    ual_u32(UAL_CPYITM_C_FROMITEM,	from_item),
		    ual_i16(UAL_CPYITM_C_TOLIST,	to_list),
		    ual_u32(UAL_CPYITM_C_TOITEM,	to_item),
		    ual_u32(UAL_CPYITM_C_FLAGS,         (long)0),
		    UAL_LAST_ARG);
}

static void PackDELFIL(char *fname)
{
    Debug1("UAL_DELFIL(%s)", fname);
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_DELFIL),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_str(UAL_DELFIL_C_FNAME,		fname),
		    UAL_LAST_ARG);
}

static void PackDELITM(int list_ref, long item_num, long delete_flags = 0l)
{
    Debug3("UAL_DELITM(%d,%ld,%ld)", list_ref, item_num, delete_flags);
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_DELITM),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_i16(UAL_DELITM_C_LISTREF,	list_ref),
		    ual_u32(UAL_DELITM_C_ITEMREF,	item_num),
		    ual_u32(UAL_DELITM_C_FLAGS,         delete_flags),
		    UAL_LAST_ARG);
}

static void PackDESLST()
{
    Debug("UAL_DESLST()");
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_DESLST),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    /* ual_u32(UAL_DESLST_C_FLAGS,   UAL_DESLST_ACTIVE), */
		    UAL_LAST_ARG);
}

static void PackDIRCLS(int handle)
{
    Debug1("UAL_DIRCLS(%d)",handle);
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_DIRCLS),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_i16(UAL_DIRCLS_C_HANDLE,   	handle),
		    UAL_LAST_ARG);
}

static void PackDIRCRT(char *name, char *passwd = 0, int ispublic = 0)
{
    Debug3("UAL_DIRCRT(%s,%s,%d)",name,passwd,ispublic);
    unsigned long typ = ispublic ? UAL_DIR_TYPE_SHARED : UAL_DIR_TYPE_PERSONAL;
    if (passwd)
        ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_DIRCRT),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_DIRCRT_C_TYPE,   	typ),
		    ual_str(UAL_DIRCRT_C_NAME,   	name),
		    ual_str(UAL_DIRCRT_C_PWD,   	passwd),
		    UAL_LAST_ARG);
    else
        ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_DIRCRT),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_DIRCRT_C_TYPE,   	typ),
		    ual_str(UAL_DIRCRT_C_NAME,   	name),
		    UAL_LAST_ARG);
}

static void PackDIRDEL(char *name, int ispublic = 0)
{
    Debug2("UAL_DIRDEL(%s,%d)",name,ispublic);
    unsigned long typ = ispublic ? UAL_DIR_TYPE_SHARED : UAL_DIR_TYPE_PERSONAL;
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_DIRDEL),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_DIRDEL_C_TYPE,   	typ),
		    ual_str(UAL_DIRDEL_C_NAME,   	name),
		    UAL_LAST_ARG);
}

static void PackDIRLST()
{
    Debug("UAL_DIRLST");
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_DIRLST),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_DIRLST_C_FLAGS,   	0),
		    UAL_LAST_ARG);
}

static void PackDIROPN(char *name, char *passwd = 0, int ispublic = 0)
{
    Debug3("UAL_DIROPN(%s,%s,%d)",name,passwd,ispublic);
    unsigned long typ = ispublic ? UAL_DIR_TYPE_SHARED : UAL_DIR_TYPE_PERSONAL;
    if (passwd)
        ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_DIROPN),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_DIROPN_C_TYPE,   	typ),
		    ual_str(UAL_DIROPN_C_NAME,   	name),
		    ual_str(UAL_DIRCRT_C_PWD,   	passwd),
		    UAL_LAST_ARG);
    else
        ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_DIROPN),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_DIROPN_C_TYPE,   	typ),
		    ual_str(UAL_DIROPN_C_NAME,   	name),
		    UAL_LAST_ARG);
}

static void PackENTADD(int handle, char *entry)
{
    Debug2("UAL_ENTADD(%d,%s)", handle, entry);
    ual_packcommand(&errorstatus, buf, 
		    ual_i16(UAL_COMM_ID,		UAL_ENTADD),
		    ual_i16(UAL_COMM_UAREF, 		PCTYPE),
		    ual_i16(UAL_COMM_SEQNO, 		0),
		    ual_u32(UAL_ENTADD_C_FLAGS,		UAL_ENTADD_SERIAL),
		    ual_i16(UAL_ENTADD_C_HANDLE,	handle),
		    ual_str(UAL_ENTADD_C_ENTRY,		entry),
		    UAL_LAST_ARG);
}

static void PackENTDEL(int handle, char *attribs = 0, char *filter = 0,
	long flags = 0)
{
    Debug4("UAL_ENTDEL(%d,%s,%s,%ld)", handle, attribs, filter, flags);
    flags |= UAL_ENTDEL_SERIAL;
    if (filter)
        ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_ENTDEL),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_ENTDEL_C_FLAGS,         flags),
		    ual_i16(UAL_ENTDEL_C_HANDLE,	handle),
		    ual_str(UAL_ENTDEL_C_ATTRIBS,	attribs),
		    ual_str(UAL_ENTDEL_C_FILTER,	filter),
		    UAL_LAST_ARG);
    else
        ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_ENTDEL),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_ENTDEL_C_FLAGS,         flags),
		    ual_i16(UAL_ENTDEL_C_HANDLE,	handle),
		    ual_str(UAL_ENTDEL_C_ATTRIBS,	attribs),
		    UAL_LAST_ARG);
}

static void PackENTMOD(int handle, char *modifier, char *attribs = 0,
	char *filter = 0, long flags = 0)
{
    Debug5("UAL_ENTMOD(%d,%s,%s,%s,%ld)",handle,modifier,attribs,filter,flags);
    flags |= UAL_ENTMOD_SERIAL;
    if (filter)
        ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_ENTMOD),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_ENTMOD_C_FLAGS,         flags),
		    ual_i16(UAL_ENTMOD_C_HANDLE,	handle),
		    ual_str(UAL_ENTMOD_C_ATTRIBS,	attribs),
		    ual_str(UAL_ENTMOD_C_FILTER,	filter),
		    ual_str(UAL_ENTMOD_C_MODIFIER,	modifier),
		    UAL_LAST_ARG);
    else
        ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_ENTMOD),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_ENTMOD_C_FLAGS,         flags),
		    ual_i16(UAL_ENTMOD_C_HANDLE,	handle),
		    ual_str(UAL_ENTMOD_C_ATTRIBS,	attribs),
		    ual_str(UAL_ENTMOD_C_MODIFIER,	modifier),
		    UAL_LAST_ARG);
}

static void PackERRMSG(int err_num, int err_1, int err_2)
{
    Debug3("UAL_ERRMSG(%d,%d,%d)", err_num, err_1, err_2);
    ual_packcommand(&errorstatus, buf, 
		    ual_i16(UAL_COMM_ID,		UAL_ERRMSG),
		    ual_i16(UAL_COMM_UAREF, 		PCTYPE),
		    ual_i16(UAL_COMM_SEQNO, 		0),
		    ual_u32(UAL_ERRMSG_C_FLAGS,		(long)0),
		    ual_i16(UAL_ERRMSG_C_ERRNO,		err_num),
		    ual_i16(UAL_ERRMSG_C_EXTRA1,	err_1),
		    ual_i16(UAL_ERRMSG_C_EXTRA2,	err_2),
		    UAL_LAST_ARG);
}

static void PackEXPITM(int list_ref, long item_num, char *fname,int filetype=-1)
{
    Debug4("UAL_EXPITM(%d,%ld,%s,%d)", list_ref, item_num, fname, filetype);
    long exp_flags = UAL_EXPITM_OVERWRT | UAL_EXPITM_FOR_DLOAD;
    if (filetype < 0)
    {
	exp_flags |= UAL_EXPITM_NOCONV;
	filetype = 0;
    }
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_EXPITM),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_i16(UAL_EXPITM_C_LISTREF,	list_ref),
		    ual_u32(UAL_EXPITM_C_ITEMREF,	item_num),
		    ual_u32(UAL_EXPITM_C_FLAGS,         exp_flags),
		    ual_i16(UAL_EXPITM_C_FILETYPE,	filetype),
		    ual_str(UAL_EXPITM_C_FILENAME,	fname),
		    ual_i16(UAL_EXPITM_C_FILEID,	0),
		    UAL_LAST_ARG);
}

static void PackGETFIL(int fileid, int fromfno = 0, int getsystemcopy = 0)
{
    Debug3("UAL_GETFIL(%d,%d,%d)", fileid, fromfno, getsystemcopy);
    long flags = UAL_GETFIL_OVRWRT;
    if (getsystemcopy) flags |= UAL_GETFIL_SYSTEM;
    if (fromfno)
        ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_GETFIL),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_GETFIL_C_FLAGS,         flags),
		    ual_i16(UAL_GETFIL_C_FROMFID,	fileid),
		    ual_i16(UAL_GETFIL_C_FROMFNO,	fromfno),
		    ual_i16(UAL_GETFIL_C_TOFID,		0),
		    ual_str(UAL_GETFIL_C_TOFNAME,	host_work_file),
		    UAL_LAST_ARG);
    else
        ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_GETFIL),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_GETFIL_C_FLAGS,         flags),
		    ual_i16(UAL_GETFIL_C_FROMFID,	fileid),
		    ual_i16(UAL_GETFIL_C_TOFID,		0),
		    ual_str(UAL_GETFIL_C_TOFNAME,	host_work_file),
		    UAL_LAST_ARG);
}

static void PackINCFIL(int list_ref, int filecode, char *filename, char *subject)
{
    Debug4("UAL_INCFIL(%d,%d,%s,%s)", list_ref, filecode, filename, subject);
    long inc_flags = UAL_INCFIL_DELETE; 
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_INCFIL),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_i16(UAL_INCFIL_C_MSGREF,	list_ref),
		    ual_u32(UAL_INCFIL_C_FLAGS,         inc_flags),
		    ual_i16(UAL_INCFIL_C_TOTYPE,	filecode),
		    ual_i16(UAL_INCFIL_C_FROMTYPE,	filecode),
		    ual_str(UAL_INCFIL_C_FILENAME,	filename),
		    ual_str(UAL_INCFIL_C_SUBJECT,	subject),
		    UAL_LAST_ARG);
}

static void PackINCNAM(char *name, int isfirst, int typ)
{
    Debug3("UAL_INCNAM(%s,%d,%d)", name, isfirst, typ);
    long inc_flags = UAL_INCNAM_USECAT;
    if (isfirst) inc_flags |= UAL_INCNAM_START;
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_INCNAM),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_INCNAM_C_FLAGS,         inc_flags),
		    ual_u32(UAL_INCNAM_C_CAT,           (long)typ),
		    ual_str(UAL_INCNAM_C_NAME,		name),
		    UAL_LAST_ARG);
}

static void PackINIT(char *ua_id_str, char *user)
{
    Debug2("UAL_INIT(%s,%s)", ua_id_str, user);
    ual_packcommand(&errorstatus, buf, 
		    ual_i16(UAL_COMM_ID,		UAL_INIT),
		    ual_i16(UAL_COMM_UAREF, 		PCTYPE),
		    ual_i16(UAL_COMM_SEQNO, 		0),
		    ual_u32(UAL_INIT_C_FLAGS, 		(long)0),
		    ual_str(UAL_INIT_C_UAIDENT,		ua_id_str),
		    ual_i16(UAL_INIT_C_SERVICE,		UAL_INIT_SERVICE_0),
		    ual_str(UAL_INIT_C_CHARSET,		CHAR_SET),
		    ual_str(UAL_INIT_C_USERNAME,	user),
		    UAL_LAST_ARG);
}

static void PackLIST(int list_ref, int start, int count)
{
    Debug3("UAL_LIST(%d,%d,%d)", list_ref, start, count);
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_LIST),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_i16(UAL_LIST_C_LISTREF,         list_ref),
		    ual_u32(UAL_LIST_C_FLAGS,           (long)0),
		    ual_u32(UAL_LIST_C_START,           start),
		    ual_i16(UAL_LIST_C_MAX,             count),
		    ual_u32(UAL_LIST_C_FIELDS,          (long)-1),
		    UAL_LAST_ARG);
}

static void PackMAIL(int list_ref, long item_num, long mail_flags = 0l)
{
    Debug3("UAL_MAIL(%d,%ld,%ld)", list_ref, item_num, mail_flags);
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_MAIL),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_i16(UAL_MAIL_C_LISTREF,         list_ref),
		    ual_u32(UAL_MAIL_C_ITEMREF,         item_num),
		    ual_u32(UAL_MAIL_C_FLAGS,           mail_flags),
		    UAL_LAST_ARG);
}

// this one is overloaded

static void PackMODITM(int list_ref, long item_num, char *subject,
	int mod_fields = UAL_MODITM_F_SUBJECT, long mod_flags = 0l)
{
    Debug5("UAL_MODITM(%d,%ld,%s,%d,%ld)",list_ref,item_num,subject,mod_fields,mod_flags);
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_MODITM),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_i16(UAL_MODITM_C_LISTREF,	list_ref),
		    ual_u32(UAL_MODITM_C_ITEMREF,	item_num),
		    ual_u32(UAL_MODITM_C_FLAGS,         mod_flags),
		    ual_i16(UAL_MODITM_C_FIELDS,	mod_fields),
		    ual_str(UAL_MODITM_C_SUBJECT,	subject),
		    UAL_LAST_ARG);
}

static void PackMODITM(int list_ref, long item_num, long flags, long msg_flags,
			long deferred)
{
    Debug5("UAL_MODITM(%d,%ld,%ld,%ld,%ld)",list_ref,item_num,flags,msg_flags,
		deferred);
    int modflds = UAL_MODITM_F_FLAGS1 | UAL_MODITM_F_MFLAGS | 
    			UAL_MODITM_F_DEFERRED;
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_MODITM),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_i16(UAL_MODITM_C_LISTREF,	list_ref),
		    ual_u32(UAL_MODITM_C_ITEMREF,	item_num),
		    ual_i16(UAL_MODITM_C_FIELDS,	modflds),
		    ual_i32(UAL_MODITM_C_FLAGS1,	flags),
		    ual_i32(UAL_MODITM_C_MSGFLAGS,	msg_flags),
		    ual_i32(UAL_MODITM_C_DEFERRED,	deferred),
		    UAL_LAST_ARG);
}

static void PackMODITM(int list_ref, long item_num)
{
    Debug2("UAL_MODITM(%d,%ld,%s)",list_ref,item_num);
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_MODITM),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_i16(UAL_MODITM_C_LISTREF,	list_ref),
		    ual_u32(UAL_MODITM_C_ITEMREF,	item_num),
		    ual_i16(UAL_MODITM_C_FLAGS,		UAL_MODITM_DELETE),
		    ual_i16(UAL_MODITM_C_FIELDS,	UAL_MODITM_F_FILE),
		    ual_str(UAL_MODITM_C_CONTENT,	host_work_file),
		    UAL_LAST_ARG);
}

static void PackMODUSR()
{
    Debug("UAL_MODUSR()");
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_MODUSR),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_MODUSR_C_FLAGS,		0),
		    ual_u32(UAL_MODUSR_C_FIELDS1,	0),
		    UAL_LAST_ARG);
}

static void PackMODUSR(int wbclear, int tabsize, int readdl, int printdl,
	int xtra, int savemail)
{
    Debug6("UAL_MODUSR(%d,%d,%d,%d,%d,%d)", wbclear, tabsize, readdl, printdl,
			xtra, savemail);
    long flds = UAL_MODUSR_F_WBCLEAR | UAL_MODUSR_F_TABSIZE |
		UAL_MODUSR_F_READDL | UAL_MODUSR_F_PRINTDL |
		UAL_MODUSR_F_MSGATTR | UAL_MODUSR_F_MAILSAVE;
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_MODUSR),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_MODUSR_C_FLAGS,		0),
		    ual_u32(UAL_MODUSR_C_FIELDS1,	flds),
		    ual_i16(UAL_MODUSR_C_WBCLEAR,	wbclear),
		    ual_i16(UAL_MODUSR_C_TABSIZE,	tabsize),
		    ual_i16(UAL_MODUSR_C_READDL,	readdl),
		    ual_i16(UAL_MODUSR_C_PRINTDL,	printdl),
		    ual_i16(UAL_MODUSR_C_MSGATTR,	xtra),
		    ual_i16(UAL_MODUSR_C_MAILSAVE,	savemail),
		    UAL_LAST_ARG);
}

static void PackMVITM(int from_list, long from_item, int to_list, long to_item)
{
    Debug4("UAL_MVITM(%d,%ld,%d,%ld)", from_list, from_item, to_list, to_item);
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_MVITM),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_i16(UAL_MVITM_C_FROMLIST,	from_list),
		    ual_u32(UAL_MVITM_C_FROMITEM,	from_item),
		    ual_i16(UAL_MVITM_C_TOLIST,         to_list),
		    ual_u32(UAL_MVITM_C_TOITEM,         to_item),
		    ual_u32(UAL_MVITM_C_FLAGS,          (long)0),
		    UAL_LAST_ARG);
}

static void PackNEWMSG()
{
    Debug("UAL_NEWMSG()");
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_NEWMSG),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_NEWMSG_C_FLAGS,         0),
		    UAL_LAST_ARG);
}

static void PackORDITM(int ref, long item_num, long after_item)
{
    Debug3("UAL_ORDITM(%d,%ld,%ld)", ref, item_num, after_item);
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_ORDITM),
		    ual_i16(UAL_COMM_UAREF,		PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,		0),
		    ual_i16(UAL_ORDITM_C_LISTREF,	ref),
		    ual_u32(UAL_ORDITM_C_FROMITEM,	item_num),
		    ual_u32(UAL_ORDITM_C_AFTERITEM,	after_item),
		    ual_u32(UAL_ORDITM_C_FLAGS,		0l),
		    UAL_LAST_ARG);
}
    
static void PackPRINT(int list_ref, long item_num, char *printcmd)
{
    Debug3("UAL_PRINT(%d,%ld,%s)", list_ref, item_num, printcmd);
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_PRINT),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_i16(UAL_PRINT_C_LISTREF,	list_ref),
		    ual_u32(UAL_PRINT_C_ITEMREF,	item_num),
		    ual_u32(UAL_PRINT_C_FLAGS,		(long)0),
		    ual_str(UAL_PRINT_C_COMMAND,	printcmd),
		    UAL_LAST_ARG);
}

static void PackPRNLST()
{
    Debug("UAL_PRNLST()");
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_PRNLST),
		    ual_i16(UAL_COMM_UAREF,		PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_PRNLST_C_FLAGS,         UAL_PRNLST_ACTIVE),
		    UAL_LAST_ARG);
}

static void PackPRPFWD(int listref, long itemnum, char *subject,
	int ack_flag = 0, long prep_flags = 0l, long msg_flags = 0l,
	time_t earliest = 0)
{
    Debug7("UAL_PRPFWD(%d,%ld,%s,%d,%ld,%ld,%s)", listref, itemnum, subject,
			ack_flag, prep_flags, msg_flags, ctime(&earliest));
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_PRPFWD),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u16(UAL_PRPFWD_C_LISTREF,       listref),
		    ual_u32(UAL_PRPFWD_C_ITEMREF,       itemnum),
		    ual_u32(UAL_PRPFWD_C_FLAGS,         prep_flags),
		    ual_u32(UAL_PRPFWD_C_MSGFLAGS,	msg_flags),
		    ual_i16(UAL_PRPFWD_C_MSGACKS,	ack_flag),
		    ual_str(UAL_PRPFWD_C_SUBJECT,	subject),
		    ual_u32(UAL_PRPFWD_C_DEFERRED,	earliest),
		    UAL_LAST_ARG);
}

static void PackPRPLST(int list_ref, long item_num)
{
    Debug2("UAL_PRPLST(%d,%ld)", list_ref, item_num);
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_PRPLST),
		    ual_i16(UAL_COMM_UAREF,		PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,		0),
		    ual_i16(UAL_PRPLST_C_LISTREF,	list_ref),
		    ual_u32(UAL_PRPLST_C_ITEMREF,	item_num),
		    ual_u32(UAL_PRPLST_C_FLAGS,		UAL_PRPLST_SET_READ),
		    ual_i16(UAL_PRPLST_C_DEPTH,		1),
		    UAL_LAST_ARG);
}
    
static void PackPRPMSG(int item_type, char *subject, int ack_flag = 0,
	long prep_flags = 0l, long msg_flags = 0l, time_t earliest = 0)
{
    Debug6("UAL_PRPMSG(%d,%s,%d,%ld,%ld,%s)", item_type, subject, ack_flag,
		prep_flags, msg_flags, ctime(&earliest));
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_PRPMSG),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_PRPMSG_C_FLAGS,         prep_flags),
		    ual_i16(UAL_PRPMSG_C_TYPE,          item_type),
		    ual_u32(UAL_PRPMSG_C_MSGFLAGS,	msg_flags),
		    ual_i16(UAL_PRPMSG_C_MSGACKS,	ack_flag),
		    ual_str(UAL_PRPMSG_C_SUBJECT,	subject),
		    ual_u32(UAL_PRPMSG_C_DEFERRED,	earliest),
		    UAL_LAST_ARG);
}

static void PackPRPRD(int list_ref, long item_num, int getAcks)
{
    Debug3("UAL_PRPRD(%d,%ld,%d)", list_ref, item_num, getAcks);
    long flags = getAcks ? (UAL_PRPRD_DL_ACKS|UAL_PRPRD_NO_HDR) : 0l;
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_PRPRD),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_i16(UAL_PRPRD_C_LISTREF,	list_ref),
		    ual_u32(UAL_PRPRD_C_ITEMREF,	item_num),
		    ual_u32(UAL_PRPRD_C_FLAGS,          flags),
		    UAL_LAST_ARG);
}

static void PackPRPRD(int list_ref, long item_num, char *fname, int astext)
{
    Debug4("UAL_PRPRD(%d,%ld,%s,%d)", list_ref, item_num, fname, astext);
    long flags = UAL_PRPRD_TO_FILE | UAL_PRPRD_OVERWRT;
    if (!astext) flags |= UAL_PRPRD_NO_CHRCNV;
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_PRPRD),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_i16(UAL_PRPRD_C_LISTREF,	list_ref),
		    ual_u32(UAL_PRPRD_C_ITEMREF,	item_num),
		    ual_u32(UAL_PRPRD_C_FLAGS,          flags),
		    ual_str(UAL_PRPRD_C_FILENAME,       fname),
		    ual_i16(UAL_PRPRD_C_FILEID,         0),
		    UAL_LAST_ARG);
}

static void PackPRPREP(int listref, long itemnum, char *subject,
	int ack_flag = 0, long prep_flags = 0l, long msg_flags = 0l,
	time_t earliest = 0)
{
    Debug7("UAL_PRPREP(%d,%ld,%s,%d,%ld,%ld,%s)", listref, itemnum, subject,
			ack_flag, prep_flags, msg_flags, ctime(&earliest));
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_PRPREP),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u16(UAL_PRPREP_C_LISTREF,       listref),
		    ual_u32(UAL_PRPREP_C_ITEMREF,       itemnum),
		    ual_u32(UAL_PRPREP_C_DLTYPE,        UAL_PRPREP_UADL),
		    ual_u32(UAL_PRPREP_C_FLAGS,         prep_flags),
		    ual_u32(UAL_PRPREP_C_MSGFLAGS,	msg_flags),
		    ual_i16(UAL_PRPREP_C_MSGACKS,	ack_flag),
		    ual_str(UAL_PRPREP_C_SUBJECT,	subject),
		    ual_u32(UAL_PRPREP_C_DEFERRED,	earliest),
		    UAL_LAST_ARG);
}

static void PackPUTFIL(int fileid, int filenum, int del)
{
    long flags = UAL_PUTFIL_OVRWRT;
    Debug3("UAL_PUTFIL(%d,%d,%d)", fileid,filenum,del);
    if (del)
        ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_PUTFIL),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_PUTFIL_C_FLAGS,         flags|UAL_PUTFIL_DELETE),
		    ual_i16(UAL_PUTFIL_C_TOFID,		fileid),
		    ual_i16(UAL_PUTFIL_C_TOFNO,		filenum),
		    UAL_LAST_ARG);
    else
        ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_PUTFIL),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_PUTFIL_C_FLAGS,         flags),
		    ual_str(UAL_PUTFIL_C_FROMFNAME,	host_work_file),
		    ual_i16(UAL_PUTFIL_C_TOFID,		fileid),
		    ual_i16(UAL_PUTFIL_C_TOFNO,		filenum),
		    UAL_LAST_ARG);
}

static void PackRDLINE(int read_ref, int pgsz, int code)
{
    Debug3("UAL_RDLINE(%d,%d,%d)", read_ref, pgsz, code);
    if (code == 0) code = 2;  // first page
    else if (code < 0) code = 0; // prev page
    else code = 1; // next page
    
    ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_RDLINE),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_i16(UAL_RDLINE_C_READREF,	read_ref),
		    ual_u32(UAL_RDLINE_C_FLAGS,         (long)0),
		    ual_i16(UAL_RDLINE_C_NEXTPAGE,	code),
		    ual_i16(UAL_RDLINE_C_PAGESIZE,	pgsz),
		    UAL_LAST_ARG);
}

static void PackSEARCH(int handle, char *attribs = 0, char *filter = 0,
	long flags = UAL_SEARCH_SERIAL)
{
    Debug4("UAL_SEARCH(%d,%s,%s,%ld)", handle, attribs, filter, flags);
    if (filter)
        ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_SEARCH),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_SEARCH_C_FLAGS,         flags),
		    ual_i16(UAL_SEARCH_C_HANDLE,	handle),
		    ual_str(UAL_SEARCH_C_ATTRIBS,	attribs),
		    ual_str(UAL_SEARCH_C_FILTER,	filter),
		    UAL_LAST_ARG);
    else
        ual_packcommand(&errorstatus, buf,
		    ual_i16(UAL_COMM_ID,		UAL_SEARCH),
		    ual_i16(UAL_COMM_UAREF,             PCTYPE),
		    ual_i16(UAL_COMM_SEQNO,             0),
		    ual_u32(UAL_SEARCH_C_FLAGS,         flags),
		    ual_i16(UAL_SEARCH_C_HANDLE,	handle),
		    ual_str(UAL_SEARCH_C_ATTRIBS,	attribs),
		    UAL_LAST_ARG);
}

static void PackSIGNON(char *user, char *password, char *designame = NULL)
{
    Debug3("UAL_SIGNON(%s,%s,%s)", user, password, designame);
    long signon_flags = UAL_SIGNON_NO_ACKS | UAL_SIGNON_SET_LANG;
    if (designame)
        ual_packcommand(&errorstatus, buf, 
			ual_i16(UAL_COMM_ID,		UAL_SIGNON),
			ual_i16(UAL_COMM_UAREF, 	PCTYPE),
			ual_i16(UAL_COMM_SEQNO, 	0),
			ual_u32(UAL_SIGNON_C_FLAGS,	signon_flags),
			ual_str(UAL_SIGNON_C_USERNAME,	user),
			ual_str(UAL_SIGNON_C_PASSWD,	password),
			ual_u32(UAL_SIGNON_C_USERID,	-1),
               		ual_str(UAL_SIGNON_C_DESIGNATE, designame),
			UAL_LAST_ARG);
    else
        ual_packcommand(&errorstatus, buf, 
			ual_i16(UAL_COMM_ID,		UAL_SIGNON),
			ual_i16(UAL_COMM_UAREF, 	PCTYPE),
			ual_i16(UAL_COMM_SEQNO, 	0),
			ual_u32(UAL_SIGNON_C_FLAGS,	signon_flags),
			ual_str(UAL_SIGNON_C_USERNAME,	user),
			ual_str(UAL_SIGNON_C_PASSWD,	password),
			ual_u32(UAL_SIGNON_C_USERID,	-1),
			UAL_LAST_ARG);
}

static void PackSGNOFF()
{
    Debug("UAL_SGNOFF()");
    ual_packcommand(&errorstatus, buf, 
		    ual_i16(UAL_COMM_ID,		UAL_SGNOFF),
		    ual_i16(UAL_COMM_UAREF, 		PCTYPE),
		    ual_i16(UAL_COMM_SEQNO, 		0),
		    ual_i16(UAL_SGNOFF_C_FLAGS, 	UAL_SGNOFF_TIDY_UP),
		    UAL_LAST_ARG);
}

static void PackSHTDWN()
{
    Debug("UAL_SHTDWN()");
    ual_packcommand(&errorstatus, buf, 
		    ual_i16(UAL_COMM_ID,		UAL_SHTDWN),
		    ual_i16(UAL_COMM_UAREF, 		PCTYPE),
		    ual_i16(UAL_COMM_SEQNO, 		0),
		    UAL_LAST_ARG);
}

//-----------------------------------------------------------------

static int CheckErrors()
{
    int err_num = atoi(toks[UAL_REP_ERROR]);
    if (err_num != 0)
    {
	char errtext1[UAL_MAX_BUFFER_SIZE];
	char errtext2[UAL_MAX_BUFFER_SIZE];
	int err_1 = atoi(toks[UAL_REP_ERR1]);
	int err_2 = atoi(toks[UAL_REP_ERR2]);
	// send UAL command to get error text
	PackERRMSG(err_num, err_1, err_2);
	errtext1[0] = '\0';
	errtext2[0] = '\0';

	if (Transact() != 0)
	    (*FatalErrorHandler)("ERRMSG failed");             
	else
	{
	    if (atoi(toks[UAL_REP_ERROR]) == 0)
	    {
		(void)strcpy(errtext1,toks[UAL_ERRMSG_R_ERRTEXT1]);
		(void)strcpy(errtext2,toks[UAL_ERRMSG_R_ERRTEXT2]);
	    }
	}
	if (errtext1[0] == '\0')
	{
	    switch (err_num)
	    {
	    case UAL_ERR_NAME_INVALID:
	    case UAL_ERR_ADDR_INVALID:
	    case UAL_ERR_NAME_NOT_KNOWN:
	    case UAL_ERR_NAME_NOT_UNIQUE:
		(void)strcpy(errtext1, "Name not known");
		break;
	    default:
		if (err_1 != 0 || err_2 != 0)
		    (void)sprintf(errtext1,
				  "Server Error: %d  (Reason = %d.%d)",
				  err_num, err_1, err_2);
		else
		    (void)sprintf(errtext1, "Server Error: %d", err_num);
		break;
	    }
	    errtext2[0] = '\0';
	}
	SetServerError(errtext1);
    }
    return err_num;
}

//-----------------------------------------------------------------

static int lasterr; // kludge

static int DoPackedCommand(char *typ)
{
    if (Transact() == 0)
        return (lasterr = CheckErrors());
    return -1;
}

//-----------------------------------------------------------------
// Transaction file handling

// Read a dist list TF and return names in error

static int ReadDListTF(char *fname, char ** &names_out, int maxnames)
{
    int cnt = 0, tfh = xtf_OpenNamedFile(&errorstatus, fname, 0);
    if (tfh>=0)
    {
	char record[1024];
	char **names = new char*[maxnames];
	assert(names);
	while (cnt < maxnames && xtf_ReadRecord(&errorstatus, tfh, record) == 0)
	{
	    switch(xtf_GetINT32(&errorstatus, record, TFF_REC_TYPE))
	    {
	    case TFR_HEADER:
	        if (xtf_GetINT16(&errorstatus,record,TFF_TF_TYPE) !=
			TFV_DIST_LIST_TYPE)
		{
		    delete [] names;
		    names_out = 0;
		    return 0;
		}
		break;
	    case TFR_DL_MATCH:
		int off;
		char *nm = xtf_GetStrPtr(&errorstatus,record,TFF_DL_NAME, &off);
		if (nm != 0)
		{
		    names[cnt] = new char[strlen(nm)+1];
		    if (names[cnt]) strcpy(names[cnt++], nm);
		}
		break;
	    }
	}
	xtf_CloseFile(&errorstatus, tfh);
        if (cnt)
        {
            names_out = new char*[cnt];
	    assert(names_out);
	    for (int i = 0; i < cnt; i++)
		names_out[i] = names[i];
        }
	delete [] names;
    }
    else CheckErrors();
    return cnt;
}

//-----------------------------------------------------------------

int OMConnect(char *server, char *user, char *password, int &list_ref,
    char *omuser)
{
    char ua_id_str[81];
    if (ual_initialise(&errorstatus) ||
        ual_connect(&errorstatus, UAL_CONNECT_DEFAULT, server))
        {
	    SetServerError("Cannot connect to OpenMail server");
	    return -1;
	}
    /* issue INIT and SIGNON commands */
    sprintf(ua_id_str, "Open Mind Solutions Mail Client");
    PackINIT(ua_id_str, user);
    if (DoPackedCommand("INIT") != 0)
	(*FatalErrorHandler)("UAL_INIT failed");
    strcpy(host_work_file,toks[UAL_INIT_R_TEMPFILE]);
    PackSIGNON(user, password);
    if (Transact()!=0 || CheckErrors() != 0)
	return -1;
    list_ref = atoi(toks[UAL_SIGNON_R_UFOLDER]); 
    if (omuser) strcpy(omuser, toks[UAL_SIGNON_R_USERNAME]);
    return 0;
}

int OMDisconnect()
{    
    /* issue SIGNOFF and SHUTDOWN commands */
    PackSGNOFF();
    if (DoPackedCommand("SGNOFF") != 0)
	return -1;
    PackSHTDWN();
    if (DoPackedCommand("SHTDWN") != 0)
	return -1;
    if (ual_disconnect(&errorstatus) != 0 || ual_terminate(&errorstatus) != 0)
    {
	(void)CheckErrors();
	return -1;
    }
    return 0;
}

int OMGetListElt(int &ref, char * &subject, char * &creator, int &type,
		 long &flags, long &msgflags,
		 long &created, long &received, long &deferred)
{
    char *fmt_creator, item_type[81];
    long rec_flags;
    if (GetResponse() != 0)
	(*FatalErrorHandler)("GetResponse failed");
    if (CheckErrors() != 0)
	return -1;
    fmt_creator = toks[UAL_LIST_R_CREATOR];
    flags = atol(toks[UAL_LIST_R_FLAGS1]);
    ref = atoi(toks[UAL_LIST_R_ITEMREF]);
    creator = new char[strlen(fmt_creator)+1];
    if (creator) strcpy(creator, fmt_creator);
    subject = new char[strlen(toks[UAL_LIST_R_SUBJECT])+1];
    if (subject) strcpy(subject, toks[UAL_LIST_R_SUBJECT]);
    type = atoi(toks[UAL_LIST_R_TYPE]);
    msgflags = atol(toks[UAL_LIST_R_MSGFLAGS]);
    created = atol(toks[UAL_LIST_R_CDATE]);
    received = atol(toks[UAL_LIST_R_RECEIPT]);
    deferred = atol(toks[UAL_LIST_R_DEFERRED]);
    // check if end of list or block 
    int list_stat = atoi(toks[UAL_LIST_R_STATUS]);
    if (list_stat == UAL_LIST_EOLIST  || list_stat == UAL_LIST_EOBATCH)
	return 1; // end of list or block)
    return 0;
}

int OMPrepListPage(const int list_ref, const int first, const int pgsz)
{
    PackLIST(list_ref, first, pgsz);
    SendRequest();
    return 0;
}

int OMPrepList(const int list_ref, const long item_ref, int &newref, int &cnt)
{
    PackPRPLST(list_ref, item_ref);
    if (DoPackedCommand("PRPLST") != 0)
	return -1;
    newref = atoi(toks[UAL_PRPLST_R_NEWLIST]);
    cnt = atoi(toks[UAL_PRPLST_R_NUMITEMS]);
    return 0;
}

int OMReadLine(int &linenum, char * &text)
{
    if (GetResponse() != 0)
	(*FatalErrorHandler)("GetResponse failed");
    if (CheckErrors() != 0)
	return -1;
    linenum = atoi(toks[UAL_RDLINE_R_LINENO]);
    text = new char[strlen(toks[UAL_RDLINE_R_LINE])+1];
    if (text) strcpy(text, toks[UAL_RDLINE_R_LINE]);
    // check if end of list or block 
    return atoi(toks[UAL_RDLINE_R_STATUS]); // more to come?
}

int OMPrepReadPage(const int list_ref, const int pgsz, const int code)
{
    PackRDLINE(list_ref, pgsz, code);
    SendRequest();
    return 0;
}

int OMPrepRead(const int list_ref, const long item_ref, int getAcks,int &newref)
{
    PackPRPRD(list_ref, item_ref, getAcks);
    if (DoPackedCommand("PRPRD") != 0)
	return -1;
    newref = atoi(toks[UAL_PRPRD_R_READREF]);
    return 0;
}

int OMCancel(const int list_ref)
{
    PackCANCEL(list_ref);
    if (DoPackedCommand("CANCEL") != 0)
	return -1;
    return 0;
}

int OMSave(const int list_ref, const long item_ref, char *fname, int astext)
{
    PackPRPRD(list_ref, item_ref, host_work_file, astext);
    if (DoPackedCommand("PRPRD") != 0)
	return -1;
    int rtn = 0;
    if (OMReceiveFile(fname, host_work_file, !astext) != 0)
        rtn = -1;
    return 0;
}

int OMPrint(const int list_ref, const long item_ref, char *cmd)
{
    PackPRINT(list_ref, item_ref, cmd);
    if (DoPackedCommand("PRINT") != 0)
	return -1;
    return 0;
}

int OMDelete(const int list_ref, const long item_ref)
{
    PackDELITM(list_ref, item_ref);
    if (DoPackedCommand("DELETE") != 0)
	return -1;
    return 0;
}

char *OMFolderName(const long folder)
{
    switch(folder)
    {
    case 1:
	return "IN";
    case 2:
	return "OUT";
    case 3:
	return "PENDING";
    case 4:
	return "FILE";
    case 5:
	return "DISTLIST";
    case 6:
	return "BBS";
    default:
	return "UNKNOWN";
    }
}

int OMSendFile(char *local, char *remote, int isbinary)
{
    Debug3("ual_sendfile(%s,%s,%d)", local, remote, isbinary);
    if (ual_sendfile(&errorstatus, local, remote, isbinary?0:1) != 0)
    {
	(void)CheckErrors();
	return -1;
    }
    return 0;
}

int OMReceiveFile(char *local, char *remote, int isbinary)
{
    Debug3("ual_recvfile(%s,%s,%d)", local, remote, isbinary);
    if (ual_recvfile(&errorstatus, local, remote, isbinary?0:1) != 0)
    {
	(void)CheckErrors();
	return -1;
    }
    return 0;
}

int OMNewMail()
{
    PackNEWMSG();
    if (DoPackedCommand("NEWMSG") != 0)
	return -1;
    Debug1("NEWMSG returns %s\n", toks[UAL_NEWMSG_R_NEW]);
    return atoi(toks[UAL_NEWMSG_R_NEW]);
}

int OMChangePassword(char *old_password, char *new_password)
{
    PackCHGPWD(old_password, new_password);
    if (DoPackedCommand("CHGPWD") != 0)
	return -1;
    return 0;
}

int OMMoveItem(int srcref, long srcitem, int destref, long destitem)
{
    PackMVITM(srcref, srcitem, destref, destitem);
    if (DoPackedCommand("MVITM") != 0)
	return -1;
    return 0;
}

int OMCopyItem(int srcref, long srcitem, int destref, long destitem)
{
    PackCPYITM(srcref, srcitem, destref, destitem);
    if (DoPackedCommand("CPYITM") != 0)
	return -1;
    return 0;
}

int OMAttachItem(int srcref, int destref, long destitem)
{
    PackATTITM(srcref, destref, destitem);
    if (DoPackedCommand("ATTITM") != 0)
	return 0;
    return atoi(toks[UAL_ATTITM_R_NEWITEM]);
}

int OMCreateComposite(int parentref, char *name, int typ)
{
    PackPRPMSG(typ, name);
    if (DoPackedCommand("PRPMSG") != 0)
	return 0; // 0 is failure for this routine
    int newref = atoi(toks[UAL_PRPMSG_R_LISTREF]);
    PackATTITM(newref,parentref);
    int rtn;
    if (DoPackedCommand("ATTITM") != 0) rtn = 0;
    else rtn = atoi(toks[UAL_ATTITM_R_NEWITEM]);
    OMCancel(newref);
    return rtn;
}

int OMRenameFolder(int listref, long itemnum, char *name)
{
    PackMODITM(listref, itemnum, name);
    if (DoPackedCommand("MODITM") != 0)
	return -1;
    return 0;
}

int OMReorderFolder(int listref, long item, long after_item)
{
    PackORDITM(listref, item, after_item);
    if (DoPackedCommand("ORDITM") != 0)
	return -1;
    return 0;
}

int isfirstrecip = 1;

int OMAddRecipient(char *name, int typ)
{
    PackINCNAM(name, isfirstrecip, typ);
    isfirstrecip = 0;
    if (DoPackedCommand("INCNAM") != 0)
	return -1;
    return 0;
}

long OMNewMessageFlags(int pri, int imp, int sens, int allowpublicdls,
			int allowaltrecips, int allowconversion, int allowndns,
			int returnmsg)
{
    long rtn = (pri<<UAL_PREP_PRI_SHIFT) | (sens<<UAL_PREP_SEC_SHIFT) |
		(imp << UAL_PREP_IMP_SHIFT);
    if (!allowpublicdls)
	rtn |= UAL_PREP_NORDL;
    if (!allowaltrecips)
	rtn |= UAL_PREP_NOALT;
    if (!allowconversion)
	rtn |= UAL_PREP_NOCONVERT;
    if (!allowndns)
	rtn |= UAL_PREP_NO_DN_REQ;
    if (returnmsg)
	rtn |= UAL_PREP_ROC;
    return rtn;
}

long OMMessageFlags(int pri, int imp, int sens, int acklvl, int allowndns)
{
    static int ackvals[] = { 0, 4, 7, 9 };
    long rtn =  (pri<<UAL_FLAGS_PRI_SHIFT) |
		(sens<<UAL_FLAGS_SEC_SHIFT) |
		(imp << UAL_FLAGS_IMP_SHIFT) |
		(ackvals[acklvl]<<UAL_FLAGS_RACK_SHIFT);
    if (allowndns == 0)
	rtn |= UAL_FLAGS_NO_DN_REQ;
    return rtn;
}

long OMFlags(int allowpublicdls, int allowaltrecips, int allowconversions,
		int returnmsg)
{
    long rtn = 0l;
    if (allowpublicdls == 0)
	rtn |= UAL_LIST_NO_DL_EXP;
    if (allowaltrecips == 0)
	rtn |= UAL_LIST_NO_ALT_RECIP;
    if (allowconversions == 0)
	rtn |= UAL_LIST_NOCONVERT;
    if (returnmsg)
	rtn |= UAL_LIST_ROC;
    return rtn;
}


int OMModifyMessage(int listref, long itemnum, long flags, long msgflags,
	long deferred_date)
{
    PackMODITM(listref, itemnum, flags, msgflags, deferred_date);
    if (DoPackedCommand("MODITM") != 0)
	return -1;
    return 0;
}

static int ackflagvals[] = { 0, 4, 7, 9 };

int OMCreateMessage(char *subject, long msgflags, int acklvl, time_t earliest)
{
    isfirstrecip = 1;
    long flags = UAL_PRPMSG_USECDL|UAL_PRPMSG_MSGACKS;
    if (acklvl<0 || acklvl>3) acklvl = 0; // sanity
    PackPRPMSG(-100, subject, ackflagvals[acklvl], flags, msgflags, earliest);
    if (DoPackedCommand("PRPMSG") != 0)
	return 0; // 0 is failure for this routine
    return atoi(toks[UAL_PRPMSG_R_LISTREF]);
}

int OMCreateReply(int listref, long itemnum, char *subject, long msgflags,
	int acklvl, time_t earliest)
{
    isfirstrecip = 1;
    long flags = UAL_PRPREP_USECDL|UAL_PRPREP_MSGACKS;
    if (acklvl<0 || acklvl>3) acklvl = 0; // sanity
    PackPRPREP(listref, itemnum, subject, ackflagvals[acklvl],
		flags, msgflags, earliest);
    if (DoPackedCommand("PRPREP") != 0)
	return 0; // 0 is failure for this routine
    return atoi(toks[UAL_PRPREP_R_LISTREF]);
}

int OMCreateForward(int listref, long itemnum, char *subject, long msgflags,
	int acklvl, time_t earliest)
{
    isfirstrecip = 1;
    long flags = UAL_PRPFWD_USECDL|UAL_PRPFWD_MSGACKS;
    if (acklvl<0 || acklvl>3) acklvl = 0; // sanity
    PackPRPFWD(listref, itemnum, subject, ackflagvals[acklvl],
		flags, msgflags, earliest);
    if (DoPackedCommand("PRPFWD") != 0)
	return 0; // 0 is failure for this routine
    return atoi(toks[UAL_PRPFWD_R_LISTREF]);
}

int OMAttachFile(int msgref, char *fname, char *descrip, int ftype)
{
    if (OMSendFile(fname, host_work_file, ftype==0) != 0)
	return -1;
    PackINCFIL(msgref, ftype, host_work_file, descrip ? descrip : fname);
    if (DoPackedCommand("INCFIL") != 0)
	return -1;
    return 0;
}

int OMMailMessage(int listref, long itemnum, int musttrack)
{
    int wbclear, tabsize, readdl, printdl, xtra, savemail = 0;
    (void)OMGetUserConfig(wbclear, tabsize, readdl, printdl, xtra, savemail);
    long flags = 0;
    if (savemail) flags |= UAL_MAIL_KEEP_WB;
    if (musttrack) flags |= UAL_MAIL_KEEP;
    PackMAIL(listref, itemnum, flags);
    if (DoPackedCommand("MAIL") != 0)
	return -1;
    return 0;
}

int OMChangeSubject(int listref, long itemnum, char *subject)
{
    return OMRenameFolder(listref, itemnum, subject);
}

int OMDeleteFile(char *fname)
{
    PackDELFIL(fname);
    if (DoPackedCommand("DELFIL") != 0)
	return -1;
    return 0;
}

int OMExport(int listref, long itemnum, char *fname, int typ)
{
    PackEXPITM(listref, itemnum, host_work_file, typ);
    if (DoPackedCommand("EXPITM") == 0)
        if (OMReceiveFile(fname, host_work_file, (typ != 1167)) == 0)
            return 0;
    return -1;
}

int OMReplaceText(int ref, long itemnum, char *fname)
{
    if (OMSendFile(fname, host_work_file, 0) == 0)
    {
        PackMODITM(ref, itemnum);
    	if (DoPackedCommand("MODITM") == 0)
	    return 0;
    }
    return -1;
}

int OMCheckName(char *name_in, int maxmatches, char ** &names_out, long flags,
	int namefd)
{
    char fn[256], buf[1024];
    int rtn;
    names_out = 0;
    strcpy(buf, name_in);
Debug1("Checking name %s\n", name_in);
    // Convert wildcards to ASCII 31. We put in surrounding space in
    // case user hasn't.
    for (char *ip = buf; *ip; ip++)
	if (*ip == '*') *ip = (char)31;
    PackCHKNAM(buf, maxmatches, host_work_file, flags, namefd);
    if (DoPackedCommand("CHKNAM") != 0)
    {
	if (lasterr == UAL_ERR_NAME_NOT_UNIQUE)
	{
	    strcpy(buf, tmpnam(0));
Debug1("Not unique; getting alternatives in tmp file %s", buf);
	    if (OMReceiveFile(buf, host_work_file, 1)==0)
	    {
	        rtn = ReadDListTF(buf, names_out, maxmatches);
	        delete [] ServerError;
	        ServerError = 0;
		unlink(buf);
	    }
	    else rtn = -1;
	}
	else rtn = -1;
    }
    else
    {
Debug3("Unique; checked: %s\n   Numalts %d\n  Altsfil %s\n",
	toks[UAL_CHKNAM_R_NAME], atoi(toks[UAL_CHKNAM_R_NUMALTS]),
	toks[UAL_CHKNAM_R_ALTSFILE]);
        names_out = new char*[1];
	assert(names_out);
	names_out[0] = new char [strlen(toks[UAL_CHKNAM_R_NAME])+1];
	assert(names_out[0]);
#if 0
	strcpy(names_out[0], toks[UAL_CHKNAM_R_NAME]);
#else
	// Sometimes we get names back with the wildchar still in. This
	// strips it out...
	char *op = names_out[0], *ip = toks[UAL_CHKNAM_R_NAME];
	while (*ip)
	{
	    if (*ip!=(char)31) *op++ = *ip;
	    ip++;
	}
	*op = '\0';
#endif
	rtn = 1;
    }
    //OMDeleteFile(fn);
    return rtn;
}

/*
 * List message flags 
 */

int OMPriority(long msgflags)
{
    return (msgflags & UAL_FLAGS_PRI_MASK)>>UAL_FLAGS_PRI_SHIFT;
}

int OMSensitivity(long msgflags)
{
    return (msgflags & UAL_FLAGS_SEC_MASK)>>UAL_FLAGS_SEC_SHIFT;
}

int OMRequestedAck(long msgflags)
{
    return (msgflags & UAL_FLAGS_RACK_MASK)>>UAL_FLAGS_RACK_SHIFT;
}

int OMSentAck(long msgflags)
{
    return (msgflags & UAL_FLAGS_SACK_MASK)>>UAL_FLAGS_SACK_SHIFT;
}

int OMImportance(long msgflags)
{
    return (msgflags & UAL_FLAGS_IMP_MASK)>>UAL_FLAGS_IMP_SHIFT;
}

int OMRecipCategory(long msgflags)
{
    return (msgflags & UAL_FLAGS_RECIP_MASK)>>UAL_FLAGS_RECIP_SHIFT;
}

int OMAllowNDNs(long msgflags)
{
    return ((msgflags & UAL_FLAGS_NO_DN_REQ) == 0l);
}

/*
 * List flags 
 */

int OMIsObsolete(long flags)
{
    return ((flags & UAL_LIST_OBSOLETED) != 0l);
}

int OMIsUnread(long flags)
{
    return ((flags & UAL_LIST_UNREAD) != 0l);
}

void OMMarkAsRead(long &flags)
{
    flags &= ~UAL_LIST_UNREAD;
}

int OMIsError(long flags)
{
    return ((flags & UAL_LIST_MSG_ERR) != 0l);
}

int OMHasDistListError(long flags)
{
    return ((flags & UAL_LIST_DL_ERR) != 0l);
}

int OMAllowPublicDLs(long flags)
{
    return ((flags & UAL_LIST_NO_DL_EXP) == 0l);
}

int OMAllowAltRecips(long flags)
{
    return ((flags & UAL_LIST_NO_ALT_RECIP) == 0l);
}

int OMAllowConversions(long flags)
{
    return ((flags & UAL_LIST_NOCONVERT) == 0l);
}

int OMMustReturnMsg(long flags)
{
    return ((flags & UAL_LIST_ROC) != 0l);
}

//---------------------------------------------------------------
// Transaction file creation

static int OMStartRecord(char *buf, int typ)
{
    if (xtf_InitRecord(&errorstatus, buf, typ) != 0)
        if ((lasterr = CheckErrors()) != 0)
	    return -1;
    return 0;
}

static int OMPut16(char *buf, int offset, short val)
{
    if (xtf_PutINT16(&errorstatus, buf, offset, val) != 0)
        if ((lasterr = CheckErrors()) != 0)
	    return -1;
    return 0;
}

static int OMPut32(char *buf, int offset, long val)
{
    if (xtf_PutINT32(&errorstatus, buf, offset, val) != 0)
        if ((lasterr = CheckErrors()) != 0)
	    return -1;
    return 0;
}

static int OMPutString(char *buf, int offset, char *val)
{
    int junk;
    if (xtf_PutString(&errorstatus, buf, offset, &junk, val) != 0)
        if ((lasterr = CheckErrors()) != 0)
	    return -1;
    return junk;
}

static int OMAppendRecord(int tf_fd)
{
    if (xtf_AppendRecord(&errorstatus, tf_fd, buf) != 0)
        if ((lasterr = CheckErrors()) != 0)
	    return -1;
    return 0;
}

static int OMCreateDListHeader()
{
    if (OMStartRecord(buf, TFR_HEADER)==0)
        if (OMPut16(buf, TFF_GENERATOR, 1) == 0)
            if (OMPut16(buf, TFF_GEN_VERSION, 0) == 0)
                if (OMPut16(buf, TFF_TF_CLASS, TFV_BASIC_CLASS) == 0)
                    if (OMPut16(buf, TFF_TF_TYPE, TFV_DIST_LIST_TYPE) == 0)
                        if (OMPut32(buf, TFF_TF_FLAGS, 0l) == 0)
                            if (OMPut32(buf, TFF_CREATOR_CAPS, 0l) == 0)
                                if (OMPut32(buf, TFF_HOP_COUNT, 0l) == 0)
				    return 0;
    return -1;
}

static int OMCreateDListCount(int from, int to, int cc, int bcc, int err=0)
{
    if (OMStartRecord(buf, TFR_DL_COUNTS)==0)
        if (OMPut32(buf, TFF_DL_FLAGS, 0l) == 0)
            if (OMPut16(buf, TFF_DL_NAME_COUNT, from+to+cc+bcc+err) == 0)
                if (OMPut16(buf, TFF_DL_FROM_COUNT, from) == 0)
                    if (OMPut16(buf, TFF_DL_TO_COUNT, to) == 0)
                        if (OMPut16(buf, TFF_DL_CC_COUNT, cc) == 0)
                            if (OMPut16(buf, TFF_DL_BCC_COUNT, bcc) == 0)
                                if (OMPut16(buf, TFF_DL_ERROR_COUNT, err) == 0)
                                    if (OMPut32(buf,TFF_DL_ERROR_OFFSET,0l)==0)
				return 0;
    return -1;
}

static int OMCreateAddressEntry(int typ, char *addr)
{
    if (OMStartRecord(buf, typ)==0)
        if (OMPut32(buf, TFF_DL_DATE_TIME, 0l) == 0)
            if (OMPut16(buf, TFF_DL_GMT_OFFSET, 0) == 0)
                if (OMPut16(buf, TFF_DL_NAME_FLAGS, TFV_DL_NO_NDN) == 0)
                    if (OMPutString(buf, TFF_DL_NAME, addr) > 0)
		        return 0;
    return -1;
}

//-------------------------------------------------------------------
// Writing DList TFs

int OMCreateDListTF(char *fname, int from, int to, int cc, int bcc)
{
    int rtn = xtf_CreateNamedFile(&errorstatus, fname);
    if (rtn>=0)
    {
        if (OMCreateDListHeader()==0)
	    if (OMAppendRecord(rtn) == 0)
	        if (OMCreateDListCount(from, to, cc, bcc) == 0)
	    	    if (OMAppendRecord(rtn) == 0)
			return rtn;
	(void)xtf_CloseFile(&errorstatus, rtn);
    }
    CheckErrors();
    return -1;
}

int OMAddAddress(int xtfh, int typ, char *addr)
{
    int rtn = -1;
    switch(typ)
    {
    case 0:
	rtn = OMCreateAddressEntry(TFR_FROM, addr);
	break;
    case 1:
	rtn = OMCreateAddressEntry(TFR_TO, addr);
	break;
    case 2:
	rtn = OMCreateAddressEntry(TFR_CC, addr);
	break;
    case 3:
	rtn = OMCreateAddressEntry(TFR_BCC, addr);
	break;
    }
    if (rtn == 0)
	if (OMAppendRecord(xtfh) == 0)
	    return 0;
	else (void)xtf_CloseFile(&errorstatus, rtn);
    return -1;
}

int OMSaveDList(int xtfh, char *fname, char *descrip, int destfolderref)
{
    int rtn = -1;
    if (xtf_CloseFile(&errorstatus, xtfh)>=0)
    {
	if (OMAttachFile(destfolderref, fname, descrip, 1166) == 0)
	    rtn = 0;
    }
    unlink(fname);
    return rtn;
}

int OMReplaceDList(int xtfh, char *fname, char *descrip, int ref, long itemnum)
{
    int rtn = -1;
    if (xtf_CloseFile(&errorstatus, xtfh)>=0)
    {
        if (OMSendFile(fname, host_work_file, 1) == 0)
	{
            PackMODITM(ref, itemnum);
    	    if (DoPackedCommand("MODITM") == 0)
		rtn = OMRenameFolder(ref, itemnum, descrip);
	}
    }
    unlink(fname);
    return rtn;
}

//---------------------------------------------------------------
// Reading DList TFs

int OMOpenDListTF(char *fname)
{
    int rtn = xtf_OpenNamedFile(&errorstatus,fname,0);
    if (rtn < 0) CheckErrors();
    return rtn;
}

int OMReadDListRecord(int xtf, char *namebuf, int &typ, int &acklvl)
{
    char buf[1024];
    while (xtf_ReadRecord(&errorstatus,xtf,buf) == 0)
    {
	long rectype = xtf_GetINT32(&errorstatus,buf,TFF_REC_TYPE);
	if (rectype == TFR_FROM) typ = 0;
	else if (rectype == TFR_TO) typ = 1;
	else if (rectype == TFR_CC) typ = 2;
	else if (rectype == TFR_BCC) typ = 3;
	else continue;
	int recflags = xtf_GetINT16(&errorstatus,buf,TFF_REC_FLAGS);
	if (recflags & TFV_PASSIVE_FLAG) continue;
	int offset;
	strcpy(namebuf, xtf_GetStrPtr(&errorstatus,buf,TFF_DL_NAME,&offset));
	int nameflags = xtf_GetINT16(&errorstatus,buf,TFF_DL_NAME_FLAGS);
	acklvl = (nameflags & TFV_DL_RECV_ACK_MASK) >> TFV_DL_RECV_ACK_SHIFT;
	return 0;
    }
    return -1;
}

int OMCloseDListTF(int xtf)
{
    int rtn = xtf_CloseFile(&errorstatus,xtf);
    if (rtn != 0) CheckErrors();
    return rtn;
}

//-----------------------------------------------------------------
// Directory access - used for nickname list

int OMOpenDirectory(char *name, char *passwd, int ispublic)
{
    PackDIROPN(name, passwd, ispublic);
    if (DoPackedCommand("DIROPN") != 0)
	return -1;
    return atoi(toks[UAL_DIROPN_R_HANDLE]);
}

int OMCreateDirectory(char *name, char *passwd, int ispublic)
{
    PackDIRCRT(name, passwd, ispublic);
    if (DoPackedCommand("DIRCRT") != 0)
	return -1;
    return 0;
}

int OMDeleteDirectory(char *name, int ispublic)
{
    PackDIRDEL(name, ispublic);
    if (DoPackedCommand("DIRDEL") != 0)
	return -1;
    return 0;
}

char *OMSearchDirectory(int &cnt, int handle, char *attribs,
	char *filter, long flags)
{
    PackSEARCH(handle, attribs, filter, flags);
    if (DoPackedCommand("SEARCH") != 0)
	return 0;
    cnt = atoi(toks[UAL_SEARCH_R_LISTSIZE]);
    if (cnt == 1)
	return toks[UAL_SEARCH_R_ENTRY]; // the entry
    else if (cnt > 1)
    {
	char *rtn = tmpnam(0);
	Debug1("OMSearchDirectory getting results in %s\n", rtn);
	if (OMReceiveFile(rtn, toks[UAL_SEARCH_R_LISTFILE], 1) != 0)
	    rtn = 0;
	(void)OMDeleteFile(toks[UAL_SEARCH_R_LISTFILE]);
	return rtn;
    }
    return 0; // failed or no matches
}

int OMAddDirEntry(int handle, char *entry)
{
    PackENTADD(handle, entry);
    if (DoPackedCommand("ENTADD") != 0)
	return -1;
    return 0;
}

int OMDeleteDirEntry(int handle, char *attribs, char *filter, long flags)
{
    PackENTDEL(handle, attribs, filter, flags);
    if (DoPackedCommand("ENTDEL") != 0)
	return -1;
    return 0;
}

int OMModifyDirEntry(int handle, char *modifier, char *attribs,
	char *filter, long flags)
{
    PackENTMOD(handle, modifier, attribs, filter, flags);
    if (DoPackedCommand("ENTMOD") != 0)
	return -1;
    return 0;
}

int OMCloseDirectory(int handle)
{
    PackDIRCLS(handle);
    if (DoPackedCommand("DIRCLS") != 0)
	return -1;
    return 0;
}

//------------------------------------------------------------
// FILETYPE file and config file access

int OMGetFile(char *fname, int fileid, int fromfno, int getsyscopy)
{
    PackGETFIL(fileid, fromfno, getsyscopy);
    if (DoPackedCommand("GETFIL") == 0)
	if (OMReceiveFile(fname, host_work_file, 1) == 0)
	    return 0;
    return -1;
}

int OMGetFileTypes(char *fname)
{
    if (OMGetFile(fname, UAL_FILETYPE_FILEID, 0, 0) != 0)
    {
	delete [] ServerError;
	ServerError = 0;
        return OMGetFile(fname, UAL_FILETYPE_FILEID, 0, 1);
    }
    return 0;
}

int OMSetUserConfig(int wbclear, int tabsize, int readdl, int printdl,
	int xtra, int savemail)
{
    PackMODUSR(wbclear, tabsize, readdl, printdl, xtra, savemail);
    return DoPackedCommand("MODUSR");
}

int OMGetUserConfig(int &wbclear, int &tabsize, int &readdl, int &printdl,
	int &xtra, int &savemail)
{
    PackMODUSR();
    if (DoPackedCommand("MODUSR") == 0)
    {
	wbclear = atoi(toks[UAL_MODUSR_R_WBCLEAR]);
	tabsize = atoi(toks[UAL_MODUSR_R_TABSIZE]);
	readdl = atoi(toks[UAL_MODUSR_R_READDL]);
	printdl = atoi(toks[UAL_MODUSR_R_PRINTDL]);
	xtra = atoi(toks[UAL_MODUSR_R_MSGATTR]);
	savemail = atoi(toks[UAL_MODUSR_R_MAILSAVE]);
	return 0;
    }
    return -1;
}

//----------------------------------------------------------
// Auto-action TF handling

static int OMCreateAAHeader()
{
    if (OMStartRecord(buf, TFR_HEADER)==0)
      if (OMPut16(buf, TFF_GENERATOR, 1) == 0)
        if (OMPut16(buf, TFF_GEN_VERSION, 0) == 0)
          if (OMPut16(buf, TFF_TF_CLASS, TFV_BASIC_CLASS) == 0)
            if (OMPut16(buf, TFF_TF_TYPE, TFV_AUTO_ACT_TYPE) == 0)
              if (OMPut32(buf, TFF_TF_FLAGS, 0l) == 0)
                if (OMPut32(buf, TFF_CREATOR_CAPS, 0l) == 0)
                  if (OMPut32(buf, TFF_HOP_COUNT, 0l) == 0)
		    return 0;
    return -1;
}

static int OMCreateAANumber(int num, int enabled)
{
    if (OMStartRecord(buf, TFR_AA_NO)==0)
      if (OMPut32(buf, TFF_AA_NO_FLAGS, enabled?0:1) == 0)
        if (OMPut32(buf, TFF_AA_NO, num) == 0)
	  return 0;
    return -1;
}

static int OMCreateFilterStart(int invert = 0, int use_or = 1)
{
    if (OMStartRecord(buf, TFR_FILTER_START)==0)
      if (OMPut16(buf, TFF_FILTER_GEN_FLAGS, invert) == 0)
        if (OMPut16(buf, TFF_FILTER_GROUP_OP, use_or) == 0)
	  return 0;
    return -1;
}

static int OMCreateFilterNum(int invert, int attnum, int cmpval, int tsttyp,
	int flags = 0)
{
    if (OMStartRecord(buf, TFR_FILTER_NUM)==0)
      if (OMPut16(buf, TFF_AA_FLAGS, invert) == 0)
        if (OMPut16(buf, TFF_FILTER_NUM_FLAGS, flags) == 0)
          if (OMPut32(buf, TFF_FILT_ATT_ID, attnum) == 0)
            if (OMPut16(buf, TFF_FILTER_TEST_TYPE, tsttyp) == 0)
              if (OMPut32(buf, TFF_FILTER_ATT_NUM, cmpval) == 0)
		return 0;
    return -1;
}

static int OMCreateFilterEnd()
{
    if (OMStartRecord(buf, TFR_FILTER_END)==0)
	return 0;
    return -1;
}

static int OMCreateSimpleForward(int flags, int filtyp, char *subj = "")
{
    int off;
    if (OMStartRecord(buf, TFR_AA_SIMPLE_FORWARD)==0)
      if (OMPut32(buf, TFF_AA_FLAGS, 0l) == 0)
        if (OMPut32(buf, TFF_AASF_FLAGS, flags) == 0)
          if (OMPut32(buf, TFF_AASF_CONTENT_TYPE, filtyp) == 0)
            if ((off = OMPutString(buf, TFF_AASF_LANG, "")) > 0)
              if ((off = OMPutString(buf, off, "")) > 0) // component charset
                if ((off = OMPutString(buf, off, "")) > 0) // comment
                  if ((off = OMPutString(buf, off, "")) > 0) // native lang
                    if ((off = OMPutString(buf, off, "")) > 0) //comment charset
                      if ((off = OMPutString(buf, off, subj)) > 0) // subject
                        if ((off = OMPutString(buf, off, "")) > 0) // nativlang
                          if ((off = OMPutString(buf, off, "")) > 0) // charset
			    return 0;
    return -1;
}

static int OMCreateDelete()
{
    if (OMStartRecord(buf, TFR_AA_DELETE)==0)
      if (OMPut32(buf, TFF_AA_FLAGS, 0l) == 0)
        if (OMPut32(buf, TFF_AAD_FLAGS, 0) == 0)
	  return 0;
    return -1;
}

static int OMCreateSimpleReply(int flags, int filtyp, char *subj = "")
{
    int off;
    if (OMStartRecord(buf, TFR_AA_SIMPLE_REPLY)==0)
      if (OMPut32(buf, TFF_AA_FLAGS, 0l) == 0)
        if (OMPut32(buf, TFF_AASR_FLAGS, flags) == 0)
          if (OMPut32(buf, TFF_AASR_CONTENT_TYPE, filtyp) == 0)
            if ((off = OMPutString(buf, TFF_AASR_LANG, "")) > 0) // nativelang
              if ((off = OMPutString(buf, off, "")) > 0) // charset
                if ((off = OMPutString(buf, off, subj)) > 0) //subject
                  if ((off = OMPutString(buf, off, "")) > 0) // nativlang
                    if ((off = OMPutString(buf, off, "")) > 0) // charset
			return 0;
    return -1;
}

static int ReplaceConfigFile(char *fname, int fileid, int filenum = 0)
{
    if (fname[0] == 0 || OMSendFile(fname, host_work_file, 1) == 0)
    {
        PackPUTFIL(fileid, filenum, (fname[0]==0));
        if (DoPackedCommand("PUTFIL") == 0)
	    return 0;
    }
    return -1;
}

static int OMDisableAutoActions()
{
    ReplaceConfigFile(0, UAL_AUTO_ACTIONS_FILEID);
}

int OMSetAutoActions(int afa, int ara, int ic, int dfm, int koc,
	int fprm, int fpsm, int fccm, char* dlist, char* comments,
	int rtam, int rtum, int rtrra, char* reply)
{
    char fname[256];
    strcpy(fname, tmpnam(0));
    Debug1("Auto action TF is %s\n", fname);
    Debug1("Auto action DList TF is %s\n", dlist ? dlist : "NULL");
    Debug1("Auto action comments is %s\n", comments ? comments : "NULL");
    Debug1("Auto reply contents is %s\n", reply ? reply : "NULL");
    int fd = xtf_CreateNamedFile(&errorstatus, fname);
    if (fd<0) goto err;
    if (OMCreateAAHeader() != 0) goto err;
    if (OMAppendRecord(fd) != 0) goto err;

    // Auto forward part

    if (OMCreateAANumber(500, afa) != 0) goto err;
    if (OMAppendRecord(fd) != 0) goto err;
    if ((fprm + fpsm + fccm) <= 1)
    {
    	if (OMCreateFilterStart() != 0) goto err;
        if (OMAppendRecord(fd) != 0) goto err;
    }
    if (!fprm)
    {
        if (OMCreateFilterNum(1, 6, 2, 1, 1) != 0) goto err;
        if (OMAppendRecord(fd) != 0) goto err;
    }
    if (!fpsm)
    {
    	if (OMCreateFilterNum(1, 6, 1, 1, 1) != 0) goto err;
        if (OMAppendRecord(fd) != 0) goto err;
    }
    if (!fccm)
    {
    	if (OMCreateFilterNum(1, 6, 3, 1, 1) != 0) goto err;
        if (OMAppendRecord(fd) != 0) goto err;
    }
    if ((fprm + fpsm + fccm) <= 1)
    {
    	if (OMCreateFilterEnd() != 0) goto err;
        if (OMAppendRecord(fd) != 0) goto err;
    }
    long flags;
    flags = 0;
    if (ic) flags |= 1;	// contents present
    if (koc) flags |= 4;	// keep originator
    if (OMCreateSimpleForward(flags, 1167, 0) != 0) goto err;
    if (OMAppendRecord(fd) != 0) goto err;
    if (dfm)
    {
	if (OMCreateDelete() != 0) goto err;
        if (OMAppendRecord(fd) != 0) goto err;
    }

    // Auto reply part

    if (ara || rtam || rtum || rtrra)
    {
        if (OMCreateAANumber(501, ara) != 0) goto err;
        if (OMAppendRecord(fd) != 0) goto err;
        if (!rtam)
        {
	    if (rtum && rtrra)
	    {
    	        if (OMCreateFilterStart() != 0) goto err;
                if (OMAppendRecord(fd) != 0) goto err;
	    }
	    if (rtum)
	    {
    	        if (OMCreateFilterNum(0, 4, 2, 5) != 0) goto err;
                if (OMAppendRecord(fd) != 0) goto err;
	    }
	    if (rtrra)
	    {
    	        if (OMCreateFilterNum(0, 12, 7, 5) != 0) goto err;
                if (OMAppendRecord(fd) != 0) goto err;
	    }
	    if (rtum && rtrra)
	    {
    	        if (OMCreateFilterEnd() != 0) goto err;
                if (OMAppendRecord(fd) != 0) goto err;
	    }
        }
        if (OMCreateSimpleReply(1, 1167, "Automatically generated Reply") != 0)
	    goto err;
        if (OMAppendRecord(fd) != 0) goto err;
    }
    if (xtf_CloseFile(&errorstatus, fd) != 0) goto err;
    // Put the components (dlist = 110, comments = 111, reply = 112)
    OMDisableAutoActions();
    if (dlist) ReplaceConfigFile(dlist, UAL_AFWD_DL_FILEID, 500);
    if (comments) ReplaceConfigFile(comments, UAL_AFWD_CONT_FILEID, 500);
    if (reply) ReplaceConfigFile(reply, UAL_AREPLY_CONT_FILEID, 501);
    // Put the transaction file 
    int rtn;
    rtn = ReplaceConfigFile(fname, UAL_AUTO_ACTIONS_FILEID);
    unlink(fname);
    return rtn;
err:
    if (fd >= 0)
    {
        (void)xtf_CloseFile(&errorstatus, fd);
	unlink(fname);
    }
    CheckErrors();
    return -1;
}

int OMGetAutoActions(int &afa, int &ara, int &ic, int &dfm, int &koc,
	int &fprm, int &fpsm, int &fccm, int &rtam, int &rtum, int &rtrra)
{
    char fname[256];
    int num = -1;
    strcpy(fname, tmpnam(0));
    dfm = ara = afa = ic = rtam = rtum = rtrra = 0;
    fpsm = fprm = fccm = koc = rtam = 1;
    Debug1("OMGetAutoActions returning actions in %s", fname);
    if (OMGetFile(fname, UAL_AUTO_ACTIONS_FILEID, 0, 0) != 0) return -1;
    int fd = xtf_OpenNamedFile(&errorstatus,fname,0);
    if (fd < 0) goto err;
    char buf[1024];
    //int invert = 0;
    //int use_or = 0;
    while (xtf_ReadRecord(&errorstatus,fd,buf) == 0)
    {
	long rectype = xtf_GetINT32(&errorstatus,buf,TFF_REC_TYPE);
	switch(rectype)
	{
	case TFR_AA_NO:
	    num = xtf_GetINT32(&errorstatus, buf, TFF_AA_NO);
	    int disabled = (xtf_GetINT32(&errorstatus, buf, TFF_AA_NO_FLAGS)&1);
	    if (num == 500) afa = !disabled;
	    else if (num == 501) ara = !disabled;
	    break;
        case TFR_FILTER_START:
	    //invert = xtf_GetINT16(&errorstatus, buf, TFF_FILTER_GEN_FLAGS);
	    //use_or = xtf_GetINT16(&errorstatus, buf, TFF_FILTER_GROUP_OP);
	    break;
        case TFR_FILTER_NUM:
	    int invert = xtf_GetINT16(&errorstatus, buf, TFF_FILTER_GEN_FLAGS);
	    int att = xtf_GetINT32(&errorstatus, buf, TFF_FILT_ATT_ID);
	    int cmp = xtf_GetINT16(&errorstatus, buf, TFF_FILTER_TEST_TYPE);
	    int val = xtf_GetINT32(&errorstatus, buf, TFF_FILTER_ATT_NUM);
	    if (num == 500 && att == 6 && cmp==1 && invert) // only handle these
	    {
		if (val == 1) fpsm = 0;
		if (val == 2) fprm = 0;
		if (val == 3) fccm = 0;
	    }
	    else if (num == 501 && cmp == 5) // only handle >= tests
	    {
		rtam = 0;
		if (att == 4)
		{
		    if (val == 2) rtum = 1;
		}
		else if (att == 12)
		{
	    	    if (val == 7) rtrra = 1;
		}
	    }
	    break;
        case TFR_FILTER_END:
	    break;
        case TFR_AA_SIMPLE_FORWARD:
	    int flgs = xtf_GetINT32(&errorstatus, buf, TFF_AASF_FLAGS);
	    koc = ((flgs & 4) != 0);
	    ic = ((flgs & 1) != 0);
	    break;
        case TFR_AA_DELETE:
	    if (num == 500) dfm = 1;
	    break;
        case TFR_AA_SIMPLE_REPLY:
	    break;
	}
    }
    if (rtam) rtum = rtrra = 1;
    if (xtf_CloseFile(&errorstatus,fd) == 0)
    {
	unlink(fname);
	return 0;
    }
err:
    if (fd >= 0)
    {
        (void)xtf_CloseFile(&errorstatus, fd);
	unlink(fname);
    }
    CheckErrors();
    return -1;
}

int OMGetAutoForwardDList(char *fname)
{
    return OMGetFile(fname, UAL_AFWD_DL_FILEID, 500);
}

int OMGetAutoForwardComment(char *fname)
{
    return OMGetFile(fname, UAL_AFWD_CONT_FILEID, 500);
}

int OMGetAutoReply(char *fname)
{
    return OMGetFile(fname, UAL_AREPLY_CONT_FILEID, 501);
}

//----------------------------------------------------------

int OMGetDirectories(int &cnt, char **items, int *types, int first)
{
    int maxnum = cnt, limit = 99;
    cnt = 0;
    PackDIRLST();
    SendRequest();
    for (int i = 0, j = 0; j < limit; j++)
    {
        if (GetResponse() != 0)
	    (*FatalErrorHandler)("GetResponse failed");
        if (CheckErrors() != 0)
	    return -1;
	if (j == 0) limit = atoi(toks[UAL_DIRLST_R_TOTAL]);
	if (--first >= 0 || i>=maxnum) continue;
	if (atol(toks[UAL_DIRLST_R_TYPE]) == UAL_DIR_TYPE_SHARED)
	{
	    if ((atol(toks[UAL_DIRLST_R_CAPS]) & UAL_ACL_CAP_READ) == 0)
		continue; // can't read this non-private directory
	    types[i] = 1;
	}
	else types[i] = 0;
	items[i] = new char[strlen(toks[UAL_DIRLST_R_NAME])+1];
	if (items[i] == 0) return -1;
	strcpy(items[i], toks[UAL_DIRLST_R_NAME]);
	if (++i >=  atoi(toks[UAL_DIRLST_R_TOTAL])) break;
    }
    cnt = i;
    return 0;
}

