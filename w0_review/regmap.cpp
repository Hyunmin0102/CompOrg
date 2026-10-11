#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>

int main()
{
    // vector_practice
    // std::vector <int> v = {};
    // //Add 100 integer to vector reg
    // for(int i=0; i<100; i++)
    // {
    //     v.push_back(i);
    // }
    // // std::cout << "v.at(100) = " << v.at(100) << std::endl;
    // std::cout << "v[100] = " << v[100] << "\n" ;

    //Unordered_map practice
    //Practice 1
    // std::unordered_map <std::string, int> m = {{"a", 1}, {"b", 2}};
    // std::cout << "m.size() == " << m.size() << "\n"; //m.size() = 2;
    // std::cout << "m[\"a\"] = " << m["a"] << "\n"; // m["a"] = 1;
    // std::cout << "m[\"z\"] == " << m["z"] << "\n"; // m["z"] = 0;
    // std::cout << "m.size() after m[\"z\"] = " << m.size() << "\n"; // m.size() = 3;
    // std::cout << "m.count(\"q\") = " << m.count("q") << "\n"; // count = 0;
    // std::cout << "m.size() after m.count(\"q\") = " << m.size() << "\n"; // size = 3;

    // m["a"]=100;
    // m.insert({"a", 99});
    // std::cout << "m[\"a\"] = " << m["a"] << "\n"; // m["a"] = 100;

    //Practice 2
    // std::vector<std::string> trace = {"add", "lw", "add", "beq", "lw", "add", "sw"};
    // std::unordered_map<std::string, int> m = {};
    // for(const auto& j : trace)
    // {
    //     m[j]+=1;
    // }
    // for(const auto& kv : m)
    // {
    //     std::cout << kv.first << " " << kv.second << "\n";
    // }

    //Use unorder_map
    std::unordered_map<std::string, int> m = {};
    m["$zero"] = 0;
    m["$s0"]=16;
    m["$t0"]=8;

    auto it = m.find("$t0"); // it = iterator
    // auto cnt = m.count("$t0");
    // auto val = m["$t0"];
    if(it != m.end())
    {
        std::cout << "m.find(\"$t0\") = " << it->second << "\n";
    }
    else
    {
        std::cout << "not found" << "\n";
    }
    // std::cout << "m.count(\"$t0\") = " << cnt << "\n";
    // std::cout << "m[\"$t0\"] = " << val << "\n";
    auto bog_it = m.find("$bogus");
    if(bog_it != m.end())
    {
        std::cout << "found : " << bog_it->second << "\n";
    }
    else
    {
        std::cout << "not found" << "\n";
    }

    //operater [] 함정
    m.insert({"$sp", 29});
    m.insert({"$ra", 31});
    std::cout << "m.size() = " << m.size() << "\n";
    std::cout << "m[\"$bogus\"] = " << m["$bogus"] << "\n";
    std::cout << "m.size() = " << m.size() << "\n";

    std::cout << "from not practice 5" << "\n";
    for (auto kv : m)        { kv.second += 100; }
    for (auto& kv : m)       { kv.second += 100; }
    for(const auto& kv : m) //kv = pair
    {
        std::cout << kv.first << " " << kv.second << "\n";
    }
    // for (const auto& kv : m) { kv.second += 100; } compile error


}  