#include <iostream>
#include <vector>
using namespace std;

// int square_val(int n)
// {
//     n=n+1;
//     cout << "Address of n inside square: " << &n << endl; // address of n, I expect this to be a different address from x because n is a new variable that is storing the value of x
//     return n*n;
// }

// int square_ref(int &n)
// {
//     n=n+1;
//     cout << "Address of n inside square: " << &n << endl; // address of n, I expect this to be the same as the address of x because n is a reference to x
//     return n*n;
// }

// int square_ptr(int* &n)
// {
//     *n=*n+1;
//     cout << "Address of n inside square: " << n << endl; // address of n, I expect this to be the same as the address of x because n is a pointer to x
//     int m = (*n)*(*n);
//     n=nullptr; // n is now a null pointer, so it is not pointing to anything
//     return m;
// }

// void test_square()
// {
//     int x1 = 5;
//     int x2 = 5;
//     int x3 = 5;

//     int y = square_val(x1);
//     cout<< "Value of x1 after calling square: " << x1 << endl; // value
//     cout << "Address of x1 in main: " << &x1 << endl; // address of x1

//     int z = square_ref(x2);
//     cout<< "Value of x2 after calling square: " << x2 << endl; // value
//     cout << "Address of x2 in main: " << &x2 << endl; // address of x2
//     int* ptr = &x3;
//     int t = square_ptr(ptr);
//     cout<< "Value of x3 after calling square: " << x3 << endl; // value
//     cout << "Address of x3 in main: " << ptr << "\n"; // address of x3

//     // Eliminate warning: unused variable 'w'
//     cout << y << " " << z << " " << t << "\n";
// }

int sum_by_value(std::vector<int> v)
{
    int sum = 0;
    for (int e : v)
    {
        sum += e;
    }
    v[0]=100;
    return sum; 
}

int sum_by_pointer(std::vector<int>* v)
{
    int sum = 0;
    if(v == nullptr)
    {
        return 0;
    }
    for(int i : (*v))
    {
        sum += i;
    }
    (*v)[0]=100;
    return sum;
}

int sum_by_reference(std::vector<int> &v)
{    int sum = 0;
    for (int e : v)
    {
        sum += e;
    }
    v[0]=100;
    return sum; 
}

int sum_by_cref(const std::vector<int> &v)
{
    // v[0]=100; Compile error : assignment of read-only location

    int sum = 0;
    for (int e : v)
    {
        sum += e;
    }
    return sum; 
}

void test_vector_sum()
{
    std::vector<int> vec = {1,2,3,4,5,6,7};
    std::vector<int> vec2 = vec;
    std::vector<int> vec3 = vec;
    std::vector<int> vec4 = vec;
    


    sum_by_value(vec);
    cout << vec[0] << '\n';  // Guess vec[0] doesn't change
    sum_by_reference(vec2);
    cout << vec2[0] << '\n';  // Guess vec[0] might change
    sum_by_cref(vec3);
    cout << vec3[0] << '\n';  
    sum_by_pointer(&vec4);
    cout << vec4[0] << '\n'; // Guess vec[0] might change


}

int main()
{
    // test_square();
    // test_vector_sum();
    sum_by_value(std::vector<int> {1,2,3});
    // sum_by_reference(std::vector<int> {1,2,3});
    sum_by_cref(std::vector<int> {1,2,3});
    std::vector<int> vec = {1,2,3};
    sum_by_pointer(&vec);
    sum_by_pointer(nullptr);

    return 0;
}

/*
 * ===== 세션 2 정리: 함수 인자 전달 방식 =====
 *
 * [값 T]
 *   - 복사 비용 : 있음 (대상 전체를 새 상자에 복사)
 *   - 원본 수정 : 불가능
 *   - null      : 불가능
 *   - 임시 객체 : 가능
 *   - 언제      : 작은 타입(int, double, bool, 포인터)을 읽기만 할 때,
 *                 또는 함수 안에서 어차피 복사본을 고쳐 써야 할 때
 *
 * [포인터 T*]
 *   - 복사 비용 : 거의 없음 (주소 8바이트만 복사, 대상은 복사 안 됨)
 *   - 원본 수정 : 가능 (*p로 쪽지를 따라가서)
 *   - null      : 가능 (대신 함수 안에서 nullptr 검사 필수, 안 하면 segfault)
 *   - 임시 객체 : 불가능 (이름 없는 임시 상자의 주소는 꺼낼 수 없음)
 *   - 언제      : 대상이 없을 수도 있을 때, 가리키는 대상을 나중에 바꿔야 할 때
 *   - 주의      : 포인터 자체도 값으로 복사됨 -> 함수 안에서 p = nullptr 해도
 *                 호출한 쪽 포인터는 그대로 (바꾸려면 T*& 또는 T**)
 *
 * [참조 T&]
 *   - 복사 비용 : 없음 (원본 상자에 이름표만 하나 더 붙임)
 *   - 원본 수정 : 가능
 *   - null      : 불가능 (참조는 항상 실제 상자에 붙어 있어야 함)
 *   - 임시 객체 : 불가능 (바꿔도 아무도 결과를 못 보므로 컴파일러가 막음)
 *   - 언제      : 함수가 호출한 쪽의 객체를 바꿔야 할 때 (swap 등)
 *
 * [const 참조 const T&]
 *   - 복사 비용 : 없음
 *   - 원본 수정 : 불가능 (error: assignment of read-only location)
 *   - null      : 불가능
 *   - 임시 객체 : 가능 (읽기만 하므로 허용, 함수가 끝날 때까지 임시 객체 유지)
 *   - 언제      : 큰 객체(vector, string 등)를 읽기만 할 때 -> 큰 객체 매개변수의 기본값
 *
 * [한 줄 규칙]
 *   - 큰 객체를 읽기만       -> const T&
 *   - 인자를 바꿔야          -> T&
 *   - 없을 수도 있음         -> T* + nullptr 검사
 *   - 작은 타입을 읽기만     -> T
 */