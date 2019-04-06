#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <ctype.h>
#include <random>
#include <ctime>
#include <cmath>
#include <utility>
#include <omp.h>
#include <cstring>

using namespace std;

//class definitions and operator overloading
class QuadGram {
public:
	mutable int score;
	string gram;
	int gramChk;//unique numeric representation of gram, to aid in std::unordered_map building and lookup

	QuadGram() {
		score = 1;
		gram = "";
		gramChk = -1;
	}

	void operator()(string g) {
		gram = g;
		score = 1;
		string conv = "";
		int a;
		for (int i = 0; i < (int)g.size(); i++) {
			a = g[i];
			conv += to_string(a);
		}

		gramChk = stoi(conv);
	}
};

bool operator < (const QuadGram &lhs, const QuadGram &rhs) {
	return lhs.gramChk < rhs.gramChk;
}

class DecryptCandidate {
public:
	double keyScore;
	double plainScore;
	string text;
	string key;
};

bool operator < (const DecryptCandidate &lhs, const DecryptCandidate &rhs) {
	return lhs.plainScore < rhs.plainScore;
}


//prototypes
bool IsNumeric(char c);
bool IsAlphabetic(char c);
bool BuildNewQuadGrams(unordered_map<string, QuadGram> *gram);
bool SortByScore(const QuadGram& lhs, const QuadGram& rhs);
string ApplyFennTranspose(const string &in, const int(&order)[80]);
string Decrypt(const string &cipher, const string &key);
long long ScoreCandidate(const string &in, unordered_map<string, QuadGram> &gram);
long long FinalScore(const string &transposed, const string &key, unordered_map<string, QuadGram> &gram);
double ChiSqTest(const string &in, const double(&counts)[26], bool plain);


