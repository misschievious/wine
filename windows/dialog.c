/*
 * Dialog functions
 *
 * Copyright 1993, 1994, 1996 Alexandre Julliard
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "windows.h"
#include "dialog.h"
#include "heap.h"
#include "win.h"
#include "ldt.h"
#include "resource32.h"
#include "stackframe.h"
#include "string32.h"
#include "user.h"
#include "winproc.h"
#include "message.h"
#include "stddebug.h"
#include "debug.h"


  /* Dialog control information */
typedef struct
{
    DWORD      style;
    DWORD      exStyle;
    INT16      x;
    INT16      y;
    INT16      cx;
    INT16      cy;
    UINT16     id;
    LPCSTR     className;
    LPCSTR     windowName;
    LPVOID     data;
} DLG_CONTROL_INFO;

  /* Dialog template */
typedef struct
{
    DWORD      style;
    DWORD      exStyle;
    UINT16     nbItems;
    INT16      x;
    INT16      y;
    INT16      cx;
    INT16      cy;
    LPCSTR     menuName;
    LPCSTR     className;
    LPCSTR     caption;
    WORD       pointSize;
    LPCSTR     faceName;
} DLG_TEMPLATE;

  /* Dialog base units */
static WORD xBaseUnit = 0, yBaseUnit = 0;


/***********************************************************************
 *           DIALOG_Init
 *
 * Initialisation of the dialog manager.
 */
BOOL DIALOG_Init()
{
    TEXTMETRIC16 tm;
    HDC hdc;
    
      /* Calculate the dialog base units */

    if (!(hdc = CreateDC( "DISPLAY", NULL, NULL, NULL ))) return FALSE;
    GetTextMetrics( hdc, &tm );
    DeleteDC( hdc );
    xBaseUnit = tm.tmAveCharWidth;
    yBaseUnit = tm.tmHeight;

      /* Dialog units are based on a proportional system font */
      /* so we adjust them a bit for a fixed font. */
    if (tm.tmPitchAndFamily & TMPF_FIXED_PITCH) xBaseUnit = xBaseUnit * 5 / 4;

    dprintf_dialog( stddeb, "DIALOG_Init: base units = %d,%d\n",
                    xBaseUnit, yBaseUnit );
    return TRUE;
}


/***********************************************************************
 *           DIALOG_GetFirstTabItem
 *
 * Return the first item of the dialog that has the WS_TABSTOP style.
 */
HWND DIALOG_GetFirstTabItem( HWND hwndDlg )
{
    WND *pWnd = WIN_FindWndPtr( hwndDlg );
    for (pWnd = pWnd->child; pWnd; pWnd = pWnd->next)
        if (pWnd->dwStyle & WS_TABSTOP) return pWnd->hwndSelf;
    return 0;
}


/***********************************************************************
 *           DIALOG_GetControl16
 *
 * Return the class and text of the control pointed to by ptr,
 * fill the header structure and return a pointer to the next control.
 */
static LPCSTR DIALOG_GetControl16( LPCSTR p, DLG_CONTROL_INFO *info )
{
    static char buffer[10];

    info->x       = GET_WORD(p);  p += sizeof(WORD);
    info->y       = GET_WORD(p);  p += sizeof(WORD);
    info->cx      = GET_WORD(p);  p += sizeof(WORD);
    info->cy      = GET_WORD(p);  p += sizeof(WORD);
    info->id      = GET_WORD(p);  p += sizeof(WORD);
    info->style   = GET_DWORD(p); p += sizeof(DWORD);
    info->exStyle = 0;

    if (*p & 0x80)
    {
        switch((BYTE)*p)
        {
            case 0x80: strcpy( buffer, "BUTTON" ); break;
            case 0x81: strcpy( buffer, "EDIT" ); break;
            case 0x82: strcpy( buffer, "STATIC" ); break;
            case 0x83: strcpy( buffer, "LISTBOX" ); break;
            case 0x84: strcpy( buffer, "SCROLLBAR" ); break;
            case 0x85: strcpy( buffer, "COMBOBOX" ); break;
            default:   buffer[0] = '\0'; break;
        }
        info->className = buffer;
        p++;
    }
    else 
    {
	info->className = p;
	p += strlen(p) + 1;
    }
    dprintf_dialog(stddeb, "   %s ", info->className );

    if ((BYTE)*p == 0xff)
    {
	  /* Integer id, not documented (?). Only works for SS_ICON controls */
	info->windowName = (LPCSTR)(UINT32)GET_WORD(p+1);
	p += 3;
        dprintf_dialog( stddeb,"%04x", LOWORD(info->windowName) );
    }
    else
    {
	info->windowName = p;
	p += strlen(p) + 1;
        dprintf_dialog(stddeb,"'%s'", info->windowName );
    }

    info->data = (LPVOID)(*p ? p + 1 : NULL);  /* FIXME: should be a segptr */
    p += *p + 1;

    dprintf_dialog( stddeb," %d, %d, %d, %d, %d, %08lx, %08lx\n", 
                    info->id, info->x, info->y, info->cx, info->cy,
                    info->style, (DWORD)info->data);
    return p;
}


/***********************************************************************
 *           DIALOG_GetControl32
 *
 * Return the class and text of the control pointed to by ptr,
 * fill the header structure and return a pointer to the next control.
 */
