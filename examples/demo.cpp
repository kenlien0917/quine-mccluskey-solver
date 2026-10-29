#define main coursework_demo_main
#include "../src/QM.cpp"
#undef main

// Added portfolio example. The submitted solver is included without edits.
int main() {
    int n = 4;
    vector<int> on{0, 4, 5, 10, 11};
    vector<int> dc{1, 13, 14, 15};
    auto* sol = solveQm(n, on, dc, false);
    auto forms = sol->minimumSops;
    if (forms.empty() && on.empty()) cout << "SOP: 0\n";
    for (const auto& sop : forms) {
        cout << "SOP: ";
        if (sop.empty()) cout << "0";
        for (size_t i=0; i<sop.size(); ++i) {
            if (i) cout << " + ";
            cout << (sop[i].empty() ? "1" : sop[i]);
        }
        cout << '\n';
    }
    delete sol;
}
