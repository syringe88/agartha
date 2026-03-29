#pragma once
#include <Windows.h>
#include <iostream>
#include <cstdint>
#include <intrin.h>
#include <filesystem>
#include <fstream>
#include <array>
#include <map>

void patches()
{
	//username
	memset(reinterpret_cast<void*>(0x35D8CB24), 0x90, 3);
	std::string name = "ZXC1";
	memcpy(reinterpret_cast<void*>(0x35D8CB24), name.c_str(), name.size());
	//watermark cheat name
	memset(reinterpret_cast<void*>(0x35D13AE0), 0x90, 3);
	std::string watermark = "Ex3zenBEST";
	memcpy(reinterpret_cast<void*>(0x35D13AE0), watermark.c_str(), watermark.size());
	
}