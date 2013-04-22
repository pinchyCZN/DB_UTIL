#define VC_EXTRALEAN

#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

struct DataBinding {
   SQLSMALLINT TargetType;
   SQLPOINTER TargetValuePtr;
   SQLINTEGER BufferLength;
   SQLINTEGER StrLen_or_Ind;
};
void printCatalog(const struct DataBinding* catalogResult) {
   if (catalogResult[0].StrLen_or_Ind != SQL_NULL_DATA) 
      printf("Catalog Name = %s\n", (char *)catalogResult[0].TargetValuePtr);
}
int MySQLSuccess(SQLRETURN rc) {
   return (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO);
}
int get_tables(DB_TREE *tree)
{
	HSTMT hStmt;
	char pcName[256];
	long lLen;
	int count=0;
	
	SQLAllocStmt(tree->hdbc, &hStmt);
	if (SQLTables (hStmt, NULL, SQL_NTS, NULL, SQL_NTS, NULL, SQL_NTS, (unsigned char*)"'TABLE'", SQL_NTS) != SQL_ERROR)
	{
		if (SQLFetch (hStmt) != SQL_NO_DATA_FOUND)
		{
			while (!SQLGetData (hStmt, 3, SQL_C_CHAR, pcName, 256, &lLen))
			{
				printf("%s\n",pcName);
				insert_item(pcName,tree->hroot);
				SQLFetch(hStmt);
			}
		}
	}
	SQLFreeStmt (hStmt, SQL_CLOSE);
}


int open_db(DB_TREE *tree)
{
    SQLHENV     hEnv = NULL;
    SQLHDBC     hDbc = NULL;
    if(SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&hEnv)==SQL_ERROR)
		return 0;
	if(SQLSetEnvAttr(hEnv,SQL_ATTR_ODBC_VERSION,(SQLPOINTER)SQL_OV_ODBC3,0)==SQL_SUCCESS){
		if(SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc)==SQL_SUCCESS){
			char str[1024]={0};
			char obdc_str[1024]={0};
			int result=0;
			_snprintf(obdc_str,sizeof(obdc_str),"%s",tree->name);
			result=SQLDriverConnect(hDbc,
							 GetDesktopWindow(),
							 (SQLCHAR*)obdc_str, //"ODBC;",
							 SQL_NTS,
							 (SQLCHAR*)str,
							 sizeof(str),
							 NULL,
							 SQL_DRIVER_COMPLETE);
			if(result==SQL_SUCCESS || result==SQL_SUCCESS_WITH_INFO){
				if(result==SQL_SUCCESS_WITH_INFO){
					SQLCHAR state[6],msg[SQL_MAX_MESSAGE_LENGTH];
					SQLINTEGER  error;
					SQLSMALLINT msglen;
					SQLGetDiagRec(SQL_HANDLE_DBC,hDbc,1,state,&error,msg,sizeof(msg),&msglen);
					printf("msg=%s\n",msg);
				}
				if(str[0]!=0){
					write_ini_str("DATABASES","1",str);
				}
				tree->hdbc=hDbc;
				tree->hdbenv=hEnv;
				return TRUE;
			}
		}
	}

    if (hDbc)
    {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    }

    if (hEnv)
    {
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
    }
	return FALSE;
}
int close_db(void *db)
{
	if(db!=0){
	}
	return TRUE;
}
int free_db(void *db)
{
	if(db!=0){
	}
	return TRUE;
}
int get_db_info(void *db,void **info)
{

//SQLTables

	return 0;
}

/*
int get_field_count(CRecordset *rec)
{
	if(rec!=0){
		TRY{
			return rec->GetODBCFieldCount();
		}CATCH(CDBException, e){
			printf("Error getting field count:%s\n",e->m_strError);
		}
		END_CATCH
		return 0;
	}
	else
		return 0;
}

int get_field_name(CRecordset *rec,int index,char *str,int size)
{
	if(index<rec->GetODBCFieldCount()){
		CODBCFieldInfo info;
		rec->GetODBCFieldInfo(index,info);
		if(str!=0 && size>0){
			strncpy(str,info.m_strName,size);
			str[size-1]=0;
		}
		return TRUE;
	}
	return FALSE;
}
int get_fields(CDatabase *db,char *table,char *list,int size)
{
	CString str,SqlString;
	int i;
	int result=FALSE;
	CRecordset rec1( db );

	SqlString = 
	"SELECT TOP 1 * FROM [";
	SqlString+=table;
	SqlString+="]; ";

	TRY{
		rec1.Open(CRecordset::snapshot,SqlString,CRecordset::readOnly);
		if(size>0 && list!=0){
			list[0]=0;
			for(i=0;i<rec1.GetODBCFieldCount();i++)
			{
				CODBCFieldInfo finfo;
				rec1.GetODBCFieldInfo((short)i,finfo);
				if(_snprintf(list,size,"%s%s,",list,finfo.m_strName)>0)
					result=TRUE;
				else
					result=FALSE;
			}
		}
		rec1.Close();
	}CATCH(CDBException, e){
		if(rec1.IsOpen())
			rec1.Close();
	}END_CATCH
	return result;
}

*/