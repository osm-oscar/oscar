#ifndef OSCAR_GUI_SEMAPHORE_LOCKER_H
#define OSCAR_GUI_SEMAPHORE_LOCKER_H
#include <QSemaphore>

namespace oscar_gui {

class SemaphoreLocker {
public:
	typedef enum { READ_LOCK=0x1, WRITE_LOCK=0x7FFFFFFF} Type;
public:
	SemaphoreLocker(QSemaphore & s, const int c = READ_LOCK) : m_s(s), m_c(c), m_locked(false) {
		lock();
	}
	SemaphoreLocker(SemaphoreLocker && other) :
	m_s(other.m_s),
	m_c(other.m_c),
	m_locked(other.m_locked)
	{
		other.m_locked = false;
	}
	~SemaphoreLocker() {
		unlock();
	}
	inline void unlock() {
		if (m_locked) {
			m_s.release(m_c);
			m_locked = false;
		}
	}
	inline void lock() {
		if (!m_locked) {
			m_s.acquire(m_c);
			m_locked = true;
		}
	}
	inline void update(Type t) {
		if (!m_locked) {
			m_c = t;
		}
		else if (m_c < t) { // read -> write
			m_s.acquire(WRITE_LOCK - READ_LOCK);
		}
		else if (m_c > t) { //write -> read
			m_s.release(WRITE_LOCK - READ_LOCK);
		}
	}
private:
	QSemaphore & m_s;
	int m_c;
	bool m_locked;
};

}

#endif
