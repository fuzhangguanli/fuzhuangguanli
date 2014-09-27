#include "conference.h"
#include <algorithm>
#define MAXPKG 10

user::user():m_framelen(FRAMELEN)
{
	memset(&m_rtp,0,sizeof(m_rtp));

}
user::~user()
{
	if(m_codec)
		delete m_codec;
	while(!m_idlelist.empty())
	{
		delete m_idlelist.front();
		m_idlelist.pop_front();
	
	}
	if(!m_usedlist.empty())
	{
		delete m_usedlist.front();
		m_usedlist.pop_front();
	}

}
bool user::setinfo(pj_uint32_t userid, pj_uint64_t pt)
{
	m_userid = userid;
	for(int i =0; i<MAXPKG; i++)
	{
		m_idlelist.push_back(new pkgbuf);
	}

	m_codec = new opuscodec(pt);
	return m_codec->init();
}
bool user::getframe(pj_int16_t *out ,int &outlen)
{
	m_framelen = FRAMELEN;
	getonepkg(m_framebuf, m_framelen);
}

bool user::getonepkg(pj_int16_t *out ,int &outlen)
{
	if(out== NULL && outlen <= 640)
	{
		return false;
	}
	if(m_usedlist.empty())
	{
		m_codec->decode(NULL,0,out, outlen);
	}
	else
	{
		m_usedmutex.lock();
		pkgbuf *pbuf = m_usedlist.front();
		m_usedlist.pop_front();
		m_usedmutex.unlock();

		m_codec->decode(pbuf->buf,pbuf->buflen,out, outlen);
		m_idlemutex.lock();
		m_idlelist.push_back(pbuf);
		m_idlemutex.unlock();
		
	}
	return true;

}
bool user::setonepkg(pkgbuf *pkg)
{
	m_usedmutex.lock();
	m_usedlist.push_back(pkg);
	m_usedmutex.unlock();

}
pkgbuf * user::getonepkg()
{
	if(m_idlelist.empty())
	{
		return new pkgbuf;
	}
	else
	{
		PWaitAndSignal ws(m_idlemutex);
		pkgbuf *pbuf = m_idlelist.front();
		m_idlelist.pop_front();
		return pbuf;
	}
}
pkgbuf * user::getusedpkg()
{
	if(m_usedlist.empty())
		return NULL;
	else
	{
		PWaitAndSignal ws(m_usedmutex);
		pkgbuf *pbuf = m_usedlist.front();
		m_usedlist.pop_front();
		return pbuf;
	}
}
#define MAXLOSTPKG 100
bool pkgsort(pkgbuf* a, pkgbuf *b)
{
	if(a->seq < b->seq && b->seq - a->seq < MAXLOSTPKG)
		return true;
	if(a->seq > b->seq && a->seq - b->seq > MAXLOSTPKG)
		return true;

	return false;
}
void user::sort()
{
	if(m_usedlist.size()<= 1)
		return ;
	PWaitAndSignal ws(m_usedmutex);
	std::sort(m_usedlist.begin(), m_usedlist.end(), pkgsort);

}

conference::conference(pj_uint64_t confid, pj_uint64_t pt):m_confid(confid)
{
	m_codec = new opuscodec(pt);
}

conference::~conference()
{
	if(m_codec)
		delete m_codec;
}

void conference::Exec()
{
	

}
bool conference::operator==(pj_uint64_t pt)
{
	return (*m_codec)==pt;
}
void conference::sendpkg(user *u, pkgbuf *pkg, pj_uint32_t time)
{


}
