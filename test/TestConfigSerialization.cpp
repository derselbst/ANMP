
#include <iostream>
#include <fstream>
#include <cereal/archives/json.hpp>

#include "Config.h"
#include "Test.h"

using namespace std;

int main()
{
    {
        std::ofstream os("/home/tom/Eigene Programme/ANMP/build/config.json");
        cereal::JSONOutputArchive ar(os);
        
        Config c;
        ar(c);
    }
    /*
    {
        std::ifstream is("config.json");
        cereal::JSONInputArchive archive(is);
        Config::serialize(ar);
    }*/
  
    return 0;
}
