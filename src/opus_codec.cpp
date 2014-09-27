#include "opus_codec.h"
#define THIS_FILE "opus_codec.cpp"

opuscodec::opuscodec(pj_uint64_t pt):codec("opus",pt)
{
	int fs = 0;
	int bt = 40000;
    int bandwidth=OPUS_AUTO;
	if(pt == 8)
	{
		fs = 8000;
		bt = 20000;
		bandwidth = OPUS_BANDWIDTH_NARROWBAND;
	}
	else if(pt == 16)
	{
		fs = 16000;
		bt = 40000;
		bandwidth = OPUS_BANDWIDTH_WIDEBAND;
	}
	else
	{
		fs = 8000;
		bt = 20000;
		bandwidth = OPUS_BANDWIDTH_NARROWBAND;
	}
	int err =0;
	m_encode = opus_encoder_create(fs , 1, OPUS_APPLICATION_AUDIO,&err);
	if(err != OPUS_OK)
	{
		m_encode = NULL;
		PJ_LOG(3,(THIS_FILE, "opus_encoder_create error"));
	}
	else
	{
       opus_encoder_ctl(m_encode, OPUS_SET_BITRATE(bt));
       opus_encoder_ctl(m_encode, OPUS_SET_BANDWIDTH(bandwidth));
       opus_encoder_ctl(m_encode, OPUS_SET_VBR(1));
       opus_encoder_ctl(m_encode, OPUS_SET_VBR_CONSTRAINT(1));
       opus_encoder_ctl(m_encode, OPUS_SET_COMPLEXITY(10));
       opus_encoder_ctl(m_encode, OPUS_SET_INBAND_FEC(1));
       opus_encoder_ctl(m_encode, OPUS_SET_FORCE_CHANNELS(1));
       opus_encoder_ctl(m_encode, OPUS_SET_DTX(1));
       opus_encoder_ctl(m_encode, OPUS_SET_PACKET_LOSS_PERC(20));

       //opus_encoder_ctl(m_encode, OPUS_GET_LOOKAHEAD(&skip));
       opus_encoder_ctl(m_encode, OPUS_SET_LSB_DEPTH(16));

	}
	m_decode = opus_decoder_create(fs,1,&err);
   if(err != OPUS_OK)
   {
	   m_decode = NULL;
		PJ_LOG(3,(THIS_FILE, "opus_decoder_create error"));
   }	   
}

opuscodec::~opuscodec()
{

	if(m_encode)
	{
		opus_encoder_destroy(m_encode);
	}
	if(m_decode)
	{
		opus_decoder_destroy(m_decode);
	}
}

bool opuscodec::init()
{
	if(m_encode && m_decode)
		return true;
	return false;
}
void opuscodec::encode(const pj_int16_t *buf, int buflen, pj_uint8_t *out, int &outlen)
{
	if(!m_encode)
	{
		outlen = 0;
		return ;
	}
	pj_int32_t ret = opus_encode(m_encode, buf, buflen, out, outlen);

	if(ret > 0)
		outlen = ret;
	else
	{
		PJ_LOG(3,(THIS_FILE,"opus_encode error"));
	}


}
void opuscodec::decode(const pj_uint8_t *buf, int buflen, pj_int16_t *out, int &outlen) 
{
	if(!m_decode)
	{
		outlen = 0;
		return ;
	}
	pj_int32_t ret  = opus_decode(m_decode, buf, buflen,out, outlen,buf==NULL?1:0);
	if(ret > 0)
		outlen = ret;
	else
	{
		PJ_LOG(3,(THIS_FILE,"opus_decode error"));
	}
}
