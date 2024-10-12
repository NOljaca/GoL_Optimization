//
// Authors: Richard Nicols and Nikola Oljaca
//

#include <utility>
#include <functional>
#include <atomic>
#include "../include/GameOfLife.h"

GameOfLife::GameOfLife(int h, int w) : height(h), width(w){
    
    this->w_size = (height * width);

    // Assign memory
    past.resize(w_size);
    present.resize(w_size);
    future.resize(w_size);
}

GameOfLife::GameOfLife(const std::string &path) {
    load(path);
}

void GameOfLife::run_simulation(int gens, std::string type) {

    auto start = Clock_t::now();
    auto finish = Clock_t::now();

    if (((height % 10 != 0) || (width % 10 != 0)) && type == "CL") {
        std::cout << std::endl;
        std::cout << "====================================================================================" << std::endl;
        std::cout << "The size of the world does not match the group size to use OpenCL changing to Scalar" << std::endl;
        std::cout << "====================================================================================" << std::endl;
        std::cout << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        type = "scalar";
    }

    std::function<void()> evolve_func;
    std::function<bool()> compare_func;
    if (type == "CL") {
        setupOpenCL();
        evolve_func = [this]() { evolve_opencl(); };
        compare_func = [this]() { return compare_cl(); };
    } else {
        evolve_func = [this]() {
            std::vector<uint8_t> neighbors = count_neighbors(present);
            evolve(present, future, neighbors);
        };
        compare_func = [this]() { return is_stable(); };
    }

    for (int i = 0; i < gens; ++i) {
        if (print_enable) {
            print();
            std::this_thread::sleep_for(std::chrono::milliseconds(print_delay_ms));
        }
        
        start = Clock_t::now();
        evolve_func();

        if (compare_func()) {
            std::cout << "The system is stable and the simulation has been stopped" << std::endl;
            break;
        }
        finish = Clock_t::now();

        data.push_back(finish - start);
        // Assign new values to compare later
        past = present;
        present = future;

        if (debug) {
            std::cout << "starting " << i << " generation" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}


void GameOfLife::evolve_opencl(){
    cl_int err;

    // Creates the buffers
    cl_mem bufferMap = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(uint8_t)*w_size, present.data(), &err);
    checkError(err, "clCreateBuffer (bufferMap)");
    cl_mem bufferNext = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(uint8_t)*w_size, nullptr, &err);
    checkError(err, "clCreateBuffer (bufferNext)");

    // Set the arguments for the evolve_kernel function
    err = clSetKernelArg(evolve_kernel, 0, sizeof(cl_mem), &bufferMap);
    checkError(err, "Kernel Arg map: ");
    err = clSetKernelArg(evolve_kernel, 1, sizeof(cl_mem), &bufferNext);
    checkError(err, "Kernel Arg future: ");
    // OMFG how are we supposed to know that size_t as other types are not supported by Kernel??
    err = clSetKernelArg(evolve_kernel, 2, sizeof(int), &width);
    checkError(err, "Kernel Arg width: ");
    err = clSetKernelArg(evolve_kernel, 3, sizeof(int), &height);
    checkError(err, "Kernel Arg height: ");

    // Specifies the global and local work sizes
    size_t global_work_size[2] = {(size_t)width, (size_t)height};
    size_t local_work_size[2] = {10, 10};

    // Queues the evolve_kernel and then reads the data back to the future array
    err = clEnqueueNDRangeKernel(queue, evolve_kernel, 2, nullptr, global_work_size, local_work_size, 0, nullptr, nullptr);
    checkError(err, "clEnqueueNDRangeKernel");
    err = clEnqueueReadBuffer(queue, bufferNext, CL_TRUE, 0, sizeof(uint8_t)*w_size, future.data(), 0, nullptr, nullptr);
    checkError(err, "clEnqueueReadBuffer");

    // Release the buffer
    clReleaseMemObject(bufferMap);
    clReleaseMemObject(bufferNext);
}

