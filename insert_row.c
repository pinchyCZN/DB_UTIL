#include <windows.h>
#include <ctype.h>
#include <Commctrl.h>
#include "resource.h"
#include "structs.h"

extern HINSTANCE ghinstance;
extern HWND ghmainframe;

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
		w=get_str_width(hlistview,str);
		if(w>widths[FIELD_POS])
			widths[FIELD_POS]=w;
		if(row_sel>=0){
			str[0]=0;
			ListView_GetItemText(win->hlistview,row_sel,i,str,sizeof(str));
			lv_insert_data(hlistview,i,DATA_POS,str);
			w=get_str_width(hlistview,str);
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
			w=get_str_width(hlistview,s);
			if(w>widths[TYPE_POS])
				widths[TYPE_POS]=w;

			_snprintf(str,sizeof(str),"%i",win->col_attr[i].length);
			lv_insert_data(hlistview,i,SIZE_POS,str);
			w=get_str_width(hlistview,str);
			if(w>widths[SIZE_POS])
				widths[SIZE_POS]=w;
		}
	}
	{
		int total_width=0;
		for(i=0;i<4;i++){
			widths[i]+=12;
			ListView_SetColumnWidth(hlistview,i,widths[i]);
			total_width+=widths[i];
		}
		if(total_width>0){
			int width,height;
			RECT rect={0},irect={0},nrect={0};
			GetWindowRect(hwnd,&irect);
			get_nearest_monitor(irect.left,irect.top,total_width,100,&nrect);
			ListView_GetItemRect(hlistview,0,&rect,LVIR_BOUNDS);
			height=80+(count*(rect.bottom-rect.top+2));
			if((irect.top+height)>nrect.bottom){
				height=nrect.bottom-nrect.top-irect.top;
				if(height<320)
					height=320;
			}
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
	if(FALSE)
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
static int set_title(HWND hwnd,TABLE_WINDOW *win)
{
	char str[255]={0};
	char *lbrack="",*rbrack="";
	if(is_sql_reserved(win->table) || strchr(win->table,' ')){
		if(strstri(win->name,"DSN=visual foxpro")!=0){
			lbrack=rbrack="`";
		}
		else{
			lbrack="[";rbrack="]";
		}
	}
	_snprintf(str,sizeof(str),"Add Row %s%s%s",lbrack,win->table,rbrack);
	str[sizeof(str)-1]=0;
	return SetWindowText(hwnd,str);
}
static int add_row_tablewindow(TABLE_WINDOW *win,HWND hlistview)
{
	int i,count,row;
	if(win==0 || hlistview==0)
		return FALSE;
	count=ListView_GetItemCount(win->hlistview);
	row=ListView_GetSelectionMark(win->hlistview);
	if(row<0)
		row=count;
	else
		row++;
	count=ListView_GetItemCount(hlistview);
	for(i=0;i<count;i++){
		char str[255]={0};
		ListView_GetItemText(hlistview,i,DATA_POS,str,sizeof(str));
		lv_insert_data(win->hlistview,row,i,str);
	}
	ListView_EnsureVisible(win->hlistview,row,FALSE);
	ListView_SetSelectionMark(win->hlistview,row);
	ListView_SetItemState(win->hlistview,row,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
	return TRUE;
}
static int resize_column(HWND hwnd,HWND hlistview,char *str,int col)
{
	int cw,sw;
	cw=ListView_GetColumnWidth(hlistview,col);
	sw=get_str_width(hlistview,str)+14;
	if(sw>cw){
		RECT rect={0};
		int diff=sw-cw;
		int w,h;
		GetWindowRect(hwnd,&rect);
		w=rect.right-rect.left;
		h=rect.bottom-rect.top;
		SetWindowPos(hwnd,NULL,0,0,w+diff,h,SWP_NOMOVE|SWP_NOZORDER);
		ListView_SetColumnWidth(hlistview,col,cw+diff);
	}
	return TRUE;
}
int is_entry_key(int key)
{
	int result=FALSE;
	if(isupper(key) && isalpha(key))
		result=TRUE;
	if(isdigit(key))
		result=TRUE;
	return result;
}
LRESULT CALLBACK insert_dlg_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static TABLE_WINDOW *win=0;
	static HWND hlistview=0,hgrippy=0,hedit=0;
	static HFONT hfont=0;
	static WNDPROC origlistview=0;
	if(FALSE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		if(hwnd==hlistview)
			printf("-");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	if(origlistview!=0 && hwnd==hlistview){
		if(msg==WM_GETDLGCODE){
			return DLGC_WANTARROWS;
		}
		return CallWindowProc(origlistview,hwnd,msg,wparam,lparam);
	}
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
		if(hlistview!=0){
			ListView_SetExtendedListViewStyle(hlistview,LVS_EX_FULLROWSELECT);
			hfont=SendMessage(win->hlistview,WM_GETFONT,0,0);
			if(hfont!=0)
				SendMessage(hlistview,WM_SETFONT,hfont,0);
			populate_insert_dlg(hwnd,hlistview,win);
			ListView_SetItemState(hlistview,0,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
			origlistview=SetWindowLong(hlistview,GWL_WNDPROC,(LONG)insert_dlg_proc);
		}
		set_title(hwnd,win);
		hgrippy=create_grippy(hwnd);
		resize_insert_dlg(hwnd);
		break;
	case WM_NOTIFY:
		{
			NMHDR *nmhdr=lparam;
			if(nmhdr!=0){
				switch(nmhdr->code){
				case NM_DBLCLK:
					if(hedit==0 && nmhdr->hwndFrom==hlistview)
						SendMessage(hwnd,WM_COMMAND,IDOK,0);
					break;
				case  LVN_COLUMNCLICK:
					{
						static sort_dir=0;
						NMLISTVIEW *nmlv=lparam;
						if(nmlv!=0){
							sort_listview(hlistview,sort_dir,nmlv->iSubItem);
							sort_dir=!sort_dir;
						}
					}
					break;
				case LVN_KEYDOWN:
					if(nmhdr->hwndFrom==hlistview)
					{
						LV_KEYDOWN *key=lparam;
						switch(key->wVKey){
						case VK_DOWN:
						case VK_RIGHT:
						case VK_NEXT:
							{
								int count,row_sel;
								count=ListView_GetItemCount(hlistview);
								row_sel=ListView_GetSelectionMark(hlistview);
								if(count>0 && row_sel==count-1)
									SetFocus(GetDlgItem(hwnd,IDOK));
							}
							break;
						case VK_DELETE:
							clear_selected_items(hlistview);
							break;
						case VK_F5:
							if(task_insert_row(win,hlistview))
								SetWindowText(GetDlgItem(hwnd,IDOK),"Busy");
							break;
						case 'A':
							if(GetKeyState(VK_CONTROL)&0x8000){
								int i,count;
								count=ListView_GetItemCount(hlistview);
								for(i=0;i<count;i++){
									ListView_SetItemState(hlistview,i,LVIS_SELECTED,LVIS_SELECTED);
								}
								break;
							}
						default:
							{
								int ignore=FALSE;
								if(!is_entry_key(key->wVKey))
									ignore=TRUE;
								if(GetKeyState(VK_CONTROL)&0x8000)
									ignore=TRUE;
								if(ignore)
									return 1;
							}
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
										if(hfont!=0)
											SendMessage(hedit,WM_SETFONT,hfont,0);
										if(is_entry_key(key->wVKey)){
											char str[2];
											char c=tolower(key->wVKey);
											if((GetKeyState(VK_SHIFT)&0x8000) || (GetKeyState(VK_CAPITAL)&1))
												c=toupper(c);
											str[0]=c;
											str[1]=0;
											SetWindowText(hedit,str);
											SendMessage(hedit,EM_SETSEL,1,1);

										}else{
											populate_edit_control(hlistview,hedit,row_sel);
										}
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
	case WM_HSCROLL:
		if(lparam==hgrippy)
			SetFocus(hlistview);
		break;
	case WM_USER:
		if(hedit!=0 && lparam==hedit){
			hedit=0;
			SetFocus(hlistview);
		}
		else if(lparam==hlistview){
			if(wparam==IDOK){
				add_row_tablewindow(win,hlistview);
			}
			else
				SetFocus(GetDlgItem(hwnd,IDCANCEL));

			SetWindowText(GetDlgItem(hwnd,IDOK),"OK");
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
					int count,row_sel=ListView_GetSelectionMark(hlistview);
					GetWindowText(hedit,str,sizeof(str));
					resize_column(hwnd,hlistview,str,1);
					lv_update_data(hlistview,row_sel,DATA_POS,str);
					SendMessage(hedit,WM_CLOSE,0,0);
					count=ListView_GetItemCount(hlistview);
					if(row_sel < (count-1)){
						ListView_SetItemState(hlistview,row_sel,0,LVIS_SELECTED|LVIS_FOCUSED);
						row_sel++;
						ListView_SetItemState(hlistview,row_sel,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
						ListView_SetSelectionMark(hlistview,row_sel);
					}
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
				if(task_insert_row(win,hlistview))
					SetWindowText(GetDlgItem(hwnd,IDOK),"Busy");

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


int do_insert_dlg(HWND hwnd,TABLE_WINDOW *win)
{
	int result;
	result=DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_INSERT),hwnd,insert_dlg_proc,win);
	if(win!=0 && win->hlistview!=0)
		PostMessage(ghmainframe,WM_USER,IDC_MDI_LISTVIEW,win->hlistview);
	return result;
}