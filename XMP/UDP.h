
#include<winsock2.h>

#pragma comment(lib,"ws2_32.lib")
class UDP
{
public:
	UDP(void);
	~UDP(void);
	void Createconnect();
	char sendDATA;
private:
	char *DATAbuf;
};