bool GameOfLife::compare_cl(){
    cl_int err;
    bool result = true;

    // Set the work batch sizes
    size_t global_work_size[1] = {(size_t)w_size};
    size_t local_work_size[1] = {10};

    // Creates Buffers
    cl_mem buffer_past = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(uint8_t)*w_size, past.data(), &err);
    checkError(err, "clCreateBuffer (buffer_past comparison)");
    cl_mem buffer_present = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(uint8_t)*w_size, present.data(), &err);
    checkError(err, "clCreateBuffer (buffer_present comparison)");
    cl_mem buffer_future = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(uint8_t)*w_size, future.data(), &err);
    checkError(err, "clCreateBuffer (buffer_future comparison)");
    cl_mem buffer_result = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(bool), nullptr, &err);
    checkError(err, "clCreateBuffer (buffer_result comparison)");

    // Set Arguments
    err = clSetKernelArg(compare_kernel, 0, sizeof(cl_mem), &buffer_present);
    checkError(err, "Kernel Arg::compare::present: ");
    err = clSetKernelArg(compare_kernel, 1, sizeof(cl_mem), &buffer_future);
    checkError(err, "Kernel Arg::compare::future: ");
    err = clSetKernelArg(compare_kernel, 2, sizeof(cl_mem), &buffer_result);
    checkError(err, "Kernel Arg::compare::result: ");

    // Compare n to n-1 generation
    err = clEnqueueNDRangeKernel(queue, compare_kernel, 1, nullptr, global_work_size, local_work_size, 0, nullptr, nullptr);
    checkError(err, "clEnqueueNDRangeKernel comparison::function1");
    err = clEnqueueReadBuffer(queue, buffer_result, CL_TRUE, 0, sizeof(bool), &result, 0, nullptr, nullptr);
    checkError(err, "clEnqueueNDRangeKernel comparison::function::writeback1");
    clReleaseMemObject(buffer_present);
    
    //compare n to the n-2 generation
    err = clSetKernelArg(compare_kernel, 0, sizeof(cl_mem), &buffer_past);
    checkError(err, "Kernel Arg::compare::present: ");
    err = clSetKernelArg(compare_kernel, 1, sizeof(cl_mem), &buffer_future);
    checkError(err, "Kernel Arg::compare::future: ");
    err = clSetKernelArg(compare_kernel, 2, sizeof(cl_mem), &buffer_result);
    checkError(err, "Kernel Arg::compare::result: ");
    err = clEnqueueNDRangeKernel(queue, compare_kernel, 1, nullptr, global_work_size, local_work_size, 0, nullptr, nullptr);
    checkError(err, "clEnqueueNDRangeKernel comparison::function2");
    
    err = clEnqueueReadBuffer(queue, buffer_result, CL_TRUE, 0, sizeof(bool), &result, 0, nullptr, nullptr);
    checkError(err, "clEnqueueNDRangeKernel comparison::function::writeback2");

    // Free memory
    clReleaseMemObject(buffer_past);
    clReleaseMemObject(buffer_future);
    clReleaseMemObject(buffer_result);

    return result;
}

void GameOfLife::evolve(std::vector<uint8_t>& map, std::vector<uint8_t>& next, std::vector<uint8_t>& neighbors) {
    std::transform(map.begin(), map.end(), neighbors.begin(), next.begin(), [](uint8_t live, uint8_t n) {
        if (live == 1) {
            return (n == 2 || n == 3) ? static_cast<uint8_t>(1) : static_cast<uint8_t>(0); 
        }
        return (n == 3) ? static_cast<uint8_t>(1) : static_cast<uint8_t>(0);
    });
}


std::vector<uint8_t> GameOfLife::count_neighbors(const std::vector<uint8_t>& vec) {
    // Initialize vector to store the results
    std::vector<uint8_t> result;

    // Calculates the state of the 8 neighbors around each cell
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {

            int count = 0;

            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {

                    if (dx == 0 && dy == 0) continue;

                    if (get_element_value(vec, j + dx, i + dy) == 1) {
                        count++;
                    }
                }
            }
            result.push_back(count);
        }
    }
    return result;
}

