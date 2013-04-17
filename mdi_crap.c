#define _WIN32_WINNT 0x400
#define WINVER 0x500
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <richedit.h>
#include <math.h>
#include "Commctrl.h"
#include "resource.h"

extern HINSTANCE ghinstance;

typedef struct{
	int in_use;
	char name[1024];
	HWND hwnd,hbutton,hstatic,hlistview,hedit;
}DB_WINDOW;

static DB_WINDOW db_windows[100];

#include "treeview.h"

LRESULT CALLBACK MDIChildWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static int list_drag=FALSE,list_width=60;
	static DWORD tick=0;
	if(/*msg!=WM_NCMOUSEMOVE&&*/msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE/*&&msg!=WM_NOTIFY*/)
		//if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE)
	{
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("m");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
    switch(msg)
    {
	case WM_CREATE:
		{
		DB_WINDOW *win=0;
		LPCREATESTRUCT pcs = (LPCREATESTRUCT)lparam;
		LPMDICREATESTRUCT pmdics = (LPMDICREATESTRUCT)(pcs->lpCreateParams);
		win=pmdics->lParam;
		if(win!=0){
			win->hwnd=hwnd;
		}
		create_mdi_window(hwnd,ghinstance,win);
		}
        break;
	case WM_MOUSEACTIVATE:
		if(LOWORD(lparam)==HTCLIENT){
			//SetFocus(GetDlgItem(hwnd,MDI_EDIT));
			//return MA_NOACTIVATE;
		}
		break;
	case WM_VKEYTOITEM:
		break;
	case WM_SETFOCUS:
	case WM_IME_SETCONTEXT:
		break;
	case WM_MDIACTIVATE:
        break;
	case WM_CONTEXTMENU:
		break;
	case WM_SYSCOMMAND:
		switch(wparam&0xFFF0){
		case SC_MAXIMIZE:
			write_ini_str("SETTINGS","MDI_MAXIMIZED","1");
			break;
		case SC_RESTORE:
			write_ini_str("SETTINGS","MDI_MAXIMIZED","0");
			break;
		}
		break;

	case WM_CTLCOLORSTATIC:
		/*{
			RECT rect;
			HDC  hdc=(HDC)wparam;
			HWND ctrl=(HWND)lparam;
			COLORREF color=GetSysColor(COLOR_WINDOWTEXT);
			HBRUSH hbrush=0;
			hbrush=GetSysColorBrush(COLOR_BACKGROUND);
			if(last_static_msg!=WM_PAINT && last_static_msg!=WM_ERASEBKGND){
			//	GetClientRect(ctrl,&rect);
			//	FillRect(hdc,&rect,hbrush);
				InvalidateRect(ctrl,NULL,TRUE);
			}
			SetBkMode(hdc,TRANSPARENT);
			SetTextColor(hdc,color);
			return (LRESULT)hbrush;
		}*/
		break;
	case WM_DESTROY:
		break;
	case WM_ENDSESSION:
		break;
	case WM_CLOSE:
		break;
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
		ReleaseCapture();
		write_ini_value("SETTINGS","LIST_WIDTH",list_width);
		list_drag=FALSE;
		break;
	case WM_LBUTTONDOWN:
		SetCapture(hwnd);
		SetCursor(LoadCursor(NULL,IDC_SIZEWE));
		list_drag=TRUE;
		break;
	case WM_MOUSEFIRST:
		break;
    case WM_COMMAND:
		//HIWORD(wParam) notification code
		//LOWORD(wParam) item control
		//lParam handle of control
		switch(LOWORD(wparam)){
		}
		break;
	case WM_HELP:
		break;
	case WM_USER://custom edit input wparam=key
		break;
	case WM_SIZE:
		break;

    }
	return DefMDIChildProc(hwnd, msg, wparam, lparam);
}

int move_console()
{
	char title[MAX_PATH]={0}; 
	HWND hcon; 
	GetConsoleTitle(title,sizeof(title));
	if(title[0]!=0){
		hcon=FindWindow(NULL,title);
		SetWindowPos(hcon,0,600,0,800,600,SWP_NOZORDER);
	}
	return 0;
}
void open_console()
{
	char title[MAX_PATH]={0}; 
	HWND hcon; 
	FILE *hf;
	static BYTE consolecreated=FALSE;
	static int hcrt=0;
	
	if(consolecreated==TRUE)
	{
		GetConsoleTitle(title,sizeof(title));
		if(title[0]!=0){
			hcon=FindWindow(NULL,title);
			ShowWindow(hcon,SW_SHOW);
		}
		hcon=(HWND)GetStdHandle(STD_INPUT_HANDLE);
		FlushConsoleInputBuffer(hcon);
		return;
	}
	AllocConsole(); 
	hcrt=_open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE),_O_TEXT);

	fflush(stdin);
	hf=_fdopen(hcrt,"w"); 
	*stdout=*hf; 
	setvbuf(stdout,NULL,_IONBF,0);
	GetConsoleTitle(title,sizeof(title));
	if(title[0]!=0){
		hcon=FindWindow(NULL,title);
		ShowWindow(hcon,SW_SHOW); 
		SetForegroundWindow(hcon);
	}
	consolecreated=TRUE;
}
void hide_console()
{
	char title[MAX_PATH]={0}; 
	HANDLE hcon; 
	
	GetConsoleTitle(title,sizeof(title));
	if(title[0]!=0){
		hcon=FindWindow(NULL,title);
		ShowWindow(hcon,SW_HIDE);
		SetForegroundWindow(hcon);
	}
}

