#include "search.h"

static HMENU lv_menu=0;
static HMENU lv_col_menu=0;
enum {
	CMD_COL_INFO=10000,
	CMD_COL_WIDTH_HEADER,
	CMD_COL_WIDTH_DATA,
	CMD_SQL_UPDATE,
	CMD_EXPORT_DATA,
};
int get_str_width(HWND hwnd,char *str)
{
	if(hwnd!=0 && str!=0){
		SIZE size={0};
		HDC hdc;
		hdc=GetDC(hwnd);
		if(hdc!=0){
			HFONT hfont;
			hfont=SendMessage(hwnd,WM_GETFONT,0,0);
			if(hfont!=0){
				HGDIOBJ hold=0;
				hold=SelectObject(hdc,hfont);
				GetTextExtentPoint32(hdc,str,strlen(str),&size);
				if(hold!=0)
					SelectObject(hdc,hold);
			}
			else{
				GetTextExtentPoint32(hdc,str,strlen(str),&size);
			}
			ReleaseDC(hwnd,hdc);
			return size.cx;
		}
	}
	return 0;
}
int lv_scroll_column(HWND hlistview,int index)
{
	RECT rect={0};
	int result=FALSE;
	if(lv_get_col_rect(hlistview,index,&rect)){
		RECT rectlv={0};
		SCROLLINFO si;
		si.cbSize=sizeof(si);
		si.fMask=SIF_POS;
		if(	GetClientRect(hlistview,&rectlv) &&
			GetScrollInfo(hlistview,SB_HORZ,&si)){
			int diff=0;
			if(rect.right-si.nPos>rectlv.right)
				diff=rect.right-rectlv.right-si.nPos;
			else if(rect.left-si.nPos<rectlv.left)
				diff=-si.nPos+rect.left;
			if(diff!=0)
				result=ListView_Scroll(hlistview,diff,0);
		}
	}
	return result;
}
int lv_get_col_rect(HWND hlistview,int col,RECT *rect)
{
	int result=FALSE;
	if(hlistview!=0 && rect!=0){
		HWND header=SendMessage(hlistview,LVM_GETHEADER,0,0);
		if(header!=0)
			if(SendMessage(header,HDM_GETITEMRECT,col,rect))
				result=TRUE;
	}
	return result;
}
int lv_get_column_count(HWND hlistview)
{
	HWND header;
	int count=0;
	header=SendMessage(hlistview,LVM_GETHEADER,0,0);
	if(header!=0){
		count=SendMessage(header,HDM_GETITEMCOUNT,0,0);
		if(count<0)
			count=0;
	}
	return count;
}
int lv_get_col_text(HWND hlistview,int index,char *str,int size)
{
	LV_COLUMN col;
	if(hlistview!=0 && str!=0 && size>0){
		col.mask = LVCF_TEXT;
		col.pszText = str;
		col.cchTextMax = size;
		return ListView_GetColumn(hlistview,index,&col);
	}
	return FALSE;
}
int lv_add_column(HWND hlistview,char *str,int index)
{
	LV_COLUMN col;
	if(hlistview!=0 && str!=0){
		HWND header;
		int width=0;
		header=SendMessage(hlistview,LVM_GETHEADER,0,0);
		width=get_str_width(header,str);
		width+=14;
		if(width<40)
			width=40;
		col.mask = LVCF_WIDTH|LVCF_TEXT;
		col.cx = width;
		col.pszText = str;
		if(ListView_InsertColumn(hlistview,index,&col)>=0)
			return width;
	}
	return 0;
}
int lv_update_data(HWND hlistview,int row,int col,char *str)
{
	if(hlistview!=0 && str!=0){
		LV_ITEM item;
		memset(&item,0,sizeof(item));
		item.mask=LVIF_TEXT;
		item.iItem=row;
		item.pszText=str;
		item.iSubItem=col;
		return ListView_SetItem(hlistview,&item);
	}
	return FALSE;
}
int lv_insert_data(HWND hlistview,int row,int col,char *str)
{
	if(hlistview!=0 && str!=0){
		LV_ITEM item;
		memset(&item,0,sizeof(item));
		if(col==0){
			item.mask=LVIF_TEXT|LVIF_PARAM;
			item.iItem=row;
			item.pszText=str;
			item.lParam=row;
			ListView_InsertItem(hlistview,&item);
		}
		else{
			item.mask=LVIF_TEXT;
			item.iItem=row;
			item.pszText=str;
			item.iSubItem=col;
			ListView_SetItem(hlistview,&item);
		}
		return TRUE;
	}
	return FALSE;
}
LRESULT CALLBACK filename_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static char *fname=0;
	switch(msg){
	case WM_INITDIALOG:
		{
			if(lparam==0)
				EndDialog(hwnd,FALSE);
			else{
				char *s=lparam;
				fname=lparam;
				if(fname[0]==0){
					char str[MAX_PATH]={0};
					GetCurrentDirectory(sizeof(str),str);
					s=str;
				}
				SendDlgItemMessage(hwnd,IDC_EDIT1,WM_SETTEXT,0,s);
			}
			SendDlgItemMessage(hwnd,IDC_EDIT1,EM_SETLIMITTEXT,MAX_PATH,0);
			SetFocus(GetDlgItem(hwnd,IDC_EDIT1));
		}
		break;
	case WM_SIZE:
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
			{
				char str[MAX_PATH]={0};
				SendDlgItemMessage(hwnd,IDC_EDIT1,WM_GETTEXT,sizeof(str),str);
				_snprintf(fname,MAX_PATH,"%s",str);
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

static WNDPROC wporiglistview=0;
LRESULT APIENTRY sc_listview(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static int xscroll=-1,yscroll=-1;
	if(FALSE)
	if(msg<=0x1000)
	if(msg!=WM_NCMOUSEMOVE&&msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_NOTIFY
		&&msg!=WM_USER&&msg!=WM_GETFONT)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("l");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	switch(msg){
	case WM_COMMAND:
		switch(wparam){
		case CMD_COL_INFO:
			DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_COL_INFO),hwnd,col_info_proc,hwnd);
			break;
		case CMD_SQL_UPDATE:
			{
				//create a SQL script and copy to clipboard
			}
			break;
		case CMD_EXPORT_DATA:
			{
				static char fname[MAX_PATH]={0};
				if(DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_FILENAME),hwnd,filename_proc,fname)==TRUE){
					FILE *f;
					f=fopen(fname,"wb");
					if(f!=0){
						int i,j,item_count,col_count;
						char *buf;
						int buf_size=0x10000;
						item_count=ListView_GetItemCount(hwnd);
						col_count=lv_get_column_count(hwnd);
						set_status_bar_text(ghstatusbar,0,"exporting data to %s",fname);
						for(i=0;i<col_count;i++){
							char str[255]={0};
							lv_get_col_text(hwnd,i,str,sizeof(str));
							fprintf(f,"%s%s",str,i==col_count-1?"\n":",");
						}
						buf=malloc(buf_size);
						if(buf!=0){
							for(i=0;i<item_count;i++){
								for(j=0;j<col_count;j++){
									buf[0]=0;
									ListView_GetItemText(hwnd,i,j,buf,buf_size);
									fprintf(f,"%s%s",buf,j==col_count-1?"\n":",");
								}
							}
							free(buf);
						}
						else
							fprintf(f,"cant allocate buffer of size %08X\n",buf_size);
						fclose(f);
						set_status_bar_text(ghstatusbar,0,"finished export to %s",fname);
					}else{
						set_status_bar_text(ghstatusbar,0,"cant open %s",fname);
						fname[0]=0;
					}
				}

			}
			break;
		case CMD_COL_WIDTH_DATA:
			{
				TABLE_WINDOW *win=0;
				if(find_win_by_hlistview(hwnd,&win)){
					int i,width,top,count,done;
					top=ListView_GetTopIndex(win->hlistview);
					count=ListView_GetItemCount(win->hlistview);
					width=0;
					done=0;
					for(i=top;i<count;i++){
						char str[100]={0};
						ListView_GetItemText(win->hlistview,i,win->selected_column,str,sizeof(str));
						str[sizeof(str)-1]=0;
						if(str[0]!=0){
							int w=get_str_width(win->hlistview,str);
							if(w>width)
								width=w;
						}
						done++;
						if(done>500)
							break;
					}
					if(width<=0)
						width=10;
					else if(width>1000)
						width=1000;
					ListView_SetColumnWidth(win->hlistview,win->selected_column,width);
				}
			}
			break;
		case CMD_COL_WIDTH_HEADER:
			{
				TABLE_WINDOW *win=0;
				if(find_win_by_hlistview(hwnd,&win)){
					char str[80]={0};
					lv_get_col_text(win->hlistview,win->selected_column,str,sizeof(str));
					if(str[0]!=0){
						int width;
						width=get_str_width(win->hlistview,str);
						if(width>0)
							ListView_SetColumnWidth(win->hlistview,win->selected_column,width+14);

					}
				}
			}
			break;
		}
		break;
	case WM_VSCROLL:
		break;
	case WM_HSCROLL:
		{
			TABLE_WINDOW *win=0;
			if(find_win_by_hlistview(hwnd,&win)){
				switch(LOWORD(wparam)){
				case SB_THUMBTRACK:
					if(win->hlvedit!=0){
						RECT rect={0};
						int pos=HIWORD(wparam);
						if(xscroll<0)
							xscroll=pos;

						GetWindowRect(win->hlvedit,&rect);
						MapWindowPoints(NULL,win->hlistview,&rect,2);
						if(SetWindowPos(win->hlvedit,HWND_TOP,rect.left-(pos-xscroll),rect.top,0,0,SWP_NOSIZE)){
							int item;
							item=ListView_GetSelectionMark(win->hlistview);
							ListView_RedrawItems(win->hlistview,item,item);
							InvalidateRect(win->hlvedit,NULL,FALSE);
							printf("pos=%i xscroll=%i item=%i\n",pos,xscroll,item);
						}
						xscroll=pos;
					}
					break;
				case SB_ENDSCROLL:
					xscroll=-1;
					yscroll=-1;
					break;

				}
			}
		}
		break;
	case WM_KEYFIRST:
		switch(wparam){
/*
		case VK_ESCAPE:
			{
				TABLE_WINDOW *win=0;
				if(find_win_by_hlistview(hwnd,&win)){
					SendMessage(win->hwnd,WM_USER,0,IDC_MDI_LISTVIEW);
					SetFocus(win->hedit);
				}
			}
			break;
		case 'C':
			if(GetKeyState(VK_CONTROL)&0x8000){
				TABLE_WINDOW *win=0;
				if(find_win_by_hlistview(hwnd,&win)){
					int sel=ListView_GetSelectionMark(win->hlistview);
					if(sel>=0){
						char str[255]={0};
						ListView_GetItemText(win->hlistview,sel,win->selected_column,str,sizeof(str))
						copy_str_clipboard(str);
					}
				}
			}
			break;
		case 'F':
			if(GetKeyState(VK_CONTROL)&0x8000){
				TABLE_WINDOW *win=0;
				if(find_win_by_hlistview(hwnd,&win)){
					DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_SEARCH),hwnd,search_proc,win);
				}
			}
			break;
*/
		}
		break;
	case WM_CONTEXTMENU:
		{
			RECT rect={0};
			HANDLE header;
			int x=0,y=0;
			HMENU hmenu=0;
			header=SendMessage(hwnd,LVM_GETHEADER,0,0);
			if(header!=0)
				GetWindowRect(header,&rect);
			x=LOWORD(lparam);
			y=HIWORD(lparam);

			if(y<=rect.bottom){
				LV_HITTESTINFO ht={0};
				ht.pt.x=x;
				ht.pt.y=rect.bottom+2;
				ScreenToClient(hwnd,&ht.pt);
				if(ListView_SubItemHitTest(hwnd,&ht)>=0){
					TABLE_WINDOW *win=0;
					if(find_win_by_hlistview(hwnd,&win)){
						win->selected_column=ht.iSubItem;
						InvalidateRect(hwnd,NULL,FALSE);
					}
					printf("hit test %08X %i %i\n",ht.flags,ht.iItem,ht.iSubItem);
				}
				hmenu=lv_col_menu;
			}
			else{
				LV_HITTESTINFO ht={0};
				ht.pt.x=x;
				ht.pt.y=y;
				ScreenToClient(hwnd,&ht.pt);
				ListView_SubItemHitTest(hwnd,&ht);
				printf("hit test %08X %i %i\n",ht.flags,ht.iItem,ht.iSubItem);
				hmenu=lv_menu;
				//LVHT_ONITEMLABEL
			}
			if(hmenu!=0)
				TrackPopupMenu(hmenu,TPM_LEFTALIGN,x,y,0,hwnd,NULL);
		}
		break;
	case WM_DESTROY:
		break; //return 0; //CallWindowProc(wporiglistview,hwnd,msg,wparam,lparam);
	}
    return CallWindowProc(wporiglistview,hwnd,msg,wparam,lparam);
}
int subclass_listview(HWND hlistview)
{
	wporiglistview=SetWindowLong(hlistview,GWL_WNDPROC,(LONG)sc_listview);
	printf("subclass=%08X\n",wporiglistview);
	return wporiglistview;
}