static const WORD *DIALOG_GetControl32( const WORD *p, DLG_CONTROL_INFO *info )
{
    static WCHAR buffer[10];

    info->style   = GET_DWORD(p); p += 2;
    info->exStyle = GET_DWORD(p); p += 2;
    info->x       = GET_WORD(p); p++;
    info->y       = GET_WORD(p); p++;
    info->cx      = GET_WORD(p); p++;
    info->cy      = GET_WORD(p); p++;
    info->id      = GET_WORD(p); p++;

    if (GET_WORD(p) == 0xffff)
    {
        switch(GET_WORD(p+1))
        {
            case 0x80: STRING32_AnsiToUni( buffer, "BUTTON" ); break;
            case 0x81: STRING32_AnsiToUni( buffer, "EDIT" ); break;
            case 0x82: STRING32_AnsiToUni( buffer, "STATIC" ); break;
            case 0x83: STRING32_AnsiToUni( buffer, "LISTBOX" ); break;
            case 0x84: STRING32_AnsiToUni( buffer, "SCROLLBAR" ); break;
            case 0x85: STRING32_AnsiToUni( buffer, "COMBOBOX" ); break;
            default:   buffer[0] = '\0'; break;
        }
        info->className = (LPCSTR)buffer;
        p += 2;
    }
    else
    {
        info->className = (LPCSTR)p;
        p += lstrlen32W( (LPCWSTR)p ) + 1;
    }
    dprintf_dialog(stddeb, "   %p ", info->className );

    if (GET_WORD(p) == 0xffff)
    {
	info->windowName = (LPCSTR)(p + 1);
	p += 2;
        dprintf_dialog( stddeb,"%04x", LOWORD(info->windowName) );
    }
    else
    {
	info->windowName = (LPCSTR)p;
        p += lstrlen32W( (LPCWSTR)p ) + 1;
        dprintf_dialog(stddeb,"'%p'", info->windowName );
    }

    if (GET_WORD(p))
    {
        info->data = (LPVOID)(p + 1);
        p += GET_WORD(p) / sizeof(WORD);
    }
    else info->data = NULL;
    p++;

    dprintf_dialog( stddeb," %d, %d, %d, %d, %d, %08lx, %08lx, %08lx\n", 
                    info->id, info->x, info->y, info->cx, info->cy,
                    info->style, info->exStyle, (DWORD)info->data);
    /* Next control is on dword boundary */
    return (const WORD *)((((int)p) + 3) & ~3);
}


/***********************************************************************
 *           DIALOG_CreateControls
 *
 * Create the control windows for a dialog.
 */
static BOOL32 DIALOG_CreateControls( WND *pWnd, LPCSTR template, INT32 items,
                                     HINSTANCE32 hInst, BOOL win32 )
{
    DIALOGINFO *dlgInfo = (DIALOGINFO *)pWnd->wExtra;
    DLG_CONTROL_INFO info;
    HWND32 hwndCtrl, hwndDefButton = 0;

    dprintf_dialog(stddeb, " BEGIN\n" );
    while (items--)
    {
        if (!win32)
        {
            HINSTANCE16 instance;
            template = DIALOG_GetControl16( template, &info );
            if (HIWORD(info.className) && !strcmp( info.className, "EDIT") &&
                ((info.style & DS_LOCALEDIT) != DS_LOCALEDIT))
            {
                if (!dlgInfo->hDialogHeap)
                {
                    dlgInfo->hDialogHeap = GlobalAlloc16(GMEM_FIXED, 0x10000);
                    if (!dlgInfo->hDialogHeap)
                    {
                        fprintf( stderr, "CreateDialogIndirectParam: Insufficient memory to create heap for edit control\n" );
                        continue;
                    }
                    LocalInit(dlgInfo->hDialogHeap, 0, 0xffff);
                }
                instance = dlgInfo->hDialogHeap;
            }
            else instance = (HINSTANCE16)hInst;

            hwndCtrl = CreateWindowEx16( info.exStyle | WS_EX_NOPARENTNOTIFY,
                                         info.className, info.windowName,
                                         info.style | WS_CHILD,
                                         info.x * dlgInfo->xBaseUnit / 4,
                                         info.y * dlgInfo->yBaseUnit / 8,
                                         info.cx * dlgInfo->xBaseUnit / 4,
                                         info.cy * dlgInfo->yBaseUnit / 8,
                                         pWnd->hwndSelf, (HMENU)info.id,
                                         instance, info.data );
        }
        else
        {
            template = (LPCSTR)DIALOG_GetControl32( (WORD *)template, &info );
            hwndCtrl = CreateWindowEx32W( info.exStyle | WS_EX_NOPARENTNOTIFY,
                                          (LPCWSTR)info.className,
                                          (LPCWSTR)info.windowName,
                                          info.style | WS_CHILD,
                                          info.x * dlgInfo->xBaseUnit / 4,
                                          info.y * dlgInfo->yBaseUnit / 8,
                                          info.cx * dlgInfo->xBaseUnit / 4,
                                          info.cy * dlgInfo->yBaseUnit / 8,
                                          pWnd->hwndSelf, (HMENU)info.id,
                                          hInst, info.data );
        }
        if (!hwndCtrl) return FALSE;

        /* Make the control last one in Z-order, so that controls remain
           in the order in which they were created */
	SetWindowPos( hwndCtrl, HWND_BOTTOM, 0, 0, 0, 0,
                      SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );

            /* Send initialisation messages to the control */
        if (dlgInfo->hUserFont) SendMessage32A( hwndCtrl, WM_SETFONT,
                                               (WPARAM)dlgInfo->hUserFont, 0 );
        if (SendMessage32A(hwndCtrl, WM_GETDLGCODE, 0, 0) & DLGC_DEFPUSHBUTTON)
        {
              /* If there's already a default push-button, set it back */
              /* to normal and use this one instead. */
            if (hwndDefButton)
                SendMessage32A( hwndDefButton, BM_SETSTYLE32,
                                BS_PUSHBUTTON,FALSE );
            hwndDefButton = hwndCtrl;
            dlgInfo->msgResult = GetWindowWord( hwndCtrl, GWW_ID );
        }
    }    
    dprintf_dialog(stddeb, " END\n" );
    return TRUE;
}


/***********************************************************************
 *           DIALOG_ParseTemplate16
 *
 * Fill a DLG_TEMPLATE structure from the dialog template, and return
 * a pointer to the first control.
 */
