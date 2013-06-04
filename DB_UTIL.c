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
int load_recent(HWND hwnd,int list_ctrl)
{
	int i;
	for(i=0;i<100;i++){
	//	char str[1024]={0};

	}
}
LRESULT CALLBACK recent_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	switch(msg){
	case WM_INITDIALOG:
		load_recent(hwnd,IDC_LIST1);
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){

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
	static DWORD tick=0;
	RECT rect;
	if(FALSE)
	//if(msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_NOTIFY)
	if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_MOUSEMOVE&&msg!=WM_NCMOUSEMOVE)
	{
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
			GetClientRect(hwnd,&rect);
			get_ini_value("SETTINGS","TREE_WIDTH",&tree_width);
			if(tree_width>rect.right-10 || tree_width<10){
				tree_width=rect.right/4;
				if(tree_width<12)
					tree_width=12;
			}
		}
		break;
	case WM_USER:
		break;
	case WM_USER+1:
		break;
	case WM_USER+2:
		break;
	case WM_ACTIVATEAPP: //close any tooltip on app switch
		break;
	case WM_ACTIVATE:
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
			task_open_db( //"DSN=OFW Visual FoxPro;UID=;PWD=;SourceDB=C:\\Program Files\\Pinnacle\\Oaswin\\;SourceType=DBF;Exclusive=No;BackgroundFetch=Yes;Collate=Machine;Null=Yes;Deleted=Yes;");
			"DSN=Journal");
			//task_open_db( //"DSN=OFW Visual FoxPro;UID=;PWD=;SourceDB=C:\\Program Files\\Pinnacle\\Oaswin\\;SourceType=DBF;Exclusive=No;BackgroundFetch=Yes;Collate=Machine;Null=Yes;Deleted=Yes;");
			//"");
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
	case WM_KILLFOCUS:
		break;
	case WM_NCCALCSIZE:
		break;
	case WM_SIZE:
		resize_main_window(hwnd,tree_width);
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
	int debug=0;
#ifdef _DEBUG
	debug=1;
#endif

	ghinstance=hInstance;
	init_ini_file();
	get_ini_value("SETTINGS","DEBUG",&debug);
	if(debug!=0){
		open_console();
		move_console();
	}
	init_mdi_stuff();

	LoadLibrary("RICHED20.DLL");
	LoadLibrary("Msftedit.dll");

	ctrls.dwSize=sizeof(ctrls);
    ctrls.dwICC = ICC_LISTVIEW_CLASSES|ICC_TREEVIEW_CLASSES|ICC_BAR_CLASSES;
	InitCommonControlsEx(&ctrls);
	
	InitializeCriticalSection(&mutex);

	start_worker_thread();

	setup_mdi_classes(ghinstance);
	
	ghmenu=LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU1));
	ghmainframe=create_mainwindow(&WndProc,ghmenu,hInstance);

	ghmdiclient=create_mdiclient(ghmainframe,ghmenu,ghinstance);
	ghdbview=create_dbview(ghmainframe,ghinstance);
	ghstatusbar=CreateStatusWindow(WS_CHILD|WS_VISIBLE,"ready",ghmainframe,IDC_STATUS);

	ShowWindow(ghmainframe,nCmdShow);
	UpdateWindow(ghmainframe);
	load_window_ini(ghmainframe);
	haccel=LoadAccelerators(ghinstance,MAKEINTRESOURCE(IDR_ACCELERATOR1));

    while(GetMessage(&msg,NULL,0,0)){
		if(!custom_dispatch(&msg))
		if(!TranslateMDISysAccel(ghmdiclient, &msg) && !TranslateAccelerator(ghmainframe,haccel,&msg)){
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
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
    }
	DeleteCriticalSection(&mutex);
    return msg.wParam;
	
}



