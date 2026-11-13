// TaskManager C++ by R.Waddilove (github.com/rwaddilove)

#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <iomanip>      // align text left or right in fixed with space
#include <format>       // text formatting - format("{:05}", n)
#include <cctype>       // remove leading/trailing white space
#include <string>
#include <chrono>       // date/time

using namespace std;
using namespace std::chrono;

constexpr string WHITE = "\033[97m";
constexpr string BLACK = "\033[30m";
constexpr string RED = "\033[31m";
constexpr string GREEN = "\033[32m";
constexpr string YELLOW = "\033[33m";
constexpr string BLUE = "\033[34m";
constexpr string MAGENTA = "\033[35m";
constexpr string CYAN = "\033[36m";
constexpr string GREY = "\033[37m";
constexpr string RESET = "\033[0m";
constexpr string CLEARSCREEN = "\x1b[2J\x1b[H";

constexpr int INTERROR = -9999;

class Task {    // title, due, repeat, label, done, notes
    public:
    string title;
    year_month_day date = year_month_day{floor<days>(system_clock::now())};
    int priority = 0;
    string repeat;
    string label;
    bool done = false;
    string notes;
};

static string lowercase(string str) {
    ranges::transform(str, str.begin(),
    [](unsigned char c){ return tolower(c); });
    return str;
}

static string Trim(const string& s) {
    const auto start = find_if_not(s.begin(), s.end(), ::isspace);
    const auto end   = find_if_not(s.rbegin(), s.rend(), ::isspace).base();
    return (start < end) ? string(start, end) : "";
}

static string InputStr(const string& prompt, const int length) {
    cout << GREEN << prompt;
    string input;
    getline(cin, input);
    cout << WHITE;
    if (input.empty()) return "";
    input = Trim(input);
    return input.substr(0, length);
}

static int InputInt(const string& prompt, const int min, const int max) {
    int number;
    try {
        number = stoi(InputStr(prompt, 6)); }
    catch (const invalid_argument& e) {
        return INTERROR; }      // no negative numbers used in this app!
    if (number < min || number > max) return INTERROR;
    return number;
}

static string InputRepeat() {
    string repeat = lowercase(InputStr("Repeat (day, week, 4week, month, year): ", 6));
    if (repeat.starts_with('d')) return "day";
    if (repeat.starts_with('w')) return "week";
    if (repeat.starts_with('4')) return "4week";
    if (repeat.starts_with('m')) return "month";
    if (repeat.starts_with('y')) return "year";
    return "none";
}

// get second part of command like 'd 2' - delete person with index 2
static int GetId(const vector<Task>& taskList, const string& input) {
    if (taskList.empty()) return INTERROR;      // no tasks = invalid id
    string s;
    int id;
    istringstream iss(input);
    if (iss >> s >> id && id >= 0 && id < taskList.size()) return id;
    id = InputInt("ID of task? ", 0, static_cast<int>(taskList.size()) - 1);
    if (id < 0) return INTERROR;
    return id;
}

static year_month_day ParseDate(const string& str) {
    int year, month, day;
    char dash1, dash2;
    istringstream iss(str);
    if (iss>>year>>dash1>>month>>dash2>>day && ((dash1=='-' && dash2=='-') || (dash1=='/' && dash2=='/'))) {
        return {
            chrono::year{year},
            chrono::month{static_cast<unsigned>(month)},
            chrono::day{static_cast<unsigned>(day)}
        };
    }
    return ParseDate("2099-12-31");
}

static void AddCSVTask(vector<Task>& taskList, string csv) {     // my .csv decoding
    if (csv.length() < 10) return;                               // a short string is obviously wrong
    csv = csv.substr(1, csv.length() - 2);                 // remove start/end quotes
    string p[7] = {"","","","","","",""};   // title,date,priority,repeat,label,done,notes
    for (auto & i : p) {
        auto p2 = csv.find("\",\"");
        i = csv.substr(0, p2);
        if (p2 + 3 < csv.length())
            csv = csv.substr(p2 + 3, string::npos);
    }
    year_month_day date = ParseDate(p[1]);
    int priority;
    try { priority = stoi(p[2]);}
    catch (const invalid_argument& e) {priority = 0;}
    bool done = p[5] == "1";
    taskList.emplace_back(p[0], date, priority, p[3], p[4], done, p[6]);
}

// ========================== get path to data file from settings file: cppTaskList.txt============
static string GetDataFolderPath() {
    // settings file stored in user home folder
#if defined(_WIN32)
    string home = getenv("USERPROFILE");    // running on PC
#else
    string home = getenv("HOME");                   // running on Mac
#endif
    char dirSep = filesystem::path::preferred_separator;
    string settings = home + dirSep + "cppTaskList.txt";    // settings file

    // read 1st line of settings file to get path to task file
    string taskFilePath;
    ifstream file(settings);
    if (file.is_open()) {
        if (getline(file, taskFilePath)) return taskFilePath;
    }

    // no settings file (should check path is valid)
    taskFilePath = InputStr("Where to store tasks? eg. c:\\users\\MyName\\TaskList.csv\nPath: ", 300);
    ofstream MyWriteFile(settings);
        MyWriteFile << taskFilePath << endl;
    return taskFilePath;
}

