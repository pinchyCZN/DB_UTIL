#include <windows.h>
#include "resource.h"
#define INI_SETTINGS "SETTINGS"

extern LRESULT CALLBACK file_assoc_proc(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam);
extern HINSTANCE ghinstance;

int trim_trailing=0;
int left_justify=0;

struct FONT_NAME{
	int font_num;
	char *font_name;
};
struct FONT_NAME font_names[7]={
	{OEM_FIXED_FONT,"OEM_FIXED_FONT"},
	{ANSI_FIXED_FONT,"ANSI_FIXED_FONT"},
	{ANSI_VAR_FONT,"ANSI_VAR_FONT"},
	{SYSTEM_FONT,"SYSTEM_FONT"},
	{DEVICE_DEFAULT_FONT,"DEVICE_DEFAULT_FONT"},
	{SYSTEM_FIXED_FONT,"SYSTEM_FIXED_FONT"},
	{DEFAULT_GUI_FONT,"DEFAULT_GUI_FONT"}
};

int add_fonts(HWND hwnd,int ctrl)
{
	int i;
	for(i=0;i<sizeof(font_names)/sizeof(struct FONT_NAME);i++)
		SendDlgItemMessage(hwnd,ctrl,CB_ADDSTRING,0,font_names[i].font_name);
	return TRUE;
}
int int_to_fontname(int font,char *name,int len)
{
	int i,result=FALSE;
	for(i=0;i<sizeof(font_names)/sizeof(struct FONT_NAME);i++){
		if(font==font_names[i].font_num){
			_snprintf(name,len,font_names[i].font_name);
			result=TRUE;
			break;
		}
	}
	if(!result){
		_snprintf(name,len,"SYSTEM_FONT");
		result=TRUE;
	}
	name[len-1]=0;
	return result;
}
int fontname_to_int(char *name)
{
	int i;
	for(i=0;i<sizeof(font_names)/sizeof(struct FONT_NAME);i++){
		if(stricmp(name,font_names[i].font_name)==0){
			return font_names[i].font_num;
		}
	}
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
	static int font_changed=FALSE;
	static int justify_changed=FALSE;
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

			get_ini_value(INI_SETTINGS,"TRIM_TRAILING",&trim_trailing);
			if(trim_trailing)
				CheckDlgButton(hwnd,IDC_TRIM_TRAILING,BST_CHECKED);

			get_ini_value(INI_SETTINGS,"LEFT_JUSTIFY",&left_justify);
			if(left_justify)
				CheckDlgButton(hwnd,IDC_LEFT_JUSTIFY,BST_CHECKED);

			add_fonts(hwnd,IDC_SQL_FONT);
			add_fonts(hwnd,IDC_LISTVIEW_FONT);
			add_fonts(hwnd,IDC_TREEVIEW_FONT);
			select_current_font(hwnd,IDC_SQL_FONT);
			select_current_font(hwnd,IDC_LISTVIEW_FONT);
			select_current_font(hwnd,IDC_TREEVIEW_FONT);
			font_changed=FALSE;
			justify_changed=FALSE;
		}
		grippy=create_grippy(hwnd);
		break;
	case WM_SIZE:
		grippy_move(hwnd,grippy);
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDC_DEBUG:
			printf("debug %08X %08X\n",wparam,lparam);
			if(IsDlgButtonChecked(hwnd,IDC_DEBUG)==BST_CHECKED)
				open_console();
			else
				hide_console();
			break;
		case IDC_FILE_ASSOCIATIONS:
			DialogBoxParam(ghinstance,IDD_FILE_ASSOCIATIONS,hwnd,file_assoc_proc,0);
			break;
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
				font_changed=TRUE;
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
				font_changed=TRUE;
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
				font_changed=TRUE;
			}
			break;
		case IDC_LEFT_JUSTIFY:
			if(HIWORD(wparam)==BN_CLICKED){
				extern int left_justify;
				int i,max=get_max_table_windows();
				if(IsDlgButtonChecked(hwnd,IDC_LEFT_JUSTIFY)==BST_CHECKED)
					left_justify=1;
				else
					left_justify=0;
				for(i=0;i<max;i++){
					HWND hlistview;
					if(get_win_hwnds(i,NULL,NULL,&hlistview))
						InvalidateRect(hlistview,NULL,TRUE);
				}
				justify_changed=TRUE;
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
			
			write_ini_value(INI_SETTINGS,"KEEP_CLOSED",keep_closed);

			val=0;
			if(IsDlgButtonChecked(hwnd,IDC_SINGLE_INSTANCE)==BST_CHECKED)
				val=1;
			write_ini_value(INI_SETTINGS,"SINGLE_INSTANCE",val);
			set_single_instance(val);

			val=0;
			if(IsDlgButtonChecked(hwnd,IDC_DEBUG)==BST_CHECKED)
				val=1;
			write_ini_value(INI_SETTINGS,"DEBUG",val);

			
			if(IsDlgButtonChecked(hwnd,IDC_TRIM_TRAILING)==BST_CHECKED)
				trim_trailing=1;
			else
				trim_trailing=0;
			write_ini_value(INI_SETTINGS,"TRIM_TRAILING",trim_trailing);

			if(IsDlgButtonChecked(hwnd,IDC_LEFT_JUSTIFY)==BST_CHECKED)
				left_justify=1;
			else
				left_justify=0;
			write_ini_value(INI_SETTINGS,"LEFT_JUSTIFY",left_justify);

			for(i=0;i<sizeof(ctrls)/sizeof(int);i++){
				key[0]=0;
				get_control_name(ctrls[i],key,sizeof(key));
				str[0]=0;
				GetDlgItemText(hwnd,ctrls[i],str,sizeof(str));
				write_ini_str(INI_SETTINGS,key,str);
			}
			}
		case IDCANCEL:
			if(font_changed){
				int i,ctrls[3]={IDC_SQL_FONT,IDC_LISTVIEW_FONT,IDC_TREEVIEW_FONT};
				for(i=0;i<sizeof(ctrls)/sizeof(int);i++){
					char key[80],str[80];
					key[0]=0;
					get_control_name(ctrls[i],key,sizeof(key));
					str[0]=0;
					GetDlgItemText(hwnd,ctrls[i],str,sizeof(str));
					get_ini_str(INI_SETTINGS,key,str,sizeof(str));
					if(str[0]!=0){
						int max=get_max_table_windows();
						int font=fontname_to_int(str);
						if(ctrls[i]==IDC_SQL_FONT){
							int j;
							for(j=0;j<max;j++){
								HWND hedit=0;
								if(get_win_hwnds(j,NULL,&hedit,NULL)){
									SendMessage(hedit,WM_SETFONT,GetStockObject(font),0);
									InvalidateRect(hedit,NULL,TRUE);
								}
							}
						}
						else if(ctrls[i]==IDC_LISTVIEW_FONT){
							int j;
							for(j=0;j<max;j++){
								HWND hlistview;
								if(get_win_hwnds(j,NULL,NULL,&hlistview)){
									SendMessage(hlistview,WM_SETFONT,GetStockObject(font),0);
									InvalidateRect(hlistview,NULL,TRUE);
								}
							}
						}
						else if(ctrls[i]==IDC_TREEVIEW_FONT){
							extern HWND ghtreeview;
							SendMessage(ghtreeview,WM_SETFONT,GetStockObject(font),0);
						}
					}
				}
			}
			if(justify_changed){
				int i,max=get_max_table_windows();
				get_ini_value(INI_SETTINGS,"LEFT_JUSTIFY",&left_justify);
				for(i=0;i<max;i++){
					HWND hlistview;
					if(get_win_hwnds(i,NULL,NULL,&hlistview))
						InvalidateRect(hlistview,NULL,TRUE);
				}
			}
			EndDialog(hwnd,0);
			break;
		}
		break;
	}
	return 0;
}