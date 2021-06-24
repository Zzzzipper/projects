#ifndef COMMON_UTILS_LEDINTERFACE_H
#define COMMON_UTILS_LEDINTERFACE_H

class LedInterface {
public:
	enum State {
		State_Off = 0,
		State_InProgress,
		State_Success,
		State_Failure,
		State_Failure1,
		State_Failure2,
		State_Failure3,
	};
	virtual ~LedInterface() {}
	virtual void setInternet(State state) = 0;
	virtual void setPayment(State state) = 0;
	virtual void setFiscal(State state) = 0;
	virtual void setServer(State state) = 0;
};

#endif
