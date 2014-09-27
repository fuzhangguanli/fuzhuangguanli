#include <pjlib.h>
#include <string>

using namespace std;
class codec
{
public:
	codec(const char *name, pj_uint64_t pt):m_name(name),m_pt(pt)
	{}
	virtual ~codec(){}
	virtual bool init() = 0;
	virtual void encode(const pj_int16_t *buf, int buflen, pj_uint8_t *out, int &outlen) = 0;
	virtual void decode(const pj_uint8_t *buf, int buflen, pj_int16_t *out, int &outlen) = 0;
	string getname(){ return m_name;}
	bool operator==(pj_uint64_t pt)
	{
		return m_pt == pt;
	}
private:
	string m_name;
	pj_uint64_t m_pt;
};
