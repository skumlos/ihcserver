#ifndef IHCIO_H
#define IHCIO_H

class IHCIO {
public:
	IHCIO(int moduleNumber, int ioNumber) :
		m_moduleNumber(moduleNumber),
		m_ioNumber(ioNumber),
		m_state(false)
	{};
	virtual ~IHCIO(){};
	int getModuleNumber() { return m_moduleNumber; };
	int getIONumber() { return ioNumber; };
	void getState() { return m_state; };
private:
	int m_moduleNumber;
	int m_ioNumber;
	bool m_state;
};

#endif /* IHCIO_H */
