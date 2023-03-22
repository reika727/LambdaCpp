# LambdaCpp
C++ でラムダ計算を処理するためのヘッダオンリーライブラリです。

# Example
```c++
#include "lambda-expression.hpp"
#include <vector>
#include <iterator>
#include <iostream>

using namespace lambda::combinators;

int main()
{
    /*
     * チャーチエンコーディングされた自然数に対し階乗を求める
     */
    lambda::expression fact = Y(
        [](lambda::expression f) {
            return [f](lambda::expression n) {
                return is_zero(n)(
                    lambda::church_code::encode(1)
                )(
                    mult(n)(f(pred(n)))
                );
            };
        }
    );

    /*
     * スコットエンコーディングされたリストを受け取り、
     * 各要素を階乗で置き換えたリストを返す
     */
    lambda::expression program = Y(
        [fact](lambda::expression f) {
            return [fact, f](lambda::expression l) {
                return is_empty(l)(
                    empty_list
                )(
                    cons(fact(car(l)))(f(cdr(l)))
                );
            };
        }
    );

    /*
     * プログラムへの入力。各要素をチャーチエンコーディングしたのち
     * それらをスコットエンコーディングしたものが program への引数となる。
     */
    std::vector<unsigned> numbers {1, 2, 3, 4, 5};

    /*
     * program の計算結果をスコットデコーディングとチャーチデコーディングで
     * 自然数の列に戻したものをここに入れる
     */
    std::vector<unsigned> result;

    /*
     * 実行
     */
    lambda::run_on_integer_sequence(numbers.begin(), numbers.end(), program, std::back_inserter(result));

    /*
     * 計算結果を表示する
     */
    for (auto num : result) {
        std::cout << num << ' ';
    }
    std::cout << std::endl;
}
```

```shell
$ ./a.out
1 2 6 24 120
```

# このライブラリに含まれるもの
- `namespace lambda`
  - `class expression` : ラムダ式の実装です。こんな風に関数オブジェクトを代入できます。

    ```c++
    /* λxy.x */
    lambda::expression e = [](lambda x) {
        return [x](lambda y) {
            return x;
        };
    };

    /* λx.x */
    lambda::expression f = [](lambda x) {
        return x;
    };

    e(f); /* 関数呼び出し。遅延評価になっているので Y コンビネータ等にも安心して渡せます。 */
    ```

  - `namespace combinators` : 各種コンビネータが入っています。
    - チャーチブール値（`truth`, `falsity`）
    - Y コンビネータ（`Y`）
    - SKI コンビネータ（`S`, `K`, `I`）
    - イオタコンビネータ（`i`）
    - チャーチ自然数への各種算術（`succ`, `pred`, `add`, `sub`, `mult`, `is_zero`）
    - スコットリストへの各種演算（`cons`, `car`, `cdr`, `empty_list`, `is_empty`）
  - `class church_code`
    - `static expression encode(std::size_t n)` : `n` をチャーチエンコーディングしたラムダ式を作ります。
    - `static std::size_t decode(expression n)` : `n` をデコードした自然数を返却します。
  - `class scott_code`
    - `static expression encode(InputIterator first, InputIterator last)` : [`first`, `last`) 内の `expression` オブジェクトをスコットエンコーディングしてリストを作ります。
    - `static void decode(expression list, OutputIterator result)` : スコットリスト `list` をデコードして `result` に書き込みます。
  - `void run_on_integer_sequence(InputIterator first, InputIterator last, expression program, OutputIterator result)` : このライブラリのミソです。[`first`, `last`) 内の自然数をチャーチエンコーディングしたのちスコットエンコーディングでまとめたものを `program` に引数として与え、それをデコードして自然数の列に戻したものを `result` に書き込みます。
