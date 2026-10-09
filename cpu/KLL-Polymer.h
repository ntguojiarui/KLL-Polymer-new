#ifndef _KLLPOLYMER_H_
#define _KLLPOLYMER_H_

#include <bits/stdc++.h>

template<typename ID_TYPE, typename DATA_TYPE>
class KLL_Polymer {
public:
    KLL_Polymer() {}
    KLL_Polymer(int _N, double _c, int _s, int memory): N(_N), c(_c), s(_s) {
        /*
        N: length of the data stream
        c: decreasing rate of capacity
        s: number of levels with fixed capacity
        memory: memory of KLL-Polymer
        */
        int capacity_sum = memory * 1024 / (sizeof(ID_TYPE) + sizeof(DATA_TYPE));
        l = capacity_sum / (s + pow(c, s) / (1 - c));
        H = log2(1.0 * N / l) + 2;
        assert(s < H);
        for (int i = 0; i < H - s; ++i) {
            int capacity = std::max(
                (int)ceil(1.0 * l * pow(c, H - 1 - i)), 2);
            if (capacity <= 2) {
                reservoir_level = i + 1;
            }
            else {
                std::vector<std::pair<ID_TYPE, DATA_TYPE>> buffer;
                array.push_back(buffer);
                max_size.push_back(capacity);
            }
        }
        for (int i = H - s; i < H; ++i) {
            std::vector<std::pair<ID_TYPE, DATA_TYPE>> buffer;
            array.push_back(buffer);
            max_size.push_back(l);
        }
        assert(array.size() + reservoir_level == H);
    }

    ~KLL_Polymer() {}

    void insert(ID_TYPE key, DATA_TYPE value) {
        // Step 1: in lower level, handling sampled values
        current_reservoir_samples++;
        double r = static_cast<double>(std::rand()) / RAND_MAX;
        if (r <= 1.0 / current_reservoir_samples) {
            reservoir_key = key;
            reservoir_value = value;
        }

        // Step 2: checking whether current_reservoir_samples reaches maximum value
        if (current_reservoir_samples == (1 << reservoir_level)) {
            // output reservoir_value to higher level
            current_reservoir_samples = 0;
            array[0].push_back({reservoir_key, reservoir_value});
        }

        // Step 3: handling compressing operation in higher level
        for (int i = 0; i < H - reservoir_level - 1; ++i) {
            while (array[i].size() >= max_size[i]) {
                // group-by and sort operation
                std::vector<std::pair<ID_TYPE, DATA_TYPE>> grouped_array;
                std::unordered_map<ID_TYPE, std::vector<DATA_TYPE>> kv_array;
                std::vector<ID_TYPE> key_order;
                for (int j = 0; j < max_size[i]; ++j) {
                    ID_TYPE current_key = array[i][j].first;
                    if (kv_array.find(current_key) == kv_array.end()) {
                        key_order.push_back(current_key);
                    }
                    kv_array[current_key].push_back(array[i][j].second);
                }
                for (auto _key : key_order) {
                    auto vec = kv_array[_key];
                    std::sort(vec.begin(), vec.begin() + vec.size());
                    for (auto _value : vec) {
                        grouped_array.push_back({_key, _value});
                    }
                }
                double r = static_cast<double>(std::rand()) / RAND_MAX;
                int random_bit = r < 0.5 ? 0 : 1;
                for (int j = random_bit; j < max_size[i]; j += 2) {
                    array[i + 1].push_back(grouped_array[j]);
                }
                array[i].erase(array[i].begin(), array[i].begin() + max_size[i]);
            }
        }
    }

    int query_rank(ID_TYPE key, DATA_TYPE value) {
        int rank = 0;
        if (reservoir_key == key && reservoir_value <= value) {
            rank += current_reservoir_samples;
        }
        for (int i = 0; i < H - reservoir_level; ++i) {
            for (auto item : array[i]) {
                rank += ((item.first == key) && (item.second <= value)) * (1 << (i + reservoir_level));
            }
        }
        return rank;
    }

    int query_frequency(ID_TYPE key) {
        int frequency = 0;
        if (reservoir_key == key) {
            frequency += current_reservoir_samples;
        }
        for (int i = 0; i < H - reservoir_level; ++i) {
            for (auto item : array[i]) {
                frequency += (item.first == key) * (1 << (i + reservoir_level));
            }
        }
        return frequency;
    }

    double query_quantile(ID_TYPE key, DATA_TYPE value) {
        int rank = query_rank(key, value), frequency = query_frequency(key);
        return 1.0 * rank / frequency;
    }

    DATA_TYPE query_value(ID_TYPE key, double w) {
        int frequency = 0;
        std::vector<std::pair<DATA_TYPE, int>> value_weight_vec;
        if (reservoir_key == key) {
            value_weight_vec.push_back({reservoir_value, current_reservoir_samples});
            frequency += current_reservoir_samples;
        }
        for (int i = 0; i < H - reservoir_level; ++i) {
            for (auto item : array[i]) {
                if (item.first == key) {
                    value_weight_vec.push_back({item.second, (1 << (i + reservoir_level))});
                    frequency += (1 << (i + reservoir_level));
                }
            }
        }
        if (frequency == 0) {
            return 0;
        }

        // assert(frequency == query_frequency(key));
        std::sort(value_weight_vec.begin(), value_weight_vec.end(), 
                  [](auto const& x, auto const& y) {
                    return x.first < y.first;
                  });

        double target = w * frequency;
        double current = 0;

        for (int i = 0; i < value_weight_vec.size(); ++i) {
            current += value_weight_vec[i].second;
            if (current >= target) {
                return value_weight_vec[i].first;
            }
        }
        return value_weight_vec.back().first;
    }

