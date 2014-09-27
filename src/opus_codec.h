
#include "codec.h"
#include <opus.h>
#ifndef OPUSCODEC_H
#define OPUSCODEC_H

class opuscodec :public codec
{
public:
	opuscodec(pj_uint64_t pt);
	~opuscodec();
	virtual bool init() ;
	virtual void encode(const pj_int16_t *buf, int buflen, pj_uint8_t *out, int &outlen);
	virtual void decode(const pj_uint8_t *buf, int buflen, pj_int16_t *out, int &outlen); 
private:
	OpusEncoder *m_encode;
	OpusDecoder *m_decode;
};


#endif
