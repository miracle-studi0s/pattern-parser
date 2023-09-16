// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"
#include <thread>
#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <assert.h>

HMODULE get_module_recursive(const char* name) {
    HMODULE out{};
    while (true) {
        out = GetModuleHandleA(name);
        if (out)
            break;

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return out;
}

namespace console {
    FILE* stream = NULL;

    void create() {
        AllocConsole();
        freopen_s(&stream, ("CONIN$"), ("r"), stdin);
        freopen_s(&stream, ("CONOUT$"), ("w"), stdout);
        freopen_s(&stream, ("CONOUT$"), ("w"), stderr);
    }

    void destroy() {
        HWND console = GetConsoleWindow();
        FreeConsole();
        PostMessage(console, WM_CLOSE, 0, 0);
        fclose(stream);
    }
}

namespace cheat_modules {
    HMODULE client{};
    HMODULE server{};
    HMODULE engine{};
    HMODULE materialsystem{};
    HMODULE tier0{};
    HMODULE shaderapidx9{};
    HMODULE vgui2{};
    HMODULE inputsystem{};
    HMODULE vstdlib{};
    HMODULE localize{};
    HMODULE datacache{};
    HMODULE studiorender{};
    HMODULE vguimatsurface{};
    HMODULE vphysics{};
    HMODULE gameoverlayrenderer{};
    HMODULE ntdll{};
    HMODULE filesystem_stdio{};

    void update_modules_handles() {
        client              = get_module_recursive(("client.dll"));
        server              = get_module_recursive(("server.dll"));
        engine              = get_module_recursive(("engine.dll"));
        materialsystem      = get_module_recursive(("materialsystem.dll"));
        tier0               = get_module_recursive(("tier0.dll"));
        shaderapidx9        = get_module_recursive(("shaderapidx9.dll"));
        vgui2               = get_module_recursive(("vgui2.dll"));
        inputsystem         = get_module_recursive(("inputsystem.dll"));
        vstdlib             = get_module_recursive(("vstdlib.dll"));
        localize            = get_module_recursive(("localize.dll"));
        datacache           = get_module_recursive(("datacache.dll"));
        studiorender        = get_module_recursive(("studiorender.dll"));
        vguimatsurface      = get_module_recursive(("vguimatsurface.dll"));
        vphysics            = get_module_recursive(("vphysics.dll"));
        ntdll               = get_module_recursive(("ntdll.dll"));
        filesystem_stdio    = get_module_recursive(("filesystem_stdio.dll"));
        gameoverlayrenderer = get_module_recursive(("gameoverlayrenderer.dll"));
    }
}

namespace memory {
    using pattern_byte_t = std::pair<std::uint8_t, bool>;

    struct address_t {
        std::uintptr_t pointer{};

        constexpr address_t() : pointer(0) {}
        constexpr address_t(const std::uintptr_t& ptr) : pointer(ptr) {}
        constexpr address_t(const void* ptr) : pointer((std::uintptr_t)ptr) {}

        template <typename T = address_t>
        T cast() {
            if (pointer == 0)
                return {};

            return reinterpret_cast<T>(pointer);
        }

        template <typename T = address_t>
        T add(const std::size_t& amt) {
            if (pointer == 0)
                return {};

            return static_cast<T>(pointer + amt);
        }

        template <typename T = address_t>
        T sub(const std::size_t& amt) {
            if (pointer == 0)
                return {};

            return static_cast<T>(pointer - amt);
        }

        template <typename T = address_t>
        T deref() {
            if (pointer == 0)
                return {};

            return *reinterpret_cast<T*>(pointer);
        }

        template <typename T = address_t>
        T relative(const std::size_t& amt) {
            if (!pointer)
                return {};

            auto out = pointer + amt;
            auto r = *reinterpret_cast<std::uint32_t*>(out);

            if (!r)
                return {};

            out = (out + 4) + r;

            return (T)out;
        }

        template <typename T = address_t>
        T at(std::size_t n) {
            return *reinterpret_cast<T*>(pointer + n);
        }

        template <typename T = address_t>
        T get(std::size_t n = 1) {
            std::uintptr_t out{};

            if (!pointer)
                return T{ };

            out = pointer;

            for (std::size_t i = n; i > 0; --i) {
                if (!valid(out))
                    return T{ };

                out = *reinterpret_cast<std::uintptr_t*>(out);
            }

            return (T)out;
        }

        static bool valid(std::uintptr_t addr) {
            MEMORY_BASIC_INFORMATION mbi{};

            if (!addr)
                return false;

            if (!VirtualQuery((const void*)addr, &mbi, sizeof(mbi)))
                return false;

            if ((mbi.Protect & PAGE_NOACCESS) || (mbi.Protect & PAGE_GUARD))
                return false;

            return true;
        }
    };

    std::vector<int> pattern_to_byte(const char* pattern) {
        auto bytes = std::vector<int>{};
        auto start = const_cast<char*>(pattern);
        auto end = const_cast<char*>(pattern) + strlen(pattern);

        for (auto current = start; current < end; ++current) {
            if (*current == '?') {
                ++current;
                if (*current == '?')
                    ++current;
                bytes.push_back(-1);
            }
            else {
                bytes.push_back(strtoul(current, &current, 16));
            }
        }
        return bytes;
    }

    std::uint8_t* find_pattern_original(HMODULE module, const char* pat) {
        auto dosHeader = (PIMAGE_DOS_HEADER)module;
        auto ntHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)module + dosHeader->e_lfanew);

        auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
        auto patternBytes = pattern_to_byte(pat);
        auto scanBytes = reinterpret_cast<std::uint8_t*>(module);

        auto s = patternBytes.size();
        auto d = patternBytes.data();

        for (auto i = 0ul; i < sizeOfImage - s; ++i) {
            bool found = true;
            for (auto j = 0ul; j < s; ++j) {
                if (scanBytes[i + j] != d[j] && d[j] != -1) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return &scanBytes[i];
            }
        }

        return {};
    }

    address_t find_pattern(HMODULE module, const char* pat) {
        // to get the address without base dll addr just sub address of dll handle lolll
        // thx to @izke
        return { reinterpret_cast<std::uintptr_t>(find_pattern_original(module, pat) - reinterpret_cast<std::uintptr_t>(module)) };
    }
}

