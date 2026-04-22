# C++ Similarity Detector

**C++ Similarity Detector** is a lightweight tool for analyzing the structural and lexical similarity between multiple C++ source files. Inspired by tools like [MOSS (Measure of Software Similarity)](https://theory.stanford.edu/~aiken/moss/), it uses **token-based hashing**, **variable normalization**, and **Jaccard similarity** to detect logical overlap between `.cpp` files — even if they look different on the surface.

> Developed for **CS 2413 – Data Structures (Spring 2025)** at the **University of Oklahoma**.

---

## Projects

This repository contains two related plagiarism-detection tools:

| Project | Description | README |
|---|---|---|
| [`p5-text-fingerprinting`](./p5-text-fingerprinting/) | Document similarity detection for plain text using k-gram hashing | [→ README](./p5-text-fingerprinting/README.md) |
| [`p6-code-plagiarism-detector`](./p6-code-plagiarism-detector/) | C++ source code plagiarism detection with variable normalization | [→ README](./p6-code-plagiarism-detector/README.md) |

---

## Features

- **Code Preprocessing**
  - Removes extra whitespace, blank lines, and comments (`//` and `/* ... */`)
  - Normalizes variable names to placeholders (`var1`, `var2`, ...)

- **Lexical Tokenization**
  - Uses regex to extract meaningful tokens like identifiers, literals, operators, and symbols

- **K-Gram Hashing**
  - Forms overlapping token sequences (`k`-grams, tested with k = 3, 5, 7)
  - Computes fingerprints using a polynomial rolling hash (base 257, mod 10⁹+7)

- **Jaccard Similarity Matrix**
  - Compares fingerprint sets for each file pair
  - Outputs a symmetric similarity matrix

---

## How It Works

```
Raw .cpp → Normalize Whitespace → Remove Comments → Standardize Variables → Tokenize → k-gram → Hash → Jaccard Matrix
```

1. **Input**: Load multiple `.cpp` or `.txt` files
2. **Normalize**: Clean up whitespace, remove comments, and rename all variable names
3. **Tokenize**: Break code into meaningful pieces using regular expressions
4. **K-Grams**: Create `k`-length token sequences (e.g., `int main (`, `main ( )`, etc.)
5. **Hashing**: Hash each k-gram with polynomial rolling hash and store in `unordered_set`
6. **Compare**: Compute Jaccard similarity `J(A,B) = |A∩B| / |A∪B|` between file pairs
7. **Output**: Display a similarity matrix

---

## Benchmark Results

Measured on the code plagiarism detector (`p6`) — see [full benchmark report](./p6-code-plagiarism-detector/README.md#benchmark-results).

| Metric | Value |
|---|---|
| **Corpus scale** | 50 files (1,225 pairs) in a single run |
| **Performance** | 6 files in 6 ms · 50 files in 628 ms |
| **Peak memory** | 6.8 MB |
| **Speedup vs brute-force** | **13.9×** faster |
| **True Positive Rate** | **100%** |
| **Precision** | **100%** |
| **F1 Score** | **1.00** |

---

## Project Structure

```
Similarity-Checker/
├── README.md
├── LICENSE
├── p5-text-fingerprinting/
│   ├── project5.cpp
│   ├── README.md
│   ├── test-corpus/          # 8 text documents + stopwords
│   └── output/               # Pre-generated results (k=3,5,7)
└── p6-code-plagiarism-detector/
    ├── project6.cpp
    ├── README.md
    ├── test-corpus/           # 6 C++ test files
    ├── benchmarks/            # Performance measurement harness
    └── docs/                  # Project spec PDF
```

---

## Technologies

- **Language**: C++20
- **Build**: g++ / CMake
- **Libraries**: STL (`regex`, `unordered_map`, `unordered_set`, `vector`, `chrono`)
- **Algorithms**: K-gram hashing, polynomial rolling hash, Jaccard similarity

---

## Use Cases

- Detecting **plagiarism** in C++ assignments
- Studying **tokenization**, **hashing**, and **similarity detection**
- Exploring C++ **regex** and **data structures** (e.g., `unordered_map`, `unordered_set`)

---

## Credits

Created by Luc Thierry Mwizerwa
Course: **CS 2413 – Data Structures, Spring 2025**
University of Oklahoma

---

## License

This project is for academic purposes only and distributed under the MIT License.
