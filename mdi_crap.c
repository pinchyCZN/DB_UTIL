#if _WIN32_WINNT<0x400
#define _WIN32_WINNT 0x400
#define WINVER 0x500
#endif
#if _WIN32_IE<=0x300
#define _WIN32_IE 0x400
#endif
#include <windows.h>
#include <Commctrl.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <richedit.h>
#include <math.h>
#include "resource.h"

extern HINSTANCE ghinstance;
extern HWND ghmainframe,ghmdiclient,ghtreeview,ghdbview,ghstatusbar;

#include "structs.h"


static TABLE_WINDOW table_windows[50];
static DB_TREE db_tree[20];

#include "col_info.h"
#include "treeview.h"
#include "listview.h"
#include "tile_win_dialog.h"

LRESULT CALLBACK MDIChildWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
#define DEFAULT_SPLIT_POS 60
	static int split_drag=FALSE,mdi_split=DEFAULT_SPLIT_POS;
	static HWND last_focus=0,hwndTT=0;
	if(FALSE)
	if(msg!=WM_NCMOUSEMOVE&&msg!=WM_MOUSEFIRST&&msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE&&msg!=WM_NOTIFY
		&&msg!=WM_ERASEBKGND&&msg!=WM_DRAWITEM) 
		//if(msg!=WM_NCHITTEST&&msg!=WM_SETCURSOR&&msg!=WM_ENTERIDLE)
	{
		static DWORD tick=0;
		if((GetTickCount()-tick)>500)
			printf("--\n");
		printf("m");
		print_msg(msg,lparam,wparam,hwnd);
		tick=GetTickCount();
	}
    switch(msg)
    {
	case WM_CREATE:
		{
			TABLE_WINDOW *win=0;
			LPCREATESTRUCT pcs = (LPCREATESTRUCT)lparam;
			LPMDICREATESTRUCT pmdics = (LPMDICREATESTRUCT)(pcs->lpCreateParams);
			win=pmdics->lParam;
			if(win!=0){
				win->hwnd=hwnd;
				win->split_pos=DEFAULT_SPLIT_POS;
			}
			create_mdi_window(hwnd,ghinstance,win);

			SendDlgItemMessage(hwnd,IDC_MDI_EDIT,WM_SETFONT,GetStockObject(get_font_setting(IDC_SQL_FONT)),0);
			SendDlgItemMessage(hwnd,IDC_MDI_LISTVIEW,WM_SETFONT,GetStockObject(get_font_setting(IDC_LISTVIEW_FONT)),0);
			load_mdi_size(hwnd);
		}
        break;
	case WM_CHAR:
		if(GetFocus()==GetDlgItem(hwnd,IDC_SQL_ABORT)){
			TABLE_WINDOW *win=0;
			if(find_win_by_hwnd(hwnd,&win))
				win->abort;
		}
		break;
	case WM_SETFOCUS:
		break;
	case WM_NCACTIVATE:
		break;
	case WM_MDIACTIVATE:
		{
			if(lparam==0) //nothing gaining focus so move to tree
				PostMessage(ghmainframe,WM_USER,IDC_TREEVIEW,ghtreeview);
			else{
				TABLE_WINDOW *win=0;
				if(find_win_by_hwnd(hwnd,&win)){
					if(wparam==hwnd){ //losing focus
						HANDLE hfocus=GetFocus();
						if(hfocus==win->hlistview || hfocus==win->hedit)
							win->hlastfocus=hfocus;
						else if(hfocus==win->hlvedit)
							win->hlastfocus=win->hlistview;
						else
							win->hlastfocus=win->hlistview;
						printf("last focus=%08X\n",win->hlastfocus);
						if(win->hlastfocus==win->hlistview)
							printf("using listview\n");
						if(win->hlastfocus==win->hedit)
							printf("using edit\n");
					}
					else{
						PostMessage(hwnd,WM_USER,win->hlastfocus,IDC_MDI_CLIENT);
						printf("sending user %08X\n",win->hlastfocus);
					}
				}
			}
		}
        break;
	case WM_NOTIFY:
		{
			NMHDR *nmhdr=lparam;
			TABLE_WINDOW *win=0;
			if(nmhdr!=0 && nmhdr->idFrom==IDC_MDI_LISTVIEW){
				LV_HITTESTINFO lvhit={0};
				switch(nmhdr->code){
				case NM_DBLCLK:
					find_win_by_hwnd(hwnd,&win);
					if(win!=0){
						GetCursorPos(&lvhit.pt);
						ScreenToClient(nmhdr->hwndFrom,&lvhit.pt);
						if(ListView_SubItemHitTest(nmhdr->hwndFrom,&lvhit)>=0)
							create_lv_edit_selected(win);
					}
					break;
				case NM_RCLICK:
				case NM_CLICK:
					GetCursorPos(&lvhit.pt);
					ScreenToClient(nmhdr->hwndFrom,&lvhit.pt);
					if(ListView_SubItemHitTest(nmhdr->hwndFrom,&lvhit)>=0){
						find_win_by_hwnd(hwnd,&win);
						if(win!=0){
							win->selected_column=lvhit.iSubItem;
							ListView_RedrawItems(nmhdr->hwndFrom,lvhit.iItem,lvhit.iItem);
							UpdateWindow(nmhdr->hwndFrom);
							set_status_bar_text(ghstatusbar,1,"row=%3i col=%2i",lvhit.iItem+1,lvhit.iSubItem+1);
							do_search(win,0,0,0,0,0); //reset search position
						}
					}
					printf("item = %i\n",lvhit.iSubItem);
					break;
				case LVN_ITEMCHANGED:
					{
						find_win_by_hwnd(hwnd,&win);
						if(win!=0){
							set_status_bar_text(ghstatusbar,1,"row=%3i col=%2i",ListView_GetSelectionMark(win->hlistview)+1,win->selected_column+1);
						}
					}
					break;
				case LVN_KEYDOWN:
					{
						find_win_by_hwnd(hwnd,&win);
						if(win!=0){
							int dir=0;
							LV_KEYDOWN *lvkey=lparam;
							switch(lvkey->wVKey){
							case VK_F2:
							case VK_RETURN:
								if(GetKeyState(VK_MENU)&0x8000){
									int message=WM_MDIMAXIMIZE;
									if(IsZoomed(win->hwnd))
										message=WM_MDIRESTORE;
									SendMessage(ghmdiclient,message,win->hwnd,0);
								}
								else
									create_lv_edit_selected(win);
								break;
							case VK_UP:
							case VK_DOWN:
							case VK_NEXT:
							case VK_PRIOR:
							case VK_HOME:
							case VK_END:
								do_search(win,0,0,0,0,0); //reset search position
								break;
							case VK_LEFT:
								dir=-1;
								do_search(win,0,0,0,0,0); //reset search position
								break;
							case VK_RIGHT:
								dir=1;
								do_search(win,0,0,0,0,0); //reset search position
								break;
							case VK_ESCAPE:
								destroy_lv_edit(win);
								SendMessage(win->hwnd,WM_USER,0,IDC_MDI_LISTVIEW);
								SetFocus(win->hedit);
								break;
							case VK_INSERT:
								do_insert_dlg(hwnd,win);
								break;
							case VK_DELETE:
								{
									int row=ListView_GetSelectionMark(win->hlistview);
									if(row>=0){
										char str[256]={0};
										char col[80]={0};
										ListView_GetItemText(win->hlistview,row,win->selected_column,col,sizeof(col));
										_snprintf(str,sizeof(str),"OK to delete row %i ?\r\n(cell data=%s)",row+1,col);
										if(MessageBox(win->hwnd,str,"warning",MB_OKCANCEL)==IDOK)
											task_delete_row(win,row);
									}
								}
								break;
							case VK_F1:
								SendMessage(win->hlistview,WM_COMMAND,CMD_COL_INFO,0);
								break;
							case VK_F3:
								{
									char *find=0;
									search_history(0,0,0,&find);
									if(find!=0 && find[0]!=0){
										int dir=IDC_SEARCH_DOWN,whole_word=FALSE;
										int result;
										if(GetKeyState(VK_SHIFT)&0x8000)
											dir=IDC_SEARCH_UP;
										if(GetKeyState(VK_CONTROL)&0x8000)
											whole_word=TRUE;
										set_status_bar_text(ghstatusbar,0,"searching for %s",find);
										result=do_search(win,NULL,find,dir,0,whole_word);
										set_status_bar_text(ghstatusbar,0,"searched for:%s%s%s",find,whole_word?",(whole word)":"",result?", found":", nothing found");
									}
								}
								break;
							case 'E':
								if(GetKeyState(VK_CONTROL)&0x8000){
									PostMessage(win->hlistview,WM_COMMAND,CMD_EXPORT_DATA,0);
								}
								break;
							case 'C':
								if(GetKeyState(VK_CONTROL)&0x8000){
									int sel=ListView_GetSelectionMark(win->hlistview);
									if(sel>=0){
										if(GetKeyState(VK_SHIFT)&0x8000){
											RECT rect={0};
											int x,y,count;
											const char *busymsg="Busy copying to clipboard";
											GetWindowRect(GetParent(hwnd),&rect);
											x=((rect.left+rect.right)/2)-(get_str_width(win->hlistview,busymsg)/2);
											y=(rect.top+rect.bottom)/2;
											create_tooltip(win->hlistview,busymsg,x,y,&hwndTT);
											count=copy_cols_clip(win->hlistview,GetKeyState(VK_MENU)&0x8000);
											set_status_bar_text(ghstatusbar,0,"copied %i rows to clipboard",count);
											destroy_tooltip(hwndTT);
											hwndTT=0;
										}
										else{
											char *buf;
											int buf_size=0x10000;
											buf=malloc(buf_size);
											if(buf!=0){
												if(GetKeyState(VK_MENU)&0x8000){
													int len=get_clipboard(buf,buf_size);
													if(buf_size>(len+1)){
														buf[len]=',';
														buf[len+1]=0;
														len++;
														ListView_GetItemText(win->hlistview,sel,win->selected_column,buf+len,buf_size-len);
													}
												}
												else
													ListView_GetItemText(win->hlistview,sel,win->selected_column,buf,buf_size);
												buf[buf_size-1]=0;
												copy_str_clipboard(buf);
												free(buf);
												set_status_bar_text(ghstatusbar,0,"copied string to clipboard");
											}
										}
									}
								}
								break;
							case 'A':
								if(GetKeyState(VK_CONTROL)&0x8000){
									int i,count;
									count=ListView_GetItemCount(win->hlistview);
									for(i=0;i<count;i++){
										ListView_SetItemState(win->hlistview,i,LVIS_SELECTED,LVIS_SELECTED);
									}
								}
								break;
							case 'F':
								if(GetKeyState(VK_CONTROL)&0x8000)
									DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_SEARCH),hwnd,search_proc,win);
								break;
							}
							if(dir!=0){
								int ctrl=GetKeyState(VK_CONTROL)&0x8000;
								if(ctrl){
									if(dir>0)
										win->selected_column=win->columns-1;
									else
										win->selected_column=0;
								}
								else
									win->selected_column+=dir;
								if(win->selected_column<0)
									win->selected_column=0;
								if(win->selected_column>=win->columns)
									win->selected_column=win->columns-1;
								lv_scroll_column(win->hlistview,win->selected_column);
								set_status_bar_text(ghstatusbar,1,"row=%3i col=%2i",ListView_GetSelectionMark(win->hlistview)+1,win->selected_column+1);

							}
						}
					}
					break;
				case LVN_COLUMNCLICK:
					break;
				}
			}
		}
		break;
	case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT *di=lparam;
			if(di!=0 && di->CtlType==ODT_LISTVIEW){
				TABLE_WINDOW *win=0;
				find_win_by_hwnd(hwnd,&win);
				if(win!=0){
					draw_item(di,win);
					return TRUE;
				}
			}
		}
		break;
	case WM_CTLCOLORSTATIC:
		/*{
			RECT rect;
			HDC  hdc=(HDC)wparam;
			HWND ctrl=(HWND)lparam;
			COLORREF color=GetSysColor(COLOR_WINDOWTEXT);
			HBRUSH hbrush=0;
			hbrush=GetSysColorBrush(COLOR_BACKGROUND);
			if(last_static_msg!=WM_PAINT && last_static_msg!=WM_ERASEBKGND){
			//	GetClientRect(ctrl,&rect);
			//	FillRect(hdc,&rect,hbrush);
				InvalidateRect(ctrl,NULL,TRUE);
			}
			SetBkMode(hdc,TRANSPARENT);
			SetTextColor(hdc,color);
			return (LRESULT)hbrush;
		}*/
		break;
	case WM_DESTROY:
		break;
	case WM_ENDSESSION:
		break;
	case WM_CLOSE:
		save_mdi_size(hwnd);
		destroy_tooltip(hwndTT);
		hwndTT=0;
		{
			TABLE_WINDOW *win=0;
			if(find_win_by_hwnd(hwnd,&win)){
				free_window(win);
			}
		}
		break;
	case WM_KILLFOCUS:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
		if(split_drag){
			TABLE_WINDOW *win=0;
			ReleaseCapture();
			write_ini_value("SETTINGS","MDI_SPLIT",mdi_split);
			split_drag=FALSE;
			if(find_win_by_hwnd(hwnd,&win))
				win->split_pos=mdi_split;
		}
		break;
	case WM_LBUTTONDOWN:
		{
			TABLE_WINDOW *win=0;
			int y=HIWORD(lparam);
			int split=mdi_split;
			if(find_win_by_hwnd(hwnd,&win)){
				RECT rect={0};
				GetWindowRect(win->hedit,&rect);
				MapWindowPoints(NULL,win->hwnd,&rect,2);
				split=rect.bottom;
			}
			if(y>=(split-10) && y<=(split+10)){
				SetCapture(hwnd);
				SetCursor(LoadCursor(NULL,IDC_SIZENS));
				split_drag=TRUE;
			}
		}
		break;
	case WM_MOUSEFIRST:
		{
			TABLE_WINDOW *win=0;						
			int y=HIWORD(lparam);
			int split=mdi_split;
			if(find_win_by_hwnd(hwnd,&win)){
				RECT rect={0};
				GetWindowRect(win->hedit,&rect);
				MapWindowPoints(NULL,win->hwnd,&rect,2);
				split=rect.bottom;
			}
			if(y>=(split-10) && y<=(split+10))
				SetCursor(LoadCursor(NULL,IDC_SIZENS));
			if(split_drag){
				RECT rect;
				GetClientRect(hwnd,&rect);
				if(y>5 && y<rect.bottom-8){
					mdi_split=y;
					resize_mdi_window(hwnd,mdi_split);
					if(win!=0)
						win->split_pos=mdi_split;
				}
			}
		}
		break;
	case WM_VKEYTOITEM:
		switch(LOWORD(wparam)){
		case VK_RETURN:
			{
				TABLE_WINDOW *win=0;
				if(!find_win_by_hwnd(hwnd,&win))
					break;
				SendMessage(win->hedit,WM_KEYFIRST,VK_RETURN,0);
			}
			break;
		}
		break;
    case WM_COMMAND:
		//HIWORD(wParam) notification code
		//LOWORD(wParam) item control
		//lParam handle of control
		switch(LOWORD(wparam)){
		case IDC_MDI_EDIT:
			switch(HIWORD(wparam)){
			case EN_UPDATE:
				{
					TABLE_WINDOW *win=0;
					if(split_drag)
						break;
					if(find_win_by_hwnd(hwnd,&win)){
						HWND hedit=lparam;
						if(win->split_locked)
							break;
						if(hedit){
							int lines,index,h;
							POINT ptop={0},pbot={0};
							lines=SendMessage(win->hedit,EM_GETLINECOUNT,0,0);
							index=SendMessage(win->hedit,EM_LINEINDEX,lines-1,0);
							SendMessage(win->hedit,EM_POSFROMCHAR,&pbot,index);
							SendMessage(win->hedit,EM_POSFROMCHAR,&ptop,0);
							h=get_str_height(win->hedit,"X");
							h=(pbot.y-ptop.y)+h;
							if(h>0 && win->split_pos!=h){
								RECT rect={0};
								GetClientRect(win->hwnd,&rect);
								if(h<(rect.bottom-20)){
									win->split_pos=h;
									PostMessage(hwnd,WM_SIZE,0,0);
								}
							}
						}
					}
				}
			}
			break;
		case IDC_SPLIT_LOCK:
			switch(HIWORD(wparam)){
			case BN_CLICKED:
				{
					TABLE_WINDOW *win=0;
					if(find_win_by_hwnd(hwnd,&win)){
						win->split_locked^=1;
						if(win->hlock){
							char *s="O";
							if(win->split_locked)
								s="L";
							SetWindowText(win->hlock,s);
						}
					}
				}
				break;
			}
			break;
		case IDC_INTELLISENSE:
			switch(HIWORD(wparam)){
			case LBN_DBLCLK:
				{
				TABLE_WINDOW *win=0;
				if(!find_win_by_hwnd(hwnd,&win))
					break;
				SendMessage(win->hedit,WM_KEYFIRST,VK_RETURN,0);
				}
				break;
			}
			break;
		case IDC_SQL_ABORT:
			{
				TABLE_WINDOW *win=0;
				find_win_by_hwnd(hwnd,&win);
				if(win!=0)
					win->abort=TRUE;
			}
			break;
		}
		break;
	case WM_HELP:
		break;
	case WM_USER+1:
		ShowWindow(hwnd,SW_MAXIMIZE);
		break;
	case WM_USER:
		{
			TABLE_WINDOW *win=0;
			find_win_by_hwnd(hwnd,&win);
			switch(LOWORD(lparam)){
			case IDC_TREEVIEW:
				SetFocus(ghtreeview);
				break;
			case IDC_LV_EDIT:
				switch(HIWORD(lparam)){
				case IDOK:
				default:
				case IDCANCEL:
					destroy_lv_edit(wparam);
					set_status_bar_text(ghstatusbar,0,"");
					break;
				}
				break;
			case IDC_MDI_CLIENT:
				if(wparam!=0){
					printf("setting focus %08X\n",wparam);
					if(wparam==hwnd){
						if(win!=0)
							if(win->hlastfocus!=0)
								SetFocus(win->hlastfocus);
							else
								SetFocus(win->hlistview);
					}
					else
						SetFocus(wparam);
				}
				return 0;
				break;
			case IDC_MDI_LISTVIEW:
				if(GetKeyState(VK_SHIFT)&0x8000){
					RECT rect={0};
					int y;
					GetClientRect(hwnd,&rect);
					y=rect.bottom-8;
					if(y<0)
						y=0;
					resize_mdi_window(hwnd,y);
				}
				else{
					int split=mdi_split;
					if(win!=0)
						split=win->split_pos;
					resize_mdi_window(hwnd,split);
				}
				break;
			case IDC_MDI_EDIT:
				{
					int y=2;
					resize_mdi_window(hwnd,y);
				}
				break;
			case IDC_SQL_ABORT:
				if(HIWORD(lparam))
					create_abort(wparam);
				else
					destroy_abort(wparam);
				break;
			case IDC_SPLIT_LOCK:
				{
					int dir=1;
					switch(wparam){
					case VK_F4:
						{
							char *s="O";
							win->split_locked^=1;
							if(win->split_locked)
								s="L";
							SetWindowText(win->hlock,s);
						}
						break;
					case VK_UP:
						dir=-1;
					case VK_DOWN:
						if(win!=0){
							int y=20;
							int pos=win->split_pos;
							RECT rect={0};
							GetClientRect(win->hwnd,&rect);
							pos+=y*dir;
							if(pos>5 && pos<(rect.bottom-20)){
								win->split_pos=pos;
								win->split_locked=1;
								SetWindowText(win->hlock,"L");
								resize_mdi_window(hwnd,pos);
							}
						}
					}
				}
				break;
			}
		}
		break;
	case WM_SYSCOMMAND:
		switch(wparam&0xFFF0){
		case SC_MAXIMIZE:
			write_ini_value("SETTINGS","mdi_maximized",1);
			break;
		case SC_RESTORE:
			write_ini_value("SETTINGS","mdi_maximized",0);
			break;
		}
		break;
	case WM_SIZE:
		{
			TABLE_WINDOW *win=0;
			int split=mdi_split;
			if(find_win_by_hwnd(hwnd,&win))
				split=win->split_pos;
			resize_mdi_window(hwnd,split);
		}
		break;

    }
	return DefMDIChildProc(hwnd, msg, wparam, lparam);
}
int get_str_height(HWND hwnd,char *str)
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
			return size.cy;
		}
	}
	return 25;
}
int load_mdi_size(HWND hwnd)
{
extern int save_mdi_win_size;
	if(save_mdi_win_size){
		int width=0,height=0,max=0;
		get_ini_value("SETTINGS","mdi_width",&width);
		get_ini_value("SETTINGS","mdi_height",&height);
		get_ini_value("SETTINGS","mdi_maximized",&max);
		if(width!=0 && height!=0){
			RECT rect={0};
			GetClientRect(ghmdiclient,&rect);
			if(width>rect.right)
				width=rect.right;
			if(height>rect.bottom)
				height=rect.bottom;
			if(width<(rect.right/4))
				width=rect.right/4;
			if(height<(rect.bottom/4))
				height=rect.bottom/4;
			SetWindowPos(hwnd,HWND_TOP,0,0,width,height,SWP_SHOWWINDOW|SWP_NOMOVE);
			if(max)
				PostMessage(hwnd,WM_USER+1,0,0);
			return TRUE;
		}
		return FALSE;
	}
	else{
		int i,ypos,limit,w,h,count=0;
		int flags=0;
		RECT rect={0};
		RECT wrect={0};
		for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
			TABLE_WINDOW *win=&table_windows[i];
			if(win->hwnd!=0 && win->hwnd!=hwnd)
				count++;
		}
		ypos=GetSystemMetrics(SM_CYCAPTION);
		ypos+=GetSystemMetrics(SM_CXEDGE)*2;

		GetClientRect(ghmdiclient,&rect);
		GetWindowRect(ghmdiclient,&wrect);

		limit=(rect.bottom*2/3);
		if(limit<=0)
			limit=23*12;
		ypos=(count*ypos)%limit;

		{
			//int xv=GetSystemMetrics(SM_CXVSCROLL);
			//if(((wrect.right-wrect.left)-rect.right)<xv/2)
			//	w=rect.right-xv;
			//else
				w=rect.right;
		}
		h=rect.bottom-ypos;
		if(w<=0 || w<rect.right/2)
			w=rect.right/2;
		if(w<=16)
			flags|=SWP_NOSIZE;
		if(h<=0 || h<rect.bottom/2)
			h=rect.bottom/2;
		if(h<=16)
			flags|=SWP_NOSIZE;
		SetWindowPos(hwnd,NULL,0,ypos,w,h,flags);
	}
	return TRUE;
}
int save_mdi_size(HWND hwnd)
{
extern int save_mdi_win_size;
	if(save_mdi_win_size){
		WINDOWPLACEMENT wp;
		wp.length=sizeof(wp);
		if(GetWindowPlacement(hwnd,&wp)!=0){
			RECT rect={0};
			rect=wp.rcNormalPosition;
			write_ini_value("SETTINGS","mdi_width",rect.right-rect.left);
			write_ini_value("SETTINGS","mdi_height",rect.bottom-rect.top);
			write_ini_value("SETTINGS","mdi_maximized",wp.flags&WPF_RESTORETOMAXIMIZED?1:0);
			return TRUE;
		}
		return FALSE;
	}
	return TRUE;
}
int move_console(int x,int y)
{
	char title[MAX_PATH]={0};
	HWND hcon;
	GetConsoleTitle(title,sizeof(title));
	if(title[0]!=0){
		hcon=FindWindow(NULL,title);
		SetWindowPos(hcon,0,x,y,0,0,SWP_NOZORDER|SWP_NOSIZE);
	}
	return 0;
}
int get_max_console(int *w,int *h)
{
	int result=0;
	HANDLE hcon;
	hcon=GetStdHandle(STD_OUTPUT_HANDLE);
	if(hcon!=0){
		CONSOLE_SCREEN_BUFFER_INFO conbi={0};
		result=GetConsoleScreenBufferInfo(hcon,&conbi);
		if(w)
			*w=conbi.dwMaximumWindowSize.X;
		if(h)
			*h=conbi.dwMaximumWindowSize.Y;
	}
	return result;
}
int resize_console(int width,int height)
{
	int result=0;
	HANDLE hcon;
	hcon=GetStdHandle(STD_OUTPUT_HANDLE);
	if(hcon!=0){
		SMALL_RECT rect={0};
		CONSOLE_SCREEN_BUFFER_INFO conbi={0};
		GetConsoleScreenBufferInfo(hcon,&conbi);
		if(width>=conbi.dwSize.X || height>=conbi.dwSize.Y){
			if(width>=conbi.dwSize.X)
				conbi.dwSize.X=width+1;
			if(height>=conbi.dwSize.Y)
				conbi.dwSize.Y=height+1;
			SetConsoleScreenBufferSize(hcon,conbi.dwSize);
		}
		rect.Bottom=height;
		rect.Right=width;
		rect.Top=0;
		rect.Left=0;
		result=SetConsoleWindowInfo(hcon,TRUE,&rect);
	}
	return result;
}
void open_console()
{
	char title[MAX_PATH]={0};
	HWND hcon;
	FILE *hf;
	static BYTE consolecreated=FALSE;
	static int hcrt=0;

	if(consolecreated==TRUE)
	{
		GetConsoleTitle(title,sizeof(title));
		if(title[0]!=0){
			hcon=FindWindow(NULL,title);
			ShowWindow(hcon,SW_SHOW);
		}
		hcon=(HWND)GetStdHandle(STD_INPUT_HANDLE);
		FlushConsoleInputBuffer(hcon);
		return;
	}
	AllocConsole();
	hcrt=_open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE),_O_TEXT);

	fflush(stdin);
	hf=_fdopen(hcrt,"w");
	*stdout=*hf;
	setvbuf(stdout,NULL,_IONBF,0);
	GetConsoleTitle(title,sizeof(title));
	if(title[0]!=0){
		hcon=FindWindow(NULL,title);
		ShowWindow(hcon,SW_SHOW);
		SetForegroundWindow(hcon);
	}
	consolecreated=TRUE;
}
void hide_console()
{
	char title[MAX_PATH]={0};
	HANDLE hcon;

	GetConsoleTitle(title,sizeof(title));
	if(title[0]!=0){
		hcon=FindWindow(NULL,title);
		ShowWindow(hcon,SW_HIDE);
		SetForegroundWindow(hcon);
	}
}


