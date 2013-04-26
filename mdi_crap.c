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
extern HWND ghmainframe,ghmdiclient,ghtreeview;

typedef struct{
	char name[1024];
	void *hdbc;
	void *hdbenv;
	int abort;
	HWND hwnd,hbutton,hstatic,hlistview,hedit,hroot;
}TABLE_WINDOW;

typedef struct{
	char name[1024];
	void *hdbc;
	void *hdbenv;
	HWND htree,hroot;
}DB_TREE;


static TABLE_WINDOW table_windows[5];
static DB_TREE db_tree[5];

#include "treeview.h"
#include "listview.h"


LRESULT CALLBACK MDIChildWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static int split_drag=FALSE,mdi_split=60;
	static DWORD tick=0;
	if(FALSE)
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
		TABLE_WINDOW *win=0;
		LPCREATESTRUCT pcs = (LPCREATESTRUCT)lparam;
		LPMDICREATESTRUCT pmdics = (LPMDICREATESTRUCT)(pcs->lpCreateParams);
		win=pmdics->lParam;
		if(win!=0){
			win->hwnd=hwnd;
		}
		create_mdi_window(hwnd,ghinstance,win);
		load_mdi_size(hwnd);
		resize_mdi_window(hwnd,mdi_split);
		//test_listview(win->hlistview);
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
		{
			TABLE_WINDOW *win=0;
			if(find_win_by_hwnd(hwnd,&win)){
				free_window(win);
			}
		}
		break;
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
		ReleaseCapture();
		write_ini_value("SETTINGS","MDI_SPLIT",mdi_split);
		split_drag=FALSE;
		break;
	case WM_LBUTTONDOWN:
		SetCapture(hwnd);
		SetCursor(LoadCursor(NULL,IDC_SIZENS));
		split_drag=TRUE;
		break;
	case WM_MOUSEFIRST:
		{
			int y=HIWORD(lparam);
			int x=LOWORD(lparam);
			SetCursor(LoadCursor(NULL,IDC_SIZENS));
			if(split_drag){
				RECT rect;
				GetClientRect(hwnd,&rect);
				if(y>5 && y<rect.bottom-8){
					mdi_split=y;
					resize_mdi_window(hwnd,mdi_split);
				}
			}
		}
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
		resize_mdi_window(hwnd,mdi_split);
		save_mdi_size(hwnd);
		break;

    }
	return DefMDIChildProc(hwnd, msg, wparam, lparam);
}