static LPCSTR DIALOG_ParseTemplate16( LPCSTR p, DLG_TEMPLATE * result )
{
    result->style   = GET_DWORD(p); p += sizeof(DWORD);
    result->exStyle = 0;
    result->nbItems = *p++;
    result->x       = GET_WORD(p);  p += sizeof(WORD);
    result->y       = GET_WORD(p);  p += sizeof(WORD);
    result->cx      = GET_WORD(p);  p += sizeof(WORD);
    result->cy      = GET_WORD(p);  p += sizeof(WORD);
    dprintf_dialog( stddeb, "DIALOG %d, %d, %d, %d\n",
                    result->x, result->y, result->cx, result->cy );
    dprintf_dialog( stddeb, " STYLE %08lx\n", result->style );

    /* Get the menu name */

    switch( (BYTE)*p )
    {
    case 0:
        result->menuName = 0;
        p++;
        break;
    case 0xff:
        result->menuName = (LPCSTR)(UINT32)GET_WORD( p + 1 );
        p += 3;
	dprintf_dialog(stddeb, " MENU %04x\n", LOWORD(result->menuName) );
        break;
    default:
        result->menuName = p;
        dprintf_dialog( stddeb, " MENU '%s'\n", p );
        p += strlen(p) + 1;
        break;
    }

    /* Get the class name */

    if (*p)
    {
        result->className = p;
        dprintf_dialog( stddeb, " CLASS '%s'\n", result->className );
    }
    else result->className = DIALOG_CLASS_ATOM;
    p += strlen(p) + 1;

    /* Get the window caption */

    result->caption = p;
    p += strlen(p) + 1;
    dprintf_dialog( stddeb, " CAPTION '%s'\n", result->caption );

    /* Get the font name */

    if (result->style & DS_SETFONT)
    {
	result->pointSize = GET_WORD(p);
        p += sizeof(WORD);
	result->faceName = p;
        p += strlen(p) + 1;
	dprintf_dialog( stddeb, " FONT %d,'%s'\n",
                        result->pointSize, result->faceName );
    }
    return p;
}


/***********************************************************************
 *           DIALOG_ParseTemplate32
 *
 * Fill a DLG_TEMPLATE structure from the dialog template, and return
 * a pointer to the first control.
 */
static LPCSTR DIALOG_ParseTemplate32( LPCSTR template, DLG_TEMPLATE * result )
{
    const WORD *p = (const WORD *)template;

    result->style   = GET_DWORD(p); p += 2;
    result->exStyle = GET_DWORD(p); p += 2;
    result->nbItems = GET_WORD(p); p++;
    result->x       = GET_WORD(p); p++;
    result->y       = GET_WORD(p); p++;
    result->cx      = GET_WORD(p); p++;
    result->cy      = GET_WORD(p); p++;
    dprintf_dialog( stddeb, "DIALOG %d, %d, %d, %d\n",
                    result->x, result->y, result->cx, result->cy );
    dprintf_dialog( stddeb, " STYLE %08lx\n", result->style );
    dprintf_dialog( stddeb, " EXSTYLE %08lx\n", result->exStyle );

    /* Get the menu name */

    switch(GET_WORD(p))
    {
    case 0x0000:
        result->menuName = NULL;
        p++;
        break;
    case 0xffff:
        result->menuName = (LPCSTR)(UINT32)GET_WORD( p + 1 );
        p += 2;
	dprintf_dialog(stddeb, " MENU %04x\n", LOWORD(result->menuName) );
        break;
    default:
        result->menuName = (LPCSTR)p;
        dprintf_dialog( stddeb, " MENU '%p'\n", p );
        p += lstrlen32W( (LPCWSTR)p ) + 1;
        break;
    }

    /* Get the class name */

    switch(GET_WORD(p))
    {
    case 0x0000:
        result->className = DIALOG_CLASS_ATOM;
        p++;
        break;
    case 0xffff:
        result->className = (LPCSTR)(UINT32)GET_WORD( p + 1 );
        p += 2;
	dprintf_dialog(stddeb, " CLASS %04x\n", LOWORD(result->className) );
        break;
    default:
        result->className = (LPCSTR)p;
        dprintf_dialog( stddeb, " CLASS '%p'\n", p );
        p += lstrlen32W( (LPCWSTR)p ) + 1;
        break;
    }

    /* Get the window caption */

    result->caption = (LPCSTR)p;
    p += lstrlen32W( (LPCWSTR)p ) + 1;
    dprintf_dialog( stddeb, " CAPTION '%p'\n", result->caption );

    /* Get the font name */

    if (result->style & DS_SETFONT)
    {
	result->pointSize = GET_WORD(p);
        p++;
	result->faceName = (LPCSTR)p;
        p += lstrlen32W( (LPCWSTR)p ) + 1;
	dprintf_dialog( stddeb, " FONT %d,'%p'\n",
                        result->pointSize, result->faceName );
    }
    /* First control is on dword boundary */
    return (LPCSTR)((((int)p) + 3) & ~3);
}


/***********************************************************************
 *           DIALOG_CreateIndirect
 */
