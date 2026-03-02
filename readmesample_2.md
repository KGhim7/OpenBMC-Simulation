[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/GhsWBgcv)
[![Open in Visual Studio Code](https://classroom.github.com/assets/open-in-vscode-2e0aaae1b6195c2367325f4f02e2d04e9abb55f0b24a779b69b11b9e10269abc.svg)](https://classroom.github.com/online_ide?assignment_repo_id=21373111&assignment_repo_type=AssignmentRepo)
# 🧪 Week 11 Wed Lab: Exploring Probing Techniques in Closed Hash Tables

## 🎯 Objective
This lab demonstrates how **different probing techniques** affect the performance of hash tables that use **closed hashing (open addressing)**.  
You will implement and analyze three probing strategies:

- **Linear Probing with Skips**
- **Quadratic Probing**
- **Double Hashing**

The experiment measures the **number of probes**, **insertion and search times**, and compares their **complexities** under various load factors.

---

## 📘 Learning Outcomes
By the end of this lab, you will be able to:
1. Implement hash tables with closed hashing in C++.
2. Use and compare different collision resolution strategies.
3. Measure and interpret the effect of load factor and skip value on performance.
4. Analyze the **time complexity** of insert and search operations.

---

## 🧩 Implementation Overview

### Table and Experiment Parameters
| Symbol | Description | Value |
|---------|--------------|--------|
| `M` | Hash table size | **101** |
| `N` | Number of keys to insert | **80** |
| `c` | Skip factor for linear probing | **3** |

> These values ensure the table is not full (load factor ≈ 0.79) and collisions occur frequently enough to compare techniques.

### Note:
You need to implement code in HashTableClosed.tpp and LinearProbing.hpp, QuadraticProbing.hpp and DoubleHashing.hpp.

To access the base class functions and fields in the derived classes, you should use **this->**. For example you will use this->M instead of M in LinearProbing.hpp, QuadraticProbing.hpp and DoubleHashing.hpp. 


---

## ⚙️ Hash Functions

- Primary hash function:  
  ```cpp
  h1(key) = key % M
