#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <map>
#include <algorithm>
#include <limits>
#include <sstream>
#include <cctype>

using namespace std;

// Constants
const double SAVINGS_INTEREST_RATE = 0.04; // 4% annual
const double CURRENT_INTEREST_RATE = 0.01; // 1% annual
const int MAX_LOGIN_ATTEMPTS = 3;

// Account types
enum AccountType { SAVINGS, CURRENT };

// Transaction types
enum TransactionType { DEPOSIT, WITHDRAWAL, TRANSFER };

// Transaction record
struct Transaction {
    time_t timestamp;
    TransactionType type;
    double amount;
    string description;
    double balanceAfter;
};

// Bank Account
class BankAccount {
private:
    string accountNumber;
    string holderName;
    string pin;
    double balance;
    AccountType type;
    vector<Transaction> transactions;

public:
    BankAccount(string num, string name, string pin, AccountType type, double initial = 0.0)
        : accountNumber(num), holderName(name), pin(pin), type(type), balance(initial) {}

    string getAccountNumber() const { return accountNumber; }
    string getHolderName() const { return holderName; }
    double getBalance() const { return balance; }
    AccountType getAccountType() const { return type; }

    bool verifyPin(string inputPin) const {
        return pin == inputPin;
    }

    void changePin(string newPin) {
        pin = newPin;
        recordTransaction("PIN Changed", 0, balance);
    }

    void deposit(double amount, string description = "Deposit") {
        if (amount <= 0) {
            throw invalid_argument("Amount must be positive");
        }
        balance += amount;
        recordTransaction(description, amount, balance);
    }

    bool withdraw(double amount, string description = "Withdrawal") {
        if (amount <= 0) {
            throw invalid_argument("Amount must be positive");
        }
        if (balance >= amount) {
            balance -= amount;
            recordTransaction(description, -amount, balance);
            return true;
        }
        return false;
    }

    void addInterest() {
        double interest = 0;
        if (type == SAVINGS) {
            interest = balance * SAVINGS_INTEREST_RATE / 12; // Monthly interest
        } else {
            interest = balance * CURRENT_INTEREST_RATE / 12;
        }
        balance += interest;
        recordTransaction("Interest Credited", interest, balance);
    }

    void recordTransaction(string desc, double amount, double newBalance) {
        Transaction t;
        t.timestamp = time(nullptr);
        t.amount = amount;
        t.description = desc;
        t.balanceAfter = newBalance;
        
        if (amount > 0) {
            t.type = DEPOSIT;
        } else if (amount < 0) {
            t.type = WITHDRAWAL;
        } else {
            t.type = TRANSFER;
        }
        
        transactions.push_back(t);
    }

    void printStatement(int count = 5) const {
        cout << "\nAccount Statement for " << holderName << " (" << accountNumber << ")\n";
        cout << "Current Balance: $" << fixed << setprecision(2) << balance << "\n\n";
        cout << "Last " << count << " transactions:\n";
        cout << "--------------------------------------------------\n";
        cout << "Date/Time           | Type      | Amount   | Balance\n";
        cout << "--------------------------------------------------\n";

        int start = transactions.size() > count ? transactions.size() - count : 0;
        for (size_t i = start; i < transactions.size(); i++) {
            const Transaction& t = transactions[i];
            cout << put_time(localtime(&t.timestamp), "%Y-%m-%d %H:%M:%S") << " | ";
            
            switch (t.type) {
                case DEPOSIT: cout << "Deposit   "; break;
                case WITHDRAWAL: cout << "Withdrawal"; break;
                case TRANSFER: cout << "Transfer  "; break;
            }
            
            cout << " | $" << setw(8) << fixed << setprecision(2) << abs(t.amount) 
                 << " | $" << setw(8) << fixed << setprecision(2) << t.balanceAfter << endl;
        }
        cout << "--------------------------------------------------\n";
    }
};

// Bank Management System
class BankSystem {
private:
    map<string, BankAccount*> accounts;
    string adminPassword = "admin123";

    string generateAccountNumber() {
        static int lastNumber = 1000;
        return "ACCT" + to_string(++lastNumber);
    }

    bool isAdmin(string password) {
        return password == adminPassword;
    }

public:
    ~BankSystem() {
        for (auto& pair : accounts) {
            delete pair.second;
        }
    }

