/*
 *	ole.h	-	Declarations for OLESVR and OLECLI
 */

#ifndef __WINE_OLE_H
#define __WINE_OLE_H

#include "windows.h"

typedef	LPVOID	LPUNKNOWN;
typedef LPCSTR	OLE_LPCSTR;

/* object types */
#define OT_LINK		1
#define OT_EMBEDDED	2
#define OT_STATIC	3

/* activate verbs */
#define OLEVERB_PRIMARY	0

typedef enum
{
    OLE_OK,
    OLE_WAIT_FOR_RELEASE,
    OLE_BUSY,
    OLE_ERROR_PROTECT_ONLY,
    OLE_ERROR_MEMORY,
    OLE_ERROR_STREAM,
    OLE_ERROR_STATIC,
    OLE_ERROR_BLANK,
    OLE_ERROR_DRAW,
    OLE_ERROR_METAFILE,
    OLE_ERROR_ABORT,
    OLE_ERROR_CLIPBOARD,
    OLE_ERROR_FORMAT,
    OLE_ERROR_OBJECT,
    OLE_ERROR_OPTION,
    OLE_ERROR_PROTOCOL,
    OLE_ERROR_ADDRESS,
    OLE_ERROR_NOT_EQUAL,
    OLE_ERROR_HANDLE,
    OLE_ERROR_GENERIC,
    OLE_ERROR_CLASS,
    OLE_ERROR_SYNTAX,
    OLE_ERROR_DATATYPE,
    OLE_ERROR_PALETTE,
    OLE_ERROR_NOT_LINK,
    OLE_ERROR_NOT_EMPTY,
    OLE_ERROR_SIZE,
    OLE_ERROR_DRIVE,
    OLE_ERROR_NETWORK,
    OLE_ERROR_NAME,
    OLE_ERROR_TEMPLATE,
    OLE_ERROR_NEW,
    OLE_ERROR_EDIT,
    OLE_ERROR_OPEN,
    OLE_ERROR_NOT_OPEN,
    OLE_ERROR_LAUNCH,
    OLE_ERROR_COMM,
    OLE_ERROR_TERMINATE,
    OLE_ERROR_COMMAND,
    OLE_ERROR_SHOW,
    OLE_ERROR_DOVERB,
    OLE_ERROR_ADVISE_NATIVE,
    OLE_ERROR_ADVISE_PICT,
    OLE_ERROR_ADVISE_RENAME,
    OLE_ERROR_POKE_NATIVE,
    OLE_ERROR_REQUEST_NATIVE,
    OLE_ERROR_REQUEST_PICT,
    OLE_ERROR_SERVER_BLOCKED,
    OLE_ERROR_REGISTRATION,
    OLE_ERROR_ALREADY_REGISTERED,
    OLE_ERROR_TASK,
    OLE_ERROR_OUTOFDATE,
    OLE_ERROR_CANT_UPDATE_CLIENT,
    OLE_ERROR_UPDATE,
    OLE_ERROR_SETDATA_FORMAT,
    OLE_ERROR_STATIC_FROM_OTHER_OS,
    OLE_WARN_DELETE_DATA = 1000
} OLESTATUS;

typedef enum {
	OLE_SERVER_MULTI,
	OLE_SERVER_SINGLE
} OLE_SERVER_USE;

typedef enum {
	OLE_CHANGED,
	OLE_SAVED,
	OLE_CLOSED,
	OLE_RENAMED,
	OLE_QUERY_PAINT,
	OLE_RELEASE,
	OLE_QUERY_RETRY
} OLE_NOTIFICATION;

typedef enum {
	olerender_none,
	olerender_draw,
	olerender_format
} OLEOPT_RENDER;

typedef enum {
	oleupdate_always,
	oleupdate_onsave,
	oleupdate_oncall,
	oleupdate_onclose,
} OLEOPT_UPDATE;

typedef	LONG	LHSERVER;
typedef LONG	LHSERVERDOC;
typedef LONG	LHCLIENTDOC;
typedef LONG	OLECLIPFORMAT;/* dunno about this type, please change/add */
typedef OLEOPT_UPDATE*	LPOLEOPT_UPDATE;

typedef enum {
	OLE_NONE,	/* none */
	OLE_DELETE,	/* delete object */
	OLE_LNKPASTE,	/* link paste */
	OLE_EMBPASTE,	/* paste(and update) */
	OLE_SHOW,
	OLE_RUN,
	OLE_ACTIVATE,
	OLE_UPDATE,
	OLE_CLOSE,
	OLE_RECONNECT,
	OLE_SETUPDATEOPTIONS,
	OLE_SERVERRUNLAUNCH,	/* unlaunch (terminate?) server */
	OLE_LOADFROMSTREAM,	/* (auto reconnect) */
	OLE_SETDATA,		/* OleSetData */
	OLE_REQUESTDATA,	/* OleRequestData */
	OLE_OTHER,
	OLE_CREATE,
	OLE_CREATEFROMTEMPLATE,
	OLE_CREATELINKFROMFILE,
	OLE_COPYFROMLNK,
	OLE_CREATREFROMFILE,
	OLE_CREATEINVISIBLE
} OLE_RELEASE_METHOD;

