#ifndef OSCAR_GUI_SEMAPHORE_LOCKER_H
#define OSCAR_GUI_SEMAPHORE_LOCKER_H
#include <QSemaphore>

namespace oscar_gui {

class SemaphoreLocker {
public:
	typedef enum { READ_LOCK=0x1, WRITE_LOCK=0x7FFFFFFF} Type;
public:
	SemaphoreLocker(QSemaphore & s, const int c = READ_LOCK) : m_s(s), m_c(c) {
		m_s.acquire(m_c);
	}
	SemaphoreLocker(SemaphoreLocker &&) = default;
	~SemaphoreLocker() {
		m_s.release(m_c);
	}
	SemaphoreLocker & operator=(SemaphoreLocker &&) = default;
private:
	QSemaphore & m_s;
	const int m_c;
};

}

#endif