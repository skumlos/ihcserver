#include "IHCServer.h"
#include <unistd.h>

int main(int argc, char* argv[]) {
	IHCServer server("/dev/ttyS1");
	while(server.isRunning()) {
		sleep(100);
	}
};
