/*
//#include "stdafx.h"
#include <string> // ���������� 2017: � �++ 17 ������ ���� ����������� ������ ����������� ����� ����� �������� �������: std::filesystem. ���� �������� �������� ����� �� Shreevardhan � ���� �������� �����: @ http://qaru.site/questions/17239/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c //UPDATE 2017: @ https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
#include <iostream>
#include <filesystem>
//namespace fs = std::filesystem;
namespace fs = std::experimental::filesystem; //c++17 `filesystem` is not a namespace-name @ https://stackoverflow.com/questions/48312460/c17-filesystem-is-not-a-namespace-name

int main() {
    //std::string path = "path_to_directory";
	std::string path = ".";
    for (auto & p : fs::directory_iterator(path))
        std::cout << p << std::endl;
	 return 0;
}
*/
