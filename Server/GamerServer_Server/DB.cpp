#include "DB.h"

DB::DB()
{
	hstmt = 0;
	cbName = 0, cbLevel = 0, cbId = 0, cbPosX = 0, cbPosY = 0, cbHp = 0;

    setlocale(LC_ALL, "korean");

    // Allocate environment handle  
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
}

DB::~DB()
{
}

void DB::disp_error(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
{
    SQLSMALLINT iRec = 0;
    SQLINTEGER  iError;
    WCHAR
        wszMessage[1000];
    WCHAR
        wszState[SQL_SQLSTATE_SIZE + 1];
    if (RetCode == SQL_INVALID_HANDLE) {
        fwprintf(stderr, L"Invalid handle!\n");
        return;
    }
    while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
        (SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT*)NULL) == SQL_SUCCESS) {
        // Hide data truncated..
        if (wcsncmp(wszState, L"01004", 5)) {
            fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
        }
    }
}

void DB::show_error()
{
    printf("error\n");
}

//wchar_t 에서 char 로의 형변환 함수

char* ConvertWCtoC(wchar_t* str)
{
    //반환할 char* 변수 선언
    char* pStr;

    //입력받은 wchar_t 변수의 길이를 구함
    int strSize = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    //char* 메모리 할당
    pStr = new char[strSize];

    //형 변환 
    WideCharToMultiByte(CP_ACP, 0, str, -1, pStr, strSize, 0, 0);
    return pStr;
}

bool DB::check_id(int id)
{
    // Allocate environment handle  
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

    // Set the ODBC version environment attribute  
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

        // Allocate connection handle  
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

            // Set login timeout to 5 seconds  
            if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

                // Connect to data source  
                retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2018156032_GameServer_TF", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

                // Allocate statement handle  
                if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

                    swprintf(query, sizeof(query) / sizeof(wchar_t), L"EXEC check_id %d", id);
                    retcode = SQLExecDirect(hstmt, query, SQL_NTS);
                    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

                        // Bind columns 1, 2, and 3  
                        retcode = SQLBindCol(hstmt, 1, SQL_C_LONG, &dId, 10, &cbId);
                        retcode = SQLBindCol(hstmt, 2, SQL_C_WCHAR, szName, 20, &cbName);
                        retcode = SQLBindCol(hstmt, 3, SQL_C_SHORT, &dLevel, 10, &cbLevel);
                        retcode = SQLBindCol(hstmt, 4, SQL_C_SHORT, &dPosX, 10, &cbPosX);
                        retcode = SQLBindCol(hstmt, 5, SQL_C_SHORT, &dPosY, 10, &cbPosY);
                        retcode = SQLBindCol(hstmt, 6, SQL_C_SHORT, &dHp, 10, &cbHp);

                        // Fetch and print each row of data. On an error, display a message and exit.  
                        for (int i = 0; ; i++) {
                            retcode = SQLFetch(hstmt);
                            if (retcode == SQL_ERROR)
                                show_error();
                            if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
                            {
                                //replace wprintf with printf
                                //%S with %ls
                                //warning C4477: 'wprintf' : format string '%S' requires an argument of type 'char *'
                                //but variadic argument 2 has type 'SQLWCHAR *'
                                //wprintf(L"%d: %S %S %S\n", i + 1, sCustID, szName, szPhone);  
                                wprintf(L"[DB 읽기] USER | %d: %6d %20s %3d %3d %3d %3d\n", i + 1, dId, szName, dLevel, dPosX, dPosY, dHp);

                                name = ConvertWCtoC(szName);

                                posX = static_cast<short>(dPosX);
                                posY = static_cast<short>(dPosY);

                                return true;
                            }
                            else
                            {
                                return false;
                            }
                                
                        }
                    }
                    else {
                        disp_error(hstmt, SQL_HANDLE_STMT, retcode);
                        return false;
                    }


                    // Process data  
                    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                        SQLCancel(hstmt);
                        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
                    }

                    SQLDisconnect(hdbc);
                }

                SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
            }
        }
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
    }

    return false;
}

void DB::update_pos(int id, short x, short y)
{
    // Allocate environment handle  
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

    // Set the ODBC version environment attribute  
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

        // Allocate connection handle  
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

            // Set login timeout to 5 seconds  
            if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

                // Connect to data source  
                retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2018156032_GameServer_TF", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

                // Allocate statement handle  
                if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

                    swprintf(query, sizeof(query) / sizeof(wchar_t), L"EXEC update_pos %d, %d, %d", id, x, y);
                    retcode = SQLExecDirect(hstmt, query, SQL_NTS);
                    std::cout << query << std::endl;
                    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

                        // Bind columns 1, 2, and 3  
                        retcode = SQLBindCol(hstmt, 1, SQL_C_LONG, &dId, 10, &cbId);
                        retcode = SQLBindCol(hstmt, 2, SQL_C_WCHAR, szName, 20, &cbName);
                        retcode = SQLBindCol(hstmt, 3, SQL_C_SHORT, &dLevel, 10, &cbLevel);
                        retcode = SQLBindCol(hstmt, 4, SQL_C_SHORT, &dPosX, 10, &cbPosX);
                        retcode = SQLBindCol(hstmt, 5, SQL_C_SHORT, &dPosY, 10, &cbPosY);
                        retcode = SQLBindCol(hstmt, 6, SQL_C_SHORT, &dHp, 10, &cbHp);

                        // Fetch and print each row of data. On an error, display a message and exit.  
                        for (int i = 0; ; i++) {
                            retcode = SQLFetch(hstmt);
                            if (retcode == SQL_ERROR)
                                show_error();
                            if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
                            {
                                //replace wprintf with printf
                                //%S with %ls
                                //warning C4477: 'wprintf' : format string '%S' requires an argument of type 'char *'
                                //but variadic argument 2 has type 'SQLWCHAR *'
                                //wprintf(L"%d: %S %S %S\n", i + 1, sCustID, szName, szPhone);  
                                wprintf(L"[DB 갱신] pos %d: %3d %3d\n", i + 1, dId, dPosX, dPosY);

                                return;
                            }
                            else
                            {
                                disp_error(hstmt, SQL_HANDLE_STMT, retcode);
                                return;
                            }

                        }
                    }
                    else {
                        disp_error(hstmt, SQL_HANDLE_STMT, retcode);
                        return;
                    }


                    // Process data  
                    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                        SQLCancel(hstmt);
                        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
                    }

                    SQLDisconnect(hdbc);
                }

                SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
            }
        }
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
    }

    return;
}
