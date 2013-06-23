#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#include "resource.h"
#include "Commctrl.h"

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
int load_window_ini(HWND hwnd)
{
	char str[20];
	RECT rect;
	int width=0,height=0,x=0,y=0;
	int result=FALSE;
	get_ini_value("SETTINGS","main_window_width",&width);
	get_ini_value("SETTINGS","main_window_height",&height);
	get_ini_value("SETTINGS","main_window_xpos",&x);
	get_ini_value("SETTINGS","main_window_ypos",&y);
	if(GetWindowRect(GetDesktopWindow(),&rect)!=0){
		int flags=SWP_SHOWWINDOW;
		if(width<50 || height<50)
			flags|=SWP_NOSIZE;
		if(x<-32 || y<=-32)
			flags|=SWP_NOMOVE;
		if(x<((rect.right-rect.left)-50))
			if(y<((rect.bottom-rect.top)-50))
				if(SetWindowPos(hwnd,HWND_TOP,x,y,width,height,flags)!=0)
					result=TRUE;
	}
	str[0]=0;
	if(get_ini_str("SETTINGS","main_window_maximized",str,sizeof(str))){
		if(str[0]=='1'){
			ShowWindow(hwnd,SW_SHOWMAXIMIZED);
			result=TRUE;
		}
	}
	return result;
}
int save_window_ini(HWND hwnd)
{
	char str[20];
	RECT rect;
	WINDOWPLACEMENT wp;
	wp.length=sizeof(wp);
	if(GetWindowPlacement(hwnd,&wp)!=0){
		write_ini_str("SETTINGS","main_window_maximized",wp.flags&WPF_RESTORETOMAXIMIZED?"1":"0");
		rect=wp.rcNormalPosition;
		str[0]=0;
		_snprintf(str,sizeof(str),"%i",rect.right-rect.left);
		write_ini_str("SETTINGS","main_window_width",str);
		_snprintf(str,sizeof(str),"%i",rect.bottom-rect.top);
		write_ini_str("SETTINGS","main_window_height",str);
		_snprintf(str,sizeof(str),"%i",rect.left);
		write_ini_str("SETTINGS","main_window_xpos",str);
		_snprintf(str,sizeof(str),"%i",rect.top);
		write_ini_str("SETTINGS","main_window_ypos",str);
		return TRUE;
	}
	return FALSE;
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
int set_status_bar_text(HWND hstatus,int part,char *fmt,...)
{
	if(hstatus!=0){
		char str[80]={0};
		va_list va;
		va_start(va,fmt);
		_vsnprintf(str,sizeof(str),fmt,va);
		va_end(va);
		return SendMessage(hstatus,SB_SETTEXT,part,str);
	}
	return FALSE;
}
int load_recent(HWND hwnd,int list_ctrl)
{
	int i,count=0;
	const char *section="DATABASES";
	SendDlgItemMessage(hwnd,list_ctrl,LB_RESETCONTENT,0,0); 
	for(i=0;i<100;i++){
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
		for(i=0;i<100;i++){
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
int remove_ini_gaps(char *section,int max)
{
	int i,j;
	char str[1024];
	for(i=0,j=1;i<max;i++){
next:
		str[0]=0;
		get_ini_entry(section,i,str,sizeof(str));
		if(str[0]==0){
			if(j<i)
				j=i+1;
			for(  ;j<max;j++){
				str[0]=0;
				get_ini_entry(section,j,str,sizeof(str));
				if(str[0]!=0){
					set_ini_entry(section,i,str);
					set_ini_entry(section,j,"");
					i++;
					j++;
					goto next;
				}
			}
			if(j>=max)
				break;
		}
	}
	return TRUE;
}
int add_connect_str(char *connect)
{
	int i,result=FALSE;
	char str[1024]={0};
	char *section="DATABASES";
	if(connect!=0 && connect[0]!=0){
		for(i=0;i<100;i++){
			str[0]=0;
			get_ini_entry(section,i,str,sizeof(str));
			if(str[0]!=0 && stricmp(connect,str)==0){
				if(i!=0){
					int j;
					set_ini_entry(section,i,"");
					for(j=i-1;j>=0;j--){
						str[0]=0;
						if(get_ini_entry(section,j,str,sizeof(str)))
							set_ini_entry(section,j+1,str);
					}
				}
				result=TRUE;
			}
		}
		if(result)
			set_ini_entry(section,0,connect);
		else{
			str[0]=0;
			get_ini_entry(section,0,str,sizeof(str));
			if(str[0]!=0){
				remove_ini_gaps(section,100);
				for(i=100-1;i>=0;i--){
					str[0]=0;
					get_ini_entry(section,i,str,sizeof(str));
					set_ini_entry(section,i+1,str);
				}
			}
			set_ini_entry(section,0,connect);
			result=TRUE;
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
						task_open_db(str);
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
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static HWND last_focus=0;
	//if(FALSE)
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
			int parts[2];
			extern int keep_closed;
			GetClientRect(hwnd,&rect);
			get_ini_value("SETTINGS","TREE_WIDTH",&tree_width);
			if(tree_width>rect.right-10 || tree_width<10){
				tree_width=rect.right/4;
				if(tree_width<12)
					tree_width=12;
			}
			get_ini_value("SETTINGS","KEEP_CLOSED",&keep_closed);
			parts[0]=100;parts[1]=-1;
		}
		break;
	case WM_USER:
		switch(wparam){
		case IDC_TREEVIEW:
			if(lparam!=0)
				SetFocus(lparam);
			break;
		}
		break;
	case WM_USER+1:
		break;
	case WM_USER+2:
			if(last_focus!=0)
				SetFocus(last_focus);
		break;
	case WM_NCACTIVATE:
		if(wparam==0){
			last_focus=GetFocus();
			printf("main saving focus %08X\n",last_focus);
		}
		else{
			PostMessage(hwnd,WM_USER+2,0,0);
			printf("main ncactivating focus %08X\n",last_focus);
		}
		break;
	case WM_ACTIVATEAPP: //close any tooltip on app switch
		if(wparam){
			PostMessage(hwnd,WM_USER+2,0,0);
			printf("main psoting focus %08X\n",last_focus);
		}
		break;
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
		ReleaseCapture();
		main_drag=FALSE;
		write_ini_value("SETTINGS","TREE_WIDTH",tree_width);
		break;
	case WM_LBUTTONDOWN:
		SetCapture(hwnd);
		SetCursor(LoadCursor(NULL,IDC_SIZEWE));
		main_drag=TRUE;
		break;
	case WM_MOUSEFIRST:
		{
			int y=HIWORD(lparam);
			int x=LOWORD(lparam);
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
		case IDM_QUERY:
			task_new_query();
			break;
		case IDC_EXECUTE_SQL:
			task_execute_query();
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
		}
		return 0;
	case WM_CLOSE:
        break;
	case WM_DESTROY:
		save_window_ini(hwnd);
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
#ifdef _DEBUG
	debug=1;
#endif
	SetLastError(NO_ERROR);
	CreateMutex(NULL,FALSE,"DB_UTIL Instance");
	if(GetLastError()==ERROR_ALREADY_EXISTS)
		first_instance=FALSE;

	ghinstance=hInstance;
	init_ini_file();
	get_ini_value("SETTINGS","DEBUG",&debug);
	if(debug!=0){
		open_console();
		move_console();
	}
	{
		int val=0;
		get_ini_value("SETTINGS","SINGLE_INSTANCE",&val);
		if(val && (!first_instance)){

		}
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
	load_window_ini(ghmainframe);
	haccel=LoadAccelerators(ghinstance,MAKEINTRESOURCE(IDR_ACCELERATOR1));

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



