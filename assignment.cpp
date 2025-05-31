/**********|**********|**********|
Program: RobotWarSimulator.cpp
Course: OOPDS
Trimester: 2510
Name: Anisah
ID: 242UC244NY
Lecture Section: TC2L
Tutorial Section: TT5L
Email: nurul.anisah.hasan@student.mmu.edu.my
Phone: 018-9107032
**********|**********|**********/

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <memory>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <queue>
#include <map>
#include <set>
#include <climits>
#include <chrono>
#include <thread>

using namespace std;

int ROWS = 20;
int COLS = 30;
const int DEFAULT_STEPS = 10;  // Default number of steps
int STEPS = DEFAULT_STEPS;     // Can be overridden by input file
int SEED = 12345;  // Fixed seed for deterministic random number generation

ofstream logFile;  // Changed to not initialize here

void log(const string& msg) {
    cout << msg << endl;
    logFile << msg << endl;
}

// Initialize random seed at program start
void initializeRandom() {
    srand(SEED);
}

class Battlefield;

class MovingRobot { public: virtual void move(Battlefield&) = 0; };
class ShootingRobot { public: virtual void fire(Battlefield&) = 0; };
class SeeingRobot { public: virtual void look(Battlefield&) = 0; };
class ThinkingRobot { public: virtual void think(Battlefield&) = 0; };

class Robot : public MovingRobot, public ShootingRobot, public SeeingRobot, public ThinkingRobot {
protected:
    string name, type;
    int x, y;
    int lives = 3, shells = 10;
    bool alive = true;
    int jumpUses = 0, scoutUses = 0, hideUses = 0, trackersLeft = 0;
    bool hasScout = false, isSemiAuto = false, isHiding = false;
    int longShotRange = 1;
    int upgrades = 0, reentries = 0;
    set<string> upgradeCategories;

public:
    Robot(string t, string n, int px, int py) : type(t), name(n), x(px), y(py) {}

    string getName() const { return name; }
    int getX() const { return x; }
    int getY() const { return y; }
    bool isAlive() const { return alive; }
    bool canReenter() const { return reentries < 3; }

    void reviveAt(int newX, int newY) {
        if (reentries >= 3) return;
        x = newX; y = newY; alive = true; shells = 10; reentries++;
    }

    void damage() {
        if (!isHiding) {
            if (--lives <= 0) alive = false;
        }
    }

    void upgrade(const string& ability) {
        if (upgrades >= 3) return;
        string category;
        if (ability == "Jump" || ability == "Hide") category = "Moving";
        else if (ability == "ThirtyShot" || ability == "LongShot" || ability == "SemiAuto") category = "Shooting";
        else if (ability == "Scout" || ability == "Track") category = "Seeing";
        else return;

        if (upgradeCategories.count(category)) {
            log(name + " already has a " + category + " upgrade.");
            return;
        }

        upgradeCategories.insert(category);
        upgrades++;

        if (ability == "Jump") jumpUses = 3;
        else if (ability == "Hide") hideUses = 3;
        else if (ability == "Scout") { hasScout = true; scoutUses = 3; }
        else if (ability == "Track") trackersLeft = 3;
        else if (ability == "ThirtyShot") shells = 30;
        else if (ability == "LongShot") longShotRange = 3;
        else if (ability == "SemiAuto") isSemiAuto = true;
    }

    virtual void move(Battlefield&) = 0;
    virtual void fire(Battlefield&) = 0;
    virtual void look(Battlefield&) = 0;
    virtual void think(Battlefield&) = 0;
};

class Battlefield {
private:
    vector<shared_ptr<Robot>> robots;
    queue<shared_ptr<Robot>> reentry;
    vector<vector<char>> grid;
    bool allowReentry;  // New flag to control re-entry

public:
    Battlefield(bool allowReentry = true) : allowReentry(allowReentry) {
        grid = vector<vector<char>>(ROWS, vector<char>(COLS, '.'));
    }

    void addRobot(shared_ptr<Robot> r) { robots.push_back(r); }

    void update() {
        for (auto& row : grid) fill(row.begin(), row.end(), '.');
        for (auto& r : robots)
            if (r->isAlive())
                grid[r->getX()][r->getY()] = r->getName()[0];
    }

