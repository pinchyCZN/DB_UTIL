#if _WIN32_WINNT<0x400
#define _WIN32_WINNT 0x400
#define WINVER 0x500
#endif
#if _WIN32_IE<=0x300
#define _WIN32_IE 0x400
#endif
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
extern HWND ghmainframe,ghmdiclient,ghtreeview,ghstatusbar;

typedef struct{
	int type;
	int length;
	int col_width;
}COL_ATTR;
typedef struct{
	char name[1024];
	char table[80];
	void *hdbc;
	void *hdbenv;
	int abort;
	int columns;
	COL_ATTR *col_attr;
	int rows;
	int selected_column;
	HWND hwnd,hlistview,hlvedit,hedit,hroot,habort,hintel;
}TABLE_WINDOW;

typedef struct{
	char name[1024];
	char connect_str[1024];
	void *hdbc;
	void *hdbenv;
	HWND htree,hroot;
}DB_TREE;


static TABLE_WINDOW table_windows[5];
static DB_TREE db_tree[5];

#include "col_info.h"
#include "treeview.h"
#include "listview.h"
#include "intellisense.h"

LRESULT CALLBACK MDIChildWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static int split_drag=FALSE,mdi_split=60;
	static HWND last_focus=0;
	if(FALSE)
	if(msg!=WM_NCMOUSEMOVE&&msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_NOTIFY
		&&msg!=WM_ERASEBKGND&&msg!=WM_DRAWITEM) 
		//if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE)
	{
		static DWORD tick=0;
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

		SendDlgItemMessage(hwnd,IDC_MDI_EDIT,WM_SETFONT,GetStockObject(get_font_setting(IDC_SQL_FONT)),0);
		SendDlgItemMessage(hwnd,IDC_MDI_LISTVIEW,WM_SETFONT,GetStockObject(get_font_setting(IDC_LISTVIEW_FONT)),0);
		load_mdi_size(hwnd);
		}
        break;
	case WM_MOUSEACTIVATE:
		if(LOWORD(lparam)==HTCLIENT){
			//SetFocus(GetDlgItem(hwnd,MDI_EDIT));
			//return MA_NOACTIVATE;
		}
		break;
	case WM_CHAR:
		if(GetFocus()==GetDlgItem(hwnd,IDC_SQL_ABORT)){
			TABLE_WINDOW *win=0;
			if(find_win_by_hwnd(hwnd,&win))
				win->abort;
		}
		break;
	case WM_KILLFOCUS:
		{
		TABLE_WINDOW *win=0;
		if(find_win_by_hwnd(hwnd,&win)){
		}
		}
		break;
	case WM_SETFOCUS:
