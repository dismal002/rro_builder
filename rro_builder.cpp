#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <cstdlib>
#include <sstream>
#include <algorithm>

namespace fs = std::filesystem;

struct OverlayConfig {
    std::string target_package = "android";
    int priority = 50;
    bool is_static = false;
    std::string rro_package = "com.privset.frameworkresoverlay";
    std::map<std::string, bool> bool_overrides;
    std::map<std::string, int> int_overrides;
    std::map<std::string, std::string> string_overrides;
    bool use_aapt2 = true;
};

bool ask_yes_no(const std::string& question) {
    while (true) {
        std::cout << question << " (y/n): ";
        std::string reply;
        std::cin >> reply;
        std::transform(reply.begin(), reply.end(), reply.begin(), ::tolower);
        if (reply == "y" || reply == "yes") return true;
        if (reply == "n" || reply == "no") return false;
    }
}

void open_editor(const std::string& rtype, std::map<std::string, std::string>& results) {
    std::string temp_filename = "rro_edit_" + rtype + ".txt";
    std::ofstream ofs(temp_filename);
    ofs << "# Edit " << rtype << " resources below." << std::endl;
    ofs << "# Format: name = value" << std::endl;
    ofs << "# Example: config_showNavigationBar = true" << std::endl;
    ofs << "#" << std::endl;
    ofs.close();

    const char* editor = std::getenv("EDITOR");
    if (!editor) editor = "nano";

    std::string cmd = std::string(editor) + " " + temp_filename;
    std::system(cmd.c_str());

    std::ifstream ifs(temp_filename);
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty() || line[0] == '#') continue;
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string name = line.substr(0, pos);
            std::string val = line.substr(pos + 1);
            // Trim whitespace
            name.erase(name.find_last_not_of(" \n\r\t") + 1);
            name.erase(0, name.find_first_not_of(" \n\r\t"));
            val.erase(val.find_last_not_of(" \n\r\t") + 1);
            val.erase(0, val.find_first_not_of(" \n\r\t"));
            if (!name.empty()) results[name] = val;
        }
    }
    ifs.close();
    fs::remove(temp_filename);
}

void run_cmd(const std::string& cmd) {
    std::cout << "Executing: " << cmd << std::endl;
    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "Error: Command failed with code " << ret << std::endl;
        exit(1);
    }
}