int create_mdi_window(HWND hwnd,HINSTANCE hinstance,TABLE_WINDOW *win)
{
	HWND hedit,hlistview,hintel,hsplit_lock;
	if(win==0)
		return FALSE;

    hsplit_lock=CreateWindowEx(WS_EX_TOPMOST,"button","O", 
      WS_TABSTOP|WS_CHILD|WS_VISIBLE, //|BS_ICON,
        0, 0, 0, 0, hwnd, IDC_SPLIT_LOCK, hinstance, 0);
    hedit = CreateWindow(RICHEDIT_CLASS,// "RichEdit50W", //"RichEdit20A",
                                     "",
                                     WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|WS_HSCROLL|WS_VSCROLL|ES_AUTOHSCROLL|ES_AUTOVSCROLL|ES_MULTILINE|ES_WANTRETURN,
                                     0,0,
                                     0,0,
                                     hwnd,
                                     IDC_MDI_EDIT,
                                     hinstance,
                                     NULL);
    hlistview = CreateWindow(WC_LISTVIEW,
                                     "",
                                     WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|LVS_REPORT|LVS_OWNERDRAWFIXED	, //|LVS_OWNERDRAWFIXED, //|LVS_SHOWSELALWAYS,
                                     0,0,
                                     0,0,
                                     hwnd,
                                     IDC_MDI_LISTVIEW,
                                     hinstance,
                                     NULL);
	hintel = CreateWindowEx(WS_EX_CLIENTEDGE,"LISTBOX",
										 "",
									 WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|LBS_HASSTRINGS|LBS_SORT|LBS_STANDARD|LBS_WANTKEYBOARDINPUT,
									 0,0,
									 0,0,
									 hwnd,
									 IDC_INTELLISENSE,
									 ghinstance,
									 NULL);

	if(hsplit_lock){
		/*
		static HICON hicon=0;
		if(hicon==0)
			hicon=LoadImage(ghinstance,MAKEINTRESOURCE(IDI_LOCK),IMAGE_ICON,12,12,NULL);
		if(hicon)
			SendMessage(hsplit_lock,BM_SETIMAGE,(WPARAM)IMAGE_ICON,(LPARAM)hicon);
		*/
	}
	win->hlistview=hlistview;
	win->hedit=hedit;
	win->hintel=hintel;
	win->hlock=hsplit_lock;
	if(hlistview!=0){
		ListView_SetExtendedListViewStyle(hlistview,ListView_GetExtendedListViewStyle(hlistview)|LVS_EX_FULLROWSELECT);
		subclass_listview(hlistview);
	}
	if(hedit!=0){
		SendMessage(hedit,EM_EXLIMITTEXT,0,0x200000);
		subclass_edit(hedit);
	}
	return TRUE;
}


