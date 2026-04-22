/**
* Text Fingerprinting with Jaccard Similarity
 * ===========================================
 *
 * This program implements a document similarity detection system using k-gram hashing.
 * It serves as a simplified version of a plagiarism detector by creating text fingerprints
 * and comparing them with Jaccard similarity.
 *
 * Functionality:
 * 1. Reads multiple text files as input
 * 2. Cleans and normalizes the text (lowercase, whitespace normalization)
 * 3. Tokenizes the text into words using regex
 * 4. Generates k-grams (sequences of k consecutive tokens)
 * 5. Hashes each k-gram using a polynomial rolling hash function
 * 6. Stores hashes in an unordered_set for efficient comparison
 * 7. Computes Jaccard similarity between document pairs
 * 8. Outputs a similarity matrix showing relationships between all documents
 *
 * The Jaccard similarity between two documents is calculated as:
 * J(A,B) = |A∩B| / |A∪B|
 * Where A and B are the sets of hashed k-grams from each document.
 *
 * Author: Lucm23
 * Course: CS 2413 - Data Structures
 * Project: Text Fingerprinting and Plagiarism Detection
 * Date: Spring 2025
 */


#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <regex>
#include <algorithm>
//#include <iomanip> // for precision
using namespace std;

// ---------------------------
// Step 1: Helper Functions
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

// Normalize text: convert to lowercase, remove extra spaces and line breaks
string normalizeText(const string& rawText) {
    string result = rawText;

    // Convert to lowercase
    transform(result.begin(), result.end(), result.begin(), ::tolower);

    // Replace multiple spaces with a single space using regex
    regex multipleSpaces("\\s+");
    result = regex_replace(result, multipleSpaces, " ");

    // Trim leading and trailing spaces
    regex leadingSpaces("^\\s+");
    regex trailingSpaces("\\s+$");
    result = regex_replace(result, leadingSpaces, "");
    result = regex_replace(result, trailingSpaces, "");

    return result;
}

// Read stopwords from file and store in unordered_set
unordered_set<string> readStopwords(const string& filename) {
    unordered_set<string> stopwords;
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Warning: Stopwords file not found. Proceeding without stopwords removal." << endl;
        return stopwords;
    }

    string word;
    while (file >> word) {
        stopwords.insert(word);
    }
    file.close();
    return stopwords;
}

// Tokenize text into words using regex
vector<string> tokenizeText(const string& text, const unordered_set<string>& stopwords = {}) {
    vector<string> tokens;
    regex wordPattern(R"([a-zA-Z0-9]+)");
    string s = text;
    smatch match;

    while (regex_search(s, match, wordPattern)) {
        string token = match.str(0);
        // Only add token if it's not a stopword
        if (stopwords.empty() || stopwords.find(token) == stopwords.end()) {
            tokens.push_back(token);
        }
        s = match.suffix();
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
        cout << "K-gram: " << kgram << endl;
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

//Print similarity matrix with precision.
void printSimilarityMatrix(const vector<unordered_set<unsigned long>>& allHashes, const vector<string>& fileNames) {
    cout << "\nSimilarity Matrix:" << endl;
    cout << "   ";
    for (const auto& name : fileNames) {
        cout << name << " ";
    }
    cout << endl;

    for (size_t i = 0; i < allHashes.size(); ++i) {
        cout << fileNames[i] << " ";
        for (size_t j = 0; j < allHashes.size(); ++j) {
            double similarity = computeJaccard(allHashes[i], allHashes[j]);
            // cout << fixed << setprecision(6) << similarity << " ";
            cout << similarity << " ";
        }
        cout << endl;
    }
}

// This a helper method to help us get the stopword output...
void processStopwordsOutput(const vector<string>& fileNames, int k, bool removeStopwords, const string& stopwordsFile)
{
    // 1. Load stopwords if requested
    unordered_set<string> stopwords;
    if (removeStopwords) {
        stopwords = readStopwords(stopwordsFile);
    }


    // 2. Print the appropriate header
    cout << "=== Similarity Matrix "
             << (removeStopwords ? "with" : "without")
             << " Stopwords Removed ===\n";

    // 3. For each file: tokenize (with or without stopwords), build k-grams, hash, collecting all sets
    vector<unordered_set<unsigned long>> allHashedKGrams;

    for (const string& filename : fileNames) {
        string rawText = readFile(filename);
        string cleanText = normalizeText(rawText);
        vector<string> tokens = tokenizeText(cleanText, stopwords);
        vector<string> kgrams = createKGrams(tokens, k);

        unordered_set<unsigned long> hashed = hashKGrams(kgrams);
        allHashedKGrams.push_back(hashed);
    }

    // Compute and display similarity matrix
    printSimilarityMatrix(allHashedKGrams, fileNames);
    cout << endl;
}

// ---------------------------
// Step 2: Main Program Logic
// ---------------------------

int main(int argc, char* argv[]) {
    // Parse input arguments or hardcode test filenames
    vector<string> fileNames = {"test-corpus/t1.txt", "test-corpus/t2.txt", "test-corpus/t3.txt", "test-corpus/t4.txt", "test-corpus/t5.txt", "test-corpus/t6.txt", "test-corpus/t7.txt", "test-corpus/t8.txt"};
    //vector<string> fileNames = {"test-corpus/tA.txt", "test-corpus/tB.txt"}; // just for debugging and testing
    int k =3; // Changed this value to 3, 5, or 7 for different k-gram analyses


    /*------ section (1):  For all other k-gram outputs without the Stopword-output------*/

    // // For stopwords removal (I set this to true for the bonus part)
    // bool removeStopwords = false;
    // unordered_set<string> stopwords;
    // if (removeStopwords) {
    //     stopwords = readStopwords("test-corpus/stopwords.txt");
    // }
    //
    // vector<unordered_set<unsigned long>> allHashedKGrams;
    //
    // for (const string& filename : fileNames) {
    //     //cout << "Processing file: " << filename << endl;
    //     string rawText = readFile(filename);
    //     string cleanText = normalizeText(rawText);
    //     vector<string> tokens = tokenizeText(cleanText, stopwords);
    //     vector<string> kgrams = createKGrams(tokens, k);
    //
    //     unordered_set<unsigned long> hashed = hashKGrams(kgrams);
    //     allHashedKGrams.push_back(hashed);
    // }
    //
    // // Compute and display similarity matrix
    // printSimilarityMatrix(allHashedKGrams, fileNames);


    /*------section (2): for 3-gram and the Stopword-output------*/

    // Pass 1: without stopwords removed
    processStopwordsOutput(fileNames, k, false, "test-corpus/stopwords.txt");

    // Pass 2: with stopwords removed
    processStopwordsOutput(fileNames, k, true, "test-corpus/stopwords.txt");

    return 0;
}