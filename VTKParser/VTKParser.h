#pragma once

#include <functional>
#include <iostream>
#include <filesystem>
#include <memory>
#include <vector>

namespace fs = std::filesystem;

struct Dimension { int x; int y; int z;};
struct Origin { float x; float y; float z;};
struct Spacing { float x; float y; float z;};

template<typename T>
struct VTKField {
    std::string name;
    Dimension dimension;
    Spacing spacing;
    std::vector<T> data;
    T min, max;

    T &operator()(size_t x, size_t y, size_t z) {
        return data[x + y * dimension.x + z * dimension.x * dimension.y];
    }

    T* ptr()
    {
        return data.data();
    }

    T min_val()
    {
        return min;
    }

    T max_val()
    {
        return max;
    }

    VTKField(std::string& name, Dimension d, Spacing s);
};

template<typename T>
VTKField<T>::VTKField(std::string& a_name, Dimension d, Spacing s)
    : name(a_name), data(d.x * d.y * d.z), dimension(d), spacing(s) {}

struct VTKData {
    std::string version;
    std::string title;
    Dimension dimension;
    Origin origin;
    Spacing spacing;
    std::vector<VTKField<double>> fields;
};

class VTKParser {
public:
    static VTKData from_file(const fs::path& filepath);
private:
    VTKParser(std::unique_ptr<std::istream>);

    VTKData parse();
    
    void parse_header();
    void parse_title();
    void parse_datatype();
    void parse_geomtype();
    void parse_data();
    void parse_field();
    
    bool get_line(std::string& line);
private:
    std::unique_ptr<std::istream> m_is;
    VTKData m_data;
};