int setup_mdi_classes(HINSTANCE hinstance)
{
	int result=TRUE;
    WNDCLASS wc;
	memset(&wc,0,sizeof(wc));
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = MDIChildWndProc;
    wc.hInstance     = hinstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
    wc.lpszClassName = "tablewindow";

    if(!RegisterClass(&wc))
		result=FALSE;
	return result;
}
int create_mdiclient(HWND hwnd,HMENU hmenu,HINSTANCE hinstance)
{
	CLIENTCREATESTRUCT MDIClientCreateStruct;
	HWND hmdiclient;
	MDIClientCreateStruct.hWindowMenu   = GetSubMenu(hmenu,5);
	MDIClientCreateStruct.idFirstChild  = 50000;
	hmdiclient = CreateWindow("MDICLIENT",NULL,
		WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VSCROLL|WS_HSCROLL|WS_VISIBLE, //0x56300001
		0,0,0,0,
		hwnd,
		IDC_MDI_CLIENT,//ID
		hinstance,
		(LPVOID)&MDIClientCreateStruct);
	return hmdiclient;
}

int create_mainwindow(void *wndproc,HMENU hmenu,HINSTANCE hinstance,char *class_name,char *title)
{
	WNDCLASS wndclass;
	HWND hframe=0;
	memset(&wndclass,0,sizeof(wndclass));

	wndclass.style=0; //CS_HREDRAW|CS_VREDRAW;
	wndclass.lpfnWndProc=wndproc;
	wndclass.hCursor=LoadCursor(NULL, IDC_ARROW);
	wndclass.hInstance=hinstance;
	wndclass.hbrBackground=COLOR_BTNFACE+1;
	wndclass.lpszClassName=class_name;

	if(RegisterClass(&wndclass)!=0){
		hframe = CreateWindowEx(WS_EX_ACCEPTFILES,class_name,title,
			WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_OVERLAPPEDWINDOW, //0x6CF0000
			0,0,
			400,300,
			NULL,           // handle to parent window
			hmenu,
			hinstance,
			NULL);
	}
	return hframe;
}
int create_dbview(HWND hwnd,HINSTANCE hinstance)
{
	WNDCLASS wndclass;
	HWND hswitch=0;
	memset(&wndclass,0,sizeof(wndclass));
	wndclass.lpfnWndProc=dbview_proc;
	wndclass.hCursor=LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground=COLOR_BTNFACE+1;
	wndclass.lpszClassName="dbview";
	wndclass.style=CS_HREDRAW|CS_VREDRAW;
	if(RegisterClass(&wndclass)!=0){
		hswitch=CreateWindow("dbview","dbview_window",
			WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VISIBLE|WS_BORDER,
			0,0,
			0,0,
			hwnd,
			IDC_TREEVIEW,
			hinstance,
			NULL);
	}
	return hswitch;
}