static HWND DIALOG_CreateIndirect( HINSTANCE hInst, LPCSTR dlgTemplate,
                                   HWND owner, HANDLE32 dlgProc,
                                   LPARAM param, BOOL win32 )
{
    HMENU hMenu = 0;
    HFONT hFont = 0;
    HWND hwnd;
    RECT16 rect;
    WND * wndPtr;
    DLG_TEMPLATE template;
    DIALOGINFO * dlgInfo;
    WORD xUnit = xBaseUnit;
    WORD yUnit = yBaseUnit;

      /* Parse dialog template */

    if (!dlgTemplate) return 0;
    if (win32) dlgTemplate = DIALOG_ParseTemplate32( dlgTemplate, &template );
    else dlgTemplate = DIALOG_ParseTemplate16( dlgTemplate, &template );

      /* Load menu */

    if (template.menuName)
    {
        LPSTR str = SEGPTR_STRDUP( template.menuName );  /* FIXME: win32 */
        hMenu = LoadMenu( hInst, SEGPTR_GET(str) );
        SEGPTR_FREE( str );
    }

      /* Create custom font if needed */

    if (template.style & DS_SETFONT)
    {
          /* The font height must be negative as it is a point size */
          /* (see CreateFont() documentation in the Windows SDK).   */
	hFont = CreateFont( -template.pointSize, 0, 0, 0, FW_DONTCARE,
			    FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0,
			    DEFAULT_QUALITY, FF_DONTCARE,
                            template.faceName );  /* FIXME: win32 */
	if (hFont)
	{
	    TEXTMETRIC16 tm;
	    HFONT oldFont;
	    HDC hdc;

	    hdc = GetDC(0);
	    oldFont = SelectObject( hdc, hFont );
	    GetTextMetrics( hdc, &tm );
	    SelectObject( hdc, oldFont );
	    ReleaseDC( 0, hdc );
	    xUnit = tm.tmAveCharWidth;
	    yUnit = tm.tmHeight;
            if (tm.tmPitchAndFamily & TMPF_FIXED_PITCH)
                xBaseUnit = xBaseUnit * 5 / 4;  /* See DIALOG_Init() */
	}
    }
    
    /* Create dialog main window */

    rect.left = rect.top = 0;
    rect.right = template.cx * xUnit / 4;
    rect.bottom = template.cy * yUnit / 8;
    if (template.style & DS_MODALFRAME)
        template.exStyle |= WS_EX_DLGMODALFRAME;
    AdjustWindowRectEx16( &rect, template.style, 
                          hMenu ? TRUE : FALSE , template.exStyle );
    rect.right -= rect.left;
    rect.bottom -= rect.top;

    if ((INT16)template.x == CW_USEDEFAULT16)
        rect.left = rect.top = CW_USEDEFAULT16;
    else
    {
        rect.left += template.x * xUnit / 4;
        rect.top += template.y * yUnit / 8;
        if (!(template.style & DS_ABSALIGN))
            ClientToScreen16( owner, (POINT16 *)&rect );
    }

    if (win32)
        hwnd = CreateWindowEx32W(template.exStyle, (LPCWSTR)template.className,
                                 (LPCWSTR)template.caption,
                                 template.style & ~WS_VISIBLE,
                                 rect.left, rect.top, rect.right, rect.bottom,
                                 owner, hMenu, hInst, NULL );
    else
        hwnd = CreateWindowEx16(template.exStyle, template.className,
                                template.caption, template.style & ~WS_VISIBLE,
                                rect.left, rect.top, rect.right, rect.bottom,
                                owner, hMenu, hInst, NULL );
    if (!hwnd)
    {
	if (hFont) DeleteObject( hFont );
	if (hMenu) DestroyMenu( hMenu );
	return 0;
    }
    wndPtr = WIN_FindWndPtr( hwnd );

      /* Initialise dialog extra data */

    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    dlgInfo->hUserFont = hFont;
    dlgInfo->hMenu     = hMenu;
    dlgInfo->xBaseUnit = xUnit;
    dlgInfo->yBaseUnit = yUnit;
    dlgInfo->msgResult = 0;  /* This is used to store the default button id */
    dlgInfo->hDialogHeap = 0;

    /* Create controls */

    if (!DIALOG_CreateControls( wndPtr, dlgTemplate, template.nbItems,
                                hInst, win32 ))
    {
        DestroyWindow( hwnd );
        return 0;
    }

    /* Send initialisation messages and set focus */

    dlgInfo->dlgProc   = dlgProc;
    dlgInfo->hwndFocus = DIALOG_GetFirstTabItem( hwnd );
    if (dlgInfo->hUserFont)
	SendMessage32A( hwnd, WM_SETFONT, (WPARAM)dlgInfo->hUserFont, 0 );
    if (SendMessage32A(hwnd, WM_INITDIALOG, (WPARAM)dlgInfo->hwndFocus, param))
	SetFocus( dlgInfo->hwndFocus );
    if (template.style & WS_VISIBLE) ShowWindow( hwnd, SW_SHOW );
    return hwnd;
}


/***********************************************************************
 *           CreateDialog16   (USER.89)
 */