    double calculate_memory() {
        std::cout << "Height: " << H << "\n";
        std::cout << "Revervoir height: " << reservoir_level << "\n";
        std::cout << "Fixed height: " << s << "\n";
        double memory = 0;
        for (int i = 0; i < reservoir_level; ++i) {
            std::cout << 2 << " ";
        }
        for (int i = 0; i < H - reservoir_level; ++i) {
            memory += max_size[i] * (sizeof(ID_TYPE) + sizeof(DATA_TYPE));
            std::cout << max_size[i] << " ";
        }
        std::cout << "\n";
        return 1.0 * memory / 1024;
    }
    void calculate_reduced_weight_sum() {
        std::cout << get_reduced_weight_sum() << "\n";
    }
    long long get_reduced_weight_sum() {
        long long sum_weight = current_reservoir_samples;
        for (int i = 0; i < H - reservoir_level; ++i) {
            sum_weight += array[i].size() *
                          (1LL << (reservoir_level + i));
        }
        return N - sum_weight;
    }
protected:
    double c;
    int l;
    int H;
    int N;
    int s;
    std::vector<std::vector<std::pair<ID_TYPE, DATA_TYPE>>> array;
    std::vector<int> max_size;
    int reservoir_level = 0;
    ID_TYPE reservoir_key;
    DATA_TYPE reservoir_value;
    int current_reservoir_samples = 0;
};


template<typename ID_TYPE, typename DATA_TYPE>
class KLL_Polymer_DC : protected KLL_Polymer<ID_TYPE, DATA_TYPE> {
    using KLL_Polymer<ID_TYPE, DATA_TYPE>::c;
    using KLL_Polymer<ID_TYPE, DATA_TYPE>::l;
    using KLL_Polymer<ID_TYPE, DATA_TYPE>::H;
    using KLL_Polymer<ID_TYPE, DATA_TYPE>::N;
    using KLL_Polymer<ID_TYPE, DATA_TYPE>::s;
    using KLL_Polymer<ID_TYPE, DATA_TYPE>::array;
    using KLL_Polymer<ID_TYPE, DATA_TYPE>::max_size;
    using KLL_Polymer<ID_TYPE, DATA_TYPE>::reservoir_level;
    using KLL_Polymer<ID_TYPE, DATA_TYPE>::reservoir_key;
    using KLL_Polymer<ID_TYPE, DATA_TYPE>::reservoir_value;
    using KLL_Polymer<ID_TYPE, DATA_TYPE>::current_reservoir_samples;
public:
    using KLL_Polymer<ID_TYPE, DATA_TYPE>::query_rank;
    using KLL_Polymer<ID_TYPE, DATA_TYPE>::query_value;
    using KLL_Polymer<ID_TYPE, DATA_TYPE>::query_frequency;
    using KLL_Polymer<ID_TYPE, DATA_TYPE>::query_quantile;
    using KLL_Polymer<ID_TYPE, DATA_TYPE>::calculate_memory;
    using KLL_Polymer<ID_TYPE, DATA_TYPE>::calculate_reduced_weight_sum;
    using KLL_Polymer<ID_TYPE, DATA_TYPE>::get_reduced_weight_sum;
    KLL_Polymer_DC(int _N, double _c, int _s, int memory) {
        /*
        N: length of the data stream
        c: decreasing rate of capacity
        s: number of levels with fixed capacity
        memory: memory of KLL-Polymer
        */
        N = _N;
        c = _c;
        s = _s;
        int capacity_sum = memory * 1024 / (sizeof(ID_TYPE) + sizeof(DATA_TYPE));
        l = capacity_sum / (s + pow(c, s) / (1 - c));
        H = log2(1.0 * N / l) + 1;
        assert(s < H);
        for (int i = 0; i < H - s; ++i) {
            int capacity = std::max(
                (int)ceil(1.0 * l * pow(c, H - 1 - i)), 2);
            if (capacity <= 2) {
                reservoir_level = i + 1;
            }
            else {
                std::vector<std::pair<ID_TYPE, DATA_TYPE>> buffer;
                array.push_back(buffer);
                max_size.push_back(capacity);
            }
        }
        for (int i = H - s; i < H; ++i) {
            std::vector<std::pair<ID_TYPE, DATA_TYPE>> buffer;
            array.push_back(buffer);
            max_size.push_back(l);
        }
        assert(array.size() + reservoir_level == H);
    }

    ~KLL_Polymer_DC() {}