typedef struct _OLETARGETDEVICE {
	UINT	otdDeviceNameOffset;
	UINT	otdDriverNameOffset;
	UINT	otdPortNameOffset;
	UINT	otdExtDevmodeOffset;
	UINT	otdExtDevmodeSize;
	UINT	otdEnvironmentOffset;
	UINT	otdEnvironmentSize;
	BYTE	otdData[1];
	/* ... */
} OLETARGETDEVICE;
typedef struct _OLESTREAM* LPOLESTREAM;
typedef struct _OLESTREAMVTBL {
	DWORD	(*Get)(LPOLESTREAM,LPSTR,DWORD);
	DWORD	(*Put)(LPOLESTREAM,LPSTR,DWORD);
} OLESTREAMVTBL;
typedef OLESTREAMVTBL*	LPOLESTREAMVTBL;
typedef struct _OLESTREAM {
	LPOLESTREAMVTBL	lpstbl;
} OLESTREAM;
typedef struct _OLESERVERDOC*	LPOLESERVERDOC;
typedef struct _OLEOBJECT*	LPOLEOBJECT;
typedef struct _OLECLIENT*	LPOLECLIENT;
typedef struct _OLESERVERDOCVTBL {
	OLESTATUS	(*Save)(LPOLESERVERDOC);
	OLESTATUS	(*Close)(LPOLESERVERDOC);
	OLESTATUS	(*SetHostNames)(LPOLESERVERDOC,OLE_LPCSTR,OLE_LPCSTR);
	OLESTATUS	(*SetDocDimensions)(LPOLESERVERDOC,LPRECT16);
#undef GetObject  /* FIXME */
	OLESTATUS	(*GetObject)(LPOLESERVERDOC,OLE_LPCSTR,LPOLEOBJECT*,LPOLECLIENT);
	OLESTATUS	(*Release)(LPOLESERVERDOC);
	OLESTATUS	(*SetColorScheme)(LPOLESERVERDOC,LPLOGPALETTE);
	OLESTATUS	(*Execute)(LPOLESERVERDOC,HGLOBAL);
} OLESERVERDOCVTBL;
typedef OLESERVERDOCVTBL*	LPOLESERVERDOCVTBL;
typedef struct _OLESERVERDOC {
	LPOLESERVERDOCVTBL	lpvtbl;
	/* server provided state info */
} OLESERVERDOC;

typedef struct _OLESERVER*	LPOLESERVER;
typedef struct _OLESERVERVTBL {
	OLESTATUS	(*Open)(LPOLESERVER,LHSERVERDOC,OLE_LPCSTR,LPOLESERVERDOC *);
	OLESTATUS	(*Create)(LPOLESERVER,LHSERVERDOC,OLE_LPCSTR,OLE_LPCSTR,LPOLESERVERDOC*);
	OLESTATUS	(*CreateFromTemplate)(LPOLESERVER,LHSERVERDOC,OLE_LPCSTR,OLE_LPCSTR,OLE_LPCSTR,LPOLESERVERDOC *);
	OLESTATUS	(*Edit)(LPOLESERVER,LHSERVERDOC,OLE_LPCSTR,OLE_LPCSTR,LPOLESERVERDOC *);
	OLESTATUS	(*Exit)(LPOLESERVER);
	OLESTATUS	(*Release)(LPOLESERVER); 
	OLESTATUS	(*Execute)(LPOLESERVER); 
} OLESERVERVTBL; 
typedef OLESERVERVTBL	*LPOLESERVERVTBL;
typedef struct _OLESERVER {
	LPOLESERVERVTBL	lpvtbl;
	/* server specific data */
} OLESERVER;
typedef struct _OLECLIENTVTBL {
	int (*CallBack)(LPOLECLIENT,OLE_NOTIFICATION,LPOLEOBJECT);
} OLECLIENTVTBL;
typedef OLECLIENTVTBL	*LPOLECLIENTVTBL;

typedef struct _OLECLIENT {
	LPOLECLIENTVTBL	lpvtbl;
	/* client data... */
} OLECLIENT;

