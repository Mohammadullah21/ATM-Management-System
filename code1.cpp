#include <iostream>

#include <vector>

#include <string>

#include <fstream>

#include <sstream>

#include <iomanip>

#include <limits>

#include <conio.h>

#include <tuple>



using namespace std;



// ================================================================

//  Person  —  base class for any individual in the system

// ================================================================

class Person {

protected:

    string name;

public:

    Person(string n = "") : name(n) {}

    virtual ~Person() {}

    string getName() const { return name; }

};



// ================================================================

//  Transaction  —  single record of one banking operation

// ================================================================

class Transaction {

private:

    string type;

    double amount;



public:

    Transaction(string t = "", double a = 0.0) : type(t), amount(a) {}



    string getType()   const { return type; }

    double getAmount() const { return amount; }



    void display(int index) const {

        cout << left << setw(5) << index

            << setw(30) << type

            << "Rs." << amount << "\n";

    }



    string toCSV() const {

        return type + "," + to_string(amount);

    }

};



// ================================================================

//  Account  —  abstract base class for all account types

// ================================================================

class Account {

protected:

    int    accountNumber;

    int    pin;

    double balance;

    string iban;



public:

    Account(int acc = 0, int p = 0, double b = 0.0, string ib = "")

        : accountNumber(acc), pin(p), balance(b), iban(ib) {}



    virtual ~Account() {}



    virtual string getType() const = 0;



    virtual void deposit(double amount) {

        if (amount <= 0) return;

        balance += amount;

    }



    virtual bool withdraw(double amount) {

        if (amount <= 0)       return false;

        if (amount > balance)  return false;

        balance -= amount;

        return true;

    }



    double getBalance()       const { return balance; }

    int    getAccountNumber() const { return accountNumber; }

    string getIBAN()          const { return iban; }



    // XOR encryption — lossless, no rounding bugs

    int  encryptPin()              const { return pin ^ 0xABCD; }

    bool verifyPin(int entered)    const { return pin == entered; }

    void changePin(int newPin) { pin = newPin; }

};



// ================================================================

//  Concrete account types

// ================================================================

class SavingsAccount : public Account {

public:

    SavingsAccount(int acc, int p, double b, string ib)

        : Account(acc, p, b, ib) {}

    string getType() const override { return "Savings"; }

};



class CurrentAccount : public Account {

public:

    CurrentAccount(int acc, int p, double b, string ib)

        : Account(acc, p, b, ib) {}

    string getType() const override { return "Current"; }

};



// ================================================================

//  FileManager  —  handles all file I/O (Single Responsibility)

// ================================================================

class FileManager {

public:

    // Append one transaction line to transactions.csv

    static void logTransaction(const string& customerName,

        const string& type,

        double        amount) {

        ofstream file("transactions.csv", ios::app);

        if (!file) return;

        file << customerName << "," << type << "," << amount << "\n";

        file.close();

    }



    // Overwrite bank.csv with current state of all accounts

    // Takes raw data so it stays decoupled from Customer/Bank internals

    static void saveBank(const vector<tuple<string, int, int, double, string, string>>& records) {

        ofstream file("bank.csv");

        if (!file) return;

        file << "Name,AccountNumber,EncryptedPIN,Balance,IBAN,AccountType\n";

        for (const auto& r : records) {

            file << get<0>(r) << ","   // Name

                << get<1>(r) << ","   // AccountNumber

                << get<2>(r) << ","   // EncryptedPIN

                << get<3>(r) << ","   // Balance

                << get<4>(r) << ","   // IBAN

                << get<5>(r) << "\n"; // AccountType

        }

        file.close();

    }



    // Read bank.csv and return raw records

    static vector<tuple<string, int, int, double, string, string>> loadBank() {

        vector<tuple<string, int, int, double, string, string>> records;

        ifstream file("bank.csv");

        if (!file) return records;



        string line;

        getline(file, line); // skip header



        while (getline(file, line)) {

            if (line.empty()) continue;

            stringstream ss(line);

            string name, accStr, pinStr, balStr, iban, type;



            getline(ss, name, ',');

            getline(ss, accStr, ',');

            getline(ss, pinStr, ',');

            getline(ss, balStr, ',');

            getline(ss, iban, ',');

            getline(ss, type, ',');



            try {

                records.emplace_back(

                    name,

                    stoi(accStr),

                    stoi(pinStr),

                    stod(balStr),

                    iban,

                    type

                );

            }

            catch (...) {

                continue; // skip corrupted lines

            }

        }

        file.close();

        return records;

    }

};



// ================================================================

//  Customer  —  a Person who owns an Account and has history

// ================================================================

class Customer : public Person {

private:

    Account* account;

    vector<Transaction> history;



public:

    Customer(string n = "", Account* acc = nullptr)

        : Person(n), account(acc) {}



    Customer(const Customer& other)

        : Person(other.name), account(other.account), history(other.history) {}



    Account* getAccount() const { return account; }



    void recordTransaction(const string& type, double amount) {

        history.push_back(Transaction(type, amount));

        FileManager::logTransaction(getName(), type, amount);

    }



