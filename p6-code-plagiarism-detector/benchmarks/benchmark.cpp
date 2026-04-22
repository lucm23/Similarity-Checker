/**
 * Benchmark Harness for C++ Plagiarism Detection Pipeline
 * ========================================================
 * Measures: corpus scale, wall-clock time, peak memory,
 *           k-gram stats, brute-force vs. hashing speedup, accuracy.
 *
 * Compile: g++ -O2 -std=c++20 -o benchmark benchmark.cpp
 * Run:     ./benchmark   (from the p6 directory so test*.cpp are found)
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
#include <iomanip>
#include <chrono>
#include <cmath>
#include <numeric>
#include <set>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

using namespace std;
namespace fs = std::filesystem;

// ================================================
//  UTILITIES — memory measurement
// ================================================
size_t getPeakMemoryKB() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return pmc.PeakWorkingSetSize / 1024;
#endif
    return 0;
}

// ================================================
//  PIPELINE FUNCTIONS (copied from project6.cpp)
// ================================================

string readFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) { cerr << "Error opening file: " << filename << endl; return ""; }
    stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}

string normalizeSpacesAndLines(const string& code) {
    stringstream ss(code);
    string line, result;
    while (getline(ss, line)) {
        line = regex_replace(line, regex("^\\s+|\\s+$"), "");
        line = regex_replace(line, regex("[ \\t]+"), " ");
        if (!line.empty()) result += line + "\n";
    }
    return result;
}

string removeComments(const string& code) {
    regex multiLineComments(R"(/\*[\s\S]*?\*/)");
    string withoutMultiLine = regex_replace(code, multiLineComments, "");
    regex singleLineComments(R"(//[^\n]*)");
    return regex_replace(withoutMultiLine, singleLineComments, "");
}

// Per-run variable map — we reset between benchmark iterations
unordered_map<string, string> variableMap;
int varCounter = 1;

void resetVariableMap() {
    variableMap.clear();
    varCounter = 1;
}

string normalizeVariables(string code) {
    unordered_set<string> skipNames = {"main","cout","cin","endl","vector","string",
                                       "bool","char","int","float","double","return","for","if","while"};
    regex declLinePattern(R"(\b(int|float|double|char|string|bool|vector|auto|size_t)\b\s+([^;=\)]+)[;=\)])");
    smatch match;
    string::const_iterator searchStart(code.cbegin());
    while (regex_search(searchStart, code.cend(), match, declLinePattern)) {
        string varList = match[2];
        stringstream ss(varList);
        string token;
        while (getline(ss, token, ',')) {
            token = regex_replace(token, regex(R"(\[.*\])"), "");
            token = regex_replace(token, regex(R"(^\s+|\s+$)"), "");
            smatch varMatch;
            regex varNamePattern(R"(([a-zA-Z_][a-zA-Z0-9_]*))");
            if (regex_search(token, varMatch, varNamePattern)) {
                string varName = varMatch[1];
                if (!varName.empty() && skipNames.find(varName) == skipNames.end()
                    && variableMap.find(varName) == variableMap.end()) {
                    variableMap[varName] = "var" + to_string(varCounter++);
                }
            }
        }
        searchStart = match.suffix().first;
    }
    for (const auto& [original, normalized] : variableMap)
        code = regex_replace(code, regex("\\b" + original + "\\b"), normalized);
    return code;
}

vector<string> tokenize(const string& code) {
    vector<string> tokens;
    regex pattern(R"((\".*?\")|([a-zA-Z_][a-zA-Z0-9_]*)|(\d+(\.\d+)?)|(\+\+|--|==|!=|<=|>=)|([=+\-*/%<>&|^!;:.,()[\]{}]))");
    smatch match;
    string input = code;
    while (regex_search(input, match, pattern)) {
        tokens.push_back(match.str(0));
        input = match.suffix().str();
    }
    return tokens;
}

vector<string> createKGrams(const vector<string>& tokens, int k) {
    vector<string> kgrams;
    if ((int)tokens.size() < k) return kgrams;
    for (size_t i = 0; i <= tokens.size() - k; ++i) {
        stringstream kgram;
        for (int j = 0; j < k; ++j) {
            kgram << tokens[i + j];
            if (j < k - 1) kgram << " ";
        }
        kgrams.push_back(kgram.str());
    }
    return kgrams;
}