    void insert(ID_TYPE key, DATA_TYPE value) {
        // Step 1: in lower level, handling sampled values
        current_reservoir_samples++;
        double r = static_cast<double>(std::rand()) / RAND_MAX;
        if (r <= 1.0 / current_reservoir_samples) {
            reservoir_key = key;
            reservoir_value = value;
        }

        // Step 2: checking whether current_reservoir_samples reaches maximum value
        if (current_reservoir_samples == (1 << reservoir_level)) {
            // output reservoir_value to higher level
            current_reservoir_samples = 0;
            array[0].push_back({reservoir_key, reservoir_value});
        }

        // Step 3: handling compressing operation in higher level
        // lower level, using randomized compaction
        for (int i = 0; i < H - reservoir_level - s; ++i) {
            while (array[i].size() >= max_size[i]) {
                // group-by and sort operation
                std::vector<std::pair<ID_TYPE, DATA_TYPE>> grouped_array;
                std::unordered_map<ID_TYPE, std::vector<DATA_TYPE>> kv_array;
                std::vector<ID_TYPE> key_order;
                for (int j = 0; j < max_size[i]; ++j) {
                    ID_TYPE current_key = array[i][j].first;
                    if (kv_array.find(current_key) == kv_array.end()) {
                        key_order.push_back(current_key);
                    }
                    kv_array[current_key].push_back(array[i][j].second);
                }
                for (auto _key : key_order) {
                    auto vec = kv_array[_key];
                    std::sort(vec.begin(), vec.begin() + vec.size());
                    for (auto _value : vec) {
                        grouped_array.push_back({_key, _value});
                    }
                }
                double r = static_cast<double>(std::rand()) / RAND_MAX;
                int random_bit = r < 0.5 ? 0 : 1;
                for (int j = random_bit; j < max_size[i]; j += 2) {
                    array[i + 1].push_back(grouped_array[j]);
                }
                array[i].erase(array[i].begin(), array[i].begin() + max_size[i]);
            }
        }

        // top level, using deterministic compaction
        for (int i = H - reservoir_level - s; i < H - reservoir_level - 1; ++i) {
            while (array[i].size() >= max_size[i]) {
                std::unordered_map<ID_TYPE, std::vector<DATA_TYPE>> kv_array;
                std::vector<ID_TYPE> key_order;
                int promoted_items = 0;
                for (int j = 0; j < max_size[i]; ++j) {
                    ID_TYPE current_key = array[i][j].first;
                    if (kv_array.find(current_key) == kv_array.end()) {
                        key_order.push_back(current_key);
                    }
                    kv_array[current_key].push_back(array[i][j].second);
                }
                for (auto _key : key_order) {
                    auto vec = kv_array[_key];
                    std::sort(vec.begin(), vec.begin() + vec.size());
                    double r = static_cast<double>(std::rand()) / RAND_MAX;
                    int random_bit = r < 0.5 ? 0 : 1;
                    for (int j = random_bit; j < vec.size() + random_bit - 1; j += 2) {
                        array[i + 1].push_back({_key, vec[j]});
                        promoted_items++;
                    }
                }
                array[i].erase(array[i].begin(), array[i].begin() + max_size[i]);
            }
        }
    }    
};



template<typename ID_TYPE, typename DATA_TYPE>
class KLL_Polymer_No_Opt {
public:
    KLL_Polymer_No_Opt() {}
    KLL_Polymer_No_Opt(int _N, double _c, int memory) {
        initialize(_N, _c, memory, false);
    }

    ~KLL_Polymer_No_Opt() {}

    void insert(ID_TYPE key, DATA_TYPE value) {
        // Step 1: in lower level, handling sampled values
        current_reservoir_samples++;
        double r = static_cast<double>(std::rand()) / RAND_MAX;
        if (r <= 1.0 / current_reservoir_samples) {
            reservoir_key = key;
            reservoir_value = value;
        }

        // Step 2: checking whether current_reservoir_samples reaches maximum value
        if (current_reservoir_samples == calculate_weight(reservoir_level) &&
            array.size() > 0) {
            current_reservoir_samples = 0;
            array[0].push_back({reservoir_key, reservoir_value});
        }

        // Step 3: handling compressing operation in higher level
        for (int i = 0; i < H - reservoir_level - 1; ++i) {
            while (array[i].size() >= max_size[i]) {
                std::vector<std::pair<ID_TYPE, DATA_TYPE>> grouped_array;
                std::unordered_map<ID_TYPE, std::vector<DATA_TYPE>> kv_array;
                std::vector<ID_TYPE> key_order;
                for (int j = 0; j < max_size[i]; ++j) {
                    ID_TYPE current_key = array[i][j].first;
                    if (kv_array.find(current_key) == kv_array.end()) {
                        key_order.push_back(current_key);
                    }
                    kv_array[current_key].push_back(array[i][j].second);
                }
                for (auto _key : key_order) {
                    auto vec = kv_array[_key];
                    std::sort(vec.begin(), vec.begin() + vec.size());
                    for (auto _value : vec) {
                        grouped_array.push_back({_key, _value});
                    }
                }
                double r = static_cast<double>(std::rand()) / RAND_MAX;
                int random_bit = r < 0.5 ? 0 : 1;
                for (int j = random_bit; j < max_size[i]; j += 2) {
                    array[i + 1].push_back(grouped_array[j]);
                }
                array[i].erase(array[i].begin(),
                               array[i].begin() + max_size[i]);
            }
        }
    }

    int query_rank(ID_TYPE key, DATA_TYPE value) {
        long long rank = 0;
        if (current_reservoir_samples > 0 &&
            reservoir_key == key && reservoir_value <= value) {
            rank += current_reservoir_samples;
        }
        for (int i = 0; i < H - reservoir_level; ++i) {
            for (auto item : array[i]) {
                if (item.first == key && item.second <= value) {
                    rank += calculate_weight(i + reservoir_level);
                }
            }
        }
        return rank;
    }

    int query_frequency(ID_TYPE key) {
        long long frequency = 0;
        if (current_reservoir_samples > 0 && reservoir_key == key) {
            frequency += current_reservoir_samples;
        }
        for (int i = 0; i < H - reservoir_level; ++i) {
            for (auto item : array[i]) {
                if (item.first == key) {
                    frequency += calculate_weight(i + reservoir_level);
                }
            }
        }
        return frequency;
    }

    double query_quantile(ID_TYPE key, DATA_TYPE value) {
        int rank = query_rank(key, value), frequency = query_frequency(key);
        if (frequency == 0) {
            return 0;
        }
        return 1.0 * rank / frequency;
    }