int create_table_window(HWND hmdiclient,TABLE_WINDOW *win)
{
	int style,handle;
	MDICREATESTRUCT cs;
	char title[256]={0};
	style = MDIS_ALLCHILDSTYLES;
	cs.cx=cs.cy=cs.x=cs.y=CW_USEDEFAULT;
	cs.szClass="tablewindow";
	cs.szTitle=title;
	cs.style=style;
	cs.hOwner=ghinstance;
	cs.lParam=win;
	handle=SendMessage(hmdiclient,WM_MDICREATE,0,&cs);
	return handle;
}
int get_max_table_windows()
{
	return sizeof(table_windows)/sizeof(TABLE_WINDOW);
}
int get_win_hwnds(int i,HWND *hwnd,HWND *hedit,HWND *hlistview)
{
	int result=FALSE;
	if(i>sizeof(table_windows)/sizeof(TABLE_WINDOW))
		return FALSE;
	if(hwnd!=0 && table_windows[i].hwnd!=0){
		*hwnd=table_windows[i].hwnd;
		result=TRUE;
	}
	if(hedit!=0 && table_windows[i].hedit!=0){
		*hedit=table_windows[i].hedit;
		result=TRUE;
	}
	if(hlistview!=0 && table_windows[i].hlistview!=0){
		*hlistview=table_windows[i].hlistview;
		result=TRUE;
	}
	return result;
}
int find_win_by_hwnd(HWND hwnd,TABLE_WINDOW **win)
{
	int i;
	static TABLE_WINDOW *w=0;
	if(w!=0 && w->hwnd==hwnd){
		*win=w;
		return TRUE;
	}
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(table_windows[i].hwnd==hwnd){
			*win=&table_windows[i];
			w=&table_windows[i];
			return TRUE;
		}
	}
	return FALSE;
}
int find_win_by_hedit(HWND hedit,TABLE_WINDOW **win)
{
	int i;
	static TABLE_WINDOW *w=0;
	if(w!=0 && w->hedit==hedit){
		*win=w;
		return TRUE;
	}
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(table_windows[i].hedit==hedit){
			*win=&table_windows[i];
			w=&table_windows[i];
			return TRUE;
		}
	}
	return FALSE;
}
int free_window(TABLE_WINDOW *win)
{
	if(win!=0){
		if(win->col_attr!=0)
			free(win->col_attr);
		memset(win,0,sizeof(TABLE_WINDOW));
	}
	return TRUE;
}
int acquire_table_window(TABLE_WINDOW **win,char *tname)
{
	int i;
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(table_windows[i].hwnd==0){
			*win=&table_windows[i];
			if(tname!=0)
				strncpy(table_windows[i].table,tname,sizeof(table_windows[i].table));
			else
				table_windows[i].table[0]=0;
			return TRUE;
		}
	}
	return FALSE;
}

