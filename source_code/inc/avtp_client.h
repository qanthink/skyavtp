/*---------------------------------------------------------------- 
xxx 版权所有。
作者：
时间：2022.3.13
----------------------------------------------------------------*/

#pragma once

#include "avtp.h"

/* 通过继承抽象类形成Client */
class AvtpVideoClient : public AvtpVideoBase
{
public:
	/* 借助父类构造子类 */
	AvtpVideoClient(const char *hostIP, const char *destIP) : AvtpVideoBase(hostIP, destIP){}
private:
	/* 子类必须重写纯虚函数 */
	void *stateMachine(void *arg);
};

