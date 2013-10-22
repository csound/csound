#include "message_buffer.h"

using namespace dvx;

message_buffer::message_buffer(t_object *o) : m_obj(o), m_q(), m_lock((char*)"message_buffer")
{
#ifdef _DEBUG
	m_largest_size = 0;
#endif
}

void message_buffer::add(int type, const string & s)
{
	ScopedLock k(m_lock);
	if(m_q.size() < _LIMIT)
		m_q.push_back(new message(type,s));

	// Silently fail if _LIMIT is reached (MUST NOT POST ANYTHING!).
}

void message_buffer::add(int type, const char * s)
{
	ScopedLock k(m_lock);
	if(m_q.size() < _LIMIT)
		m_q.push_back(new message(type,s));

	// Silently fail if _LIMIT is reached (MUST NOT POST ANYTHING!).
}

void message_buffer::addv(int type, const char * s, ... )
{
	char text[MAX_STRING_LENGTH];
	va_list args;
	va_start(args,s);
	vsprintf(text,s,args);
	add(type,text);	
}

void message_buffer::post()
{
	message *m = NULL;
	ScopedLock k(m_lock);

#ifdef _DEBUG
	m_largest_size = (m_largest_size < m_q.size() ? m_q.size() : m_largest_size);
#endif

	while(!m_q.empty())
	{
		m = &m_q.front();
		switch(m->m_type)
		{
		case message::_NORMAL_MSG:
			object_post(m_obj, (char*)m->m_string.c_str());
			break;
		case message::_WARNING_MSG:
			object_warn(m_obj, (char*)m->m_string.c_str());
			break;
		case message::_ERROR_MSG:
			object_error(m_obj, (char*)m->m_string.c_str());
			break;
		}
		m_q.pop_front();
	}
}
