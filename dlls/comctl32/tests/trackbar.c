/* Unit tests for the track bar control.
 *
 * Copyright 2007 Keith Stevens
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#include "wine/test.h"
#include "msg.h"

#define expect(expected, got) ok(got == expected, "Expected %d, got %d\n", expected, got)
#define NUM_MSG_SEQUENCE 2
#define PARENT_SEQ_INDEX 0
#define TRACKBAR_SEQ_INDEX 1

static const DWORD defaultstyle = WS_VISIBLE | TBS_TOOLTIPS | TBS_ENABLESELRANGE | TBS_FIXEDLENGTH | TBS_AUTOTICKS;
static HWND hWndParent;

static struct msg_sequence *sequences[NUM_MSG_SEQUENCE];

static const struct message empty_seq[] = {
    {0}
};

static const struct message parent_create_trackbar_wnd_seq[] = {
    { WM_NOTIFYFORMAT, sent},
    { WM_QUERYUISTATE, sent|optional},
    { WM_WINDOWPOSCHANGING, sent},
    { WM_NCACTIVATE, sent},
    { PBT_APMRESUMECRITICAL, sent},
    { WM_WINDOWPOSCHANGING, sent},
    { PBT_APMRESUMESTANDBY, sent},
    { WM_IME_SETCONTEXT, sent|optional},
    { WM_IME_NOTIFY, sent|optional},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    {0}
};

static const struct message parent_new_window_test_seq[] = {
    { WM_QUERYNEWPALETTE, sent|optional },
    { WM_WINDOWPOSCHANGING, sent|optional},
    { WM_NCACTIVATE, sent|optional},
    { PBT_APMRESUMECRITICAL, sent|optional},
    { WM_IME_SETCONTEXT, sent|defwinproc|optional},
    { WM_IME_NOTIFY, sent|defwinproc|optional},
    { WM_SETFOCUS, sent|defwinproc|optional},
    { WM_NOTIFYFORMAT, sent},
    { WM_QUERYUISTATE, sent|optional},
    {0}
};

static const struct message buddy_window_test_seq[] = {
    { TBM_GETBUDDY, sent|wparam, TRUE},
    { TBM_SETBUDDY, sent|wparam, FALSE},
    { WM_PAINT, sent|defwinproc},
    { TBM_SETBUDDY, sent|wparam, FALSE},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETBUDDY, sent|wparam, TRUE},
    { TBM_SETBUDDY, sent|wparam, TRUE},
    { WM_PAINT, sent|defwinproc},
    { TBM_SETBUDDY, sent|wparam, TRUE},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETBUDDY, sent|wparam, FALSE},
    { TBM_GETBUDDY, sent|wparam, TRUE},
    {0}
};

static const struct message parent_buddy_window_test_seq[] = {
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    {0}
};

static const struct message line_size_test_seq[] = {
    { TBM_SETLINESIZE, sent|lparam, 0, 10},
    { TBM_SETLINESIZE, sent|lparam, 0, 4},
    { TBM_GETLINESIZE, sent},
    {0}
};

static const struct message page_size_test_seq[] = {
    { TBM_SETPAGESIZE, sent|lparam, 0, 10},
    { TBM_SETPAGESIZE, sent|lparam, 0, -1},
    { TBM_GETPAGESIZE, sent},
    {0}
};

static const struct message position_test_seq[] = {
    { TBM_SETPOS, sent|wparam|lparam, TRUE, -1},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETPOS, sent},
    { TBM_SETPOS, sent|wparam|lparam, TRUE, 5},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETPOS, sent},
    { TBM_SETPOS, sent|wparam|lparam, TRUE, 5},
    { TBM_SETPOS, sent|wparam|lparam, TRUE, 1000},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETPOS, sent},
    { TBM_SETPOS, sent|wparam|lparam, FALSE, 20},
    { TBM_GETPOS, sent},
    { TBM_SETPOS, sent|wparam|lparam, TRUE, 20},
    { TBM_GETPOS, sent},
    {0}
};

static const struct message parent_position_test_seq[] = {
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    {0}
};

static const struct message range_test_seq[] = {
    { TBM_SETRANGE, sent|wparam|lparam, TRUE, MAKELONG(0, 10)},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETRANGEMAX, sent},
    { TBM_GETRANGEMIN, sent},
    { TBM_SETRANGE, sent|wparam|lparam, TRUE, MAKELONG(-1, 1000)},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETRANGEMAX, sent},
    { TBM_GETRANGEMIN, sent},
    { TBM_SETRANGE, sent|wparam|lparam, TRUE, MAKELONG(10, 0)},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETRANGEMAX, sent},
    { TBM_GETRANGEMIN, sent},
    { TBM_SETRANGE, sent|wparam|lparam, FALSE, MAKELONG(0, 10)},
    { TBM_GETRANGEMAX, sent},
    { TBM_GETRANGEMIN, sent},
    { TBM_SETRANGEMAX, sent|wparam|lparam, TRUE, 10},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETRANGEMAX, sent},
    { TBM_SETRANGEMAX, sent|wparam|lparam, TRUE, -1},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETRANGEMAX, sent},
    { TBM_SETRANGEMAX, sent|wparam|lparam, FALSE, 10},
    { TBM_GETRANGEMAX, sent},
    { TBM_SETRANGEMIN, sent|wparam|lparam, TRUE, 0},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETRANGEMIN, sent},
    { TBM_SETRANGEMIN, sent|wparam|lparam, TRUE, 10},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETRANGEMIN, sent},
    { TBM_SETRANGEMIN, sent|wparam|lparam, TRUE, -10},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETRANGEMIN, sent},
    { TBM_SETRANGEMIN, sent|wparam|lparam, FALSE, 5},
    { TBM_GETRANGEMIN, sent},
    { TBM_GETRANGEMAX, sent},
    { TBM_GETRANGEMIN, sent},
    {0}
};

static const struct message parent_range_test_seq[] = {
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    {0}
};

static const struct message selection_test_seq[] = {
    { TBM_SETSEL, sent|wparam|lparam, TRUE, MAKELONG(0, 10)},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETSELEND, sent},
    { TBM_GETSELSTART, sent},
    { TBM_SETSEL, sent|wparam|lparam, TRUE, MAKELONG(5, 20)},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETSELEND, sent},
    { TBM_GETSELSTART, sent},
    { TBM_SETSEL, sent|wparam|lparam, FALSE, MAKELONG(5, 10)},
    { TBM_GETSELEND, sent},
    { TBM_GETSELSTART, sent},
    { TBM_SETSELEND, sent|wparam|lparam, TRUE, 10},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETSELEND, sent},
    { TBM_SETSELEND, sent|wparam|lparam, TRUE, 20},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETSELEND, sent},
    { TBM_SETSELEND, sent|wparam|lparam, TRUE, 4},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETSELEND, sent},
    { TBM_SETSELEND, sent|wparam|lparam, FALSE, 2},
    { TBM_GETSELEND, sent},
    { TBM_GETSELEND, sent},
    { TBM_SETSELSTART, sent|wparam|lparam, TRUE, 5},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETSELSTART, sent},
    { TBM_SETSELSTART, sent|wparam|lparam, TRUE, 0},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETSELSTART, sent},
    { TBM_SETSELSTART, sent|wparam|lparam, TRUE, 20},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETSELSTART, sent},
    { TBM_SETSELSTART, sent|wparam|lparam, FALSE, 8},
    { TBM_GETSELSTART, sent},
    { TBM_GETSELSTART, sent},
    {0}
};

static const struct message parent_selection_test_seq[] = {
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    {0}
};

static const struct message tic_settings_test_seq[] = {
    { TBM_SETTIC, sent|lparam, 0, 0},
    { TBM_SETTIC, sent|lparam, 0, 5},
    { TBM_SETTIC, sent|lparam, 0, 10},
    { TBM_SETTIC, sent|lparam, 0, 20},
    { TBM_SETRANGE, sent|wparam|lparam, TRUE, MAKELONG(0,10)},
    { WM_PAINT, sent|defwinproc},
    { TBM_SETTICFREQ, sent|wparam, 2},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETNUMTICS, sent},
    { TBM_SETTICFREQ, sent|wparam, 5},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETNUMTICS, sent},
    { TBM_SETTICFREQ, sent|wparam, 15},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETNUMTICS, sent},
    { TBM_GETNUMTICS, sent},
    {0}
};

static const struct message parent_tic_settings_test_seq[] = {
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    {0}
};

static const struct message thumb_length_test_seq[] = {
    { TBM_SETTHUMBLENGTH, sent|wparam|lparam, 15, 0},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETTHUMBLENGTH, sent},
    { TBM_SETTHUMBLENGTH, sent|wparam|lparam, 20, 0},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETTHUMBLENGTH, sent},
    { TBM_GETTHUMBLENGTH, sent},
    { WM_SIZE, sent},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETTHUMBLENGTH, sent},
    { WM_SIZE, sent},
    { WM_PAINT, sent|defwinproc},
    { TBM_GETTHUMBLENGTH, sent},
    {0}
};

static const struct message parent_thumb_length_test_seq[] = {
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    { WM_CTLCOLORSTATIC, sent},
    { WM_NOTIFY, sent},
    {0}
};

static const struct message tic_placement_test_seq[] = {
    { TBM_GETPTICS, sent},
    { TBM_GETTIC, sent|wparam, 0},
    { TBM_GETTIC, sent|wparam, 2},
    { TBM_GETTIC, sent|wparam, 4},
    { TBM_GETTICPOS, sent|wparam, 0},
    { TBM_GETTICPOS, sent|wparam, 2},
    {0}
};

static const struct message tool_tips_test_seq[] = {
    { TBM_SETTIPSIDE, sent|wparam, TBTS_TOP},
    { TBM_SETTIPSIDE, sent|wparam, TBTS_LEFT},
    { TBM_SETTIPSIDE, sent|wparam, TBTS_BOTTOM},
    { TBM_SETTIPSIDE, sent|wparam, TBTS_RIGHT},
    { TBM_SETTOOLTIPS, sent},
    { TBM_GETTOOLTIPS, sent},
    { TBM_SETTOOLTIPS, sent},
    { TBM_GETTOOLTIPS, sent},
    { TBM_SETTOOLTIPS, sent},
    { TBM_GETTOOLTIPS, sent},
    { TBM_GETTOOLTIPS, sent},
    {0}
};

static const struct message unicode_test_seq[] = {
    { TBM_SETUNICODEFORMAT, sent|wparam, TRUE},
    { TBM_SETUNICODEFORMAT, sent|wparam, FALSE},
    { TBM_GETUNICODEFORMAT, sent},
    {0}
};

static const struct message ignore_selection_test_seq[] = {
    { TBM_SETSEL, sent|wparam|lparam, TRUE, MAKELONG(0,10)},
    { TBM_GETSELEND, sent},
    { TBM_GETSELSTART, sent},
    { TBM_SETSEL, sent|wparam|lparam, FALSE, MAKELONG(0,10)},
    { TBM_GETSELEND, sent},
    { TBM_GETSELSTART, sent},
    { TBM_SETSELEND, sent|wparam|lparam, TRUE,0},
    { TBM_GETSELEND, sent},
    { TBM_SETSELEND, sent|wparam|lparam, TRUE, 10},
    { TBM_GETSELEND, sent},
    { TBM_SETSELEND, sent|wparam|lparam, FALSE,0},
    { TBM_GETSELEND, sent},
    { TBM_SETSELSTART, sent|wparam|lparam, TRUE,0},
    { TBM_GETSELSTART, sent},
    { TBM_SETSELSTART, sent|wparam|lparam, TRUE, 10},
    { TBM_GETSELSTART, sent},
    { TBM_SETSELSTART, sent|wparam|lparam, FALSE,0},
    { TBM_GETSELSTART, sent},
    {0}
};

static LRESULT WINAPI parent_wnd_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    /* log system messages, except for painting */
    if (message < WM_USER &&
        message != WM_PAINT &&
        message != WM_ERASEBKGND &&
        message != WM_NCPAINT &&
        message != WM_NCHITTEST &&
        message != WM_GETTEXT &&
        message != WM_GETICON &&
        message != WM_DEVICECHANGE)
    {
        msg.message = message;
        msg.flags = sent|wparam|lparam;
        if (defwndproc_counter) msg.flags |= defwinproc;
        msg.wParam = wParam;
        msg.lParam = lParam;
        msg.id = 0;
        add_message(sequences, PARENT_SEQ_INDEX, &msg);
    }

    defwndproc_counter++;
    ret = DefWindowProcA(hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static BOOL register_parent_wnd_class(void){
    WNDCLASSA cls;

    cls.style = 0;
    cls.lpfnWndProc = parent_wnd_proc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(NULL);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "Trackbar test parent class";
    return RegisterClassA(&cls);
}

static HWND create_parent_window(void){
    if (!register_parent_wnd_class())
        return NULL;

    return CreateWindowA("Trackbar test parent class", "Trackbar test parent window",
                         WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE,
                         0, 0, 100, 100, GetDesktopWindow(), NULL, GetModuleHandleA(NULL), NULL);
}

static LRESULT WINAPI trackbar_subclass_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){
    WNDPROC oldproc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    static LONG defwndproc_counter = 0;
    LRESULT ret;
    struct message msg;

    msg.message = message;
    msg.flags = sent|wparam|lparam;
    if (defwndproc_counter) msg.flags |= defwinproc;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.id = 0;
    add_message(sequences, TRACKBAR_SEQ_INDEX, &msg);

    defwndproc_counter++;
    ret = CallWindowProcA(oldproc, hwnd, message, wParam, lParam);
    defwndproc_counter--;

    return ret;
}

