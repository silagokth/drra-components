/*
 * Uncomment the following macro to enable debug mode.
 */
#define DEBUG

/*
 * Uncomment the following macro to generate human readable output file for
 * debugging. It is mandatory to enable define DATA_TYPE if DEBUG is enabled.
 */
#define DATA_TYPE int16_t

#include "Drra.hpp"
#include "Util.hpp"
#include <cstdlib>


int main() { return run_simulation(); }

/*
 * Generate the input SRAM image.
 */
void init() {
#define N 16

    vector<int32_t> v(N + N / 2);   // Data and Twiddle factors
        
    ifstream datafile("../fft_data/input_data.txt"); // Open the file
    if (!datafile) {
        std::cerr << "Error opening file!" << std::endl;
        return;
    }

    float real, imag;
    for (int i = 0; i < N; i++) {
        if (!(datafile >> real >> imag)) {
            std::cerr << "Error reading file!" << std::endl;
            return;
        }
        int16_t real_fixed = static_cast<int16_t>(real * 256); // Convert to fixed-point
        int16_t imag_fixed = static_cast<int16_t>(imag * 256);
        v[i] = (static_cast<int32_t>(imag_fixed) & 0xFFFF) | (static_cast<int32_t>(real_fixed) << 16);
    }
    datafile.close();

    ifstream twiddlefile("../fft_data/twiddle_factors.txt"); // Open the file
    if (!twiddlefile) {
        std::cerr << "Error opening file!" << std::endl;
        return;
    }

    for (int i = 0; i < N/2; i++) {
        if (!(twiddlefile >> real >> imag)) {
            std::cerr << "Error reading file!" << std::endl;
            return;
        }
        int16_t real_fixed = static_cast<int16_t>(real * 256); // Convert to fixed-point
        int16_t imag_fixed = static_cast<int16_t>(imag * 256);
        v[i + N] = (static_cast<int32_t>(imag_fixed) & 0xFFFF) | (static_cast<int32_t>(real_fixed) << 16);
    }
    twiddlefile.close();
    
    // Write the processed numbers to the input buffer
    __input_buffer__.write<int32_t>(0, 3, v);
}

/*
 * Define the reference algorithm model. It will generate the reference output
 * SRAM image. You can use free C++ programs.
 */
void model_l0() {
#define N 16

    // Read output file from MATLAB and write to the output buffer for comparison
    vector<int32_t> X(N);   // Data and Twiddle factors
    
    ifstream outfile("../fft_data/output_data.txt"); // Open the file
    if (!outfile) {
        std::cerr << "Error opening file!" << std::endl;
        return;
    }
    
    float real, imag;
    for (int i = 0; i < N; i++) {
        if (!(outfile >> real >> imag)) {
            std::cerr << "Error reading file!" << std::endl;
            return;
        }
        int16_t real_fixed = static_cast<int16_t>(real * 256); // Convert to fixed-point
        int16_t imag_fixed = static_cast<int16_t>(imag * 256);
        X[i] = (static_cast<int32_t>(imag_fixed) & 0xFFFF) | (static_cast<int32_t>(real_fixed) << 16);
    }
    outfile.close();

    __output_buffer__.write<int32_t>(0, 2, X);  
}

/*
 * Define the DRR algorithm model. It will generate the DRR output SRAM image by
 * simulate the instruction execution using python simulator.
 */
void model_l1() { simulate_code_segment(0); }
