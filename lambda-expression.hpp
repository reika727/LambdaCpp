/**
 * @file lambda-expression.hpp
 * @brief ラムダ計算を実装したヘッダオンリーライブラリです。
 */

#include <algorithm>
#include <functional>
#include <iterator>
#include <numeric>
#include <vector>

namespace lambda {
    /**
     * @brief ラムダ式の実装
     */
    class expression final : std::function<expression(expression)> {
        using std::function<expression(expression)>::function;
        /* デコード処理だけは pass_by_value を使ってもよい */
        friend std::size_t church_decode(expression);
        template <class OutputIterator>
        friend void scott_decode(expression, OutputIterator);

    private:
        /**
         * @brief 値呼びを行う
         * @param[in] arg 引数
         * @return 評価結果
         */
        expression pass_by_value(expression arg) const
        {
            return std::function<expression(expression)>::operator()(arg);
        }

    public:
        /**
         * @brief 名前呼びを行う
         * @param[in] arg 引数
         * @return 評価結果
         */
        expression operator()(expression arg) const
        {
            return [arg, *this](expression _) {
                return pass_by_value(arg).pass_by_value(_);
            };
        }
    };

    /**
     * @brief 自然数をチャーチエンコーディングする
     * @param[in] n エンコードする自然数
     * @returns チャーチエンコーディングによるエンコード結果
     */
    inline expression church_encode(std::size_t n)
    {
        return [n](expression f) {
            return [n, f](expression x) {
                for (std::size_t i = 0; i < n; ++i) {
                    x = f(x);
                }
                return x;
            };
        };
    }

    /**
     * @brief 一般的なコンビネータのまとめ
     */
    namespace combinators {
        /** 真値  */
        static inline const expression truth = [](expression x) {
            return [x](expression y) {
                return x;
            };
        };

        /** 偽値 */
        static inline const expression falsity = [](expression x) {
            return [](expression y) {
                return y;
            };
        };

        /** Y コンビネータ。不動点コンビネータとして使用できる。 */
        static inline const expression Y = [](expression f) {
            return [f](expression x) {
                return f(x(x));
            }([f](expression x) {
                return f(x(x));
            });
        };

        /** SKI コンビネータの I */
        static inline const expression I = [](expression x) {
            return x;
        };

        /** SKI コンビネータの K */
        static inline const expression K = [](expression x) {
            return [x](expression y) {
                return x;
            };
        };

        /** SKI コンビネータの S */
        static inline const expression S = [](expression x) {
            return [x](expression y) {
                return [x, y](expression z) {
                    return x(z)(y(z));
                };
            };
        };

        /** iota コンビネータ */
        static inline const expression i = [](expression f) {
            return f(S)(K);
        };

        /** チャーチエンコーディングされた自然数の後者関数 */
        static inline const expression succ = [](expression n) {
            return [n](expression f) {
                return [n, f](expression x) {
                    return f(n(f)(x));
                };
            };
        };

        /** チャーチエンコーディングされた自然数の前者関数 */
        static inline const expression pred = [](expression n) {
            return [n](expression f) {
                return [n, f](expression x) {
                    return n(
                        [f](expression g) {
                            return [f, g](expression h) {
                                return h(g(f));
                            };
                        })([x](expression y) {
                        return x;
                    })([](expression y) {
                        return y;
                    });
                };
            };
        };

        /** チャーチエンコーディングされた自然数の加算 */
        static inline const expression add = [](expression n) {
            return [n](expression m) {
                return n(succ)(m);
            };
        };

        /** チャーチエンコーディングされた自然数の減算 */
        static inline const expression sub = [](expression n) {
            return [n](expression m) {
                return m(pred)(n);
            };
        };

        /** チャーチエンコーディングされた自然数の乗算 */
        static inline const expression mult = [](expression n) {
            return [n](expression m) {
                return n(add(m))(church_encode(0));
            };
        };

        /** チャーチエンコーディングされた自然数が 0 と等しいか */
        static inline const expression is_zero = [](expression n) {
            return n(
                [](expression x) {
                    return falsity;
                })(truth);
        };

