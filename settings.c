#include <windows.h>
#include "resource.h"
#define INI_SETTINGS "SETTINGS"
int add_fonts(HWND hwnd,int ctrl)
{
	char tmp[80];
	_snprintf(tmp,sizeof(tmp),"%s","OEM_FIXED_FONT");
	SendDlgItemMessage(hwnd,ctrl,CB_ADDSTRING,0,tmp);
	_snprintf(tmp,sizeof(tmp),"%s","ANSI_FIXED_FONT");
	SendDlgItemMessage(hwnd,ctrl,CB_ADDSTRING,0,tmp);
	_snprintf(tmp,sizeof(tmp),"%s","ANSI_VAR_FONT");
	SendDlgItemMessage(hwnd,ctrl,CB_ADDSTRING,0,tmp);
	_snprintf(tmp,sizeof(tmp),"%s","SYSTEM_FONT");
	SendDlgItemMessage(hwnd,ctrl,CB_ADDSTRING,0,tmp);
	_snprintf(tmp,sizeof(tmp),"%s","DEVICE_DEFAULT_FONT");
	SendDlgItemMessage(hwnd,ctrl,CB_ADDSTRING,0,tmp);
	_snprintf(tmp,sizeof(tmp),"%s","SYSTEM_FIXED_FONT");
	SendDlgItemMessage(hwnd,ctrl,CB_ADDSTRING,0,tmp);
	_snprintf(tmp,sizeof(tmp),"%s","DEFAULT_GUI_FONT");
	SendDlgItemMessage(hwnd,ctrl,CB_ADDSTRING,0,tmp);
	return TRUE;
}
int int_to_fontname(int font,char *name,int len)
{
	switch(font){
	case OEM_FIXED_FONT:_snprintf(name,len,"OEM_FIXED_FONT");break;
	case ANSI_FIXED_FONT:_snprintf(name,len,"ANSI_FIXED_FONT");break;
	case ANSI_VAR_FONT:_snprintf(name,len,"ANSI_VAR_FONT");break;
	default:
	case SYSTEM_FONT:_snprintf(name,len,"SYSTEM_FONT");break;
	case DEVICE_DEFAULT_FONT:_snprintf(name,len,"DEVICE_DEFAULT_FONT");break;
	case SYSTEM_FIXED_FONT:_snprintf(name,len,"SYSTEM_FIXED_FONT");break;
	case DEFAULT_GUI_FONT:_snprintf(name,len,"DEFAULT_GUI_FONT");break;
	}
	name[len-1]=0;
	return TRUE;
}
int fontname_to_int(char *name)
{
	if(strnicmp(name,"OEM_FIXED_FONT",sizeof("OEM_FIXED_FONT")-1)==0)
		return OEM_FIXED_FONT;
	if(strnicmp(name,"ANSI_FIXED_FONT",sizeof("ANSI_FIXED_FONT")-1)==0)
		return ANSI_FIXED_FONT;
	if(strnicmp(name,"ANSI_VAR_FONT",sizeof("ANSI_VAR_FONT")-1)==0)
		return ANSI_VAR_FONT;
	if(strnicmp(name,"SYSTEM_FONT",sizeof("SYSTEM_FONT")-1)==0)
		return SYSTEM_FONT;
	if(strnicmp(name,"DEVICE_DEFAULT_FONT",sizeof("DEVICE_DEFAULT_FONT")-1)==0)
		return DEVICE_DEFAULT_FONT;
	if(strnicmp(name,"SYSTEM_FIXED_FONT",sizeof("SYSTEM_FIXED_FONT")-1)==0)
		return SYSTEM_FIXED_FONT;
	if(strnicmp(name,"DEFAULT_GUI_FONT",sizeof("DEFAULT_GUI_FONT")-1)==0)
		return DEFAULT_GUI_FONT;
	return DEFAULT_GUI_FONT;
}
int get_control_name(int ctrl,char *name,int len){
	char *s=0;
	switch(ctrl){
	default:
	case IDC_SQL_FONT:
		s="sql_font";
		break;
	case IDC_LISTVIEW_FONT:
		s="list_view_font";
		break;
	case IDC_TREEVIEW_FONT:
		s="tree_view_font";
		break;
	}
	_snprintf(name,len,"%s",s);
	name[len-1]=0;
	return TRUE;
}
int get_font_setting(int ctrl)
{
	char tmp[80]={0};
	char key[80]={0};
	get_control_name(ctrl,key,sizeof(key));
	get_ini_str(INI_SETTINGS,key,tmp,sizeof(tmp));
	return fontname_to_int(tmp);
}
int select_current_font(HWND hwnd,int ctrl)
{
	char tmp[80];
	char key[80];
	int index;
	key[0]=0;
	get_control_name(ctrl,key,sizeof(key));
	tmp[0]=0;
	get_ini_str(INI_SETTINGS,key,tmp,sizeof(tmp));
	index=SendDlgItemMessage(hwnd,ctrl,CB_FINDSTRINGEXACT,-1,tmp);
	if(index<0)index=0;
	SendDlgItemMessage(hwnd,ctrl,CB_SETCURSEL,index,0);
	return TRUE;
}
int set_single_instance(int set)
{
	static char *instname="DB_UTIL Instance";
	static HANDLE hmutex=0;
	if(set){
		SetLastError(NO_ERROR);
		if(hmutex==0)
			hmutex=CreateMutex(NULL,FALSE,instname);
		if(GetLastError()==ERROR_ALREADY_EXISTS)
			return FALSE;
		else
			return TRUE;
	}
	else{
		if(hmutex!=0){
			if(WaitForSingleObject(hmutex,0)==WAIT_OBJECT_0)
				if(ReleaseMutex(hmutex)!=0)
					if(CloseHandle(hmutex)!=0){
						hmutex=0;
						return TRUE;
					}
		}
	}
	return FALSE;
}
LRESULT CALLBACK settings_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static HWND grippy=0;
	switch(msg){
	case WM_INITDIALOG:
		{
			extern int keep_closed;
			int val;
			get_ini_value(INI_SETTINGS,"KEEP_CLOSED",&keep_closed);
			if(!keep_closed)
				CheckDlgButton(hwnd,IDC_KEEP_CONNECTED,BST_CHECKED);
			val=0;
			get_ini_value(INI_SETTINGS,"SINGLE_INSTANCE",&val);
			if(val)
				CheckDlgButton(hwnd,IDC_SINGLE_INSTANCE,BST_CHECKED);
			val=0;
			get_ini_value(INI_SETTINGS,"DEBUG",&val);
			if(val)
				CheckDlgButton(hwnd,IDC_DEBUG,BST_CHECKED);

			add_fonts(hwnd,IDC_SQL_FONT);
			add_fonts(hwnd,IDC_LISTVIEW_FONT);
			add_fonts(hwnd,IDC_TREEVIEW_FONT);
			select_current_font(hwnd,IDC_SQL_FONT);
			select_current_font(hwnd,IDC_LISTVIEW_FONT);
			select_current_font(hwnd,IDC_TREEVIEW_FONT);
		}
		grippy=create_grippy(hwnd);
		break;
	case WM_SIZE:
		grippy_move(hwnd,grippy);
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDC_OPEN_INI:
			open_ini(hwnd,FALSE);
			break;
		case IDC_SQL_FONT:
			if(HIWORD(wparam)==CBN_SELENDOK){
				int i,max=get_max_table_windows();
				int font;
				char str[80]={0};
				GetDlgItemText(hwnd,IDC_SQL_FONT,str,sizeof(str));
				font=fontname_to_int(str);
				for(i=0;i<max;i++){
					HWND hedit=0;
					if(get_win_hwnds(i,NULL,&hedit,NULL)){
						SendMessage(hedit,WM_SETFONT,GetStockObject(font),0);
						InvalidateRect(hedit,NULL,TRUE);
					}
				}
			}
			break;
		case IDC_LISTVIEW_FONT:
			if(HIWORD(wparam)==CBN_SELENDOK){
				int i,max=get_max_table_windows();
				int font;
				char str[80]={0};
				GetDlgItemText(hwnd,IDC_LISTVIEW_FONT,str,sizeof(str));
				font=fontname_to_int(str);
				for(i=0;i<max;i++){
					HWND hlistview;
					if(get_win_hwnds(i,NULL,NULL,&hlistview)){
						SendMessage(hlistview,WM_SETFONT,GetStockObject(font),0);
						InvalidateRect(hlistview,NULL,TRUE);
					}
				}
			}
			break;
		case IDC_TREEVIEW_FONT:
			if(HIWORD(wparam)==CBN_SELENDOK){
				extern HWND ghtreeview;
				int font;
				char str[80]={0};
				GetDlgItemText(hwnd,IDC_TREEVIEW_FONT,str,sizeof(str));
				font=fontname_to_int(str);
				if(ghtreeview!=0){
					SendMessage(ghtreeview,WM_SETFONT,GetStockObject(font),0);
					InvalidateRect(ghtreeview,NULL,TRUE);
				}
			}
			break;
		case IDOK:
			{
			extern int keep_closed;
			char key[80],str[80];
			int i,val,ctrls[3]={IDC_SQL_FONT,IDC_LISTVIEW_FONT,IDC_TREEVIEW_FONT};
			if(IsDlgButtonChecked(hwnd,IDC_KEEP_CONNECTED)==BST_CHECKED)
				keep_closed=FALSE;
			else
				keep_closed=TRUE;
			
			val=0;
			if(IsDlgButtonChecked(hwnd,IDC_KEEP_CONNECTED)==BST_CHECKED)
				val=1;
			write_ini_value(INI_SETTINGS,"SINGLE_INSTANCE",val);
			
			val=0;
			if(IsDlgButtonChecked(hwnd,IDC_SINGLE_INSTANCE)==BST_CHECKED)
				val=1;
			write_ini_value(INI_SETTINGS,"SINGLE_INSTANCE",val);
			set_single_instance(val);

			val=0;
			if(IsDlgButtonChecked(hwnd,IDC_DEBUG)==BST_CHECKED)
				val=1;
			write_ini_value(INI_SETTINGS,"DEBUG",val);

			write_ini_value(INI_SETTINGS,"KEEP_CLOSED",keep_closed);
			for(i=0;i<sizeof(ctrls)/sizeof(int);i++){
				key[0]=0;
				get_control_name(ctrls[i],key,sizeof(key));
				str[0]=0;
				GetDlgItemText(hwnd,ctrls[i],str,sizeof(str));
				write_ini_str(INI_SETTINGS,key,str);
			}
			}
		case IDCANCEL:
			EndDialog(hwnd,0);
			break;
		}
		break;
	}
	return 0;
}