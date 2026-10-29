#define main coursework_demo_main
#include "../src/QM.cpp"
#undef main

#include <cstdlib>
// Independent Boolean evaluator: a is the most significant variable.
bool matches(const string& term, int n, int x) {
    if(term == "1" || term.empty()) return true;
    if(term == "0") return false;
    for(size_t i=0;i<term.size();++i) {
        int v = term[i]-'a';
        if(v<0 || v>=n) { cerr << "Invalid term: " << term << '\n'; exit(2); }
        bool bit = (x >> (n-v-1)) & 1;
        bool inverted = i+1<term.size() && term[i+1]=='\'';
        if(inverted) ++i;
        if(bit == inverted) return false;
    }
    return true;
}
int failures=0, cases=0;
void check(int n, vector<int> on, vector<int> dc) {
    ++cases;
    auto* sol = solveQm(n, on, dc, false);
    auto forms = sol->minimumSops;
    bool ok = !forms.empty() || on.empty();
    for(const auto& sop:forms) {
        for(int x=0;x<(1<<n);++x) {
            if(find(dc.begin(),dc.end(),x)!=dc.end()) continue;
            bool actual=false;
            for(const auto& term:sop) actual |= matches(term,n,x);
            bool expected=find(on.begin(),on.end(),x)!=on.end();
            ok &= actual==expected;
        }
    }
    if(!ok) {
        ++failures;
        if(failures<=8) {
            cerr << "FAIL n=" << n << " on=";
            for(int x:on) cerr << x << ',';
            cerr << " dc="; for(int x:dc) cerr << x << ',';
            cerr << '\n';
        }
    }
    delete sol;
}
int main() {
    // Every 2-variable on/off/don't-care assignment (3^4 cases).
    for(int code=0;code<81;++code) {
        int c=code; vector<int> on,dc;
        for(int x=0;x<4;++x,c/=3) { if(c%3==1) on.push_back(x); if(c%3==2) dc.push_back(x); }
        check(2,on,dc);
    }
    // Every 3-variable truth table without don't-cares (2^8 cases).
    for(int mask=0;mask<256;++mask) {
        vector<int> on;
        for(int x=0;x<8;++x) if((mask>>x)&1) on.push_back(x);
        check(3,on,{});
    }
    check(4,{0,4,5,10,11},{1,13,14,15});
    check(4,{},{});
    check(4,{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15},{});
    check(4,{0,2,5,7,8,10,13,15},{});
    cout << "Cases: " << cases << "; failures: " << failures << '\n';
    return failures ? 1 : 0;
}
