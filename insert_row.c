#include <windows.h>
#include <Commctrl.h>
#include "resource.h"
#include "structs.h"

extern HINSTANCE ghinstance;

int populate_insert_dlg(HWND hwnd,HWND hlistview,TABLE_WINDOW *win)
{
	int i,count,widths[4]={0,0,0,0};
	int row_sel;
	char *cols[]={"field","data","type","size"};
	if(hlistview==0 || win==0)
		return FALSE;

	for(i=0;i<4;i++)
		widths[i]=lv_add_column(hlistview,cols[i],i);

	row_sel=ListView_GetSelectionMark(win->hlistview);

	count=lv_get_column_count(win->hlistview);

	for(i=0;i<count;i++){
		int w;
		char str[80]={0};
		lv_get_col_text(win->hlistview,i,str,sizeof(str));
		lv_insert_data(hlistview,i,0,str);
		w=get_str_width(hlistview,str)+8;
		if(w>widths[0])
			widths[0]=w;
		if(row_sel>=0){
			str[0]=0;
			ListView_GetItemText(win->hlistview,row_sel,i,str,sizeof(str));
			lv_insert_data(hlistview,i,1,str);
			w=get_str_width(hlistview,str)+8;
			if(w>widths[1])
				widths[1]=w;
		}
		if(win->col_attr!=0){
			char *s="";
			if(!find_sql_type_str(win->col_attr[i].type,&s)){
				_snprintf(str,sizeof(str),"%i",win->col_attr[i].type);
				lv_insert_data(hlistview,i,2,str);
			}
			else
				lv_insert_data(hlistview,i,2,s);
			w=get_str_width(hlistview,s)+8;
			if(w>widths[2])
				widths[2]=w;

			_snprintf(str,sizeof(str),"%i",win->col_attr[i].length);
			lv_insert_data(hlistview,i,3,str);
			w=get_str_width(hlistview,str)+8;
			if(w>widths[2])
				widths[2]=w;
		}
	}
	{
		int total_width=0;
		for(i=0;i<4;i++){
			ListView_SetColumnWidth(hlistview,i,widths[i]);
			total_width+=widths[i];
		}
		if(total_width>0){
			int width,height;
			RECT rect={0};
			ListView_GetItemRect(hlistview,0,&rect,LVIR_BOUNDS);
			height=80+(count*(rect.bottom-rect.top+2));
			width=total_width+20;
			SetWindowPos(hwnd,NULL,0,0,width,height,SWP_NOMOVE|SWP_NOZORDER);
		}
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
		populate_insert_dlg(hwnd,hlistview,win);
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