    DATA_TYPE query_value(ID_TYPE key, double w) {
        long long frequency = 0;
        std::vector<std::pair<DATA_TYPE, long long>> value_weight_vec;
        if (current_reservoir_samples > 0 && reservoir_key == key) {
            value_weight_vec.push_back({reservoir_value,
                                        current_reservoir_samples});
            frequency += current_reservoir_samples;
        }
        for (int i = 0; i < H - reservoir_level; ++i) {
            for (auto item : array[i]) {
                if (item.first == key) {
                    long long weight = calculate_weight(i + reservoir_level);
                    value_weight_vec.push_back({item.second, weight});
                    frequency += weight;
                }
            }
        }
        if (value_weight_vec.size() == 0) {
            return DATA_TYPE();
        }
        std::sort(value_weight_vec.begin(), value_weight_vec.end());
        double target = w * frequency;
        long long current = 0;
        for (int i = 0; i < value_weight_vec.size(); ++i) {
            current += value_weight_vec[i].second;
            if (current >= target) {
                return value_weight_vec[i].first;
            }
        }
        return value_weight_vec.back().first;
    }

    double calculate_memory() {
        std::cout << "Height: " << H << "\n";
        std::cout << "Revervoir height: " << reservoir_level << "\n";
        std::cout << "Top capacity: " << l << "\n";
        for (int i = 0; i < reservoir_level; ++i) {
            std::cout << 2 << " ";
        }
        for (int i = 0; i < max_size.size(); ++i) {
            std::cout << max_size[i] << " ";
        }
        std::cout << "\n";
        return 1.0 * reserved_memory_bytes() / 1024;
    }

    void calculate_reduced_weight_sum() {
        std::cout << get_reduced_weight_sum() << "\n";
    }

    long long get_reduced_weight_sum() {
        long long sum_weight = current_reservoir_samples;
        for (int i = 0; i < H - reservoir_level; ++i) {
            sum_weight += array[i].size() *
                          calculate_weight(reservoir_level + i);
        }
        return N - sum_weight;
    }

    long long reserved_memory_bytes() {
        long long slots = reservoir_level > 0 ? 1 : 0;
        for (int i = 0; i < max_size.size(); ++i) {
            slots += max_size[i];
        }
        return slots * (sizeof(ID_TYPE) + sizeof(DATA_TYPE));
    }

    long long actual_memory_bytes() {
        long long slots = current_reservoir_samples > 0 ? 1 : 0;
        for (int i = 0; i < array.size(); ++i) {
            slots += array[i].size();
        }
        return slots * (sizeof(ID_TYPE) + sizeof(DATA_TYPE));
    }

    int height() {
        return H;
    }

    int reservoir_levels() {
        return reservoir_level;
    }

    int top_capacity() {
        return l;
    }

    std::vector<int> capacities() {
        return max_size;
    }

    bool within_level_capacities() {
        for (int i = 0; i < array.size(); ++i) {
            if (array[i].size() > max_size[i]) {
                return false;
            }
        }
        return true;
    }

protected:
    void initialize(int _N, double _c, int memory, bool use_sampler) {
        N = _N;
        c = _c;
        reservoir_level = 0;
        current_reservoir_samples = 0;
        array.clear();
        max_size.clear();
        assert(N > 0);
        assert(c > 0.5 && c < 1);

        int capacity_sum = memory * 1024 /
                           (sizeof(ID_TYPE) + sizeof(DATA_TYPE));
        l = capacity_sum / (1 + c / (1 - c));
        H = log2(1.0 * N / l) + 2;

        for (int i = 0; i < H; ++i) {
            int capacity = calculate_level_capacity(l, H - 1 - i);
            if (use_sampler && capacity <= 2) {
                reservoir_level = i + 1;
            }
            else {
                std::vector<std::pair<ID_TYPE, DATA_TYPE>> buffer;
                array.push_back(buffer);
                max_size.push_back(capacity);
            }
        }
        assert(array.size() + reservoir_level == H);
    }

    int calculate_level_capacity(int _l, int exponent) {
        return std::max((int)ceil(1.0 * _l * pow(c, exponent)), 2);
    }

    long long calculate_weight(int level) {
        long long weight = 1;
        for (int i = 0; i < level; ++i) {
            weight *= 2;
        }
        return weight;
    }

    double c;
    int l;
    int H;
    int N;
    std::vector<std::vector<std::pair<ID_TYPE, DATA_TYPE>>> array;
    std::vector<int> max_size;
    int reservoir_level = 0;
    ID_TYPE reservoir_key;
    DATA_TYPE reservoir_value;
    long long current_reservoir_samples = 0;
};


template<typename ID_TYPE, typename DATA_TYPE>
class KLL_Polymer_Opt1 : protected KLL_Polymer_No_Opt<ID_TYPE, DATA_TYPE> {
    using KLL_Polymer_No_Opt<ID_TYPE, DATA_TYPE>::initialize;
public:
    using KLL_Polymer_No_Opt<ID_TYPE, DATA_TYPE>::insert;
    using KLL_Polymer_No_Opt<ID_TYPE, DATA_TYPE>::query_rank;
    using KLL_Polymer_No_Opt<ID_TYPE, DATA_TYPE>::query_value;
    using KLL_Polymer_No_Opt<ID_TYPE, DATA_TYPE>::query_frequency;
    using KLL_Polymer_No_Opt<ID_TYPE, DATA_TYPE>::query_quantile;
    using KLL_Polymer_No_Opt<ID_TYPE, DATA_TYPE>::calculate_memory;
    using KLL_Polymer_No_Opt<ID_TYPE, DATA_TYPE>::get_reduced_weight_sum;
    using KLL_Polymer_No_Opt<ID_TYPE, DATA_TYPE>::calculate_reduced_weight_sum;
    using KLL_Polymer_No_Opt<ID_TYPE, DATA_TYPE>::reserved_memory_bytes;
    using KLL_Polymer_No_Opt<ID_TYPE, DATA_TYPE>::actual_memory_bytes;
    using KLL_Polymer_No_Opt<ID_TYPE, DATA_TYPE>::height;
    using KLL_Polymer_No_Opt<ID_TYPE, DATA_TYPE>::reservoir_levels;
    using KLL_Polymer_No_Opt<ID_TYPE, DATA_TYPE>::top_capacity;
    using KLL_Polymer_No_Opt<ID_TYPE, DATA_TYPE>::capacities;
    using KLL_Polymer_No_Opt<ID_TYPE, DATA_TYPE>::within_level_capacities;