    void display(int turn) {
        log("=== Turn " + to_string(turn) + " ===");
        for (int i = 0; i < ROWS; i++) {
            string row;
            for (int j = 0; j < COLS; j++)
                row += grid[i][j], row += ' ';
            log(row);
        }
    }

    bool isInside(int x, int y) {
        return x >= 0 && x < ROWS && y >= 0 && y < COLS;
    }

    shared_ptr<Robot> getRobotAt(int x, int y) {
        for (auto& r : robots)
            if (r->isAlive() && r->getX() == x && r->getY() == y)
                return r;
        return nullptr;
    }

    void simulateTurn() {
        for (auto& r : robots) {
            if (r->isAlive()) {
                r->think(*this);
                r->look(*this);
                r->fire(*this);
                r->move(*this);
            } else if (allowReentry) {  // Only add to reentry queue if reentry is allowed
                reentry.push(r);
            }
        }

        while (!reentry.empty() && allowReentry) {  // Only process reentry if allowed
            auto r = reentry.front(); reentry.pop();
            if (r->canReenter()) {
                int x = rand() % ROWS, y = rand() % COLS;
                r->reviveAt(x, y);
                log(r->getName() + " has re-entered!");
                break;
            }
        }

        update();
    }

    vector<shared_ptr<Robot>>& getRobots() { return robots; }
};

// ========================== GENERIC ROBOT =============================
class GenericRobot : public Robot {
public:
    GenericRobot(string n, int x, int y) : Robot("Generic", n, x, y) {}

    void think(Battlefield&) override {
        if (hideUses > 0 && rand() % 10 < 3) {
            isHiding = true;
            hideUses--;
            log(name + " is hiding this turn!");
        } else {
            isHiding = false;
        }
    }

    void look(Battlefield& field) override {
        if (hasScout && scoutUses > 0 && rand() % 10 < 3) {
            log(name + " uses Scout vision!");
            scoutUses--;
            for (auto& r : field.getRobots())
                if (r->isAlive())
                    log("- Sees " + r->getName() + " at (" + to_string(r->getX()) + "," + to_string(r->getY()) + ")");
            return;
        }

        vector<pair<int, int>> dirs = {
            {-1,-1},{-1,0},{-1,1},{0,-1},{0,0},{0,1},{1,-1},{1,0},{1,1}
        };
        auto [dx, dy] = dirs[rand() % dirs.size()];
        int nx = x + dx, ny = y + dy;
        if (field.isInside(nx, ny)) {
            auto target = field.getRobotAt(nx, ny);
            log(name + " sees " + (target && target->isAlive() ? target->getName() : "empty") + " at (" + to_string(nx) + "," + to_string(ny) + ")");
        }
    }

    void fire(Battlefield& field) override {
        if (shells <= 0) return;
        for (int dx = -longShotRange; dx <= longShotRange; dx++) {
            for (int dy = -longShotRange; dy <= longShotRange; dy++) {
                if (dx == 0 && dy == 0 || abs(dx) + abs(dy) > longShotRange) continue;
                int tx = x + dx, ty = y + dy;
                if (!field.isInside(tx, ty)) continue;
                auto target = field.getRobotAt(tx, ty);
                if (target && target->getName() != name) {
                    log(name + " fires at (" + to_string(tx) + "," + to_string(ty) + ")");
                    bool wasAlive = target->isAlive();
                    bool hit = false;
                    int shots = isSemiAuto ? 3 : 1;
                    for (int i = 0; i < shots; ++i)
                        if (rand() % 100 < 70) { hit = true; target->damage(); }

                    log(hit ? "Hit!" : "Missed.");
                    if (wasAlive && !target->isAlive()) {
                        log(name + " destroyed " + target->getName() + ", gaining an upgrade.");
                        performUpgrade();
                    }

                    if (--shells == 0) {
                        alive = false;
                        log(name + " self-destructed (out of shells).");
                    }
                    return;
                }
            }
        }
    }