    void showHistory() const {

        cout << "\n===== Transaction History: " << getName() << " =====\n";

        if (history.empty()) { cout << "No transactions yet.\n"; return; }



        cout << left << setw(5) << "No." << setw(30) << "Type" << "Amount\n";

        cout << string(52, '-') << "\n";

        for (int i = 0; i < (int)history.size(); i++)

            history[i].display(i + 1);

    }



    void showInfo() const {

        cout << "\n===== USER INFORMATION =====\n";

        cout << "Name           : " << getName() << "\n";

        cout << "Account Type   : " << account->getType() << "\n";

        cout << "Account Number : " << account->getAccountNumber() << "\n";

        cout << "IBAN           : " << account->getIBAN() << "\n";

        cout << "Balance        : Rs." << account->getBalance() << "\n";

    }

};



// ================================================================

//  Bank  —  manages all customers, handles login and search

// ================================================================

class Bank {

private:

    vector<Customer*> customers;



    Account* makeAccount(const string& type, int acc, int pin, double bal, const string& iban) {

        if (type == "Current")

            return new CurrentAccount(acc, pin, bal, iban);

        return new SavingsAccount(acc, pin, bal, iban);

    }



public:

    ~Bank() {

        for (auto* c : customers) {

            delete c->getAccount();

            delete c;

        }

    }



    void addCustomer(Customer* c) { customers.push_back(c); }



    vector<Customer*>& getCustomers() { return customers; }



    // Seed default data when no bank.csv exists

    void seedDefaults() {

        addCustomer(new Customer("Mohammad", new SavingsAccount(1001, 1234, 15000, "PK12HABB00012345678901")));

        addCustomer(new Customer("Shayan", new CurrentAccount(1002, 5678, 25000, "PK45UBL00098765432100")));

        addCustomer(new Customer("Saqib", new SavingsAccount(1003, 4321, 35000, "PK78MCB00045678912345")));

    }



    // Load customers from saved CSV records

    void loadFromRecords(const vector<tuple<string, int, int, double, string, string>>& records) {

        for (const auto& r : records) {

            int originalPin = get<2>(r) ^ 0xABCD; // XOR decrypt

            Account* acc = makeAccount(get<5>(r), get<1>(r), originalPin, get<3>(r), get<4>(r));

            addCustomer(new Customer(get<0>(r), acc));

        }

    }



    // Collect current state for FileManager to save

    void save() {

        vector<tuple<string, int, int, double, string, string>> records;

        for (auto* c : customers) {

            records.emplace_back(

                c->getName(),

                c->getAccount()->getAccountNumber(),

                c->getAccount()->encryptPin(),

                c->getAccount()->getBalance(),

                c->getAccount()->getIBAN(),

                c->getAccount()->getType()

            );

        }

        FileManager::saveBank(records);

    }



    Customer* login(int accNo, int pin) {

        for (auto* c : customers)

            if (c->getAccount()->getAccountNumber() == accNo &&

                c->getAccount()->verifyPin(pin))

                return c;

        return nullptr;

    }



    Customer* findByIBAN(const string& iban) {

        for (auto* c : customers)

            if (c->getAccount()->getIBAN() == iban)

                return c;

        return nullptr;

    }

};



// ================================================================

//  InputHandler  —  all user input validation in one place

// ================================================================

class InputHandler {

public:

    static int getHiddenPin() {

        string entered;

        char ch;

        while (true) {

            ch = _getch();

            if (ch == 13) {

                if (entered.empty()) { cout << "\nPIN Cannot Be Empty\n"; return -1; }

                break;

            }

            if (ch >= '0' && ch <= '9') { entered += ch; cout << "*"; }

            else if (ch == 8 && !entered.empty()) { entered.pop_back(); cout << "\b \b"; }

        }

        cout << "\n";

        if (entered.size() > 10) { cout << "PIN too long\n"; return -1; }

        return stoi(entered);

    }



    static double getPositiveAmount(const string& prompt) {

        double amount;

        cout << prompt;

        if (!(cin >> amount) || amount <= 0) {

            cin.clear();

            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            return -1;

        }

        return amount;

    }



    static int getMenuChoice() {

        int choice;

        if (!(cin >> choice)) {

            cin.clear();

            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            return -1;

        }

        return choice;

    }



    static string getAccountNumber() {

        string input;

        cin >> input;

        if (input.empty()) return "";

        for (char c : input)

            if (!isdigit(c)) return "";

        return input;

    }

};



// ================================================================

//  ATM  —  orchestrates sessions, uses Bank and InputHandler

// ================================================================

class ATM {

private:

    Bank bank;



    void initBank() {

        auto records = FileManager::loadBank();

        if (records.empty()) {

            bank.seedDefaults();

            bank.save();

        }

        else {

            bank.loadFromRecords(records);

        }

    }



