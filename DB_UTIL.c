#define WINVER 0x500
#define _WIN32_WINNT 0x500
#include <windows.h>
#include <Commctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#include "resource.h"
#include "structs.h"

HINSTANCE ghinstance=0;
HWND ghmainframe=0,ghmdiclient=0,ghdbview=0,ghstatusbar=0;
static HMENU ghmenu=0;
static int mousex=0,mousey=0;
static int lmb_down=FALSE;
static int main_drag=FALSE;
int tree_width=0;
CRITICAL_SECTION mutex;
LRESULT CALLBACK settings_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam);

int get_clipboard(char *str,int len)
{
	int result=0;
	if(str==0 || len<=0)
		return result;
	if(OpenClipboard(NULL)!=0){
		HANDLE h;
		h=GetClipboardData(CF_TEXT);
		if(h){
			char *lock=GlobalLock(h);
			if(lock){
				strncpy(str,lock,len);
				str[len-1]=0;
				result=strlen(str);
				GlobalUnlock(h);
			}
		}
		CloseClipboard();
	}
	return result;
}
int copy_str_clipboard(char *str)
{
	int len,result=FALSE;
	HGLOBAL hmem;
	char *lock;
	len=strlen(str);
	if(len==0)
		return result;
	len++;
	hmem=GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,len);
	if(hmem!=0){
		lock=GlobalLock(hmem);
		if(lock!=0){
			memcpy(lock,str,len);
			GlobalUnlock(hmem);
			if(OpenClipboard(NULL)!=0){
				EmptyClipboard();
				SetClipboardData(CF_TEXT,hmem);
				CloseClipboard();
				result=TRUE;
			}
		}
		if(!result)
			GlobalFree(hmem);
	}
	return result;
}
int load_icon(HWND hwnd)
{
	HICON hIcon = LoadIcon(ghinstance,MAKEINTRESOURCE(IDI_ICON));
    if(hIcon){
		SendMessage(hwnd,WM_SETICON,ICON_SMALL,(LPARAM)hIcon);
		SendMessage(hwnd,WM_SETICON,ICON_BIG,(LPARAM)hIcon);
		return TRUE;
	}
	return FALSE;
}
int get_nearest_monitor(int x,int y,int width,int height,RECT *rect)
{
	HMONITOR hmon;
	MONITORINFO mi;
	RECT r={0};
	r.left=x;
	r.top=y;
	r.right=x+width;
	r.bottom=y+height;
	hmon=MonitorFromRect(&r,MONITOR_DEFAULTTONEAREST);
    mi.cbSize=sizeof(mi);
	if(GetMonitorInfo(hmon,&mi)){
		*rect=mi.rcWork;
		return TRUE;
	}
	return FALSE;
}
int clamp_window_size(int *x,int *y,int *width,int *height,RECT *monitor)
{
	int mwidth,mheight;
	mwidth=monitor->right-monitor->left;
	mheight=monitor->bottom-monitor->top;
	if(mwidth<=0)
		return FALSE;
	if(mheight<=0)
		return FALSE;
	if(*x<monitor->left)
		*x=monitor->left;
	if(*width>mwidth)
		*width=mwidth;
	if(*x+*width>monitor->right)
		*x=monitor->right-*width;
	if(*y<monitor->top)
		*y=monitor->top;
	if(*height>mheight)
		*height=mheight;
	if(*y+*height>monitor->bottom)
		*y=monitor->bottom-*height;
	return TRUE;
}
int load_window_size(HWND hwnd,char *section)
{
	RECT rect={0};
	int width=0,height=0,x=0,y=0,maximized=0;
	int result=FALSE;
	get_ini_value(section,"width",&width);
	get_ini_value(section,"height",&height);
	get_ini_value(section,"xpos",&x);
	get_ini_value(section,"ypos",&y);
	get_ini_value(section,"maximized",&maximized);
	if(get_nearest_monitor(x,y,width,height,&rect)){
		int flags=SWP_SHOWWINDOW;
		if((GetKeyState(VK_SHIFT)&0x8000)==0){
			if(width<50 || height<50){
				RECT tmp={0};
				GetWindowRect(hwnd,&tmp);
				width=tmp.right-tmp.left;
				height=tmp.bottom-tmp.top;
				flags|=SWP_NOSIZE;
			}
			if(!clamp_window_size(&x,&y,&width,&height,&rect))
				flags|=SWP_NOMOVE;
			if(SetWindowPos(hwnd,HWND_TOP,x,y,width,height,flags)!=0)
				result=TRUE;
		}
	}
	if(maximized)
		PostMessage(hwnd,WM_SYSCOMMAND,SC_MAXIMIZE,0);
	return result;
}
int save_window_size(HWND hwnd,char *section)
{
	WINDOWPLACEMENT wp;
	RECT rect={0};
	int x,y;

	wp.length=sizeof(wp);
	if(GetWindowPlacement(hwnd,&wp)!=0){
		if(wp.flags&WPF_RESTORETOMAXIMIZED)
			write_ini_value(section,"maximized",1);
		else
			write_ini_value(section,"maximized",0);

		rect=wp.rcNormalPosition;
		x=rect.right-rect.left;
		y=rect.bottom-rect.top;
		write_ini_value(section,"width",x);
		write_ini_value(section,"height",y);
		write_ini_value(section,"xpos",rect.left);
		write_ini_value(section,"ypos",rect.top);
	}
	return TRUE;
}