    KLL_Polymer_Opt1(int _N, double _c, int memory) {
        initialize(_N, _c, memory, true);
    }

    ~KLL_Polymer_Opt1() {}
};


template<typename DATA_TYPE>
class GK_Summary {
public:
    struct Entry {
        DATA_TYPE value;
        int key;
        int g;
        int delta;
        int w;
    };

    GK_Summary() {}

    void initialize(int _max_size, double _epsilon) {
        max_size = std::max(_max_size, 2);
        epsilon = _epsilon;
        N = 0;
        current_insert = 0;
        has_item = false;
        array.clear();
        pending.clear();
        prefix_valid = false;
    }


    void insert(long long key, DATA_TYPE value) {
        assert(key >= INT_MIN && key <= INT_MAX);
        N++;
        long long allowance = calculate_allowance();
        Entry item;
        item.key = key;
        item.value = value;
        item.g = 1;
        item.w = 1;
        if (!has_item ||
            key_less(key, value, minimum_key, minimum_value) ||
            !key_less(key, value, maximum_key, maximum_value) ||
            allowance == 0) {
            item.delta = 0;
        }
        else {
            item.delta = allowance;
        }
        pending.push_back(item);
        prefix_valid = false;

        if (!has_item ||
            key_less(key, value, minimum_key, minimum_value)) {
            minimum_key = key;
            minimum_value = value;
        }
        if (!has_item ||
            !key_less(key, value, maximum_key, maximum_value)) {
            maximum_key = key;
            maximum_value = value;
        }
        has_item = true;

        current_insert++;
        if (current_insert >= calculate_interval()) {
            compress();
            current_insert = 0;
        }
        keep_memory_bound();
    }

    long long rank_before_key(long long key) {
        flush_pending();
        int left = 0, right = array.size();
        while (left < right) {
            int middle = left + (right - left) / 2;
            if (array[middle].key < key) {
                left = middle + 1;
            }
            else {
                right = middle;
            }
        }
        long long rank = estimate_rank(left);
        save_key_prefix();
        return rank;
    }

    long long rank_through_key(long long key) {
        flush_pending();
        int left = 0, right = array.size();
        while (left < right) {
            int middle = left + (right - left) / 2;
            if (array[middle].key <= key) {
                left = middle + 1;
            }
            else {
                right = middle;
            }
        }
        return estimate_rank(left);
    }

    long long rank_at_most(long long key, DATA_TYPE value) {
        flush_pending();
        int left = 0, right = array.size();
        while (left < right) {
            int middle = left + (right - left) / 2;
            if (array[middle].key <= key) {
                if (array[middle].key < key ||
                    array[middle].value <= value) {
                    left = middle + 1;
                }
                else {
                    right = middle;
                }
            }
            else {
                right = middle;
            }
        }
        return estimate_rank(left);
    }

    std::vector<DATA_TYPE> query_values(long long key) {
        flush_pending();
        std::vector<DATA_TYPE> values;
        int left = 0, right = array.size();
        while (left < right) {
            int middle = left + (right - left) / 2;
            if (array[middle].key < key) {
                left = middle + 1;
            }
            else {
                right = middle;
            }
        }
        for (int i = left; i < array.size(); ++i) {
            if (array[i].key != key) {
                break;
            }
            values.push_back(array[i].value);
        }
        return values;
    }

    std::vector<std::pair<DATA_TYPE, long long>> query_items(long long key) {
        flush_pending();
        std::vector<std::pair<DATA_TYPE, long long>> items;
        int left = 0, right = array.size();
        while (left < right) {
            int middle = left + (right - left) / 2;
            if (array[middle].key < key) {
                left = middle + 1;
            }
            else {
                right = middle;
            }
        }
        long long last_rank = estimate_rank(left);
        save_key_prefix();
        for (int i = left; i < array.size(); ++i) {
            if (array[i].key != key) {
                break;
            }
            long long current_rank = estimate_rank(i + 1);
            items.push_back({array[i].value,
                             current_rank - last_rank});
            last_rank = current_rank;
        }
        return items;
    }

    long long get_N() {
        return N;
    }

    int get_size() {
        flush_pending();
        return array.size();
    }

    int get_max_size() {
        return max_size;
    }

    double get_epsilon() {
        return epsilon;
    }

    int entry_bytes() {
        return sizeof(Entry);
    }

    int prefix_memory_bytes() {
        return 2 * sizeof(int) + 4 * sizeof(long long);
    }

private:
    bool key_less(long long key1, DATA_TYPE value1,
                  long long key2, DATA_TYPE value2) {
        if (key1 != key2) {
            return key1 < key2;
        }
        return value1 < value2;
    }

    long long calculate_allowance() {
        return floor(2.0 * epsilon * N);
    }

    long long calculate_interval() {
        long long interval = floor(1.0 / (2.0 * epsilon));
        return std::max(interval, 1LL);
    }