int load_mdi_size(HWND hwnd)
{
	int width=0,height=0,max=0;
	get_ini_value("SETTINGS","mdi_width",&width);
	get_ini_value("SETTINGS","mdi_height",&height);
	get_ini_value("SETTINGS","mdi_height",&max);
	if(width!=0 && height!=0){
		RECT rect={0};
		GetClientRect(ghmdiclient,&rect);
		if(width>rect.right)
			width=rect.right;
		if(height>rect.bottom)
			height=rect.bottom;
		SetWindowPos(hwnd,HWND_TOP,0,0,width,height,SWP_NOMOVE|SWP_SHOWWINDOW);
		return TRUE;
	}
	return FALSE;
}
int save_mdi_size(HWND hwnd)
{
	WINDOWPLACEMENT wp;
	if(GetWindowPlacement(hwnd,&wp)!=0){
		RECT rect={0};
		char str[80]={0};
		rect=wp.rcNormalPosition;
		_snprintf(str,sizeof(str),"%i",rect.right-rect.left);
		write_ini_str("SETTINGS","mdi_width",str);
		_snprintf(str,sizeof(str),"%i",rect.bottom-rect.top);
		write_ini_str("SETTINGS","mdi_height",str);
		write_ini_str("SETTINGS","mdi_maximized",wp.flags&WPF_RESTORETOMAXIMIZED?"1":"0");
		return TRUE;
	}
	return FALSE;
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
int create_mdi_window(HWND hwnd,HINSTANCE hinstance,TABLE_WINDOW *win)
{
	HWND hedit,hlistview=0;
	if(win==0)
		return FALSE;

    hedit = CreateWindow("EDIT", 
                                     "",
                                     WS_TABSTOP|WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_AUTOVSCROLL|ES_MULTILINE|ES_WANTRETURN,
                                     0,0,
                                     0,0,
                                     hwnd,
                                     IDC_MDI_EDIT,
                                     hinstance,
                                     NULL);
    hlistview = CreateWindow(WC_LISTVIEW, 
                                     "",
                                     WS_TABSTOP|WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_SHOWSELALWAYS, //|LVS_OWNERDRAWFIXED, //|LVS_EDITLABELS,
                                     0,0,
                                     0,0,
                                     hwnd,
                                     IDC_MDI_LISTVIEW,
                                     hinstance,
                                     NULL);
	win->hlistview=hlistview;
	win->hedit=hedit;
	if(hlistview!=0)
		ListView_SetExtendedListViewStyle(hlistview,ListView_GetExtendedListViewStyle(hlistview)|LVS_EX_FULLROWSELECT);
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
    wc.lpszClassName = "tablewindow";

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
	WNDCLASS wndclass;
	HWND hswitch=0;
	memset(&wndclass,0,sizeof(wndclass));
	wndclass.lpfnWndProc=dbview_proc;
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

int create_table_window(HWND hmdiclient,TABLE_WINDOW *win)
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
	cs.szClass="tablewindow";
	cs.szTitle=title;
	cs.style=style;
	cs.hOwner=ghinstance;
	cs.lParam=win;
	handle=SendMessage(hmdiclient,WM_MDICREATE,0,&cs);
	return handle;
}

int find_win_by_hwnd(HWND hwnd,TABLE_WINDOW **win)
{
	int i;
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(table_windows[i].hwnd==hwnd){
			*win=&table_windows[i];
			return TRUE;
		}
	}
	return FALSE;
}
int free_window(TABLE_WINDOW *win)
{
	if(win!=0){
		memset(win,0,sizeof(TABLE_WINDOW));
	}
	return TRUE;
}
int acquire_table_window(TABLE_WINDOW **win)
{
	int i;
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(table_windows[i].hwnd==0){
			*win=&table_windows[i];
			return TRUE;
		}
	}
	return FALSE;
}
int find_db_tree(char *name,DB_TREE **tree)
{
	int i;
	for(i=0;i<sizeof(db_tree)/sizeof(DB_TREE);i++){
		if(db_tree[i].hroot!=0)
			if(stricmp(name,db_tree[i].name)==0){
				*tree=&db_tree[i];
				return TRUE;
			}
	}
	return FALSE;
}
int acquire_db_tree(char *name,DB_TREE **tree)
{
	int i;
	if(find_db_tree(name,tree))
		return TRUE;
	for(i=0;i<sizeof(db_tree)/sizeof(DB_TREE);i++){
		if(db_tree[i].hroot==0){
			strncpy(db_tree[i].name,name,sizeof(db_tree[i].name));
			db_tree[i].hroot=insert_root(name,IDC_DB_ITEM);
			db_tree[i].htree=ghtreeview;
			*tree=&db_tree[i];
			return TRUE;
		}
	}
	return FALSE;
}
int mdi_open_db(DB_TREE *tree)
{
	if(open_db(tree)){
		if(tree->hroot!=0){
			tree_delete_all_child(tree->hroot);
			get_tables(tree);
			expand_root(tree->hroot);
			return TRUE;
		}
	}
	return FALSE;
}
int mdi_open_table(DB_TREE *tree,TABLE_WINDOW *win)
{
}
int mdi_close_db(DB_TREE *tree)
{
	if(tree!=0 && close_db(tree)){
		TreeView_DeleteItem(ghtreeview,tree->hroot);
		memset(tree,0,sizeof(DB_TREE));
		return TRUE;
	}
	return FALSE;
}



int mdi_test_db(TABLE_WINDOW *win)
{
	//get_fields(win->DB,
}









int custom_dispatch(MSG *msg)
{
	return FALSE;
}

int create_popup_menus()
{
	create_treeview_menus();
	return 0;
}

int init_mdi_stuff()
{
	extern int show_joins,lua_script_enable;
	int list_width=60;
	memset(&table_windows,0,sizeof(table_windows));
	memset(&db_tree,0,sizeof(db_tree));
	create_popup_menus();
	return TRUE;
}

#include "DB_stuff.h"
