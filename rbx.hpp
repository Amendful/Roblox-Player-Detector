#pragma once

#include <Windows.h>
#include <TlHelp32.h>
#include <vector>

#include "offsets.hpp"

namespace RBX
{
	namespace Memory
	{

		HANDLE handle{ nullptr };
		int PID{ 0 };

		bool attach()
		{
			HANDLE hSnapshot{ CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0) };
			if (hSnapshot == INVALID_HANDLE_VALUE)
			{
				return false;
			}

			PROCESSENTRY32W pe;
			pe.dwSize = sizeof(PROCESSENTRY32W);

			bool found{ false };
			if (Process32FirstW(hSnapshot, &pe))
			{
				do
				{
					if (std::wstring(pe.szExeFile) == L"RobloxPlayerBeta.exe")
					{
						PID = pe.th32ProcessID;
						found = true;

						break;
					}
				} while (Process32NextW(hSnapshot, &pe));
			}

			CloseHandle(hSnapshot);

			if (!found)
			{
				return false;
			}

			handle = OpenProcess(PROCESS_ALL_ACCESS, false, PID);
			return handle != nullptr;
		}

		void detach()
		{
			PID = 0x000000;

			if (handle != nullptr)
			{
				CloseHandle(handle);
				handle = nullptr;
			}
		}

		template <typename T>
		T read(void* addr)
		{
			T val{};
			SIZE_T len;

			ReadProcessMemory(handle, addr, &val, sizeof(T), &len);

			return val;
		}

		template <typename T>
		bool write(void* addr, T val)
		{
			SIZE_T written;

			return WriteProcessMemory(handle, addr, &val, sizeof(T), &written) && written == sizeof(T);
		}

		std::string readStr(void* addr)
		{
			int strLenght{ Memory::read<int>((void*)((uintptr_t)addr + Offsets::StringLength)) };

			if (strLenght >= 16)
			{
				addr = Memory::read<void*>((void*)((uintptr_t)addr));
			}

			std::vector<char> buffer(256);
			SIZE_T bytesRead{ 0 };

			ReadProcessMemory(handle, addr, buffer.data(), strLenght, &bytesRead);

			size_t len{ 0 };
			while (len < bytesRead && buffer[len] != '\0')
				len++;

			return std::string(buffer.data(), len);
		}

		void* getRobloxBaseAddr()
		{
			HANDLE hSnapshot{ CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, PID) };
			if (hSnapshot == INVALID_HANDLE_VALUE)
			{
				return nullptr;
			}

			MODULEENTRY32W me;
			me.dwSize = sizeof(MODULEENTRY32W);

			void* baseAddr{ nullptr };
			if (Module32FirstW(hSnapshot, &me))
			{
				do
				{
					if (std::wstring(me.szModule) == L"RobloxPlayerBeta.exe")
					{
						baseAddr = me.modBaseAddr;

						break;
					}
				} while (Module32NextW(hSnapshot, &me));
			}

			CloseHandle(hSnapshot);