    void move(Battlefield& field) override {
        if (jumpUses > 0 && rand() % 10 < 2) {
            int newX = rand() % ROWS, newY = rand() % COLS;
            if (!field.getRobotAt(newX, newY)) {
                x = newX; y = newY;
                jumpUses--;
                log(name + " jumps to (" + to_string(x) + "," + to_string(y) + ")");
                return;
            }
        }
        int dirs[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
        int d = rand() % 4;
        int nx = x + dirs[d][0], ny = y + dirs[d][1];
        if (field.isInside(nx, ny) && !field.getRobotAt(nx, ny)) {
            x = nx; y = ny;
            log(name + " moves to (" + to_string(x) + "," + to_string(y) + ")");
        }
    }

    void performUpgrade() {
        if (upgrades >= 3) {
            log(name + " has used all upgrades.");
            return;
        }
        log("Choose upgrade for " + name + ":");
        if (!upgradeCategories.count("Moving")) log("1. JumpBot (J), 2. HideBot (H)");
        if (!upgradeCategories.count("Seeing")) log("3. ScoutBot (S), 4. TrackBot (K)");
        if (!upgradeCategories.count("Shooting")) log("5. ThirtyShotBot (T), 6. LongShotBot (L), 7. SemiAutoBot (A)");
        log("Enter choice (J/H/S/K/T/L/A): ");
        char choice; cin >> choice; choice = toupper(choice);
        switch (choice) {
            case 'J': upgrade("Jump"); log(name + " upgraded to JumpBot."); break;
            case 'H': upgrade("Hide"); log(name + " upgraded to HideBot."); break;
            case 'S': upgrade("Scout"); log(name + " upgraded to ScoutBot."); break;
            case 'K': upgrade("Track"); log(name + " upgraded to TrackBot."); break;
            case 'T': upgrade("ThirtyShot"); log(name + " upgraded to ThirtyShotBot."); break;
            case 'L': upgrade("LongShot"); log(name + " upgraded to LongShotBot."); break;
            case 'A': upgrade("SemiAuto"); log(name + " upgraded to SemiAutoBot."); break;
            default: log("Invalid input. No upgrade applied."); break;
        }
    }
};

// ======================= AGGRESSIVE ROBOT ============================
class AggressiveRobot : public GenericRobot {
public:
    AggressiveRobot(string n, int x, int y) : GenericRobot(n, x, y) { type = "Aggressive"; }

    void think(Battlefield& field) override {
        int minDist = INT_MAX, tx = -1, ty = -1;
        for (auto& r : field.getRobots()) {
            if (r->isAlive() && r->getName() != name) {
                int dist = abs(r->getX() - x) + abs(r->getY() - y);
                if (dist < minDist) {
                    minDist = dist;
                    tx = r->getX(); ty = r->getY();
                }
            }
        }
        if (tx != -1 && ty != -1) {
            if (tx > x) x++; else if (tx < x) x--;
            if (ty > y) y++; else if (ty < y) y--;
            log(name + " (Aggressive) charges toward enemy!");
        }
    }
};

// ======================= SNIPER ROBOT ============================
class SniperRobot : public GenericRobot {
public:
    SniperRobot(string n, int x, int y) : GenericRobot(n, x, y) {
        type = "Sniper";
        longShotRange = 3;
    }
};

// ======================= DEFENDER ROBOT ============================
class DefenderRobot : public GenericRobot {
public:
    DefenderRobot(string n, int x, int y) : GenericRobot(n, x, y) {
        type = "Defender";
        hideUses = 3;
    }

