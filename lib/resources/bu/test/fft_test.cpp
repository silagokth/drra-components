// fftw64.cpp

#include <cmath>
#include <fftw3.h>
#include <fstream>
#include <iomanip>
#include <iostream>

int main() {
  // Constants
  const double fs = 1000.0;
  const double pi = 3.14159265358979323846;

  // Get the number of samples from the input file
  std::ifstream file("fft_data/input_data.txt");
  std::string line;
  int N = 0;
  while (std::getline(file, line)) {
    ++N;
  }

  // Allocate input and output arrays
  fftw_complex *in = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N);
  fftw_complex *out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * N);

  // Create FFTW plan
  fftw_plan p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

  // Store the samples in the input array
  file.clear();
  file.seekg(0, std::ios::beg);
  for (int n = 0; n < N; ++n) {
    std::getline(file, line);
    std::istringstream iss(line);
    double real, imag;
    iss >> real >> imag;
    in[n][0] = real; // Real part
    in[n][1] = imag; // Imaginary part
  }

  // Execute FFT
  fftw_execute(p);

  // Output results (real and imag parts) to file
  std::ofstream fout("output_data.txt");
  if (!fout) {
    std::cerr << "Error opening output_data.txt for writing\n";
    return 1;
  }

  for (int i = 0; i < N; ++i) {
    fout << std::fixed << std::setprecision(8) << out[i][0] << " " << out[i][1]
         << std::endl;
  }
  fout.close();

  // Clean up
  fftw_destroy_plan(p);
  fftw_free(in);
  fftw_free(out);

  return 0;
}