int main() {
	const string FennCipher = "SVRWYULOKKOZMIEAYULFUTIITYWBLBHKAVCAZUAUMWXCLLQFRMJMYPJLSVLCUSOKLLICTXBXACUHRBVG";
	const int FennTrans[80] = { 41,10,73,22,80,28,76,6,32,53,61,19,1,36,56,23,65,40,13,67,29,2,21,45,39,3,79,51,49,24,
		27,11,17,69,4,71,50,5,30,57,25,7,60,35,12,20,8,70,52,26,62,42,14,54,31,9,33,72,55,37,
		15,58,75,66,43,74,68,59,16,46,34,18,77,63,44,64,78,38,47,48 };

    double FennLetterFreqs[26]; //custom fenn letter frequency distribution based on corpus.txt
    memset(FennLetterFreqs, 0, 26 * sizeof(double));

	string opt;
	ifstream quadGramIn;
	quadGramIn.open("QuadGrams.txt");
	//hash the QuadGrams for quick lookup
	unordered_map<string, QuadGram> dataGrams;

	if (quadGramIn.is_open()) {
		cout << "QuadGrams.txt file found." << endl << "Do you wish to generate a NEW QuadGram file from corpus.txt?" << endl << "  (Y or y to agree. Type anything else to skip)" << endl;
		cin >> opt;
		if (opt == "Y" || opt == "y") {
			cout << "Building new QuadGram database..." << endl;
			if (!BuildNewQuadGrams(&dataGrams)) {
				cout << "Exiting program." << endl;

				return 0;
			}
		}
		else { //load the QuadGrams via QuadGrams.txt
			cout << "Loading QuadGram database from QuadGrams.txt..." << endl;
			char c;
			QuadGram foo;
			string get = "";
			bool alt = true;

			c = quadGramIn.get();
			while (quadGramIn.good()) {

				if (IsAlphabetic(c) || IsNumeric(c)) {
					get += c;
				}
				else {
					if (alt) {
						foo(get);
						alt = false;
						get = "";
					}
					else {
						foo.score = stoi(get);
						alt = true;
						get = "";

						dataGrams.insert(make_pair(foo.gram, foo));
					}
				}

				c = quadGramIn.get();//put this last as eof will not be reached until eof character is extracted from stream
			}

			cout << "QuadGram database loaded: " << dataGrams.size() << " QuadGrams." << endl;
			quadGramIn.close();
		}
	}
	else {
		cout << "quadGrams.txt not found/corrupt. Building new QuadGram database from corpus.txt..." << endl;
		if (!BuildNewQuadGrams(&dataGrams)) {
			cout << "Exiting program." << endl;

			return 0;
		}
	}


	///// Build custom Fenn letter frequency count based on corpus.txt
	cout << "Building custom Fenn letter frequency based on corpus.txt" << endl;
    ifstream corp;
    char f;
    double ctr = 0.0;
    corp.open("corpus.txt");
    if(corp.is_open()) {
        while(corp.good()) {
            f = corp.get();
            if(IsAlphabetic(f)) {
                FennLetterFreqs[toupper(f) - 'A'] += 1.0;
                ctr += 1.0;
            }
        }
    }

    for(int i = 0; i < 26; i++) {
        FennLetterFreqs[i] = FennLetterFreqs[i] / ctr;
        cout << (char)('A' + i) << ": " << FennLetterFreqs[i] << endl;
    }
    corp.close();
    /////

	vector<DecryptCandidate> candList;
	int numThreads = 0;
	//determine how many iterations to run to
	long long limit = 30000000;// 3220000000 = 9235 seconds
	unsigned int seed = (unsigned int)time(0);
	cout << "Initial seed: " << seed << endl;
	unordered_map<string, QuadGram>::iterator gi;

	//need to have a vector of the dataGrams as the iterators are linear for unordered_map
	vector<string> dataGramList;
	for (gi = dataGrams.begin(); gi != dataGrams.end(); ++gi) {
		dataGramList.push_back(gi->first);
	}

#pragma omp parallel shared(candList)
	{

		if (omp_get_thread_num() == 0) {
			numThreads = omp_get_num_threads();
		}

		//need mersenne twister for long period length > 2^32. Each thread must have its own unique seed
		mt19937 rando(seed + omp_get_thread_num());

		int maxDataGrams = (int)dataGrams.size() - 1;
		QuadGram fetch;
		uniform_int_distribution<int> dist(0, maxDataGrams);
		string test, key;
		DecryptCandidate cand;
		vector<DecryptCandidate> _candList;
		double kScore = 0, pScore = 0;
		int num;
		vector<string>::iterator dgi;
		double least = 1000;

#pragma omp for
		for (long long i = 0; i < limit; i++) {
			if (i % 50000 == 0)
				cout << i << " ";


			key = "";
			//generate a 52 character key
			for (int j = 0; j < 13; j++) {
				dgi = dataGramList.begin();
				num = dist(rando);
				advance(dgi, num);
				key += *dgi;
			}

            //flesh out the full 80 character key needed
			key += key;
			key.erase(key.begin() + 80, key.end());

			//find the score of the key. the key should score reasonably high and if it doesnt, we shouldnt waste any more processing time
			//kScore = ScoreCandidate(key, dataGrams);
			kScore = ChiSqTest(key, FennLetterFreqs, false);
			if (kScore < 500) {
				test = Decrypt(FennCipher, key);
				if (test != "") {
					test = ApplyFennTranspose(test, FennTrans);

					//perform final scoring, in which both the key and the plaintext must score high,
					pScore = ChiSqTest(test, FennLetterFreqs, true);
					cand.plainScore = pScore;
					cand.text = test;
					cand.keyScore = kScore;
					cand.key = key;

					if(abs(pScore - kScore) < 20) {//the key and the plaintext must be similarly scored
                        if(_candList.size() < 1) {
                            _candList.push_back(cand);
                        } else {
                            if(cand.plainScore <= least) {
                                _candList.push_back(cand);
                                least = cand.plainScore;

                                if (_candList.size() > 1500) {
                                    sort(_candList.begin(), _candList.end());
                                    _candList.erase(_candList.begin() + (_candList.size() / 2),_candList.end());
                                }
                            }
                        }
					}


				}
			}


		}//end omp for


#pragma omp critical
		{
			candList.insert(candList.end(), _candList.begin(), _candList.end());
		}
	}

	cout << endl << endl << "Number of Threads: " << numThreads << endl;
	sort(candList.begin(), candList.end());

	string out = "";
	out += "  Initial Seed: ";
	out += to_string(seed);
	out += '\n';
	out += '\n';

	for (int i = 0; i < (int)candList.size(); i++) {
		out += "Plain Text Score: ";
		out += to_string(candList[i].plainScore);
		out += '\n';
		out += "Plaintext: ";
		out += '\n';
		out += candList[i].text;
		out += '\n';
		out += "Key Score: ";
		out += to_string(candList[i].keyScore);
		out += '\n';
		out += "Key: ";
		out += '\n';
		out += candList[i].key;
		out += '\n';
		out += '\n';
	}
	ofstream outFile("output.txt");
	if (outFile.is_open()) {
		outFile << out;
		cout << "Done. Output.txt file written." << endl;
	}




	return 0;
}

