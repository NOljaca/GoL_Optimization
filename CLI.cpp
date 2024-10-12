//
// Authors: Nikola Oljaca
//
#include "../include/GameOfLife.h"
#include <chrono>

class CLI{
    int t = 650;
    GameOfLife* gof;

    void create_world(){
        int height, width, populate;
        std::cout << "Enter world height: ";
        std::cin >> height;
        std::cout << "Enter world width: ";
        std::cin >> width;
        std::cout << "Creating world" << std::endl;
        gof = new GameOfLife(height, width);
        std::this_thread::sleep_for(std::chrono::milliseconds(t));

        this->populate();
    }

    void populate(){
        if (!gof) {
            std::cout << "No world created or loaded.\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
            return;
        }
        int t;
        std::cout << "Do you want to populate the world? (0=no, 1=sca-easy, 2=sca-hard ,3=OpenMP): ";
        std::cin >> t;

        if (t == 1){
            std::cout << "Populating using a normal distribution for the values" << std::endl;
            gof->simple_randomize();
            std::cout << "World has been populated" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
        } else if (t == 2){
            std::cout << "The standard values for the population is 0.7 Entropy within 10000 iterations" << std::endl;
            gof->randomize();
            std::cout << "World has been populated" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
        } else if (t == 3){
            std::cout << "The standard values for the population is 0.7 Entropy within 10000 iterations" << std::endl;
            gof->randomize1();
            std::cout << "World has been populated" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
        } else {
            std::cout << "The map will not be populated" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
        }
    }

    void load_world(){
        std::string path;
        std:: cout << "Most of the time the file are stored in the resource folder" << std::endl;
        std::cout << "Please enter the name/path to the file you want to load: ";
        std::cin >> path;
        gof = new GameOfLife(path);
        std::this_thread::sleep_for(std::chrono::milliseconds(t));
    }

    void save_world(){
        if (!gof) {
            std::cout << "No world has been started, please start one before trying to save it.\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
            return;
        }
        std::string name;
        std::cout << "What should be the name for the file? (.txt will be added): ";
        std::cin >> name;
        std::cout << "The world has been saved into resources as "+name << std::endl;;
        gof->save_game(name);
        std::this_thread::sleep_for(std::chrono::milliseconds(t));
    }

    void toggle_display(){
        if (!gof) {
            std::cout << "No world created or loaded.\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
            return;
        }
        gof->toggle_display();
        std::cout << "The display of the world has been changed (standard is OFF)" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(t));
    }

    void set_delay_time_in_ms(){
        if (!gof) {
            std::cout << "No world created or loaded.\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
            return;
        }
        int t;
        std::cout << "Please enter the amount of ms between evolutions (default 200ms)" << std::endl;
        std::cin >> t;
        gof->set_delay(t);
        std::this_thread::sleep_for(std::chrono::milliseconds(t));
    }

    void run_evolution(){
        if (!gof) {
            std::cout << "No world created or loaded.\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
            return;
        }
        int n;
        std::string type;
        std::cout << "Please enter the number of generation that should be simulated: ";
        std::cin >> n;
        std::cout << "Please enter which method should be used for calculation (scalar or CL): ";
        std::cin >> type;
        std::cout << "The simulation is starting... " << n << " generations will be simulated using " << type << std::endl;
        gof->run_simulation(n, type);
        std::this_thread::sleep_for(std::chrono::milliseconds(t));
    }

    void add_figure() {
        if (!gof) {
            std::cout << "No world created or loaded.\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
            return;
        }
        int x, y;
        std::string figure;
        std::cout << "Enter figure (Glider, Toad, Beacon, Methuselah) and position (x, y): ";
        std::cin >> figure >> x >> y;
        // Assuming World class has these methods to add specific figures
        if (figure == "Glider") {
            gof->addGlider(x, y);
        } else if (figure == "Toad") {
            gof->addToad(x, y);
        } else if (figure == "Beacon") {
            gof->addBeacon(x, y);
        } else if (figure == "Methuselah") {
            gof->addMethuselah(x, y);
        } else {
            std::cout << "Invalid figure name.\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(t));
    }

    void set_cell(){
        if (!gof) {
            std::cout << "No world created or loaded.\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
            return;
        }
        int x, y, s;
        std::cout << "Please enter the x,y coordinates and the state (0 or 1)" << std::endl;
        gof->set_state(x,y,s);
        std::this_thread::sleep_for(std::chrono::milliseconds(t));
    }

    void get_cell(){
        if (!gof) {
            std::cout << "No world created or loaded.\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
            return;
        }
        int x, y, s;
        std::cout << "Please enter the x,y coordinates of the cell" << std::endl;
        gof->get_state(x,y);
        std::this_thread::sleep_for(std::chrono::milliseconds(t));
    }

    void present_data() {
        if (!gof) {
            std::cout << "No world created or loaded.\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
            return;
        }

        std::chrono::duration<double> last;
        auto data = gof->get_data();
        for (const auto& duration : data) {
            std::cout << std::chrono::duration_cast<TimeUnit_t>(duration).count() << " ms" << std::endl;
            last += duration; 
        }

        std::cout << "The simulation took in total " << std::chrono::duration_cast<TimeUnit_t>(last).count() << " ms" << std::endl;

        std::cout << "Press enter to go back to the menu" << std::endl;
        std::cin.ignore(); 
        std::cin.get();
    }

    void store_data() {

        if (!gof) {
            std::cout << "No world created or loaded.\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
            return;
        }
        
        // Clear the input buffer
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        std::string fName;
        std::cout << "Please enter the name of the file to be stored: ";
        std::getline(std::cin, fName);

        if (fName.empty()) {
            fName = "../resources/data.txt";
        }

        auto data = gof->get_data();
        std::ofstream file(fName, std::ios_base::app);

        if (file.is_open()) {
            for (const auto& duration : data) {
                file << std::chrono::duration_cast<TimeUnit_t>(duration).count() << " ms\n";
            }
            file << "\n"; // Add an empty line after each set of data
            file.close();
            std::cout << "Data stored in " << fName << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
        } else {
            std::cerr << "Failed to open file " << fName << " for writing." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(t));
        }
    }

public:
    void start() {
        int choice = 0;
        do {
            std::cout << "\033[2J\033[H";
            std::cout << "1. Create world\n"
                      << "2. Load world\n"
                      << "3. Populate world\n"
                      << "4. Save world\n"
                      << "5. Toggle printing\n"
                      << "6. Set delay\n"
                      << "7. Run simulation\n"
                      << "8. Set cell state\n"
                      << "9. Get cell state\n"
                      << "10. Add figure\n"
                      << "11. Present data\n"
                      << "12. Save data\n"
                      << "0. Exit\n"
                      << "Enter choice: ";
            std::cin >> choice;
            switch (choice) {
                case 1: create_world(); break;
                case 2: load_world(); break;
                case 3: populate(); break;
                case 4: save_world(); break;
                case 5: toggle_display(); break;
                case 6: set_delay_time_in_ms(); break;
                case 7: run_evolution(); break;
                case 8: set_cell(); break;
                case 9: get_cell(); break;
                case 10: add_figure(); break;
                case 11: present_data(); break;
                case 12: store_data(); break;
                case 0: break;
                default: std::cout << "Invalid choice, try again.\n";
            }
        } while (choice != 0);
        delete gof;
    }
};