    void handleDeposit(Customer* user) {

        double amount = InputHandler::getPositiveAmount("Enter Deposit Amount: ");

        if (amount < 0) { cout << "Invalid Amount\n"; return; }

        user->getAccount()->deposit(amount);

        user->recordTransaction("Deposit", amount);

        cout << "Deposit Successful. New Balance: Rs." << user->getAccount()->getBalance() << "\n";

        bank.save();

    }



    void handleWithdraw(Customer* user) {

        double amount = InputHandler::getPositiveAmount("Enter Withdraw Amount: ");

        if (amount < 0) { cout << "Invalid Amount\n"; return; }

        if (user->getAccount()->withdraw(amount)) {

            user->recordTransaction("Withdrawal", amount);

            cout << "Withdrawal Successful. New Balance: Rs." << user->getAccount()->getBalance() << "\n";

            bank.save();

        }

        else {

            cout << "Insufficient Balance\n";

        }

    }



    void handleChangePin(Customer* user) {

        cout << "Enter New PIN: ";

        int newPin = InputHandler::getHiddenPin();

        if (newPin == -1) return;

        user->getAccount()->changePin(newPin);

        cout << "PIN Changed Successfully\n";

        bank.save();

    }



    void handleFastCash(Customer* user) {

        cout << "\n===== FAST CASH =====\n";

        cout << "1. Rs.500\n2. Rs.1000\n3. Rs.5000\n";

        cout << "Select Option: ";

        int option = InputHandler::getMenuChoice();

        double amounts[] = { 0, 500, 1000, 5000 };

        if (option < 1 || option > 3) { cout << "Invalid Option\n"; return; }

        double amount = amounts[option];

        if (user->getAccount()->withdraw(amount)) {

            user->recordTransaction("Fast Cash", amount);

            cout << "Please Collect Cash. Rs." << amount << " Withdrawn.\n";

            bank.save();

        }

        else {

            cout << "Insufficient Balance\n";

        }

    }



    void handleTransfer(Customer* user) {

        cout << "\n===== MONEY TRANSFER =====\n";

        cout << "Enter Receiver IBAN: ";

        string iban; cin >> iban;



        if (iban == user->getAccount()->getIBAN()) {

            cout << "Cannot Transfer To Same Account\n";

            return;

        }

        Customer* receiver = bank.findByIBAN(iban);

        if (!receiver) { cout << "IBAN Not Found\n"; return; }



        cout << "Receiver: " << receiver->getName() << "\n";

        double amount = InputHandler::getPositiveAmount("Enter Transfer Amount: ");

        if (amount < 0) { cout << "Invalid Amount\n"; return; }



        if (user->getAccount()->withdraw(amount)) {

            receiver->getAccount()->deposit(amount);

            user->recordTransaction("Transfer Sent to " + receiver->getName(), amount);

            receiver->recordTransaction("Transfer Received from " + user->getName(), amount);

            cout << "Transfer Successful\n";

            bank.save();

        }

        else {

            cout << "Insufficient Balance\n";

        }

    }



    Customer* doLogin() {

        cout << "\n===== ATM MANAGEMENT SYSTEM =====\n";

        cout << "Enter Account Number: ";

        string input = InputHandler::getAccountNumber();

        if (input.empty()) { cout << "Invalid Account Number\n"; return nullptr; }



        int acc = stoi(input);

        int attempts = 0;

        Customer* user = nullptr;



        while (attempts < 3) {

            cout << "Enter PIN: ";

            int pin = InputHandler::getHiddenPin();

            if (pin == -1) continue;

            user = bank.login(acc, pin);

            if (user) { cout << "Login Successful\n"; return user; }

            cout << "Wrong PIN. Attempt " << ++attempts << "/3\n";

        }



        cout << "\nCard Captured! Contact your bank.\n";

        return nullptr;

    }



public:

    ATM() { initBank(); }



    void start() {

        Customer* user = doLogin();

        if (!user) return;



        int choice;

        do {

            cout << "\n===== ATM MENU =====\n";

            cout << "1. Check Balance\n"

                << "2. Deposit Money\n"

                << "3. Withdraw Money\n"

                << "4. Change PIN\n"

                << "5. Transaction History\n"

                << "6. Fast Cash\n"

                << "7. Money Transfer\n"

                << "8. User Information\n"

                << "9. Logout\n"

                << "Enter Choice: ";



            choice = InputHandler::getMenuChoice();



            switch (choice) {

            case 1: cout << "Current Balance: Rs." << user->getAccount()->getBalance() << "\n"; break;

            case 2: handleDeposit(user);     break;

            case 3: handleWithdraw(user);    break;

            case 4: handleChangePin(user);   break;

            case 5: user->showHistory();     break;

            case 6: handleFastCash(user);    break;

            case 7: handleTransfer(user);    break;

            case 8: user->showInfo();        break;

            case 9: bank.save(); cout << "Logging Out...\n"; break;

            default: cout << "Invalid Choice\n";

            }

        } while (choice != 9);

    }

};



// ================================================================

//  main

// ================================================================

int main() {

    ATM atm;

    while (true) {

        atm.start();

        cout << "\nReturning To Login Screen...\n\n";

    }

    return 0;

}