static HWND create_trackbar(DWORD style, HWND parent){
    HWND hWndTrack;
    WNDPROC oldproc;
    RECT rect;

    GetClientRect(parent, &rect);
    hWndTrack = CreateWindowA(TRACKBAR_CLASSA, "Trackbar Control", style,
                              rect.right, rect.bottom, 100, 50,
                              parent, NULL, GetModuleHandleA(NULL), NULL);

    if (!hWndTrack) return NULL;

    oldproc = (WNDPROC)SetWindowLongPtrA(hWndTrack, GWLP_WNDPROC, (LONG_PTR)trackbar_subclass_proc);
    SetWindowLongPtrA(hWndTrack, GWLP_USERDATA, (LONG_PTR)oldproc);

    return hWndTrack;
}

/* test functions for setters, getters, and sequences */

static void test_trackbar_buddy(void)
{
    HWND hWndLeftBuddy, hWndTrackbar;
    HWND hWndRightBuddy;
    HWND hWndCurrentBuddy;
    HWND rTest;

    hWndTrackbar = create_trackbar(defaultstyle, hWndParent);
    ok(hWndTrackbar != NULL, "Expected non NULL value\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCE);

    hWndLeftBuddy = CreateWindowA(STATUSCLASSNAMEA, NULL, 0, 0, 0, 300, 20, NULL, NULL, NULL, NULL);
    ok(hWndLeftBuddy != NULL, "Expected non NULL value\n");

    if (hWndLeftBuddy != NULL){
        hWndCurrentBuddy = (HWND) SendMessageA(hWndTrackbar, TBM_GETBUDDY, TRUE, 0);
        rTest = (HWND) SendMessageA(hWndTrackbar, TBM_SETBUDDY, FALSE, (LPARAM) hWndLeftBuddy);
        ok(rTest == hWndCurrentBuddy, "Expected hWndCurrentBuddy\n");
        rTest = (HWND) SendMessageA(hWndTrackbar, TBM_SETBUDDY, FALSE, (LPARAM) hWndLeftBuddy);
        ok(rTest == hWndLeftBuddy, "Expected hWndLeftBuddy\n");
    } else
        skip ("left buddy control not present?\n");

    hWndRightBuddy = CreateWindowA(STATUSCLASSNAMEA, NULL, 0, 0, 0, 300, 20, NULL, NULL, NULL, NULL);

    ok(hWndRightBuddy != NULL, "expected non NULL value\n");

    /* test TBM_SETBUDDY */
    if (hWndRightBuddy != NULL){
        hWndCurrentBuddy = (HWND) SendMessageA(hWndTrackbar, TBM_GETBUDDY, TRUE, 0);
        rTest = (HWND) SendMessageA(hWndTrackbar, TBM_SETBUDDY, TRUE, (LPARAM) hWndRightBuddy);
        ok(rTest == hWndCurrentBuddy, "Expected hWndCurrentBuddy\n");
        rTest = (HWND) SendMessageA(hWndTrackbar, TBM_SETBUDDY, TRUE, (LPARAM) hWndRightBuddy);
        ok(rTest == hWndRightBuddy, "Expected hWndRightbuddy\n");
     } else
       skip("Right buddy control not present?\n");

    /* test TBM_GETBUDDY */
    if (hWndLeftBuddy != NULL){
        rTest = (HWND) SendMessageA(hWndTrackbar, TBM_GETBUDDY, FALSE, 0);
        ok(rTest == hWndLeftBuddy, "Expected hWndLeftBuddy\n");
        DestroyWindow(hWndLeftBuddy);
    }
    if (hWndRightBuddy != NULL){
        rTest = (HWND) SendMessageA(hWndTrackbar, TBM_GETBUDDY, TRUE,0);
        ok(rTest == hWndRightBuddy, "Expected hWndRightBuddy\n");
        DestroyWindow(hWndRightBuddy);
    }

    ok_sequence(sequences, TRACKBAR_SEQ_INDEX, buddy_window_test_seq, "buddy test sequence", TRUE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_buddy_window_test_seq, "parent buddy test seq", TRUE);

    DestroyWindow(hWndTrackbar);
}

static void test_line_size(void)
{
    HWND hWndTrackbar;
    int r;

    hWndTrackbar = create_trackbar(defaultstyle, hWndParent);
    ok(hWndTrackbar != NULL, "Expected non NULL value\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCE);

    /* test TBM_SETLINESIZE */
    r = SendMessageA(hWndTrackbar, TBM_SETLINESIZE, 0, 10);
    expect(1,r);
    r = SendMessageA(hWndTrackbar, TBM_SETLINESIZE, 0, 4);
    expect(10, r);

    /* test TBM_GETLINESIZE */
    r = SendMessageA(hWndTrackbar, TBM_GETLINESIZE, 0,0);
    expect(4, r);

    ok_sequence(sequences, TRACKBAR_SEQ_INDEX, line_size_test_seq, "linesize test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "parent line test sequence", FALSE);

    DestroyWindow(hWndTrackbar);
}


static void test_page_size(void)
{
    HWND hWndTrackbar;
    int r;

    hWndTrackbar = create_trackbar(defaultstyle, hWndParent);
    ok(hWndTrackbar != NULL, "Expected non NULL value\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCE);

    /* test TBM_SETPAGESIZE */
    r = SendMessageA(hWndTrackbar, TBM_SETPAGESIZE, 0, 10);
    expect(20, r);
    r = SendMessageA(hWndTrackbar, TBM_SETPAGESIZE, 0, -1);
    expect(10, r);

    /* test TBM_GETPAGESIZE */
    r = SendMessageA(hWndTrackbar, TBM_GETPAGESIZE, 0,0);
    expect(20, r);

    ok_sequence(sequences, TRACKBAR_SEQ_INDEX, page_size_test_seq, "page size test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "parent page size test sequence", FALSE);

    /* check for zero page size */
    r = SendMessageA(hWndTrackbar, TBM_SETPAGESIZE, 0, 0);
    expect(20, r);
    r = SendMessageA(hWndTrackbar, TBM_GETPAGESIZE, 0, 0);
    expect(0, r);
    /* revert to default */
    r = SendMessageA(hWndTrackbar, TBM_SETPAGESIZE, 0, -1);
    expect(0, r);
    r = SendMessageA(hWndTrackbar, TBM_GETPAGESIZE, 0, 0);
    expect(20, r);
    /* < -1 */
    r = SendMessageA(hWndTrackbar, TBM_SETPAGESIZE, 0, -2);
    expect(20, r);
    r = SendMessageA(hWndTrackbar, TBM_GETPAGESIZE, 0, 0);
    expect(-2, r);

    DestroyWindow(hWndTrackbar);
}

static void test_position(void)
{
    HWND hWndTrackbar;
    int r;

    hWndTrackbar = create_trackbar(defaultstyle, hWndParent);
    ok(hWndTrackbar != NULL, "Expected non NULL value\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCE);

    /* test TBM_SETPOS */
    SendMessageA(hWndTrackbar, TBM_SETPOS, TRUE, -1);
    r = SendMessageA(hWndTrackbar, TBM_GETPOS, 0, 0);
    expect(0, r);
    SendMessageA(hWndTrackbar, TBM_SETPOS, TRUE, 5);
    r = SendMessageA(hWndTrackbar, TBM_GETPOS, 0,0);
    expect(5, r);
    SendMessageA(hWndTrackbar, TBM_SETPOS, TRUE, 5);
    SendMessageA(hWndTrackbar, TBM_SETPOS, TRUE, 1000);
    r = SendMessageA(hWndTrackbar, TBM_GETPOS, 0,0);
    expect(100, r);
    SendMessageA(hWndTrackbar, TBM_SETPOS, FALSE, 20);
    r = SendMessageA(hWndTrackbar, TBM_GETPOS, 0,0);
    expect(20, r);
    SendMessageA(hWndTrackbar, TBM_SETPOS, TRUE, 20);

    /* test TBM_GETPOS */
    r = SendMessageA(hWndTrackbar, TBM_GETPOS, 0,0);
    expect(20, r);

    ok_sequence(sequences, TRACKBAR_SEQ_INDEX, position_test_seq, "position test sequence", TRUE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_position_test_seq, "parent position test sequence", TRUE);

    DestroyWindow(hWndTrackbar);
}

static void test_range(void)
{
    HWND hWndTrackbar;
    int r;

    hWndTrackbar = create_trackbar(defaultstyle, hWndParent);
    ok(hWndTrackbar != NULL, "Expected non NULL value\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCE);

    /* test TBM_SETRANGE */
    SendMessageA(hWndTrackbar, TBM_SETRANGE, TRUE, MAKELONG(0, 10));
    r = SendMessageA(hWndTrackbar, TBM_GETRANGEMAX, 0,0);
    expect(10, r);
    r = SendMessageA(hWndTrackbar, TBM_GETRANGEMIN, 0,0);
    expect(0, r);
    SendMessageA(hWndTrackbar, TBM_SETRANGE, TRUE, MAKELONG(-1, 1000));
    r = SendMessageA(hWndTrackbar, TBM_GETRANGEMAX, 0,0);
    expect(1000, r);
    r = SendMessageA(hWndTrackbar, TBM_GETRANGEMIN, 0,0);
    expect(-1, r);
    SendMessageA(hWndTrackbar, TBM_SETRANGE, TRUE, MAKELONG(10, 0));
    r = SendMessageA(hWndTrackbar, TBM_GETRANGEMAX, 0,0);
    expect(0, r);
    r = SendMessageA(hWndTrackbar, TBM_GETRANGEMIN, 0,0);
    expect(10, r);
    SendMessageA(hWndTrackbar, TBM_SETRANGE, FALSE, MAKELONG(0,10));
    r = SendMessageA(hWndTrackbar, TBM_GETRANGEMAX, 0,0);
    expect(10, r);
    r = SendMessageA(hWndTrackbar, TBM_GETRANGEMIN, 0,0);
    expect(0, r);

    /*test TBM_SETRANGEMAX */
    SendMessageA(hWndTrackbar, TBM_SETRANGEMAX, TRUE, 10);
    r = SendMessageA(hWndTrackbar, TBM_GETRANGEMAX, 0,0);
    expect(10, r);
    SendMessageA(hWndTrackbar, TBM_SETRANGEMAX, TRUE, -1);
    r = SendMessageA(hWndTrackbar, TBM_GETRANGEMAX, 0,0);
    expect(-1, r);
    SendMessageA(hWndTrackbar, TBM_SETRANGEMAX, FALSE, 10);
    r = SendMessageA(hWndTrackbar, TBM_GETRANGEMAX, 0,0);
    expect(10, r);

    /* testing TBM_SETRANGEMIN */
    SendMessageA(hWndTrackbar, TBM_SETRANGEMIN, TRUE, 0);
    r = SendMessageA(hWndTrackbar, TBM_GETRANGEMIN, 0,0);
    expect(0, r);
    SendMessageA(hWndTrackbar, TBM_SETRANGEMIN, TRUE, 10);
    r = SendMessageA(hWndTrackbar, TBM_GETRANGEMIN, 0,0);
    expect(10, r);
    SendMessageA(hWndTrackbar, TBM_SETRANGEMIN, TRUE, -10);
    r = SendMessageA(hWndTrackbar, TBM_GETRANGEMIN, 0,0);
    expect(-10, r);
    SendMessageA(hWndTrackbar, TBM_SETRANGEMIN, FALSE, 5);
    r = SendMessageA(hWndTrackbar, TBM_GETRANGEMIN, 0,0);
    expect(5, r);

    /* test TBM_GETRANGEMAX */
    r = SendMessageA(hWndTrackbar, TBM_GETRANGEMAX, 0,0);
    expect(10, r);

    /* test TBM_GETRANGEMIN */
    r = SendMessageA(hWndTrackbar, TBM_GETRANGEMIN, 0,0);
    expect(5, r);

    ok_sequence(sequences, TRACKBAR_SEQ_INDEX, range_test_seq, "range test sequence", TRUE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_range_test_seq, "parent range test sequence", TRUE);

    DestroyWindow(hWndTrackbar);
}

static void test_selection(void)
{
    HWND hWndTrackbar;
    int r;

    hWndTrackbar = create_trackbar(defaultstyle, hWndParent);
    ok(hWndTrackbar != NULL, "Expected non NULL value\n");

    SendMessageA(hWndTrackbar, TBM_SETRANGEMIN, FALSE, 5);
    SendMessageA(hWndTrackbar, TBM_SETRANGEMAX, FALSE, 10);

    flush_sequences(sequences, NUM_MSG_SEQUENCE);

    /* test TBM_SETSEL */
    SendMessageA(hWndTrackbar, TBM_SETSEL, TRUE, MAKELONG(0,10));
    r = SendMessageA(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(10, r);
    r = SendMessageA(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(5, r);
    SendMessageA(hWndTrackbar, TBM_SETSEL, TRUE, MAKELONG(5, 20));
    r = SendMessageA(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(10, r);
    r = SendMessageA(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(5, r);
    SendMessageA(hWndTrackbar, TBM_SETSEL, FALSE, MAKELONG(5, 10));
    r = SendMessageA(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(10, r);
    r = SendMessageA(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(5, r);

    /* test TBM_SETSELEND */
    SendMessageA(hWndTrackbar, TBM_SETSELEND, TRUE, 10);
    r = SendMessageA(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(10, r);
    SendMessageA(hWndTrackbar, TBM_SETSELEND, TRUE, 20);
    r = SendMessageA(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(10, r);
    SendMessageA(hWndTrackbar, TBM_SETSELEND, TRUE, 4);
    r = SendMessageA(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(4, r);
    SendMessageA(hWndTrackbar, TBM_SETSELEND, FALSE, 2);
    r = SendMessageA(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(2, r);

    /* test TBM_GETSELEND */
    r = SendMessageA(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(2, r);

    /* testing TBM_SETSELSTART */
    SendMessageA(hWndTrackbar, TBM_SETSELSTART, TRUE, 5);
    r = SendMessageA(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(5, r);
    SendMessageA(hWndTrackbar, TBM_SETSELSTART, TRUE, 0);
    r = SendMessageA(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(5, r);
    SendMessageA(hWndTrackbar, TBM_SETSELSTART, TRUE, 20);
    r = SendMessageA(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(20, r);
    SendMessageA(hWndTrackbar, TBM_SETSELSTART, FALSE, 8);
    r = SendMessageA(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(8, r);

    /* test TBM_GETSELSTART */
    r = SendMessageA(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(8, r);

    ok_sequence(sequences, TRACKBAR_SEQ_INDEX, selection_test_seq, "selection test sequence", TRUE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_selection_test_seq, "parent selection test sequence", TRUE);

    DestroyWindow(hWndTrackbar);
}

static void test_thumb_length(void)
{
    HWND hWndTrackbar;
    int r;

    hWndTrackbar = create_trackbar(defaultstyle, hWndParent);
    ok(hWndTrackbar != NULL, "Expected non NULL value\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCE);

    /* testing TBM_SETTHUMBLENGTH */
    SendMessageA(hWndTrackbar, TBM_SETTHUMBLENGTH, 15, 0);
    r = SendMessageA(hWndTrackbar, TBM_GETTHUMBLENGTH, 0,0);
    expect(15, r);
    SendMessageA(hWndTrackbar, TBM_SETTHUMBLENGTH, 20, 0);
    r = SendMessageA(hWndTrackbar, TBM_GETTHUMBLENGTH, 0,0);
    expect(20, r);

    /* test TBM_GETTHUMBLENGTH */
    r = SendMessageA(hWndTrackbar, TBM_GETTHUMBLENGTH, 0,0);
    expect(20, r);

    r = SendMessageA(hWndTrackbar, WM_SIZE, 0,0);
    expect(0, r);
    r = SendMessageA(hWndTrackbar, TBM_GETTHUMBLENGTH, 0,0);
    expect(20, r);
    r = SendMessageA(hWndTrackbar, WM_SIZE, 0, MAKELPARAM(50, 50) );
    expect(0, r);
    r = SendMessageA(hWndTrackbar, TBM_GETTHUMBLENGTH, 0,0);
    expect(20, r);

    ok_sequence(sequences, TRACKBAR_SEQ_INDEX, thumb_length_test_seq, "thumb length test sequence", TRUE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_thumb_length_test_seq, "parent thumb length test sequence", TRUE);

    DestroyWindow(hWndTrackbar);
}

static void test_tic_settings(void)
{
    HWND hWndTrackbar;
    int r;

    hWndTrackbar = create_trackbar(defaultstyle, hWndParent);
    ok(hWndTrackbar != NULL, "Expected non NULL value\n");

    SendMessageA(hWndTrackbar, TBM_SETRANGEMIN, FALSE, 5);
    SendMessageA(hWndTrackbar, TBM_SETRANGEMAX, FALSE, 10);

    /* testing TBM_SETTIC */
    /* Set tics at 5 and 10 */
    /* 0 and 20 are out of range and should not be set */
    r = SendMessageA(hWndTrackbar, TBM_GETRANGEMAX, 0, 0);
    expect(10, r);
    r = SendMessageA(hWndTrackbar, TBM_GETRANGEMIN, 0, 0);
    expect(5, r);

    flush_sequences(sequences, NUM_MSG_SEQUENCE);
    r = SendMessageA(hWndTrackbar, TBM_SETTIC, 0, 0);
    ok(r == FALSE, "Expected FALSE, got %d\n", r);
    r = SendMessageA(hWndTrackbar, TBM_SETTIC, 0, 5);
    ok(r == TRUE, "Expected TRUE, got %d\n", r);
    r = SendMessageA(hWndTrackbar, TBM_SETTIC, 0, 10);
    ok(r == TRUE, "Expected TRUE, got %d\n", r);

    r = SendMessageA(hWndTrackbar, TBM_SETTIC, 0, 20);
    ok(r == FALSE, "Expected FALSE, got %d\n", r);

    /* test TBM_SETTICFREQ */
    SendMessageA(hWndTrackbar, TBM_SETRANGE, TRUE, MAKELONG(0, 10));
    SendMessageA(hWndTrackbar, TBM_SETTICFREQ, 2, 0);
    r = SendMessageA(hWndTrackbar, TBM_GETNUMTICS, 0, 0);
    expect(6, r);
    SendMessageA(hWndTrackbar, TBM_SETTICFREQ, 5, 0);
    r = SendMessageA(hWndTrackbar, TBM_GETNUMTICS, 0, 0);
    expect(3, r);
    SendMessageA(hWndTrackbar, TBM_SETTICFREQ, 15, 0);
    r = SendMessageA(hWndTrackbar, TBM_GETNUMTICS, 0, 0);
    expect(2, r);

    /* test TBM_GETNUMTICS */
    /* since TIC FREQ is 15, there should be only 2 tics now */
    r = SendMessageA(hWndTrackbar, TBM_GETNUMTICS, 0, 0);
    expect(2, r);

    ok_sequence(sequences, TRACKBAR_SEQ_INDEX, tic_settings_test_seq, "tic settings test sequence", TRUE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_tic_settings_test_seq, "parent tic settings test sequence", TRUE);

    /* range [0,0], freq = 1 */
    SendMessageA(hWndTrackbar, TBM_SETRANGEMAX, TRUE, 0);
    SendMessageA(hWndTrackbar, TBM_SETRANGEMIN, TRUE, 0);
    SendMessageA(hWndTrackbar, TBM_SETTICFREQ, 1, 0);
    r = SendMessageA(hWndTrackbar, TBM_GETNUMTICS, 0, 0);
    expect(2, r);
    /* range [0,1], freq = 1 */
    SendMessageA(hWndTrackbar, TBM_SETRANGEMAX, TRUE, 1);
    SendMessageA(hWndTrackbar, TBM_SETRANGEMIN, TRUE, 0);
    SendMessageA(hWndTrackbar, TBM_SETTICFREQ, 1, 0);
    r = SendMessageA(hWndTrackbar, TBM_GETNUMTICS, 0, 0);
    expect(2, r);
    /* range [0,2], freq = 1 */
    SendMessageA(hWndTrackbar, TBM_SETRANGEMAX, TRUE, 2);
    SendMessageA(hWndTrackbar, TBM_SETRANGEMIN, TRUE, 0);
    SendMessageA(hWndTrackbar, TBM_SETTICFREQ, 1, 0);
    r = SendMessageA(hWndTrackbar, TBM_GETNUMTICS, 0, 0);
    expect(3, r);
}

static void test_tic_placement(void)
{
    HWND hWndTrackbar;
    int r;
    DWORD *rPTics;
    DWORD numtics;

    hWndTrackbar = create_trackbar(defaultstyle, hWndParent);
    ok(hWndTrackbar != NULL, "Expected non NULL value\n");

    SendMessageA(hWndTrackbar, TBM_SETRANGE, TRUE, MAKELONG(1, 6));
    SendMessageA(hWndTrackbar, TBM_SETTICFREQ, 1, 0);

    numtics = SendMessageA(hWndTrackbar, TBM_GETNUMTICS, 0, 0);
    ok(numtics == 6, "Expected 6, got %d\n", numtics);

    flush_sequences(sequences, NUM_MSG_SEQUENCE);
    /* test TBM_GETPTICS */
    rPTics = (DWORD *) SendMessageA(hWndTrackbar, TBM_GETPTICS, 0,0);
    expect(2, rPTics[0]);
    expect(3, rPTics[1]);
    expect(4, rPTics[2]);
    expect(5, rPTics[3]);

    /* test TBM_GETTIC */
    r = SendMessageA(hWndTrackbar, TBM_GETTIC, 0,0);
    expect(2, r);
    r = SendMessageA(hWndTrackbar, TBM_GETTIC, 2,0);
    expect(4, r);
    r = SendMessageA(hWndTrackbar, TBM_GETTIC, 4,0);
    expect(-1, r);

    /* test TBM_GETTICPIC */
    r = SendMessageA(hWndTrackbar, TBM_GETTICPOS, 0, 0);
    ok(r > 0, "Expected r > 0, got %d\n", r);
    r = SendMessageA(hWndTrackbar, TBM_GETTICPOS, 2, 0);
    ok(r > 0, "Expected r > 0, got %d\n", r);

    ok_sequence(sequences, TRACKBAR_SEQ_INDEX, tic_placement_test_seq, "get tic placement test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "parent get tic placement test sequence", FALSE);

    DestroyWindow(hWndTrackbar);
}

static void test_tool_tips(void)
{
    HWND hWndTooltip, hWndTrackbar;
    HWND rTest;
    int r;

    hWndTrackbar = create_trackbar(defaultstyle, hWndParent);
    ok(hWndTrackbar != NULL, "Expected non NULL value\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCE);

    /* testing TBM_SETTIPSIDE */
    r = SendMessageA(hWndTrackbar, TBM_SETTIPSIDE, TBTS_TOP, 0);
    expect(TBTS_TOP, r);
    r = SendMessageA(hWndTrackbar, TBM_SETTIPSIDE, TBTS_LEFT, 0);
    expect(TBTS_TOP, r);
    r = SendMessageA(hWndTrackbar, TBM_SETTIPSIDE, TBTS_BOTTOM, 0);
    expect(TBTS_LEFT, r);
    r = SendMessageA(hWndTrackbar, TBM_SETTIPSIDE, TBTS_RIGHT, 0);
    expect(TBTS_BOTTOM, r);

    /* testing TBM_SETTOOLTIPS */
    hWndTooltip = CreateWindowExA(WS_EX_TOPMOST, TOOLTIPS_CLASSA, NULL, 0,
                                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                  NULL, NULL, NULL, NULL);

    ok(hWndTooltip != NULL, "Expected non NULL value\n");
    if (hWndTooltip != NULL){
        SendMessageA(hWndTrackbar, TBM_SETTOOLTIPS, (LPARAM) hWndTooltip, 0);
        rTest = (HWND) SendMessageA(hWndTrackbar, TBM_GETTOOLTIPS, 0,0);
        ok(rTest == hWndTooltip, "Expected hWndToolTip, got\n");
        SendMessageA(hWndTrackbar, TBM_SETTOOLTIPS, 0, 0);
        rTest = (HWND) SendMessageA(hWndTrackbar, TBM_GETTOOLTIPS, 0,0);
        ok(rTest == NULL, "Expected NULL\n");
        SendMessageA(hWndTrackbar, TBM_SETTOOLTIPS, (LPARAM) hWndTooltip, 5);
        rTest = (HWND) SendMessageA(hWndTrackbar, TBM_GETTOOLTIPS, 0,0);
        ok(rTest == hWndTooltip, "Expected hWndTooltip, got\n");
    } else
        skip("tool tip control not present?\n");

    /* test TBM_GETTOOLTIPS */
    rTest = (HWND) SendMessageA(hWndTrackbar, TBM_GETTOOLTIPS, 0,0);
    ok(rTest == hWndTooltip, "Expected hWndTooltip\n");

    ok_sequence(sequences, TRACKBAR_SEQ_INDEX, tool_tips_test_seq, "tool tips test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "parent tool tips test sequence", FALSE);

    DestroyWindow(hWndTrackbar);
}


static void test_unicode(void)
{
    HWND hWndTrackbar;
    int r;

    hWndTrackbar = create_trackbar(defaultstyle, hWndParent);
    ok(hWndTrackbar != NULL, "Expected non NULL value\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCE);

    /* testing TBM_SETUNICODEFORMAT */
    r = SendMessageA(hWndTrackbar, TBM_SETUNICODEFORMAT, TRUE, 0);
    ok(r == FALSE, "Expected FALSE, got %d\n",r);
    r = SendMessageA(hWndTrackbar, TBM_SETUNICODEFORMAT, FALSE, 0);
    ok(r == TRUE, "Expected TRUE, got %d\n",r);

    /* test TBM_GETUNICODEFORMAT */
    r = SendMessageA(hWndTrackbar, TBM_GETUNICODEFORMAT, 0,0);
    ok(r == FALSE, "Expected FALSE, got %d\n",r);

    ok_sequence(sequences, TRACKBAR_SEQ_INDEX, unicode_test_seq, "unicode test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "parent unicode test sequence", FALSE);

    DestroyWindow(hWndTrackbar);
}

static void test_ignore_selection(void)
{
    HWND hWndTrackbar;
    int r;

    hWndTrackbar = create_trackbar(0, hWndParent);
    ok(hWndTrackbar != NULL, "Expected non NULL value\n");

    flush_sequences(sequences, NUM_MSG_SEQUENCE);
    /* test TBM_SETSEL  ensure that it is ignored */
    SendMessageA(hWndTrackbar, TBM_SETSEL, TRUE, MAKELONG(0,10));
    r = SendMessageA(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(0, r);
    r = SendMessageA(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(0, r);
    SendMessageA(hWndTrackbar, TBM_SETSEL, FALSE, MAKELONG(0,10));
    r = SendMessageA(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(0, r);
    r = SendMessageA(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(0, r);

    /* test TBM_SETSELEND, ensure that it is ignored */
    SendMessageA(hWndTrackbar, TBM_SETSELEND, TRUE, 0);
    r = SendMessageA(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(0, r);
    SendMessageA(hWndTrackbar, TBM_SETSELEND, TRUE, 10);
    r = SendMessageA(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(0,r);
    SendMessageA(hWndTrackbar, TBM_SETSELEND, FALSE, 0);
    r = SendMessageA(hWndTrackbar, TBM_GETSELEND, 0,0);
    expect(0, r);

    /* test TBM_SETSELSTART, ensure that it is ignored */
    SendMessageA(hWndTrackbar, TBM_SETSELSTART, TRUE, 0);
    r = SendMessageA(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(0, r);
    SendMessageA(hWndTrackbar, TBM_SETSELSTART, TRUE, 10);
    r = SendMessageA(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(0,r);
    SendMessageA(hWndTrackbar, TBM_SETSELSTART, FALSE, 0);
    r = SendMessageA(hWndTrackbar, TBM_GETSELSTART, 0,0);
    expect(0, r);

    ok_sequence(sequences, TRACKBAR_SEQ_INDEX, ignore_selection_test_seq, "ignore selection setting test sequence", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, empty_seq, "parent ignore selection setting test sequence", FALSE);

    DestroyWindow(hWndTrackbar);
}

static void test_initial_state(void)
{
    HWND hWnd;
    int ret;

    hWnd = create_trackbar(0, hWndParent);

    ret = SendMessageA(hWnd, TBM_GETNUMTICS, 0, 0);
    expect(2, ret);
    ret = SendMessageA(hWnd, TBM_GETTIC, 0, 0);
    expect(-1, ret);
    ret = SendMessageA(hWnd, TBM_GETTICPOS, 0, 0);
    expect(-1, ret);
    ret = SendMessageA(hWnd, TBM_GETRANGEMIN, 0, 0);
    expect(0, ret);
    ret = SendMessageA(hWnd, TBM_GETRANGEMAX, 0, 0);
    expect(100, ret);

    ret = SendMessageA(hWnd, TBM_SETRANGEMAX, TRUE, 200);
    expect(0, ret);

    ret = SendMessageA(hWnd, TBM_GETNUMTICS, 0, 0);
    expect(2, ret);

    ret = SendMessageA(hWnd, TBM_SETRANGEMIN, TRUE, 10);
    expect(0, ret);

    ret = SendMessageA(hWnd, TBM_GETNUMTICS, 0, 0);
    expect(2, ret);

    DestroyWindow(hWnd);
}

static void test_TBS_AUTOTICKS(void)
{
    HWND hWnd;
    int ret;

    hWnd = create_trackbar(TBS_AUTOTICKS, hWndParent);

    ret = SendMessageA(hWnd, TBM_GETNUMTICS, 0, 0);
    expect(2, ret);
    ret = SendMessageA(hWnd, TBM_GETTIC, 0, 0);
    expect(-1, ret);
    ret = SendMessageA(hWnd, TBM_GETTICPOS, 0, 0);
    expect(-1, ret);
    ret = SendMessageA(hWnd, TBM_GETRANGEMIN, 0, 0);
    expect(0, ret);
    ret = SendMessageA(hWnd, TBM_GETRANGEMAX, 0, 0);
    expect(100, ret);

    /* TBM_SETRANGEMAX rebuilds tics */
    ret = SendMessageA(hWnd, TBM_SETRANGEMAX, TRUE, 200);
    expect(0, ret);
    ret = SendMessageA(hWnd, TBM_GETNUMTICS, 0, 0);
    expect(201, ret);

    /* TBM_SETRANGEMIN rebuilds tics */
    ret = SendMessageA(hWnd, TBM_SETRANGEMAX, TRUE, 100);
    expect(0, ret);
    ret = SendMessageA(hWnd, TBM_SETRANGEMIN, TRUE, 10);
    expect(0, ret);
    ret = SendMessageA(hWnd, TBM_GETNUMTICS, 0, 0);
    expect(91, ret);

    ret = SendMessageA(hWnd, TBM_SETRANGEMIN, TRUE, 0);
    expect(0, ret);

    /* TBM_SETRANGE rebuilds tics */
    ret = SendMessageA(hWnd, TBM_SETRANGE, TRUE, MAKELONG(10, 200));
    expect(0, ret);
    ret = SendMessageA(hWnd, TBM_GETNUMTICS, 0, 0);
    expect(191, ret);

    DestroyWindow(hWnd);
}

static void test_create(void)
{
    HWND hWndTrackbar;

    flush_sequences(sequences, NUM_MSG_SEQUENCE);

    hWndTrackbar = create_trackbar(defaultstyle, hWndParent);
    ok(hWndTrackbar != NULL, "Expected non NULL value\n");
    ok_sequence(sequences, TRACKBAR_SEQ_INDEX, empty_seq, "create Trackbar Window", FALSE);
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_create_trackbar_wnd_seq, "parent trackbar window", TRUE);

    DestroyWindow(hWndTrackbar);

    /* no style bits */
    flush_sequences(sequences, NUM_MSG_SEQUENCE);
    hWndTrackbar = create_trackbar(0, hWndParent);
    ok(hWndTrackbar != NULL, "Expected non NULL value\n");
    ok_sequence(sequences, PARENT_SEQ_INDEX, parent_new_window_test_seq, "new trackbar window test sequence", TRUE);
    DestroyWindow(hWndTrackbar);
}

START_TEST(trackbar)
{
    init_msg_sequences(sequences, NUM_MSG_SEQUENCE);
    InitCommonControls();

    /* create parent window */
    hWndParent = create_parent_window();
    ok(hWndParent != NULL, "Failed to create parent Window!\n");

    if(!hWndParent){
        skip("parent window not present\n");
        return;
    }

    test_create();
    test_trackbar_buddy();
    test_line_size();
    test_page_size();
    test_position();
    test_range();
    test_selection();
    test_thumb_length();
    test_tic_settings();
    test_tic_placement();
    test_tool_tips();
    test_unicode();
    test_TBS_AUTOTICKS();
    test_ignore_selection();
    test_initial_state();

    DestroyWindow(hWndParent);
}
