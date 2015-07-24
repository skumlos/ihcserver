#ifndef USERLEVEL_H
#define USERLEVEL_H
#include <string>

namespace Userlevel {
	enum Levels {
		ADMIN,
		SUPERUSER,
		BASIC
	};

	class UserlevelToken;

	void init();

	void login(UserlevelToken* &token, std::string code);
	void loginSHA(UserlevelToken* &token, std::string codeSHA);

	void setCode(enum Levels level, std::string code);
	void setCodeSHA(enum Levels level, std::string codeSHA);

	enum Levels getUserlevel(UserlevelToken* &token);
	std::string tokenToString(UserlevelToken* &token);
};


#endif /* USERLEVEL_H */