uint8_t GameOfLife::get_element_value(const std::vector<uint8_t>& map, size_t x, size_t y) const {
    /*
    + Be noted that this is necesary as C style modulo can return negative values, which destroyed the
    + toroidal aspect of the world.
    */
    x = (x+width) % width;
    y = (y+height) % height;
    return map[y * width + x];
}

std::string GameOfLife::readKernelSource(const char* filename) {
    // This function comes from what was showed in task 08
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void GameOfLife::setupOpenCL(){
    // Initialize a int to store error codes
    cl_int err;

    // OpenCL platform configuration
    cl_uint platformCount;
    cl_platform_id platform;
    err = clGetPlatformIDs(1, &platform, &platformCount);
    checkError(err, "clGetPlatformIDs");

    // OpenCL devices configuration
    cl_uint deviceCount;
    cl_device_id device;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, &deviceCount);
    checkError(err, "clGetDeviceIDs");
    
    // Assigning the context and the queue for said context
    this->context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    checkError(err, "clCreateContext");
    this->queue = clCreateCommandQueue(context, device, 0, &err);
    checkError(err, "clCreateCommandQueue");

    // Get the path to the evolve_kernel
    std::string path = readKernelSource("evolution.cl");
    const char *source = path.c_str();

    // Assign the program to the context with the gathered evolve_kernel function
    this->program = clCreateProgramWithSource(context, 1, &source, nullptr, &err);
    checkError(err, "clCreateProgramWithSource");

    // Creates the program
    err = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
    if(err != CL_SUCCESS){
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
        std::vector<char> log(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log.data(), nullptr);
        std::cerr << "Error during operation 'clBuildProgram': " << err << std::endl;
        std::cerr << "Build log:" << std::endl << log.data() << std::endl;
        exit(1);
    }

    this->evolve_kernel = clCreateKernel(program, "evolve", &err);
    checkError(err, "clCreateKernel evolve");

    //======================================================================================
    
    // Get the path to the compare_kernel
    path = readKernelSource("comparison.cl");
    const char *src = path.c_str();

    this->program = clCreateProgramWithSource(context, 1, &src, nullptr, &err);
    checkError(err, "clCreateProgramWithSource");

    // Creates the program
    err = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
    if(err != CL_SUCCESS){
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
        std::vector<char> log(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log.data(), nullptr);
        std::cerr << "Error during operation 'clBuildProgram': " << err << std::endl;
        std::cerr << "Build log:" << std::endl << log.data() << std::endl;
        exit(1);
    }

    this->compare_kernel = clCreateKernel(program, "compare", &err);
    checkError(err, "clCreateKernel comparison");
}

bool GameOfLife::is_stable() {
    return (std::equal(present.begin(), present.end(), future.begin())
    || std::equal(past.begin(), past.end(), future.begin()));
}

void GameOfLife::print() {
    int l = 0;

    if (!debug) {
    std::cout << CLEAN;
    std::for_each(present.begin(), present.end(), [this, &l](int live){
            std::cout << (live ? LIVE : DEAD) << " ";
            l++;
            if (l%width == 0){std::cout << "\n";
            }
        });
    } else {
        std::cout << "This is the present map" << std::endl;
        std::for_each(present.begin(), present.end(), [this, &l](int live) {
            std::cout << live << " "; 
            l++;
            if (l % width == 0) {
                std::cout << "\n";
            }
        });
        std::cout << std::endl;
        std::cout << "This is the neighbor map" << std::endl;
        std::vector<uint8_t> t = count_neighbors(present);
        int l = 0;  // Initialize the line counter
        std::for_each(t.begin(), t.end(), [this, &l](int live) {
            std::cout << live << " ";  // Cast to int for printing
            l++;
            if (l % width == 0) {
                std::cout << "\n";
            }
        });
    }
}

