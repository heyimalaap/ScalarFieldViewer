#include "VTKParser.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#define dbg(msg) \
    std::cout << "[DEBUG] " << __FILE__ << ":" << __LINE__ << " (" << __FUNCTION__ << ") " << msg << std::endl;

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define RUNTIME_VERIFY(cond) \
    { if (!(cond)) { throw std::runtime_error("VERIFY FAILED " #cond " [" __FILE__ ":" TOSTRING(__LINE__) "]"); } }

VTKParser::VTKParser(std::unique_ptr<std::istream> is)
    : m_is(std::move(is)) {}

VTKData VTKParser::from_file(const fs::path& filepath)
{
    auto file = std::make_unique<std::ifstream>(filepath);
    if (!file->is_open()) {
        throw std::runtime_error("Failed to open file.");
    }

    VTKParser parser = VTKParser(std::move(file));
    return parser.parse();
}

VTKData VTKParser::parse()
{
    parse_header();
    parse_title();
    parse_datatype();
    parse_geomtype();
    parse_data();
    return m_data;
}

void VTKParser::parse_header()
{
    std::string line;
    if (get_line(line)) {
        std::istringstream iss(line);
        {
            std::string tmp;
            iss >> tmp >> tmp >> tmp >> tmp;
        }
        if (iss >> m_data.version) {
            dbg("VERSION: " << m_data.version);
        } else {
            throw std::runtime_error("No VTK version string found.");
        }
    } else {
        throw std::runtime_error("No VTK version header.");
    }
}

void VTKParser::parse_title()
{
    if (get_line(m_data.title)) {
        dbg("TITLE: " << m_data.title);
    } else {
        throw std::runtime_error("VTK file ended too soon. Could not find title line.");
    }
}

void VTKParser::parse_datatype()
{
    std::string data_type;
    if (get_line(data_type)) {
        dbg("DATA TYPE: " << data_type);
        if (data_type != "ASCII") {
            // TODO: Add support for other legacy VTK file data types
            throw std::runtime_error("Parser only supports ASCII VTK files.");
        }
    } else {
        throw std::runtime_error("VTK file ended too soon. Could not find data type line.");
    }
}

void VTKParser::parse_geomtype()
{
    std::string geom_type;
    if (get_line(geom_type)) {
        std::istringstream iss(geom_type);
        iss >> geom_type;
        iss >> geom_type;
        dbg("GEOMETRY TYPE: " << geom_type);
        if (geom_type != "STRUCTURED_POINTS") {
            // TODO: Add support for other legacy VTK file types
            throw std::runtime_error("Parser only supports STRUCTURED_POINTS geometry type.");
        }

        for (int i = 0; i < 3; i++) {
            std::string line;
            if (get_line(line)) {
                std::istringstream iss(line);
                std::string attrib_name;
                iss >> attrib_name;

                if (attrib_name == "DIMENSIONS") {
                    iss >> m_data.dimension.x >> m_data.dimension.y >> m_data.dimension.z;
                    dbg("DIMENSIONS: [" << m_data.dimension.x << ", " <<  m_data.dimension.y << ", " << m_data.dimension.z << "]");
                } else if (attrib_name == "ORIGIN") {
                    iss >> m_data.origin.x >> m_data.origin.y >> m_data.origin.z;
                    dbg("ORIGIN: [" << m_data.origin.x << ", " <<  m_data.origin.y << ", " << m_data.origin.z << "]");
                } else if (attrib_name == "SPACING") {
                    iss >> m_data.spacing.x >> m_data.spacing.y >> m_data.spacing.z;
                    dbg("SPACING: [" << m_data.spacing.x << ", " <<  m_data.spacing.y << ", " << m_data.spacing.z << "]");
                } else {
                    throw std::runtime_error("Failed parsing geometry type. Unknown geometry type attribute: " + attrib_name);
                }

            } else {
                throw std::runtime_error("Failed parsing geometry type. Could not find any geometry type attributes.");
            }
        }

    } else {
        throw std::runtime_error("VTK file ended too soon. Could not find geometry type line.");
    }
}

void VTKParser::parse_data()
{
    std::string data_type_line;
    if (get_line(data_type_line)) {
        std::istringstream iss(data_type_line);
        std::string data_type;
        long count;
        iss >> data_type >> count;

        if (data_type == "POINT_DATA") {
            RUNTIME_VERIFY(m_data.dimension.x * m_data.dimension.y * m_data.dimension.z == count);
            // TODO: Add support for other types
            parse_field();
        } else {
            // TODO: Support CELL_DATA
            throw std::runtime_error("Parser only supports POINT_DATA.");
        }
    } else {
        throw std::runtime_error("VTK file ended too soon. Expected CELL_DATA or POINT_DATA.");
    }
}

void VTKParser::parse_field()
{
    std::string field_line;
    if (get_line(field_line)) {
        std::istringstream iss(field_line);
        std::string field;
        std::string field_name;
        int field_count;
        iss >> field >> field_name >> field_count;

        if (field == "FIELD") {
            for (int i = 0; i < field_count; i++) {
                double field_min = std::numeric_limits<double>::max();
                double field_max = std::numeric_limits<double>::min();

                std::string line;
                if (get_line(line)) {
                    std::istringstream iss(line);
                    std::string sf_name;
                    int sf_components;
                    long sf_count;
                    std::string sf_type;
                    iss >> sf_name >> sf_components >> sf_count >> sf_type;

                    RUNTIME_VERIFY(sf_components == 1);
                    RUNTIME_VERIFY(m_data.dimension.x * m_data.dimension.y * m_data.dimension.z == sf_count);
                    RUNTIME_VERIFY(sf_type == "double");

                    VTKField<double> new_field(sf_name, m_data.dimension);
                    for (int j = 0; j < sf_count; j++) {
                        std::string data_line;
                        if (!get_line(data_line)) {
                            throw std::runtime_error("Failed to parse the scalar field " + sf_name);
                        }
                        std::istringstream diss(data_line);

                        // TODO: Make this agnostic to the number of values per line
                        for (int p = 0; p < 9; p++) {
                            double val;
                            diss >> val;

                            field_max = std::max(field_max, val);
                            field_min = std::min(field_min, val);

                            new_field.data[j++] = val;
                        }
                        j--;
                    }

                    new_field.min = field_min;
                    new_field.max = field_max;

                    m_data.fields.push_back(new_field);
                } else {
                    throw std::runtime_error("Failed to parse the field " + field_name);
                }
            }
        } else {
            // TODO: Support other types
            throw std::runtime_error("Parser only supports FIELD.");
        }
    } else {
        throw std::runtime_error("VTK file ended too soon. Expected CELL_DATA or POINT_DATA.");
    }
}

bool VTKParser::get_line(std::string& line)
{
    bool ret = false;
    while (line == "" && (ret = !!std::getline(*m_is, line)));
    return ret;
}


