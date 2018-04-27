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

    void Add(string_t strPath);// 添加路径到列表尾部
	void Delete(int Pos);// 删除当前列表
    vector<string_t> GetPlaylist();// 获取播放列表所有路径
    string_t GetPlaylist(unsigned uIndex);// 获取播放列表指定下标的路径

private:
    vector<string_t> m_vctPath;// 存放播放列表的路径
};

