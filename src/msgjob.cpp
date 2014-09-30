#include "msgjob.h"
#include "msgstruct.h"
#include "conference.h"
#include <iostream>
#include <algorithm>
using namespace std;
#define THIS_FILE "msgjob"

void msgjob::Stop()
{
	PJ_LOG(3,(THIS_FILE ,"msgjob stop"));
	pj_sock_shutdown(m_sock, SHUT_RDWR);
	RegularJob::Stop();	
}
void msgjob::Exec() 
{
	pj_sockaddr_in remoteaddr;
	int addrlen = sizeof(remoteaddr);
	pj_ssize_t recvlen = BUFLEN;
	pj_status_t status = pj_sock_recvfrom(m_sock,m_buf, &recvlen,0,&remoteaddr, &addrlen);
	if(status == PJ_SUCCESS)
	{
		if(recvlen <= 0)
		{
			PJ_LOG(3,(THIS_FILE, "recvfrom socket close"));
			return;
		}
		msg_head *head = (msg_head*)m_buf;	
		switch(head->_type)
		{
		case MEDIA_RTP:
			{
				if(m_confs.find(head->_call_id) == m_confs.end())
				{
					PJ_LOG(3,(THIS_FILE, "没有会议：%llu , rtp数据被丢弃", head->_call_id));
				}
				else
				{
					user *us = m_confs[head->_call_id]->getuser(head->_user_id);
					if(us == NULL)
					{
						PJ_LOG(3,(THIS_FILE, "在会议:%llu 中没有找到用户:%u", head->_call_id, head->_user_id));
					}
					else
					{
						if(head->_body_len <=0 || head->_body_len > PKGLEN)
						{
							PJ_LOG(3,(THIS_FILE, "rtp 长度是错误的：%d", head->_body_len));
						}
						else
						{
							pkgbuf *pkg = us->getidlepkg();
							rtp_hdr *rtp = (rtp_hdr *)(m_buf+sizeof(msg_head));
							pkg->seq = ntohs(rtp->seq);
							pkg->time = ntohl(rtp->ts);
							memcpy(pkg->buf,(m_buf+sizeof(msg_head)+sizeof(rtp_hdr)),head->_body_len-sizeof(rtp_hdr)); 
							us->setusedpkg(pkg);
						}
					}
				}
			}
			break;
		case MEDIA_REGISTER:
			{
				if(head->_body_len != 8)
				{
					PJ_LOG(3,(THIS_FILE, "注册信令格式错误 userid = %u callid = %llu", head->_user_id, head->_call_id));
					break;
				}
				pj_uint64_t *pt = (pj_uint64_t *)(m_buf+sizeof(msg_head));
				if(m_confs.find(head->_call_id) == m_confs.end())
				{
					conference *conf = new conference(head->_call_id, *pt);
					conf->adduser(head->_user_id, *pt, remoteaddr);
					conf->Execute();
					PJ_LOG(5,(THIS_FILE, "创建会议:%llu 增加用户: %u", head->_call_id, head->_user_id));
				}
				else
				{
					conference *conf = m_confs[head->_call_id];
					conf->adduser(head->_user_id, *pt, remoteaddr);
					PJ_LOG(5,(THIS_FILE, "增加用户: %u", head->_user_id));
				}
				head->_type = MEDIA_REGISTER_SUCCESS_RESPONSE;
				pj_sock_sendto(m_sock, m_buf, &recvlen, 0 , &remoteaddr, sizeof(remoteaddr)); 
			}
			break;
		case MEDIA_UNREGISTER:
			{
				if(m_confs.find(head->_call_id) != m_confs.end())
				{
					conference *conf = m_confs[head->_call_id];
					conf->deluser(head->_user_id);
					PJ_LOG(5,(THIS_FILE, "从会议:%llu 中删除用户: %u", head->_call_id, head->_user_id));

				}
				else
				{
					PJ_LOG(3,(THIS_FILE , "没有会议: %llu", head->_call_id));
				}

			}
			break;
		default:
			PJ_LOG(3,(THIS_FILE, "读取的数据为非法数据"));
			break;
		}

	}
	else
	{
		PJ_LOG(3,(THIS_FILE,"recvfrom udp data error"));
	}

	if(!m_clearnum)
	{
		m_clearnum++;
		m_clearnum%=1000;
		checkconfs();	
	}
}

bool msgjob::monitorudp(int port)
{
	pj_status_t status;

	status = pj_sock_socket(pj_AF_INET(),pj_SOCK_DGRAM(), pj_SOL_UDP(),&m_sock);
	
	if(status != PJ_SUCCESS)
	{
		PJ_LOG(3,(THIS_FILE, "create sock failed"));
		return false;
	}

	status = pj_sock_bind_in(m_sock, 0, port);
	if(status != PJ_SUCCESS)
	{
		PJ_LOG(3,(THIS_FILE, "sock bind port %d failed", port));
		return false;
	}
}
void msgjob::checkconfs()
{
	map<pj_uint64_t , conference *>::iterator iter = m_confs.begin();
	while(iter != m_confs.end())
	{
		if((*iter).second->isstop())
		{
			delete (*iter).second;
			map<pj_uint64_t , conference *>::iterator tmpiter = iter;
			iter++;
			m_confs.erase(tmpiter);
		}
		else
			iter++;
	}

}