int create_status_bar_parts(HWND hwnd,HWND hstatus)
{
	if(hwnd!=0 && hstatus!=0){
		int parts[2]={-1,-1};
		RECT rect={0};
		GetClientRect(hwnd,&rect);
		parts[0]=rect.right/2;
		return SendMessage(hstatus,SB_SETPARTS,2,&parts);
	}
	return FALSE;
}
int set_window_text(HWND hwnd,char *fmt,...)
{
	if(hwnd!=0){
		char str[256]={0};
		va_list va;
		va_start(va,fmt);
		_vsnprintf(str,sizeof(str),fmt,va);
		va_end(va);
		str[sizeof(str)-1]=0;
		return SetWindowText(hwnd,str);
	}
	return FALSE;
}
int set_status_bar_text(HWND hstatus,int part,char *fmt,...)
{
	if(hstatus!=0){
		char str[100]={0};
		va_list va;
		va_start(va,fmt);
		_vsnprintf(str,sizeof(str),fmt,va);
		va_end(va);
		str[sizeof(str)-1]=0;
		return SendMessage(hstatus,SB_SETTEXT,part,str);
	}
	return FALSE;
}
#define MAX_RECENT_ENTRIES 20
int load_recent(HWND hwnd,int list_ctrl)
{
	int i,count=0;
	const char *section="DATABASES";
	SendDlgItemMessage(hwnd,list_ctrl,LB_RESETCONTENT,0,0); 
	for(i=0;i<MAX_RECENT_ENTRIES;i++){
		char str[1024]={0};
		get_ini_entry(section,i,str,sizeof(str));
		if(str[0]!=0){
			str[sizeof(str)-1]=0;
			SendDlgItemMessage(hwnd,list_ctrl,LB_ADDSTRING,0,str);
			count++;
		}
	}
	return count;
}
int delete_connect_str(char *connect)
{
	int i,result=FALSE;
	const char *section="DATABASES";
	if(connect!=0 && connect[0]!=0){
		for(i=0;i<MAX_RECENT_ENTRIES;i++){
			char str[1024]={0};
			get_ini_entry(section,i,str,sizeof(str));
			if(str[0]!=0 && stricmp(connect,str)==0){
				set_ini_entry(section,i,"");
				result=TRUE;
				break;
			}
		}
	}
	return result;
}
int get_ini_entry(char *section,int num,char *str,int len)
{
	char key[20];
	_snprintf(key,sizeof(key),"ENTRY%02i",num);
	return get_ini_str(section,key,str,len);
}
int set_ini_entry(char *section,int num,char *str)
{
	char key[20];
	_snprintf(key,sizeof(key),"ENTRY%02i",num);
	if(str==NULL)
		return delete_ini_key(section,key);
	return write_ini_str(section,key,str);
}
int save_entries(HWND hlbox)
{
	int i,index,count;
	const char *section="DATABASES";
	count=SendMessage(hlbox,LB_GETCOUNT,0,0);
	index=0;
	for(i=0;i<count;i++){
		char str[1024];
		str[0]=0;
		SendMessage(hlbox,LB_GETTEXT,i,str);
		if(str[0]!=0){
			set_ini_entry(section,index,str);
			index++;
		}
	}
	for(i=index;i<MAX_RECENT_ENTRIES;i++){
		set_ini_entry(section,i,"");
	}
	return 0;
}
int add_connect_str(char *connect)
{
	int result=FALSE;
	const char *section="DATABASES";
	const int max_entries=MAX_RECENT_ENTRIES;
	const int max_len=1024;

	if(connect!=0 && connect[0]!=0){
		int i;
		char *entries;
		entries=calloc(max_entries,max_len);
		if(entries!=0){
			int have=FALSE;
			char *ptr;
			for(i=0;i<max_entries;i++){
				char *str=entries+i*max_len;
				get_ini_entry(section,i,str,max_len);
				str[max_len-1]=0;
			}
			//dont add if exists
			ptr=connect;
			if(ptr[0]=='>')
				ptr++;
			for(i=0;i<max_entries;i++){
				char *src,a;
				src=entries+i*max_len;
				a=src[0];
				if(a=='>')
					src++;
				if(stricmp(src,ptr)==0){
					have=TRUE;
					break;
				}
			}
			if(!have){
				int added=FALSE;
				for(i=0;i<max_entries;i++){
					char *str=entries+i*max_len;
					char a,b;
					int do_insert=FALSE;
					a=str[0];
					b=connect[0];
					if(b=='>' && a!='>')
						do_insert=TRUE;
					else if(b!='>' && a!='>')
						do_insert=TRUE;
					if(do_insert){
						char *src;
						char *dst;
						int len,tsize;
						src=str;
						dst=str+max_len;
						tsize=max_entries*max_len;
						len=tsize-(i+1)*max_len;
						if(len>0){
							memmove(dst,src,len);
							strncpy(str,connect,max_len);
							str[max_len-1]=0;
							added=TRUE;
							break;
						}
					}
				}
				if(!added){
					char *dst=entries+(max_entries-1)*max_len;
					strncpy(dst,connect,max_len);
					dst[max_len-1]=0;
					result=TRUE;
				}
				{
					int index=0;
					for(i=0;i<max_entries;i++){
						char *str=entries+i*max_len;
						if(str[0]!=0){
							set_ini_entry(section,index,str);
							index++;
						}
					}
					for(i=index;i<max_entries;i++)
						set_ini_entry(section,i,"");
				}

			}
		}
		if(entries!=0)
			free(entries);
	}
	return result;
}

