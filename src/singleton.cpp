//////////////////////////////////////////////////////////////////
//
// singleton.cxx
//
// Copyright (c) 2001-2013, Jan Willamowius
//
// This work is published under the GNU Public License version 2 (GPLv2)
// see file COPYING for details.
// We also explicitly grant the right to link this code
// with the OpenH323/H323Plus and OpenSSL library.
//
//////////////////////////////////////////////////////////////////

#include "job.h"
#include "singleton.h"
//#include "zlog.h"

static int singleton_cnt = 0;
//extern zlog_category_t *zc;

listptr<SingletonBase> SingletonBase::_instance_list;

SingletonBase::SingletonBase(const char *n) : m_name(n)
{
	++singleton_cnt;
	PJ_LOG(3, ("SingletonBase","Create instance:%s %d ", m_name , singleton_cnt) );
	_instance_list.push_back(this);
}

SingletonBase::~SingletonBase()
{
	--singleton_cnt;
	PJ_LOG(4, ("SingletonBase", "Delete instance:%s %d objects left" , m_name , singleton_cnt));
	if (!_instance_list.clear_list)
		_instance_list.remove(this);
}
