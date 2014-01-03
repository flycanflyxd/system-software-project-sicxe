#include <iostream>
#include <regex>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <utility>
#include <cmath>

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
			fin >> tmpMnemonic >> hex >> tmpOpcode >> dec >> tmpFormat;
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
		if (!fin || fin.bad())
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
	vector<unsigned int> loc;
	vector<int> objectCode;
	vector<int> format;
};

class REGTAB
{
public:
	REGTAB()
	{
		reg['A'] = 0x0;
		reg['X'] = 0x1;
		reg['S'] = 0x4;
		reg['T'] = 0x5;
	}
	map<char, int> reg;
};

void HandleComment(Line &line)
{
	line.loc.push_back(-1);
	line.objectCode.push_back(-1);
	line.format.push_back(-1);
}

bool HandleInstruction(const int mnemonicPosition, Line &line, unsigned int &LOCCTR, OPTAB &optab, vector<string> &statement)
{
	bool check = false;
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
		if (line.format.size() == line.objectCode.size())//deal with format 2 and 3
		{
			line.loc.push_back(LOCCTR);
			line.format.push_back(opcode_format.second);
			LOCCTR += opcode_format.second;
		}
		else
			opcode_format.second = 4;
		if (opcode_format.second == 2)
			line.objectCode.push_back(opcode_format.first << 8);
		else if (opcode_format.second == 3)
			line.objectCode.push_back(opcode_format.first << 16);
		else if (opcode_format.second == 4)
			line.objectCode.push_back(opcode_format.first << 24);
	}
	return check;
}

bool HandleDirective(const int directivePosition, Line &line, unsigned int &LOCCTR, OPTAB &optab, vector<string> &statement, DIRTAB &dirtab)
{
	bool check = false;
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
	return check;
}

bool ImmediateAddressing(int& objectCode, const vector<string>& statement, map<string, int>& label, const int& loc, const int& format, const pair<string, int>& base)
{
	format == 3 ? objectCode |= 0x010000 : objectCode |= 0x01000000;
	string tmp(statement.back());
	tmp.erase(tmp.begin());
	if (statement.back()[1] >= '0' && statement.back()[1] <= '9')//operand is an int
	{
		int immediate;
		istringstream iss(tmp);
		iss >> immediate;
		objectCode += immediate;
	}
	else//operand is a label
	{
		int disp = label[tmp] - loc - format;
		if (disp <= 2047 && disp >= -2048)//program counter relative
		{
			if(format == 3)
				disp >= 0 ? objectCode |= 0x002000 : objectCode |= 0x003000;
			else
				disp >= 0 ? objectCode |= 0x00200000 : objectCode |= 0x00300000;
		}
		else//base relative
		{
			format == 3 ? objectCode |= 0x004000 : objectCode |= 0x00400000;
			disp = label[statement.back()] - base.second;
			if (disp < 0 || disp > 4095)//out of range
			{
				cerr << "Error";
			}
		}
		objectCode += disp;
	}
	return true;
}

bool IndirectAddressing(int& objectCode, const vector<string>& statement, Line& line, map<string, int>& label, const int& loc, const int& format, const pair<string, int>& base)
{
	format == 3 ? objectCode |= 0x020000 : objectCode |= 0x02000000;
	string tmp(statement.back());
	tmp.erase(tmp.begin());
	for (int j = 0; j < line.statement.size(); ++j)
	{
		if (line.statement[j][0] == tmp)
		{
			int disp = line.loc[j] - loc - format;
			if (disp <= 2047 && disp >= -2048)//program counter relative
			{
				if (format == 3)
					disp >= 0 ? objectCode |= 0x002000 : objectCode |= 0x003000;
				else
					disp >= 0 ? objectCode |= 0x00200000 : objectCode |= 0x00300000;
			}
			else//base relative
			{
				format == 3 ? objectCode |= 0x004000 : objectCode |= 0x00400000;
				disp = label[statement.back()] - base.second;
				if (disp < 0 || disp > 4095)//out of range
				{
					cerr << "Error";
				}
			}
			int value;
			tmp = line.statement[j][2];
			istringstream iss(tmp);
			iss >> value;
			line.statement[j][1] == "RESB" ? objectCode += value : objectCode += 3 * value;//one word = three-byte
		}
	}
	return  true;
}

