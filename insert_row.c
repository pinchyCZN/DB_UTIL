#include <windows.h>
#include <Commctrl.h>
#include "resource.h"
#include "structs.h"

extern HINSTANCE ghinstance;

int populate_insert_dlg(HWND hlistview,TABLE_WINDOW *win)
{
	int i,cols;
	if(hlistview==0 || win==0)
		return FALSE;

	lv_add_column(hlistview,"field",0);
	lv_add_column(hlistview,"data",1);
	lv_add_column(hlistview,"type",2);
	lv_add_column(hlistview,"size",3);
	cols=lv_get_column_count(win->hlistview);
	for(i=0;i<cols;i++){
		char str[80]={0};
		lv_get_col_text(win->hlistview,i,str,sizeof(str));
		lv_insert_data(hlistview,0,0,str);
	}
	return TRUE;
}
LRESULT CALLBACK insert_dlg_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static TABLE_WINDOW *win=0;
	static HWND hlistview,hgrippy;

	switch(msg){
	case WM_INITDIALOG:
		if(lparam==0){
			EndDialog(hwnd,0);
			break;
		}
		win=lparam;
		hlistview=CreateWindow(WC_LISTVIEW,"",WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS,
                                     0,0,
                                     0,0,
                                     hwnd,
                                     IDC_LIST1,
                                     ghinstance,
                                     NULL);
		ListView_SetExtendedListViewStyle(hlistview,LVS_EX_FULLROWSELECT);
		populate_insert_dlg(hlistview,win);
		hgrippy=create_grippy(hwnd);
		resize_insert_dlg(hwnd);
		break;
	case WM_SIZE:
		resize_insert_dlg(hwnd);
		grippy_move(hwnd,hgrippy);
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


int do_insert_dlg(HWND hwnd,void *win)
{
	return DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_INSERT),hwnd,insert_dlg_proc,win);
}