//	case WM_IME_SETCONTEXT:
//	case WM_NCACTIVATE:
//	case WM_MDIACTIVATE:
		{
		TABLE_WINDOW *win=0;
		if(find_win_by_hwnd(hwnd,&win)){

			if(lparam!=0)
				SetFocus(lparam);
			else if(win->rows>0){
				SetFocus(win->hlistview);
			}
			else{
				SetFocus(win->hedit);
				ShowCaret(win->hedit);
			}
		}
		}
        break;
	case WM_CONTEXTMENU:
		break;
	case WM_NOTIFY:
		{
			NMHDR *nmhdr=lparam;
			TABLE_WINDOW *win=0;
			if(nmhdr!=0 && nmhdr->idFrom==IDC_MDI_LISTVIEW){
				LV_HITTESTINFO lvhit={0};
				switch(nmhdr->code){
				case NM_DBLCLK:
					find_win_by_hwnd(hwnd,&win);
					if(win!=0){
						GetCursorPos(&lvhit.pt);
						ScreenToClient(nmhdr->hwndFrom,&lvhit.pt);
						if(ListView_SubItemHitTest(nmhdr->hwndFrom,&lvhit)>=0)
							create_lv_edit_selected(win);
						/*
							if(ListView_GetSubItemRect(nmhdr->hwndFrom,lvhit.iItem,lvhit.iSubItem,LVIR_BOUNDS,&rect)!=0){
								char text[255]={0};
								create_lv_edit(win,&rect);
								ListView_GetItemText(nmhdr->hwndFrom,lvhit.iItem,lvhit.iSubItem,text,sizeof(text));
								if(text[0]!=0 && win->hlvedit!=0){
									SetWindowText(win->hlvedit,text);
									SetFocus(win->hlvedit);
								}
							}
						*/
					}
					break;
				case NM_RCLICK:
				case NM_CLICK:
					GetCursorPos(&lvhit.pt);
					ScreenToClient(nmhdr->hwndFrom,&lvhit.pt);
					if(ListView_SubItemHitTest(nmhdr->hwndFrom,&lvhit)>=0){
						find_win_by_hwnd(hwnd,&win);
						if(win!=0){
							RECT rect={0};
							ListView_GetItemRect(nmhdr->hwndFrom,lvhit.iItem,&rect,LVIR_BOUNDS);
							win->selected_column=lvhit.iSubItem;
							InvalidateRect(nmhdr->hwndFrom,&rect,TRUE);
							set_status_bar_text(ghstatusbar,1,"row=%3i col=%2i",lvhit.iItem+1,lvhit.iSubItem+1);
						}
					}
					printf("item = %i\n",lvhit.iSubItem);
					break;
				case LVN_ITEMCHANGED:
					{
						find_win_by_hwnd(hwnd,&win);
						if(win!=0){
							set_status_bar_text(ghstatusbar,1,"row=%3i col=%2i",ListView_GetSelectionMark(win->hlistview)+1,win->selected_column+1);
						}
					}
					break;
				case LVN_KEYDOWN:
					{
						find_win_by_hwnd(hwnd,&win);
						if(win!=0){
							int dir=0;
							LV_KEYDOWN *lvkey=lparam;
							switch(lvkey->wVKey){
							case VK_F2:
							case VK_RETURN:
								create_lv_edit_selected(win);
								break;
							case VK_LEFT:dir=-1;break;
							case VK_RIGHT:dir=1;break;
							case VK_ESCAPE:
								destroy_lv_edit(win);
							}
							win->selected_column+=dir;
							if(win->selected_column<0)
								win->selected_column=0;
							if(win->selected_column>=win->columns)
								win->selected_column=win->columns-1;
							lv_scroll_column(win->hlistview,win->selected_column);
						}
					}
					break;
				case LVN_COLUMNCLICK:
					break;
				}
			}
		}
		break;
	case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *di=lparam;
			if(di!=0 && di->CtlType==ODT_LISTVIEW){
				TABLE_WINDOW *win=0;
				find_win_by_hwnd(hwnd,&win);
				if(win!=0){
					draw_item(di,win);
					return TRUE;
				}
			}
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
		save_mdi_size(hwnd);
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
	case WM_SYSCOMMAND:
		switch(wparam&0xFFF0){
		case SC_MAXIMIZE:
			write_ini_str("SETTINGS","mdi_maximized","1");
			break;
		case SC_RESTORE:
			write_ini_str("SETTINGS","mdi_maximized","0");
			break;
		}
		break;

	case WM_VKEYTOITEM:
		switch(LOWORD(wparam)){
		case VK_RETURN:
			{
			TABLE_WINDOW *win=0;
			if(!find_win_by_hwnd(hwnd,&win))
				break;
			SendMessage(win->hedit,WM_KEYFIRST,VK_RETURN,0);
			}
			break;
		}
		break;
    case WM_COMMAND:
		//HIWORD(wParam) notification code
		//LOWORD(wParam) item control
		//lParam handle of control
		switch(LOWORD(wparam)){
		case IDC_INTELLISENSE:
			switch(HIWORD(wparam)){
			case LBN_DBLCLK:
				{
				TABLE_WINDOW *win=0;
				if(!find_win_by_hwnd(hwnd,&win))
					break;
				SendMessage(win->hedit,WM_KEYFIRST,VK_RETURN,0);
				}
				break;
			}
			break;
		case IDC_SQL_ABORT:
			{
			TABLE_WINDOW *win=0;
			find_win_by_hwnd(hwnd,&win);
			if(win!=0)
				win->abort=TRUE;
			}
			break;
		}
		break;
	case WM_HELP:
		break;
	case WM_USER+1:
		ShowWindow(hwnd,SW_MAXIMIZE);
		break;
	case WM_USER:
		switch(LOWORD(lparam)){
		case IDC_LV_EDIT:
			switch(HIWORD(lparam)){
			case IDOK:
			default:
			case IDCANCEL:
				destroy_lv_edit(wparam);
				break;
			}
			break;
		case IDC_MDI_LISTVIEW:
			resize_mdi_window(hwnd,mdi_split);
			break;
		case IDC_MDI_EDIT:
			resize_mdi_window(hwnd,2);
			break;
		case IDC_SQL_ABORT:
			if(HIWORD(lparam))
				create_abort(wparam);
			else
				destroy_abort(wparam);
			break;
		}
		break;
	case WM_SIZE:
		resize_mdi_window(hwnd,mdi_split);
		break;

    }
	return DefMDIChildProc(hwnd, msg, wparam, lparam);
}

