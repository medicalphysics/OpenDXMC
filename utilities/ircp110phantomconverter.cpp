

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
//convert fram ascii to binary
void convert(const std::string& in, const std::string& out)
{
    std::vector<char> organs;
    std::ifstream input(in);
    if (!input.is_open())
        return;
    int c;
    while (input >> c) {
        organs.push_back(static_cast<unsigned char>(c));
    }
    std::ofstream output(out, std::ios::binary);
    output.write(reinterpret_cast<char*>(organs.data()), sizeof(unsigned char) * organs.size());
    input.close();
    output.close();
    return;

}
//application to convert icrp 110 phantoms from integer ascii representation to binary
int main(int argc, char** argv)
{
    if (argc >= 2) {
        std::string in = argv[1];
        std::string out = argv[2];
        convert(in, out);
    }
    return 1;
}