typedef struct _OLEOBJECTVTBL {
        void *  	(*QueryProtocol)(LPOLEOBJECT,OLE_LPCSTR);
	OLESTATUS	(*Release)(LPOLEOBJECT);
	OLESTATUS	(*Show)(LPOLEOBJECT,BOOL);
	OLESTATUS	(*DoVerb)(LPOLEOBJECT,UINT,BOOL,BOOL);
	OLESTATUS	(*GetData)(LPOLEOBJECT,OLECLIPFORMAT,HANDLE *);
	OLESTATUS	(*SetData)(LPOLEOBJECT,OLECLIPFORMAT,HANDLE);
	OLESTATUS	(*SetTargetDevice)(LPOLEOBJECT,HGLOBAL);
	OLESTATUS	(*SetBounds)(LPOLEOBJECT,LPRECT16);
	OLESTATUS	(*EnumFormats)(LPOLEOBJECT,OLECLIPFORMAT);
	OLESTATUS	(*SetColorScheme)(LPOLEOBJECT,LPLOGPALETTE);
	OLESTATUS	(*Delete)(LPOLEOBJECT);
	OLESTATUS	(*SetHostNames)(LPOLEOBJECT,OLE_LPCSTR,OLE_LPCSTR);
	OLESTATUS	(*SaveToStream)(LPOLEOBJECT,LPOLESTREAM);
	OLESTATUS	(*Clone)(LPOLEOBJECT,LPOLECLIENT,LHCLIENTDOC,OLE_LPCSTR,LPOLEOBJECT *);
	OLESTATUS	(*CopyFromLink)(LPOLEOBJECT,LPOLECLIENT,LHCLIENTDOC,OLE_LPCSTR,LPOLEOBJECT *);
	OLESTATUS	(*Equal)(LPOLEOBJECT,LPOLEOBJECT);
	OLESTATUS	(*CopyToClipBoard)(LPOLEOBJECT);
	OLESTATUS	(*Draw)(LPOLEOBJECT,HDC,LPRECT16,LPRECT16,HDC);
	OLESTATUS	(*Activate)(LPOLEOBJECT,UINT,BOOL,BOOL,HWND,LPRECT16);
	OLESTATUS	(*Execute)(LPOLEOBJECT,HGLOBAL,UINT);
	OLESTATUS	(*Close)(LPOLEOBJECT);
	OLESTATUS	(*Update)(LPOLEOBJECT);
	OLESTATUS	(*Reconnect)(LPOLEOBJECT);
	OLESTATUS	(*ObjectConvert)(LPOLEOBJECT,OLE_LPCSTR,LPOLECLIENT,LHCLIENTDOC,OLE_LPCSTR,LPOLEOBJECT*);
	OLESTATUS	(*GetLinkUpdateOptions)(LPOLEOBJECT,LPOLEOPT_UPDATE);
	OLESTATUS	(*SetLinkUpdateOptions)(LPOLEOBJECT,OLEOPT_UPDATE);
	OLESTATUS	(*Rename)(LPOLEOBJECT,OLE_LPCSTR);
	OLESTATUS	(*QueryName)(LPOLEOBJECT,LPSTR,LPUINT16);
	OLESTATUS	(*QueryType)(LPOLEOBJECT,LPLONG);
	OLESTATUS	(*QueryBounds)(LPOLEOBJECT,LPRECT16);
	OLESTATUS	(*QuerySize)(LPOLEOBJECT,LPDWORD);
	OLESTATUS	(*QueryOpen)(LPOLEOBJECT);
	OLESTATUS	(*QueryOutOfDate)(LPOLEOBJECT);
	OLESTATUS	(*QueryReleaseStatus)(LPOLEOBJECT);
	OLESTATUS	(*QueryReleaseError)(LPOLEOBJECT);
	OLE_RELEASE_METHOD (*QueryReleaseMethod)(LPOLEOBJECT);
	OLESTATUS	(*RequestData)(LPOLEOBJECT,OLECLIPFORMAT);
	OLESTATUS	(*ObjectLong)(LPOLEOBJECT,UINT,LPLONG);
} OLEOBJECTVTBL;
typedef OLEOBJECTVTBL*	LPOLEOBJECTVTBL;

typedef struct _OLEOBJECT {
	LPOLEOBJECTVTBL	lpvtbl;
} OLEOBJECT;


OLESTATUS OleRegisterServer(LPCSTR,LPOLESERVER,LHSERVER *,HINSTANCE,OLE_SERVER_USE);
OLESTATUS OleUnblockServer(LHSERVER,BOOL *);
OLESTATUS OleRegisterServerDoc(LHSERVER,LPCSTR,LPOLESERVERDOC,LHSERVERDOC *);
OLESTATUS OleRegisterClientDoc(LPCSTR,LPCSTR,LONG,LHCLIENTDOC *);
OLESTATUS OleRenameClientDoc(LHCLIENTDOC,LPCSTR);
OLESTATUS OleRevokeServerDoc(LHSERVERDOC);
OLESTATUS OleRevokeClientDoc(LHCLIENTDOC);
OLESTATUS OleRevokeServer(LHSERVER);

#endif  /* __WINE_OLE_H */