//check for good ascii and numeric characters
bool IsNumeric(char c) {
	if (isascii(c)) {
		if (c >= '0' && c <= '9')
			return true;
	}

	return false;
}

//determine if purely Alphabetic  (there still may be random hyphens or quotation marks not cleaned up
bool IsAlphabetic(char c) {
	if (isascii(c)) {
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
			return true;
	}

	return false;
}

//returns true on success
bool BuildNewQuadGrams(unordered_map<string, QuadGram> *gram) {
	QuadGram g;
	pair<unordered_map<string, QuadGram>::iterator, bool> res;
	unordered_map<string, QuadGram>::iterator gi;
	vector<QuadGram> bar;

	string strIn = "", foo, strOut = "";
	char c;
	ifstream input("corpus.txt");
	if (input.is_open()) {
		while (input.good()) {
			c = input.get();

			//dont store any numeric characters
			if (IsAlphabetic(c)) {
				c = toupper(c); //store in all uppercase
				strIn += c;

				if (strIn.size() > 3) {
					foo = "";
					for (int i = (int)strIn.size() - 4; i < (int)strIn.size(); i++) {
						foo += strIn[i];
					}

					g(foo);

					//attempt the insert
					res = gram->insert(make_pair(g.gram, g));
					//if unique, the QuadGram will successfully insert into unordered_map, if not, we increment the score of the quadgram that is already in the unordered_map
					if (!res.second) {
						gi = res.first;
						gi->second.score++;
					}
				}
			}
		}

		//save to QuadGrams.txt
		ofstream out("QuadGrams.txt");
		if (!out.is_open()) {
			cout << "Could not open QuadGrams.txt for writing!" << endl;
			input.close();
			return false;
		}

		//parse into vector for sorting
		for (gi = gram->begin(); gi != gram->end(); ++gi) {
			g = gi->second;
			bar.push_back(g);
		}
		sort(bar.begin(), bar.end(), SortByScore);

		for (int i = 0; i < (int)bar.size(); i++) {
			strOut += bar[i].gram;
			strOut += " ";//string delimited
			strOut += to_string(bar[i].score);
			strOut += " ";
		}

		out << strOut;
		out.close();

		cout << to_string(bar.size()) << " QuadGrams found with the highest score being: " << bar[0].score << " for the QuadGram: " << bar[0].gram << endl;
	}
	else {
		cout << "Could not open corpus.txt!" << endl;
		return false;
	}

	input.close();
	return true;
}

bool SortByScore(const QuadGram& lhs, const QuadGram& rhs) {
	return lhs.score > rhs.score;
}

string ApplyFennTranspose(const string &in, const int(&order)[80]) {
	string out = "";

	if (in.size() == 80) {
		for (int i = 0; i < (int)in.size(); i++) {
			out += in[order[i] - 1];
		}
	}
	else {
		cerr << "The std::string in parameter not 80 characters in length for ApplyFennTranspose()" << endl;
	}

	return out;
}

string Decrypt(const string &cipher, const string &key) {
	string out = "";
	char a, c, k;
	const char mod = 26;
	//ciphertext minus the key mod 26

	if (cipher.size() == key.size()) {
		for (int i = 0; i < (int)key.size(); i++) {
			c = cipher[i];
			k = key[i];
			a = (c - k + mod) % mod;
			a += 'A';
			out += a;
		}
	}
	else {
		cout << "Cipher and Key size mismatch in Decrypt()" << endl;
		return "";
	}

	return out;
}