int draw_item(DRAWITEMSTRUCT *di,TABLE_WINDOW *win)
{
	int i,count,xpos;
	int textcolor,bgcolor;
	RECT focus_rect={0},client_rect={0};

	count=lv_get_column_count(di->hwndItem);
	if(count>1000)
		count=1000;

	GetClientRect(di->hwndItem,&client_rect);

	bgcolor=GetSysColor(di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHT:COLOR_WINDOW);
	textcolor=GetSysColor(di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHTTEXT:COLOR_WINDOWTEXT);
	xpos=0;
	for(i=0;i<count;i++){
		char text[1024]={0};
		LV_ITEM lvi={0};
		RECT rect;
		int width,style;

		width=ListView_GetColumnWidth(di->hwndItem,i);

		rect=di->rcItem;
		rect.left+=xpos;
		rect.right=rect.left+width;
		if(rect.right<0)
			i=i;
		if(rect.left>client_rect.right)
			i=i;
		xpos+=width;


		if(rect.right>=0 && rect.left<=client_rect.right){
			lvi.mask=LVIF_TEXT;
			lvi.iItem=di->itemID;
			lvi.iSubItem=i;
			lvi.pszText=text;
			lvi.cchTextMax=sizeof(text);

			ListView_GetItemText(di->hwndItem,di->itemID,i,text,sizeof(text));
			text[sizeof(text)-1]=0;

			//rect.left=xpos;
			//rect.right=xpos+width;
		//	DrawText(di->hDC,text,-1,&rect,style);
		//	SetTextColor(di->hDC,(0xFFFFFF^GetSysColor(COLOR_BTNTEXT)));
			if((di->itemState&ODS_SELECTED) && win!=0 && win->selected_column==i){
				FillRect(di->hDC,&rect,COLOR_WINDOW+1);
				focus_rect=rect;
			}
			else{
				FillRect(di->hDC,&rect,di->itemState&ODS_SELECTED ? COLOR_HIGHLIGHT+1:COLOR_WINDOW+1);
			}
			FrameRect(di->hDC,&rect,GetStockObject(DKGRAY_BRUSH));
			if(text[0]!=0){

				SetTextColor(di->hDC,textcolor);
				SetBkColor(di->hDC,bgcolor);

				style=DT_LEFT|DT_NOPREFIX;
				style=DT_RIGHT|DT_NOPREFIX;

			//		SetTextColor(di->hDC,file_text_color);

				//GetTextExtentPoint32(di->hDC,text,strlen(text),&size);
				//rect.right=rect.left+size.cx;
				DrawText(di->hDC,text,-1,&rect,style);
			}
		}
	}
	if(di->itemState&ODS_SELECTED)
		DrawFocusRect(di->hDC,&focus_rect);

	//if(di->itemState&ODS_FOCUS)
		

	/*
	switch(di->itemAction){
	case ODA_DRAWENTIRE:
		if(di->itemState&ODS_FOCUS)
			DrawFocusRect(di->hDC,&di->rcItem); 
		break;
	case ODA_FOCUS:
		if(di->itemState&ODS_FOCUS)
			DrawFocusRect(di->hDC,&di->rcItem); 
		break;
	case ODA_SELECT:
		if(di->itemState&ODS_FOCUS)
			DrawFocusRect(di->hDC,&di->rcItem); 
		break;
	}
	*/
	//GetTextExtentPoint32(di->hDC,list_string,strlen(list_string),&size);
	//set_list_width(size.cx+16);
	return TRUE;
}

