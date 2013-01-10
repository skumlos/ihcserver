#include <unistd.h>
#include "IHCServer.h"

int main(int argc, char* argv[]) {
	IHCServer* m_ihcserver = new IHCServer();
	while(m_ihcserver->isRunning()) {
		sleep(100);
	}
	delete m_ihcserver;
};
