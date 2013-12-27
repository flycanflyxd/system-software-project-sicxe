#include <iostream>
#include <regex>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <utility>

using namespace std;

class OPTAB
{
public:
	OPTAB()
	{
		string tmpMnemonic;
		int tmpOpcode;
		int tmpFormat;
		ifstream fin("OPTAB.txt");
		if (fin.bad())
			cerr << "Cannot open TPTAB.txt\n";
		while (!fin.eof())
		{
			fin >> tmpMnemonic >> hex >>tmpOpcode >> dec >> tmpFormat;
			mnemonic[tmpMnemonic] = make_pair(tmpOpcode, tmpFormat);
		}
		fin.close();
	}
	map<string, pair<int, int> > mnemonic;//mnemonic opcode format
};

class SYMTAB
{
public:
	void add(string label, unsigned int location)
	{
		this->label[label] = location;
	}
	map<string, int> label;//label location
};

class DIRTAB
{
public:
	DIRTAB()
	{
		string tmpDirective;
		ifstream fin("DIRTAB.txt");
		if (fin.bad())
			cerr << "Cannot open DIRTAB.txt\n";
		while (!fin.eof())
		{
			fin >> tmpDirective;
			directive.push_back(tmpDirective);
		}
		fin.close();
	}
	vector<string> directive;
};

class Line
{
public:
	void read(string &fileName)
	{
		ifstream fin(fileName.c_str());
		if (fin.bad())
			cerr << "Cannot open " << fileName << endl;
		while (!fin.eof())
		{
			regex rgx("\\s+");
			smatch token;
			string tmpInstruction;
			vector<string> tmp;
			getline(fin, tmpInstruction);
			sregex_token_iterator iter(tmpInstruction.begin(), tmpInstruction.end(), rgx, -1);
			sregex_token_iterator end;
			for (; iter != end; ++iter)
			{
				if (*iter == "\0") continue;//Don't know why that if a line is starting with space will get '\0'
				tmp.push_back(*iter);
			}
			statement.push_back(tmp);
		}
		fin.close();
	}
	vector< vector<string> > statement;
	vector<int> loc;
	vector<int> objectCode;
	vector<int> format;
};

void HandleComment(Line &line)
{
	line.loc.push_back(-1);
	line.objectCode.push_back(-1);
	line.format.push_back(-1);
}

void HandleInstruction(bool &check, const int mnemonicPosition, Line &line, unsigned int &LOCCTR, OPTAB &optab, vector<string> &statement)
{
	string tmpMnemonic(statement[mnemonicPosition]);
	if (tmpMnemonic[0] == '+')//deal with format 4 situation
	{
		line.loc.push_back(LOCCTR);
		tmpMnemonic.erase(tmpMnemonic.begin());
		line.format.push_back(4);
		LOCCTR += 4;
	}
	map<string, pair<int, int> >::iterator iter = optab.mnemonic.find(tmpMnemonic);
	if (check = iter != optab.mnemonic.end())
	{
		pair<int, int> opcode_format = optab.mnemonic[tmpMnemonic];
		if (line.format.size() == line.objectCode.size())//deal with format 3
		{
			line.loc.push_back(LOCCTR);
			line.format.push_back(opcode_format.second);
			LOCCTR += opcode_format.second;
		}
		if (opcode_format.second == 2)
			line.objectCode.push_back(opcode_format.first << 8);
		else if (opcode_format.second == 3)
			line.objectCode.push_back(opcode_format.first << 16);
		else if (opcode_format.second == 4)
			line.objectCode.push_back(opcode_format.first << 24);
	}
}

void HandleDirective(bool &check, const int directivePosition, Line &line, unsigned int &LOCCTR, OPTAB &optab, vector<string> &statement, DIRTAB &dirtab)
{
	for (string directive : dirtab.directive)
	{
		if (check = statement[directivePosition] == directive)
		{
			if (directive == "START")
			{
				line.format.push_back(-1);
				line.loc.push_back(LOCCTR);
				line.objectCode.push_back(-1);
				break;
			}
			else if (directive == "BASE")
			{
				line.format.push_back(-1);
				line.loc.push_back(-1);
				line.objectCode.push_back(-1);
				break;
			}
			else if (directive == "BYTE")
			{
				line.format.push_back(-1);
				line.loc.push_back(LOCCTR);
				if (statement[directivePosition + 1 ][0] == 'C')
				{
					int tmpObcode = 0;
					LOCCTR += statement[directivePosition + 1].size() - 3;
					for (string::iterator iter = statement[directivePosition + 1].begin() + 2; iter != statement[directivePosition + 1].end() - 1; ++iter)
					{
						tmpObcode <<= 8;
						tmpObcode += *iter;
					}
					line.objectCode.push_back(tmpObcode);
				}
				else if (statement[directivePosition + 1][0] == 'X')
				{
					LOCCTR += (statement[directivePosition + 1].size() - 3) / 2;
					string tmp(statement[directivePosition + 1].begin() + 2, statement[directivePosition + 1].end() - 1);
					int tmpObcode;
					istringstream iss(tmp);
					iss >> hex >> tmpObcode;
					line.objectCode.push_back(tmpObcode);
				}
				break;
			}
			else if (directive == "RESW")
			{
				line.format.push_back(-1);
				line.loc.push_back(LOCCTR);
				line.objectCode.push_back(-1);
				istringstream iss(statement[directivePosition + 1]);
				int resw;
				iss >> resw;
				LOCCTR += 3 * resw;
				break;
			}
			else if (directive == "RESB")
			{
				line.format.push_back(-1);
				line.loc.push_back(LOCCTR);
				line.objectCode.push_back(-1);
				istringstream iss(statement[directivePosition + 1]);
				int resb;
				iss >> resb;
				LOCCTR += resb;
				break;
			}
			else if (directive == "END")
			{
				line.format.push_back(-1);
				line.loc.push_back(-1);
				line.objectCode.push_back(-1);
				break;
			}
		}
	}
}

