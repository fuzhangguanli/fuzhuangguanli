#include "conference.h"
#include <algorithm>
#include "msgstruct.h"
#define MAXPKG 10
#define THIS_FILE "conference.cpp"

user::user():m_framelen(FRAMELEN),m_isdel(false),m_seq(0)
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
bool user::setinfo(pj_uint32_t userid, pj_uint64_t pt, pj_sockaddr_in &addr)
{
	m_userid = userid;
	m_remoteaddr = addr;
	for(int i =0; i<MAXPKG; i++)
	{
		m_idlelist.push_back(new pkgbuf);
	}
	memset(&m_rtp,0,sizeof(m_rtp));

	m_rtp.pt = pt;
	m_rtp.v = 2;
	m_rtp.ssrc = pj_htonl(userid);

	m_codec = new opuscodec(pt);
	return m_codec->init();
}
void user::outrtp(pj_int8_t *buf, pj_uint32_t ts)
{
	m_rtp.ts = pj_htonl(ts);
	m_rtp.seq = pj_htons(m_seq++);
	pj_memcpy(buf, &m_rtp, sizeof(m_rtp));
}

bool user::mixframe(pj_int16_t *mix, int mixlen)
{
	if(m_framelen <= 0)
		return false;
	else
	{
		pj_int32_t tmp = 0;
		for(int i=0; i< mixlen && mixlen<m_framelen;i++)
		{
			tmp=m_framebuf[i]+mix[i];
			if(tmp > 32767)
				tmp= 32767;
			if(tmp < -32767)
				tmp = -32767;
			mix[i] = tmp;
			tmp=0;
		}
		return true;
	}

}
bool user::getframe()
{
	m_framelen = FRAMELEN;
	getonepkg(m_framebuf, m_framelen);
	m_volume = 0;
	
	if(m_framelen > 0)
	{
		for(int i=0; i< m_framelen; i++)
		{
			if(m_framebuf[i] > 0)
				m_volume += m_framebuf[i];
			else
				m_volume -= m_framebuf[i];
		}
		m_volume/= m_framelen;
	}

}

bool user::getonepkg(pj_int16_t *out ,int &outlen)
{
	if(out== NULL && outlen <= 640)
	{
		outlen = 0;
		return false;
	}
	if(m_usedlist.empty())
	{
		m_codec->decode(NULL,0,out, outlen);
		memset(&m_pkg, 0 , sizeof(m_pkg));
	}
	else
	{
		m_usedmutex.lock();
		pkgbuf *pbuf = m_usedlist.front();
		m_usedlist.pop_front();
		m_usedmutex.unlock();
		m_pkg = pbuf;

		m_codec->decode(pbuf->buf,pbuf->buflen,out, outlen);
		m_idlemutex.lock();
		m_idlelist.push_back(pbuf);
		m_idlemutex.unlock();
	}
	return true;

}
void user::setusedpkg(pkgbuf *pkg)
{
	m_usedmutex.lock();
	m_usedlist.push_back(pkg);
	m_usedmutex.unlock();

}
void user::setidlepkg(pkgbuf *pkg)
{
	m_idlemutex.lock();
	m_idlelist.push_back(pkg);
	m_idlemutex.unlock();
}
pkgbuf * user::getidlepkg()
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



pj_sock_t conference::s_sock = -1;
PMutex conference::s_sendmutex;
conference::conference(pj_uint64_t confid, pj_uint64_t pt):
	m_confid(confid)
	,m_stop(false)
	,m_del(false)
	,m_timestamp(0)
{
	if(pt == 8)
		m_framelen = 320;
	if(pt == 16)
		m_framelen = 640;

	m_codec = new opuscodec(pt);
}

conference::~conference()
{
	if(m_codec)
		delete m_codec;
	map<pj_uint32_t , user*>::iterator iter = m_users.begin();
	while(iter != m_users.end())
	{
		delete iter->second;
	}
	m_users.clear();
}

void conference::usertodeque()
{
	map<pj_uint32_t ,user *>::iterator iter = m_users.begin();
	while(iter != m_users.end())
	{
		m_usersdeque.push_back(iter->second);
		iter++;
	}
}
void usersort(user * us)
{
	us->sort();
}
void userdecode(user *us)
{
	us->getframe();
}

