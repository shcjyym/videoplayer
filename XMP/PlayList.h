#pragma once
#include <vector>
#include "DefineUnicode.h"
using namespace duilib;
using namespace std;

class CPlaylist
{
public:
    CPlaylist(void);
    ~CPlaylist(void);

    void Add(string_t strPath);// ���·�����б�β��
	void Delete(int Pos);// ɾ����ǰ�б�
    vector<string_t> GetPlaylist();// ��ȡ�����б�����·��
    string_t GetPlaylist(unsigned uIndex);// ��ȡ�����б�ָ���±��·��

private:
    vector<string_t> m_vctPath;// ��Ų����б��·��
};

