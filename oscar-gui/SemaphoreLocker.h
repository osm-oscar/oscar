#ifndef OSCAR_GUI_SEMAPHORE_LOCKER_H
#define OSCAR_GUI_SEMAPHORE_LOCKER_H
#include <QSemaphore>

namespace oscar_gui {

class SemaphoreLocker {
	QSemaphore & m_s;
	const int m_c;
public:
	SemaphoreLocker(QSemaphore & s, const int c) : m_s(s), m_c(c) {
		m_s.acquire(m_c);
	}
	~SemaphoreLocker() {
		m_s.release(m_c);
	}
};

}

#endif