    void flush_pending() {
        if (pending.size() == 0) {
            return;
        }
        std::vector<std::pair<std::pair<long long, DATA_TYPE>, int>>
            pending_order;
        for (int i = 0; i < pending.size(); ++i) {
            pending_order.push_back(
                {{pending[i].key, pending[i].value}, i});
        }
        std::sort(pending_order.begin(), pending_order.end());
        std::vector<Entry> sorted_pending;
        for (auto item : pending_order) {
            sorted_pending.push_back(pending[item.second]);
        }
        pending.swap(sorted_pending);

        std::vector<Entry> merged_array;
        int left = 0, right = 0;
        while (left < array.size() && right < pending.size()) {
            if (key_less(pending[right].key, pending[right].value,
                         array[left].key, array[left].value)) {
                pending[right].delta = std::min(pending[right].delta,
                    array[left].g + array[left].delta - array[left].w);
                merged_array.push_back(pending[right]);
                right++;
            }
            else {
                merged_array.push_back(array[left]);
                left++;
            }
        }
        while (left < array.size()) {
            merged_array.push_back(array[left]);
            left++;
        }
        while (right < pending.size()) {
            pending[right].delta = 0;
            merged_array.push_back(pending[right]);
            right++;
        }
        std::vector<Entry> combined_array;
        for (auto item : merged_array) {
            if (combined_array.size() > 0 &&
                combined_array.back().key == item.key &&
                combined_array.back().value == item.value) {
                item.g += combined_array.back().g;
                item.w += combined_array.back().w;
                item.delta = std::min(item.delta,
                                      combined_array.back().delta);
                combined_array.back() = item;
            }
            else {
                combined_array.push_back(item);
            }
        }
        array.swap(combined_array);
        pending.clear();
        prefix_valid = false;
    }

    void compress() {
        flush_pending();
        if (array.size() < 3) {
            return;
        }
        long long allowance = calculate_allowance();
        std::vector<Entry> compressed_array;
        compressed_array.push_back(array.back());
        for (int i = array.size() - 2; i > 0; --i) {
            int right = compressed_array.size() - 1;
            if (array[i].g + compressed_array[right].g +
                compressed_array[right].delta -
                compressed_array[right].w <= allowance) {
                compressed_array[right].g += array[i].g;
            }
            else {
                compressed_array.push_back(array[i]);
            }
        }
        compressed_array.push_back(array[0]);
        std::reverse(compressed_array.begin(), compressed_array.end());
        array.swap(compressed_array);
        prefix_valid = false;
    }

    void keep_memory_bound() {
        if (array.size() + pending.size() <= max_size) {
            return;
        }
        compress();
        while (array.size() > max_size && epsilon < 0.5) {
            long long allowance = calculate_allowance();
            epsilon = 1.0 * (allowance + 1) / (2 * N);
            if (calculate_allowance() <= allowance) {
                epsilon = nextafter(epsilon, 0.5);
            }
            compress();
        }
        while (array.size() > max_size) {
            array[2].g += array[1].g;
            array.erase(array.begin() + 1);
            prefix_valid = false;
        }
    }

    void build_prefix_sum() {
        if (prefix_valid) {
            return;
        }
        prefix_position = 0;
        key_prefix_position = 0;
        prefix_rank = 0;
        key_prefix_rank = 0;
        prefix_estimate = 0;
        key_prefix_estimate = 0;
        prefix_valid = true;
    }

    void save_key_prefix() {
        key_prefix_position = prefix_position;
        key_prefix_rank = prefix_rank;
        key_prefix_estimate = prefix_estimate;
    }

    long long estimate_rank(int position) {
        build_prefix_sum();
        if (position < prefix_position) {
            if (position >= key_prefix_position) {
                prefix_position = key_prefix_position;
                prefix_rank = key_prefix_rank;
                prefix_estimate = key_prefix_estimate;
            }
            else {
                prefix_position = 0;
                prefix_rank = 0;
                prefix_estimate = 0;
                key_prefix_position = 0;
                key_prefix_rank = 0;
                key_prefix_estimate = 0;
            }
        }
        while (prefix_position < position) {
            prefix_rank += array[prefix_position].g;
            prefix_position++;
            long long current_rank = prefix_rank;
            if (prefix_position < array.size()) {
                current_rank +=
                    (1LL * array[prefix_position].g +
                     array[prefix_position].delta -
                     array[prefix_position].w) / 2;
            }
            else {
                current_rank = N;
            }
            prefix_estimate = std::max(prefix_estimate, current_rank);
            prefix_estimate = std::min(prefix_estimate, N);
        }
        return prefix_estimate;
    }

    int max_size;
    double epsilon;
    long long N;
    long long current_insert;
    long long minimum_key;
    long long maximum_key;
    DATA_TYPE minimum_value;
    DATA_TYPE maximum_value;
    std::vector<Entry> array;
    std::vector<Entry> pending;
    int prefix_position;
    int key_prefix_position;
    long long prefix_rank;
    long long key_prefix_rank;
    long long prefix_estimate;
    long long key_prefix_estimate;
    bool has_item;
    bool prefix_valid;
};