int find_win_by_hlvedit(HWND hwnd,TABLE_WINDOW **win)
{
	int i;
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(table_windows[i].hlvedit==hwnd){
			*win=&table_windows[i];
			return TRUE;
		}
	}
	return FALSE;
}
static WNDPROC lvorigedit=0;
LRESULT APIENTRY sc_lvedit(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	if(FALSE)
	if(msg!=WM_NCMOUSEMOVE&&msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE //&&msg!=WM_NOTIFY
		&&msg!=WM_ERASEBKGND)
		//if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("e");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
	switch(msg){
	case WM_HELP:
		return TRUE;
		break;
	case WM_KILLFOCUS:
		//if(wparam!=0)
		{
			TABLE_WINDOW *win=0;
			if(find_win_by_hlvedit(hwnd,&win))
				PostMessage(win->hwnd,WM_USER,win,MAKELPARAM(IDC_LV_EDIT,IDCANCEL));
		}
		break;
	case WM_KEYFIRST:
		switch(wparam){
		case 'A':
			if(GetKeyState(VK_CONTROL)&0x8000)
				SendMessage(hwnd,EM_SETSEL,0,-1);
			break;
		case VK_F1:
			{
				RECT rect={0};
				GetClientRect(hwnd,&rect);
				if(rect.right>5000)
					break;
				SetWindowPos(hwnd,NULL,0,0,rect.right*1.75,rect.bottom,SWP_NOOWNERZORDER|SWP_NOMOVE);
			}
			break;
		case VK_RETURN:
			{
				TABLE_WINDOW *win=0;
				if(find_win_by_hlvedit(hwnd,&win)){
					int index;
					char text[1024]={0};
					index=ListView_GetSelectionMark(win->hlistview);
					GetWindowText(win->hlvedit,text,sizeof(text));
					task_update_record(win,index,text);
					PostMessage(win->hwnd,WM_USER,win,MAKELPARAM(IDC_LV_EDIT,IDOK));
				}
			}
			break;
		case VK_ESCAPE:
			{
				TABLE_WINDOW *win=0;
				if(find_win_by_hlvedit(hwnd,&win))
					PostMessage(win->hwnd,WM_USER,win,MAKELPARAM(IDC_LV_EDIT,IDCANCEL));
			}
		}
		break;
	}
    return CallWindowProc(lvorigedit,hwnd,msg,wparam,lparam);
}

