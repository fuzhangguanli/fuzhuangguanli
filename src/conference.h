#include "job.h"
#include "opus_codec.h"
#include <deque>
#include <map>
#ifndef CONFERENCE_H
#define CONFERENCE_H
#define FRAMELEN 1280
using namespace std;
typedef struct pkgbuf
{
	pj_uint32_t time;
	pj_uint16_t seq;
	pj_uint16_t buflen;
	pj_uint8_t buf[512];
	
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
	bool setinfo(pj_uint32_t userid, pj_uint64_t pt);
	bool setonepkg(pkgbuf *pkg);
	bool getframe(pj_int16_t *out ,int &outlen);
	pkgbuf * getonepkg();
	pkgbuf * getusedpkg();
	void sort();
private:
	bool getonepkg(pj_int16_t *out ,int &outlen);
	opuscodec *m_codec;
	pj_uint32_t m_userid;
	deque<ppkgbuf> m_idlelist;
	deque<ppkgbuf> m_usedlist;
	PMutex m_idlemutex,m_usedmutex;
	pj_int16_t m_framebuf[1280];
	pj_int32_t m_framelen;
	pj_int16_t m_volume;
	pj_sockaddr_in m_remoteaddr;
	rtp_hdr m_rtp;
};

class conference : public RegularJob
{
	conference():m_timetamp(0){}
public:
	conference(pj_uint64_t confid, pj_uint64_t pt);
	~conference();

	virtual void Exec() ;
	bool operator==(pj_uint64_t pt);
	void sendpkg(user *u, pkgbuf *pkg, pj_uint32_t time);
	void sort();
public:
	static PMutex s_sendmutex;
	static int s_sock;
private:
	pj_uint64_t m_confid;
	opuscodec *m_codec;
	pj_uint32_t m_timetamp;
	map<pj_uint32_t, user*> m_users;
	deque<user*> m_usersdeque;
};

#endif
