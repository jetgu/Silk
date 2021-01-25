// Silk.cpp : Defines the entry point for the console application.
//

#include <string>
#include "Lexer.h"
#include "Interpreter.h"


int main(int argc, char* argv[])
{
#ifdef _WIN32
	string slash = "\\";
#else
	string slash = "/";
#endif		

	string filename;
	if (argc == 1)
	{
		printf("Silk programming language version %s (%s)\n", _VERSION,_PLATFORM);
		printf("Copyright (c) 2021 Gu Hong. All Rights Reserved.\n\n");
		printf("You can run Silk followed by your script file (Silk myscript.si)\n");
		printf("Or input your script file here to run:\n");
		char buff[1024];
		if (fgets(buff, 1024, stdin))
		{
			char* pFind = strchr(buff, '\n');
			if (pFind)
				*pFind = '\0';
			filename = buff;
			string::size_type pos = filename.find(slash);
			if (pos == string::npos)
			{
				string exeFile = argv[0];
				pos = exeFile.rfind(slash);
				if (pos != string::npos)
					filename = exeFile.substr(0, pos + 1)+filename;
			}

		}
	}

	if (argc > 1)
		filename = argv[1];

	string strText = Tool::readfile(filename);
	if (strText.size() == 0)
	{
		printf("cannot open file %s.", filename.c_str());
		getchar();
		return 0;
	}

	string curdir;
	string::size_type pos = filename.rfind(slash);
	if (pos != string::npos)
		curdir = filename.substr(0, pos + 1);

	CGlobalData globaldata;
	CLexer lexer(strText, filename);
	CParser parser = CParser(lexer, &globaldata);
	parser.set_curdir(curdir);
	CInterpreter interpreter = CInterpreter(parser);
	vector<CVariable> vecArgv;
	for (int i = 1; i < argc; i++)
		vecArgv.push_back(CVariable(argv[i]));
	if (argc == 1)
		vecArgv.push_back(CVariable(filename));
	interpreter.set_argv(vecArgv);

	CVariable result = interpreter.interpret();

	return 0;
}