int move_item(HWND hlbox,int item,int dir)
{
	int result=FALSE;
	int count;
	char tmp[512];
	if(item==0 && dir<0)
		return result;
	count=SendMessage(hlbox,LB_GETCOUNT,0,0);
	if(count>=0){
		if(dir>0 && item>=(count-1))
			return result;
	}
	else
		return result;
	count=SendMessage(hlbox,LB_GETTEXTLEN,item,0);
	if((count+1)>=sizeof(tmp))
		return result;
	SendMessage(hlbox,LB_GETTEXT,item,tmp);
	SendMessage(hlbox,LB_DELETESTRING,item,0);
	item+=dir;
	count=SendMessage(hlbox,LB_INSERTSTRING,item,tmp);
	if(0<=count){
		SendMessage(hlbox,LB_SETCURSEL,count,0);
		result=TRUE;
	}
	return result;
}

LRESULT CALLBACK recent_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND grippy=0;
	switch(msg){
	case WM_INITDIALOG:
		if(load_recent(hwnd,IDC_LIST1)>0){
			SendDlgItemMessage(hwnd,IDC_LIST1,LB_SETCURSEL,0,0);
			SetFocus(GetDlgItem(hwnd,IDC_LIST1));
		}
		else
			SetFocus(GetDlgItem(hwnd,IDC_RECENT_EDIT));
		SendDlgItemMessage(hwnd,IDC_RECENT_EDIT,EM_LIMITTEXT,1024,0);
		grippy=create_grippy(hwnd);
		resize_recent_window(hwnd);
		{
			RECT crect={0},wrect={0};
			GetClientRect(hwnd,&crect);
			GetClientRect(ghmainframe,&wrect);
			if(crect.right<wrect.right)
				SetWindowPos(hwnd,NULL,0,0,wrect.right,crect.bottom,SWP_NOMOVE|SWP_NOZORDER);
		}
		break;
	case WM_VKEYTOITEM:
		switch(LOWORD(wparam)){
		case VK_UP:
		case VK_DOWN:
			if(GetKeyState(VK_CONTROL)&0x8000){
				int key=LOWORD(wparam);
				if(move_item(lparam,HIWORD(wparam),key==VK_UP?-1:1))
					save_entries(lparam);
				return -2;
			}
			break;
		case VK_DELETE:
			{
				int item;
				item=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETCURSEL,0,0);
				if(item>=0){
					char str[1024]={0};
					SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETTEXT,item,str);
					str[sizeof(str)-1]=0;
					if(delete_connect_str(str))
						load_recent(hwnd,IDC_LIST1);
					item--;
					if(item<0)
						item=0;
					SendDlgItemMessage(hwnd,IDC_LIST1,LB_SETCURSEL,item,0);
				}
			}
			break;
		}
		return -1;
		break;
	case WM_SIZE:
		grippy_move(hwnd,grippy);
		resize_recent_window(hwnd);
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDC_ADD:
			{
				char str[1024]={0};
				GetDlgItemText(hwnd,IDC_RECENT_EDIT,str,sizeof(str));
				if(str[0]!=0){
					add_connect_str(str);
					load_recent(hwnd,IDC_LIST1);
				}
			}
			break;
		case IDC_DELETE:
			SendMessage(hwnd,WM_VKEYTOITEM,VK_DELETE,0);
			break;
		case IDC_LIST1:
			if(HIWORD(wparam)==LBN_SELCHANGE){
				char str[1024]={0};
				int item;
				item=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETCURSEL,0,0);
				if(item>=0){
					SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETTEXT,item,str);
					SetDlgItemText(hwnd,IDC_RECENT_EDIT,str);
				}
				break;
			}
			if(HIWORD(wparam)!=LBN_DBLCLK)
				break;
		case IDOK:
			{
				char str[1024]={0};
				int item;
				HWND focus=GetFocus();
				if(GetDlgItem(hwnd,IDC_RECENT_EDIT)==focus){
					SendMessage(hwnd,WM_COMMAND,IDC_ADD,0);
					break;
				}
				if(GetDlgItem(hwnd,IDC_LIST1)!=focus)
					break;
				item=SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETCURSEL,0,0);
				if(item>=0){
					SendDlgItemMessage(hwnd,IDC_LIST1,LB_GETTEXT,item,str);
					if(str[0]!=0){
						char *s=str;
						if(str[0]=='>')
							s++;
						task_open_db(s);
						EndDialog(hwnd,0);
					}

				}
			}
			break;
		case IDCANCEL:
			EndDialog(hwnd,0);
			break;
		}
		break;
	}
	return 0;
}
const char * strstri(const char *s1,const char *s2)
{
	int i,j,k;
	for(i=0;s1[i];i++)
		for(j=i,k=0;tolower(s1[j])==tolower(s2[k]);j++,k++)
			if(!s2[k+1])
				return (s1+i);
	return NULL;
}
int debug_window_focus(HWND hwnd,char *fmt,...)
{
#ifdef _DEBUG
	va_list list;
	char str[80]={0};
	char other[80]={0};
	va_start(list,fmt);
	_vsnprintf(other,sizeof(other),fmt,list);
	other[sizeof(other)-1]=0;
	GetClassName(hwnd,str,sizeof(str));
	printf("hwnd val=%08X name=%s info=%s\n",hwnd,str,other);
#endif
	return 0;
}

