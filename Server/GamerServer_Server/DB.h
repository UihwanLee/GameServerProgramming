#pragma once
#include <windows.h>  
#include <stdio.h>  
#include <locale.h>

#define UNICODE  
#include <sqlext.h>  

#include <iostream>
#include <codecvt>
#include <string>

#define NAME_LEN 20
#define PHONE_LEN 60

class DB
{
private:
    SQLHENV henv;
    SQLHDBC hdbc;
    SQLHSTMT hstmt;
    SQLRETURN retcode;
    SQLWCHAR szName[NAME_LEN];
    SQLINTEGER dId;
    SQLSMALLINT dLevel;
    SQLSMALLINT dPosX;
    SQLSMALLINT dPosY;
    SQLSMALLINT dHp;
    SQLLEN cbName, cbLevel, cbId, cbPosX, cbPosY, cbHp;
    wchar_t query[256];

    short posX;
    short posY;
    char* name;
    int   hp;
    int   level;

public:
    DB();
    ~DB();

    void show_error();
    void disp_error(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);

    bool check_id(int id);
    void update_pos(int id, short x, short y);

    char* getName() { return name; }
    short getPosX() { return posX; }
    short getPosY() { return posY; }
    int   getHP() { return hp; }
    int   getLevel() { return level; }
};

