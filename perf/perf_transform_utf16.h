/* Performance test for Unicode Algorithms Implementation.
 * License: Public Domain or MIT - choose whatever you want.
 * See LICENSE.md */

// All performance tests are a mess. If you want to use them you're on your own.

//#define MSVC_COMPILER
//#define ENABLE_ICU_TEST

#ifdef MSVC_COMPILER
#include "stdafx.h"
#ifdef ICU_UTF8TO16
#pragma comment(lib, "icuio.lib")
#pragma comment(lib, "icuuc.lib")
#pragma comment(lib, "icuin.lib")
#endif
#endif
#ifdef ENABLE_ICU_TEST
#include "unicode\ustream.h"
#include "unicode\errorcode.h"
#include "unicode\translit.h"
#endif
#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <stdexcept>
#include "../src/cpp_uni_iterator.h"
#include "../src/cpp_uni_norm.h"

std::u16string trans_uni(std::u16string_view str)
{
    uni::iter::utf16<decltype(str.cbegin())> it_begin{str.cbegin(), str.cend()};

    uni::iter::norm::nfd<decltype(it_begin), uni::sentinel_t> it_nfd_begin{it_begin, uni::sentinel};

    struct func { bool operator()(char32_t c) { return c != 0x0301 && c != 0x0300; } };
    uni::iter::func::filter<func, decltype(it_nfd_begin), uni::sentinel_t> it_func_begin{func{}, it_nfd_begin, uni::sentinel};

    uni::iter::norm::nfc<decltype(it_func_begin), uni::sentinel_t> it_nfc_begin{it_func_begin, uni::sentinel};

    std::u16string result;
    uni::iter::output::utf16<decltype(std::back_inserter(result))> it_out{std::back_inserter(result)};

    for (auto it = it_nfc_begin; it != uni::sentinel; ++it)
        it_out = *it;

    return result;
}

#ifdef ENABLE_ICU_TEST
UErrorCode icu_status = U_ZERO_ERROR;
icu::Transliterator *accentsConverter =
    icu::Transliterator::createInstance(L"NFD; [[\u0300][\u0301]] Remove; NFC", UTRANS_FORWARD, icu_status);
std::u16string trans_ICU(std::u16string_view str)
{
    int count = 0;
    icu::UnicodeString cs(str.data(), str.size());

    accentsConverter->transliterate(cs);

    std::u16string utf16(cs.getBuffer(), cs.length());

    return utf16;
}
#endif

const size_t number_of_passes = 50000; // Text/File
std::vector<std::u16string> strs;

void fill_1()
{
    if (number_of_passes > 50000)
        throw std::runtime_error("RAM! RAM! RAM!");

    // Random text from: https://creativecommons.org/licenses/by-sa/1.0/deed.en
    std::u32string s1 = U"Attribution You must give appropriate credit, provide a link to the license, and indicate if changes were made. You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.";
    std::u32string s2 = U"Attiecinājums — Jums ir atbilstoši jāatsaucas uz darbu, norādot saiti uz licenci un jānorāda veiktās izmaiņas. Jūs varat to darīt dažādos saprātīgos veidos, bet noteikti ne tā, kas liktu domāt, ka licencētājs ir apstiprinājis jūsu darbu vai tajā veiktās izmaiņas.";
    std::u32string s3 = U"«Attribution» («Атрибуция») — Вы должны обеспечить соответствующее указание авторства, предоставить ссылку на лицензию, и обозначить изменения, если таковые были сделаны. Вы можете это делать любым разумным способом, но не таким, который подразумевал бы, что лицензиар одобряет вас или ваш способ использования произведения.";
    std::u32string s4 = U"অ্যাট্রিবিউশন — আপনাকে অবশ্যই যথাযথ কৃতিত্ব দিতে হবে, লাইসেন্সের একটি লিঙ্ক প্রদান করতে হবে, এবং কোন পরিবর্তন করা হলে তা নির্দেশ করতে হবে। আপনি যে কোন যুক্তিসঙ্গত পদ্ধতিতে তা করতে পারেন, কিন্তু এমন কোন পদ্ধতিতে নয় যাতে মনে হয় লাইসেন্সকারী আপনাকে বা আপনার এই ব্যবহারের জন্য অনুমোদন দিয়েছেন।";
    std::u32string s5 = U"署名 — 您必须给出适当的署名，提供指向本许可协议的链接，同时标明是否（对原始作品）作了修改。您可以用任何合理的方式来署名，但是不得以任何方式暗示许可人为您或您的使用背书。";
    std::u32string s6 = U"表示 — あなたは 適切なクレジットを表示し、ライセンスへのリンクを提供し、変更があったらその旨を示さなければなりません。これらは合理的であればどのような方法で行っても構いませんが、許諾者があなたやあなたの利用行為を支持していると示唆するような方法は除きます。";
    std::u32string s7 = U"저작자표시 — 적절한 출처와, 해당 라이센스 링크를 표시하고, 변경이 있는 경우 공지해야 합니다. 합리적인 방식으로 이렇게 하면 되지만, 이용 허락권자가 귀하에게 권리를 부여한다거나 귀하의 사용을 허가한다는 내용을 나타내서는 안 됩니다.";

    // MSVS messed up encoding. Again!
    static_assert(U'㋡' == 0x32E1);

    for (size_t i = 0; i < number_of_passes; i++)
    {
        std::u32string str;
        for (size_t j = 0; j < 3; j++)
        {
            str += s1 + s2 + s3 + s4 + s5 + s6 + s7;
        }
        strs.emplace_back(uni::utf32to16<char32_t, char16_t>(str));
    }
}