int load_mdi_size(HWND hwnd)
{
	int width=0,height=0,max=0;
	get_ini_value("SETTINGS","mdi_width",&width);
	get_ini_value("SETTINGS","mdi_height",&height);
	get_ini_value("SETTINGS","mdi_maximized",&max);
	if(width!=0 && height!=0){
		WINDOWPLACEMENT wp={0};
		RECT rect={0};
		wp.length=sizeof(wp);
		wp.showCmd=SW_SHOWNORMAL;
		wp.rcNormalPosition.bottom=height;
		wp.rcNormalPosition.right=width;
		GetClientRect(ghmdiclient,&rect);
		if(width>rect.right)
			width=rect.right;
		if(height>rect.bottom)
			height=rect.bottom;
		SetWindowPos(hwnd,HWND_TOP,0,0,width,height,SWP_SHOWWINDOW|SWP_NOMOVE);
		if(max)
			PostMessage(hwnd,WM_USER+1,0,0);
		return TRUE;
	}
	return FALSE;
}
int save_mdi_size(HWND hwnd)
{
	WINDOWPLACEMENT wp;
	wp.length=sizeof(wp);
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
		SetWindowPos(hcon,0,1200,0,800,600,SWP_NOZORDER);
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


int create_mdi_window(HWND hwnd,HINSTANCE hinstance,TABLE_WINDOW *win)
{
	HWND hedit,hlistview,hintel;
	if(win==0)
		return FALSE;

    hedit = CreateWindow("RichEdit50W", //"RichEdit20A",
                                     "",
                                     WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|WS_HSCROLL|WS_VSCROLL|ES_AUTOHSCROLL|ES_AUTOVSCROLL|ES_MULTILINE|ES_WANTRETURN,
                                     0,0,
                                     0,0,
                                     hwnd,
                                     IDC_MDI_EDIT,
                                     hinstance,
                                     NULL);
    hlistview = CreateWindow(WC_LISTVIEW,
                                     "",
                                     WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|LVS_REPORT|LVS_OWNERDRAWFIXED	, //|LVS_OWNERDRAWFIXED, //|LVS_SHOWSELALWAYS,
                                     0,0,
                                     0,0,
                                     hwnd,
                                     IDC_MDI_LISTVIEW,
                                     hinstance,
                                     NULL);
	hintel = CreateWindowEx(WS_EX_CLIENTEDGE,"LISTBOX",
										 "",
									 WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|LBS_HASSTRINGS|LBS_SORT|LBS_STANDARD|LBS_WANTKEYBOARDINPUT,
									 0,0,
									 0,0,
									 hwnd,
									 IDC_INTELLISENSE,
									 ghinstance,
									 NULL);
	win->hlistview=hlistview;
	win->hedit=hedit;
	win->hintel=hintel;
	if(hlistview!=0){
		ListView_SetExtendedListViewStyle(hlistview,ListView_GetExtendedListViewStyle(hlistview)|LVS_EX_FULLROWSELECT);
		subclass_listview(hlistview);
	}
	if(hedit!=0)
		subclass_edit(hedit);
	if(hintel!=0)
		ShowWindow(hintel,SW_HIDE);
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
	int style,handle;
	MDICREATESTRUCT cs;
	char title[256]={0};
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
int get_max_table_windows()
{
	return sizeof(table_windows)/sizeof(TABLE_WINDOW);
}
int get_win_hwnds(int i,HWND *hwnd,HWND *hedit,HWND *hlistview)
{
	int result=FALSE;
	if(i>sizeof(table_windows)/sizeof(TABLE_WINDOW))
		return FALSE;
	if(hwnd!=0 && table_windows[i].hwnd!=0){
		*hwnd=table_windows[i].hwnd;
		result=TRUE;
	}
	if(hedit!=0 && table_windows[i].hedit!=0){
		*hedit=table_windows[i].hedit;
		result=TRUE;
	}
	if(hlistview!=0 && table_windows[i].hlistview!=0){
		*hlistview=table_windows[i].hlistview;
		result=TRUE;
	}
	return result;
}
int find_win_by_hwnd(HWND hwnd,TABLE_WINDOW **win)
{
	int i;
	static TABLE_WINDOW *w=0;
	if(w!=0 && w->hwnd==hwnd){
		*win=w;
		return TRUE;
	}
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(table_windows[i].hwnd==hwnd){
			*win=&table_windows[i];
			w=&table_windows[i];
			return TRUE;
		}
	}
	return FALSE;
}
int free_window(TABLE_WINDOW *win)
{
	if(win!=0){
		if(win->col_attr!=0)
			free(win->col_attr);
		memset(win,0,sizeof(TABLE_WINDOW));
	}
	return TRUE;
}
int acquire_table_window(TABLE_WINDOW **win,char *tname)
{
	int i;
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(table_windows[i].hwnd==0){
			*win=&table_windows[i];
			if(tname!=0)
				strncpy(table_windows[i].table,tname,sizeof(table_windows[i].table));
			else
				table_windows[i].table[0]=0;
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
int find_selected_tree(DB_TREE **tree)
{
	int i;
	HANDLE hroot=0;
	tree_find_focused_root(&hroot);
	for(i=0;i<sizeof(db_tree)/sizeof(DB_TREE);i++){
		if(hroot!=0){
			if(db_tree[i].hroot==hroot){
				*tree=&db_tree[i];
				return TRUE;
			}
		}
		else if(db_tree[i].hroot!=0){
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
int	acquire_db_tree_from_win(TABLE_WINDOW *win,DB_TREE **tree)
{
	return acquire_db_tree(win->name,tree);
}

int mdi_open_db(DB_TREE *tree,int load_tables)
{
	int result=FALSE;
	if(open_db(tree)){
		if(load_tables){
			if(tree->hroot!=0){
				rename_tree_item(tree->hroot,tree->name);
				tree_delete_all_child(tree->hroot);
				get_tables(tree);
				expand_root(tree->hroot);
				result=TRUE;
			}
		}
		else
			result=TRUE;
	}
	return result;
}
int mdi_remove_db(DB_TREE *tree)
{
	if(tree!=0){
		if(close_db(tree)){
			if(tree->hroot!=0){
				int i;
				TreeView_DeleteItem(ghtreeview,tree->hroot);
				for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
					if(table_windows[i].hroot==tree->hroot)
						table_windows[i].hroot=0;
				}
			}
			memset(tree,0,sizeof(DB_TREE));
			return TRUE;
		}
	}
	return FALSE;
}

int mdi_clear_listview(TABLE_WINDOW *win)
{
	if(win!=0 && win->hlistview!=0){
		int i;
		ListView_DeleteAllItems(win->hlistview);
		for(i=0;i<win->columns;i++)
			ListView_DeleteColumn(win->hlistview,0);
		return TRUE;
	}
	else
		return FALSE;
}

int mdi_get_current_win(TABLE_WINDOW **win)
{
	HWND hwnd;
	hwnd=SendMessage(ghmdiclient,WM_MDIGETACTIVE,0,0);
	if(hwnd!=0){
		return find_win_by_hwnd(hwnd,win);
	}
	return FALSE;
}

int mdi_create_abort(TABLE_WINDOW *win)
{
	if(win==0 || win->hwnd==0)
		return FALSE;
	if(win->hintel!=0)
		ShowWindow(win->hintel,SW_HIDE);
	//PostMessage(ghmdiclient,WM_USER,win,MAKELPARAM(TRUE,IDC_SQL_ABORT));
	return PostMessage(win->hwnd,WM_USER,win,MAKELPARAM(IDC_SQL_ABORT,TRUE));
}
int mdi_destroy_abort(TABLE_WINDOW *win)
{
	if(win==0 || win->hwnd==0)
		return FALSE;
//	PostMessage(ghmdiclient,WM_USER,win,MAKELPARAM(IDC_SQL_ABORT,FALSE));
	return PostMessage(win->hwnd,WM_USER,win,MAKELPARAM(IDC_SQL_ABORT,FALSE));
}
int mdi_set_edit_text(TABLE_WINDOW *win,char *str)
{
	if(win!=0 && win->hedit!=0)
		SetWindowText(win->hedit,str);
	return TRUE;
}
int mdi_get_edit_text(TABLE_WINDOW *win,char *str,int size)
{
	if(win!=0 && win->hedit!=0)
		GetWindowText(win->hedit,str,size);
	return TRUE;
}
int mdi_set_title(TABLE_WINDOW *win,char *title)
{
	if(win!=0 && win->hwnd!=0)
		SetWindowText(win->hwnd,title);
	return TRUE;
}
int create_abort(TABLE_WINDOW *win)
{
	if(win==0 || win->hwnd==0 || win->habort!=0)
		return FALSE;
    win->habort = CreateWindow("BUTTON",
                                     "abort",
                                     WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|BS_TEXT,
                                     0,0,
                                     80,20,
                                     win->hwnd,
                                     IDC_SQL_ABORT,
                                     ghinstance,
                                     NULL);
	if(win->habort!=0){
		SetWindowPos(win->habort,HWND_TOP,0,0,80,20,SWP_SHOWWINDOW);
		SetWindowText(win->habort,"Abort");
		SetFocus(win->habort);
		win->abort=FALSE;
	}
	return win->habort!=0;
}

int destroy_abort(TABLE_WINDOW *win)
{
	int result=FALSE;
	if(win==0 || win->habort==0)
		return FALSE;
	win->abort=TRUE;
	result=SendMessage(win->habort,WM_CLOSE,0,0);
	win->habort=0;
	return result;
}

int set_focus_after_result(TABLE_WINDOW *win,int result)
{
	if(win!=0 && win->hedit!=0 && win->hlistview!=0){
		if(result==FALSE)
			PostMessage(win->hwnd,WM_SETFOCUS,win->hlistview,win->hedit);
		else if(win->rows>0)
			PostMessage(win->hwnd,WM_SETFOCUS,win->hedit,win->hlistview);
		else
			PostMessage(win->hwnd,WM_SETFOCUS,win->hlistview,win->hedit);
	}
	return TRUE;
}


int custom_dispatch(MSG *msg)
{
	TABLE_WINDOW *win=0;
	HWND hwnd=0;
	int i,type=0;

	hwnd=WindowFromPoint(msg->pt);

	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(table_windows[i].hlistview==msg->hwnd){
			win=&table_windows[i];
			type=IDC_MDI_LISTVIEW;
		}
		else if(table_windows[i].hedit==msg->hwnd){
			win=&table_windows[i];
			type=IDC_MDI_EDIT;
		}
		else if(table_windows[i].hwnd==msg->hwnd){
			win=&table_windows[i];
			type=IDC_MDI_CLIENT;
		}
		else if(table_windows[i].habort==msg->hwnd){
			win=&table_windows[i];
			type=IDC_SQL_ABORT;
		}
		else if(msg->message==WM_MOUSEWHEEL && hwnd!=0){
			if(table_windows[i].hlistview==hwnd){
				win=&table_windows[i];
				type=IDC_MDI_LISTVIEW;
			}
			else if(table_windows[i].hedit==hwnd){
				win=&table_windows[i];
				type=IDC_MDI_EDIT;
			}
			else if(table_windows[i].hwnd==hwnd){
				win=&table_windows[i];
				type=IDC_MDI_CLIENT;
			}
		}
	}
	if(win!=0){
		switch(msg->message){
		case WM_MOUSEWHEEL:
			switch(type){
			case IDC_MDI_LISTVIEW:
				msg->hwnd=win->hlistview;
				DispatchMessage(msg);
				return TRUE;
			case IDC_MDI_EDIT:
				msg->hwnd=win->hedit;
				DispatchMessage(msg);
				return TRUE;
			case IDC_MDI_CLIENT:
				msg->hwnd=win->hlistview;
				DispatchMessage(msg);
				return TRUE;
			}
			break;
		case WM_CHAR:
			switch(msg->wParam){
			case VK_ESCAPE:
				if(win->habort!=0 && win->hwnd!=0 && type!=0)
					mdi_destroy_abort(win);
				break;
			}
			break;
		case WM_KEYFIRST:
			switch(msg->wParam){
			case VK_TAB:
				if(type==IDC_MDI_LISTVIEW || type==IDC_MDI_CLIENT)
					SetFocus(win->hedit);
				else if(type==IDC_MDI_EDIT){
					if(GetKeyState(VK_CONTROL)&0x8000 && GetKeyState(VK_SHIFT)&0x8000){
						SetFocus(win->hlistview);
						return TRUE;
					}
				}
				else if(type!=0){
					SetFocus(win->hlistview);
					return TRUE;
				}

				break;
			case 'W':
				if(GetKeyState(VK_CONTROL)&0x8000)
					if(win->hwnd!=0)
						PostMessage(win->hwnd,WM_CLOSE,0,0);
			default:
				if(type==IDC_MDI_CLIENT){
					msg->hwnd=win->hedit;
					SetFocus(win->hedit);
				}
				break;
			}
			break;
		}
	}
	return FALSE;
}
int create_popup_menus()
{
	create_treeview_menus();
	create_lv_menus();
	return 0;
}
int init_mdi_stuff()
{
	extern int show_joins,lua_script_enable;
	memset(&table_windows,0,sizeof(table_windows));
	memset(&db_tree,0,sizeof(db_tree));
	create_popup_menus();
	return TRUE;
}

#include "DB_stuff.h"

