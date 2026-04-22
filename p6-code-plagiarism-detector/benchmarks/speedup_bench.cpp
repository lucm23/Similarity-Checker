/**
 * Refined Speedup Measurement (lighter version)
 * Runs the Jaccard comparison enough times to get measurable times.
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <set>
#include <regex>
#include <algorithm>
#include <iomanip>
#include <chrono>

using namespace std;

unsigned long simpleHash(const string& s) {
    const int base = 257, mod = 1000000007;
    unsigned long hash = 0;
    for (char c : s) hash = (hash * base + c) % mod;
    return hash;
}

vector<string> makeKgrams(int id, int count) {
    vector<string> kg;
    for (int i = 0; i < count; i++) {
        kg.push_back("token" + to_string(id) + "_" + to_string(i) + " token" + to_string(i+1) + " token" + to_string(i+2));
    }
    return kg;
}

int main() {
    const int N_FILES = 20;
    const int KG_PER_FILE = 100;
    const int ITERS = 50;

    vector<vector<string>> allKgrams(N_FILES);
    vector<unordered_set<unsigned long>> allHashes(N_FILES);

    for (int i = 0; i < N_FILES; i++) {
        allKgrams[i] = makeKgrams(i, KG_PER_FILE);
        for (auto& kg : allKgrams[i])
            allHashes[i].insert(simpleHash(kg));
    }

    int pairs = N_FILES * (N_FILES - 1) / 2;

    // --- Hash-based Jaccard ---
    volatile double sink = 0;
    auto h0 = chrono::high_resolution_clock::now();
    for (int iter = 0; iter < ITERS; iter++) {
        for (int i = 0; i < N_FILES; i++) {
            for (int j = i + 1; j < N_FILES; j++) {
                int inter = 0;
                for (auto& h : allHashes[i])
                    if (allHashes[j].count(h)) inter++;
                int uni = allHashes[i].size() + allHashes[j].size() - inter;
                sink += (double)inter / uni;
            }
        }
    }
    auto h1 = chrono::high_resolution_clock::now();
    double hashMs = chrono::duration<double, milli>(h1 - h0).count() / ITERS;

    // --- Brute-force string Jaccard ---
    auto b0 = chrono::high_resolution_clock::now();
    for (int iter = 0; iter < ITERS; iter++) {
        for (int i = 0; i < N_FILES; i++) {
            for (int j = i + 1; j < N_FILES; j++) {
                set<string> sA(allKgrams[i].begin(), allKgrams[i].end());
                set<string> sB(allKgrams[j].begin(), allKgrams[j].end());
                int inter = 0;
                for (auto& s : sA)
                    if (sB.count(s)) inter++;
                int uni = sA.size() + sB.size() - inter;
                sink += (double)inter / uni;
            }
        }
    }
    auto b1 = chrono::high_resolution_clock::now();
    double bruteMs = chrono::duration<double, milli>(b1 - b0).count() / ITERS;

    cout << "=== Refined Speedup Measurement ===\n";
    cout << "Files: " << N_FILES << ", K-grams/file: " << KG_PER_FILE << ", Pairs: " << pairs << "\n";
    cout << "Iterations averaged: " << ITERS << "\n\n";
    cout << "Hash-based Jaccard (per run) : " << fixed << setprecision(3) << hashMs << " ms\n";
    cout << "Brute-force Jaccard (per run): " << fixed << setprecision(3) << bruteMs << " ms\n";
    cout << "Speedup ratio                : " << fixed << setprecision(1) << (bruteMs / hashMs) << "x\n";
    cout << "(sink=" << sink << ")\n";
    return 0;
}