int stop_thread_menu(int create)
{
	if(create){
		DeleteMenu(ghmenu,IDM_STOP_THREAD,MF_BYCOMMAND);
		InsertMenu(ghmenu,IDM_STOP_THREAD,MF_BYCOMMAND|MF_STRING,IDM_STOP_THREAD,"Cancel thread");
	}
	else{
		DeleteMenu(ghmenu,IDM_STOP_THREAD,MF_BYCOMMAND);
	}
	DrawMenuBar(ghmainframe);
	return 0;
}

LRESULT CALLBACK textentry_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static TEXT_ENTRY *params=0;
	switch(msg){
	case WM_INITDIALOG:
		{
			params=lparam;
			if(params==0 || params->len<=0 || params->data==0)
				EndDialog(hwnd,FALSE);
			else{
				if(params->data[0]!=0)
					SendDlgItemMessage(hwnd,IDC_EDIT1,WM_SETTEXT,0,params->data);
				if(params->title!=0)
					SetWindowText(hwnd,params->title);
			}
			SendDlgItemMessage(hwnd,IDC_EDIT1,EM_SETLIMITTEXT,params->len-1,0);
			SetFocus(GetDlgItem(hwnd,IDC_EDIT1));
			SendDlgItemMessage(hwnd,IDC_EDIT1,EM_SETSEL,params->len,-1);
			{
				RECT rect={0};
				int x,y;
				GetWindowRect(ghmdiclient,&rect);
				x=(rect.left+rect.right)/2;
				y=(rect.top+rect.bottom)/2;
				GetWindowRect(hwnd,&rect);
				x-=(rect.right-rect.left)/2;
				y-=(rect.bottom-rect.top)/2;
				SetWindowPos(hwnd,NULL,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER);
			}
		}
		break;
	case WM_SIZE:
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
			{
				SendDlgItemMessage(hwnd,IDC_EDIT1,WM_GETTEXT,params->len,params->data);
				EndDialog(hwnd,TRUE);
			}
			break;
		case IDCANCEL:
			EndDialog(hwnd,FALSE);
			break;
		}
		break;
	}
	return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static HWND last_focus=0;
	if(FALSE)
	//if(msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_NOTIFY)
	if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_MOUSEMOVE&&msg!=WM_NCMOUSEMOVE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
    switch(msg)
    {
	case WM_MENUSELECT:
		break;
	case WM_CREATE:
		{
			RECT rect={0};
			extern int keep_closed,trim_trailing,left_justify;
			get_ini_value("SETTINGS","KEEP_CLOSED",&keep_closed);
			get_ini_value("SETTINGS","TRIM_TRAILING",&trim_trailing);
			get_ini_value("SETTINGS","LEFT_JUSTIFY",&left_justify);
			load_icon(hwnd);

			load_window_size(hwnd,"MAIN_WINDOW");

			GetClientRect(hwnd,&rect);
			get_ini_value("SETTINGS","TREE_WIDTH",&tree_width);
			if(tree_width>rect.right-10 || tree_width<10){
				tree_width=rect.right/4;
				if(tree_width<12)
					tree_width=12;
			}

			ghmdiclient=create_mdiclient(hwnd,ghmenu,ghinstance);
			ghdbview=create_dbview(hwnd,ghinstance);
			ghstatusbar=CreateStatusWindow(WS_CHILD|WS_VISIBLE,"ready",hwnd,IDC_STATUS);
			create_status_bar_parts(hwnd,ghstatusbar);

			resize_main_window(hwnd,tree_width);
		}
		break;
	case WM_DROPFILES:
		process_drop(hwnd,wparam);
		break;
	case WM_COPYDATA:
		if(lparam!=0){
			COPYDATASTRUCT *cd=lparam;
			process_cmd_line(cd->lpData);
		}
		break;
	case WM_USER:
		debug_window_focus(lparam,"WM_USER");
		switch(wparam){
		case IDC_TREEVIEW:
			if(lparam!=0)
				SetFocus(lparam);
			break;
		case IDC_MDI_LISTVIEW:
			if(lparam!=0){
				last_focus=lparam;
				SetFocus(lparam);
			}
			break;
		case IDC_LV_EDIT:
			if(lparam!=0)
				last_focus=lparam;
			break;
		}
		break;
	case WM_USER+1:
		debug_window_focus(last_focus,"WMUSER+1");
		if(last_focus!=0)
			SetFocus(last_focus);
		break;
	case WM_NCACTIVATE:
		debug_window_focus(last_focus,"NCACTIVATE wparam=%08X",wparam);
		if(wparam==0){
			last_focus=GetFocus();
		}
		else{
			PostMessage(hwnd,WM_USER+1,0,0);
		}
		break;
	case WM_ACTIVATEAPP: //close any tooltip on app switch
		debug_window_focus(last_focus,"ACTIVATEAPP wparam=%08X",wparam);
		if(wparam){
			PostMessage(hwnd,WM_USER+1,0,0);
		}
		break;
	case WM_KILLFOCUS:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
		if(main_drag){
			ReleaseCapture();
			main_drag=FALSE;
			write_ini_value("SETTINGS","TREE_WIDTH",tree_width);
		}
		break;
	case WM_LBUTTONDOWN:
		{
			int x=LOWORD(lparam);
			if(x>=(tree_width-10) && x<(tree_width+10)){
				SetCapture(hwnd);
				SetCursor(LoadCursor(NULL,IDC_SIZEWE));
				main_drag=TRUE;
			}
		}
		break;
	case WM_MOUSEFIRST:
		{
			int x=LOWORD(lparam);
			if(x>=(tree_width-10) && x<(tree_width+10))
				SetCursor(LoadCursor(NULL,IDC_SIZEWE));
			if(main_drag){
				RECT rect;
				GetClientRect(ghmainframe,&rect);
				if(x>10 && x<rect.right-10){
					tree_width=x;
					resize_main_window(hwnd,tree_width);
				}
			}
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDM_OPEN:
			if((GetKeyState(VK_SHIFT)&0x8000) || GetKeyState(VK_CONTROL)&0x8000)
				task_open_db("");
			else{
				static TEXT_ENTRY param={0};
				static char connect_str[1024]={0};
				param.title="Enter connection string";
				param.data=connect_str;
				param.len=sizeof(connect_str);
				if(DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_TEXTENTRY),hwnd,textentry_proc,&param)==TRUE){
					task_open_db(connect_str);
				}
			}
			break;
		case IDM_CLOSE:
			{
			HANDLE hroot=0;
			if(tree_find_focused_root(&hroot)){
				char str[MAX_PATH]={0};
				tree_get_db_table(hroot,str,sizeof(str),0,0,0);
				if(str[0]!=0){
					set_status_bar_text(ghstatusbar,0,"closing %s",str);
					task_close_db(str);
				}
			}
			else
				set_status_bar_text(ghstatusbar,0,"select a DB");
			}
			break;
		case IDM_SETTINGS:
			DialogBox(ghinstance,MAKEINTRESOURCE(IDD_SETTINGS),hwnd,settings_proc);
			break;
		case IDM_RECENT:
			DialogBox(ghinstance,MAKEINTRESOURCE(IDD_RECENT),hwnd,recent_proc);
			break;
		case IDM_STOP_THREAD:
			{
				int click=MessageBox(hwnd,"Are you sure you want to terminate the task?","Warning",MB_OKCANCEL|MB_SYSTEMMODAL);
				if(click==IDOK){
					terminate_worker_thread();
					stop_thread_menu(FALSE);
				}
			}
			break;
		case IDM_QUERY:
			task_new_query();
			break;
		case IDC_EXECUTE_SQL:
			task_execute_query(NULL);
			break;
		case IDM_TILE_DIALOG:
			tile_select_dialog(hwnd);
			break;
		case IDM_WINDOW_TILE:
			mdi_tile_windows_vert();
			break;
		case IDM_WINDOW_CASCADE:
			mdi_cascade_win_vert();
			break;
		case IDM_WINDOW_LRTILE:
			mdi_tile_windows_horo();
			break;
		case IDM_REFRESH_ALL:
			refresh_all_dialog(hwnd);
			break;
		case IDM_REORDER:
			reorder_win_dialog(hwnd);
			break;
		}
		break;
	case WM_SIZE:
		resize_main_window(hwnd,tree_width);
		create_status_bar_parts(ghmainframe,ghstatusbar);
		return 0;
		break;
	case WM_QUERYENDSESSION:
		return 1; //ok to end session
		break;
	case WM_ENDSESSION:
		if(wparam){
			if(!(GetKeyState(VK_SHIFT)&0x8000))
				save_window_size(hwnd,"MAIN_WINDOW");
		}
		return 0;
	case WM_CLOSE:
        break;
	case WM_DESTROY:
		if(!(GetKeyState(VK_SHIFT)&0x8000))
			save_window_size(hwnd,"MAIN_WINDOW");
		PostQuitMessage(0);
        break;
    }
	return DefFrameProc(hwnd, ghmdiclient, msg, wparam, lparam);
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
    INITCOMMONCONTROLSEX ctrls;
	HACCEL haccel;
	static char *class_name="DB_UTIL_CLASS";
	int first_instance=TRUE;
	int debug=0;

	first_instance=set_single_instance(TRUE);

	ghinstance=hInstance;
	init_ini_file();