        /** スコットエンコーディングによるリストを構築する  */
        static inline const expression cons = [](expression a) {
            return [a](expression b) {
                return [a, b](expression f) {
                    return f(a)(b);
                };
            };
        };

        /** スコットエンコーディングによるリストの先頭要素 */
        static inline const expression car = [](expression p) {
            return p(
                [](expression x) {
                    return [x](expression y) {
                        return x;
                    };
                });
        };

        /** スコットエンコーディングによるリストの先頭要素を除いたリスト */
        static inline const expression cdr = [](expression p) {
            return p(
                [](expression x) {
                    return [](expression y) {
                        return y;
                    };
                });
        };

        /** スコットエンコーディングによる空リスト */
        static inline const expression empty_list = [](expression f) {
            return [](expression x) {
                return [x](expression y) {
                    return x;
                };
            };
        };

        /** スコットエンコーディングによるリストが空であるか */
        static inline const expression is_empty = [](expression l) {
            return l(
                [](expression x) {
                    return [](expression y) {
                        return falsity;
                    };
                });
        };
    }

    /**
     * @brief チャーチエンコーディングされた自然数をデコードする
     * @param[in] n チャーチエンコーディングされた自然数
     * @returns デコード結果
     */
    inline std::size_t church_decode(expression n)
    {
        std::size_t decoded = 0;
        n.pass_by_value(
             [&decoded](expression x) {++decoded; return x; })
            .pass_by_value(combinators::I)
            .pass_by_value(combinators::I);
        return decoded;
    }

    /**
     * @brief スコットエンコーディングによるリストを作成する
     * @param[in] first 先頭要素を指すイテレータ
     * @param[in] last 最後の要素の次を指すイテレータ
     * @return スコットエンコーディングによるエンコード結果
     */
    template <class InputIterator>
    inline expression scott_encode(InputIterator first, InputIterator last)
    {
        return std::accumulate(
            std::reverse_iterator(last), std::reverse_iterator(first),
            combinators::empty_list,
            [](expression acc, expression e) {
                return combinators::cons(e)(acc);
            });
    }


    /**
     * @brief スコットエンコーディングによるリストを分解する
     * @param[in] list スコットエンコーディングによるリスト
     * @param[out] result リストに含まれていた各ラムダ式の出力先
     */
    template <class OutputIterator>
    inline void scott_decode(expression list, OutputIterator result)
    {
        expression _output_list = [&result](expression f) {
            return [&result, f](expression l) {
                return combinators::is_empty.pass_by_value(l)
                    .pass_by_value(
                        combinators::empty_list)
                    .pass_by_value(
                        [&result, l, f](expression _) {
                            *result++ = combinators::car.pass_by_value(l);
                            return f.pass_by_value(combinators::cdr.pass_by_value(l)).pass_by_value(_);
                        });
            };
        };
        combinators::Y.pass_by_value(_output_list).pass_by_value(list).pass_by_value(combinators::I).pass_by_value(combinators::I);
    }

    /**
     * @brief 自然数の列に対しラムダ計算によるプログラムを実行する
     * @param[in] first 先頭要素を指すイテレータ
     * @param[in] last 最後の要素の次を指すイテレータ
     * @param[in] program 実行するラムダ式
     * @paran[out] result program を実行した結果の自然数のリストの出力先
     * @detail 自然数のリストに対し、各要素をチャーチデコーディングによりエンコードしたのちに
     * それらをスコットエンコーディングにより一つのラムダ式にする。これを L とし、program(L) を求める。
     * あとは逆の要領で、program(L) をスコットデコーディングによりラムダ式の一覧へ分解したのち
     * 各要素をチャーチデコーディングにより自然数へ戻したものを計算結果として出力する。
     */
    template <class InputIterator, class OutputIterator>
    inline void run_on_integer_sequence(InputIterator first, InputIterator last, expression program, OutputIterator result)
    {
        std::vector<expression> church_encoded;
        std::transform(first, last, std::back_inserter(church_encoded), church_encode);
        std::vector<expression> scott_decoded;
        scott_decode(program(scott_encode(church_encoded.begin(), church_encoded.end())), std::back_inserter(scott_decoded));
        std::transform(scott_decoded.begin(), scott_decoded.end(), result, church_decode);
    }
}