int find_db_tree(char *name,DB_TREE **tree)
{
	int i;
	HANDLE hroot=0;
	if(name==0 || tree==0)
		return FALSE;
	if(!tree_get_root(name,&hroot)){
		char alt_name[512]={0};
		extract_short_db_name(name,alt_name,sizeof(alt_name));
		if(!tree_get_root(alt_name,&hroot))
			return FALSE;
	}
	if(hroot==0)
		return FALSE;
	for(i=0;i<sizeof(db_tree)/sizeof(DB_TREE);i++){
		if(db_tree[i].hroot==hroot){
			*tree=&db_tree[i];
			return TRUE;
		}
	}
	return FALSE;
}
int find_selected_tree(DB_TREE **tree)
{
	int i;
	HANDLE hroot=0;
	tree_find_focused_root(&hroot);
	for(i=0;i<sizeof(db_tree)/sizeof(DB_TREE);i++){
		if(hroot!=0){
			if(db_tree[i].hroot==hroot){
				*tree=&db_tree[i];
				return TRUE;
			}
		}
		else if(db_tree[i].hroot!=0){
			*tree=&db_tree[i];
			return TRUE;
		}
	}
	return FALSE;
}
int copy_param(const char *str,const char *search,char *out,int olen)
{
	int found=FALSE;
	const char *s;
	s=strstri(str,search);
	if(s!=0){
		int i,index,len=strlen(s);
		index=0;
		for(i=0;i<len;i++){
			if(index >= olen-1)
				break;
			if(s[i]==';')
				break;
			out[index++]=s[i];
		}
		found=TRUE;
		out[index]=0;
	}
	return found;
}
//get a tree list friendly name of the connect string
int extract_short_db_name(char *name,char *out,int olen)
{
	int i,found=FALSE;
	const char *params[]={"SourceDB=","DatabaseFile=","DSN=","DBQ=","Driver=","UID=","PWD="};
	if(name==0 || out==0 || olen<=0)
		return FALSE;
	out[0]=0;
	for(i=0;i<sizeof(params)/sizeof(char *);i++){
		char tmp[512]={0};
		if(copy_param(name,params[i],tmp,sizeof(tmp))){
			char *semi=";";
			if(!found)
				semi="";
			_snprintf(out,olen,"%s%s%s",out,semi,tmp);
			found=TRUE;
		}
	}
	if(!found){
		strncpy(out,name,olen);
		found=TRUE;
	}
	out[olen-1]=0;
	return found;
}
int acquire_db_tree(char *name,DB_TREE **tree)
{
	int i;
	if(tree==0 || name==0)
		return FALSE;
	if(find_db_tree(name,tree))
		return TRUE;
	for(i=0;i<sizeof(db_tree)/sizeof(DB_TREE);i++){
		if(db_tree[i].hroot==0){
			char node_name[512]={0};
			strncpy(db_tree[i].name,name,sizeof(db_tree[i].name));
			//give the node name something easier to read
			//may cause problems elsewhere when searching by node name
			extract_short_db_name(name,node_name,sizeof(node_name));
			db_tree[i].hroot=insert_root(node_name,IDC_DB_ITEM);
			//db_tree[i].hroot=insert_root(name,IDC_DB_ITEM);
			db_tree[i].htree=ghtreeview;
			*tree=&db_tree[i];
			return TRUE;
		}
	}
	return FALSE;
}
int	acquire_db_tree_from_win(TABLE_WINDOW *win,DB_TREE **tree)
{
	return acquire_db_tree(win->name,tree);
}

