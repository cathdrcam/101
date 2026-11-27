/*=====================================================================
  REPAIRATORY - Repair Shop Job Order System
  CC 101 - Computer Programming | Final Project
  Console-based, pure C++ (no libraries beyond the standard ones).
  Written to compile even on older IDEs (Dev-C++, Code::Blocks).
  (Simplified: same functions and features, simpler code style.)

  WHAT IT DOES
    HOME PAGE                : welcome screen shown first; explains what
                               Repairatory is about
    [1] Customer Management  : New / View All / Edit / Delete
    [2] Job Order Management : New / View All / Update Status / Delete
    [3] Search and Sort      : search a customer or a job order,
                               group job orders by status or device type
    [4] Summary Report       : ONE page of counts, percentages, a
                               status-by-device table, customer analysis
    [5] Exit

  FILE HANDLING (saved after EVERY change, loaded when the program starts)
    customers.txt -> line 1 : last customer ID used
                     others : id|name|telephone
    joborders.txt -> line 1 : last job order ID used
                     others : id|customerId|status|device|model|issue

  HOW THE DATA IS STORED (parallel arrays: row i of every array is the
  same record)
    Numbers  -> 1D arrays  (IDs, status, device)
    Text     -> 2D arrays  ([i][0], [i][1] ...)
=====================================================================*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cctype>
#include <cstdlib>
using namespace std;

// ====================================================================
//  CONSTANTS
// ====================================================================
const int MAX_CUST = 100;      // maximum number of customers
const int MAX_JOB  = 200;      // maximum number of job orders
const int MAX_TEXT = 40;       // maximum length of any typed text
const int MAX_ID   = 999999;   // largest ID number the user may type
const int PAGE_W   = 55;       // width of the one-page summary report

// Repair status codes (stored as numbers, shown as words)
const int PENDING     = 1;
const int IN_PROGRESS = 2;
const int COMPLETED   = 3;
const int RELEASED    = 4;

const string CUST_FILE = "customers.txt";
const string JOB_FILE  = "joborders.txt";

// ====================================================================
//  GLOBAL DATA (the "database" of the program)
// ====================================================================
// ---- Customers ----
int    custId[MAX_CUST];            // 1D: customer ID numbers (1001, 1002...)
string custText[MAX_CUST][2];       // 2D: [0] name, [1] telephone
int    custCount  = 0;              // how many customers are stored
int    lastCustId = 1000;           // last ID given out (never reused)

// ---- Job orders ----
int    jobId[MAX_JOB];              // 1D: job order ID numbers (5001, 5002...)
int    jobCustId[MAX_JOB];          // 1D: ID of the customer who owns the job
int    jobStatus[MAX_JOB];          // 1D: 1=Pending 2=In Progress 3=Completed 4=Released
int    jobDevice[MAX_JOB];          // 1D: 1=Phone 2=Laptop 3=Tablet 4=Other
string jobText[MAX_JOB][2];         // 2D: [0] brand/model, [1] issue
int    jobCount  = 0;               // how many job orders are stored
int    lastJobId = 5000;            // last ID given out (never reused)

// ====================================================================
//  FUNCTION DECLARATIONS (prototypes)
// ====================================================================
// ---- screen and text helpers ----
void   clearScreen();
void   pauseScreen();
void   readLine(string& line);
void   cancelled();
string trim(const string& s);
string toLower(const string& s);
bool   isAllDigits(const string& s);
string intToStr(int n);
string fmt(double value, int decimals);
string pct(double value);
string shorten(const string& s, int width);
string label(const string& prefix, int id);
string statusText(int s);
string deviceText(int d);

// ---- calculation helpers ----
double percent(int part, int total);
double average(int sum, int count);

// ---- input helpers (all input goes through these) ----
int    getInt(const string& prompt, int minVal, int maxVal);
bool   getText(const string& prompt, string& result);
bool   isPhoneOk(const string& s);
bool   getPhone(const string& prompt, string& result);
bool   askYesNo(const string& prompt);

// ---- finding records ----
int    findCustomer(int id);
int    findJob(int id);
int    countJobsOf(int custIdx);
int    pickCustomer();

// ---- customers ----
void   printCustomerHeader();
void   printCustomerRow(int i);
void   printCustomerTable();
void   addCustomer();
void   viewCustomers();
void   editCustomer();
void   removeCustomerAt(int idx);
void   deleteCustomer();

// ---- job orders ----
void   printJobHeader();
void   printJobRow(int i);
void   printJobTable();
void   showJobDetails(int idx);
void   addJob();
void   viewJobs();
void   updateStatus();
void   removeJobAt(int idx);
void   deleteJob();

// ---- search and sort ----
void   searchCustomer();
void   searchJob();
void   sortJobs(int order[], int key[]);
void   printGrouped(int order[], int key[], bool byStatus);
void   showSorted(int key[], bool byStatus);

// ---- summary report ----
void   printTitle(const string& title);
void   printSection(const string& title);
void   summaryReport();

// ---- files ----
int    splitLine(const string& line, string parts[], int maxParts);
void   saveAll();
void   loadAll();

// ---- home page ----
void   printCentered(const string& text);
void   homePage();

// ---- menus ----
void   customerMenu();
void   jobMenu();
void   searchSortMenu();
void   mainMenu();

// ====================================================================
//  MAIN
// ====================================================================
int main() {
    loadAll();      // read saved records (if the files exist)
    homePage();     // welcome screen: what Repairatory is about
    mainMenu();     // run the program
    return 0;
}

// ====================================================================
//  SCREEN AND TEXT HELPERS
// ====================================================================
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Reads one full line. If the input stream is closed, leave safely
// (nothing is lost because the data is saved after every change).
void readLine(string& line) {
    if (!getline(cin, line)) {
        cout << endl << "Input closed. Exiting program." << endl;
        exit(0);
    }
}

void pauseScreen() {
    string dummy;
    cout << endl << "Press Enter to continue...";
    readLine(dummy);
}

void cancelled() {
    cout << "Cancelled." << endl;
    pauseScreen();
}

// Removes spaces/tabs at the start and end of a string
string trim(const string& s) {
    int startPos = 0;
    int endPos = (int)s.length() - 1;
    while (startPos <= endPos && isspace((unsigned char)s[startPos])) {
        startPos++;
    }
    while (endPos >= startPos && isspace((unsigned char)s[endPos])) {
        endPos--;
    }
    return s.substr(startPos, endPos - startPos + 1);
}

string toLower(const string& s) {
    string result = s;
    for (int i = 0; i < (int)result.length(); i++) {
        result[i] = (char)tolower((unsigned char)result[i]);
    }
    return result;
}

bool isAllDigits(const string& s) {
    if (s.empty()) {
        return false;
    }
    for (int i = 0; i < (int)s.length(); i++) {
        if (!isdigit((unsigned char)s[i])) {
            return false;
        }
    }
    return true;
}

string intToStr(int n) {
    ostringstream out;
    out << n;
    return out.str();
}

// Number -> text with a fixed number of decimals, e.g. fmt(1.6, 2) = "1.60"
string fmt(double value, int decimals) {
    ostringstream out;
    out << fixed << setprecision(decimals) << value;
    return out.str();
}

string pct(double value) {
    return fmt(value, 1) + "%";
}

// Cuts long text so tables stay neat
string shorten(const string& s, int width) {
    if ((int)s.length() <= width) {
        return s;
    }
    return s.substr(0, width - 2) + "..";
}

// label("C", 1001) -> "C1001"      label("JO", 5001) -> "JO5001"
string label(const string& prefix, int id) {
    return prefix + intToStr(id);
}

string statusText(int s) {
    switch (s) {
        case PENDING:     return "Pending";
        case IN_PROGRESS: return "In Progress";
        case COMPLETED:   return "Completed";
        case RELEASED:    return "Released";
    }
    return "Unknown";
}

string deviceText(int d) {
    switch (d) {
        case 1: return "Phone";
        case 2: return "Laptop";
        case 3: return "Tablet";
        case 4: return "Other";
    }
    return "Unknown";
}

// ====================================================================
//  CALCULATION HELPERS
// ====================================================================
// part / total x 100, but never divides by zero
double percent(int part, int total) {
    if (total == 0) {
        return 0.0;
    }
    return part * 100.0 / total;
}

double average(int sum, int count) {
    if (count == 0) {
        return 0.0;
    }
    return (double)sum / count;
}

// ====================================================================
//  INPUT HELPERS - typing letters in a number prompt can never crash
// ====================================================================
// Keeps asking until the user types a whole number from minVal to maxVal
int getInt(const string& prompt, int minVal, int maxVal) {
    string line;
    while (true) {
        cout << prompt;
        readLine(line);
        line = trim(line);

        if (line.empty()) {
            cout << "  Input cannot be empty." << endl;
        } else if (line.length() > 9 || !isAllDigits(line)) {
            cout << "  Invalid input. Please type a whole number only." << endl;
        } else {
            int value = atoi(line.c_str());
            if (value >= minVal && value <= maxVal) {
                return value;
            }
            cout << "  Number must be from " << minVal << " to " << maxVal << "." << endl;
        }
    }
}

// Asks for text. Returns false if the user typed 0 (cancel).
// Rejects empty text, text with the | character, and very long text.
bool getText(const string& prompt, string& result) {
    string line;
    while (true) {
        cout << prompt << " (0 = Cancel): ";
        readLine(line);
        line = trim(line);

        if (line == "0") {
            return false;
        }
        if (line.empty()) {
            cout << "  This field cannot be empty." << endl;
        } else if (line.find('|') != string::npos) {
            cout << "  The | character is not allowed." << endl;
        } else if ((int)line.length() > MAX_TEXT) {
            cout << "  Too long (maximum " << MAX_TEXT << " characters)." << endl;
        } else {
            result = line;
            return true;
        }
    }
}

// A phone number may contain digits, + - and spaces (7 to 15 digits)
bool isPhoneOk(const string& s) {
    int digits = 0;
    for (int i = 0; i < (int)s.length(); i++) {
        char c = s[i];
        if (isdigit((unsigned char)c)) {
            digits++;
        } else if (c != '+' && c != '-' && c != ' ') {
            return false;
        }
    }
    return digits >= 7 && digits <= 15;
}

bool getPhone(const string& prompt, string& result) {
    while (true) {
        if (!getText(prompt, result)) {
            return false;
        }
        if (isPhoneOk(result)) {
            return true;
        }
        cout << "  Use digits only (+ - and spaces allowed), 7 to 15 digits." << endl;
    }
}

bool askYesNo(const string& prompt) {
    string line;
    while (true) {
        cout << prompt << " (Y/N): ";
        readLine(line);
        line = trim(line);
        if (line == "Y" || line == "y") {
            return true;
        }
        if (line == "N" || line == "n") {
            return false;
        }
        cout << "  Please type Y or N." << endl;
    }
}

// ====================================================================
//  FINDING RECORDS (linear search)
// ====================================================================
// Returns the array position of the customer, or -1 if not found
int findCustomer(int id) {
    for (int i = 0; i < custCount; i++) {
        if (custId[i] == id) {
            return i;
        }
    }
    return -1;
}

int findJob(int id) {
    for (int i = 0; i < jobCount; i++) {
        if (jobId[i] == id) {
            return i;
        }
    }
    return -1;
}

// How many job orders belong to the customer at position custIdx
int countJobsOf(int custIdx) {
    int n = 0;
    for (int j = 0; j < jobCount; j++) {
        if (jobCustId[j] == custId[custIdx]) {
            n++;
        }
    }
    return n;
}

// Lets the user choose a customer by ID ("1001" or "C1001") or by name
// (not case-sensitive, partial names work).
// Returns: array position of the customer
//          -1 = not found / nothing chosen (a message was already shown)
//          -2 = user typed 0 to cancel
int pickCustomer() {
    if (custCount == 0) {
        cout << "There are no customers yet. Add a customer first." << endl;
        return -1;
    }

    string input;
    if (!getText("Enter customer ID or name", input)) {
        return -2;
    }

    // 1) Did the user type an ID?
    string digits = input;
    if (digits.length() > 1 && (digits[0] == 'C' || digits[0] == 'c')) {
        digits = digits.substr(1);
    }
    if (isAllDigits(digits) && digits.length() <= 9) {
        int idx = findCustomer(atoi(digits.c_str()));
        if (idx == -1) {
            cout << "No customer has that ID." << endl;
        }
        return idx;
    }

    // 2) Otherwise it is a name: collect every partial match
    int matches[MAX_CUST];
    int found = 0;
    string wanted = toLower(input);
    for (int i = 0; i < custCount; i++) {
        if (toLower(custText[i][0]).find(wanted) != string::npos) {
            matches[found] = i;
            found++;
        }
    }

    if (found == 0) {
        cout << "No customer name matches \"" << input << "\"." << endl;
        return -1;
    }
    if (found == 1) {
        return matches[0];
    }

    cout << endl << "Several customers match:" << endl;
    for (int k = 0; k < found; k++) {
        cout << "  " << label("C", custId[matches[k]]) << "  "
             << custText[matches[k]][0] << endl;
    }
    int id = getInt("Enter the ID number of the one you want (0 = Cancel): ", 0, MAX_ID);
    if (id == 0) {
        return -2;
    }
    int chosen = findCustomer(id);
    for (int k = 0; k < found; k++) {
        if (matches[k] == chosen) {
            return chosen;
        }
    }
    cout << "That ID is not in the list above." << endl;
    return -1;
}

// ====================================================================
//  CUSTOMER MANAGEMENT
// ====================================================================
void printCustomerHeader() {
    cout << left << " " << setw(8) << "ID" << setw(32) << "NAME"
         << setw(20) << "TELEPHONE" << "JOBS" << endl;
    cout << " " << string(64, '-') << endl;
}

void printCustomerRow(int i) {
    cout << left << " " << setw(8) << label("C", custId[i])
         << setw(32) << shorten(custText[i][0], 30)
         << setw(20) << shorten(custText[i][1], 18)
         << countJobsOf(i) << endl;
}

void printCustomerTable() {
    if (custCount == 0) {
        cout << "No customers yet." << endl;
        return;
    }
    printCustomerHeader();
    for (int i = 0; i < custCount; i++) {
        printCustomerRow(i);
    }
    cout << endl << "Total customers: " << custCount << endl;
}

void addCustomer() {
    clearScreen();
    cout << "=========== NEW CUSTOMER ===========" << endl << endl;

    if (custCount >= MAX_CUST) {
        cout << "Storage full! (maximum " << MAX_CUST << " customers)" << endl;
        pauseScreen();
        return;
    }

    string name, phone;
    if (!getText("Customer name", name))      { cancelled(); return; }
    if (!getPhone("Telephone number", phone)) { cancelled(); return; }

    lastCustId++;                              // new ID = last ID + 1
    custId[custCount]      = lastCustId;
    custText[custCount][0] = name;
    custText[custCount][1] = phone;
    custCount++;

    saveAll();
    cout << endl << "Customer added. ID: " << label("C", lastCustId) << endl;
    pauseScreen();
}

void viewCustomers() {
    clearScreen();
    cout << "============ ALL CUSTOMERS ============" << endl << endl;
    printCustomerTable();
    pauseScreen();
}

void editCustomer() {
    clearScreen();
    cout << "=========== EDIT CUSTOMER ===========" << endl << endl;

    int idx = pickCustomer();
    if (idx == -2) {
        return;
    }
    if (idx < 0)   { pauseScreen(); return; }

    cout << endl << "Customer: " << label("C", custId[idx]) << "  "
         << custText[idx][0] << "  /  " << custText[idx][1] << endl;
    cout << "  [1] Change name" << endl;
    cout << "  [2] Change telephone" << endl;
    cout << "  [0] Cancel" << endl;
    int choice = getInt("Choose (0-2): ", 0, 2);

    string newValue;
    if (choice == 1) {
        if (!getText("New name", newValue)) { cancelled(); return; }
        custText[idx][0] = newValue;
    } else if (choice == 2) {
        if (!getPhone("New telephone number", newValue)) { cancelled(); return; }
        custText[idx][1] = newValue;
    } else {
        cancelled();
        return;
    }

    saveAll();
    cout << "Customer updated." << endl;
    pauseScreen();
}

// Shifts later records up by one position to close the gap
void removeCustomerAt(int idx) {
    for (int i = idx; i < custCount - 1; i++) {
        custId[i]      = custId[i + 1];
        custText[i][0] = custText[i + 1][0];
        custText[i][1] = custText[i + 1][1];
    }
    custCount--;
}

void deleteCustomer() {
    clearScreen();
    cout << "========== DELETE CUSTOMER ==========" << endl << endl;

    int idx = pickCustomer();
    if (idx == -2) {
        return;
    }
    if (idx < 0)   { pauseScreen(); return; }

    cout << endl << "Customer: " << label("C", custId[idx]) << "  "
         << custText[idx][0] << "  /  " << custText[idx][1] << endl;

    int n = countJobsOf(idx);
    if (n > 0) {
        cout << endl << "Cannot delete: this customer still has " << n
             << " job order(s)." << endl << "Delete those job orders first." << endl;
        pauseScreen();
        return;
    }

    if (askYesNo("Are you sure you want to delete this customer")) {
        removeCustomerAt(idx);
        saveAll();
        cout << "Customer deleted." << endl;
    } else {
        cout << "Deletion cancelled." << endl;
    }
    pauseScreen();
}

// ====================================================================
//  JOB ORDER MANAGEMENT
// ====================================================================
void printJobHeader() {
    cout << left << " " << setw(8) << "JO ID" << setw(16) << "CUSTOMER"
         << setw(8) << "DEVICE" << setw(14) << "MODEL"
         << setw(18) << "ISSUE" << "STATUS" << endl;
    cout << " " << string(75, '-') << endl;
}

void printJobRow(int i) {
    int ci = findCustomer(jobCustId[i]);
    string name;
    if (ci == -1) {
        name = "(unknown)";
    } else {
        name = custText[ci][0];
    }

    cout << left << " " << setw(8) << label("JO", jobId[i])
         << setw(16) << shorten(name, 15)
         << setw(8)  << deviceText(jobDevice[i])
         << setw(14) << shorten(jobText[i][0], 13)
         << setw(18) << shorten(jobText[i][1], 17)
         << statusText(jobStatus[i]) << endl;
}

void printJobTable() {
    if (jobCount == 0) {
        cout << "No job orders yet." << endl;
        return;
    }
    printJobHeader();
    for (int i = 0; i < jobCount; i++) {
        printJobRow(i);
    }
    cout << endl << "Total job orders: " << jobCount << endl;
}

// Full details of one job order (used by search and delete)
void showJobDetails(int idx) {
    int ci = findCustomer(jobCustId[idx]);

    cout << endl << " JO ID       : " << label("JO", jobId[idx]) << endl;
    if (ci == -1) {
        cout << " Customer    : (unknown)" << endl;
    } else {
        cout << " Customer    : " << label("C", custId[ci]) << "  " << custText[ci][0] << endl;
        cout << " Telephone   : " << custText[ci][1] << endl;
    }
    cout << " Device      : " << deviceText(jobDevice[idx]) << endl;
    cout << " Brand/Model : " << jobText[idx][0] << endl;
    cout << " Issue       : " << jobText[idx][1] << endl;
    cout << " Status      : " << statusText(jobStatus[idx]) << endl;
}

void addJob() {
    clearScreen();
    cout << "=========== NEW JOB ORDER ===========" << endl << endl;

    if (jobCount >= MAX_JOB) {
        cout << "Storage full! (maximum " << MAX_JOB << " job orders)" << endl;
        pauseScreen();
        return;
    }

    int ci = pickCustomer();
    if (ci == -2) {
        return;
    }
    if (ci < 0)   { pauseScreen(); return; }
    cout << endl << "Customer: " << label("C", custId[ci]) << "  " << custText[ci][0] << endl << endl;

    cout << "Device type:" << endl;
    for (int d = 1; d <= 4; d++) {
        cout << "  [" << d << "] " << deviceText(d) << endl;
    }
    cout << "  [0] Cancel" << endl;
    int device = getInt("Choose device type (0-4): ", 0, 4);
    if (device == 0) { cancelled(); return; }

    string model, issue;
    if (!getText("Device brand / model", model)) { cancelled(); return; }
    if (!getText("Problem / issue", issue))      { cancelled(); return; }

    lastJobId++;                               // new ID = last ID + 1
    jobId[jobCount]      = lastJobId;
    jobCustId[jobCount]  = custId[ci];
    jobStatus[jobCount]  = PENDING;            // every new job starts as Pending
    jobDevice[jobCount]  = device;
    jobText[jobCount][0] = model;
    jobText[jobCount][1] = issue;
    jobCount++;

    saveAll();
    cout << endl << "Job order created. ID: " << label("JO", lastJobId)
         << "   Status: Pending" << endl;
    pauseScreen();
}

void viewJobs() {
    clearScreen();
    cout << "============ ALL JOB ORDERS ============" << endl << endl;
    printJobTable();
    pauseScreen();
}

void updateStatus() {
    clearScreen();
    cout << "======== UPDATE JOB ORDER STATUS ========" << endl << endl;

    if (jobCount == 0) {
        cout << "No job orders yet." << endl;
        pauseScreen();
        return;
    }
    printJobTable();

    cout << endl;
    int id = getInt("Enter JO ID number (0 = Cancel): ", 0, MAX_ID);
    if (id == 0) {
        return;
    }

    int idx = findJob(id);
    if (idx == -1) {
        cout << "No job order has that ID." << endl;
        pauseScreen();
        return;
    }

    cout << endl << label("JO", id) << " is currently: "
         << statusText(jobStatus[idx]) << endl;
    for (int s = PENDING; s <= RELEASED; s++) {
        cout << "  [" << s << "] " << statusText(s) << endl;
    }
    cout << "  [0] Cancel" << endl;
    int newStatus = getInt("Choose the new status (0-4): ", 0, 4);

    if (newStatus == 0) { cancelled(); return; }
    if (newStatus == jobStatus[idx]) {
        cout << "The job order already has that status." << endl;
        pauseScreen();
        return;
    }

    jobStatus[idx] = newStatus;
    saveAll();
    cout << "Status updated to " << statusText(newStatus) << "." << endl;
    pauseScreen();
}

// Shifts later records up by one position to close the gap
void removeJobAt(int idx) {
    for (int i = idx; i < jobCount - 1; i++) {
        jobId[i]      = jobId[i + 1];
        jobCustId[i]  = jobCustId[i + 1];
        jobStatus[i]  = jobStatus[i + 1];
        jobDevice[i]  = jobDevice[i + 1];
        jobText[i][0] = jobText[i + 1][0];
        jobText[i][1] = jobText[i + 1][1];
    }
    jobCount--;
}

void deleteJob() {
    clearScreen();
    cout << "========== DELETE JOB ORDER ==========" << endl << endl;

    if (jobCount == 0) {
        cout << "No job orders yet." << endl;
        pauseScreen();
        return;
    }
    printJobTable();

    cout << endl;
    int id = getInt("Enter JO ID number to delete (0 = Cancel): ", 0, MAX_ID);
    if (id == 0) {
        return;
    }

    int idx = findJob(id);
    if (idx == -1) {
        cout << "No job order has that ID." << endl;
        pauseScreen();
        return;
    }

    showJobDetails(idx);
    cout << endl;
    if (askYesNo("Are you sure you want to delete this job order")) {
        removeJobAt(idx);
        saveAll();
        cout << "Job order deleted." << endl;
    } else {
        cout << "Deletion cancelled." << endl;
    }
    pauseScreen();
}

// ====================================================================
//  SEARCH AND SORT
// ====================================================================
// Shows one customer and every job order that belongs to them
void searchCustomer() {
    clearScreen();
    cout << "=========== SEARCH CUSTOMER ===========" << endl << endl;

    int idx = pickCustomer();
    if (idx == -2) {
        return;
    }
    if (idx < 0)   { pauseScreen(); return; }

    cout << endl << "Customer  : " << label("C", custId[idx]) << "  " << custText[idx][0] << endl;
    cout << "Telephone : " << custText[idx][1] << endl << endl;

    int total = countJobsOf(idx);
    if (total == 0) {
        cout << "This customer has no job orders yet." << endl;
    } else {
        int openJobs = 0;
        int finishedJobs = 0;
        printJobHeader();
        for (int j = 0; j < jobCount; j++) {
            if (jobCustId[j] == custId[idx]) {
                printJobRow(j);
                if (jobStatus[j] <= IN_PROGRESS) {
                    openJobs++;
                } else {
                    finishedJobs++;
                }
            }
        }
        cout << endl << "Total job orders: " << total << "   (Open: " << openJobs
             << ", Finished: " << finishedJobs << ")" << endl;
    }
    pauseScreen();
}

void searchJob() {
    clearScreen();
    cout << "=========== SEARCH JOB ORDER ===========" << endl << endl;

    if (jobCount == 0) {
        cout << "No job orders yet." << endl;
        pauseScreen();
        return;
    }

    int id = getInt("Enter JO ID number (0 = Cancel): ", 0, MAX_ID);
    if (id == 0) {
        return;
    }

    int idx = findJob(id);
    if (idx == -1) {
        cout << "No job order has that ID." << endl;
    } else {
        showJobDetails(idx);
    }
    pauseScreen();
}

// BUBBLE SORT on an index array.
// order[] receives the positions 0..jobCount-1, then it is rearranged so
// that key[order[0]] <= key[order[1]] <= ...  The real records never move.
// key[] can be jobStatus or jobDevice - the same function serves both.
void sortJobs(int order[], int key[]) {
    for (int i = 0; i < jobCount; i++) {
        order[i] = i;
    }
    for (int pass = 0; pass < jobCount - 1; pass++) {
        for (int j = 0; j < jobCount - 1 - pass; j++) {
            if (key[order[j]] > key[order[j + 1]]) {
                int temp     = order[j];        // swap the two positions
                order[j]     = order[j + 1];
                order[j + 1] = temp;
            }
        }
    }
}

// Prints the job orders in sorted order, with a heading for each group
void printGrouped(int order[], int key[], bool byStatus) {
    int p = 0;
    while (p < jobCount) {
        int groupKey = key[order[p]];
        int q = p;
        while (q < jobCount && key[order[q]] == groupKey) q++;   // end of group

        string groupName;
        if (byStatus) {
            groupName = statusText(groupKey);
        } else {
            groupName = deviceText(groupKey);
        }
        cout << endl << "--- " << groupName << " (" << (q - p) << ") ---" << endl;
        printJobHeader();
        for (int r = p; r < q; r++) {
            printJobRow(order[r]);
        }
        p = q;
    }
}

void showSorted(int key[], bool byStatus) {
    clearScreen();
    if (byStatus) {
        cout << "====== JOB ORDERS GROUPED BY REPAIR STATUS ======" << endl;
    } else {
        cout << "====== JOB ORDERS GROUPED BY DEVICE TYPE ======" << endl;
    }

    if (jobCount == 0) {
        cout << endl << "No job orders yet." << endl;
        pauseScreen();
        return;
    }

    int order[MAX_JOB];
    sortJobs(order, key);
    printGrouped(order, key, byStatus);
    cout << endl << "Total job orders: " << jobCount << endl;
    pauseScreen();
}

// ====================================================================
//  SUMMARY REPORT (one page)
// ====================================================================
void printTitle(const string& title) {
    string text = " " + title + " ";
    int space = PAGE_W - (int)text.length();
    int leftPad = space / 2;
    int rightPad = space - leftPad;
    cout << string(leftPad, '=') << text << string(rightPad, '=') << endl;
}

void printSection(const string& title) {
    string text = "--- " + title + " ";
    cout << text << string(PAGE_W - (int)text.length(), '-') << endl;
}

void summaryReport() {
    clearScreen();

    // ---- 1) Count everything with ONE pass over the job orders ----
    int tally[4][4];        // tally[status-1][device-1] = number of job orders
    int rowTotal[4];        // job orders per status
    int colTotal[4];        // job orders per device
    for (int s = 0; s < 4; s++) {
        rowTotal[s] = 0;
        colTotal[s] = 0;
        for (int d = 0; d < 4; d++) {
            tally[s][d] = 0;
        }
    }
    for (int j = 0; j < jobCount; j++) {
        tally[jobStatus[j] - 1][jobDevice[j] - 1]++;
    }
    for (int s = 0; s < 4; s++) {
        for (int d = 0; d < 4; d++) {
            rowTotal[s] += tally[s][d];
            colTotal[d] += tally[s][d];
        }
    }

    int total = jobCount;
    int openJobs     = rowTotal[PENDING - 1] + rowTotal[IN_PROGRESS - 1];
    int finishedJobs = rowTotal[COMPLETED - 1] + rowTotal[RELEASED - 1];

    // ---- 2) Customer analysis ----
    string bucketName[4] = { "0 jobs", "1 job", "2 jobs", "3+ jobs" };
    int bucket[4] = { 0, 0, 0, 0 };       // customers with 0 / 1 / 2 / 3+ jobs
    int topIdx = -1;                      // customer with the most jobs
    int topJobs = 0;
    for (int c = 0; c < custCount; c++) {
        int n = countJobsOf(c);
        if (n == 0) {
            bucket[0]++;
        } else if (n == 1) {
            bucket[1]++;
        } else if (n == 2) {
            bucket[2]++;
        } else {
            bucket[3]++;
        }
        if (n > topJobs) {
            topJobs = n;
            topIdx = c;
        }
    }

    // ---- 3) Print the page ----
    printTitle("REPAIRATORY SUMMARY REPORT");
    cout << " Customers: " << custCount << "    |    Job Orders: " << total << endl;

    printSection("BY REPAIR STATUS");
    cout << " " << left << setw(14) << "STATUS" << right << setw(8) << "JOBS"
         << setw(10) << "PERCENT" << endl;
    for (int s = 0; s < 4; s++) {
        cout << " " << left << setw(14) << statusText(s + 1) << right
             << setw(8) << rowTotal[s]
             << setw(10) << pct(percent(rowTotal[s], total)) << endl;
    }
    cout << " " << left << setw(14) << "TOTAL" << right << setw(8) << total
         << setw(10) << pct(percent(total, total)) << endl;

    printSection("PROGRESS");
    cout << " Open " << openJobs << " (" << pct(percent(openJobs, total))
         << ")  |  Finished " << finishedJobs << " ("
         << pct(percent(finishedJobs, total)) << ")" << endl;
    cout << " Waiting for pickup: " << rowTotal[COMPLETED - 1]
         << "  |  Release rate: " << pct(percent(rowTotal[RELEASED - 1], total)) << endl;

    printSection("STATUS BY DEVICE");
    cout << " " << left << setw(14) << "STATUS" << right
         << setw(8) << "PHONE" << setw(8) << "LAPTOP" << setw(8) << "TABLET"
         << setw(8) << "OTHER" << setw(8) << "TOTAL" << endl;
    for (int s = 0; s < 4; s++) {
        cout << " " << left << setw(14) << statusText(s + 1) << right;
        for (int d = 0; d < 4; d++) {
            cout << setw(8) << tally[s][d];
        }
        cout << setw(8) << rowTotal[s] << endl;
    }
    cout << " " << left << setw(14) << "TOTAL" << right;
    for (int d = 0; d < 4; d++) {
        cout << setw(8) << colTotal[d];
    }
    cout << setw(8) << total << endl;

    cout << " " << left << setw(14) << "SHARE" << right;                 // % of all jobs
    for (int d = 0; d < 4; d++) {
        cout << setw(8) << pct(percent(colTotal[d], total));
    }
    cout << setw(8) << pct(percent(total, total)) << endl;

    cout << " " << left << setw(14) << "DONE %" << right;                // finished per device
    for (int d = 0; d < 4; d++) {
        int doneCount = tally[COMPLETED - 1][d] + tally[RELEASED - 1][d];
        cout << setw(8) << pct(percent(doneCount, colTotal[d]));
    }
    cout << setw(8) << pct(percent(finishedJobs, total)) << endl;

    printSection("CUSTOMERS");
    cout << " " << left << setw(20) << "JOBS PER CUSTOMER" << right
         << setw(11) << "CUSTOMERS" << setw(10) << "PERCENT" << endl;
    for (int b = 0; b < 4; b++) {
        cout << " " << left << setw(20) << bucketName[b] << right
             << setw(11) << bucket[b]
             << setw(10) << pct(percent(bucket[b], custCount)) << endl;
    }
    cout << " Avg jobs per customer: " << fmt(average(total, custCount), 2)
         << "  |  Repeat customers (2+): "
         << pct(percent(bucket[2] + bucket[3], custCount)) << endl;
    if (topIdx == -1) {
        cout << " Top customer: none yet" << endl;
    } else {
        cout << " Top customer: " << custText[topIdx][0] << " (" << topJobs;
        if (topJobs == 1) {
            cout << " job)" << endl;
        } else {
            cout << " jobs)" << endl;
        }
    }
    cout << string(PAGE_W, '=') << endl;

    pauseScreen();
}

// ====================================================================
//  FILE HANDLING
// ====================================================================
// Splits "a|b|c" into parts[0]="a", parts[1]="b", parts[2]="c".
// Returns how many pieces the line has.
int splitLine(const string& line, string parts[], int maxParts) {
    int n = 0;
    string current = "";
    for (int i = 0; i < (int)line.length(); i++) {
        if (line[i] == '|') {
            if (n < maxParts) {
                parts[n] = current;
            }
            n++;
            current = "";
        } else {
            current += line[i];
        }
    }
    if (n < maxParts) {
        parts[n] = current;
    }
    n++;
    return n;
}

void saveAll() {
    // ---- customers ----
    ofstream cf(CUST_FILE.c_str());
    if (!cf) {
        cout << "WARNING: could not save " << CUST_FILE << endl;
    } else {
        cf << lastCustId << endl;
        for (int i = 0; i < custCount; i++) {
            cf << custId[i] << "|" << custText[i][0] << "|" << custText[i][1] << endl;
        }
        cf.close();
    }

    // ---- job orders ----
    ofstream jf(JOB_FILE.c_str());
    if (!jf) {
        cout << "WARNING: could not save " << JOB_FILE << endl;
    } else {
        jf << lastJobId << endl;
        for (int i = 0; i < jobCount; i++) {
            jf << jobId[i] << "|" << jobCustId[i] << "|" << jobStatus[i] << "|"
               << jobDevice[i] << "|" << jobText[i][0] << "|" << jobText[i][1] << endl;
        }
        jf.close();
    }
}

void loadAll() {
    string line;
    string parts[6];

    // ---- customers ----
    ifstream cf(CUST_FILE.c_str());
    if (cf) {
        if (getline(cf, line)) {
            lastCustId = atoi(line.c_str());
        }
        while (custCount < MAX_CUST && getline(cf, line)) {
            line = trim(line);
            int n = 0;
            if (!line.empty()) {
                n = splitLine(line, parts, 6);
            }
            if (n == 3 && isAllDigits(parts[0])) {
                custId[custCount]      = atoi(parts[0].c_str());
                custText[custCount][0] = parts[1];
                custText[custCount][1] = parts[2];
                if (custId[custCount] > lastCustId) {
                    lastCustId = custId[custCount];
                }
                custCount++;
            }
        }
        cf.close();
    }
    if (lastCustId < 1000) {
        lastCustId = 1000;
    }

    // ---- job orders ----
    ifstream jf(JOB_FILE.c_str());
    if (jf) {
        if (getline(jf, line)) {
            lastJobId = atoi(line.c_str());
        }
        while (jobCount < MAX_JOB && getline(jf, line)) {
            line = trim(line);
            int n = 0;
            if (!line.empty()) {
                n = splitLine(line, parts, 6);
            }
            if (n == 6 && isAllDigits(parts[0]) && isAllDigits(parts[1])
                       && isAllDigits(parts[2]) && isAllDigits(parts[3])) {
                int status = atoi(parts[2].c_str());
                int device = atoi(parts[3].c_str());
                if (status >= PENDING && status <= RELEASED && device >= 1 && device <= 4) {
                    jobId[jobCount]      = atoi(parts[0].c_str());
                    jobCustId[jobCount]  = atoi(parts[1].c_str());
                    jobStatus[jobCount]  = status;
                    jobDevice[jobCount]  = device;
                    jobText[jobCount][0] = parts[4];
                    jobText[jobCount][1] = parts[5];
                    if (jobId[jobCount] > lastJobId) {
                        lastJobId = jobId[jobCount];
                    }
                    jobCount++;
                }
            }
        }
        jf.close();
    }
    if (lastJobId < 5000) {
        lastJobId = 5000;
    }
}

// ====================================================================
//  HOME PAGE (shown once, before the main menu)
// ====================================================================
// Prints text in the middle of the page width
void printCentered(const string& text) {
    int pad = (PAGE_W - (int)text.length()) / 2;
    if (pad < 0) {
        pad = 0;
    }
    cout << string(pad, ' ') << text << endl;
}

void homePage() {
    clearScreen();
    cout << string(PAGE_W, '=') << endl;
    printCentered("R E P A I R A T O R Y");
    printCentered("Repair Shop Job Order Management System");
    cout << string(PAGE_W, '=') << endl << endl;

    cout << " WELCOME!" << endl;
    cout << " Repairatory is a digital logbook for a repair shop" << endl;
    cout << " that fixes phones, laptops and tablets. It replaces" << endl;
    cout << " paper job slips by keeping every customer and every" << endl;
    cout << " repair ticket in one place, so nothing gets lost." << endl << endl;

    cout << " WHAT YOU CAN DO" << endl;
    cout << "  - Manage customers (add, view, edit, delete)" << endl;
    cout << "  - Create job orders and follow each repair:" << endl;
    cout << "      Pending > In Progress > Completed > Released" << endl;
    cout << "  - Search a customer or a job order in seconds" << endl;
    cout << "  - Group job orders by repair status or device type" << endl;
    cout << "  - See a one-page summary report of the shop" << endl << endl;

    cout << " Your data is saved automatically after every change" << endl;
    cout << " and loaded again the next time the program opens." << endl << endl;

    cout << " Saved records -> Customers: " << custCount
         << "   Job orders: " << jobCount << endl;
    cout << string(PAGE_W, '=') << endl;

    string dummy;
    cout << endl << " Press Enter to open the main menu...";
    readLine(dummy);
}

// ====================================================================
//  MENUS
// ====================================================================
void customerMenu() {
    int choice;
    do {
        clearScreen();
        cout << "======== CUSTOMER MANAGEMENT ========" << endl;
        cout << " [1] New Customer" << endl;
        cout << " [2] View All Customers" << endl;
        cout << " [3] Edit Customer" << endl;
        cout << " [4] Delete Customer" << endl;
        cout << " [0] Back" << endl << endl;
        choice = getInt("Enter choice (0-4): ", 0, 4);

        switch (choice) {
            case 1: addCustomer();   break;
            case 2: viewCustomers(); break;
            case 3: editCustomer();  break;
            case 4: deleteCustomer(); break;
        }
    } while (choice != 0);
}

void jobMenu() {
    int choice;
    do {
        clearScreen();
        cout << "======= JOB ORDER MANAGEMENT =======" << endl;
        cout << " [1] New Job Order" << endl;
        cout << " [2] View All Job Orders" << endl;
        cout << " [3] Update Status" << endl;
        cout << " [4] Delete Job Order" << endl;
        cout << " [0] Back" << endl << endl;
        choice = getInt("Enter choice (0-4): ", 0, 4);

        switch (choice) {
            case 1: addJob();       break;
            case 2: viewJobs();     break;
            case 3: updateStatus(); break;
            case 4: deleteJob();    break;
        }
    } while (choice != 0);
}

void searchSortMenu() {
    int choice;
    do {
        clearScreen();
        cout << "========== SEARCH AND SORT ==========" << endl;
        cout << " [1] Search Customer (ID or name)" << endl;
        cout << " [2] Search Job Order (JO ID)" << endl;
        cout << " [3] Sort Job Orders by Repair Status" << endl;
        cout << " [4] Sort Job Orders by Device Type" << endl;
        cout << " [0] Back" << endl << endl;
        choice = getInt("Enter choice (0-4): ", 0, 4);

        switch (choice) {
            case 1: searchCustomer();            break;
            case 2: searchJob();                 break;
            case 3: showSorted(jobStatus, true); break;
            case 4: showSorted(jobDevice, false); break;
        }
    } while (choice != 0);
}

void mainMenu() {
    int choice;
    do {
        clearScreen();
        cout << "=====================================" << endl;
        cout << "       REPAIRATORY  -  MAIN MENU" << endl;
        cout << "=====================================" << endl;
        cout << " Customers: " << custCount << "    Job orders: " << jobCount << endl << endl;
        cout << " [1] Customer Management" << endl;
        cout << " [2] Job Order Management" << endl;
        cout << " [3] Search and Sort" << endl;
        cout << " [4] Summary Report" << endl;
        cout << " [5] Exit" << endl << endl;
        choice = getInt("Enter choice (1-5): ", 1, 5);

        switch (choice) {
            case 1: customerMenu();   break;
            case 2: jobMenu();        break;
            case 3: searchSortMenu(); break;
            case 4: summaryReport();  break;
            case 5:
                saveAll();
                cout << endl << "Data saved. Thank you for using Repairatory!" << endl;
                break;
        }
    } while (choice != 5);
}