void fill_2()
{
    // The file contains the same data that is generated with fill_1() function above
    // The performance can be a bit different just because
    std::string file = "random_text.txt";

    std::ifstream stream;
    stream.open(file, std::ios::binary);
    if (!stream.is_open())
        throw std::runtime_error(file + " file not found");

    size_t size = (size_t)stream.tellg();
    stream.seekg(0, std::ios::end);
    size = (size_t)stream.tellg() - size;
    stream.seekg(0, std::ios::beg);

    if (number_of_passes * size > 1000000 * 500)
        throw std::runtime_error("RAM! REM! RAM!");

    std::istreambuf_iterator<char> eos;
    std::string str(std::istreambuf_iterator<char>(stream), eos);

    for (size_t i = 0; i < number_of_passes; i++)
        strs.emplace_back(uni::utf8to16<char, char16_t>(str));
}

void test_performance();
void generate_table();

int main5()
{
    fill_1();

    //test_performance();
    generate_table();

    return 0;
}

void test_performance()
{
    unsigned int nothing = 0;

    for (int j = 0; j < 10; j++)
    {
        double duration1 = 0.0;

        {
            auto time1 = std::chrono::steady_clock::now();
            for (size_t i = 0; i < number_of_passes; i++)
            {

                std::u16string result = trans_uni(strs[i]);
                //std::u16string result = trans_ICU(strs[i]);

                // Use it somehow just because
                nothing += result.back();
            }
            auto time2 = std::chrono::steady_clock::now();
            duration1 = std::chrono::duration<double, std::milli>(time2 - time1).count();
        }

        std::cout << duration1 << '\n';
    }

    std::cout << nothing << '\n';
}

void generate_table()
{
    unsigned int nothing = 0;

    std::cout << "UNI" << '\t'  << "ICU" << '\n';

    for (size_t j = 0; j < 10; j++)
    {
        double duration1 = 0.0;
        double duration2 = 0.0;

        {
            auto time1 = std::chrono::steady_clock::now();
            for (size_t i = 0; i < number_of_passes; i++)
            {
                std::u16string result = trans_uni(strs[i]);
                nothing += result.back();
            }
            auto time2 = std::chrono::steady_clock::now();
            duration1 = std::chrono::duration<double, std::milli>(time2 - time1).count();
        }
#ifdef ENABLE_ICU_TEST
        {
            auto time1 = std::chrono::steady_clock::now();
            for (size_t i = 0; i < number_of_passes; i++)
            {
                std::u16string result = trans_ICU(strs[i]);
                nothing += result.back();
            }
            auto time2 = std::chrono::steady_clock::now();
            duration2 = std::chrono::duration<double, std::milli>(time2 - time1).count();
        }
#endif

        std::cout << duration1 << '\t' << duration2 << '\n';
    }

    std::cout << nothing << '\n';
}
