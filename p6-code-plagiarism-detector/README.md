# C++ Source Code Plagiarism Detector

A source code similarity detection system that uses **code normalization**, **variable standardization**, **k-gram hashing**, and **Jaccard similarity** to identify plagiarized C++ submissions.

## Pipeline

```
Raw .cpp → Normalize Whitespace → Remove Comments → Standardize Variables → Tokenize → 3-gram → Hash → Jaccard Matrix
```

## How It Works

1. **Normalize Whitespace** — Strips leading/trailing spaces, collapses tabs and blank lines
2. **Remove Comments** — Strips both `//` single-line and `/* */` multi-line comments
3. **Standardize Variables** — Renames all user-declared variables to `var1`, `var2`, … (defeats variable-renaming obfuscation)
4. **Tokenize** — Extracts identifiers, literals, operators, and symbols via regex
5. **k-gram Generation** — Creates sliding windows of 3 consecutive tokens
6. **Polynomial Rolling Hash** — Hashes each k-gram (base = 257, mod = 10⁹+7) into `unordered_set`
7. **Jaccard Similarity** — Computes `J(A,B) = |A∩B| / |A∪B|` over hash sets for every file pair

## Data Structures & Algorithms

| Component | Implementation |
|---|---|
| Token storage | `std::vector<string>` |
| Hash storage | `std::unordered_set<unsigned long>` — O(1) lookups |
| Variable mapping | `std::unordered_map<string, string>` |
| Hashing | Polynomial rolling hash |
| Tokenization | `std::regex` with pattern for identifiers, numbers, operators, symbols |

## Benchmark Results

| Metric | Value |
|---|---|
| **Corpus scale** | 50 files (1,225 pairs) in a single run |
| **Performance** | 6 files in 6.3 ms · 50 files in 628 ms |
| **Peak memory** | 6.8 MB |
| **k-grams/file** | ~26 (real) · ~246 (synthetic 50-LOC files) |
| **Speedup vs brute-force** | **13.9×** faster than `std::set<string>` baseline |
| **True Positive Rate** | **100%** (3/3 plagiarized pairs detected) |
| **Precision** | **100%** (0 false positives) |
| **F1 Score** | **1.00** |

## Test Corpus & Detection Results

| Pair | Relationship | Jaccard | Detected? |
|---|---|---|---|
| test1 ↔ test6 | Exact copy | **1.0000** | ✅ |
| test1 ↔ test3 | Reformatted, same logic | **0.7778** | ✅ |
| test1 ↔ test2 | Renamed variables only | **0.2973** | ✅ |
| test1 ↔ test5 | Completely different logic | **0.1321** | ✅ TN |

## Build & Run

```bash
# Compile
g++ -std=c++20 -O2 -o project6 project6.cpp

# Run (from the p6-code-plagiarism-detector/ directory)
./project6
```

### Running Benchmarks

```bash
cd benchmarks/
g++ -O2 -std=c++20 -o benchmark benchmark.cpp -lpsapi
./benchmark
```

## Project Structure

```
p6-code-plagiarism-detector/
├── project6.cpp            # Main source
├── CMakeLists.txt          # CMake build config
├── README.md
├── test-corpus/            # C++ test files
│   ├── test1.cpp … test6.cpp
│   └── expected-output.txt
├── benchmarks/             # Performance measurement
│   ├── benchmark.cpp       # Full benchmark harness
│   └── speedup_bench.cpp   # Isolated speedup comparison
└── docs/
    ├── CS2413-Project-Six-Spring2025.pdf
    └── main-4.txt
```

---

*Author: Luc Mwizerwa · CS 2413 Data Structures · Spring 2025*
