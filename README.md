# KLL-Polymer: Optimal Per-Key Streaming Quantile Estimation

This repository provides source codes for **KLL-Polymer** and its variant, KLL-Polymer with deterministic compaction (**KLL-Polymer-DC** for short) for per-key quantile estimation in data streams. 
The CPU implementation also provides variants for evaluating the sampler optimization, the fixed-capacity top levels, and the MG-GK optimization of KLL-Polymer.

## Dataset settings

We evaluate **KLL-Polymer** and its variant **KLL-Polymer-DC** on six datasets, including two real datasets (CAIDA and MAWI) and four synthetic datasets. 

To run on CAIDA or MAWI dataset, download the datasets from their websites. 

To run synthetic datasets, run `python generate-dataset.py` to generate synthetic datasets, and compile the source codes to run. 

## Baselines

We also provide source codes for baseline solutions, including **HistSketch**, **SQUAD**, **SketchPolymer** and **M4**. 


## Run KLL-Polymer and KLL-Polymer-DC on static datasets: 

First enter the `cpu` directory. 
```bash
$ cd cpu
```

Compile the source code using:

```bash
$ make
```

Then use the following command to run: 

```bash
$ ./test [DATASET_PATH] [SAVE_PATH]
```

The ARE and AQE results are printed in the console, and AQE for different quantiles are saved in `SAVE_PATH`. 

## Run KLL-Polymer variants on static datasets:

The CPU source codes also provide **KLL_Polymer_No_Opt**, **KLL_Polymer_Opt1** and **KLL_Polymer_Opt3** in `cpu/KLL-Polymer.h`. These variants evaluate the optimizations of KLL-Polymer together with KLL-Polymer and KLL-Polymer-DC.

**KLL_Polymer_No_Opt** uses decreasing capacities at all levels, without replacing the lowest levels with a sampler. **KLL_Polymer_Opt1** adds the sampler optimization to this version. **KLL_Polymer** also uses fixed-capacity top levels. **KLL_Polymer_Opt3** uses the sampler and lower-level compaction together with Misra-Gries (MG) key tracking and a GK summary at the top. **KLL_Polymer_DC** uses deterministic compaction.

To run the variant comparison, compile the source codes in the `cpu` directory and use the following command:

```bash
$ ./test --variants [DATASET_PATH] [MEMORY_KiB]
```

The memory argument is optional and defaults to 8000 KiB. For example, use `./test --variants [DATASET_PATH] 16000` to run with 16000 KiB. Use `./test --help` to display the available commands.

Each configuration runs five times with random seeds from 1 to 5. The output labels are `no opt` for KLL_Polymer_No_Opt, `opt 1` for KLL_Polymer_Opt1, `opt 2` for KLL_Polymer, `opt 3` for KLL_Polymer_Opt3, and `dc` for KLL_Polymer_DC.

Each output row contains the configuration label, `memory / 1024` (integer MiB), average ARE, average AQE, average insertion throughput, average rank-query throughput, average quantile-query throughput, and average retained sample weight difference. Throughput is measured in millions of operations per second. The weight difference is `N` minus the total retained sample weight.

The results are printed in the console. To save the output, use:

```bash
$ ./test --variants [DATASET_PATH] 8000 > variants.out
```

## RocksDB implementation

We implement KLL-Polymer on top of RocksDB database to reduce tail latency for quantile queries. 

## Run KLL-Polymer on RocksDB: 

First enter the `db` directory.

```bash
$ cd db
```

Clone the repository of RocksDB from GitHub: 

```bash
$ git clone https://github.com/facebook/rocksdb.git
```

Compile RocksDB project to obtain `librocksdb.a`. 

```bash
$ cd rocksdb
$ make
```

Then run KLL-Polymer on RocksDB using the following command: 

```bash
$ cd ..
$ make
$ ./kll_bench [DATASET_PATH] [RUN_MODE(heavy/mixed)] [MEMORY]
```

The results are printed in the console. 