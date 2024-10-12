#ifndef GAMEOFLIFE_H
#define GAMEOFLIFE_H

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <stdio.h>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <random>
#include <algorithm>
#include <chrono>
#include <thread>
#include <gegl-0.4/opencl/cl.h>
#include <omp.h>

using Clock_t = std::chrono::steady_clock;
using TimeUnit_t = std::chrono::milliseconds;


class GameOfLife {
    private:

        const std::string LIVE = "\033[32mX\033[0m";
        const std::string DEAD = "\033[90mO\033[0m";
        const std::string CLEAN = "\033[2J\033[H";

        std::vector<std::chrono::duration<double>> data;
        int width, height;
        size_t w_size;
        std::vector<uint8_t> past, present, future; // This definition is completely optional
        bool print_enable = false;
        bool debug = false;
        int print_delay_ms = 200;

        void evolve(std::vector<uint8_t>& map, std::vector<uint8_t>&next, std::vector<uint8_t>& neighbors);
        void evolve_opencl();
        bool compare_cl();
        bool is_stable();
        void print();
        void load(std::string p);
        void save(std::string name);
        std::vector<uint8_t> count_neighbors(const std::vector<uint8_t>& vec);
        uint8_t get_element_value(const std::vector<uint8_t> &col, size_t x, size_t y) const;

        // OpenCL variables
        cl_context context;
        cl_command_queue queue;
        cl_program program;
        cl_kernel evolve_kernel;
        cl_kernel compare_kernel;
        void setupOpenCL();
        std::string readKernelSource(const char *filename);

        // Extra stuff
        double get_entropy(const std::vector<uint8_t>& map);

    public:
        GameOfLife(int h, int w);
        GameOfLife(const std::string& path);
        void simple_randomize(); // for bigger maps.
        void randomize(double targetEntropy=0.7, int maxIterations = 10000); // Default value is entropy of 0.7 and 10000 iterations
        void randomize1(double targetEntropy=0.7, int maxIterations = 10000);// Same as randomize() but parallelized
        void run_simulation(int gens, std::string type); // type must be "scalar" or "CL", this is case sensitive
        void toggle_display() {print_enable = !print_enable;} // Default is always OFF
        void toggle_debug(){debug = !debug;}// Default is OFF
        void set_delay(size_t delay_ms) {print_delay_ms = delay_ms;} // Default delay is 200ms
        void save_game(std::string name);
        void load_world(std::string path);
        void set_state(size_t i, uint8_t s);
        void set_state(size_t x, size_t y, uint8_t s);
        void set_states(std::vector<std::tuple<size_t, size_t, uint8_t>>& states);
        int get_state(size_t i);
        int get_state(size_t x, size_t y);
        void addGlider(size_t x, size_t y); // Start position is the middle bottom cell.
        void addToad(size_t x, size_t y); // Start position is the middle of the first "stick".
        void addBeacon(size_t x, size_t y); // Start position is the bottom-left corner.
        void addMethuselah(size_t x, size_t y);// Start position is the center of figure.
        void display(); // For testing porpuse only streams the map into the console.
        void checkError(cl_int err, const char* operation);
        std::vector<std::chrono::duration<double>> get_data();
};


#endif //GAMEOFLIFE_H