int main()
{
	OPTAB optab;
	SYMTAB symtab;
	DIRTAB dirtab;
	Line line;
	unsigned int LOCCTR = 0;
	string fileName = "SICXE.dat";
	//cout << "Please input the file name: ";
	//cin >> fileName;
	line.read(fileName);
	//==================pass one=======================
	for (auto statement : line.statement)
	{
		if (statement[0][0] == '.')//deal with comment
		{
			HandleComment(line);
			/*line.loc.push_back(-1);
			line.objectCode.push_back(-1);
			line.format.push_back(-1);*/
		}
		else
		{
			bool check = false;
			//if statement[0] is mnemonic
			HandleInstruction(check, 0, line, LOCCTR, optab, statement);
			//string tmpMnemonic(statement[0]);
			//if (tmpMnemonic[0] == '+')//deal with format 4 situation
			//{
			//	line.loc.push_back(LOCCTR);
			//	tmpMnemonic.erase(tmpMnemonic.begin());
			//	line.format.push_back(4);
			//	LOCCTR += 4;
			//}
			//map<string, pair<int, int> >::iterator iter = optab.mnemonic.find(tmpMnemonic);
			//if (check = iter != optab.mnemonic.end())
			//{
			//	pair<int, int> opcode_format = optab.mnemonic[tmpMnemonic];
			//	if (line.format.size() == line.objectCode.size())//deal with format 3
			//	{
			//		line.loc.push_back(LOCCTR);
			//		line.format.push_back(opcode_format.second);
			//		LOCCTR += opcode_format.second;
			//	}
			//	if (opcode_format.second == 2)
			//		line.objectCode.push_back(opcode_format.first << 8);
			//	else if (opcode_format.second == 3)
			//		line.objectCode.push_back(opcode_format.first << 16);
			//	else if (opcode_format.second == 4)
			//		line.objectCode.push_back(opcode_format.first << 24);
			//}
			if (check) continue;
			//if statement[0] is directive
			HandleDirective(check, 0, line, LOCCTR, optab, statement, dirtab);
			//for (string directive : dirtab.directive)
			//{
			//	if (check = statement[0] == directive)
			//	{
			//		if (directive == "START")
			//		{
			//			line.format.push_back(-1);
			//			line.loc.push_back(LOCCTR);
			//			line.objectCode.push_back(-1);
			//			break;
			//		}
			//		else if (directive == "BASE")
			//		{
			//			line.format.push_back(-1);
			//			line.loc.push_back(-1);
			//			line.objectCode.push_back(-1);
			//			break;
			//		}
			//		else if (directive == "BYTE")
			//		{
			//			line.format.push_back(-1);
			//			line.loc.push_back(LOCCTR);
			//			if (statement[1][0] == 'C')
			//			{
			//				int tmpObcode = 0;
			//				LOCCTR += statement[1].size() - 3;
			//				for (string::iterator iter = statement[1].begin() + 2; iter != statement[1].end() - 1; ++iter)
			//				{
			//					tmpObcode <<= 8;
			//					tmpObcode += *iter;
			//				}
			//				line.objectCode.push_back(tmpObcode);
			//			}
			//			else if (statement[1][0] == 'X')
			//			{
			//				LOCCTR += (statement[1].size() - 3) / 2;
			//				string tmp(statement[1].begin() + 2, statement[1].end() - 1);
			//				int tmpObcode;
			//				istringstream iss(tmp);
			//				iss >> hex >> tmpObcode;
			//				line.objectCode.push_back(tmpObcode);
			//			}
			//			break;
			//		}
			//		else if (directive == "RESW")
			//		{
			//			line.format.push_back(-1);
			//			line.loc.push_back(LOCCTR);
			//			line.objectCode.push_back(-1);
			//			istringstream iss(statement[1]);
			//			int resw;
			//			iss >> resw;
			//			LOCCTR += 3 * resw;
			//			break;
			//		}
			//		else if (directive == "RESB")
			//		{
			//			line.format.push_back(-1);
			//			line.loc.push_back(LOCCTR);
			//			line.objectCode.push_back(-1);
			//			istringstream iss(statement[1]);
			//			int resb;
			//			iss >> resb;
			//			LOCCTR += resb;
			//			break;
			//		}
			//		else if (directive == "END")
			//		{
			//			line.format.push_back(-1);
			//			line.loc.push_back(-1);
			//			line.objectCode.push_back(-1);
			//			break;
			//		}
			//	}
			//}
			if (check) continue;
			//if statement[0] is label
			symtab.add(statement[0], LOCCTR);
			//if statement[1] is mnemonic
			HandleInstruction(check, 1, line, LOCCTR, optab, statement);
			//tmpMnemonic = statement[1];//?
			//if (tmpMnemonic[0] == '+')//deal with format 4 situation
			//{
			//	line.loc.push_back(LOCCTR);
			//	tmpMnemonic.erase(tmpMnemonic.begin());
			//	line.format.push_back(4);
			//	LOCCTR += 4;
			//}
			//iter = optab.mnemonic.find(tmpMnemonic);
			//if (check = iter != optab.mnemonic.end())
			//{
			//	pair<int, int> opcode_format = optab.mnemonic[tmpMnemonic];
			//	if (line.format.size() == line.objectCode.size())//deal with format 3
			//	{
			//		line.loc.push_back(LOCCTR);
			//		line.format.push_back(opcode_format.second);
			//		LOCCTR += opcode_format.second;
			//	}
			//	if (opcode_format.second == 2)
			//		line.objectCode.push_back(opcode_format.first << 8);
			//	else if (opcode_format.second == 3)
			//		line.objectCode.push_back(opcode_format.first << 16);
			//	else if (opcode_format.second == 4)
			//		line.objectCode.push_back(opcode_format.first << 24);
			//}
			if (check) continue;
			//if statement[1] is directive
			HandleDirective(check, 1, line, LOCCTR, optab, statement, dirtab);
			//for (string directive : dirtab.directive)
			//{
			//	if (check = statement[1] == directive)
			//	{
			//		if (directive == "START")
			//		{
			//			line.format.push_back(-1);
			//			line.loc.push_back(LOCCTR);
			//			line.objectCode.push_back(-1);
			//			break;
			//		}
			//		else if (directive == "BASE")
			//		{
			//			line.format.push_back(-1);
			//			line.loc.push_back(-1);
			//			line.objectCode.push_back(-1);
			//			break;
			//		}
			//		else if (directive == "BYTE")
			//		{
			//			line.format.push_back(-1);
			//			line.loc.push_back(LOCCTR);
			//			if (statement[2][0] == 'C')
			//			{
			//				int tmpObcode = 0;
			//				LOCCTR += statement[2].size() - 3;
			//				for (string::iterator iter = statement[2].begin() + 2; iter != statement[2].end() - 1; ++iter)
			//				{
			//					tmpObcode <<= 8;
			//					tmpObcode += *iter;
			//				}
			//				line.objectCode.push_back(tmpObcode);
			//			}
			//			else if (statement[2][0] == 'X')
			//			{
			//				LOCCTR += (statement[2].size() - 3) / 2;
			//				string tmp(statement[2].begin() + 2, statement[2].end() - 1);
			//				int tmpObcode;
			//				istringstream iss(tmp);
			//				iss >> hex >> tmpObcode;
			//				line.objectCode.push_back(tmpObcode);
			//			}
			//			break;
			//		}
			//		else if (directive == "RESW")
			//		{
			//			line.format.push_back(-1);
			//			line.loc.push_back(LOCCTR);
			//			line.objectCode.push_back(-1);
			//			istringstream iss(statement[2]);
			//			int resw;
			//			iss >> resw;
			//			LOCCTR += 3 * resw;
			//			break;
			//		}
			//		else if (directive == "RESB")
			//		{
			//			line.format.push_back(-1);
			//			line.loc.push_back(LOCCTR);
			//			line.objectCode.push_back(-1);
			//			istringstream iss(statement[2]);
			//			int resb;
			//			iss >> resb;
			//			LOCCTR += resb;
			//			break;
			//		}
			//		else if (directive == "END")
			//		{
			//			line.format.push_back(-1);
			//			line.loc.push_back(-1);
			//			line.objectCode.push_back(-1);
			//			break;
			//		}
			//	}
			//}
		}
	}

	//This is the test
	for (int i = 0; i < line.statement.size(); ++i)
	{
		if (line.loc[i] == -1)
			cout << "\t";
		else
		{
			cout << setw(4) << hex << uppercase << setfill('0') << line.loc[i];
			cout << setfill(' ') << "\t";
			if (line.statement[i].size() == 3)
			{
				cout << left << setw(8) << line.statement[i][0] << setw(8) << line.statement[i][1] << setw(8) << line.statement[i][2];
			}
			else if (line.statement[i].size() == 2)
			{
				cout << left << setw(8) << " " << setw(8) << line.statement[i][0] << setw(8) << line.statement[i][1];
			}
			else if (line.statement[i].size() == 1)
			{
				cout << left << setw(8) << " " << setw(8) << line.statement[i][0];
			}
		}
		if (line.objectCode[i] != -1)
			cout << setw(4) << " " << hex << line.objectCode[i];
		cout << endl;
	}
	system("pause");
	return 0;
}