			return baseAddr;
		}

	};

	struct Vector2
	{
		float x{ 0 };
		float y{ 0 };
	};

	struct Vector3
	{
		float x{ 0 };
		float y{ 0 };
		float z{ 0 };
	};

	struct Vector4
	{
		float x{ 0 };
		float y{ 0 };
		float z{ 0 };
		float w{ 0 };
	};

	struct Matrix3
	{
		float data[9];
	};

	struct Matrix4
	{
		float data[16];
	};

	class Instance
	{
	public:
		void* address;

		Instance(void* addr) : address(addr) {}

		std::string name()
		{
			void* ptr{ Memory::read<void*>((void*)((uintptr_t)address + Offsets::Name)) };

			return (ptr != nullptr) ? Memory::readStr(ptr) : std::string();
		}

		std::string className()
		{
			void* classDesc{ Memory::read<void*>((void*)((uintptr_t)address + Offsets::ClassDescriptor)) };
			if (classDesc == nullptr)
			{
				return std::string();
			}

			void* namePtr{ Memory::read<void*>((void*)((uintptr_t)classDesc + Offsets::ClassDescriptorToClassName)) };
			if (namePtr == nullptr)
			{
				return std::string();
			}

			return Memory::readStr(namePtr);
		}

		Instance parent()
		{
			return Instance(Memory::read<void*>((void*)((uintptr_t)address + Offsets::Parent)));
		}

		std::vector<Instance> getChildren()
		{
			std::vector<Instance> children;

			uintptr_t start{ Memory::read<uintptr_t>((void*)((uintptr_t)address + Offsets::Children)) };
			uintptr_t end{ Memory::read<uintptr_t>((void*)(start + Offsets::ChildrenEnd)) };

			for (uintptr_t ptr{ Memory::read<uintptr_t>((void*)start) }; ptr != end; ptr += 0x10)
			{
				uintptr_t childAddr{ Memory::read<uintptr_t>((void*)ptr) };

				children.emplace_back((void*)childAddr);
			}

			return children;
		}

		Instance findFirstChild(const std::string& name)
		{
			std::vector<Instance> children{ getChildren() };

			for (Instance& child : children)
			{
				if (child.name() == name)
				{
					return child;
				}
			}

			return Instance(nullptr);
		}

		Instance findFirstChildOfClass(const std::string& className)
		{
			std::vector<Instance> children{ getChildren() };

			for (Instance& child : children)
			{
				if (child.className() == className)
				{
					return child;
				}
			}

			return Instance(nullptr);
		}

		Instance waitForChild(const std::string& name)
		{
			while (true)
			{
				std::vector<Instance> children{ getChildren() };

				for (Instance& child : children)
				{
					if (child.name() == name)
					{
						return child;
					}
				}
			}
		}

		void* getPrimitive()
		{
			void* primitive{ RBX::Memory::read<void*>((void*)((uintptr_t)address + Offsets::Primitive)) };

			return primitive;
		}

		Vector3 getPosition()
		{
			void* primitive{ getPrimitive() };

			Vector3 position{ RBX::Memory::read<Vector3>((void*)((uintptr_t)primitive + Offsets::Position)) };
			return position;
		}

		RBX::Instance getModelInstance()
		{
			RBX::Instance mi{ RBX::Instance(RBX::Memory::read<void*>((void*)((uintptr_t)address + Offsets::ModelInstance))) };

			return mi;
		}

		float getDistance(const Vector3& pos)
		{
			Vector3 currentPos{ getPosition() };

			float dx{ currentPos.x - pos.x };
			float dy{ currentPos.y - pos.y };
			float dz{ currentPos.z - pos.z };

			return sqrtf(dx * dx + dy * dy + dz * dz);
		}
	};

	class VisualEngine : public Instance
	{
	public:
		VisualEngine(void* addr) : Instance(addr) {}

		Matrix4 getViewMatrix()
		{
			return Memory::read<Matrix4>((void*)((uintptr_t)address + Offsets::viewmatrix));
		}

		Vector2 worldToScreen(const Vector3& world)
		{
			Vector4 quaternion;

			Vector2 dimensions{ GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) }; // roblox window needs to be maximized or fullscreened
			Matrix4 viewMatrix{ getViewMatrix() };

			quaternion.x = (world.x * viewMatrix.data[0]) + (world.y * viewMatrix.data[1]) + (world.z * viewMatrix.data[2]) + viewMatrix.data[3];
			quaternion.y = (world.x * viewMatrix.data[4]) + (world.y * viewMatrix.data[5]) + (world.z * viewMatrix.data[6]) + viewMatrix.data[7];
			quaternion.z = (world.x * viewMatrix.data[8]) + (world.y * viewMatrix.data[9]) + (world.z * viewMatrix.data[10]) + viewMatrix.data[11];
			quaternion.w = (world.x * viewMatrix.data[12]) + (world.y * viewMatrix.data[13]) + (world.z * viewMatrix.data[14]) + viewMatrix.data[15];

			Vector2 screen;

			if (quaternion.w < 0.1f)
			{
				return screen;
			}

			Vector3 normalizedDeviceCoordinates;
			normalizedDeviceCoordinates.x = quaternion.x / quaternion.w;
			normalizedDeviceCoordinates.y = quaternion.y / quaternion.w;
			normalizedDeviceCoordinates.z = quaternion.z / quaternion.w;

			screen.x = (dimensions.x / 2.0f * normalizedDeviceCoordinates.x) + (dimensions.x / 2.0f);
			screen.y = -(dimensions.y / 2.0f * normalizedDeviceCoordinates.y) + (dimensions.y / 2.0f);

			return screen;
		}
	};

	void* getDataModel()
	{
		void* fakeDM{ Memory::read<void*>((void*)((uintptr_t)Memory::getRobloxBaseAddr() + Offsets::FakeDataModelPointer)) };

		return Memory::read<void*>((void*)((uintptr_t)fakeDM + Offsets::FakeDataModelToDataModel));
	}

	void setWalkSpeed(Instance humanoid, float speed)
	{
		Memory::write((void*)((uintptr_t)humanoid.address + Offsets::WalkSpeed), speed);
		Memory::write((void*)((uintptr_t)humanoid.address + Offsets::WalkSpeedCheck), speed);
	}

	void setJumpPower(Instance humanoid, float power)
	{
		Memory::write((void*)((uintptr_t)humanoid.address + Offsets::JumpPower), power);
	}

	void setHealth(Instance humanoid, int health)
	{
		Memory::write((void*)((uintptr_t)humanoid.address + Offsets::Health), health);
	}

};
