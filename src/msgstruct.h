#include <pjlib.h>
enum msgtype
{
	MEDIA_RTP = 1000,
	MEDIA_REGISTER,
	MEDIA_UNREGISTER,
	MEDIA_REGISTER_SUCCESS_RESPONSE

};

typedef struct msg_head
{
	pj_int32_t _type;
	pj_int32_t _user_id; 
	pj_int64_t _call_id; 
	pj_int64_t _body_len; 
}msg_head;