bool SimpleAddressing(int& objectCode, const vector<string>& statement, map<string, int>& label, const int& loc, const int& format, const pair<string, int>& base)
{
	int disp;
	format == 3 ? objectCode |= 0x030000 : objectCode |= 0x03000000;
	if (format == 3)
	{
		if (statement.size() == 1) return false;//just in case that if there is no any augument
		string tmp(statement.back());
		if (statement.back().back() == 'X')
		{
			format == 3 ? objectCode |= 0x008000 : objectCode |= 0x00800000;
			tmp.erase(tmp.end() - 2, tmp.end());
		}
		statement.size() == 1 ? disp = 0 : disp = label[tmp] - loc - format;
		if (disp <= 2047 && disp >= -2048)//program counter relative
		{
			if (format == 3)
				disp >= 0 ? objectCode |= 0x002000 : objectCode |= 0x003000;
			else
				disp >= 0 ? objectCode |= 0x00200000 : objectCode |= 0x00300000;
		}
		else//base relative
		{
			format == 3 ? objectCode |= 0x004000 : objectCode |= 0x00400000;
			disp = label[tmp] - base.second;
			if (disp < 0 || disp > 4095)//out of range
			{
				cerr << "Error";
			}
		}
	}
	else
		disp = label[statement.back()];
	objectCode += disp;
	return true;
}

