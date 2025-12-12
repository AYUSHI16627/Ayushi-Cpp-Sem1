#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <functional>

using namespace std;

// ---------- Simple data structs ----------
struct Subject {
    string name;
    int credits;
    float gradePoint; // 0-10
};

struct Semester {
    int semNo;
    vector<Subject> subjects;
    float sgpa;
};

struct Student {
    string username;
    string password; // stored in students.txt (simple/plain for microproject)
    string name;
    string roll;
    vector<Semester> semesters;
    float cgpa;
};

// ---------- Filenames ----------
const string USERS_FILE = "students.txt"; // stores: username,password,name,roll
// per-student file: student_<roll>.txt

// ---------- Utility: split by comma ----------
vector<string> splitCSV(const string &line, char delim = ',') {
    vector<string> out;
    string cur;
    stringstream ss(line);
    while (getline(ss, cur, delim)) out.push_back(cur);
    return out;
}

// ---------- Save/load user list ----------
vector<Student> loadUserList() {
    vector<Student> list;
    ifstream fin(USERS_FILE.c_str());
    if (!fin) return list;
    string line;
    while (getline(fin, line)) {
        if (line.size() == 0) continue;
        // format: username,password,name,roll
        vector<string> f = splitCSV(line);
        if (f.size() < 4) continue;
        Student s;
        s.username = f[0];
        s.password = f[1];
        s.name = f[2];
        s.roll = f[3];
        s.cgpa = 0.0f;
        // load academic data separately
        list.push_back(s);
    }
    fin.close();
    // now load per-student files (if exist)
    for (auto &s : list) {
        string fname = "student_" + s.roll + ".txt";
        ifstream fin2(fname.c_str());
        if (!fin2) continue;
        int semCount = 0;
        fin2 >> semCount;
        s.semesters.clear();
        for (int i = 0; i < semCount; ++i) {
            Semester sem;
            fin2 >> sem.semNo;
            int subCount; fin2 >> subCount;
            sem.subjects.clear();
            for (int j = 0; j < subCount; ++j) {
                Subject sub;
                fin2 >> ws;
                getline(fin2, sub.name); // read full line (subjectName credits grade)
                // subject line format we will parse: <name>|<credits>|<grade>
                vector<string> parts = splitCSV(sub.name, '|');
                if (parts.size() >= 3) {
                    sub.name = parts[0];
                    sub.credits = stoi(parts[1]);
                    sub.gradePoint = stof(parts[2]);
                } else {
                    sub.name = "SUB";
                    sub.credits = 0;
                    sub.gradePoint = 0.0f;
                }
                sem.subjects.push_back(sub);
            }
            fin2 >> sem.sgpa;
            s.semesters.push_back(sem);
        }
        fin2 >> s.cgpa;
        fin2.close();
    }
    return list;
}

void saveUserList(const vector<Student> &list) {
    ofstream fout(USERS_FILE.c_str());
    for (const auto &s : list) {
        // username,password,name,roll  (no commas inside fields)
        fout << s.username << "," << s.password << "," << s.name << "," << s.roll << "\n";
    }
    fout.close();
}

// ---------- Save single student's academic file ----------
void saveStudentAcademic(const Student &s) {
    string fname = "student_" + s.roll + ".txt";
    ofstream fout(fname.c_str());
    fout << s.semesters.size() << "\n";
    for (const auto &sem : s.semesters) {
        fout << sem.semNo << "\n";
        fout << sem.subjects.size() << "\n";
        // subject saved as: <name>|<credits>|<grade> on single line
        for (const auto &sub : sem.subjects) {
            fout << sub.name << "|" << sub.credits << "|" << fixed << setprecision(2) << sub.gradePoint << "\n";
        }
        fout << fixed << setprecision(4) << sem.sgpa << "\n";
    }
    fout << fixed << setprecision(4) << s.cgpa << "\n";
    fout.close();
}

// ---------- Calculations ----------
float calcSGPA(const vector<Subject> &subs) {
    float totalPoints = 0.0f;
    int totalCredits = 0;
    for (const auto &sub : subs) {
        totalPoints += sub.credits * sub.gradePoint;
        totalCredits += sub.credits;
    }
    if (totalCredits == 0) return 0.0f;
    return totalPoints / totalCredits;
}