int create_listview_columns(HWND hlistview,char *list)
{
	LV_COLUMN col;
	int i;
	for(i=0;i<10;i++)
		ListView_InsertColumn(hlistview,i,&col);

}
int create_mdi_window(HWND hwnd,HINSTANCE hinstance,DB_WINDOW *win)
{
	HWND hstatic,hedit,hlistview=0;
	HMENU hmenu;
	RECT rect;
	if(win==0)
		return FALSE;

    GetClientRect(hwnd,&rect); 

    hlistview = CreateWindow(WC_LISTVIEW, 
                                     "",
                                     WS_TABSTOP|WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_SHOWSELALWAYS, //|LVS_OWNERDRAWFIXED, //|LVS_EDITLABELS,
                                     0, 32,
                                     rect.right - rect.left,
                                     rect.bottom - rect.top-60,
                                     hwnd,
                                     IDC_RESULTS,
                                     (HINSTANCE)GetWindowLong(hwnd,GWL_HINSTANCE),
                                     NULL);
	if(hlistview==0)
		return FALSE;
	win->hlistview=hlistview;
	//ListView_SetExtendedListViewStyle(hlistview,ListView_GetExtendedListViewStyle(hlistview)|LVS_EX_FULLROWSELECT);
	return TRUE;
}


int setup_mdi_classes(HINSTANCE hinstance)
{
	int result=TRUE;
    WNDCLASS wc;
	memset(&wc,0,sizeof(wc));
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = MDIChildWndProc;
    wc.hInstance     = hinstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
    wc.lpszClassName = "dbwindow";

    if(!RegisterClass(&wc))
		result=FALSE;
	/*
    wc.lpszClassName = "privmsgwindow";
    if(!RegisterClass(&wc))
		result=FALSE;
    wc.lpszClassName = "serverwindow";
    if(!RegisterClass(&wc))
		result=FALSE;
	*/
	return result;
}
int create_mdiclient(HWND hwnd,HMENU hmenu,HINSTANCE hinstance)
{
	CLIENTCREATESTRUCT MDIClientCreateStruct;
	HWND hmdiclient;
	MDIClientCreateStruct.hWindowMenu   = GetSubMenu(hmenu,2);
	MDIClientCreateStruct.idFirstChild  = 50000;
	hmdiclient = CreateWindow("MDICLIENT",NULL,
		WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VSCROLL|WS_HSCROLL|WS_VISIBLE, //0x56300001
		0,0,0,0,
		hwnd,
		IDC_MDI_CLIENT,//ID
		hinstance,
		(LPVOID)&MDIClientCreateStruct);
	return hmdiclient;
}

int create_mainwindow(void *wndproc,HMENU hmenu,HINSTANCE hinstance)
{
	WNDCLASS wndclass;
	HWND hframe=0;	
	memset(&wndclass,0,sizeof(wndclass));

	wndclass.style=0; //CS_HREDRAW|CS_VREDRAW;
	wndclass.lpfnWndProc=wndproc;
	wndclass.hCursor=LoadCursor(NULL, IDC_ARROW);
	wndclass.hInstance=hinstance;
	wndclass.hbrBackground=COLOR_BTNFACE+1;
	wndclass.lpszClassName="mdiframe";
	
	if(RegisterClass(&wndclass)!=0){	
		hframe = CreateWindow("mdiframe","DB_UTIL",
			WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_OVERLAPPEDWINDOW, //0x6CF0000
			0,0,
			400,300,
			NULL,           // handle to parent window
			hmenu,
			hinstance,
			NULL);
	}
	return hframe;
}
int create_dbview(HWND hwnd,HINSTANCE hinstance)
{
	//extern LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	WNDCLASS wndclass;
	HWND hswitch=0;
	memset(&wndclass,0,sizeof(wndclass));
	wndclass.lpfnWndProc=treeview_proc;
	wndclass.hCursor=LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground=COLOR_BTNFACE+1;
	wndclass.lpszClassName="dbview";
	wndclass.style=CS_HREDRAW|CS_VREDRAW;
	if(RegisterClass(&wndclass)!=0){
		hswitch=CreateWindow("dbview","dbview",
			WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VISIBLE|WS_BORDER,
			0,0,
			0,0,
			hwnd,
			IDC_TREEVIEW,
			hinstance,
			NULL);
	}
	return hswitch;
}
int create_db_window(HWND hmdiclient,DB_WINDOW *win)
{ 
	int maximized=0,style,handle;
	MDICREATESTRUCT cs;
	char title[256]={0};
	get_ini_value("SETTINGS","MDI_MAXIMIZED",&maximized);
	if(maximized!=0)
		style = WS_MAXIMIZE|MDIS_ALLCHILDSTYLES;
	else
		style = MDIS_ALLCHILDSTYLES;
	cs.cx=cs.cy=cs.x=cs.y=CW_USEDEFAULT;
	cs.szClass="dbwindow";
	cs.szTitle=title;
	cs.style=style;
	cs.hOwner=ghinstance;
	cs.lParam=win;
	handle=SendMessage(hmdiclient,WM_MDICREATE,0,&cs);
	return handle;
}

int acquire_db_window(DB_WINDOW **win)
{
	int i;
	for(i=0;i<sizeof(db_windows)/sizeof(DB_WINDOW);i++){
		if(!db_windows[i].in_use){
			*win=&db_windows[i];
			return TRUE;
		}
	}
	return FALSE;
}
int custom_dispatch(MSG *msg)
{
	return FALSE;
}

int create_popup_menus()
{
	return 0;
}

int init_mdi_stuff()
{
	extern int show_joins,lua_script_enable;
	int list_width=60;
	memset(&db_windows,0,sizeof(db_windows)/sizeof(DB_WINDOW));
	create_popup_menus();
	return TRUE;
}