void GameOfLife::load(std::string p) {
    std::ifstream file(p);
    if (file.is_open()) {
        if (!(file >> height >> width)) {
            throw std::runtime_error("Failed to read height and width from file.");
        }
        
        w_size = height * width;
        
        // Initialize vectors
        past = std::vector<uint8_t>(w_size);
        present = std::vector<uint8_t>(w_size);
        future = std::vector<uint8_t>(w_size);

        for (uint8_t & pre : present) {
            char c;
            if (!(file >> c) || (c != '0' && c != '1')) {
                throw std::runtime_error("Invalid data in file. Expected '0' or '1'.");
            }
            pre = c - '0';  // Convert ASCII '0' or '1' to numeric 0 or 1
        }
    } else {
        throw std::runtime_error("Failed to open file: " + p);
    }
}

void GameOfLife::save(std::string name) {
    std::ofstream file("../resources/"+name+".txt");
    
    if (file.is_open()) {
        file << height << " " << width << "\n";
        for (const uint8_t& num : present) {
            file << static_cast<int>(num) << "\n";
        }
    } else {
        throw std::runtime_error("Failed to open file for writing.");
    }
}

void GameOfLife::save_game(std::string name){
    save(name);
}

void GameOfLife::load_world(std::string path) {
    load(path);
}

void GameOfLife::set_state(size_t i, uint8_t s) {
    if (i > present.size()) {
        std::cout << "Invalid index";
        return;
    }
    present[i] = s;
}

void GameOfLife::set_state(size_t x, size_t y, uint8_t s) {
    x = (x+width) % width;
    y = (y+height) % height;
    present[y * width + x] = s;
}

void GameOfLife::set_states(std::vector<std::tuple<size_t, size_t, uint8_t>>& states) {

    for (std::tuple<size_t, size_t, uint8_t> c: states){
        auto [x, y, s] = c;
        set_state(x, y, s);
    }
}

int GameOfLife::get_state(size_t i) {
    if (i > present.size()) {
        std::cout << "Invalid index";
        return -1;
    }
    return present[i];
}

int GameOfLife::get_state(size_t x, size_t y) {
    x = (x+width) % width;
    y = (y+height) % height;
    return present[y * width + x];
}

void GameOfLife::addGlider(size_t x, size_t y) {
    set_state(x, y+1, 1);
    set_state(x+1, y-1, 1);
    set_state(x+1, y, 1);
    set_state(x+1, y+1, 1);
    set_state(x-1, y, 1);
}

void GameOfLife::addToad(size_t x, size_t y) {
    set_state(x, y, 1);
    set_state(x+1, y+1, 1);
    set_state(x+1, y-1, 1);
    set_state(x+1, y, 1);
    set_state(x, y-1, 1);
    set_state(x, y-2, 1);
}

void GameOfLife::addBeacon(size_t x, size_t y) {
    set_state(x, y, 1);
    set_state(x, y-1, 1);
    set_state(x+1, y, 1);
    set_state(x+3, y-3, 1);
    set_state(x+3, y-2, 1);
    set_state(x+2, y-3, 1);
}

void GameOfLife::addMethuselah(size_t x, size_t y) { // Creates a R-Pentonimo
    set_state(x,y,1);
    set_state(x-1,y,1);
    set_state(x,y+1,1);
    set_state(x,y-1,1);
    set_state(x+1,y-1,1);
}

void GameOfLife::display(){
    print();
}

/* Taken from given source code */
void GameOfLife::checkError(cl_int err, const char* operation) {
    if (err != CL_SUCCESS) {
        std::cerr << "Error during operation '" << operation << "': " << err << std::endl;
        exit(1);
    }
}

double GameOfLife::get_entropy(const std::vector<uint8_t>& world_map) {
    std::unordered_map<int, int> frequencies;
    int totalCells = w_size;
    int numThreads = omp_get_max_threads();
    
    // Create a vector of maps to store local frequency counts for each thread
    std::vector<std::unordered_map<int, int>> partialSum(numThreads);

    #pragma omp parallel
    {
        int threadId = omp_get_thread_num();
        std::unordered_map<int, int>& localFrequencyMap = partialSum[threadId];
        
        #pragma omp for
        for (int i = 0; i < totalCells; ++i) {
            localFrequencyMap[world_map[i]]++;
        }
    }

    // Combine them back into the global map
    for (const auto& localFrequencyMap : partialSum) {
        for (const auto& pair : localFrequencyMap) {
            frequencies[pair.first] += pair.second;
        }
    }

    // Calculate the Shannon entropy
    double entropy = 0.0;
    for (const auto& pair : frequencies) {
        double probability = static_cast<double>(pair.second) / totalCells;
        if (probability > 0) {
            entropy -= probability * std::log2(probability);
        }
    }
    if (debug){
        std::cout << entropy << " This is entropy" << std::endl;
    }
    return entropy;
}