int refresh_tables(DB_TREE *tree,int all)
{
	int result=FALSE;
	if(tree!=0 && tree->hroot!=0){
		tree_delete_all_child(tree->hroot);
		set_status_bar_text(ghstatusbar,0,"retrieving %s tables:%s",all?"all":"",tree->name);
		get_tables(tree,all);
		expand_root(tree->hroot);
		result=TRUE;
	}
	return result;
}
int load_tables_if_empty(DB_TREE *tree)
{
	int result=FALSE;
	if(tree!=0 && tree->hroot!=0){
		HTREEITEM h;
		h=TreeView_GetChild(tree->htree,tree->hroot);
		if(h==0)
			result=refresh_tables(tree,FALSE);
	}
	return result;
}
int mdi_open_db(DB_TREE *tree)
{
	int result=FALSE;
	if(tree!=0){
		if(open_db(tree)){
			char str[1024]={0};
			tree_get_item_text(tree->hroot,str,sizeof(str));
			if(str[0]==0)
				tree_set_item_text(tree->hroot,tree->connect_str);
			result=TRUE;
		}
	}
	return result;
}
int mdi_remove_db(DB_TREE *tree)
{
	if(tree!=0){
		if(close_db(tree)){
			if(tree->hroot!=0){
				int i;
				TreeView_DeleteItem(ghtreeview,tree->hroot);
				for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
					if(table_windows[i].hroot==tree->hroot)
						table_windows[i].hroot=0;
				}
			}
			memset(tree,0,sizeof(DB_TREE));
			return TRUE;
		}
	}
	return FALSE;
}