    void think(Battlefield&) override {
        if (hideUses > 0 && rand() % 5 == 0) {
            isHiding = true;
            hideUses--;
            log(name + " (Defender) is hiding this turn.");
        } else {
            isHiding = false;
        }
    }
};

// ======================= CONFIG LOADER ============================
void loadConfig(const string& filename, Battlefield& field) {
    ifstream in(filename);
    if (!in) {
        cerr << "Error: Could not open " << filename << endl;
        exit(1);
    }

    string line;
    while (getline(in, line)) {
        if (line.empty()) continue;
        if (line.find("M by N") != string::npos) {
            sscanf(line.c_str(), "M by N : %d %d", &ROWS, &COLS);
        } else if (line.find("steps") != string::npos) {
            int fileSteps;
            sscanf(line.c_str(), "steps: %d", &fileSteps);
            if (fileSteps > 0) {  // Only override if the file specifies a valid number
                STEPS = fileSteps;
            }
            log("Simulation will run for " + to_string(STEPS) + " steps");
        } else if (isalpha(line[0])) {
            string type, name, sx, sy;
            istringstream ss(line);
            ss >> type >> name >> sx >> sy;
            if (type.empty() || name.empty() || sx.empty() || sy.empty()) {
                log("⚠️  Skipping malformed line: " + line);
                continue;
            }

            int x, y;
            try {
                x = (sx == "random") ? rand() % ROWS : stoi(sx);
                y = (sy == "random") ? rand() % COLS : stoi(sy);
            } catch (...) {
                log("⚠️  Invalid position values for robot: " + name);
                continue;
            }

            shared_ptr<Robot> bot;
            if (type == "GenericRobot")      bot = make_shared<GenericRobot>(name, x, y);
            else if (type == "AggressiveRobot") bot = make_shared<AggressiveRobot>(name, x, y);
            else if (type == "SniperRobot")     bot = make_shared<SniperRobot>(name, x, y);
            else if (type == "DefenderRobot")   bot = make_shared<DefenderRobot>(name, x, y);

            if (bot) field.addRobot(bot);
        }
    }
}

// Add these functions at the top level, before main()
void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void waitForInput() {
    cout << "\nPress Enter to continue to next turn...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
}

// ======================= MAIN ============================
int main() {
    initializeRandom();  // Set the fixed seed at program start, before ANY random number generation
    
    string inputFile;
    string logFileName;
    string endgameLogName;
    bool endOnOneRobot = false;  // Flag to determine end game condition
    
    cout << "\n🤖 Welcome to the Robot War Simulator! 🤖\n" << endl;
    cout << "Choose your battle scenario:\n" << endl;
    cout << "1️⃣  Endurance Challenge" << endl;
    cout << "    - Robots battle for a fixed number of rounds" << endl;
    cout << "    - Multiple robots can survive until the end" << endl;
    cout << "    - Perfect for testing robot durability!\n" << endl;
    
    cout << "2️⃣  Last Bot Standing" << endl;
    cout << "    - Battle continues until only one robot remains" << endl;
    cout << "    - No robot re-entry allowed - once you're out, you're out!" << endl;
    cout << "    - The ultimate test of combat supremacy!\n" << endl;
    
    cout << "Enter your choice (1 or 2): ";
    int choice;
    cin >> choice;
    
    if (choice == 1) {
        inputFile = "simulation1.txt";
        logFileName = "log1.txt";
        endgameLogName = "endgame1.txt";
        endOnOneRobot = false;  // End on max turns
        cout << "\n⚔️  Starting Endurance Challenge! ⚔️" << endl;
    } else if (choice == 2) {
        inputFile = "simulation2.txt";
        logFileName = "log2.txt";
        endgameLogName = "endgame2.txt";
        endOnOneRobot = true;   // End when one robot remains
        cout << "\n🏆 Starting Last Bot Standing! 🏆" << endl;
    } else {
        cout << "\n⚠️  Invalid choice. Defaulting to Endurance Challenge." << endl;
        inputFile = "simulation1.txt";
        logFileName = "log1.txt";
        endgameLogName = "endgame1.txt";
        endOnOneRobot = false;
    }
    
    // Open the main log file
    logFile.open(logFileName);
    if (!logFile.is_open()) {
        cerr << "Error: Could not open log file " << logFileName << endl;
        return 1;
    }
    
    // Open the endgame log file
    ofstream endgameLog(endgameLogName);
    if (!endgameLog.is_open()) {
        cerr << "Error: Could not open endgame log file " << endgameLogName << endl;
        return 1;
    }
    
    endgameLog << "=== Robot War Tournament Results ===\n" << endl;
    
    // Run 3 complete games
    for (int game = 1; game <= 3; game++) {
        endgameLog << "\n=== Game " << game << " ===\n" << endl;
        log("\n=== Starting Game " + to_string(game) + " ===\n");
        
        Battlefield field(!endOnOneRobot);  // Disable reentry for simulation2.txt
        loadConfig(inputFile, field);

        cout << "\nStarting Game " << game << " of 3..." << endl;
        cout << "Simulation Controls:" << endl;
        cout << "- Press Enter to advance to next turn" << endl;
        cout << "- Type 'q' and press Enter to quit" << endl;
        cout << "- Type 'a' and press Enter for auto-play mode" << endl;
        cout << "- Type 's' and press Enter to skip to end of current game" << endl;
        cout << "\nStarting simulation in 3 seconds...\n" << endl;
        this_thread::sleep_for(chrono::seconds(3));
        
        bool autoPlay = false;
        bool skipToEnd = false;
        int t = 1;
        while (true) {  // Infinite loop for simulation2.txt
            if (!skipToEnd) {
                clearScreen();
                field.display(t);
                field.simulateTurn();
            } else {
                // Fast forward to end
                field.simulateTurn();
            }
            
            // Check end game conditions based on selected file
            if (endOnOneRobot) {
                // Count alive robots
                int aliveCount = 0;
                for (auto& r : field.getRobots()) {
                    if (r->isAlive()) aliveCount++;
                }
                
                // Stop if only one robot remains
                if (aliveCount <= 1) {
                    if (skipToEnd) {
                        clearScreen();
                        field.display(t);
                    }
                    log("=== Game " + to_string(game) + " Ended - Only One Robot Remains ===");
                    break;
                }
            } else if (t >= STEPS) {  // For simulation1.txt, end after STEPS
                if (skipToEnd) {
                    clearScreen();
                    field.display(t);
                }
                break;
            }
            
            if (!autoPlay && !skipToEnd) {
                cout << "\nGame " << game << " - Turn " << t << " completed. ";
                cout << "Press Enter to continue, 'a' for auto-play, 's' to skip to end, or 'q' to quit: ";
                string input;
                getline(cin, input);
                
                if (input == "q") {
                    cout << "\nSimulation terminated by user." << endl;
                    endgameLog.close();
                    logFile.close();
                    return 0;
                } else if (input == "a") {
                    autoPlay = true;
                    cout << "\nAuto-play mode enabled. Press 'q' to quit." << endl;
                } else if (input == "s") {
                    skipToEnd = true;
                    cout << "\nSkipping to end of current game..." << endl;
                }
            } else if (autoPlay) {
                this_thread::sleep_for(chrono::milliseconds(500));  // Half second delay in auto-play
                if (cin.rdbuf()->in_avail()) {  // Check if user pressed a key
                    string input;
                    getline(cin, input);
                    if (input == "q") {
                        cout << "\nSimulation terminated by user." << endl;
                        endgameLog.close();
                        logFile.close();
                        return 0;
                    }
                }
            }
            
            t++;
        }

        // Log game results
        if (!endOnOneRobot) {
            string result = "=== Game " + to_string(game) + " Completed - Reached Maximum Steps (" + to_string(STEPS) + ") ===\n";
            log(result);
            endgameLog << result;
            
            // Find surviving robots after max steps
            vector<string> survivors;
            for (auto& r : field.getRobots()) {
                if (r->isAlive()) {
                    survivors.push_back(r->getName());
                }
            }
            
            if (survivors.empty()) {
                string msg = "No robots survived the simulation!";
                log(msg);
                endgameLog << msg << "\n";
            } else {
                string msg = "🏆 Surviving robots after " + to_string(STEPS) + " steps:";
                log(msg);
                endgameLog << msg << "\n";
                for (const auto& name : survivors) {
                    string botMsg = "- " + name;
                    log(botMsg);
                    endgameLog << botMsg << "\n";
                }
            }
        } else {
            string result = "=== Game " + to_string(game) + " Completed - Last Robot Standing ===\n";
            log(result);
            endgameLog << result;
            
            // Find the last surviving robot
            for (auto& r : field.getRobots()) {
                if (r->isAlive()) {
                    string msg = "🏆 " + r->getName() + " is the last robot standing! 🏆";
                    log(msg);
                    endgameLog << msg << "\n";
                    break;
                }
            }
        }
        
        if (game < 3) {
            cout << "\nGame " << game << " completed. Starting next game in 3 seconds..." << endl;
            this_thread::sleep_for(chrono::seconds(3));
        }
    }
    
    endgameLog << "\n=== Tournament Complete ===\n";
    cout << "\nAll games completed! Check " << endgameLogName << " for tournament results." << endl;
    cout << "\nPress Enter to exit...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cin.get();
    
    endgameLog.close();
    logFile.close();
    return 0;
}