#ifdef _DEBUG
	debug=1;
#else
	get_ini_value("SETTINGS","DEBUG",&debug);
#endif
	if(debug!=0){
		open_console();
	}

	{
		int val=0;
		get_ini_value("SETTINGS","SINGLE_INSTANCE",&val);
		if(val && (!first_instance)){
			COPYDATASTRUCT cd={0};
			HWND hdbutil;
			cd.cbData=nCmdShow;
			cd.cbData=strlen(lpCmdLine)+1;
			cd.lpData=lpCmdLine;
			hdbutil=FindWindow("DB_UTIL_CLASS",NULL);
			if(hdbutil!=0){
				int sw;
				SendMessage(hdbutil,WM_COPYDATA,hInstance,&cd);
				if (IsZoomed(hdbutil))
					sw=SW_MAXIMIZE;
				else if(IsIconic(hdbutil))
					sw=SW_RESTORE;
				else
					sw=SW_SHOW;
				ShowWindow(hdbutil,sw);
				SetForegroundWindow(hdbutil);
			}
			return TRUE;
		}
		set_single_instance(val);
	}
	init_mdi_stuff();

	LoadLibrary("RICHED20.DLL");
	LoadLibrary("Msftedit.dll");

	ctrls.dwSize=sizeof(ctrls);
    ctrls.dwICC = ICC_LISTVIEW_CLASSES|ICC_TREEVIEW_CLASSES|ICC_BAR_CLASSES;
	InitCommonControlsEx(&ctrls);
	
	InitializeCriticalSection(&mutex);

	start_worker_thread();
	start_intellisense_thread();

	setup_mdi_classes(ghinstance);
	
	ghmenu=LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU1));
	ghmainframe=create_mainwindow(&WndProc,ghmenu,hInstance,class_name,"DB_UTIL");

	ShowWindow(ghmainframe,nCmdShow);
	UpdateWindow(ghmainframe);

	haccel=LoadAccelerators(ghinstance,MAKEINTRESOURCE(IDR_ACCELERATOR1));

	process_cmd_line(lpCmdLine);

    while(GetMessage(&msg,NULL,0,0)){
		if(!custom_dispatch(&msg))
		if(!TranslateMDISysAccel(ghmdiclient, &msg) && !TranslateAccelerator(ghmainframe,haccel,&msg)){
			TranslateMessage(&msg);
			//if(msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_NOTIFY)
			if(FALSE)
			if(msg.message!=0x118&&msg.message!=WM_NCHITTEST&&msg.message!=WM_SETCURSOR&&msg.message!=WM_ENTERIDLE&&msg.message!=WM_NCMOUSEMOVE&&msg.message!=WM_MOUSEFIRST)
			{
				static DWORD tick=0;
				if((GetTickCount()-tick)>500)
					printf("--\n");
				printf("x");
				print_msg(msg.message,msg.lParam,msg.wParam,msg.hwnd);
				tick=GetTickCount();
			}
			DispatchMessage(&msg);
		}
    }
	DeleteCriticalSection(&mutex);
    return msg.wParam;
	
}



