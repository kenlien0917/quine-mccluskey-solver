#include <vector>
#include <numeric>
#include <string>
#include <iostream>
#include <algorithm>
#include <set>
#include <climits>
#include <functional>
#include <map>
#include <iomanip>

using namespace std;

typedef vector<int> Bits;

// Structure representing a logic implicant used in the Quine-McCluskey algorithm
struct Implicant {
    Bits bits;               // The bitwise representation of the term (e.g. 1, 0, or 2 where 2 means "don't care")
    vector<int> origins;     // List of minterms that this implicant covers
    bool used = false;       // True if this implicant has been combined into another implicant
};

// Structure to hold the solution of the Quine-McCluskey algorithm
struct QmSolution {
    int numVar;                                 // Number of variables in the boolean function
    vector<string> primes;                      // All prime implicants found
    vector<string> essentialPrimes;             // Subset of primes that are essential
    vector<vector<string>> minimumSops;         // List of minimal Sum-of-Products (SOP) expressions
};

// Converts a decimal minterm into its binary vector representation
// Example: minterm = 5, numVar = 3 -> bits = {1, 0, 1}
static Bits Binarize(int minterm, int var) {
    Bits bits(var);
    for (int i = 0; i < var; ++i)
        bits[i] = (minterm >> (var - 1 - i)) & 1;  // Fill bits from most to least significant
    return bits;
}

// Expands a Bits vector that may contain wildcards (2 = don't care) into all the concrete minterm integers it covers
// Example: bits = {1, 2, 0} -> expands to {4, 6}
static vector<int> Expand(const Bits& bits) {
    vector<int> res;
    int n = bits.size();

    // Recursive function to explore all combinations
    function<void(int, int)> dfs = [&](int pos, int val) {
        if (pos == n) {
            res.push_back(val); // Store fully constructed binary number
            return;
        }
        if (bits[pos] == 2) { // If current bit is a wildcard, try both 0 and 1
            dfs(pos + 1, val << 1);
            dfs(pos + 1, (val << 1) | 1);
        }
        else {
            dfs(pos + 1, (val << 1) | bits[pos]);
        }
        };

    dfs(0, 0); // Start recursive expansion
    return res;
}

// Initializes the first column of the implicant table, grouping implicants by the number of 1s in their binary form
// Each group will be a list of implicants with the same number of 1s
static vector<vector<Implicant>> InitColumn(const vector<int>& terms, int numVar) {
    vector<vector<Implicant>> groups(numVar + 1);  // Create groups indexed by number of 1s

    for (int t : terms) {
        Bits b = Binarize(t, numVar);              // Convert term to binary
        int ones = accumulate(b.begin(), b.end(), 0);  // Count number of 1s in binary
        Implicant imp{ b, { t }, false };          // Create new implicant for this term
        groups[ones].push_back(imp);               // Place in corresponding group
    }

    return groups;
}


// Attempts to combine two implicants `a` and `b`
// Returns true and stores the result in `out` if they differ by exactly one bit
static bool TryCombine(const Implicant& a, const Implicant& b, Implicant& out) {
    int diffCount = 0, diffIndex = -1;

    // Compare bits to find one differing position
    for (int i = 0; i < a.bits.size(); ++i) {
        if (a.bits[i] != b.bits[i]) {
            // They must differ by exactly 1 vs 0 (not 2 vs anything)
            if ((a.bits[i] ^ b.bits[i]) == 1) {
                diffIndex = i;
                if (++diffCount > 1)
                    return false; // More than one difference: can't combine
            }
            else {
                return false; // Incompatible difference (e.g. 0 vs 2)
            }
        }
    }

    // If exactly one bit differs, combine the two implicants
    if (diffCount == 1) {
        out = a; // Start from one implicant
        out.bits[diffIndex] = 2; // Replace differing bit with wildcard (-)

        // Merge their origins (minterms)
        out.origins.insert(out.origins.end(), b.origins.begin(), b.origins.end());
        sort(out.origins.begin(), out.origins.end());
        out.origins.erase(unique(out.origins.begin(), out.origins.end()), out.origins.end());

        out.used = false;
        return true;
    }

    return false; // No difference or too many differences
}


