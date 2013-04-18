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
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static DWORD tick=0;
	RECT rect;
	//if(msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_NOTIFY)
	if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE)
	{
		if((GetTickCount()-tick)>500)
			printf("--\n");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
    switch(msg)
    {
	case WM_MENUSELECT:
		switch(LOWORD(wparam)){
		case IDM_OPEN:
			task_open_db("C:\\Journal Manager\\journal.db");
			break;
		}
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
		{
			RECT rect={0};
			WINDOWPLACEMENT wp;
			int w=0,h=0,maximized=0;
			wp.length=sizeof(wp);
			if(GetWindowPlacement(hwnd,&wp)!=0){
				rect=wp.rcNormalPosition;
				if(wp.flags&WPF_RESTORETOMAXIMIZED)
					maximized=1;
				w=rect.right-rect.left;
				h=rect.bottom-rect.top;
			}
		}
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

	ctrls.dwSize=sizeof(ctrls);
    ctrls.dwICC = ICC_LISTVIEW_CLASSES|ICC_TREEVIEW_CLASSES;
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


    while(GetMessage(&msg,NULL,0,0)){
		static DWORD tick=0;
		//if(msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_NOTIFY)
		if(FALSE)
		if(msg.message!=0x118&&msg.message!=WM_NCHITTEST&&msg.message!=WM_SETCURSOR&&msg.message!=WM_ENTERIDLE)
		{
			if((GetTickCount()-tick)>500)
				printf("--\n");
			printf("x");
			print_msg(msg.message,msg.lParam,msg.wParam,msg.hwnd);
			tick=GetTickCount();
		}
		if(!TranslateMDISysAccel(ghmdiclient, &msg)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
    }
	DeleteCriticalSection(&mutex);
    return msg.wParam;
	
}



