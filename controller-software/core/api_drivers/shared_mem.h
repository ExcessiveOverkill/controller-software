#include <windows.h>
#include <iostream>

#pragma once

class shared_mem
{
private:
        const unsigned int data_buffer_size = 512 * 4; // size of shared memory buffer for data (512 32-bit values)
        const unsigned int control_buffer_size = 128 * 4 * 4; // size of shared memory buffer for control info (able to hold 128 API calls)

        LPVOID web_to_controller_data_mem;
        LPVOID web_to_controller_control_mem;
        LPVOID controller_to_web_data_mem;
        LPVOID controller_to_web_control_mem;

        HANDLE web_to_controller_data_map;
        HANDLE web_to_controller_control_map;
        HANDLE controller_to_web_data_map;
        HANDLE controller_to_web_control_map;

        unsigned int create_shared_mem(unsigned int size, LPVOID &mem, HANDLE &mapFile, const LPCSTR name, unsigned int permissions);
        unsigned int open_shared_mem(unsigned int size, LPVOID &mem, HANDLE &mapFile, const LPCSTR name, unsigned int permissions);

public:

    LPVOID get_web_to_controller_data_mem() const { return web_to_controller_data_mem; }
    LPVOID get_web_to_controller_control_mem() const { return web_to_controller_control_mem; }
    LPVOID get_controller_to_web_data_mem() const { return controller_to_web_data_mem; }
    LPVOID get_controller_to_web_control_mem() const { return controller_to_web_control_mem; }

    ~shared_mem();

    unsigned int controller_create_shared_mem();
    unsigned int web_create_shared_mem();

    unsigned int get_data_buffer_size() { return data_buffer_size; }
    unsigned int get_control_buffer_size() { return control_buffer_size; }

    void close_shared_mem();
};

unsigned int shared_mem::open_shared_mem(unsigned int size, LPVOID &mem, HANDLE &mapFile, const LPCSTR name, unsigned int permissions){
    // permissions are: FILE_MAP_READ, FILE_MAP_WRITE, FILE_MAP_ALL_ACCESS
    
    std::wstring wstr(name, name + strlen(name));
    LPCWSTR wname = wstr.c_str();

    mapFile = OpenFileMappingW(
        FILE_MAP_ALL_ACCESS,   // read/write access
        FALSE,                 // do not inherit the name
        wname);           // name of the mapping object

    if (mapFile == NULL) {
        std::cerr << "Could not open file mapping object: " << GetLastError() << std::endl;
        return 1;
    }

    mem = MapViewOfFile(
        mapFile,               // handle to map object
        FILE_MAP_ALL_ACCESS,           // read/write permission
        0,
        0,
        size);

    if (mem == NULL) {
        std::cerr << "Could not map view of file: " << GetLastError() << std::endl;
        CloseHandle(mapFile);
        return 1;
    }
    return 0;
}

unsigned int shared_mem::create_shared_mem(unsigned int size, LPVOID &mem, HANDLE &mapFile, const LPCSTR name, unsigned int permissions) {
    // permissions are: FILE_MAP_READ, FILE_MAP_WRITE, FILE_MAP_ALL_ACCESS

    std::wstring wstr(name, name + strlen(name));
    LPCWSTR wname = wstr.c_str();

    mapFile = CreateFileMappingW(
        INVALID_HANDLE_VALUE,    // Use paging file for shared memory
        NULL,                    // Default security
        PAGE_READWRITE,          // Read/Write access
        0,                       // Maximum object size (high-order DWORD)
        size,                    // Maximum object size (low-order DWORD)
        wname); // Name of the shared memory object

    if (mapFile == NULL) {
        std::cerr << "Could not create file mapping object: " << GetLastError() << std::endl;
        return 1;
    }

    mem = MapViewOfFile(
        mapFile,                // Handle to the map object
        FILE_MAP_ALL_ACCESS,            // Read/write permission
        0,                      // File offset high
        0,                      // File offset low
        size);                  // Number of bytes to map

    if (mem == NULL) {
        std::cerr << "Could not map view of file: " << GetLastError() << std::endl;
        CloseHandle(mapFile);
        return 1;
    }
    return 0;
    
}

unsigned int shared_mem::controller_create_shared_mem(){   // called from the controller side
    if(create_shared_mem(data_buffer_size, web_to_controller_data_mem, web_to_controller_data_map, "web_to_controller_data_mem", FILE_MAP_READ) != 0){
        return 1;
    }
    if(create_shared_mem(control_buffer_size, web_to_controller_control_mem, web_to_controller_control_map, "web_to_controller_control_mem", FILE_MAP_READ) != 0){
        return 1;
    }
    if(create_shared_mem(data_buffer_size, controller_to_web_data_mem, controller_to_web_data_map, "controller_to_web_data_mem", FILE_MAP_WRITE) != 0){
        return 1;
    }
    if(create_shared_mem(control_buffer_size, controller_to_web_control_mem, controller_to_web_control_map, "controller_to_web_control_mem", FILE_MAP_WRITE) != 0){
        return 1;
    }
    return 0;
}

unsigned int shared_mem::web_create_shared_mem(){   // called from the web side
    if(open_shared_mem(data_buffer_size, web_to_controller_data_mem, web_to_controller_data_map, "web_to_controller_data_mem", FILE_MAP_WRITE) != 0){
        return 1;
    }
    if(open_shared_mem(control_buffer_size, web_to_controller_control_mem, web_to_controller_control_map, "web_to_controller_control_mem", FILE_MAP_WRITE) != 0){
        return 1;
    }
    if(open_shared_mem(data_buffer_size, controller_to_web_data_mem, controller_to_web_data_map, "controller_to_web_data_mem", FILE_MAP_READ) != 0){
        return 1;
    }
    if(open_shared_mem(control_buffer_size, controller_to_web_control_mem, controller_to_web_control_map, "controller_to_web_control_mem", FILE_MAP_READ) != 0){
        return 1;
    }
    return 0;
}

void shared_mem::close_shared_mem(){
    UnmapViewOfFile(web_to_controller_data_mem);
    UnmapViewOfFile(web_to_controller_control_mem);
    UnmapViewOfFile(controller_to_web_data_mem);
    UnmapViewOfFile(controller_to_web_control_mem);

    CloseHandle(web_to_controller_data_map);
    CloseHandle(web_to_controller_control_map);
    CloseHandle(controller_to_web_data_map);
    CloseHandle(controller_to_web_control_map);
}

shared_mem::~shared_mem(){
}
