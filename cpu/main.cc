#include <bits/stdc++.h>
#include "Benchmark.h"
using namespace std;

void comparison_test(string path, string save_path) {
	int start = 2000, end = 32000, step = 2;
	Benchmark benchmark(path, save_path);
	benchmark.generate_ground_truth();
	
	for (int memory = start; memory <= end; memory *= step) {
		benchmark.Run_KLL_Polymer(memory);
	}
	std::cout << "\n";
	for (int memory = start; memory <= end; memory *= step) {
		benchmark.Run_HistSketch(memory);
	}
	std::cout << "\n";
	for (int memory = start; memory <= end; memory *= step) {
		benchmark.Run_SQUAD(memory);
	}
	std::cout << "\n";
	for (int memory = start; memory <= end; memory *= step) {
		benchmark.Run_SketchPolymer(memory);
	}
	std::cout << "\n";
	for (int memory = start; memory <= end; memory *= step) {
		benchmark.Run_M4(memory);
	}
	std::cout << "\n";
	for (int memory = start; memory <= end; memory *= step) {
		benchmark.Run_KLL_Polymer_DC(memory);
	}
	std::cout << "\n";
}

void dc_test(string path, string save_path) {
	int start = 10, end = 30, step = 1;
	int memory = 8000;

	Benchmark** benchmark = new Benchmark* [21];
	for (int alpha = start; alpha <= end; alpha += step) {
		string new_path = path + std::to_string(alpha) + ".bin";
		benchmark[alpha - 10] = new Benchmark(new_path, save_path);
		benchmark[alpha - 10]->generate_ground_truth();
	}
	for (int alpha = start; alpha <= end; alpha += step) {
		benchmark[alpha - 10]->Run_KLL_Polymer(memory);
	}
	for (int alpha = start; alpha <= end; alpha += step) {
		benchmark[alpha - 10]->Run_KLL_Polymer_DC(memory);
	}
}

void variants_test(string path, int memory) {
	Benchmark benchmark(path, "");
	benchmark.generate_ground_truth();
	benchmark.Run_KLL_Polymer_Variants(memory);
}

void print_usage() {
	std::cerr << "Usage: ./test [DATASET_PATH] [SAVE_PATH]\n"
	          << "       ./test --variants [DATASET_PATH] [MEMORY_KiB]\n"
	          << "       ./test --help\n"
	          << "MEMORY_KiB is optional and defaults to 8000.\n";
}

int main(int argc, char** argv) {
	if (argc == 2 && (string(argv[1]) == "--help" || string(argv[1]) == "-h")) {
		print_usage();
		return 0;
	}
	if (argc > 1 && string(argv[1]) == "--variants") {
		if (argc < 3 || argc > 4) {
			print_usage();
			return 1;
		}
		int memory = 8000;
		if (argc == 4) {
			try {
				string memory_arg = argv[3];
				size_t parsed_length = 0;
				memory = std::stoi(memory_arg, &parsed_length);
				if (parsed_length != memory_arg.size() || memory <= 0 || memory > INT_MAX / 1024) {
					throw std::invalid_argument("Invalid memory setting");
				}
			}
			catch (const std::exception&) {
				std::cerr << "MEMORY_KiB must be a positive integer no greater than "
				          << INT_MAX / 1024 << ".\n";
				return 1;
			}
		}
		variants_test(argv[2], memory);
		return 0;
	}
	if (argc < 3) {
		print_usage();
		return 1;
	}
	string path = argv[1], save_path = argv[2];
	comparison_test(path, save_path);
	// dc_test(path, save_path);
	return 0;
}