// Combines implicants from adjacent groups in a column
// Returns the new next column of combined implicants
static vector<vector<Implicant>> CombineColumn(vector<vector<Implicant>>& col, int numVar) {
    vector<vector<Implicant>> next(numVar + 1);

    // Compare each group with the next one (e.g. group 0 with group 1, 1 with 2, ...)
    for (int k = 0; k + 1 < col.size(); ++k) {
        for (size_t i = 0; i < col[k].size(); ++i) {
            for (size_t j = 0; j < col[k + 1].size(); ++j) {
                Implicant combined;

                // Try to combine implicants from group k and k+1
                if (TryCombine(col[k][i], col[k + 1][j], combined)) {
                    // Mark both original implicants as used
                    col[k][i].used = col[k + 1][j].used = true;

                    // Place new implicant in appropriate group by number of 1s
                    int ones = count(combined.bits.begin(), combined.bits.end(), 1);
                    next[ones].push_back(combined);
                }
            }
        }
    }

    return next;
}


// Converts a bit vector to a term string representation
// e.g., bits = {1, 0, 2} -> "a b'" (2 = wildcard is omitted)
static string BitsToTerm(const Bits& bits) {
    string term;

    for (int i = 0; i < bits.size(); ++i) {
        char var = 'a' + i;

        if (bits[i] == 1)
            term.push_back(var);         // 1 → variable (e.g. a)
        else if (bits[i] == 0) {
            term.push_back(var);         // 0 → complemented variable (e.g. a')
            term.push_back('\'');
        }
        // 2 (wildcard) is skipped
    }

    return term.empty() ? "1" : term;
}


// Prints a single column of implicants with annotations
// - 'v': used implicants (combined in next round)
// - 'd': implicants composed entirely of don't-cares
// - 'x': duplicate implicants (already seen)
static void PrintColumn(const vector<vector<Implicant>>& col, const set<int>& dontcares, int columnIndex) {
    cout << "\n=====================\n";
    cout << "Column " << columnIndex << "\n";
    cout << "=====================\n";

    map<string, int> seen; // Used to detect duplicate implicants

    // Converts bit vector to string format like 01-1
    auto bitsToStr = [](const Bits& b) {
        string s;
        for (int bit : b) {
            if (bit == 2) s += '-';
            else s += (bit ? '1' : '0');
        }
        return s;
        };

    for (const auto& group : col) {
        if (group.empty()) continue;

        for (const auto& imp : group) {
            string bitStr = bitsToStr(imp.bits);
            string prefix = "  "; // Default: unmarked

            // If all origins are don't-cares, mark as 'd'
            bool isDontCareOnly = all_of(imp.origins.begin(), imp.origins.end(), [&](int o) {
                return dontcares.count(o);
                });

            if (isDontCareOnly)
                prefix = "d ";
            else if (imp.used)
                prefix = "v ";

            // If bit pattern seen more than once, mark as duplicate
            if (++seen[bitStr] > 1)
                prefix = "x ";

            // Print row
            cout << prefix << bitStr << ": ";
            for (size_t i = 0; i < imp.origins.size(); ++i) {
                cout << imp.origins[i];
                if (i + 1 < imp.origins.size()) cout << ", ";
            }
            cout << "\n";
        }

        cout << "---------------------\n";
    }
}

