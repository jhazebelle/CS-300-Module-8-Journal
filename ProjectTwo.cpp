//============================================================================
// Name        : ProjectTwo.cpp
// Author      : Jhasmin Gambon
// Course      : CS 300 - DSA Design and Analysis
// Version     : 1.0
// Description : Command line program that loads course data (CSV) and
//               supports:
//                 1) Load Data Structure
//                 2) Print Course List (sorted alphanumeric)
//                 3) Print a Single Course (title and prerequisites)
// Notes       : -No external CSV parser; I split on commas and trim.
//               -I uppercase course numbers so user input is case insensitive.
//               -BST in order traversal prints the list already sorted.
//               -For prerequisites, I print codes in the order given in the file
//                to match the sample output.
//============================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>
#include <algorithm>

using namespace std;

// --------------------------- Utility helpers --------------------------------

// trim: remove leading/trailing whitespace
static inline string trim(const string& s) {
    size_t start = 0, end = s.size();
    while (start < end && isspace(static_cast<unsigned char>(s[start]))) ++start;
    while (end > start && isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

// split a CSV line by ',' and trim each token (no quotes handling needed per project)
static vector<string> splitCSV(const string& line) {
    vector<string> out;
    string token;
    stringstream ss(line);
    while (getline(ss, token, ',')) {
        out.push_back(trim(token));
    }
    // Handle trailing comma case (if any) we already read empty token above, which trim keeps as ""
    return out;
}

// uppercase a course number so comparisons/user input are consistent
static string upperCopy(string s) {
    for (char& ch : s) ch = static_cast<char>(toupper(static_cast<unsigned char>(ch)));
    return s;
}

// ------------------------------ Data Model -----------------------------------

struct Course {
    string number;                // example CSCI200
    string title;                 // example Intro to Algorithms
    vector<string> prerequisites; // example {"CSCI100","MATH101"}
};

// ---------------------------- Binary Search Tree -----------------------------

class CourseBST {
private:
    struct Node {
        Course course;
        Node* left;
        Node* right;
        Node(const Course& c) : course(c), left(nullptr), right(nullptr) {}
    };

    Node* root = nullptr;

    // I’m using recursive helpers to keep public API tiny and readable.
    void destroy(Node* n) {
        if (!n) return;
        destroy(n->left);
        destroy(n->right);
        delete n;
    }

    void inOrder(Node* n) const {
        if (!n) return;
        inOrder(n->left);
        cout << n->course.number << ", " << n->course.title << '\n';
        inOrder(n->right);
    }

    Node* insert(Node* n, const Course& c) {
        if (!n) return new Node(c);
        if (c.number < n->course.number) {
            n->left = insert(n->left, c);
        } else if (c.number > n->course.number) {
            n->right = insert(n->right, c);
        } else {
            // If duplicate key appears, I’ll update the title and prereqs (safer than ignoring).
            n->course = c;
        }
        return n;
    }

    const Course* search(Node* n, const string& number) const {
        Node* cur = n;
        while (cur) {
            if (number == cur->course.number) return &cur->course;
            cur = (number < cur->course.number) ? cur->left : cur->right;
        }
        return nullptr;
    }

public:
    CourseBST() = default;
    ~CourseBST() { destroy(root); }

    void Clear() {
        destroy(root);
        root = nullptr;
    }

    void Insert(const Course& c) { root = insert(root, c); }
    const Course* Search(const string& number) const { return search(root, number); }
    void PrintInOrder() const { inOrder(root); }
    bool Empty() const { return root == nullptr; }
};

// --------------------------- Loader / Validation -----------------------------

// I prompt for filename in main, but encapsulate the file processing here.
static bool LoadCoursesFromFile(const string& filePath, CourseBST& bst, vector<string>& errors, size_t& loadCount) {
    ifstream file(filePath);
    if (!file.is_open()) {
        errors.push_back("Error: cannot open file '" + filePath + "'.");
        return false;
    }

    // I’m clearing the previous tree so “Load” can be run multiple times with different files.
    bst.Clear();
    errors.clear();
    loadCount = 0;

    string rawLine;
    size_t lineNumber = 0;

    // First pass: parse each line -> build Course -> insert into BST
    while (getline(file, rawLine)) {
        ++lineNumber;
        string line = trim(rawLine);
        if (line.empty()) continue;

        vector<string> tokens = splitCSV(line);
        if (tokens.size() < 2) {
            errors.push_back("Line " + to_string(lineNumber) + ": needs at least Course Number and Title.");
            continue;
        }

        Course c;
        c.number = upperCopy(tokens[0]);
        c.title  = tokens[1];

        if (c.number.empty()) {
            errors.push_back("Line " + to_string(lineNumber) + ": missing course number.");
            continue;
        }
        if (c.title.empty()) {
            errors.push_back("Line " + to_string(lineNumber) + ": missing course title.");
            continue;
        }

        // 0..N prerequisites
        for (size_t i = 2; i < tokens.size(); ++i) {
            if (!tokens[i].empty())
                c.prerequisites.push_back(upperCopy(tokens[i]));
        }

        bst.Insert(c);
        ++loadCount;
    }
    file.close();

    // Second pass: validate that every prerequisite appears as its own course number
    // (I’m not failing the load, just reporting issues so advisors are informed.)
    // This is O(p log n) on average with a BST.
    // If there were format errors in pass 1, they’re already listed in errors.
    // Here I traverse by printing into a vector via in order so I can re search.
    // To keep it simple and memory light, I’ll just walk the tree by printing then re search.
    // But we don’t have an exposed iterator, so instead, I’ll reopen file to check prereqs exist.


    ifstream file2(filePath);
    lineNumber = 0;
    while (getline(file2, rawLine)) {
        ++lineNumber;
        string line = trim(rawLine);
        if (line.empty()) continue;

        vector<string> tokens = splitCSV(line);
        if (tokens.size() < 2) continue; // already reported earlier

        string courseNum = upperCopy(tokens[0]);

        for (size_t i = 2; i < tokens.size(); ++i) {
            string p = upperCopy(tokens[i]);
            if (p.empty()) continue;
            if (bst.Search(p) == nullptr) {
                errors.push_back("Course '" + courseNum + "' lists missing prerequisite '" + p + "'.");
            }
        }
    }
    file2.close();

    return true;
}

// ------------------------------- Printing ------------------------------------

static void PrintCourse(const CourseBST& bst, const string& queryNumber) {
    string key = upperCopy(queryNumber);
    const Course* c = bst.Search(key);
    if (!c) {
        cout << "Course not found.\n";
        return;
    }

    cout << c->number << " - " << c->title << '\n';

    if (c->prerequisites.empty()) {
        cout << "Prerequisites: None\n";
        return;
    }

    cout << "Prerequisites:\n";
    for (const string& p : c->prerequisites) {
        const Course* pc = bst.Search(p);
        if (pc) {
            cout << "  " << pc->number << " - " << pc->title << '\n';
        } else {
            // If a prerequisite didn’t exist, I still show the code so the advisor knows.
            cout << "  " << p << " (missing from catalog)\n";
        }
    }
}

// ------------------------------- Menu UI -------------------------------------

static void PrintMenu() {
    cout << "\nABCU Advisor Menu\n";
    cout << "  1. Load Data\n";
    cout << "  2. Print Course List (Sorted)\n";
    cout << "  3. Print Course\n";
    cout << "  9. Exit\n";
    cout << "Enter choice: ";
}

// --------------------------------- main --------------------------------------

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    CourseBST bst;
    bool dataLoaded = false;
    string loadedFile;
    size_t loadedCount = 0;

    while (true) {
        PrintMenu();
        string choice;
        if (!getline(cin, choice)) break; // EOF/stream closed

        if (choice == "1") {
            cout << "Enter the course data filename (e.g., courses.txt): ";
            string filePath;
            if (!getline(cin, filePath)) {
                cout << "Input aborted.\n";
                continue;
            }
            filePath = trim(filePath);

            vector<string> errors;
            size_t count = 0;
            bool ok = LoadCoursesFromFile(filePath, bst, errors, count);

            if (!ok) {
                cout << "Load failed.\n";
            }
            if (!errors.empty()) {
                cout << "\nValidation issues (" << errors.size() << "):\n";
                for (const string& e : errors) cout << " - " << e << '\n';
            } else if (ok) {
                cout << "File validated. Loaded " << count << " courses.\n";
            }

            dataLoaded = ok;
            loadedFile = filePath;
            loadedCount = count;

        } else if (choice == "2") {
            if (!dataLoaded || bst.Empty()) {
                cout << "Please load data first (Option 1).\n";
                continue;
            }
            cout << "\nCourse List (alphanumeric):\n";
            bst.PrintInOrder();

        } else if (choice == "3") {
            if (!dataLoaded || bst.Empty()) {
                cout << "Please load data first (Option 1).\n";
                continue;
            }
            cout << "Enter course number (e.g., CSCI300): ";
            string target;
            if (!getline(cin, target)) {
                cout << "Input aborted.\n";
                continue;
            }
            target = trim(target);
            if (target.empty()) {
                cout << "Please enter a non-empty course number.\n";
                continue;
            }
            PrintCourse(bst, target);

        } else if (choice == "9") {
            cout << "Goodbye.\n";
            break;

        } else {
            // Industry standard best practice: handle invalid menu input
            cout << "Invalid choice. Please select 1, 2, 3, or 9.\n";
        }
    }

    return 0;
}