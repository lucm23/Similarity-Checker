# Text Fingerprinting with Jaccard Similarity

A document similarity detection system that uses **k-gram hashing** and **Jaccard similarity** to compare text documents — a simplified plagiarism detection engine.

## Pipeline

```
Raw Text → Normalize (lowercase, whitespace) → Tokenize (regex) → Generate k-grams → Hash (polynomial rolling) → Jaccard Similarity Matrix
```

## How It Works

1. **Normalize** — Converts text to lowercase, collapses whitespace
2. **Tokenize** — Extracts word tokens using regex `[a-zA-Z0-9]+`
3. **Stopword Removal** *(optional)* — Filters common words (the, in, a, etc.)
4. **k-gram Generation** — Creates sliding windows of *k* consecutive tokens
5. **Polynomial Rolling Hash** — Hashes each k-gram (base = 257, mod = 10⁹+7)
6. **Jaccard Similarity** — Computes `J(A,B) = |A∩B| / |A∪B|` over hash sets
7. **Similarity Matrix** — Outputs pairwise similarity for all documents

## Data Structures & Algorithms

| Component | Implementation |
|---|---|
| Token storage | `std::vector<string>` |
| Hash storage | `std::unordered_set<unsigned long>` — O(1) lookups |
| Stopwords | `std::unordered_set<string>` |
| Hashing | Polynomial rolling hash |
| Tokenization | `std::regex` |

## Parameters

The tool was tested with multiple k-gram sizes:

- **k = 3** — Captures short phrase patterns
- **k = 5** — Balanced granularity
- **k = 7** — Longer sequence matching

## Build & Run

```bash
# Compile
g++ -std=c++20 -O2 -o project5 project5.cpp

# Run (from the p5-text-fingerprinting/ directory)
./project5
```

## Project Structure

```
p5-text-fingerprinting/
├── project5.cpp          # Main source
├── CMakeLists.txt        # CMake build config
├── test-corpus/          # Input text files
│   ├── t1.txt … t8.txt   # 8 test documents
│   ├── tA.txt, tB.txt    # Debug test pair
│   └── stopwords.txt     # Common words to filter
└── output/               # Pre-generated results
    ├── 3-gram_output.txt
    ├── 5-gram_output.txt
    ├── 7-gram_output.txt
    └── stopwords_output.txt
```

## Sample Output (k=3, 8 files)

```
Similarity Matrix:
     t1    t2    t3    t4    t5    t6    t7    t8
t1  1.00  0.43  0.43  0.30  0.27  0.21  0.09  0.30
t2  0.43  1.00  0.23  0.21  0.27  0.13  0.17  0.13
...
```

---

*Author: Luc Mwizerwa · CS 2413 Data Structures · Spring 2025*
