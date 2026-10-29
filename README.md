# Quine–McCluskey Solver

以 **C++** 完成的數位邏輯課程專案。以 implicant 合併找出 prime implicants，建立覆蓋表、挑出 essential primes，再用 Petrick’s method 搜尋 SOP 覆蓋。

## 展示重點

以 implicant 合併找出 prime implicants，建立覆蓋表、挑出 essential primes，再用 Petrick’s method 搜尋 SOP 覆蓋。

程式保留提交版演算法，另外附上可執行範例、真值表驗證與 GitHub Actions。來源與整理範圍見 [PROVENANCE.md](PROVENANCE.md)。

## 編譯與執行

需要支援 C++17 的 GCC 或 Clang。在 repo 根目錄執行：

```sh
mkdir -p build
g++ -std=c++17 -O2 examples/demo.cpp -o build/demo
./build/demo
g++ -std=c++17 -O2 tests/truth_table.cpp -o build/truth_table
./build/truth_table
```

Windows PowerShell 可先用 `New-Item -ItemType Directory -Force build` 建立目錄，再將輸出檔名改為 `build/demo.exe`、`build/truth_table.exe` 並直接執行。

只編譯範例／測試檔即可：它們會包含 `src/` 的提交版來源，不要再次把來源檔加入同一個編譯命令。

示範輸入為 4 變數、minterms `0,4,5,10,11`、don't-cares `1,13,14,15`。實測輸出：

```text
SOP: a'c' + ac
```

`a` 是最高位元；相連變數表示 AND、`+` 表示 OR、`'` 表示 NOT。修改 `examples/demo.cpp` 的 `n`、`on`、`dc` 可試其他合法輸入。

## 驗證

2026-10-30 使用 GCC 13.3.0 / C++17 在隔離 Linux 容器編譯並執行範例及測試：**341 cases，0 failures**。

- 全部 81 種 2 變數 on/off/don't-care 配置。
- 全部 256 種 3 變數真值表。
- 4 個 4 變數案例：don't-cares、常數 0、常數 1 與分離群組。

每個 SOP 都由獨立的布林運算求值器檢查所有非 don't-care 輸入。這些結果只代表列出的案例，並非對任意輸入的正確性或最小性證明。

## 已知限制

- Petrick 展開設有約 1,000,000 products 的限制，超過門檻時可能提前中止，無法保證列出所有最佳解；本次未驗證大型輸入的最小性。
- 測試驗證布林等價，不是全域最小化的獨立證明。
- 空 minterms 回傳空解集，表示常數 0；空 literal term 以 `1` 表示。
- 原始 API 沒有輸入驗證；使用合法、不重複且不重疊的項目，避免負索引或超出變數數量範圍。變數名稱從 `a` 開始，展示用途限於小型函數。
- 回傳結構由呼叫端 `delete`。原 `main()` 是固定 8 變數的 verbose 示範；本 repo 另附印出 SOP 的範例。

相關作品：[FPGA portfolio](https://github.com/kenlien0917/fpga-logic-design-portfolio) · [K-map](https://github.com/kenlien0917/kmap-solver)
