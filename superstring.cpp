#include <iostream>
#include <vector>
#include <string>
#include <cstdio>
#include <map>
#include <regex>
#include <algorithm>
#include <iterator>
#include <set>
#include <cctype>
#include <fstream>

using namespace std;

map<string, bool> resultCache;
string outputFileName;

void ReplaceAll(string &str, const string& from, const string& to) {
    int pos = 0;
    while((pos = str.find(from, pos)) != string::npos) {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
}

bool satisfiesConstraints(const string &s, const vector<string> &t, const map<char, string> &charToString) {
	// Replace each t[0],t[1]...t[k] with the characters to be replaced, and then any characters
	// not yet given a replacement are replaced with a wildcard
	// Check if each of these are a substring of s
	// If any are not a substring of s, then the constraints are not satisfied and we return false
	// Otherwise return true

	for (int i = 0; i < t.size(); ++i) {
		string replaced = t[i];
		
		// Replace all the characters in the test string with its replacement, or a wildcard if not replaced yet
		// As we're using regex the wildcard is [a-z]+ (written in charToString)
		// e.g. ADB -> g*f -> ga[a-z]+f
		for (map<char, string>::const_iterator it = charToString.begin(); it != charToString.end(); ++it) {
			string c(1, it->first);
			ReplaceAll(replaced, c, it->second);
		}
		
		// Search the input string for the replaced string, to see if it is a candidate match
		bool regex_result;
		
		// We use a cache to speed up the process. Gives ~20x speedup on the given problems
		if (resultCache.find(replaced) != resultCache.end()) {
			regex_result = resultCache[replaced];
		} else {
			// Check if our new string with wildcards is a substring of s
			regex rgx(replaced.c_str());
			// Necessary?
			cmatch match;
			regex_result = regex_search(s.c_str(), match, rgx);
			// Add result to cache
			resultCache[replaced] = regex_result;
		}

		if (!regex_result) {
			return false;
		}
	}
	return true;
}

void writeToFile(map<char, string> m, const string &filename) {
	ofstream f(filename);
	for (map<char, string>::iterator it = m.begin(); it != m.end(); ++it) {
        f << it->first << ":" << it->second << endl;
    }
	f.close();
}

bool backtrack(const string& s, const vector<string> &t, map<char, string> charToString, map<char, vector<string> > &replaceChars) {
	if (!satisfiesConstraints(s, t, charToString))
		return false;

	// Find next unassigned letter
	map<char, string>::iterator unassignedLetterIT;

	for (unassignedLetterIT = charToString.begin(); unassignedLetterIT != charToString.end(); ++unassignedLetterIT) {
		if (unassignedLetterIT->second == "[a-z]+") {
			// Found the unassigned letter - end the loop
			break;
		}
	}

	// If no characters unassigned then the solution is found :)
	if (unassignedLetterIT == charToString.end()) {
		writeToFile(charToString, outputFileName);
		return true;
	}

	// Else try every combination of replacements for the next unchosen character
	// (rename c)
	char unassignedLetter = unassignedLetterIT->first;
	for (int i = 0; i < replaceChars[unassignedLetter].size(); ++i) {
		charToString[unassignedLetter] = replaceChars[unassignedLetter][i];
		bool result = backtrack(s, t, charToString, replaceChars);
		if (result) 
			return true;
	}
}

void printHelp() {
	cout << "Usage: superstring TEST.SWE" << endl;
}

bool isFileValid(ifstream& f) {
	// Throughout, check that all the SWE elements are on separate lines

	string s, k;
	getline(f, k);
	getline(f, s);
	
	// Check that there is a numerical value in the first line
	regex integer("[[:digit:]]+");
	if (!regex_match(k,integer)) {
		return false;
	}

	// Check that the second line is a string that only contains lower case alphabetical characters
	regex lowerCaseLetter("[a-z]+");
	if(!regex_match(s,lowerCaseLetter)){
		return false;
	}
	
	// Check that t[k] strings only contain upper and lower case alphabetical letters
	// Also save all the distinct uppercase characters 
	set<char> usedChars;
	int k_int = atoi(k.c_str());
	regex kStringChecks("[a-zA-Z]+");
	for (int i = 0; i < k_int; ++i) {
		string tmp;
		getline(f, tmp);
		for (int i = 0; i < tmp.size(); ++i) {
			if (isupper(tmp[i])) {
				usedChars.insert(tmp[i]);
			}
		}
		if (!regex_match(tmp,kStringChecks)){
			return false;
		}
	}
	
	// First character should be upper case. Second character should be :
	// There should not be any other characters than lower case alphabetical characters and commas
	// All commas should be preceded and succeeded by a lower case alphabetical character
	set<char> rj;
	regex rStringsChecks("[A-Z][:][a-z]+(,\[a-z\]+)*");
	string r;
	while(getline(f, r)){
		if (!regex_match(r,rStringsChecks)){
			return false;
		}
		
		char c = r[0];
		if(rj.find(c) != rj.end()){
			return false;
		}
		rj.insert(c);
	}

	// All upper case letters in the t[k] strings should be given candidate replacement characters
	for (set<char>::iterator it = usedChars.begin(); it != usedChars.end(); ++it) {
		if (rj.find(*it) == rj.end()) {
			return false;
		}
	}
	
	
	return true;
}

int main(int argc, char* argv[]){
	if (argc != 2) {
		printHelp();
		return 0;
	}

	// Check that the file is valid
	ifstream checkInputFile(argv[1]);
	if (!isFileValid(checkInputFile)) {
		cout << "Input SWE file is invalid" << endl;
		return 0;
	}

	ifstream inputFile(argv[1]);
	outputFileName = string(argv[1]);
	outputFileName = outputFileName.substr(0, outputFileName.length() - 4) + ".SOL"; 
	
	// get input
	int k;
	string s;
	inputFile >> k >> s;

	// get the strings to be replaced
	set<char> usedChars;
	vector<string> t;
	for (int i = 0; i < k; ++i) {
		string tmp;
		inputFile >> tmp;
		if (find(t.begin(), t.end(), tmp) == t.end()) {
			t.push_back(tmp);
			// save each character used to check if the later character replacements are used
			for (int j = 0; j < tmp.size(); ++j) {
				if (isupper(tmp[j])) {
					usedChars.insert(tmp[j]);
				}
			}
		}
	}
	
	// get the replacement data
	map<char, vector<string> > replaceChars;
	string line, token;
	getline(inputFile, line); // eat up the newline
	size_t pos = 0;
	char c, colon;
	while (inputFile >> c >> colon) {
		getline(inputFile, line);
		vector<string> replaceTarget;
		if (usedChars.find(c) == usedChars.end()) {
			replaceTarget.push_back("UNUSED");
		} else {
			while ((pos = line.find(",")) != string::npos) {
			    token = line.substr(0, pos);
				// Only add a new string if it is a substring of s already
				if (s.find(token) != string::npos) {
					replaceTarget.push_back(token);
				}
	    		line.erase(0, pos + 1);
			}
			replaceTarget.push_back(line);
		}
		replaceChars[c] = replaceTarget;
	}

	// Our matching of character to strings
	map<char, string> charToString;
	for (map<char, vector<string> >::iterator it = replaceChars.begin(); it != replaceChars.end(); ++it) {
		if (usedChars.find(it->first) == usedChars.end()) {
			charToString[it->first] = "UNUSED";
		} else {
			// Regular expression wildcard
			charToString[it->first] = "[a-z]+";
		}
	}

	if (backtrack(s, t, charToString, replaceChars)) {
		cout << "YES" << endl;
	} else {
		cout << "NO" << endl;
	}

	return 0;
}