float calcCGPA(const vector<Semester> &sems) {
    float weighted = 0.0f;
    int totalCredits = 0;
    for (const auto &sem : sems) {
        int semCr = 0;
        for (const auto &sub : sem.subjects) semCr += sub.credits;
        weighted += sem.sgpa * semCr;
        totalCredits += semCr;
    }
    if (totalCredits == 0) return 0.0f;
    return weighted / totalCredits;
}

// ---------- Ranking ----------
void showRanking(vector<Student> &list) {
    // Ensure cgpa values are up to date by loading academic files
    for (auto &s : list) {
        // attempt to reload academic file for each student (simple)
        string fname = "student_" + s.roll + ".txt";
        ifstream fin(fname.c_str());
        if (!fin) { s.cgpa = 0.0f; continue; }
        int semCount; fin >> semCount;
        vector<Semester> sems;
        for (int i = 0; i < semCount; ++i) {
            Semester sem;
            fin >> sem.semNo;
            int sc; fin >> sc;
            sem.subjects.clear();
            string tmp;
            getline(fin, tmp); // move to next line
            for (int j = 0; j < sc; ++j) {
                string whole;
                getline(fin, whole);
                vector<string> parts = splitCSV(whole, '|');
                Subject sub;
                if (parts.size() >= 3) {
                    sub.name = parts[0];
                    sub.credits = stoi(parts[1]);
                    sub.gradePoint = stof(parts[2]);
                } else {
                    sub.name = "SUB";
                    sub.credits = 0;
                    sub.gradePoint = 0.0f;
                }
                sem.subjects.push_back(sub);
            }
            fin >> sem.sgpa;
            sems.push_back(sem);
        }
        float cg = 0.0f;
        fin >> cg;
        s.cgpa = cg;
        fin.close();
    }

    sort(list.begin(), list.end(), [](const Student &a, const Student &b) {
        return a.cgpa > b.cgpa;
    });

    cout << "\n--- Overall Ranking by CGPA ---\n";
    cout << left << setw(6) << "Rank" << setw(12) << "Roll" << setw(20) << "Name" << setw(8) << "CGPA" << "\n";
    for (size_t i = 0; i < list.size(); ++i) {
        cout << setw(6) << (i+1) << setw(12) << list[i].roll << setw(20) << list[i].name << fixed << setprecision(2) << list[i].cgpa << "\n";
    }
}

// ---------- Export CSV for one student ----------
void exportCSV(const Student &s) {
    string fname = "export_" + s.roll + ".csv";
    ofstream fout(fname.c_str());
    fout << "Roll,Name,Semester,Subject,Credits,GradePoint,SGPA,CGPA\n";
    float cgpa = s.cgpa;
    for (const auto &sem : s.semesters) {
        for (size_t i = 0; i < sem.subjects.size(); ++i) {
            fout << s.roll << "," << s.name << "," << sem.semNo << "," << "\"" << sem.subjects[i].name << "\"" << "," << sem.subjects[i].credits << "," << sem.subjects[i].gradePoint;
            if (i == 0) fout << "," << fixed << setprecision(4) << sem.sgpa << "," << fixed << setprecision(4) << cgpa;
            fout << "\n";
        }
    }
    fout.close();
    cout << "Exported CSV to " << fname << "\n";
}

// ---------- Simple menus ----------
int findUserIndex(const vector<Student> &db, const string &username) {
    for (size_t i = 0; i < db.size(); ++i) if (db[i].username == username) return (int)i;
    return -1;
}