unsigned long simpleHash(const string& s) {
    const int base = 257;
    const int mod = 1000000007;
    unsigned long hash = 0;
    for (char c : s) hash = (hash * base + c) % mod;
    return hash;
}

unordered_set<unsigned long> hashKGrams(const vector<string>& kgrams) {
    unordered_set<unsigned long> hashSet;
    for (const string& kgram : kgrams)
        hashSet.insert(simpleHash(kgram));
    return hashSet;
}

double computeJaccard(const unordered_set<unsigned long>& A, const unordered_set<unsigned long>& B) {
    if (A.empty() && B.empty()) return 1.0;
    int intersectionSize = 0;
    for (const auto& h : A)
        if (B.count(h)) intersectionSize++;
    int unionSize = (int)A.size() + (int)B.size() - intersectionSize;
    return (double)intersectionSize / unionSize;
}

// ================================================
//  BRUTE-FORCE BASELINE  (O(n²) string comparison)
// ================================================
// Compare two normalized code strings using exact k-gram STRING matching
// (no hashing — store actual strings in sets and compute Jaccard on strings).
double bruteForceJaccard(const vector<string>& kgramsA, const vector<string>& kgramsB) {
    set<string> setA(kgramsA.begin(), kgramsA.end());
    set<string> setB(kgramsB.begin(), kgramsB.end());
    if (setA.empty() && setB.empty()) return 1.0;
    int inter = 0;
    for (const auto& s : setA)
        if (setB.count(s)) inter++;
    int uni = (int)setA.size() + (int)setB.size() - inter;
    return (double)inter / uni;
}

// ================================================
//  SYNTHETIC FILE GENERATOR  (for scale testing)
// ================================================
string generateSyntheticCpp(int id, int targetLines) {
    // Generate a realistic-looking C++ file with the given number of lines
    ostringstream out;
    out << "// Synthetic file " << id << "\n";
    out << "#include <iostream>\n";
    out << "#include <vector>\n";
    out << "#include <string>\n";
    out << "using namespace std;\n\n";

    int linesWritten = 6;
    int funcId = 0;

    while (linesWritten < targetLines) {
        string varA = "val" + to_string(id) + "_" + to_string(funcId);
        string varB = "res" + to_string(id) + "_" + to_string(funcId);
        out << "int func" << funcId << "_" << id << "(int " << varA << ") {\n";
        out << "    int " << varB << " = " << varA << " * " << (id + funcId + 2) << ";\n";
        out << "    for (int i = 0; i < " << varA << "; i++) {\n";
        out << "        " << varB << " += i;\n";
        out << "        if (" << varB << " > 100) {\n";
        out << "            " << varB << " = " << varB << " % 100;\n";
        out << "        }\n";
        out << "    }\n";
        out << "    return " << varB << ";\n";
        out << "}\n\n";
        linesWritten += 11;
        funcId++;
    }

    out << "int main() {\n";
    out << "    cout << func0_" << id << "(10) << endl;\n";
    out << "    return 0;\n";
    out << "}\n";
    return out.str();
}