int main() {
    OverlayConfig config;

    std::cout << "\n--- RRO Builder C++ Interactive Setup ---" << std::endl;

    std::map<std::string, std::string> bool_raw, int_raw, string_raw;

    if (ask_yes_no("Do you want to add any boolean values?")) {
        open_editor("bool", bool_raw);
        for (auto const& [name, val] : bool_raw) {
            std::string v = val;
            std::transform(v.begin(), v.end(), v.begin(), ::tolower);
            config.bool_overrides[name] = (v == "true" || v == "1" || v == "yes");
        }
    }

    if (ask_yes_no("Do you want to add any integer values?")) {
        open_editor("integer", int_raw);
        for (auto const& [name, val] : int_raw) {
            try {
                config.int_overrides[name] = std::stoi(val);
            } catch (...) {
                std::cerr << "Warning: Invalid integer for " << name << ": " << val << std::endl;
            }
        }
    }

    if (ask_yes_no("Do you want to add any string values?")) {
        open_editor("string", string_raw);
        for (auto const& [name, val] : string_raw) {
            config.string_overrides[name] = val;
        }
    }

    std::cout << "\nChoose Target Android Version (Compiler):" << std::endl;
    std::cout << "1) Marshmallow/Nougat (Use aapt)" << std::endl;
    std::cout << "2) Oreo/Pie/10+ (Use aapt2)" << std::endl;
    std::cout << "Choice [1-2]: ";
    int choice;
    std::cin >> choice;
    config.use_aapt2 = (choice == 2);

    if (config.use_aapt2) {
        if (ask_yes_no("Do you want this to be a STATIC overlay? (Oreo+ only)")) {
            config.is_static = true;
        }
    }

    // Workspace initialization
    fs::path workspace = fs::current_path() / "build_temp";
    fs::create_directories(workspace / "res/values");

    // Generate Manifest
    std::ofstream manifest(workspace / "AndroidManifest.xml");
    manifest << "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"no\"?>" << std::endl;
    manifest << "<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\"" << std::endl;
    manifest << "    package=\"" << config.rro_package << "\"" << std::endl;
    manifest << "    android:versionName=\"1.0\">" << std::endl;
    manifest << "    <overlay android:targetPackage=\"" << config.target_package << "\"" << std::endl;
    manifest << "             android:priority=\"" << config.priority << "\"";
    if (config.use_aapt2) {
        manifest << "\n             android:isStatic=\"" << (config.is_static ? "true" : "false") << "\"";
    }
    manifest << "/>" << std::endl;
    manifest << "    <application android:label=\"" << config.rro_package << "\" android:hasCode=\"false\">" << std::endl;
    manifest << "    </application>" << std::endl;
    manifest << "</manifest>" << std::endl;
    manifest.close();

    // Generate Resources
    std::ofstream res(workspace / "res/values/config.xml");
    res << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
    res << "<resources xmlns:xliff=\"urn:oasis:names:tc:xliff:document:1.2\">" << std::endl;
    for (auto const& [name, val] : config.bool_overrides) {
        res << "    <bool name=\"" << name << "\">" << (val ? "true" : "false") << "</bool>" << std::endl;
    }
    for (auto const& [name, val] : config.int_overrides) {
        res << "    <integer name=\"" << name << "\">" << val << "</integer>" << std::endl;
    }
    for (auto const& [name, val] : config.string_overrides) {
        res << "    <string name=\"" << name << "\">" << val << "</string>" << std::endl;
    }
    res << "</resources>" << std::endl;
    res.close();

    // Compile & Link
    std::cout << "Building APK..." << std::endl;
    
    std::string framework_res = "framework-res.apk";
    if (!fs::exists(framework_res)) {
        std::cout << "framework-res.apk not found locally. Pulling from device..." << std::endl;
        run_cmd("adb pull /system/framework/framework-res.apk " + framework_res);
    }

    if (config.use_aapt2) {
        // AAPT2 path
        run_cmd("aapt2 compile --dir " + (workspace / "res").string() + " -o " + (workspace / "res_compiled.zip").string());
        run_cmd("aapt2 link -I " + framework_res + " --manifest " + (workspace / "AndroidManifest.xml").string() + " -o " + (workspace / "unsigned.apk").string() + " " + (workspace / "res_compiled.zip").string());
    } else {
        // Legacy AAPT path
        std::string cmd = "aapt package -f -M " + (workspace / "AndroidManifest.xml").string() + 
                          " -S " + (workspace / "res").string() + 
                          " -I " + framework_res + 
                          " -F " + (workspace / "unsigned.apk").string();
        run_cmd(cmd);
    }

    // Align & Sign
    fs::create_directories("dist");
    run_cmd("zipalign -f 4 " + (workspace / "unsigned.apk").string() + " dist/overlay-aligned.apk");
    run_cmd("apksigner sign --key debug.pk8 --cert debug.x509.pem --out dist/overlay.apk dist/overlay-aligned.apk");

    std::cout << "\nSuccessfully built: dist/overlay.apk" << std::endl;

    if (ask_yes_no("Do you want to deploy to a connected device via ADB?")) {
        std::string pkg = config.rro_package;
        run_cmd("adb shell su -c \"mkdir -p /system/app/frameworkresoverlay\"");
        run_cmd("adb push dist/overlay.apk /data/local/tmp/overlay.apk");
        run_cmd("adb shell su -c \"mv /data/local/tmp/overlay.apk /system/app/frameworkresoverlay/frameworkresoverlay.apk\"");
        run_cmd("adb shell su -c \"chmod 644 /system/app/frameworkresoverlay/frameworkresoverlay.apk\"");
        run_cmd("adb shell su -c \"cmd overlay enable " + pkg + "\"");
        std::cout << "Deployment successful." << std::endl;
    }

    // Cleanup
    fs::remove_all(workspace);

    return 0;
}
