#include <iostream>
#include <vector>

class RegisterFile {
public:
    int read(int i) const;
    void write(int i, int value);
    void dump() const;

private:
    std::vector<int> reg_file = std::vector<int>(32, 0);

};


int RegisterFile::read(int i) const
{
    return reg_file.at(i);
}

void RegisterFile::write(int i, int value)
{
    if(i == 0)
    {
        return;
    }
    reg_file.at(i) = value;
}

void RegisterFile::dump() const
{
    for(std::size_t i = 0; i < reg_file.size(); i++)
    {
        std::cout << "r" << i << " : " << reg_file[i] << "\n";
    }
}


int main() {
    RegisterFile rf;
    rf.write(0,5);
    std::cout << "read(0) : " << rf.read(0) << "\n";
    rf.write(8,7);
    std::cout << "read(8) : " << rf.read(8) << "\n";
    rf.dump();
    rf.read(40); //termiante called
}