long long ScoreCandidate(const string &in, unordered_map<string, QuadGram> &gram) {
	long long result = 0;
	string test;
	unordered_map<string, QuadGram>::iterator gi;

	for (int i = 0; i < (int)in.size() - 4; i++) {
		test = "";
		for (int j = 0; j < 4; j++) {
			test += in[i + j];
		}

		gi = gram.find(test);
		if (gi != gram.end()) {
			result += gi->second.score;
		}
	}

	return result;
}

long long FinalScore(const string &transposed, const string &key, unordered_map<string, QuadGram> &gram) {
	long long result = 0, addA = 0, addB;
	string test, keyT;
	unordered_map<string, QuadGram>::iterator gi;

	if (transposed.size() != key.size()) {
		cout << "Key and Plaintext size mismatch in FinalScore().";
		return 0;
	}

	for (int i = 0; i < (int)key.size() - 4; i++) {
		test = "";
		keyT = "";
		addA = 0;
		addB = 0;
		for (int j = 0; j < 4; j++) {
			test += transposed[i + j];
			keyT += key[i + j];
		}

		//score the transposed plaintext
		gi = gram.find(test);
		if (gi != gram.end()) {
			addA += gi->second.score;
		}
		//score the key
		gi = gram.find(keyT);
		if (gi != gram.end()) {
			addB += gi->second.score;
		}

		/*
		//this code is commented out because the fenn transpose makes it an invalid way to score...

		//calculate the final score, with respect to the position (ie, the score at this step is only high, if both are high at this same position in both strings)
		div = abs(addA - addB);
		if(div != 0) {
		result += (addA + addB) / div;
		} else {
		div = 1;
		result += (addA + addB) / div;
		}
		*/
	}

	result = addA + addB;

	return result;
}

double ChiSqTest(const string &in, const double(&counts)[26], bool plain) {
    double freqs[26];
    double scores[26];
    double length;

    //if this is plaintext use the full 80 letter phrase length
    if(plain)
        length = 80;
    else
        length = 52;

    /*
    //use cornell letter frequencies (consider using a custom fenn frequency?)
    freqs[0] = 0.0812 * length; //A
    freqs[1] = 0.0149 * length; //B
    freqs[2] = 0.0271 * length; //C
    freqs[3] = 0.0432 * length; //D
    freqs[4] = 0.1202 * length; //E
    freqs[5] = 0.0230 * length; //F
    freqs[6] = 0.0203 * length; //G
    freqs[7] = 0.0592 * length; //H
    freqs[8] = 0.0731 * length; //I
    freqs[9] = 0.0010 * length; //J
    freqs[10] = 0.0069 * length; //K
    freqs[11] = 0.0398 * length; //L
    freqs[12] = 0.0261 * length; //M
    freqs[13] = 0.0695 * length; //N
    freqs[14] = 0.0768 * length; //O
    freqs[15] = 0.0182 * length; //P
    freqs[16] = 0.0011 * length; //Q
    freqs[17] = 0.0602 * length; //R
    freqs[18] = 0.0628 * length; //S
    freqs[19] = 0.0910 * length; //T
    freqs[20] = 0.0288 * length; //U
    freqs[21] = 0.0111 * length; //V
    freqs[22] = 0.0209 * length; //W
    freqs[23] = 0.0017 * length; //X
    freqs[24] = 0.0211 * length; //Y
    freqs[25] = 0.0007 * length; //Z
    */

    //use fenn letter frequencies from corpus.txt
    for(int i = 0; i < 26; i++) {
        freqs[i] = counts[i] * length;
    }

    //initialize all as 0
    memset(scores, 0, 26 * sizeof(double));

    //get letter counts in the input phrase
    if(in.size() != 80)
        cerr << "ChiSqTest() was given a wrongly sized input string: " << in << endl;
    else {
        for(int i = 0; i < length; i++) {
            scores[toupper(in[i]) - 'A'] += 1.0;
        }
    }

    //calculate chi squared value
    double result = 0;
    for(int i = 0; i < 26; i++) {
        result += (pow(scores[i] - freqs[i], 2) / freqs[i]);
    }

    return result;
}