HWND16 CreateDialog16( HINSTANCE16 hInst, SEGPTR dlgTemplate,
                       HWND16 owner, DLGPROC16 dlgProc )
{
    return CreateDialogParam16( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           CreateDialogParam16   (USER.241)
 */
HWND16 CreateDialogParam16( HINSTANCE16 hInst, SEGPTR dlgTemplate,
                            HWND16 owner, DLGPROC16 dlgProc, LPARAM param )
{
    HWND16 hwnd = 0;
    HRSRC hRsrc;
    HGLOBAL16 hmem;
    LPCVOID data;

    dprintf_dialog(stddeb, "CreateDialogParam16: %04x,%08lx,%04x,%08lx,%ld\n",
                   hInst, (DWORD)dlgTemplate, owner, (DWORD)dlgProc, param );

    if (!(hRsrc = FindResource( hInst, dlgTemplate, RT_DIALOG ))) return 0;
    if (!(hmem = LoadResource( hInst, hRsrc ))) return 0;
    if (!(data = LockResource( hmem ))) hwnd = 0;
    else hwnd = CreateDialogIndirectParam16( hInst, data, owner,
                                             dlgProc, param );
    FreeResource( hmem );
    return hwnd;
}


/***********************************************************************
 *           CreateDialogParam32A   (USER32.72)
 */
HWND32 CreateDialogParam32A( HINSTANCE32 hInst, LPCSTR name,
                             HWND32 owner, DLGPROC32 dlgProc, LPARAM param )
{
    if (HIWORD(name))
    {
        LPWSTR str = STRING32_DupAnsiToUni( name );
        HWND32 hwnd = CreateDialogParam32W( hInst, str, owner, dlgProc, param);
        free( str );
        return hwnd;
    }
    return CreateDialogParam32W( hInst, (LPCWSTR)name, owner, dlgProc, param );
}


/***********************************************************************
 *           CreateDialogParam32W   (USER32.73)
 */
HWND32 CreateDialogParam32W( HINSTANCE32 hInst, LPCWSTR name,
                             HWND32 owner, DLGPROC32 dlgProc, LPARAM param )
{
    HANDLE32 hrsrc = FindResource32( hInst, name, (LPWSTR)RT_DIALOG );
    if (!hrsrc) return 0;
    return CreateDialogIndirectParam32W( hInst, LoadResource32( hInst, hrsrc ),
                                         owner, dlgProc, param );
}


/***********************************************************************
 *           CreateDialogIndirect16   (USER.219)
 */
HWND16 CreateDialogIndirect16( HINSTANCE16 hInst, LPCVOID dlgTemplate,
                               HWND16 owner, DLGPROC16 dlgProc )
{
    return CreateDialogIndirectParam16( hInst, dlgTemplate, owner, dlgProc, 0);
}


/***********************************************************************
 *           CreateDialogIndirectParam16   (USER.242)
 */
HWND16 CreateDialogIndirectParam16( HINSTANCE16 hInst, LPCVOID dlgTemplate,
                                    HWND16 owner, DLGPROC16 dlgProc,
                                    LPARAM param )
{
    HANDLE32 proc = WINPROC_AllocWinProc( (UINT32)dlgProc, WIN_PROC_16 );
    return DIALOG_CreateIndirect( hInst, dlgTemplate,
                                  owner, proc, param, FALSE );
}


/***********************************************************************
 *           CreateDialogIndirectParam32A   (USER32.69)
 */
HWND32 CreateDialogIndirectParam32A( HINSTANCE32 hInst, LPCVOID dlgTemplate,
                                     HWND32 owner, DLGPROC32 dlgProc,
                                     LPARAM param )
{
    HANDLE32 proc = WINPROC_AllocWinProc( (UINT32)dlgProc, WIN_PROC_32A );
    return DIALOG_CreateIndirect( hInst, dlgTemplate,
                                  owner, proc, param, TRUE );
}


/***********************************************************************
 *           CreateDialogIndirectParam32W   (USER32.71)
 */
HWND32 CreateDialogIndirectParam32W( HINSTANCE32 hInst, LPCVOID dlgTemplate,
                                     HWND32 owner, DLGPROC32 dlgProc,
                                     LPARAM param )
{
    HANDLE32 proc = WINPROC_AllocWinProc( (UINT32)dlgProc, WIN_PROC_32W );
    return DIALOG_CreateIndirect( hInst, dlgTemplate,
                                  owner, proc, param, TRUE );
}


/***********************************************************************
 *           DIALOG_DoDialogBox
 */
static INT32 DIALOG_DoDialogBox( HWND hwnd, HWND owner )
{
    WND * wndPtr;
    DIALOGINFO * dlgInfo;
    HANDLE msgHandle;
    MSG16* lpmsg;
    INT32 retval;

      /* Owner must be a top-level window */
    owner = WIN_GetTopParent( owner );
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return -1;
    if (!(msgHandle = USER_HEAP_ALLOC( sizeof(MSG16) ))) return -1;
    lpmsg = (MSG16 *) USER_HEAP_LIN_ADDR( msgHandle );
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    EnableWindow( owner, FALSE );
    ShowWindow( hwnd, SW_SHOW );

    while (MSG_InternalGetMessage( (SEGPTR)USER_HEAP_SEG_ADDR(msgHandle), hwnd, owner,
                                   MSGF_DIALOGBOX, PM_REMOVE,
                                   !(wndPtr->dwStyle & DS_NOIDLEMSG) ))
    {
	if (!IsDialogMessage( hwnd, lpmsg))
	{
	    TranslateMessage( lpmsg );
	    DispatchMessage( lpmsg );
	}
	if (dlgInfo->fEnd) break;
    }
    retval = dlgInfo->msgResult;
    DestroyWindow( hwnd );
    USER_HEAP_FREE( msgHandle );
    EnableWindow( owner, TRUE );
    return retval;
}


/***********************************************************************
 *           DialogBox16   (USER.87)
 */
INT16 DialogBox16( HINSTANCE16 hInst, SEGPTR dlgTemplate,
                   HWND16 owner, DLGPROC16 dlgProc )
{
    return DialogBoxParam16( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           DialogBoxParam16   (USER.239)
 */
INT16 DialogBoxParam16( HINSTANCE16 hInst, SEGPTR template,
                        HWND16 owner, DLGPROC16 dlgProc, LPARAM param )
{
    HWND16 hwnd = CreateDialogParam16( hInst, template, owner, dlgProc, param);
    if (hwnd) return (INT16)DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxParam32A   (USER32.138)
 */
INT32 DialogBoxParam32A( HINSTANCE32 hInst, LPCSTR name,
                         HWND32 owner, DLGPROC32 dlgProc, LPARAM param )
{
    HWND32 hwnd = CreateDialogParam32A( hInst, name, owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxParam32W   (USER32.139)
 */
INT32 DialogBoxParam32W( HINSTANCE32 hInst, LPCWSTR name,
                         HWND32 owner, DLGPROC32 dlgProc, LPARAM param )
{
    HWND32 hwnd = CreateDialogParam32W( hInst, name, owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxIndirect16   (USER.218)
 */
INT16 DialogBoxIndirect16( HINSTANCE16 hInst, HANDLE16 dlgTemplate,
                           HWND16 owner, DLGPROC16 dlgProc )
{
    return DialogBoxIndirectParam16( hInst, dlgTemplate, owner, dlgProc, 0 );
}


/***********************************************************************
 *           DialogBoxIndirectParam16   (USER.240)
 */
INT16 DialogBoxIndirectParam16( HINSTANCE16 hInst, HANDLE16 dlgTemplate,
                                HWND16 owner, DLGPROC16 dlgProc, LPARAM param )
{
    HWND16 hwnd;
    LPCVOID ptr;

    if (!(ptr = GlobalLock16( dlgTemplate ))) return -1;
    hwnd = CreateDialogIndirectParam16( hInst, ptr, owner, dlgProc, param );
    GlobalUnlock16( dlgTemplate );
    if (hwnd) return (INT16)DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxIndirectParam32A   (USER32.135)
 */
INT32 DialogBoxIndirectParam32A( HINSTANCE32 hInstance, LPCVOID template,
                                 HWND32 owner, DLGPROC32 dlgProc ,LPARAM param)
{
    HWND32 hwnd = CreateDialogIndirectParam32A( hInstance, template,
                                                owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           DialogBoxIndirectParam32W   (USER32.137)
 */
INT32 DialogBoxIndirectParam32W( HINSTANCE32 hInstance, LPCVOID template,
                                 HWND32 owner, DLGPROC32 dlgProc ,LPARAM param)
{
    HWND32 hwnd = CreateDialogIndirectParam32W( hInstance, template,
                                                owner, dlgProc, param );
    if (hwnd) return DIALOG_DoDialogBox( hwnd, owner );
    return -1;
}


/***********************************************************************
 *           EndDialog   (USER.88) (USER32.173)
 */
BOOL16 EndDialog( HWND32 hwnd, INT32 retval )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    DIALOGINFO * dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    dlgInfo->msgResult = retval;
    dlgInfo->fEnd = TRUE;
    dprintf_dialog(stddeb, "EndDialog: %04x %d\n", hwnd, retval );
    return TRUE;
}


/***********************************************************************
 *           IsDialogMessage   (USER.90)
 */
BOOL IsDialogMessage( HWND hwndDlg, LPMSG16 msg )
{
    WND * wndPtr;
    int dlgCode;

    if (!(wndPtr = WIN_FindWndPtr( hwndDlg ))) return FALSE;
    if ((hwndDlg != msg->hwnd) && !IsChild( hwndDlg, msg->hwnd )) return FALSE;

      /* Only the key messages get special processing */
    if ((msg->message != WM_KEYDOWN) &&
        (msg->message != WM_SYSCHAR) &&
	(msg->message != WM_CHAR))
        return FALSE;

    dlgCode = SendMessage16( msg->hwnd, WM_GETDLGCODE, 0, 0 );
    if (dlgCode & DLGC_WANTMESSAGE)
    {
        DispatchMessage( msg );
        return TRUE;
    }

    switch(msg->message)
    {
    case WM_KEYDOWN:
        if (dlgCode & DLGC_WANTALLKEYS) break;
        switch(msg->wParam)
        {
        case VK_TAB:
            if (!(dlgCode & DLGC_WANTTAB))
            {
                SendMessage16( hwndDlg, WM_NEXTDLGCTL,
                             (GetKeyState(VK_SHIFT) & 0x80), 0 );
                return TRUE;
            }
            break;
            
        case VK_RIGHT:
        case VK_DOWN:
            if (!(dlgCode & DLGC_WANTARROWS))
            {
                SetFocus(GetNextDlgGroupItem(hwndDlg,GetFocus(),FALSE));
                return TRUE;
            }
            break;

        case VK_LEFT:
        case VK_UP:
            if (!(dlgCode & DLGC_WANTARROWS))
            {
                SetFocus(GetNextDlgGroupItem(hwndDlg,GetFocus(),TRUE));
                return TRUE;
            }
            break;

        case VK_ESCAPE:
#ifdef WINELIB32
            SendMessage32A( hwndDlg, WM_COMMAND, 
                            MAKEWPARAM( IDCANCEL, 0 ),
                            (LPARAM)GetDlgItem(hwndDlg,IDCANCEL) );
#else
            SendMessage16( hwndDlg, WM_COMMAND, IDCANCEL,
                           MAKELPARAM( GetDlgItem(hwndDlg,IDCANCEL), 0 ));
#endif
            break;

        case VK_RETURN:
            {
                DWORD dw = SendMessage16( hwndDlg, DM_GETDEFID, 0, 0 );
                if (HIWORD(dw) == DC_HASDEFID)
#ifdef WINELIB32
                    SendMessage32A( hwndDlg, WM_COMMAND, 
                                    MAKEWPARAM( LOWORD(dw), BN_CLICKED ),
                                   (LPARAM)GetDlgItem( hwndDlg, LOWORD(dw) ) );
                else
                    SendMessage32A( hwndDlg, WM_COMMAND, 
                                    MAKEWPARAM( IDOK, 0 ),
                                    (LPARAM)GetDlgItem(hwndDlg,IDOK) );
#else
                    SendMessage16( hwndDlg, WM_COMMAND, LOWORD(dw),
                                 MAKELPARAM( GetDlgItem( hwndDlg, LOWORD(dw) ),
                                             BN_CLICKED ));
                else
                    SendMessage16( hwndDlg, WM_COMMAND, IDOK,
                                 MAKELPARAM( GetDlgItem(hwndDlg,IDOK), 0 ));
#endif
            }
            break;

	default: 
	    TranslateMessage( msg );
        }
        break;  /* case WM_KEYDOWN */

        
    case WM_CHAR:
        if (dlgCode & (DLGC_WANTALLKEYS | DLGC_WANTCHARS)) break;
        break;

    case WM_SYSCHAR:
        if (dlgCode & DLGC_WANTALLKEYS) break;
        break;
    }

      /* If we get here, the message has not been treated specially */
      /* and can be sent to its destination window. */
    DispatchMessage( msg );
    return TRUE;
}


/****************************************************************
 *         GetDlgCtrlID           (USER.277)
 */
int GetDlgCtrlID( HWND hwnd )
{
    WND *wndPtr = WIN_FindWndPtr(hwnd);
    if (wndPtr) return wndPtr->wIDmenu;
    else return 0;
}
 

/***********************************************************************
 *           GetDlgItem   (USER.91)
 */
HWND GetDlgItem( HWND hwndDlg, WORD id )
{
    WND *pWnd;

    if (!(pWnd = WIN_FindWndPtr( hwndDlg ))) return 0;
    for (pWnd = pWnd->child; pWnd; pWnd = pWnd->next)
        if (pWnd->wIDmenu == id) return pWnd->hwndSelf;
    return 0;
}


/*******************************************************************
 *           SendDlgItemMessage16   (USER.101)
 */
LRESULT SendDlgItemMessage16( HWND16 hwnd, INT16 id, UINT16 msg,
                              WPARAM16 wParam, LPARAM lParam )
{
    HWND16 hwndCtrl = GetDlgItem( hwnd, id );
    if (hwndCtrl) return SendMessage16( hwndCtrl, msg, wParam, lParam );
    else return 0;
}


/*******************************************************************
 *           SendDlgItemMessage32A   (USER32.451)
 */
LRESULT SendDlgItemMessage32A( HWND32 hwnd, INT32 id, UINT32 msg,
                               WPARAM32 wParam, LPARAM lParam )
{
    HWND hwndCtrl = GetDlgItem( (HWND16)hwnd, (INT16)id );
    if (hwndCtrl) return SendMessage32A( hwndCtrl, msg, wParam, lParam );
    else return 0;
}


/*******************************************************************
 *           SendDlgItemMessage32W   (USER32.452)
 */
LRESULT SendDlgItemMessage32W( HWND32 hwnd, INT32 id, UINT32 msg,
                               WPARAM32 wParam, LPARAM lParam )
{
    HWND hwndCtrl = GetDlgItem( (HWND16)hwnd, (INT16)id );
    if (hwndCtrl) return SendMessage32W( hwndCtrl, msg, wParam, lParam );
    else return 0;
}


/*******************************************************************
 *           SetDlgItemText16   (USER.92)
 */
void SetDlgItemText16( HWND16 hwnd, INT16 id, SEGPTR lpString )
{
    SendDlgItemMessage16( hwnd, id, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *           SetDlgItemText32A   (USER32.477)
 */
void SetDlgItemText32A( HWND32 hwnd, INT32 id, LPCSTR lpString )
{
    SendDlgItemMessage32A( hwnd, id, WM_SETTEXT, 0, (LPARAM)lpString );
}


/*******************************************************************
 *           SetDlgItemText32W   (USER32.478)
 */
void SetDlgItemText32W( HWND32 hwnd, INT32 id, LPCWSTR lpString )
{
    SendDlgItemMessage32W( hwnd, id, WM_SETTEXT, 0, (LPARAM)lpString );
}


/***********************************************************************
 *           GetDlgItemText16   (USER.93)
 */
INT16 GetDlgItemText16( HWND16 hwnd, INT16 id, SEGPTR str, UINT16 len )
{
    return (INT16)SendDlgItemMessage16( hwnd, id, WM_GETTEXT,
                                        len, (LPARAM)str );
}


/***********************************************************************
 *           GetDlgItemText32A   (USER32.236)
 */
INT32 GetDlgItemText32A( HWND32 hwnd, INT32 id, LPSTR str, UINT32 len )
{
    return (INT32)SendDlgItemMessage32A( hwnd, id, WM_GETTEXT,
                                         len, (LPARAM)str );
}


/***********************************************************************
 *           GetDlgItemText32W   (USER32.237)
 */
INT32 GetDlgItemText32W( HWND32 hwnd, INT32 id, LPWSTR str, UINT32 len )
{
    return (INT32)SendDlgItemMessage32W( hwnd, id, WM_GETTEXT,
                                         len, (LPARAM)str );
}


/*******************************************************************
 *           SetDlgItemInt16   (USER.94)
 */
void SetDlgItemInt16( HWND16 hwnd, INT16 id, UINT16 value, BOOL16 fSigned )
{
    char *str = (char *)SEGPTR_ALLOC( 20 * sizeof(char) );

    if (!str) return;
    if (fSigned) sprintf( str, "%d", (INT32)(INT16)value );
    else sprintf( str, "%u", value );
    SendDlgItemMessage16( hwnd, id, WM_SETTEXT, 0, (LPARAM)SEGPTR_GET(str) );
    SEGPTR_FREE(str);
}


/*******************************************************************
 *           SetDlgItemInt32   (USER32.476)
 */
void SetDlgItemInt32( HWND32 hwnd, INT32 id, UINT32 value, BOOL32 fSigned )
{
    char str[20];

    if (fSigned) sprintf( str, "%d", (INT32)value );
    else sprintf( str, "%u", value );
    SendDlgItemMessage32A( hwnd, id, WM_SETTEXT, 0, (LPARAM)str );
}


/***********************************************************************
 *           GetDlgItemInt   (USER.95)
 */
WORD GetDlgItemInt( HWND hwnd, WORD id, BOOL * translated, BOOL fSigned )
{
    char *str;
    long result = 0;
    
    if (translated) *translated = FALSE;
    if (!(str = (char *)SEGPTR_ALLOC( 30 * sizeof(char) ))) return 0;
    if (SendDlgItemMessage16( hwnd, id, WM_GETTEXT, 30, (LPARAM)SEGPTR_GET(str)))
    {
	char * endptr;
	result = strtol( str, &endptr, 10 );
	if (endptr && (endptr != str))  /* Conversion was successful */
	{
	    if (fSigned)
	    {
		if ((result < -32767) || (result > 32767)) result = 0;
		else if (translated) *translated = TRUE;
	    }
	    else
	    {
		if ((result < 0) || (result > 65535)) result = 0;
		else if (translated) *translated = TRUE;
	    }
	}
    }
    SEGPTR_FREE(str);
    return (WORD)result;
}


/***********************************************************************
 *           CheckDlgButton   (USER.97)
 */
BOOL CheckDlgButton( HWND hwnd, INT id, UINT check )
{
    SendDlgItemMessage32A( hwnd, id, BM_SETCHECK32, check, 0 );
    return TRUE;
}


/***********************************************************************
 *           IsDlgButtonChecked   (USER.98)
 */
WORD IsDlgButtonChecked( HWND hwnd, WORD id )
{
    return (WORD)SendDlgItemMessage16( hwnd, id, BM_GETCHECK16, 0, 0 );
}


/***********************************************************************
 *           CheckRadioButton   (USER.96)
 */
BOOL CheckRadioButton( HWND hwndDlg, UINT firstID, UINT lastID, UINT checkID )
{
    WND *pWnd = WIN_FindWndPtr( hwndDlg );
    if (!pWnd) return FALSE;

    for (pWnd = pWnd->child; pWnd; pWnd = pWnd->next)
        if ((pWnd->wIDmenu == firstID) || (pWnd->wIDmenu == lastID)) break;
    if (!pWnd) return FALSE;

    if (pWnd->wIDmenu == lastID)
        lastID = firstID;  /* Buttons are in reverse order */
    while (pWnd)
    {
	SendMessage32A( pWnd->hwndSelf, BM_SETCHECK32,
                        (pWnd->wIDmenu == checkID), 0 );
        if (pWnd->wIDmenu == lastID) break;
	pWnd = pWnd->next;
    }
    return TRUE;
}


/***********************************************************************
 *           GetDialogBaseUnits   (USER.243)
 */
DWORD GetDialogBaseUnits()
{
    return MAKELONG( xBaseUnit, yBaseUnit );
}


/***********************************************************************
 *           MapDialogRect16   (USER.103)
 */
void MapDialogRect16( HWND16 hwnd, LPRECT16 rect )
{
    DIALOGINFO * dlgInfo;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return;
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    rect->left   = (rect->left * dlgInfo->xBaseUnit) / 4;
    rect->right  = (rect->right * dlgInfo->xBaseUnit) / 4;
    rect->top    = (rect->top * dlgInfo->yBaseUnit) / 8;
    rect->bottom = (rect->bottom * dlgInfo->yBaseUnit) / 8;
}


/***********************************************************************
 *           MapDialogRect32   (USER32.381)
 */
void MapDialogRect32( HWND32 hwnd, LPRECT32 rect )
{
    DIALOGINFO * dlgInfo;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return;
    dlgInfo = (DIALOGINFO *)wndPtr->wExtra;
    rect->left   = (rect->left * dlgInfo->xBaseUnit) / 4;
    rect->right  = (rect->right * dlgInfo->xBaseUnit) / 4;
    rect->top    = (rect->top * dlgInfo->yBaseUnit) / 8;
    rect->bottom = (rect->bottom * dlgInfo->yBaseUnit) / 8;
}


/***********************************************************************
 *           GetNextDlgGroupItem   (USER.227)
 */
HWND GetNextDlgGroupItem( HWND hwndDlg, HWND hwndCtrl, BOOL fPrevious )
{
    WND *pWnd, *pWndStart, *pWndCtrl, *pWndDlg;

    if (!(pWndDlg = WIN_FindWndPtr( hwndDlg ))) return 0;
    if (!(pWndCtrl = WIN_FindWndPtr( hwndCtrl ))) return 0;
    if (pWndCtrl->parent != pWndDlg) return 0;

    if (!fPrevious && pWndCtrl->next)  /* Check if next control is in group */
    {
        if (!(pWndCtrl->next->dwStyle & WS_GROUP))
            return pWndCtrl->next->hwndSelf;
    }

      /* Now we will have to find the start of the group */

    for (pWnd = pWndStart = pWndDlg->child; pWnd; pWnd = pWnd->next)
    {
        if (pWnd->dwStyle & WS_GROUP) pWndStart = pWnd;  /* Start of a group */
	if (pWnd == pWndCtrl) break;
    }

    if (!pWnd) fprintf(stderr, "GetNextDlgGroupItem: hwnd not in dialog!\n");

      /* only case left for forward search: wraparound */
    if (!fPrevious) return pWndStart->hwndSelf;

    pWnd = pWndStart->next;
    while (pWnd && (pWnd != pWndCtrl))
    {
        if (pWnd->dwStyle & WS_GROUP) break;
        pWndStart = pWnd;
        pWnd = pWnd->next;
    }
    return pWndStart->hwndSelf;
}


/***********************************************************************
 *           GetNextDlgTabItem   (USER.228)
 */
HWND GetNextDlgTabItem( HWND hwndDlg, HWND hwndCtrl, BOOL fPrevious )
{
    WND *pWnd, *pWndLast, *pWndCtrl, *pWndDlg;

    if (!(pWndDlg = WIN_FindWndPtr( hwndDlg ))) return 0;
    if (!(pWndCtrl = WIN_FindWndPtr( hwndCtrl ))) return 0;
    if (pWndCtrl->parent != pWndDlg) return 0;

    pWndLast = pWndCtrl;
    pWnd = pWndCtrl->next;
    while (1)
    {
        if (!pWnd) pWnd = pWndDlg->child;
        if (pWnd == pWndCtrl) break;
	if ((pWnd->dwStyle & WS_TABSTOP) && (pWnd->dwStyle & WS_VISIBLE))
	{
            pWndLast = pWnd;
	    if (!fPrevious) break;
	}
        pWnd = pWnd->next;
    }
    return pWndLast->hwndSelf;
}
