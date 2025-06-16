# C++ Similarity Detector

**C++ Similarity Detector** is a lightweight tool for analyzing the structural and lexical similarity between multiple C++ source files. Inspired by tools like [MOSS (Measure of Software Similarity)](https://theory.stanford.edu/~aiken/moss/), it uses **token-based hashing**, **variable normalization**, and **Jaccard similarity** to detect logical overlap between `.cpp` files — even if they look different on the surface.

> Developed for **CS 2413 – Data Structures (Spring 2025)** at the **University of Oklahoma**.

---

##  Features

- ** Code Preprocessing**
  - Removes extra whitespace, blank lines, and comments (`//` and `/* ... */`)
  - Normalizes variable names to placeholders (`var1`, `var2`, ...)

- ** Lexical Tokenization**
  - Uses regex to extract meaningful tokens like identifiers, literals, operators, and symbols

- ** K-Gram Hashing**
  - Forms overlapping token sequences (`k`-grams)
  - Computes fingerprints using a custom hash function

- ** Jaccard Similarity Matrix**
  - Compares fingerprint sets for each file pair
  - Outputs a symmetric similarity matrix rounded to 2 decimal places


---

## 🛠️ How It Works

1. **Input**: Load multiple `.cpp` files
2. **Normalize**: Clean up whitespace, remove comments, and rename all variable names
3. **Tokenize**: Break code into meaningful pieces using regular expressions
4. **K-Grams**: Create `k`-length token sequences (e.g., `int main (`, `main ( )`, etc.)
5. **Hashing**: Hash each k-gram and store in a set
6. **Compare**: Compute Jaccard similarity between file pairs based on hashed sets
7. **Output**: Display a similarity matrix like:



---

##  Use Cases

- Detecting **plagiarism** in C++ assignments  
- Studying **tokenization**, **hashing**, and **similarity detection**  
- Exploring C++ **regex** and **data structures** (e.g., `unordered_map`, `unordered_set`)

---

##  Technologies

- **Language**: C++
- **Libraries**: STL (`regex`, `unordered_map`, `unordered_set`, `stringstream`)
- **Algorithms**: K-gram hashing, Jaccard similarity

---

##  Credits

Created by Luc Thierry Mwizerwa  
Course: **CS 2413 – Data Structures, Spring 2025**  
University of Oklahoma

---

## License

This project is for academic purposes only and distributed under the MIT License.