// ========================== read data file (in csv format) ================
static void ReadFile(vector<Task>& taskList, const string& fileName) {
    ifstream MyReadFile(fileName);
    if (!MyReadFile.is_open()) return;
    taskList.clear();
    string myText;
    while (getline (MyReadFile, myText))        // read file line by line into myText
        AddCSVTask(taskList, myText);           // add csv line as Task in taskList
    MyReadFile.close();
}

// ========================= save taskList as .csv file: "field1","field2","field3...
static void WriteFile(const vector<Task>& taskList, const string& fileName) {
    ofstream MyWriteFile(fileName);
    for (auto & task : taskList) {
        MyWriteFile << "\""
        << task.title << "\",\""
        << task.date << "\",\""
        << task.priority << "\",\""
        << task.repeat << "\",\""
        << task.label << "\",\""
        << task.done << "\",\""
        << task.notes << "\""
        << endl;
    }
    MyWriteFile.close();
    cout << "\ntaskList have been saved!\n";
}

static year_month_day LastDayofMonth(year_month_day date) {
    year_month_day date2 = date.year() / date.month() / year_month_day_last{date.year(),month_day_last{date.month(),}}.day();
    return date2;
}

static void ShowTasks(vector<Task>& taskList, const string& label, const bool& hideDone) {
    if (taskList.empty()) return;
    cout << "ID TITLE                           DATE        PRTY  REPEAT  LABEL       DONE\n";
    year_month_day noDate = ParseDate("2099-12-31");
    int id = -1;
    for (Task& task : taskList) {
        ++id;
        if (hideDone && task.done) continue;
        if (!label.empty() && label != task.label) continue;
        cout << right << setw(2) << id << " ";
        string title = task.title.length() < 28 ? task.title : task.title.substr(0, 28) + "..";
        cout << left << setw(30) << title;
        if (task.date < noDate)
            cout << "  " << (task.date.ok() ? task.date : LastDayofMonth(task.date)) << "   ";
        else
            cout << "               ";
        cout << left << setw(5) << task.priority;
        cout << left << setw(8) << task.repeat;
        cout << left << setw(12) << task.label;
        cout << (task.done ? "yes" : "");
        cout << endl;
    }
    cout << endl;
}

static void NewTask(vector<Task>& taskList) {
    cout << CYAN << "----- NEW TASK -----" << WHITE << endl;
    string title = InputStr("Title: ", 30);
    if (title.empty()) return;      // a way to cancel
    string dateStr = InputStr("Date (yyyy-mm-dd): ", 10);
    year_month_day date = ParseDate(dateStr);
    int priority = InputInt("Priority (0-5): ", 0, 5);
    if (priority < 0) priority = 0;
    string repeat = InputRepeat();
    string label = lowercase(InputStr("Label: ", 12));
    bool done = false;
    string notes = InputStr("Notes: ", 250);
    taskList.emplace_back(title, date, priority, repeat, label, done, notes);
}

static void EditTask(vector<Task>& taskList, int id) {
    if (taskList.empty()) return;
    if (id < 0 || id >= taskList.size())
        id = InputInt("ID of task? ", 0, static_cast<int>(taskList.size()) - 1);
    if (id < 0) return;

    cout << CYAN << "----- EDIT TASK -----" << WHITE << endl;
    Task& task = taskList[id];
    cout << "1 Title, 2 Date, 3 Priority, 4 Repeat, 5 Label, 6 Done, 7 Notes\n";
    int item = InputInt("Item: ", 1, 7);
    if (item < 0) return;
    if (item == 1) {
        string title = InputStr("Title: ", 30);
        if (title.empty()) return;      // a way to cancel
        task.title = title;
    }
    if (item == 2) {
        string dateStr = InputStr("Date (yyyy-mm-dd): ", 10);
        year_month_day date = ParseDate(dateStr);
        task.date = date; }
    if (item == 3) {
        int priority = InputInt("Priority (0-5): ", 0, 5);
        task.priority = priority < 0 ? 0 : priority; }
    if (item == 4) task.repeat = InputRepeat();
    if (item == 5) {
        string label = lowercase(InputStr("Label: ", 12));
        task.label = label; }
    if (item == 6)
        task.done = !task.done;
    if (item == 7)
        task.notes = InputStr("Notes: ", 250);
    InputStr("Task updated, press Enter...", 1);
}