void GameOfLife::simple_randomize(){
    std::random_device rd;
    std::mt19937 gen(rd());
    
    std::uniform_int_distribution<> dis(0,1);
    std::for_each(present.begin(), present.end(), [&](uint8_t &cell){
        cell = static_cast<uint8_t>(dis(gen));
    });
}

void GameOfLife::randomize(double targetEntropy, int maxIterations) {
    // Randomnes
    std::random_device rd;
    std::mt19937 gen(rd());

    // Needed calculation (I did this to avoid overpopulation)
    double entropy = get_entropy(present);
    int iteration = 0;

    // Adjust population to achieve target entropy
    while ((entropy - targetEntropy) < 0.01 && iteration < maxIterations) {
        //Random numbers
        std::uniform_int_distribution<> dis(0,4);
        std::uniform_int_distribution<> indx(0, width - 1);
        std::uniform_int_distribution<> indy(0, height - 1);

         
        int k = dis(gen);
        int x = indx(gen);
        int y = indy(gen);
        
        switch(k)
        {
            case 0:
            //Random figure that i found in internet
                set_state(x,y,1);
                set_state(x+1,y,1);
                set_state(x+2,y,1);
                set_state(x-1,y-1,1);
                break;
            case 1:
                addBeacon(x,y);
                break;
            case 2:
                addToad(x,y);
                break;
            case 3:
                addGlider(x,y);
                break;
            case 4:
                addMethuselah(x,y);
                break;
        }
        
        entropy = get_entropy(present);
        iteration++;
    }
}

void GameOfLife::randomize1(double targetEntropy, int maxIterations) {
    // Randomness
    std::random_device rd;
    std::mt19937 gen(rd());

    // Calculate initial entropy
    double entropy = get_entropy(present);
    std::atomic<bool> flag(false);

    // Adjust population to achieve target entropy
    #pragma omp parallel
    {   
        // Random numbers
        std::uniform_int_distribution<> dis(0, 4);
        std::uniform_int_distribution<> indx(0, width - 1);
        std::uniform_int_distribution<> indy(0, height - 1);

        #pragma omp for 
        for (int iteration = 0; iteration < maxIterations; ++iteration) {
            
            int k = dis(gen);
            int x = indx(gen);
            int y = indy(gen);
            
            if (flag.load()) continue;

            // Update entropy (Race condition MUST be avoided)
            // Adds the figure following a random normal distribution
            #pragma omp critical
            {
                switch(k) {
                    case 0:
                        // Random figure that I found on the internet
                        set_state(x, y, 1);
                        set_state(x + 1, y, 1);
                        set_state(x + 2, y, 1);
                        set_state(x - 1, y - 1, 1);
                        break;
                    case 1:
                        addBeacon(x, y);
                        break;
                    case 2:
                        addToad(x, y);
                        break;
                    case 3:
                        addGlider(x, y);
                        break;
                    case 4:
                        addMethuselah(x, y);
                        break;
                }
                entropy = get_entropy(present);
            }

            if(debug){
                std::cout << "We are in iteration " << iteration << " of the population" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }

            // Check if desired entropy level was achieved
            if ((entropy - targetEntropy) >= 0.01) {
                /*
                Stops the loop execution thanks to https://stackoverflow.com/questions/9793791/parallel-openmp-loop-with-break-statement
                that helped me to get how to get out as break is not allowed inside OpenMP loops.
                */
                flag.store(true);
                #pragma omp cancel for
            }
        }
    }
}

std::vector<std::chrono::duration<double>> GameOfLife::get_data(){
    return this->data;
}