#define SLEEPTIME 40
void conference::Run()
{
	pj_timestamp timeold, timecur, lastpkgtime;
	while(!m_stop)
	{
		pj_get_timestamp(&timeold);
		map<pj_uint32_t , user*>::iterator iter = m_users.begin();
		while(iter != m_users.end())
		{
			if(iter->second->isdel())
			{
				map<pj_uint32_t , user*>::iterator olditer = iter;
				iter++;
				delete iter->second;
				m_users.erase(olditer);
			}
			else
				iter++;
		}
		m_usersdeque.clear();
		usertodeque();
		for_each(m_usersdeque.begin(), m_usersdeque.end(), usersort);
		if(m_usersdeque.size() <=1)
		{
			if(m_usersdeque.size() == 1)
			{
				user *us = m_usersdeque[0];
				pkgbuf *buf = us->getusedpkg();
				if(buf)
				{
					us->setidlepkg(buf);
					pj_get_timestamp(&lastpkgtime);
				}
			}
		}
		else if(m_usersdeque.size() == 2)
		{
			user *a = m_usersdeque[0];
			user *b = m_usersdeque[1];
			
			pkgbuf *pkg = a->getusedpkg();
			if(pkg!= NULL)
			{
				sendpkg(b, pkg, m_timestamp);
				a->setidlepkg(pkg);
				pj_get_timestamp(&lastpkgtime);
			}
			pkg = b->getusedpkg();
			if(pkg != NULL )
			{
				sendpkg(a, pkg, m_timestamp);
				b->setidlepkg(pkg);
				pj_get_timestamp(&lastpkgtime);
			}

		}
		else
		{
			memset(m_fbuf, 0, FRAMEBUFLEN*2);
			for_each(m_usersdeque.begin(), m_usersdeque.end(), userdecode);
			std::sort(m_usersdeque.begin(), m_usersdeque.end());
			user *a = m_usersdeque[0];
			user *b = m_usersdeque[1]; 
			pkgbuf &apkg = a->getcurrentpkg();
			if(apkg.buflen > 0)
			{
				pj_get_timestamp(&lastpkgtime);
				sendpkg(b, &apkg, m_timestamp);
			}
			pkgbuf &bpkg = b->getcurrentpkg();
			if(bpkg.buflen > 0)
			{
				pj_get_timestamp(&lastpkgtime);
				sendpkg(a, &bpkg, m_timestamp);
			}

			if(a->mixframe(m_fbuf, m_framelen) ||
					b->mixframe(m_fbuf, m_framelen))
			{
				m_pkg.buflen = PKGLEN;
				m_codec->encode(m_fbuf,m_framelen, m_pkg.buf, (int &) m_pkg.buflen); 

				for(int i=2; i< m_usersdeque.size();i++)
				{
					sendpkg(m_usersdeque[i],&m_pkg, m_timestamp); 
				}
			}
		}

		pj_get_timestamp(&timecur);
		pj_uint32_t msec =  pj_elapsed_msec(&timeold, &timecur);
		m_timestamp+=m_framelen;
		pj_uint32_t lmsec = pj_elapsed_msec(&lastpkgtime, &timecur);
		if(lmsec > 300000)
		{
			PJ_LOG(5,(THIS_FILE, "会议:%llu 超过5分钟没有接收到数据，会议停止。",m_confid));
			Stop();
			break;
		}
		pj_thread_sleep(SLEEPTIME-msec);
	}
	m_del = true;
}
void conference::Stop()
{
	m_stop = true;

}
bool conference::operator==(pj_uint64_t pt)
{
	return (*m_codec)==pt;
}
void conference::sendpkg(user *u, pkgbuf *pkg, pj_uint32_t time)
{
	msg_head *head = (msg_head *)m_sendbuf;	
	head->_call_id = m_confid;
	head->_user_id = CONFUSERID;
	head->_type = MEDIA_RTP;
	head->_body_len = sizeof(rtp_hdr) + pkg->buflen;
	u->outrtp(m_sendbuf+sizeof(msg_head), time);
	memcpy(m_sendbuf+sizeof(msg_head) + sizeof(rtp_hdr), pkg->buf, pkg->buflen);

	s_sendmutex.lock();
	pj_ssize_t sendlen = sizeof(msg_head)+sizeof(rtp_hdr)+ pkg->buflen;
	pj_sock_sendto(s_sock, m_sendbuf, &sendlen, 0, &(u->getremoteaddr()), sizeof(pj_sockaddr_in));
	s_sendmutex.unlock();
}
bool conference::adduser(pj_uint32_t userid, pj_uint64_t pt, pj_sockaddr_in &addr)
{
	user *us = new user;
	us->setinfo(userid, pt, addr);
	PWaitAndSignal ws(m_usermutex);
	m_users[userid] = us;
}
void conference::deluser(pj_uint32_t userid)
{
	if(m_users.find(userid) != m_users.end())
	{
		m_users[userid]->setdel();
	}
}
user* conference::getuser(pj_uint32_t userid)
{
	if(m_users.find(userid) != m_users.end())
	{
		return m_users[userid];
	}
	else
		return NULL;
	
}