// Displays a Prime Implicant Chart in the console.
// Rows = prime implicants (in literal form)
// Columns = original minterms
// 'x' marks which implicants cover which minterms
static void PrintPrimeImplicantChart(const vector<Implicant>& primeImps, const vector<int>& minterms, int numVar) {
    // If no data to display, exit early
    if (primeImps.empty() || minterms.empty()) {
        cout << "[Warning] No prime implicants or minterms to display.\n";
        return;
    }

    // Chart title
    cout << "\n=====================\n";
    cout << "Prime Implicant Chart\n";
    cout << "=====================\n";

    // Copy and sort minterms so columns are in ascending order
    vector<int> sortedMinterms = minterms;
    sort(sortedMinterms.begin(), sortedMinterms.end());

    // Generate human-readable literal labels for each implicant (e.g., a'b or abc)
    int labelWidth = 0;                // Used for proper alignment
    vector<string> labels;
    for (const auto& imp : primeImps) {
        string label;
        for (int i = 0; i < numVar; ++i) {
            if (imp.bits[i] == 0)
                label += (char)('a' + i) + string("'");  // 0 = negated variable
            else if (imp.bits[i] == 1)
                label += (char)('a' + i);                // 1 = variable
            // 2 (wildcard) is skipped
        }
        labels.push_back(label);
        labelWidth = max(labelWidth, (int)label.length());  // Track the widest label
    }
    labelWidth = max(labelWidth, 20); // Ensure a minimum label column width

    // Print header row with minterms
    cout << setw(labelWidth + 1) << " ";  // Padding for left column
    for (int m : sortedMinterms)
        cout << "|" << setw(5) << m;
    cout << "|\n";

    // Print horizontal header separator
    cout << " " << string(labelWidth, '-') << "+";
    for (size_t i = 0; i < sortedMinterms.size(); ++i)
        cout << string(5, '-') << "+";
    cout << "\n";

    // Print each row (prime implicant → minterm coverage)
    for (size_t idx = 0; idx < primeImps.size(); ++idx) {
        const auto& imp = primeImps[idx];
        const string& label = labels[idx];

        cout << setw(labelWidth) << left << label << " |";  // Left-aligned literal name

        // Expand implicant to get all covered minterms
        vector<int> expanded = Expand(imp.bits);
        set<int> covered(expanded.begin(), expanded.end());

        // Print 'x' in columns where this implicant covers the minterm
        for (int m : sortedMinterms) {
            if (covered.count(m))
                cout << setw(5) << "x" << "|";
            else
                cout << setw(5) << " " << "|";
        }
        cout << "\n";
    }

    // Bottom separator to close the table
    cout << " " << string(labelWidth, '-') << "+";
    for (size_t i = 0; i < sortedMinterms.size(); ++i)
        cout << string(5, '-') << "+";
    cout << "\n";
}