template<typename ID_TYPE, typename DATA_TYPE>
class KLL_Polymer_Opt3 {
public:
    KLL_Polymer_Opt3(int _N, double _c, int _s, int memory, double _theta,
                   double _gk_ratio = 2.0, double _mg_ratio = 2.0) {
        /*
        N: length of the data stream
        c: decreasing rate of capacity
        s: number of top levels replaced by MG-GK
        memory: memory of KLL-Polymer
        theta: threshold of heavy hitters
        */
        N = _N;
        c = _c;
        s = _s;
        theta = _theta;
        gk_ratio = _gk_ratio;
        mg_ratio = _mg_ratio;
        reservoir_level = 0;
        current_reservoir_samples = 0;
        next_key = 1;
        assert(N > 0);
        assert(c > 0.5 && c < 1);
        assert(s > 0);
        assert(theta > 0 && theta < 1);
        assert(gk_ratio > 0);
        assert(mg_ratio >= 1);

        int memory_bytes = memory * 1024;
        int capacity_sum = memory_bytes /
                           (sizeof(ID_TYPE) + sizeof(DATA_TYPE));
        int fixed_l = capacity_sum /
                      (s + pow(c, s) / (1 - c));
        int fixed_H = log2(1.0 * N / fixed_l) + 2;
        H = fixed_H - 2;
        assert(H >= s + 1);

        int low = 1;
        int high = N;
        assert(calculate_memory_bytes(low, H) <= memory_bytes);
        while (low < high) {
            int middle = low + (high - low + 1) / 2;
            if (calculate_memory_bytes(middle, H) <= memory_bytes) {
                low = middle;
            }
            else {
                high = middle - 1;
            }
        }
        l = low;

        lower_levels = H - s - 1;
        for (int i = 0; i < lower_levels; ++i) {
            int capacity = calculate_level_capacity(l, H - 1 - i);
            if (capacity <= 2) {
                reservoir_level = i + 1;
            }
            else {
                std::vector<std::pair<ID_TYPE, DATA_TYPE>> buffer;
                array.push_back(buffer);
                max_size.push_back(capacity);
            }
        }
        assert(array.size() + reservoir_level == lower_levels);

        B = ceil(mg_ratio / theta);
        gk_max_size = ceil(gk_ratio * l);
        gk.initialize(gk_max_size, 1.0 / (5 * gk_max_size));
        top_weight = calculate_weight(lower_levels);
        memory_size = calculate_memory_bytes(l, H);
    }

    ~KLL_Polymer_Opt3() {}

    void insert(ID_TYPE key, DATA_TYPE value) {
        if (lower_levels == 0) {
            insert_top(key, value);
            return;
        }

        // Step 1: in lower level, handling sampled values
        current_reservoir_samples++;
        double r = static_cast<double>(std::rand()) / RAND_MAX;
        if (r <= 1.0 / current_reservoir_samples) {
            reservoir_key = key;
            reservoir_value = value;
        }

        // Step 2: output the sampled item
        if (current_reservoir_samples == calculate_weight(reservoir_level)) {
            current_reservoir_samples = 0;
            if (array.size() == 0) {
                insert_top(reservoir_key, reservoir_value);
            }
            else {
                array[0].push_back({reservoir_key, reservoir_value});
            }
        }

        // Step 3: handling compacting operation in lower levels
        for (int i = 0; i < array.size(); ++i) {
            while (array[i].size() >= max_size[i]) {
                int compacted_size = max_size[i] - max_size[i] % 2;
                std::vector<std::pair<ID_TYPE, DATA_TYPE>> grouped_array;
                std::unordered_map<ID_TYPE, std::vector<DATA_TYPE>> kv_array;
                std::vector<ID_TYPE> key_order;
                for (int j = 0; j < compacted_size; ++j) {
                    ID_TYPE current_key = array[i][j].first;
                    if (kv_array.find(current_key) == kv_array.end()) {
                        key_order.push_back(current_key);
                    }
                    kv_array[current_key].push_back(array[i][j].second);
                }
                for (auto _key : key_order) {
                    auto vec = kv_array[_key];
                    std::sort(vec.begin(), vec.begin() + vec.size());
                    for (auto _value : vec) {
                        grouped_array.push_back({_key, _value});
                    }
                }

                double r = static_cast<double>(std::rand()) / RAND_MAX;
                int random_bit = r < 0.5 ? 0 : 1;
                std::vector<std::pair<ID_TYPE, DATA_TYPE>> promoted_array;
                for (int j = random_bit; j < compacted_size; j += 2) {
                    promoted_array.push_back(grouped_array[j]);
                }
                array[i].erase(array[i].begin(),
                               array[i].begin() + compacted_size);

                if (i + 1 < array.size()) {
                    for (auto item : promoted_array) {
                        array[i + 1].push_back(item);
                    }
                }
                else {
                    for (auto item : promoted_array) {
                        insert_top(item.first, item.second);
                    }
                }
            }
        }
    }

    int query_rank(ID_TYPE key, DATA_TYPE value) {
        long long rank = query_lower_rank(key, value);
        auto position = mg.find(key);
        if (position != mg.end()) {
            long long new_key = position->second.second;
            long long before = gk.rank_before_key(new_key);
            long long current = gk.rank_at_most(new_key, value);
            rank += (current - before) * top_weight;
        }
        return rank;
    }

    int query_frequency(ID_TYPE key) {
        long long frequency = query_lower_frequency(key);
        auto position = mg.find(key);
        if (position != mg.end()) {
            long long new_key = position->second.second;
            long long before = gk.rank_before_key(new_key);
            long long current = gk.rank_through_key(new_key);
            frequency += (current - before) * top_weight;
        }
        return frequency;
    }

    double query_quantile(ID_TYPE key, DATA_TYPE value) {
        int rank = query_rank(key, value), frequency = query_frequency(key);
        if (frequency == 0) {
            return 0;
        }
        return 1.0 * rank / frequency;
    }

    DATA_TYPE query_value(ID_TYPE key, double w) {
        std::vector<std::pair<DATA_TYPE, long long>> values;
        if (current_reservoir_samples > 0 && reservoir_key == key) {
            values.push_back({reservoir_value, current_reservoir_samples});
        }
        for (int i = 0; i < array.size(); ++i) {
            for (auto item : array[i]) {
                if (item.first == key) {
                    values.push_back({item.second,
                                      calculate_weight(reservoir_level + i)});
                }
            }
        }
        auto position = mg.find(key);
        if (position != mg.end()) {
            std::vector<std::pair<DATA_TYPE, long long>> top_values =
                gk.query_items(position->second.second);
            for (auto item : top_values) {
                values.push_back({item.first, item.second * top_weight});
            }
        }
        if (values.size() == 0) {
            return DATA_TYPE();
        }
        std::sort(values.begin(), values.end());
        long long frequency = 0;
        for (auto item : values) {
            frequency += item.second;
        }
        double target = w * frequency;
        long long rank = 0;
        for (auto item : values) {
            rank += item.second;
            if (rank >= target) {
                return item.first;
            }
        }
        return values.back().first;
    }

