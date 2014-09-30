#include "job.h"
#include "opus_codec.h"
#include <deque>
#include <map>
#ifndef CONFERENCE_H
#define CONFERENCE_H
#define FRAMELEN 1280
#define PKGLEN 512
#define CONFUSERID 0x0fffffff;
using namespace std;
typedef struct pkgbuf
{
	pj_uint32_t time;
	pj_uint16_t seq;
	pj_ssize_t buflen;
	pj_uint8_t buf[PKGLEN];

	void operator =(pkgbuf *pkg)
	{
		if(pkg)
		{
			this->seq = pkg->seq;
			this->time =  pkg->time;
			if(pkg->buflen > FRAMELEN)
				pkg->buflen = FRAMELEN;
			this->buflen= pkg->buflen;
			memcpy(this->buf, pkg->buf, pkg->buflen);
		}
	}
	
}pkgbuf,*ppkgbuf;

#pragma pack(1)
typedef struct pjmedia_rtp_hdr
{
#if defined(PJ_IS_BIG_ENDIAN) && (PJ_IS_BIG_ENDIAN!=0)
    pj_uint16_t v:2;		/**< packet type/version	    */
    pj_uint16_t p:1;		/**< padding flag		    */
    pj_uint16_t x:1;		/**< extension flag		    */
    pj_uint16_t cc:4;		/**< CSRC count			    */
    pj_uint16_t m:1;		/**< marker bit			    */
    pj_uint16_t pt:7;		/**< payload type		    */
#else
    pj_uint16_t cc:4;		/**< CSRC count			    */
    pj_uint16_t x:1;		/**< header extension flag	    */ 
    pj_uint16_t p:1;		/**< padding flag		    */
    pj_uint16_t v:2;		/**< packet type/version	    */
    pj_uint16_t pt:7;		/**< payload type		    */
    pj_uint16_t m:1;		/**< marker bit			    */
#endif
    pj_uint16_t seq;		/**< sequence number		    */
    pj_uint32_t ts;		/**< timestamp			    */
    pj_uint32_t ssrc;		/**< synchronization source	    */
}rtp_hdr;
#pragma pack()

class user
{
public:
	user();
	~user();
	bool setinfo(pj_uint32_t userid, pj_uint64_t pt, pj_sockaddr_in &addr);
	void setusedpkg(pkgbuf *pkg);
	void setidlepkg(pkgbuf *pkg);
	bool getframe();
	pkgbuf * getidlepkg();
	pkgbuf * getusedpkg();
	void sort();
	pkgbuf & getcurrentpkg() { return m_pkg; }
	bool mixframe(pj_int16_t *mix, int mixlen);
	pj_sockaddr_in & getremoteaddr() {return m_remoteaddr; }
	void setdel() { m_isdel = true; }
	bool isdel() { return m_isdel;}
	pj_uint32_t getvolume() { return m_volume;}
	bool operator <( user *b) { return m_volume > b->getvolume(); }
	void outrtp(pj_int8_t *buf, pj_uint32_t ts);
private:
	bool getonepkg(pj_int16_t *out ,int &outlen);
	opuscodec *m_codec;
	pj_uint32_t m_userid;
	deque<ppkgbuf> m_idlelist;
	deque<ppkgbuf> m_usedlist;
	PMutex m_idlemutex,m_usedmutex;
	pj_int16_t m_framebuf[1280];
	pj_int32_t m_framelen;
	pj_uint32_t m_volume;
	pkgbuf	m_pkg;
	pj_sockaddr_in m_remoteaddr;
	rtp_hdr m_rtp;
	pj_uint16_t m_seq;
	bool m_isdel;
};

#define FRAMEBUFLEN 1280
class conference : public Job
{
	conference(){}
public:
	conference(pj_uint64_t confid, pj_uint64_t pt);
	~conference();

	virtual void Run();
	virtual void Stop();
	bool operator==(pj_uint64_t pt);
	void sendpkg(user *u, pkgbuf *pkg, pj_uint32_t time);
	void sort();
	bool adduser(pj_uint32_t userid, pj_uint64_t pt, pj_sockaddr_in &addr);
	void deluser(pj_uint32_t userid);
	bool isstop() { return m_stop;}
	bool isdel() {return m_del;}
	user* getuser(pj_uint32_t userid);
public:
	static PMutex s_sendmutex;
	static pj_sock_t s_sock;
private:
	void usertodeque();
	pj_uint64_t m_confid;
	opuscodec *m_codec;
	pj_uint32_t m_timestamp;
	pj_int32_t m_framelen;
	pj_int16_t m_fbuf[FRAMEBUFLEN];
	pj_int8_t m_sendbuf[PKGLEN];
	pkgbuf  m_pkg;
	PMutex	m_usermutex;
	map<pj_uint32_t, user*> m_users;
	deque<user*> m_usersdeque;
	bool m_stop;
	bool m_del;
};

#endif