static void ViewTask(vector<Task>& taskList, int id) {
    if (taskList.empty()) return;
    if (id < 0 || id >= taskList.size())
        id = InputInt("ID of task? ", 0, static_cast<int>(taskList.size()) - 1);
    if (id < 0) return;
    Task& task = taskList[id];
    cout << CYAN << "----- VIEW TASK -----" << WHITE << endl;
    cout << "Title: " << task.title << endl;
    cout << "Date: " << task.date << endl;
    cout << "Priority: " << task.priority << endl;
    cout << "Repeat: " << task.repeat << endl;
    cout << "Label: " << task.label << endl;
    cout << "Done: " << (task.done > 0 ? "Yes" : "No") << endl;
    cout << "Notes: " << task.notes << endl;
    string inp = InputStr("(e)dit or (c)lose: ", 2);
    if (inp.starts_with('e')) EditTask(taskList, id);
}

static void TrashTask(vector<Task>& taskList, int id) {
    if (taskList.empty()) return;
    if (id < 0 || id >= taskList.size())
        id = InputInt("ID of task? ", 0, static_cast<int>(taskList.size()) - 1);
    if (id < 0) return;
    taskList.erase(taskList.begin() + id);
    // InputStr("Task deleted, press Enter...", 1);
}

year_month_day getNewDate(year_month_day date, string& repeat) {
    sys_days sd{date};
    if (repeat == "day") sd += days{1};
    if (repeat == "week") sd += weeks{1};
    if (repeat == "4week") sd += weeks{4};
    if (repeat == "month") {
        date += months{1};
        return date; }
    date = year_month_day{sd};
    return date;
}

static void DoneTask(vector<Task>& taskList, const int& id) {
    if (id < 0) return;
    taskList[id].done = !taskList[id].done;
    if (taskList[id].repeat == "none" || !taskList[id].done) return;

    // repeating task - get next due date
    string title = taskList[id].title;
    year_month_day date = getNewDate(taskList[id].date, taskList[id].repeat);
    int priority = taskList[id].priority;
    string repeat = taskList[id].repeat;
    string label = taskList[id].label;
    bool done = false;
    string notes = taskList[id].notes;
    taskList.emplace_back(title, date, priority, repeat, label, done, notes);
}

static void ClearAllDone(vector<Task>& taskList) {
    int id = 0;
    while (id < taskList.size()){
        if (!taskList[id].done) {
            ++id;
            continue;
        }
        taskList.erase(taskList.begin() + id);
        id = 0;
    }
}

static void SortBy(vector<Task>& taskList, const string& field) {
    for (int i = 0; i < taskList.size() - 1; ++i) {
        int smallest = i;
        for (int j = i + 1; j < taskList.size(); ++j) {
            if (
                (field == "title" && lowercase(taskList[j].title) < lowercase(taskList[smallest].title)) ||
                (field == "date" && taskList[j].date < taskList[smallest].date) ||
                (field == "label" && taskList[j].label < taskList[smallest].label) ||
                (field == "priority" && taskList[j].priority > taskList[smallest].priority)
                ) smallest = j;
        }
        if (smallest != i) swap(taskList[smallest], taskList[i]);
    }
}

static void SortTasks(vector<Task>& taskList) {
    if (taskList.size() < 4) {
        InputStr("Not enough tasks to sort. Press Enter...", 1);
        return;
    }
    string s = InputStr("Sort tasks by (d)ate, (p)riority, (t)itle, (l)abel: ", 5);
    if (s.starts_with('d')) SortBy(taskList, "date");
    if (s.starts_with('p')) SortBy(taskList, "priority");
    if (s.starts_with('t')) SortBy(taskList, "title");
    if (s.starts_with('l')) SortBy(taskList, "label");
}

int main() {
    vector<Task> taskList;
    const string fpath = GetDataFolderPath();
    ReadFile(taskList, fpath);
    string filter;
    bool hideDone = true;
    while (true) {
        cout << CLEARSCREEN << CYAN;
        cout << "-----------------------------------\n";
        cout << "   T A S K   M A N A G E R C++\n";
        cout << "-----------------------------------" << YELLOW << endl;
        ShowTasks(taskList, filter, hideDone);
        string inp = InputStr("(n)ew, (v)iew, (e)dit, (f)ilter, (d)one, (c)lear Done, (h)idden, (s)ort, (t)rash, (q)uit: ",9);
        if (inp.empty()) continue;
        if (inp.starts_with('n')) NewTask(taskList);
        if (inp.starts_with('h')) hideDone = !hideDone;
        if (inp.starts_with('c')) ClearAllDone(taskList);
        if (inp.starts_with('f')) filter = InputStr("Filter label: ", 15);
        if (inp.starts_with('v')) ViewTask(taskList, GetId(taskList, inp));
        if (inp.starts_with('e')) EditTask(taskList, GetId(taskList, inp));
        if (inp.starts_with('d')) DoneTask(taskList, GetId(taskList, inp));
        if (inp.starts_with('s')) SortTasks(taskList);
        if (inp.starts_with('t')) TrashTask(taskList, GetId(taskList, inp));
        if (inp.starts_with('q')) break;
    }
    string inp = InputStr("Save " + fpath + " (y/n)? ", 1);
    if (inp.starts_with('y')) WriteFile(taskList, fpath);
    return 0;
}