namespace output_writer {
    void print_pattern(std::ofstream& stream, const char* module_name, const char* addr_name, HMODULE module, const char* pat, int offset = 0, bool relative = false) {
        auto address = memory::find_pattern(module, pat).add(offset);

        if (address.pointer == 0)
            MessageBoxA(0, "Pizdec", 0, 0);

       //assert(address.pointer > 0);

        stream << std::endl;
        stream << "/*" << module_name << "*/" << std::endl;
     
        if (offset)
            stream << "/*" << pat << " + " << offset << "*/" << std::endl;
        else
            stream << "/*" << pat << "*/" << std::endl;

        if (relative)
            stream << "/*" << "relative" << "*/" << std::endl;

        stream << "OFFSET_" << addr_name << " = " << "0x" << std::hex << std::uppercase << address.pointer << "," << std::endl;
    }

    char* get_current_time() {
        auto clock = std::chrono::system_clock::now();
        std::time_t current_time = std::chrono::system_clock::to_time_t(clock);

        char buf[256]{};
        ctime_s(buf, 256, &current_time);

        return buf;
    }

    void get_patterns_to_file() {
        std::ofstream output{"output.hpp"};

        output << "// offsets were parsed at " << get_current_time() << std::endl;
        output << "enum offsets_t : unsigned int {" << std::endl;
        {
            print_pattern(output, "shaderapidx9.dll", "D3D_DEVICE", cheat_modules::shaderapidx9, "A1 ? ? ? ? 50 8B 08 FF 51 0C", 1, false);
            print_pattern(output, "client.dll", "STUDIO_HDR", cheat_modules::client, "8B B6 ? ? ? ? 85 F6 74 05 83 3E 00 75 02 33 F6 F3 0F 10 44 24", 2, false);
            print_pattern(output, "client.dll", "RETURN_ADDR_MAINTAIN_SEQ_TRANSITIONS", cheat_modules::client, "84 C0 74 ? 8B 87 ? ? ? ? 89 87", 0, false);
            print_pattern(output, "client.dll", "UPDATE_CLIENT_SIDE_ANIMATION", cheat_modules::client, "55 8B EC 51 56 8B F1 80 BE ?? ?? 00 00 00 74 36 8B 06 FF 90 ?? ?? 00 00", 0, false);
            print_pattern(output, "client.dll", "WORLD_TO_SCREEN_MATRIX", cheat_modules::client, "0F 10 05 ? ? ? ? 8D 85 ? ? ? ? B9", 0, false);
            print_pattern(output, "client.dll", "GET_NAME", cheat_modules::client, "56 8B 35 ? ? ? ? 85 F6 74 21 8B 41", 0, false);
            print_pattern(output, "client.dll", "LOAD_FROM_BUFFER", cheat_modules::client, "55 8B EC 83 E4 F8 83 EC 34 53 8B 5D 0C 89", 0, false);
            print_pattern(output, "materialsystem.dll", "GET_COLOR_MODULATION", cheat_modules::materialsystem, "55 8B EC 83 EC ? 56 8B F1 8A 46", 0, false);
            print_pattern(output, "client.dll", "DRAW_MODELS", cheat_modules::client, "55 8B EC 83 E4 F8 51 8B 45 18", 0, false);
            print_pattern(output, "engine.dll", "LOAD_NAMED_SKY", cheat_modules::engine, "55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45", 0, false);
            print_pattern(output, "client.dll", "DISABLE_POST_PROCESS", cheat_modules::client, "80 3D ? ? ? ? ? 53 56 57 0F 85", 2, false);
            print_pattern(output, "client.dll", "POST_PROCESS_DATA", cheat_modules::client, "0F 11 05 ? ? ? ? 0F 10 87", 3, false);

            print_pattern(output, "client.dll", "RETURN_ADDR_DRIFT_PITCH", cheat_modules::client, "84 C0 75 0B 8B 0D ? ? ? ? 8B 01 FF 50 4C", 0, false);
            print_pattern(output, "client.dll", "RETURN_ADDR_APPLY_SHAKE", cheat_modules::client, "84 C0 75 24 A1 ? ? ? ? B9 ? ? ? ? FF 50 1C A1 ? ? ? ? 51 C7 04 24 ? ? 80 3F B9 ? ? ? ? 53 57 FF 50 20", 0, false);
            print_pattern(output, "client.dll", "REMOVE_SMOKE", cheat_modules::client, "55 8B EC 83 EC 08 8B 15 ? ? ? ? 0F 57 C0", 8, false);
            print_pattern(output, "client.dll", "PREDICTION_RANDOM_SEED", cheat_modules::client, "A3 ? ? ? ? 66 0F 6E 86", 1, false);
            print_pattern(output, "client.dll", "PREDICTION_PLAYER", cheat_modules::client, "89 35 ? ? ? ? F3 0F 10 48", 2, false);
            print_pattern(output, "client.dll", "ADD_VIEW_MODEL_BOB", cheat_modules::client, "55 8B EC A1 ? ? ? ? 83 EC 20 8B 40 34", 0, false);
            print_pattern(output, "client.dll", "CALC_VIEW_MODEL_BOB", cheat_modules::client, "55 8B EC A1 ? ? ? ? 83 EC 10 56 8B F1 B9", 0, false);
            print_pattern(output, "client.dll", "CREATE_ANIMSTATE", cheat_modules::client, "55 8B EC 56 8B F1 B9 ? ? ? ? C7 46", 0, false);
            print_pattern(output, "client.dll", "RESET_ANIMSTATE", cheat_modules::client, "56 6A 01 68 ? ? ? ? 8B F1", 0, false);
            print_pattern(output, "client.dll", "UPDATE_ANIMSTATE", cheat_modules::client, "55 8B EC 83 E4 F8 83 EC 18 56 57 8B F9 F3 0F 11 54 24", 0, false);
            print_pattern(output, "client.dll", "SET_ABS_ANGLES", cheat_modules::client, "55 8B EC 83 E4 F8 83 EC 64 53 56 57 8B F1 E8", 0, false);
            print_pattern(output, "client.dll", "SET_ABS_ORIGIN", cheat_modules::client, "55 8B EC 83 E4 F8 51 53 56 57 8B F1 E8", 0, false);

            print_pattern(output, "client.dll", "SHOULD_SKIP_ANIM_FRAME", cheat_modules::client, "57 8B F9 8B 07 8B 80 ? ? ? ? FF D0 84 C0 75 02", 0, false);
            print_pattern(output, "client.dll", "SETUP_BONES", cheat_modules::client, "55 8B EC 83 E4 F0 B8 D8", 0, false);
            print_pattern(output, "client.dll", "STANDARD_BLENDING_RULES", cheat_modules::client, "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 8B 75 08 57 8B F9 85 F6", 0, false);
            print_pattern(output, "client.dll", "DO_EXTRA_BONE_PROCESSING", cheat_modules::client, "55 8B EC 83 E4 F8 81 EC ? ? ? ? 53 56 8B F1 57 89 74 24 1C", 0, false);
            print_pattern(output, "client.dll", "RETURN_ADDR_SETUP_VELOCITY", cheat_modules::client, "84 C0 75 38 8B 0D ? ? ? ? 8B 01 8B 80", 0, false);
            print_pattern(output, "client.dll", "RETURN_ADDR_ACCUMULATE_LAYERS", cheat_modules::client, "84 C0 75 0D F6 87", 0, false);
            print_pattern(output, "client.dll", "RETURN_ADDR_REEVALUATE_ANIM_LOD", cheat_modules::client, "84 C0 0F 85 ? ? ? ? A1 ? ? ? ? 8B B7", 0, false);
            print_pattern(output, "client.dll", "RETURN_ADDR_EXTRAPOLATION", cheat_modules::client, "FF D0 A1 ? ? ? ? B9 ? ? ? ? D9 1D ? ? ? ? FF 50 34 85 C0 74 22 8B 0D ? ? ? ?", 0x29, false);
            print_pattern(output, "client.dll", "RETURN_ADDR_SETUP_BONES", cheat_modules::client, "84 C0 0F 84 ? ? ? ? 8B 44 24 24", 0, false);
            print_pattern(output, "client.dll", "INVALIDATE_BONE_CACHE", cheat_modules::client, "80 3D ? ? ? ? ? 74 16 A1 ? ? ? ? 48 C7 81", 0, false);
            print_pattern(output, "client.dll", "ATTACHMENTS_HELPER", cheat_modules::client, "55 8B EC 83 EC 48 53 8B 5D 08 89 4D F4", 0, false);

            print_pattern(output, "client.dll", "STUDIO_HDR_PTR", cheat_modules::client, "8B B7 ? ? ? ? 89 74 24 20", 0, false);
            print_pattern(output, "client.dll", "INVALIDATE_PHYSICS_RECURSIVE", cheat_modules::client, "55 8B EC 83 E4 F8 83 EC 0C 53 8B 5D 08 8B C3 56 83 E0 04", 0, false);
            print_pattern(output, "client.dll", "TRACE_FILTER", cheat_modules::client, "55 8B EC 83 E4 F0 83 EC 7C 56 52", 0, false);
            print_pattern(output, "client.dll", "TRACE_FILTER_SKIP_ENTITIES", cheat_modules::client, "C7 45 ? ? ? ? ? 89 45 E4 8B 01", 0, false);
            print_pattern(output, "client.dll", "WRITE_USER_CMD", cheat_modules::client, "55 8B EC 83 E4 F8 51 53 56 8B D9", 0, false);
            print_pattern(output, "client.dll", "LOOKUP_BONE", cheat_modules::client, "55 8B EC 53 56 8B F1 57 83 BE ? ? ? ? ? 75", 0, false);
            print_pattern(output, "client.dll", "GLOW_OBJECT", cheat_modules::client, "0F 11 05 ? ? ? ? 83 C8 01 C7 05 ? ? ? ? ? ? ? ?", 3, false);
            print_pattern(output, "server.dll", "DRAW_SERVER_HITBOX", cheat_modules::server, "55 8B EC 81 EC ? ? ? ? 53 56 8B 35 ? ? ? ? 8B D9 57 8B CE", 0, false);
            print_pattern(output, "server.dll", "GET_SERVER_EDICT", cheat_modules::server, "8B 15 ? ? ? ? 33 C9 83 7A 18 01", 0, false);
            print_pattern(output, "client.dll", "MODEL_RENDERABLE_ANIMATING", cheat_modules::client, "E8 ? ? ? ? 85 C0 75 04 33 C0 5E C3 83", 0, true);
            print_pattern(output, "client.dll", "SETUP_CLR_MODULATION", cheat_modules::client, "55 8B EC 83 7D 08 ? 7E", 0, false);
            print_pattern(output, "client.dll", "SET_COLLISION_BOUNDS", cheat_modules::client, "E8 ? ? ? ? 0F BF 87", 0, true);
            print_pattern(output, "client.dll", "FIND_HUD_ELEMENT", cheat_modules::client, "55 8B EC 53 8B 5D 08 56 57 8B F9 33 F6 39 77 28", 0, false);

            print_pattern(output, "engine.dll", "PROCESS_PACKET", cheat_modules::engine, "55 8B EC 83 E4 C0 81 EC ? ? ? ? 53 56 57 8B 7D 08 8B D9", 0, false);
            print_pattern(output, "gameoverlayrenderer.dll", "DIRECT_PRESENT", cheat_modules::gameoverlayrenderer, "FF 15 ? ? ? ? 8B F0 85 FF", 2, false);
            print_pattern(output, "gameoverlayrenderer.dll", "DIRECT_RESET", cheat_modules::gameoverlayrenderer, "C7 45 ? ? ? ? ? FF 15 ? ? ? ? 8B D8", 9, false);
            print_pattern(output, "shaderapidx9.dll", "DIRECT_DEVICE", cheat_modules::shaderapidx9, "A1 ? ? ? ? 50 8B 08 FF 51 0C", 1, false);
            print_pattern(output, "client.dll", "LOCAL_PLAYER", cheat_modules::client, "8B 0D ? ? ? ? 83 FF FF 74 07", 2, false);
            print_pattern(output, "client.dll", "SCREEN_MATRIX", cheat_modules::client, "0F 10 05 ? ? ? ? 8D 85 ? ? ? ? B9", 0, false);
            print_pattern(output, "engine.dll", "READ_PACKETS", cheat_modules::engine, "53 8A D9 8B 0D ? ? ? ? 56 57 8B B9", 0, false);
            print_pattern(output, "client.dll", "ADD_RENDERABLE", cheat_modules::client, "55 8B EC 56 8B 75 08 57 FF 75 18", 0, false);
            print_pattern(output, "client.dll", "PERFORM_SCREEN_OVERLAY", cheat_modules::client, "55 8B EC 51 A1 ? ? ? ? 53 56 8B D9", 0, false);
            print_pattern(output, "client.dll", "RENDER_GLOW_BOXES", cheat_modules::client, "53 8B DC 83 EC ? 83 E4 ? 83 C4 ? 55 8B 6B ? 89 6C ? ? 8B EC 83 EC ? 56 57 8B F9 89 7D ? 8B 4F", 0, false);
            print_pattern(output, "client.dll", "CLIP_RAY_TO_HITBOX", cheat_modules::client, "55 8B EC 83 E4 F8 F3 0F 10 42", 0, false);
            print_pattern(output, "engine.dll", "READ_PACKETS_RETURN_ADDR", cheat_modules::engine, "85 C0 0F 95 C0 84 C0 75 0C", 0, false);
            print_pattern(output, "client.dll", "COMPUTE_HITBOX_SURROUNDING_BOX", cheat_modules::client, "55 8B EC 83 E4 F8 81 EC 24 04 00 00 ? ? ? ? ? ?", 0, false);

            print_pattern(output, "engine.dll", "CLANTAG", cheat_modules::engine, "53 56 57 8B DA 8B F9 FF 15", 0, false);
            print_pattern(output, "client.dll", "IS_BREAKABLE_ENTITY", cheat_modules::client, "55 8B EC 51 56 8B F1 85 F6 74 68", 0, false);
            print_pattern(output, "client.dll", "PROCESS_INTERPOLATED_LIST", cheat_modules::client, "53 0F B7 1D ? ? ? ? 56", 0, false);
            print_pattern(output, "client.dll", "ALLOW_EXTRAPOLATION", cheat_modules::client, "A2 ? ? ? ? 8B 45 E8", 0, false);
            print_pattern(output, "client.dll", "GET_EXPOSURE_RANGE", cheat_modules::client, "55 8B EC 51 80 3D ? ? ? ? ? 0F 57", 0, false);
            print_pattern(output, "engine.dll", "TEMP_ENTITIES", cheat_modules::engine, "55 8B EC 83 E4 F8 83 EC 4C A1 ? ? ? ? 80", 0, false);
            print_pattern(output, "client.dll", "CALC_SHOTGUN_SPREAD", cheat_modules::client, "E8 ? ? ? ? EB 38 83 EC 08", 0, true);
            print_pattern(output, "client.dll", "LINE_GOES_THROUGH_SMOKE", cheat_modules::client, "55 8B EC 83 EC 08 8B 15 ? ? ? ? 0F 57 C0", 0, false);
            print_pattern(output, "engine.dll", "FILE_SYSTEM_PTR", cheat_modules::engine, "8B 0D ? ? ? ? 83 C1 04 8B 01 FF 37 FF 50 1C 89 47 10", 2, false);
            print_pattern(output, "client.dll", "ITEM_SYSTEM", cheat_modules::client, "A1 ? ? ? ? 85 C0 75 53", 0, false);
            print_pattern(output, "engine.dll", "FORCE_UPDATE", cheat_modules::engine, "A1 ? ? ? ? B9 ? ? ? ? 56 FF 50 14 8B 34 85", 0, false);
            print_pattern(output, "client.dll", "UPDATE_VISIBILITY", cheat_modules::client, "55 8B EC 83 E4 F8 83 EC 24 53 56 57 8B F9 8D B7", 0, false);

            print_pattern(output, "tier0.dll", "THREAD_ID_ALLOCATED", cheat_modules::tier0, "C6 86 ? ? ? ? ? 83 05 ? ? ? ? ? 5E 75 04 33 C0 87 07", 2, false);
            print_pattern(output, "client.dll", "WEAPON_SHOOTPOS", cheat_modules::client, "55 8B EC 56 8B 75 ? 57 8B F9 56 8B 07 FF 90", 0, false);
            print_pattern(output, "engine.dll", "CONSTRUCT_VOICE_DATA_MESSAGE", cheat_modules::engine, "56 57 8B F9 8D 4F 08 C7 07 ? ? ? ? E8 ? ? ? ? C7", 0, false);
            print_pattern(output, "client.dll", "PUT_ATTACHMENTS", cheat_modules::client, "55 8B EC 8B 45 ? 8B D1 83 F8 ? 0F 8C", 0, false);
            print_pattern(output, "client.dll", "CALC_ABSOLUTE_POSITION", cheat_modules::client, "55 8B EC 83 E4 ? 83 EC ? 80 3D ? ? ? ? ? 56 57 8B F9", 0, false);
            print_pattern(output, "client.dll", "EYE_ANGLES", cheat_modules::client, "56 8B F1 85 F6 74 ? 8B 06 8B 80 ? ? ? ? FF D0 84 C0 74 ? 8A 86 ? ? ? ? 84 C0 74 ? 83 3D", 0, false);
            print_pattern(output, "client.dll", "INPUT_PTR", cheat_modules::client, "B9 ? ? ? ? F3 0F 11 04 24 FF 50 10", 1, false);
            print_pattern(output, "engine.dll", "CLIENT_STATE", cheat_modules::engine, "A1 ? ? ? ? 8B 80 ? ? ? ? C3", 1, false);
            print_pattern(output, "client.dll", "MOVE_HELPER", cheat_modules::client, "8B 0D ? ? ? ? 8B 46 08 68", 2, false);
            print_pattern(output, "client.dll", "GLOBAL_VARS", cheat_modules::client, "A1 ? ? ? ? F3 0F 10 40 ? 0F 5A C0 F2 0F 11 04", 1, false);
            print_pattern(output, "client.dll", "WEAPON_SYSTEM", cheat_modules::client, "8B 35 ? ? ? ? FF 10 0F B7 C0", 2, false);
            print_pattern(output, "engine.dll", "ENGINE_PAINT", cheat_modules::engine, "55 8B EC 83 EC 40 53 8B D9 8B 0D ? ? ? ? 89 5D F8", 0, false);
            print_pattern(output, "client.dll", "CLIENT_SIDE_ANIMATION_LIST", cheat_modules::client, "A1 ? ? ? ? F6 44", 1, false);
            print_pattern(output, "client.dll", "INTERPOLATE_SERVER_ENTITIES", cheat_modules::client, "55 8B EC 83 EC 1C 8B 0D ? ? ? ? 53 56", 0, false);

            print_pattern(output, "client.dll", "GET_POSE_PARAMETER", cheat_modules::client, "55 8B EC 8B 45 08 57 8B F9 8B 4F 04 85 C9 75 15 8B", 0, false);
            print_pattern(output, "client.dll", "GET_BONE_MERGE", cheat_modules::client, "89 86 ? ? ? ? E8 ? ? ? ? FF 75 08", 2, false);
            print_pattern(output, "client.dll", "UPDATE_MERGE_CACHE", cheat_modules::client, "E8 ? ? ? ? 83 7E 10 ? 74 64", 0, true);
            print_pattern(output, "client.dll", "COPY_TO_FOLLOW", cheat_modules::client, "E8 ? ? ? ? 8B 87 ? ? ? ? 8D 8C 24 ? ? ? ? 8B 7C 24 18", 0, true);
            print_pattern(output, "client.dll", "COPY_FROM_FOLLOW", cheat_modules::client, "E8 ? ? ? ? F3 0F 10 45 ? 8D 84 24 ? ? ? ?", 0, true);
            print_pattern(output, "client.dll", "ADD_DEPENDENCIES", cheat_modules::client, "55 8B EC 81 EC BC ? ? ? 53 56 57", 0, false);
            print_pattern(output, "engine.dll", "SEND_MOVE_ADDR", cheat_modules::engine, "B8 ? ? ? ? 3B F0 0F 4F F0 89 5D FC", 0, false);
            print_pattern(output, "filesystem_stdio.dll", "GET_UNVERIFIED_FILE_HASHES", cheat_modules::filesystem_stdio, "55 8B EC 81 C1 ? ? ? ? 5D E9 ? ? ? ? CC 55 8B EC 8B 45 ? 89 41", 0, false);
            print_pattern(output, "client.dll", "CALC_CHASE_CAM_VIEW", cheat_modules::client, "55 8B EC 83 E4 ? 81 EC ? ? ? ? 56 8B F1 57 8B 06", 0, false);
            print_pattern(output, "engine.dll", "START_SOUND_IMMEDIATE", cheat_modules::engine, "E8 ? ? ? ? 3B F8 0F 4F C7", 1, false);

            print_pattern(output, "client.dll", "INDEX_FROM_ANIM_TAG_NAME", cheat_modules::client, "56 57 8B F9 BE ? ? ? ? 0F 1F 80", 0, false);
            print_pattern(output, "client.dll", "SETUP_AIM_MATRIX", cheat_modules::client, "55 8B EC 81 EC ? ? ? ? 53 56 57 8B 3D", 0, false);
            print_pattern(output, "client.dll", "SETUP_MOVEMENT", cheat_modules::client, "55 8B EC 83 E4 ? 81 EC ? ? ? ? 56 57 8B 3D ? ? ? ? 8B F1", 0, false);
            print_pattern(output, "client.dll", "POST_DATA_UPDATE", cheat_modules::client, "55 8B EC 83 E4 ? 83 EC ? 53 56 57 83 CB", 0, false);
            print_pattern(output, "client.dll", "ESTIMATE_ABS_VELOCITY", cheat_modules::client, "55 8B EC 83 E4 ? 83 EC ? 56 8B F1 85 F6 74 ? 8B 06 8B 80 ? ? ? ? FF D0 84 C0 74 ? 8A 86", 0, false);
            print_pattern(output, "client.dll", "NOTIFY_ON_LAYER_CHANGE_WEIGHT", cheat_modules::client, "55 8B EC 8B 45 ? 85 C0 74 ? 80 B9 ? ? ? ? ? 74 ? 56 8B B1 ? ? ? ? 85 F6 74 ? 8D 4D ? 51 50 8B CE E8 ? ? ? ? 84 C0 74 ? 83 7D ? ? 75 ? F3 0F 10 45 ? F3 0F 11 86 ? ? ? ? 5E 5D C2 ? ? CC CC CC CC CC CC CC CC CC CC 55 8B EC 8B 45", 0, false);
            print_pattern(output, "client.dll", "NOTIFY_ON_LAYER_CHANGE_SEQUENCE", cheat_modules::client, "55 8B EC 8B 45 ? 85 C0 74 ? 80 B9 ? ? ? ? ? 74 ? 8B 89", 0, false);
            print_pattern(output, "client.dll", "ACCUMULATE_LAYERS", cheat_modules::client, "55 8B EC 57 8B F9 8B 0D ? ? ? ? 8B 01 8B 80", 0, false);
            print_pattern(output, "client.dll", "ON_LATCH_INTERPOLATED_VARIABLES", cheat_modules::client, "55 8B EC 83 EC ? 53 56 8B F1 57 80 BE ? ? ? ? ? 75 ? 8B 06", 0, false);
            print_pattern(output, "client.dll", "INTERPOLATE", cheat_modules::client, "55 8B EC 83 E4 ? 83 EC ? 53 56 8B F1 57 83 BE ? ? ? ? ? 75 ? 8B 46 ? 8D 4E ? FF 50 ? 85 C0 74 ? 8B CE E8 ? ? ? ? 8B 9E", 0, false);
            print_pattern(output, "client.dll", "INTERPOLATE_PLAYER", cheat_modules::client, "55 8B EC 83 EC ? 56 8B F1 83 BE ? ? ? ? ? 0F 85", 0, false);
            print_pattern(output, "client.dll", "RESET_LATCHED", cheat_modules::client, "56 8B F1 57 8B BE ? ? ? ? 85 FF 74 ? 8B CF E8 ? ? ? ? 68", 0, false);

            print_pattern(output, "client.dll", "GLOW_OBJECT_MANAGER", cheat_modules::client, "0F 11 05 ? ? ? ? 83 C8 01", 3, false);
            print_pattern(output, "client.dll", "RETURN_ADDR_LOADOUT_ALLOWED", cheat_modules::client, "84 C0 75 05 B0", 0, false);
            print_pattern(output, "engine.dll", "USING_STATIC_PROP_DEBUG", cheat_modules::engine, "8B 0D ? ? ? ? 81 F9 ? ? ? ? 75 ? F3 0F 10 05 ? ? ? ? 0F 2E 05 ? ? ? ? 8B 15 ? ? ? ? 9F F6 C4 ? 7A ? 39 15 ? ? ? ? 75 ? A1 ? ? ? ? 33 05 ? ? ? ? A9 ? ? ? ? 74 ? 8B 0D ? ? ? ? 85 C9 74 ? 8B 01 68 ? ? ? ? FF 90 ? ? ? ? 8B 15 ? ? ? ? 8B 0D ? ? ? ? 81 F2 ? ? ? ? EB ? 8B 01 FF 50 ? 8B 0D ? ? ? ? 8B D0 83 FA ? 0F 85", 0, false);
            print_pattern(output, "client.dll", "BUILD_TRANSFORMATIONS", cheat_modules::client, "55 8B EC 53 56 57 FF 75 ? 8B 7D", 0, false);
            print_pattern(output, "client.dll", "UPDATE_POSTSCREEN_EFFECTS", cheat_modules::client, "55 8B EC 51 53 56 57 8B F9 8B 4D 04 E8 ? ? ? ? 8B", 0, false);
            print_pattern(output, "client.dll", "GET_SEQUENCE_ACTIVITY", cheat_modules::client, "55 8B EC 53 8B 5D 08 56 8B F1 83", 0, false);
            print_pattern(output, "client.dll", "MODIFY_EYE_POSITION", cheat_modules::client, "55 8B EC 83 E4 F8 83 EC 70 56 57 8B F9 89 7C 24 14 83 7F 60", 0, false);
            print_pattern(output, "client.dll", "INIT_KEY_VALUES", cheat_modules::client, "E8 ? ? ? ? 8B F0 EB 02 33 F6 8B 45 08 8B 78", 1, true);
            print_pattern(output, "engine.dll", "ALLOC_KEY_VALUES_ENGINE", cheat_modules::engine, "85 C0 74 0F 51 6A 00 56 8B C8 E8 ? ? ? ? 8B F0", 0, false);
            print_pattern(output, "client.dll", "VIEW_RENDER", cheat_modules::client, "FF 50 14 E8 ? ? ? ? 5F", -7, false);

            print_pattern(output, "client.dll", "ALLOC_KEY_VALUES_CLIENT", cheat_modules::client, "85 C0 74 10 6A 00 6A 00 56 8B C8 E8 ? ? ? ? 8B F0", 0, false);
            print_pattern(output, "client.dll", "ON_BBOX_CHANGE_CALLBACK", cheat_modules::client, "55 8B EC 8B 45 10 F3 0F 10 81", 0, false);
            print_pattern(output, "client.dll", "WANT_RETICLE_SHOWN", cheat_modules::client, "55 8B EC 53 56 8B F1 8B 4D ? 57 E8", 0, false);
            print_pattern(output, "client.dll", "UPDATE_ALL_VIEWMODEL_ADDONS", cheat_modules::client, "55 8B EC 83 E4 F8 83 EC 2C 53 8B D9 56 57 8B", 0, false);
            print_pattern(output, "client.dll", "GET_VIEWMODEL", cheat_modules::client, "55 8B EC 8B 45 08 53 8B D9 56 8B 84 83 ? ? ? ? 83 F8 FF 74 1A 0F B7 F0", 0, false);
            print_pattern(output, "client.dll", "CALC_VIEW", cheat_modules::client, "55 8B EC 83 EC 14 53 56 57 FF 75 18", 0, false);
            print_pattern(output, "client.dll", "GET_HUD_PTR", cheat_modules::client, "B9 ? ? ? ? E8 ? ? ? ? 8B 5D 08", 1, false);
            print_pattern(output, "client.dll", "SHOULD_DRAW_FOG", cheat_modules::client, "53 56 8B F1 8A DA 8B 0D", 0, false);
            print_pattern(output, "engine.dll", "CL_MOVE", cheat_modules::engine, "55 8B EC 81 EC 64 01 00 00 53 56 8A F9", 0, false);
            print_pattern(output, "client.dll", "CLEAR_KILLFEED", cheat_modules::client, "55 8B EC 83 EC 0C 53 56 8B 71 58", 0, false);
            print_pattern(output, "client.dll", "PHYSICS_SIMULATE", cheat_modules::client, "56 8B F1 8B 8E ? ? ? ? 83 F9 FF 74 23", 0, false);

            print_pattern(output, "client.dll", "CLAMP_BONES_IN_BBOX", cheat_modules::client, "E8 ? ? ? ? 80 BE ? ? ? ? ? 0F 84 ? ? ? ? 83 BE ? ? ? ? ? 0F 84 ? ? ? ? B8", 0, true);
            print_pattern(output, "client.dll", "CLEAR_HUD_WEAPONS", cheat_modules::client, "55 8B EC 51 53 56 8B 75 08 8B D9 57 6B FE 34", 0, false);
            print_pattern(output, "engine.dll", "SEND_DATAGRAM", cheat_modules::engine, "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? 56 57 8B F9 89 7C 24 14", 0, false);
            print_pattern(output, "client.dll", "LIST_LEAVES_IN_BOX", cheat_modules::client, "FF 50 18 89 44 24 14 EB", 3, false);
            print_pattern(output, "client.dll", "CALC_VIEWMODEL_VIEW", cheat_modules::client, "55 8B EC 83 EC 58 56 57", 0, false);
            print_pattern(output, "client.dll", "RETURN_ADDR_CAM_THINK", cheat_modules::client, "85 C0 75 30 38 87", 0, false);
            print_pattern(output, "engine.dll", "SEND_NET_MSG", cheat_modules::engine, "55 8B EC 83 EC 08 56 8B F1 8B 4D 04", 0, false);
            print_pattern(output, "client.dll", "RETURN_ADDR_PROCESS_INPUT", cheat_modules::client, "84 C0 74 ? 8D 8E ? ? ? ? E8 ? ? ? ? B9", 0, false);
            print_pattern(output, "client.dll", "VIEWMODEL_ARM_CFG", cheat_modules::client, "E8 ? ? ? ? 89 87 ? ? ? ? 6A", 0, true);

            print_pattern(output, "client.dll", "TRACE_FILTER_TO_HEAD_COLLISION", cheat_modules::client, "55 8B EC 83 B9 ? ? ? ? ? 75 0F", 0, false);
            print_pattern(output, "client.dll", "MASK_PTR", cheat_modules::client, "FF 35 ? ? ? ? FF 90 ? ? ? ? 8B 8F", 2, false);
            print_pattern(output, "client.dll", "UPDATE_ADDON_MODELS", cheat_modules::client, "55 8B EC 83 EC ? 53 8B D9 8D 45 ? 8B 08", 0, false);
            print_pattern(output, "engine.dll", "HOST_SHUTDOWN", cheat_modules::engine, "55 8B EC 83 E4 ? 56 8B 35", 0, false);
            print_pattern(output, "engine.dll", "DESTRUCT_VOICE_DATA_MESSAGE", cheat_modules::engine, "E8 ? ? ? ? 5E 8B E5 5D C3 CC CC CC CC CC CC CC CC CC CC CC CC 51", 0, true);
            print_pattern(output, "engine.dll", "MSG_VOICE_DATA", cheat_modules::engine, "55 8B EC 83 E4 F8 A1 ? ? ? ? 81 EC 84 01 00", 0, false);
            print_pattern(output, "client.dll", "PHYSICS_RUN_THINK", cheat_modules::client, "55 8B EC 83 EC 10 53 56 57 8B F9 8B 87", 0, false);
            print_pattern(output, "client.dll", "THINK", cheat_modules::client, "55 8B EC 56 57 8B F9 8B B7 ? ? ? ? 8B", 0, false);
            print_pattern(output, "client.dll", "POST_THINK_PHYSICS", cheat_modules::client, "55 8B EC 83 E4 F8 81 EC ? ? ? ? 53 8B D9 56 57 83 BB ? ? ? ? ? 0F 84", 0, false);
            print_pattern(output, "client.dll", "SIMULATE_PLAYER_SIMULATED_ENTITIES", cheat_modules::client, "56 8B F1 57 8B BE ? ? ? ? 83 EF 01 78 74", 0, false);
            print_pattern(output, "client.dll", "GAME_RULES", cheat_modules::client, "A1 ? ? ? ? 85 C0 0F 84 ? ? ? ? 80 B8 ? ? ? ? ? 74 7A", 1, false);
            print_pattern(output, "engine.dll", "RETURN_ADDR_SEND_DATAGRAM_CL_MOVE", cheat_modules::engine, "6A 00 8B 01 FF 90 B8 00 00 00 83 BF 08 01", 0xA, false);
            print_pattern(output, "client.dll", "FIRE_BULLET", cheat_modules::client, "55 8B EC 83 E4 ? 81 EC ? ? ? ? F3 0F 7E 45", 0, false);

            print_pattern(output, "server.dll", "ADD_ACTIVITY_MODIFIER", cheat_modules::server, "55 8B EC 8B 55 08 83 EC 30 56 8B F1 85 D2 0F 84 ? ? ? ? 8D 45 D0 8B C8 2B D1", 0, false);
            print_pattern(output, "client.dll", "GET_WEAPON_PREFIX", cheat_modules::client, "53 56 57 8B F9 33 F6 8B 4F ? 8B 01 FF 90 ? ? ? ? 89 47", 0, false);
            print_pattern(output, "client.dll", "FIND_MAPPING", cheat_modules::client, "55 8B EC 83 E4 ? 81 EC ? ? ? ? 53 56 57 8B F9 8B 17", 0, false);
            print_pattern(output, "client.dll", "SELECT_SEQUENCE_FROM_MODS", cheat_modules::client, "55 8B EC 83 E4 ? 83 EC ? 53 56 8B 75 ? 8B D9 57 89 5C 24 ? 8B 16", 0, false);
            print_pattern(output, "client.dll", "GET_SEQUENCE_DESC", cheat_modules::client, "55 8B EC 56 8B 75 ? 57 8B F9 85 F6 78 ? 8B 47", 0, false);
            print_pattern(output, "client.dll", "LOOKUP_SEQUENCE", cheat_modules::client, "55 8B EC 56 8B F1 83 BE ? ? ? ? ? 75 ? 8B 46 ? 8D 4E ? FF 50 ? 85 C0 74 ? 8B CE E8 ? ? ? ? 8B B6 ? ? ? ? 85 F6 74 ? 83 3E ? 74 ? 8B CE E8 ? ? ? ? 84 C0 74 ? FF 75", 0, false);
            print_pattern(output, "client.dll", "GET_SEQUENCE_LINEAR_MOTION", cheat_modules::client, "55 8B EC 83 EC ? 56 8B F1 57 8B FA 85 F6 75 ? 68", 0, false);
            print_pattern(output, "client.dll", "UPDATE_LAYER_ORDER_PRESET", cheat_modules::client, "55 8B EC 51 53 56 57 8B F9 83 7F ? ? 0F 84", 0, false);

            print_pattern(output, "client.dll", "IK_CONTEXT_CONSTRUCT", cheat_modules::client, "53 8B D9 F6 C3 03 74 0B FF 15 ? ? ? ? 84 C0 74 01 ? C7 83 ? ? ? ? ? ? ? ? 8B CB", 0, false);
            print_pattern(output, "client.dll", "IK_CONTEXT_DESTRUCT", cheat_modules::client, "56 8B F1 57 8D 8E ? ? ? ? E8 ? ? ? ? 8D 8E ? ? ? ? E8 ? ? ? ? 83 BE ? ? ? ? ?", 0, false);
            print_pattern(output, "client.dll", "IK_CONTEXT_INIT", cheat_modules::client, "55 8B EC 83 EC 08 8B 45 08 56 57 8B F9 8D 8F", 0, false);
            print_pattern(output, "client.dll", "IK_CONTEXT_UPDATE_TARGETS", cheat_modules::client, "E8 ? ? ? ? 8B 47 FC 8D 4F FC F3 0F 10 44 24", 0, true);
            print_pattern(output, "client.dll", "IK_CONTEXT_SOLVE_DEPENDENCIES", cheat_modules::client, "E8 ? ? ? ? 8B 44 24 40 8B 4D 0C", 0, true);
            print_pattern(output, "client.dll", "INIT_POSE", cheat_modules::client, "55 8B EC 83 EC 10 53 8B D9 89 55 F8 56 57 89 5D F4 8B 0B 89 4D F0", 0, false);

            print_pattern(output, "client.dll", "ACCUMULATE_POSE", cheat_modules::client, "55 8B EC 83 E4 F0 B8 ? ? ? ? E8 ? ? ? ? A1", 0, false);
            print_pattern(output, "client.dll", "CALC_AUTOPLAY_SEQUENCES", cheat_modules::client, "55 8B EC 83 EC 10 53 56 57 8B 7D 10 8B D9 F3 0F 11 5D", 0, false);
            print_pattern(output, "client.dll", "CALC_BONE_ADJUST", cheat_modules::client, "55 8B EC 83 E4 F8 81 EC ? ? ? ? 8B C1 89 54 24 04 89 44 24 2C 56 57 8B", 0, false);
            print_pattern(output, "client.dll", "NOTE_PREDICTION_ERROR", cheat_modules::client, "55 8B EC 83 EC ? 56 8B F1 8B 06 8B 80 ? ? ? ? FF D0 84 C0 75", 0, false);
            print_pattern(output, "client.dll", "CHECK_MOVING_GROUND", cheat_modules::client, "55 8B EC 83 EC ? 56 8B 75 ? F6 86", 0, false);
            print_pattern(output, "client.dll", "VIEW_RENDER_BEAMS", cheat_modules::client, "B9 ? ? ? ? A1 ? ? ? ? FF 10 A1 ? ? ? ? B9", 1, false);
            print_pattern(output, "client.dll", "RETURN_ADDR_SHOW_RADAR", cheat_modules::client, "84 C0 0F 85 ? ? ? ? 8B CB E8 ? ? ? ? 84 C0 0F 85 ? ? ? ? 83 BF", 0, false);
            print_pattern(output, "client.dll", "CALC_ROAMING_VIEW", cheat_modules::client, "55 8B EC 83 E4 ? 81 EC ? ? ? ? 53 8B D9 8D 4C 24", 0, false);
            print_pattern(output, "client.dll", "RETURN_ADDR_POST_PROCESS", cheat_modules::client, "85 D2 75 ? E8 ? ? ? ? 8B C8", 0, false);

            print_pattern(output, "client.dll", "NOTIFY_ON_LAYER_CHANGE_CYCLE", cheat_modules::client, "55 8B EC 8B 45 ? 85 C0 74 ? 80 B9 ? ? ? ? ? 74 ? 56 8B B1 ? ? ? ? 85 F6 74 ? 8D 4D ? 51 50 8B CE E8 ? ? ? ? 84 C0 74 ? 83 7D ? ? 75 ? F3 0F 10 45 ? F3 0F 11 86 ? ? ? ? 5E 5D C2 ? ? CC CC CC CC CC CC CC CC CC CC 55 8B EC 53", 0, false);
            print_pattern(output, "client.dll", "SETUP_WEAPON_ACTION", cheat_modules::client, "55 8B EC 51 53 56 57 8B F9 8B 77 ? 83 BE", 0, false);
            print_pattern(output, "client.dll", "SETUP_WHOLE_BODY_ACTION", cheat_modules::client, "55 8B EC 83 EC ? 56 57 8B F9 8B 77", 0, false);
            print_pattern(output, "client.dll", "SETUP_FLINCH", cheat_modules::client, "55 8B EC 51 56 8B 71 ? 83 BE ? ? ? ? ? 0F 84 ? ? ? ? 8B B6 ? ? ? ? 81 C6 ? ? ? ? 0F 84 ? ? ? ? F3 0F 10 56 ? 0F 28 C2 E8 ? ? ? ? 0F 57 DB 0F 2F D8 73 ? F3 0F 10 49 ? F3 0F 10 66 ? F3 0F 59 CA F3 0F 10 15", 0, false);
            print_pattern(output, "client.dll", "CACHE_SEQUENCES", cheat_modules::client, "55 8B EC 83 E4 ? 83 EC ? 53 56 8B F1 57 8B 46", 0, false);
        }
        output << std::endl;
        output << "};" << std::endl;
    }

    void run() {
        get_patterns_to_file();
    }
}

HMODULE main_module{};

void main_function() {
    HWND window{};
    HMODULE module{};
    
    while (!window) {
        window = FindWindowA("Valve001", NULL);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    while (!module) {
        module = GetModuleHandleA("serverbrowser.dll");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    cheat_modules::update_modules_handles();
    console::create();
    output_writer::run();

    printf("work has been completed successfuly! \n");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    console::destroy();
    FreeLibraryAndExitThread(main_module, 0);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        main_module = hModule;

        CreateThread(0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(main_function), 0, 0, 0);
        break;
    }
    return TRUE;
}

