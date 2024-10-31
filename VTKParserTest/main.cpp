#include <VTKParser.h>

int main(int argc, char** argv) {
    auto data = VTKParser::from_file("data/redseasmall.vtk");
    return 0;
}