// Main function that performs the Quine-McCluskey minimization
QmSolution* solveQm(int numVar, const vector<int>& minterms, const vector<int>& dontcares, bool verbose) {
    QmSolution* sol = new QmSolution{ numVar };
    if (minterms.empty()) return sol;  // Edge case: no work to do

    // Combine minterms and don't-cares for simplification
    vector<int> all = minterms;
    all.insert(all.end(), dontcares.begin(), dontcares.end());

    // Initialize first column from minterms + don't-cares
    vector<vector<vector<Implicant>>> columns;
    columns.push_back(InitColumn(all, numVar));
    int columnIndex = 1;

    // Generate all columns by combining adjacent groups iteratively
    while (true) {
        vector<vector<Implicant>>& current = columns.back();  // Get last column
        vector<vector<Implicant>> next = CombineColumn(current, numVar); // Combine pairs to next column

        // Print column if verbose
        if (verbose) {
            PrintColumn(current, set<int>(dontcares.begin(), dontcares.end()), columnIndex++);
        }

        // Stop if no further combinations possible
        bool any = false;
        for (auto& group : next)
            if (!group.empty()) any = true;
        if (!any) break;

        columns.push_back(next);
    }

    // Extract prime implicants (those not used in further combinations)
    vector<Implicant> primeImps;
    for (auto& col : columns) {
        for (auto& group : col) {
            for (auto& imp : group) {
                if (!imp.used) {
                    for (int o : imp.origins) {
                        if (find(minterms.begin(), minterms.end(), o) != minterms.end()) {
                            primeImps.push_back(imp);
                            break;
                        }
                    }
                }
            }
        }
    }

    // Optionally print the prime implicant chart (without duplicates)
    if (verbose && !primeImps.empty()) {
        vector<Implicant> uniqueImps;
        set<string> seen;
        auto bitsToKey = [](const Bits& bits) {
            string key;
            for (int b : bits)
                key += (b == 2 ? '-' : (b == 1 ? '1' : '0'));
            return key;
            };
        for (const auto& imp : primeImps) {
            string key = bitsToKey(imp.bits);
            if (seen.insert(key).second) {
                uniqueImps.push_back(imp);
            }
        }
        PrintPrimeImplicantChart(uniqueImps, minterms, numVar);
    }

    // Determine which prime implicants cover which minterms
    set<int> M(minterms.begin(), minterms.end());
    int P = primeImps.size();
    vector<set<int>> covers(P);
    for (int i = 0; i < P; ++i) {
        vector<int> expanded = Expand(primeImps[i].bits);
        for (int x : expanded) {
            if (M.count(x)) covers[i].insert(x);
        }
    }

    // Identify essential prime implicants (those that cover a minterm uniquely)
    set<int> essentialIdx;
    set<int> covered;
    for (int m : M) {
        int count = 0, idx = -1;
        for (int i = 0; i < P; ++i) {
            if (covers[i].count(m)) { ++count; idx = i; }
        }
        if (count == 1) {
            essentialIdx.insert(idx);
            covered.insert(covers[idx].begin(), covers[idx].end());
        }
    }

    // Add essential implicants to result
    for (int i : essentialIdx)
        sol->essentialPrimes.push_back(BitsToTerm(primeImps[i].bits));

    // If all minterms are covered, we're done
    if (covered.size() == M.size()) {
        sol->minimumSops.push_back(sol->essentialPrimes);
        for (auto& pi : primeImps)
            sol->primes.push_back(BitsToTerm(pi.bits));
        return sol;
    }

    // Prepare clauses for Petrick's method (used to find minimal cover)
    vector<set<int>> clauses;
    for (int m : M) {
        if (covered.count(m)) continue;
        set<int> clause;
        for (int i = 0; i < P; ++i) {
            if (essentialIdx.count(i)) continue;
            if (covers[i].count(m)) clause.insert(i);
        }
        if (!clause.empty()) clauses.push_back(clause);
    }

    // Apply Petrick's method to find all valid combinations
    const size_t MAX_PRODUCT_SIZE = 1000000;
    set<set<int>> products;
    for (int idx : clauses[0]) products.insert({ idx });

    for (size_t c = 1; c < clauses.size(); ++c) {
        set<set<int>> next;
        for (const auto& p : products) {
            for (int idx : clauses[c]) {
                set<int> merged = p;
                merged.insert(idx);
                next.insert(merged);
                if (next.size() > MAX_PRODUCT_SIZE) break;
            }
            if (next.size() > MAX_PRODUCT_SIZE) break;
        }
        products = move(next);
        if (products.size() > MAX_PRODUCT_SIZE) break;
    }

    // Select the best product(s): fewest terms, then fewest literals
    int minSize = INT_MAX, minLiterals = INT_MAX;
    vector<set<int>> bestCovers;
    auto countLiterals = [&](const set<int>& s) {
        int count = 0;
        for (int idx : s)
            count += count_if(primeImps[idx].bits.begin(), primeImps[idx].bits.end(), [](int b) { return b != 2; });
        return count;
        };

    for (auto& s : products) {
        set<int> total = covered;
        for (int idx : s) total.insert(covers[idx].begin(), covers[idx].end());
        if (total != M) continue;

        int terms = s.size();
        int lits = countLiterals(s);
        if (terms < minSize || (terms == minSize && lits < minLiterals)) {
            minSize = terms;
            minLiterals = lits;
            bestCovers.clear();
            bestCovers.push_back(s);
        }
        else if (terms == minSize && lits == minLiterals) {
            bestCovers.push_back(s);
        }
    }

    // Collect all unique prime implicants
    set<string> allPrimes;
    for (auto& pi : primeImps)
        allPrimes.insert(BitsToTerm(pi.bits));
    sol->primes.assign(allPrimes.begin(), allPrimes.end());

    // Generate all minimum SOPs from best covers
    set<vector<string>> uniqueSops;
    for (auto& s : bestCovers) {
        vector<string> sop(sol->essentialPrimes.begin(), sol->essentialPrimes.end());
        for (int idx : s)
            sop.push_back(BitsToTerm(primeImps[idx].bits));
        sort(sop.begin(), sop.end());
        uniqueSops.insert(sop);
    }

    for (const auto& sop : uniqueSops)
        sol->minimumSops.push_back(sop);

    return sol;
}




int main() {
    int numVar = 8;
    vector<int> minterms = { 0, 2, 5, 6, 7, 8, 10, 12, 13, 14, 15 };
    vector<int> dontcares = { 255 };
    bool verbose = true;
    QmSolution* sol = solveQm(numVar, minterms, dontcares, verbose);
    delete sol;
    return 0;
}
