#include "PlayList.h"
#include <tchar.h>
#include <iostream>			
#include <sstream> 		
#include <fstream>	
using namespace std;

CPlaylist::CPlaylist(void)
{
}

CPlaylist::~CPlaylist(void)
{
}

void CPlaylist::Add(string_t strPath)
{
    m_vctPath.push_back(strPath);
}

void CPlaylist::Delete(int Pos)
{
	m_vctPath.erase(m_vctPath.begin() + Pos);
}

vector<string_t> CPlaylist::GetPlaylist()
{
    return m_vctPath;
}

duilib::string_t CPlaylist::GetPlaylist( unsigned uIndex )
{
    if (uIndex < 0 || uIndex >= m_vctPath.size())
    {
        return _T("");
    }

    return m_vctPath[uIndex];
}