// ================================================
//  MAIN — benchmark driver
// ================================================
int main() {
    cout << "========================================================\n";
    cout << "  C++ Plagiarism Detection — Benchmark Report\n";
    cout << "========================================================\n\n";

    const int k = 3;  // k-gram parameter used in the project

    // ------------------------------------------------------------------
    // 1.  LOAD REAL TEST FILES
    // ------------------------------------------------------------------
    vector<string> realFiles = {"../test-corpus/test1.cpp","../test-corpus/test2.cpp","../test-corpus/test3.cpp","../test-corpus/test4.cpp","../test-corpus/test5.cpp","../test-corpus/test6.cpp"};
    vector<string> rawContents;
    int totalLinesReal = 0;
    for (auto& fn : realFiles) {
        string c = readFile(fn);
        rawContents.push_back(c);
        // count lines
        int nl = 0;
        for (char ch : c) if (ch == '\n') nl++;
        totalLinesReal += nl;
    }
    double avgLinesReal = (double)totalLinesReal / realFiles.size();

    cout << "[1] CORPUS SCALE\n";
    cout << "    Real test files loaded  : " << realFiles.size() << "\n";
    cout << "    Total lines (real)      : " << totalLinesReal << "\n";
    cout << "    Average file size       : " << fixed << setprecision(1) << avgLinesReal << " LOC\n";

    // ------------------------------------------------------------------
    // 2.  SYNTHETIC SCALE TEST  — push to 50 files
    // ------------------------------------------------------------------
    const int NUM_SYNTHETIC = 50;
    const int SYNTHETIC_LINES = 50;   // ~50 LOC each → realistic student submissions
    vector<string> synthContents;
    for (int i = 0; i < NUM_SYNTHETIC; i++)
        synthContents.push_back(generateSyntheticCpp(i, SYNTHETIC_LINES));

    cout << "    Synthetic files created : " << NUM_SYNTHETIC << " (each ~" << SYNTHETIC_LINES << " LOC)\n";
    cout << "    Max single-run corpus   : " << NUM_SYNTHETIC << " files\n\n";

    // ------------------------------------------------------------------
    // 3.  PERFORMANCE BENCHMARK  (hashing approach on 6 & 50 files)
    // ------------------------------------------------------------------
    cout << "[2] PERFORMANCE BENCHMARK\n";

    // --- 6 real files ---
    auto t0 = chrono::high_resolution_clock::now();
    resetVariableMap();
    vector<unordered_set<unsigned long>> hashSets6;
    vector<vector<string>> allKgrams6;     // save for brute-force later
    vector<int> kgramCounts6;
    for (auto& raw : rawContents) {
        string clean = normalizeSpacesAndLines(raw);
        clean = removeComments(clean);
        clean = normalizeVariables(clean);
        auto tok = tokenize(clean);
        auto kg  = createKGrams(tok, k);
        kgramCounts6.push_back((int)kg.size());
        allKgrams6.push_back(kg);
        hashSets6.push_back(hashKGrams(kg));
    }
    // pairwise Jaccard
    int pairs6 = 0;
    for (size_t i = 0; i < hashSets6.size(); i++)
        for (size_t j = i+1; j < hashSets6.size(); j++) {
            computeJaccard(hashSets6[i], hashSets6[j]);
            pairs6++;
        }
    auto t1 = chrono::high_resolution_clock::now();
    double ms6 = chrono::duration<double, milli>(t1 - t0).count();

    cout << "    6 real files  (" << pairs6 << " pairs)  : "
         << fixed << setprecision(2) << ms6 << " ms\n";

    // --- 50 synthetic files ---
    auto t2 = chrono::high_resolution_clock::now();
    resetVariableMap();
    vector<unordered_set<unsigned long>> hashSets50;
    vector<vector<string>> allKgrams50;
    for (auto& raw : synthContents) {
        string clean = normalizeSpacesAndLines(raw);
        clean = removeComments(clean);
        clean = normalizeVariables(clean);
        auto tok = tokenize(clean);
        auto kg  = createKGrams(tok, k);
        allKgrams50.push_back(kg);
        hashSets50.push_back(hashKGrams(kg));
    }
    int pairs50 = 0;
    for (size_t i = 0; i < hashSets50.size(); i++)
        for (size_t j = i+1; j < hashSets50.size(); j++) {
            computeJaccard(hashSets50[i], hashSets50[j]);
            pairs50++;
        }
    auto t3 = chrono::high_resolution_clock::now();
    double ms50 = chrono::duration<double, milli>(t3 - t2).count();

    cout << "    50 synth files (" << pairs50 << " pairs) : "
         << fixed << setprecision(2) << ms50 << " ms\n\n";

    // ------------------------------------------------------------------
    // 4.  MEMORY USAGE
    // ------------------------------------------------------------------
    size_t peakKB = getPeakMemoryKB();
    cout << "[3] PEAK MEMORY\n";
    if (peakKB > 0)
        cout << "    Peak working set : " << fixed << setprecision(2) << (peakKB / 1024.0) << " MB\n\n";
    else
        cout << "    (Memory measurement not available on this platform)\n\n";

    // ------------------------------------------------------------------
    // 5.  K-GRAM PARAMETERS
    // ------------------------------------------------------------------
    cout << "[4] K-GRAM PARAMETERS\n";
    cout << "    k value              : " << k << "\n";
    double avgKgrams6 = 0;
    for (auto c : kgramCounts6) avgKgrams6 += c;
    avgKgrams6 /= kgramCounts6.size();
    cout << "    k-grams per file (real, avg) : " << fixed << setprecision(1) << avgKgrams6 << "\n";

    // Also compute for synthetic
    double avgKgramsSynth = 0;
    for (auto& kg : allKgrams50) avgKgramsSynth += kg.size();
    avgKgramsSynth /= allKgrams50.size();
    cout << "    k-grams per file (synth,avg) : " << fixed << setprecision(1) << avgKgramsSynth << "\n";
    cout << "    Hash function        : polynomial rolling (base=257, mod=10^9+7)\n";
    cout << "    Hash storage         : unordered_set<unsigned long>\n\n";

    // ------------------------------------------------------------------
    // 6.  SPEEDUP vs. BRUTE-FORCE
    // ------------------------------------------------------------------
    cout << "[5] SPEEDUP vs. BRUTE-FORCE BASELINE\n";
    cout << "    Baseline: O(n^2) string comparison using std::set<string> Jaccard\n";

    // Brute-force on 6 real files
    auto bf0 = chrono::high_resolution_clock::now();
    for (size_t i = 0; i < allKgrams6.size(); i++)
        for (size_t j = i+1; j < allKgrams6.size(); j++)
            bruteForceJaccard(allKgrams6[i], allKgrams6[j]);
    auto bf1 = chrono::high_resolution_clock::now();
    double bfMs6 = chrono::duration<double, milli>(bf1 - bf0).count();

    // Hash-based on 6 real files (just the Jaccard part, hashes already built)
    auto hb0 = chrono::high_resolution_clock::now();
    for (size_t i = 0; i < hashSets6.size(); i++)
        for (size_t j = i+1; j < hashSets6.size(); j++)
            computeJaccard(hashSets6[i], hashSets6[j]);
    auto hb1 = chrono::high_resolution_clock::now();
    double hbMs6 = chrono::duration<double, milli>(hb1 - hb0).count();

    cout << "    6 files — brute-force Jaccard : " << fixed << setprecision(3) << bfMs6 << " ms\n";
    cout << "    6 files — hash-based  Jaccard : " << fixed << setprecision(3) << hbMs6 << " ms\n";
    if (hbMs6 > 0)
        cout << "    Speedup ratio (6 files)       : " << fixed << setprecision(1) << (bfMs6 / hbMs6) << "x\n";

    // Brute-force on 50 synthetic files
    auto bf2 = chrono::high_resolution_clock::now();
    for (size_t i = 0; i < allKgrams50.size(); i++)
        for (size_t j = i+1; j < allKgrams50.size(); j++)
            bruteForceJaccard(allKgrams50[i], allKgrams50[j]);
    auto bf3 = chrono::high_resolution_clock::now();
    double bfMs50 = chrono::duration<double, milli>(bf3 - bf2).count();

    auto hb2 = chrono::high_resolution_clock::now();
    for (size_t i = 0; i < hashSets50.size(); i++)
        for (size_t j = i+1; j < hashSets50.size(); j++)
            computeJaccard(hashSets50[i], hashSets50[j]);
    auto hb3 = chrono::high_resolution_clock::now();
    double hbMs50 = chrono::duration<double, milli>(hb3 - hb2).count();

    cout << "    50 files — brute-force Jaccard: " << fixed << setprecision(3) << bfMs50 << " ms\n";
    cout << "    50 files — hash-based  Jaccard: " << fixed << setprecision(3) << hbMs50 << " ms\n";
    if (hbMs50 > 0)
        cout << "    Speedup ratio (50 files)      : " << fixed << setprecision(1) << (bfMs50 / hbMs50) << "x\n";
    cout << "\n";

    // ------------------------------------------------------------------
    // 7.  ACCURACY — known ground-truth pairs
    // ------------------------------------------------------------------
    cout << "[6] ACCURACY (True Positive Rate)\n";
    cout << "    Ground truth from test corpus:\n";
    cout << "      test1 ≡ test6   (exact copy)             → expected J ≈ 1.00\n";
    cout << "      test1 ~ test2   (renamed vars only)      → expected J > 0.20\n";
    cout << "      test1 ~ test3   (reformatted, same logic) → expected J > 0.50\n";
    cout << "      test5  (different logic)                  → expected J < 0.20\n\n";

    // Compute actual Jaccard values for the 6 real files
    vector<vector<double>> simMatrix(6, vector<double>(6, 0));
    for (int i = 0; i < 6; i++)
        for (int j = 0; j < 6; j++)
            simMatrix[i][j] = computeJaccard(hashSets6[i], hashSets6[j]);

    cout << "    Similarity matrix (6 real files, k=" << k << "):\n";
    cout << "           ";
    for (auto& fn : realFiles) cout << setw(10) << fn;
    cout << "\n";
    for (int i = 0; i < 6; i++) {
        cout << "    " << setw(10) << realFiles[i];
        for (int j = 0; j < 6; j++)
            cout << setw(10) << fixed << setprecision(4) << simMatrix[i][j];
        cout << "\n";
    }

    // Evaluate TP / FP / TN / FN
    // Ground truth: plagiarized pairs (0-indexed)
    //   (0,5)  test1-test6  exact copy
    //   (0,2)  test1-test3  reformatted same logic
    //   (0,1)  test1-test2  renamed vars
    //   (2,5)  test3-test6  (since test6==test1, test3 is similar to test6 too)
    //   (1,5)  test2-test6  (since test6==test1, test2 ~ test6)
    //   (1,2)  test2-test3  both similar to test1
    // Non-plagiarized: any pair involving test5 (completely different logic)

    double THRESHOLD = 0.25;  // similarity threshold for "plagiarism detected"

    struct PairLabel { int i, j; bool plagiarized; };
    vector<PairLabel> groundTruth = {
        {0, 5, true},   // test1-test6 exact copy
        {0, 2, true},   // test1-test3 reformatted
        {0, 1, true},   // test1-test2 renamed vars
        {0, 4, false},  // test1-test5 different logic
        {1, 4, false},  // test2-test5 different logic
        {2, 4, false},  // test3-test5 different logic
        {3, 4, false},  // test4-test5 different logic
        {4, 5, false},  // test5-test6 different logic
    };

    int TP = 0, FP = 0, TN = 0, FN = 0;
    for (auto& g : groundTruth) {
        bool detected = simMatrix[g.i][g.j] >= THRESHOLD;
        if (g.plagiarized && detected) TP++;
        else if (g.plagiarized && !detected) FN++;
        else if (!g.plagiarized && detected) FP++;
        else TN++;
    }

    cout << "\n    Detection threshold : " << THRESHOLD << "\n";
    cout << "    True  Positives (TP): " << TP << "\n";
    cout << "    False Negatives (FN): " << FN << "\n";
    cout << "    True  Negatives (TN): " << TN << "\n";
    cout << "    False Positives (FP): " << FP << "\n";
    double tpRate = (TP + FN) > 0 ? (double)TP / (TP + FN) : 0;
    double precision = (TP + FP) > 0 ? (double)TP / (TP + FP) : 0;
    double f1 = (precision + tpRate) > 0 ? 2 * precision * tpRate / (precision + tpRate) : 0;
    cout << "    True Positive Rate  : " << fixed << setprecision(1) << (tpRate * 100) << "%\n";
    cout << "    Precision           : " << fixed << setprecision(1) << (precision * 100) << "%\n";
    cout << "    F1 Score            : " << fixed << setprecision(2) << f1 << "\n";

    cout << "\n========================================================\n";
    cout << "  Benchmark complete.\n";
    cout << "========================================================\n";

    return 0;
}