    BankAccount* createAccount(string name, string pin, AccountType type, double initialDeposit = 0.0) {
        string accNum = generateAccountNumber();
        BankAccount* account = new BankAccount(accNum, name, pin, type, initialDeposit);
        accounts[accNum] = account;
        return account;
    }

    BankAccount* login(string accountNumber, string pin, int& attemptsLeft) {
        auto it = accounts.find(accountNumber);
        if (it != accounts.end()) {
            if (it->second->verifyPin(pin)) {
                attemptsLeft = MAX_LOGIN_ATTEMPTS;
                return it->second;
            } else {
                attemptsLeft--;
                if (attemptsLeft <= 0) {
                    cout << "Too many failed attempts. Account temporarily locked.\n";
                }
                return nullptr;
            }
        }
        return nullptr;
    }

    bool transfer(BankAccount* from, string toAccountNumber, double amount) {
        auto it = accounts.find(toAccountNumber);
        if (it != accounts.end() && from->withdraw(amount, "Transfer to " + toAccountNumber)) {
            it->second->deposit(amount, "Transfer from " + from->getAccountNumber());
            return true;
        }
        return false;
    }

    void applyMonthlyInterest() {
        for (auto& pair : accounts) {
            pair.second->addInterest();
        }
        cout << "Monthly interest applied to all accounts.\n";
    }

    void printAllAccounts(string adminPassword) {
        if (!isAdmin(adminPassword)) {
            cout << "Unauthorized access!\n";
            return;
        }

        cout << "\nAll Accounts Summary\n";
        cout << "--------------------------------------------------\n";
        cout << "Account Number | Holder Name       | Type     | Balance\n";
        cout << "--------------------------------------------------\n";

        for (const auto& pair : accounts) {
            cout << pair.first << " | " << setw(17) << left << pair.second->getHolderName() << " | ";
            cout << (pair.second->getAccountType() == SAVINGS ? "Savings " : "Current ") << " | $";
            cout << fixed << setprecision(2) << pair.second->getBalance() << endl;
        }
        cout << "--------------------------------------------------\n";
    }

    void saveToFile(string filename) {
        ofstream file(filename);
        if (!file.is_open()) {
            cerr << "Error saving data to file.\n";
            return;
        }

        for (const auto& pair : accounts) {
            file << pair.first << "," 
                 << pair.second->getHolderName() << ","
                 << pair.second->getAccountType() << ","
                 << pair.second->getBalance() << "\n";
        }
        file.close();
    }

    void loadFromFile(string filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "No existing data file found. Starting fresh.\n";
            return;
        }

        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string accNum, name, pin, typeStr, balanceStr;
            
            getline(ss, accNum, ',');
            getline(ss, name, ',');
            getline(ss, typeStr, ',');
            getline(ss, balanceStr);

            AccountType type = static_cast<AccountType>(stoi(typeStr));
            double balance = stod(balanceStr);
            
            // For simplicity, we're not loading PINs and transactions from file
            accounts[accNum] = new BankAccount(accNum, name, "0000", type, balance);
        }
        file.close();
    }
};

// Helper functions
void displayMainMenu() {
    cout << "\nBanking Management System\n";
    cout << "1. Create Account\n";
    cout << "2. Login\n";
    cout << "3. Admin Functions\n";
    cout << "4. Exit\n";
    cout << "Enter choice: ";
}

void displayCustomerMenu() {
    cout << "\nCustomer Menu\n";
    cout << "1. Deposit\n";
    cout << "2. Withdraw\n";
    cout << "3. Transfer\n";
    cout << "4. View Statement\n";
    cout << "5. Change PIN\n";
    cout << "6. Logout\n";
    cout << "Enter choice: ";
}

void displayAdminMenu() {
    cout << "\nAdmin Menu\n";
    cout << "1. Apply Monthly Interest\n";
    cout << "2. View All Accounts\n";
    cout << "3. Back to Main Menu\n";
    cout << "Enter choice: ";
}

AccountType getAccountType() {
    int type;
    while (true) {
        cout << "Account Type:\n";
        cout << "1. Savings Account (4% annual interest)\n";
        cout << "2. Current Account (1% annual interest)\n";
        cout << "Enter choice: ";
        cin >> type;
        
        if (type == 1 || type == 2) {
            return type == 1 ? SAVINGS : CURRENT;
        }
        cout << "Invalid choice. Please try again.\n";
    }
}

