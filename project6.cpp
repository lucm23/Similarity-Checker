/**
 * Code Fingerprinting with Jaccard Similarity for C++ Source Files
 * =================================================================
 *
 * This program implements a code similarity detection system for C++ source files.
 * It normalizes code formatting, removes comments, standardizes variable names,
 * and applies k-gram hashing to compute similarity between multiple source files.
 *
 * Functionality:
 * 1. Reads multiple C++ source files as input
 * 2. Normalizes code by removing extra spaces and comments
 * 3. Standardizes variable names to eliminate trivial differences
 * 4. Tokenizes the code into meaningful units
 * 5. Generates k-grams (sequences of k consecutive tokens)
 * 6. Hashes each k-gram using a polynomial rolling hash function
 * 7. Computes Jaccard similarity between file pairs
 * 8. Outputs a similarity matrix showing relationships between all files
 *
 * The Jaccard similarity between two files is calculated as:
 * J(A,B) = |A∩B| / |A∪B|
 * Where A and B are the sets of hashed k-grams from each file.
 *
 * Author: Lucm23
 * Course: CS 2413 - Data Structures
 * Project: Code Fingerprinting and Similarity Detection
 * Date: Spring 2025
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <regex>
#include <algorithm>
#include <iomanip>  // for setprecision
using namespace std;

// ---------------------------
// Step 1: Helper Functions for Code Normalization
// ---------------------------

// Read file content into a string
string readFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return "";
    }
    stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}

// Normalize spaces and empty lines in code
string normalizeSpacesAndLines(const string& code) {
    stringstream ss(code);
    string line, result;
    while (getline(ss, line)) {
        // Remove leading/trailing whitespace
        line = regex_replace(line, regex("^\\s+|\\s+$"), "");
        // Collapse multiple spaces/tabs into one space
        line = regex_replace(line, regex("[ \t]+"), " ");
        if (!line.empty()) {
            result += line + "\n";
        }
    }
    return result;
}

// Remove C++ comments (both single-line and multi-line)
string removeComments(const string& code) {
    // First remove multi-line comments
    regex multiLineComments(R"(/\*[\s\S]*?\*/)"/*, regex::dotall*/);
    string withoutMultiLine = regex_replace(code, multiLineComments, "");

    // Then remove single-line comments
    regex singleLineComments(R"(//[^\n]*)");
    string withoutComments = regex_replace(withoutMultiLine, singleLineComments, "");

    return withoutComments;
}

// Normalize variable names to standardized format (var1, var2, etc.)
unordered_map<string, string> variableMap;
int varCounter = 1;
string normalizeVariables(string code) {
    unordered_set<string> skipNames = {"main", "cout", "cin", "endl", "vector", "string", "bool", "char", "int", "float", "double", "return", "for", "if", "while"};

    // Find variable declarations
    regex declLinePattern(R"(\b(int|float|double|char|string|bool|vector|auto|size_t)\b\s+([^;=\)]+)[;=\)])");
    smatch match;
    string::const_iterator searchStart(code.cbegin());

    while (regex_search(searchStart, code.cend(), match, declLinePattern)) {
        //string type = match[1]; // int, float, etc.
        string varList = match[2]; // variable name or list of variables

        // Handle multiple variables in one declaration
        stringstream ss(varList);
        string token;
        while (getline(ss, token, ',')) {
            // Clean up whitespace and remove array brackets
            token = regex_replace(token, regex(R"(\[.*\])"), ""); // remove array specs
            token = regex_replace(token, regex(R"(^\s+|\s+$)"), ""); // trim

            // Extract just the variable name (no initializers)
            smatch varMatch;
            regex varNamePattern(R"(([a-zA-Z_][a-zA-Z0-9_]*))");
            if (regex_search(token, varMatch, varNamePattern)) {
                string varName = varMatch[1];
                if (!varName.empty() && skipNames.find(varName) == skipNames.end() && variableMap.find(varName) == variableMap.end()) {
                    variableMap[varName] = "var" + to_string(varCounter++);
                }
            }
        }

        searchStart = match.suffix().first;
    }

    // // Also catch for-loop variables
    // regex forLoopPattern(R"(\bfor\s*\(\s*(int|float|double|char|size_t)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*)");
    // searchStart = code.cbegin();
    // while (regex_search(searchStart, code.cend(), match, forLoopPattern)) {
    //     string varName = match[2];
    //     if (!varName.empty() && skipNames.find(varName) == skipNames.end() && variableMap.find(varName) == variableMap.end()) {
    //         variableMap[varName] = "var" + to_string(varCounter++);
    //     }
    //     searchStart = match.suffix().first;
    // }

    // Replace all variable names in code
    for (const auto& [original, normalized] : variableMap) {
        code = regex_replace(code, regex("\\b" + original + "\\b"), normalized);
    }

    return code;
}

