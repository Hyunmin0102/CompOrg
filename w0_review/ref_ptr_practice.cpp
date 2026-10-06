#include <iostream>
using namespace std;
int add(int a, int b)
{
    int x = a+b;
    a=20; // pass by value, so this change will not affect the original variable
    return x;
}

int subtract(int &a, int b)
{
    int x = a-b;
    a=20; // pass by reference, so this change will affect the original variable
    return x;
}

int multiply(int* a, int b)
{
    int x = (*a)*b;
    *a=10; // pass by pointer, so this change will affect the original variable
    return x;
}

int main()
{
    // Practice 1 - reference and pointer basic
    // int x = 10;
    // int &ref = x; // ref is a reference to x
    // int* ptr = &x; // ptr is a pointer to x
    // // int* ptr2 ;
    // // cout << ptr2 << endl; // value of ptr2, I expect this to be a random address
    // // *ptr2=5;
    // cout << "Value of x: " << x << endl; // value of x, I expect this to be 10
    // cout << "Value of ref : " << ref << endl; // value of ref, I expect this to be 10
    // cout << "Value of ptr : " << *ptr << endl; // value of ptr, I expect this to be 10

    // cout << "Value of &x: " << &x << endl; // address of x, I expect this to be a random address
    // cout << "Value of &ref: " << &ref << endl; // address of ref, I expect this to be the same as the address of x
    // cout << "Value of ptr: " << ptr << endl; // value of ptr, I expect this to be the same as the adress of x
    // cout << "Value of &ptr: " << &ptr << endl; // address of ptr, I expect this to be a different address from x because ptr is a new variable that is storing the address of x

    // ref = 20;
    // cout << "Value of x after changing ref: " << x << endl; // value of x, I expect this to be 20
    // cout << "Value of *ptr after changing ref: " << *ptr << endl; // value of ptr, I expect this to be 20
    // *ptr = 30;
    // cout << "Value of x after changing ptr: " << x << endl; // value of x, I expect this to be 30
    // cout << "Value of *ptr after changing ptr: " << *ptr << endl; // value of ptr, I expect this to be 30

    // int y = 99;
    // ptr=&y; // ptr is now pointing to y
    // *ptr=0; // y is now 0
    // cout << "Value of y after changing ptr: " << y << endl; // value of y, I expect this to be 0
    // cout << "Value of x after changing ptr: " << x << endl; // value of x, I expect this to be 30

    // ref = y; 
    // cout << "Value of ref after changing ref: " << ref << endl; // value of ref, I expect this to be 0
    // cout << "Value of x after changing ref: " << x << endl; // value of x, I expect this to be 0
    // cout << "Value of &ref after changing ref: " << &ref << endl; // address of ref, I expect this to be the same as the address of x
    // cout << "Value of &x after changing ref: " << &x << endl; // address of x, I expect this to be the same as the address of ref
    // cout << "Value of &y after changing ref: " << &y << endl; // address of y, I expect this to be a different address from x and ref
    // cout << "Value of ptr : " << ptr << endl; // value of ptr, I expect this to be the same as the address of y
    // ref=10;
    // cout << "Value of y after changing ref: " << y << endl; // value of y, I expect this to be 0
    int x=3;
    int r = add(x,5);    
    cout << "Value of x after calling add: " << x << endl; // value
    cout << "Value of r after calling add: " << r << endl; // value

    int s = subtract(x,5);

    cout << "Value of x after calling subtract: " << x << endl; // value
    cout << "Value of s after calling subtract: " << s << endl; // value

    int t = multiply(&x,5);
    cout << "Value of x after calling multiply: " << x << endl; // value
    cout << "Value of t after calling multiply: " << t << endl; // value    
    return 0;

}