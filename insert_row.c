#include <windows.h>
#include <Commctrl.h>
#include "resource.h"
#include "structs.h"

extern HINSTANCE ghinstance;

enum {FIELD_POS=0,DATA_POS,TYPE_POS,SIZE_POS};

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
		lv_insert_data(hlistview,i,FIELD_POS,str);
		w=get_str_width(hlistview,str)+8;
		if(w>widths[FIELD_POS])
			widths[FIELD_POS]=w;
		if(row_sel>=0){
			str[0]=0;
			ListView_GetItemText(win->hlistview,row_sel,i,str,sizeof(str));
			lv_insert_data(hlistview,i,DATA_POS,str);
			w=get_str_width(hlistview,str)+8;
			if(w>widths[DATA_POS])
				widths[DATA_POS]=w;
		}
		if(win->col_attr!=0){
			char *s="";
			if(!find_sql_type_str(win->col_attr[i].type,&s)){
				_snprintf(str,sizeof(str),"%i",win->col_attr[i].type);
				lv_insert_data(hlistview,i,TYPE_POS,str);
			}
			else
				lv_insert_data(hlistview,i,TYPE_POS,s);
			w=get_str_width(hlistview,s)+8;
			if(w>widths[TYPE_POS])
				widths[TYPE_POS]=w;

			_snprintf(str,sizeof(str),"%i",win->col_attr[i].length);
			lv_insert_data(hlistview,i,SIZE_POS,str);
			w=get_str_width(hlistview,str)+8;
			if(w>widths[SIZE_POS])
				widths[SIZE_POS]=w;
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
int clear_selected_items(HWND hlistview)
{
	int result=0;
	if(hlistview!=0){
		int i,count;
		count=ListView_GetItemCount(hlistview);
		for(i=0;i<count;i++){
			int state=ListView_GetItemState(hlistview,i,LVIS_SELECTED);
			if(state&LVIS_SELECTED){
				lv_update_data(hlistview,i,DATA_POS,"");
				result++;
			}
		}
	}
	return result;
}
static int populate_edit_control(HWND hlistview,HWND hedit,int row)
{
	char str[80]={0};
	ListView_GetItemText(hlistview,row,DATA_POS,str,sizeof(str));
	SetWindowText(hedit,str);
	SendMessage(hedit,EM_SETSEL,0,-1);
	return TRUE;
}
static WNDPROC wporigtedit=0;
static LRESULT APIENTRY sc_edit(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	//if(FALSE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("i");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	switch(msg){
	case WM_KILLFOCUS:
		SendMessage(GetParent(hwnd),WM_USER,0,hwnd);
		SendMessage(hwnd,WM_CLOSE,0,0);
		break;
	}
    return CallWindowProc(wporigtedit,hwnd,msg,wparam,lparam);
}

LRESULT CALLBACK insert_dlg_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static TABLE_WINDOW *win=0;
	static HWND hlistview=0,hgrippy=0,hedit=0;

	switch(msg){
	case WM_INITDIALOG:
		if(lparam==0){
			EndDialog(hwnd,0);
			break;
		}
		hedit=0;
		win=lparam;
		hlistview=CreateWindow(WC_LISTVIEW,"",WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|LVS_REPORT|LVS_SHOWSELALWAYS,
                                     0,0,
                                     0,0,
                                     hwnd,
                                     IDC_LIST1,
                                     ghinstance,
                                     NULL);
		ListView_SetExtendedListViewStyle(hlistview,LVS_EX_FULLROWSELECT);
		SendMessage(hlistview,WM_SETFONT,GetStockObject(get_font_setting(IDC_LISTVIEW_FONT)),0);
		populate_insert_dlg(hwnd,hlistview,win);
		hgrippy=create_grippy(hwnd);
		resize_insert_dlg(hwnd);
		break;
	case WM_NOTIFY:
		{
			NMHDR *nmhdr=lparam;
			if(nmhdr!=0 && nmhdr->hwndFrom==hlistview){
				switch(nmhdr->code){
				case NM_DBLCLK:
					if(hedit==0)
						SendMessage(hwnd,WM_COMMAND,IDOK,0);
					break;
				case LVN_KEYDOWN:
					{
						LV_KEYDOWN *key=lparam;
						switch(key->wVKey){
						case 'A':
							if(GetKeyState(VK_CONTROL)&0x8000){
								int i,count;
								count=ListView_GetItemCount(hlistview);
								for(i=0;i<count;i++){
									ListView_SetItemState(hlistview,i,LVIS_SELECTED,LVIS_SELECTED);
								}
							}
							break;
						case VK_DELETE:
							clear_selected_items(hlistview);
							break;
						case ' ':
						case VK_F2:
						case VK_INSERT:
							{
								int row_sel=ListView_GetSelectionMark(hlistview);
								if(row_sel>=0 && hedit==0){
									hedit=CreateWindow("EDIT","",WS_THICKFRAME|WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|
											ES_LEFT|ES_AUTOHSCROLL,
											0,0,
											0,0,
											hwnd,
											IDC_EDIT1,
											ghinstance,
											NULL);
									if(hedit!=0){
										RECT rect={0},crect={0};
										int x,y,w,h;
										ListView_GetItemRect(hlistview,row_sel,&rect,LVIR_BOUNDS);
										lv_get_col_rect(hlistview,DATA_POS,&crect);
										x=crect.left-2;
										y=rect.top-2;
										w=crect.right-crect.left+8;
										h=rect.bottom-rect.top+8;
										SetWindowPos(hedit,HWND_TOP,x,y,w,h,0);
										SendMessage(hedit,WM_SETFONT,GetStockObject(get_font_setting(IDC_LISTVIEW_FONT)),0);
										populate_edit_control(hlistview,hedit,row_sel);
										SetFocus(hedit);
										wporigtedit=SetWindowLong(hedit,GWL_WNDPROC,(LONG)sc_edit);
									}
								}
							}
							break;
						}

					}
					break;
				}

			}
		}
		break;
	case WM_USER:
		if(hedit!=0 && lparam==hedit){
			hedit=0;
			SetFocus(hlistview);
		}
		break;
	case WM_SIZE:
		resize_insert_dlg(hwnd);
		grippy_move(hwnd,hgrippy);
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
			if(hedit!=0){
				if(GetFocus()==hedit){
					char str[80]={0};
					int row_sel=ListView_GetSelectionMark(hlistview);
					GetWindowText(hedit,str,sizeof(str));
					lv_update_data(hlistview,row_sel,DATA_POS,str);
					SendMessage(hedit,WM_CLOSE,0,0);
					hedit=0;
				}
				break;
			}
			else if(GetFocus()==hlistview){
				static LV_KEYDOWN lvk={0};
				lvk.hdr.hwndFrom=hlistview;
				lvk.hdr.code=LVN_KEYDOWN;
				lvk.wVKey=VK_INSERT;
				SendMessage(hwnd,WM_NOTIFY,0,&lvk);
				break;
			}
			else{
				EndDialog(hwnd,0);
			}
			break;
		case IDCANCEL:
			if(hedit!=0){
				SendMessage(hedit,WM_CLOSE,0,0);
				hedit=0;
				break;
			}
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