// Tokenize code into meaningful units
vector<string> tokenize(const string& code) {
    vector<string> tokens;
    // Match string literals, identifiers, numbers, operators, and symbols
    regex pattern(R"((\".*?\")|([a-zA-Z_][a-zA-Z0-9_]*)|(\d+(\.\d+)?)|(\+\+|--|==|!=|<=|>=)|([=+\-*/%<>&|^!;:.,()[\]{}]))");

    smatch match;
    string input = code;
    while (regex_search(input, match, pattern)) {
        tokens.push_back(match.str(0));
        input = match.suffix().str();
    }

    return tokens;
}

// Create k-grams from tokens
vector<string> createKGrams(const vector<string>& tokens, int k) {
    vector<string> kgrams;

    if (tokens.size() < k) {
        return kgrams; // Not enough tokens to form k-grams
    }

    for (size_t i = 0; i <= tokens.size() - k; ++i) {
        stringstream kgram;
        for (int j = 0; j < k; ++j) {
            kgram << tokens[i + j];
            if (j < k - 1) {
                kgram << " ";
            }
        }
        kgrams.push_back(kgram.str());
    }

    return kgrams;
}

// Simple polynomial rolling hash function
unsigned long simpleHash(const string& s) {
    const int base = 257;
    const int mod = 1000000007;
    unsigned long hash = 0;
    for (char c : s) {
        hash = (hash * base + c) % mod;
    }
    return hash;
}

// Hash all k-grams and store in unordered_set
unordered_set<unsigned long> hashKGrams(const vector<string>& kgrams) {
    unordered_set<unsigned long> hashSet;

    for (const string& kgram : kgrams) {
        // Print each k-gram before hashing as specified in the requirements
        //cout << "K-gram: " << kgram << endl;
        unsigned long hash = simpleHash(kgram);
        hashSet.insert(hash);
    }

    return hashSet;
}

// Compute Jaccard similarity between two sets
double computeJaccard(const unordered_set<unsigned long>& A, const unordered_set<unsigned long>& B) {
    if (A.empty() && B.empty()) {
        return 1.0; // Both empty sets are considered identical
    }

    // Calculate intersection size
    int intersectionSize = 0;
    for (const auto& hashVal : A) {
        if (B.find(hashVal) != B.end()) {
            intersectionSize++;
        }
    }

    // Calculate union size: A.size() + B.size() - intersection size
    int unionSize = A.size() + B.size() - intersectionSize;

    return static_cast<double>(intersectionSize) / unionSize;
}

// Print similarity matrix with precision
void printSimilarityMatrix(const vector<unordered_set<unsigned long>>& allHashes, const vector<string>& fileNames) {
    // cout << "\nSimilarity Matrix:" << endl;
    cout << "\t";
    for (const auto& name : fileNames) {
        cout << name << " ";
    }
    cout << endl;

    for (size_t i = 0; i < allHashes.size(); ++i) {
        cout << fileNames[i] << " ";
        for (size_t j = 0; j < allHashes.size(); ++j) {
            double similarity = computeJaccard(allHashes[i], allHashes[j]);
            cout << fixed << setprecision(2) << similarity << " ";
            //cout << similarity << " ";
        }
        cout << endl;
    }
}


// ---------------------------
// Main Program Logic
// ---------------------------

int main(int argc, char* argv[]) {
    // Parse input arguments or hardcode test filenames
    vector<string> fileNames = {"test1.cpp", "test2.cpp", "test3.cpp", "test4.cpp", "test5.cpp", "test6.cpp"};
    int k = 3;
    vector<unordered_set<unsigned long>> allHashes;
    for (auto& fn : fileNames) {
        ifstream in(fn);
        if (!in) { cerr << "Cannot open " << fn << "\n"; continue; }
        string code((istreambuf_iterator<char>(in)), {});
        string clean = normalizeSpacesAndLines(code);
        clean = removeComments(clean);
        clean = normalizeVariables(clean);
        auto tok = tokenize(clean);
        cout << "Tokens for " << fn << ":\n";
        for (auto& t : tok) cout << t << " "; cout << "\n";
        allHashes.push_back(hashKGrams(createKGrams(tok, k)));
    }
    printSimilarityMatrix(allHashes, fileNames);
    return 0;
}

