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
    SQLSMALLINT dExp;
    SQLSMALLINT dMaxExp;
    SQLLEN cbName, cbLevel, cbId, cbPosX, cbPosY, cbHp, cbExp, cbMaxExp;
    wchar_t query[256];

    short posX;
    short posY;
    char* name;
    int   hp;
    int   level;
    int   exp;
    int   max_exp;

public:
    DB();
    ~DB();

    void show_error();
    void disp_error(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);

    bool check_id(int id);
    void update_pos(int id, short x, short y);
    void update_level(int id, int level, int exp, int max_exp);

    char* getName() { return name; }
    short getPosX() { return posX; }
    short getPosY() { return posY; }
    int   getHP() { return hp; }
    int   getLevel() { return level; }
    int   getExp() { return exp; }
    int   getMaxExp() { return max_exp; }
};

