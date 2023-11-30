/* Copyright (c) 2019-2021 Marvell International Ltd. All rights reserved */
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "SlanCfgReader.h"
#include <unistd.h>


//OLEG TODO
std::string getpath()
{
//  char buf[200 + 1];
//  if (readlink("/proc/self/exe", buf, sizeof(buf) - 1) == -1)
//  throw std::string("readlink() failed");


  char tmp[256];
  getcwd(tmp, 256);
  cout << "Current working directory: " << tmp << endl;

  std::string str(tmp);
  return str.substr(0, str.rfind('/'));
}

using namespace std;

static const string& trimString(string& s, const char* t = " \r")
{
	s.erase(0, s.find_first_not_of(t));
	s.erase(s.find_last_not_of(t) + 1);

	return s;
}

bool readSlanCfgFile(const char* slanCfgFile, StringVector& slansList)
{

    std::cout << "This program resides in!!!!! " << getpath() << std::endl;

    slansList.clear();

	ifstream infile;
	infile.open(slanCfgFile);
	if (! infile.is_open()) 
	{
		return false;
	}

	while (infile)
	{
		string s;
		if (! getline(infile, s)) 
		{
			break;
		}

		istringstream ss(s);

		while (ss)
		{
			string s;
			if (! getline(ss, s, ',')) 
			{
				break;
			}
			slansList.push_back( trimString(s) );
		}
	}
	
	return true;
}
