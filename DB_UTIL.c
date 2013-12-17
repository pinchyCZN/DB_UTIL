#if _WIN32_WINNT<0x400
#define _WIN32_WINNT 0x400
#define COMPILE_MULTIMON_STUBS
#endif
#include <windows.h>
#include <Commctrl.h>
#include <multimon.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#include "resource.h"

HINSTANCE ghinstance=0;
HWND ghmainframe=0,ghmdiclient=0,ghdbview=0,ghstatusbar=0;
static HMENU ghmenu=0;
static int mousex=0,mousey=0;
static int lmb_down=FALSE;
static int main_drag=FALSE;
int tree_width=20;
CRITICAL_SECTION mutex;
LRESULT CALLBACK settings_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam);

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
			if(width<50 || height<50)
				flags|=SWP_NOSIZE;
			if(x>(rect.right-25) || x<(rect.left-25))
				x=rect.left;
			if(y<(rect.top-25) || y>(rect.bottom-25))
				y=rect.top;
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
int load_recent(HWND hwnd,int list_ctrl)
{
	int i,count=0;
	const char *section="DATABASES";
	SendDlgItemMessage(hwnd,list_ctrl,LB_RESETCONTENT,0,0); 
	for(i=0;i<20;i++){
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
		for(i=0;i<20;i++){
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

int add_connect_str(char *connect)
{
	int result=FALSE;
	const char *section="DATABASES";
	const int max_entries=20;
	const int max_len=1024;

	if(connect!=0 && connect[0]!=0){
		int i;
		char **entries;
		entries=malloc(max_entries*sizeof(char *));
		if(entries!=0){
			int index=0;
			for(i=0;i<max_entries;i++){
				entries[i]=malloc(max_len);
				if(entries[i]!=0){
					entries[i][0]=0;
					get_ini_entry(section,i,entries[i],max_len);
				}
			}
			// entries that start with > are meant to stay at the top
			for(i=0;i<max_entries;i++){
				if(entries[i]!=0){
					if(entries[i][0]=='>'){
						if(strnicmp(entries[i]+1,connect,max_len-1)==0){
							index=max_entries+1; //go ahead and bail, dont dupe this entry
							break;
						}
					}
					else{
						index=i;
						break;
					}
				}
			}
			if(index<max_entries){
				for(i=index;i<max_entries;i++){
					if(entries[i]!=0){
						if(entries[i][0]!=0){
							if(strnicmp(entries[i],connect,max_len)==0)
								entries[i][0]=0;
						}
					}
				}
				set_ini_entry(section,index,connect);
				for(i=index;i<max_entries-1;i++){
					if(entries[i]!=0){
						if(entries[i][0]!=0){
							set_ini_entry(section,index+1,entries[i]);
							index++;
						}
					}
				}
			}
		}
		if(entries!=0){
			for(i=0;i<max_entries;i++){
				if(entries[i]!=0)
					free(entries[i]);
			}
			free(entries);
		}
	}
	return result;
}


LRESULT CALLBACK recent_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND grippy=0;
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
	switch(msg){
	case WM_INITDIALOG:
		if(load_recent(hwnd,IDC_LIST1)>0){
			SendDlgItemMessage(hwnd,IDC_LIST1,LB_SETCURSEL,0,0);			
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
		case VK_INSERT:
			{
				char str[1024]={0};
				GetDlgItemText(hwnd,IDC_RECENT_EDIT,str,sizeof(str));

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
		case VK_RETURN:
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
char * strstri(char *s1,char *s2)
{
	int i,j,k;
	for(i=0;s1[i];i++)
		for(j=i,k=0;tolower(s1[j])==tolower(s2[k]);j++,k++)
			if(!s2[k+1])
				return (s1+i);
	return NULL;
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
			GetClientRect(hwnd,&rect);
			get_ini_value("SETTINGS","TREE_WIDTH",&tree_width);
			if(tree_width>rect.right-10 || tree_width<10){
				tree_width=rect.right/4;
				if(tree_width<12)
					tree_width=12;
			}
			get_ini_value("SETTINGS","KEEP_CLOSED",&keep_closed);
			get_ini_value("SETTINGS","TRIM_TRAILING",&trim_trailing);
			get_ini_value("SETTINGS","LEFT_JUSTIFY",&left_justify);
			load_icon(hwnd);
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
			if(last_focus!=0)
				SetFocus(last_focus);
		break;
	case WM_NCACTIVATE:
		if(wparam==0){
			last_focus=GetFocus();
			//printf("main saving focus %08X\n",last_focus);
		}
		else{
			PostMessage(hwnd,WM_USER+1,0,0);
			//printf("main ncactivating focus %08X\n",last_focus);
		}
		break;
	case WM_ACTIVATEAPP: //close any tooltip on app switch
		if(wparam){
			PostMessage(hwnd,WM_USER+1,0,0);
			//printf("main psoting focus %08X\n",last_focus);
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
			int y=HIWORD(lparam);
			int x=LOWORD(lparam);
			if(x>=(tree_width-10) && x<(tree_width+10))
				SetCursor(LoadCursor(NULL,IDC_SIZEWE));
			if(main_drag){
				RECT rect;
				GetClientRect(ghmainframe,&rect);
				if(x>10 && x<rect.right-10){
					tree_width=x;
					resize_main_window(hwnd,tree_width);
					printf("rect.right=%i x=%i y=%i\n",rect.right,x,y);
				}
			}
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDM_OPEN:
			if(GetKeyState(VK_SHIFT)&0x8000)
				task_open_db("");
			else
			task_open_db( //"DSN=OFW Visual FoxPro;UID=;PWD=;SourceDB=C:\\Program Files\\Pinnacle\\Oaswin\\;SourceType=DBF;Exclusive=No;BackgroundFetch=Yes;Collate=Machine;Null=Yes;Deleted=Yes;");
			"UID=dba;PWD=sql;DSN=Journal");
			//task_open_db( //"DSN=OFW Visual FoxPro;UID=;PWD=;SourceDB=C:\\Program Files\\Pinnacle\\Oaswin\\;SourceType=DBF;Exclusive=No;BackgroundFetch=Yes;Collate=Machine;Null=Yes;Deleted=Yes;");
			//"");
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
		case IDM_WINDOW_TILE:
			mdi_tile_windows_vert();
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
			save_window_size(hwnd,"MAIN_WINDOW");
		}
		return 0;
	case WM_CLOSE:
        break;
	case WM_DESTROY:
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

	ghmdiclient=create_mdiclient(ghmainframe,ghmenu,ghinstance);
	ghdbview=create_dbview(ghmainframe,ghinstance);
	ghstatusbar=CreateStatusWindow(WS_CHILD|WS_VISIBLE,"ready",ghmainframe,IDC_STATUS);
	create_status_bar_parts(ghmainframe,ghstatusbar);

	ShowWindow(ghmainframe,nCmdShow);
	UpdateWindow(ghmainframe);
	load_window_size(ghmainframe,"MAIN_WINDOW");

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



