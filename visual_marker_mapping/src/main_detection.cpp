#include <ceres/ceres.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include "visual_marker_mapping/TagDetector.h"
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "visual_marker_mapping/CameraUtilities.h"
#include "visual_marker_mapping/TagReconstructor.h"
#include "visual_marker_mapping/DetectionIO.h"

//------------------------------------------------------------------------------------------------------------
template <typename T>
void checkRange(
    T param, std::string paramName, T lowerBound = 0, T upperBound = std::numeric_limits<T>::max())
{
    if (param < lowerBound)
        throw std::runtime_error(
            "Value for " + paramName + " is smaller than " + std::to_string(lowerBound) + " .");
    if (param > upperBound)
        throw std::runtime_error("Value for parameter " + paramName + " is bigger than "
            + std::to_string(upperBound) + " .");
}
//------------------------------------------------------------------------------------------------------------
namespace po = boost::program_options;
po::variables_map loadParameters(int argc, char* argv[])
{
    po::options_description options("Allowed options");
    po::options_description fileOptions("Options for the config file");
    // clang-format off
    options.add_options()("help,?", "produces this help message")
        ("project_path", po::value<std::string>()->required(), "Path to project to be processed")
        ("marker_type", po::value<std::string>()->default_value("apriltag_36h11"), "Marker type to be used. "
         " Options are: apriltag_16h5, apriltag_25h7, apriltag_25h9, apriltag_36h9, apriltag_36h11")
        ("do_corner_refinement", po::bool_switch()->default_value(false),
         "If this option is set an additional corner refinement of the marker "
        "corners is done.")
        ("marker_width", po::value<double>()->default_value(0.1285)->notifier([](int param)
            {
                checkRange<double>(param, "marker_width", 0.0);
            }),
         "Width of an tag measured parallel to the tag name.")
        ("marker_height", po::value<double>()->default_value(0.1285)->notifier([](int param)
            {
                checkRange<double>(param, "marker_height", 0.0);
            }),
         "Height of an tag measured perpendicular to the tag name.");
    // clang-format on
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, options), vm);
    if (vm.count("help"))
    {
        std::cout << options << std::endl;
        std::cout << fileOptions << std::endl;
        exit(0);
    }

    po::notify(vm);

    return vm;
}
//------------------------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    try
    {
        const auto start_time = std::chrono::system_clock::now();

        boost::program_options::variables_map vm = loadParameters(argc, argv);

        const boost::filesystem::path project_path = vm["project_path"].as<std::string>();
        const std::string detection_result_filename
            = (project_path / "marker_detections.json").string();
        const std::string img_path = (project_path / "images").string();
        const std::string marker_detection_path = (project_path / "marker_detections").string();

        if (boost::filesystem::exists(detection_result_filename))
        {
            while (1)
            {
                std::cerr << "Output file '" << detection_result_filename
                          << "' already exists. Overwrite? (y/n) ";
                char yn;
                std::cin >> yn;
                if (yn == 'n')
                {
                    std::cout << "Exiting!" << std::endl;
                    exit(1);
                }
                else if (yn == 'y')
                    break;
            }
        }


        double marker_width = vm["marker_width"].as<double>();
        double marker_height = vm["marker_height"].as<double>();
        const std::string marker_type = vm["marker_type"].as<std::string>();
        const auto detection_result = visual_marker_mapping::detectTags(img_path, marker_width,
            marker_height, marker_type, vm["do_corner_refinement"].as<bool>());
        visual_marker_mapping::writeDetectionResult(detection_result, detection_result_filename);

        std::cout << "Wrote " << detection_result_filename << "!" << std::endl;

        std::cout << "Writing visualization results to " << marker_detection_path << " ..."
                  << std::endl;

        visual_marker_mapping::visualizeDetectionResults(detection_result, marker_detection_path);


        const auto end_time = std::chrono::system_clock::now();
        const std::chrono::duration<double> elapsed_seconds = end_time - start_time;
        std::cout << "Marker detection completed in " << elapsed_seconds.count() << " s"
                  << std::endl;
    }
    catch (const std::exception& ex)
    {
        std::cout << "An exception occurred: " << ex.what() << std::endl;
    }
    return 0;
}
