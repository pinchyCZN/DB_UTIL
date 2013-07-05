typedef struct{
	int type;
	int length;
	int col_width;
}COL_ATTR;
typedef struct{
	char name[1024];
	char table[80];
	void *hdbc;
	void *hdbenv;
	int abort;
	int columns;
	COL_ATTR *col_attr;
	int rows;
	int selected_column;
	HWND hwnd,hlistview,hlvedit,hedit,hroot,habort,hintel,hlastfocus;
}TABLE_WINDOW;

typedef struct{
	char name[1024];
	char connect_str[1024];
	void *hdbc;
	void *hdbenv;
	HWND htree,hroot;
}DB_TREE;