int mdi_clear_listview(TABLE_WINDOW *win)
{
	if(win!=0 && win->hlistview!=0){
		int i;
		ListView_DeleteAllItems(win->hlistview);
		for(i=0;i<win->columns;i++)
			ListView_DeleteColumn(win->hlistview,0);
		return TRUE;
	}
	else
		return FALSE;
}

int mdi_get_current_win(TABLE_WINDOW **win)
{
	HWND hwnd;
	hwnd=SendMessage(ghmdiclient,WM_MDIGETACTIVE,0,0);
	if(hwnd!=0){
		return find_win_by_hwnd(hwnd,win);
	}
	return FALSE;
}

int mdi_create_abort(TABLE_WINDOW *win)
{
	if(win==0 || win->hwnd==0)
		return FALSE;
	if(win->hintel!=0)
		ShowWindow(win->hintel,SW_HIDE);
	//PostMessage(ghmdiclient,WM_USER,win,MAKELPARAM(TRUE,IDC_SQL_ABORT));
	return PostMessage(win->hwnd,WM_USER,win,MAKELPARAM(IDC_SQL_ABORT,TRUE));
}
int mdi_destroy_abort(TABLE_WINDOW *win)
{
	if(win==0 || win->hwnd==0)
		return FALSE;
//	PostMessage(ghmdiclient,WM_USER,win,MAKELPARAM(IDC_SQL_ABORT,FALSE));
	return PostMessage(win->hwnd,WM_USER,win,MAKELPARAM(IDC_SQL_ABORT,FALSE));
}
int mdi_set_edit_text(TABLE_WINDOW *win,char *str)
{
	if(win!=0 && win->hedit!=0)
		SetWindowText(win->hedit,str);
	return TRUE;
}
int mdi_get_edit_text(TABLE_WINDOW *win,char **str,int *size)
{
	int result=FALSE;
	if(win!=0 && win->hedit!=0 && str!=0 && size!=0){
		GETTEXTLENGTHEX tl={GTL_CLOSE|GTL_NUMBYTES,CP_ACP};
		int len;
		len=SendMessage(win->hedit,EM_GETTEXTLENGTHEX,&tl,0);
		if(len<0)
			len=0x10000;
		else if(len<=0x10000)
			len=0x10000;
		else
			len=0x400000;
		*size=len;
		*str=malloc(len);
		if(*str!=0){
			CHARRANGE range={0};
			GetWindowText(win->hedit,*str,*size);
			SendMessage(win->hedit,EM_EXGETSEL,0,&range);
			if(range.cpMax!=range.cpMin){
				int len;
				if(range.cpMin>range.cpMax)
					len=range.cpMin-range.cpMax;
				else
					len=range.cpMax-range.cpMin;
				if(len>2 && len<*size){
					SendMessage(win->hedit,EM_GETSELTEXT,0,*str);
				}
			}
			result=TRUE;
		}
	}
	return result;
}
int mdi_tile_windows_horo()
{
	RECT rect={0};
	int i,side=0;
	int x,y=0;
	int caption_height=0;
	
	caption_height=GetSystemMetrics(SM_CYCAPTION);
	caption_height+=GetSystemMetrics(SM_CXEDGE)*2;

	GetClientRect(ghmdiclient,&rect);
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		TABLE_WINDOW *win=&table_windows[i];
		if(win->hwnd!=0){
			int flags=0;
			int width,height;
			if(side)
				x=rect.right/2;
			else
				x=0;
			width=rect.right/2;
			height=rect.bottom-y;
			if(width<=0 || height<=0)
				flags|=SWP_NOSIZE;
			SetWindowPos(win->hwnd,NULL,x,y,width,height,flags);
			if(side)
				y+=caption_height;
			if(y > (rect.bottom*2/3))
				y=0;
			side^=1;
		}
	}
	return TRUE;
}
int mdi_tile_windows_vert()
{
	int i,count=0;
	int caption_height,y,width,height;
	RECT rect={0};
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		TABLE_WINDOW *win=&table_windows[i];
		if(win->hwnd!=0)
			count++;
	}
	caption_height=GetSystemMetrics(SM_CYCAPTION);
	caption_height+=GetSystemMetrics(SM_CXEDGE)*2;
	GetClientRect(ghmdiclient,&rect);
	if(count<=0)
		return FALSE;
	height=rect.bottom/count;
	if(height<caption_height)
		height=caption_height;
	width=rect.right;
	y=0;
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		TABLE_WINDOW *win=&table_windows[i];
		if(win->hwnd!=0){
			int flags=0;
			if(width<=0 || height<=0)
				flags=SWP_NOSIZE;
			SetWindowPos(win->hwnd,NULL,0,y,width,height,flags);
			y+=height;
		}
	}
	return TRUE;
}
int mdi_cascade_win_vert()
{
	int i,caption_height,y,width,height;
	RECT rect={0};
	caption_height=GetSystemMetrics(SM_CYCAPTION);
	caption_height+=GetSystemMetrics(SM_CXEDGE)*2;
	if(caption_height==0)
		caption_height=19+4;
	GetClientRect(ghmdiclient,&rect);
	height=rect.bottom;
	width=rect.right;
	y=0;
	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		TABLE_WINDOW *win=&table_windows[i];
		if(win->hwnd!=0){
			int flags=0;
			if(width<=0 || height<=0)
				flags=SWP_NOSIZE;
			SetWindowPos(win->hwnd,NULL,0,y,width,height,flags);
			height-=caption_height;
			if(height<(rect.bottom/3))
				height=rect.bottom/3;
			y+=caption_height;
		}
	}
	return TRUE;
}
int create_abort(TABLE_WINDOW *win)
{
	if(win==0 || win->hwnd==0 || win->habort!=0)
		return FALSE;
    win->habort = CreateWindow("BUTTON",
                                     "abort",
                                     WS_TABSTOP|WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE|BS_TEXT,
                                     0,0,
                                     80,20,
                                     win->hwnd,
                                     IDC_SQL_ABORT,
                                     ghinstance,
                                     NULL);
	if(win->habort!=0){
		SetWindowPos(win->habort,HWND_TOP,0,0,80,20,SWP_SHOWWINDOW);
		SetWindowText(win->habort,"Abort");
		SetFocus(win->habort);
		win->abort=FALSE;
	}
	return win->habort!=0;
}