int main() {
    cout << "=== MITS SGPA & CGPA MANAGEMENT (Simple) ===\n";
    vector<Student> db = loadUserList();

    while (true) {
        cout << "\n1) Register  2) Login  3) Overall Ranking  4) Exit\nChoose: ";
        int opt; if (!(cin >> opt)) break;

        if (opt == 1) {
            Student s;
            cout << "Choose username (no commas): ";
            cin >> s.username;
            if (findUserIndex(db, s.username) != -1) { cout << "Username exists.\n"; continue; }
            cout << "Choose password: "; cin >> s.password;
            cout << "Student Name (no commas): "; cin >> s.name;
            cout << "Roll number (no commas): "; cin >> s.roll;
            s.cgpa = 0.0f;
            db.push_back(s);
            saveUserList(db);
            // create empty academic file
            saveStudentAcademic(s);
            cout << "Registered.\n";
        }

        else if (opt == 2) {
            string user, pass;
            cout << "Username: "; cin >> user;
            cout << "Password: "; cin >> pass;
            int idx = findUserIndex(db, user);
            if (idx == -1 || db[idx].password != pass) {
                cout << "Invalid credentials.\n";
                continue;
            }
            // load student's academic file into db[idx]
            {
                string fname = "student_" + db[idx].roll + ".txt";
                ifstream fin(fname.c_str());
                db[idx].semesters.clear();
                if (fin) {
                    int semCount; fin >> semCount;
                    for (int i = 0; i < semCount; ++i) {
                        Semester sem;
                        fin >> sem.semNo;
                        int sc; fin >> sc;
                        string tmp; getline(fin, tmp); // move
                        sem.subjects.clear();
                        for (int j = 0; j < sc; ++j) {
                            string whole; getline(fin, whole);
                            vector<string> parts = splitCSV(whole, '|');
                            Subject sub;
                            if (parts.size() >= 3) {
                                sub.name = parts[0];
                                sub.credits = stoi(parts[1]);
                                sub.gradePoint = stof(parts[2]);
                            } else {
                                sub.name = "SUB"; sub.credits = 0; sub.gradePoint = 0;
                            }
                            sem.subjects.push_back(sub);
                        }
                        fin >> sem.sgpa;
                        db[idx].semesters.push_back(sem);
                    }
                    fin >> db[idx].cgpa;
                    fin.close();
                }
            }

            cout << "Login successful. Welcome " << db[idx].name << " (" << db[idx].roll << ")\n";

            // user menu
            while (true) {
                cout << "\n1) Add Semester  2) View Performance  3) Export CSV  4) Logout\nChoose: ";
                int uopt; cin >> uopt;
                if (uopt == 1) {
                    Semester sem;
                    cout << "Enter semester number: "; cin >> sem.semNo;
                    int subCount; cout << "Number of subjects: "; cin >> subCount;
                    sem.subjects.clear();
                    for (int i = 0; i < subCount; ++i) {
                        Subject sub;
                        cout << "Subject " << i+1 << " name (no commas): "; cin >> ws; getline(cin, sub.name);
                        cout << "Credits: "; cin >> sub.credits;
                        cout << "Grade point (0-10): "; cin >> sub.gradePoint;
                        sem.subjects.push_back(sub);
                    }
                    sem.sgpa = calcSGPA(sem.subjects);
                    db[idx].semesters.push_back(sem);
                    db[idx].cgpa = calcCGPA(db[idx].semesters);
                    saveStudentAcademic(db[idx]);
                    cout << "Saved. SGPA: " << fixed << setprecision(4) << sem.sgpa << "  CGPA: " << fixed << setprecision(4) << db[idx].cgpa << "\n";
                }
                else if (uopt == 2) {
                    cout << "\n--- Performance for " << db[idx].name << " (" << db[idx].roll << ") ---\n";
                    for (const auto &sem : db[idx].semesters) {
                        cout << "Semester " << sem.semNo << "  SGPA: " << fixed << setprecision(4) << sem.sgpa << "\n";
                        for (const auto &sub : sem.subjects) {
                            cout << "  " << sub.name << " | Credits: " << sub.credits << " | GP: " << sub.gradePoint << "\n";
                        }
                    }
                    cout << "CGPA: " << fixed << setprecision(4) << db[idx].cgpa << "\n";
                }
                else if (uopt == 3) {
                    exportCSV(db[idx]);
                }
                else if (uopt == 4) {
                    break;
                }
                else cout << "Invalid.\n";
            }

            // after logout, save global user list and student's academic file
            saveUserList(db);
            saveStudentAcademic(db[idx]);
        }

        else if (opt == 3) {
            showRanking(db);
        }

        else if (opt == 4) {
            cout << "Exiting...\n";
            break;
        }

        else cout << "Invalid option.\n";
    }

    return 0;
}