int main()
{
	OPTAB optab;
	SYMTAB symtab;
	DIRTAB dirtab;
	REGTAB regtab;
	Line line;
	unsigned int LOCCTR = 0;
	string fileName = "SICXE.txt";
	//cout << "Please input the file name: ";
	//cin >> fileName;
	line.read(fileName);
	//==================pass one=======================
	for (auto statement : line.statement)
	{
		if (statement[0][0] == '.')//deal with comment
			HandleComment(line);
		else
		{
			//if statement[0] is mnemonic
			if (HandleInstruction(0, line, LOCCTR, optab, statement)) continue;
			//if statement[0] is directive
			if (HandleDirective(0, line, LOCCTR, optab, statement, dirtab)) continue;
			//if statement[0] is label
			symtab.add(statement[0], LOCCTR);
			//if statement[1] is mnemonic
			if (HandleInstruction(1, line, LOCCTR, optab, statement)) continue;
			//if statement[1] is directive
			HandleDirective(1, line, LOCCTR, optab, statement, dirtab);
		}
	}
	//==================pass two=======================
	pair<string, int> base;
	for (int i = 0; i < line.statement.size(); ++i)
	{
		if (line.statement[i][0] == "BASE")//set base
		{
			base.first = line.statement[i][1];
			base.second = symtab.label[base.first];
		}
		if (line.format[i] == 2)//format 2
		{
			int r = 0;
			if (line.statement[i].back().size() == 1)
				r = regtab.reg[line.statement[i].back().front()] << 4;
			else
				r = (regtab.reg[line.statement[i].back().front()] << 4) + regtab.reg[line.statement[i].back().back()];
			line.objectCode[i] += r;
		}
		else if (line.format[i] == 3)//format 3
		{
			if (line.statement[i].back()[0] == '#')//immediate addressing
				ImmediateAddressing(line.objectCode[i], line.statement[i], symtab.label, line.loc[i], line.format[i], base);
			else if (line.statement[i].back()[0] == '@')//indirect addressing
			{
				IndirectAddressing(line.objectCode[i], line.statement[i], line, symtab.label, line.loc[i], line.format[i], base);
			}
			else//simple addressing
			{
				SimpleAddressing(line.objectCode[i], line.statement[i], symtab.label, line.loc[i], line.format[i], base);
			}
		}
		else if (line.format[i] == 4)//format 4
		{
			line.objectCode[i] |= 0x00100000;
			if (line.statement[i].back()[0] == '#')
			{
				ImmediateAddressing(line.objectCode[i], line.statement[i], symtab.label, line.loc[i], line.format[i], base);
			}
			else if (line.statement[i].back()[0] == '@')
			{
				IndirectAddressing(line.objectCode[i], line.statement[i], line, symtab.label, line.loc[i], line.format[i], base);
			}
			else
				SimpleAddressing(line.objectCode[i], line.statement[i], symtab.label, line.loc[i], line.format[i], base);
		}
	}
	//print T record
	//must to refactor it!!!!!!
	//freopen("Result.txt", "w", stdout);
	cout << hex << "H" << left << setw(6) << line.statement[0].front() << setfill('0') << setw(6) << right << line.loc[0];
	int tmp = line.objectCode.back() << 4;
	int counter = 0;
	int i = 0;
	int j = 0;//end
	int k = 0;//new start
	int l = 0;//start
	while (tmp != 0)
	{
		tmp /= 256;
		++counter;
	}
	cout << hex << setw(6) << line.loc[line.loc.size() - 2] + counter << endl;
	while (i != line.statement.size())
	{
		counter = 0;
		for (i = k; i < line.statement.size(); ++i)
		{
			if (line.format[i] != -1)
			{
				if (counter + line.format[i] > 0x1D)
				{
					j = i - 1;
					l = k;
					k = i;
					break;
				}
				counter += line.format[i];
			}
			else if (line.objectCode[i] != -1)
			{
				tmp = line.objectCode[i];
				int tmpCounter = 0;
				while (tmp != 0)
				{
					tmp /= 256;
					++tmpCounter;
				}
				if (counter + tmpCounter > 0x1D)
				{
					j = i - 1;
					l = k;
					k = i;
					break;
				}
				counter += tmpCounter;
			}
			if (line.statement[i].size() > 1 && (line.statement[i][line.statement[i].size() - 2] == "RESB" || line.statement[i][line.statement[i].size() - 2] == "RESW"))
			{
				j = i - 1;
				while (line.statement[i][0][0] == '.' || line.statement[i].size() > 1 && (line.statement[i][line.statement[i].size() - 2] == "RESB" || line.statement[i][line.statement[i].size() - 2] == "RESW"))
				{
					++i;
				}
				l = k;
				k = i;
				break;
			}
			if (line.statement[i].front() == "END")
			{
				j = i;
				l = k;
				break;
			}
		}
		cout << "T" << uppercase << setfill('0') << hex << setw(6) <<line.loc[l] << setw(2) << counter;
		for (i = l; i <= j; ++i)
		{
			if (line.format[i] == 2)
				cout << uppercase << setfill('0') << hex << setw(4) << line.objectCode[i];
			else if (line.format[i] == 3)
				cout << uppercase << setfill('0') << hex << setw(6) << line.objectCode[i];
			else if (line.format[i] == 4)
				cout << uppercase << setfill('0') << hex << setw(8) << line.objectCode[i];
			else if (line.objectCode[i] != -1)
			{
				int tmpCounter = 0;
				tmp = line.objectCode[i];
				while (tmp != 0)
				{
					tmp /= 256;
					++tmpCounter;
				}
				cout << uppercase << setfill('0') << hex << setw(2 * tmpCounter) << line.objectCode[i];
			}
		}
		cout << endl;
	}
	for (int i = 0; i < line.statement.size(); ++i)
	{
		if (line.format[i] == 4 && line.statement[i].back().front() >= 'A' && line.statement[i].back().front() <= 'Z')
		{
			cout << hex << uppercase << setfill('0') << "M" << setw(6) << line.loc[i] + 1 << "05\n";
		}
		if (line.format[i] == 3 && (line.objectCode[i] & 0x010000) == true)
		{
			cout << hex << uppercase << setfill('0') << "M" << setw(6) << line.loc[i] << "06\n";
		}
	}
	cout << "E" << setw(6) << line.loc[0];
	system("pause");
	return 0;
}