string getPin() {
    string pin;
    while (true) {
        cout << "Enter 4-digit PIN: ";
        cin >> pin;
        
        if (pin.length() == 4 && all_of(pin.begin(), pin.end(), ::isdigit)) {
            return pin;
        }
        cout << "PIN must be 4 digits. Try again.\n";
    }
}

int main() {
    BankSystem bank;
    bank.loadFromFile("bank_data.txt");

    while (true) {
        displayMainMenu();
        int choice;
        cin >> choice;

        if (choice == 1) {
            // Create Account
            cin.ignore();
            string name;
            cout << "Enter account holder name: ";
            getline(cin, name);
            
            string pin = getPin();
            AccountType type = getAccountType();
            
            double initialDeposit = 0;
            cout << "Enter initial deposit amount: $";
            cin >> initialDeposit;
            
            BankAccount* account = bank.createAccount(name, pin, type, initialDeposit);
            cout << "\nAccount created successfully!\n";
            cout << "Your account number is: " << account->getAccountNumber() << "\n";
            
        } else if (choice == 2) {
            // Login
            string accNum, pin;
            cout << "Enter account number: ";
            cin >> accNum;
            cout << "Enter PIN: ";
            cin >> pin;
            
            int attemptsLeft = MAX_LOGIN_ATTEMPTS;
            BankAccount* account = bank.login(accNum, pin, attemptsLeft);
            
            if (account) {
                cout << "\nLogin successful! Welcome, " << account->getHolderName() << "!\n";
                
                while (true) {
                    displayCustomerMenu();
                    int customerChoice;
                    cin >> customerChoice;
                    
                    if (customerChoice == 1) {
                        // Deposit
                        double amount;
                        cout << "Enter deposit amount: $";
                        cin >> amount;
                        account->deposit(amount);
                        cout << "Deposit successful. New balance: $" 
                             << fixed << setprecision(2) << account->getBalance() << "\n";
                            
                    } else if (customerChoice == 2) {
                        // Withdraw
                        double amount;
                        cout << "Enter withdrawal amount: $";
                        cin >> amount;
                        
                        if (account->withdraw(amount)) {
                            cout << "Withdrawal successful. New balance: $" 
                                 << fixed << setprecision(2) << account->getBalance() << "\n";
                        } else {
                            cout << "Insufficient funds!\n";
                        }
                        
                    } else if (customerChoice == 3) {
                        // Transfer
                        string toAccount;
                        double amount;
                        cout << "Enter recipient account number: ";
                        cin >> toAccount;
                        cout << "Enter transfer amount: $";
                        cin >> amount;
                        
                        if (bank.transfer(account, toAccount, amount)) {
                            cout << "Transfer successful. New balance: $" 
                                 << fixed << setprecision(2) << account->getBalance() << "\n";
                        } else {
                            cout << "Transfer failed. Check recipient account or balance.\n";
                        }
                        
                    } else if (customerChoice == 4) {
                        // View Statement
                        account->printStatement();
                        
                    } else if (customerChoice == 5) {
                        // Change PIN
                        string newPin = getPin();
                        account->changePin(newPin);
                        cout << "PIN changed successfully.\n";
                        
                    } else if (customerChoice == 6) {
                        // Logout
                        break;
                    } else {
                        cout << "Invalid choice. Try again.\n";
                    }
                }
            } else {
                cout << "Login failed. " << attemptsLeft << " attempts remaining.\n";
            }
            
        } else if (choice == 3) {
            // Admin Functions
            string password;
            cout << "Enter admin password: ";
            cin >> password;
            
            if (password == "admin123") {
                while (true) {
                    displayAdminMenu();
                    int adminChoice;
                    cin >> adminChoice;
                    
                    if (adminChoice == 1) {
                        // Apply Interest
                        bank.applyMonthlyInterest();
                        
                    } else if (adminChoice == 2) {
                        // View All Accounts
                        bank.printAllAccounts(password);
                        
                    } else if (adminChoice == 3) {
                        // Back
                        break;
                    } else {
                        cout << "Invalid choice. Try again.\n";
                    }
                }
            } else {
                cout << "Invalid admin password!\n";
            }
            
        } else if (choice == 4) {
            // Exit
            bank.saveToFile("bank_data.txt");
            cout << "Thank you for using our banking system!\n";
            break;
            
        } else {
            cout << "Invalid choice. Please try again.\n";
        }
    }

    return 0;
}