    double calculate_memory() {
        std::cout << "Height: " << H << "\n";
        std::cout << "Revervoir height: " << reservoir_level << "\n";
        std::cout << "MG slots: " << B << "\n";
        std::cout << "GK entries: " << gk_max_size << "\n";
        std::cout << "GK epsilon: " << gk.get_epsilon() << "\n";
        return 1.0 * memory_size / 1024;
    }

    void calculate_reduced_weight_sum() {
        std::cout << get_reduced_weight_sum() << "\n";
    }

    long long get_reduced_weight_sum() {
        long long sum_weight = current_reservoir_samples;
        for (int i = 0; i < array.size(); ++i) {
            sum_weight += array[i].size() *
                          calculate_weight(reservoir_level + i);
        }
        sum_weight += gk.get_N() * top_weight;
        return N - sum_weight;
    }

    long long reserved_memory_bytes() {
        return memory_size;
    }

    int top_capacity_parameter() {
        return l;
    }

    int mg_capacity() {
        return B;
    }

    int gk_capacity() {
        return gk_max_size;
    }

    int gk_size() {
        return gk.get_size();
    }

    double gk_epsilon() {
        return gk.get_epsilon();
    }

    long long top_level_weight() {
        return top_weight;
    }

    int height() {
        return H;
    }

    int reservoir_levels() {
        return reservoir_level;
    }

    std::vector<int> lower_capacities() {
        return max_size;
    }

    bool within_level_capacities() {
        for (int i = 0; i < array.size(); ++i) {
            if (array[i].size() > max_size[i]) {
                return false;
            }
        }
        if (mg.size() > B || gk.get_size() > gk_max_size) {
            return false;
        }
        return true;
    }

    int mg_entry_bytes() {
        return sizeof(ID_TYPE) + 2 * sizeof(long long);
    }

    int gk_entry_bytes() {
        return gk.entry_bytes();
    }

private:
    int calculate_level_capacity(int _l, int exponent) {
        return std::max((int)ceil(1.0 * _l * pow(c, exponent)), 2);
    }

    long long calculate_memory_bytes(int _l, int height) {
        int current_lower_levels = height - s - 1;
        long long lower_slots = 0;
        bool has_sampler = false;
        for (int i = 0; i < current_lower_levels; ++i) {
            int capacity = calculate_level_capacity(_l, height - 1 - i);
            if (capacity <= 2) {
                has_sampler = true;
            }
            else {
                lower_slots += capacity;
            }
        }
        if (has_sampler) {
            lower_slots++;
        }

        long long mg_slots = ceil(mg_ratio / theta);
        long long gk_slots = ceil(gk_ratio * _l);
        long long lower_memory = lower_slots *
                                 (sizeof(ID_TYPE) + sizeof(DATA_TYPE));
        long long mg_memory = mg_slots *
                              (sizeof(ID_TYPE) + 2 * sizeof(long long));
        long long gk_memory = gk_slots * gk_entry_bytes() +
                              gk.prefix_memory_bytes();
        return lower_memory + mg_memory + gk_memory;
    }

    long long calculate_weight(int level) {
        long long weight = 1;
        for (int i = 0; i < level; ++i) {
            weight *= 2;
        }
        return weight;
    }

    void insert_top(ID_TYPE key, DATA_TYPE value) {
        auto position = mg.find(key);
        if (position != mg.end()) {
            position->second.first++;
            gk.insert(position->second.second, value);
            return;
        }

        if (mg.size() < B) {
            long long new_key = next_key++;
            mg[key] = {1, new_key};
            gk.insert(new_key, value);
            return;
        }

        std::vector<ID_TYPE> remove_key;
        for (auto item : mg) {
            mg[item.first].first--;
            if (mg[item.first].first == 0) {
                remove_key.push_back(item.first);
            }
        }
        for (auto current_key : remove_key) {
            mg.erase(current_key);
        }
    }

    long long query_lower_rank(ID_TYPE key, DATA_TYPE value) {
        long long rank = 0;
        if (current_reservoir_samples > 0 &&
            reservoir_key == key && reservoir_value <= value) {
            rank += current_reservoir_samples;
        }
        for (int i = 0; i < array.size(); ++i) {
            for (auto item : array[i]) {
                if (item.first == key && item.second <= value) {
                    rank += calculate_weight(reservoir_level + i);
                }
            }
        }
        return rank;
    }

    long long query_lower_frequency(ID_TYPE key) {
        long long frequency = 0;
        if (current_reservoir_samples > 0 && reservoir_key == key) {
            frequency += current_reservoir_samples;
        }
        for (int i = 0; i < array.size(); ++i) {
            for (auto item : array[i]) {
                if (item.first == key) {
                    frequency += calculate_weight(reservoir_level + i);
                }
            }
        }
        return frequency;
    }

    double c;
    double theta;
    double gk_ratio;
    double mg_ratio;
    int l;
    int H;
    int N;
    int s;
    int lower_levels;
    int B;
    int gk_max_size;
    long long top_weight;
    long long memory_size;
    std::vector<std::vector<std::pair<ID_TYPE, DATA_TYPE>>> array;
    std::vector<int> max_size;
    int reservoir_level = 0;
    ID_TYPE reservoir_key;
    DATA_TYPE reservoir_value;
    long long current_reservoir_samples = 0;
    std::unordered_map<ID_TYPE, std::pair<long long, long long>> mg;
    GK_Summary<DATA_TYPE> gk;
    long long next_key;
};


#endif