int destroy_abort(TABLE_WINDOW *win)
{
	int result=FALSE;
	if(win==0 || win->habort==0)
		return FALSE;
	win->abort=TRUE;
	result=SendMessage(win->habort,WM_CLOSE,0,0);
	win->habort=0;
	return result;
}

int set_focus_after_result(TABLE_WINDOW *win,int result)
{
	if(win!=0){
		if(result==FALSE)
			PostMessage(win->hwnd,WM_USER,win->hedit,IDC_MDI_CLIENT);
		else if(win->rows>0)
			PostMessage(win->hwnd,WM_USER,win->hlistview,IDC_MDI_CLIENT);
		else
			PostMessage(win->hwnd,WM_USER,win->hedit,IDC_MDI_CLIENT);
	}
	return TRUE;
}
int set_focus_after_open(DB_TREE *tree)
{
	if(tree!=0 && tree->htree!=0 && tree->hroot!=0){
		PostMessage(ghdbview,WM_USER,IDC_TREEVIEW,tree->htree);
		return TRUE;
	}
	return FALSE;
}

int custom_dispatch(MSG *msg)
{
	TABLE_WINDOW *win=0;
	HWND hwnd=0;
	int i,type=0;

	hwnd=WindowFromPoint(msg->pt);

	for(i=0;i<sizeof(table_windows)/sizeof(TABLE_WINDOW);i++){
		if(msg->message==WM_MOUSEWHEEL && hwnd!=0){
			if(table_windows[i].hlistview==hwnd){
				win=&table_windows[i];
				type=IDC_MDI_LISTVIEW;
				break;
			}
			else if(table_windows[i].hedit==hwnd){
				win=&table_windows[i];
				type=IDC_MDI_EDIT;
				break;
			}
			else if(table_windows[i].hwnd==hwnd){
				win=&table_windows[i];
				type=IDC_MDI_CLIENT;
				break;
			}
		}
		else if(table_windows[i].hlistview==msg->hwnd){
			win=&table_windows[i];
			type=IDC_MDI_LISTVIEW;
			break;
		}
		else if(table_windows[i].hedit==msg->hwnd){
			win=&table_windows[i];
			type=IDC_MDI_EDIT;
			break;
		}
		else if(table_windows[i].hlock==msg->hwnd){
			win=&table_windows[i];
			type=IDC_SPLIT_LOCK;
			break;
		}
		else if(table_windows[i].hwnd==msg->hwnd){
			win=&table_windows[i];
			type=IDC_MDI_CLIENT;
			break;
		}
		else if(table_windows[i].habort==msg->hwnd){
			win=&table_windows[i];
			type=IDC_SQL_ABORT;
			break;
		}
	}
	if(win!=0){
		switch(msg->message){
		case WM_MOUSEWHEEL:
			switch(type){
			case IDC_MDI_LISTVIEW:
				msg->hwnd=win->hlistview;
				DispatchMessage(msg);
				return TRUE;
			case IDC_MDI_EDIT:
				msg->hwnd=win->hedit;
				DispatchMessage(msg);
				return TRUE;
			case IDC_MDI_CLIENT:
				msg->hwnd=win->hlistview;
				DispatchMessage(msg);
				return TRUE;
			}
			break;
		case WM_CHAR:
			switch(msg->wParam){
			case VK_ESCAPE:
				if(win->habort!=0 && win->hwnd!=0 && type!=0)
					mdi_destroy_abort(win);
				break;
			}
			break;
		case WM_KEYFIRST:
			switch(msg->wParam){
			case VK_TAB:
				if(type==IDC_MDI_LISTVIEW || type==IDC_MDI_CLIENT){
					if(!(GetKeyState(VK_CONTROL)&0x8000 || GetKeyState(VK_SHIFT)&0x8000))
						SetFocus(win->hedit);
				}
				else if(type==IDC_MDI_EDIT){
					if(GetKeyState(VK_CONTROL)&0x8000 && GetKeyState(VK_SHIFT)&0x8000){
						SetFocus(win->hlistview);
						return TRUE;
					}
				}
				else if(type==IDC_SPLIT_LOCK){
					SetFocus(win->hedit);
					return TRUE;
				}
				else if(type!=0){
					SetFocus(win->hlistview);
					return TRUE;
				}

				break;
			case VK_F4:
			case VK_UP:
			case VK_DOWN:
				if(type==IDC_MDI_EDIT){
					int post=FALSE;
					if(GetKeyState(VK_CONTROL)&0x8000){
						if(GetKeyState(VK_SHIFT)&0x8000){
							post=TRUE;
						}
					}
					if(msg->wParam==VK_F4)
						post=TRUE;
					if(post){
						PostMessage(win->hwnd,WM_USER,msg->wParam,IDC_SPLIT_LOCK);
						return TRUE;
					}
				}
				break;
			case 'L':
				if(GetKeyState(VK_CONTROL)&0x8000){
					if(GetKeyState(VK_SHIFT)&0x8000){
						SetFocus(win->hlock);
						return TRUE;
					}
					else if(ghtreeview!=0)
						SetFocus(ghtreeview);
				}
				break;
			case 'W':
				if(GetKeyState(VK_CONTROL)&0x8000)
					if(win->hwnd!=0)
						PostMessage(win->hwnd,WM_CLOSE,0,0);
			}
			break;
		}
	}
	switch(msg->message){
	case WM_KEYFIRST:
		switch(msg->wParam){
		case VK_F12:
			if(GetKeyState(VK_CONTROL)&0x8000){
				extern int automation_thread();
				extern int automation_busy;
				if(!automation_busy)
					_beginthread(automation_thread,0,0);
			}
		}
	}
	return FALSE;
}
int create_popup_menus()
{
	create_treeview_menus();
	create_lv_menus();
	return 0;
}
int init_mdi_stuff()
{
	extern int show_joins,lua_script_enable;
	memset(&table_windows,0,sizeof(table_windows));
	memset(&db_tree,0,sizeof(db_tree));
	create_popup_menus();
	return TRUE;
}

#include "DB_stuff.h"