int create_lv_edit_selected(TABLE_WINDOW *win)
{
	if(win!=0 && win->hlistview!=0){
		int index=0;
		index=ListView_GetSelectionMark(win->hlistview);
		if(index>=0){
			RECT rect={0};
			if(ListView_GetSubItemRect(win->hlistview,index,win->selected_column,LVIR_LABEL,&rect)!=0){
				char text[255]={0};
				create_lv_edit(win,&rect);
				ListView_GetItemText(win->hlistview,index,win->selected_column,text,sizeof(text));
				if(win->hlvedit!=0){
					if(text[0]!=0)
						SetWindowText(win->hlvedit,text);
					SetFocus(win->hlvedit);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}
int create_lv_menus()
{
	if(lv_col_menu!=0)DestroyMenu(lv_col_menu);
	if(lv_col_menu=CreatePopupMenu()){
		InsertMenu(lv_col_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_COL_WIDTH_HEADER,"col width from header");
		InsertMenu(lv_col_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_COL_WIDTH_DATA,"col width from data");
		InsertMenu(lv_col_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_COL_INFO,"col info");
	}
	if(lv_menu!=0)DestroyMenu(lv_menu);
	if(lv_menu=CreatePopupMenu()){
		InsertMenu(lv_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_COL_INFO,"col info");
		InsertMenu(lv_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_SQL_UPDATE,"create update SQL statement");
		InsertMenu(lv_menu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,CMD_EXPORT_DATA,"export data");
	}
	return TRUE;
}
int create_lv_edit(TABLE_WINDOW *win,RECT *rect)
{
	int result=FALSE;
	if(win!=0 && rect!=0){
		if(win->hlvedit!=0)
			destroy_lv_edit(win);
		win->hlvedit = CreateWindow("EDIT", //"RichEdit50W",
										 "",
										 WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|
										 ES_LEFT|ES_AUTOHSCROLL|ES_MULTILINE|ES_AUTOVSCROLL,
										 rect->left-1,rect->top-1,
										 rect->right - rect->left+2,
										 rect->bottom - rect->top+2,
										 win->hlistview,
										 IDC_LV_EDIT,
										 ghinstance,
										 NULL);
		if(win->hlvedit!=0){
			lvorigedit=SetWindowLong(win->hlvedit,GWL_WNDPROC,(LONG)sc_lvedit);
			SetWindowText(ghstatusbar,"");
			result=TRUE;
		}
	}
	return result;
}

int destroy_lv_edit(TABLE_WINDOW *win)
{
	if(win!=0 && win->hlvedit!=0){
		SendMessage(win->hlvedit,WM_CLOSE,0,0);
		win->hlvedit=0;
	}
	return TRUE;
}